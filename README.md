# DWM3000 and Zephyr 2.5

**NOTE: This set of projects require Zephyr Version 2.5.**  

This project contains firmware examples for the Decawave DWM3000-series Ultra Wideband (UWB) modules with Zephyr RTOS. It's a port of Qorvo/Decawave's SDK found on their website. 

Since this Zephyr port generally follows the the Qorvo/Decawave [DWM3000 SDK](https://www.qorvo.com/products/p/DWS3000#documents) code, it is advised to review their SDK to get a general understanding of how the examples are intended to function. 

### Terminology
* "Decawave" means "Qorvo/Decwave".   Decawave was recently acquiered by Qorvo.
* "DWS3000" is the Qorvo/Decawave DevKit which implements a DWM3000-series module on an Arduino form-factor board. 
* "PCA10056" means "Nordic nRF52840DK board". The PCA10056 board is part of DevKit for the Nordic nRF52840 SoC product. The PCA10056 board form-factor is compliant with the Arduino form-factor layout, thus allowing the DWS3000 shield plugged on top of the PCA10056 board.

### Assumptions 
* This project assumes some familiarity with the Zephyr RTOS.  
Zephyr is relativey easy to install and learn, and there are good tutorials available which explain how to establish a working version of Zephyr on your development system.

* The example projects have been developed using the PCA10056 and the DWS3000 Arduino-shild board.

### Overview of Changes from Decawave's SDK code
The major changes from the original Decawave project are:
* Use a custom shield-board DTS definition: the "DWM3000".
* Port SPI-bus access and GPIO access.
* Most of the porting effort entailed changes to the "platform" directory modules.

* The original code had comment lines which extended well past 80 columns.  This is inconvienent for development within VMs on laptops where screen real-estate limited. So the code was reformatted to 80-column max lines.  It's just easier to read and understand: that is the point of examples, right?!

## Getting Started

## What's required?
### OS
Linux, Mac or Windows


This project was developed in a Ubuntu 18.04 (LTS) and MacOS (Big Sur), but there is no reason these changes should work with the other OSes.
Windows OSes have not been part of the development process, but following Zephyr's instruction for Windows setup, it should not be a problem.

### Hardware
You wil need two boards: 
* host board PCA10056
* DWS3000 arduino shield board.

The conbination of PCA10056 board + DWS3000 shield will be the standard/default platform thoughout this document.

While other Zephyr-supported boards might be used, they have not been tested. Only Zephyr API calls have been used, so the code is intended to be portable to other boards.

A `micro-USB` cable will also be needed to connect the PCA10056 to your build system. This USB cable will allow both debugging and flashing of firmware onto the PCA10056 board. Use of Segger's Ozone is recommended, but not required.

Many of the examples will require two or more PCA10056+DWS3000 setups, such as the micro-location examples.

**WARNING:** You will need to trim the pins on the PCA10056 board's P20 connector, as they extend too far upwards and will contact pads on the bottom of the DWS3000 shield, causing electical problems.

**NOTE:** Because the PCA10056 board incorporates a Segger JLink debugger (on-board), it is highly recommended to install the Segger JLink package on your development system: it's free, and provides support for gdb, pyocd, and Ozone (Segger's debugger).  

**NOTE:** The PCA10056 board incorporates JLink software includes RTT-console support, which is used as a logging console.  This eliminates the need to configure and run a seperated UART-based console when developing. An easy-to-use shell command (rtt.sh, see below) included, can be use to display console output.

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


In each example sub-project, the CMakeList.txt file has been updated with the following statement. This statement merges the custom board definitions into the Zephyr board configuration process. 

```
set(DTS_ROOT   "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SHIELD qorvo_dwm3000)
```

### Software
* Install Zephyr (V2.5) on your build system.
* Install Segger JLink (latest) on your build system.
* (Optional) Install Segger Ozone (latest) on your build system.
* (Optional) Install the Nordic [nrfjprog](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools/Download) utility. After installing, make sure that your system PATH contains the path to where it is installed.

**NOTE:** For MacOS build systems, you may need to install the ARM toolchain. The Zephyr install instructions can guide you though this process.

#### Establishing the Build Environment

Before firmware made, you must establish the Zephyr build environment.  In this readme, it is assumed `~/zephyr` is the root directory for Zephyr: make the appropiate changes if your Zephyr root path is different.
```
> cd ~/zephyr/zephyrproject/zephyr
> source zephyr-env.sh
```
**NOTE** These projects was developed using only `cmake`, not `west` or `ninja`, but you should be able to use them if you prefer.  
The build examples which follow will use `cmake`.

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
**NOTE:** If you want to change the target board to something other than the nRF52840, then edit the individual example's CMakeLists.txt and change the `set(BOARD nrf52840dk_nrf52840)` option to your target board.

#### Flashing
There are two ways to flash one of the example project's firmware onto a PCA10056 board.
* Use the debugger (gdb, Ozone, etc) to flash.
* Use the Nordic-provided `nrfjprog` utility.

The Segger debugger, `Ozone`, was used extensively during development, and is recommended.  
GDB may also be use with the PCA10056 on-board JLink support in conjunction with OpenODB.

### Console Messages (JLink RTT Console)
If you are using Ozone for debugging, the RTT console support is built into the debugger: just select `terminal` under the View menu. 
Below is a screenshot of Ozone staging the ex_00a_reading_dev_id example. 
![screenshot1](https://github.com/foldedtoad/dwm3000/blob/master/docs/ozone_debugger.png)

If you are developing on a Linux or MacOS system and have installled the JLink package, then you can use the `rtt.sh` script (in the root directory) to start console instance.  Something like the `rtt.sh` script may be possible on Windows, but it has not be tried.  Be sure to follow the directions displayed when `rtt.sh` starts: `h`, `r`, `g` in the JLinkExe shell.

For the above build example of `ex_00a_reading_dev_id`, if you have RTT message support and started, then you should see the following.

![screenshot2](https://github.com/foldedtoad/dwm3000/blob/master/docs/rtt_console.png)


## Examples
Below is the examples directory tree layout.
```
.
├── bin
├── build_all.sh
├── ex_00a_reading_dev_id
│   ├── CMakeLists.txt
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── read_dev_id.c
├── ex_01a_simple_tx
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── simple_tx.c
├── ex_01b_tx_sleep
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   ├── tx_sleep.c
│   └── tx_sleep_idleRC.c
├── ex_01c_tx_sleep_auto
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── tx_sleep_auto.c
├── ex_01d_tx_timed_sleep
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── tx_timed_sleep.c
├── ex_01e_tx_with_cca
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── tx_with_cca.c
├── ex_01g_simple_tx_sts_sdc
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── simple_tx_sts_sdc.c
├── ex_01h_simple_tx_pdoa
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
│   ├── prj.conf
│   └── simple_tx_pdoa.c
├── ex_01i_simple_tx_aes
│   ├── CMakeLists.txt
│   ├── README.rst
│   ├── configure.sh
│   ├── ozone.jdebug
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
│   ├── ozone.jdebug
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
