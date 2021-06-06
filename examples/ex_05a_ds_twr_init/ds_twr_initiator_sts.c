/*! ----------------------------------------------------------------------------
 *  @file    ds_twr_initiator_sts.c
 *  @brief   Double-sided two-way ranging (DS TWR) initiator example code
 *
 *           This is a simple code example which acts as the initiator in a 
 *           DS TWR distance measurement exchange. This application sends a 
 *           "poll" frame (recording the TX time-stamp of the poll), after 
 *           which it waits for a "response" message from the 
 *           "ds_twr_responder_sts" example code (companion to this application)
 *           to complete the exchange.
 *
 *           This example utilises the 802.15.4z STS to accomplish secure 
 *           timestamps between the initiator and responder. A 32-bit STS 
 *           counter is part of the STS IV used to generate the scrambled 
 *           timestamp sequence (STS) in the transmitted packet and to cross 
 *           correlate in the receiver. 
 *           This count normally advances by 1 for every 1024 chips (~2µs) of 
 *           STS in BPRF mode, and by 1 for every 512 chips (~1µs) of STS
 *           in HPRF mode. If both devices (initiator and responder) have count 
 *           values that are synced, then the communication between devices 
 *           should result in secure timestamps which can be used to calculate 
 *           distance. 
 *           If not, then the devices need to re-sync their STS counter values.
 *           In this example, the initiator will send a plain-text value of 
 *           it's 32-bit STS counter inside the "poll" frame. The receiver first
 *           checks the quality of the STS of the received frame. If the 
 *           received frame has bad STS quality, it can then use the plain-text
 *           counter value received to adjust it's own STS counter value to match.
 *           This means that the next message in the sequence should be in sync 
 *           again.
 *
 * @attention
 *
 * Copyright 2019 - 2020 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2021 (c) Callender-Consulting, LLC  (port to Zephyr)
 *
 * All rights reserved.
 *
 * @author Decawave
 */

#include <stdlib.h>
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <deca_types.h>
#include <port.h>
#include <shared_defines.h>
#include <shared_functions.h>
#include <config_options.h>

//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(ds_twr_init_sts);

/* Example application name */
#define APP_NAME "DS TWR INIT v1.0"

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 1000

/* Default antenna delay values for 64 MHz PRF. See NOTE 2 below. */
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

/* Frames used in the ranging process. See NOTE 3 below. */
static uint8_t tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8_t rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0};
static uint8_t tx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'D', 'E', 'C', 'A', 0xE2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the function code, see NOTE 3 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed 
 * to handle. */
#define RX_BUF_LEN 24
static uint8_t rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be 
 * examined at a debug breakpoint. */
static uint32_t status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 1 below. */
#define POLL_TX_TO_RESP_RX_DLY_UUS (290 + CPU_COMP)

/* This is the delay from Frame RX timestamp to TX reply timestamp used for 
 * calculating/setting the DW IC's delayed TX function. This includes the
 * frame length of approximately 550 us with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS (480 + CPU_COMP)

/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 300

/* Hold the amount of errors that have occurred */
static uint32_t errors[23] = {0};

extern dwt_config_t config_options;
extern dwt_txconfig_t txconfig_options;
extern dwt_txconfig_t txconfig_options_ch9;

/*
 * 128-bit STS key to be programmed into CP_KEY register.
 *
 * This key needs to be known and programmed the same at both units performing
 * the SS-TWR.
 * In a real application for security this would be private and unique to the 
 * two communicating units and chosen/assigned in a secure manner lasting just 
 * for the period of their association.
 *
 * Here we use a default KEY as specified in the IEEE 802.15.4z annex
 */
static dwt_sts_cp_key_t cp_key = {
    0x14EB220F,0xF86050A8,0xD1D336AA,0x14148674
};

