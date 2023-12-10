/*
 * status_bar.h
 * @brief
 * Created on: Jul 6, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_MODEL_STATUS_BAR_H_
#define APP_INCLUDE_MODEL_STATUS_BAR_H_

#include "c_types.h"

// 系统状态栏信息
typedef struct _status_bar {
	// 室内湿度
	uint8_t humidity;
	// 室内温度
	sint8_t temperature;
	// 电量等级
	uint8_t batLevel;
	// wifi信号强度
	sint8_t rssi;
	// 系统工作模式：正常，间歇唤醒，深度睡眠...
	uint8_t sysopmode;
	// 系统电源模式
	uint8_t syspowermode;
	// 字节补齐
	uint8_t dummy2;
	uint8_t dummy3;
} StatusBar;



#endif /* APP_INCLUDE_MODEL_STATUS_BAR_H_ */
