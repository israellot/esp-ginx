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
#include "cgi.h"
#include "cgi_wifi.h"
#include "cgi_relay.h"
#include "user_config.h"

#include "http.h"
#include "http_process.h"
#include "websocket.h"
#include "http_websocket_server.h"

#define HTTP_PORT 80

#define WS_APP_STREAM_START "stream start"
#define WS_APP_STREAM_STOP "stream stop"

struct ws_app_context {

	uint8_t stream_data;
	int packet_requested_size;
	http_connection *conn;
	uint8_t waiting_sent;
	char * packet;
	int packet_size;
};


static void ICACHE_FLASH_ATTR ws_app_send_packet(struct ws_app_context *context){

	NODE_DBG("Webscoket app send packet size %d, requested: %d",context->packet_size,context->packet_requested_size);

	if( (!context->waiting_sent) && context->stream_data==1){
		//send packet

		if(context->packet_requested_size != context->packet_size ){
			NODE_DBG("Webscoket app changing packet size %p",context);

			if(context->packet!=NULL)
				os_free(context->packet); //free previous packet

			context->packet=NULL;
			context->packet_size=0;
		}	

		if(context->packet==NULL)
		{
			NODE_DBG("Webscoket allocating packet %p",context);
			context->packet = (char *)os_zalloc(context->packet_requested_size);
			context->packet_size=context->packet_requested_size;
			//fill with trash data
			int i;
			for(i=0; i < context->packet_size;i++)
				context->packet[i]=i%0xFF;
		} 
		
		http_ws_push_bin(context->conn,context->packet,context->packet_size);
		context->waiting_sent=1;
	}

}


static int  ICACHE_FLASH_ATTR ws_app_msg_sent(http_connection *c){

	NODE_DBG("Webscoket app msg sent %p",c);

	struct ws_app_context *context = (struct ws_app_context*)c->reverse;

	if(context!=NULL){

		NODE_DBG("\tcontext found %p",context);

		context->waiting_sent=0;

		if(context->stream_data==1){
			//no requet to stop made, send next packet
			ws_app_send_packet(context);
		}

	}

}

static int  ICACHE_FLASH_ATTR ws_app_client_disconnected(http_connection *c){

	NODE_DBG("Webscoket app client disconnected %p",c);

	//clean up
	struct ws_app_context *context = (struct ws_app_context*)c->reverse;

	if(context!=NULL){

		if(context->packet!=NULL)
			os_free(context->packet);
		context->packet=NULL;
	}
	os_free(context);	

}

static int  ICACHE_FLASH_ATTR ws_app_client_connected(http_connection *c){

	NODE_DBG("Webscoket app client connected %p",c);
		
	//create config
	struct ws_app_context *context = (struct  ws_app_context*)os_zalloc(sizeof(struct ws_app_context));
	context->conn=c;
	c->reverse = context; //so we may find it on callbacks
	
	NODE_DBG("\tcontext %p",context);
}


static int  ICACHE_FLASH_ATTR ws_app_msg_received(http_connection *c){

	NODE_DBG("Webscoket app msg received %p",c);

	struct ws_app_context *context = (struct ws_app_context*)c->reverse;

	ws_frame *msg = (ws_frame *)c->cgi.argument;
	if(msg==NULL)
		return HTTP_WS_CGI_MORE; //just ignore and move on

	if(msg->SIZE <=0)
		return HTTP_WS_CGI_MORE; //just ignore and move on

	char * s = msg->DATA;
	char *last = s+msg->SIZE;

	//make a null terminated string
	char * str = (char *)os_zalloc(msg->SIZE + 1);
	os_memcpy(str,s,msg->SIZE);
	str[msg->SIZE]=0;

	NODE_DBG("\tmsg: %s",str);
	
	if(strstr(str,WS_APP_STREAM_START)==str){
		//request to start stream
		NODE_DBG("\trequest stream start");

		char * s= str + strlen(WS_APP_STREAM_START);		

		int pSize = atoi(s);

		NODE_DBG("\trequest stream packet size %d",pSize);

		if(pSize>0 && pSize <= 1024)
		{					
			context->packet_requested_size=pSize;
			context->stream_data =1;
			if(!context->waiting_sent)
				ws_app_send_packet(context); //send fisrt pkt
		}

	}
	else if(strstr(str,WS_APP_STREAM_STOP)==str){
		NODE_DBG("\trequest stream stop");
		context->stream_data=0;
	}

	os_free(str);

	return HTTP_WS_CGI_MORE;

}

void ICACHE_FLASH_ATTR init_ws_server(){

	NODE_DBG("Webscoket app init");

	//ws
	http_ws_server_init(ws_app_client_connected,ws_app_client_disconnected,ws_app_msg_received,ws_app_msg_sent);
	http_ws_server_start();

}