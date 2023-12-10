/*
 * ssd1765b.c
 *  Created on: Aug 13, 2020
 *  Author: Yanye
 */

#include "driver/ssd1675b.h"

// 全局刷新
const uint8_t LUT_FULL_UPDATE[] = {
    0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 局部刷新
const uint8_t LUT_PARTIAL_UPDATE[] = {
    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
//	    0x18, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	    0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void SoftSpiSendByte(uint8_t byte, TransferType type);

#ifdef USE_SOFT_SPI
static void SoftSpiRead(uint8_t cmd, uint8_t *buffer, uint32_t length);
#endif

#define HspiSendCMD(cmd) \
do{ \
	SoftSpiSendByte((cmd), COMMAND_TYPE); \
}while(0)

#define HspiSendDATA(data) \
do{ \
	SoftSpiSendByte((data), DATA_TYPE); \
}while(0)

static void ICACHE_FLASH_ATTR EPDAutoSleep();
static void ICACHE_FLASH_ATTR timerCallback(void *timer_arg);

static void ICACHE_FLASH_ATTR EPDSetWindow(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);
static void ICACHE_FLASH_ATTR EPDSetCursor(uint16_t x, uint16_t y);

static void ICACHE_FLASH_ATTR EPDDoSleep();

static uint8_t *buffer = NULL;
static TransferType transfer_type = UNKNOWN_TYPE;
static STATUS epd_status = IDLE;
static ETSTimer timer;
static uint8_t sleepTimeOut;

/**
 * @brief EPD硬件配置
 * */
void ICACHE_FLASH_ATTR EPDGpioSetup() {
	SpiAttr pAttr;

	// EPD RESET PIN
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);
	GPIO_OUTPUT_SET(EPD_RESET_PIN, GPIO_PIN_HIGH);

	// 默认关闭EPD供电
	// EPD POWER P-MOSFET GATE PIN
	gpio16_output_conf();
	gpio16_output_set(GPIO_PIN_HIGH);

	// EPD BUSY PIN, NO PULL AND SET INPUT MODE
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO4_U);
	GPIO_DIS_OUTPUT(EPD_BUSY_PIN);

	// DATA/COMMAND PIN, LOW LEVEL: COMMAND, HIGH LEVEL: DATA
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTDI_U);

#ifdef USE_USE_SOFT_SPI
	// in soft spi mode, pinmux same as hardware hspi implement
	// HSPI MOSI
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTCK_U);
	// HSPI SCLK
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U);
	GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_LOW);
	// HSPI CS
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDO_U);
	GPIO_OUTPUT_SET(HSPI_CS_PIN, GPIO_PIN_HIGH);
#else
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPI_MOSI);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI_CS);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI_SCLK);

	// Init HSPI GPIO
	// @see esp8266-technical_reference_cn.pdf page:46
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);

    pAttr.mode = SpiMode_Master;
    pAttr.subMode = SpiSubMode_0;
    pAttr.speed = SpiSpeed_10MHz;
    pAttr.bitOrder = SpiBitOrder_MSBFirst;
    SPIInit(SpiNum_HSPI, &pAttr);
#endif

	// 刷新完成自动断电定时器
	os_timer_disarm(&timer);
	os_timer_setfn(&timer, &timerCallback, NULL);
}

void ICACHE_FLASH_ATTR EPDReset() {
	epd_status = IDLE;
	// enable EPD power control p-mosfet
	gpio16_output_set(GPIO_PIN_LOW);
	// pull dowm reset pin 10ms
	GPIO_OUTPUT_SET(EPD_RESET_PIN, GPIO_PIN_LOW);
	os_delay_us(10000);
	GPIO_OUTPUT_SET(EPD_RESET_PIN, GPIO_PIN_HIGH);
}

/**
 * @brief soft spi send one byte, MSB first.
 * @param byte (0x00~0xFF)
 * */
