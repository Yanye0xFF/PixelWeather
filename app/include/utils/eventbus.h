/*
 * eventbus.h
 * @brief eventbus的精简实现，不支持粘性事件，同一事件ID的分发顺序等于注册顺序
 * Created on: Mar 22, 2021
 * Author: Yanye
 */
#ifndef _EVENTBUS_H_
#define _EVENTBUS_H_

#include "c_types.h"
#include "osapi.h"
#include "mem.h"

#define SUBSCRIBER_MAX    (32)


typedef void (* EventHandler)(uint32_t eventId, uint32_t arg);

typedef struct _handler_entity {
	uint32_t eventId;
	EventHandler handler;
} HandlerEntity;

typedef struct _eventbus {
	HandlerEntity *handlerEntity;
	// register为C关键字 这里使用regist替代
	void (* regist)(EventHandler handler, uint32_t eventId);
	void (* unregist)(EventHandler handler);
	void (* post)(uint32_t eventId, uint32_t arg);
	// void (* delete)(EventBus *instance);
} EventBus;

EventBus * ICACHE_FLASH_ATTR EventBusGetDefault(void);

#endif /* _EVENTBUS_H_ */
