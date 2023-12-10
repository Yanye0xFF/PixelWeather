/*
 * record_file.c
 * @brief
 * Created on: Nov 2, 2021
 * Author: Yanye
 */

#include "../include/utils/fixed_file.h"
#include "osapi.h"
#include "c_types.h"

/**
 * @brief 指定定长文件的节大小和最大容量
 * */
void ICACHE_FLASH_ATTR fixed_file_init(fixed_file_t *rfile, uint32_t section_size, uint32_t max_size) {
	rfile->section_size = section_size;
	rfile->max_size = max_size;
}

BOOL ICACHE_FLASH_ATTR fixed_file_create(fixed_file_t *rfile, char *filename, char *extname) {
	FileInfo info;
	Result res;
	make_finfo(&info, 2000, 1, 1, FSTATE_SYSTEM);
	make_file(&(rfile->file), filename, extname);
	res = create_file(&(rfile->file), &info);
	return (res == CREATE_FILE_SUCCESS);
}

BOOL ICACHE_FLASH_ATTR fixed_file_open(fixed_file_t *rfile, char *filename, char *extname) {
	BOOL result = open_file(&(rfile->file), filename, extname);
	return result;
}

BOOL ICACHE_FLASH_ATTR fixed_file_append (fixed_file_t *rfile, uint8_t *data, uint32_t length) {
	Result result;
	FileInfo finfo;
	File *file = &(rfile->file);
	// 超出一个扇区rfile.max_size字节的记录删掉当前文件，重新找个扇区新建文件, 避免记录过多无用数据导致打开配置文件变慢
	if((file->length + length) > rfile->max_size) {
		delete_file(file);
		make_finfo(&finfo, 2000, 1, 1, FSTATE_SYSTEM);
		create_file(file, &finfo);
		result = write_file(file, data, length, OVERRIDE);
		return (result == WRITE_FILE_SUCCESS);

	}else {
		result = write_file(file, data, length, APPEND);
		if(APPEND_FILE_SUCCESS != result) {
			return FALSE;
		}
		result = write_finish(file);
		return (result == APPEND_FILE_FINISH);
	}
}

/**
 * @brief 更新定长文件的指定字节
 * @note 由于flash特性，只能从1更新为0, 只能用于max_size<=4088的文件(一个物理扇区大小之内)
 * @param offset 被修改字节在最后一个section的偏移量, 从0开始计数
 * @param data 被修改的字节数据，需要先读出原始值，在使用&清除对应位，最后回写
 * */
BOOL ICACHE_FLASH_ATTR fixed_file_update(fixed_file_t *rfile, uint32_t offset, uint8_t data) {
	uint8_t rawData;
	uint32_t address;
	Integer temp;
	File *file = &(rfile->file);

	if((offset >= rfile->section_size) || (offset >= rfile->file.length)) {
		return FALSE;
	}

	address = file->cluster + SECTOR_MARK_SIZE + (file->length - rfile->section_size + offset);
	// flash只能4字节对齐读写
	spi_flash_read(address, &(temp.value), sizeof(uint32_t));
	temp.bytes[0] = data;
	spi_flash_write(address, &(temp.value), sizeof(uint32_t));

	return TRUE;
}

/**
 * @brief 读取文件最后一个有效数据记录
 * @param length 读取的长度, 必须<=rfile->section_size
 * @return 实际读出的长度
 * */
uint32_t ICACHE_FLASH_ATTR fixed_file_read(fixed_file_t *rfile, uint8_t *buffer, uint32_t length) {
	uint32_t readin;
	File *file = &(rfile->file);
	if((file->length == EMPTY_INT_VALUE) || (file->length < rfile->section_size)) {
		return 0;
	}
	readin = read_file(file, (file->length - rfile->section_size), buffer, length);
	return readin;
}
