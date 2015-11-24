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

#include "espconn.h"
#include "http_parser.h"
#include "http_process.h"
#include "http_server.h"
#include "http_helper.h"
#include "user_config.h"
#include "cgi.h"

static http_connection connection_poll[MAX_CONNECTIONS];

void http_execute_cgi(http_connection *conn){

	if (conn->espConnection==NULL)
		return;

	if(conn->cgi.execute==NULL)
		return;

	conn->cgi.execute(conn);		

}

int ICACHE_FLASH_ATTR http_transmit(http_connection *c){

	NODE_DBG("Transmit Buffer");

	int len = (c->output.bufferPos - c->output.buffer);
	if(len>0 && len <= HTTP_BUFFER_SIZE){

			espconn_send(c->espConnection, (uint8_t*)(c->output.buffer),len);			
	}
	else{
		NODE_DBG("Wrong transmit size %d",len);
	}

	//free buffer
	http_reset_buffer(c);

	return len;
}

int ICACHE_FLASH_ATTR http_nwrite(http_connection *c,const char * message,size_t size){

	int rem;

		
	rem = c->output.buffer + HTTP_BUFFER_SIZE - c->output.bufferPos;
	//NODE_DBG("Response write %d, Buffer rem %d , buffer add : %p",size,rem,c->response.buffer);


	if(rem < size && c->cgi.function!=cgi_transmit){
		NODE_DBG("Buffer Overflow");

		//copy what's possible
		os_memcpy(c->output.bufferPos,message,rem);
		message+=rem; //advance message
		size-=rem; //adjust size
		c->output.bufferPos+=rem; //mark buffer pos

		struct cgi_transmit_arg *transmit_cgi = (struct cgi_transmit_arg*)os_malloc(sizeof(struct cgi_transmit_arg));
		os_memcpy(&transmit_cgi->previous_cgi,&c->cgi,sizeof(cgi_struct));
		c->cgi.function=cgi_transmit;

		transmit_cgi->data = (uint8_t*)os_malloc(size);
		os_memcpy(transmit_cgi->data,message,size);
		transmit_cgi->len=size;
		transmit_cgi->dataPos=transmit_cgi->data;

		c->cgi.data = transmit_cgi;
		c->cgi.done=0;

		//http_transmit(c);
		//goto process; //restart;

	}else{
		os_memcpy(c->output.bufferPos,message,size);
		c->output.bufferPos+=size;
	}	

	return 1;
}

int ICACHE_FLASH_ATTR http_write(http_connection *c,const char * message){
	
	size_t len = strlen(message);

	return http_nwrite(c,message,len);

}



// HEADER RELATED FUNCTIONS -------------------------------------------------------
// 
//
header * ICACHE_FLASH_ATTR http_get_header(http_connection *conn,const char* header){

	int i=0;
	while(conn->headers[i].key!=NULL){

		if(os_strcmp(conn->headers[i].key,header)==0)
		{
			if(conn->headers[i].value!=NULL)
				return &(conn->headers[i]);
			else 
				return NULL;
		}

		i++;
	}

	return NULL;
}

void ICACHE_FLASH_ATTR http_set_save_header(http_connection *conn,const char* header){

	NODE_DBG("http_parser will save header :%s",header);

	int j=0;
	while(conn->headers[j].key!=NULL && j< MAX_HEADERS) j++;
	if(j==MAX_HEADERS) return;

	conn->headers[j].key=(char*)header;
	conn->headers[j].save=0;

}

void ICACHE_FLASH_ATTR http_set_save_body(http_connection *conn){

	NODE_DBG("http_parser will save body");

	conn->body.save=1;

	//make sure body is free
	if(conn->body.data!=NULL){
		os_free(conn->body.data);
		conn->body.data=NULL;
	}
}




char * ICACHE_FLASH_ATTR http_url_get_field(http_connection *c,enum http_parser_url_fields field){

	if(c->url_parsed.field_set & (1<<field)){

		char * start = c->url + c->url_parsed.field_data[field].off;
		char * end = start + c->url_parsed.field_data[field].len -1;
		end++;
		*end=0;

		if(*start==0){
			if(field==UF_PATH)
				*start='/';

		}

		return start;
	}
	else
		return NULL;

}

