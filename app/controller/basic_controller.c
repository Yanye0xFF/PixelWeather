/*
 * basic_controller.c
 * @brief 默认页面控制器
 * Created on: Jun 25, 2021
 * Author: Yanye
 */

#include "controller/basic_controller.h"

void ICACHE_FLASH_ATTR basicHttpResultHandler(uint32_t eventId, uint32_t arg);

/**
 * @brief 初始化一个eventbus回调
 * */
void ICACHE_FLASH_ATTR basicControllerInit(void) {
	EventBusGetDefault()->regist(basicHttpResultHandler, BASIC_CTRL_EVENT_UPDATE);
}

void ICACHE_FLASH_ATTR requestBasicWeather(void) {
	// 启动第一个请求, 后续的请求会通过eventbus传递顺序 7 8 102 suning
	// 超时处理独立，html解析即使失败仍会请求下一步
	requestItianqi7(BASIC_CTRL_EVENT_UPDATE, CTRL_NEXT_STEP1);
}

void ICACHE_FLASH_ATTR basicHttpResultHandler(uint32_t eventId, uint32_t arg) {

	if(arg == CTRL_NEXT_STEP1) {
		requestItianqi8(BASIC_CTRL_EVENT_UPDATE, CTRL_NEXT_STEP2);
	}else if(arg == CTRL_NEXT_STEP2) {
		requestItianqi102(BASIC_CTRL_EVENT_UPDATE, CTRL_NEXT_STEP3);
	}else if(arg == CTRL_NEXT_STEP3) {
		requestItianqi3(BASIC_CTRL_EVENT_UPDATE, CTRL_NEXT_STEP4);
	}else if(arg == CTRL_NEXT_STEP4) {
		// 请求suning时间接口
		requestCalendar(BASIC_CTRL_EVENT_UPDATE, CTRL_FINISHED);
	}else if(arg == CTRL_FINISHED) {
		// 通知main.c
		EventBusGetDefault()->post(MAIN_EVENT_REQUEST_FINISH, 0);
	}

}


