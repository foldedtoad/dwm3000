# Nordic nRF52-series + Decawave DWM3000 on Zephyr v2.5

**NOTE: This set of projects require Zephyr Version 2.5.**  

This project contains firmware examples for the Decawave DWM3000-series Ultra Wideband (UWB) modules with Zephyr RTOS. It's a port of Qorvo/Decawave's SDK found on their website. 

This port to Zephyr generally follows the Qorvo/Decawave [DWM3000 SDK Release V1.1](https://www.qorvo.com/products/p/DWS3000#documents). It is advised to review the SDK's content to get a general understanding of how the examples are intended to function. 

## State of Project
* The combination of PCA10056 (nRF52840) + DWS3000 are fully functional: see Sample Outputs section below.
* The combination of PCA10040 (nRF52832) + DWS3000 are fully functional.
* The combination of Nucleo-F429ZI + DWS3000 is in development: board ordered.
* Interaction with DWM1001 boards does not interact correctly with PCA100xx + DWS3000: framing errors. 

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

* The `example` projects have been developed using the DWS3000 Arduino-shield plugged into either the PCA10040 or PCA10056 board.

### Overview of Changes from the DWM3000 SDK
The major changes from the original Decawave project are:
* Define and use a custom shield-board DTS definition: the "DWM3000". (ToDo: Change the shield name to "DWS3000" in future.)
* Port SPI-bus access and GPIO access.
* Most of the porting effort entailed changes to the "platform" directory modules, which is where the `io` driver layer exists: SPI & GPIO.
* Little changes were made in the `decadriver` directory, which is where the "functional" driver layer for the DWM3000 exists: should be target board independent code.
* The original code had comment lines which extended well past 80 columns.  This is inconvienent for development within VMs on laptops where screen real-estate limited. So the code was reformatted to 80-column max lines.  It's just easier to read and understand: that is the point of examples, right?!

### Supported Development OSes
Linux, Mac or Windows

This project was developed in a Ubuntu 18.04 (LTS) and MacOS (Big Sur), but there is no reason these changes should work with the other OSes.
Windows OSes have not been part of the development process, but following Zephyr's instruction for Windows setup, it should not be a problem.

### Hardware
You will need two boards: a host board and a shield board --
* Host board PCA10040
![pca10040](https://github.com/foldedtoad/dwm3000/blob/master/docs/pca10040.png)
* Host board PCA10056
![pca10056](https://github.com/foldedtoad/dwm3000/blob/master/docs/pca10056.png)
* DWS3000 arduino shield board.
![dws3000](https://github.com/foldedtoad/dwm3000/blob/master/docs/dws3000.jpg)

**NOTE:** The conbination of PCA100xx board + DWS3000 shield will be the reference configuration thoughout this document.

While other Zephyr-supported boards might be used, they have not been tested. Only Zephyr API calls have been used, so the code is intended to be reasonably portable to other boards: mostly target board DeviceTree defines may need to be adjusted.

A `micro-USB` cable will also be needed to connect the PCA100xx to your build system. This USB cable will allow both debugging and flashing of firmware onto the PCA100xx board. Use of Segger's Ozone debugger is recommended, but not required.

Many of the examples will require two or more PCA100xx + DWS3000 setups, such as the micro-location examples.

**WARNING:** You will need to trim the pins on the PCA100xx board's P20 connector, as they extend too far upwards and will contact pads on the bottom of the DWS3000 shield, causing electical problems.

**WARNING:** There are issues with the early "engineering versions" of the nRF52840. The chips are identified with QIAAAA marking on their top.  Apparently the SPI3 bus was not fully support with these early engineering releases. For the PCA10056, the devicetree definition for `spi3` bus is equated with the `arduino_spi` bus. Therefore, it is suggested to avoid using these particular PCA10056 boards. Production releases of the PCA10056 should work without issues.

**NOTE:** Because the PCA100xx board incorporates a Segger JLink debugger (on-board), it is highly recommended to install the Segger JLink package on your development system: it's free, and provides support for gdb, pyocd, and Ozone (Segger's debugger).  

**NOTE:** The PCA100xx boards incorporate JLink software includes RTT-console support, which is used as a logging console.  This eliminates the need to configure and run a seperated UART-based console when developing firmware. An easy-to-use shell command (rtt.sh, see below) included, can be use to display console output.

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
│           │   └── index.rst
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
* Install Zephyr (V2.5) on your build system.
* Install Segger JLink (latest) on your build system.
* (Optional) Install Segger Ozone (latest) on your build system.
* (Optional) Install the Nordic [nrfjprog](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools/Download) utility. After installing, make sure that your system's PATH contains the path to where it is installed.

**NOTE:** For MacOS build systems, you may need to install the ARM toolchain. The Zephyr install instructions can guide you though this process. For Linux, the toolchain is included in the Zephyr installion/setup process.

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
**NOTE:** If you want to change the target board to something other than the default nRF52832, then edit the individual example's CMakeLists.txt and change the `set(BOARD xxxxx)` option to your target board.
```
  #set(BOARD nrf52dk_nrf52832)
  set(BOARD nrf52840dk_nrf52840)
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
Below is a matching of send and receive demos: `ex_01a_simple_tx` and `ex_02a_simple_rx`.

This shows th3e output from the sending example `ex_01a_simple_tx`.
```
*** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

[00:00:03.958,068] <inf> main: main_thread
[00:00:03.958,099] <inf> port: Configure WAKEUP pin
[00:00:03.958,099] <inf> port: Configure RESET pin
[00:00:03.958,099] <inf> port: Configure RX LED pin
[00:00:03.958,129] <inf> port: Configure TX LED pin
[00:00:03.958,129] <inf> deca_spi: openspi bus SPI_3
[00:00:04.985,931] <inf> simple_tx: SIMPLE TX v1.0
[00:00:04.985,961] <inf> port: reset_DWIC
[00:00:04.990,631] <inf> deca_device: dev_id "deca0302"
[00:00:04.993,743] <inf> simple_tx: len 10
                                    c5 00 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:05.494,262] <inf> simple_tx: len 10
                                    c5 01 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:06.026,947] <inf> simple_tx: len 10
                                    c5 02 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:06.527,465] <inf> simple_tx: len 10
                                    c5 03 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:07.054,229] <inf> simple_tx: len 10
                                    c5 04 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:07.554,748] <inf> simple_tx: len 10
                                    c5 05 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:08.081,512] <inf> simple_tx: len 10
                                    c5 06 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:08.582,031] <inf> simple_tx: len 10
                                    c5 07 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:09.108,856] <inf> simple_tx: len 10
                                    c5 08 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:09.609,375] <inf> simple_tx: len 10
                                    c5 09 44 45 43 41 57 41  56 45                   |..DECAWA VE
