/*
 * misc.c
 * @brief
 * Created on: Jul 6, 2021
 * Author: Yanye
 */

#include "utils/misc.h"

/**
 * @brief 更新Date对象
 * @param info 刷新计数器
 * @param date 日期结构
 * @param unit 时间增量(秒)
 * */
void ICACHE_FLASH_ATTR updateClockByTick(UpdateInfo *info, Date *date, uint32_t unit) {
	// time update selection(minutes):  1,  2,   3,   5,   10 , 15,  20,  30
	// wakeup interval(minutes):        1,  2,   3,   2.5, 2.5, 2.5, 4,   3.75
	// wakeup interval(seconds):        60, 120, 180, 150, 150, 150, 240, 225
	uint32_t minute, second;
	Calendar *calendar;

	info->timeTick += unit;
	info->weatherTick += unit;
	calendar = Context.getCalendar();

	do{
		minute = (unit / 60);
		second = (unit - minute * 60);
		// second:0~59， date->second:0~59 相加不会溢出
		date->second += second;
		if(date->second > 59) {
			date->second -= 60;
			date->minute++;
		}
		date->minute += minute;
		if(date->minute <= 59){
			break;
		}
		date->minute -= 60;
		date->hour++;
		if(date->hour <= 23) {
			break;
		}
		date->hour = 0;
	}while(0);
	// 复制date结构到calendar头部
	os_memcpy(calendar, date, sizeof(Date));
}
// 24bytes + '\0'
static const uint8_t *PATTERN = "晴雷雨云雪阴雾风";
static const sint8_t MATCHS[8] = {0, 4, 6, 1, 14, 2, 20, 30};
/**
 * @brief 根据天气描述匹配天气图标ID，兼容getWeatherIcon
 * @param weatherDesc 天气中文描述，UTF-8编码
 * @return weatherId, view中需要调用getWeatherIcon找到实际的图标名称
 *
 * */
sint8_t ICACHE_FLASH_ATTR matchWeatherId(uint8_t *weatherDesc) {
	uint32_t i, len;
	uint8_t buffer[4];
	len = os_strlen(weatherDesc);
	if(len < 3) {
		return 127;
	}
	os_memset(buffer, 0x00, sizeof(buffer));
	for(i = 0; i < sizeof(MATCHS); i++) {
		// 每次固定拷贝3字节，buffer[3]可以保证为'\0'
        // 一个中文UTF-8占3字节
		os_memcpy(buffer, (PATTERN + i * 3), 3);
		if(contains(weatherDesc, buffer) != -1) {
			return MATCHS[i];
		}
	}
	return 127;
}

/**
 * @brief 加载运行时数据到RTC存储区，随CLOCKARM_POS使能时调用
 * @note 必须判断sysConfExist和sysConfRead!=NULL前调用
 * */
void ICACHE_FLASH_ATTR loadConfigIntoRTCMenory(void) {
	SystemConfig *config;
	UpdateInfo info;
	AutoSleep autosleep;
	NightSpan nightSpan;

	config = sys_config_get();
	os_memcpy(&autosleep, config->autoSleep, sizeof(AutoSleep));
	os_memcpy(&nightSpan, config->nightSpan, sizeof(NightSpan));

	info.weatherCompare = config->weatherUpdate * 3600;
	info.weatherTick = 0;
	info.timeCompare = config->timeUpdate * 60;
	info.timeTick = 0;
	system_rtc_mem_write(UPDATEINFO_POS, (const void *)&info, sizeof(UpdateInfo));
	// 自动低功耗
	autosleep.tick = 0;
	system_rtc_mem_write(AUTO_SLEEP_POS, (const void *)&autosleep, sizeof(AutoSleep));
	// 夜间不更新
	nightSpan.isEntered = 0;
	system_rtc_mem_write(NIGHT_SPAN_POS, (const void *)&nightSpan, sizeof(NightSpan));
}


#define MIN_RSSI     -100
#define MAX_RSSI     -55

int ICACHE_FLASH_ATTR calculateSignalLevel(int rssi, int numLevels) {
  if(rssi <= MIN_RSSI) {
    return 0;
  } else if (rssi >= MAX_RSSI) {
    return numLevels - 1;
  } else {
    float inputRange = (MAX_RSSI - MIN_RSSI);
    float outputRange = (numLevels - 1);
    return (int)((float)(rssi - MIN_RSSI) * outputRange / inputRange);
  }
}


/*
解析时间戳信息到年月日时分秒
@param ts 10位时间戳信息,4字节
@param timeZone 时区,相对格林威治时间的偏移
@param *buffer 用于存储转换结果的缓冲区,转换后的结果每位占2字节
*/
void ICACHE_FLASH_ATTR praseTimestamp(int ts, int timeZone, int *buffer) {
	const uint8_t dayOfMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    //闰年标记
    uint8_t leapYear = 0;
    int i = 0;

    //中国区需要补28800秒
    //计算时间戳的天数
    int days = (ts + 3600 * timeZone) / 86400 + 1;
    //四年周期数
    int year4 = (days / 1461);
    //不足四年周期剩下的天数
    int remain = (days % 1461);

    //得到时间戳范围内满足四年周期的年数
    buffer[yyyy] = 1970 + year4 * 4;

    //剩下的天数小于一年@365不处理
    if(remain < 365){
        //@占位保留
    //第二年@365+365
    }else if(remain < 730){
        buffer[yyyy]++;
        remain -= 365;
    //第三年@365+365+365
    }else if(remain < 1095){
        buffer[yyyy] += 2;
        remain -= 730;
    //第四年
    }else{
        buffer[yyyy] += 3;
        remain -= 1095;
        leapYear = 1;
    }

    //剩下一年内的天数 用12个月天数循环减
    //直到剩下天数<=0 则找到对应月份
    //剩下天数则为日期
    for(i=0;i<12;i++){

        //year4复用于暂存每月天数
        //闰年2月补一天
        year4 = (i == 1 && leapYear) ? (dayOfMonth[i] + 1) : dayOfMonth[i];

        //days复用于暂存减去当前月份天数的剩余天数
        days = remain - year4;

        if(days <= 0){

            buffer[MM] = i + 1;
            if(days == 0){
                buffer[dd] = year4;

            }else{
				buffer[dd] = remain;
            }
            break;
        }
        remain = days;
    }

    buffer[ss] = (ts % 60);

    buffer[mm] = (ts /60 % 60);

    while(ts > 86400){
        ts -= 86400;
    }

    if(ts == 86400){
        buffer[HH] = 0;
    }else{
        //中国区需要+8h
        buffer[HH] =(ts / 3600) + timeZone;
    }

    if(buffer[HH] >= 24){
         buffer[HH] -= 24;
    }
}

