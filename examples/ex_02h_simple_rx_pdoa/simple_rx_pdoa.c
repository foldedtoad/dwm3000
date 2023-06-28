/*! ----------------------------------------------------------------------------
 *  @file    simple_rx_pdoa.c
 *  @brief   This examples prints the PDOA value to the virtual COM.
 *           The transmitter should be simple_tx_pdoa.c
 *           See note 3 regarding calibration and offset
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

#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <port.h>
#include <deca_vals.h>
#include <shared_defines.h>
#include <shared_functions.h>

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simple_rx_pdoa);

static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);

/* Example application name and version to display. */
#define APP_NAME "PDOA example"

/* Default communication configuration. We use default non-STS DW mode. see note 2*/
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PLEN_128,     /* Preamble length. Used in TX only. */
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
    (129 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    (DWT_STS_MODE_1 | DWT_STS_MODE_SDC), /* STS enabled */
    DWT_STS_LEN_256, /* Cipher length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M3      /* PDOA mode 3 */
};

int16_t   pdoa_val = 0;

/* Will hold the data to send to the virtual COM */
uint8_t   pdoa_message_data[40]; 

/**
 * Application entry point.
 */
int app_main(void)
{
    int16_t last_pdoa_val = 0;

    /* Sends application name to test_run_info function. */
    LOG_INF(APP_NAME);

    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    /* Target specific drive of RSTn line into DW IC low for a period. */
    reset_DWIC(); 

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC */
    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_IDLE) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Configure DW3000. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if (dwt_configure(&config)) {
        LOG_INF("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Register RX callback. */
    dwt_setcallbacks(NULL, rx_ok_cb, rx_err_cb, rx_err_cb, NULL, NULL);

    /* Enable wanted interrupts (RX good frames and RX errors). */
    dwt_setinterrupt(SYS_ENABLE_LO_RXFCG_ENABLE_BIT_MASK | SYS_STATUS_ALL_RX_ERR, 0, DWT_ENABLE_INT);

    /* Clearing the SPI ready interrupt */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RCINIT_BIT_MASK | SYS_STATUS_SPIRDY_BIT_MASK);

    /* Install DW IC IRQ handler. */
    port_set_dwic_isr(dwt_isr);

    /* Activate reception immediately. See NOTE 1 below. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    /* Loop forever receiving frames */
    while (1) {
        if (last_pdoa_val != pdoa_val) {
            last_pdoa_val = pdoa_val;
            LOG_INF("PDOA val = %d", last_pdoa_val);
        }
    }
    return DWT_SUCCESS;
}


/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_ok_cb()
 *
 * @brief Callback to process RX good frame events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    int16_t cpqual;
    (void) cb_data;

    /* Checking STS quality see note 4 */
    if (dwt_readstsquality(&cpqual)) {
        pdoa_val = dwt_readpdoa();
    }
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_err_cb()
 *
 * @brief Callback to process RX error and timeout events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void rx_err_cb(const dwt_cb_data_t *cb_data)
{
    (void) cb_data;

    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. Manual reception activation is performed here but DW IC offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 2. This is the default configuration recommended for optimum performance. An offset between the clocks of the transmitter and receiver will
 *    occur. The DW3000 can tolerate a difference of +/- 20ppm. For optimum performance an offset of +/- 5ppm is recommended.
 * 3. A natural offset will always occur between any two boards. To combat this offset the transmitter and receiver should be placed
 *    with a real PDOA of 0 degrees. When the PDOA is calculated this will return a non-zero value. This value should be subtracted from all
 *    PDOA values obtained by the receiver in order to obtain a calibrated PDOA.
 * 4. If the STS quality is poor the returned PDoA value will not be accurate and as such will not be recorded
 ****************************************************************************************************************************************************/
