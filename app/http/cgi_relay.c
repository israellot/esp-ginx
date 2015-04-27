/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "c_string.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "c_stdio.h"
#include "platform.h"
#include "user_config.h"
#include "driver/relay.h"
#include "sensor/sensors.h"

#include "cgi.h"

#include "http.h"
#include "http_parser.h"
#include "http_server.h"
#include "http_process.h"
#include "http_helper.h"
#include "http_client.h"

#include "json/jsmn.h"
#include "json/json.h"



int ICACHE_FLASH_ATTR http_relay_api_status(http_connection *c) {


	NODE_DBG("http_wifi_api_get_status");
		
	//wait for whole body
	if(c->state <HTTPD_STATE_BODY_END)
		return HTTPD_CGI_MORE; 	

	//write headers
	http_SET_HEADER(c,HTTP_CONTENT_TYPE,JSON_CONTENT_TYPE);	
	http_response_OK(c);

	write_json_object_start(c);
	write_json_key(c,"relays");
	write_json_object_separator(c);
	write_json_array_start(c);

	int i;
	for(i=0;i<relay_count();i++){

		if(i>0)
			write_json_list_separator(c);

		write_json_object_start(c);
		write_json_pair_int(c,"relay",i);
		write_json_list_separator(c);
		write_json_pair_int(c,"state",relay_get_state(i));
		write_json_object_end(c);

		
	}
	
	write_json_array_end(c);
	write_json_object_end(c);
	

			
	return HTTPD_CGI_DONE;


}

int ICACHE_FLASH_ATTR http_relay_api_toggle(http_connection *c) {


	NODE_DBG("http_wifi_api_get_status");	
	

	//wait for whole body
	if(c->state <HTTPD_STATE_BODY_END)
		return HTTPD_CGI_MORE; 

	
	int ret = json_count_token(c->body.data,c->body.len);
	
	jsonPair fields[1]={
		{
			.key="relay",
			.value=NULL
		}
	};

	if(json_parse(&fields[0],1,c->body.data,c->body.len)){
		
		int relayNumber = atoi(fields[0].value);	

		if(relayNumber<0 || relayNumber >=relay_count()){
			http_response_BAD_REQUEST(c);
			NODE_DBG("Wrong relay");		
			return HTTPD_CGI_DONE;
		}

		unsigned status = relay_get_state(relayNumber);
		status = relay_toggle_state(relayNumber);

		//write headers
		http_SET_HEADER(c,HTTP_CONTENT_TYPE,JSON_CONTENT_TYPE);	
		http_response_OK(c);

		write_json_object_start(c);
		write_json_pair_int(c,"relay",relayNumber);
		write_json_list_separator(c);
		write_json_pair_int(c,"state",status);
		write_json_object_end(c);

		return HTTPD_CGI_DONE;

	}
	else{
		http_response_BAD_REQUEST(c);		
		return HTTPD_CGI_DONE;
	}

	http_response_BAD_REQUEST(c);		
	return HTTPD_CGI_DONE;


}

//TODO move to own file
int ICACHE_FLASH_ATTR http_dht_api_read(http_connection *c) {


	NODE_DBG("http_dht_api_read");
		
	//wait for whole body
	if(c->state <HTTPD_STATE_BODY_END)
		return HTTPD_CGI_MORE; 	

	//write headers
	http_SET_HEADER(c,HTTP_CONTENT_TYPE,JSON_CONTENT_TYPE);	
	http_response_OK(c);

	sensor_data data;
	sensors_get_data(&data);
		
	write_json_object_start(c);
	write_json_pair_float(c,"temp",data.dht22.temp);
	write_json_list_separator(c);
	write_json_pair_float(c,"hum",data.dht22.hum);	
	write_json_object_end(c);

			
	return HTTPD_CGI_DONE;


}

