#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "espconn.h"
#include "eagle_soc.h"
#include "mem.h"
#include "gpio.h"
#include "c_types.h"
#include "spi_flash.h"
#include "ets_sys.h"

#include "driver/gpio16.h"
#include "driver/uart.h"
#include "driver/gpio16.h"
#include "driver/ssd1675b.h"
#include "driver/dht11.h"
#include "driver/user_button.h"

#include "graphics/displayio.h"
#include "graphics/font.h"

#include "spifsmini/spifs.h"

#include "network/udp_handler.h"
#include "network/http_utils.h"

#include "utils/jsonobj.h"
#include "utils/strings.h"
#include "utils/sysconf.h"
#include "utils/rtc_mem.h"
#include "utils/eventdef.h"
#include "utils/eventbus.h"
#include "utils/hardware.h"
#include "utils/misc.h"
#include "utils/fixed_file.h"

#include "model/basic_weather.h"
#include "model/forecast_weather.h"
#include "model/status_bar.h"
#include "model/calendar.h"
#include "model/date.h"

#include "view/appimage.h"
#include "view/appicon.h"
#include "view/basic_layout.h"
#include "view/nointernet_layout.h"
#include "view/forecast_layout.h"
#include "view/note_layout.h"
#include "view/gallery_layout.h"

#include "controller/basic_controller.h"
#include "controller/context.h"

#define EVENT_NONE            0
#define EVENT_UPDATE_EPD      100
#define EVENT_DOUBLE_CLICK    101
#define EVENT_LONG_CLICK      102

#define WIFI_RECONNECT_MAX    2
#define	FPM_SLEEP_MAX_TIME    0xFFFFFFF

// 空闲态超时时间(分钟)
#define IDLE_TIMEOUT_MAX      10

enum {
	DisplayNone = 0,
	DisplayBasic,
	DisplayForecast,
	DisplayNote,
	DisplayGallery,
};

#define EAGLE_FLASH_BIN_ADDR				      (SYSTEM_PARTITION_CUSTOMER_BEGIN + 1)
#define EAGLE_IROM0TEXT_BIN_ADDR			      (SYSTEM_PARTITION_CUSTOMER_BEGIN + 2)
#define SYSTEM_PARTITION_RF_CAL_ADDR              (0x3FB000)
#define SYSTEM_PARTITION_PHY_DATA_ADDR            (0x3FC000)
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR    (0x3FD000)

// 授权验证插桩点
// 系统启动SHA256+MD5, 配置文件整体更新MD5, 显示自定义图片MD5

static const partition_item_t partition_table[] = {
    {EAGLE_FLASH_BIN_ADDR, 	0x00000, 0x10000},
    {EAGLE_IROM0TEXT_BIN_ADDR, 0x10000, 0xC0000},
    {SYSTEM_PARTITION_RF_CAL, SYSTEM_PARTITION_RF_CAL_ADDR, 0x1000},
    {SYSTEM_PARTITION_PHY_DATA, SYSTEM_PARTITION_PHY_DATA_ADDR, 0x1000},
	{SYSTEM_PARTITION_SYSTEM_PARAMETER, SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 0x3000}
};

static void ICACHE_FLASH_ATTR softap_info_setup(const char *ssid, const char *pwd);
static void ICACHE_FLASH_ATTR station_info_setup(char *ssid, char *pwd);
static void ICACHE_FLASH_ATTR wifi_event_callback(System_Event_t *evt);

static os_timer_t postDelayTimer;
static uint32_t delayEventId = EVENT_NONE;
static void ICACHE_FLASH_ATTR postDelayTimerCallback(void *timer_arg);
static void ICACHE_FLASH_ATTR postEventDelay(uint32_t eventId, uint32_t miliseconds);

static os_timer_t clockTimer;
static void ICACHE_FLASH_ATTR clockTimerCallback(void *timer_arg);

static os_timer_t idleTimer;
static void ICACHE_FLASH_ATTR idleTimerCallback(void *timer_arg);

void ICACHE_FLASH_ATTR lightSleepWakeupCallback(void);

static void ICACHE_FLASH_ATTR requestInternetUpdate(void);
static void ICACHE_FLASH_ATTR invalidateView(void);

static struct espconn udp_espconn;
static void ICACHE_FLASH_ATTR udp_server_setup(uint8 if_index);
static void ICACHE_FLASH_ATTR udp_recv_callback(void *arg, char *pdata, unsigned short len);

static void ICACHE_FLASH_ATTR wifi_mode_setup(SystemConfig *cfg);

static void ICACHE_FLASH_ATTR requestFinishedHandler(uint32_t eventId, uint32_t arg);
static void ICACHE_FLASH_ATTR epdFinishedHandler(uint32_t eventId, uint32_t arg);
static void ICACHE_FLASH_ATTR displayImageHandler(uint32_t eventId, uint32_t arg);
static void ICACHE_FLASH_ATTR closeTimerHandler(uint32_t eventId, uint32_t arg);
static void ICACHE_FLASH_ATTR powerToggleHandler(uint32_t eventId, uint32_t arg);

static void ICACHE_FLASH_ATTR httpTimeoutHandler(void);

static void ICACHE_FLASH_ATTR enterLightSleep(void);

static void ICACHE_FLASH_ATTR batteryLowShutdown(void);

static void ICACHE_FLASH_ATTR userClickListener(void);

// do not add 'ICACHE_FLASH_ATTR'
static void userDoubleClickListener(void);
static void userLongClickListener(void);
// do not add 'ICACHE_FLASH_ATTR'

