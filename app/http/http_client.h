/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __HTTP_CLIENT_H
#define __HTTP_CLIENT_H



http_connection * ICACHE_FLASH_ATTR http_client_new(http_callback callback);
int ICACHE_FLASH_ATTR http_client_open_url(http_connection *c,char *url);

#endif