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
#include "http_server.h"
#include "http_helper.h"
#include "http_process.h"
#include "user_config.h"

#include "cgi.h"



static http_server_config server_config;

// Called after cgi execution to flush any data
void ICACHE_FLASH_ATTR http_send_response(http_connection * conn){

	NODE_DBG("http_send_response");
	int sent = http_transmit(conn);

	//any data sent?
	if(sent == 0){
		//if there was no data sent and cgi is done, we should destroy the connection
		if (conn->cgi.done==1) { 
			NODE_DBG("Conn %p is done. Closing.", conn->espConnection);
			espconn_disconnect(conn->espConnection);
			http_process_free_connection(conn);				
			return; 
		}

	}	

}

int ICACHE_FLASH_ATTR http_server_flag_check(http_connection *c,http_server_url *url){
	NODE_DBG("http_server_flag_check");
	int r=HTTPD_CGI_NEXT_RULE;

	if(url->method!=HTTP_ANY_METHOD){
		c->cgi.argument=&url->method;
		r = cgi_enforce_method(c);
		if(r==HTTPD_CGI_DONE) return r; //return now as request already failed
	}
	
	if(url->flags & NEED_BODY){
		r = cgi_enforce_body(c);
		if(r==HTTPD_CGI_DONE) return r; //return now as request already failed
	}
	
	return r;
}

// CGI dispatcher for the http server
//
int ICACHE_FLASH_ATTR http_server_cgi_execute(http_connection * conn){

	NODE_DBG("http_execute_cgi");

	//request is finished and we should start sending response
	//if not final, cgi should not expect data to be sent
	uint8_t final = conn->state==HTTPD_STATE_BODY_END;

	if(conn->cgi.done)
		return 0;

	//Any CGI function already attached?
	if(conn->cgi.function!=NULL){ 

		NODE_DBG("Executing previous cgi ");

		int r;
		r=conn->cgi.function(conn);
		NODE_DBG("Cgi return is %d", r);
		
		if(r==HTTPD_CGI_DONE){
			conn->cgi.function=NULL; //mark for destruction
			conn->cgi.done=1;						
		}
		
		if(final)
			http_send_response(conn);

		if(r==HTTPD_CGI_MORE ||r==HTTPD_CGI_DONE){
			return 0;
		}		

	}
	else{
		//find which cgi to execute base on the url

		NODE_DBG("Finding cgi to execute ");

		int i=0;
		while (server_config.urls[i]->url!=NULL && conn->url!=NULL) {

			
			const char * url = server_config.urls[i]->url; 
			//NODE_DBG("Checking url %s against %s",server_config.urls[i]->url,conn->url);

			int match=0;
			
			char *url_path = http_url_get_path(conn);
			NODE_DBG("Url path: %s",url_path);

			if (os_strcmp(url, url_path)==0) match=1;

			if (url[os_strlen(url)-1]=='*' &&
					os_strncmp(url, url_path, os_strlen(url)-1)==0) match=1;

			if (match) {
				NODE_DBG("Url match index %d", i);

				//general rules go here				
				int r = http_server_flag_check(conn,server_config.urls[i]);
				
				if(r!=HTTPD_CGI_DONE){
					//if passed general rules
					//execute cgi
					conn->cgi.function=server_config.urls[i]->cgiFunction;
					conn->cgi.argument=server_config.urls[i]->cgiArgument;
					r=conn->cgi.function(conn);
				}		
								
				NODE_DBG("Cgi return is %d", r);				

				if(r==HTTPD_CGI_DONE){ // cgi is done, nothing more to do
					conn->cgi.function=NULL; //mark for destruction
					conn->cgi.done=1;
					http_send_response(conn); //force sending the response regardless of the state we are
					return 0; //exit loop and return
				}
				
				if(final) // if we are in final state, try to send the response for any return case
					http_send_response(conn);				

				if(r==HTTPD_CGI_NEXT_RULE){ //cgi signal that we should allow other cgi to execute on the same url
					conn->cgi.function=NULL;	//clear cgi function					
				}

				if(r==HTTPD_CGI_NOTFOUND){	//the cgi doesn't recognize the request				
					conn->cgi.function=NULL; //clear cgi function					
				}				

				if(r==HTTPD_CGI_MORE){ //cgi signaled ok, but there's more to do on a next round
					return 0; // return so cgi function remains attached
				}
				
			}		
			i++;
		}

		

	}	

	if(final){
		//if we got here, it's a 404
		http_response_NOT_FOUND(conn);
		conn->cgi.function=NULL; //clear cgi	
		conn->cgi.done=1; //mark cgi end
		http_send_response(conn);			
	}

	return 0;

}

