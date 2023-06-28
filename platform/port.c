/*! ----------------------------------------------------------------------------
 * @file    port.c
 * @brief   HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2016 (c) DecaWave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 * Copyright 2021 (c) Callender-Consulting, LLC
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include "port.h"
#include "deca_device_api.h"
#include "deca_spi.h"

// zephyr includes
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <soc.h>

#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(port);


/****************************************************************************//**
 *
 *                              DeviceTree information
 *
 *******************************************************************************/

#define IRQ_GPIO_PORT      DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_irq_gpios, 0))
#define IRQ_GPIO_PIN       DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_irq_gpios, pin)
#define IRQ_GPIO_FLAGS     DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_irq_gpios, flags)

#define WAKEUP_GPIO_PORT   DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_wakeup_gpios, 0))
#define WAKEUP_GPIO_PIN    DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_wakeup_gpios, pin)
#define WAKEUP_GPIO_FLAGS  DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_wakeup_gpios, flags)

#define RESET_GPIO_PORT    DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_reset_gpios, 0))
#define RESET_GPIO_PIN     DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_reset_gpios, pin)
#define RESET_GPIO_FLAGS   DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_reset_gpios, flags)

#define RX_LED_GPIO_PORT   DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_rx_led_gpios, 0))
#define RX_LED_GPIO_PIN    DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_rx_led_gpios, pin)
#define RX_LED_GPIO_FLAGS  DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_rx_led_gpios, flags)

#define TX_LED_GPIO_PORT   DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_tx_led_gpios, 0))
#define TX_LED_GPIO_PIN    DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_tx_led_gpios, pin)
#define TX_LED_GPIO_FLAGS  DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_tx_led_gpios, flags)

#define IRQ_GPIO_PORT      DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_irq_gpios, 0))
#define IRQ_GPIO_PIN       DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_irq_gpios, pin)
#define IRQ_GPIO_FLAGS     DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_irq_gpios, flags)

#define POL_GPIO_PORT      DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_spi_pol_gpios, 0))
#define POL_GPIO_PIN       DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_spi_pol_gpios, pin)
#define POL_GPIO_FLAGS     DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_spi_pol_gpios, flags)

#define PHA_GPIO_PORT      DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_INST(0, qorvo_dwm3000), dwm_spi_pha_gpios, 0))
#define PHA_GPIO_PIN       DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_spi_pha_gpios, pin)
#define PHA_GPIO_FLAGS     DT_PHA(DT_INST(0, qorvo_dwm3000), dwm_spi_pha_gpios, flags)

/****************************************************************************//**
 *
 *******************************************************************************/

static struct gpio_callback gpio_cb;

static const struct device * wakeup_dev = NULL;
static const struct device * reset_dev  = NULL;
static const struct device * rx_led_dev = NULL;
static const struct device * tx_led_dev = NULL;
static const struct device * irq_dev    = NULL;
static const struct device * pha_dev    = NULL;
static const struct device * pol_dev    = NULL;

/****************************************************************************//**
 *
 *                              APP global variables
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                  Port private variables and function prototypes
 *
 *******************************************************************************/
static volatile uint32_t signalResetDone;

/****************************************************************************//**
 *
 *                              Time section
 *
 *******************************************************************************/

/* @fn    portGetTickCnt
 * @brief wrapper for to read a SysTickTimer, which is incremented with
 *        CLOCKS_PER_SEC frequency.
 *        The resolution of time32_incr is usually 1/1000 sec.
 * */
unsigned long
portGetTickCnt(void)
{
    //TODO
    return 0;
}

/* @fn    Sleep
 * @brief Sleep delay in ms using SysTick timer
 * */
void Sleep(uint32_t x)
{
    k_msleep(x);
}

/****************************************************************************//**
 *
 *                              END OF Time section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                              Configuration section
 *
 *******************************************************************************/

/* @fn    peripherals_init
 * */
