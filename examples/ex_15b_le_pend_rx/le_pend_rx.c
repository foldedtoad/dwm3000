/*! ----------------------------------------------------------------------------
 *  @file    le_pend_rx.c
 *  @brief   Sets data pending bit in ack message if correct criteria is met.
 *           The data pending bit will be set if the following criteria is met:
 *           1. The frame received is a MAC command frame.
 *           2. The address of the device sending the MAC command frame matches
 *           one of the four 16-bit addresses programmed into LE_PEND01 or
 *           LE_PEND23 registers and the data pending bits are set in FF_CFG
 *           register: LE0_PEND – LE3_PEND.
 *           3.The address of the device sending the MAC command is 16-bits and SSADRAPE bit is set.
 *           4.The address of the device sending the MAC command is 64-bits and LSADRAPE bit is set.
 *           5.Security bit is not set in Frame Control and frame version is 0 or 1.
 * @attention
 *
 * Copyright 2020 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2021 (c) Callender-Consulting, LLC  (port to Zephyr)
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include <string.h>
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
LOG_MODULE_REGISTER(le_pend_rx);

/* Example application name */
#define APP_NAME "LE PEND RX v1.0"

#define PAN_ID      0xDECA
#define SHORT_ADDR  0x5258 /* "RX" */
#define SRC_ADDR    0x5854 /* "XT" this is the source address value sent by 
                            * the transmitter in ex LE PEND TX */

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
    DWT_STS_LEN_64,  /* STS length see allowed values in Enum dwt_sts_lengths_e */
    DWT_PDOA_M0      /* PDOA mode off */
};

/* Buffer to store received frame. See NOTE 2 below. */
static uint8_t rx_buffer[FRAME_LEN_MAX];


/**
 * Application entry point.
 */
int app_main(void)
{
    uint32_t status_reg;

    /* Hold copy of frame length of frame received (if good) so that 
     * it can be examined at a debug breakpoint. */
    uint32_t frame_len;

    /* Display application name. */
    LOG_INF(APP_NAME);

    /* SPI rate, DW IC supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC();

    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Enabling LEDs here for debug so that for each RX-enable the D2 LED will
     * flash on DW3000 red eval-shield boards. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    /* Configure DW IC. */
    if (dwt_configure(&config)) {
        LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Set PAN ID and short address. See NOTE 1 below. */
    dwt_setpanid(PAN_ID);
    dwt_setaddress16(SHORT_ADDR);

    /* Enable auto ack*/
    dwt_enableautoack(0,1);

    /* Loop forever receiving frames. */
    while (1) {

        /* filter selection */
        dwt_configureframefilter(DWT_FF_ENABLE_802_15_4, DWT_FF_MAC_LE2_EN); 

        /* Address value "XT" to be written to desired LE address register. */
        dwt_configure_le_address(SRC_ADDR, LE2);

        /* Clear local RX buffer to avoid having leftovers from previous 
         * receptions,  This is not necessary but is included here to aid 
         * reading the RX buffer.
         * This is a good place to put a breakpoint. Here (after first time 
         * through the loop) the local status register will be set for last 
         * event and if a good receive has happened the data buffer will have 
         * the data in it, and frame_len will be set to the length of the RX 
         * frame. */
        memset(rx_buffer,0,sizeof(rx_buffer));

        /* Activate reception immediately. See NOTE 4 below. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received or an error/timeout occurs. 
         * See NOTE 5 below.
         * STATUS register is 5 bytes long but, as the event we are looking 
         * at is in the first byte of the register, we can use this simplest 
         * API function to access it. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | 
                                                                   SYS_STATUS_ALL_RX_TO | 
                                                                   SYS_STATUS_ALL_RX_ERR)))
        { /* spin */ };

        if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {

            /* A frame has been received, copy it to our local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_BIT_MASK;
            if (frame_len <= FRAME_LEN_MAX) {

                /* No need to read the FCS/CRC. */
                dwt_readrxdata(rx_buffer, frame_len - FCS_LEN, 0); 
            }

            /* Clear good RX frame event in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            /* Poll DW IC until confirmation of transmission of the ACK frame. */
            while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS_BIT_MASK))
            { /* spin */ };

            /* Clear TXFRS event. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
        }
        else {
            /* Clear RX error/timeout events in the DW IC status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. PAN ID and short address are hard coded constants to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW IC unique ID by combining the Lot ID & Part Number values programmed into the DW IC
 *    during its manufacture. However there is no guarantee this will not conflict with someone else’s implementation. We recommended that customers
 *    buy a block of addresses from the IEEE Registration Authority for their production items.
 * 2. In this example, maximum frame length is set to 127 bytes which is 802.15.4z UWB standard maximum frame length. DW IC supports an extended frame
 *    length (up to 1023 bytes long) mode which is not used in this example.
 * 3. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW IC OTP memory.
 * 4. Manual reception activation is performed here but DW IC offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 5. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW IC User Manual for more details on "interrupts".
 * 6. This is the purpose of the AAT bit in DW IC's STATUS register but because of an issue with the operation of AAT, it is simpler to directly
 *    check in the frame control if the ACK request bit is set. Please refer to DW IC User Manual for more details on Auto ACK feature and the AAT
 *    bit.
 * 7. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW IC API Guide for more details on the DW IC driver functions.
 * 8. Desired configuration by user may be different to the current programmed configuration. dwt_configure is called to set desired
 *    configuration.
 ****************************************************************************************************************************************************/