char * ICACHE_FLASH_ATTR http_url_get_query_param(http_connection *c,char* param){
	NODE_DBG("http_url_get_query_param");
	if(!(c->url_parsed.field_set & (1<<UF_QUERY))) //return null if there's no query at all
		return NULL;


	//find field
	char *start = c->url + c->url_parsed.field_data[UF_QUERY].off;
	char * end = start + c->url_parsed.field_data[UF_QUERY].len -1;

	char *param_search = (char*)os_malloc(strlen(param)+2);
	os_strcpy(param_search,param);
	os_strcat(param_search,"=");

	NODE_DBG("search for : %s",param_search);

	char *ret=NULL;

	start--; //to start at ?
	while(start<=end){

		NODE_DBG("char : %c",*start);

		if(*start == '?' || *start == '&' || *start=='\0'){

			NODE_DBG("Is match?");

			start++;
			//check param name, case insensitive
			if(os_strcasecmp(param_search,start)==0){
				NODE_DBG("yes");
				//match
				start +=strlen(param_search);

				ret = start;

				//0 end string				
				while(*start!='\0' && *start!='&')
					start++;
				
				*start='\0';

				break;

			}

		}

		start++;

	}

	os_free(param_search);
	return ret;
}




char * ICACHE_FLASH_ATTR http_url_get_host(http_connection *c){
	return http_url_get_field(c,UF_HOST);
}
char * ICACHE_FLASH_ATTR http_url_get_path(http_connection *c){
	return http_url_get_field(c,UF_PATH);
}
char * ICACHE_FLASH_ATTR http_url_get_query(http_connection *c){
	return http_url_get_field(c,UF_QUERY);
}


void ICACHE_FLASH_ATTR http_parse_url(http_connection *c){
	
	memset(&c->url_parsed,0,sizeof(struct http_parser_url));

	http_parser_parse_url(
		(const char *)c->url,
		strlen(c->url),
		0,
		&c->url_parsed
		);

	#ifdef DEVELOP_VERSION

		NODE_DBG("Parse URL : %s",c->url);

		NODE_DBG("\tPORT: %d",c->url_parsed.port);			

		if(c->url_parsed.field_set & (1<<UF_SCHEMA)){
			NODE_DBG("\tSCHEMA: ");
			nprintf(c->url + c->url_parsed.field_data[UF_SCHEMA].off,c->url_parsed.field_data[UF_SCHEMA].len);
		}
		if(c->url_parsed.field_set & (1<<UF_HOST)){
			NODE_DBG("\tHOST: ");
			nprintf(c->url + c->url_parsed.field_data[UF_HOST].off,c->url_parsed.field_data[UF_HOST].len);
		}
		if(c->url_parsed.field_set & (1<<UF_PORT)){
			NODE_DBG("\tPORT: ");
			nprintf(c->url + c->url_parsed.field_data[UF_PORT].off,c->url_parsed.field_data[UF_PORT].len);
		}
		if(c->url_parsed.field_set & (1<<UF_PATH)){
			NODE_DBG("\tPATH: ");
			nprintf(c->url + c->url_parsed.field_data[UF_PATH].off,c->url_parsed.field_data[UF_PATH].len);
		}
		if(c->url_parsed.field_set & (1<<UF_QUERY)){
			NODE_DBG("\tQUERY: ");
			nprintf(c->url + c->url_parsed.field_data[UF_QUERY].off,c->url_parsed.field_data[UF_QUERY].len);
		}
		if(c->url_parsed.field_set & (1<<UF_FRAGMENT)){
			NODE_DBG("\tFRAGMENT: ");
			nprintf(c->url + c->url_parsed.field_data[UF_FRAGMENT].off,c->url_parsed.field_data[UF_FRAGMENT].len);
		}
		if(c->url_parsed.field_set & (1<<UF_USERINFO)){
			NODE_DBG("\tUSER INFO: ");
			nprintf(c->url + c->url_parsed.field_data[UF_USERINFO].off,c->url_parsed.field_data[UF_USERINFO].len);
		}


	#endif

}

//Parser callbacks
static int ICACHE_FLASH_ATTR on_message_begin(http_parser *parser){
	NODE_DBG("http_parser message begin");
	
	//nothing to do here
	return 0;
}

