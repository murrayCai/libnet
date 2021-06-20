#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_HTTP_RSP
#endif

#define HTML_EXPIRES 12 * 3600
#define HTML404 "HTTP/1.1 404 Not Found\r\n\r\n"
#define HTML_TAG_CODE_200 "HTTP/1.1 200 OK\r\n"
#define HTML_HEADER_CONTENT_TYPE_HTML "Content-Type: text/html; charset=utf-8\r\n" 
#define HTML_HEADER_CONTENT_TYPE_JS "Content-Type: text/javascript; charset=utf-8\r\n" 
#define HTML_HEADER_CONTENT_TYPE_CSS "Content-Type: text/style; charset=utf-8\r\n" 

int http_rsp_generate(http_resource_type_e type,const char *data,unsigned int dataSize,char **dist,unsigned int *distSize){
    int ret = 0;
    R( HTTP_RESOURCE_TYPE_CHECK(type) );
    R( NULL == dist );
    R( NULL != (*dist) );
    R( NULL == distSize );
    R( 0 >= dataSize );
    
    /* Header Date */
    time_t t = time(NULL);
    struct tm *gmt = gmtime(&t);
    char dateBuf[64] = {0};
    unsigned int dateSize = strftime(dateBuf,64,"Date: %a, %d %b %Y %T GMT\r\n",gmt);

    /* Content Length */
    char contentLengthBuf[32] = {0};
    snprintf(contentLengthBuf,32,"Content-Length: %u\r\n",dataSize);

    /* Content Type */
    const char *contentType = ( HRT_PAGE == type ? HTML_HEADER_CONTENT_TYPE_HTML : 
            ( HRT_JS == type ? HTML_HEADER_CONTENT_TYPE_JS : HTML_HEADER_CONTENT_TYPE_CSS ) );

    /* Expires */
    time_t t1 = t + HTML_EXPIRES;
    struct tm *gmt1 = gmtime(&t1);
    char expiresBuf[64] = {0};
    unsigned int expireSize = strftime(expiresBuf, 64,"Expires: %a, %d %b %Y %T GMT\r\n", gmt1);


    unsigned int headerSize = 0;
    headerSize = strlen(HTML_TAG_CODE_200) + dateSize + strlen(contentLengthBuf)
        + strlen(contentType) + expireSize + 2;
    (*distSize) = headerSize + dataSize;

    R(MALLOC_TN(dist,str_t,(*distSize)));
    snprintf(*dist,headerSize + 1,"%s%s%s%s%s\r\n",
            HTML_TAG_CODE_200,contentType,dateBuf,contentLengthBuf,expiresBuf);

    if(NULL != data){
        memcpy( (*dist) + headerSize , data, dataSize);
    }
    return ret;
}

int http_rsp(void *client,client_type_e ct,http_req_t *req,http_resource_t *resources){
    int ret = 0;
    R( NULL == client );
    R( CLIENT_TYPE_CHECK(ct) );
    R( NULL == req );
    R( NULL == resources );

    http_resource_item_t *item = NULL;
    switch( req->type ){
        case HRT_PAGE:
            RL(HTTP_RESOURCE_GET_PAGE_OR_404(resources, req->page, &item),"http_resource_get()!\n");
            break;
        case HRT_CSS:
            RL(HTTP_RESOURCE_GET_CSS(resources, req->page, &item),"http_resource_get_css()!\n");
            break;
        case HRT_JS:
            RL(HTTP_RESOURCE_GET_JS(resources, req->page, &item),"http_resource_get_js()!\n");
            break;
        default:
            RL(HTTP_RESOURCE_GET_PAGE_404(resources,&item),"http_resource_get_page_404()!\n");
            break;
    }
    
    if( PMCT_SERVER_CLIENT == ct ){
        R(tcp_server_client_write(client,item->data,item->dataSize));
    }else if( PMCT_CLIENT == ct ){
        R(tcp_client_send(client,item->data,item->dataSize));
    }

    return ret;
}