/*
 * 128-bit initial value for the nonce to be programmed into the CP_IV register.
 *
 * The IV, like the key, needs to be known and programmed the same at both 
 * units performing the SS-TWR.
 * It can be considered as an extension of the KEY. The low 32 bits of the 
 * IV is the counter.
 * In a real application for any particular key the value of the IV including 
 * the count should not be reused, i.e. if the counter value wraps the upper 
 * 96-bits of the IV should be changed, e.g. incremented.
 *
 * Here we use a default IV as specified in the IEEE 802.15.4z annex
 */
static dwt_sts_cp_iv_t cp_iv = {
    0x1F9A3DE4,0xD37EC3CA,0xC44FA8FB,0x362EEB34
};

/*
 * The 'poll' message initiating the ranging exchange includes 
 * a 32-bit counter which is part of the IV used to generate the 
 * scrambled timestamp sequence (STS) in the transmitted packet.
 */
static void send_tx_poll_msg(void)
{
    /* Write frame data to DW IC and prepare transmission. See NOTE 7 below. */
    tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */

    /* Start transmission. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    /* Poll DW IC until TX frame sent event set. See NOTE 8 below. */
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
    { };

    /* Clear TXFRS event. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
}

/*! ---------------------------------------------------------------------------
 * @fn ds_twr_initiator_sts()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
int app_main(void)
{
    int16_t stsQual; /* This will contain STS quality index and status */
    int goodSts = 0; /* Used for checking STS quality in received signal */
    uint8_t firstLoopFlag = 0; /* Used for checking if the program has gone 
                                * through the main loop for the first time */

    /* Display application name on UART. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    /* See config_options.h for details */
#ifdef CONFIG_SPI_FAST_RATE
    port_set_dw_ic_spi_fastrate();
#endif /* CONFIG_SPI_FAST_RATE */
#ifdef CONFIG_SPI_SLOW_RATE
    port_set_dw_ic_spi_slowrate();
#endif /* CONFIG_SPI_SLOW_RATE */

    /* Reset DW IC */
    /* Target specific drive of RSTn line into DW IC low for a period. */
    reset_DWIC(); 

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC) */
    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { };

    if (dwt_initialise(DWT_DW_IDLE) == DWT_ERROR) {
        LOG_INF("INIT FAILED");
        while (1) { };
    }

    /* Enabling LEDs here for debug so that for each TX the D1 LED will flash 
     * on DW3000 red eval-shield boards.
     * Note, in real low power applications the LEDs should not be used. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK) ;

    /* Configure DW IC. See NOTE 15 below. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if(dwt_configure(&config_options)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    if (config_options.chan == 5) {
        dwt_configuretxrf(&txconfig_options);
    }
    else {
        dwt_configuretxrf(&txconfig_options_ch9);
    }

    /* Apply default antenna delay value. See NOTE 2 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* Set expected response's delay and timeout. See NOTE 14, 17 and 18 below.
     * As this example only handles one incoming frame with always the same 
     * delay and timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

    /* Set expected response's timeout. See NOTE 1 and 5 below.
     * As this example only handles one incoming frame with always the same
     * delay, this value can be set here once for all. */
    set_resp_rx_timeout(RESP_RX_TIMEOUT_UUS, &config_options);

    /* Next can enable TX/RX states output on GPIOs 5 and 6 to help 
     * diagnostics, and also TX/RX LEDs */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    LOG_INF("Initiator ready");

    /* Loop for user defined number of ranges. */
    while (1) {
        /*
         * Set STS encryption key and IV (nonce).
         * See NOTE 16 below.
         */
        if (!firstLoopFlag) {
            /*
             * On first loop, configure the STS key & IV, then load them.
             */
            dwt_configurestskey(&cp_key);
            dwt_configurestsiv(&cp_iv);
            dwt_configurestsloadiv();
            firstLoopFlag = 1;
        }
        else {
            /*
             * On subsequent loops, we only need to reload the lower 32 bits of STS IV.
             */
            dwt_writetodevice(STS_IV0_ID, 0, 4, (uint8_t *)&cp_iv);
            dwt_configurestsloadiv();
        }

        /*
         * Send the poll message to the responder.
         */
        send_tx_poll_msg();

        /* We assume that the transmission is achieved correctly, poll for 
         * reception of a frame or error/timeout. See NOTE 8 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & 
                                                (SYS_STATUS_RXFCG_BIT_MASK | 
                                                 SYS_STATUS_ALL_RX_TO      | 
                                                 SYS_STATUS_ALL_RX_ERR)))
        { };

        /* Need to check the STS has been received and is good. */
        goodSts = dwt_readstsquality(&stsQual);

        /* Increment frame sequence number after transmission of the 
         * poll message (modulo 256). */
        frame_seq_nb++;

        /*
         * Here we are checking for a good frame and good STS quality.
         */
        if ((status_reg & SYS_STATUS_RXFCG_BIT_MASK) && (goodSts >= 0)) {

            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_GOOD);

            /* A frame has been received, read it into the local buffer. */
            uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;

            if (frame_len <= sizeof(rx_buffer)) {
                dwt_readrxdata(rx_buffer, frame_len, 0);

                /* Check that the frame is the expected response from the 
                 * companion "DS TWR responder STS" example.
                 * As the sequence number field of the frame is not relevant, 
                * it is cleared to simplify the validation of the frame. */
                rx_buffer[ALL_MSG_SN_IDX] = 0;

                if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0) {
                    uint32_t final_tx_time;
                    uint64_t poll_tx_ts, resp_rx_ts, final_tx_ts;
                    int ret = DWT_ERROR;

                    /* Retrieve poll transmission and response reception 
                     * timestamps. See NOTE 9 below. */
                    poll_tx_ts = get_tx_timestamp_u64();
                    resp_rx_ts = get_rx_timestamp_u64();

                    /* Compute final message transmission time. See NOTE 19 below. */
                    final_tx_time = (resp_rx_ts + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
                    dwt_setdelayedtrxtime(final_tx_time);

                    final_tx_ts = (((uint64_t)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

                    /* Write all timestamps in the final message. See NOTE 19 below. */
                    final_msg_set_ts(&tx_final_msg[FINAL_MSG_POLL_TX_TS_IDX], poll_tx_ts);
                    final_msg_set_ts(&tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX], resp_rx_ts);
                    final_msg_set_ts(&tx_final_msg[FINAL_MSG_FINAL_TX_TS_IDX], final_tx_ts);

                    /* Write and send final message. See NOTE 7 below. */
                    tx_final_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
                    dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0); /* Zero offset in TX buffer. */
                    dwt_writetxfctrl(sizeof(tx_final_msg), 0, 1); /* Zero offset in TX buffer, ranging bit set. */

                    ret = dwt_starttx(DWT_START_TX_DELAYED);
                    /* If dwt_starttx() returns an error, abandon this ranging
                     * exchange and proceed to the next one. 
                     * See NOTE 13 below. */
                    if (ret == DWT_SUCCESS) {
                       /* Poll DW IC until TX frame sent event set. See NOTE 8 below. */
                       while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                       { };

                       /* Clear TXFRS event. */
                       dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

                       /* Increment frame sequence number after transmission of 
                        * the final message (modulo 256). */
                       frame_seq_nb++;
                    }
                }
                else {
                    errors[BAD_FRAME_ERR_IDX] += 1;
                }
            }
            else {
                errors[RTO_ERR_IDX] += 1;
            }
        }
        else {
            check_for_status_errors(status_reg, errors);

            if (!(status_reg & SYS_STATUS_RXFCG_BIT_MASK)) {
                errors[BAD_FRAME_ERR_IDX] += 1;
            }
            if (goodSts < 0) {
                errors[PREAMBLE_COUNT_ERR_IDX] += 1;
            }
            if (stsQual <= 0) {
                errors[CP_QUAL_ERR_IDX] += 1;
            }
        }

        /* Clear RX error/timeout events in the DW IC status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_GOOD | 
                                         SYS_STATUS_ALL_RX_TO   |
                                         SYS_STATUS_ALL_RX_ERR);

        /* Execute a delay between ranging exchanges. */
        Sleep(RNG_DELAY_MS);
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The double-sided two-way ranging scheme implemented here has to be considered carefully as the accuracy of the distance measured is highly
 *    sensitive to the clock offset error between the devices and the length of the response delay between frames. To achieve the best possible
 *    accuracy, this response delay must be kept as low as possible. In order to do so, 6.8 Mbps data rate is used in this example and the response
 *    delay between frames is defined as low as possible. The user is referred to User Manual for more details about the double-sided two-way ranging
 *    process.  NB:SEE ALSO NOTE 14.
 * 2. The sum of the values is the TX to RX antenna delay, this should be experimentally determined by a calibration process. Here we use a hard coded
 *    value (expected to be a little low so a positive error will be seen on the resultant distance estimate). For a real production application, each
 *    device should have its own antenna delay properly calibrated to get good precision when performing range measurements.
 * 3. The frames used here are Decawave specific ranging frames, complying with the IEEE 802.15.4 standard data frame encoding. The frames are the
 *    following:
 *     - a poll message sent by the initiator to trigger the ranging exchange.
 *     - a response message sent by the responder to complete the exchange and provide all information needed by the initiator to compute the
 *       time-of-flight (distance) estimate.
 *    The first 10 bytes of those frame are common and are composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: destination address, see NOTE 4 below.
 *     - byte 7/8: source address, see NOTE 4 below.
 *     - byte 9: function code (specific values to indicate which message it is in the ranging process).
 *    The remaining bytes are specific to each message as follows:
 *    Poll message:
 *     - no more data
 *    Response message:
 *     - byte 10 -> 13: poll message reception timestamp.
 *     - byte 14 -> 17: response message transmission timestamp.
 *    All messages end with a 2-byte checksum automatically set by DW IC.
 * 4. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 5. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive the complete response frame sent by the responder at the
 *    6.8M data rate used (around 200 µs).
 * 6. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 * 7. dwt_writetxdata() takes the full size of the message as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW IC. This means that our variable could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 8. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW IC User Manual for more details on "interrupts". It is also to be noted that STATUS register is 5 bytes long but, as the event we
 *    use are all in the first bytes of the register, we can use the simple dwt_read32bitreg() API call to access it instead of reading the whole 5
 *    bytes.
 * 9. The high order byte of each 40-bit time-stamps is discarded here. This is acceptable as, on each device, those time-stamps are not separated by
 *    more than 2**32 device time units (which is around 67 ms) which means that the calculation of the round-trip delays can be handled by a 32-bit
 *    subtraction.
 * 10. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW IC API Guide for more details on the DW IC driver functions.
 * 11. The use of the clock offset value to correct the TOF calculation, significantly improves the result of the SS-TWR where the remote
 *     responder unit's clock is a number of PPM offset from the local initiator unit's clock.
 *     As stated in NOTE 2 a fixed offset in range will be seen unless the antenna delay is calibrated and set correctly.
 * 12. In this example, the DW IC is put into IDLE state after calling dwt_initialise(). This means that a fast SPI rate of up to 20 MHz can be used
 *     thereafter.
 * 13. This example uses the 802.15.4z STS with a packet configuration of mode 1 which looks like so:
 *     ---------------------------------------------------
 *     | Ipatov Preamble | SFD | STS | PHR | PHY Payload |
 *     ---------------------------------------------------
 *     There is a possibility that the TX and RX units in this example will go out of sync as their STS IV values may be misaligned. The STS IV value
 *     changes upon each receiving and transmitting event by the chip. While the TX and RX devices in this example start at the same STS IV values, it
 *     is possible that they can go out sync if a signal is not received correctly, devices are out of range, etc. To combat this, the 'poll message'
 *     that the initiator sends to the responder contains a plain-text STS counter value. The responder receives this message and first checks if
 *     the received frame is out of sync with it's own counter. If so, it will use this received counter value to update it's own counter. When out
 *     of sync with each other, the STS will not align correctly - thus we get no secure timestamp values.
 * 14. The receiver is enabled with reference to the timestamp of the previously received signal.
 *     The receiver will start after a defined delay.
 *     This defined delay is currently the same as the delay between the responder's received
 *     timestamp of it's last received frame and the timestamp of the transmitted signal that is
 *     sent in response.
 *     This means that the initiator needs to reduce it's delay by the configured preamble length.
 *     This allows for the receiver to enable on the initiator at the same time as responder is
 *     transmitting it's message. It should look something like this:
 *
 *     Initiator: |Poll TX| ..... |Resp RX| ........ |Final TX|
 *     Responder: |Poll RX| ..... |Resp TX| ........ |Final RX|
 *                    ^|P RMARKER|                                    - time of Poll TX/RX
 *                                    ^|R RMARKER|                    - time of Resp TX/RX
 *                                                       ^|R RMARKER| - time of Final TX/RX
 *
 *                        <--TDLY->                                   - POLL_TX_TO_RESP_RX_DLY_UUS (RDLY-RLEN)
 *                                <-RLEN->                            - RESP_RX_TIMEOUT_UUS   (length of poll frame)
 *                     <----RDLY------>                               - POLL_RX_TO_RESP_TX_DLY_UUS (depends on how quickly responder
 *                                                                                                                       can turn around and reply)
 *
 *
 *                                         <--T2DLY->                 - RESP_TX_TO_FINAL_RX_DLY_UUS (R2DLY-FLEN)
 *                                                   <-FLEN--->       - FINAL_RX_TIMEOUT_UUS   (length of response frame)
 *                                     <----RDLY--------->            - RESP_RX_TO_FINAL_TX_DLY_UUS (depends on how quickly initiator
 *                                                                                                                       can turn around and reply)
 * 15. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *     configuration.
 * 16. This example will set the STS key and IV upon each iteration of the main while loop. While this has the benefit of keeping the STS count in
 *     sync with the responder device (which does the same), it should be noted that this is not a 'secure' implementation as the count is reset upon
 *     each iteration of the loop. An attacker could potentially recognise this pattern if the signal was being monitored. While it serves it's
 *     purpose in this simple example, it should not be utilised in any final solution.
 * 17. Delays between frames have been chosen here to ensure proper synchronisation of transmission and reception of the frames between the initiator
 *     and the responder and to ensure a correct accuracy of the computed distance. The user is referred to DecaRanging ARM Source Code Guide for more
 *     details about the timings involved in the ranging process.
 *
 *  EXAMPLE 1: with SPI rate set to 18 MHz (default on this platform), and frame lengths of ~190 us, the delays can be set to:
 *             POLL_RX_TO_RESP_TX_DLY_UUS of 400uus, and RESP_RX_TO_FINAL_TX_DLY_UUS of 400uus (TXtoRX delays are set to 210uus)
 *             reducing the delays further can be achieved by using interrupt to handle the TX/RX events, or other code optimisations/faster SPI
 *
 *  EXAMPLE 2: with SPI rate set to 4.5 MHz, and frame lengths of ~190 us, the delays can be set to:
 *             POLL_RX_TO_RESP_TX_DLY_UUS of 550uus, and RESP_RX_TO_FINAL_TX_DLY_UUS of 600uus (TXtoRX delays are set to 360 and 410 uus respectively)
 *
 * 18. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *     is arbitrary but chosen large enough to make sure that there is enough time to receive the complete response frame sent by the responder at the
 *     6.81 Mbps data rate used (around 200 us).
 * 19. As we want to send final TX timestamp in the final message, we have to compute it in advance instead of relying on the reading of DW IC
 *     register. Timestamps and delayed transmission time are both expressed in device time units so we just have to add the desired response delay to
 *     response RX timestamp to get final transmission time. The delayed transmission time resolution is 512 device time units which means that the
 *     lower 9 bits of the obtained value must be zeroed. This also allows to encode the 40-bit value in a 32-bit words by shifting the all-zero lower
 *     8 bits.
 ****************************************************************************************************************************************************/
