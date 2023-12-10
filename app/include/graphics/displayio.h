/*
 * FileName: displayio.h
 * @brief 
 * Created on: Jun 1, 2020
 * Author: Yanye
 */

#ifndef _DISPLAYIO_H_
#define _DISPLAYIO_H_

#include "c_types.h"
#include "osapi.h"
#include "mem.h"
#include "font.h"

#include "driver/ssd1675b.h"
#include "graphics/color.h"
#include "graphics/font.h"
#include "spifsmini/spifs.h"
#include "utils/strings.h"

// 24*24
#define FONTBITMAP_BUFFER_SIZE    72

#define SCREEN_WIDTH    250
#define SCREEN_HEIGHT   122

// start of bmp-dib header
// 此字段的值总为‘BM’
#define BITMAP_DIB          0x4D42
// BI_RGB无压缩类型
#define BI_RGB              0
// 单色
#define MONOCHROME          0x1

#define BFTYPE              0
#define BFTYPE_LENGTH       2

#define BFOFFBITS           10
#define BFOFFBITS_LENGTH    4

#define BIBITCOUNT          28
#define BIBITCOUNT_LENGTH   2

#define BICOMPRESSION            30
#define BICOMPRESSION_LENGTH     4

#define BIWIDTH            18
#define BIWIDTH_LENGTH     4

#define BIHEIGHT           22
#define BIHEIGHT_LENGTH    4
// end of bmp-dib header

// start of bmp-packed header
#define BITMAP_PACKED     0xFEFD
#define PACKED_HEADER_LENGTH    2

#define PACKED_SIZE_OFFSET    2
#define PACKED_SIZE_LENGTH    4

#define PACKED_WIDTH_OFFSET    6
#define PACKED_WIDTH_LENGTH    2

#define PACKED_HEIGHT_OFFSET    8
#define PACKED_HEIGHT_LENGTH    2

#define PACKED_DATA_OFFSET    10
// end of bmp-packed header

/**
 * @brief 底层绘制api，调用屏幕驱动
 * */
void ICACHE_FLASH_ATTR GuiDrawPixel(uint16_t x, uint16_t y, uint8_t color);
void ICACHE_FLASH_ATTR GuiFillColor(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd, uint8_t color);

/**
 * @brief 高级图形api
 * */
void ICACHE_FLASH_ATTR GuiDrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c );
void ICACHE_FLASH_ATTR GuiDrawDashLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c );
void ICACHE_FLASH_ATTR GuiDrawRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c );

void ICACHE_FLASH_ATTR GuiDrawArc(int16_t x0, int16_t y0, int16_t r, uint8_t s, uint8_t color);
void ICACHE_FLASH_ATTR GuiDrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c);
void ICACHE_FLASH_ATTR GuiFillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c);

void ICACHE_FLASH_ATTR GuiDrawRoundRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t c);
void ICACHE_FLASH_ATTR GuiFillRoundRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t c);
void ICACHE_FLASH_ATTR GuiDrawMesh(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t gridSize, uint8_t c);

void ICACHE_FLASH_ATTR GuiDrawChar(wchar ch, uint16_t x, uint16_t y, Color foreground, Color background, Font *font, uint16_t height);
uint16_t ICACHE_FLASH_ATTR GuiDrawString(uint8_t *str, uint16_t xStart, uint16_t yStart, Font *font, Font *engFont);
uint16_t ICACHE_FLASH_ATTR GuiDrawStringUTF8(uint8_t *str, uint16_t xStart, uint16_t yStart, Font *font, Font *engFont);

void ICACHE_FLASH_ATTR GuiDrawBmpImageImpl(File *file, uint16_t xStart, uint16_t yStart);
void ICACHE_FLASH_ATTR GuiDrawBmpImage(char *fileName, char *extName, uint16_t xStart, uint16_t yStart);
void ICACHE_FLASH_ATTR GuiDrawImagePacked(const uint8_t *data, uint16_t xStart, uint16_t yStart);

//void ICACHE_FLASH_ATTR GuiDrawBmpRaw(const uint8_t *data, uint16_t xStart, uint16_t yStart);

uint8_t ICACHE_FLASH_ATTR GuiCheckBMPFormat(File *file, uint32_t *width, uint32_t *height, uint32_t *dataOffset);

#endif
