/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "c_types.h"
#include "c_stdio.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "platform.h"
#include "user_interface.h"
#include "user_config.h"

#include "sensors.h"
#include "bmp180.h"
#include "dht22.h"
#include "ds18b20.h"

#define DS18B20_PIN 4

#define SENSOR_TASK_QUEUE_LEN 1
#define SENSOR_TASK_PRIORITY 1
#define SENSOR_TASK_SIG 981237
static os_event_t sensor_read_task_queue[SENSOR_TASK_QUEUE_LEN];
static os_timer_t sensor_read_timer;

sensor_data global_sensor_data;

void ICACHE_FLASH_ATTR sensors_get_data(sensor_data *return_data)
{
	//NODE_DBG("sensors_get_data");	
	*return_data = global_sensor_data;
	return;
}

static void ICACHE_FLASH_ATTR sensor_read_task(os_event_t *e){

	if(e->sig != SENSOR_TASK_SIG)
        return; //not our signal

    dht22_read(&global_sensor_data.dht22);
    bmp180_read(&global_sensor_data.bmp180); 

}

static void ICACHE_FLASH_ATTR sensor_read_timer_cb(void *arg){
	system_os_post(SENSOR_TASK_PRIORITY, SENSOR_TASK_SIG, (os_param_t) NULL );
}

void ICACHE_FLASH_ATTR sensors_init(){

	NODE_DBG("sensors_init");

	//init data
	os_memset(&global_sensor_data,0,sizeof(sensor_data));

	//init sensors
    ds18b20_init();
    dht22_init();
    bmp180_init(); 

	//Start os task
    system_os_task(sensor_read_task, SENSOR_TASK_PRIORITY ,sensor_read_task_queue, SENSOR_TASK_QUEUE_LEN);
    system_os_post(SENSOR_TASK_PRIORITY, SENSOR_TASK_SIG, (os_param_t) NULL );

	//arm sensor read timer
	os_memset(&sensor_read_timer,0,sizeof(os_timer_t));
	os_timer_disarm(&sensor_read_timer);
	os_timer_setfn(&sensor_read_timer, (os_timer_func_t *)sensor_read_timer_cb, NULL);
	os_timer_arm(&sensor_read_timer, 2000, 1);

}
