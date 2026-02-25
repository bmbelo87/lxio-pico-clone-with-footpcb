cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT ON)
include(pico_sdk_import.cmake)

project(true_lxio_pico)

pico_sdk_init()

add_executable(true_lxio_pico
    main.c)

target_sources(true_lxio_pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(true_lxio_pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(true_lxio_pico
                      PUBLIC
                      pico_stdlib
                      pico_multicore
                      tinyusb_board
                      tinyusb_device
                      hardware_pio
                      hardware_dma
                      hardware_irq
                      hardware_flash)

# Uncomment this line to enable fix for Errata RP2040-E5 (the fix requires use of GPIO 15)
#target_compile_definitions(true_lxio_pico PUBLIC PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1)

pico_add_extra_outputs(true_lxio_pico)
pico_enable_stdio_usb(true_lxio_pico 0)
pico_enable_stdio_uart(true_lxio_pico 0)
pico_add_extra_outputs(true_lxio_pico)

add_compile_definitions(
    PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
)
