/*
 * hardware.c
 * @brief
 * Created on: Jun 23, 2021
 * Author: Yanye
 */

#include "utils/hardware.h"

/**
 * @brief 获取电池电量
 * @return 电量等级0~100，步进值10，例(0, 10, 20, 30...90 100)
 * */
uint32_t ICACHE_FLASH_ATTR hw_get_battery_level(void) {
	// 10% 20% ... 90% 100%
	const float batlut[] = {3.68f, 3.74f, 3.77f, 3.79f, 3.82f, 3.87f, 3.92f, 3.98f, 4.06f, 4.10f, 4.50f};
	const float RES_TOTAL = 61.0F;
	const float RES_CAP = 10.0F;
	uint32_t i;
	uint16 level = system_adc_read();

	// 采样点电压(V) 51K对10K
	float voltage = (level * 0.9765f) / 1000.0f;
	voltage = (voltage * RES_TOTAL) / RES_CAP;

	for(i = 0; i < 11; i++) {
		if(voltage < batlut[i]) {
			break;
		}
	}

	return (i * 10);
}


