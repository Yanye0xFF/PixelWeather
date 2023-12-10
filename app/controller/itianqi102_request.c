/*
 * itianqi102_request.c
 * @brief 请求itianqi id=102 获取当前城市名称，相对湿度，紫外线强度
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#include "controller/itianqi102_request.h"

static uint32_t eventCallbackId, nextRequestId;

static void ICACHE_FLASH_ATTR itianqi102RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode);

void ICACHE_FLASH_ATTR requestItianqi102(uint32_t callbackId, uint32_t nextId) {
	SystemConfig *config;
	uint8_t urlBuffer[96];
	HttpUtils *http;

	os_memset(urlBuffer, 0x00, sizeof(urlBuffer));
	http = HttpGetInstance();
	config = sys_config_get();
	eventCallbackId = callbackId;
	nextRequestId = nextId;

	// http://i.tianqi.com/index.php?c=code&a=getcode&id=102&py=changzhou
	Context.dumpURL(ItianqiUrl, ITIANQI_URL_LENGTH, urlBuffer);
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH), "102&py=");
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH + 7), (const char *)config->cityName);
	http->setOnRecvCallback((onRecvCallback)itianqi102RecvCallback);
	http->doGet(urlBuffer);
	// leave this function immediately
}

static void ICACHE_FLASH_ATTR itianqi102RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode) {
	BasicWeather *basicWeather;
	Calendar *calendar;
	// 跳过了大部分的 html header
	uint32_t cursor = 2860;
	int32_t next, i;
#define DUMP_BUFFER_SIZE    48
	uint8_t dump[DUMP_BUFFER_SIZE];

	HttpGetInstance()->disconnect();
	basicWeather = Context.getBasicWeather();
	calendar = Context.getCalendar();

	basicWeather->weatherIcon = -1;
	basicWeather->cityName[0] = '\0';
	basicWeather->humidity = 0;
	// ultravioletDesc使用的memcpy，因此先填充'\0'
	os_memset(basicWeather->ultravioletDesc, 0x00, 16);

	do {
		while((next = startWithEx((htmlBody + cursor), "<div class=\"picimg\">")) == -1) {
			next = nextLine(htmlBody + cursor);
			if(next == -1) {
				break;
			}
			cursor += next;
		}
		if(next == -1) {
			break;
		}
		cursor += next;
		// <div class="picimg"><img class='pngtqico' align='absmiddle' src='/static/images/tianqibig/b8.png'
		next = contains((htmlBody + cursor), "images");
		if(next == -1) {
			break;
		}
		cursor += next;
		next = charAt((htmlBody + cursor), '\'');
		// images/tianqibig/b8.png
		subString((htmlBody + cursor), 0, next, dump);

		// 提取图片id
		for(i = 0; i < DUMP_BUFFER_SIZE; i++) {
			if(dump[i] >= '0' && dump[i] <= '9') {
				break;
			}
		}
		basicWeather->weatherIcon = (sint8_t)atoi((const char *)(dump + i));

		// 获取地区名
		do{
			if((next = nextLine(htmlBody + cursor)) == -1) {
				break;
			}
			cursor += next;
		}while((next = startWithEx((htmlBody + cursor), "<p>")) == -1);
		if(next == -1) {
			break;
		}
		cursor += next;
		// <p>常州</p>
		cursor += 3;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, basicWeather->cityName);

		// 获取湿度
		do{
			if((next = nextLine(htmlBody + cursor)) == -1) {
				break;
			}
			cursor += next;
		}while((next = startWithEx((htmlBody + cursor), "<li class=\"box3\">")) == -1);
		if(next == -1) {
			break;
		}
		cursor += next;

		next = nextLine(htmlBody + cursor);
		cursor += next;
		next = startWithEx((htmlBody + cursor), "<em>");
		cursor += next;

		// 4 = strlen("<em>")
		cursor += 4;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, dump);
		// <em>相对湿度：72%</em>
		for(i = 0; i < next; i++) {
			if(dump[i] >= '0' && dump[i] <= '9') {
				break;
			}
		}
		basicWeather->humidity = atoi((const char *)(dump + i));

		next = nextLine(htmlBody + cursor);
		cursor += next;
		next = startWithEx((htmlBody + cursor), "<span>");
		cursor += next;

		// 6 = strlen("<span>")
		cursor += 6;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, dump);
		// <span>紫外线指数：中等</span>
		for(i = 0; i < (next - 3); i++) {
			// '：'的UTF8编码LSB->MSB EF BC 9A
			if(dump[i] == 0xEF && dump[i + 1] == 0xBC && dump[i + 2] == 0x9A) {
				i += 3;
				break;
			}
		}
		os_memcpy(basicWeather->ultravioletDesc, (dump + i), (next - i));

#ifdef REQUEST_DEBUG_ENABLE
		os_printf("end:itianqi102\n");
#endif
	}while(0);

	EventBusGetDefault()->post(eventCallbackId, nextRequestId);
}

