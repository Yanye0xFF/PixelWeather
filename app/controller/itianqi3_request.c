/*
 * itianqi3_request.c
 * @brief 请求itianqi id=3 获取今天+后4天的天气描述，风向描述，最高/最低温度
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#include "controller/itianqi3_request.h"

static uint32_t eventCallbackId, nextRequestId;

static void ICACHE_FLASH_ATTR itianqi3RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode);

void ICACHE_FLASH_ATTR requestItianqi3(uint32_t callbackId, uint32_t nextId) {
	SystemConfig *config;
	uint8_t urlBuffer[96];
	HttpUtils *http;

	os_memset(urlBuffer, 0x00, sizeof(urlBuffer));
	http = HttpGetInstance();
	config = sys_config_get();
	eventCallbackId = callbackId;
	nextRequestId = nextId;

	// http://i.tianqi.com/index.php?c=code&a=getcode&id=3&py=changzhou
	Context.dumpURL(ItianqiUrl, ITIANQI_URL_LENGTH, urlBuffer);
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH), "3&py=");
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH + 7), (const char *)config->cityName);
	http->setOnRecvCallback((onRecvCallback)itianqi3RecvCallback);
	http->doGet(urlBuffer);
	// leave this function immediately
}


static void ICACHE_FLASH_ATTR itianqi3RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode) {
	const uint32_t maxDays = 6;
	uint32_t count = 0, cursor = 1600;
	int32_t next, i;
	int32_t start, end;
	uint8_t dump[16];
	BOOL needCut;

	ForecastWeather *forecastPtr;
	ForecastWeather *forecastWeather;

	HttpGetInstance()->disconnect();
	forecastPtr = Context.getForecastWeathers();

	do {
		forecastWeather = (forecastPtr + count);
		// <div class="wtbg">
		while((next = startWithEx((htmlBody + cursor), "<div class=\"wtbg\">")) == -1) {
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

		// 获取日期描述， <div class="wtbg">后天天气</div>
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		needCut = TRUE;
		if(*(htmlBody + cursor) == '<') {
			// 星期天会变化，<div class="wtbg"><font color='green'>星期日</font>天气</div>
			next = charAtSkip((htmlBody + cursor), '>');
			cursor += next;
			needCut = FALSE;
		}
		next = charAt((htmlBody + cursor), '<');
		i = (needCut) ? (next - 6) : next;
		subString((htmlBody + cursor), 0, i, forecastWeather->dayTag);
		cursor += next;

		// 天气描述信息 <div class="wtwt">小雨到中雨</div>
		next = nextLine(htmlBody + cursor);
		cursor += next;
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, forecastWeather->weatherDesc);
		cursor += next;

		// 匹配天气图标
		forecastWeather->weatherIcon = matchWeatherId(forecastWeather->weatherDesc);

		// 风向描述信息 删除中间的空格 <div class="wtwind">西南风 3级</div>
		next = nextLine(htmlBody + cursor);
		cursor += next;
		start = charAt((htmlBody + cursor), '>');
		cursor += start;
		end = charAt((htmlBody + cursor), '<');

		forecastWeather->windDesc[0] = '\0';
		if(end > 1) {
			subString((htmlBody + start), 1, end, forecastWeather->windDesc);
			// 剔除中间的空格
			for(i = 0; i < end; i++) {
				// ' '空格符
				if(forecastWeather->windDesc[i] == 0x20) break;
			}
			os_memmove((forecastWeather->windDesc + i), (forecastWeather->windDesc + i + 1), end - i);
		}
		cursor += end;

		// 最高/最低温度 <div class="wttemp"><font color="#f00">30℃</font>～<font color="#4899be">36℃</font></div>
		next = nextLine(htmlBody + cursor);
		cursor += next;
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, dump);
		forecastWeather->tempLowest = atoi((const char *)dump);
		cursor += next;

		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, dump);
		forecastWeather->tempHighest = atoi((const char *)dump);
		cursor += next;

		count++;

	}while(count < maxDays);

#ifdef REQUEST_DEBUG_ENABLE
		os_printf("end:itianqi3\n");
#endif

	EventBusGetDefault()->post(eventCallbackId, nextRequestId);
}
