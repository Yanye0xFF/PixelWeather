/*
 * forecast_weather.h
 * @brief
 * Created on: Jun 24, 2021
 * Author: Yanye
 */

#ifndef _FORECAST_WEATHER_H_
#define _FORECAST_WEATHER_H_

#include "c_types.h"

// 预报天气结构单条，每项代表一天的天气情况 56bytes
typedef struct _forecast_weather {
	uint8_t dayTag[12];
	uint8_t weatherDesc[24];
	uint8_t windDesc[16];
	// 最低/最高温度
	sint8_t tempLowest;
	sint8_t tempHighest;
	sint8_t weatherIcon;
	uint8_t : 8;
} ForecastWeather;


#endif /* APP_INCLUDE_MODEL_FORECAST_WEATHER_H_ */
