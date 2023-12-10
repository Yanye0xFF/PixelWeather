/*
 * calendar.h
 * @brief
 * Created on: Jun 24, 2021
 * Author: Yanye
 */

#ifndef _CALENDAR_H_
#define _CALENDAR_H_

#include "c_types.h"

// 时间公历/农历结构 88bytes
typedef struct _calendar {
	// 年 0~65535
    uint16_t year;
    // 月0~12
    uint8_t month;
    // 日0~31
    uint8_t day;
    // 小时 0~23
    uint8_t hour;
    // 分0~59
    uint8_t minute;
    // 秒0~59
    uint8_t second;
    // 字节对齐
    uint8_t : 8;
    // 公历描述信息 2021年06月24日  星期四
    uint8_t calendarDesc[40];
    // 农历描述信息 农历辛丑牛年 五月十五
    uint8_t lunarDesc[40];
} Calendar;


#endif /* APP_INCLUDE_MODEL_CALENDAR_H_ */
