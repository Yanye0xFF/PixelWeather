/*
 * context.h
 * @brief
 * Created on: Jun 25, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_CONTROLLER_CONTEXT_H_
#define APP_INCLUDE_CONTROLLER_CONTEXT_H_

#include "c_types.h"

#include "model/basic_weather.h"
#include "model/calendar.h"
#include "model/forecast_weather.h"
#include "model/status_bar.h"

#define FORECAST_DAYS         6
#define ITIANQI_URL_LENGTH    50
#define SYSTIME_URL_LENGTH    65

#define HTML_DUMP_BUFFER_LENGTH    72

//#define REQUEST_DEBUG_ENABLE

extern const uint8_t *ItianqiUrl;
extern const uint8_t *SystimeUrl;

typedef struct _context_t {
	BasicWeather * (*getBasicWeather)(void);
	Calendar * (*getCalendar)(void);
	ForecastWeather * (*getForecastWeathers)(void);
	StatusBar *(*getStatusBar)(void);
	void (*dumpURL)(const uint8_t * encryptUrl, uint32_t urlLength, char *buffer);
} Context_t;

extern Context_t Context;

#endif /* APP_INCLUDE_CONTROLLER_CONTEXT_H_ */
