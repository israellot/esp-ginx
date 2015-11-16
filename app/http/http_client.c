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
#include "c_string.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "platform.h"
#include "user_config.h"
#include "lwip/dns.h"

#include "http.h"
#include "http_client.h"
#include "http_parser.h"
#include "http_helper.h"
#include "http_process.h"


void ICACHE_FLASH_ATTR http_client_request_execute(http_connection *c)
{
	NODE_DBG("http_client_request_execute ");

	if(c->state==HTTP_CLIENT_CONNECT_OK){
		//we should send request headers

		char * path = http_url_get_path(c);
		char * query = http_url_get_query(c);

		char * requestPath;
		if(query!=NULL){
			requestPath = (char *)os_zalloc(strlen(path)+strlen(query)+2);
			os_strcpy(requestPath,path);
			os_strcat(requestPath,"?");
			os_strcat(requestPath,query);
		}
		else{
			requestPath = (char *)os_zalloc(strlen(path)+1);
			os_strcpy(requestPath,path);
		}			

		if(path==NULL)
			path = "/";

		http_SET_HEADER(c,HTTP_HOST,http_url_get_host(c));
		http_SET_HEADER(c,HTTP_USER_AGENT,HTTP_DEFAULT_SERVER);

		if(c->method==HTTP_GET){
			http_request_GET(c,requestPath);
			os_free(requestPath);
		}
		else if(c->method==HTTP_POST){
			http_request_POST(c,requestPath);
			os_free(requestPath);
		}
		else
			return;
		
		c->state=HTTP_CLIENT_REQUEST_HEADERS_SENT;

		http_transmit(c); // send request
	}
	else if(c->state==HTTP_CLIENT_REQUEST_HEADERS_SENT){
		//we can now send the body

		if(c->request_body!=NULL){
			//body is already here, send
			int bodyLen = os_strlen(c->request_body);
			if(bodyLen>=HTTP_BUFFER_SIZE){
				http_nwrite(c,c->request_body,HTTP_BUFFER_SIZE);
				c->request_body+=HTTP_BUFFER_SIZE;
			}
			else
				http_write(c,c->request_body);

			http_transmit(c);			
		}
		else{
			//callback so body can be sent elsewhere
			http_execute_cgi(c);	
		}

		c->state=HTTP_CLIENT_REQUEST_BODY_SENT;

	}
	

}


void ICACHE_FLASH_ATTR http_client_dns_found_cb(const char *name, ip_addr_t *ipaddr, void *arg){

	NODE_DBG("http_client_dns_callback conn=%p",arg);

	http_connection *c = (http_connection*)arg;

	if(c==NULL)
		return;

	os_timer_disarm(&c->timeout_timer);
	c->host_ip.addr=ipaddr->addr;

	if(name!=NULL && ipaddr!=NULL){

		#ifdef DEVELOP_VERSION
		NODE_DBG("http_client_dns_callback: %s %d.%d.%d.%d",
			name,
			ip4_addr1(&ipaddr->addr),
			ip4_addr2(&ipaddr->addr),
			ip4_addr3(&ipaddr->addr),
			ip4_addr4(&ipaddr->addr));
		#endif	
		
		//set ip on tcp
		c->espConnection->proto.tcp->remote_ip[0]=ip4_addr1(&ipaddr->addr);
		c->espConnection->proto.tcp->remote_ip[1]=ip4_addr2(&ipaddr->addr);
		c->espConnection->proto.tcp->remote_ip[2]=ip4_addr3(&ipaddr->addr);
		c->espConnection->proto.tcp->remote_ip[3]=ip4_addr4(&ipaddr->addr);

		//DNS found
		c->state = HTTP_CLIENT_DNS_FOUND;
	}
	else{
		//DNS found
		c->state = HTTP_CLIENT_DNS_NOT_FOUND;
	}

	http_execute_cgi(c);	
	
}

void ICACHE_FLASH_ATTR http_client_dns_timeout(void *arg){
	NODE_DBG("http_client_dns_timeout");
	http_connection *c = (http_connection*)arg;
	
	// put null so if dns query response arrives, it wont trigger 
	// execute again
	c->espConnection->reverse=NULL;

	c->state = HTTP_CLIENT_DNS_NOT_FOUND;
	http_execute_cgi(c);
}

void ICACHE_FLASH_ATTR http_client_dns(http_connection *c){

	char * host = http_url_get_host(c);
	NODE_DBG("http_client_dns: %s",host);
	
	os_memset(&c->host_ip,0,sizeof(ip_addr_t));
	dns_gethostbyname(host, &c->host_ip, &http_client_dns_found_cb,(void*)c);

    os_timer_disarm(&c->timeout_timer);
    os_timer_setfn(&c->timeout_timer, (os_timer_func_t *)http_client_dns_timeout, c);
    os_timer_arm(&c->timeout_timer, 1000, 0);

}

void ICACHE_FLASH_ATTR http_client_connect_callback(void *arg) {

	struct espconn *conn=arg;

	NODE_DBG("http_client_connect_callback: %d",conn->state==ESPCONN_CONNECT?1:0);

	http_connection *c = (http_connection *)conn->reverse;

	if(conn->state==ESPCONN_CONNECT)
		c->state=HTTP_CLIENT_CONNECT_OK;	
	else
		c->state=HTTP_CLIENT_CONNECT_FAIL;

	http_execute_cgi(c);

}

int ICACHE_FLASH_ATTR http_client_cgi_execute(http_connection *c){

	if(c->cgi.function==NULL)
		return 0;

	int ret = c->cgi.function(c);

	if(c->state==HTTP_CLIENT_DNS_NOT_FOUND || (ret== HTTP_CLIENT_CGI_DONE && c->state==HTTP_CLIENT_DNS_FOUND)){
		//this was a dns request only
		NODE_DBG("Client conn %p done after DNS",c);
		http_process_free_connection(c);
		return 0;
	}
	else if(ret== HTTP_CLIENT_CGI_DONE){
		//abort
		espconn_disconnect(c->espConnection);
		http_process_free_connection(c);
		return 0;
	}

	if(c->state==HTTP_CLIENT_DNS_FOUND){		
		//connect
		espconn_connect(c->espConnection);		
	}
	if(c->state==HTTP_CLIENT_CONNECT_OK){
		//we are ready to make the request
		http_client_request_execute(c);
	}
	if(c->state==HTTP_CLIENT_CONNECT_FAIL){
		//free
		espconn_disconnect(c->espConnection);
		http_process_free_connection(c);		
	}

}

http_connection * ICACHE_FLASH_ATTR http_client_new(http_callback callback){

	http_connection * client = http_new_connection(0,NULL);

	if(client==NULL)
		return NULL;
	
	client->cgi.execute=http_client_cgi_execute;
	client->cgi.function = callback;

	espconn_regist_connectcb(client->espConnection, http_client_connect_callback);	

	return client;
}

int ICACHE_FLASH_ATTR http_client_open_url(http_connection *c,char *url){
	
	NODE_DBG("http_client_open_url: %s",url);

	os_strcpy(c->url,url);
	http_parse_url(c);
		
	//set port
	if(c->url_parsed.port>0)
		c->client_connection.proto.tcp->remote_port=c->url_parsed.port;		
	else	
		c->client_connection.proto.tcp->remote_port=80;

	http_client_dns(c);

	return 1;
}

int ICACHE_FLASH_ATTR http_client_GET(http_connection *c,char *url){
	c->method=HTTP_GET;
	return http_client_open_url(c,url);
}




