#include "net.h"

#define WS_PORT 80
#define WS_IP "192.168.10.188"
#define WS_BACKLOG 5
#define HTML_BODY "<html><head></head><body>hello</body></html>"
http_resource_t *resources = NULL;
#define HTML_DIR_PAGE "/mc/projects/libnet/test/html/pages"
#define HTML_DIR_CSS "/mc/projects/libnet/test/html/css"
#define HTML_DIR_JS "/mc/projects/libnet/test/html/js"

unsigned int recvedCount = 0;

int on_ws_recved(tcp_server_client_t *client,const char *msg,unsigned int size){
    int ret = 0;
    recvedCount++;
    R(ws_server_send(client,msg,size));
    return ret;
}

int on_client_recved(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    http_req_t http = {0};

    /* init client connect type */
    if( TCCT_NONE == client->connectType ){
        if(IS_HTTP_REQ(buf,size,&http) ){
            if( NULL != http.webSocketKey ){
                client->connectType = TCCT_WEB_SOCKET;
            }else{
                client->connectType = TCCT_HTTP;
            }
        }else{
            client->connectType = TCCT_TCP;
        }
    }

    if( TCCT_TCP == client->connectType ){
        (*used) = size;
        printf("recved => size[%u]\tdata[%s]\n",size,buf);
    }else if( TCCT_WEB_SOCKET == client->connectType ){
        if(0 == client->isWebSocketInited){
            R(ws_server_hand_shake(client,http.webSocketKey,http.webSocketKeySize));
            client->isWebSocketInited = 1;
            (*used) = size;
        }else{
            unsigned int isFin = 0;
            R(ws_server_parse(client,buf,size,used,&isFin,on_ws_recved));
        }
    }else if( TCCT_HTTP == client->connectType ){
        R(http_rsp(client,PMCT_SERVER_CLIENT,&http,resources));
        (*used) = http.size;
    }else{
        R(1);
    }

    return ret;
}

int on_client_closed(tcp_server_client_t *client){
    int ret = 0;
    printf("client[%u] closed!\n",client->fd);
    return ret;
}

int on_server_error(struct kevent *ev){
    int ret = 0;
    LOGE("[%d]\tfilter[%d/%d/%d]\tflags[%u]\tfflags[%u]\n",ev->ident,ev->filter, EVFILT_READ,EVFILT_WRITE, ev->flags,ev->fflags); 
    call_stack_print();
    mc_call_stack_clear();
    return ret;
}

int isExit = 0;
void on_signal(int sigNo){
    isExit = 1;
}

int main(int argc,char *argv[]){
    int ret = 0;
    signal(SIGTERM,on_signal);
    signal(SIGINT,on_signal);

    printf("%lu\n",strtol("1.html",NULL,10));

    http_resource_config_t httpResourceConfig = {
        1024,32,32
    };

    RL(http_resource_init(&resources,&httpResourceConfig),"http_resource_init() failed!\n");
    RL(http_resource_load_from_dir(resources,HRT_PAGE,HTML_DIR_PAGE),"http_resource_load_from_dir() failed!\n");
    RL(http_resource_load_from_dir(resources,HRT_CSS,HTML_DIR_CSS),"http_resource_load_from_dir() failed!\n");
    RL(http_resource_load_from_dir(resources,HRT_JS,HTML_DIR_JS),"http_resource_load_from_dir() failed!\n");

    tcp_server_t *server = NULL;
    tcp_server_config_t config = {
        WS_IP,WS_PORT,WS_BACKLOG,
        10000,1024,1024,5,
        on_client_recved,on_client_closed
    };
    RL(tcp_server_init(&server,&config),"tcp_server_init() failed!\n");
    struct timeval start = {0},end = {0};
    double diff = 0;
    gettimeofday(&start,NULL);
    while(1){
        RL(tcp_server_run(server,on_server_error),"tcp_server_run() failed!\n");
        if(isExit){
            R(tcp_server_exit(&server));
            R(http_resource_free(&resources));
            break;
        }
        gettimeofday(&end,NULL);
        timeval_diff(&end,&start,&diff);
        printf("\rrecved [%u]\t(%f)\tlv(%f)",recvedCount,server->loopTimeMax,recvedCount/diff);
        fflush(stdout);
    }
    mem_show();
    return ret;
}
