/*
 * FileName: displayio.c
 * @brief 
 * Created on: Jun 1, 2020
 * Author: Yanye
 */

#include "graphics/displayio.h"

static BOOL ICACHE_FLASH_ATTR loadFontBitmap(uint8_t *buffer, wchar ch, Font *font);

/**
 * @breif 在屏幕上绘制一像素
 * @param x x坐标
 * @param y y坐标
 * @param color RGB565颜色
 * */
void ICACHE_FLASH_ATTR GuiDrawPixel(uint16_t x, uint16_t y, uint8_t color) {
	EPDDrawHorizontal(x, y, color);
}

/**
 * @breif 使用特定颜色填充屏幕指定区域
 * @param xStart x左上坐标
 * @param yStart y左上坐标
 * @param xEnd y右下坐标
 * @param yEnd y右下坐标
 * @param color RGB565颜色
 * */
void ICACHE_FLASH_ATTR GuiFillColor(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd, uint8_t color) {
	uint16_t i, j;
    for(i = yStart; i <= yEnd; i++) {
        for(j = xStart; j <= xEnd; j++) {
        	EPDDrawHorizontal(j, i, color);
        }
    }
}

/**
 * @brief 绘制直线段, 使用bresenham算法绘制
 * @brief 两点距离8 直线xy = 6 节点
 * */
void ICACHE_FLASH_ATTR GuiDrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color) {
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    int16_t dxabs = (dx > 0) ? dx : -dx;
    int16_t dyabs = (dy > 0) ? dy : -dy;
    int16_t sgndx = (dx > 0) ? 1  : -1;
    int16_t sgndy = (dy > 0) ? 1 : -1;
    int16_t x = dyabs >> 1;
    int16_t y = dxabs >> 1;
    int16_t drawx = x1;
    int16_t drawy = y1;
    int16_t n = 0;

    GuiDrawPixel(drawx, drawy, color);

    if(dxabs >= dyabs) {
        for(n = 0; n < dxabs; n++) {
            y += dyabs;
            if(y >= dxabs) {
                y -= dxabs;
                drawy += sgndy;
            }
            drawx += sgndx;
            GuiDrawPixel(drawx, drawy, color);
        }
    }else {
        for(n = 0; n < dyabs; n++) {
            x += dxabs;
            if(x >= dyabs) {
                x -= dyabs;
                drawx += sgndx;
            }
            drawy += sgndy;
            GuiDrawPixel(drawx, drawy, color);
        }
    }
}

/**
 * @brief 绘制2像素，空6像素...
 * @note y1==y2时，x2必须大于x1，且为正整数
 *       x1==x2时，y2必须大于y1，且为正整数
 * */
void ICACHE_FLASH_ATTR GuiDrawDashLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c ) {
	if(y1 == y2) {
		// 水平虚线
		while(x1 < x2) {
			if((x1 + 1) > x2) break;

			GuiDrawPixel(x1, y1, c);
			GuiDrawPixel(x1 + 1, y1, c);

			x1 += 2;

			if((x1 + 5) > x2) break;

			x1 += 6;
		}
	}else {
		// 垂直虚线
		while(y1 < y2) {
			if((y1 + 1) > y2) break;

			GuiDrawPixel(x1, y1, c);
			GuiDrawPixel(x1, y1 + 1, c);

			y1 += 2;

			if((y1 + 5) > y2) break;

			y1 += 6;
		}
	}
}

/**
 * @brief 绘制矩形
 * */
void ICACHE_FLASH_ATTR GuiDrawRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
	uint16_t temp;
	if(x1 > x2) {
	    temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if(y1 > y2) {
	    temp = y1;
		y1 = y2;
		y2 = temp;
	}

    GuiFillColor(x1, y1, x2, y1, c);
    GuiFillColor(x1, y2, x2, y2, c);

    GuiFillColor(x1, y1, x1, y2, c);
    GuiFillColor(x2, y1, x2, y2, c);
}

/**
 * @brief 绘制圆弧
 * @param x0 圆心x
 * @param y0 圆心y
 * @param r 圆弧半径
 * @param s 弧长
 * */
