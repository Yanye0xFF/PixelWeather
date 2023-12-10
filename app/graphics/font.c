/*
 * font.c
 * @brief
 * Created on: Mar 9, 2021
 * Author: Yanye
 */

#include "graphics/font.h"

static Font fonts[5];

static uint32_t ICACHE_FLASH_ATTR calcOffset12CN(Font *font, wchar charCode);
static uint32_t ICACHE_FLASH_ATTR calcOffset16CN(Font *font, wchar charCode);
static uint32_t ICACHE_FLASH_ATTR endianSwapCN(uint32_t bitmapline);

static uint32_t ICACHE_FLASH_ATTR calcOffset16EN(Font *font, wchar charCode);
static uint32_t ICACHE_FLASH_ATTR calcOffset12EN(Font *font, wchar charCode);
static uint32_t ICACHE_FLASH_ATTR endianSwapEN(uint32_t bitmapline);

void ICACHE_FLASH_ATTR loadFont(void) {
	os_memset(fonts, 0x00, sizeof(fonts));

	fonts[0].data = NULL;
	fonts[0].user_data = NULL;
	fonts[0].baseAddr = 0;
	fonts[0].calcOffset = calcOffset12CN;
	fonts[0].endianSwap = endianSwapCN;
	fonts[0].startChar = 0xA1A1;
	fonts[0].endChar = 0xFEFE;
	fonts[0].width = 12;
	fonts[0].height = 12;
	fonts[0].lineBytes = 2;
	fonts[0].fontBytes = 24;
	os_strcpy((char *)(fonts[0].filename), "GB231212");
	os_strcpy((char *)(fonts[0].extname), "bin");

	fonts[1].data = NULL;
	fonts[1].user_data = NULL;
	fonts[1].baseAddr = 0;
	fonts[1].calcOffset = calcOffset16CN;
	fonts[1].endianSwap = endianSwapCN;
	fonts[1].startChar = 0xA1A1;
	fonts[1].endChar = 0xFEFE;
	fonts[1].width = 16;
	fonts[1].height = 16;
	fonts[1].lineBytes = 2;
	fonts[1].fontBytes = 32;
	os_strcpy((char *)(fonts[1].filename), "GB231216");
	os_strcpy((char *)(fonts[1].extname), "bin");

	fonts[2].data = NULL;
	fonts[2].user_data = NULL;
	fonts[2].baseAddr = 0;
	fonts[2].calcOffset = calcOffset16EN;
	fonts[2].endianSwap = endianSwapEN;
	fonts[2].startChar = 0x20;
	fonts[2].endChar = 0x7E;
	fonts[2].width = 8;
	// 本应是16 屏幕高度不够改为12
	fonts[2].height = 16;
	fonts[2].lineBytes = 1;
	fonts[2].fontBytes = 16;
	os_strcpy((char *)(fonts[2].filename), "AS08_16");
	os_strcpy((char *)(fonts[2].extname), "bin");

	fonts[3].data = NULL;
	fonts[3].user_data = NULL;
	fonts[3].baseAddr = 0;
	fonts[3].calcOffset = calcOffset12EN;
	fonts[3].endianSwap = endianSwapEN;
	fonts[3].startChar = 0x20;
	fonts[3].endChar = 0x7E;
	fonts[3].width = 6;
	fonts[3].height = 12;
	fonts[3].lineBytes = 1;
	fonts[3].fontBytes = 12;
	os_strcpy((char *)(fonts[3].filename), "AS06_12");
	os_strcpy((char *)(fonts[3].extname), "bin");

	fonts[4].data = NULL;
	fonts[4].user_data = NULL;
	fonts[4].baseAddr = 0;
	fonts[4].calcOffset = calcOffset16EN;
	fonts[4].endianSwap = endianSwapEN;
	fonts[4].startChar = 0x00;
	fonts[4].endChar = 0x40;
	fonts[4].width = 8;
	// fonts[3].height = 16 屏幕区域不够，高度裁掉2像素 = 14
	fonts[4].height = 16;
	fonts[4].lineBytes = 1;
	fonts[4].fontBytes = 16;
	os_strcpy((char *)(fonts[4].filename), "GB64SP");
	os_strcpy((char *)(fonts[4].extname), "bin");
}


Font * ICACHE_FLASH_ATTR getFont(FontType tp) {
	uint32_t cursor = (uint32_t)tp;
	return (fonts + cursor);
}

static uint32_t ICACHE_FLASH_ATTR calcOffset12CN(Font *font, wchar charCode) {
    // 默认的gb2312编码大端模式， 区码在低字节位码在高字节
    uint8_t LSB = (charCode >> 8) & 0xFF;
    uint8_t MSB  = charCode & 0xFF;
    if(MSB >= 0xA1 && MSB <= 0xA9 && LSB >= 0xA1) {
    	// GB2312 非汉字符号区。即 GBK/1: A1A1-A9FE
        return ((MSB - 0xA1) * 94 + (LSB - 0xA1)) * 24;
    }else if(MSB >= 0xB0 && MSB <= 0xF7 && LSB >= 0xA1) {
    	// GB2312 汉字区。即 GBK/2: B0A1-F7FE
        return ((MSB - 0xB0) * 94 + (LSB - 0xA1) + 846) * 24;
    }else {
        return 0;
    }
}

static uint32_t ICACHE_FLASH_ATTR calcOffset16CN(Font *font, wchar charCode) {
    // 默认的gb2312编码大端模式， 区码在低字节位码在高字节
    uint8_t LSB = (charCode >> 8) & 0xFF;
    uint8_t MSB  = charCode & 0xFF;
    if(MSB >= 0xA1 && MSB <= 0xA9 && LSB >= 0xA1) {
    	// GB2312 非汉字符号区。即 GBK/1: A1A1-A9FE
        return ((MSB - 0xA1) * 94 + (LSB - 0xA1)) * 32;
    }else if(MSB >= 0xB0 && MSB <= 0xF7 && LSB >= 0xA1) {
    	// GB2312 汉字区。即 GBK/2: B0A1-F7FE
        return ((MSB - 0xB0) * 94 + (LSB - 0xA1) + 846) * 32;
    }else {
        return 0;
    }
}

/**
 * @brief 交换字体扫描线的字节序，最大支持行宽为32的字体
 * @note 由于交换了双字节，因此兼容12*12和16*16字体
 * @param bitmapline 字体行扫描线数据
 * @return 字节序交换后的行扫描线(当前之交换了最低2字节)
 * */
static uint32_t ICACHE_FLASH_ATTR endianSwapCN(uint32_t bitmapline) {
	 uint8_t heightpart, lowpart;
	 heightpart = bitmapline & 0xFF;
	 lowpart = (bitmapline >> 8) & 0xFF;
	 bitmapline = (heightpart << 8) | lowpart;
	 return bitmapline;
}

static uint32_t ICACHE_FLASH_ATTR calcOffset16EN(Font *font, wchar charCode) {
	return (charCode - font->startChar) * 16;
}

static uint32_t ICACHE_FLASH_ATTR endianSwapEN(uint32_t bitmapline) {
	return (bitmapline );
}

static uint32_t ICACHE_FLASH_ATTR calcOffset12EN(Font *font, wchar charCode) {
	return (charCode - font->startChar) * 12;
}
