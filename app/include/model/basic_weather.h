/*
 * basic_weather.h
 * @brief
 * Created on: Jun 24, 2021
 * Author: Yanye
 */

#ifndef _TODAY_WEATHER_H_
#define _TODAY_WEATHER_H_

#include "c_types.h"

// 基础天气信息结构 172bytes
typedef struct _basic_weather {
	// 地区名称 最长7个UTF8
	uint8_t cityName[16];
	// 湿度
	uint8_t humidity;
	// 最低/最高温度
	sint8_t tempLowest;
	sint8_t tempHighest;
	// 天气图标(值>=0) 当值为负数时表明itianqi网站访问异常
	sint8_t weatherIcon;
	// 天气描述
	uint8_t weatherDesc[16];
	// 紫外线强度描述
	uint8_t ultravioletDesc[16];
	// 风向风力描述
	uint8_t windDesc[24];
	// 明天：24℃～31℃ 阴转雨
	uint8_t tomorrowWeatherDesc[48];
	// 后天：23℃～27℃ 小雨转大雨
	uint8_t dayAfterTomorrowWeatherDesc[48];
} BasicWeather;

#endif /* APP_INCLUDE_MODEL_TODAY_WEATHER_H_ */
