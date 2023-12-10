/*
 * http_utils.h
 * @brief
 * Created on: Aug 29, 2020
 * Author: Yanye
 */

#ifndef APP_USER_HTTP_UTILS_C_
#define APP_USER_HTTP_UTILS_C_

#include "utils/strings.h"
#include "osapi.h"
#include "os_type.h"
#include "c_types.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "ip_addr.h"

// #define HTTP_DEBUG    (1)

#define HTTP_PORT    (80)
#define HTTP_OK             (200)
#define HTTP_FORBIDDEN      (403)
#define HTTP_NOT_FOUND      (404)
#define HTTP_HEADER_UA      (" HTTP/1.1\r\nUser-Agent: Esp8266\r\n")
#define HTTP_HEADER_HOST    ("Host: ")
#define HTTP_HEADER_END     ("\r\n\r\n")
#define HTTP_CHUNK_END      ("0\r\n\r\n")

// http接收缓冲区 6KB
#define HTTP_CONTENT_MAX        (6 * 1024)
// http响应超时时间 3秒
#define HTTP_CONNECT_TIMEOUT    3000

struct _http_utils;
typedef struct _http_utils    HttpUtils;

/**
 * @brief http收到数据回调
 * @param *data http协议数据域中的数据(Body)，不包含http协议头数据
 * @param length http协议数据域中的数据长度，单位字节
 * @param httpCode http返回码，正常200，重定向302，服务器异常404等等
 * */
typedef void (*onRecvCallback)(uint8_t *data, uint32_t length, uint32_t httpCode);

/**
 * @biref http请求后等待响应的超时回调
 * @note 超时时间设置 @see HTTP_CONNECT_TIMEOUT
 * */
typedef void (*TimeoutCallback)(void);

struct _http_utils {
	// private:
	char url[128];
	uint8_t *httpBuffer;
	uint32_t bufferSize;
	onRecvCallback recvCallback;
	TimeoutCallback timeoutCallback;
	// public:
	void (*setOnRecvCallback)(onRecvCallback callback);
	void (*setTimeoutCallback)(TimeoutCallback callback);
	// GET请求
	STATUS (*doGet)(char *url);
	// POST方式暂未实现
	// STATUS (*doPost)(HttpUtils *self, char *url, uint8_t *postData);
	void(*disconnect)(void);
	void(*delete)(void);
};

HttpUtils * ICACHE_FLASH_ATTR HttpGetInstance(void);

#endif
