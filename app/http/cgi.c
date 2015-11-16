/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Israel Lot <me@israellot.com> and Jeroen Domburg <jeroen@spritesmods.com> 
 * wrote this file. As long as you retain this notice you can do whatever you 
 * want with this stuff. If we meet some day, and you think this stuff is 
 * worth it, you can buy us a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "c_string.h"
#include "osapi.h" 
#include "user_interface.h"
#include "c_stdio.h"
#include "mem.h"

#include "user_config.h"

#include "http.h"
#include "http_parser.h"
#include "http_server.h"
#include "http_process.h"
#include "http_helper.h"

#include "rofs.h"
#include "cgi.h"


// If a request to transmit data overflows the send buffer, the cgi function will be temporarely 
// replaced by this one and later restored when all data is sent.
int ICACHE_FLASH_ATTR cgi_transmit(http_connection *connData){

	NODE_DBG("cgi_transmit");
	struct cgi_transmit_arg *arg = (struct cgi_transmit_arg*)connData->cgi.data;

	if(arg->len > 0){
		NODE_DBG("cgi_transmit %d bytes",arg->len);
		int rem = connData->output.buffer + HTTP_BUFFER_SIZE - connData->output.bufferPos;
		int bytesToWrite = rem;
		if(arg->len < rem )
			bytesToWrite = arg->len;

		http_nwrite(connData,arg->dataPos,bytesToWrite);

		arg->len -= bytesToWrite;
		arg->dataPos+=bytesToWrite;

	}
	
	if(arg->len==0){

		//all written
		
		//free data
		os_free(arg->data);

		//copy old cgi back
		os_memcpy(&connData->cgi,&arg->previous_cgi,sizeof(cgi_struct));

		//free cgi arg
		os_free(arg);

	}

	return HTTPD_CGI_MORE;

}


// This makes sure we aren't serving static files on POST requests for example
int ICACHE_FLASH_ATTR cgi_enforce_method(http_connection *connData) {	

	enum http_method *method = (enum http_method *)connData->cgi.argument;

	if(connData->state == HTTPD_STATE_BODY_END)
	if(connData->parser.method!=*method && (int)*method>=0)
	{
		NODE_DBG("Wrong HTTP method. Enforce is %d and request is %d",method,connData->parser.method);

		http_response_BAD_REQUEST(connData);
		return HTTPD_CGI_DONE;
	}
	

	return HTTPD_CGI_NEXT_RULE;
}

// This makes sure we have a body
int ICACHE_FLASH_ATTR cgi_enforce_body(http_connection *connData) {	

	if(connData->state ==HTTPD_STATE_ON_URL){	
		http_set_save_body(connData); //request body to be saved
	}

	//wait for whole body
	if(connData->state <HTTPD_STATE_BODY_END)
		return HTTPD_CGI_NEXT_RULE; 

	//if body empty, bad request
	if(connData->body.len <=0){
		http_response_BAD_REQUEST(connData);
		NODE_DBG("No body");		
		return HTTPD_CGI_DONE;
	}
	else
		return HTTPD_CGI_NEXT_RULE;

	
}

//cgi that adds CORS ( Cross Origin Resource Sharing ) to our server
int ICACHE_FLASH_ATTR cgi_cors(http_connection *connData) {
	
	http_server_config *config = (http_server_config*)connData->cgi.argument;;
	if(config==NULL)
		return HTTPD_CGI_NEXT_RULE;

	if(!config->enable_cors)
		return HTTPD_CGI_NEXT_RULE;


	if(connData->state==HTTPD_STATE_ON_URL){

		//save cors headers 
		http_set_save_header(connData,HTTP_ACCESS_CONTROL_REQUEST_HEADERS);
		http_set_save_header(connData,HTTP_ACCESS_CONTROL_REQUEST_METHOD);
		
		return HTTPD_CGI_NEXT_RULE;
	}
	
	if(connData->state==HTTPD_STATE_HEADERS_END){

		//SET CORS Allow Origin for every request
		http_SET_HEADER(connData,HTTP_ACCESS_CONTROL_ALLOW_ORIGIN,"*");

		header * allow_headers = http_get_header(connData,HTTP_ACCESS_CONTROL_REQUEST_HEADERS);
		header * allow_methods = http_get_header(connData,HTTP_ACCESS_CONTROL_REQUEST_METHOD);
		
		if(allow_headers!=NULL)
			http_SET_HEADER(connData,HTTP_ACCESS_CONTROL_ALLOW_HEADERS,allow_headers->value);
		if(allow_methods!=NULL)
			http_SET_HEADER(connData,HTTP_ACCESS_CONTROL_ALLOW_METHODS,allow_methods->value);

		// Browsers will send an OPTIONS pre-flight request when posting data on a cross-domain situation
		// If that's the case here, we can safely return 200 OK with our CORS headers set
		if(connData->parser.method==HTTP_OPTIONS)
		{
			http_response_OK(connData);
			return HTTPD_CGI_DONE;
		}
	}

	return HTTPD_CGI_NEXT_RULE;
}

