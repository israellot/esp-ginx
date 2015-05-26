#include "c_types.h"
#include "c_string.h"
#include "c_stdio.h"
#include "user_interface.h"
#include "mem.h"
#include "osapi.h"
#include "user_config.h"
#include "espconn.h"

#include "mqtt.h"
#include "mqtt_msg.h"

#include "driver/relay.h"

#include "sensor/sensors.h"

#include "serial_number.h"

#define MQTT_SERVER_IP "104.131.71.110"
#define MQTT_RELAY_SET_SUFIX "/SET"

static os_timer_t mqtt_timer;
static MQTT_Client mqtt_client;

static os_timer_t sensor_read_timer;

static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;

typedef struct mqtt_relay{
	uint8_t id;
	char topic[20];		
}mqtt_relay;

mqtt_relay relays[RELAY_COUNT];

static void mqtt_relay_change_cb(int id,int state){

	MQTT_DBG("mqtt_relay_change_cb #%d %d",id,state);

	mqtt_relay * relay = &relays[id];	

	if(state<0)
		state = relay_get_state(id);

	MQTT_Publish(&mqtt_client, relay->topic, state?"1":"0", 1, 0, 1);

}

static void ICACHE_FLASH_ATTR mqtt_app_timer_cb(void *arg){

	
	struct ip_info ipConfig;
	
	wifi_get_ip_info(STATION_IF, &ipConfig);	
	wifiStatus = wifi_station_get_connect_status();

	//check wifi
	if(wifiStatus != lastWifiStatus){

		lastWifiStatus = wifiStatus;
		
		if(wifiStatus==STATION_GOT_IP && ipConfig.ip.addr != 0){
        	MQTT_DBG("MQTT: Detected wifi network up");
        	MQTT_Connect(&mqtt_client);       
	    }
	    else{
	    	MQTT_DBG("MQTT: Detected wifi network down");
	    	MQTT_Disconnect(&mqtt_client);    
	    }

	}
	    
}



static void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_DBG("MQTT: Connected");

	int i;
	for(i=0;i<relay_count();i++){

		char * setTopic = (char *)os_zalloc(strlen(relays[i].topic)+strlen(MQTT_RELAY_SET_SUFIX)+1);
		os_strcpy(setTopic,relays[i].topic);
		os_strcat(setTopic,MQTT_RELAY_SET_SUFIX);

		MQTT_Subscribe(client, setTopic, 1);

		os_free(setTopic);

		mqtt_relay_change_cb(i,-1); //force 1st publish
	}
	
	

}

static void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_DBG("MQTT: Disconnected");
}

static void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	MQTT_DBG("MQTT: Published");
}

static void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	MQTT_DBG("Receive topic: %s, data: %s ", topicBuf, dataBuf);

	os_free(topicBuf);
	os_free(dataBuf);

	// set relay
	int i;
	for(i=0;i<relay_count();i++){

		mqtt_relay *r = &relays[i];

		//compare topic
		if(os_strncmp(topic,r->topic,strlen(r->topic))==0){
			MQTT_DBG("Topic 1st part match");
			//check set suffix
			if(os_strncmp(topic+strlen(r->topic),MQTT_RELAY_SET_SUFIX,strlen(MQTT_RELAY_SET_SUFIX))==0)
			{
				MQTT_DBG("Topic 2nd part match");
				int state = data[0] - '0';

				if(state==0 || state == 1)
				{
					MQTT_DBG("Setting relay #%d to %d via MQTT", i, state);
					relay_set_state(i,state);
					break;
				}

				
			}
		} 
			

	}

	
}

static void ICACHE_FLASH_ATTR sensor_read_timer_cb(void *arg){

	char * buff = (char *)os_zalloc(64);

	sensor_data data;
	sensors_get_data(&data);	

	c_sprintf(buff,"%f",data.dht22.temp);
	MQTT_Publish(&mqtt_client, "temperature/"SERIAL_NUMBER, data.dht22.temp_string, strlen(data.dht22.temp_string), 0, 1);

	os_memset(buff,0,64);
	c_sprintf(buff,"%f",data.dht22.hum);
	MQTT_Publish(&mqtt_client, "humidity/"SERIAL_NUMBER, data.dht22.hum_string, strlen(data.dht22.hum_string), 0, 1);

	os_memset(buff,0,64);
	c_sprintf(buff,"%d",data.bmp180.press);
	MQTT_Publish(&mqtt_client, "pressure/"SERIAL_NUMBER, buff, strlen(buff), 0, 1);

	os_free(buff);



}

void ICACHE_FLASH_ATTR mqtt_app_init(){
	

	//set relays
	int i;
	for(i=0;i<relay_count();i++){
		relays[i].id=i;		
		os_strcpy(relays[i].topic,"relay/");
		os_strcat(relays[i].topic,SERIAL_NUMBER"/");
		os_strcati(relays[i].topic,i);
	}

	//register listeners 
	relay_register_listener(mqtt_relay_change_cb);

	MQTT_InitConnection(&mqtt_client, MQTT_SERVER_IP, 1883, 0);
	MQTT_InitClient(&mqtt_client, SERIAL_NUMBER, NULL, NULL, 120, 1);

	MQTT_OnConnected(&mqtt_client, mqttConnectedCb);
	MQTT_OnDisconnected(&mqtt_client, mqttDisconnectedCb);
	MQTT_OnPublished(&mqtt_client, mqttPublishedCb);
	MQTT_OnData(&mqtt_client, mqttDataCb);
	
	//arm mqtt timer
	os_memset(&mqtt_timer,0,sizeof(os_timer_t));
	os_timer_disarm(&mqtt_timer);
	os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_app_timer_cb, NULL);
	os_timer_arm(&mqtt_timer, 2000, 1);

	//arm sensor read timer
	os_memset(&sensor_read_timer,0,sizeof(os_timer_t));
	os_timer_disarm(&sensor_read_timer);
	os_timer_setfn(&sensor_read_timer, (os_timer_func_t *)sensor_read_timer_cb, NULL);
	os_timer_arm(&sensor_read_timer, 5000, 1);

}