/*! ----------------------------------------------------------------------------
 *  @file    gpio_example.c
 *  @brief   This example demonstrates how to enable DW IC GPIOs as inputs and
 *           outputs. It also demonstrates how to drive the output to turn
 *           on/off LEDs on DW3000 HW.
 *
 * @attention
 *
 * Copyright 2017 - 2020 (c) Decawave Ltd, Dublin, Ireland.
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
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ack_data_rx);

/* Example application name and version. */
#define APP_NAME "GPIO v1.0"

#define SLOW_BLINK  500 // 500ms between blinks. When GPIO0 = '0'
#define FAST_BLINK  100 // 100ms between blinks. When GPIO0 = '1'

/* Enable all GPIOs (See MFIO_MODE register) */
/* Configure all GPIOs as inputs */
#define ENABLE_ALL_GPIOS_MASK 0x200000

/* Set GPIOs 2 & 3 as outputs (See GPIO_DIR register) */
#define SET_OUTPUT_GPIO2_GPIO3 0xFFF3

/**
 * Application entry point.
 */
int app_main(void)
{
    uint16_t blink_delay;
    uint16_t read_ios_val;

    /* Display application name on LCD. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC();

    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { /* spin */ };

    if (dwt_initialise(DWT_DW_IDLE) == DWT_ERROR) {
        LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* See NOTE 1. */
    /* 1st enable GPIO clocks */
    dwt_enablegpioclocks();

    /* See NOTE 2.
     * At this point we need to adjust the MFIO_MODE register to change the
     * mode of the GPIO pins (GPIO, LED, etc.). Note that GPIO_4, GPIO_5, and
     * GPIO_6 are already set to GPIO by default. */

    /* Set mode to GPIOs */
    dwt_write32bitoffsetreg(GPIO_MODE_ID, 0, ENABLE_ALL_GPIOS_MASK);

    /* Set output level for output pin to low. */
    dwt_write16bitoffsetreg(GPIO_OUT_ID, 0, 0x0);

    /* Set GPIOs 2 & 3 as outputs and all other GPIOs as input */
    dwt_write16bitoffsetreg(GPIO_DIR_ID, 0, SET_OUTPUT_GPIO2_GPIO3);

    /* This function will loop around forever turning the GPIOs controlling
     * the LEDs on and off */

    /* The blink rate will be set according to the GPIO0 read level */
    while (1) {

        read_ios_val = dwt_read16bitoffsetreg(GPIO_RAW_ID, 0);

        /* Checks if GPIO0 input is high */
        if (read_ios_val & GPIO_RAW_GRAWP0_BIT_MASK) {
            blink_delay = FAST_BLINK;
        }
        else {
            blink_delay = SLOW_BLINK;
        }

        /* Set GPIO2 and GPIO3 high */
        /* This will turn D1 (Green LED) and D2 (Red LED) on */
        dwt_or16bitoffsetreg(GPIO_OUT_ID, 0, (GPIO_OUT_GOP3_BIT_MASK |
                                              GPIO_OUT_GOP2_BIT_MASK));

        Sleep(blink_delay);

        /* set GPIO2 & GPIO3 low (LEDs will be turned off) */
        /* Clear bits 2,3 */
        dwt_and16bitoffsetreg(GPIO_OUT_ID, 0, (uint16_t)(~(GPIO_OUT_GOP3_BIT_MASK |
                                                           GPIO_OUT_GOP2_BIT_MASK)));
        Sleep(blink_delay);
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. When enabling the GPIO mode/value, the GPIO clock needs to be enabled.
 *
 * 2. By default, all the available GPIO pins on the DW3000 B0 red evaluation boards (GPIO0, GPIO1, GPIO2, GPIO3 & GPIO4) are set to work as GPIO.
 *    Other modes available are LED, AOA_SW, DBG_MUX, etc. Please see MFIO_MODE register details in datasheet for more information on this. You can
 *    see examples of how to set this register in functions like dwt_setleds().
 *
 * 3. The code above focuses mostly on how to test the GPIO outputs of the DW3000 chip. However, there is some code commented out that illustrates
 *    how to also test the input functionality of the GPIOs. On the DW3000 HW (Decawave shield / red board), there are a number of test points
 *    that are linked to the GPIO pins. In total, there are 9 GPIO pins available on the DW3000. However, only 5 of them are wired to test points on
 *    the Decawave shield / red board. The details of these GPIO pins are as follows:
 *    - GPIO0 - Test Point 3 (TP3)
 *    - GPIO1 - Test Point 4 (TP4)
 *    - GPIO2 - Test Point 6 (TP6) - Also controls RX LED (Green)
 *    - GPIO3 - Test Point 7 (TP7) - Also controls TX LED (Red)
 *    - GPIO4 - Test Point 8 (TP8)
 *    To use these GPIO pins, you can solder connections to your circuit to the test points on the Decawave Shield / red board. The simple GPIO input
 *    code example that is commented out above can be used if there are wires soldered to TP3 and TP4. These wires can be used as 'switches' to pull
 *    them high and act as an input on those GPIO input pins. This then controls the flashing frequency of the LEDs.
 *
 ****************************************************************************************************************************************************/
