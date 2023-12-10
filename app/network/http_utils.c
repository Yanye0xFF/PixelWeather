/*
 * http_utils.c
 * @brief http网络请求类，单例实现
 * @note 支持的HTML传输类型: 带ContentLength以及chunked模式；不支持gzip
 * Created on: Aug 29, 2020
 * Author: Yanye
 */
#include "network/http_utils.h"

static void ICACHE_FLASH_ATTR setOnRecvCallback(onRecvCallback callback);

static void ICACHE_FLASH_ATTR setTimeoutCallback(TimeoutCallback callback);

static STATUS ICACHE_FLASH_ATTR doGet(char *url);

static void ICACHE_FLASH_ATTR delete(void);

static void ICACHE_FLASH_ATTR dns_callback(const char *name, ip_addr_t *ipaddr, void *arg);

static void ICACHE_FLASH_ATTR http_connect_callback(void *arg);

static void ICACHE_FLASH_ATTR http_disconnect_callback(void *arg);

static void ICACHE_FLASH_ATTR disconnect(void);

static void ICACHE_FLASH_ATTR http_recv_callback(void *arg, char *pdata, unsigned short len);

static void ICACHE_FLASH_ATTR http_timeout_callback(void *timer_arg);

// 这里的类变量导致HttpUtils仅能工作在单例模式下
static struct espconn http_espconn;
static ip_addr_t host_ip;
static uint32_t http_code;
static BOOL hasContentLength;
static volatile BOOL isTimeout;
static uint32_t content_length;
static volatile uint32_t total;
static HttpUtils *httpUtils = NULL;
static os_timer_t timeoutTimer;

/**
 * @brief 实例化HttpUtils类, 用完需要使用成员方法disconnect断开连接
 * @note 单例实现模式，仅会初始化一次，bufferSize由第一次初始化时决定
 * @return NULL: 内存分配失败，可能堆空间不足； !NULL: 空间分配成功
 * */
HttpUtils * ICACHE_FLASH_ATTR HttpGetInstance(void) {
	if(httpUtils == NULL) {
		httpUtils = (HttpUtils *)os_malloc(sizeof(HttpUtils));
		if(httpUtils != NULL) {
			// 实际分配内存HTTP_CONTENT_MAX，允许使用(HTTP_CONTENT_MAX - 1)，留出'\0'位置
			httpUtils->bufferSize = (HTTP_CONTENT_MAX - 1);
			httpUtils->httpBuffer = (uint8_t *)os_malloc(sizeof(uint8_t) * HTTP_CONTENT_MAX);
			httpUtils->setOnRecvCallback = setOnRecvCallback;
			httpUtils->setTimeoutCallback = setTimeoutCallback;
			httpUtils->doGet = doGet;
			httpUtils->disconnect = disconnect;
			httpUtils->delete = delete;
			// 配置超时定时器
			os_timer_disarm(&timeoutTimer);
			os_timer_setfn(&timeoutTimer, http_timeout_callback, NULL);
		}
	}
	return httpUtils;
}

/**
 * @brief 设置HTTP数据接收回调
 * @param callback 回调接口函数
 * */
static void ICACHE_FLASH_ATTR setOnRecvCallback(onRecvCallback callback) {
	httpUtils->recvCallback = callback;
}

/**
 * @brief 设置http请求超时回调
 * @note 产生超时回调时可以保证不会触发recvCallback
 * @param callback 超时回调函数，请在该函数中实现超时处理
 * */
static void ICACHE_FLASH_ATTR setTimeoutCallback(TimeoutCallback callback) {
	httpUtils->timeoutCallback = callback;
}

/**
 * @brief 使用GET方式访问获取数据
 * @param *self HttpUtils实例
 * @param *url 网址
 * */
static STATUS ICACHE_FLASH_ATTR doGet(char *url) {
	uint32_t start, end;
	total = 0;
	http_code = 0;
	content_length = 0;
	isTimeout = FALSE;
	hasContentLength = FALSE;
	os_memset(&host_ip, 0x00, sizeof(ip_addr_t));
	os_memset(&http_espconn, 0x00, sizeof(struct espconn));

	//self->url = url;
	os_strcpy(httpUtils->url, (const char *)url);

	start = charAt(url, '/') + 2;
    end = charAt((url + start), '/');
    subString(url, start, (start + end), httpUtils->httpBuffer);
#ifdef HTTP_DEBUG
    os_printf("doGet:%s\n", httpUtils->httpBuffer);
#endif
    // 启动超时定时器
    os_timer_arm(&timeoutTimer, HTTP_CONNECT_TIMEOUT, FALSE);
    // 请求DNS服务器, 以域名换取对应的IP地址, DNS服务器定义在./app/include/user_config.h
    espconn_gethostbyname(&http_espconn, httpUtils->httpBuffer, &host_ip, dns_callback);
	return PENDING;
}

