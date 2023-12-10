/*
 * forecast_layout.h
 * @brief
 * Created on: Jun 25, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_VIEW_FORECAST_LAYOUT_H_
#define APP_INCLUDE_VIEW_FORECAST_LAYOUT_H_

#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"

#include "model/forecast_weather.h"
#include "model/calendar.h"
#include "model/status_bar.h"
#include "utils/rtc_mem.h"
#include "utils/strings.h"
#include "graphics/font.h"
#include "graphics/displayio.h"

#include "driver/ssd1675b.h"
#include "controller/context.h"

void ICACHE_FLASH_ATTR invalidateForecast(Calendar *calendar, BasicWeather *weather, ForecastWeather *forecastWeathers, StatusBar *status);

#endif /* APP_INCLUDE_VIEW_FORECAST_LAYOUT_H_ */
