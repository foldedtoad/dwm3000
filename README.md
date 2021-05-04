# DWM3000 and Zephyr 2.5

**NOTE: This set of projects require Zephyr Version 2.5.**  

This project contains firmware examples for the Decawave DWM3000-series Ultra Wideband (UWB) modules with Zephyr RTOS. It's a port of Qorvo/Decawave's SDK found on their website. 

### Terminology
* "Decawave" means "Qorvo/Decwave".   Decawave was recently acquiered by Qorvo.
* "DWS3000" is the Qorvo/Decawave DevKit which implements a DWM3000-series module on an Arduino form-factor board. 
* "PCA10056" means "Nordic nRF52840DK board". The PCA10056 is part of DevKit for the Nordic nRF52840 SoC product. The PCA10056 board form-factor is compliant with the Arduino form-factor layout, thus allowing the DWS3000 shield plugged on top of the PCA10056 board.

### Assumptions 
* This project assumes some familiarity with the Zephyr RTOS.  
Zephyr is relativey easy to install and learn, and there are good tutorials available which explain how to establish a working version of Zephyr on your development system.

* The example projects have been developed using the PCA10056 and the DWS3000 Arduino-shild board.

### Overview of Changes from Decawave's SDK code
The major changes from the original Decawave project are:
* Use a custom shield-board DTS definition: the "DWM3000".
* Port SPI-bus access and GPIO access.
* Most of the porting effort entailed changes to the "platform" directory modules.

* The original code had comment lines which extended well past 80 columns.  This is very inconvienent for development within VMs on laptops where screen real-estate limited. So the code was reformatted to 80-column max lines.  It's just easier to read and understand: that is the point of examples, right?!

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

While other Zephyr-supported boards might be used, they have not been tested. Only Zephyr API calls have been used, so code >>should<< be portable.

A `micro-USB` cable will also be needed. This USB cable will allow both debugging and flashing of firmware onto the PCA10056 board. Use of Segger's Ozone is recommended, but not required.

Many of the examples will require two or more PCA10056+DWS3000 setups, such as the micro-location examples.

**WARNING:** You will need to trim the PCA10056 board's P20 connector pins, as they extend too far upwards and will contact pads on the bottom of the DWS3000 shield, causing electical problems.

**NOTE:** Because the PCA10056 board incorporates a Segger JLink debugger (on-board), it is highly recommended to install the Segger JLink package on your development system: it's free, and provides support for gdb, pyocd, and Ozone (Segger's debugger).  

**NOTE:** The PCA10056 board incorporates JLink software includes RTT-console support, which is used as a logging console.  This eliminates the need to configure and run a seperated UART-based console when developing. An easy-to-use shell command (rtt.sh) included, can be use to display console output.

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
* (Optional) Install the Nordic [nrfjprog](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools/Download) utility.

**NOTE:** For MacOS build systems, you may need to install the ARM toolchain. The Zephyr install instructions can guide you though this process.

#### Building
Follow the instructions from Zephyr [here](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system).

**NOTE:** The toolchain is now provided in the latest version of Zephyr, so you will not need to install or build them yourself.
This provides build-consistency across Zephyr projects.

This project was developed using only `cmake`, not `west` or `ninja`, but you should be able to use them if you prefer.

#### Flashing
There are two ways to flash one of the example project's firmware onto a PCA10056 board.
* Use the debugger (gdb, Ozone, etc) to flash.
* Use the Nordic-provided `nrfjprog` utility.

In order to flash the boards with `nrfjprog` you will need to install `nrfjprog`. This tool is also available on all 3 main OS's. You can find it [here](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools). After installing, make sure that your system PATH contains the path to where it is installed.

After installing `nrfjprog`, quickest way is probably using `make flash`. 
But note, this does not allow you to see the console messages.
If you are developing a new project or modifying an existig project, the you will probably be using an debugger with some form of graphical interface. 

### Zephyr Environment Variables
Now change your active directory:
```
    cd zephyr
```

Now source the script zephyr-env.sh (linux & macOS) or run zephyr-env.cmd to make sure all the environment variables are set correctly.

### Build Your First Application
The github repository is the one that contains the specific DWM3000 example code.
Download or clone [this](https://github.com/foldedtoad/dwm3000) repository to your local computer:
```
    git clone https://github.com/foldedtoad/dwm3000.git
```

Start building the real examples from the project.  
NOTE: The procedure is identical for all examples.  

We will proceed with building the first simple example: `./examples/ex_01a_simple_tx/`.
```
    cd examples/ex_01a_simple_tx
```
Configure the build system with `cmake` as follows:
```
    cmake -B build -DBOARD=nrf52840dk-nrf52840 .
```
NOTE: You will need to re-do the above step whenever new "C"-type files are added to your project.  The Zephyr cmake support is good at detecting changes in existing files, but doesn't detect newly added files; thus the need to run the above again.

NOTE: Sometimes you might get error messages from the above configuration procedure. If so, delete the whole build directory and try again; often there are residual files in an existing build directory which appear to collide with the new configuration definitions. 

OPTIONAL: If you are developing on a Linux or OSX system, then you may use the script `configure.sh`, which does the same operation.

And we actually build or firmware with `make`:
```
    cd build
    make
```

### Flash
Now let's flash the binary file that we just built onto the board. 
Make sure you have nrfjprog properly installed and that it is in the system PATH.

#### Program the binary file on the board:
```
make flash
```

### Console Messages (JLink RTT Console)
If you are developing on a Linux or OSX system and have installled the JLink package, then you can use the `rtt.sh` script (in the root directory) to start console instance.  Something like the `rtt.sh` script may be possible on Windows, but it has not be tried.  Be sure to follow the directions displayed when `rtt.sh` starts: `h`, `r`, `g` in the JLinkExe shell.

If you have RTT message support and started, then you should see the following

```
***** Booting Zephyr OS build zehpher-V2.0.0-882-g89a984e64424 *****
main_thread
SIMPLE TX 13
device_id: deca0130
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
