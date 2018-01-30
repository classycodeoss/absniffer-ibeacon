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
#include <ble_gap.h>
#include "nordic_common.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "app_timer.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

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

// Beacon identity
#define APP_MAJOR_VALUE                 0x00, 0x01
#define APP_MINOR_VALUE                 0x00, 0x01
#define APP_BEACON_UUID                 0xcc, 0xcc, 0xcc, 0xcc,    0xcc, 0xcc, 0xcc, 0xcc,    0xcc, 0xcc, 0xcc, 0xcc,    0xcc, 0xcc, 0xcc, 0xcc

static ble_gap_adv_params_t m_adv_params;
static uint8_t              m_beacon_info[APP_BEACON_INFO_LENGTH] = {
        APP_DEVICE_TYPE,
        APP_ADV_DATA_LENGTH,
        APP_BEACON_UUID,
        APP_MAJOR_VALUE,
        APP_MINOR_VALUE,
        APP_MEASURED_RSSI
};
static ble_advdata_manuf_data_t m_manuf_specific_data;

void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void advertising_init(void) {
    uint32_t err_code;
    ble_advdata_t adv_data;
    uint8_t flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

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

/*
static void advertising_stop(void) {
    ret_code_t err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("... stopped advertising");
}
*/

static void advertising_start(void) {
    ret_code_t err_code;
    NRF_LOG_DEBUG("Starting to advertise...");
    err_code = sd_ble_gap_adv_start(&m_adv_params, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);
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

static void log_init(void) {
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void timer_init(void) {
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

int main(void) {
    log_init();
    timer_init();
    ble_stack_init();
    advertising_init();
    advertising_start();
    while (true) {
        if (!NRF_LOG_PROCESS()) { // process deferred logs
            uint32_t err_code = sd_app_evt_wait();
            APP_ERROR_CHECK(err_code);
        }
    }
}
