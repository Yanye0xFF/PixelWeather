/*
 * basic_layout.c
 * @brief 基本页面布局，显示大图标时间，今日天气
 * Created on: Jun 25, 2021
 * Author: Yanye
 */

#include "view/basic_layout.h"
#include "utils/misc.h"

static const char *tempText = "温度";
static const char * numIconPrefix = "digit";

/**
 * @brief 刷新天气预报页面布局
 * @param calendar 日期信息
 * @param weather 今日天气信息
 * @param status 状态栏信息，电量、信号强度、温湿度传感器
 * */
void ICACHE_FLASH_ATTR invalidateBasic(Calendar *calendar, BasicWeather *weather, StatusBar *status) {
	Font *chs12px, *eng16px, *eng12px, *ext16px;
	char *iconFileName;

	char buffer[16];
	uint8_t parts[4];
	int32_t i, xpos;
	// 时间图片绘制的左上X轴坐标(Y轴相同)
	const uint16_t numIconPosX[] = {98, 135, 176, 213};
	const uint16_t numIconPosY = 26;
	const uint8_t limits[] = {90, 70, 40, 10, 0};

	chs12px = getFont(FONT12x12_CN);
	eng16px = getFont(FONT08x16_EN);
	eng12px = getFont(FONT06x12_EN);
	ext16px = getFont(FONT08x16_EXT);

	EPDDisplayClear();

	// 天气图标48*48
	iconFileName = (char *)getWeatherIcon(weather->weatherIcon);
	GuiDrawBmpImage(iconFileName, "bmp", 0, 0);

	// 城市名称 "常州"，两个字的居中，三个字以上的居左
	xpos = (os_strlen(weather->cityName) < 7) ? 60 : 48;
	GuiDrawStringUTF8(weather->cityName, xpos, 0, chs12px, eng16px);

	// 湿度  湿度99%
	GuiDrawBmpImage("ic_sd", "bmp", 50, 16);
	// xpos 复用做湿度临时变量, 限制只显示2个字符（湿度总是正数）
	xpos = (weather->humidity > 99) ? 99 : weather->humidity;
	i = integer2String(xpos, buffer, sizeof(buffer));
	os_strcpy((buffer + i), "%");
	GuiDrawString(buffer, 67, 18, chs12px, eng16px);
	// 紫外线强度
	GuiDrawBmpImage("ic_uv", "bmp", 50, 34);
	GuiDrawStringUTF8(weather->ultravioletDesc, 67, 36, chs12px, eng16px);

	// 气象描述 "小雨到中雨"
	GuiDrawStringUTF8(weather->weatherDesc, 0, 50, chs12px, eng16px);
	// 最高/最低温度
	xpos = GuiDrawStringUTF8((uint8_t *)tempText, 0, 66, chs12px, eng16px);
	i = integer2String(weather->tempLowest, buffer, sizeof(buffer));
	os_strcpy((buffer + i), "～");
	xpos = GuiDrawStringUTF8(buffer, (xpos + 2), 66, chs12px, eng16px);
	i = integer2String(weather->tempHighest, buffer, sizeof(buffer));
	os_strcpy((buffer + i), "℃");
	GuiDrawStringUTF8(buffer, xpos, 66, chs12px, eng16px);
	// 风向
	GuiDrawStringUTF8(weather->windDesc, 0, 82, chs12px, eng16px);
	// 明天/后天天气情况
	GuiDrawStringUTF8(weather->tomorrowWeatherDesc, 0, 97, chs12px, eng16px);
	GuiDrawStringUTF8(weather->dayAfterTomorrowWeatherDesc, 0, 110, chs12px, eng16px);

	// 公历信息
	GuiDrawStringUTF8(calendar->calendarDesc, 98, 0, chs12px, eng16px);
	// 农历信息
	GuiDrawStringUTF8(calendar->lunarDesc, 98, 13, chs12px, eng16px);

	// 时间显示
	parts[0] = (uint8_t)(calendar->hour / 10);
	parts[1] = (uint8_t)(calendar->hour % 10);
	parts[2] = (uint8_t)(calendar->minute / 10);
	parts[3] = (uint8_t)(calendar->minute % 10);

	os_strcpy(buffer, numIconPrefix);
	buffer[6] = '\0';
	// 绘制时间数字图片
	for(i = 0; i < 4; i++) {
		buffer[5] = '0' + parts[i];
		GuiDrawBmpImage(buffer, "bmp", numIconPosX[i], numIconPosY);
	}

	// 绘制小时:分钟中间的':'分隔符
	GuiFillColor(172, 45, 175, 51, BLACK);
	GuiFillColor(172, 71, 175, 77, BLACK);

	// 底部状态栏, 室内图标
	GuiDrawBmpImage("home", "bmp", 173, 97);
	// 温度支持范围-9 ~ 99，小于0度只能显示低位, 大于0度显示2位
	buffer[0] = (status->temperature < 0) ? '-' : ('0' + (status->temperature / 10));
	buffer[1] = '0' + (status->temperature % 10);
	os_strcpy((buffer + 2), "'C/");
	// 湿度总是正数 显示范围00~99
	i = (status->humidity > 99) ? 99 : status->humidity;
	buffer[5] = '0' + (i / 10);
	buffer[6] = '0' + (i % 10);
	os_strcpy((buffer + 7), "%RH");
	GuiDrawString(buffer, 185, 98, chs12px, eng12px);

	// 电量等级图标
	for(i = 0; i < 5; i++) {
		if(status->batLevel >= limits[i]) break;
	}
    os_strcpy(buffer, "bat");
    buffer[3] = '0' + i;
    buffer[4] = '\0';
    GuiDrawBmpImage(buffer, "bmp", 173, 110);
    // 清除[电量/信号]显示区域
	GuiFillColor(185, 110, 249, 121, WHITE);

	if(status->batLevel > 90) {
        os_strcpy(buffer, "100%");
	}else if(status->batLevel > 0) {
		buffer[0] = '0' + (status->batLevel / 10);
    	os_strcpy((buffer + 1), "0%");
    }else {
    	os_strcpy(buffer, "0%");
    }
	GuiDrawString(buffer, 185, 110, chs12px, eng12px);

	// 国标特殊字符的信号图标
	GuiDrawChar(0x1F, 215, 108, BLACK, WHITE, ext16px, 14);
	if(status->sysopmode == STATION_MODE && status->syspowermode == POWER_NONE_SLEEP) {
		i = calculateSignalLevel(status->rssi, MAX_LEVEL);
		// 信号等级icon
		GuiDrawChar(0x20 + i, 223, 108, BLACK, WHITE, ext16px, 14);
	}else {
		GuiDrawChar('X', 223, 110, BLACK, WHITE, eng12px, 12);
		if(POWER_MODEM_SLEEP == status->syspowermode) {
			GuiDrawBmpImage("mslp", "bmp", 232, 110);
		}else if(POWER_LIGHT_SLEEP == status->syspowermode) {
			GuiDrawBmpImage("lslp", "bmp", 232, 110);
		}
	}
}

