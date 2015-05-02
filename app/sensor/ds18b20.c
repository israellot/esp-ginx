// based on "How to measure temperature with your Arduino and a DS18B20"
// http://www.tweaking4all.com/hardware/arduino/arduino-ds18b20-temperature-sensor/

#include "ets_sys.h"
#include "osapi.h"
#include "c_stdio.h"
#include "mem.h"
#include "platform.h"
#include "user_interface.h"
#include "user_config.h"
#include "driver/onewire.h"

#include "ds18b20.h"

#define DS18B20_DEBUG_ON

#ifdef DS18B20_DEBUG_ON
#define DS18B20_DBG NODE_DBG
#else
#define DS18B20_DBG
#endif

#define DS18B20_PIN 4

#define delay_ms(ms) os_delay_us(1000*ms)

static int gpioPin;

int ICACHE_FLASH_ATTR ds18b20_read(ds18b20_data *read) {
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	byte addr[8];
	byte crc;
	float celsius;
	//float fahrenheit;

	DS18B20_DBG("Look for OneWire Devices on PIN =%d", gpioPin);

	if (!onewire_search(gpioPin, addr)) {
		DS18B20_DBG("No more addresses.");
		onewire_reset_search(gpioPin);
		delay_ms(250);
		return;
	}

	DS18B20_DBG("ADDRESS = %02x %02x %02x %02x %02x %02x %02x %02x",
			addr[0], addr[1], addr[2], addr[3],
			addr[4], addr[5], addr[6], addr[7]);

	// the first ROM byte indicates which chip
	switch (addr[0]) {
		case 0x10:
			DS18B20_DBG("  Chip = DS18S20");  // or old DS1820
			type_s = 1;
			break;
		case 0x28:
			DS18B20_DBG("  Chip = DS18B20");
			type_s = 0;
			break;
		case 0x22:
			DS18B20_DBG("  Chip = DS1822");
			type_s = 0;
			break;
		default:
			DS18B20_DBG("Device is not a DS18x20 family device.");
			return;
	} 

	onewire_reset(gpioPin);
	onewire_select(gpioPin, addr);
	onewire_write(gpioPin, 0x44, 1);
        // start conversion, use ds.write(0x44,1) with parasite power on at the end

	delay_ms(1000);

	present = onewire_reset(gpioPin);
	onewire_select(gpioPin, addr);
	onewire_write(gpioPin, 0xBE, 1);   // Read Scratchpad

	//DS18B20_DBG("  Present = %d", present);
	for ( i = 0; i < 9; i++) {         // we need 9 bytes
		data[i] = onewire_read(gpioPin);
		DS18B20_DBG("  DATA: %02x ", data[i]);
	}
	crc = onewire_crc8(data, 8);
	DS18B20_DBG(" CRC=%d ", crc);

	if (crc != data[8]) {
		DS18B20_DBG("CRC is not valid!");
		return;
	}

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
		//// default is 12 bit resolution, 750 ms conversion time
	}

	celsius = (float) raw / 16.0;
	//fahrenheit = celsius * 1.8 + 32.0;
	read->temp = celsius;

	DS18B20_DBG("Temperature = ");

	char *temp_string = (char*) os_zalloc(64);

	c_sprintf(temp_string, "%f", read->temp);
	DS18B20_DBG("  %s Celsius", temp_string);
	os_free(temp_string);

	//DS18B20_DBG(" %2.2f Fahrenheit", fahrenheit);

	return 1;
}

void ICACHE_FLASH_ATTR ds18b20_init(uint8_t pin) {
	DS18B20_DBG("ds18b20_init");

	gpioPin = DS18B20_PIN; // default pin
	if ( pin > 0 && pin < 15) gpioPin = pin;
	onewire_init(gpioPin);

	onewire_reset_search(gpioPin);
}
