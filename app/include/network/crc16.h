#ifndef _CRC16_H_
#define _CRC16_H_

#include "c_types.h"

uint16_t ICACHE_FLASH_ATTR crc16_ccitt(uint8_t *buf, int len);

#endif
