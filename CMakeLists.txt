cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.c
)
add_executable(main ${SRC_FILES})

# important for IO
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(main 
    pico_cyw43_arch_lwip_threadsafe_background 
    pico_stdlib pico_cyw43_arch_none hardware_pwm
    pico_btstack_ble pico_btstack_cyw43
    m
    pico_multicore
)
pico_btstack_make_gatt_header(main PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bt/bp.gatt")
