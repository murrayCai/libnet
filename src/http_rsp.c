#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_HTTP_RSP
#else
#define MODULE_CURR MODULE_NET_HTTP_RSP
#endif

#define HTML_EXPIRES 12 * 3600
#define HTML_TAG_CODE_200 "HTTP/1.1 200 OK\r\n"
#define HTML_HEADER_CONTENT_TYPE_HTML "Content-Type: text/html; charset=utf-8\r\n" 
#define HTML_HEADER_CONTENT_TYPE_JS "Content-Type: text/javascript; charset=utf-8\r\n" 
#define HTML_HEADER_CONTENT_TYPE_CSS "Content-Type: text/style; charset=utf-8\r\n" 

int http_rsp_free(http_rsp_t **pp){
    int ret = 0;
    http_rsp_t *rsp = NULL;
    R(NULL == pp);
    R(NULL == *pp);
    rsp = *pp;
    MC_LIST_ITERATOR(rsp->headerList,kv_t,{
        R(MC_LIST_RM_DATA(rsp->headerList,item,kv_t));
        R(kv_free(&item));
    });
    MC_LIST_FREE(&rsp->headerList,kv_t);

    if(NULL != rsp->data.data){
        R(buf_data_free(&rsp->data));
    }
    if(NULL != rsp->buf.data){
        R(buf_data_free(&rsp->buf));
    }
    FREE_T(pp,http_rsp_t);
    return ret;
}

int http_rsp_new(http_rsp_t **pp){
    int ret = 0,retErr = 0;
    http_rsp_t *rsp = NULL;
    R(NULL == pp);
    R(NULL != *pp);
    

    R(MALLOC_T(&rsp,http_rsp_t));
    G_E(MC_LIST_INIT(&rsp->headerList));
    
    *pp = rsp;
end:
    return retErr;
error:
    retErr = ret;
    FREE_T(&rsp,http_rsp_t);
    goto end;
}

#define HTML_RSP_200_HEAD "HTTP/1.1 200 OK\r\n"
#define HTML_RSP_201_HEAD "HTTP/1.1 201 Created\r\n"
#define HTML_RSP_202_HEAD "HTTP/1.1 202 Accepted\r\n"
#define HTML_RSP_203_HEAD "HTTP/1.1 203 Non-Authoritative Information\r\n"
#define HTML_RSP_204_HEAD "HTTP/1.1 204 No Content\r\n"
#define HTML_RSP_205_HEAD "HTTP/1.1 205 Reset Content\r\n"
#define HTML_RSP_206_HEAD "HTTP/1.1 205 Partial Content\r\n"


#define HTML_RSP_400 "HTTP/1.1 400 Bad Request\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_401 "HTTP/1.1 401 Unauthorized\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_402 "HTTP/1.1 402 Payment Required\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_403 "HTTP/1.1 403 Forbidden\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_404 "HTTP/1.1 404 Not Found\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_405 "HTTP/1.1 405 Method Not Allowed\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_406 "HTTP/1.1 406 Not Acceptable\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_407 "HTTP/1.1 407 Proxy Authentication Required\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_408 "HTTP/1.1 408 Request Timeout\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_409 "HTTP/1.1 409 Conflict\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_410 "HTTP/1.1 410 Gone\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_411 "HTTP/1.1 411 Length Required\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_412 "HTTP/1.1 412 Precondition Failed\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_413 "HTTP/1.1 413 Request Entity Too Large\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_414 "HTTP/1.1 414 Request-URI Too Long\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_415 "HTTP/1.1 415 Unsupported Media Type\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_416 "HTTP/1.1 416 Requested Range Not Satisfiable\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_417 "HTTP/1.1 417 Expectation Failed\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"


#define HTML_RSP_500 "HTTP/1.1 500 Internal Server Error\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_501 "HTTP/1.1 501 Not Implemented\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_502 "HTTP/1.1 502 Bad Gateway\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_503 "HTTP/1.1 503 Service Unavailable\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_504 "HTTP/1.1 504 Gateway Timeout\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_505 "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTML_RSP_511 "HTTP/1.1 511 Network Authentication Required\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n"
#define HTTP_SEND(_c_) \
    tcp_send_buf((tcp_client_define_t *)(_c_),(_c_)->httpRsp->buf.data,(_c_)->httpRsp->buf.size,0,0)