#define VIEW_PAGE_MAX 4
static uint8_t ViewPages[VIEW_PAGE_MAX];

void ICACHE_FLASH_ATTR user_pre_init(void) {
	// 4 == SPI_FLASH_SIZE_MAP already definded in root makefile
    if(!system_partition_table_regist(partition_table, sizeof(partition_table) / sizeof(partition_table[0]), 4)) {
		while(1);
	}
}

/**
 * @brief 正常运行/modem_sleep时钟回调, 每60秒进入一次
 * @param *timer_arg 未使用
 * */
static void ICACHE_FLASH_ATTR clockTimerCallback(void *timer_arg) {
	Date date;
	UpdateInfo info;
	PowerMode powermode;
	AutoSleep autoslp;
	uint32_t temp;

	temp = hw_get_battery_level();
	if(temp < 20) {
		// 电量小于20%, 显示电量低图片并关机
		batteryLowShutdown();
		return;
	}
	// 加载rtc memory中运行时参数
	system_rtc_mem_read(DATE_POS, (void *)&date, sizeof(Date));
	system_rtc_mem_read(UPDATEINFO_POS, (void *)&info, sizeof(UpdateInfo));
	system_rtc_mem_read(AUTO_SLEEP_POS, (void *)&autoslp, sizeof(AutoSleep));
	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));

	updateTokenTimeout();
	updateClockByTick(&info, &date, 60);

	system_rtc_mem_write(DATE_POS, (const void *)&date, sizeof(Date));
	system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));

	// wifi交替打开不受自动休眠限制
	// POWER_MODEM_SLEEP
	if(powermode.type == POWER_MODEM_SLEEP && powermode.wifiState == POWER_STATE_WIFI_ON && (info.weatherTick < info.weatherCompare)) {
		powermode.wifiState = POWER_STATE_WIFI_OFF;
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		wifi_station_disconnect();

	}else if(powermode.type == POWER_MODEM_SLEEP && powermode.wifiState == POWER_STATE_WIFI_OFF) {
		powermode.wifiState = POWER_STATE_WIFI_ON;
		powermode.refreshAfterConnect = FALSE;
		if(info.weatherTick >= info.weatherCompare) {
			powermode.refreshAfterConnect = TRUE;
			info.weatherTick = 0; info.timeTick = 0;
		}
		system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));

		wifi_fpm_do_wakeup();
		wifi_fpm_close();
		wifi_fpm_set_sleep_type(NONE_SLEEP_T);
		wifi_set_opmode(STATION_MODE);
		wifi_station_connect();
	}

	// temp复用做CLOCKARM_POS标记
	temp = 0x0;
	// 仅正常模式受自动休眠限制, 由于休眠模式复位才能唤醒，autoslp.tick无需清除
	if(powermode.type == POWER_NONE_SLEEP && autoslp.valid) {
		autoslp.tick++;
		if(autoslp.tick >= autoslp.compare) {
			// 切换到light_sleep模式
			powermode.type = POWER_LIGHT_SLEEP;
			system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		}
		system_rtc_mem_write(AUTO_SLEEP_POS, (const void *)&autoslp, sizeof(AutoSleep));
	}

	// POWER_NONE_SLEEP
	// 天气和时间更新同时满足时，只需要更新天气(天气会联网请求，并同步更新时间)
	if(info.weatherTick >= info.weatherCompare) {
		info.weatherTick = 0;
		info.timeTick = 0;
		// 关闭clockTimer，requestFinishedHandler回调中会再次启用
		os_timer_disarm(&clockTimer);
		system_rtc_mem_write(CLOCKARM_POS, (const void *)&temp, sizeof(uint32_t));
		system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));
		requestInternetUpdate();

	}else if(info.timeTick >= info.timeCompare) {
		info.timeTick = 0;
		// 回写数据
		system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));
		// 刷新显示
		invalidateView();
		postEventDelay(EVENT_UPDATE_EPD, 100);

	}else {
		// POWER_LIGHT_SLEEP优先级最低，前面的case都有可能更新display
		if((powermode.type == POWER_LIGHT_SLEEP) && (wifi_get_opmode() == STATION_MODE)) {
			os_timer_disarm(&clockTimer);
			system_rtc_mem_write(CLOCKARM_POS, (const void *)&temp, sizeof(uint32_t));
			wifi_station_disconnect();
		}
	}
}

/**
 * @brief light_sleep唤醒回调，执行间隔由时间刷新周期决定
 * */
