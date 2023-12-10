/*
 * sysconfig.h
 * @brief
 * Created on: Sep 9, 2020
 * Author: Yanye
 */

#ifndef APP_USER_SYSCONF_H_
#define APP_USER_SYSCONF_H_

#include "c_types.h"

// update 20210803 humidity范围问题
#define SOFTWARE_VERSION       (0x0E)
#define HARDWARE_VERSION       (0x05)

#define DEFAULT_NAME_PREFIX    ("天气站")
#define DEFAULT_NAME_LENGTH    (9)

#define MAX_CONFIG_BIT         (4)
#define CONFIG_VALUE_MAX       (1)

#define DEFAULT_SOFTAP_SSID    ("PixelWeather")
#define DEFAULT_SOFTAP_PASS    ("11223344")

#define SYSTEM_CONFIG_LENGTH    180
#define STATUS_VALID            0
#define STATUS_INVALID          1

#define RUNTIME_INTERNET_BIT    0
#define RUNTIME_UPDATE_BIT      1
#define MAX_RUNTIME_BIT        (1)
#define RUNTIME_VALUE_MAX      (1)

/**
 * @brief 运行时数据, 重启全部复位0
 * @brief 位定义: 1:TRUE, 0:FALSE
 * */
typedef struct _system_runtime {
	// 是否可联网, 成功请求一次即被置位
	// 每次request之前复位
	uint8_t internet : 1;
	// 固件升级标记为，为TRUE时，软件重启前显示升级中图片
	uint8_t update: 1;
	uint8_t : 6;
} SystemRunTime;

/**
 * @brief 系统配置项位定义
 * @brief 位定义: 1:FALSE, 0:TRUE
 * */
typedef struct _system_status {
	// 是否配置了wifi/城市/刷新时间等
	uint8_t isConfiged : 1;
	// 连接时是否需要密码
	uint8_t needPwd : 1 ;
	// 是否使用自定义名字
	uint8_t customName : 1;
	// 是否配置完成资源文件
	uint8_t hasResource:1;
	// AP 是否需要密码
	uint8_t apNeedPwd : 1;
	uint8_t : 3;
} SystemStatus;

// 识别头
#define CONFIG_RECORD_VALID     0xFC
#define CONFIG_RECORD_INVALID   0xF0
#define CONFIG_RECORD_NULL      0xFF

// 天气/时间刷新上下限
#define CONFIG_UPDATE_MIN            1
#define CONFIG_WEATHER_UPDATE_MAX    24
#define CONFIG_TIME_UPDATE_MAX       30

// 字符串相关的最大长度
#define CONFIG_DEVICE_NAME_MAX    47
#define CONFIG_ACCESS_PWD_MAX     31
#define CONFIG_WIFI_NAME_MAX      31
#define CONFIG_WIFI_PWD_MAX       31
#define CONFIG_CITY_NAME_MAX      23
#define CONFIG_CITY_CODE_MAX      8

#define CONFIG_SYSTEM_STATUS_DEFAULT    0xFF

// 共180字节，读取时固定地址跳转
typedef struct _system_config {
	// 识别头(实际中没有使用，采用定长存储结构，每次只取最后一项数据)
	uint8_t header;
	// 系统配置项
	SystemStatus status;
	// 天气站名字\0结尾
	char deviceName[48]; // 2
	// 连接密码\0结尾
	char accessPwd[32]; // 50
	// wifi名字\0结尾
	char wifiName[32]; // 82
	// wifi密码\0结尾
	char wifiPwd[32];  // 114
	// 天气/时间联网更新间隔(CONFIG_UPDATE_MIN ~ CONFIG_WEATHER_UPDATE_MAX小时)
	uint8_t weatherUpdate; // 146
	// 时间本地刷新间隔(CONFIG_UPDATE_MIN ~ CONFIG_TIME_UPDATE_MAX分钟)
	uint8_t timeUpdate; //147
	// 城市拼音\0结尾
	char cityName[24]; // 148
	// 自动休眠参数
	char autoSleep[4]; // 172
	// 夜间不更新区间参数
	char nightSpan[4]; // 176
} SystemConfig;

void ICACHE_FLASH_ATTR sys_config_init();
SystemConfig * ICACHE_FLASH_ATTR sys_config_get();

void ICACHE_FLASH_ATTR sys_runtime_init();
SystemRunTime * ICACHE_FLASH_ATTR sys_runtime_get();
void ICACHE_FLASH_ATTR sys_runtime_set(uint8_t position, uint8_t value);

#endif
