/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __HTTP_SERVER_H
#define __HTTP_SERVER_H

#include "http.h"

#define ANY_HOST NULL
#define HTTP_ANY_METHOD -1
#define NO_FLAG 0

#define NEED_POST	1<<0
#define NEED_GET	1<<1
#define NEED_BODY	1<<2

//A struct describing an url and how it should be processed
typedef struct {	
	const char *url;
	http_callback cgiFunction;
	const void *cgiArgument;
	enum http_method method;
	uint32_t flags;
} http_server_url;

typedef struct{
	char * match_url;
	char * rewrite_url;
}url_rewrite;

typedef struct {

	//Listening connection data
	struct espconn server_conn;
	esp_tcp server_tcp;

	http_server_url **urls;

	const char * host_domain;
	int port;

	uint8_t enable_captive;
	uint8_t enable_cors;
	
	url_rewrite * rewrites;	
	int rewrite_count;

} http_server_config;



void ICACHE_FLASH_ATTR http_server_init();
void ICACHE_FLASH_ATTR http_server_bind_port(int port);
void ICACHE_FLASH_ATTR http_server_bind_domain(const char * domain);
void ICACHE_FLASH_ATTR http_server_bind_urls(http_server_url *urls);
void ICACHE_FLASH_ATTR http_server_enable_captive_portal();
void ICACHE_FLASH_ATTR http_server_enable_cors();
void ICACHE_FLASH_ATTR http_server_start();

#endif