// called when a client connects to our server
void ICACHE_FLASH_ATTR http_server_connect_callback(void *arg) {

	struct espconn *conn=arg;

	http_connection *c = http_new_connection(1,conn); // get a connection from the pool, signal it's an incomming connection
	c->cgi.execute = http_server_cgi_execute; // attach our cgi dispatcher	

	//let's disable NAGLE alg so TCP outputs faster ( in theory )
	espconn_set_opt(conn, ESPCONN_NODELAY | ESPCONN_REUSEADDR );

}

//SERVER CONFIG FUNCTIONS ---------------------------------------------------

void ICACHE_FLASH_ATTR http_server_init() {	
	
	espconn_delete(&server_config.server_conn); //just to be sure we are on square 1
	
	server_config.server_conn.type = ESPCONN_TCP;
	server_config.server_conn.state = ESPCONN_NONE;
	server_config.server_conn.proto.tcp = &server_config.server_tcp;
	server_config.server_conn.proto.tcp->local_port = 80;	
	
	NODE_DBG("Http server init, conn=%p", &server_config.server_conn);
	
}


void ICACHE_FLASH_ATTR http_server_bind_port(int port){

	if(server_config.server_conn.state != ESPCONN_NONE){
		NODE_DBG("Can't change port after server started");
		return;
	}

	server_config.server_conn.proto.tcp->local_port = port;

}

//create the url map
http_server_url checkHost_url = {"*", cgi_check_host, &server_config,HTTP_ANY_METHOD,NO_FLAG};
http_server_url cors_url = {"*", cgi_cors, &server_config,HTTP_ANY_METHOD,NO_FLAG};
http_server_url fileSystem_url = {"*", cgi_file_system, NULL,HTTP_ANY_METHOD,NO_FLAG};
http_server_url null_url = {NULL,NULL,NULL,HTTP_ANY_METHOD,NO_FLAG};
void ICACHE_FLASH_ATTR http_server_bind_urls(http_server_url *urls){

	int i;
	int count=0;

	//count urls
	while(urls[count].url!=NULL) count++;


	if(server_config.urls){		
		//free alloced rewrite urls
		for(i=0;i<server_config.rewrite_count;i++){
			os_free( server_config.urls[2+i] );
		}

		os_free(server_config.urls);
	}

 	int array_size = server_config.rewrite_count + count + 4;
	server_config.urls = (http_server_url **)os_zalloc(sizeof(http_server_url *) * (array_size) );
	
	server_config.urls[0]=&checkHost_url;
	server_config.urls[1]=&cors_url;
	server_config.urls[array_size-2]=&fileSystem_url;
	server_config.urls[array_size-1]=&null_url;

	
	for(i=0;i<server_config.rewrite_count;i++){

		http_server_url *rewrite_url = (http_server_url *)os_zalloc(sizeof(http_server_url));
		rewrite_url->url=server_config.rewrites[i].match_url;
		rewrite_url->cgiFunction=cgi_url_rewrite;
		rewrite_url->cgiArgument=server_config.rewrites[i].rewrite_url;
		rewrite_url->method=HTTP_ANY_METHOD;
		rewrite_url->flags=NO_FLAG;
		server_config.urls[2+i]=rewrite_url;
	}
	
	for(i=0;i<count;i++)
		server_config.urls[i+2+server_config.rewrite_count]=&urls[i];

	#ifdef DEVELOP_VERSION
	NODE_DBG("http server urls:");
	for(i=0;i<array_size;i++){

		NODE_DBG("\t%d %s",i,server_config.urls[i]->url);

	}

	#endif

}

//Should be called before http_server_bind_urls
void ICACHE_FLASH_ATTR http_server_rewrite(url_rewrite *rewrites){

	int count=0;
	//count urls
	while(rewrites[count].match_url!=NULL) count++;

	server_config.rewrites=rewrites;
	server_config.rewrite_count=count;
}

void ICACHE_FLASH_ATTR http_server_bind_domain(const char * domain){
	
	server_config.host_domain = domain;
}

void ICACHE_FLASH_ATTR http_server_enable_captive_portal(){
	
	server_config.enable_captive=1;

}
void ICACHE_FLASH_ATTR http_server_enable_cors(){
	
	server_config.enable_cors=1;
}

void ICACHE_FLASH_ATTR http_server_start(){

	if(!server_config.urls)
		http_server_bind_urls(NULL);

	NODE_DBG("Http server start, conn=%p", &server_config.server_conn);
	espconn_regist_connectcb(&server_config.server_conn, http_server_connect_callback);	
	espconn_accept(&server_config.server_conn);

	espconn_tcp_set_max_con_allow(&server_config.server_conn,10);
	NODE_DBG("Http server max conn = %d", espconn_tcp_get_max_con_allow(&server_config.server_conn));

	
}