int peripherals_init (void)
{
    /* Wakeup */
    LOG_INF("Configure WAKEUP pin on port \"%s\" pin %d", WAKEUP_GPIO_PORT, WAKEUP_GPIO_PIN);
    wakeup_dev = device_get_binding(WAKEUP_GPIO_PORT);
    if (!wakeup_dev) {
        LOG_ERR("error: \"%s\" not found", WAKEUP_GPIO_PORT);
        return -1;
    }
    gpio_pin_configure(wakeup_dev, WAKEUP_GPIO_PIN, GPIO_OUTPUT);
    gpio_pin_set(wakeup_dev, WAKEUP_GPIO_PIN, 1);

    /* Reset */
    LOG_INF("Configure RESET pin on port \"%s\" pin %d", RESET_GPIO_PORT, RESET_GPIO_PIN);
    reset_dev = device_get_binding(RESET_GPIO_PORT);
    if (!reset_dev) {
        LOG_ERR("error: \"%s\" not found", RESET_GPIO_PORT);
        return -1;
    }
    gpio_pin_configure(reset_dev, RESET_GPIO_PIN, GPIO_OUTPUT);
    gpio_pin_set(reset_dev, RESET_GPIO_PIN, 1);

    /* RX LED */
    LOG_INF("Configure RX LED pin on port \"%s\" pin %d", RX_LED_GPIO_PORT, RX_LED_GPIO_PIN);
    rx_led_dev = device_get_binding(RX_LED_GPIO_PORT);
    if (!rx_led_dev) {
        LOG_ERR("error: \"%s\" not found", RX_LED_GPIO_PORT);
        return -1;
    }
    gpio_pin_configure(rx_led_dev, RX_LED_GPIO_PIN, GPIO_OUTPUT);
    gpio_pin_set(rx_led_dev, RX_LED_GPIO_PIN, 1);

    /* TX LED */
    LOG_INF("Configure TX LED pin on port \"%s\" pin %d", TX_LED_GPIO_PORT, TX_LED_GPIO_PIN);
    tx_led_dev = device_get_binding(RX_LED_GPIO_PORT);
    if (!tx_led_dev) {
        LOG_ERR("error: \"%s\" not found", TX_LED_GPIO_PORT);
        return -1;
    }
    gpio_pin_configure(tx_led_dev, TX_LED_GPIO_PIN, GPIO_OUTPUT);
    gpio_pin_set(tx_led_dev, TX_LED_GPIO_PIN, 1);

    /* SPI POLARITY */
    LOG_INF("Configure SPI Polarity pin on port \"%s\" pin %d", POL_GPIO_PORT, POL_GPIO_PIN);
    pol_dev = device_get_binding(POL_GPIO_PORT);
    if (!pol_dev) {
        LOG_ERR("error: \"%s\" not found", POL_GPIO_PORT);
        return -1;
    }
    gpio_pin_configure(pol_dev, POL_GPIO_PIN, GPIO_OUTPUT_INACTIVE);

    /* SPI PHASE */
    LOG_INF("Configure SPI Phase pin on port \"%s\" pin %d", PHA_GPIO_PORT, PHA_GPIO_PIN);
    pha_dev = device_get_binding(PHA_GPIO_PORT);
    if (!pha_dev) {
        LOG_ERR("error: \"%s\" not found", PHA_GPIO_PORT);
        return -1;
    }
    gpio_pin_configure(pha_dev, PHA_GPIO_PIN, GPIO_OUTPUT_INACTIVE);

    return 0;
}

/* @fn    spi_peripheral_init
 * */
void spi_peripheral_init()
{
    openspi();
}

/****************************************************************************//**
 *
 *                          End of configuration section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                          DW3000 port section
 *
 *******************************************************************************/

/* @fn      reset_DWIC
 * @brief   DW_RESET pin on DW3000 has 2 functions
 *          In general it is output, but it also can be used to reset the digital
 *          part of DW3000 by driving this pin low.
 *          Note, the DW_RESET pin should not be driven high externally.
 * */
void reset_DWIC(void)
{
    LOG_INF("%s", __func__);

#if 0
    /*
     * Use RSTn gpio pin to reset DWM3000.
     */

    /* Enable GPIO used for DW3000 reset as open collector output */
    gpio_pin_configure(reset_dev, RESET_GPIO_PIN, (GPIO_OUTPUT | GPIO_OPEN_DRAIN));

    /* Drive the RSTn pin low */
    gpio_pin_set(reset_dev, RESET_GPIO_PIN, 0);

    deca_usleep(10);

    /* Drive the RSTn pin high */
    gpio_pin_set(reset_dev, RESET_GPIO_PIN, 1);

    /* Put the pin back to output open-drain (not active) */
    setup_DW3000RSTnIRQ(0);

    Sleep(2);
#else
    /*
     *  Use Soft Reset api to reset DWM3000
     *  SPI bus must be <= 7MHz, e.g. slowrate
     */
    port_set_dw_ic_spi_slowrate();

    dwt_softreset();

    /* Set SPI bus to working rate: fastrate. */
    port_set_dw_ic_spi_fastrate();
#endif

}

/* @fn      setup_DW3000RSTnIRQ
 * @brief   setup the DW_RESET pin mode
 *          0 - output Open collector mode
 *          !0 - input mode with connected EXTI0 IRQ
 * */
void setup_DW3000RSTnIRQ(int enable)
{
    if (enable) {
        /* Enable GPIO used as DECA RESET for interrupt */
        gpio_pin_configure(reset_dev, RESET_GPIO_PIN, (GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INT_EDGE_RISING));
    }
    else {
        /* Put the pin back to tri-state, as output open-drain (not active) */
        gpio_pin_configure(reset_dev, RESET_GPIO_PIN, (GPIO_OUTPUT | GPIO_OPEN_DRAIN));
    }
}

