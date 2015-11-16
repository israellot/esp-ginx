/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __HTTP_H
#define __HTTP_H

#define HTTP_BUFFER_SIZE 1460 // Let's keep it close the TCP MTU (maximum transmit unit) ( 1460 on this lwip ) 
 							  //so we will send a whole packet per time
#define MAX_HEADERS 10
#define URL_MAX_SIZE 256


#include "http_parser.h"
#include "espconn.h"
#include "lwip/ip_addr.h"

typedef struct http_connection http_connection;
typedef int (*http_callback)(http_connection *c);  //callback function
typedef int (*cgi_execute_function)(http_connection *c); //cgi execute function

typedef struct {
	const char * key;
	char * value;
	uint8_t save;
} header;

typedef struct {
		uint8_t *buffer;
		uint8_t *bufferPos;
		header headers[MAX_HEADERS];
} http_buffer;

typedef struct {	
		cgi_execute_function execute;		

		http_callback function;

		void *data;
		const void *argument;
		uint8_t done;

		uint32_t flags;

} cgi_struct;

struct http_connection{

	uint8_t type;

	struct espconn *espConnection;

	struct espconn client_connection;
	esp_tcp client_tcp;
	ip_addr_t host_ip;
	os_timer_t timeout_timer;
	enum http_method method;
	char * request_body;

	struct http_parser parser;
	struct http_parser_settings parser_settings;
	
	uint8_t state;

	char url[URL_MAX_SIZE];
	struct http_parser_url url_parsed;

	//headers
	header headers[MAX_HEADERS];	

	//body
	struct {
		uint8_t save;
		char *data;
		size_t len;
	} body;	

	cgi_struct cgi;

	http_buffer output;

	//websocket related
	uint8_t handshake_ok;

	

	void *reverse;
};



#endif