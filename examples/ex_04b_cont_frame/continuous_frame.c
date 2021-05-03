/*! ----------------------------------------------------------------------------
 *  @file    ex_04b_main.c
 *  @brief   Continuous frame mode example code
 *
 *           This example application enables continuous frame mode to transmit
 *           frames without interruption for 2 minutes before stopping all
 *           operation.
 *
 * @attention
 *
 * Copyright 2015-2020 (c) Decawave Ltd, Dublin, Ireland.
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

//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(cont_frame);

/* Example application name and version to display on LCD screen. */
#define APP_NAME "CONT FRAME v1.0"

/* Start-to-start delay between frames, expressed in halves of the 499.2 MHz
 * fundamental frequency (around 4 ns). 
 * See NOTE 1 below. */
#define CONT_FRAME_PERIOD 249600

/* Continuous frame duration, in milliseconds. */
#define CONT_FRAME_DURATION_MS 120000

/* Default communication configuration. See NOTE 1 below. */
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

/* Recommended TX power and Pulse Generator delay values for Channel 5 and 
 * 64 MHz PRF as selected in the configuration above. 
 * See NOTE 2 below. */
/* Power configuration has been specifically set for DW3000 B0 rev devices. */
extern dwt_txconfig_t txconfig_options;

/* The frame sent in this example is an 802.15.4e standard blink. 
 * It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, put to 0.
 *     - byte 2 -> 9: device ID, hard coded constant in this example 
 *                    for simplicity.
 *     - byte 10/11: frame check-sum, automatically set by DW IC in a normal 
 *                   transmission and set to 0 here for simplicity.
 * See NOTEs 1 and 3 below. */
static uint8_t tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};

/**
 * Application entry point.
 */
int app_main(void)
{
    /* Display application name on LCD. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC(); /* Target specific drive of RSTn line into DW IC low for a period. */

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC */
    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { };
    }

    /* Configure DW IC. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { };
    }

    dwt_configuretxrf(&txconfig_options);

    /* Activate continuous frame mode. */
    dwt_configcontinuousframemode(CONT_FRAME_PERIOD, config.chan);

    /* Once configured, continuous frame must be started like a 
     * normal transmission. */
    dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */
    dwt_starttx(DWT_START_TX_IMMEDIATE);

    /* Wait for the required period of repeated transmission. */
    Sleep(CONT_FRAME_DURATION_MS);

    /* Software reset of the DW IC to deactivate continuous frame mode and 
     * go back to default state. Initialisation and configuration should be run
     * again if one wants to get the DW IC back to normal operation. */
    dwt_softreset();

    /* End here. */
    while (1) { };
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. Continuous frame mode is typically used to tune transmit power for regulatory purposes. This example has been designed to reproduce the use case
 *    of a tag blinking at a high rate: the blink length is around 180 µs in this configuration, emitted once per millisecond. In this configuration,
 *    the frame's transmission power can be increased while still complying with regulations. For more details about the management of TX power, the
 *    user is referred to DW IC User Manual.
 * 2. The user is referred to DW IC User Manual for references values applicable to different channels and/or PRF. Those reference values may need to
 *    be tuned for optimum performance and regulatory approval depending on the target product design.
 * 3. The device ID is a hard coded constant in the blink to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW IC unique ID by combining the Lot ID & Part Number values programmed into the
 *    DW IC during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that
 *    customers buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW IC User Manual.
 * 4. In this example, the DW IC is left in INIT state after calling dwt_initialise(), because only the slow SPI speed is used, i.e. <= 6 MHz
 * 5. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW IC API Guide for more details on the DW IC driver functions.
 ****************************************************************************************************************************************************/
