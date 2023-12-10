/*
 * record_file.h
 * @brief 定长记录文件，每次写入以追加的方式加入文件尾部，同时更新文件头
 * @note section_size用于指定每段记录的长度，
 *       max_size用于指定最大存储长度，超出后会删除旧文件创建新的
 * Created on: Nov 2, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_UTILS_FIXED_FILE_H_
#define APP_INCLUDE_UTILS_FIXED_FILE_H_

#include "c_types.h"
#include "spifsmini/spifs.h"

#define FILE_CONFIG_SECTION_SIZE     180
#define FILE_CONFIG_MAX_SIZE         4088

#define FILE_PAGE_SECTION_SIZE     9
#define FILE_PAGE_MAX_SIZE         4088

#define FILE_IMAGE_SECTION_SIZE    15
#define FILE_IMAGE_MAX_SIZE        4088

#define FILE_NOTE_SECTION_SIZE     256
#define FILE_NOTE_MAX_SIZE         25600

typedef struct _fixed_file {
	File file;
	uint32_t max_size;
	uint32_t section_size;
} fixed_file_t;

void ICACHE_FLASH_ATTR fixed_file_init(fixed_file_t *rfile, uint32_t section_size, uint32_t max_size) ;

BOOL ICACHE_FLASH_ATTR fixed_file_create(fixed_file_t *rfile, char *filename, char *extname);

BOOL ICACHE_FLASH_ATTR fixed_file_open(fixed_file_t *rfile, char *filename, char *extname);

BOOL ICACHE_FLASH_ATTR fixed_file_append(fixed_file_t *rfile, uint8_t *data, uint32_t length);

BOOL ICACHE_FLASH_ATTR fixed_file_update(fixed_file_t *rfile, uint32_t offset, uint8_t data);

uint32_t ICACHE_FLASH_ATTR fixed_file_read(fixed_file_t *rfile, uint8_t *buffer, uint32_t length);

#endif /* APP_INCLUDE_UTILS_FIXED_FILE_H_ */
