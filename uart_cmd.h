#ifndef _UART_CMD_H
#define _UART_CMD_H

#include <stdint.h>

// Types of commands received
typedef enum {
    CONFIGURATION,
    INFORMATION
} uart_cmd_evt_type_t;

// Event data structure
typedef struct {
    uart_cmd_evt_type_t evt_type;
    uint16_t major;
    uint16_t minor;
    uint8_t proximity_uuid[16];
} uart_cmd_evt_t;

// Event handler type
typedef void (*uart_cmd_evt_handler_t)(const uart_cmd_evt_t *p_evt);

// Client data structure
typedef struct uart_cmd {
    uart_cmd_evt_handler_t evt_handler;
} uart_cmd_client_t;

// Module interface
uint32_t uart_cmd_init(uart_cmd_client_t* uart_cmd_client);
void uart_cmd_send_configuration_response(int error);
void uart_cmd_send_information_response(const char *info);

#endif //_UART_CMD_H
