/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#ifndef __ROFS_H
#define __ROFS_H

#include "c_stdlib.h"
#include "c_stdint.h"
#include "c_stddef.h"
#include "user_interface.h"
#include "user_config.h"

typedef struct {

	size_t size;
	uint8_t gzip;
	const char *name;	
	uint32_t offset;	

} RO_FILE_ENTRY;

typedef struct {

	size_t count;
	RO_FILE_ENTRY files[];

} RO_FS;


typedef struct {

	const RO_FILE_ENTRY *file;
	uint32_t readPos;
	uint8_t eof;

} RO_FILE;

RO_FILE* ICACHE_FLASH_ATTR f_open(const char *fileName);
int ICACHE_FLASH_ATTR f_read(RO_FILE *file,char * buff,int len);
void ICACHE_FLASH_ATTR f_close(RO_FILE *file);

#endif