/*
 *    Copyright 2018 Classy Code GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdbool.h>
#include <stdint.h>

// NRF SDK
#include "ble_gap.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "app_timer.h"
#include "fds.h"

// Own modules
#include "uart_cmd.h"
#include "nvconfig.h"
#include "hex_utils.h"

#define FIRMWARE_VERSION                "1.0.0"

// Radio transmit power in dBm (accepted values are -40, -20, -16, -12, -8, -4, 0, 3, and 4 dBm).
#define TX_POWER                        (-16)

// iBeacon specifies an advertising interval of 100ms
#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(100, UNIT_0_625_MS)

// tag identifying the SoftDevice BLE configuration
#define APP_BLE_CONN_CFG_TAG            1

// Value used as error code on stack dump, can be used to identify stack location on stack unwind.
#define DEAD_BEEF                       0xDEADBEEF

// Beacon advertisement contents
#define APP_BEACON_INFO_LENGTH          0x17        // Total length of information advertised by the iBeacon
#define APP_ADV_DATA_LENGTH             0x15        // Length of manufacturer specific data in the advertisement.
#define APP_DEVICE_TYPE                 0x02        // 0x02 = Beacon
#define APP_MEASURED_RSSI               0xaf        // The Beacon's measured RSSI at 1 meter distance in dBm.
#define APP_COMPANY_IDENTIFIER          0x004c      // Company identifier for Apple iBeacon

// Beacon configuration
static configuration_t m_beacon_cfg;

static ble_gap_adv_params_t m_adv_params;
static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH];
static ble_advdata_manuf_data_t m_manuf_specific_data;

static uart_cmd_client_t m_uart_cmd_client;

// Flash data storage initialization is asynchronous, this holds its result
static bool volatile m_fds_initialized;

void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void advertising_init(const uint8_t *beacon_uuid, uint16_t major, uint16_t minor) {
    uint32_t err_code;
    ble_advdata_t adv_data;
    uint8_t flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    uint16_t short_val;

    m_beacon_info[0] = APP_DEVICE_TYPE;
    m_beacon_info[1] = APP_ADV_DATA_LENGTH;
    memcpy(&m_beacon_info[2], beacon_uuid, 16);

    short_val = __htons(major);
    memcpy(&m_beacon_info[18], &short_val, sizeof(short_val));
    short_val = __htons(minor);
    memcpy(&m_beacon_info[20], &short_val, sizeof(short_val));
    m_beacon_info[22] = APP_MEASURED_RSSI;

    m_manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    m_manuf_specific_data.data.p_data = (uint8_t *) m_beacon_info;
    m_manuf_specific_data.data.size = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&adv_data, 0, sizeof(adv_data));
    adv_data.name_type = BLE_ADVDATA_NO_NAME;
    adv_data.flags = flags;
    adv_data.p_manuf_specific_data = &m_manuf_specific_data;
    err_code = ble_advdata_set(&adv_data, NULL); // will set on SD via sd_ble_gap_adv_data_set
    APP_ERROR_CHECK(err_code);

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    m_adv_params.type = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
    m_adv_params.p_peer_addr = NULL;    // Undirected advertisement.
    m_adv_params.fp = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.timeout = 0;       // Never time out
}

static void advertising_stop(void) {
    ret_code_t err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);
}

static void advertising_start(void) {
    ret_code_t err_code;
    err_code = sd_ble_gap_adv_start(&m_adv_params, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);
}

static void handle_information_cmd() {
    char buf[256];
    char mac_addr_str[32];
    char uuid_str[33];
    ble_gap_addr_t mac_addr;

    // Send firmware version and MAC address
    sd_ble_gap_addr_get(&mac_addr);
    sprintf(mac_addr_str, "%2X:%2X:%2X:%2X:%2X:%2X", mac_addr.addr[5], mac_addr.addr[4], mac_addr.addr[3],
            mac_addr.addr[2], mac_addr.addr[1], mac_addr.addr[0]);
    for (int i = 0; i < 16; i++) {
        uint8_to_hex_char(m_beacon_cfg.beacon_uuid[i], &uuid_str[2*i]);
    }
    uuid_str[32] = 0;
    sprintf(buf, "V%s %s %s %d %d", FIRMWARE_VERSION, mac_addr_str, uuid_str, m_beacon_cfg.beacon_major, m_beacon_cfg.beacon_minor);
    uart_cmd_send_information_response(buf);
}

static void handle_configuration_cmd(const uint8_t *proximity_uuid, uint16_t major, uint16_t minor) {
    ret_code_t err_code;

    memcpy(m_beacon_cfg.beacon_uuid, proximity_uuid, 16);
    m_beacon_cfg.beacon_major = major;
    m_beacon_cfg.beacon_minor = minor;
    err_code = nvconfig_save(&m_beacon_cfg);

    advertising_stop();
    advertising_init(m_beacon_cfg.beacon_uuid, m_beacon_cfg.beacon_major, m_beacon_cfg.beacon_minor);
    advertising_start();
    uart_cmd_send_configuration_response(err_code);
}

static void uart_cmd_evt_handler(const uart_cmd_evt_t *p_uart_cmd_evt) {
    switch (p_uart_cmd_evt->evt_type) {
        case INFORMATION: // Send firmware version and MAC address
            handle_information_cmd();
            break;
        case CONFIGURATION:
            handle_configuration_cmd(p_uart_cmd_evt->proximity_uuid, p_uart_cmd_evt->major, p_uart_cmd_evt->minor);
            break;
        default:
            break;
    }
}

// handler for asynchronous flash data storage events
static void fds_evt_handler(fds_evt_t const *p_evt) {
    switch (p_evt->id) {
        case FDS_EVT_INIT:
            if (p_evt->result == FDS_SUCCESS) {
                m_fds_initialized = true;
            }
            break;
        default:
            break;
    }
}

static void ble_stack_init(void) {
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Set transmission power
    err_code = sd_ble_gap_tx_power_set(TX_POWER);
    APP_ERROR_CHECK(err_code);
}

static void timer_init(void) {
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void uart_init() {
    uint32_t err_code;
    memset(&m_uart_cmd_client, 0, sizeof(uart_cmd_client_t));
    m_uart_cmd_client.evt_handler = uart_cmd_evt_handler;
    err_code = uart_cmd_init(&m_uart_cmd_client);
    APP_ERROR_CHECK(err_code);
}

static void flash_data_storage_init() {
    ret_code_t err_code;
    err_code = fds_register(fds_evt_handler);
    APP_ERROR_CHECK(err_code);

    m_fds_initialized = false;
    err_code = fds_init();
    APP_ERROR_CHECK(err_code);
}

static void wait_for_fds_initialization() {
    while (!m_fds_initialized) {
        sd_app_evt_wait();
    }
}

int main(void) {
    ret_code_t err_code;

    timer_init();
    uart_init();
    ble_stack_init();

    // initialize and wait for storage, load configuration
    flash_data_storage_init();
    wait_for_fds_initialization();
    err_code = nvconfig_load(&m_beacon_cfg);
    APP_ERROR_CHECK(err_code);

    advertising_init(m_beacon_cfg.beacon_uuid, m_beacon_cfg.beacon_major, m_beacon_cfg.beacon_minor);
    advertising_start();
    while (true) {
        err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);
    }
}
