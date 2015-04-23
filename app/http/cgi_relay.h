/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef CGI_RELAY_H
#define CGI_RELAY_H

int ICACHE_FLASH_ATTR http_relay_api_status(http_connection *c);
int ICACHE_FLASH_ATTR http_relay_api_toggle(http_connection *c);
int ICACHE_FLASH_ATTR http_dht_api_read(http_connection *c);
#endif