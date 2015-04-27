/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __BMP180_H
#define __BMP180_H

typedef struct {
	int32_t temp;
	int32_t press;
} bmp_data;

int ICACHE_FLASH_ATTR bmp180_init();
int ICACHE_FLASH_ATTR bmp180_read(bmp_data *data);

#endif