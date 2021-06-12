#include "net.h"

#define WS_PORT 14000
#define WS_IP "192.168.10.188"
#define WS_BACKLOG 5

int on_client_closed(tcp_server_client_t *client){
    int ret = 0;
    pm_t *pm = (pm_t *)client->userData;
    pm_config_t *config = (pm_config_t *)pm->config;
    R(FREE_T(&config,pm_config_t));
    R(pm_free(&pm));
    printf("client[%u] closed!\n",client->fd);
    return ret;
}

int on_recved_pack(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
//    printf("recved => size[%u]\tdata[%s]\n", PACK_GET_SIZE(pack),(char *)pack);
    R(pm_server_client_send(client,"hello!",strlen("hello!")));

    return ret;
}
int on_client_connected(tcp_server_client_t *client){
    int ret = 0;
    pm_t *pm = NULL;
    pm_config_t *config = NULL;
    R(MALLOC_T(&config,pm_config_t));
    config->onNormalPackRecved = on_recved_pack; 

    R(pm_init(&pm,config));
    client->userData = pm;

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
        pm_on_server_recv,
        on_client_closed,
        NULL,
        
        on_client_connected
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
