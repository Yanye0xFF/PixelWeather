/*
 * rtc_mem.h
 * @brief
 * Created on: Mar 7, 2021
 * Author: Yanye
 */

#ifndef _RTC_MEM_H_
#define _RTC_MEM_H_

typedef struct _update_info {
	// 天气更新比较值(秒)
	uint32_t weatherCompare;
	// 天气更新计数值(秒/tick)
	uint32_t weatherTick;
	// 时间更新比较值(秒)
	uint32_t timeCompare;
	// 时间更新计数值(秒/tick)
	uint32_t timeTick;
} UpdateInfo;

#define POWER_NONE_SLEEP      0x00
// 射频部分(1分钟间隔交替打开/关闭)
#define POWER_MODEM_SLEEP     0x01
// 关闭射频及CPU，仅在时间更新节点唤醒刷新屏幕，仅在天气更新节点打开射频
#define POWER_LIGHT_SLEEP     0x02
// 关机模式
#define POWER_SHUTDOWN        0x03
// 升级固件刷新完屏幕重启
#define POWER_REBOOT          0x05
// 低电量强制关机
#define POWER_BATLOW          0x06

// 低功耗未开启默认状态
#define POWER_STATE_WIFI_DEFAULT    0x00
// 低功耗开启且当前wifi打开
#define POWER_STATE_WIFI_ON         0x01
// 低功耗开启且当前wifi关闭
#define POWER_STATE_WIFI_OFF        0x02

typedef struct _power_mode {
	// 电源模式POWER_NONE_SLEEP...POWER_SHUTDOWN
	uint8_t type;
	// 当前射频模块状态POWER_STATE_WIFI_DEFAULT...POWER_STATE_WIFI_OFF
	uint8_t wifiState;
	// 连接WiFi后(EVENT_STAMODE_GOT_IP) 是否需要联网更新天气和时间
	uint8_t refreshAfterConnect;
	uint8_t dummy;
} PowerMode;

#define FREEZE_TYPE_PREVIEW    0x0
#define FREEZE_TYPE_FIXED      0x1
#define FREEZE_TYPE_SLEEP      0x2

#define IMAGE_ALIGN_START    0x0
#define IMAGE_ALIGN_CENTER   0x1
#define IMAGE_ALIGN_END      0x2

typedef struct _freeze_frame {
	// 定格显示类型
	uint8_t type;
	// 对齐方式，高4位水平，低4位垂直
	uint8_t align;
	uint8_t dummy;
	uint8_t dummy1;
} FreezeFrame;

// 自动休眠参数配置
typedef struct _auto_sleep {
	// 是否开启自动休眠  0:未开启 1:开启
	uint8_t valid;
	// 自动休眠计数器，范围0~255，单位分钟
	uint8_t tick;
	// 自动休眠比较器，范围0~255，单位分钟
	int8_t compare;
	// 保留
	uint8_t dummy;
} AutoSleep;

// 夜间不更新屏幕参数
typedef struct _night_span {
	// 是否开启夜间模式 0:未开启 1:开启
	uint8_t valid;
	// 起始时间(小时)，24小时制，范围0点~23点
	uint8_t start;
	// 终止时间(小时)，24小时制，范围0点~23点
	// 终止时间 > 起始时间
	uint8_t end;
	// 当前是否进入了不更新区间
	uint8_t isEntered;
} NightSpan;

// onNetUpdateRequest 限制1分钟后更新时间戳(RTC时间) 4bytes
#define NETUPDATE_POS            64

// clockTimerCallback 时间/天气刷新计时 16bytes
#define UPDATEINFO_POS           65

// DATE结构 8bytes
#define DATE_POS                 69

// requestCallback启动clockTimer防重入 4bytes
#define CLOCKARM_POS             71

// wifi EVENT_STAMODE_DISCONNECTED 重连计数 4bytes
#define RECONNECT_POS            72

// list_file list_file_raw 起始地址记录 4bytes
#define FILEBLOCK_START_POS      73

// 存储PowerMode结构 4bytes
#define POWERMODE_INFO_POS       74

// 存储FreezeFrame结构 4bytes
#define FREEZEFRAME_POS          75

// 存储唤醒周期(秒) 4bytes
#define WAKEUP_INTERVAL_POS      76

// 当前显示页信息
#define CURRENT_PAGE_POS         77

#define POWER_BY_RESET_CLEAR     0
#define POWER_BY_RESET_SET       1
// 由Reset按钮启动，需要显示联网中图片
#define POWER_ON_REASON_POS      78

// 自动切到休眠模式
#define AUTO_SLEEP_POS           79

// 夜间不更新区间
#define NIGHT_SPAN_POS           80

// 系统空闲计数
#define IDLE_TICK_POS            81

// must update
#define RTCMEM_START       64
#define RTCMEM_END         81

#endif /* APP_USER_RTC_MEM_H_ */
