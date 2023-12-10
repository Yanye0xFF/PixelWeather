
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define ESP_PLATFORM        1

#define USE_DNS

#ifdef USE_DNS
#define ESP_DOMAIN      "iot.espressif.cn"
#endif

#define USE_US_TIMER

#define AP_CACHE           1

#if AP_CACHE
#define AP_CACHE_NUMBER    5
#endif


#endif

