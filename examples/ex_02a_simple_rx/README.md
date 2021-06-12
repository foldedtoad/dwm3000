# DWM3000 - ex_02a_simple_rx

## Overview


## Requirements


## Building and Running


## Sample Output
Below is output from PCA10056 (nRF52840) + DWS3000.

```
*** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

[00:00:07.322,723] <inf> main: main_thread
[00:00:07.322,753] <inf> port: Configure WAKEUP pin on port "GPIO_1" pin 11
[00:00:07.322,753] <inf> port: Configure RESET pin on port "GPIO_1" pin 8
[00:00:07.322,784] <inf> port: Configure RX LED pin on port "GPIO_1" pin 5
[00:00:07.322,784] <inf> port: Configure TX LED pin on port "GPIO_1" pin 4
[00:00:07.322,784] <inf> port: Configure SPI Phase pin on port "GPIO_1" pin 1
[00:00:07.322,814] <inf> port: Configure SPI Polarity pin on port "GPIO_1" pin 2
[00:00:07.322,814] <inf> deca_spi: openspi bus SPI_3
[00:00:08.373,382] <inf> simple_rx: SIMPLE RX v1.0
[00:00:08.373,413] <inf> port: reset_DWIC
[00:00:08.378,082] <inf> deca_device: dev_id "deca0302"
[00:00:08.381,103] <inf> simple_rx: Ready to Receive
[00:00:15.248,413] <inf> simple_rx: len 10
                                    c5 00 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:15.748,931] <inf> simple_rx: len 10
                                    c5 01 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:16.282,104] <inf> simple_rx: len 10
                                    c5 02 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:16.782,653] <inf> simple_rx: len 10
                                    c5 03 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:17.309,600] <inf> simple_rx: len 10
                                    c5 04 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:17.809,814] <inf> simple_rx: len 10
                                    c5 05 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:18.337,097] <inf> simple_rx: len 10
                                    c5 06 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:18.837,463] <inf> simple_rx: len 10
                                    c5 07 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:19.364,562] <inf> simple_rx: len 10
                                    c5 08 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:19.865,112] <inf> simple_rx: len 10
                                    c5 09 44 45 43 41 57 41  56 45                   |..DECAWA VE


```
