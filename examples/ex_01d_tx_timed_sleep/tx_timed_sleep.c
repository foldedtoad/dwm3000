/*! ----------------------------------------------------------------------------
 *  @file    tx_timed_sleep.c
 *  @brief   TX with timed sleep example code
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
LOG_MODULE_REGISTER(tx_timed_sleep);

/* Example application name and version to display on LCD screen. */
#define APP_NAME "TX TIME SLP v1.0"

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
    DWT_STS_MODE_OFF,/* STS disabled */
    DWT_STS_LEN_64,  /* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* The frame sent in this example is an 802.15.4e standard blink. 
 * It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID, see NOTE 1 below.
 *     - byte 10/11: frame check-sum, automatically set by DW IC.  */
static uint8_t tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};

/* Index to access to sequence number of the blink frame in the tx_msg array. */
#define BLINK_FRAME_SN_IDX 1

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

/* Crystal frequency, in hertz. */
#define XTAL_FREQ_HZ 38400000

/* Sleep time, in milliseconds. See NOTE 2 below.
 * Take a small margin compared to TX_DELAY_MS so that the DW IC has the time 
 * to wake up and prepare the next frame. */
#define SLEEP_TIME_MS (TX_DELAY_MS - 10)

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth 
 * and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference
 * measurements. See NOTE 2 below. */
extern dwt_txconfig_t txconfig_options;

/* This will be set to 1 in the main as device is put to sleep,
 * and it will be cleared in the callback once device wakes up */
static volatile int sleeping = 0; 

static void spi_ready_cb(const dwt_cb_data_t *cb_data);

/**
 * Application entry point.
 */
int app_main(void)
{
    uint16_t lp_osc_freq, sleep_cnt;

    /* Display application name on LCD. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC(); /* Target specific drive of RSTn line into DW IC low for a period. */

    Sleep(2); // Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC)

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR)
    {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /*Clearing the SPI ready interrupt*/
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RCINIT_BIT_MASK | SYS_STATUS_SPIRDY_BIT_MASK);

    /* Install DW IC IRQ handler. NOTE: the IRQ line must have a PULLDOWN or
     * else it may trigger incorrectly when the device is sleeping */
    port_set_dwic_isr(dwt_isr);

    /* Calibrate and configure sleep count. */
    lp_osc_freq = XTAL_FREQ_HZ / dwt_calibratesleepcnt();
    sleep_cnt = ((SLEEP_TIME_MS * ((uint32_t) lp_osc_freq)) / 1000) >> 12;

    // sleep_cnt = 0x06; // 1 step is ~ 175ms, 6 ~= 1s
    dwt_configuresleepcnt(sleep_cnt);

    /* Configure DW IC. See NOTE 6 below. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if(dwt_configure(&config)) 
    {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Configure sleep and wake-up parameters. */
    dwt_configuresleep(DWT_CONFIG, DWT_PRES_SLEEP | DWT_WAKE_CSN | DWT_SLEEP | DWT_SLP_EN);

    /* Register the call-backs (only SPI ready callback is used). */
    dwt_setcallbacks(NULL, NULL, NULL, NULL, NULL, &spi_ready_cb);


    /* Loop forever sending frames periodically. */
    while (1)
    {
        /* Write frame data to DW IC and prepare transmission. See NOTE 4 below. */
        dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */

        /* Start transmission. */
        dwt_starttx(DWT_START_TX_IMMEDIATE);

        /* It is not possible to access DW IC registers once it has sent the 
         * frame and gone to sleep, and therefore we do not try to poll for TX
         * frame sent, but instead simply wait sufficient time for the DW IC 
         * to wake up again before we loop back to send another frame.
         * If interrupts are enabled, (e.g. if MTXFRS bit is set in the SYS_MASK 
         * register) then the TXFRS event will cause an active interrupt and
         * prevent the DW IC from sleeping. */

        /* Poll DW IC until TX frame sent event set. See NOTE 7 below.
         * STATUS register is 4 bytes long but, as the event we are looking at 
         * is in the first byte of the register, we can use this simplest API
         * function to access it. */
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)) 
        { /* spin */ };

        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        /* Put DW IC to sleep. Go to IDLE state after wakeup*/
        dwt_entersleep(DWT_DW_IDLE);

        sleeping = 1;

        /* In this example, there is nothing to do to wake the DW IC up as it 
         * is handled by the sleep timer. */
        while (sleeping) { /* spin */ }; /* Wait for device to wake up */


        /* Increment the blink frame sequence number (modulo 256). */
        tx_msg[BLINK_FRAME_SN_IDX]++;
        
        /* Reflect frame number */
        LOG_INF("frame: %d", (int) tx_msg[BLINK_FRAME_SN_IDX]);
    }
}

/*
 *
 * @brief Callback to process SPI ready events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void spi_ready_cb(const dwt_cb_data_t *cb_data)
{
    (void) cb_data;

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    /* Restore the required configurations on wake */
    dwt_restoreconfig();

    /* Device is awake */
    sleeping = 0; 
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The device ID is a hard coded constant in the blink to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW IC unique ID by combining the Lot ID & Part Number values programmed into the
 *    DW IC during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that
 *    customers buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW IC User Manual.
 * 2. The sleep counter is 16 bits wide but represents the upper 16 bits of a 28 bits counter. Thus the granularity of this counter is 4096 counts.
 *    Combined with the frequency of the internal RING oscillator being typically between 15 and 34 kHz, this means that the time granularity that we
 *    get when using the timed sleep feature is typically between 120 and 273 ms. As the sleep time calculated is rounded down to the closest integer
 *    number of sleep counts, this means that the actual sleep time can be significantly less than the one defined here.
 * 3. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 * 4. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW IC. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 5. Here we just wait for the DW IC to wake up but, in a practical implementation, this microprocessor could be put to sleep too and waken up using
 *    an interrupt generated by the DW IC waking.
 * 6. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *    configuration.
 * 7. We use polled mode of operation here to keep the example as simple as possible, but the TXFRS status event can be used to generate an interrupt.
 *    Please refer to DW IC User Manual for more details on "interrupts".
 ****************************************************************************************************************************************************/
