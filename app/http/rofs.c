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

//Accessing the flash through the mem emulation at 0x40200000 is a bit hairy: All accesses
//*must* be aligned 32-bit accesses. Reading a short, byte or unaligned word will result in
//a memory exception, crashing the program.
//


//Copies len bytes over from dst to src, but does it using *only*
//aligned 32-bit reads. Yes, it's no too optimized but it's short and sweet and it works.

void ICACHE_FLASH_ATTR memcpyAligned(char *dst,const uint8 *src, int len) {
	int x;
	int w, b;
	for (x=0; x<len; x++) {
		b=((int)src&3);
		w=*((int *)(src-b));
		if (b==0) *dst=(w>>0);
		if (b==1) *dst=(w>>8);
		if (b==2) *dst=(w>>16);
		if (b==3) *dst=(w>>24);
		dst++; src++;
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
	//spi_flash_read((uint32)&rofs_data + f->file->offset + f->readPos,(uint32*)buff,bytesToRead/4);

	f->readPos+=bytesToRead;
	
	f->eof= (f->readPos == f->file->size);	

	return bytesToRead;

}

void f_close(RO_FILE *file){

	if(file!=NULL)
		os_free(file);

}