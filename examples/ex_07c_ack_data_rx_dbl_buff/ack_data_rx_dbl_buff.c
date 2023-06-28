/*! ----------------------------------------------------------------------------
 *  @file    ack_data_rx_dbl_buff.c
 *  @brief   Automatically Acknowledged data RX example code with double RX buffer
 *
 *           This is a simple code example that turns on the DW IC receiver to
 *           receive a frame, (expecting the frame as sent by the companion simple
 *           example "ACK DATA TX"). The DW IC is configured so that when a 
 *           correctly addressed data frame is received with the ACK request 
 *           (AR) bit set in the frame control field, the DW IC will automatically
 *           respond with an ACK frame. The code loops after each frame reception
 *           to await another frame.
 *
 * @attention
 *
 * Copyright 2016-2020 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2021 (c) Callender-Consulting, LLC  (port to Zephyr)
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <port.h>
#include <shared_defines.h>
#include <deca_vals.h>
#include <shared_functions.h>

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ack_data_rx_dbl_buff);

/* Example application name and version. */
#define APP_NAME "ACK DATA RX DB v1.0"

typedef enum {
    buff_types_A = 0,
    buff_types_B
} buff_types_e;

void read_received_data(uint8_t *rx_buff, buff_types_e buff_types);
void wait_for_ACK_TX_end_if_needed(uint8_t ack_flag);

/* Default communication configuration. We use default non-STS DW mode. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard 8 symbol SFD, 
                      *   1 to use non-standard 8 symbol, 
                      *   2 for non-standard 16 symbol SFD and 
                      *   3 for 4z 8 symbol SDF type */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    DWT_PHRRATE_STD, /* PHY header rate. */
    (129 + 8 - 8),   /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_OFF, /* STS disabled */
    DWT_STS_LEN_64,  /* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* PAN ID/short address. See NOTE 1*/
#define PAN_ID          0xDECA
#define SHORT_ADDR      0x5258 /* "RX" */

/* Buffer to store received frame. See NOTE 2 below. */
static uint8_t rx_buffer_a[FRAME_LEN_MAX];//First buffer
static uint8_t rx_buffer_b[FRAME_LEN_MAX];//Second buffer

/* ACK request bit mask in DATA and MAC COMMAND frame control's first byte. */
#define FCTRL_ACK_REQ_MASK          0x20/*The mask value to check if ack bit is enabled*/
#define FRAME_CTRL_FIRST_BYTE_IDX   0/*Index of frame ctrl first byte*/


/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth 
 * and power of the spectrum at the current temperature. These values can 
 * be calibrated prior to taking reference measurements. See NOTE 4 below. */
extern dwt_txconfig_t txconfig_options;


/*! ---------------------------------------------------------------------------
* @fn read_received_data()
*
* @brief This reads data from 1 of the 2 RX buffers
*
* input rx_buff - data pointer to write the received data
* input buff_types_e - type of buffer A/B
*
* output - None
*
*/
void read_received_data(uint8_t *rx_buff,buff_types_e buff_types)
{
    /* Hold copy of frame length of frame received (if good) so that it can 
     * be examined at a debug breakpoint. */
    uint16_t frame_len;

    /* A frame has been received, read it into the local buffer. */
    /* Read the right buffer size of A or B */
    if (buff_types == buff_types_A) {
        frame_len = dwt_read16bitoffsetreg(BUF0_RX_FINFO, 0);
    }
    else {   
        /* INDIRECT POINTER B is set up as part of DB configure to point to 
         * the buffer B - BUF1_FINFO. This is done in order to save time. */
        frame_len = dwt_read16bitoffsetreg(INDIRECT_POINTER_B_ID, 0);
    }

    frame_len &= EXT_FRAME_LEN;
    if (frame_len <= FRAME_LEN_MAX) {
        dwt_readrxdata(rx_buff,frame_len,0);
    }
}

