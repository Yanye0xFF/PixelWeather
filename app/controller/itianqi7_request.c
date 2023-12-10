/*
 * itianqi7_request.c
 * @brief 请求itianqi id=7 获取当前天气状态， 高/低温， 风向， 公历/农历描述
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#include "controller/itianqi7_request.h"

static uint32_t eventCallbackId, nextRequestId;

static void ICACHE_FLASH_ATTR itianqi7RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode);

/**
 * @param callbackId 执行回调的事件id
 * @param nextId 发给被执行回调函数的参数，为下个请求的id
 * */
void ICACHE_FLASH_ATTR requestItianqi7(uint32_t callbackId, uint32_t nextId) {
	SystemConfig *config;
	uint8_t urlBuffer[96];
	HttpUtils *http;

	os_memset(urlBuffer, 0x00, sizeof(urlBuffer));
	http = HttpGetInstance();
	config = sys_config_get();
	eventCallbackId = callbackId;
	nextRequestId = nextId;

	// "http://i.tianqi.com/index.php?c=code&a=getcode&id=7&py=changzhou"
	Context.dumpURL(ItianqiUrl, ITIANQI_URL_LENGTH, urlBuffer);
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH), "7&py=");
	os_strcpy((urlBuffer + ITIANQI_URL_LENGTH + 5), (const char *)config->cityName);
	http->setOnRecvCallback((onRecvCallback)itianqi7RecvCallback);
	http->doGet(urlBuffer);
	// leave this function immediately
}

static void ICACHE_FLASH_ATTR itianqi7RecvCallback(uint8_t *htmlBody, uint32_t length, uint32_t httpCode) {
	BasicWeather *basicWeather;
	Calendar *calendar;
	// 2139跳过了大部分的header meta style
	uint32_t cursor = 1689;
	int32_t next = 0, i;
	uint16_t xpos;

	HttpGetInstance()->disconnect();
	basicWeather = Context.getBasicWeather();
	calendar = Context.getCalendar();
	// clean
	basicWeather->weatherIcon = -1;
	basicWeather->weatherDesc[0] = '\0';
	basicWeather->windDesc[0] = '\0';
	basicWeather->tempLowest = 0;
	basicWeather->tempHighest = 0;
	calendar->calendarDesc[0] = '\0';
	calendar->lunarDesc[0] = '\0';

	do {
		// 天气数据样例
		// <span class="wtwt3">常州天气 </span>中雨到大雨            <span class="wttemp" style="color:#f00">20℃</span>～<span class="wttemp" style="color:#4899be">28℃</span>
		while((next = startWithEx((htmlBody + cursor), "<span class=\"wtwt3\">")) == -1) {
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

		if((next = contains((htmlBody + cursor), "</span>")) == -1) {
			break;
		}
		// 此处获取天气描述信息, 7 = os_strlen("</span>")
		cursor += (next + 7);
		next = charAt((htmlBody + cursor), ' ');
		subString((htmlBody + cursor), 0, next, basicWeather->weatherDesc);

		// <span class="wttemp" style="color:#f00">20℃</span>～<span class="wttemp" style="color:#4899be">28℃</span>
		// 此处获取今日最高/最低温度信息
		// 最低温度
		cursor += next;
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		basicWeather->tempLowest = atoi(htmlBody + cursor);

		next = charAt((htmlBody + cursor), '<');

		// 最高温度
		cursor += next;
		for(i = 0; i < 2; i++) {
			next = charAtSkip((htmlBody + cursor), '>');
			cursor += next;
		}
		basicWeather->tempHighest = atoi(htmlBody + cursor);

		// 此处获取风向/日期/农历
		// 数据样例  <div class="wtwind">东北风 2级<br>2020年11月24日  星期二 <br>农历庚子鼠年 十月初十 </div>
		do{
			next = nextLine(htmlBody + cursor);
			if(next == -1) {
				break;
			}
			cursor += next;
		}while((next = startWithEx((htmlBody + cursor), "<div class=\"wtwind\">")) == -1);
		if(next == -1) {
			break;
		}
		cursor += next;

		// 风向描述信息, 20 = os_strlen("<div class=\"wtwind\">")
		cursor += 20;
		next = charAt((htmlBody + cursor), '<');
		subString((htmlBody + cursor), 0, next, basicWeather->windDesc);

		cursor += next;

		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAt((htmlBody + cursor), '<');
		// <div class="wtwind">西北风<br>2021年04月16日  星期五 <br>农历辛丑牛年 三月初五</div>
		// 周末会变
		// <div class="wtwind">北风 2级<br>2020年11月29日  <font color='green'>星期日</font> <br>农历庚子鼠年 十月十五</div>
		if(*(htmlBody + cursor + next + 1) == 'f') {
			// 只留一个空格
			i = (next - 1);
			subString((htmlBody + cursor), 0, i, calendar->calendarDesc);

			cursor += next;
			next = charAtSkip((htmlBody + cursor), '>');
			cursor += next;
			next = charAt((htmlBody + cursor), '<');
			subString((htmlBody + cursor), 0, next, (calendar->calendarDesc + i));

			// 跳过 </font>
			cursor += next;
			next = charAtSkip((htmlBody + cursor), '>');

		}else {
			// (next - 1) 去除星期后面的空格
			next--;
			subString((htmlBody + cursor), 0, next, calendar->calendarDesc);
			for(i = 0; i < next; i++) {
				// 找到第一个空格
				if(calendar->calendarDesc[i] == 0x20) break;
			}
			// 把'\0'也向前移一位，(next - i)不需要再减1
			os_memmove((calendar->calendarDesc + i), (calendar->calendarDesc + i + 1), (next - i));

		}
		cursor += next;

		// 农历信息
		next = charAtSkip((htmlBody + cursor), '>');
		cursor += next;
		next = charAt((htmlBody + cursor), '<');

		subString((htmlBody + cursor), 0, next, calendar->lunarDesc);
		trim(calendar->lunarDesc);
#ifdef REQUEST_DEBUG_ENABLE
		os_printf("end:itianqi7\n");
#endif
	}while(0);

	EventBusGetDefault()->post(eventCallbackId, nextRequestId);
}
