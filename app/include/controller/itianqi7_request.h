/*
 * itianqi7_request.h
 * @brief
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_CONTROLLER_ITIANQI7_REQUEST_H_
#define APP_INCLUDE_CONTROLLER_ITIANQI7_REQUEST_H_

#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"

#include "model/basic_weather.h"
#include "model/calendar.h"

#include "controller/context.h"

#include "network/http_utils.h"

#include "utils/strings.h"
#include "utils/sysconf.h"
#include "utils/eventbus.h"
#include "utils/eventdef.h"

void ICACHE_FLASH_ATTR requestItianqi7(uint32_t callbackId, uint32_t nextId);

#endif /* APP_INCLUDE_CONTROLLER_ITIANQI7_REQUEST_H_ */