void ICACHE_FLASH_ATTR GuiDrawArc(int16_t x0, int16_t y0, int16_t r, uint8_t s, uint8_t color) {

    if ( x0 < 0 ) return;
    if ( y0 < 0 ) return;
    if ( r <= 0 ) return;

    int16_t xd = 1 - (r << 1);
    int16_t yd = 0, e = 0, x = r,  y = 0;

    while ( x >= y ) {
        // Q1
        if ( s & 0x01 ) GuiDrawPixel(x0 + x, y0 - y, color);
        if ( s & 0x02 ) GuiDrawPixel(x0 + y, y0 - x, color);
        // Q2
        if ( s & 0x04 ) GuiDrawPixel(x0 - y, y0 - x, color);
        if ( s & 0x08 ) GuiDrawPixel(x0 - x, y0 - y, color);
        // Q3
        if ( s & 0x10 ) GuiDrawPixel(x0 - x, y0 + y, color);
        if ( s & 0x20 ) GuiDrawPixel(x0 - y, y0 + x, color);
        // Q4
        if ( s & 0x40 ) GuiDrawPixel(x0 + y, y0 + x, color);
        if ( s & 0x80 ) GuiDrawPixel(x0 + x, y0 + y, color);
        y++;
        e += yd;
        yd += 2;
        if(((e << 1) + xd) > 0 ) {
            x--;
            e += xd;
            xd += 2;
        }
    }
}

/**
 * @brief 绘制圆
 * @param x0 圆心x
 * @param y0 圆心y
 * */
void ICACHE_FLASH_ATTR GuiDrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
    if (x0 < 0) return;
    if (y0 < 0) return;
    if (r <= 0) return;

    int16_t xd = 1 - (r << 1);
    int16_t yd = 0, e = 0, x = r, y = 0;

    while (x >= y) {
        GuiDrawPixel(x0 - x, y0 + y, c);
        GuiDrawPixel(x0 - x, y0 - y, c);
        GuiDrawPixel(x0 + x, y0 + y, c);
        GuiDrawPixel(x0 + x, y0 - y, c);
        GuiDrawPixel(x0 - y, y0 + x, c);
        GuiDrawPixel(x0 - y, y0 - x, c);
        GuiDrawPixel(x0 + y, y0 + x, c);
        GuiDrawPixel(x0 + y, y0 - x, c);
        y++;
        e += yd;
        yd += 2;
        if (((e << 1) + xd) > 0) {
            x--;
            e += xd;
            xd += 2;
        }
    }
}

void ICACHE_FLASH_ATTR GuiFillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {

    if ( x0 < 0 ) return;
    if ( y0 < 0 ) return;
    if ( r <= 0 ) return;

    int16_t xd = 3 - (r << 1);
    int16_t x = 0, y = r;

    while ( x <= y ) {
        if( y > 0 ) {
            GuiFillColor(x0 - x, y0 - y,x0 - x, y0 + y, c);
            GuiFillColor(x0 + x, y0 - y,x0 + x, y0 + y, c);
        }
        if( x > 0 ) {
            GuiFillColor(x0 - y, y0 - x,x0 - y, y0 + x, c);
            GuiFillColor(x0 + y, y0 - x,x0 + y, y0 + x, c);
        }
        if ( xd < 0 ) {
            xd += (x << 2) + 6;
        }else {
            xd += ((x - y) << 2) + 10;
            y--;
        }
        x++;
    }
    GuiDrawCircle(x0, y0, r,c);
}

void ICACHE_FLASH_ATTR GuiDrawRoundRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t c) {
    int16_t n;
    if(x2 < x1) {
        n = x2;
        x2 = x1;
        x1 = n;
    }
    if (y2 < y1) {
        n = y2;
        y2 = y1;
        y1 = n;
    }

    if(r > x2) return;
    if(r > y2) return;

    GuiFillColor(x1 + r, y1, x2 - r, y1, c);
    GuiFillColor(x1 + r, y2, x2 - r, y2, c);
    GuiFillColor(x1, y1 + r, x1, y2 - r, c);
    GuiFillColor(x2, y1 + r, x2, y2 - r, c);
    GuiDrawArc(x1+r, y1+r, r, 0x0C, c);
    GuiDrawArc(x2-r, y1+r, r, 0x03, c);
    GuiDrawArc(x1+r, y2-r, r, 0x30, c);
    GuiDrawArc(x2-r, y2-r, r, 0xC0, c);
}

