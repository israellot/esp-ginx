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
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "c_stdio.h"

#include "websocket.h"



void ICACHE_FLASH_ATTR ws_write_frame(ws_frame *frame,write_function w,void *arg){

    uint8_t byte;

    //fin
    byte = 0x80 ; //set first bit
    byte |= frame->TYPE; //set op code

    //write first byte
    w(&byte,1,arg);

    
    byte=0;
    if(frame->SIZE < 126)
    {
        byte = frame->SIZE;
        w(&byte,1,arg); //transmit size
    }
    else if(frame->SIZE <  0xFFFF){ //can we fit it into an uint16?
        byte=126;
        w(&byte,1,arg); //transmit extended lenght indicator

        //send lower 2 bytes of the uint64 len 
        byte = 0xFF & frame->SIZE >> 8;
        w(&byte,1,arg);

        byte = 0xFF & frame->SIZE;
        w(&byte,1,arg);        

    }
    else{
        byte=127;
        w(&byte,1,arg); //transmit extended lenght indicator

        //transmit the whole uint64
        w((char *)&frame->SIZE,8,arg);
    }

    //transmit the data
    w(frame->DATA,frame->SIZE,arg);

}

void ICACHE_FLASH_ATTR ws_output_frame(ws_frame *frame,enum ws_frame_type type,char * payload,size_t payloadSize){

    
    //is FIN?
    frame->FIN = 1; //we don't support continuation frames, yet

    frame->SIZE = payloadSize;

    frame->TYPE = type;    

    frame->MASKED=0; //server shouldn't mask according to https://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-17

    frame->DATA = payload;

}

void ICACHE_FLASH_ATTR ws_parse_frame(ws_frame *frame,char * data,size_t dataLen){


    uint8_t byte = data[0];

    //is FIN?
    frame->FIN = byte & 0x80;

    //opcode
    frame->TYPE = byte & 0x0F;


    if( (frame->TYPE > 0x03 && frame->TYPE < 0x08) || frame->TYPE > 0x0B )
    {
       NODE_DBG("\tInvalid frame type %02X",frame->TYPE);
       frame->TYPE=WS_INVALID;
       return;
    }

    //mask
    byte = data[1];
    frame->MASKED = byte & 0x80;

    //frame size
    frame->SIZE = byte & 0x7F;

    int offset = 2;
    if(frame->SIZE == 126)
    {
        frame->SIZE=0;
        frame->SIZE = data[3];                 //LSB
        frame->SIZE |= (uint64_t)data[2] << 8; //MSB
        offset=4;
    }
    else if(frame->SIZE == 127){
        frame->SIZE=0;
        frame->SIZE |= (uint64_t)data[2] << 56; 
        frame->SIZE |= (uint64_t)data[3] << 48; 
        frame->SIZE |= (uint64_t)data[4] << 40; 
        frame->SIZE |= (uint64_t)data[5] << 32; 
        frame->SIZE |= (uint64_t)data[6] << 24; 
        frame->SIZE |= (uint64_t)data[7] << 16; 
        frame->SIZE |= (uint64_t)data[8] <<  8; 
        frame->SIZE |= (uint64_t)data[9] ; 
        offset=10;
    }

    if(frame->MASKED){

        //read mask key
        char mask[4];
        mask[0]=data[offset];
        mask[1]=data[offset+1];
        mask[2]=data[offset+2];
        mask[3]=data[offset+3];

        offset+=4;

        //unmaks data
        uint64_t i;
        for(i=0;i<frame->SIZE;i++){
            data[i+offset] ^= mask[i % 4];                    
        }

    }

    frame->DATA = &data[offset];

}



ws_frame * ICACHE_FLASH_ATTR ws_make_pong(ws_frame *ping){

    ws_output_frame(ping,WS_PONG,ping->DATA,ping->SIZE);

    return ping;
}