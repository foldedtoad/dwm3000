# DWM3000 - ex_01a_simple_tx

## Overview

## Requirements

## Building and Running

## Sample Output
Below is output from PCA10056 (nRF52840) + DWS3000.

```
*** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

[00:00:04.104,003] <inf> main: main_thread
[00:00:04.104,034] <inf> port: Configure WAKEUP pin on port "GPIO_1" pin 11
[00:00:04.104,034] <inf> port: Configure RESET pin on port "GPIO_1" pin 8
[00:00:04.104,064] <inf> port: Configure RX LED pin on port "GPIO_1" pin 5
[00:00:04.104,064] <inf> port: Configure TX LED pin on port "GPIO_1" pin 4
[00:00:04.104,095] <inf> port: Configure SPI Phase pin on port "GPIO_1" pin 1
[00:00:04.104,095] <inf> port: Configure SPI Polarity pin on port "GPIO_1" pin 2
[00:00:04.104,095] <inf> deca_spi: openspi bus SPI_3
[00:00:05.155,212] <inf> simple_tx: SIMPLE TX v1.0
[00:00:05.155,273] <inf> port: reset_DWIC
[00:00:05.159,942] <inf> deca_device: dev_id "deca0302"
[00:00:05.163,085] <inf> simple_tx: Sending started
[00:00:05.163,116] <inf> simple_tx: len 10
                                    c5 00 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:05.663,604] <inf> simple_tx: len 10
                                    c5 01 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:06.201,507] <inf> simple_tx: len 10
                                    c5 02 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:06.702,026] <inf> simple_tx: len 10
                                    c5 03 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:07.229,156] <inf> simple_tx: len 10
                                    c5 04 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:07.729,644] <inf> simple_tx: len 10
                                    c5 05 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:08.256,744] <inf> simple_tx: len 10
                                    c5 06 44 45 43 41 57 41  56 45                   |..DECAWA VE
[00:00:08.757,263] <inf> simple_tx: len 10
                                    c5 07 44 45 43 41 57 41  56 45                   |..DECAWA VE
```