void ICACHE_FLASH_ATTR lightSleepWakeupCallback(void) {
	Date date;
	UpdateInfo info;
	PowerMode powermode;
	NightSpan nightSpan;
	Font *eng16px;
	uint32_t seconds, level;

	wifi_fpm_close();

	// 电量小于20%, 显示电量低图片并关机
	level = hw_get_battery_level();
	if(level < 20) {
		batteryLowShutdown();
		return;
	}

	system_rtc_mem_read(DATE_POS, (void *)&date, sizeof(Date));
	system_rtc_mem_read(UPDATEINFO_POS, (void *)&info, sizeof(UpdateInfo));
	system_rtc_mem_read(NIGHT_SPAN_POS, (void *)&nightSpan, sizeof(NightSpan));
	system_rtc_mem_read(WAKEUP_INTERVAL_POS, (void *)&seconds, sizeof(uint32_t));
	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));
	// 每次唤醒自增计数器
	updateClockByTick(&info, &date, seconds);

	system_rtc_mem_write(DATE_POS, (const void *)&date, sizeof(Date));
	system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));

	// 夜间不更新区间检查，检查当前小时是否在不更新区间内, 区间[nightSpan.start, nightSpan.end)
	if(nightSpan.valid && (date.hour >= nightSpan.start) && (date.hour < nightSpan.end)) {
		if(nightSpan.isEntered) {
			enterLightSleep();
		}else {
			nightSpan.isEntered = 1;
			system_rtc_mem_write(NIGHT_SPAN_POS, (const void *)&nightSpan, sizeof(NightSpan));
			// 绘制背景图
			GuiDrawBmpImage("stopmode", "bmp", 0, 0);
			// 图片上绘制时间信息
			eng16px = getFont(FONT08x16_EN);
			// 开始时间
			GuiDrawChar(('0' + nightSpan.start / 10), 120, 57, BLACK, WHITE, eng16px, 16);
			GuiDrawChar(('0' + nightSpan.start % 10), 129, 57, BLACK, WHITE, eng16px, 16);
			// 结束时间
			GuiDrawChar(('0' + nightSpan.end / 10), 162, 57, BLACK, WHITE, eng16px, 16);
			GuiDrawChar(('0' + nightSpan.end % 10), 171, 57, BLACK, WHITE, eng16px, 16);
			postEventDelay(EVENT_UPDATE_EPD, 100);
		}
		return;
	}

	nightSpan.isEntered = 0;
	system_rtc_mem_write(NIGHT_SPAN_POS, (const void *)&nightSpan, sizeof(NightSpan));

	// 天气和时间更新同时满足时，只需要更新天气(天气会联网请求，并同步更新时间)
	if(info.weatherTick >= info.weatherCompare) {
		info.timeTick = 0;
		info.weatherTick = 0;
		powermode.refreshAfterConnect = TRUE;
		// 回写数据
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));
		// 连接wifi
		wifi_set_opmode(STATION_MODE);
		wifi_station_connect();

	}else if(info.timeTick >= info.timeCompare) {
		info.timeTick = 0;
		// 回写数据
		system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));
		// 刷新显示
		invalidateView();
		postEventDelay(EVENT_UPDATE_EPD, 100);

	}else {
		enterLightSleep();
	}
}

void ICACHE_FLASH_ATTR user_init(void) {
    SystemConfig *config = NULL;
	uint32_t i, initial = 0x00000000, temp = 0;
	uint32_t rand[8];

	fixed_file_t fixedFile;
	uint8_t pages[FILE_PAGE_SECTION_SIZE];

	uart_init(BIT_RATE_74880, BIT_RATE_74880);
	system_update_cpu_freq(SYS_CPU_80MHZ);
	wifi_set_opmode(NULL_MODE);

	// runtime init
	sys_runtime_init();
	sys_config_init();
	// 文件系统ftl
	spifs_ftl_init();

	// 初始化EPD
	EPDDisplayRAMInit();
	// 用户按钮
	UserButtonInit();
	// 温湿度传感器
	DHT11GpioSetup();

	// 显示刷新定时器
	os_timer_disarm(&postDelayTimer);
	os_timer_setfn(&postDelayTimer, postDelayTimerCallback, NULL);
	// 时间更新定时器
	os_timer_disarm(&clockTimer);
	os_timer_setfn(&clockTimer, &clockTimerCallback, NULL);
	// 系统空闲定时器，未配置AP/WiFi掉线AP，1分钟检测电量
	os_timer_disarm(&idleTimer);
	os_timer_setfn(&idleTimer, &idleTimerCallback, NULL);

	// rtc memory clean
	for(i = RTCMEM_START; i < (RTCMEM_END + 1); i++) {
		system_rtc_mem_write(i, (const void *)&initial, sizeof(uint32_t));
	}

	fixed_file_init(&fixedFile, FILE_PAGE_SECTION_SIZE, FILE_PAGE_MAX_SIZE);
	os_memset(ViewPages, 0x00, sizeof(ViewPages));

	if(fixed_file_open(&fixedFile, "page", "ini")) {
		// 读取文件需要刷新的页面，放入顺序表
		if(fixed_file_read(&fixedFile, pages, FILE_PAGE_SECTION_SIZE) == FILE_PAGE_SECTION_SIZE) {
			// 默认取最后一项记录，忽略配置头
			for(i = 1; i < FILE_PAGE_SECTION_SIZE; i++) {
				if(pages[i] == 0xFF) break;
				ViewPages[temp++] = pages[i];
			}
		}
	}else {
		ViewPages[0] = DisplayBasic;
		ViewPages[1] = DisplayForecast;
	}

	config = sys_config_get();
	if(config != NULL && (config->status.isConfiged + config->status.hasResource) == STATUS_VALID) {
		loadFont();
	}

	EventBusGetDefault()->regist(displayImageHandler, MAIN_EVENT_DISPLAY_IMAGE);
	EventBusGetDefault()->regist(requestFinishedHandler, MAIN_EVENT_REQUEST_FINISH);
	EventBusGetDefault()->regist(epdFinishedHandler, MAIN_EVENT_EPD_FINISH);
	EventBusGetDefault()->regist(closeTimerHandler, MAIN_EVENT_CLOSE_TIMER);
	EventBusGetDefault()->regist(powerToggleHandler, MAIN_EVENT_POWERTOGGLE);
	// 控制器初始化也仅是注册事件回调
	basicControllerInit();
	HttpGetInstance()->setTimeoutCallback(httpTimeoutHandler);

	// setup irq handler
	UserButtonIRQInit();
	UserButtonAddClickListener(userClickListener);
	UserButtonAddDoubleClickListener(userDoubleClickListener);
	UserButtonAddLongClickListener(userLongClickListener);

	temp = hw_get_battery_level();
	if(temp > 10) {
		//电量 >= 20% 正常启动
		wifi_set_event_handler_cb(wifi_event_callback);
		wifi_mode_setup(config);
	}else {
		// 电量小于20%, 显示电量低图片并关机
		batteryLowShutdown();
	}
}

