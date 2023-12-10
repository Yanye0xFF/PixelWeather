/*
 * FileName: font.h
 * @brief 字体定义结构体
 * @brief 添加文件字体支持，文件系统依赖spifs
 * @brief 添加每个字体的字节序反转接口endianSwap
 * Created on: Jun 1, 2020
 * Author: Yanye
 */

#ifndef _FONT_H_
#define _FONT_H_

#include "c_types.h"
#include "spifsmini/spifs.h"

typedef uint16_t wchar;

struct _font;
typedef struct _font Font;

/**
 * 字体结构，仅支持双字节编码的字体，兼容内存字体和文件字体
 */
struct _font {
	// 字模数据地址指针，内存字体时指向内存中字模数据，文件字体时为NULL
	uint8_t *data;
	// 字模数据基址，仅对文件字体有效，当一个文件中存在多个字模数据时，使用各字体基址跳转
	uint32_t baseAddr;
	// 计算字模数据偏移量回调
	uint32_t (*calcOffset)(Font *font, wchar charCode);
	// (bitmapline)行字节大小端交换, 解决字库大端方式扫描，显示接口小端方式显示
	uint32_t (*endianSwap)(uint32_t data);
	// 开始字符编码
	wchar startChar;
	// 结束字符编码
	wchar endChar;
	// 字体宽度(pixel)
	uint8_t width;
	// 字体高度(pixel)
	uint8_t height;
	// 逐行扫描, 每行的字节数(byte)
	uint8_t lineBytes;
	// 单个bitmap字体总字节数(byte)，出于结构体对齐添加，实际上根据height * lineBytes可以计算出
	uint8_t fontBytes;
	// 文件字体的文件名，最大8个字符+'\0'
	uint8_t filename[FILENAME_SIZE + 4];
	// 文件字体的拓展名，最大3个字符+'\0'
	uint8_t extname[EXTNAME_SIZE];
	//  字体绘制水平间隔, 默认0
	int16_t hspacing;
	// 字体绘制垂直间隔, 默认0
	int16_t vspacing;
	// 用户定义数据
	void *user_data;
};

typedef enum _font_type {
	// 12x12 点阵 GB2312 字库
	FONT12x12_CN = 0,
	// 16x16 点阵 GB2312 字库
	FONT16x16_CN,
	// 8x16 点阵 ASCII标准字符
	FONT08x16_EN,
	// 6x12 点阵 ASCII标准字符
	FONT06x12_EN,
	// 国标特殊字符内码组成为ACA1~ACDF 共计 64 个字符
	FONT08x16_EXT
} FontType;

void ICACHE_FLASH_ATTR loadFont(void);

Font * ICACHE_FLASH_ATTR getFont(FontType tp);

#endif
