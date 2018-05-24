cmake_minimum_required(VERSION 3.6)
project(absniffer-ibeacon C ASM)

include("CMakeEnv.cmake")
include("CMake_nRF52.cmake")

nRF52_setup()

include_directories(
        "${NRF5_SDK_PATH}/components/libraries/fifo"
        "${NRF5_SDK_PATH}/components/libraries/scheduler"
        "${NRF5_SDK_PATH}/components/drivers_nrf/uart"
        "${NRF5_SDK_PATH}/components/libraries/uart"
        "${NRF5_SDK_PATH}/components/libraries/fstorage"
        "${NRF5_SDK_PATH}/components/libraries/fds"
        "${NRF5_SDK_PATH}/components/libraries/atomic_fifo"
        "${NRF5_SDK_PATH}/components/libraries/fds"
)

list(APPEND SDK_SOURCE_FILES
        "${NRF5_SDK_PATH}/components/libraries/timer/app_timer.c"
        "${NRF5_SDK_PATH}/components/libraries/fifo/app_fifo.c"
        "${NRF5_SDK_PATH}/components/libraries/scheduler/app_scheduler.c"
        "${NRF5_SDK_PATH}/components/softdevice/common/nrf_sdh.c"
        "${NRF5_SDK_PATH}/components/drivers_nrf/uart/nrf_drv_uart.c"
        "${NRF5_SDK_PATH}/components/libraries/uart/retarget.c"
        "${NRF5_SDK_PATH}/components/libraries/uart/app_uart_fifo.c"
        "${NRF5_SDK_PATH}/components/libraries/fstorage/nrf_fstorage.c"
        "${NRF5_SDK_PATH}/components/libraries/fstorage/nrf_fstorage_sd.c"
        "${NRF5_SDK_PATH}/components/libraries/atomic_fifo/nrf_atfifo.c"
        "${NRF5_SDK_PATH}/components/libraries/fds/fds.c"
        )

include_directories(".")
list(APPEND SOURCE_FILES "main.c" "uart_cmd.c" "nvconfig.c" "hex_utils.c")

nRF52_addExecutable(${PROJECT_NAME} "${SOURCE_FILES}")
