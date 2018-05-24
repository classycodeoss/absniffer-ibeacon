#ifndef _NVCONFIG_H
#define _NVCONFIG_H

#include <stdint.h>

typedef struct {
    uint8_t beacon_uuid[16];
    uint16_t beacon_major;
    uint16_t beacon_minor;
} configuration_t;

uint32_t nvconfig_init();
uint32_t nvconfig_save(configuration_t* cfg);
uint32_t nvconfig_load(configuration_t* cfg);

#endif // _NVCONFIG_H
