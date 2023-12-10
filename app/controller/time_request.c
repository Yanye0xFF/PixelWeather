/*
 * calendar_controller.c
 * @brief 请求suning的时间接口获取日期时间
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#include "../include/controller/time_request.h"
#include "../include/utils/misc.h"

static uint32_t eventCallbackId, nextRequestId;

static void ICACHE_FLASH_ATTR calendarRecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode);

/**
 * @brief 请求http://api.m.taobao.com/rest/api3.do?api=mtop.common.getTimestamp 获取日期时间数据
 * @param none
 * @return none
 * */
void ICACHE_FLASH_ATTR requestCalendar(uint32_t callbackId, uint32_t nextId) {
	uint8_t urlBuffer[128];
	HttpUtils *http;

	os_memset(urlBuffer, 0x00, sizeof(urlBuffer));
	http = HttpGetInstance();
	eventCallbackId = callbackId;
	nextRequestId = nextId;

	Context.dumpURL(SystimeUrl, SYSTIME_URL_LENGTH, urlBuffer);

	http->setOnRecvCallback((onRecvCallback)calendarRecvCallback);
	http->doGet(urlBuffer);
	// leave this function immediately
}

/**
 * @brief http回调
 * @param htmlBody http响应中body部分数据，已经在末尾补'\0'
 * @param length body部分数据数据长度
 * @param httpCode http响应码，正常应为200
 * */
static void ICACHE_FLASH_ATTR calendarRecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode) {
	char ts[32];
	int32_t pos, end, i;
	int utc_time = 0;
	int time_result[6];

	uint32 rtc_time;
	Calendar *calendar;
	Date date;

	HttpGetInstance()->disconnect();
	calendar = Context.getCalendar();

	// clean
	calendar->year = 0;
	calendar->month = 0;
	calendar->day = 0;
	calendar->hour = 0;
	calendar->minute = 0;
	calendar->second = 0;

	// {"api":"mtop.common.getTimestamp","v":"*","ret":["SUCCESS::接口调用成功"],"data":{"t":"1640181936797"}}
	// 这里只前10位，精确到秒级

	os_memset(ts, 0, sizeof(ts));
	pos = contains(htmlBody, "data");

	if(pos > 0) {
		pos += 12;
		end = charAt(htmlBody + pos, '\"') - 3;
		os_memcpy(ts, (htmlBody + pos), end);

		for(i = 0; i < end; i++){
			utc_time = 10 * utc_time + (ts[i] - '0');
		}

		praseTimestamp(utc_time, 8, time_result);

		calendar->year = time_result[yyyy];
		calendar->month = time_result[MM];
		calendar->day = time_result[dd];
		calendar->hour = time_result[HH];
		calendar->minute = time_result[mm];
		calendar->second = time_result[ss];

		// 仅复制共有部分
		os_memcpy(&date, calendar, sizeof(Date));
		// 将当前时间写入rtc memory
		rtc_time = system_get_time();
		system_rtc_mem_write(NETUPDATE_POS, (const void *)&rtc_time, sizeof(uint32_t));
		system_rtc_mem_write(DATE_POS, (const void *)&date, sizeof(Date));
	}

	// 通过eventbus传递消息
	EventBusGetDefault()->post(eventCallbackId, nextRequestId);
}
