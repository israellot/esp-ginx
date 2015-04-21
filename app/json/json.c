#include "c_stdlib.h"
#include "ets_sys.h"
#include "c_types.h"
#include "user_interface.h"
#include "c_stdio.h"
#include "osapi.h"
#include "mem.h"
#include "json/jsmn.h"
#include "json/json.h"

int json_count_token(char *js,size_t len){

    jsmn_parser parser;
    jsmn_init(&parser);

    int ret = jsmn_parse(&parser, js,len, NULL, 0);

    return ret;
}

jsmntok_t * json_tokenise(char *js,size_t len,jsmntok_t *tokens ,int tokenLen)
{        
   
    jsmn_parser parser;   
    jsmn_init(&parser); 
    int ret = jsmn_parse(&parser, js,len, tokens, tokenLen);

    if(ret<0){
        os_printf("\njson parse error : %d",ret);
    }

    return tokens;
}



bool json_token_streq(char *js, jsmntok_t *t, char *s)
{
    return (strncmp(js + t->start, s, t->end - t->start) == 0
            && strlen(s) == (size_t) (t->end - t->start));
}

char * json_token_tostr(char *js, jsmntok_t *t)
{
    js[t->end] = '\0';
    return js + t->start;
}

char * json_find_token(char *tokenKey,char *js,jsmntok_t *tokens,size_t token_len){

    char *value=NULL;
    int i;
    for(i=0;i<token_len;i++){

        jsmntok_t *t = &tokens[i];
        
        if (t->type == JSMN_STRING){

            os_printf("\njson parse, token : %s",json_token_tostr(js, t));

            if (json_token_streq(js, t, tokenKey))
            {
                i++;
                t=&tokens[i];

                value = json_token_tostr(js, t);

                os_printf("\n found %s : %s",tokenKey,value);
                return value;
            }

        }

    }

    return value;

}

int json_parse(jsonPair *fields,int fieldCount,char *js,int jsLen){

    int ret = json_count_token(js,jsLen);

    if(ret < 0){

        NODE_DBG("jsmn_parse error : %d",ret);        
        return 0;
    }

    jsmntok_t *tokens = (jsmntok_t*) os_malloc(sizeof(jsmntok_t) * ret);
    json_tokenise(js,jsLen,tokens,ret);

    NODE_DBG("json parse, tokens needed : %d",ret);

    int i;
    for(i=0;i<fieldCount;i++){
        fields[i].value = json_find_token(fields[i].key,js,tokens,ret);
        NODE_DBG("json %s:%s",fields[i].key,fields[i].value);
    }
        
    NODE_DBG("json parse complete");

    os_free(tokens);

    return 1;

}