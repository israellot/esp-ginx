/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
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

#include "driver/i2c_master.h"
#include "bmp180.h"

#define BMP_DEBUG_ON

#ifdef BMP_DEBUG_ON
#define BMP_DEBUG NODE_DBG
#else
#define BMP_DEBUG
#endif

#define BMP180_SDA_PIN 3
#define BMP180_SCL_PIN 4


#define BMP180_ADDRESS   0xee 
#define CONVERSION_TIME 5
#define BMP180_CTRL_REG 0xf4
#define BMP180_DATA_REG 0xf6
#define BMP180_VERSION_REG 0xd1

#define BMP_CMD_MEASURE_TEMP       (0x2E) // Max conversion time 4.5ms
#define BMP_CMD_MEASURE_PRESSURE_0 (0x34) // Max conversion time 4.5ms (OSS = 0)
//#define BMP_CMD_MEASURE_PRESSURE_1 (0x74) // Max conversion time 7.5ms (OSS = 1)
//#define BMP_CMD_MEASURE_PRESSURE_2 (0xB4) // Max conversion time 13.5ms (OSS = 2)
//#define BMP_CMD_MEASURE_PRESSURE_3 (0xF4) // Max conversion time 25.5ms (OSS = 3)

static struct bmp_cal_data{

	int16_t ac1;
	int16_t ac2;
	int16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1;
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;

}bmp_cal_data;

static uint8_t bmp_init_ok=0;

static uint8_t ICACHE_FLASH_ATTR bmp180_write_wait_ack(uint8_t byte,uint8_t start){

	//i2c start signal
	if(start)
		i2c_master_start();

	i2c_master_writeByte(byte); //address the bmp
	if(!i2c_master_checkAck()){

		BMP_DEBUG("ack fail for byte %02x", byte);
		i2c_master_stop();
		return 0;
	}
	return 1;
}

static uint8_t ICACHE_FLASH_ATTR bmp180_read_byte_ack(uint8_t ack_level){

	uint8_t byte = i2c_master_readByte();
	i2c_master_setAck(ack_level);
	i2c_master_send_ack();

	return byte;
}

//read 16 bits
static int16_t  ICACHE_FLASH_ATTR bmp180_read_reg_16(uint8_t reg) {

	//BMP_DEBUG("bmp180_read_reg");

	if(bmp180_write_wait_ack(BMP180_ADDRESS,1))
	if(bmp180_write_wait_ack(reg,0))
	if(bmp180_write_wait_ack(BMP180_ADDRESS+1,1))
	{
		//read bytes
		uint8_t msb = bmp180_read_byte_ack(1);
		uint8_t lsb = bmp180_read_byte_ack(0); 	

		i2c_master_stop();

		int16_t res = msb << 8;
	 	res += lsb;	

	 	//BMP_DEBUG("\tvalue: %08x",res);

	  	return res;	
	}
	


	return 0; //if we got here means there was some error

}

static int16_t ICACHE_FLASH_ATTR bmp180_read_raw(uint8_t cmd) {

	//BMP_DEBUG("bmp180_read_raw");

	if(bmp180_write_wait_ack(BMP180_ADDRESS,1))	
	if(bmp180_write_wait_ack(BMP180_CTRL_REG,0))
	if(bmp180_write_wait_ack(cmd,0))
	{
		//BMP_DEBUG("bmp180_read_raw init ok");

		i2c_master_stop();
		os_delay_us(CONVERSION_TIME*1000);
		int16_t res = bmp180_read_reg_16(BMP180_DATA_REG);
  		return res;
	}

  	return 0; //if we got here means there was some error
}


int ICACHE_FLASH_ATTR bmp180_read(bmp_data *data){

  	if(!bmp_init_ok)
  		return 0;

  	int32_t UT;
  	uint16_t UP;
	int32_t B3, B5, B6;
	uint32_t B4, B7;
	int32_t X1, X2, X3;
	int32_t T, P;

	UT = bmp180_read_raw(BMP_CMD_MEASURE_TEMP);
	UP = bmp180_read_raw(BMP_CMD_MEASURE_PRESSURE_0);

	//calc temp
	X1 = (UT - (int32_t)bmp_cal_data.ac6) * ((int32_t)bmp_cal_data.ac5) >> 15;
	X2 = ((int32_t)bmp_cal_data.mc << 11) / (X1 + (int32_t)bmp_cal_data.md); 
	B5 = X1 + X2;
	T  = (B5+8) >> 4;
	
	//calc pressure
	B6 = B5 - 4000;
	X1 = ((int32_t)bmp_cal_data.b2 * ((B6 * B6) >> 12)) >> 11;
	X2 = ((int32_t)bmp_cal_data.ac2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((int32_t)bmp_cal_data.ac1 * 4 + X3) + 2) / 4;
	X1 = ((int32_t)bmp_cal_data.ac3 * B6) >> 13;
	X2 = ((int32_t)bmp_cal_data.b1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = ((uint32_t)bmp_cal_data.ac4 * (uint32_t)(X3 + 32768)) >> 15;
	B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL);

	if (B7 < 0x80000000) {
	P = (B7 * 2) / B4;
	} else {
	P = (B7 / B4) * 2;
	}

	X1 = (P >> 8) * (P >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * P) >> 16;
	P = P + ((X1 + X2 + (int32_t)3791) >> 4);

	data->temp=T;
	data->press=P;

#ifdef BMP_DEBUG_ON

	BMP_DEBUG("bmp180 pressure : %d kPa",data->press);
	BMP_DEBUG("bmp180 temperature : %d mC",data->temp);
	

#endif

	return 1;


}

int ICACHE_FLASH_ATTR bmp180_init(){

	BMP_DEBUG("bmp180_init");

	//setup i2c
	i2c_master_gpio_init(BMP180_SDA_PIN,BMP180_SCL_PIN);
	i2c_master_init();

	if(!bmp180_read_reg_16(BMP180_VERSION_REG))
	{
		BMP_DEBUG("failed reading bmp version");
		return 0;
	}

	//calibration data
	bmp_cal_data.ac1 = bmp180_read_reg_16(0xAA);				 
	bmp_cal_data.ac2 = bmp180_read_reg_16(0xAC);
	bmp_cal_data.ac3 = bmp180_read_reg_16(0xAE);
	bmp_cal_data.ac4 = bmp180_read_reg_16(0xB0);
	bmp_cal_data.ac5 = bmp180_read_reg_16(0xB2);
	bmp_cal_data.ac6 = bmp180_read_reg_16(0xB4);
	bmp_cal_data.b1 =  bmp180_read_reg_16(0xB6);
	bmp_cal_data.b2 =  bmp180_read_reg_16(0xB8);
	bmp_cal_data.mb =  bmp180_read_reg_16(0xBA);
	bmp_cal_data.mc =  bmp180_read_reg_16(0xBC);
	bmp_cal_data.md =  bmp180_read_reg_16(0xBE);

	bmp_init_ok=1;
	return 1;
}