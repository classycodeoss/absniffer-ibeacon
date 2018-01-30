cmake_minimum_required(VERSION 3.6)
project(absniffer-ibeacon C ASM)

include("CMakeEnv.cmake")
include("CMake_nRF52.cmake")

nRF52_setup()

include_directories(
        "${NRF5_SDK_PATH}/components/libraries/fifo"
        "${NRF5_SDK_PATH}/components/ble/ble_advertising"
        "${NRF5_SDK_PATH}/components/libraries/scheduler"
)

list(APPEND SDK_SOURCE_FILES
        "${NRF5_SDK_PATH}/components/libraries/timer/app_timer.c"
        "${NRF5_SDK_PATH}/components/libraries/fifo/app_fifo.c"
        "${NRF5_SDK_PATH}/components/ble/ble_advertising/ble_advertising.c"
        "${NRF5_SDK_PATH}/components/libraries/scheduler/app_scheduler.c"
        "${NRF5_SDK_PATH}/components/softdevice/common/nrf_sdh.c"
)

include_directories(".")
list(APPEND SOURCE_FILES "main.c")

nRF52_addExecutable(${PROJECT_NAME} "${SOURCE_FILES}")
