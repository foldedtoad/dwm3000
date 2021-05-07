/*! ----------------------------------------------------------------------------
 * @file    deca_spi.c
 * @brief   SPI access functions
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 * Copyright 2019 (c) Callender-Consulting LLC.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include "deca_spi.h"
#include "deca_device_api.h"
#include "port.h"

#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(deca_spi);

const struct device * spi;
struct spi_config * spi_cfg;
struct spi_config   spi_cfgs [4] = {0};

#define SPI_CFGS_COUNT ((sizeof(spi_cfgs)/sizeof(spi_cfgs[0])))

uint8_t tx_buf [255];
uint8_t rx_buf [255];

struct spi_buf bufs [2];

struct spi_buf_set tx; 
struct spi_buf_set rx;

static struct spi_cs_control cs_ctrl;

/*
 *****************************************************************************
 *
 *                              DeviceTree Information
 *
 *****************************************************************************
 */

#define DWM_SPI      DT_PARENT(DT_INST(0, qorvo_dwm3000))
#define DWM_CS_GPIO  DT_PHANDLE_BY_IDX(DT_PARENT(DT_INST(0, qorvo_dwm3000)), cs_gpios, 0)
#define DWM_CS_PIN   DT_PHA(DWM_SPI, cs_gpios, pin)
#define DWM_CS_FLAGS DT_PHA(DWM_SPI, cs_gpios, flags)

/*
 *****************************************************************************
 *
 *                              DW3000 SPI section
 *
 *****************************************************************************
 */

/*
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi(void)
{
    LOG_INF("%s bus %s", __func__, DT_LABEL(DWM_SPI));

    /* Propagate CS config into all spi_cfgs[] elements */
    cs_ctrl.gpio_dev = device_get_binding(DT_LABEL(DWM_CS_GPIO));
    if (!cs_ctrl.gpio_dev) {
        LOG_ERR("%s: GPIO binding failed.\n", __func__);
        return -1;
    }
    cs_ctrl.gpio_pin = DWM_CS_PIN;
    cs_ctrl.delay = 0U;
    cs_ctrl.gpio_dt_flags = DWM_CS_FLAGS;
    for (int i=0; i < SPI_CFGS_COUNT; i++) {
        spi_cfgs[i].cs = &cs_ctrl;
    }

    gpio_pin_set(cs_ctrl.gpio_dev, DWM_CS_PIN, 1);

    spi_cfg = &spi_cfgs[0];

    spi = device_get_binding(DT_LABEL(DWM_SPI));
    if (!spi) {
        LOG_ERR("%s: SPI binding failed.\n", __func__);
        return -1;
    }
    spi_cfg->operation = SPI_WORD_SET(8);
    spi_cfg->frequency = 2000000;

    memset(&tx_buf[0], 0, 255);
    memset(&rx_buf[0], 0, 255);
    bufs[0].buf = &tx_buf[0];
    bufs[1].buf = &rx_buf[0];
    tx.buffers = &bufs[0];
    rx.buffers = &bufs[1];
    tx.count = 1;
    rx.count = 1;

    return 0;
}

void set_spi_speed_slow(void)
{
    spi_cfg = &spi_cfgs[0];
    spi_cfg->operation = SPI_WORD_SET(8); // | SPI_MODE_CPHA;  // SPI mode(0,1)
    spi_cfg->frequency = 2000000;

    memset(&tx_buf[0], 0, 255);
    memset(&rx_buf[0], 0, 255);
}

void set_spi_speed_fast(void)
{
    spi_cfg = &spi_cfgs[1];
    spi_cfg->operation = SPI_WORD_SET(8); // | SPI_MODE_CPHA;  // SPI mode(0,1)
    spi_cfg->frequency = 8000000;

    memset(&tx_buf[0], 0, 255);
    memset(&rx_buf[0], 0, 255);
}

/*
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
int closespi(void)
{
    //TODO
    return 0;
}

/*
 * Function: writetospiwithcrc()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success
 */
int writetospiwithcrc(uint16_t           headerLength,
                      const    uint8_t * headerBuffer,
                      uint16_t           bodyLength,
                      const    uint8_t * bodyBuffer,
                      uint8_t            crc8)
{
    decaIrqStatus_t  stat;
 
    uint16_t len =  headerLength + bodyLength + sizeof(crc8);

    if (len > sizeof(tx_buf))
        return -1;

    stat = decamutexon();

    memcpy(&tx_buf[0],            headerBuffer, headerLength);
    memcpy(&tx_buf[headerLength], bodyBuffer,   bodyLength);

    tx_buf[headerLength + bodyLength] = crc8;

    bufs[0].len = len;
    bufs[1].len = len;

    spi_transceive(spi, spi_cfg, &tx, &rx);

    decamutexoff(stat);

    return 0;
}

/*
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success
 */
int writetospi(uint16_t           headerLength,
               const    uint8_t * headerBuffer,
               uint16_t           bodyLength,
               const    uint8_t * bodyBuffer)
{
    decaIrqStatus_t  stat;

#if 0
    LOG_HEXDUMP_INF(headerBuffer, headerLength, "writetospi: Header");
    LOG_HEXDUMP_INF(bodyBuffer, bodyLength, "writetospi: Body");
#endif

    stat = decamutexon();

    memcpy(&tx_buf[0], headerBuffer, headerLength);
    memcpy(&tx_buf[headerLength], bodyBuffer, bodyLength);

    bufs[0].len = headerLength + bodyLength;
    bufs[1].len = headerLength + bodyLength;

    spi_transceive(spi, spi_cfg, &tx, &rx);

    decamutexoff(stat);

    return 0;
}

/*
 * Function: readfromspi()
 *
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data 
 * may be found, or returns 0
 */
int readfromspi(uint16_t        headerLength,
                const uint8_t * headerBuffer,
                uint16_t        readLength,
                uint8_t       * readBuffer)
{
    decaIrqStatus_t  stat;

    stat = decamutexon();

    memset(&tx_buf[0], 0, headerLength + readLength);
    memcpy(&tx_buf[0], headerBuffer, headerLength);

    bufs[0].len = headerLength + readLength;
    bufs[1].len = headerLength + readLength;

    spi_transceive(spi, spi_cfg, &tx, &rx);

    memcpy(readBuffer, rx_buf + headerLength, readLength);

    decamutexoff(stat);

#if 0
    LOG_HEXDUMP_INF(headerBuffer, headerLength, "readfromspi: Header");
    LOG_HEXDUMP_INF(readBuffer, readLength, "readfromspi: Body");
#endif

    return 0;
}
