/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef CGI_H
#define CGI_H

#include "http.h"


struct cgi_transmit_arg{
	cgi_struct previous_cgi;

	uint8_t *data;
	uint8_t *dataPos;
	uint16_t len;

};

int ICACHE_FLASH_ATTR cgi_transmit(http_connection *connData);
int ICACHE_FLASH_ATTR cgi_cors(http_connection *connData);
int ICACHE_FLASH_ATTR cgi_url_rewrite(http_connection *connData);
int ICACHE_FLASH_ATTR cgi_redirect(http_connection *connData);
int ICACHE_FLASH_ATTR cgi_check_host(http_connection *connData);
int ICACHE_FLASH_ATTR cgi_file_system(http_connection *connData);
int ICACHE_FLASH_ATTR cgi_enforce_method(http_connection *connData);


#endif