/*! ----------------------------------------------------------------------------
 *  @file    simple_rx_sts_sdc.c
 *  @brief   Simple RX example code that utilises STS with deterministic code.
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

#include <string.h>
#include <float.h>
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
LOG_MODULE_REGISTER(simple_rx_sts_sdc);

/* Example application name */
#define APP_NAME "RX 4Z STS v1.0"

/* Default communication configuration. We use default non-STS DW mode. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    3,               /* 0 to use standard 8 symbol SFD, 
                      *   1 to use non-standard 8 symbol, 
                      *   2 for non-standard 16 symbol SFD and 
                      *   3 for 4z 8 symbol SDF type */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    DWT_PHRRATE_STD, /* PHY header rate. */
    (129 + 8 - 8),   /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
    DWT_STS_MODE_1 | DWT_STS_MODE_SDC,  /* Use STS. See NOTE 5 & 6 below. */
    DWT_STS_LEN_64,/* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};


/* Index to the start of the payload data in the TX frame */
#define FRAME_PAYLOAD_IDX 9

/* Buffer to store received frame. See NOTE 1 below. */
static uint8_t rx_buffer[FRAME_LEN_MAX];

/* Hold copy of status register state here for reference so that 
 * it can be examined at a debug breakpoint. */
static uint32_t status_reg;// = 0;

/* Hold copy of frame length of frame received (if good) so that 
 * it can be examined at a debug breakpoint. */
static uint16_t frame_len;// = 0;

/**
 * Application entry point.
 */
int app_main(void)
{
    int goodCPQ;
    int16_t cpqual; /* This will contain STS quality index, can be checked if needed */
    uint16_t cpStatus;


    /* Display application name. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    /* Target specific drive of RSTn line into DW IC low for a period. */
    reset_DWIC(); 

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC */
    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Enabling LEDs here for debug so that for each RX-enable the D2 LED will 
     * flash on DW3000 red eval-shield boards. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK) ;

    /* Configure DW IC. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device. */
    if (dwt_configure(&config)) {
        LOG_INF("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Loop forever receiving frames. */
    while (TRUE)
    {
        /* TESTING BREAKPOINT LOCATION #1 */

        /* Clear local RX buffer to avoid having leftovers from previous 
         * receptions.
         * This is not necessary but is included here to aid reading
         * the RX buffer.
         * This is a good place to put a breakpoint. Here (after first time 
         * through the loop) the local status register will be set for last 
         * event and if a good receive has happened the data buffer will have 
         * the data in it, and frame_len will be set to the length of the 
         * RX frame. */
        memset(rx_buffer, 0, sizeof(rx_buffer));

        /* Activate reception immediately. See NOTE 2 below. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received or an error/timeout occurs.
         * See NOTE 3 below.
         * STATUS register is 5 bytes long but, as the event we are looking at 
         * is in the first byte of the register, we can use this simplest API
         * function to access it. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
        { /* spin */ };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {
            /* A frame has been received, copy it to our local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
            if (frame_len <= FRAME_LEN_MAX) {
                /* No need to read the CRC. This example uses CRC */
                //if (USING_CRC_LEN) frame_len -= FCS_LEN; 

                /* No need to read the FCS/CRC. */
                dwt_readrxdata(rx_buffer, frame_len - FCS_LEN, 0);
            }

            /*
             * Need to check the STS has been received and is good. -- 
             * this will always be true in this example as companion example 1g
             * is sending STS with SDC - using same deterministic code.
             */
            if (((goodCPQ = dwt_readstsquality(&cpqual)) >= 0) &&
                (dwt_readstsstatus(&cpStatus, 0) == DWT_SUCCESS)) {
                LOG_INF("STS is GOOD");
            }
            else {
                LOG_ERR("STS qual/status FAIL");
            }

            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
        }
        else {
            /* Clear RX error events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW IC supports an extended
 *    frame length (up to 1023 bytes long) mode which is not used in this example.
 * 2. Manual reception activation is performed here but DW IC offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 3. We use polled mode of operation here to keep the example as simple as possible, but RXFCG and error/timeout status events can be used to generate
 *    interrupts. Please refer to DW IC User Manual for more details on "interrupts".
 * 4. This example code functions in the same manner as the simple_rx.c test code, however instead of using no STS, it uses the new 4z STS
 *    that was introduced in IEEE 802.15.4z
 * 5. Since this example is using STS, it will be using one of the newer packet formats that were introduced in IEEE 802.15.4z.
 *    It will use packet configuration 1 which looks like:
 *    ---------------------------------------------------
 *    | Ipatov Preamble | SFD | STS | PHR | PHY Payload |
 *    ---------------------------------------------------
 *    Since this example is for test purposes only and not meant to illustrate a working use case, we will be sending unencrypted data in the PHY
 *    Payload to the receiver device. This is obviously not recommended in a real use case as it is not a very secure format of data transmission.
 *    However, it is useful to illustrate how a transmitter and receiver will work with one and other at a basic level using the STS.
 *    Also the STS will be using deterministic code thus the receiver will stay in sync with transmitter even in case of missed frames/errored frames.
 *    There are more realistic examples in the code base that utilise STS, ranging and encrypted data payloads for a more complete solution.
 ****************************************************************************************************************************************************/
