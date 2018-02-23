#include <stdint.h>
#include <stdio.h>
#include "hex_utils.h"

static const char hex_chars[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

void uint8_to_hex_char(uint8_t b, char *buf) {
    buf[0] = hex_chars[(b >> 4) & 0x0f];
    buf[1] = hex_chars[b & 0x0f];
}

void hex_string_to_uint8_array(const char *str, int str_len, uint8_t *buf) {
    if (str_len < 0 || str_len % 2 == 1) {
        return;
    }
    for (int pos = 0; pos < str_len; pos += 2) {
        unsigned int b;
        sscanf(str + pos, "%2x", &b); // no error checking...
        buf[pos / 2] = (uint8_t) b;
    }
}
