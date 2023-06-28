/*! ----------------------------------------------------------------------------
 *  @file    otp_write.c
 *  @brief   This example writes to the OTP memory and check if the write was 
 *           successful.
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

#include <stdio.h>
#include <deca_device_api.h>
#include <port.h>

//zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LOG_LEVEL 3
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(opt_write);

/* Example application name and version. */
#define APP_NAME "OTP Write"

#define OTP_ADDRESS     0x50        // Address to write - OTP
#define OTP_DATA        0x87654321  // Data to write    - OTP

/**
 * Application entry point.
 */
int app_main(void)
{
    /* Display application name. */
    LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    reset_DWIC();

    Sleep(2);

    int err = dwt_otpwriteandverify(OTP_DATA, OTP_ADDRESS);

    if (err == DWT_SUCCESS) {
        LOG_INF("OTP write PASS");
    }
    else {
        LOG_ERR("OTP write FAIL");
    }

    return err;
}

/*****************************************************************************************************************************************************
 * NOTES:
 * 1. You can write only 1 time to the memory - OTP memory.
 * 2. You can write only to a specific address range (see specification/manual).
 * 3. Data size is 32bit.
 ****************************************************************************************************************************************************/