int http_rsp_by_code(tcp_server_client_t *client){
    int ret = 0;
    char *data = NULL;
    R(NULL == client);
    R(NULL == client->httpRsp);
    switch(client->httpRsp->code){
        case 400 : data = HTML_RSP_400; break;
        case 401 : data = HTML_RSP_401; break;
        case 402 : data = HTML_RSP_402; break;
        case 403 : data = HTML_RSP_403; break;
        case 404 : data = HTML_RSP_404; break;
        case 405 : data = HTML_RSP_405; break;
        case 406 : data = HTML_RSP_406; break;
        case 407 : data = HTML_RSP_407; break;
        case 408 : data = HTML_RSP_408; break;
        case 409 : data = HTML_RSP_409; break;
        case 410 : data = HTML_RSP_410; break;
        case 411 : data = HTML_RSP_411; break;
        case 412 : data = HTML_RSP_412; break;
        case 413 : data = HTML_RSP_413; break;
        case 414 : data = HTML_RSP_414; break;
        case 415 : data = HTML_RSP_415; break;
        case 416 : data = HTML_RSP_416; break;
        case 417 : data = HTML_RSP_417; break;
        case 501 : data = HTML_RSP_501; break;
        case 502 : data = HTML_RSP_502; break;
        case 503 : data = HTML_RSP_503; break;
        case 504 : data = HTML_RSP_504; break;
        case 505 : data = HTML_RSP_505; break;
        case 511 : data = HTML_RSP_511; break;
        default : data = HTML_RSP_500; break;
    }
    BUF_SET(&client->httpRsp->buf,data,strlen(data),0);
    R(HTTP_SEND(client));
    return ret;
}

int http_rsp(tcp_server_client_t *client){
    int ret = 0;
    char *buf = NULL,*ptr = NULL,*dataSizeBuf = NULL;
    char *head = NULL;
    unsigned int bufSize = 0,tmpSize = 0;
    kv_t *kv = NULL,*tmp = NULL,*kvContentLength = NULL,*kvClose = NULL;

    R(NULL == client);
    http_rsp_t *rsp = client->httpRsp;
    R(NULL == rsp);

    if(rsp->code >= 300){
        R(http_rsp_by_code(client));
        return ret;
    }else{
        switch(rsp->code){
            case 201 : head = HTML_RSP_201_HEAD; break;
            case 202 : head = HTML_RSP_202_HEAD; break;
            case 203 : head = HTML_RSP_203_HEAD; break;
            case 204 : head = HTML_RSP_204_HEAD; break;
            case 205 : head = HTML_RSP_205_HEAD; break;
            case 206 : head = HTML_RSP_206_HEAD; break;
            default: head = HTML_RSP_200_HEAD; break;
        }
    }

    R(rsp->data.size >= pow(10,HTTP_BODY_SIZE_STR_BYTES_MAX - 1));
    R(MALLOC_T(&kvContentLength,kv_t));
    BUF_SET(&kvContentLength->key,"Content-Length",strlen("Content-Length") + 1, 0);
    R(MALLOC_TN(&dataSizeBuf,str_t,HTTP_BODY_SIZE_STR_BYTES_MAX));
    BUF_SET(&kvContentLength->value,dataSizeBuf,HTTP_BODY_SIZE_STR_BYTES_MAX, 1);
    snprintf(dataSizeBuf,HTTP_BODY_SIZE_STR_BYTES_MAX,"%u",
            NULL == rsp->data.data ? 0 : rsp->data.size);
    R(MC_LIST_PUSH(client->httpRsp->headerList,kvContentLength,kv_t));
    
    R(MALLOC_T(&kvClose,kv_t));
    BUF_SET(&kvClose->key,"Connection",strlen("Connection") + 1, 0);
    BUF_SET(&kvClose->value,"Close",strlen("Close") + 1, 0);
    R(MC_LIST_PUSH(client->httpRsp->headerList,kvClose,kv_t));

    // compute response buf size
    bufSize = strlen(head);
    MC_LIST_FOREACH(kv,client->httpRsp->headerList){
        R(0 >= strlen(kv->key.data));
        R(0 >= strlen(kv->value.data));
        bufSize += strlen(kv->key.data) + strlen(kv->value.data) + 4;
    }
    bufSize += 2;
    if(NULL != rsp->data.data){
        bufSize += rsp->data.size;
    }

    R(MALLOC_TN(&buf,str_t,bufSize));
    BUF_SET(&client->httpRsp->buf,buf,bufSize,1);
    ptr = buf;

    memcpy(ptr,head,strlen(head));
    ptr += strlen(head);
    MC_LIST_FOREACH(kv,client->httpRsp->headerList){
        tmpSize = strlen(kv->key.data) + strlen(kv->value.data) + 4;
        snprintf(ptr,tmpSize + 1,"%s: %s\r\n",kv->key.data,kv->value.data);
        ptr += tmpSize;
    }
    memcpy(ptr,"\r\n",2);
    ptr += 2;
    memcpy(ptr, rsp->data.data, rsp->data.size);
    R(HTTP_SEND(client));
    return ret;
}
