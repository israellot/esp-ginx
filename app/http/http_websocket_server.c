/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "c_stdio.h"

#include "espconn.h"
#include "http_parser.h"
#include "http_helper.h"
#include "http_process.h"
#include "http_websocket_server.h"
#include "user_config.h"

#include "ssl/ssl_crypto.h"
#include "util/base64.h"

#include "websocket.h"

#define WS_PORT 8088

static http_ws_config ws_config;

static void ICACHE_FLASH_ATTR ws_output_write_function(const char * data,size_t len,void *arg){

	http_connection *c = (http_connection *)arg;
	http_nwrite(c,data,len);
}

static http_callback app_callback;
static http_callback data_sent_callback;
static http_callback client_disconnected_callback;
static http_callback client_connected_callback;

static void http_ws_handle_message(http_connection *c,ws_frame *msg){

	if(c->cgi.function!=NULL){

		//put frame on argument
		c->cgi.argument=(void *)msg;
		//call cgi
		int r = c->cgi.function(c);
		int sent = http_transmit(c);

		if(r==HTTP_WS_CGI_DONE){
			//we should close the socket
			c->cgi.done=1;

			if(sent==0){
				//we should active close it now as no data was sent, so there will be no further callback
				//TODO send close frame instead of hard closing
				espconn_disconnect(c->espConnection);
				http_process_free_connection(c);	
			}
		}

		
	}

}


static const char  *ws_uuid ="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
int ICACHE_FLASH_ATTR http_ws_handle_connect(http_connection *c) {	

	NODE_DBG("http_ws_handle_connect c =%p",c);

	if(c->state == HTTPD_STATE_ON_URL){
		http_set_save_header(c,HTTP_ORIGIN);	
		http_set_save_header(c,HTTP_CONNECTION);	
		http_set_save_header(c,HTTP_UPGRADE);		
		http_set_save_header(c,HTTP_SEC_WEBSOCKET_KEY);
		http_set_save_header(c,HTTP_SEC_WEBSOCKET_PROTOCOL);
		http_set_save_header(c,HTTP_SEC_WEBSOCKET_VERSION);

		return HTTP_WS_CGI_MORE;
	}

	//wait handshake request complete
	if(c->state != HTTPD_STATE_BODY_END)
		return HTTP_WS_CGI_MORE;


	header * upgrade_header = http_get_header(c,HTTP_UPGRADE);
	header * connection_header = http_get_header(c,HTTP_CONNECTION);
	header * origin_header = http_get_header(c,HTTP_ORIGIN);
	header * key_header = http_get_header(c,HTTP_SEC_WEBSOCKET_KEY);

	if(upgrade_header==NULL) goto badrequest;
	if(connection_header==NULL) goto badrequest;
	if(origin_header==NULL) goto badrequest;
	if(key_header==NULL) goto badrequest;

	NODE_DBG("headers ok");

	if(os_strcasecmp(upgrade_header->value,"websocket")!=0) goto badrequest;

	// Following (https://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-17)
	//calculate sha1 of concatenetion key+uuid
	uint8_t digest[20]; //sha1 is always 20 byte long
	SHA1_CTX ctx;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx,key_header->value,os_strlen(key_header->value));
	SHA1_Update(&ctx,ws_uuid,os_strlen(ws_uuid));
	SHA1_Final(digest,&ctx);
		
	char base64Digest[31]; // 
	Base64encode(base64Digest,(const char*)digest,20);

	//accept the handshake
	http_SET_HEADER(c,HTTP_UPGRADE,"WebSocket");
	http_SET_HEADER(c,HTTP_CONNECTION,"Upgrade");
	
	http_SET_HEADER(c,HTTP_WEBSOCKET_ACCEPT,base64Digest);

	http_websocket_HANDSHAKE(c);
	c->handshake_ok=1;

	if(client_connected_callback!=NULL)
		client_connected_callback(c);

	return HTTP_WS_CGI_MORE;

badrequest:
	http_response_BAD_REQUEST(c);
	return HTTP_WS_CGI_DONE;

}



