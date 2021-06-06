# DWM3000 - ex_05b_ds_twr_resp
Measure the distance between two host+DWS3000 boards.

## Overview
This sub-project is paired with `ex_05b_ds_twr_init`

## Requirements
Two complete host+DWS3000 boards are needed: one Init side, and one Resp side.


## Building and Running

## Sample Output
```
*** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

[00:00:04.785,400] <inf> main: main_thread
[00:00:04.785,400] <inf> port: Configure WAKEUP pin
[00:00:04.785,430] <inf> port: Configure RESET pin
[00:00:04.785,430] <inf> port: Configure RX LED pin
[00:00:04.785,430] <inf> port: Configure TX LED pin
[00:00:04.785,430] <inf> deca_spi: openspi bus SPI_3
[00:00:05.813,659] <inf> dw_twr_resp: DS TWR RESP v1.0
[00:00:05.813,690] <inf> port: reset_DWIC
[00:00:05.818,389] <inf> deca_device: dev_id "deca0302"
[00:00:10.850,372] <inf> dw_twr_resp: dist 1.17 m
[00:00:11.861,206] <inf> dw_twr_resp: dist 1.31 m
[00:00:14.866,485] <inf> dw_twr_resp: dist 1.46 m
[00:00:16.869,842] <inf> dw_twr_resp: dist 0.96 m
[00:00:18.873,229] <inf> dw_twr_resp: dist 1.14 m
[00:00:20.876,617] <inf> dw_twr_resp: dist 0.23 m
[00:00:22.879,974] <inf> dw_twr_resp: dist 0.39 m
[00:00:24.883,300] <inf> dw_twr_resp: dist 0.66 m
[00:00:26.886,718] <inf> dw_twr_resp: dist 0.44 m
[00:00:28.890,045] <inf> dw_twr_resp: dist 1.37 m
[00:00:30.893,402] <inf> dw_twr_resp: dist 1.40 m
[00:00:32.896,789] <inf> dw_twr_resp: dist 1.39 m
[00:00:34.900,146] <inf> dw_twr_resp: dist 1.39 m
[00:00:36.903,472] <inf> dw_twr_resp: dist 1.33 m
[00:00:38.906,860] <inf> dw_twr_resp: dist 1.36 m
```

In addition to the above RTT console output, the LEDs will blink.  
Red   -- Send  
Green -- Receive  
