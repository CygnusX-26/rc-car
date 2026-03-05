cmake_minimum_required(VERSION 3.13)

# Default to Pico 2 W so CYW43/lwIP targets are available unless overridden.
if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2_w CACHE STRING "Board type")
endif()

# Use a sibling pico-sdk checkout by default if not provided via env/cache.
if (NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk" CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)
endif()

include(pico_sdk_import.cmake)
project(flashprint C CXX ASM)
set(CMAKE_C STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
add_executable(main
    main.c
)
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
pico_add_extra_outputs(main)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(main pico_cyw43_arch_lwip_threadsafe_background pico_stdlib)
