/*
 * eventbus.c
 * @brief
 * Created on: Mar 22, 2021
 * Author: Yanye
 */
#include "utils/eventbus.h"
// 单例实现
static EventBus *evtbus = NULL;
static uint32_t EntityBitmap;

static void ICACHE_FLASH_ATTR regist(EventHandler handler, uint32_t eventId);
static void ICACHE_FLASH_ATTR unregist(EventHandler handler);
static void ICACHE_FLASH_ATTR post(uint32_t eventId, uint32_t arg);

EventBus * ICACHE_FLASH_ATTR EventBusGetDefault(void) {
	if(evtbus == NULL) {
		EntityBitmap = 0x00000000;
		evtbus = (EventBus *)os_malloc(sizeof(EventBus));
		evtbus->handlerEntity = (HandlerEntity *)os_malloc(sizeof(HandlerEntity) * SUBSCRIBER_MAX);
		evtbus->regist = regist;
		evtbus->unregist = unregist;
		evtbus->post = post;
	}
	return evtbus;
}

/**
 * @brief 注册订阅，由于使用了EntityBitmap，最多支持32个订阅者
 * @note 一个EventHandler对应唯一的eventId
 * @param handler 响应事件的接口
 * @param eventId 可用响应的事件ID
 * */
static void ICACHE_FLASH_ATTR regist(EventHandler handler, uint32_t eventId) {
	uint32_t i;
	for(i = 0; i < SUBSCRIBER_MAX; i++) {
		if(!((EntityBitmap >> i) & 0x1)) {
			EntityBitmap |= ((uint32_t)1 << i);
			(evtbus->handlerEntity + i)->handler = handler;
			(evtbus->handlerEntity + i)->eventId = eventId;
			break;
		}
	}
}

/**
 * @brief 解除订阅
 * @param handler 响应事件的接口
 * */
static void ICACHE_FLASH_ATTR unregist(EventHandler handler) {
	uint32_t i;
	for(i = 0; i < SUBSCRIBER_MAX; i++) {
		if(((EntityBitmap >> i) & 0x1) && ((size_t)((evtbus->handlerEntity + i)->handler) == (size_t)handler)) {
			EntityBitmap &= ~((uint32_t)1 << i);
			break;
		}
	}
}

/**
 * @brief 向总线广播一条消息
 * @param eventId 事件ID
 * @param arg 附带的参数
 * */
static void ICACHE_FLASH_ATTR post(uint32_t eventId, uint32_t arg) {
	uint32_t i;
	for(i = 0; i < SUBSCRIBER_MAX; i++) {
		if(((EntityBitmap >> i) & 0x1) && ((evtbus->handlerEntity + i)->eventId == eventId)) {
			(evtbus->handlerEntity + i)->handler(eventId, arg);
			break;
		}
	}
}
