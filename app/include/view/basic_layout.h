/*
 * basic_layout.h
 * @brief
 * Created on: Jun 25, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_VIEW_BASIC_LAYOUT_H_
#define APP_INCLUDE_VIEW_BASIC_LAYOUT_H_

#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"

#include "model/basic_weather.h"
#include "model/calendar.h"
#include "model/status_bar.h"
#include "graphics/font.h"
#include "graphics/displayio.h"
#include "controller/context.h"
#include "utils/rtc_mem.h"
#include "utils/strings.h"
#include "driver/ssd1675b.h"

void ICACHE_FLASH_ATTR invalidateBasic(Calendar *calendar, BasicWeather *weather, StatusBar *status);

#endif /* APP_INCLUDE_VIEW_BASIC_LAYOUT_H_ */
