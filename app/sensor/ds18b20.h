#ifndef __DS18b20_H
#define __DS18b20_H

typedef uint8_t boolean;
typedef uint8_t byte;

typedef struct{
	float temp;
} ds18b20_data;

int ICACHE_FLASH_ATTR ds18b20_read(ds18b20_data *data);

#endif
