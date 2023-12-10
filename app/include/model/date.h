/*
 * date.h
 * @brief
 * Created on: Jun 29, 2021
 * Author: Yanye
 */

#ifndef APP_INCLUDE_MODEL_DATE_H_
#define APP_INCLUDE_MODEL_DATE_H_

// 存储于RTC MEM的日期时间数据，兼容老版本格式
// 更新完成后需要复制回Calendar以便view显示正确的内容
typedef struct _date {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    // 字节对齐
    uint8_t : 8;
} Date;

#endif /* APP_INCLUDE_MODEL_DATE_H_ */