static void ICACHE_FLASH_ATTR SoftSpiSendByte(uint8_t byte, TransferType type) {
	SpiData pDat;
	// 这里尽量不要强制指针转换，esp8266需要4字节对齐
	uint32_t buffer = byte;
	// 使用D/C脚状态缓存在连续DATA/COMMAND传输时每字节节省0.688us
	if(transfer_type != type) {
		transfer_type = type;
		GPIO_OUTPUT_SET(EPD_DC_PIN, (type & 0x1));
	}
#ifdef USE_SOFT_SPI
	GPIO_OUTPUT_SET(HSPI_CS_PIN, GPIO_PIN_LOW);
	int32_t i = 7;
	for(; i >= 0; i--) {
		GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_LOW);
		GPIO_OUTPUT_SET(HSPI_MOSI_PIN, ((byte >> i) & 0x1));
		GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_HIGH);
	}
	GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_LOW);
	GPIO_OUTPUT_SET(HSPI_CS_PIN, GPIO_PIN_HIGH);
#else
	pDat.cmd = 0;
	pDat.cmdLen = 0;
	pDat.addr = NULL;
	pDat.addrLen = 0;
	pDat.data = &buffer;
	pDat.dataLen = 1;
	SPIMasterSendData(SpiNum_HSPI, &pDat);
#endif
}

#ifdef USE_SOFT_SPI

/**
 * @brief ssd1675b的SDA是I/O类型，对于主机来说MOSI也充当MISO，因此只能在软件spi的情况下实现读取功能
 * @param cmd 发往ssd1675b命令
 * @param *buffer 读取数据存储区
 * @param length 读取数据长度
 * */
static void SoftSpiRead(uint8_t cmd, uint8_t *buffer, uint32_t length) {

	int32_t i = 7, j = 0;
	uint8_t inByte;

	if(transfer_type != COMMAND_TYPE) {
		transfer_type = COMMAND_TYPE;
		GPIO_OUTPUT_SET(EPD_DC_PIN, (COMMAND_TYPE & 0x1));
	}

	GPIO_OUTPUT_SET(HSPI_CS_PIN, GPIO_PIN_LOW);


	for(; i >= 0; i--) {
		GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_LOW);
		GPIO_OUTPUT_SET(HSPI_MOSI_PIN, ((cmd >> i) & 0x1));
		GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_HIGH);
	}

	transfer_type = DATA_TYPE;
	GPIO_OUTPUT_SET(EPD_DC_PIN, (DATA_TYPE & 0x1));

	GPIO_DIS_OUTPUT(HSPI_MOSI_PIN);

	for(; j < length; j++) {
		inByte = 0x00;
		for(i = 0; i < 8; i++) {
			GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_LOW);
			if(GPIO_INPUT_GET(HSPI_MOSI_PIN) == GPIO_PIN_HIGH) {
				inByte |= ((uint8_t)1 << (7 - i));
			}
			GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_HIGH);
		}
		*(buffer + j) = inByte;
	}

	GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_LOW);
	GPIO_OUTPUT_SET(HSPI_MOSI_PIN, GPIO_PIN_LOW);
	GPIO_OUTPUT_SET(HSPI_CS_PIN, GPIO_PIN_HIGH);
}

#endif

static void ICACHE_FLASH_ATTR timerCallback(void *timer_arg) {
	sleepTimeOut++;
	// 超时10秒或EPD主动拉低忙信号
	if((sleepTimeOut >= (EPD_REFRESH_TIMEOUT / 500)) || (GPIO_INPUT_GET(EPD_BUSY_PIN) == GPIO_PIN_LOW)) {
		os_timer_disarm(&timer);
		epd_status = IDLE;
		EPDDoSleep();
		EventBusGetDefault()->post(MAIN_EVENT_EPD_FINISH, 0);
	}
}

/**
 * @brief epd全局刷新一次需要3s左右，500ms一次回调判断busy状态
 * */
