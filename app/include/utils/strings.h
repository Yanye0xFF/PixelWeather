/*
 * string_utils.h
 * @brief 字符串操作类，以单字节为基础，多字节编码需要自行解决，char类型作为unsigned char处理
 * Created on: Aug 23, 2020
 * Author: Yanye
 */

#ifndef APP_USER_STRINGS_H_
#define APP_USER_STRINGS_H_

#include "c_types.h"
#include "mem.h"
#include "osapi.h"
#include "spifsmini/spifs.h"

#define KMP_SUB_MAX_LENGTH    18
#define PARSE_ERROR    0

typedef uint8_t byte;

Short ICACHE_FLASH_ATTR QueryGB2312ByUnicode(uint16_t unicode);

uint8_t * ICACHE_FLASH_ATTR UnicodeToGB2312(uint8_t *unicode);

uint8_t ICACHE_FLASH_ATTR UTF8ToUnicode(uint8_t *utf8, uint32_t *unicode);

uint8_t ICACHE_FLASH_ATTR UnicodeToUTF16(uint32_t unicode, uint16_t *utf16);

uint32_t ICACHE_FLASH_ATTR endianSwap(uint32_t data, uint32_t length);

int32_t ICACHE_FLASH_ATTR charAt(char *str, char ch);

int32_t ICACHE_FLASH_ATTR charAtSkip(char *str, char ch);

int32_t ICACHE_FLASH_ATTR lastIndexOf(uint8_t *src, uint8_t ch);

int32_t ICACHE_FLASH_ATTR nextLine(uint8_t *str);

STATUS ICACHE_FLASH_ATTR subString(uint8_t *str, uint32_t from, uint32_t to, uint8_t *dest);

BOOL ICACHE_FLASH_ATTR startsWith(uint8_t *src, uint8_t *prefix);

int32_t ICACHE_FLASH_ATTR startWithEx(uint8_t *src, uint8_t *prefix);

BOOL ICACHE_FLASH_ATTR startsWithStr(char *src, const char *prefix);

BOOL ICACHE_FLASH_ATTR compareWith(char *src, const char *text);
BOOL ICACHE_FLASH_ATTR compareWithEx(char *src, uint8_t *target);

STATUS ICACHE_FLASH_ATTR trim(uint8_t *src);

int32_t ICACHE_FLASH_ATTR contains(uint8_t *src, uint8_t *sub);

uint32_t ICACHE_FLASH_ATTR strlenEx(char *str, uint32_t max);

int32_t ICACHE_FLASH_ATTR integer2String(int32_t value, uint8_t *buffer, uint32_t length);

#endif
