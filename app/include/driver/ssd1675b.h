#ifndef _SSD1675B_H_
#define _SSD1675B_H_

#include "c_types.h"
#include "os_type.h"
#include "gpio.h"
#include "mem.h"
#include "osapi.h"
#include "gpio16.h"
#include "spi_interface.h"
#include "graphics/color.h"
#include "utils/eventbus.h"
#include "utils/eventdef.h"

/**
 * NODE MCU 引脚连接图
 * EPD       NODEMCU
 * CS       D8
 * SCLK     D5
 * MOSI     D7
 * BUSY     D2
 * D/C      D6
 * REST     D4
 * N-MOSFET D1
 * GND      GND
 * 3V3      3V3
 * */

#define HSPI_CS_PIN        (15)
#define HSPI_SCLK_PIN      (14)
#define HSPI_MOSI_PIN      (13)

#define EPD_DC_PIN         (12)
#define EPD_BUSY_PIN       (4)
#define EPD_MOSFET_PIN     (5)
#define EPD_RESET_PIN      (2)

// @see 0d-esp8266_pin_list_release_15-11-2014.xlsx
#define FUNC_HSPI_CS        (2)
#define FUNC_HSPI_SCLK      (2)
#define FUNC_HSPI_MOSI      (2)
#define FUNC_HSPI_MISO      (2)

typedef enum _transfer_type {
	COMMAND_TYPE = 0,
	DATA_TYPE,
	UNKNOWN_TYPE
} TransferType;

#define EPD_WIDTH       122
#define EPD_HEIGHT      250

// epd 显存轴宽度(字节)，最后一字节的高6位无效
#define EPD_RAM_WIDTH    16

// epd刷新超时(显示完成后拉低EPD_BUSY_PIN)时间，正常应在4秒左右
#define EPD_REFRESH_TIMEOUT    10000

//#define USE_SOFT_SPI

/**
 * After this command initiated, the chip will enter Deep Sleep Mode, BUSY pad will keep output high.
 * Remark: To Exit Deep Sleep mode, User required to send HWRESET to the driver
 * */
#define DEEP_SLEEP_MODE_CMD    (0x10)
// DC/DC off, No clock, No output load, No MCU interface access, Retain RAM data but cannot access the RAM (power consume 1uA)
#define DEEP_SLEEP_MODE1       (0x01)
// same as 'DEEP_SLEEP_MODE1' but cannot retain RAM data  (power consume 0.7uA)
#define DEEP_SLEEP_MODE2       (0x03)

#define TEMP_SENSOR_SEL_CMD     (0x18)
#define EXTERNAL_TEMP_SENSOR    (0x48)
#define INTERNAL_TEMP_SENSOR    (0x80)

#define READ_INTERNAL_TEMP_CMD    (0x1B)

extern const uint8_t LUT_FULL_UPDATE[];
extern const uint8_t LUT_PARTIAL_UPDATE[];

STATUS ICACHE_FLASH_ATTR EPDDisplayRAMInit();

uint8_t * ICACHE_FLASH_ATTR EPDGetDisplayRAM();

void ICACHE_FLASH_ATTR EPDDisplayClear();

void ICACHE_FLASH_ATTR EPDGpioSetup();

void ICACHE_FLASH_ATTR EPDReset();

STATUS ICACHE_FLASH_ATTR EPDInit(const uint8_t *lut);

void ICACHE_FLASH_ATTR EPDDrawVertical(uint16_t x, uint16_t y, uint8_t color);
void ICACHE_FLASH_ATTR EPDDrawHorizontal(uint16_t x, uint16_t y, uint8_t color);

STATUS ICACHE_FLASH_ATTR EPDFlush();

STATUS ICACHE_FLASH_ATTR EPDFillData(uint8_t *data);

STATUS ICACHE_FLASH_ATTR EPDTurnOnDisplay();
// STATUS ICACHE_FLASH_ATTR EPDTurnOnDisplayEx();

STATUS ICACHE_FLASH_ATTR EPDGetStatus();

void ICACHE_FLASH_ATTR EPDFillDisplayRAM(uint8_t *ptr, uint16_t offset, uint16_t length);

#endif