static int ICACHE_FLASH_ATTR on_url(http_parser *parser, const char *url, size_t length)
{	
	NODE_DBG("\nhttp_parser url: ");
	nprintf(url,length);

	NODE_DBG("http_parser method: %d",parser->method);
		

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	conn->state=HTTPD_STATE_ON_URL; //set state

	os_memcpy(conn->url,url,length); //copy url to connection info
	conn->url[length]=0; //null terminate string

	http_parse_url(conn);

	//execute cgi
	http_execute_cgi(conn);

	return 0;
}

static int ICACHE_FLASH_ATTR on_status(http_parser *parser, const char *url, size_t length)
{	
	NODE_DBG("http_parser status: ");
	nprintf(url,length);

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	conn->state=HTTPD_STATE_ON_STATUS; //set state

	//execute cgi again
	http_execute_cgi(conn);

	return 0;
}


static int ICACHE_FLASH_ATTR on_header_field(http_parser *parser, const char *at, size_t length)
{
	NODE_DBG("http_parser header: ");
	nprintf(at,length);

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	int i=0;
	while(conn->headers[i].key!=NULL){		
		if(os_strncmp(conn->headers[i].key,at,length)==0){
			NODE_DBG("marking header to save");
			//match header			
			//turn on save header			
			conn->headers[i].save=1;

			break;
		}

		i++;
	}
	
	return 0;
}

static int ICACHE_FLASH_ATTR on_header_value(http_parser *parser, const char *at, size_t length)
{
	NODE_DBG("http_parser header value: ");
	nprintf(at,length);
	
	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	int i=0;
	while(conn->headers[i].key!=NULL){		
		if(conn->headers[i].save==1){
			NODE_DBG("saving header");

			conn->headers[i].value=(char *) os_malloc(length+1);
			os_memcpy(conn->headers[i].value,at,length);
			conn->headers[i].value[length]=0; //terminate string;

			conn->headers[i].save=0;

			break;

			
		}

		i++;
	}
	
	return 0;
}

static int ICACHE_FLASH_ATTR on_headers_complete(http_parser *parser){
	NODE_DBG("\nhttp_parser headers complete");

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	conn->state = HTTPD_STATE_HEADERS_END; //set state

	//execute cgi again
	http_execute_cgi(conn);
	
}

static int ICACHE_FLASH_ATTR on_body(http_parser *parser, const char *at, size_t length)
{
	NODE_DBG("\nhttp_parser body: ");
	nprintf(at,length);

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	conn->state = HTTPD_STATE_ON_BODY; //set state

	if(conn->body.save){

		if(conn->body.data==NULL){

			NODE_DBG("saving body len %d",length);

			conn->body.data = (char *) os_malloc(length+1);		
			os_memcpy(conn->body.data,at,length);	
			conn->body.len = length;
			conn->body.data[length]=0;
		}
		else{
			//assuming body can come in different tcp packets, this callback will be called
			//more than once
			
			NODE_DBG("appending body len %d",length);

			size_t newLenght = conn->body.len+length;
			char * newBuffer = (char *) os_malloc(newLenght+1);
			os_memcpy(newBuffer,conn->body.data,conn->body.len); //copy previous data
			os_memcpy(newBuffer+conn->body.len,at,length); //copy new data
			os_free(conn->body.data); //free previous
			conn->body.data=newBuffer;
			conn->body.len=newLenght;
			conn->body.data[newLenght]=0;
		}

	}
	
	//execute cgi again
	http_execute_cgi(conn);

	return 0;	
}

//special callback like function to pass ws data to cgi
static int ICACHE_FLASH_ATTR on_websocket_data(http_parser *parser, char *data, size_t length)
{
	NODE_DBG("\non_websocket_data : ");
	int i;
	for(i=0;i<length;i++)
		os_printf("%02X",data[i]);
	os_printf("\r\n");

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	conn->state = HTTPD_STATE_WS_DATA; //set state

	
	conn->body.data = (char *)data;
	conn->body.len = length;	
	
	//execute cgi again
	http_execute_cgi(conn);

	conn->body.data=NULL;
	conn->body.len=0;

	return 0;	
}



static int ICACHE_FLASH_ATTR on_message_complete(http_parser *parser){

	NODE_DBG("\nhttp_parser message complete");

	//grab the connection
	http_connection * conn = (http_connection *)parser->data;

	conn->state = HTTPD_STATE_BODY_END; //set state

	//execute cgi again
	http_execute_cgi(conn);		

	//free body
	if(conn->body.save==1 && conn->body.data!=NULL){
		NODE_DBG("freeing body memory");
		os_free(conn->body.data);
		conn->body.len=0;	
	}	

	return 0;
}