```

This shows the output from the receiving example `ex_02a_simple_rx`.
```
*** Booting Zephyr OS build zephyr-v2.5.0-1675-gd6567ad494a0  ***

[00:00:05.493,286] <inf> main: main_thread
[00:00:05.493,316] <inf> port: Configure WAKEUP pin
[00:00:05.493,316] <inf> port: Configure RESET pin
[00:00:05.493,347] <inf> port: Configure RX LED pin
[00:00:05.493,347] <inf> port: Configure TX LED pin
[00:00:05.493,347] <inf> deca_spi: openspi bus SPI_3
[00:00:06.521,514] <inf> simple_rx: SIMPLE RX v1.0
[00:00:06.521,545] <inf> port: reset_DWIC
[00:00:06.526,214] <inf> deca_device: dev_id "deca0302"
[00:00:06.529,235] <inf> simple_rx: Ready to Receive
[00:00:06.893,493] <inf> simple_rx: len 10
                                    c5 d3 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:07.394,012] <inf> simple_rx: len 10
                                    c5 d4 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:07.920,806] <inf> simple_rx: len 10
                                    c5 d5 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:08.421,295] <inf> simple_rx: len 10
                                    c5 d6 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:08.948,577] <inf> simple_rx: len 10
                                    c5 d7 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:09.448,669] <inf> simple_rx: len 10
                                    c5 d8 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:09.976,348] <inf> simple_rx: len 10
                                    c5 d9 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:10.475,982] <inf> simple_rx: len 10
                                    c5 da 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:11.004,150] <inf> simple_rx: len 10
                                    c5 db 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:11.503,295] <inf> simple_rx: len 10
                                    c5 dc 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:12.031,982] <inf> simple_rx: len 10
                                    c5 dd 44 45 43 41 57 41  56 45                   |..DECAWA VE      
