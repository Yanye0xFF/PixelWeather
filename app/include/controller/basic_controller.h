/*
 * basic_controller.h
 * @brief
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_CONTROLLER_BASIC_CONTROLLER_H_
#define APP_INCLUDE_CONTROLLER_BASIC_CONTROLLER_H_

#include "c_types.h"

#include "model/basic_weather.h"

#include "controller/context.h"

#include "utils/eventdef.h"
#include "utils/eventbus.h"

void ICACHE_FLASH_ATTR basicControllerInit(void);

void ICACHE_FLASH_ATTR requestBasicWeather(void);


#endif /* APP_INCLUDE_CONTROLLER_BASIC_CONTROLLER_H_ */
