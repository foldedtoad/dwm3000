/*! ----------------------------------------------------------------------------
 *  @file    ack_data_tx.c
 *  @brief   Automatically acknowledged data TX example code
 *
 *           This is a simple code example that sends a frame and then turns on
 *           the DW IC receiver to receive a response, expected to be an ACK
 *           frame as sent by the companion simple example "ACK DATA RX" example 
 *           code. After the ACK is received, this application proceeds to the
 *           sending of the next frame (increasing the frame sequence number). 
 *           If the expected valid ACK is not received, the application immediately
 *           retries to send the frame (keeping the same frame sequence number).
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

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ack_data_tx);

/* Example application name and versions. */
#define APP_NAME "ACK DATA TX v1.0"

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

/* The frame sent in this example is a data frame encoded as per the 
 * IEEE 802.15.4-2011 standard. It is a 21-byte frame composed of the 
 * following fields:
 *     - byte 0/1: frame control (0x8861 to indicate a data frame using 16-bit addressing and requesting ACK).
 *       - bits 0-2: Frame Type: 001 - Data frame
 *       - bit 3: Security Enabled: 0 - No security enabled
 *       - bit 4: Frame Pending: 0 - No additional data for recipient
 *       - bit 5: AR: 1 - ACK frame required from recipient device on receipt of data frame
 *       - bit 6: PAN ID Compression: 1 - PAN IDs are identical, Source PAN ID field shall be omitted from transmitted frame
 *       - bit 7: Reserved: 0
 *       - bit 8: Sequence Number Suppression: 0 - Sequence number field is present
 *       - bit 9: IE Present: 1 - IEs contained in frame
 *       - bits 10-11: Destination Addressing Mode: 10 - Address field contains short address
 *       - bits 12-13: Frame Version: 00 - Using IEEE Std 802.15.4-2003 frames
 *       - bits 14-15: Source Addressing Mode: 10 - Include source address in frame
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA)
 *     - byte 5/6: destination address, see NOTE 2 below.
 *     - byte 7/8: source address, see NOTE 2 below.
 *     - byte 9 to 18: MAC payload, see NOTE 1 below.
 *     */
static uint8_t tx_msg[] = {0x61, 0x88, 0, 0xCA, 0xDE, 'X', 'R', 'X', 'T', 'm', 'a', 'c', 'p', 'a', 'y', 'l', 'o', 'a', 'd'};

/* Index to access the sequence number and frame control fields 
 * in frames sent and received. */
#define FRAME_FC_IDX    0
#define FRAME_SN_IDX    2
/* ACK frame control value. */
#define ACK_FC_0        0x02
#define ACK_FC_1        0x00

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

/* Receive response timeout, expressed in UWB microseconds 
 * (UUS, 1 uus = 512/499.2 µs). See NOTE 3 below. */
#define RX_RESP_TO_UUS  2200

/* Buffer to store received frame. See NOTE 4 below. */
#define ACK_FRAME_LEN   5

static uint8_t rx_buffer[ACK_FRAME_LEN];

/* Hold copy of status register state here for reference so that it 
 *can be examined at a debug breakpoint. */
static uint32_t status_reg = 0;


/* ACK status for last transmitted frame. */
static int tx_frame_acked = 0;

/* Counters of frames sent, frames ACKed and frame retransmissions. 
 * See NOTE 1 below. */
static uint32_t tx_frame_nb = 0;
static uint32_t tx_frame_ack_nb = 0;
static uint32_t tx_frame_retry_nb = 0;

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth 
 * and power of the spectrum at the current temperature. 
 * These values can be calibrated prior to taking reference measurements. 
 * See NOTE 2 below. */
extern dwt_txconfig_t txconfig_options;

/**
 * Application entry point.
 */
int app_main(void)
{
    /* Hold copy of frame length of frame received (if good) so that it can be 
     * examined at a debug breakpoint. */
    uint16_t frame_len = 0;
    uint16_t tx_frame_len;

    tx_frame_len = sizeof(tx_msg) + FCS_LEN;

    /* Display application name. */
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

    /* Configure DW IC. See NOTE 11 below. */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Set delay to turn reception on immediately after transmission of the frame. 
     * See NOTE 6 below. */
    dwt_setrxaftertxdelay(50);

    /* Set RX frame timeout for the response. */
    dwt_setrxtimeout(RX_RESP_TO_UUS);

    /* Can enable TX/RX states output on GPIOs 5 and 6 to help debug. */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    /* Loop forever transmitting data. */
    while (1)
    {
        /* Write frame data to DW IC and prepare transmission. 
         * See NOTE 7 below.*/
        dwt_writetxdata(tx_frame_len-FCS_LEN, tx_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(tx_frame_len, 0, 0); /* Zero offset in TX buffer, no ranging. */

        /* Start transmission, indicating that a response is expected so that
         * reception is enabled immediately after the frame is sent. */
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

        /* We assume that the transmission is achieved normally, now poll for 
         * reception of a frame or error/timeout. See NOTE 8 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | 
                                                                   SYS_STATUS_ALL_RX_TO | 
                                                                   SYS_STATUS_ALL_RX_ERR)))
        { /* spin */ };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            /* A frame has been received, check frame length is correct for ACK,
             * then read and verify the ACK. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
            if (frame_len == ACK_FRAME_LEN) {

                dwt_readrxdata(rx_buffer, frame_len, 0);

                /* Check if it is the expected ACK. */
                if ((rx_buffer[FRAME_FC_IDX] == ACK_FC_0)     && 
                    (rx_buffer[FRAME_FC_IDX + 1] == ACK_FC_1) && 
                    (rx_buffer[FRAME_SN_IDX] == tx_msg[FRAME_SN_IDX])) {

                    tx_frame_acked = 1;

                    /* Execute a delay between transmissions. 
                     * See NOTE 1 below. */
                    Sleep(TX_DELAY_MS);

                    /* Increment the sent frame sequence number 
                     * (modulo 256). */
                    tx_msg[FRAME_SN_IDX]++;

                    /* Update number of frames acknowledged. */
                    tx_frame_ack_nb++;
                }
            }
        }
        else {
            /* Clear RX error/timeout events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO |
                                             SYS_STATUS_ALL_RX_ERR);
        }

        /* Update number of frames sent. */
        tx_frame_nb++;

        if (!tx_frame_acked) {

            Sleep(TX_DELAY_MS/5);

            /* Update number of retransmissions. */
            tx_frame_retry_nb++;
        }
        else {
            /* Reset acknowledged frame flag. */
            tx_frame_acked = 0;
        }
    }
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
 * 3. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive a complete ACK frame sent by the "ACK DATA RX" example
 *    at the 110k data rate used (around 2 ms).
 * 4. In this example, receive buffer is set to the exact size of the only frame we want to handle but 802.15.4z UWB standard maximum frame length is
 *    127 bytes. DW IC also supports an extended frame length (up to 1023 bytes long) mode which is not used in this example..
 * 5. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 * 6. TX to RX delay is set to 0 to activate reception immediately after transmission, as the companion "ACK DATA RX" example is configured to send
 *    the ACK immediately after reception of the frame sent by this example application.
 * 7. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW IC. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 8. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW IC User Manual for more details on "interrupts".
 * 9. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW IC API Guide for more details on the DW IC driver functions.
 * 10. In this example, the DW IC is put into IDLE state after calling dwt_initialise(). This means that a fast SPI rate of up to 20 MHz can be used
 *     thereafter.
 * 11. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *     configuration.
 ****************************************************************************************************************************************************/
