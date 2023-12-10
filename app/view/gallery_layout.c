/*
 * gallery_layout.c
 * @brief
 * Created on: Oct 28, 2021
 * Author: Yanye
 */

#include "view/gallery_layout.h"

#include "utils/fixed_file.h"
#include "utils/rtc_mem.h"
#include "graphics/displayio.h"

// private
#define IMAGE_FILENAME_INDEX       1
#define IMAGE_EXTNAME_INDEX        9
#define HORIZONTAL_ALIGN_INDEX     13
#define VERTICAL_ALIGN_INDEX       14

void ICACHE_FLASH_ATTR invalidateGalleryView(void) {
	Font *chs16px, *eng16px;

	fixed_file_t fixedFile;
	uint8_t buffer[16];
	uint32_t err = 0;

	File imageFile;
	uint32_t width, height, top, left;
	uint8_t halign, valign;

	chs16px = getFont(FONT16x16_CN);
	eng16px = getFont(FONT08x16_EN);

	EPDDisplayClear();

	fixed_file_init(&fixedFile, FILE_IMAGE_SECTION_SIZE, FILE_IMAGE_MAX_SIZE);

	do {
		if(fixed_file_open(&fixedFile, "image", "ini") == FALSE) {
			err = 1;
			break;
		}
		if(fixed_file_read(&fixedFile, buffer, FILE_IMAGE_SECTION_SIZE) != FILE_IMAGE_SECTION_SIZE) {
			err = 1;
			break;
		}
		if(open_file_raw(&imageFile, (buffer + IMAGE_FILENAME_INDEX), (buffer + IMAGE_EXTNAME_INDEX)) == FALSE) {
			err = 2;
			break;
		}
	}while(0);
	// 错误提示信息
	if(err == 1) {
		chs16px->vspacing = 3;
		GuiDrawStringUTF8("》未设置显示的图片《", 40, 16, chs16px, eng16px);
		GuiDrawStringUTF8("请打开“天气站”APP，在“页面管理”->“图片显示”中选择图片。", 0, 42, chs16px, eng16px);
		chs16px->vspacing = 0;
		return;
	}else if(err == 2) {
		chs16px->vspacing = 3;
		GuiDrawStringUTF8("无法打开设定的图片", 40, 16, chs16px, eng16px);
		GuiDrawStringUTF8("可能图片文件已被删除，请使用“天气站”APP重新设置。", 0, 42, chs16px, eng16px);
		chs16px->vspacing = 0;
		return;
	}
	// 检查bmp文件格式，并获取宽/高信息
	if(GuiCheckBMPFormat(&imageFile, &width, &height, NULL) != 0) {
		return;
	}
	halign = buffer[HORIZONTAL_ALIGN_INDEX];
	valign = buffer[VERTICAL_ALIGN_INDEX];
	// 根据对齐方式计算绘制图片的top-left
	// 水平
	if(IMAGE_ALIGN_START == halign) {
		left = 0;
	}else if(IMAGE_ALIGN_CENTER == halign) {
		left = (SCREEN_WIDTH / 2) - (width / 2);
	}else {
		// IMAGE_ALIGN_END
		left = (SCREEN_WIDTH - width);
	}
	// 垂直
	if(IMAGE_ALIGN_START == valign) {
		top = 0;
	}else if(IMAGE_ALIGN_CENTER == valign) {
		top = (SCREEN_HEIGHT / 2) - (height / 2);
	}else {
		// IMAGE_ALIGN_END
		top = (SCREEN_HEIGHT - height);
	}

	GuiDrawBmpImageImpl(&imageFile, left, top);
}