/*
 * @fn wakeup_device_with_io()
 *
 * @brief This function wakes up the device by toggling io with a delay.
 *
 * input None
 *
 * output -None
 *
 */
 void wakeup_device_with_io(void)
{
    gpio_pin_set(wakeup_dev, WAKEUP_GPIO_PIN, 1);
    deca_usleep(500);
    gpio_pin_set(wakeup_dev, WAKEUP_GPIO_PIN, 0);
}

/*
 * @fn make_very_short_wakeup_io()
 *
 * @brief This will toggle the wakeup pin for a very short time. The device should not wakeup
 *
 * input None
 *
 * output -None
 *
 */
void make_very_short_wakeup_io(void)
{
    uint8_t   cnt;

    gpio_pin_set(wakeup_dev, WAKEUP_GPIO_PIN, 1);
    for (cnt=0; cnt<10; cnt++)  __NOP();
    gpio_pin_set(wakeup_dev, WAKEUP_GPIO_PIN, 0);
}

/* @fn      led_off
 * @brief   switch off the led from led_t enumeration
 * */
void led_off (uint32_t led)
{
    switch (led) {
        case 0:
            gpio_pin_set(rx_led_dev, RX_LED_GPIO_PIN, 0);
            break;
        case 1:
            gpio_pin_set(tx_led_dev, TX_LED_GPIO_PIN, 0);
            break;
        default:
            // do nothing for undefined led number
            break;
    }
}

/* @fn      led_on
 * @brief   switch on the led from led_t enumeration
 * */
void led_on (uint32_t led)
{
    switch (led) {
        case 0:
            gpio_pin_set(rx_led_dev, RX_LED_GPIO_PIN, 1);
            break;
        case 1:
            gpio_pin_set(tx_led_dev, TX_LED_GPIO_PIN, 1);
            break;
        default:
            // do nothing for undefined led number
            break;
    }
}


/* @fn      port_wakeup_dw3000
 * @brief   "slow" waking up of DW3000 using DW_CS only
 * */
void port_wakeup_dw3000(void)
{
    gpio_pin_set(wakeup_dev, WAKEUP_GPIO_PIN, 0);
    //TODO
}

/* @fn      port_wakeup_dw3000_fast
 * @brief   waking up of DW3000 using DW_CS and DW_RESET pins.
 *          The DW_RESET signalling that the DW3000 is in the INIT state.
 *          the total fast wakeup takes ~2.2ms and depends on crystal startup time
 * */
void port_wakeup_dw3000_fast(void)
{
    //TODO
}

/* @fn      port_set_dw_ic_spi_slowrate
 * @brief   set 2MHz
 * */
void port_set_dw_ic_spi_slowrate(void)
{
    set_spi_speed_slow();
}

/* @fn      port_set_dw_ic_spi_fastrate
 * @brief   set 8MHz
 * */
void port_set_dw_ic_spi_fastrate(void)
{
    //TODO
    set_spi_speed_fast();
}


/****************************************************************************//**
 *
 *                          End APP port section
 *
 *******************************************************************************/


/****************************************************************************//**
 *
 *                              IRQ section
 *
 *******************************************************************************/

/* DW3000 IRQ handler definition. */

/*! ---------------------------------------------------------------------------
 * @fn port_set_dwic_isr()
 *
 * @brief This function is used to install the handling function for DW3000 IRQ.
 *
 * NOTE:
 *
 * @param deca_isr function pointer to DW3000 interrupt handler to install
 *
 * @return none
 */
void port_set_dwic_isr(port_deca_isr_t deca_isr)
{
    if (irq_dev == NULL) {
        LOG_INF("Configure IRQ on port \"%s\" pin %d", IRQ_GPIO_PORT, IRQ_GPIO_PIN);
        irq_dev = device_get_binding(IRQ_GPIO_PORT);
        if (!irq_dev) {
            LOG_ERR("error: \"%s\" not found", IRQ_GPIO_PORT);
            return;
        }
    }

    LOG_INF("Configure IRQ pin");

    /* Decawave interrupt */
    gpio_pin_configure(irq_dev, IRQ_GPIO_PIN, (GPIO_INPUT | IRQ_GPIO_FLAGS));

    gpio_init_callback(&gpio_cb, (gpio_callback_handler_t)(deca_isr), BIT(IRQ_GPIO_PIN));

    gpio_add_callback(irq_dev, &gpio_cb);

    gpio_pin_interrupt_configure(irq_dev, IRQ_GPIO_PIN, GPIO_INT_EDGE_RISING);
}

/****************************************************************************//**
 *
 *******************************************************************************/
