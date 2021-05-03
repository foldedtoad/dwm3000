cmake_minimum_required(VERSION 3.13.1)

set(DTS_ROOT   "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SHIELD qorvo_dwm3000)

set(BOARD nrf52840dk_nrf52840)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(Example_01e)

add_definitions(-DTX_WITH_CCA)

target_sources(app PRIVATE ../../main.c)
target_sources(app PRIVATE ../../config_options.c)
target_sources(app PRIVATE tx_with_cca.c)

target_sources(app PRIVATE ../../decadriver/deca_device.c)

target_sources(app PRIVATE ../../platform/port.c)
target_sources(app PRIVATE ../../platform/deca_mutex.c)
target_sources(app PRIVATE ../../platform/deca_sleep.c)
target_sources(app PRIVATE ../../platform/deca_spi.c)
target_sources(app PRIVATE ../../platform/port.c)

target_sources(app PRIVATE ../../shared_data/shared_functions.c)

target_include_directories(app PRIVATE ../../)
target_include_directories(app PRIVATE ../../decadriver/)
target_include_directories(app PRIVATE ../../platform/)
target_include_directories(app PRIVATE ../../compiler/)
target_include_directories(app PRIVATE ../../shared_data/)

# zephyr_compile_options(-save-temps)