static int ICACHE_FLASH_ATTR http_ws_cgi_execute(http_connection * c){

	NODE_DBG("http_ws_cgi_execute c =%p",c);

	if(!c->handshake_ok)
	{
		int r = http_ws_handle_connect(c);
		if(r==HTTP_WS_CGI_DONE)
			c->cgi.done=1; //mark for destruction
		
		http_transmit(c);
	}
	else{

		if(c->body.data==NULL){

			if(c->state==HTTPD_STATE_WS_DATA_SENT){
				if(data_sent_callback!=NULL)
					data_sent_callback(c);					
			}
			
		}
		else if(c->state==HTTPD_STATE_WS_DATA){

			NODE_DBG("websocket frame size %d",c->body.len);

			ws_frame frame;
			ws_parse_frame(&frame,c->body.data,c->body.len);


			switch(frame.TYPE){

				case WS_INVALID:
					NODE_DBG("\treceived invalid frame");
					break;
				case WS_PING:
					//send ping
					ws_output_frame(&frame,WS_PING,"PING",strlen("PING"));
					ws_write_frame(&frame,ws_output_write_function,c);
					http_transmit(c);
				case WS_CLOSE:
					//send close back
					ws_output_frame(&frame,WS_PING,"\0\0",2);
					ws_write_frame(&frame,ws_output_write_function,c);
					http_transmit(c);
					//mark as done
					c->cgi.done=1;
					break;
				case WS_TEXT:
				case WS_BINARY:
					http_ws_handle_message(c,&frame);
					break;
				case WS_CONTINUATION:
					break; // TODO: Implement continuation logic

			}		


		}
		else if(c->state == HTTPD_STATE_WS_CLIENT_DISCONNECT){

			if(client_disconnected_callback!=NULL)
				client_disconnected_callback(c);
		}

		return 1;

	}

}

// called when a client connects to our server
static void ICACHE_FLASH_ATTR http_ws_connect_callback(void *arg) {

	struct espconn *conn=arg;

	http_connection *c = http_new_connection(1,conn); // get a connection from the pool, signal it's an incomming connection
	c->cgi.execute = http_ws_cgi_execute; 		  // attach our cgi dispatcher	

	//attach app callback to cgi function
	c->cgi.function=app_callback;

	c->handshake_ok=0;

	//let's disable NAGLE alg so TCP outputs faster ( in theory )
	espconn_set_opt(conn, ESPCONN_NODELAY | ESPCONN_REUSEADDR );

	//override timeout to 240s
	espconn_regist_time(conn,240,0);

}

//PUBLIC function

void ICACHE_FLASH_ATTR http_ws_push_text(http_connection *c,char *msg,size_t msgLen){

	ws_frame frame;
	ws_output_frame(&frame,WS_TEXT,msg,msgLen);
	ws_write_frame(&frame,ws_output_write_function,c);	
	http_transmit(c);

}

void ICACHE_FLASH_ATTR http_ws_push_bin(http_connection *c,char *msg,size_t msgLen){

	ws_frame frame;
	ws_output_frame(&frame,WS_BINARY,msg,msgLen);
	ws_write_frame(&frame,ws_output_write_function,c);
	http_transmit(c);
	
}

void ICACHE_FLASH_ATTR http_ws_server_init(http_callback connect,http_callback disconnect,http_callback received,http_callback data_sent)
{	

	app_callback = received;
	data_sent_callback=data_sent;
	client_disconnected_callback = disconnect;
	client_connected_callback = connect;

	espconn_delete(&ws_config.server_conn); //just to be sure we are on square 1
	
	ws_config.server_conn.type = ESPCONN_TCP;
	ws_config.server_conn.state = ESPCONN_NONE;
	ws_config.server_conn.proto.tcp = &ws_config.server_tcp;
	ws_config.server_conn.proto.tcp->local_port = WS_PORT;		

	NODE_DBG("Websocket server init, conn=%p", &ws_config.server_conn);

}


void ICACHE_FLASH_ATTR http_ws_server_start(){


	NODE_DBG("Websocket server start, conn=%p", &ws_config.server_conn);
	espconn_regist_connectcb(&ws_config.server_conn, http_ws_connect_callback);	
	espconn_accept(&ws_config.server_conn);

}
