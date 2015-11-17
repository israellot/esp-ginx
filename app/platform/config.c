#include "platform.h"
#include "c_stdio.h"
#include "c_string.h"
#include "c_stdlib.h"
#include "gpio.h"
#include "user_interface.h"
#include "user_config.h"
#include "driver/uart.h"
#include "config.h"

static config_data temp_data;
uint8_t local_up_to_date=0;
uint8_t init_ok = 0 ;

void config_save(config_data * data){

	NODE_DBG("Config save");

	platform_flash_erase_sector(CONFIG_SECTOR);
	
	if(data!=NULL){
		platform_flash_write((const void *)data,CONFIG_ADDRESS,sizeof(config_data));
		os_memcpy(temp_data,data,sizeof(config_data));
		local_up_to_date=1;
	}
	else{
		
		platform_flash_write((const void *)&temp_data,CONFIG_ADDRESS,sizeof(config_data));
	}		
}

static config_data * config_read_s(){

	platform_s_flash_read(&temp_data,CONFIG_ADDRESS,sizeof(config_data));
	local_up_to_date=1;

	return &temp_data;
}

config_data * config_read(){

	if(!init_ok){
		config_init();
		init_ok=1;
	}

	if(!local_up_to_date)
		config_read_s();

	return &temp_data;
} 



void config_init(){
		
	config_data * config = config_read_s();

	NODE_DBG("Config init");

	NODE_DBG("Config Magic =%08x",config->magic);

	if(config->magic == CONFIG_MAGIC)
		return;	

	NODE_DBG("Config writing 0s");

	os_memset(config,0,sizeof(config_data));

	config->magic = CONFIG_MAGIC;

	config_save(config);

	
}