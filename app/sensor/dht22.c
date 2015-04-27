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

#include "dht22.h"

#define DHT_DEBUG_ON

#ifdef DHT_DEBUG_ON
#define DHT_DBG NODE_DBG
#else
#define DHT_DBG
#endif

#define DHT_PIN 5

#define delay_ms(ms) os_delay_us(1000*ms)

int ICACHE_FLASH_ATTR dht22_read(dht_data *read){
        
    DHT_DBG("dht22_read");
       
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
        return 0;
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
       return 0;
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
       
        read->temp = temp_p;
        read->hum = hum_p;       

#ifdef DHT_DEBUG_ON

        char * buff = (char *)os_zalloc(64);

        c_sprintf(buff,"%f",read->temp);
        DHT_DBG("dht22_init temp : %s C",buff);

        os_memset(buff,0,64);
        c_sprintf(buff,"%f",read->hum);
        DHT_DBG("dht22_init humidity : %s %%",buff);

        os_free(buff);
       
#endif      
       
    }    

    return 1;
}


void ICACHE_FLASH_ATTR dht22_init(){

    DHT_DBG("dht22_init");

    //nothing to do actually   
   

}