void ICACHE_FLASH_ATTR GuiFillRoundRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t c) {
    int16_t  x, y, xd;

    if ( x2 < x1 ) {
        x = x2;
        x2 = x1;
        x1 = x;
    }
    if ( y2 < y1 ) {
        y = y2;
        y2 = y1;
        y1 = y;
    }

    if (r <= 0) return;

    xd = 3 - (r << 1);
    x = 0;
    y = r;

    GuiFillColor(x1 + r, y1, x2 - r, y2, c);

    while ( x <= y ) {
        if( y > 0 ) {
            GuiFillColor(x2 + x - r, y1 - y + r, x2 + x - r, y + y2 - r, c);
            GuiFillColor(x1 - x + r, y1 - y + r, x1 - x + r, y + y2 - r, c);
        }
        if( x > 0 ) {
            GuiFillColor(x1 - y + r, y1 - x + r, x1 - y + r, x + y2 - r, c);
            GuiFillColor(x2 + y - r, y1 - x + r, x2 + y - r, x + y2 - r, c);
        }
        if ( xd < 0 ) {
            xd += (x << 2) + 6;
        }else {
            xd += ((x - y) << 2) + 10;
            y--;
        }
        x++;
    }
}

/**
 * @brief 绘制格点区域
 * */
void ICACHE_FLASH_ATTR GuiDrawMesh(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t gridSize, uint8_t c) {
   int16_t n,m;
   if(x2 < x1) {
      n = x2;
      x2 = x1;
      x1 = n;
   }
   if(y2 < y1) {
      n = y2;
      y2 = y1;
      y1 = n;
   }

   for(m = y1; m <= y2; m += gridSize) {
      for(n = x1; n <= x2; n += gridSize) {
    	  EPDDrawHorizontal(n, m, c);
      }
   }
}



/**
 * @brief 读取单个字体模型数据
 * @param *buffer 数据接收缓冲区
 * @param ch 字符
 * @param *font 字体
 * @return TRUE:读取成功, FALSE:读取失败
 * */
static BOOL ICACHE_FLASH_ATTR loadFontBitmap(uint8_t *buffer, wchar ch, Font *font) {
    File *file = (File *)font->user_data;
    uint32_t offset = font->baseAddr;
    offset += font->calcOffset(font, ch);
    if(font->data == NULL && file != NULL) {
    	return read_file(file, offset, buffer, (font->lineBytes * font->height));
    }
    return TRUE;
}

/**
 * @brief 绘制bitmap字体
 * @param ch 宽字符uint16_t
 * @param x 字体左顶点位置
 * @param y 字体上顶点位置
 * @param foreground 前景色
 * @param background 背景色
 * @param font 字体文件
 * @param height 字体实际绘制的高度<=font->height
 * */
void ICACHE_FLASH_ATTR GuiDrawChar(wchar ch, uint16_t x, uint16_t y, Color foreground, Color background, Font *font, uint16_t height) {
	uint32_t j, i;
	uint32_t bitmapline;
    // 最大支持单个字模 72字节数据
    uint8_t buffer[FONTBITMAP_BUFFER_SIZE];
    File file;
    // 字体范围检查
    if((ch < font->startChar) || (ch > font->endChar)) {
    	return;
    }
    // 超出大小的字模不绘制
    if(font->lineBytes > sizeof(uint32_t) || font->fontBytes > FONTBITMAP_BUFFER_SIZE) {
    	return;
    }
    // 对于直接调用GuiDrawChar绘制单个字符时，需要先打开字体文件
    if(font->user_data == NULL) {
    	if(!open_file(&file, (char *)font->filename, (char *)font->extname)) {
    		return ;
    	}
    	font->user_data = &file;
    }

    if(!loadFontBitmap(buffer, ch, font)) {
    	// 读取字模数据失败时以foregroundColor填充
    	os_memset(buffer, 0x55, (sizeof(uint8_t) * FONTBITMAP_BUFFER_SIZE));
    }
    // 字模绘制 // font->height
    for(j = 0; j < height; j++) {
    	os_memcpy(&bitmapline, (buffer + j * font->lineBytes), font->lineBytes);
    	//字模为大端模式，需要对所使用字模字节序反转
    	bitmapline = font->endianSwap(bitmapline);
        for(i = 1; i <= font->width; i++) {
        	// (font->width - i)移位操作需要从(font->width - 1) ~ 0
        	// font为字节正序即大端模式
            if((bitmapline >> (font->lineBytes * 8 - i)) & 0x1) {
            	EPDDrawHorizontal(x + i, y + j, foreground);
            }else {
            	EPDDrawHorizontal(x + i, y + j, background);
            }
        }
    }

    font->user_data = NULL;
}

