/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "ets_sys.h"
#include "osapi.h"
#include "c_stdio.h"
#include "mem.h"
#include "gpio.h"
#include "platform.h"
#include "user_interface.h"
#include "user_config.h"

#include "driver/dht22.h"

//#define DHT_DEBUG_ON

#ifdef DHT_DEBUG_ON
#define DHT_DBG NODE_DBG
#else
#define DHT_DBG
#endif

#define DHT_PIN 4 

#define delay_ms(ms) os_delay_us(1000*ms)

static dht_data read;

static os_timer_t dht_timer;
os_event_t taskQueue[2];
#define SIG_DHT 22

dht_data ICACHE_FLASH_ATTR dht22_read(){
    return read;
}



static void ICACHE_FLASH_ATTR dht22_task(os_event_t *e){

    DHT_DBG("dht22_task");

    if(e->sig != SIG_DHT)
        return;

    //put dht pin on output mode with pullup
    platform_gpio_mode(DHT_PIN,PLATFORM_GPIO_OUTPUT,PLATFORM_GPIO_PULLUP);
    

    uint64_t data;   
    os_memset(&data,0,sizeof(uint64_t));

    //init sequence
    platform_gpio_write(DHT_PIN,1);
    delay_ms(5);
    platform_gpio_write(DHT_PIN,0); 
    delay_ms(1);
    platform_gpio_write(DHT_PIN,1);
    
    //enable input reading
    platform_gpio_mode(DHT_PIN,PLATFORM_GPIO_INPUT,PLATFORM_GPIO_PULLUP);

    //find ACK start
    //wait pin to drop
    int timeout = 100;
    while(platform_gpio_read(DHT_PIN) == 1 && timeout > 0)
    {
        os_delay_us(5);
        timeout-=5;
    }
    if(timeout==0){
        DHT_DBG("\tACK start timeout");
        return;
    }

    //find ACK end
    //wait pin to rise
    timeout = 100;
    while(platform_gpio_read(DHT_PIN) == 0 && timeout > 0)
    {
        os_delay_us(10);
        timeout-=10;
    }
    if(timeout==0){
        DHT_DBG("\tACK end timeout");
       return;
    }

    //continue to read data
       

   while(1){

        //wait pin to go low, signaling next bit
        timeout = 200;
        while(platform_gpio_read(DHT_PIN) == 1 && timeout > 0)
        {
            os_delay_us(5);
            timeout-=5;
        }
        if(timeout==0){
            //NODE_DBG("\tdata signal timeout");
            break;
        }

        //wait start of bit    
        timeout = 200;
        while(platform_gpio_read(DHT_PIN) == 0 && timeout > 0)
        {
            os_delay_us(5);
            timeout-=5;
        }
        if(timeout==0){
            //NODE_DBG("\tdata bit start timeout");
            break;
        }

        //mearure pulse lenght        
        timeout = 200;
        while(platform_gpio_read(DHT_PIN) == 1 && timeout > 0)
        {
            os_delay_us(5);
            timeout-=5;
        }
        if(timeout==0){
            //NODE_DBG("\tdata bit read timeout");
            break;
        }

        int pulseWidth = 200 - timeout;

        //shift data                
        data <<= 1;

        if(pulseWidth > 40)
        {
            //bit is 1
            data |=1;
        }
        else{
            //bit is 0
            //nothing to do
        }

   }

   //do the math

   

   float temp_p, hum_p;

   uint8_t data_part[5];
   data_part[4] = ((uint8_t*)&data)[0];
   data_part[3] = ((uint8_t*)&data)[1];
   data_part[2] = ((uint8_t*)&data)[2];
   data_part[1] = ((uint8_t*)&data)[3];
   data_part[0] = ((uint8_t*)&data)[4];
  

   int checksum = (data_part[0] + data_part[1] + data_part[2] + data_part[3]) & 0xFF;
    if (data_part[4] == checksum) {
        // yay! checksum is valid 
        
        hum_p = data_part[0] * 256 + data_part[1];
        hum_p /= 10;

        temp_p = (data_part[2] & 0x7F)* 256 + data_part[3];
        temp_p /= 10.0;
        if (data_part[2] & 0x80)
            temp_p *= -1;
       
        read.temp = temp_p;
        read.hum = hum_p;
        

    }
    
    
}

void ICACHE_FLASH_ATTR dht22_timer_cb(){
    system_os_post(1, SIG_DHT, NULL ); //schedule task to run
}

void ICACHE_FLASH_ATTR dht22_init(){

    DHT_DBG("dht22_init");

    os_timer_disarm(&dht_timer);
    os_timer_setfn(&dht_timer, (os_timer_func_t *)dht22_timer_cb, NULL);
    os_timer_arm(&dht_timer, 5000, 1);

    //Start os task
    system_os_task(dht22_task, 1 ,taskQueue, 2);
    system_os_post(1, SIG_DHT, NULL );

}

