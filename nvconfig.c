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
#include "nvconfig.h"

#include <string.h>
#include "fds.h"

#define CONFIG_FILE     (0xF010)
#define CONFIG_REC_KEY  (0x7010)

// The default beacon configuration
static configuration_t m_default_cfg = {
        .beacon_uuid = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc},
        .beacon_major = 1,
        .beacon_minor = 1
};

// The default configuration record
static fds_record_t const m_default_record = {
        .file_id           = CONFIG_FILE,
        .key               = CONFIG_REC_KEY,
        .data.p_data       = &m_default_cfg,
        // The length of a record is always expressed in 4-byte units (words)
        .data.length_words = (sizeof(m_default_cfg) + 3) / sizeof(uint32_t)
};

// Flash data storage initialization is asynchronous, this holds its result
static bool volatile m_fds_initialized;

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

uint32_t nvconfig_init() {
    ret_code_t err_code;
    err_code = fds_register(fds_evt_handler);
    if (err_code != FDS_SUCCESS) return err_code;

    m_fds_initialized = false;
    err_code = fds_init();
    if (err_code != FDS_SUCCESS) return err_code;

    // wait for initialization to finish
    while (!m_fds_initialized) {
        sd_app_evt_wait();
    }

    return 0;
}

uint32_t nvconfig_save(configuration_t *cfg) {
    ret_code_t err_code;
    fds_record_desc_t desc = {0};
    fds_find_token_t token = {0};

    fds_record_t record = {
            .file_id = CONFIG_FILE,
            .key = CONFIG_REC_KEY,
            .data.p_data = cfg,
            .data.length_words = (sizeof(configuration_t) + 3) / sizeof(uint32_t)
    };

    err_code = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &token);
    if (err_code == FDS_SUCCESS) {
        return fds_record_update(&desc, &record);
    } else if (err_code == FDS_ERR_NOT_FOUND) {
        return fds_record_write(&desc, &record);
    } else {
        return err_code;
    }
}

uint32_t nvconfig_load(configuration_t *cfg) {
    ret_code_t err_code;
    fds_record_desc_t desc = {0};
    fds_find_token_t token = {0};

    err_code = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &token);
    if (err_code == FDS_SUCCESS) {
        fds_flash_record_t record = {0};
        err_code = fds_record_open(&desc, &record);
        APP_ERROR_CHECK(err_code);

        memcpy(cfg, record.p_data, sizeof(configuration_t));
        err_code = fds_record_close(&desc);
        APP_ERROR_CHECK(err_code);
        return 0;
    } else if (err_code == FDS_ERR_NOT_FOUND) { // config not found, write and return default config
        fds_record_write(&desc, &m_default_record);
        memcpy(cfg, &m_default_cfg, sizeof(configuration_t));
        return 0;
    } else {
        return err_code;
    }
}