//Looks up the connection for a specific esp connection
static http_connection ICACHE_FLASH_ATTR *http_process_find_connection(void *arg) {
	int i;

	if(arg==0){
		NODE_DBG("http_process_find_connection: Couldn't find connection for %p", arg);
		return NULL;
	}

	for(i=0;i<MAX_CONNECTIONS;i++) if(connection_poll[i].espConnection==arg) break;

	if(i<MAX_CONNECTIONS){
		return &connection_poll[i];
	}else{
		NODE_DBG("http_process_find_connection: Couldn't find connection for %p", arg);
		return NULL; //WtF?
	}
		
}

void ICACHE_FLASH_ATTR http_process_free_connection(http_connection *conn){

	http_reset_buffer(conn);

	conn->espConnection=NULL;	
	
	conn->cgi.function=NULL;
	conn->cgi.data=NULL;
	conn->cgi.argument=NULL;

	//free headers	
	int j=0;
	while(j<MAX_HEADERS){
		
		if(conn->headers[j].value!=NULL 
			&& (conn->headers[j].value!=conn->headers[j].key))
		{
			os_free(conn->headers[j].value);	
			conn->headers[j].value=NULL;
		}
		conn->headers[j].key=NULL;

		j++;
	}

	//free buffer
	os_free(conn->output.buffer);
}

//esp conn callbacks
static void ICACHE_FLASH_ATTR http_process_sent_cb(void *arg) {

	NODE_DBG("\nhttp_process_sent_cb, conn %p",arg);	
	
	http_connection *conn = http_process_find_connection(arg);
	
	if(conn==NULL)
			return;	
	
	if (conn->cgi.done==1) { //Marked for destruction?
		NODE_DBG("Conn %p is done. Closing.", conn->espConnection);
		espconn_disconnect(conn->espConnection);
		http_process_free_connection(conn);				
		return; //No need to execute cgi again
	}//
	
	if(conn->parser.upgrade){
		conn->state = HTTPD_STATE_WS_DATA_SENT; //set state
	}	

	http_execute_cgi(conn);		
		
}

//Callback called when there's data available on a socket.
static void ICACHE_FLASH_ATTR http_process_received_cb(void *arg, char *data, unsigned short len) {
	NODE_DBG("\nhttp_process_received_cb, len: %d",len);

	http_connection *conn = http_process_find_connection(arg);
	if(conn==NULL){
		espconn_disconnect(arg);
		return;
	}

	//pass data to http_parser	
	size_t nparsed = http_parser_execute(
		&(conn->parser),
		&(conn->parser_settings),
		data,
		len);

	if (conn->parser.upgrade) {
  		/* handle new protocol */
		on_websocket_data(&conn->parser,data,len);

	} else if (nparsed != len) {
	  /* Handle error. Usually just close the connection. */
		espconn_disconnect(conn->espConnection);
		http_process_free_connection(conn);	
	}
	
}


static void ICACHE_FLASH_ATTR http_process_disconnect_cb(void *arg) {

	NODE_DBG("Disconnect conn=%p",arg);


	http_connection *conn = http_process_find_connection(arg);
	if(conn!=NULL){

		//is it a client connection?
		if(conn->espConnection== &conn->client_connection){			
			//tell parser about EOF			
			http_parser_execute(
				&(conn->parser),
				&(conn->parser_settings),
				NULL,
				0);
		}

		if (conn->parser.upgrade) {
			conn->state=HTTPD_STATE_WS_CLIENT_DISCONNECT;
			http_execute_cgi(conn);
		}

		http_process_free_connection(conn);
	}
	else{

		//find connections that should be closed
		int i;
		for(i=0;i<MAX_CONNECTIONS;i++)
		{
			struct espconn *conn = connection_poll[i].espConnection;

			if(conn!=NULL){
				if(conn->state==ESPCONN_NONE || conn->state >=ESPCONN_CLOSE){
				//should close

				//is it a client connection? If yes, don't free
				if(&connection_poll[i].client_connection != connection_poll[i].espConnection)
				{
					http_process_free_connection(&connection_poll[i]);
				}				
					
			}
					
			}
		}


	}

	

}


