/*
 * eventdef.h
 * @brief
 * Created on: May 22, 2021
 * Author: Yanye
 */

#ifndef _EVENTDEF_H_
#define _EVENTDEF_H_

// main.c 占用事件id 200~299
#define MAIN_EVENT_DISPLAY_IMAGE     200
#define MAIN_EVENT_REQUEST_FINISH    201
#define MAIN_EVENT_EPD_FINISH        202
#define MAIN_EVENT_CLOSE_TIMER       203
// arg参数
#define CLOSE_TIMER_AND_UPDATE       2031
#define MAIN_EVENT_POWERTOGGLE       204
// arg参数
#define POWERTOGGLE_SHUTDOWN         0     // 电源切换-关机
#define POWERTOGGLE_UPDATE           1     // 电源切换-更新固件
#define POWERTOGGLE_IDLE_TIMEOUT     2     // 电源切换-空闲超时

#define BASIC_CTRL_EVENT_UPDATE         300
// arg参数
#define CTRL_NEXT_STEP1        1
#define CTRL_NEXT_STEP2        2
#define CTRL_NEXT_STEP3        3
#define CTRL_NEXT_STEP4        4
#define CTRL_FINISHED          5

#endif /* APP_INCLUDE_UTILS_EVENTDEF_H_ */