/**
 * @brief HttpUtils析构函数
 * @param none
 * */
static void ICACHE_FLASH_ATTR delete() {
	if(httpUtils == NULL) {
		return;
	}
#ifdef HTTP_DEBUG
	os_printf("delete\n");
#endif
	if(httpUtils->httpBuffer != NULL) {
		os_free(httpUtils->httpBuffer);
		httpUtils->httpBuffer = NULL;
	}
	os_free(httpUtils);
	httpUtils = NULL;
}

/**
 * @brief dns域名解析回调
 * @param *name 被解析的域名由espconn_gethostbyname传入
 * @param *ipaddr 该域名对应的IP，实际地址由espconn_gethostbyname的参数ip_addr_t *addr传入
 * @param *arg 实质上是struct espconn *，由espconn_gethostbyname的参数struct espconn *pespconn传入
 * */
static void ICACHE_FLASH_ATTR dns_callback(const char *name, ip_addr_t *ipaddr, void *arg) {
#ifdef HTTP_DEBUG
	Integer integer;
#endif
	struct ip_info info;
    //struct espconn *pespconn = (struct espconn *)arg;
    wifi_get_ip_info(STATION_IF, &info);
#ifdef HTTP_DEBUG
    os_printf("dns_found_callback %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    integer.value = info.ip.addr;
    os_printf("local ip %d.%d.%d.%d\n", integer.bytes[0], integer.bytes[1],integer.bytes[2],integer.bytes[3]);
#endif

	http_espconn.type = ESPCONN_TCP;
	http_espconn.state = ESPCONN_NONE;
	http_espconn.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));

	os_memcpy(http_espconn.proto.tcp->local_ip, &(info.ip), sizeof(struct ip_addr));
	os_memcpy(http_espconn.proto.tcp->remote_ip, ipaddr, sizeof(struct ip_addr));

	http_espconn.proto.tcp->local_port = espconn_port();
	http_espconn.proto.tcp->remote_port = HTTP_PORT;

	espconn_regist_connectcb(&http_espconn, http_connect_callback);
	espconn_regist_disconcb(&http_espconn, http_disconnect_callback);
	espconn_regist_recvcb(&http_espconn, http_recv_callback);

	espconn_connect(&http_espconn);
}

static void ICACHE_FLASH_ATTR http_connect_callback(void *arg) {
#ifdef HTTP_DEBUG
	uint32_t i = 0;
	os_printf("espconn_connect_callback\n");
#endif
	uint32_t idx, start, cursor = 0;
	idx = os_strlen(httpUtils->url);
	start = charAt(httpUtils->url, ':') + 3;
	start = charAt((httpUtils->url + start), '/') + start;
	// GET方式声明，GET后面存在一个空格
	os_memcpy(httpUtils->httpBuffer, "GET ", 4);
	cursor += 4;
	// get随url附加的参数
	subString(httpUtils->url, start, idx, (httpUtils->httpBuffer + cursor));
	cursor += (idx - start);
	// http请求头, http1.1 User-Agent
	os_strcpy((char *)(httpUtils->httpBuffer + cursor), HTTP_HEADER_UA);
	cursor += strlen(HTTP_HEADER_UA);
	// host tag
	os_strcpy((char *)(httpUtils->httpBuffer + cursor), HTTP_HEADER_HOST);
	cursor += strlen(HTTP_HEADER_HOST);
	// 填充host为域名
	start = charAt(httpUtils->url, ':') + 3;
	idx = charAt((httpUtils->url + start), '/');
    subString(httpUtils->url, start, (start + idx), (httpUtils->httpBuffer + cursor));
	cursor += idx;
	// http头结束两次换行
	os_strcpy((char *)(httpUtils->httpBuffer + cursor), HTTP_HEADER_END);
	cursor += strlen(HTTP_HEADER_END);

#ifdef HTTP_DEBUG
	os_printf("httpBuffer length:%d\n", cursor);
	for(; i < (cursor); i++) {
		os_printf("%c", *(httpUtils->httpBuffer + i));
	}
	os_printf("\n");
#endif
	espconn_send(&http_espconn, httpUtils->httpBuffer, cursor);
}

