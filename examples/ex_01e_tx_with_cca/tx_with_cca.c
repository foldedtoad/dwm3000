/*! ----------------------------------------------------------------------------
 *  @file    tx_with_cca.c
 *  @brief   Here we implement a simple Clear Channel Assessment (CCA) mechanism
 *           before frame transmission. The CCA can be used to avoid collisions
 *           with other frames on the air. See Note 1 for more details.
 *
 *           Note this is not doing CCA the way a continuous carrier radio would do it by
 *           looking for energy/carrier in the band. It is only looking for preamble so
 *           will not detect PHR or data phases of the frame. In a UWB data network it
 *           is advised to also do a random back-off before re-transmission in the event
 *           of not receiving acknowledgement to a data frame transmission.
 *
 *           This example has been designed to operate with the transmitter in
 *           Continuous Frame mode (CF).
 *           The transmitter  will fill the air with frames.
 *           The receiver is set in dwt_starttx(DWT_START_TX_CCA) mode.
 *           In this mode transmission will occurr if the receiver does not detect a preamble,
 *           otherwise the transmission will be cancelled.
 *           Note, the Continuous Frame example actually stops after 2 minutes
 *           interval (thus the user should toggle the reset button on the unit running
 *           CF example to restart it if they wish to continue observing this pseudo CCA
 *           experiencing an environment of high air-utilisation). Thus the radio configuration
 *           used here matches that of CF example.
 * @attention
 *
 * Copyright 2019 - 2020 (c) Decawave Ltd, Dublin, Ireland.
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
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(tx_with_cca);


/* Example application name */
#define APP_NAME "TX WITH CCA v1.0"

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
    (129 + 8 - 8),   /* SFD timeout (preamble length + 1 + SFD length - PAC size). 
                      *  Used in RX only. */
    DWT_STS_MODE_OFF, /* STS disabled */
    DWT_STS_LEN_64,/* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* The frame sent in this example is an 802.15.4 standard blink. 
 * It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID, see NOTE 2 below.
 *     - byte 10/11: frame check-sum, automatically set/added by DW3000.  */
static uint8_t tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E'};

/* Index to access to sequence number of the blink frame in the tx_msg array. */
#define BLINK_FRAME_SN_IDX 1

/* The real length that is going to be transmitted */
#define FRAME_LENGTH    (sizeof(tx_msg)+FCS_LEN) 

/* Inter-frame delay period, in milliseconds.
 * this example will try to transmit a frame every 100 ms*/
#define TX_DELAY_MS 100

/* Initial backoff period when failed to transmit a frame due to preamble detection. */
#define INITIAL_BACKOFF_PERIOD 400 /* This constant would normally smaller (e.g. 1ms),
                                    * however here it is set to 400 ms so that
                                    * user can see (on Zephyr RTT console) the report 
                                    * that the CCA detects a preamble on the air occasionally,
                                    * and is doing a TX back-off.
                                    */

int tx_sleep_period; /* Sleep period until the next TX attempt */

/* Next backoff in the event of busy channel detection by this pseudo CCA algorithm */
int next_backoff_interval = INITIAL_BACKOFF_PERIOD;

/* holds copy of status register */
uint32_t status_reg = 0;
uint32_t status_regh = 0; /* holds the high 32 bits of SYS_STATUS_HI */

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and 
 * power of the spectrum at the current temperature. 
 * These values can be calibrated prior to taking reference measurements. 
 * See NOTE 3 below. */
extern dwt_txconfig_t txconfig_options;


/**
 * Application entry point.
 */
