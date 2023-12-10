/*
 * appicon.h
 * @brief
 * Created on: Nov 18, 2020
 * Author: Yanye
 */

#ifndef APP_INCLUDE_VIEW_APPICON_H_
#define APP_INCLUDE_VIEW_APPICON_H_

#include "c_types.h"

typedef struct _pair {
	sint8_t key;
	uint8_t value;
} Pair;

const char * ICACHE_FLASH_ATTR getWeatherIcon(sint8_t weatherId);

#endif /* APP_INCLUDE_VIEW_APPICON_H_ */
