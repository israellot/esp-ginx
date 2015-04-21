/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __HTTP_WS_SERVER_H
#define __HTTP_WS_SERVER_H

typedef struct {

	//Listening connection data
	struct espconn server_conn;
	esp_tcp server_tcp;

	const char * host_domain;
	int port;


} http_ws_config;

void ICACHE_FLASH_ATTR http_ws_push_bin(http_connection *c,char *msg,size_t msgLen);
void ICACHE_FLASH_ATTR http_ws_push_text(http_connection *c,char *msg,size_t msgLen);
void ICACHE_FLASH_ATTR http_ws_server_init();
void ICACHE_FLASH_ATTR http_ws_server_start();

#endif