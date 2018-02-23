#ifndef HEX_UTILS_H__
#define HEX_UTILS_H__

void uint8_to_hex_char(uint8_t b, char* buf);
void hex_string_to_uint8_array(const char *str, int str_len, uint8_t *buf);

#endif // HEX_UTILS_H__

