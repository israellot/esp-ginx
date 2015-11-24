/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef http_server_h
#define http_server_h

#include "http_parser.h"

#define HTTPD_CGI_MORE 0 //when we want to lock cgi on that function
#define HTTPD_CGI_DONE 1
#define HTTPD_CGI_NOTFOUND 2 //not found on that cgi
#define HTTPD_CGI_AUTHENTICATED 2 //for now
#define HTTPD_CGI_NEXT_RULE 3 //allow other cgi to execute

#define HTTPD_STATE_ON_URL 0
#define HTTPD_STATE_ON_STATUS 1
#define HTTPD_STATE_HEADERS_END 2
#define HTTPD_STATE_ON_BODY 3
#define HTTPD_STATE_BODY_END 4
#define HTTPD_STATE_WS_DATA 5
#define HTTPD_STATE_WS_DATA_SENT 6
#define HTTPD_STATE_WS_CLIENT_DISCONNECT 7

//client
#define HTTP_CLIENT_CGI_MORE 0
#define HTTP_CLIENT_CGI_DONE 1

#define HTTP_CLIENT_DNS_FOUND 100
#define HTTP_CLIENT_DNS_NOT_FOUND 101
#define HTTP_CLIENT_CONNECT_OK 200
#define HTTP_CLIENT_CONNECT_FAIL 201
#define HTTP_CLIENT_REQUEST_HEADERS_SENT 202
#define HTTP_CLIENT_REQUEST_BODY_SENT 203

//ws
#define HTTP_WS_CGI_MORE 0
#define HTTP_WS_CGI_DONE 1

#define MAX_CONNECTIONS 8

#include "http.h"

int ICACHE_FLASH_ATTR http_transmit(http_connection *c);
int ICACHE_FLASH_ATTR http_write(http_connection *c,const char * message);
int ICACHE_FLASH_ATTR http_nwrite(http_connection *c,const char * message,size_t size);

void ICACHE_FLASH_ATTR http_process_free_connection(http_connection *conn);


char * ICACHE_FLASH_ATTR http_url_get_path(http_connection *c);
char * ICACHE_FLASH_ATTR http_url_get_host(http_connection *c);
char * ICACHE_FLASH_ATTR http_url_get_query(http_connection *c);
char * ICACHE_FLASH_ATTR http_url_get_query_param(http_connection *c,char* param);


void ICACHE_FLASH_ATTR http_parse_url(http_connection *c);
http_connection ICACHE_FLASH_ATTR * http_new_connection(uint8_t in,struct espconn *conn);
void http_execute_cgi(http_connection *conn);


void ICACHE_FLASH_ATTR http_set_save_header(http_connection *conn,const char* header);
void ICACHE_FLASH_ATTR http_set_save_body(http_connection *conn);
header * ICACHE_FLASH_ATTR http_get_header(http_connection *conn,const char* header);




#endif


