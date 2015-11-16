/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "osapi.h"
#include "c_types.h"
#include "c_stdio.h"

#include "rofs.h"
#include "rofs_data.c"
#include "platform/common.h"

//Accessing the flash through the mem emulation at 0x40200000 is a bit hairy: All accesses
//*must* be aligned 32-bit accesses. Reading a short, byte or unaligned word will result in
//a memory exception, crashing the program.
//

//This function assumes both src and dst are 4byte aligned
//Reading 4bytes at a time speeds up reading from flash

void ICACHE_FLASH_ATTR memcpyAligned(char *dst,const uint8 *src, int len) {
	
	uint32_t *dst32 = (uint32_t*)dst;
	uint32_t *src32 = (uint32_t*)src;
	
	while(len>0){
		
		if(len>3){
			*dst32=*src32;	
			dst32++;
			src32++;			
			len-=4;			
		}
		else{			
			dst = (uint8_t*)dst32;
			uint32_t s = *src32;
			uint8_t *s8 = (uint8_t*)&s;
			int i;
			for(i=0;len>0;i++,len--,dst++) 
				*dst=s8[i];

			len=0;
		}

	}

}

RO_FILE* f_open(const char *fileName){

	const char *fName = fileName;

	if(*fName=='/') //skip leading /
		fName++;

	NODE_DBG("Trying to open file %s ",fName);
	NODE_DBG("FS Location %p ",ro_file_system);
	NODE_DBG("FS data Location %p ",rofs_data);

	int i=0;
	while(i<ro_file_system.count){

		const RO_FILE_ENTRY *entry = &ro_file_system.files[i];
		
		NODE_DBG("Checking %s",entry->name);

		if(os_strcmp(fName,entry->name)==0){

			RO_FILE *f = (RO_FILE*)os_malloc(sizeof(RO_FILE));
			f->file = entry;
			f->readPos=0;

			return f;
		}
		i++;

	}

	return NULL;
}



int f_read(RO_FILE *f,char * buff,int len){

	if(f==NULL)
		return 0;

	int bytesToRead = len;
	int remLen = f->file->size - f->readPos;
	if(remLen < len)
		bytesToRead=remLen;

	NODE_DBG("Reading %d bytes from %s",bytesToRead,f->file->name);
	memcpyAligned(buff,rofs_data+f->file->offset+f->readPos,bytesToRead);	
	//platform_flash_read(buff,(uint32_t)(rofs_data+f->file->offset+f->readPos),bytesToRead);
	f->readPos+=bytesToRead;
	
	f->eof= (f->readPos == f->file->size);	

	return bytesToRead;

}

void f_close(RO_FILE *file){

	if(file!=NULL)
		os_free(file);

}