static void ICACHE_FLASH_ATTR http_process_reconnect_cb(void *arg, sint8 err) {
	//some error

	NODE_DBG("Reconnect conn=%p err %d",arg,err);	

	struct espconn * conn = (struct espconn *)arg;
	conn->state=ESPCONN_CLOSE; // make sure of that 		

	http_process_disconnect_cb(arg);	
}

http_connection ICACHE_FLASH_ATTR * http_new_connection(uint8_t in,struct espconn *conn){

	int i;
	//Find empty connection in pool
	for (i=0; i<MAX_CONNECTIONS; i++) if (connection_poll[i].espConnection==NULL) break;
	

	if (i>=MAX_CONNECTIONS) {
		NODE_DBG("Connection pool overflow!");	

		if(conn!=NULL){			
			espconn_disconnect(conn);
		}

		return;
	}	

	NODE_DBG("\nNew connection, conn=%p, pool slot %d", conn, i);

	if(conn!=NULL){		
		connection_poll[i].espConnection=conn;
		connection_poll[i].espConnection->reverse=&connection_poll[i];
	}

	//allocate buffer
	connection_poll[i].output.buffer = (uint8_t *)os_zalloc(HTTP_BUFFER_SIZE);

	//zero headers again- for sanity
	int j=0;
	while(j<MAX_HEADERS){

		if(connection_poll[i].headers[j].value!=NULL
			&& (connection_poll[i].headers[j].value!=connection_poll[i].headers[j].key)){
			os_free(connection_poll[i].headers[j].value);	
			connection_poll[i].headers[j].value=NULL;
		}
		connection_poll[i].headers[j].key=NULL;
		
		j++;
	}

	//mark cgi as not done
	connection_poll[i].cgi.done=0;
	
	//free response buffer again
	http_reset_buffer(&connection_poll[i]);
		
	//init body
	connection_poll[i].body.len=0;
	connection_poll[i].body.save=0;
	connection_poll[i].body.data=NULL;

	//reset parser	
	http_parser_settings_init(&(connection_poll[i].parser_settings));
	connection_poll[i].parser_settings.on_message_begin=on_message_begin;
	connection_poll[i].parser_settings.on_url=on_url;
	connection_poll[i].parser_settings.on_header_field=on_header_field;
	connection_poll[i].parser_settings.on_header_value=on_header_value;
	connection_poll[i].parser_settings.on_headers_complete=on_headers_complete;
	connection_poll[i].parser_settings.on_body=on_body;
	connection_poll[i].parser_settings.on_message_complete=on_message_complete;

	//attach httpd connection to data (socket info) so we may retrieve it easily inside parser callbacks
	connection_poll[i].parser.data=(&connection_poll[i]);

	//init parser
	if(in){		
		http_parser_init(&(connection_poll[i].parser),HTTP_REQUEST);

		//register espconn callbacks
		espconn_regist_recvcb(conn, http_process_received_cb);
		espconn_regist_reconcb(conn, http_process_reconnect_cb);
		espconn_regist_disconcb(conn, http_process_disconnect_cb);
		espconn_regist_sentcb(conn, http_process_sent_cb);
	}
	else{		
		http_parser_init(&(connection_poll[i].parser),HTTP_RESPONSE);

		connection_poll[i].espConnection = &connection_poll[i].client_connection;

		connection_poll[i].espConnection->reverse=&connection_poll[i]; //set reverse object 

		connection_poll[i].espConnection->type=ESPCONN_TCP;
		connection_poll[i].espConnection->state=ESPCONN_NONE;
		connection_poll[i].espConnection->proto.tcp = &connection_poll[i].client_tcp;

		//register espconn callbacks
		espconn_regist_recvcb(connection_poll[i].espConnection, http_process_received_cb);
		espconn_regist_reconcb(connection_poll[i].espConnection, http_process_reconnect_cb);
		espconn_regist_disconcb(connection_poll[i].espConnection, http_process_disconnect_cb);
		espconn_regist_sentcb(connection_poll[i].espConnection, http_process_sent_cb);		
	}
	
	
	espconn_regist_time(conn,30,0);

	return &connection_poll[i];

}

static void ICACHE_FLASH_ATTR http_process_init(){

	int i;
	//init connection pool
	for (i=0; i<MAX_CONNECTIONS; i++) {
		//init with 0s
		os_memset(&connection_poll[i],0,sizeof(http_connection));

	}

}

