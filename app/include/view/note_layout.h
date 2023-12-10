/*
 * note_layout.h
 * @brief
 * Created on: Oct 28, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_VIEW_NOTE_LAYOUT_H_
#define APP_INCLUDE_VIEW_NOTE_LAYOUT_H_

#include "c_types.h"

#include "model/calendar.h"
#include "model/status_bar.h"
#include "model/basic_weather.h"

void ICACHE_FLASH_ATTR invalidateNoteView(Calendar *calendar, BasicWeather *weather, StatusBar *status);


#endif /* APP_INCLUDE_VIEW_NOTE_LAYOUT_H_ */
