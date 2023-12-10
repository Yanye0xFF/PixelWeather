/*
 * dht11.h
 * @brief
 * Created on: Nov 26, 2020
 * Author: Yanye
 */

#ifndef APP_INCLUDE_DRIVER_DHT11_H_
#define APP_INCLUDE_DRIVER_DHT11_H_

#include "c_types.h"
#include "eagle_soc.h"
#include "gpio.h"
#include "osapi.h"

/**
 * NODE MCU 引脚连接图
 * DHT11    NODEMCU
 * DATA     D3
 * GND      GND
 * 3V3      3V3
 * */
#define DHT_DATA_PIN         (0)

// DHT11送出数据高位在前, dht11小数部分为预留字段, 读出为零
// 8位湿度整数+8位湿度小数+8位温度整数+8位温度小数+8位校验
typedef struct _temphum_entity {
	// 8位温度小数
	uint8_t tempLowPart;
	// 8位温度整数
	sint8_t tempHighPart;
	// 8位湿度小数
	uint8_t humLowPart;
	// 8位湿度整数
	sint8_t humHighPart;
} TempHumEntity;

typedef union _temphum {
	TempHumEntity entity;
	uint32_t bytes;
}TempHum;

void ICACHE_FLASH_ATTR DHT11GpioSetup(void);

BOOL ICACHE_FLASH_ATTR DHT11Refresh(void);

TempHumEntity * ICACHE_FLASH_ATTR DHT11Read(void);

#endif /* APP_INCLUDE_DRIVER_DHT11_H_ */