/*! ---------------------------------------------------------------------------
* @fn wait_for_ACK_TX_end_if_needed()
*
* @brief This checks it it need to wait to ack transmission to be finished.
*
* input ack_flag - The byte value to check the ack bit
*
* output - None
*
*/
void wait_for_ACK_TX_end_if_needed(uint8_t ack_flag)
{
    /* Since the auto ACK feature is enabled, an ACK should be sent if the 
     * received frame requests it, so we await the ACK TX completion
     * before taking next action. See NOTE 8 below. */
    uint32_t  status;

    if (ack_flag & FCTRL_ACK_REQ_MASK) {

        /* Poll DW IC until confirmation of transmission of the ACK frame. */
        while (!(status=dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
        { };

        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    }
}


/**
 * Application entry point.
 */
int app_main(void)
{
    uint8_t  status;

    /* Display application name. */
    LOG_ERR(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

     /* Reset DW IC */
    reset_DWIC(); 

    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_IDLE) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Configure DW IC. See NOTE 11 below. */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);
    
    /* Set PAN ID and short address. See NOTE 1 below. */
    dwt_setpanid(PAN_ID);
    dwt_setaddress16(SHORT_ADDR);

    /* Configure frame filtering. Only data frames are enabled in this example. 
     * Frame filtering must be enabled for Auto ACK to work. */
    dwt_configureframefilter(DWT_FF_ENABLE_802_15_4, DWT_FF_DATA_EN);
    
    /* Activate auto-acknowledgement. Time is set to 0 so that the ACK is sent
     * as soon as possible after reception of a frame. */
    dwt_enableautoack(50, 1);
    
    /* can enable TX/RX states output on GPIOs 5 and 6 to help debug */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    /* Enable double buff - Manual mode */
    dwt_setdblrxbuffmode(DBL_BUF_STATE_EN,DBL_BUF_MODE_MAN);

    /* Enable diagnostic mode - minimal */
    dwt_configciadiag(DW_CIA_DIAG_LOG_MIN);

    /* Loop forever receiving frames. */
    while (1) {

        /* Activate reception immediately. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received.*/
        do {
            status = dwt_read8bitoffsetreg(RDB_STATUS_ID, 0);

        } while (!(status&RDB_STATUS_RXFCG0_BIT_MASK));

        /* Clear status bits */
        dwt_write8bitoffsetreg(RDB_STATUS_ID, 0, RDB_STATUS_CLEAR_BUFF0_EVENTS);

        read_received_data(rx_buffer_a,buff_types_A);
        wait_for_ACK_TX_end_if_needed(rx_buffer_a[FRAME_CTRL_FIRST_BYTE_IDX]);
        
        /* Release the received buffer */
        dwt_signal_rx_buff_free();

        /* Activate reception immediately. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        do {
            status = dwt_read8bitoffsetreg(RDB_STATUS_ID, 0);

        } while (!(status&RDB_STATUS_RXFCG1_BIT_MASK));

        /* Clear status bits */
        dwt_write8bitoffsetreg(RDB_STATUS_ID, 0, RDB_STATUS_CLEAR_BUFF1_EVENTS);
        
        read_received_data(rx_buffer_b,buff_types_B);
        wait_for_ACK_TX_end_if_needed(rx_buffer_b[FRAME_CTRL_FIRST_BYTE_IDX]);

        /* Release the received buffer */
        dwt_signal_rx_buff_free();
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. PAN ID and short address are hard coded constants to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW IC unique ID by combining the Lot ID & Part Number values programmed into the DW IC
 *    during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that customers
 *    buy a block of addresses from the IEEE Registration Authority for their production items.
 * 2. In this example, maximum frame length is set to 127 bytes which is 802.15.4z UWB standard maximum frame length. DW IC supports an extended frame
 *    length (up to 1023 bytes long) mode which is not used in this example.
 * 3. In this example, the DW IC is put into IDLE state after calling dwt_initialise(). This means that a fast SPI rate of up to 20 MHz can be used
 *     thereafter.
 * 4. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 * 5. Manual reception activation is performed here but DW IC offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 6. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW IC User Manual for more details on "interrupts".
 * 7. This is the purpose of the AAT bit in DW IC's STATUS register but because of an issue with the operation of AAT, it is simpler to directly
 *    check in the frame control if the ACK request bit is set. Please refer to DW IC User Manual for more details on Auto ACK feature and the AAT
 *    bit.
 * 8. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW IC API Guide for more details on the DW IC driver functions.
 * 9. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *    configuration.
 ****************************************************************************************************************************************************/
