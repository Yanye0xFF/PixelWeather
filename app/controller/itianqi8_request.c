/*
 * itianqi8_request.c
 * @brief 请求itianqi id=8 获取明天和后天的温度范围，天气描述
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#include "controller/itianqi8_request.h"

static void ICACHE_FLASH_ATTR itianqi8RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode);

static uint32_t eventCallbackId, nextRequestId;

void ICACHE_FLASH_ATTR requestItianqi8(uint32_t callbackId, uint32_t nextId) {
	SystemConfig *config;
	uint8_t urlBuffer[96];
	HttpUtils *http;

	os_memset(urlBuffer, 0x00, sizeof(urlBuffer));
	http = HttpGetInstance();
	config = sys_config_get();
	eventCallbackId = callbackId;
	nextRequestId = nextId;

	// "http://i.tianqi.com/index.php?c=code&a=getcode&id=8&py=changzhou"
	Context.dumpURL(ItianqiUrl, ITIANQI_URL_LENGTH, urlBuffer);
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH), "8&py=");
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH + 5), (const char *)config->cityName);
	http->setOnRecvCallback((onRecvCallback)itianqi8RecvCallback);
	http->doGet(urlBuffer);
	// leave this function immediately
}

static void ICACHE_FLASH_ATTR itianqi8RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode) {
	uint32_t cursor = 1900;
	int32_t next = 0, i;
	BasicWeather *basicWeather;

	HttpGetInstance()->disconnect();
	basicWeather = Context.getBasicWeather();

	// clean
	basicWeather->tomorrowWeatherDesc[0] = '\0';
	basicWeather->dayAfterTomorrowWeatherDesc[0] = '\0';

	do {
		// 获取明天的天气信息
		// <div id="day_2" class="wtline">明天：22℃～32℃ 多云转晴</div>
		while((next = startWithEx((htmlBody + cursor), "<div id=\"day_2\"")) == -1) {
			next = nextLine(htmlBody + cursor);
			if(next == -1) {
				break;
			}
			cursor += next;
		}
		if(next == -1) {
			break;
		}
		// 31 = os_strlen("<div id=\"day_2\" class=\"wtline\">")
		cursor += (next + 31);
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, basicWeather->tomorrowWeatherDesc);
		// 去除中间的空格
		for(i = 0; i < next; i++) {
			// 找到第一个空格
			if(basicWeather->tomorrowWeatherDesc[i] == 0x20) break;
		}
		// 把'\0'也向前移一位，(next - i)不需要再减1
		os_memmove((basicWeather->tomorrowWeatherDesc + i), (basicWeather->tomorrowWeatherDesc + i + 1), (next - i));

		cursor += next;

		next = nextLine(htmlBody + cursor);
		cursor += next;
		// 获取后天的天气信息
		// <div id="day_3" class="wtline" style="display:none;">后天：22℃～31℃ 多云转雨</div>
		next = startWithEx((htmlBody + cursor), "<div id=\"day_3\"");
		// 53 = os_strlen("<div id=\"day_3\" class=\"wtline\" style=\"display:none;\">")
		cursor += (next + 53);
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, basicWeather->dayAfterTomorrowWeatherDesc);
		// 去除中间的空格
		for(i = 0; i < next; i++) {
			// 找到第一个空格
			if(basicWeather->dayAfterTomorrowWeatherDesc[i] == 0x20) break;
		}
		// 把'\0'也向前移一位，(next - i)不需要再减1
		os_memmove((basicWeather->dayAfterTomorrowWeatherDesc + i),
				(basicWeather->dayAfterTomorrowWeatherDesc + i + 1), (next - i));

#ifdef REQUEST_DEBUG_ENABLE
		os_printf("end:itianqi8\n");
#endif
	}while(0);

	EventBusGetDefault()->post(eventCallbackId, nextRequestId);
}


