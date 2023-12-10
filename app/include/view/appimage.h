/*
 * appimage.h
 * @brief
 * Created on: Oct 13, 2020
 * Author: Yanye
 */

#ifndef _FACTORY_MODE_VIEW_H_
#define _FACTORY_MODE_VIEW_H_

#include "c_types.h"

extern const uint8_t factory_image[];
extern const uint8_t shutdown_image[];
extern const uint8_t unauth_image[];
extern const uint8_t update_image[];
extern const uint8_t batlow_image[];
extern const uint8_t idle_image[];

extern const uint32_t shutdown_size;

#endif
