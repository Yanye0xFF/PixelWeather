/*
 * appicon.c
 * @brief
 * Created on: Nov 18, 2020
 * Author: Yanye
 */

#include "view/appicon.h"

/**
 * @brief weather icon and weather id map
 * */
static const char *icon[] = {"sunny", "cloud", "yin", "rain", "rain2sun", "thunder",
		"snow", "fog", "wind", "hail","unknown"};

// key is integer part of b0 b1 bxx's
// value is index in '*icon'
static const Pair pairs[] = {{0, 0}, {1, 1}, {2, 2}, {3, 4}, {4, 5},
{6, 3}, {7, 3}, {8, 3}, {9, 3}, {10, 3}, {14, 6}, {15, 6}, {99, 6},
{19, 9}, {32, 9}, {20, 7}, {30, 8}};

const char * ICACHE_FLASH_ATTR getWeatherIcon(sint8_t weatherId) {
	uint32_t i = 0;
	uint32_t size = (sizeof(pairs) / sizeof(Pair));
	for(; i < size; i++) {
		if(pairs[i].key == weatherId) {
			break;
		}
	}
	return ((i < size) ? icon[pairs[i].value] : icon[10]);
}
