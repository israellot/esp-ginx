/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
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

#include "json/cJson.h"



int ICACHE_FLASH_ATTR http_relay_api_status(http_connection *c) {


	NODE_DBG("http_wifi_api_get_status");
		
	//wait for whole body
	if(c->state <HTTPD_STATE_BODY_END)
		return HTTPD_CGI_MORE; 	

	//write headers
	http_SET_HEADER(c,HTTP_CONTENT_TYPE,JSON_CONTENT_TYPE);	
	http_response_OK(c);

	cJSON *root, *relays, *fld;

	root = cJSON_CreateObject(); 
	cJSON_AddItemToObject(root, "relays", relays = cJSON_CreateArray());
	
	int i;
	for(i=0;i<relay_count();i++){

		cJSON_AddItemToArray(relays,fld=cJSON_CreateObject());
		cJSON_AddNumberToObject(fld,"relay",i);
		cJSON_AddNumberToObject(fld,"state",relay_get_state(i));
		
	}

	http_write_json(c,root);	

	//delete json struct
	cJSON_Delete(root);

	return HTTPD_CGI_DONE;


}

int ICACHE_FLASH_ATTR http_relay_api_toggle(http_connection *c) {


	NODE_DBG("http_wifi_api_get_status");	
	

	//wait for whole body
	if(c->state <HTTPD_STATE_BODY_END)
		return HTTPD_CGI_MORE; 

	//parse json
	cJSON * root = cJSON_Parse(c->body.data);
	if(root==NULL) goto badrequest;
	
	cJSON * relay = cJSON_GetObjectItem(root,"relay");
	if(relay==NULL) goto badrequest;


	int relayNumber = relay->valueint;
	cJSON_Delete(root);	

	if(relayNumber<0 || relayNumber >=relay_count()){
		http_response_BAD_REQUEST(c);
		NODE_DBG("Wrong relay");		
		return HTTPD_CGI_DONE;
	}
	else{
		//valid relay

		unsigned status = relay_get_state(relayNumber);
		status = relay_toggle_state(relayNumber);

		//write headers
		http_SET_HEADER(c,HTTP_CONTENT_TYPE,JSON_CONTENT_TYPE);	
		http_response_OK(c);


		//create json
		root = cJSON_CreateObject();
		cJSON_AddNumberToObject(root,"relay",relayNumber);
		cJSON_AddNumberToObject(root,"state",status);

		http_write_json(c,root);

		//delete json struct
		cJSON_Delete(root);

		return HTTPD_CGI_DONE;

	}	

badrequest:
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
		
	//create json
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root,"temp",data.dht22.temp);
	cJSON_AddNumberToObject(root,"hum",data.dht22.hum);

	//write json
	http_write_json(c,root);

	//delete json struct
	cJSON_Delete(root);
				
	return HTTPD_CGI_DONE;


}

