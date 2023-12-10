/*
 * note_layout.c
 * @brief
 * Created on: Oct 28, 2021
 * Author: Yanye
 */

#include "view/note_layout.h"

#include "osapi.h"
#include "user_interface.h"

#include "utils/rtc_mem.h"
#include "utils/strings.h"
#include "utils/fixed_file.h"
#include "utils/misc.h"
#include "graphics/font.h"
#include "graphics/displayio.h"

#include "driver/ssd1675b.h"
#include "controller/context.h"

/**
 * @brief 刷新天气预报页面布局
 * @param calendar 日期信息
 * @param weather 今日天气信息
 * @param weathers 预报天气数据集 数量为FORECAST_DAYS
 * @param status 状态栏信息，电量、信号强度、温湿度传感器
 * */
void ICACHE_FLASH_ATTR invalidateNoteView(Calendar *calendar, BasicWeather *weather, StatusBar *status) {
	const uint8_t limits[] = {90, 70, 40, 10, 0};
	const uint8_t drawIconX[] = {0, 125, 0, 125};
	const uint8_t drawIconY[] = {13, 13, 62, 62};
	ForecastWeather *weatherPtr;
	uint8_t buffer[256];
	Font *chs12px, *chs16px, *eng16px, *eng12px, *ext16px;
	int32_t cursor, xpos = 0, i;
	char *iconFileName;
	fixed_file_t fixedFile;

	chs12px = getFont(FONT12x12_CN);
	chs16px = getFont(FONT16x16_CN);
	eng16px = getFont(FONT08x16_EN);
	eng12px = getFont(FONT06x12_EN);
	ext16px = getFont(FONT08x16_EXT);

	EPDDisplayClear();

	// 城市名称限制2个字
	os_memset(buffer, 0x00, sizeof(buffer));
	os_strncpy(buffer, weather->cityName, 6);
	xpos = GuiDrawStringUTF8(buffer, 0, 0, chs12px, eng16px);
	xpos = GuiDrawStringUTF8(calendar->calendarDesc, (xpos + 4), 0, chs12px, eng16px);
	// 室内图标
	GuiDrawBmpImage("home", "bmp", (xpos), 0);
	// 温度支持范围-9 ~ 99，小于0度只能显示低位, 大于0度显示2位
	buffer[0] = (status->temperature < 0) ? '-' : ('0' + (status->temperature / 10));
	buffer[1] = '0' + (status->temperature % 10);
	os_strcpy((buffer + 2), "'C/");
	// 湿度总是正数 限制显示范围00~99
	i = (weather->humidity > 99) ? 99 : weather->humidity;
	buffer[5] = '0' + (i / 10);
	buffer[6] = '0' + (i % 10);
	os_strcpy((buffer + 7), "%RH");
	GuiDrawString(buffer, (xpos + 12), 0, chs12px, eng12px);

	i = 0;
	fixed_file_init(&fixedFile, FILE_NOTE_SECTION_SIZE, FILE_NOTE_MAX_SIZE);
	if(fixed_file_open(&fixedFile, "note", "ini")) {
		// note content
		if(fixed_file_read(&fixedFile, buffer, FILE_NOTE_SECTION_SIZE) == FILE_NOTE_SECTION_SIZE) {
			i = 1;
		}
	}
	chs16px->vspacing = 3;
	if(i) {
		// 每行有3个像素的间距 // 15 34 53 72 91
		GuiDrawStringUTF8((buffer + 10), 0, 15, chs16px, eng16px);
	}else {
		GuiDrawStringUTF8("》备忘录中没有内容《", 40, 23, chs16px, eng16px);
		GuiDrawStringUTF8("请打开“天气站”APP，在“页面管理”->“备忘录”中添加笔记。", 0, 48, chs16px, eng16px);
	}
	chs16px->vspacing = 0;

	// footer
	xpos = GuiDrawStringUTF8(calendar->lunarDesc, 0, 110, chs12px, eng16px);

	buffer[0] = '0' + (calendar->hour / 10);
	buffer[1] = '0' + (calendar->hour % 10);
	os_strcpy((buffer + 2), "：");
	xpos = GuiDrawStringUTF8(buffer, (xpos + 4), 110, chs12px, eng16px);

	buffer[0] = '0' + (calendar->minute / 10);
	buffer[1] = '0' + (calendar->minute % 10);
	buffer[2] = '\0';
	GuiDrawStringUTF8(buffer, (xpos - 4), 110, chs12px, eng16px);

	xpos = 176;
	// 电量等级图标
	for(i = 0; i < 5; i++) {
		if(status->batLevel >= limits[i]) break;
	}
	os_strcpy(buffer, "bat");
	buffer[3] = '0' + i;
	buffer[4] = '\0';
	GuiDrawBmpImage(buffer, "bmp", (xpos), 110);

	// 电量等级数字
	if(status->batLevel > 90) {
		os_strcpy(buffer, "100%");
	}else if(status->batLevel > 0) {
		buffer[0] = '0' + (status->batLevel / 10);
		os_strcpy((buffer + 1), "0%");
	}else {
		os_strcpy(buffer, "0%");
	}

	xpos = GuiDrawString(buffer, (xpos + 12), 110, chs12px, eng12px);

	// 国标特殊字符的信号图标，按照14高度绘制，屏幕空间不足
	GuiDrawChar(0x1F, (xpos), 108, BLACK, WHITE, ext16px, 14);
	if(status->sysopmode == STATION_MODE && status->syspowermode == POWER_NONE_SLEEP) {
		i = calculateSignalLevel(status->rssi, MAX_LEVEL);
		// 信号等级icon
		GuiDrawChar(0x20 + i, 223, 108, BLACK, WHITE, ext16px, 14);
	}else {
		GuiDrawChar('X', (xpos + 8), 110, BLACK, WHITE, eng12px, 12);
		if(POWER_MODEM_SLEEP == status->syspowermode) {
			GuiDrawBmpImage("mslp", "bmp", xpos + 16, 110);
		}else if(POWER_LIGHT_SLEEP == status->syspowermode) {
			GuiDrawBmpImage("lslp", "bmp", xpos + 16, 110);
		}
	}
}

