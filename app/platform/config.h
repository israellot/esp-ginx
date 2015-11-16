#ifndef __CONFIG_H
#define __CONFIG_H

#include "cpu_esp8266.h"
#include "driver/relay.h"

#define CONFIG_SECTOR (FLASH_SEC_NUM - 6) //last sector
#define CONFIG_ADDRESS (INTERNAL_FLASH_START_ADDRESS+CONFIG_SECTOR*SPI_FLASH_SEC_SIZE)

#define CONFIG_MAGIC 0x1FAF15FB				 



//4-ALIGN EVERYTHING !!

typedef struct {

	uint8_t gpioPin;
	uint8_t state;

	uint8_t pad[2];
} relay;

typedef struct {

	uint32_t magic;

	struct{
		relay relay[RELAY_COUNT];
		uint32_t init_ok;
	} relay_state;

} config_data;

void config_save(config_data * data);
config_data * config_read();
void config_init();



#endif