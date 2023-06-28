/*! ----------------------------------------------------------------------------
 *  @file    simple_tx_aes.c
 *  @brief   Simple TX + AES example code
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
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <port.h>
#include <mac_802_15_8.h>

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tx_timed_sleep);

/* Example application name and version to display. */
#define APP_NAME    "AES TX"

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
        .mic                = MIC_16,/* Means 16 bytes tag*/
        .mode               = AES_Encrypt,
        .aes_core_type      = AES_core_type_GCM,     // Use CCM core
        .aes_key_otp_type   = AES_key_RAM,
        .key_addr           = 0
};

/* Below is a payload, which will be sent encrypted in this example */
uint8_t payload[] = "Good, This is the right message";

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth 
 * and power of the spectrum at the current temperature. 
 * These values can be calibrated prior to taking reference measurements. 
 * See NOTE 1 below. */
extern dwt_txconfig_t txconfig_options;

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
    DWT_STS_MODE_OFF, /*STS mode*/
    DWT_STS_LEN_64,   /* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0/*pdoa mode*/
};


/**
* Application entry point.
*/
int app_main(void)
{
    int8_t              status;
    uint8_t               mic_size;
    /* Set the AES key */
    uint64_t            PN=0;/* Can start also with random value, should not exceed 6 bytes - 0xFFFFFFFFFFFF*/
    uint8_t             nonce[12];
    dwt_aes_job_t       aes_job;

    /* 802.15.8 Standard */
    mac_frame_802_15_8_format_t header = {
            .fc[0]      = 0x50, /* DATA, SRC is 48 bit MAC, DST is 48 bit MAC */
            .fc[1]      = 0x40, /* no ACK, no Header Information Element present,
                                 * no Payload Information Element present,
                                 * frame encrypted, R = 0 */
            .seq        = 0,                        /* Sequence number */
            .dst_addr   = {0xA,0xB,0xC,0xD,0xE,0xF},/* 48 bit Destination address (RX device) is 0x0F0E0D0C0B0A */
            .src_addr   = {0x1,0x2,0x3,0x4,0x5,0x6},/* 48 bit Source address (TX device) is 0x060504030201 */
            .nonce      = {0}
    };

    if (aes_config.mic == 0) mic_size = 0;
    else mic_size = aes_config.mic * 2 + 2;

    /* Display application name. */
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
    while (!dwt_checkidlerc()) { };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (TRUE) { };
    }

    /* Configure DW3000. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if(dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { };
    }

    /* Configure the tx spectrum parameters (power and PG delay) */
    dwt_configuretxrf(&txconfig_options);

    dwt_set_keyreg_128(&aes_key);
    dwt_configure_aes(&aes_config);

    /* Fill aes job to do */
    aes_job.nonce       = nonce;/* use constructed nonce to encrypt payload */
    aes_job.header      = (uint8_t *)&header;/* plain-text header which will not be encrypted */
    aes_job.header_len  = sizeof(header);
    aes_job.payload     = payload;  /* payload to be encrypted */
    aes_job.payload_len = sizeof(payload);
    aes_job.src_port    = AES_Src_Tx_buf;/* dwt_do_aes will take plain text to the TX buffer */
    aes_job.dst_port    = AES_Dst_Tx_buf;/* dwt_do_aes will replace the original plain text TX buffer with encrypted one */
    aes_job.mode        = aes_config.mode;
    aes_job.mic_size    = mic_size;

    memcpy(&nonce[6], &header.src_addr[0], 6);

    /* Set the frame control size*/
    dwt_writetxfctrl(sizeof(header) + sizeof(payload) + mic_size + FCS_LEN, 0, 0);
    
    /* Verify PN that was entered is not more than 6 bytes */
    PN &= 0xFFFFFFFFFFFF;

    while(TRUE)
    {
        /* Assuming head & nonce are both of the same storage container size */
        nonce[0]=header.nonce[0]=(uint8_t)PN;
        nonce[1]=header.nonce[1]=(uint8_t)(PN>>8);
        nonce[2]=header.nonce[2]=(uint8_t)(PN>>16);
        nonce[3]=header.nonce[3]=(uint8_t)(PN>>24);
        nonce[4]=header.nonce[4]=(uint8_t)(PN>>32);
        nonce[5]=header.nonce[5]=(uint8_t)(PN>>40);

        /* Embed low 6 bytes of the nonce as a GCMP HEAD as per 802.15.8 */
        //memcpy(header.nonce, &nonce, sizeof(header.nonce));   
        
        /* Note, 802.15.8 adds a MIC size of 16 bytes after payload */
        status = dwt_do_aes(&aes_job,aes_config.aes_core_type);
        if (status < 0) {
            /* Problem with Header/Payload length or mode selection */
            LOG_ERR("Length AES error");
            break;
        }
        else {
            /* Check return status and transmit if OK */
            if (status & AES_ERRORS) {
                LOG_ERR("ERROR AES");
                break;
            }
            else {
                /* There were no errors */
                /* START TX */
                dwt_starttx(DWT_START_TX_IMMEDIATE);

                /* function to access it.*/
                while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK))
                { };

                /* Clear TX frame sent event. */
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

                /* PN can be saved as 6 bytes */
                PN = (PN+1) % 0xFFFFFFFFFFFF;
                header.seq++;

                static int cnt = 0;
                LOG_INF("AES TX OK: %d", cnt++);

                Sleep(500);
            }
        }
    }
    return 0;
}

/*****************************************************************************************************************************************************
 * NOTES:
 * 1.In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *   the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 *
 * TODO: Correct and/or required notes for this example can be added later.
 *
 ****************************************************************************************************************************************************/
