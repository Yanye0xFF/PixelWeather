/*
 * nointernet_layout.c
 * @brief 无网络，网页解析错误页面
 * Created on: Jun 30, 2021
 * Author: Yanye
 */

#include "view/nointernet_layout.h"

static const char * numIconPrefix = "digit";

/**
 * @param calendar 日期信息
 * @param status 状态栏信息，电量、信号强度、温湿度传感器
 * */
void ICACHE_FLASH_ATTR invalidateNoInternet(Calendar *calendar, StatusBar *status) {
	char buffer[32];
	int32_t cursor, i, xpos;
	Font *chs12px, *eng16px, *eng12px, *ext16px;
	// 时间图片绘制的左上X轴坐标(Y轴相同)
	const uint16_t numIconPosX[] = {40, 77, 138, 175};
	const uint16_t numIconPosY = 14;
	uint8_t parts[4];
	const uint8_t limits[] = {90, 70, 40, 10, 0};
	const sint8_t rssis[] = {-60, -70, -80, -90};

	EPDDisplayClear();

	chs12px = getFont(FONT12x12_CN);
	eng16px = getFont(FONT08x16_EN);
	eng12px = getFont(FONT06x12_EN);
	ext16px = getFont(FONT08x16_EXT);

	cursor = integer2String(calendar->year, buffer, sizeof(buffer));
	os_strcpy((buffer + cursor), "年");
	cursor += 3;
	buffer[cursor] = '0' + (calendar->month / 10);
	cursor++;
	buffer[cursor] = '0' + (calendar->month % 10);
	cursor++;
	os_strcpy((buffer + cursor), "月");
	cursor += 3;
	buffer[cursor] = '0' + (calendar->day / 10);
	cursor++;
	buffer[cursor] = '0' + (calendar->day % 10);
	cursor++;
	os_strcpy((buffer + cursor), "日");
	GuiDrawStringUTF8(buffer, 70, 0, chs12px, eng16px);

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
	GuiFillCircle(126, 39, 4, COLOR_BLACK);
	GuiFillCircle(126, 59, 4, COLOR_BLACK);

	// 网络异常提示
	GuiDrawChar(0x12, 20, 82, BLACK, WHITE, ext16px, 14);
	xpos = GuiDrawStringUTF8("天气服务网页解析出现了错误...", 30, 84, chs12px, eng12px);
	GuiDrawChar(0x35, xpos + 2, 82, BLACK, WHITE, ext16px, 14);
	GuiDrawChar(0x36, xpos + 10, 82, BLACK, WHITE, ext16px, 14);

	// 底部状态栏
	GuiDrawBmpImage("home", "bmp", 20, 110);
	// 温度支持范围-9 ~ 99，小于0度只能显示低位, 大于0度显示2位
	buffer[0] = (status->temperature < 0) ? '-' : ('0' + (status->temperature / 10));
	buffer[1] = '0' + (status->temperature % 10);
	os_strcpy((buffer + 2), "'C/");
	// 湿度总是正数 显示范围00~99
	buffer[5] = '0' + (status->humidity / 10);
	buffer[6] = '0' + (status->humidity % 10);
	os_strcpy((buffer + 7), "%RH");
	GuiDrawString(buffer, (20 + 12), 110, chs12px, eng12px);

	xpos = 152;
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

	// 国标特殊字符的信号图标
	GuiDrawChar(0x1F, (xpos), 108, BLACK, WHITE, ext16px, 14);
	if(status->sysopmode == STATION_MODE && status->syspowermode == POWER_NONE_SLEEP) {
		for(i = 0; i < 4; i++) {
			if(status->rssi > rssis[i]) break;
		}
		// 限制范围，信号很差时i可能为4
		i = (i > 3) ? 3 : i;
		// 信号等级icon
		GuiDrawChar(0x20 + (3 - i), (xpos + 8), 108, BLACK, WHITE, ext16px, 14);
	}else {
		GuiDrawChar('X', (xpos + 8), 110, BLACK, WHITE, eng12px, 12);
		if(POWER_MODEM_SLEEP == status->syspowermode) {
			GuiDrawBmpImage("mslp", "bmp", xpos + 16, 110);
		}else if(POWER_LIGHT_SLEEP == status->syspowermode) {
			GuiDrawBmpImage("lslp", "bmp", xpos + 16, 110);
		}
	}
}

