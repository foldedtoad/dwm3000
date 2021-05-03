/*! ----------------------------------------------------------------------------
 *  @file    ds_twr_sts_sdc_responder.c
 *  @brief   Double-sided two-way ranging (DS TWR) with STS with SDC (STS-SDC) 
 *           responder example code
 *
 *           This is a simple code example which acts as the responder in a 
 *           DS TWR with STS-SDC distance measurement exchange. This application
 *           waits for a "poll" message (recording the RX time-stamp of the 
 *           poll) expected from the "DS TWR initiator STS-SDC" example code 
 *           (companion to this application), and then sends a "response" 
 *           message recording its TX time-stamp, after which it waits for a 
 *           "final" message from the initiator to complete the exchange. 
 *           The final message contains the remote initiator's time-stamps of 
 *           poll TX, response RX and final TX. With this data and the
 *           local time-stamps, (of poll RX, response TX and final RX), this 
 *           example application works out a value for the time-of-flight 
 *           over-the-air and, thus, the estimated distance between the two 
 *           devices, which it writes to the console.
 *
 *           Note: As STS is used, the receptions are considered valid if and 
 *           only if the STS quality index is good. Then the STS timestamp
 *           is read and used for the TWR range calculation. Please see note 
 *           below on Super Deterministic Code (SDC).
 *
 * @attention
 *
 * Copyright 2017-2020 (c) Decawave Ltd, Dublin, Ireland.
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
#include <shared_functions.h>

//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(simple_tx);

/* Example application name and version to display on LCD screen. */
#define APP_NAME "DSTWR RE STS-SDC v1.0"

/* Default communication configuration. We use STS with SDC DW mode. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PLEN_64,     /* Preamble length. Used in TX only. */
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
    (65 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_1 | DWT_STS_MODE_SDC, /* STS mode 1 with SDC see NOTE on SDC below*/
    DWT_STS_LEN_64,  /* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* have some delay after each range (e.g. so LDC can be updated (on ARM eval 
 * boards), needs to be slightly less than RNG_DELAY_MS in the initiator 
 * example */
#define DELAY_MS 980

/* Default antenna delay values for 64 MHz PRF. See NOTE 1 below. */
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8_t rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21};
static uint8_t tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0};
static uint8_t rx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the function 
 * code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN 10

/* Index to access some of the fields in the frames involved in the process. */
#define ALL_MSG_SN_IDX 2
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18

/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb = 0;

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is 
 * supposed to handle. */
#define RX_BUF_LEN 24
static uint8_t rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be 
 * examined at a debug breakpoint. */
static uint32_t status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from Frame RX timestamp to TX reply timestamp used for 
 * calculating/setting the DW3000's delayed TX function. This includes the
 * frame length of approximately 180 us with above configuration. */
#define POLL_RX_TO_RESP_TX_DLY_UUS 900

/* This is the delay from the end of the frame transmission to the enable of 
 * the receiver, as programmed for the DW3000's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 670

/* Receive final timeout. See NOTE 5 below. */
#define FINAL_RX_TIMEOUT_UUS 300

/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 5

/* Timestamps of frames transmission/reception. */
static uint64_t poll_rx_ts;
static uint64_t resp_tx_ts;
static uint64_t final_rx_ts;

/* Hold copies of computed time of flight and distance here for reference 
 * so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and 
 * power of the spectrum at the current temperature. These values can be 
 * calibrated prior to taking reference measurements. 
 * See NOTE 2 below. */
extern dwt_txconfig_t txconfig_options;