/**
 * @brief 绘制字符串
 * @param *str 字符串，GB2312码，兼容ASCII码
 * @param xStart 左顶点位置
 * @param yStart 上顶点位置
 * @param charset 字符编码
 * @param *font 中文字体
 * @param *engFont 英文字体
 * @return x 最后一个字符结束的x轴坐标
 * */
uint16_t ICACHE_FLASH_ATTR GuiDrawString(uint8_t *str, uint16_t xStart, uint16_t yStart, Font *font, Font *engFont) {
	uint16_t x = xStart, y = yStart;
	uint8_t width;
	wchar ch;
	File fileCN, fileEN;

	if(!open_file(&fileCN, (char *)font->filename, (char *)font->extname)) {
		return xStart;
	}
	if(!open_file(&fileEN, (char *)engFont->filename, (char *)engFont->extname)) {
		return xStart;
	}

	font->user_data = &fileCN;
	engFont->user_data = &fileEN;

    while(*str != '\0') {
    	width = (*str >= 0x20 && *str <= 0x7E) ? engFont->width : font->width;
    	// 超出当前行宽自动换行
        if(x > (SCREEN_WIDTH - width)) {
            x = xStart;
			y += (font->height + font->vspacing);
            if(y > (SCREEN_HEIGHT - font->height)) {
            	font->user_data = NULL;
            	engFont->user_data = NULL;
            	return x;
            }
        }
        if(*str >= 0x20 && *str <= 0x7E) {
        	ch = *str & 0xFF;
        	str += sizeof(char);
        	if(ch == '\n') {
				// \n换行, 以中文字体为基准
				x = xStart;
				y += (font->height + font->vspacing);
				if(y > (SCREEN_HEIGHT - font->height)) {
					font->user_data = NULL;
					engFont->user_data = NULL;
					return x;
				}
				continue;
			}
        	// 中文和英文一起绘制时，限制英文字体的高度为中文字体的高度
        	GuiDrawChar(ch, x, y, BLACK, WHITE, engFont, font->height);
        	x += engFont->width;
        }else {
        	os_memcpy(&ch, str, sizeof(wchar));
			str += sizeof(wchar);
			GuiDrawChar(ch, x, y, BLACK, WHITE, font, font->height);
			x += font->width;
        }
    }
    font->user_data = NULL;
	engFont->user_data = NULL;
    return x;
}

/**
 * @brief 绘制字符串，不兼容\n，不带自动换行
 * @param *str 字符串，UTF8码(UTF8兼容ASCII)
 * @param xStart 左顶点位置
 * @param yStart 上顶点位置
 * @param charset 字符编码
 * @param *font 中文字体
 * @param *engFont 英文字体
 * @return x 最后一个字符结束的x轴坐标
 * */
uint16_t ICACHE_FLASH_ATTR GuiDrawStringUTF8(uint8_t *str, uint16_t xStart, uint16_t yStart, Font *font, Font *engFont) {
	Short code;
	wchar ch;
	uint8_t length, width;
	uint32_t offset = 0, usc4 = 0;
	uint16_t utf16[2];
	uint16_t x = xStart, y = yStart;
	File fileCN, fileEN;

	if(!open_file(&fileCN, (char *)font->filename, (char *)font->extname)) {
		return xStart;
	}
	if(!open_file(&fileEN, (char *)engFont->filename, (char *)engFont->extname)) {
		return xStart;
	}

	font->user_data = &fileCN;
	engFont->user_data = &fileEN;

	while(*(str + offset) != '\0') {
		// utf8转usc4
		length = UTF8ToUnicode((str + offset), &usc4);
		offset += length;
    	width = (length == 1) ? engFont->width : font->width;
		if(x > (SCREEN_WIDTH - width)) {
			x = xStart;
			y += (font->height + font->vspacing);
			if(y > (SCREEN_HEIGHT - font->height)) {
				font->user_data = NULL;
				engFont->user_data = NULL;
				return x;
			}
		}

		if(length == 0) {
			// utf8超范围
			break;
		}else if(length == 1) {
			// 单字节 按照ascii码解析
			ch = (wchar)(usc4 & 0xFF);
			if(ch == '\n') {
				// \n换行, 以中文字体为基准
				x = xStart;
				y += (font->height + font->vspacing);
				if(y > (SCREEN_HEIGHT - font->height)) {
					font->user_data = NULL;
					engFont->user_data = NULL;
					return x;
				}
				continue;
			}
			// 限制英文字高为中文高度
        	GuiDrawChar(ch, x, y, BLACK, WHITE, engFont, font->height);
        	x += engFont->width;
		}else {
			// 多字节按照unicode码解析, usc4转usc2
			length = UnicodeToUTF16(usc4, utf16);
			if(length == 1) {
				// usc2转gb2312
				code.value = utf16[0];
				code = QueryGB2312ByUnicode(code.value);
				ch = (wchar)((code.bytes[0] << 8) | code.bytes[1]);
				GuiDrawChar(ch, x, y, BLACK, WHITE, font, font->height);

				x += font->width;
			}
		}
	}
	font->user_data = NULL;
	engFont->user_data = NULL;
	return x;
}

