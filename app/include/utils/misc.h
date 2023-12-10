/*
 * misc.h
 * @brief
 * Created on: Jul 6, 2021
 * Author: Yanye
 */

#ifndef _UTILS_MISC_H_
#define _UTILS_MISC_H_

#include "c_types.h"
#include "osapi.h"

#include "utils/rtc_mem.h"
#include "utils/strings.h"
#include "utils/sysconf.h"
#include "model/date.h"
#include "model/calendar.h"

#include "controller/context.h"

#define MAX_LEVEL    4

#define yyyy 0
#define MM 1
#define dd 2
#define HH 3
#define mm 4
#define ss 5

void ICACHE_FLASH_ATTR updateClockByTick(UpdateInfo *info, Date *date, uint32_t unit);

sint8_t ICACHE_FLASH_ATTR matchWeatherId(uint8_t *weatherDesc);

void ICACHE_FLASH_ATTR loadConfigIntoRTCMenory(void);

void ICACHE_FLASH_ATTR praseTimestamp(int ts, int timeZone, int *buffer);

#endif /* APP_INCLUDE_UTILS_MISC_H_ */
