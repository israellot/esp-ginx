#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "c_types.h"
#include "c_string.h"
#include "c_stdlib.h"
#include "c_stdio.h"
#include "gpio.h"
#include "user_interface.h"
#include "user_config.h"
#include "platform.h"
#include "config.h"
#include "driver/relay.h"

#define MAX_RELAY_LISTENERS 5

uint8_t relay_init_ok=0;

relay_change_callback listeners[MAX_RELAY_LISTENERS];

void ICACHE_FLASH_ATTR relay_register_listener(relay_change_callback f){

	NODE_DBG("Relay listener add %p",f);
	int i=0;
	while(listeners[i]!=NULL) i++;

	if(i>=MAX_RELAY_LISTENERS){
		NODE_DBG("No more listener space %d",i);
		return;
	}

	NODE_DBG("Relay listener add index %d",i);

	listeners[i]=f;
}

void ICACHE_FLASH_ATTR relay_unregister_listener(relay_change_callback f){

	NODE_DBG("Relay listener remove %p",f);

	int i=0;
	while(listeners[i]!=f) i++;

	if(i>=MAX_RELAY_LISTENERS)
		return;

	listeners[i]=NULL;
}

static void ICACHE_FLASH_ATTR relay_notify_listeners(int relayId,int state){

	NODE_DBG("relay_notify_listeners");

	int i;
	for(i=0;i<MAX_RELAY_LISTENERS;i++){

		if(listeners[i]!=NULL){
			NODE_DBG("Relay listener notify %p",listeners[i]);
			listeners[i](relayId,state);
		}
	}

}

static relay * ICACHE_FLASH_ATTR find_relay(int relayNumber){

	

	if(relayNumber>=RELAY_COUNT)
		return NULL;

	config_data * config  = config_read();

	return &config->relay_state.relay[relayNumber];	

}

int ICACHE_FLASH_ATTR relay_get_state(int relayNumber){

	relay * r = find_relay(relayNumber);
	if(r==NULL)
		return -1;

	return r->state;

}

int ICACHE_FLASH_ATTR relay_set_state(int relayNumber,unsigned state){

	relay * r = find_relay(relayNumber);
	if(r==NULL)
		return -1;

	r->state=state;
	platform_gpio_write(r->gpioPin,r->state);

	config_save(NULL);

	relay_notify_listeners(relayNumber,state);

	return r->state;
}

int ICACHE_FLASH_ATTR relay_toggle_state(int relayNumber){

	relay * r = find_relay(relayNumber);
	if(r==NULL)
		return -1;

	r->state=!r->state;

	platform_gpio_write(r->gpioPin,r->state);

	config_save(NULL);

	relay_notify_listeners(relayNumber,r->state);

	return r->state;
}

void ICACHE_FLASH_ATTR relay_init(){

	if(relay_init_ok)
		return;
	
	relay_init_ok=1;

	NODE_DBG("Relay init");

	//zero listeners
	int i;
	for(i=0;i<MAX_RELAY_LISTENERS;i++)
		listeners[i]=NULL;

	config_data * config  = config_read();

	if(config->relay_state.init_ok){

		#ifdef DEVELOP_VERSION
			NODE_DBG("Relay init ok");
			int j;
			for(j=0;j<RELAY_COUNT;j++)
				NODE_DBG("\tRelay #%d pin #%d state %d",j,config->relay_state.relay[j].gpioPin,config->relay_state.relay[j].state);

				NODE_DBG("");
		#endif

		int i;
		for(i=0;i<RELAY_COUNT;i++)
			platform_gpio_write(config->relay_state.relay[i].gpioPin,config->relay_state.relay[i].state);

		return;	
	}


	//relay #0 on pin 1	
	config->relay_state.relay[0].gpioPin=1;	
	config->relay_state.relay[0].state=0;
	platform_gpio_mode(1,PLATFORM_GPIO_OUTPUT,PLATFORM_GPIO_FLOAT);     

    //relay #1 on pin 2
	config->relay_state.relay[1].gpioPin=2;	
	config->relay_state.relay[1].state=0;
	platform_gpio_mode(2,PLATFORM_GPIO_OUTPUT,PLATFORM_GPIO_FLOAT);  	

    config->relay_state.init_ok=1;

    config_save(config);
}

int ICACHE_FLASH_ATTR relay_count(){

	return RELAY_COUNT;

}