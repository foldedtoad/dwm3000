# DWM3000 and Zephyr 2.5

**NOTE: This set of projects require Zephyr Version 2.5.99.**  

This project contains examples on how to use the Ultra Wideband (UWB) and Bluetooth hardware based DWM3000 module together with Zephyr RTOS. It's an adaptation of Decawave's examples distributed along with their driver. 

This project is a port of the Qorvo/Decawave's code to the Zephyr platform.   
This project assumes some familiarity with Zephyr.  Zephyr is relativey easy to install and learn, and there are good tutorials available which explain how to establish a working version of Zephyr on your development system.

The major changes from the original Decawave project are:

* Use a custom shield-board DTS definition: the "DWM3000".

* Port SPI-bus access and GPIO access.

* The original code had comment lines which extended well past 80 columns.  This is very inconvienent for development within VMs on laptops where screen real-estate limited. So the code was reformatted to 80-column max lines.  It's just easier to read and understand: that is the point of examples, right?!

## Getting Started

## What's required?
### OS
Linux, Mac or Windows

This project was developed in a Ubuntu 18.04 (LTS), but there is no reason these changes should work with the other OSes.

### Hardware
You wil need two boards: host board (Nordic's PCA10056/nRF52840), and a Qorvo/Decawave DWS3000 arduino shield board.
A`micro-USB` cable will also be needed.  
Many of the examples will require two or more PCA10056+DWS3000 setups, such as the micro-location examples.

NOTE: You will need to trim the PCA10056 board's P20 pins, as they extend too far upwards and will contact pads on the bottom of the DWS3000 shield, causing electical problem.

NOTE: Because the Nordic PCA10056 board incorporates a Segger JLink debugger (on-board), it is highly recommended to install the Segger JLink package on your development system: it's free, and provides support for gdb, pyocd, and Ozone (Segger's debugger).

Because this board incorporates JLink support, Segger's RTT console support is used for logging.  This eliminates the need to configure and run a seperated UART-based console when developing.

### DWS3000 Board Support

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
There's quite a lot to install if you haven't already. First we're going to build the firmware, after which we can flash it on the board.

#### Building
Follow the instructions from Zephyr [here](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system).

NOTE: The toolchain is now provided in the latest version of Zephyr, so you will not need to install or build them yourself.
This provides build-consistency across Zephyr projects.

This project was developed using only `cmake`, not `west` or `ninja`, but you should be able to use them if you prefer.

#### Flashing
There are two ways to flash one of the example project's firmware onto a DWM1001 board.
* Use the debugger (gdb, Ozone, etc) to flash.
* Use the Nordic-provided `nrfjprog` utility via running make: `make flash`

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
The following examples are provided (checkbox checked if all functionality of the example is fully functional):
 - Example 1 - transmission
  - [ ] ex_01a_simple_tx
  - [ ] ex_01b_tx_sleep
  - [ ] ex_01c_tx_sleep_auto
  - [ ] ex_01d_tx_timed_sleep
  - [ ] ex_01e_tx_with_cca
 - Example 2 - reception
  - [ ] ex_02a_simple_rx
  - [ ] ex_02b_rx_preamble_64
  - [ ] ex_02c_rx_diagnostics
  - [ ] ex_02d_rx_sniff
  - [ ] ex_02e_rx_dbl_buff
 - Example 3 - transmission + wait for response
  - [ ] ex_03a_tx_wait_resp
  - [ ] ex_03b_rx_send_resp
  - [ ] ex_03c_tx_wait_resp_leds
  - [ ] ex_03d_tx_wait_resp_interrupts
 - Example 4 - continuous transmission
  - [ ] ex_04a_cont_wave
  - [ ] ex_04b_cont_frame
 - Example 5 - double-sided two-way ranging
  - [ ] ex_05a_ds_twr_init
  - [ ] ex_05b_ds_twr_resp
  - [ ] ex_05c_ds_twr_resp_ble
 - Example 6 - single-sided two-way ranging
  - [ ] ex_06a_ss_twr_init
  - [ ] ex_06b_ss_twr_resp
 - Example 7 - acknownledgements
  - [ ] ex_07a_ack_data_tx
  - [ ] ex_07b_ack_data_rx
 - Example 8 - low power listen
  - [ ] ex_08a_low_power_listen_rx
  - [ ] ex_08b_low_power_listen_tx
 - Example 9 - bandwidth power
  - [ ] ex_09a_bandwidth_power_ref_meas
  - [ ] ex_09b_bandwidth_power_comp
 - Example 10 - GPIO
  - [ ] ex_10a_gpio
 - Example 11 - IO
  - [ ] ex_11a_button
  - [ ] ex_11b_leds
 - Example 12 - BLE
  - [ ] ex_12a_ble 
 - Example 13 - Acccelerometer
  - [ ] ex_13a_accelerometer  

## What's next?
* Examples completion
* (Mobile) readout app (alternative: use Nordic `nrf-connect` app (iOS and Android)
* Add DTS/Bindings structure to allow non-Zephyr-included drivers to be accessed from custom app
* Create a VirtualBox VM image which contains a turn-key Zephyr + DWM1001 development system.
