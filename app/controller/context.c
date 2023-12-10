/*
 * context.c
 * @brief 控制器共享数据上下文
 * Created on: Jun 25, 2021
 * Author: Yanye
 */

#include "controller/context.h"
// plaintext
// static const char *itianqiURL = "http://i.tianqi.com/index.php?c=code&a=getcode&id=";
// static const char *systimeURL = "http://quan.suning.com/getSysTime.do";
// static const char *systimeURL = "http://api.m.taobao.com/rest/api3.do?api=mtop.common.getTimestamp";

static const uint8_t XorTable[8] = {0x5D, 0x1C, 0x5A, 0x6B, 0x15, 0xA4, 0xDE, 0x92};

const uint8_t *ItianqiUrl = "\x35\x68\x2E\x1B\x2F\x8B\xF1\xFB\x73\x68"
		"\x33\xA\x7B\xD5\xB7\xBC\x3E\x73\x37\x44\x7C\xCA\xBA\xF7\x25\x32"
		"\x2A\x3\x65\x9B\xBD\xAF\x3E\x73\x3E\xE\x33\xC5\xE3\xF5\x38\x68"
		"\x39\x4\x71\xC1\xF8\xFB\x39\x21";

const uint8_t *SystimeUrl = "\x35\x68\x2E\x1B\x2F\x8B\xF1\xF3\x2D\x75\x74\x6"
		"\x3B\xD0\xBF\xFD\x3F\x7D\x35\x45\x76\xCB\xB3\xBD\x2F\x79\x29\x1F\x3A"
		"\xC5\xAE\xFB\x6E\x32\x3E\x4\x2A\xC5\xAE\xFB\x60\x71\x2E\x4\x65\x8A\xBD"
		"\xFD\x30\x71\x35\x5\x3B\xC3\xBB\xE6\x9\x75\x37\xE\x66\xD0\xBF\xFF\x2D";

static BasicWeather basicWeather;

static Calendar calendar;

static ForecastWeather forecastWeather[FORECAST_DAYS];

static StatusBar statusBar;

static BasicWeather * ICACHE_FLASH_ATTR getBasicWeather(void);

static Calendar * ICACHE_FLASH_ATTR getCalendar(void);

static ForecastWeather * ICACHE_FLASH_ATTR getForecastWeathers(void);

static StatusBar * ICACHE_FLASH_ATTR getStatusBar(void);

static void ICACHE_FLASH_ATTR dumpURL(const uint8_t * encryptUrl, uint32_t urlLength, char *buffer);

Context_t Context = {
	.getBasicWeather = getBasicWeather,
	.getCalendar = getCalendar,
	.getForecastWeathers = getForecastWeathers,
	.getStatusBar = getStatusBar,
	.dumpURL = dumpURL
};

static BasicWeather * ICACHE_FLASH_ATTR getBasicWeather(void) {
	return &basicWeather;
}

static Calendar * ICACHE_FLASH_ATTR getCalendar(void) {
	return &calendar;
}

static ForecastWeather * ICACHE_FLASH_ATTR getForecastWeathers(void) {
	return forecastWeather;
}

static StatusBar * ICACHE_FLASH_ATTR getStatusBar(void) {
	return &statusBar;
}

static void ICACHE_FLASH_ATTR dumpURL(const uint8_t * encryptUrl, uint32_t urlLength, char *buffer) {
    uint32_t i = 0;
    for(; i < urlLength; i++) {
        buffer[i] = ((*(encryptUrl + i)) ^ XorTable[i % 8]);
    }
}
