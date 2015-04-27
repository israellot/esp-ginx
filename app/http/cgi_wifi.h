/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef CGI_WIFI_H
#define CGI_WIFI_H

int ICACHE_FLASH_ATTR http_wifi_api_get_status(http_connection *c);
int ICACHE_FLASH_ATTR http_wifi_api_scan(http_connection *c);
int ICACHE_FLASH_ATTR http_wifi_api_connect_ap(http_connection *c);
int ICACHE_FLASH_ATTR http_wifi_api_disconnect(http_connection *c);
int ICACHE_FLASH_ATTR http_wifi_api_check_internet(http_connection *c);
#endif