/**
 * @brief 配置wifi模式
 * @param cfg 系统配置文件
 * */
static void ICACHE_FLASH_ATTR wifi_mode_setup(SystemConfig *cfg) {
	uint32_t tempVar;
	PowerMode powermode;

	if(cfg == NULL) {
		wifi_set_opmode(SOFTAP_MODE);
		softap_info_setup(DEFAULT_SOFTAP_SSID, DEFAULT_SOFTAP_PASS);
		// 工程模式top
		GuiDrawImagePacked(factory_image, 0, 0);

		postEventDelay(EVENT_UPDATE_EPD, 100);
		os_timer_arm(&idleTimer, 60000, TRUE);
		return;
	}

	tempVar = UserButtonLevelRead();
	if((tempVar == GPIO_PIN_HIGH) && ((cfg->status.isConfiged + cfg->status.hasResource) == STATUS_VALID)) {
		// 正常启动模式
		tempVar = POWER_BY_RESET_SET;
		system_rtc_mem_write(POWER_ON_REASON_POS, (const void *)&tempVar, sizeof(uint32_t));
		// 显示开机图片
		GuiDrawBmpImage("connect", "bmp", 0, 0);
		postEventDelay(EVENT_UPDATE_EPD, 100);
		// 在epdFinishedHandler中连接wifi

	}else {
		// 用户按钮按下+复位键开机 = 配置模式
		wifi_set_opmode(SOFTAP_MODE);
		softap_info_setup(DEFAULT_SOFTAP_SSID, DEFAULT_SOFTAP_PASS);

		if(cfg->status.hasResource == STATUS_VALID) {
			GuiDrawBmpImage("splash", "bmp", 0, 0);
		}else {
			// 未配置,且不含资源文件
			GuiDrawImagePacked(factory_image, 0, 0);
		}
		postEventDelay(EVENT_UPDATE_EPD, 100);
		os_timer_arm(&idleTimer, 60000, TRUE);
	}
}

/**
 * @brief 单击事件，定时器回调，可以放入FALSH
 * @note 请求网络刷新
 * */
static void ICACHE_FLASH_ATTR userClickListener(void) {
	uint8_t opmode;
	PowerMode powermode;
	SystemRunTime *runtime;
	uint32_t time, current, elapse;

	opmode = wifi_get_opmode();
	runtime = sys_runtime_get();
	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));
	// 无网络即可滤掉SOFTAP_MODE, STATIONAP_MODE
	if(runtime->internet == 0) {
		return;
	}
	// 至少间隔60s
	system_rtc_mem_read(NETUPDATE_POS, (void *)&time, sizeof(uint32_t));
	current = system_get_time();
	elapse = (current < time) ? (0xFFFFFFFF - time + current) : (current - time);
	if(elapse < 60000000) {
		return;
	}

	if(opmode == NULL_MODE) {
		// 仅modem sleep & POWER_STATE_WIFI_OFF会进来, light sleep 会关闭gpio中断
		system_rtc_mem_write(NETUPDATE_POS, (const void *)&current, sizeof(uint32_t));
		closeTimerHandler(0, 0);
		powermode.refreshAfterConnect = TRUE;
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		// 退出低功耗(CPU此时还运行)重连wifi，联网更新一次时间/天气
		wifi_fpm_do_wakeup();
		wifi_fpm_close();
		wifi_fpm_set_sleep_type(NONE_SLEEP_T);
		wifi_set_opmode(STATION_MODE);
		wifi_station_connect();

	}else if(opmode == STATION_MODE) {
		// 仅正常模式，modem_sleep & POWER_STATE_WIFI_ON
		// 先行写入当前时间防止多按
		system_rtc_mem_write(NETUPDATE_POS, (const void *)&current, sizeof(uint32_t));
		closeTimerHandler(0, 0);
		// 正常模式 or (POWER_MODEM_SLEEP & POWER_STATE_WIFI_ON)直接刷新
		requestInternetUpdate();
	}
	// 其余SOFTAP_MODE, STATIONAP_MODE不做处理
}

/**
 * @brief 用户按钮双击事件，中断中回调，不能放入FLASH，需使用timer切到主线程
 * @note 进入深度休眠模式
 * */
static void userDoubleClickListener(void) {
	delayEventId = EVENT_DOUBLE_CLICK;
	os_timer_arm(&postDelayTimer, 100, FALSE);
}

/**
 * @brief 用户按钮长按事件，中断中回调，不能放入FLASH，需使用timer切到主线程
 * @note 系统关机
 * */
static void userLongClickListener(void) {
	delayEventId = EVENT_LONG_CLICK;
	os_timer_arm(&postDelayTimer, 100, FALSE);
}

/**
 * @brief station配置
 * @param ssid 被连接AP的SSID, 最长32字符
 * @param pwd 密码, 最长64字符
 * */
