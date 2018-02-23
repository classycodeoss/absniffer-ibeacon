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
#include "uart_cmd.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <nrf_uart.h>
#include <app_uart.h>

#include "hex_utils.h"

// On the ABSniffer, nRF52 and CP2104 are wired like this
// http://wiki.aprbrother.com/wiki/ABSniffer_USB_Dongle_528
#define UART_RX_PIN                     8
#define UART_TX_PIN                     6

// Command buffer for receiving commands via UART
#define UART_RX_BUF_SIZE 256
#define UART_TX_BUF_SIZE 256
#define CMD_BUF_SIZE 256

// Module state
static uart_cmd_client_t *client;
static uint8_t cmd_buf[256 + 1];
static uint8_t *p_buf = &cmd_buf[0];

// Helper for sending null-terminated strings
static void uart_put_string(const char *str) {
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        app_uart_put((uint8_t) str[i]);
    }
}

static const char *response_ok = "OK\n";
static const char *response_err_configuration = "ERR: Configuration not accepted\n";
static const char *response_err_unknown_cmd = "ERR: Unknown command\n";

// parse the command: C<SP>UUID<SP>MAJOR<SP>MINOR
// no input validation whatsoever and unsafe memory handling ahead...
static void process_configuration_command(char *cmd, uart_cmd_evt_t *p_uart_cmd_evt) {
    uint8_t uuid[16];

    memset(uuid, 0, sizeof(uuid));
    p_uart_cmd_evt->evt_type = CONFIGURATION;
    strtok(cmd, " "); // skip 'C'
    const char *uuid_hex = strtok(NULL, " ");
    hex_string_to_uint8_array(uuid_hex, strlen(uuid_hex), uuid);
    memcpy(p_uart_cmd_evt->proximity_uuid, uuid, sizeof(uuid));

    const char *major = strtok(NULL, " ");
    p_uart_cmd_evt->major = (uint16_t) atoi(major);
    const char *minor = strtok(NULL, " ");
    p_uart_cmd_evt->minor = (uint16_t) atoi(minor);
}

/**
 * Process a command received via UART.
 *
 * Available commands:
 *
 * 'I' : Information about the device
 * 'C <hex-encoded proximity UUID> <Major> <Value>': Set iBeacon configuration
 */
static void process_command(char *cmd) {
    uart_cmd_evt_t uart_cmd_evt;
    memset(&uart_cmd_evt, 0, sizeof(uart_cmd_evt_t));
    if (*cmd == 'C') {
        process_configuration_command(cmd, &uart_cmd_evt);
        client->evt_handler(&uart_cmd_evt);
    } else if (*cmd == 'I') {
        uart_cmd_evt.evt_type = INFORMATION;
        client->evt_handler(&uart_cmd_evt);
    } else {
        uart_put_string(response_err_unknown_cmd);
    }
}

void uart_cmd_send_configuration_response(int error) {
    if (error) {
        uart_put_string(response_err_configuration);
    } else {
        uart_put_string(response_ok);
    }
}

void uart_cmd_send_information_response(const char *info) {
    uart_put_string("OK ");
    uart_put_string(info);
    uart_put_string("\n");
}

static void handle_uart_evt(app_uart_evt_t *p_event) {
    switch (p_event->evt_type) {
        case APP_UART_DATA_READY:
            app_uart_get(p_buf);
            p_buf++;
            if ((*(p_buf - 1) == '\n') || (*(p_buf - 1) == '\r') || (p_buf - cmd_buf >= CMD_BUF_SIZE)) {
                *p_buf = '\0';
                p_buf = &cmd_buf[0];
                process_command((char *) p_buf);
            }
            break;
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;
        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;
        default:
            break;
    }
}

ret_code_t uart_cmd_init(uart_cmd_client_t *uart_cmd_client) {
    ret_code_t err_code;

    app_uart_comm_params_t const comm_params = {
            .rx_pin_no    = UART_RX_PIN,
            .tx_pin_no    = UART_TX_PIN,
            .rts_pin_no   = 0, // ignored if flow control is disabled
            .cts_pin_no   = 0, // ignored if flow control is disabled
            .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
            .use_parity   = false,
            .baud_rate    = NRF_UART_BAUDRATE_115200
    };

    APP_UART_FIFO_INIT(&comm_params, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, handle_uart_evt, APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    client = uart_cmd_client;
    return err_code;
}
