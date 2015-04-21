#ifndef JSON_H
#define JSON_H

#include "jsmn.h"

typedef struct{
	char * key;
	char * value;
} jsonPair;

int json_count_token(char *js,size_t len);
jsmntok_t * json_tokenise(char *js,size_t len,jsmntok_t *tokens ,int tokenLen);
bool json_token_streq(char *js, jsmntok_t *t, char *s);
char * json_token_tostr(char *js, jsmntok_t *t);
char * json_find_token(char *tokenKey,char *js,jsmntok_t *tokens,size_t token_len);
int json_parse(jsonPair fields[],int fieldCount,char *js,int jsLen);



#endif