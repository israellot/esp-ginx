/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef http_response_h
#define http_response_h

#include "http_process.h"

#define HTTP_DEFAULT_SERVER "Smart Relay 1.0"
#define HTTP_VERSION "1.1"

#define HTTP_OK "200 OK"
#define HTTP_MOVED "301 Moved Permanently "
#define HTTP_REDIRECT "302 Found"
#define HTTP_NOT_MODIFIED "304 Not Modified"
#define HTTP_BAD_REQUEST "400 Bad Request"
#define HTTP_NOT_FOUND "404 Not Found"
#define HTTP_INTERNAL_ERROR "500 Internal Server Error"

//headers
#define HTTP_CONTENT_TYPE "Content-Type"
#define HTTP_CONTENT_LENGTH "Content-Length"
#define HTTP_CONNECTION "Connection"
#define HTTP_CACHE_CONTROL "Cache-Control"
#define HTTP_CONTENT_ENCODING "Content-Encoding"
#define HTTP_LOCATION "Location"
#define HTTP_HOST "Host"
#define HTTP_ORIGIN "Origin"
#define HTTP_SERVER "Server"
#define HTTP_ACCESS_CONTROL_ALLOW_ORIGIN "Access-Control-Allow-Origin"
#define HTTP_ACCESS_CONTROL_ALLOW_METHODS "Access-Control-Allow-Methods"
#define HTTP_ACCESS_CONTROL_ALLOW_HEADERS "Access-Control-Allow-Headers"
#define HTTP_ACCESS_CONTROL_REQUEST_HEADERS "Access-Control-Request-Headers"
#define HTTP_ACCESS_CONTROL_REQUEST_METHOD "Access-Control-Request-Method"
#define HTTP_USER_AGENT "User-Agent"
//websocket headers
#define HTTP_UPGRADE "Upgrade"
#define HTTP_SEC_WEBSOCKET_KEY "Sec-WebSocket-Key"
#define HTTP_SEC_WEBSOCKET_PROTOCOL "Sec-WebSocket-Protocol"
#define HTTP_SEC_WEBSOCKET_VERSION "Sec-WebSocket-Version"
#define HTTP_WEBSOCKET_ACCEPT "Sec-WebSocket-Accept"

#define HTTP_CONNECTION_CLOSE "Close"

#define JSON_CONTENT_TYPE "application/json"

#define HTTP_DEFAULT_CACHE "public, max-age=3600"



//request 
int ICACHE_FLASH_ATTR http_request_GET(http_connection *c,const char *path);
int ICACHE_FLASH_ATTR http_request_POST(http_connection *c,const char *path);


//response
int ICACHE_FLASH_ATTR http_response_OK(http_connection *c);
int ICACHE_FLASH_ATTR http_response_NOT_FOUND(http_connection *c);
int ICACHE_FLASH_ATTR http_response_BAD_REQUEST(http_connection *c);
int ICACHE_FLASH_ATTR http_response_REDIRECT(http_connection *c,const char* destination);

//ws
int ICACHE_FLASH_ATTR http_websocket_HANDSHAKE(http_connection *c);

//common
const char ICACHE_FLASH_ATTR *http_get_mime(char *url);
int ICACHE_FLASH_ATTR http_SET_HEADER(http_connection *c,const char *header,const char *value);
int ICACHE_FLASH_ATTR http_end_headers(http_connection *c);
int ICACHE_FLASH_ATTR http_reset_buffer(http_connection *c);

//json
int ICACHE_FLASH_ATTR write_json_object_start(http_connection *c);
int ICACHE_FLASH_ATTR write_json_object_end(http_connection *c);
int ICACHE_FLASH_ATTR write_json_object_separator(http_connection *c);
int ICACHE_FLASH_ATTR write_json_list_separator(http_connection *c);
int ICACHE_FLASH_ATTR write_json_array_start(http_connection *c);
int ICACHE_FLASH_ATTR write_json_array_end(http_connection *c);
int ICACHE_FLASH_ATTR write_json_int(http_connection *c,int value);
int ICACHE_FLASH_ATTR write_json_float(http_connection *c,float value);
int ICACHE_FLASH_ATTR write_json_bool(http_connection *c,int value);
int ICACHE_FLASH_ATTR write_json_string(http_connection *c,const char *s);
int ICACHE_FLASH_ATTR write_json_key(http_connection *c,const char *key);
int ICACHE_FLASH_ATTR write_json_pair(http_connection *c,const char *key,const char *value);
int ICACHE_FLASH_ATTR write_json_pair_string(http_connection *c,const char *key,const char *value);
int ICACHE_FLASH_ATTR write_json_pair_int(http_connection *c,const char *key,int value);
int ICACHE_FLASH_ATTR write_json_pair_float(http_connection *c,const char *key,float value);
int ICACHE_FLASH_ATTR write_json_pair_bool(http_connection *c,const char *key,int value);



#endif

