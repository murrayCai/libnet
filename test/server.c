#include "net.h"

#define WS_PORT 14000
#define WS_IP "192.168.10.188"
#define WS_BACKLOG 5

int on_recved_data(void *client,char *data,unsigned int dataSize,char **rspData,unsigned int *rspDataSize){
    int ret = 0;
    char *str = "server hello!";
 //   printf("recved => size[%u]\tdata[%s]\n",dataSize,data);
//    pack_manager_print( ((tcp_server_client_t *)client)->pm );
    (*rspData) = str;
    (*rspDataSize) = strlen(str);
//    R(pack_manager_tcp_server_client_send(client,data,dataSize));

    return ret;
}

int on_client_recved(tcp_server_client_t *client){
    int ret = 0;
    R(pack_manager_tcp_server_client_recv(client,on_recved_data));
    return ret;
}
int on_client_closed(tcp_server_client_t *client){
    int ret = 0;
    printf("client[%u] closed!\n",client->fd);
    pack_manager_print(client->pm);
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
    tcp_server_t *server = NULL;
    tcp_server_config_t config = {
        WS_IP,WS_PORT,WS_BACKLOG,
        10000,1024,1024,5,
        on_client_recved,on_client_closed
    };
    RL(tcp_server_init(&server,&config),"tcp_server_init() failed!\n");
    while(1){
        RL(tcp_server_run(server,on_server_error),"tcp_server_run() failed!\n");
        if(isExit){
            tcp_server_exit(&server);
            break;
        }
    }
    mem_show();
    return ret;
}
