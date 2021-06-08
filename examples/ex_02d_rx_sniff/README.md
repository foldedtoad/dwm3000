# DWM3000 - ex_02d_rx_sniff

## Overview

## Requirements

## Building and Running

## Sample Output
```
*** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

[00:00:06.338,714] <inf> main: main_thread
[00:00:06.338,714] <inf> port: Configure WAKEUP pin
[00:00:06.338,745] <inf> port: Configure RESET pin
[00:00:06.338,745] <inf> port: Configure RX LED pin
[00:00:06.338,745] <inf> port: Configure TX LED pin
[00:00:06.338,745] <inf> deca_spi: openspi bus SPI_3
[00:00:07.366,546] <inf> simple_tx: RX SNIFF v1.0
[00:00:07.366,607] <inf> port: reset_DWIC
[00:00:07.371,246] <inf> deca_device: dev_id "deca0302"
[00:00:07.392,395] <inf> simple_tx: Frame
                                    c5 0e 44 45 43 41 57 41  56 45 b0 78             |..DECAWA VE.x
[00:00:07.919,494] <inf> simple_tx: Frame
                                    c5 0f 44 45 43 41 57 41  56 45 4d 35             |..DECAWA VEM5
[00:00:08.420,013] <inf> simple_tx: Frame
                                    c5 10 44 45 43 41 57 41  56 45 f1 5c             |..DECAWA VE.\
[00:00:08.947,143] <inf> simple_tx: Frame
                                    c5 11 44 45 43 41 57 41  56 45 0c 11             |..DECAWA VE..
[00:00:09.447,662] <inf> simple_tx: Frame
                                    c5 12 44 45 43 41 57 41  56 45 0b c7             |..DECAWA VE..
[00:00:09.974,792] <inf> simple_tx: Frame
                                    c5 13 44 45 43 41 57 41  56 45 f6 8a             |..DECAWA VE..
[00:00:10.475,311] <inf> simple_tx: Frame
                                    c5 14 44 45 43 41 57 41  56 45 14 63             |..DECAWA VE.c
[00:00:11.002,471] <inf> simple_tx: Frame
                                    c5 15 44 45 43 41 57 41  56 45 e9 2e             |..DECAWA VE..
[00:00:11.502,990] <inf> simple_tx: Frame
                                    c5 16 44 45 43 41 57 41  56 45 ee f8             |..DECAWA VE..
```

The LEDs are not used: no LED indication of receiving a frame.