//Simple static url rewriter, allows us to process the request as another url without redirecting the user
//Used to serve index files on root / requests for example
int ICACHE_FLASH_ATTR cgi_url_rewrite(http_connection *connData) {	

	if(connData->state==HTTPD_STATE_HEADERS_END){

		NODE_DBG("Rewrite %s to %s",connData->url,(char*)connData->cgi.argument);
	
		int urlSize = strlen((char*)connData->cgi.argument);
		if(urlSize < URL_MAX_SIZE){
			strcpy(connData->url,(char*)connData->cgi.argument);	
			//re-parse url
			http_parse_url(connData);
		}
		
	}

	return HTTPD_CGI_NEXT_RULE;
}

//Simple cgi that redirects the user
int ICACHE_FLASH_ATTR cgi_redirect(http_connection *connData) {
	
	http_response_REDIRECT(connData, (char*)connData->cgi.argument);
	return HTTPD_CGI_DONE;
}

//Cgi that check the request has the correct HOST header
//Using it we can ensure our server has a domain of our choice
int ICACHE_FLASH_ATTR cgi_check_host(http_connection *connData) {
	
	http_server_config *config = (http_server_config*)connData->cgi.argument;
	if(config==NULL)
		return HTTPD_CGI_NEXT_RULE;

	if(config->host_domain==NULL)
		return HTTPD_CGI_NEXT_RULE;

	
	if(connData->state==HTTPD_STATE_ON_URL){

		http_set_save_header(connData,HTTP_HOST);
		return HTTPD_CGI_NEXT_RULE;
	}

	if(connData->state==HTTPD_STATE_HEADERS_END){

		header *hostHeader = http_get_header(connData,HTTP_HOST);
		if(hostHeader==NULL){
			NODE_DBG("Host header not found");
			http_response_BAD_REQUEST(connData);
			return HTTPD_CGI_DONE;
		}

		const char * domain = config->host_domain;

		NODE_DBG("Host header found: %s, domain: %s",hostHeader->value,domain);


		if(os_strncmp(hostHeader->value,domain,strlen(domain))==0) //compare ignoring http:// and last /
		{
			NODE_DBG("Hosts match");
			return HTTPD_CGI_NEXT_RULE;
		}
		else{
			NODE_DBG("Hosts don't match");
			
			if(config->enable_captive){
				//to enable a captive portal we should redirect here

				char * redirectUrl = (char *)os_zalloc(strlen(domain)+9); // domain lenght + http:// + / + \0
				os_strcpy(redirectUrl,"http://");
				os_strcat(redirectUrl,domain);
				os_strcat(redirectUrl,"/");

				http_response_REDIRECT(connData, redirectUrl);

				os_free(redirectUrl);

			}
			else{
				//bad request
				http_response_BAD_REQUEST(connData);

			}
			
			

			return HTTPD_CGI_DONE;
		}
		

	}

	
	return HTTPD_CGI_NEXT_RULE;

}

typedef struct{
	RO_FILE *f;
	char *buff;
} cgi_fs_state;

//cgi for static file serving
int ICACHE_FLASH_ATTR cgi_file_system(http_connection *connData) {

	cgi_fs_state *state = (cgi_fs_state *)connData->cgi.data;

	int len;
	char buff[HTTP_BUFFER_SIZE];		
	
	//wait for body end state
	if(connData->state<HTTPD_STATE_BODY_END)
		return HTTPD_CGI_NEXT_RULE; 	

	if (state==NULL) {
		//First call to this cgi. Open the file so we can read it.
		
		RO_FILE *f;
		if( ! connData->url_parsed.field_set & (1<<UF_PATH) ){
			//there's no PATH on the url ( WTF? ), so not found
			return HTTPD_CGI_NOTFOUND;
		}

		char * path = http_url_get_path(connData);
		
		f=f_open(path);		

		if (f==NULL) {
			return HTTPD_CGI_NOTFOUND;
		}

		NODE_DBG("File %s opened",path);

		state = (cgi_fs_state*)os_zalloc(sizeof(cgi_fs_state));
		state->f = f;
		connData->cgi.data=state; //save state for next time
		
		//set headers
		http_SET_HEADER(connData,HTTP_CONTENT_TYPE,http_get_mime(connData->url));	
		http_SET_HEADER(connData,HTTP_CACHE_CONTROL,HTTP_DEFAULT_CACHE);
		http_SET_CONTENT_LENGTH(connData,f->file->size);

		if(f->file->gzip)
			http_SET_HEADER(connData,HTTP_CONTENT_ENCODING,"gzip");

		http_response_OK(connData);		
		
		
		return HTTPD_CGI_MORE;
	}
	else{
		//file found, transmit data

		len=f_read(state->f, buff, HTTP_BUFFER_SIZE);
		if (len>0){
			NODE_DBG("Sending %d bytes of data",len);
			http_nwrite(connData,(const char*)buff,len);
			//http_response_transmit(connData);
		} 
		if (state->f->eof) {
			NODE_DBG("End of file reached");
			//We're done.
			f_close(state->f);
			os_free(state);
			return HTTPD_CGI_DONE;
		} else {
			//not done yet
			return HTTPD_CGI_MORE;
		}

	}

	
}
