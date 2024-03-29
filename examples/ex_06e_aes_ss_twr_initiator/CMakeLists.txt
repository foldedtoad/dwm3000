cmake_minimum_required(VERSION 3.13.1)

set(DTS_ROOT   "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SHIELD qorvo_dwm3000)

set(BOARD nrf52840dk_nrf52840)
#set(BOARD nrf52dk_nrf52832)
#set(BOARD nucleo_f429zi)

find_package(Zephyr)
project(Example_06e)

add_definitions(-DSS_AES_TWR_INITIATOR)

target_sources(app PRIVATE ../../main.c)
target_sources(app PRIVATE ../../config_options.c)
target_sources(app PRIVATE ss_aes_twr_initiator.c)

target_sources(app PRIVATE ../../MAC_802_15_4/mac_802_15_4.c)

target_sources(app PRIVATE ../../decadriver/deca_device.c)

target_sources(app PRIVATE ../../platform/port.c)
target_sources(app PRIVATE ../../platform/deca_sleep.c)
target_sources(app PRIVATE ../../platform/deca_spi.c)
target_sources(app PRIVATE ../../platform/port.c)

target_sources(app PRIVATE ../../shared_data/shared_functions.c)

target_include_directories(app PRIVATE ../../)
target_include_directories(app PRIVATE ../../MAC_802_15_4/)
target_include_directories(app PRIVATE ../../decadriver/)
target_include_directories(app PRIVATE ../../platform/)
target_include_directories(app PRIVATE ../../compiler/)
target_include_directories(app PRIVATE ../../shared_data/)

# zephyr_compile_options(-save-temps)