[00:00:12.530,670] <inf> simple_rx: len 10
                                    c5 de 44 45 43 41 57 41  56 45                   |..DECAWA VE      
```
## Examples
Below is the examples directory tree layout.
```
.
├── bin
├── build_all.sh
├── ex_00a_reading_dev_id
│   ├── CMakeLists.txt
│   ├── configure.sh
│   ├── prj.conf
│   └── read_dev_id.c
├── ex_01a_simple_tx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_tx.c
├── ex_01b_tx_sleep
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   ├── tx_sleep.c
│   └── tx_sleep_idleRC.c
├── ex_01c_tx_sleep_auto
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── tx_sleep_auto.c
├── ex_01d_tx_timed_sleep
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── tx_timed_sleep.c
├── ex_01e_tx_with_cca
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── tx_with_cca.c
├── ex_01g_simple_tx_sts_sdc
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_tx_sts_sdc.c
├── ex_01h_simple_tx_pdoa
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_tx_pdoa.c
├── ex_01i_simple_tx_aes
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_tx_aes.c
├── ex_02a_simple_rx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_rx.c
├── ex_02c_rx_diagnostics
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── rx_diagnostics.c
├── ex_02d_rx_sniff
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── rx_sniff.c
├── ex_02f_rx_with_crystal_trim
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── rx_with_xtal_trim.c
├── ex_02g_simple_rx_sts_sdc
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_rx_sts_sdc.c
├── ex_02h_simple_rx_pdoa
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_rx_pdoa.c
├── ex_02i_simple_rx_aes
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── simple_rx_aes.c
├── ex_03a_tx_wait_resp
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── tx_wait_resp.c
├── ex_03b_rx_send_resp
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── rx_send_resp.c
├── ex_03d_tx_wait_resp_interrupts
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── tx_wait_resp_int.c
├── ex_04a_cont_wave
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── continuous_wave.c
│   └── prj.conf
├── ex_04b_cont_frame
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── continuous_frame.c
│   └── prj.conf
├── ex_05a_ds_twr_init
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ds_twr_initiator.c
│   ├── ds_twr_initiator_sts.c
│   └── prj.conf
├── ex_05b_ds_twr_resp
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ds_twr_responder.c
│   ├── ds_twr_responder_sts.c
│   └── prj.conf
├── ex_05c_ds_twr_init_sts_sdc
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ds_twr_sts_sdc_initiator.c
│   └── prj.conf
├── ex_05d_ds_twr_resp_sts_sdc
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ds_twr_sts_sdc_responder.c
│   └── prj.conf
├── ex_06a_ss_twr_initiator
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   ├── ss_twr_initiator.c
│   ├── ss_twr_initiator_sts.c
│   └── ss_twr_initiator_sts_no_data.c
├── ex_06b_ss_twr_responder
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   ├── ss_twr_responder.c
│   ├── ss_twr_responder_sts.c
│   └── ss_twr_responder_sts_no_data.c
├── ex_06e_aes_ss_twr_initiator
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── ss_aes_twr_initiator.c
├── ex_06f_AES_ss_twr_responder
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── ss_aes_twr_responder.c
├── ex_07a_ack_data_tx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── ack_data_tx.c
│   ├── configure.sh
│   └── prj.conf
├── ex_07b_ack_data_rx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── ack_data_rx.c
│   ├── configure.sh
│   └── prj.conf
├── ex_07c_ack_data_rx_dbl_buff
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── ack_data_rx_dbl_buff.c
│   ├── configure.sh
│   └── prj.conf
├── ex_11a_spi_crc
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── prj.conf
│   └── spi_crc.c
├── ex_13a_gpio
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── gpio_example.c
│   └── prj.conf
├── ex_14a_otp_write
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── otp_write.c
│   └── prj.conf
├── ex_15a_le_pend_tx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── le_pend_tx.c
│   └── prj.conf
├── ex_15b_le_pend_rx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── le_pend_rx.c
│   └── prj.conf
└── install_hex.sh
```