/**
 * @brief 绘制bmp格式图片，仅支持单色1bit图片
 * @param *fileName 文件名
 * @param *extName 拓展名,仅支持bmp-dib
 * @param xStart 图片左边起始位置
 * @param yStart 图片上边起始位置
 * */
void ICACHE_FLASH_ATTR GuiDrawBmpImage(char *fileName, char *extName, uint16_t xStart, uint16_t yStart) {
	File file;
	if(!open_file(&file, fileName, extName)) {
		return;
	}
    GuiDrawBmpImageImpl(&file, xStart, yStart);
}

/**
 * @brief 绘制bmp格式图片实现，仅支持单色1bit图片
 * @param *file 打开的bmp文件
 * @param xStart 图片左边起始位置
 * @param yStart 图片上边起始位置
 * */
void ICACHE_FLASH_ATTR GuiDrawBmpImageImpl(File *file, uint16_t xStart, uint16_t yStart) {
    uint32_t temp = 0;
    int32_t i, j;
    uint16_t drawX, drawY = yStart, byteIndex;
    uint32_t width, height, dataOffset;
	// 行缓存, 字节对齐 250 / 8 + 1
    uint8_t lineBuffer[32];
    uint8_t bitValue;

    if(GuiCheckBMPFormat(file, &width, &height, &dataOffset) != 0) {
    	return;
    }
    if((xStart + width > SCREEN_WIDTH) || (yStart + height) > SCREEN_HEIGHT) {
		return;
    }
    temp = width / (sizeof(uint32_t) * BITS_OF_BYTE);
    // 图片行数据4字节对齐
    temp = ((temp * sizeof(uint32_t) * BITS_OF_BYTE) < width) ? (temp + 1) : temp;
    // temp = ((width % i) != 0) ? (temp + 1) : temp;
    temp <<= 2;
	// 像素填充
	for(i = (height - 1); i >= 0; i--) {
		drawX = xStart;
		// 读入一行数据，其中包括填充的4字节对齐部分
    	read_file(file, (dataOffset + (temp * i )), lineBuffer, temp);
    	for(j = 0; j < width; j++) {
    		// windows bmp扫描方式是按从左到右、从下到上
    		byteIndex = (j / 8);
    		bitValue = ((lineBuffer[byteIndex] >> (7 - (j - byteIndex * 8))) & 0x1);
    		EPDDrawHorizontal(drawX++, drawY, bitValue);
    	}
    	drawY++;
    }
}

/**
 * @brief 检查BMP格式是否受支持
 * @param *file 打开的文件指针(open_file/open_file_raw)
 * @param *width 用于接收图片宽度，只有在返回值为0有效，可以为NULL
 * @param *height 用于接收图片高度，只有在返回值为0有效，可以为NULL
 * @param *dataOffset 用于接收图片数据区偏移量，只有在返回值为0有效，可以为NULL
 * @return 0:格式检查通过, 1:格式错误, 2:尺寸超范围
 * */
uint8_t ICACHE_FLASH_ATTR GuiCheckBMPFormat(File *file, uint32_t *width, uint32_t *height, uint32_t *dataOffset) {
    uint32_t temp = 0;
    // 只支持Windows 3.1x, 95, NT... DIB(device-independent bitmap)格式
	read_file(file, BFTYPE, (uint8_t *)&temp, BFTYPE_LENGTH);
    if(BITMAP_DIB != temp) {
        return 1;
    }
    // biBitCount 只支持单色位图
    read_file(file, BIBITCOUNT, (uint8_t *)&temp, BIBITCOUNT_LENGTH);
    if(MONOCHROME != temp) {
        return 1;
    }
    // 只支持BI_RGB无压缩类型
    read_file(file, BICOMPRESSION, (uint8_t *)&temp, BICOMPRESSION_LENGTH);
    if(BI_RGB != temp) {
        return 1;
    }
    // 图片大小不可超出屏幕尺寸
    read_file(file, BIWIDTH, (uint8_t *)&temp, BIWIDTH_LENGTH);
    if(temp > SCREEN_WIDTH) {
    	return 2;
    }
    if(width != NULL) {
    	os_memcpy(width, &temp, sizeof(uint32_t));
    }
    read_file(file, BIHEIGHT, (uint8_t *)&temp, BIHEIGHT_LENGTH);
    if(temp > SCREEN_HEIGHT) {
    	return 2;
    }
    if(height != NULL) {
    	os_memcpy(height, &temp, sizeof(uint32_t));
    }
    if(dataOffset != NULL) {
        read_file(file, BFOFFBITS, (uint8_t *)&temp, BFOFFBITS_LENGTH);
    	os_memcpy(dataOffset, &temp, sizeof(uint32_t));
    }
    return 0;
}

