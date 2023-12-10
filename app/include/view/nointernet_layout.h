/*
 * nointernet_layout.h
 * @brief
 * Created on: Jun 30, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_VIEW_NOINTERNET_LAYOUT_H_
#define APP_INCLUDE_VIEW_NOINTERNET_LAYOUT_H_

#include "c_types.h"
#include "user_interface.h"

#include "model/calendar.h"
#include "model/status_bar.h"

#include "graphics/font.h"
#include "graphics/displayio.h"

#include "utils/rtc_mem.h"
#include "utils/strings.h"

void ICACHE_FLASH_ATTR invalidateNoInternet(Calendar *calendar, StatusBar *status);

#endif /* APP_INCLUDE_VIEW_NOINTERNET_LAYOUT_H_ */
