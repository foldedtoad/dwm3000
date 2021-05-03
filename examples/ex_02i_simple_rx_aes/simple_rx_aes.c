/*! ----------------------------------------------------------------------------
 *  @file    simple_rx_aes.c
 *  @brief   Simple RX + AES example code
 *
 * @attention
 *
 * Copyright 2018-2020 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2021 (c) Callender-Consulting, LLC  (port to Zephyr)
 *
 * All rights reserved.
 *
 * @author Decawave
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <port.h>
#include <shared_defines.h>
#include <mac_802_15_8.h>

//zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(simple_tx);

/* Example application name and version to display on LCD screen. */
#define APP_NAME    "AES RX AES"

/*
 * APP_KEY_0-APP_KEY_4 is a 128 bit AES key which should be set the same
 * for both Encryption and Decryption.
 * This should match complementary RX example.
 *
 * The dwt_aes_key_t structure is actually 256-bits in length. As we are only using
 * a 128-bit key in this example, we will pad the rest of the structure out with zeroes.
 */
static const dwt_aes_key_t aes_key = {
    0x41424344, 0x45464748, 0x49505152, 0x53545556,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
}; /* Initialize 128bits key */

static const dwt_aes_config_t   aes_config = {
    .key_load           = AES_KEY_Load,
    .key_size           = AES_KEY_128bit,
    .key_src            = AES_KEY_Src_Register,
    .mic                = MIC_16,               /* Means 16 bytes tag */
    .mode               = AES_Decrypt,
    .aes_core_type      = AES_core_type_GCM,    // Use GCM core
    .aes_key_otp_type   = AES_key_RAM,
    .key_addr           = 0
};


/* Default communication configuration. We use default non-STS DW mode. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode: extended frame mode, up to 1023-16-2 in the payload */
    DWT_PHRRATE_STD,
    (129 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_OFF, /* STS mode*/
    DWT_STS_LEN_64,   /* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0       /* pdoa mode */
};


/**
 * Application entry point.
 */
int app_main(void)
{
    uint32_t        status_reg;
    dwt_aes_job_t   aes_job;
    uint8_t         mic_size;
    aes_results_e   aes_results;
    uint8_t         payload[128] = {0};

    if (aes_config.mic == 0) {
        mic_size = 0;
    }
    else {
        mic_size = aes_config.mic * 2 + 2;
    }

    /* Display application name on LCD. */
    LOG_INF(APP_NAME);

    /* DW3000 chip can run from high speed from start-up.*/
    port_set_dw_ic_spi_fastrate();

    /* Reset and initialize DW chip. */
    /* Target specific drive of RSTn line into DW3000 low for a period. */
    reset_DWIC(); 

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC, 
     * or could wait for SPIRDY event) */
    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (TRUE) { /* spin */ };
    }

    /* Configure DW3000. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if (dwt_configure(&config)) {
        LOG_INF("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    dwt_set_keyreg_128(&aes_key);
    dwt_configure_aes(&aes_config);

    aes_job.src_port = AES_Src_Rx_buf_0; /* Take encrypted frame from the RX buffer */
    aes_job.dst_port = AES_Dst_Rx_buf_0; /* Decrypt the frame to the same RX buffer : this will destroy original RX frame */
    aes_job.mode     = aes_config.mode;
    aes_job.mic_size = mic_size;

    while(TRUE) {
        /* Activate reception immediately. See NOTE 2 below. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received or an error/timeout occurs.*/
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

            uint16_t finfo16 = dwt_read16bitoffsetreg(RX_FINFO_ID, 0);

            /* Decrypt received packet */
            aes_results = rx_aes_802_15_8((finfo16 & RX_FINFO_RXFLEN_BIT_MASK),
                                           &aes_job,
                                           payload,
                                           sizeof(payload),
                                           aes_config.aes_core_type);
            if (aes_results != AES_RES_OK) {
                switch (aes_results) {
                    case AES_RES_ERROR_LENGTH:
                        LOG_ERR("Length AES error");
                        break;
                    case AES_RES_ERROR:
                        LOG_ERR("ERROR AES");
                        break;
                    case AES_RES_ERROR_FRAME:
                        LOG_ERR("Error Frame");
                        break;
                    case AES_RES_ERROR_IGNORE_FRAME:
                    case AES_RES_OK:
                        break;
                }
                break; /* Exit on error */
            }
            else {
                static int cnt;
                LOG_INF("AES TX OK %d", cnt++);
            }

            /* Clear good RX frame event in the DW chip status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
        }
        else {
            /* Clear RX error events in the status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
    }
    return 0;
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * TODO: Correct and/or required notes for this example can be added later.
 *
 ****************************************************************************************************************************************************/
