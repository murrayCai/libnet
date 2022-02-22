#include "net.h"
#include <limits.h>

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_HTTP_REQ
#else
#define MODULE_CURR MODULE_NET_HTTP_REQ
#endif

static int http_get_method(const char *buf,unsigned int size,http_method_type_e *method){
    int ret = 0;
    R(NULL == buf);
    R(NULL == method);
    R(0 >= size);
    if( 1 == size ){
        if(buf[0] != 'G' && buf[0] != 'P' && buf[0] != 'H' && buf[0] != 'O' && buf[0] != 'D' && buf[0] != 't'){
            *method = HMT_NOT_HTTP;
        }else{
            *method = HMT_WANT_READ;
        }
    }else if( 2 == size ){
        switch(buf[0]){
            case 'G':
            case 'D':
            case 'H':
                *method = ('E' == buf[1] ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'P':
                *method = ('O' == buf[1] || 'U' == buf[1]) ? 
                    HMT_WANT_READ : HMT_NOT_HTTP;
                break;
            case 'O':
                *method = ('P' == buf[1] ? HMT_WANT_READ : HMT_NOT_HTTP); 
                break;
            case 't':
                *method = ('r' == buf[1] ? HMT_WANT_READ : HMT_NOT_HTTP); 
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }else if( 3 == size ){
        switch(buf[0]){
            case 'G':
                *method = ( ('E' == buf[1] && 'T' == buf[2]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'D':
                *method = ( ('E' == buf[1] && 'L' == buf[2]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'H':
                *method = ( ('E' == buf[1] && 'A' == buf[2]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'P':
                if('O' == buf[1]){
                    *method = ('S' == buf[2] ? HMT_WANT_READ : HMT_NOT_HTTP);
                }else if('U' == buf[1]){
                    *method = ('T' == buf[2] ? HMT_WANT_READ : HMT_NOT_HTTP);
                }else{
                    *method = HMT_NOT_HTTP;
                }
                break;
            case 'O':
                *method = ( ('P' == buf[1] && 'T' == buf[2]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 't':
                *method = ( ('r' == buf[1] && 'a' == buf[2]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }else if (4 == size){
        switch(buf[0]){
            case 'G':
                *method = ( ('E' == buf[1] && 'T' == buf[2] && ' ' == buf[3]) ? HMT_GET : HMT_NOT_HTTP);
                break;
            case 'D':
                *method = ( ('E' == buf[1] && 'L' == buf[2] && 'E' == buf[3]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'H':
                *method = ( ('E' == buf[1] && 'A' == buf[2] && 'D' == buf[3]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'P':
                if('O' == buf[1]){
                    *method = ( ('S' == buf[2] && 'T' == buf[3]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                }else if('U' == buf[1]){
                    *method = (('T' == buf[2] && ' ' == buf[3]) ? HMT_PUT : HMT_NOT_HTTP);
                }else{
                    *method = HMT_NOT_HTTP;
                }
                break;
            case 'O':
                *method = ( ('P' == buf[1] && 'T' == buf[2] && 'I' == buf[3]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 't':
                *method = ( ('r' == buf[1] && 'a' == buf[2] && 'c' == buf[3]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }else if(5 == size){
        switch(buf[0]){
            case 'G':
                *method = ( ('E' == buf[1] && 'T' == buf[2] && ' ' == buf[3]) ? HMT_GET : HMT_NOT_HTTP);
                break;
            case 'D':
                *method = ( ('E' == buf[1] && 'L' == buf[2] && 'E' == buf[3] && 'T' == buf[4]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 'H':
                *method = ( ('E' == buf[1] && 'A' == buf[2] && 'D' == buf[3] && ' ' == buf[4]) ? HMT_HEAD : HMT_NOT_HTTP);
                break;
            case 'P':
                if('O' == buf[1]){
                    *method = ( ('S' == buf[2] && 'T' == buf[3] && ' ' == buf[4]) ? HMT_POST : HMT_NOT_HTTP);
                }else if('U' == buf[1]){
                    *method = (('T' == buf[2] && ' ' == buf[3]) ? HMT_PUT : HMT_NOT_HTTP);
                }else{
                    *method = HMT_NOT_HTTP;
                }
                break;
            case 'O':
                *method = ( ('P' == buf[1] && 'T' == buf[2] && 'I' == buf[3] && 'O' == buf[4]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 't':
                *method = ( ('r' == buf[1] && 'a' == buf[2] && 'c' == buf[3] && 'e' == buf[4]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }else if(6 == size){
        switch(buf[0]){
            case 'G':
                *method = ( ('E' == buf[1] && 'T' == buf[2] && ' ' == buf[3]) ? HMT_GET : HMT_NOT_HTTP);
                break;
            case 'D':
                *method = ( ('E' == buf[1] && 'L' == buf[2] && 'E' == buf[3] && 'T' == buf[4] && ' ' == buf[5]) ? HMT_DELETE : HMT_NOT_HTTP);
                break;
            case 'H':
                *method = ( ('E' == buf[1] && 'A' == buf[2] && 'D' == buf[3] && ' ' == buf[4]) ? HMT_HEAD : HMT_NOT_HTTP);
                break;
            case 'P':
                if('O' == buf[1]){
                    *method = ( ('S' == buf[2] && 'T' == buf[3] && ' ' == buf[4]) ? HMT_POST : HMT_NOT_HTTP);
                }else if('U' == buf[1]){
                    *method = (('T' == buf[2] && ' ' == buf[3]) ? HMT_PUT : HMT_NOT_HTTP);
                }else{
                    *method = HMT_NOT_HTTP;
                }
                break;
            case 'O':
                *method = ( ('P' == buf[1] && 'T' == buf[2] && 'I' == buf[3] && 'O' == buf[4] && 'N' == buf[5]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 't':
                *method = ( ('r' == buf[1] && 'a' == buf[2] && 'c' == buf[3] && 'e' == buf[4] && ' ' == buf[5]) ? HMT_TRACE : HMT_NOT_HTTP);
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }else if(7 == size){
        switch(buf[0]){
            case 'G':
                *method = ( ('E' == buf[1] && 'T' == buf[2] && ' ' == buf[3]) ? HMT_GET : HMT_NOT_HTTP);
                break;
            case 'D':
                *method = ( ('E' == buf[1] && 'L' == buf[2] && 'E' == buf[3] && 'T' == buf[4] && ' ' == buf[5]) ? HMT_DELETE : HMT_NOT_HTTP);
                break;
            case 'H':
                *method = ( ('E' == buf[1] && 'A' == buf[2] && 'D' == buf[3] && ' ' == buf[4]) ? HMT_HEAD : HMT_NOT_HTTP);
                break;
            case 'P':
                if('O' == buf[1]){
                    *method = ( ('S' == buf[2] && 'T' == buf[3] && ' ' == buf[4]) ? HMT_POST : HMT_NOT_HTTP);
                }else if('U' == buf[1]){
                    *method = (('T' == buf[2] && ' ' == buf[3]) ? HMT_PUT : HMT_NOT_HTTP);
                }else{
                    *method = HMT_NOT_HTTP;
                }
                break;
            case 'O':
                *method = ( ('P' == buf[1] && 'T' == buf[2] && 'I' == buf[3] && 'O' == buf[4] && 'N' == buf[5] && 'S' == buf[6]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 't':
                *method = ( ('r' == buf[1] && 'a' == buf[2] && 'c' == buf[3] && 'e' == buf[4] && ' ' == buf[5]) ? HMT_TRACE : HMT_NOT_HTTP);
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }else{
        switch(buf[0]){
            case 'G':
                *method = ( ('E' == buf[1] && 'T' == buf[2] && ' ' == buf[3]) ? HMT_GET : HMT_NOT_HTTP );
                break;
            case 'D':
                *method = ( ('E' == buf[1] && 'L' == buf[2] && 'E' == buf[3] && 'T' == buf[4] && ' ' == buf[5]) ? HMT_DELETE : HMT_NOT_HTTP);
                break;
            case 'H':
                *method = ( ('E' == buf[1] && 'A' == buf[2] && 'D' == buf[3] && ' ' == buf[4]) ? HMT_HEAD : HMT_NOT_HTTP);
                break;
            case 'P':
                if('O' == buf[1]){
                    *method = ( ('S' == buf[2] && 'T' == buf[3] && ' ' == buf[4]) ? HMT_POST : HMT_NOT_HTTP);
                }else if('U' == buf[1]){
                    *method = (('T' == buf[2] && ' ' == buf[3]) ? HMT_PUT : HMT_NOT_HTTP);
                }else{
                    *method = HMT_NOT_HTTP;
                }
                break;
            case 'O':
                *method = ( ('P' == buf[1] && 'T' == buf[2] && 'I' == buf[3] && 'O' == buf[4] && 'N' == buf[5] && 'S' == buf[6] && ' ' == buf[7]) ? HMT_WANT_READ : HMT_NOT_HTTP);
                break;
            case 't':
                *method = ( ('r' == buf[1] && 'a' == buf[2] && 'c' == buf[3] && 'e' == buf[4] && ' ' == buf[5]) ? HMT_TRACE : HMT_NOT_HTTP);
                break;
            default:
                *method = HMT_NOT_HTTP;
                break;
        }
    }
    return ret;
}

int http_req_free(http_req_t **pp){
    int ret = 0;
    http_req_t *req = NULL;
    R(NULL == pp);
    R(NULL == *pp);
    req = *pp;
    MC_LIST_ITERATOR(req->headerList,kv_t,{
        R(MC_LIST_RM_DATA(req->headerList,item,kv_t));
        R(kv_free(&item));
    });
    R(MC_LIST_FREE(&req->headerList,kv_t));
    R(MC_LIST_FREE(&req->cookieList,kv_t));
    FREE_T(&req,http_req_t);
    return ret;
}

int http_req_new(http_req_t **pp){
    int ret = 0,retErr = 0;
    http_req_t *req = NULL;
    R(NULL == pp);
    R(NULL != *pp);
    R(MALLOC_T(&req,http_req_t));
    G_E(MC_LIST_INIT(&req->headerList));
    G_E(MC_LIST_INIT(&req->cookieList));
    *pp = req;
    return ret;
error:
    retErr = ret;
    http_req_free(&req);
    return retErr;
}

static int http_url_path_get(const char *buf,unsigned int size,http_req_t *req){
    int ret = 0;
    char *posStart = NULL,*posEnd = NULL;
    R(NULL == req);
    R(NULL == buf);
    R(NULL == (posStart = strchr(buf,' ')));
    posStart += 1;
    R(NULL == (posEnd = strchr(posStart,' ')));
    BUF_SET(&req->path,posStart,posEnd-posStart,0);
    return ret;
}

static int http_version_get(const char *buf,unsigned int size,http_req_t *req){
    int ret = 0;
    char *posStart = NULL,*posEnd = NULL;
    R(NULL == req);
    R(NULL == buf);
    if(NULL == (posStart = strstr(buf, "HTTP/"))){
        req->isHttp = 0;
        req->isRecvCompleted = 1;
    }else{
        req->isHttp = 1;
    }
    posStart += strlen("HTTP/");
    R(NULL == (posEnd = strstr(posStart,"\r\n")));
    BUF_SET(&req->version,posStart,posEnd-posStart,0);
    return ret;
}

static
int http_header_get(char *buf,unsigned int size,http_req_t *req){
    int ret = 0;
    kv_t *kv = NULL,*tmp = NULL;
    char kvNameBuf[4096] = {0},kvValueBuf[4096] = {0};
    char *posNameStart = NULL,*posPoint = NULL,*posValueEnd = NULL;
    unsigned int nameSize = 0, valueSize = 0;
    char bodySizeStr[HTTP_BODY_SIZE_STR_BYTES_MAX + 1] = {0};
    R(NULL == buf);
    R(NULL == req);
    R(0 >= size);
    posNameStart = buf;
    do{
        kv = NULL;
        R(NULL == (posPoint = strchr(posNameStart,':')));
        R(NULL == (posValueEnd = strstr(posPoint,"\r\n")));
        nameSize = posPoint - posNameStart;
        R(MALLOC_T(&kv,kv_t));
        BUF_SET(&kv->key,posNameStart,nameSize,0);
        posPoint += 1;
        while(isspace(posPoint[0])) posPoint += 1;
        valueSize = posValueEnd - posPoint;
        BUF_SET(&kv->value,posPoint,valueSize,0);
        R(MC_LIST_PUSH(req->headerList,kv,kv_t));
        if(0 == strncmp(kv->key.data,"Sec-WebSocket-Key",17)){
            BUF_SET(&req->webSocketKey,posPoint,valueSize,0);
            req->isWebSocket = 1;
        }
        
        if(0 == strncmp(kv->key.data,"Host",4)){
            BUF_SET(&req->host,posPoint,valueSize,0);
        }
        
        if(0 == strncmp(kv->key.data,"Content-Length",14)){
            R(valueSize > HTTP_BODY_SIZE_STR_BYTES_MAX);
            memcpy(bodySizeStr,posPoint,valueSize);
            req->bodySize = strtoul(bodySizeStr,NULL,10); 
        }

        posNameStart = posValueEnd + 2;
        if(posNameStart[0] == '\r' && posNameStart[1] == '\n'){
            break;
        }
    }while(1);
    return ret;
}

int http_request_parse(const char *buf, unsigned int size, http_req_t *req){
    int ret = 0,retErr = 0;
    char *posLine0End = NULL, *posHeaderEnd = NULL;
    R(NULL == buf);
    R(0 >= size);
    R(NULL == req);
    switch(req->process){
        case HRPPE_METHOD_DONE: 
            goto parseFirstLine;
        case HRPPE_FIRST_LINE_DONE: 
            R(NULL == (posLine0End = strstr(buf,"\r\n")));
            posLine0End += 2;
            goto parseHeaders;
        case HRPPE_HEADER_DONE: 
            R(NULL == (posHeaderEnd = strstr(buf,"\r\n\r\n")));
            posHeaderEnd += 4;
            goto parseBody;
        case HRPPE_PARSE_DONE: goto end;
        default: break;
    }
    R(http_get_method(buf,size,&req->method));
    switch(req->method){
        case HMT_WANT_READ: 
            return 0;
        case HMT_NOT_HTTP:
            req->isHttp = 0;
            req->isRecvCompleted = 1;
            return 0;
        case HMT_NONE:
        case HMT_MAX:
            R(1);
            break;
        default:
            R(HTTP_METHOD_VALID(req->method));
            break;
    }
    req->process = HRPPE_METHOD_DONE;

parseFirstLine:
    if(NULL == (posLine0End = strstr(buf,"\r\n"))) return 0;
    posLine0End += 2;
    R(http_url_path_get(buf,size,req));
    R(http_version_get(buf,size,req));
    req->process = HRPPE_FIRST_LINE_DONE;

parseHeaders:
    if(NULL == (posHeaderEnd = strstr(posLine0End,"\r\n\r\n"))) return 0;
    posHeaderEnd += 4;
    R(http_header_get(posLine0End,size - (posLine0End - buf),req));
    req->process = HRPPE_HEADER_DONE;

parseBody:
    if(0 < req->bodySize){
        BUF_SET(&req->body,posHeaderEnd,req->bodySize,0);
        if(req->bodySize <= size - (posHeaderEnd - buf)){
            req->isRecvCompleted = 1;
            req->process = HRPPE_PARSE_DONE;
        }
    }else{
        req->isRecvCompleted = 1;
        req->process = HRPPE_PARSE_DONE;
    }
    req->buf = buf;
    req->size = req->bodySize + (posHeaderEnd - buf);

end:
    return retErr;
error:
    retErr = ret;
    goto end;
}

int http_request_print(http_req_t *http){
    int ret = 0,errRet = 0;;
    kv_t *kv = NULL;
    char name[128] = {0},value[2048] = {0} , path[256] = {0};
    R(NULL == http);
    printf("http:%d\tweb socket:%d\tsize : [%d] => (%s)\n",
            http->isHttp,http->isWebSocket,http->size,http->buf);
    memcpy(path,http->path.data,http->path.size);
    printf("[PATH] => [%d]%s\n",http->path.size,path);
    MC_LIST_FOREACH(kv,http->headerList){
        snprintf(name,kv->key.size + 1,"%s",kv->key.data);
        snprintf(value,kv->value.size + 1,"%s",kv->value.data);
        printf("[HEADER] => %s[%d]:%s[%d]\n",name, kv->key.size, value, kv->value.size);
    }
    printf("[BODY] => [%d]%s\n",http->body.size,http->body.data);
    return ret;
}