static void ICACHE_FLASH_ATTR EPDAutoSleep() {
	sleepTimeOut = 0;
	if(GPIO_INPUT_GET(EPD_BUSY_PIN) == GPIO_PIN_HIGH) {
		epd_status = BUSY;
		os_timer_arm(&timer, 500, TRUE);
	}else {
		epd_status = IDLE;
		EPDDoSleep();
		EventBusGetDefault()->post(MAIN_EVENT_EPD_FINISH, 0);
	}
}

STATUS ICACHE_FLASH_ATTR EPDTurnOnDisplay() {
	if(BUSY == epd_status) {
		return FAIL;
	}
	HspiSendCMD(0x22); // DISPLAY_UPDATE_CONTROL_2
	HspiSendDATA(0xC7);
	HspiSendCMD(0X20);	// MASTER_ACTIVATION
	//HspiSendCMD(0xFF);	// TERMINATE_FRAME_READ_WRITE
	// 稍稍延时5ms
	os_delay_us(5000);
	EPDAutoSleep();
	return OK;
}

static void ICACHE_FLASH_ATTR EPDSetWindow(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
	// Set RAM X - address
	HspiSendCMD(0x44);
	HspiSendDATA(x_start & 0x3F);
	HspiSendDATA(x_end & 0x3F);
	// Set RAM Y - address
	HspiSendCMD(0x45);
	HspiSendDATA(y_start & 0xFF);
	HspiSendDATA((y_start >> 8) & 0x1);
	HspiSendDATA(y_end & 0xFF);
	HspiSendDATA((y_end >> 8) & 0x1);
}

static void ICACHE_FLASH_ATTR EPDSetCursor(uint16_t x, uint16_t y) {
	// Set RAM X address counter
	HspiSendCMD(0x4E);
    HspiSendDATA(x & 0x3F);
    // Set RAM Y address counter
    HspiSendCMD(0x4F);
    HspiSendDATA(y & 0xFF);
    HspiSendDATA((y >> 8) & 0x1);
}