/*! ---------------------------------------------------------------------------
 * @fn ds_twr_sts_sdc_responder()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
int app_main(void)
{
    int range_ok = 0;

    /* Display application name on LCD. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC();

    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Configure DW IC. See NOTE 15 below. */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);


    /* Next can enable TX/RX states output on GPIOs 5 and 6 to help debug, 
     * and also TX/RX LEDs.
     * Note, in real low power applications the LEDs should not be used. */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    /* Loop forever responding to ranging requests. */
    while (1) {

        /* turn off preamble timeout as the responder does not know when the 
         * poll is coming. */
        dwt_setpreambledetecttimeout(0);

        /* Clear reception timeout to start next ranging process. */
        dwt_setrxtimeout(0);

        /* Activate reception immediately. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll for reception of a frame or error/timeout. See NOTE 8 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & 
                                               (SYS_STATUS_RXFCG_BIT_MASK | 
                                                SYS_STATUS_ALL_RX_TO | 
                                                SYS_STATUS_ALL_RX_ERR)))
        { /* spin */ };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

            uint32_t frame_len;
            int16_t stsqual;

            /* Clear good RX frame event in the DW3000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            /* As STS mode is used, we only consider frames that are received 
             * with good STS quality. If STS is good this will be true >= 0*/
            if (dwt_readstsquality(&stsqual)) {

                /* A frame has been received, read it into the local buffer. */
                frame_len = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
                if (frame_len <= RX_BUF_LEN) {
                    dwt_readrxdata(rx_buffer, frame_len, 0);
                }

                /* Check that the frame is a poll sent by "DS TWR initiator" example.
                 * As the sequence number field of the frame is not relevant, 
                 * it is cleared to simplify the validation of the frame. */
                rx_buffer[ALL_MSG_SN_IDX] = 0;
                if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0) {
                    
                    uint32_t resp_tx_time;
                    int ret;

                    /* Retrieve poll reception timestamp. */
                    poll_rx_ts = get_rx_timestamp_u64();

                    /* Set send time for response. See NOTE 9 below. */
                    resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                    dwt_setdelayedtrxtime(resp_tx_time);

                    /* Set expected delay and timeout for final message reception. 
                     * See NOTE 4 and 5 below. */
                    dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
                    dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

                    /* Write and send the response message. See NOTE 10 below.*/
                    tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                    dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
                    dwt_writetxfctrl(sizeof(tx_resp_msg)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging. */
                    
                    /* Set preamble timeout for expected final frame from the 
                     * initiator. See NOTE 6 below. */
                    dwt_setpreambledetecttimeout(PRE_TIMEOUT);
                    ret = dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);

                    /* If dwt_starttx() returns an error, abandon this ranging 
                     * exchange and proceed to the next one. See NOTE 11 below. */
                    if (ret == DWT_ERROR) {
                        continue;
                    }

                    /* Poll for reception of expected "final" frame or 
                     * error/timeout. See NOTE 8 below. */
                    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & 
                                                           (SYS_STATUS_RXFCG_BIT_MASK |
                                                            SYS_STATUS_ALL_RX_TO      |
                                                            SYS_STATUS_ALL_RX_ERR)))
                    { /* spin */ };

                    /* Increment frame sequence number after transmission 
                     * of the response message (modulo 256). */
                    frame_seq_nb++;

                    if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

                        /* Clear good RX frame event and TX frame sent in 
                         * the DW3000 status register. */
                        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | 
                                                         SYS_STATUS_TXFRS_BIT_MASK);

                        /* As STS mode is used, we only consider frames that 
                         * are received with good STS quality.
                         *  If STS is good this will be true >= 0 */
                        if (dwt_readstsquality(&stsqual)) {

                            /* A frame has been received, read it into 
                             * the local buffer. */
                            frame_len = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
                            if (frame_len <= RX_BUF_LEN) {
                                dwt_readrxdata(rx_buffer, frame_len, 0);
                            }

                            /* Check that the frame is a final message sent by 
                             * "DS TWR initiator" example.  As the sequence 
                             * number field of the frame is not used in this 
                             * example, it can be zeroed to ease the validation 
                             * of the frame. */
                            rx_buffer[ALL_MSG_SN_IDX] = 0;
                            if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN) == 0) {

                                uint32_t poll_tx_ts, resp_rx_ts, final_tx_ts;
                                uint32_t poll_rx_ts_32, resp_tx_ts_32, final_rx_ts_32;
                                double Ra, Rb, Da, Db;
                                int64_t tof_dtu;

                                /* Retrieve response transmission and final
                                 * reception timestamps. */
                                resp_tx_ts = get_tx_timestamp_u64();
                                final_rx_ts = get_rx_timestamp_u64();

                                /* Get timestamps embedded in the final message. */
                                final_msg_get_ts(&rx_buffer[FINAL_MSG_POLL_TX_TS_IDX], &poll_tx_ts);
                                final_msg_get_ts(&rx_buffer[FINAL_MSG_RESP_RX_TS_IDX], &resp_rx_ts);
                                final_msg_get_ts(&rx_buffer[FINAL_MSG_FINAL_TX_TS_IDX], &final_tx_ts);

                                /* Compute time of flight. 32-bit subtractions 
                                 * give correct answers even if clock has wrapped. 
                                 * See NOTE 12 below. */
                                poll_rx_ts_32 = (uint32_t)poll_rx_ts;
                                resp_tx_ts_32 = (uint32_t)resp_tx_ts;
                                final_rx_ts_32 = (uint32_t)final_rx_ts;
                                
                                Ra = (double)(resp_rx_ts - poll_tx_ts);
                                Rb = (double)(final_rx_ts_32 - resp_tx_ts_32);
                                Da = (double)(final_tx_ts - resp_rx_ts);
                                Db = (double)(resp_tx_ts_32 - poll_rx_ts_32);
                                
                                tof_dtu = (int64_t)((Ra * Rb - Da * Db) / (Ra + Rb + Da + Db));

                                tof = tof_dtu * DWT_TIME_UNITS;
                                distance = tof * SPEED_OF_LIGHT;

                                /* Display computed distance on LCD. */
                                //LOG_INF(("DIST: %3.2f m", distance);

                                range_ok = 1;
                            }
                        }
                    }
                    else {
                        /* Clear RX error/timeout events in the DW3000 status register. */
                        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
                    }
                }
            }
        }
        else {
            /* Clear RX error/timeout events in the DW3000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | 
                                             SYS_STATUS_ALL_RX_ERR);
        }

        /* add some delay before next ranging exchange */
        if (range_ok) {
            range_ok = 0;
            Sleep(DELAY_MS);
        }
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * Super Deterministic Code (SDC):
 * Since the Ipatov preamble consists of a repeating sequence of the same Ipatov code, the time-of-arrival determined using it is
 * vulnerable to a collision with another packet. If the clock offset between the two packets on the air is low,
 * then the signal from the colliding packet will appear to be just another ray from the desired signal.
 * Depending on when it arrives this can be confused with the first path.
 * The STS uses a continually varying sequence. This means that the colliding packet will not line up with the desired signal.
 * As a result, the TOA will be unaffected.  If security is not a concern, then overhead of key management and counter control
 * is undesirable.  For this reason, the DW3000 includes a special STS mode (SDC) that uses a code optimized for TOA performance.
 * Since it is a time varying sequence optimized for TOA performance it will tolerate packet collisions without requiring any
 * key management.
 * It is important to remember that the SDC mode does not provide security but will increase confidence in the TOA when the
 * on-air packet density is high. For this reason, we would recommend that an SDC based STS is used when security is not a
 * requirement.
 *
 * 1. The sum of the values is the TX to RX antenna delay, experimentally determined by a calibration process. Here we use a hard coded typical value
 *    but, in a real application, each device should have its own antenna delay properly calibrated to get the best possible precision when performing
 *    range measurements.
 * 2. The messages here are similar to those used in the DecaRanging ARM application (shipped with EVK1000 kit). They comply with the IEEE
 *    802.15.4 standard MAC data frame encoding and they are following the ISO/IEC:24730-62:2013 standard. The messages used are:
 *     - a poll message sent by the initiator to trigger the ranging exchange.
 *     - a response message sent by the responder allowing the initiator to go on with the process
 *     - a final message sent by the initiator to complete the exchange and provide all information needed by the responder to compute the
 *       time-of-flight (distance) estimate.
 *    The first 10 bytes of those frame are common and are composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: destination address, see NOTE 3 below.
 *     - byte 7/8: source address, see NOTE 3 below.
 *     - byte 9: function code (specific values to indicate which message it is in the ranging process).
 *    The remaining bytes are specific to each message as follows:
 *    Poll message:
 *     - no more data
 *    Response message:
 *     - byte 10: activity code (0x02 to tell the initiator to go on with the ranging exchange).
 *     - byte 11/12: activity parameter, not used for activity code 0x02.
 *    Final message:
 *     - byte 10 -> 13: poll message transmission timestamp.
 *     - byte 14 -> 17: response message reception timestamp.
 *     - byte 18 -> 21: final message transmission timestamp.
 *    All messages end with a 2-byte checksum automatically set by DW3000.
 * 3. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 4. Delays between frames have been chosen here to ensure proper synchronisation of transmission and reception of the frames between the initiator
 *    and the responder and to ensure a correct accuracy of the computed distance. The user is referred to DecaRanging ARM Source Code Guide for more
 *    details about the timings involved in the ranging process.
 * 5. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive the complete final frame sent by the responder at the
 *    6.81 Mbps data rate used (around 200 us).
 * 6. The preamble timeout allows the receiver to stop listening in situations where preamble is not starting (which might be because the responder is
 *    out of range or did not receive the message to respond to). This saves the power waste of listening for a message that is not coming. We
 *    recommend a minimum preamble timeout of 5 PACs for short range applications and a larger value (e.g. in the range of 50% to 80% of the preamble
 *    length) for more challenging longer range, NLOS or noisy environments.
 * 7. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW3000 OTP memory.
 * 8. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW3000 User Manual for more details on "interrupts". It is also to be noted that STATUS register is 5 bytes long but, as the event we
 *    use are all in the first bytes of the register, we can use the simple dwt_read32bitreg() API call to access it instead of reading the whole 5
 *    bytes.
 * 9. Timestamps and delayed transmission time are both expressed in device time units so we just have to add the desired response delay to poll RX
 *    timestamp to get response transmission time. The delayed transmission time resolution is 512 device time units which means that the lower 9 bits
 *    of the obtained value must be zeroed. This also allows to encode the 40-bit value in a 32-bit words by shifting the all-zero lower 8 bits.
 * 10. dwt_writetxdata() takes the full size of the message as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *     automatically appended by the DW3000. This means that our variable could be two bytes shorter without losing any data (but the sizeof would not
 *     work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 11. When running this example on the DWK3000 platform with the POLL_RX_TO_RESP_TX_DLY response delay provided, the dwt_starttx() is always
 *     successful. However, in cases where the delay is too short (or something else interrupts the code flow), then the dwt_starttx() might be issued
 *     too late for the configured start time. The code below provides an example of how to handle this condition: In this case it abandons the
 *     ranging exchange and simply goes back to awaiting another poll message. If this error handling code was not here, a late dwt_starttx() would
 *     result in the code flow getting stuck waiting subsequent RX event that will will never come. The companion "initiator" example (ex_05a) should
 *     timeout from awaiting the "response" and proceed to send another poll in due course to initiate another ranging exchange.
 * 12. The high order byte of each 40-bit time-stamps is discarded here. This is acceptable as, on each device, those time-stamps are not separated by
 *     more than 2**32 device time units (which is around 67 ms) which means that the calculation of the round-trip delays can be handled by a 32-bit
 *     subtraction.
 * 13. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW3000 API Guide for more details on the DW3000 driver functions.
 * 14. In this example, the DW IC is put into IDLE state after calling dwt_initialise(). This means that a fast SPI rate of up to 38 MHz can be used
 *     thereafter.
 * 15. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *     configuration.
 ****************************************************************************************************************************************************/