static void ICACHE_FLASH_ATTR station_info_setup(char *ssid, char *pwd) {
	struct station_config station_conf;
	os_memset(&station_conf, 0x00, sizeof(struct station_config));
	os_strcpy(station_conf.ssid, (const char *)ssid);
	os_strcpy(station_conf.password, (const char *)pwd);
	// 设置wifi_station的接口，并保存到flash
	wifi_station_set_config(&station_conf);
	wifi_station_disconnect();
	wifi_station_connect();
}

/**
 * @brief softap配置
 * @param ssid SSID, 最长32字符
 * @param pwd 密码, 最长64字符
 * */
static void ICACHE_FLASH_ATTR softap_info_setup(const char *ssid, const char *pwd) {
	struct softap_config softap_conf;
	os_memset(&softap_conf, 0x00, sizeof(struct softap_config));
	os_strcpy(softap_conf.ssid, ssid);
	os_strcpy(softap_conf.password, pwd);
	softap_conf.ssid_len = os_strlen(ssid);
	softap_conf.channel = 1;
	softap_conf.authmode = AUTH_WPA2_PSK;
	softap_conf.beacon_interval = 100;
	softap_conf.max_connection = 2;
	wifi_softap_set_config(&softap_conf);
}

/**
 * @brief 建立udp服务，监听8080端口
 * */
static void ICACHE_FLASH_ATTR udp_server_setup(uint8 if_index) {
	struct ip_info info;
	wifi_get_ip_info(if_index, &info);
	wifi_set_broadcast_if(STATIONAP_MODE);
	udp_espconn.type = ESPCONN_UDP;
	udp_espconn.state = ESPCONN_LISTEN;
	udp_espconn.proto.udp = (esp_udp *) os_zalloc(sizeof(esp_udp));
	os_memcpy(udp_espconn.proto.udp->local_ip, &(info.ip.addr), sizeof(uint32_t));
	udp_espconn.proto.udp->local_port = UDP_LISTEN_PORT;
	// 注册发送和接收的回调函数
	espconn_regist_recvcb(&udp_espconn, udp_recv_callback);
	espconn_create(&udp_espconn);
}

/**
 * @brief wifi连接状态改变回调
 * @param evt 回调事件
 * */
static void ICACHE_FLASH_ATTR wifi_event_callback(System_Event_t *evt) {
	uint32_t reconnect = 0x00000000;
	PowerMode powerInfo;

	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powerInfo, sizeof(PowerMode));

	switch(evt->event) {
    	case EVENT_STAMODE_CONNECTED:
    		system_rtc_mem_write(RECONNECT_POS, (const void *)&reconnect, sizeof(uint32_t));
    		break;
    	case EVENT_STAMODE_GOT_IP:
    		if(powerInfo.type == POWER_NONE_SLEEP) {
				handlerInit();
				udp_server_setup(STATION_IF);
				requestInternetUpdate();

    		}else if(powerInfo.refreshAfterConnect) {
    			// POWER_MODEM_SLEEP 和 POWER_LIGHT_SLEEP都可进入
    			powerInfo.refreshAfterConnect = FALSE;
    			system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powerInfo, sizeof(PowerMode));
				requestInternetUpdate();
    		}
			break;
    	case EVENT_STAMODE_DISCONNECTED:
			if(powerInfo.type == POWER_NONE_SLEEP) {
				// 未休眠状态下对重连事件计数，达到上限后开启AP模式
				system_rtc_mem_read(RECONNECT_POS, (void *)&reconnect, sizeof(uint32_t));
				if(reconnect < WIFI_RECONNECT_MAX) {
					reconnect += 1;
					system_rtc_mem_write(RECONNECT_POS, (const void *)&reconnect, sizeof(uint32_t));
				}else {
					handlerDelete();
					os_timer_disarm(&clockTimer);
					os_timer_arm(&idleTimer, 60000, TRUE);
					wifi_set_opmode(SOFTAP_MODE);
					softap_info_setup(DEFAULT_SOFTAP_SSID, DEFAULT_SOFTAP_PASS);
					GuiDrawBmpImage("nowifi", "bmp", 0, 0);
					postEventDelay(EVENT_UPDATE_EPD, 50);
				}

			}else if(powerInfo.type == POWER_MODEM_SLEEP) {
				// 射频部分关闭，CPU仍继续运行
				wifi_set_opmode(NULL_MODE);
				wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
				wifi_fpm_open();
				wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);

			}else if(powerInfo.type == POWER_LIGHT_SLEEP) {
				// 射频&CPU都关闭
				wifi_set_opmode(NULL_MODE);
				enterLightSleep();

			}else if(powerInfo.type == POWER_SHUTDOWN || powerInfo.type == POWER_BATLOW) {
				// 深度睡眠
				wifi_set_opmode(NULL_MODE);
				system_deep_sleep(0);

			}
    		break;
    	case EVENT_STAMODE_AUTHMODE_CHANGE:
    		break;
    	case EVENT_SOFTAPMODE_STACONNECTED:
    		system_rtc_mem_write(RECONNECT_POS, (const void *)&reconnect, sizeof(uint32_t));
    		handlerInit();
    		udp_server_setup(SOFTAP_IF);
    		break;
    	case EVENT_SOFTAPMODE_STADISCONNECTED:
    		handlerDelete();
    		break;
    	default:
			break;
	}
}

static void ICACHE_FLASH_ATTR requestInternetUpdate(void) {
	// 联网后统一从默认页面开始显示
	uint32_t pageFlag = 0x0;
	system_rtc_mem_write(CURRENT_PAGE_POS, (const void *)&pageFlag, sizeof(uint32_t));
	requestBasicWeather();
}

