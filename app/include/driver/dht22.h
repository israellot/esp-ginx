/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __DHT22_H
#define __DHT22_H

typedef struct{

	float temp;
	float hum;

} dht_data;


dht_data ICACHE_FLASH_ATTR dht22_read();

#endif