/**
 * @brief 绘制存储在代码中的图片(压缩格式), 图片宽度1字节(8像素)对齐，高度不得超过屏幕高度
 * @param *data 压缩后的图片数据
 * @param xStart 图片左边起始位置
 * @param yStart 图片上边起始位置
 * */
void ICACHE_FLASH_ATTR GuiDrawImagePacked(const uint8_t *data, uint16_t xStart, uint16_t yStart) {
	uint16_t header, width, fileWidth, height;
	uint32_t fileSize, cursor, i, toWrite;
	uint16_t drawX, drawY;
	uint8_t byteValue, bitValue, count;
	// 行缓存, 字节对齐 250 / 8 + 1
	uint8_t lineBuffer[32];
	uint32_t lineCursor, lineOffset;

	os_memcpy(&header, data, PACKED_HEADER_LENGTH);
	if(header != BITMAP_PACKED) {
		return;
	}

	os_memcpy(&fileSize, (data + PACKED_SIZE_OFFSET), PACKED_SIZE_LENGTH);
	os_memcpy(&width, (data + PACKED_WIDTH_OFFSET), PACKED_WIDTH_LENGTH);
	os_memcpy(&height, (data + PACKED_HEIGHT_OFFSET), PACKED_HEIGHT_LENGTH);

    // 图片大小不可超出屏幕尺寸
    if((xStart + width > SCREEN_WIDTH) || (yStart + height) > SCREEN_HEIGHT) {
		return;
    }

    // 存储结构为字节对齐，8像素对齐
    fileWidth = (width / BITS_OF_BYTE);
    fileWidth = (fileWidth * BITS_OF_BYTE < width) ? (fileWidth + 1) : fileWidth;

	fileSize -= PACKED_DATA_OFFSET;
	cursor = PACKED_DATA_OFFSET;
	drawY = yStart;
	lineCursor = 0;

	while(cursor < fileSize) {
		byteValue = *(data + cursor);

		if((byteValue == 0x00) || (byteValue == 0xFF)) {
			// read compressed data length
			count = *(data + cursor + 1);
			if(count == 0x00) {
				// more than 255 bytes
				os_memcpy(&count, (data + cursor + 2), sizeof(uint16_t));
				cursor += 4;
			}else {
				// less than 255 bytes
				cursor += 2;
			}

			do{
				toWrite = (count > (fileWidth - lineCursor)) ? (fileWidth - lineCursor) : (count);
				os_memset((lineBuffer + lineCursor), byteValue, toWrite);
				lineCursor += toWrite;
				count -= toWrite;
				// 行缓存满
				if(lineCursor >= fileWidth) {
					drawX = xStart;
					// 只绘制图片实际宽度的内容
					for(i = 0; i < width; i++) {
						lineCursor = (i >> 3);
						lineOffset = (i - (lineCursor << 3));
						bitValue = (lineBuffer[lineCursor] >> lineOffset & 0x1);
						EPDDrawHorizontal(drawX++, drawY, bitValue);
					}
					drawY++;
					lineCursor = 0;
				}
			}while(count > 0);

		}else {
			// 未压缩的部分直接放入行缓存
			cursor++;
			lineBuffer[lineCursor++] = byteValue;
			if(lineCursor >= fileWidth) {
				drawX = xStart;
				for(i = 0; i < width; i++) {
					lineCursor = (i >> 3);
					lineOffset = (i - (lineCursor << 3));
					bitValue = (lineBuffer[lineCursor] >> lineOffset & 0x1);
					EPDDrawHorizontal(drawX++, drawY, bitValue);
				}
				drawY++;
				lineCursor = 0;
			}
		}
	}
}