int app_main(void)
{
    /* Display application name. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC(); /* Target specific drive of RSTn line into DW IC low for a period. */

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC,
     * or could wait for SPIRDY event) */
    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR)
    {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Enabling LEDs here for debug so that for each TX the D1 LED will flash
     * on DW3000 red eval-shield boards. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK) ;

    /* Configure DW IC. See NOTE 7 below. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration
     * has failed the host should reset the device */
    if (dwt_configure(&config)) 
    {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Configure preamble timeout to 3 PACs; if no preamble detected in this 
     * time we assume channel is clear. See NOTE 4*/
    dwt_setpreambledetecttimeout(3);

    /* Loop forever sending frames periodically. */
    while(1)
    {
        /* Write frame data to DW3000 and prepare transmission. See NOTE 5 below.*/
        dwt_writetxdata(FRAME_LENGTH - FCS_LEN, tx_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(FRAME_LENGTH, 0, 0); /* Zero offset in TX buffer, no ranging. */

        /* Start transmission with CCA. The transmission will only start once 
         * there is no preamble detected within 3 PACs as defined above
         * e.g. once we get the preamble timeout or TX will be canceled if a 
         * preamble is detected. */
        dwt_starttx(DWT_START_TX_CCA);

        /* Poll DW3000 until either TX complete or CCA_FAIL. See NOTE 6 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS_BIT_MASK))
        {
            if ((status_regh = dwt_read32bitreg(SYS_STATUS_HI_ID))& SYS_STATUS_HI_CCA_FAIL_BIT_MASK) {
                break;
            }
        }

        if (status_reg & SYS_STATUS_TXFRS_BIT_MASK)
        {
            tx_sleep_period = TX_DELAY_MS; /* sent a frame - set interframe period */
            next_backoff_interval = INITIAL_BACKOFF_PERIOD; /* set initial backoff period */

            /* Increment the blink frame sequence number (modulo 256). */
            tx_msg[BLINK_FRAME_SN_IDX]++;

            /* Reflect frame number */
            LOG_INF("frame: %d", (int) tx_msg[BLINK_FRAME_SN_IDX]);
        }
        else
        {
            /* If DW IC detected the preamble, device will be in IDLE */
            tx_sleep_period = next_backoff_interval; /* set the TX sleep period */

            next_backoff_interval++; /* If failed to transmit, increase backoff 
                                      * and try again.
                                      * In a real implementation the back-off 
                                      * is typically a randomised period
                                      * whose range is an exponentially related 
                                      * to the number of successive failures.
                                      * See https://en.wikipedia.org/wiki/Exponential_backoff */
        }

        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        /* Execute a delay between transmissions. */
        Sleep(tx_sleep_period);
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. For Wireless Sensor Networks application, most of the MAC protocols rely on Clear Channel Assessment (CCA) to avoid collisions with other frames
 *    in the air. This consists in sampling the air for a short period to see if the medium is idle before transmitting. For most radios this involves
 *    looking for the RF carrier, but for UWB where this is not possible, one approach is to just look for preamble to avoid conflicting transmissions,
 *    since any sending of preamble during data will typically not disturb those receivers who are demodulating in data mode.
 *    The idea then is to sample the air for a small amount of time to see if a preamble can be detected, then if preamble is not seen the transmission
 *    is initiated, otherwise we defer the transmission typically for a random back-off period after which transmission is again attempted with CCA.
 *    Note: we return to idle for the back-off period and do not receive the frame whose preamble was detected, since the MAC (and upper layer) wants
 *    to transmit and not receive at this time.
 *    This example has been designed to operate with example 4b - Continuous Frame. The 4b device will fill the air with frames which will be detected by the CCA
 *    and thus the CCA will cancel the transmission and will use back off to try sending again at later stage.
 *    This example will actually get to send when the CCA preamble detection overlaps with the data portion of the continuous TX or inter frame period,
 *    Note the Continuous Frame example actually stops after 30s interval (thus the user should toggle the reset button on the unit running example 4b
 *    to restart it if they wish to continue observing this pseudo CCA experiencing an environment of high air-utilisation).
 * 2. The device ID is a hard coded constant in the blink to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW3000 unique ID by combining the Lot ID & Part Number values programmed into the
 *    DW3000 during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that
 *    customers buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW3000 User Manual.
 * 3. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW3000 OTP memory.
 * 4. The preamble timeout of 3 PACs is recommended as sufficient for this CCA example for all modes and data rates. The PAC size should be different
 *    when is different for different preamble configurations, as per User Manual guidelines.
 * 5. dwt_writetxdata() takes the tx_msg buffer and copies it into devices TX buffer memory, the two byte check-sum at the end of the frame is
 *    automatically appended by the DW3000, thus the dwt_writetxfctrl() should be given the total length.
 * 6. We use polled mode of operation here to keep the example as simple as possible, but the TXFRS and CCA_FAIL status events can be used to generate an interrupt.
 *    Please refer to DW3000 User Manual for more details on "interrupts".
 * 7. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *    configuration.
 ****************************************************************************************************************************************************/
