/*! ----------------------------------------------------------------------------
 *  @file    le_pend_tx.c
 *  @brief   Simple example that transmits mac message and wait for ack response from rx.
 *           This example is run in parallel with simple example LE PEND RX.
 *           The receiver will set the data pending bit in the ack frame if the 
 *           follow criteria is met:
 *           1. The frame received is a MAC command frame.
 *           2. The address of the device sending the MAC command frame matches
 *           one of the four 16-bit addresses programmed into LE_PEND01 or
 *           LE_PEND23 registers and the data pending bits are set in FF_CFG
 *           register: LE0_PEND – LE3_PEND.
 *           3.The address of the device sending the MAC command is 16-bits and SSADRAPE bit is set.
 *           4.The address of the device sending the MAC command is 64-bits and LSADRAPE bit is set.
 *           5.Security bit is not set in Frame Control and frame version is 0 or 1
 *           Ack frame can then be check if data pending bit has been set correctly.
 *
 * @attention
 *
 * Copyright 2020 (c) Decawave Ltd, Dublin, Ireland.
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

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_pend_tx);

/* Example application name */
#define APP_NAME "LE PEND TX v1.0"

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
    DWT_STS_MODE_OFF,
    DWT_STS_LEN_64,  /* Cipher length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/*
The frame below is an example of a MAC command frame encoded as per the IEEE 802.15.4-2015 standard.
It is composed of the following fields:
    - byte 0/1: frame control (0x8863 to indicate a MAC command frame using 16-bit addressing and requesting ACK).
      - bits 0-2: Frame Type: 011 - MAC command frame
      - bit 3: Security Enabled: 0 - No security enabled
      - bit 4: Frame Pending: 0 - No additional data for recipient
      - bit 5: AR: 1 - ACK frame required from recipient device on receipt of data frame
      - bit 6: PAN ID Compression: 1 - PAN IDs are identical, Source PAN ID field shall be omitted from transmitted
        frame
      - bit 7: Reserved: 0
      - bit 8: Sequence Number Suppression: 0 - Sequence number field is present
      - bit 9: IE Present: 0 - No IEs contained in frame
      - bits 10-11: Destination Addressing Mode: 10 - Address field contains short address
      - bits 12-13: Frame Version: 00 - Using IEEE Std 802.15.4-2003 frames
      - bits 14-15: Source Addressing Mode: 10 - Include source address in frame
    - byte 2: sequence number, incremented for each new frame.
    - byte 3/4: PAN ID (0xDECA)
    - byte 5/6: destination address, see NOTE 2 below.
    - byte 7/8: source address, see NOTE 2 below.
    - byte 9: Command ID field
    - byte 10/11: frame check-sum, automatically set by DW IC.
 */
static uint8_t mac_frame[] = {0x63, 0x88, 0x00, 0xCA, 0xDE, 'X', 'R', 0x54, 0x58, 0x04, 0x00, 0x00};

#define FRAME_LENGTH    (sizeof(mac_frame)+FCS_LEN) // The real length that is going to be transmitted
#define BLINK_FRAME_SN_IDX 2

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 500

/* Buffer to store received frame. See NOTE 3 below. */
#define ACK_FRAME_LEN   5
static uint8_t rx_buffer[ACK_FRAME_LEN];

/* Application entry point.
 */
int app_main(void)
{
    uint32_t status_reg;

    /* Display application name on LCD. */
    LOG_INF(APP_NAME);

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

    /* Enabling LEDs here for debug so that for each TX the D1 LED will 
    * flash on DW3000 red eval-shield boards. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK) ;

    /* Configure DW IC. See NOTE 4 below. */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Loop forever */
    while (1) {

        /* Configure frame filtering to only allow ACK frames*/
        dwt_configureframefilter(DWT_FF_ENABLE_802_15_4, DWT_FF_ACK_EN);

        /* Write frame data to DW IC and prepare transmission. See NOTE 6 below.*/
         /* Zero offset in TX buffer. */
        dwt_writetxdata(FRAME_LENGTH - FCS_LEN, mac_frame, 0);

        /* Zero offset in TX buffer, no ranging. */
        dwt_writetxfctrl(FRAME_LENGTH, 0, 0); 

        /* Start transmission, indicating that a response is expected so that 
         * reception is enabled automatically after the frame is sent and the delay
         * set by dwt_setrxaftertxdelay() has elapsed. */
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

        /* Poll DW IC until TX frame sent event set. See NOTE 7 below. */
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
        { /* spin */ };

        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        /* We assume that the transmission is achieved correctly, poll for 
         * reception of a frame or error/timeout. See NOTE 8 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | 
                                                                   SYS_STATUS_ALL_RX_TO | 
                                                                   SYS_STATUS_ALL_RX_ERR)))
        { /* spin */ };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {
           uint32_t frame_len;

           /* A frame has been received, check frame length is correct 
            * for ACK, then read and verify the ACK. */
           frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
           if (frame_len == ACK_FRAME_LEN) {
               dwt_readrxdata(rx_buffer, frame_len, 0);
           }
           /* Clear good RX frame event in the DW IC status register. */
           dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
       }
       else {
          /* Clear RX error/timeout events in the DW IC status register. */
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
       }
        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        /* Execute a delay between transmissions. */
        Sleep(TX_DELAY_MS);

        /* Increment the blink frame sequence number (modulo 256). */
        mac_frame[BLINK_FRAME_SN_IDX]++;
    };
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. This example can be turned into a high speed data transfer test by removing the delay executed between two successful transmissions. The
 *    communication configuration and MAC payload size in the message can be changed to see the effect of the different parameters on the throughput
 *    (which can be computed using the different counters provided in the application). For example using the debugger to stop at the start of the
 *    while loop, and then timing from the "GO" for a few minutes before breaking in again, and examining the frame counters.
 * 2. Source and destination addresses are hard coded constants to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW IC unique ID by combining the Lot ID & Part Number values programmed into the DW IC
 *    during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that customers
 *    buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW IC User Manual.
 * 3. In this example, receive buffer is set to the exact size of the only frame we want to handle but 802.15.4z UWB standard maximum frame length is
 *    127 bytes. DW IC also supports an extended frame length (up to 1023 bytes long) mode which is not used in this example..
 * 4. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *     configuration.
 * 5. TX to RX delay is set to 0 to activate reception immediately after transmission, as the companion "LE PEND RX" example is configured to send
 *    the ACK immediately after reception of the frame sent by this example application.
 * 6. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW IC. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 7. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW IC User Manual for more details on "interrupts".
 ****************************************************************************************************************************************************/
