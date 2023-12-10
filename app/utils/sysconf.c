/*
 * sysconfig.c
 * @brief 配置读写顺序：sysConfExist，if TRUE sysConfRead else sysConfInit
 * Created on: Sep 9, 2020
 * Author: Yanye
 */

#include "mem.h"
#include "utils/sysconf.h"
#include "utils/fixed_file.h"

static SystemConfig *config = NULL;
static SystemRunTime runtime;

void ICACHE_FLASH_ATTR sys_config_init() {
	fixed_file_t fixedFile;
	fixed_file_init(&fixedFile, FILE_CONFIG_SECTION_SIZE, FILE_CONFIG_MAX_SIZE);

	if(fixed_file_open(&fixedFile, "sysconf", "ini")) {
		config = (SystemConfig *)os_malloc(sizeof(SystemConfig));
		if(fixed_file_read(&fixedFile, (uint8_t *)config, FILE_CONFIG_SECTION_SIZE) != FILE_CONFIG_SECTION_SIZE) {
			os_free(config);
			config = NULL;
		}
	}
}

/**
 * @brief 获取配置文件引用，需要判断NULL
 * */
SystemConfig * ICACHE_FLASH_ATTR sys_config_get() {
	return config;
}

/**
 * @brief 运行时数据初始化, 默认全0
 * */
void ICACHE_FLASH_ATTR sys_runtime_init() {
	os_memset(&runtime, 0x00, sizeof(SystemRunTime));
}

/**
 * @brief 读取系统运行时参数，修改参数请调用sysRuntimeSet
 * @return SystemRunTime* 系统运行时参数指针
 * */
SystemRunTime * ICACHE_FLASH_ATTR sys_runtime_get() {
	return (&runtime);
}

/**
 * @brief 系统运行时参数设置
 * @param position 标记位 位置 (0~7)
 * @param value 标记位 值 (0 or 1)
 * */
void ICACHE_FLASH_ATTR sys_runtime_set(uint8_t position, uint8_t value) {
	uint8_t reg;
	os_memcpy(&reg, &runtime, sizeof(uint8_t));
	reg &= ~((uint8_t)0x1 << position);
	reg |= ((value & 0x1) << position);
	os_memcpy(&runtime, &reg, sizeof(SystemRunTime));
}


