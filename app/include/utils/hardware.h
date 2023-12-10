/*
 * hardware.h
 * @brief
 * Created on: Jun 23, 2021
 * Author: Yanye
 */

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "c_types.h"

/**
 * @brief 获取电池电量
 * @return 电量等级0~100，步进10，例(0, 10, 20, 30...90 100)
 * */
uint32_t hw_get_battery_level(void);


#endif
