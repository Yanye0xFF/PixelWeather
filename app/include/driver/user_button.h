/*
 * user_button.h
 * @brief
 * Created on: May 20, 2021
 * Author: Yanye
 */

#ifndef _DRIVER_USER_BUTTON_H_
#define _DRIVER_USER_BUTTON_H_

#include "eagle_soc.h"
#include "os_type.h"
#include "c_types.h"
#include "gpio.h"
#include "osapi.h"

typedef enum key_event {
	KEY_EVENT_UP = 0,
	KEY_EVENT_DOWN
} KeyEvent;

typedef void (* ClickListener)(void);

typedef void (* DoubleClickListener)(void);

typedef void (* LongClickListener)(void);

// 长按最短持续时间(us)
#define LONG_CLICK_TIME       (3000000)

// 双击检测抬起超时(us)
#define DOUBLE_CLICK_TIMEOUT   (500000)

// 双击检测次数(次)
#define DOUBLE_CLICK_COUNT     (2)

// 单击定时器延时触发(ms), 要比DOUBLE_CLICK_TIMEOUT稍大
#define SINGLE_CLICK_TRIGGER_DELAY    (600)

#define USER_BUTTON_PIN       (5)

void ICACHE_FLASH_ATTR UserButtonInit(void);

void ICACHE_FLASH_ATTR UserButtonIRQInit(void);

uint8_t ICACHE_FLASH_ATTR UserButtonLevelRead(void);

void ICACHE_FLASH_ATTR UserButtonAddClickListener(ClickListener clickListener);

void ICACHE_FLASH_ATTR UserButtonAddDoubleClickListener(DoubleClickListener doubleClickListener);

void ICACHE_FLASH_ATTR UserButtonAddLongClickListener(LongClickListener longClickListener);

#endif /* APP_INCLUDE_DRIVER_USER_BUTTON_H_ */