static void ICACHE_FLASH_ATTR invalidateView(void) {
	// in context
	TempHumEntity *temphum;
	Calendar *calendar;
	BasicWeather *weather;
	StatusBar *status;
	ForecastWeather *forecastWeather;
	// in rtc men
	uint32_t dpid, position;
	PowerMode powermode;

	status = Context.getStatusBar();
	calendar = Context.getCalendar();
	weather = Context.getBasicWeather();
	forecastWeather = Context.getForecastWeathers();
/*
 * data  : 0x3ffe8000 ~ 0x3ffe894c, len: 2380
rodata: 0x3ffe8950 ~ 0x3ffebf4c, len: 13820
bss   : 0x3ffebf50 ~ 0x3fff28f0, len: 27040
heap  : 0x3fff28f0 ~ 0x3fffc000, len: 38672
free_heap_size:21976
 *
 * */

	//system_print_meminfo();
	//dpid =  system_get_free_heap_size();
	//os_printf("free_heap_size:%d\n", dpid);

	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));
	system_rtc_mem_read(CURRENT_PAGE_POS, (void *)&position, sizeof(uint32_t));
	// 收集传感器信息
	DHT11Refresh();
	temphum = DHT11Read();
	status->temperature = temphum->tempHighPart;
	status->humidity = temphum->humHighPart;
	status->batLevel = hw_get_battery_level();
	status->rssi = wifi_station_get_rssi();
	status->sysopmode = wifi_get_opmode();
	status->syspowermode = powermode.type;
	// 刷新页面
	if(weather->weatherIcon < 0) {
		// weather->weatherIcon为负数表示网页解析失败
		invalidateNoInternet(calendar, status);
	}else {
		// 交替显示
		if(ViewPages[position] == DisplayNone) {
			position = 0;
		}
		dpid = ViewPages[position];

		switch(dpid) {
			case DisplayBasic:
				invalidateBasic(calendar, weather, status);
				break;
			case DisplayForecast:
				invalidateForecast(calendar, weather, forecastWeather, status);
				break;
			case DisplayNote:
				invalidateNoteView(calendar, weather, status);
				break;
			case DisplayGallery:
				invalidateGalleryView();
				break;
			default:
				break;
		}
		position = (position < (VIEW_PAGE_MAX - 1)) ? (position + 1) : 0;
		// 回写当前页面标记
		system_rtc_mem_write(CURRENT_PAGE_POS, (const void *)&position, sizeof(uint32_t));
	}
}

/**
 * @brief request请求队列完成回调
 * */
static void ICACHE_FLASH_ATTR requestFinishedHandler(uint32_t eventId, uint32_t arg) {
	uint32_t clkarm, level;
	PowerMode powermode;

	level = hw_get_battery_level();
	if(level < 20) {
		// 电量小于20%, 显示电量低图片并关机
		batteryLowShutdown();
		return;
	}

	system_rtc_mem_read(CLOCKARM_POS, (void *)&clkarm, sizeof(uint32_t));
	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));
	sys_runtime_set(RUNTIME_INTERNET_BIT, 1);

	invalidateView();

	if(clkarm == 0 && (powermode.type != POWER_LIGHT_SLEEP)) {
		clkarm = 1;
		system_rtc_mem_write(CLOCKARM_POS, (const void *)&clkarm, sizeof(uint32_t));
		loadConfigIntoRTCMenory();
		os_timer_arm(&clockTimer, 60000, TRUE);
	}
	postEventDelay(EVENT_UPDATE_EPD, 100);
}

/**
 * @brief EPD刷新完成回调
 * @param arg 附加的整形参数
 * */
static void ICACHE_FLASH_ATTR epdFinishedHandler(uint32_t eventId, uint32_t arg) {
	uint8_t opmode;
	PowerMode powermode;
	SystemConfig *config = NULL;
	uint32_t bootFlag;

	// 由reset启动显示图片完成后再联网
	system_rtc_mem_read(POWER_ON_REASON_POS, (void *)&bootFlag, sizeof(uint32_t));
	if(bootFlag == POWER_BY_RESET_SET) {
		bootFlag = POWER_BY_RESET_CLEAR;
		system_rtc_mem_write(POWER_ON_REASON_POS, (const void *)&bootFlag, sizeof(uint32_t));
		config = sys_config_get();
		wifi_set_opmode(STATION_MODE);

		if(config->status.apNeedPwd == STATUS_VALID) {
			station_info_setup(config->wifiName, config->wifiPwd);
		}else {
			station_info_setup(config->wifiName, "");
		}
		return;
	}

	opmode = wifi_get_opmode();
	system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));

	// 在关机模式 /LIGHT_SLEEP模式主动断开WiFi, 跳转到wifi_event_callback
	switch(powermode.type) {
		case POWER_LIGHT_SLEEP:
		case POWER_SHUTDOWN:
			if(opmode == NULL_MODE) {
				// POWER_LIGHT_SLEEP 醒来刷新屏幕
				enterLightSleep();
			}else if(opmode == STATION_MODE) {
				os_timer_disarm(&clockTimer);
				wifi_station_disconnect();
			}else if(opmode == SOFTAP_MODE) {
				wifi_set_opmode(NULL_MODE);
				system_deep_sleep(0);
			}
			break;
		case POWER_REBOOT:
			// 升级固件刷新完屏幕重启
			system_restart();
			break;
		case POWER_BATLOW:
			// 低电量处理
			if(opmode == NULL_MODE || opmode == SOFTAP_MODE) {
				wifi_set_opmode(NULL_MODE);
				system_deep_sleep(0);
			}else {
				os_timer_disarm(&clockTimer);
				wifi_station_disconnect();
			}
			break;
		default:
			break;
	}
}

