/*
 * dht11.c
 * @brief 和flash按钮共用引脚
 * Created on: Nov 26, 2020
 * Author: Yanye
 */

#include "driver/dht11.h"

static TempHum cache;

void ICACHE_FLASH_ATTR DHT11GpioSetup(void) {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	os_memset(&cache, 0x00, sizeof(TempHum));
}

/**
 * 刷新温度传感器缓存, 单总线读取dht11传感器
 * @return TRUE:读取成功且校验和通过
 * */
BOOL ICACHE_FLASH_ATTR DHT11Refresh(void) {
	TempHum tempHum;
	uint8_t slaveAck = GPIO_PIN_HIGH;
	uint8_t slaveStart = GPIO_PIN_LOW;
	// dataArea数据区, part 55us间隔信号 中点处
	uint32_t i = 0, sumArea = 0x00, delay = 27;
	tempHum.bytes = 0x00000000;

	// 主机把总线拉低必须大于18毫秒
	GPIO_OUTPUT_SET(DHT_DATA_PIN, GPIO_PIN_LOW);
	os_delay_us(20000);

	// 主机拉高20~40us
	GPIO_OUTPUT_SET(DHT_DATA_PIN, GPIO_PIN_HIGH);
	os_delay_us(20);

	// DHT11 拉低总线80us(实测75us), 中点40us处采样
	GPIO_DIS_OUTPUT(DHT_DATA_PIN);
	os_delay_us(40);
	slaveAck = GPIO_INPUT_GET(DHT_DATA_PIN);
	if(slaveAck == GPIO_PIN_HIGH) {
		return FALSE;
	}

	// DHT11 拉高总线80us(实测85us),准备发送数据
	os_delay_us(35 + 45);
	slaveStart = GPIO_INPUT_GET(DHT_DATA_PIN);
	if(slaveStart == GPIO_PIN_LOW) {
		return FALSE;
	}
	os_delay_us(45);

	// 读取温湿度
	for(; i < 32; i++) {
		// 50us低电平时隙开始(实测55us), 27us处采样
		os_delay_us(delay);

		// 等待dht11 数据拉高
		while(GPIO_INPUT_GET(DHT_DATA_PIN) == GPIO_PIN_LOW) {
			os_delay_us(1);
		}

		os_delay_us(25 + 20);
		if(GPIO_INPUT_GET(DHT_DATA_PIN) == GPIO_PIN_LOW) {
			tempHum.bytes |= (0 << (31 - i));
			delay = 2;
		}else {
			tempHum.bytes |= ((uint32_t)1 << (31 - i));
			delay = 25 + 27;
		}
	}

	// 读取校验和
	for(i = 0; i < 8; i++) {
		// 50us低电平时隙开始(实测55us), 27us处采样
		os_delay_us(delay);
		// 等待dht11 数据拉高
		while(GPIO_INPUT_GET(DHT_DATA_PIN) == GPIO_PIN_LOW) {
			os_delay_us(1);
		}

		os_delay_us(25 + 20);
		if(GPIO_INPUT_GET(DHT_DATA_PIN) == GPIO_PIN_LOW) {
			sumArea |= ((uint32_t)0 << (7 - i));
			delay = 2;
		}else {
			sumArea |= ((uint32_t)1 << (7 - i));
			delay = 25 + 27;
		}
	}
	// i 复用做校验和
	i = tempHum.entity.humHighPart + tempHum.entity.humLowPart
			+ tempHum.entity.tempHighPart + tempHum.entity.tempLowPart;

	if((i & 0xFF) == (sumArea & 0xFF)) {
		os_memcpy(&cache, &tempHum, sizeof(TempHum));
	}

	return ((i & 0xFF) == (sumArea & 0xFF));
}

/**
 * @brief 读取DHT11温湿度数据，总是可以得到返回值
 * @return TempHumEntity
 * */
TempHumEntity * ICACHE_FLASH_ATTR DHT11Read(void) {
	return &(cache.entity);
}
