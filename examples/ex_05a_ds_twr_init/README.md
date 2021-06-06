# DWM3000 - ex_05a_ds_twr_init
Measure the distance between two host+DWS3000 boards.

## Overview
This sub-project is paired with `ex_05b_ds_twr_resp`

## Requirements
Two complete host+DWS3000 boards are needed: one Init side, and one Resp side.

## Building and Running

## Sample Output
```
    *** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

    [00:00:05.347,595] <inf> main: main_thread  
    [00:00:05.347,625] <inf> port: Configure WAKEUP pin  
    [00:00:05.347,625] <inf> port: Configure RESET pin  
    [00:00:05.347,625] <inf> port: Configure RX LED pin  
    [00:00:05.347,625] <inf> port: Configure TX LED pin  
    [00:00:05.347,656] <inf> deca_spi: openspi bus SPI_3  
    [00:00:06.375,457] <inf> ds_twr_init: DS TWR INIT v1.0  
    [00:00:06.375,488] <inf> port: reset_DWIC  
    [00:00:06.380,126] <inf> deca_device: dev_id "deca0302"  
    [00:00:06.383,666] <inf> ds_twr_init: Initiator ready  
```

In addition to the above RTT console output, the LEDs will blink.  
Red   -- Send  
Green -- Receive  
