/*! ----------------------------------------------------------------------------
 *  @file    rx_with_xtal_trim.c
 *  @brief   RX with Crystal Trim example code
 *
 *  This is a simple code example of a receiver that measures the clock offset of a remote transmitter
 *  and then uses the XTAL trimming function to modify the local clock to achieve a target clock offset.
 *  Note: To keep a system stable it is recommended to only adjust trimming at one end of a link.
 *
 *  @attention
 *
 *  Copyright 2019 - 2020 (c) Decawave Ltd, Dublin, Ireland.
 *  Copyright 2021 (c) Callender-Consulting, LLC  (port to Zephyr)
 *
 *  All rights reserved.
 *
 *  @author Decawave
 */
#include <string.h>
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_spi.h>
#include <port.h>
#include <shared_defines.h>

#include <math.h>

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rx_with_xtal_trim);

/* Example application name */
#define APP_NAME "RX TRIM v1.0"

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
    DWT_STS_LEN_64,/* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* Holds a Current Crystal Trimming Value, so that it can be examined 
 * at a debug breakpoint. */
static int8_t uCurrentTrim_val;

/*
 * In this example, the crystal on the receiver will be trimmed to have a fixed
 * offset with respect to the transmitter's crystal not less than 
 * TARGET_XTAL_OFFSET_VALUE_PPM_MIN and not more than 
 * TARGET_XTAL_OFFSET_VALUE_PPM_MAX
 *
 * Note. For correct operation of the code, the min and max 
 * TARGET_XTAL_OFFSET_VALUE constants specified below should use positive
 * numbers, and the separation between min and max needs to be bigger 
 * than the trimming resolution, (which is approx 1.5 PPM).
 * We recommend that (max-min >= 2).
 *
 * */
# define TARGET_XTAL_OFFSET_VALUE_PPM_MIN    (2.0f)
# define TARGET_XTAL_OFFSET_VALUE_PPM_MAX    (4.0f)

/* The FS_XTALT_MAX_VAL defined the maximum value of the trimming value */
#define FS_XTALT_MAX_VAL    (XTAL_TRIM_BIT_MASK)

/* The typical trimming range (with 4.7pF external caps is 
 * ~77ppm (-65ppm to +12ppm) over all steps, see DW3000 Datasheet */
#define AVG_TRIM_PER_PPM    ((FS_XTALT_MAX_VAL+1)/77.0f)

/**
 * Application entry point.
 */
int app_main(void)
{
    uint8_t    rx_buffer[FRAME_LEN_MAX]; /* Buffer to store received frame. 
                                          * See NOTE 1 below. */
    uint16_t   frame_len;
    uint32_t   status_reg;

    /* Display application name. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    /* Target specific drive of RSTn line into DW IC low for a period. */
    reset_DWIC(); 

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC, 
     * or could wait for SPIRDY event) */
    Sleep(2); 

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Enabling LEDs here for debug so that for each RX-enable the D2 LED will 
     * flash on DW3000 red eval-shield boards. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK) ;

    /* Configure DW IC. */
    /* If the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Read the initial crystal trimming value. This needs to be done after 
     * dwt_initialise(), which sets up initial trimming code.*/
    uCurrentTrim_val = dwt_getxtaltrim();

    /* Loop forever receiving frames. */
    while (1)
    {
        /* Clear local RX buffer to avoid having leftovers from previous 
         * receptions.  
         * This is not necessary but is included here to aid reading the 
         * RX buffer.
         * This is a good place to put a breakpoint. Here (after first time 
         * through the loop) the local status register will be set for last 
         * event and if a good receive has happened the data buffer will have 
         * the data in it, and frame_len will be set to the length of the 
         * RX frame. */
        for (int i = 0; i < FRAME_LEN_MAX; i++ ) {
            rx_buffer[i] = 0;
        }

        /* Activate reception immediately. See NOTE 2 below. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received or an 
         * error/timeout occurs. See NOTE 3 below.
         * STATUS register is 5 bytes long but, as the event we are looking at 
         * is in the first byte of the register, we can use this simplest API
         * function to access it.
         */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {
            /* Following code block is the example of reading received frame 
             * to the rx_buffer.
             * While this is not necessary to show the clock offset adjustment 
             * algorithm, in a real implementation it is obviously important
             * to read the RX frame and validate it is from the expected 
             * source node whose crystal we want to track.
             */
            {
                /* A frame has been received, copy it to our local buffer. */
                frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
                if (frame_len <= FRAME_LEN_MAX) {
                    dwt_readrxdata(rx_buffer, frame_len, 0);
                }
            }

            /* Following code block is the example of Crystal Trimming using
             * carrier integrator value.
             * In a real application it would be important to check that the 
             * message is from correct sender before we trim our crystal to 
             * follow its clock.
             */
            {
                float    xtalOffset_ppm;

                /* Now we read the carrier frequency offset of the remote 
                 * transmitter, and convert to Parts Per Million units (ppm).
                 * A positive value means the local RX clock is running 
                 * faster than the remote transmitter's clock.
                 * For a valid result the clock offset should be read before 
                 * the receiver is re-enabled.
                 */
                xtalOffset_ppm = (float)((dwt_readclockoffset()) * CLOCK_OFFSET_PPM_TO_RATIO * 1e6);

                /* TESTING BREAKPOINT LOCATION #1 */

                /* Example of crystal trimming to be in the range
                 * (TARGET_XTAL_OFFSET_VALUE_PPM_MIN..TARGET_XTAL_OFFSET_VALUE_PPM_MAX) 
                 * out of the transmitter's crystal frequency.
                 * This may be used in application, which require small offset 
                 * to be present between ranging sides.
                 */
                if ((float)fabs(xtalOffset_ppm) > (float)TARGET_XTAL_OFFSET_VALUE_PPM_MAX ||
                    (float)fabs(xtalOffset_ppm) < (float)TARGET_XTAL_OFFSET_VALUE_PPM_MIN) {

                    uCurrentTrim_val -= ((TARGET_XTAL_OFFSET_VALUE_PPM_MAX + 
                                          TARGET_XTAL_OFFSET_VALUE_PPM_MIN)/2 +
                                          xtalOffset_ppm) * AVG_TRIM_PER_PPM;
                    uCurrentTrim_val &= FS_XTALT_MAX_VAL;

                    /* Configure new Crystal Offset value */
                    dwt_setxtaltrim(uCurrentTrim_val);
                }
            }

            /* Clear good RX frame event in the DW3000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
        }
        else {
            /* Clear RX error events in the DW3000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW3000 supports an extended
 *    frame length (up to 1023 bytes long) mode which is not used in this example.
 * 2. Manual reception activation is performed here but DW3000 offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, etc.).
 * 3. We use polled mode of operation here to keep the example as simple as possible, but RXFCG and error/timeout status events can be used to generate
 *    interrupts. Please refer to DW3000 User Manual for more details on "interrupts".
 ****************************************************************************************************************************************************/