static void ICACHE_FLASH_ATTR http_disconnect_callback(void *arg) {
#ifdef HTTP_DEBUG
	os_printf("espconn_disconnect_callback\n");
#endif
	if(http_espconn.proto.tcp != NULL) {
#ifdef HTTP_DEBUG
		os_printf("os_free http_espconn.proto.tcp\n");
#endif
		os_free(http_espconn.proto.tcp);
	}
	espconn_delete(&http_espconn);

	total = 0;
	http_code = 0;
	content_length = 0;
	isTimeout = FALSE;
	hasContentLength = FALSE;
	os_memset(&http_espconn, 0x00, sizeof(struct espconn));
	os_memset(&host_ip, 0x00, sizeof(ip_addr_t));
}

static void ICACHE_FLASH_ATTR disconnect() {
	espconn_disconnect(&http_espconn);
#ifdef HTTP_DEBUG
	os_printf("call disconnect\n");
#endif
	http_disconnect_callback(NULL);
}

/**
 * @brief http接收处理，支持Transfer-Encoding: chunked，但不支持gzip压缩
 * @brief httpUtils->callback只携带http body部分数据
 * */
static void ICACHE_FLASH_ATTR http_recv_callback(void *arg, char *pdata, unsigned short len) {
#ifdef HTTP_DEBUG
	os_printf("espconn_recv_callback\n");
	os_printf("recv length:%d\n", len);
#endif
	uint32_t start = 0, end = 0, copysize;

	if(startsWithStr(pdata, "HTTP/1.1")) {
		// 检测到http协议头
		start = charAt(pdata, ' ') + 1;
		http_code = atoi((pdata + start));
#ifdef HTTP_DEBUG
		os_printf("http_code:%d\n", http_code);
#endif
		// 跳过header部分数据
		start = 0;
		// 0D 0A = \r\n
		while(*(pdata + start) != 0x0D) {
			start += nextLine(pdata + start);
			if(startsWithStrIgnoreCase((pdata + start), "Content-Length:")) {
				hasContentLength = TRUE;
				// 16 = os_strlen("Content-Length: ");
				content_length = atoi((pdata + start + 16));
#ifdef HTTP_DEBUG
		os_printf("find content length:%d\n", content_length);
#endif
			}
		}
		// content有效数据起点
		start += 2;
	}

	if(http_code != HTTP_NOT_FOUND) {
		// 带ContentLength的直接读取
		if(hasContentLength) {
			end = len;
			copysize = ((total + (end - start)) > httpUtils->bufferSize) ? (httpUtils->bufferSize - total) : (end - start);
#ifdef HTTP_DEBUG
		os_printf("with content length copysize:%d\n", copysize);
#endif
			os_memcpy((httpUtils->httpBuffer + total), (pdata + start), copysize);
			total += copysize;
			// 最后一包或接收缓冲区满
			if((!isTimeout) && ((total >= content_length) || (total >= httpUtils->bufferSize))) {
				// 关闭超时定时器
				os_timer_disarm(&timeoutTimer);
				// 在末尾补结束符
				httpUtils->httpBuffer[total] = '\0';
				httpUtils->recvCallback(httpUtils->httpBuffer, total, http_code);
			}
		}else {
			// chunk模式 以 "0\r\n\r\n"表示最后一包
			// { 0x30, 0x0D, 0x0A, 0x0D, 0x0A }
			end = compareWith((pdata + len - 5), HTTP_CHUNK_END) ? (len - 5) : len;
			// 限制数据长度不超出buffersize
			copysize = ((total + (end - start)) > httpUtils->bufferSize) ? (httpUtils->bufferSize - total) : (end - start);
			os_memcpy((httpUtils->httpBuffer + total), (pdata + start), copysize);
			total += copysize;
			// 最后一包或接收缓冲区满
			if((!isTimeout) && ((end != len) || (total >= httpUtils->bufferSize))) {
				os_timer_disarm(&timeoutTimer);
				httpUtils->httpBuffer[total] = '\0';
				httpUtils->recvCallback(httpUtils->httpBuffer, total, http_code);
			}
		}
	}
}

/**
 * @brief http响应超时回调，用户代码需要再此回调中正确关闭http连接(调用disconnect方法)
 * @note 执行了timeoutCallback可以保证recvCallback不被执行
 * */
static void ICACHE_FLASH_ATTR http_timeout_callback(void *timer_arg) {
	isTimeout = TRUE;
	os_timer_disarm(&timeoutTimer);
	httpUtils->timeoutCallback();
}

