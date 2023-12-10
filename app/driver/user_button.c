/*
 * user_button.c
 * @brief
 * Created on: May 20, 2021
 * Author: Yanye
 */

#include "driver/user_button.h"

// do not let 'gpioIRQHandler' in flash
static void gpio_irq_handler(void *arg);

static void ICACHE_FLASH_ATTR clickTimerCallback(void *timer_arg);

static ClickListener thisClickListener = NULL;

static DoubleClickListener thisDoubleClickListener = NULL;

static LongClickListener thisLongClickListener = NULL;

static uint32_t firstTime  = 0, secondTime  = 0, elapse;
static uint32_t clickCount = 0;

static KeyEvent ketEvent = KEY_EVENT_UP;

static os_timer_t clickTimer;
static volatile BOOL forceExit;

static void gpio_irq_handler(void *arg) {
    uint32_t pin_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    uint8_t level;

    ETS_GPIO_INTR_DISABLE();

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, pin_status);

    if(pin_status & BIT(USER_BUTTON_PIN)) {
    	// 消抖
    	os_delay_us(5000);
    	level = GPIO_INPUT_GET(USER_BUTTON_PIN);

    	do {
    		if(level == GPIO_PIN_HIGH) {
				// 屏蔽噪声
    			if(ketEvent == KEY_EVENT_UP) {
    				//os_printf("break by KEY_EVENT_UP noise\n");
					break;
				}
				// 抬起
    			ketEvent = KEY_EVENT_UP;
				secondTime = system_get_time();
				//os_printf("ActionUp: %d\n", secondTime);

				// long click detect
				elapse = (secondTime < firstTime) ? (0xFFFFFFFF - firstTime + secondTime) : (secondTime - firstTime);

				//os_printf("elapse:%dus\n", elapse);

				if(elapse > LONG_CLICK_TIME) {
					//os_printf("thisLongClickListener\n");
					if(thisLongClickListener != NULL) {
						thisLongClickListener();
					}
					break;
				}
				// double click timeout
				if(elapse > DOUBLE_CLICK_TIMEOUT) {
					//os_printf("GPIO_PIN_HIGH DOUBLE_CLICK_TIMEOUT\n");
					clickCount = 0;
					break;
				}
				// double click detect，优先执行双击
				clickCount++;
				if(clickCount >= DOUBLE_CLICK_COUNT) {
					clickCount = 0;
					//os_printf("thisDoubleClickListener\n");
					if(thisDoubleClickListener != NULL) {
						thisDoubleClickListener();
					}
				}else {
					forceExit = FALSE;
					os_timer_arm(&clickTimer, SINGLE_CLICK_TRIGGER_DELAY, FALSE);
				}
			}else {
				// 屏蔽噪声
				if(ketEvent == KEY_EVENT_DOWN) {
    				//os_printf("break by KEY_EVENT_DOWN noise\n");
					break;
				}
				// 按下
				ketEvent = KEY_EVENT_DOWN;
				firstTime = system_get_time();

				//os_printf("ActionDown: %d\n", firstTime);

				if(clickCount > 0) {
					forceExit = TRUE;
					os_timer_disarm(&clickTimer);
					//os_printf("force close single click timer\n");

					// double click timeout
					elapse = (firstTime < secondTime) ? (0xFFFFFFFF - secondTime + firstTime) : (firstTime - secondTime);

					//os_printf("ActionDown elapse: %d\n", elapse);

					if(elapse > DOUBLE_CLICK_TIMEOUT) {
						//os_printf("GPIO_PIN_LOW DOUBLE_CLICK_TIMEOUT\n");
						clickCount = 0;
					}
				}
			}
    	}while(0);
    }

    ETS_GPIO_INTR_ENABLE();
}

static void ICACHE_FLASH_ATTR clickTimerCallback(void *timer_arg) {
	os_timer_disarm(&clickTimer);
	clickCount = 0;
	if(!forceExit && thisClickListener != NULL) {
		thisClickListener();
	}
}

void ICACHE_FLASH_ATTR UserButtonInit(void) {
	// 用户按钮
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO5_U);
	GPIO_DIS_OUTPUT(USER_BUTTON_PIN);

	os_timer_disarm(&clickTimer);
	os_timer_setfn(&clickTimer, clickTimerCallback, NULL);
}

void ICACHE_FLASH_ATTR UserButtonIRQInit(void) {
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH(gpio_irq_handler, NULL);
	// USER_BUTTON_PIN 带上拉，下降沿触发
	gpio_pin_intr_state_set(GPIO_ID_PIN(USER_BUTTON_PIN), GPIO_PIN_INTR_ANYEDGE);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(USER_BUTTON_PIN));
	ETS_GPIO_INTR_ENABLE();
}

uint8_t ICACHE_FLASH_ATTR UserButtonLevelRead(void) {
	return GPIO_INPUT_GET(USER_BUTTON_PIN);
}

void ICACHE_FLASH_ATTR UserButtonAddClickListener(ClickListener clickListener) {
	thisClickListener = clickListener;
}

void ICACHE_FLASH_ATTR UserButtonAddDoubleClickListener(DoubleClickListener doubleClickListener) {
	thisDoubleClickListener = doubleClickListener;
}

void ICACHE_FLASH_ATTR UserButtonAddLongClickListener(LongClickListener longClickListener) {
	thisLongClickListener = longClickListener;
}