STATUS ICACHE_FLASH_ATTR EPDInit(const uint8_t *lut) {
	int i = 0;
	if(BUSY == epd_status) {
		return FAIL;
	}

    HspiSendCMD(0x01); // DRIVER_OUTPUT_CONTROL
    HspiSendDATA((EPD_HEIGHT - 1) & 0xFF);
    HspiSendDATA(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    HspiSendDATA(0x00);			// GD = 0; SM = 0; TB = 0;

    HspiSendCMD(0x0C);	// BOOSTER_SOFT_START_CONTROL
    HspiSendDATA(0xD7);
    HspiSendDATA(0xD6);
    HspiSendDATA(0x9D);

    // VCOM电压来改善电子纸显示效果，VCOM(负值)值越低，显示颜色越重
    // WRITE_VCOM_REGISTER
    HspiSendCMD(0x2C);
    HspiSendDATA(0x3C);

    HspiSendCMD(0x3A);	// SET_DUMMY_LINE_PERIOD
    HspiSendDATA(0x1A);			// 4 dummy lines per gate

    HspiSendCMD(0x3B);	// SET_GATE_TIME
    HspiSendDATA(0x08);			// 2us per line

	HspiSendCMD(0X3C);	// BORDER_WAVEFORM_CONTROL
	HspiSendDATA(0x63);			// 2us per lin

    HspiSendCMD(0X11);	// DATA_ENTRY_MODE_SETTING
    HspiSendDATA(0x03);			// X increment; Y increment

	// WRITE_LUT_REGISTER
    HspiSendCMD(0x32);
	for(i = 0; i < 30; i++) {
		HspiSendDATA(lut[i]);
	}

    return OK;
}

STATUS ICACHE_FLASH_ATTR EPDDisplayRAMInit() {
	if(buffer == NULL) {
		buffer = (uint8_t *)os_malloc(sizeof(uint8_t) * EPD_RAM_WIDTH * EPD_HEIGHT);
	}
	if(buffer != NULL) {
		os_memset(buffer, 0xFF, sizeof(uint8_t) * EPD_RAM_WIDTH * EPD_HEIGHT);
	}
	return (buffer == NULL) ? FAIL : OK;
}

/**
 * @brief 返回显存引用，外部应当只读该显存，尝试写入会干扰正常显示内容
 * @return buffer 显存引用
 * */
uint8_t * ICACHE_FLASH_ATTR EPDGetDisplayRAM() {
	return buffer;
}

void ICACHE_FLASH_ATTR EPDDisplayClear() {
	os_memset(buffer, 0xFF, sizeof(uint8_t) * EPD_RAM_WIDTH * EPD_HEIGHT);
}

/**
 * @brief 水平方式写显存
 * @param x (0~249)
 * @param y (0~121)
 * @param color BLACK = 0, WHITE = 1
 * */
void ICACHE_FLASH_ATTR EPDDrawHorizontal(uint16_t x, uint16_t y, uint8_t color) {
	uint32_t byteIndex = ((EPD_HEIGHT - 1) << 4) + (y >> 3) - (x << 4);
	uint8_t bitVal = (color & 0x1);
	uint8_t shift = (7 - (y & 0x7));

    *(buffer + byteIndex) &= ~((uint8_t)1 << shift);
    *(buffer + byteIndex) |= (bitVal << shift);
}

/**
 * @brief 垂直方式写显存
 * @param x (0~121)
 * @param y (0~249)
 * @param color BLACK = 0, WHITE = 1
 * */
void ICACHE_FLASH_ATTR EPDDrawVertical(uint16_t x, uint16_t y, uint8_t color) {
    uint32_t byteIndex = (y << 4) + (x >> 3);
	uint8_t bitVal = (color & 0x1);
	uint8_t shift = (7 - (x & 0x7));

	*(buffer + byteIndex) &= ~((uint8_t)1 << shift);
	*(buffer + byteIndex) |= (bitVal << shift);
}

STATUS ICACHE_FLASH_ATTR EPDFlush() {
	return EPDFillData(buffer);
}

STATUS ICACHE_FLASH_ATTR EPDFillData(uint8_t *data) {
    int i = 0, j = 0;
    if(BUSY == epd_status) {
    	return FAIL;
    }
    EPDSetWindow(0, EPD_WIDTH, 0, EPD_HEIGHT);
    for(j = 0; j < EPD_HEIGHT; j++) {
    	EPDSetCursor(0, j);
    	HspiSendCMD(0x24);
        for (i = 0; i < EPD_RAM_WIDTH; i++) {
        	HspiSendDATA(*(data + j * EPD_RAM_WIDTH + i));
        }
    }
    return OK;
}


STATUS ICACHE_FLASH_ATTR EPDGetStatus() {
	return epd_status;
}

void ICACHE_FLASH_ATTR EPDFillDisplayRAM(uint8_t *ptr, uint16_t offset, uint16_t length) {
	uint32_t i = 0;
	for(i = 0; i < length; i++) {
		*(buffer + offset + i) = *(ptr + i);
	}
}

static void ICACHE_FLASH_ATTR EPDDoSleep() {
	// DEEP_SLEEP_MODE
	HspiSendCMD(DEEP_SLEEP_MODE_CMD);
    HspiSendDATA(DEEP_SLEEP_MODE2);
    transfer_type = UNKNOWN_TYPE;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTCK_U);
	GPIO_OUTPUT_SET(HSPI_MOSI_PIN, GPIO_PIN_HIGH);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTDO_U);
	GPIO_OUTPUT_SET(HSPI_CS_PIN, GPIO_PIN_HIGH);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U);
	GPIO_OUTPUT_SET(HSPI_SCLK_PIN, GPIO_PIN_HIGH);

	//GPIO_OUTPUT_SET(EPD_RESET_PIN, GPIO_PIN_LOW);
	GPIO_OUTPUT_SET(EPD_BUSY_PIN, GPIO_PIN_HIGH);
	GPIO_OUTPUT_SET(EPD_DC_PIN, GPIO_PIN_HIGH);
	// disable EPD power control mosfet
	gpio16_output_set(GPIO_PIN_HIGH);
}


