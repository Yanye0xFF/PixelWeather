/*
 * itianqi3_request.h
 * @brief
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#ifndef _CONTROLLER_ITIANQI3_REQUEST_H_
#define _CONTROLLER_ITIANQI3_REQUEST_H_

#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"

#include "model/forecast_weather.h"
#include "controller/context.h"
#include "network/http_utils.h"

#include "utils/strings.h"
#include "utils/sysconf.h"
#include "utils/eventbus.h"
#include "utils/eventdef.h"
#include "utils/misc.h"

void ICACHE_FLASH_ATTR requestItianqi3(uint32_t callbackId, uint32_t nextId);

#endif /* APP_INCLUDE_CONTROLLER_ITIANQI3_REQUEST_H_ */