/**
 * @brief 显示自定义图片回调
 * @param eventId 事件ID(固定值和mainEventHandler绑定)
 * @param arg 附加的整形参数
 * */
static void ICACHE_FLASH_ATTR displayImageHandler(uint32_t eventId, uint32_t arg) {
	File file;
	uint8_t fileblock[FILENAME_FULLSIZE];
	uint32_t width, height, top, left;
	PowerMode powermode;
	FreezeFrame freezeFrame;

	if(EPDGetStatus() != IDLE) {
		// 屏幕正在刷新时，忽略重复的图片预览设置
		return;
	}
	// arg传递的为File.block
	spi_flash_read(arg, (uint32_t *)fileblock, FILENAME_FULLSIZE);
	open_file_raw(&file, fileblock, fileblock + 8);
	// 二次检查bmp文件格式，并获取宽/高信息
	if(GuiCheckBMPFormat(&file, &width, &height, NULL) != 0) {
		return;
	}

	system_rtc_mem_read(FREEZEFRAME_POS, (void *)&freezeFrame, sizeof(FreezeFrame));

	// 根据对齐方式计算绘制图片的top-left
	// 高4bit水平
	left = ((freezeFrame.align >> 4) & 0xF);
	if(IMAGE_ALIGN_START == left) {
		left = 0;
	}else if(IMAGE_ALIGN_CENTER == left) {
		left = (SCREEN_WIDTH / 2) - (width / 2);
	}else {
		// IMAGE_ALIGN_END
		left = (SCREEN_WIDTH - width);
	}
	// 低4bit垂直
	top = (freezeFrame.align & 0xF);
	if(top == IMAGE_ALIGN_START) {
		top = 0;
	}else if(IMAGE_ALIGN_CENTER == top) {
		top = (SCREEN_HEIGHT / 2) - (height / 2);
	}else {
		// IMAGE_ALIGN_END
		top = (SCREEN_HEIGHT - height);
	}

	EPDDisplayClear();
	GuiDrawBmpImageImpl(&file, left, top);

	if((freezeFrame.type == FREEZE_TYPE_FIXED) || (freezeFrame.type == FREEZE_TYPE_SLEEP)) {
		// 固定显示/冻结显示都关闭clockTimer
		os_timer_disarm(&clockTimer);
		top = 0x00000000; // top复用做CLOCKARM标记， 清除CLOCKARM_POS标记
		system_rtc_mem_write(CLOCKARM_POS, (const void *)&top, sizeof(uint32_t));
	}

	if(freezeFrame.type == FREEZE_TYPE_SLEEP) {
		powermode.type = POWER_SHUTDOWN;
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
	}

	postEventDelay(EVENT_UPDATE_EPD, 100);
}

/**
 * @brief 手动联网刷新天气前关闭clockTimer回调
 * */
static void ICACHE_FLASH_ATTR closeTimerHandler(uint32_t eventId, uint32_t arg) {
	uint32_t clkarm = 0x00000000;
	os_timer_disarm(&clockTimer);
	system_rtc_mem_write(CLOCKARM_POS, (const void *)&clkarm, sizeof(uint32_t));
	if(CLOSE_TIMER_AND_UPDATE == arg) {
		requestInternetUpdate();
	}
}

/**
 * 电源切换回调，关闭clockTimer，关机时显示关机图片, 固件升级时显示升级图片
 * @param arg POWERTOGGLE_SHUTDOWN 显示关机图片
 * @param arg POWERTOGGLE_UPDATE 显示升级图片
 * */
static void ICACHE_FLASH_ATTR powerToggleHandler(uint32_t eventId, uint32_t arg) {
	PowerMode powermode;
	os_timer_disarm(&clockTimer);
	uint32_t size, calc;

	EPDDisplayClear();

	if(arg == POWERTOGGLE_SHUTDOWN) {
		size = shutdown_size;
		calc = (size / 4);
		if((calc * 4) < size) {
			calc++;
		}
		GuiDrawImagePacked(shutdown_image, 0, 0);

	}else if(arg == POWERTOGGLE_UPDATE) {
		system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));
		powermode.type = POWER_REBOOT;
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		GuiDrawImagePacked(update_image, 0, 0);

	}else if(arg == POWERTOGGLE_IDLE_TIMEOUT) {
		GuiDrawImagePacked(idle_image, 0, 0);
	}

	postEventDelay(EVENT_UPDATE_EPD, 100);
}

/**
 * udp接收回调，由handlerDispatch分发
 * @param *arg 为espconn_regist_recvcb(&udp_espconn, udp_recv_callback);注册时提供的udp_espconn指针
 * @param *pdata 接收到的数据，非对齐
 * @param len 接收到的数据长度
 * */
static void ICACHE_FLASH_ATTR udp_recv_callback(void *arg, char *pdata, unsigned short len) {
	handlerDispatch(&udp_espconn, (uint8_t *)pdata, len);
}

/**
 * @brief epd刷新定时器回调
 * @param timer_arg
 * */
