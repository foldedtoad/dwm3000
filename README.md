# Nordic nRF52-series + Decawave DWM3000 on Zephyr v3.0

**NOTE: This set of projects require Zephyr Version 3.0.**  

This project contains firmware examples for the Decawave DWM3000-series Ultra Wideband (UWB) modules with Zephyr RTOS. It's a port of Qorvo/Decawave's SDK found on their website.

This port to Zephyr generally follows the Qorvo/Decawave [DWM3000 SDK Release V1.1](https://www.qorvo.com/products/p/DWS3000#documents). It is advised to review the SDK's content to get a general understanding of how the examples are intended to function.

## State of Project
* The combination of PCA10056 (nRF52840) + DWS3000 are not completely functional at this time.  
* The combination of PCA10040 (nRF52832) + DWS3000 are fully functional.
* The combination of Nucleo-F429ZI + DWS3000 are functional.
* Interoperation with DWM1001 boards are functional, but see the `Interoperability Between the DWM3000 Project and the DWM1001 Project` below.

### Terminology
* "Decawave" means "Qorvo/Decwave".   Decawave was recently acquiered by Qorvo.
* "DWM3000" is the Qorvo/Decawave hardware module with embedded DW3110 UWB IC chip.
* "DWS3000" is the Qorvo/Decawave DevKit which implements a DWM3000-series module on an Arduino form-factor board.
* "PCA10056" means "Nordic nRF52840DK board".
* "PCA10040" means "Nordic nRF52832DK board".
* "PCA100xx" means either the PCA10056 or PCA10040 board.

The PCA100xx boards are part of the Nordic nRF52-series DevKit products. The PCA100xx board's form-factor is compliant with the Arduino form-factor layout, thus allowing the DWS3000 shield plugged on top of the PCA100xx boards.

### Assumptions
* This project assumes some familiarity with the Zephyr RTOS.
Zephyr is relativey easy to install and learn, and there are good tutorials available which explain how to establish a working version of Zephyr on your development system.

* The `example` projects have been developed using the DWS3000 Arduino-shield plugged into the PCA10040, PCA10056 or Nucleo_F429ZI boards.

### Overview of Changes from the DWM3000 SDK
The major changes from the original Decawave project are:
* Define and use a custom shield-board DTS definition: the "DWM3000". (ToDo: Change the shield name to "DWS3000" in future.)
* Port SPI-bus access and GPIO access.
* Most of the porting effort entailed changes to the "platform" directory modules, which is where the `io` driver layer exists: SPI & GPIO.
* Little changes were made in the `decadriver` directory, which is where the "functional" driver layer for the DWM3000 exists: should be target board independent code.
* The original code had comment lines which extended well past 80 columns.  This is inconvienent for development within VMs on laptops where screen real-estate limited. So the code was reformatted to 80-column max lines.  It's just easier to read and understand: that is the point of examples, right?!

### Supported Development OSes
Linux, Mac or Windows

This project was developed in a Ubuntu 20.04 (LTS) and MacOS (Big Sur), but there is no reason these changes should work with the other OSes.
Windows OSes have not been part of the development process, but following Zephyr's instruction for Windows setup, it should not be a problem.

### Interoperability Between the DWM3000 Project and the DWM1001 Project.
The [DWM1001 Project](https://github.com/foldedtoad/dwm1001) supports the original DecaWave UWB board, the DWM1001.  
If you have a DWM1001 board and wish to interact with a DWM3000 board (DWS3000 shield) then the operating parameters must be coordinated to provide coherent send/receive interoperations.  
Currently the only know issue is the coordination of the Physical Header Mode.  
* The DWM3000 project examples default to Standard Frame mode (`DWT_PHYMODE_STD`, IEEE802.15.4 defined).  
* The DWM1001 project examples default to Long Frame mode (`DWT_PHYMODE_EXT`, IEEE802.15.8 defined).

To interoperate you must change either the DWM3000 or DWM1001 side code to match the other side.
The parameter to modify is in the `examples` subproject c source file within the `config` data definition.

### Examples
Below is a listing of the sub-projects in the `example` directory.
Most sub-projects have a `README` file which contains relavant information about that sub-project.
```
    ex_00a_reading_dev_id
    ex_01a_simple_tx
    ex_01b_tx_sleep
    ex_01c_tx_sleep_auto
    ex_01d_tx_timed_sleep
    ex_01e_tx_with_cca
    ex_01g_simple_tx_sts_sdc
    ex_01h_simple_tx_pdoa
    ex_01i_simple_tx_aes
    ex_02a_simple_rx
    ex_02c_rx_diagnostics
    ex_02d_rx_sniff
    ex_02f_rx_with_crystal_trim
    ex_02g_simple_rx_sts_sdc
    ex_02h_simple_rx_pdoa
    ex_02i_simple_rx_aes
    ex_03a_tx_wait_resp
    ex_03b_rx_send_resp
    ex_03d_tx_wait_resp_interrupts
    ex_04a_cont_wave
    ex_04b_cont_frame
    ex_05a_ds_twr_init
    ex_05b_ds_twr_resp
    ex_05c_ds_twr_init_sts_sdc
    ex_05d_ds_twr_resp_sts_sdc
    ex_06a_ss_twr_initiator
    ex_06b_ss_twr_responder
    ex_06e_aes_ss_twr_initiator
    ex_06f_aes_ss_twr_responder
    ex_07a_ack_data_tx
    ex_07b_ack_data_rx
    ex_07c_ack_data_rx_dbl_buff
    ex_11a_spi_crc
    ex_13a_gpio
    ex_14a_otp_write
    ex_15a_le_pend_tx
    ex_15b_le_pend_rx
```

### Hardware
You will need two boards: a host board and a shield board --
* Host board PCA10040 (nRF52832)
![pca10040](https://github.com/foldedtoad/dwm3000/blob/master/docs/pca10040.png)
* Host board PCA10056 (nRF52840)
![pca10056](https://github.com/foldedtoad/dwm3000/blob/master/docs/pca10056.png)
* Host board Nucleo-F429ZI (STM32F429ZI)
![nucleo](https://github.com/foldedtoad/dwm3000/blob/master/docs/nucleo_f429zi.jpg)
* DWS3000 arduino shield board.
![dws3000](https://github.com/foldedtoad/dwm3000/blob/master/docs/dws3000.jpg)

**NOTE:** The conbination of PCA100xx board + DWS3000 shield will be the reference configuration thoughout this document.

While other Zephyr-supported boards might be used, they have not been tested. Only Zephyr API calls have been used, so the code is intended to be reasonably portable to other boards: mostly target board DeviceTree defines may need to be adjusted.

A `micro-USB` cable will also be needed to connect the PCA100xx to your build system. This USB cable will allow both debugging and flashing of firmware onto the PCA100xx board. Use of Segger's Ozone debugger is recommended, but not required.

Many of the examples will require two or more PCA100xx + DWS3000 setups, such as the micro-location examples.

**WARNING:** You will need to trim the pins on the PCA100xx board's P20 connector, as they extend too far upwards and will contact pads on the bottom of the DWS3000 shield, causing electical problems.

**WARNING:** There are issues with the early "engineering versions" of the nRF52840. The chips are identified with QIAAAA marking on their top.  Apparently the SPI3 bus was not fully support with these early engineering releases. For the PCA10056, the devicetree definition for `spi3` bus is equated with the `arduino_spi` bus. Therefore, it is suggested to avoid using these particular PCA10056 boards. Production releases of the PCA10056 should work without issues.

**WARNING:** The PCA10056 board has a conflict between uart1 and the arduino-spi devicetree definitions.  This is fixed by `nrf52840dk_nrf52840.overlay`, which disables uart1's definitions, thereby allowing the arduino shield to acquire controll over the D1 and D0 lines.  These lines are used to configure DWM3000's SPI Polarity and Phase options.

**NOTE:** Because the PCA100xx board incorporates a Segger JLink debugger (on-board), it is highly recommended to install the Segger JLink package on your development system: it's free, and provides support for gdb, pyocd, and Ozone (Segger's debugger).

**NOTE:** The PCA100xx boards incorporate JLink software includes RTT-console support, which is used as a logging console.  This eliminates the need to configure and run a seperated UART-based console when developing firmware. An easy-to-use shell command (rtt.sh, see below) included, can be use to display console output.

**NOTE:** If you are using a ST-Link adapter, then no debugger firmware conversion should be necessary.
If you wish to use a Segger JLink adapter for development, then you will need to update the on-board debugger firmare for the Nucleo board: conversion details [here](https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board).  

**WARNING:** The Nucleo board needs to be modified per Decawave's recommendations: see DWS3000 Quick Start Guide for details.  
In particular there is a conflict between the on-board ethernet controller and the arduino-shield layout use of pin D11, which is used by SPI1 as the MOSI line. The Nucleo-F429ZI board's solder-bridges, SB121 and SB122, must be changed. Remove the solder from SB122 (open) and solder-bridge SB121 (close).  Without this change, the SPI MOSI line will always be pulled low and SPI transactions to the DWM3000 will fail.
See photo below for modified board's solder-bridge configuation.
![dws3000](https://github.com/foldedtoad/dwm3000/blob/master/docs/Nucleo_F429ZI_solder_bridge_config.JPG)

### DWS3000 Board/Shield Support
Under this project's root directory, there is a the following file tree structure:

```
├── boards
│   └── shields
│       └── qorvo_dwm3000
│           ├── Kconfig.defconfig
│           ├── Kconfig.shield
│           ├── doc
│           │   ├── DWS3000\ Schematics.pdf
│           │   └── index
│           └── qorvo_dwm3000.overlay
```

```
├── dts
│   └── bindings
│       └── qorvo,dwm3000
│           └── qorvo,dwm3000.yaml
```

In each example sub-project, the CMakeList.txt file has been updated with the following statement. This statement merges the DWM3000 custom shield definitions into the Zephyr board configuration process.

```
set(DTS_ROOT   "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SHIELD qorvo_dwm3000)
```

### Software
* Install Zephyr (V3.0) on your build system.
* Install Zephyr SDK -- zephyr-sdk-0.14.0.
* Install Segger JLink (latest) on your build system.
* (Optional) Install Segger Ozone (latest) on your build system.
* (Optional) Install the Nordic [nrfjprog](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools/Download) utility. After installing, make sure that your system's PATH contains the path to where it is installed.

**NOTE:** For MacOS build systems, you may need to install the ARM toolchain. The Zephyr install instructions can guide you though this process. For Linux, the toolchain is included in the Zephyr installation/setup process.

**NOTE:** The JLink suite by default will create a USB MSD (Mass Storage Device: flash drive) on the host development system whenever one of the JLink utilities are started.  In Linux and MacOS this can be annoying.  To disable this "feature", do the following.

```
host:user$ JLinkExe
SEGGER J-Link Commander V6.60e (Compiled Jan 17 2020 17:38:55)
DLL version V6.60e, compiled Jan 17 2020 17:38:41

Connecting to J-Link via USB...O.K.
Firmware: J-Link OB-SAM3U128-V2-NordicSemi compiled Mar 17 2020 14:43:00
Hardware version: V1.00
S/N: xxxxxxxxxx
License(s): RDI, FlashBP, FlashDL, JFlash, GDB
VTref=3.300V

Type "connect" to establish a target connection, '?' for help
J-Link> MSDDisable
Probe configured successfully.
J-Link>q
```

#### Establishing the Build Environment

Before the firmware can be built, you must establish the Zephyr-build environment.  In this document it is assumed that `~/zephyr` is the root directory for Zephyr: make the appropiate changes if your Zephyr root path is different.

```
> cd ~/zephyr/zephyrproject/zephyr
> source zephyr-env.sh
```

**NOTE** These projects was developed using only `cmake`, not `west` or `ninja`, but you should be able to use them if you prefer.
The build examples which follow will use `cmake`.
The target board and shield are hardcoded into the CMakeList.txt files. While this seems to be at odds with Zephyr examples, the target boards are set in the CMakeFile.txt files as typically developers work with only one board type for given project.

#### Build All Examples
To build all this examples at one time, use the `./examples/build_all.sh` shell script. This will build all the projects and put the `*.hex` file for each project into the `./examples/bin` directory.  You can then install this hex files individually with the `examples/install_hex.sh` shell script.

#### Build Individual Examples
Follow the instructions from Zephyr [here](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system).

**NOTE:** For Ubuntu, the ARM toolchain is provided in the version of Zephyr, so you will not need to install or build them yourself.
This provides build-consistency across Zephyr projects.


Next, navigate to the location where you've cloned the dwm3000 root directory, and then to the examples directory.
```
> cd ~/zephyr/dwm3000
> cd examples
```
Using the first project (./examples/ex_00a_reading_dev_id) as a build example, do the following.

```
> cd ex_00a_reading_dev_id
> ./configure.sh
> cd build
> make
```
**NOTE:** If you want to change the target board to something other than the default nRF52840, then edit the individual example's CMakeLists.txt and change the `set(BOARD xxxxx)` option to your target board.  Below shows switching to nRF52832 target.
```
  set(BOARD nrf52dk_nrf52832)
  #set(BOARD nrf52840dk_nrf52840)
```

#### Flashing
There are two ways to flash one of the example project's firmware onto a PCA100xx board.
* Use the debugger (gdb, Ozone, etc) to flash.
* Use the Nordic-provided `nrfjprog` utility.

The Segger debugger, `Ozone`, was used extensively during development, and is recommended.  
GDB may also be use with the PCA100xx on-board JLink support in conjunction with OpenODB.

### Console Messages (JLink RTT Console)
If you are using Ozone for debugging, the RTT console support is built into the debugger: just select `terminal` under the View menu.
Below is a screenshot of Ozone staging the ex_00a_reading_dev_id example.
![screenshot1](https://github.com/foldedtoad/dwm3000/blob/master/docs/ozone_debugger.png)

If you are developing on a Linux or MacOS system and have installled the JLink package, then you can use the `rtt.sh` script (in the root directory) to start console instance.  Something like the `rtt.sh` script may be possible on Windows, but it has not be tried.  Be sure to follow the directions displayed when `rtt.sh` starts: `h`, `r`, `g` in the JLinkExe shell.

For the above build example of `ex_00a_reading_dev_id`, if you have RTT message support and started, then you should see the following.

![screenshot2](https://github.com/foldedtoad/dwm3000/blob/master/docs/rtt_console.png)

## Sample Outputs
Below is a matching of send and receive demos using a pair of nRF52840 (PCA10056) boards: `ex_01a_simple_tx` and `ex_02a_simple_rx`.

This shows the output from the sending example `ex_01a_simple_tx`.
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

This shows the output from the receiving example `ex_02a_simple_rx`.
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
