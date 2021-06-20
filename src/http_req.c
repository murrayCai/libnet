#include "net.h"
#include <limits.h>

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_HTTP_REQ
#endif

#define HTTP_METHOD_SIZE(m) ( HTTP_GET == (m) ? 4 : 0 )

#define GET_METHOD_SIZE(b,size)\
    ({\
     int r = 0;const char *buf = (const char *)(b);\
     if( 3 < (size) && 'G' == *(buf) ){\
     if( 'E' != *((buf) + (++r) ) || 'T' != *((buf) + (++r) ) || ' ' != *((buf) + (++r) ) ){\
     r = 0;\
     }\
     }\
     (r);\
     })

#define HTTP_TAG_SIZE 10
#define IS_HTTP_TAG(buf,size) ( HTTP_TAG_SIZE <= (size) && 'H' == *(buf) &&\
        'T' == *((const char *)(buf) + 1) &&\
        'T' == *((const char *)(buf) + 2) &&\
        'P' == *((const char *)(buf) + 3) &&\
        '/' == *((const char *)(buf) + 4) &&\
        ( '1' == *((const char *)(buf) + 5) || '2' == *((const char *)(buf) + 5) || '0' == *((const char *)(buf) + 5)  ) &&\
        '.' == *((const char *)(buf) + 6) &&\
        ( '0' <= *((const char *)(buf) + 7) && '9' >= *((const char *)(buf) + 7) ) &&\
        '\r' == *((const char *)(buf) + 8) && '\n' == *((const char *)(buf) + 9) )

int http_req_parse(const char *buf, unsigned int size, http_req_t *http){
    int ret = 0;
    R(NULL == buf);
    R(NULL == http);
    const char *bufEnd = buf + size;
    const char *tmp = buf;
    const char *url = NULL;
    const char *tag = NULL;
    const char *header = NULL;
    const char *urlLastDashPos = NULL; // last / pos
    const char *urlLastPointPos = NULL; // last . pos
    const char *urlWhPos = NULL; // ? pos
    unsigned int methodSize = 0;
    unsigned int isInEof = 0;
    unsigned int page = 0;
    http_resource_type_e type = HRT_NONE;

    // matching support method
    methodSize = GET_METHOD_SIZE(tmp,size);
    R(0 == methodSize);
    tmp += methodSize + 1;
    url = tmp;


    // matching support url
    while(' ' != *tmp){
        if( '/' == *tmp ) urlLastDashPos = tmp;
        if( NULL != urlLastDashPos ){
            if( '.' == *tmp ){
                urlLastPointPos = tmp;
            }
        }
        R(bufEnd <= ++tmp);
    }
    tag = ++tmp;

    // matching page and type
    if( NULL != urlLastDashPos ){
        page = strtol( urlLastDashPos + 1, NULL,10 );
        if(NULL == urlLastPointPos){
            if( 0 == strncmp(url,"/ ",2) ){
                type = HRT_PAGE;
            }else if(0 == strncmp(url,"/... ", 5) ){

            }else{

            }
        }else{
            if(0 == strncmp( urlLastPointPos,HRT_PAGE_SUFFIX,strlen(HRT_PAGE_SUFFIX))){
                type = HRT_PAGE;
            }else if(0 == strncmp( urlLastPointPos,HRT_CSS_SUFFIX,strlen(HRT_CSS_SUFFIX)) ){
                type = HRT_CSS;
            }else if( 0 == strncmp( urlLastPointPos,HRT_JS_SUFFIX,strlen(HRT_JS_SUFFIX)) ){
                type = HRT_JS;
            }else{
                if( 0 == strncmp(url,"/favicon.ico ",13) ){
                    //type = HRT_PAGE;
                }
            }
        }
    }



    // matching tag
    R( 0 == IS_HTTP_TAG(tmp, bufEnd - tmp ) );
    tmp += HTTP_TAG_SIZE;
    header = tmp;


    // matching headers

    // matching end
    const char *webSocketKeyHeader = "Sec-WebSocket-Key: ";
    unsigned int wskHeaderSize = strlen(webSocketKeyHeader);
    const char *webSocketPos = NULL;
    unsigned int isMatchingWebSocketKey = 0;
    unsigned int webSocketKeySize = 0;
    do{
        R( 4 > (bufEnd - tmp) );
        if( '\r' != *tmp ){ tmp++; }
        else{
            if( '\n' == *(++tmp) ){
                if( '\r' == *(++tmp) ){
                    if( '\n' == *(++tmp) ) break;
                }else{
                    if( isMatchingWebSocketKey ){
                        webSocketKeySize = tmp - webSocketPos - 2;
                        isMatchingWebSocketKey = 0;
                    }
                    if( wskHeaderSize < (bufEnd - tmp) && 0 == strncmp(tmp,webSocketKeyHeader,wskHeaderSize)){
                        tmp += wskHeaderSize;
                        webSocketPos = tmp;
                        isMatchingWebSocketKey = 1;
                    }
                }
            }else{

            }
        }
    }while(1);

    R(buf == url);
    R(url == tag);
    R(tag == header);

    http->buf = buf;
    http->url = url;
    http->tag = tag;
    http->header = header;
    http->page = page;
    http->type = type;
    http->webSocketKey = webSocketPos;
    http->webSocketKeySize = webSocketKeySize;
    http->size = tmp - buf + 1;

    return ret;
}

int http_req_print(http_req_t *http){
    int ret = 0,errRet = 0;;
    R(NULL == http);
    char *method = NULL, *url = NULL,*tag = NULL,*header = NULL,*end = NULL;
    unsigned int methodSize = http->url - http->buf;
    R(0 >= methodSize);
    unsigned int urlSize = http->tag - http->url;
    R(0 >= urlSize);
    unsigned int tagSize = http->header - http->tag;
    R(0 >= tagSize);
    unsigned int headerSize = http->size + http->buf  - http->header;
    R(0 >= headerSize);

    R(MALLOC_TN(&method,str_t,methodSize + 1));
    memcpy(method,http->buf,methodSize);

    G_E(MALLOC_TN(&url,str_t,urlSize + 1));
    memcpy(url,http->url,urlSize);

    G_E(MALLOC_TN(&tag,str_t,tagSize + 1));
    memcpy(tag,http->tag,tagSize);

    G_E(MALLOC_TN(&header,str_t,headerSize + 1));
    memcpy(header,http->header,headerSize);

    printf("method(%s)\turl(%s)\tpage(%u)\ttype(%u)\ttag(%s)\theader(%s)\n",
            method,url, http->page,http->type, tag,header);

error:
    errRet = ret;
    if(NULL != method) R(FREE_TN(&method,str_t,methodSize + 1));
    if(NULL != url) R(FREE_TN(&url,str_t,urlSize + 1));
    if(NULL != tag) R(FREE_TN(&tag,str_t,tagSize + 1));
    if(NULL != header) R(FREE_TN(&header,str_t,headerSize + 1));
    return errRet;
}