static void ICACHE_FLASH_ATTR postDelayTimerCallback(void *timer_arg) {
	uint8_t opmode;
	PowerMode powermode;
	SystemRunTime *runtime;
	uint32_t pageFlag = 0x0;

	os_timer_disarm(&postDelayTimer);

	if(delayEventId == EVENT_UPDATE_EPD && EPDGetStatus() == IDLE) {
		EPDGpioSetup();
		EPDReset();
		EPDInit(LUT_FULL_UPDATE);
		EPDFlush();
		EPDTurnOnDisplay();

	}else if(delayEventId == EVENT_DOUBLE_CLICK) {
		opmode = wifi_get_opmode();
		runtime = sys_runtime_get();
		system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&powermode, sizeof(PowerMode));
		// 无网络即可滤掉SOFTAP_MODE, STATIONAP_MODE
		if(runtime->internet == 0) {
			return;
		}
		// SOFTAP_MODE，STATIONAP_MODE不做处理
		if(opmode == NULL_MODE || opmode == STATION_MODE) {
			closeTimerHandler(0, 0);
			// 预写入低功耗模式
			powermode.type = POWER_LIGHT_SLEEP;
			powermode.refreshAfterConnect = FALSE;
			powermode.wifiState = POWER_STATE_WIFI_DEFAULT;
			system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
			// 双击后统一从默认页面开始显示
			system_rtc_mem_write(CURRENT_PAGE_POS, (const void *)&pageFlag, sizeof(uint32_t));
			invalidateView();
			postEventDelay(EVENT_UPDATE_EPD, 100);
		}

	}else if(delayEventId == EVENT_LONG_CLICK) {
		powermode.type = POWER_SHUTDOWN;
		powermode.refreshAfterConnect = FALSE;
		powermode.wifiState = POWER_STATE_WIFI_DEFAULT;
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		powerToggleHandler(0, POWERTOGGLE_SHUTDOWN);
	}
}

/**
 * @brief 提交一个延时执行事件
 * @param eventId 事件ID, 必须是postDelayTimerCallback正确处理的ID才会被执行
 * @param miliseconds 延时时间(毫秒)
 * */
static void ICACHE_FLASH_ATTR postEventDelay(uint32_t eventId, uint32_t miliseconds) {
	delayEventId = eventId;
	os_timer_arm(&postDelayTimer, miliseconds, FALSE);
}

/**
 * @brief 空闲10分钟后或电量低于20%自动关机
 * */
static void ICACHE_FLASH_ATTR idleTimerCallback(void *timer_arg) {
	uint32_t level, tick;
	PowerMode powermode;

	system_rtc_mem_read(IDLE_TICK_POS, &tick, sizeof(uint32_t));
	tick++;

	if(tick >= IDLE_TIMEOUT_MAX) {
		// 空闲10分钟关机
		powermode.type = POWER_SHUTDOWN;
		powermode.refreshAfterConnect = FALSE;
		powermode.wifiState = POWER_STATE_WIFI_DEFAULT;
		system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
		powerToggleHandler(0, POWERTOGGLE_IDLE_TIMEOUT);
		return;
	}

	system_rtc_mem_write(IDLE_TICK_POS, (const void *)&tick, sizeof(uint32_t));

	level = hw_get_battery_level();
	if(level < 20) {
		// 电量小于20%, 显示电量低图片并关机
		batteryLowShutdown();
		return;
	}
}

static const uint32_t WakeupLUT[4] = {0xFF36210F, 0x5FFFF4FF, 0xFFF6FFFF, 0xF7FFFFFF};
static const uint8_t WakeupInterval[8] = {60, 120, 180, 150, 150, 150, 240, 225};
/**
 * @brief 进度light_sleep模式
 * */
static void ICACHE_FLASH_ATTR enterLightSleep(void) {
	// time update selection(minutes):  1,  2,   3,   4,   5,   10 , 15,  20,  30
	// wakeup interval(minutes):        1,  2,   3,   4,   2.5, 2.5, 2.5, 4,   3.75
	// wakeup interval(seconds):        60, 120, 180, 240, 150, 150, 150, 240, 225
	uint32_t compare, index, seconds;
	UpdateInfo info;
	system_rtc_mem_read(WAKEUP_INTERVAL_POS, &seconds, sizeof(uint32_t));
	if(seconds == 0) {
		wifi_fpm_close();
		ETS_GPIO_INTR_DISABLE();
		system_rtc_mem_read(UPDATEINFO_POS, (void *)&info, sizeof(UpdateInfo));
		compare = (info.timeCompare / 60);
		index = (compare >> 3);
		compare -= (index << 3);
		index = (WakeupLUT[index] >> (compare << 2)) & 0xF;
		seconds = (index != 0xF) ? WakeupInterval[index] : WakeupInterval[0];
		system_rtc_mem_write(WAKEUP_INTERVAL_POS, (const void *)&seconds, sizeof(uint32_t));
	}
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
	wifi_fpm_open();
	wifi_fpm_set_wakeup_cb(lightSleepWakeupCallback);
	// 取值范围10000~268435455us
	wifi_fpm_do_sleep(seconds * 1000000);
}

/**
 * @brief 低电量关机
 * */
static void ICACHE_FLASH_ATTR batteryLowShutdown(void) {
	PowerMode powermode;
	GuiDrawImagePacked(batlow_image, 0, 0);
	powermode.type = POWER_BATLOW;
	system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&powermode, sizeof(PowerMode));
	// 稍微延时长点，以便执行写入启动成功标记
	postEventDelay(EVENT_UPDATE_EPD, 100);
}

/**
 * @brief http超时未响应回调
 * @note 超时后用上次的数据刷新界面，然后由powermode进入对应模式
 * */
static void ICACHE_FLASH_ATTR httpTimeoutHandler(void) {
	invalidateView();
}
