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
    return ret;
}

int on_normal_pack_recved(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
    R( stimer_set_pack_from_pack(pack,ackPack) ); // write ack time 
    R(STIMER_SERVER_SEND(client,"hello!",strlen("hello!")));
    return ret;
}

int on_ack_pack_recved(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
    R( stimer_set_pack_from_pack(pack,ackPack) ); // write fin time
    return ret;
}

int on_fin_pack_recved(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
    R( stimer_set_pack_from_pack(pack,ackPack) ); // write fin ack time
    return ret;
}

int on_fin_ack_pack_recved(void *client,pack_t *pack){
    int ret = 0;
//    R(stimer_print_pack(pack));
    return ret;
}

int on_normal_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    R( stimer_set_pack(pack) ); // write write time
    return ret;
}

int on_ack_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    R( stimer_set_pack(pack) ); // ack write time
    return ret;
}

int on_fin_pack_before_write (void *client,pack_t *pack){
    int ret = 0;
    R(  stimer_set_pack(pack) ); // fin write time
    return ret;
}
int on_fin_ack_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
//    R(stimer_print_pack(pack));
    return ret;
}
int on_shut_down_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    return ret;
}
int on_shut_down_ack_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    return ret;
}

int on_client_connected(tcp_server_client_t *client){
    int ret = 0;
    pm_t *pm = NULL;
    pm_config_t *config = NULL;
    R(MALLOC_T(&config,pm_config_t));

    config->onNormalPackRecved = on_normal_pack_recved;
    config->onAckPackRecved = on_ack_pack_recved;
    config->onFinPackRecved = on_fin_pack_recved;
    config->onFinAckPackRecved = on_fin_ack_pack_recved;

    config->onNormalPackBeforeWrite = on_normal_pack_before_write;
    config->onAckPackBeforeWrite = on_ack_pack_before_write;
    config->onFinPackBeforeWrite = on_fin_pack_before_write;
    config->onFinAckPackBeforeWrite = on_fin_ack_pack_before_write;
    config->onShutDownPackBeforeWrite = on_shut_down_pack_before_write;
    config->onShutDownAckPackBeforeWrite = on_shut_down_ack_pack_before_write;


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
        pm_on_server_before_write,
        on_client_connected
    };
    RL(tcp_server_init(&server,&config),"tcp_server_init() failed!\n");
    time(&server->netStatStart);
    time_t t = 0,t0 = server->netStatStart;
    while(1){
        RL(tcp_server_run(server,on_server_error),"tcp_server_run() failed!\n");
        if(isExit){
            tcp_server_exit(&server);
            break;
        }
        time(&t);
        if(t > t0){
            t0 = t;
            printf("online:[%u]\twrite rate[%u]/s\tread rate[%u]/s!\n",
                    server->clients->used,
                    server->wCount,
                    server->rCount);
            server->wCount = 0;
            server->rCount = 0;
        }
    }
    mem_show();
    return ret;
}
