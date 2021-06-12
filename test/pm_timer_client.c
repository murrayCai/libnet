#include "net.h"

#define WS_IP "192.168.10.188"
int childIndex = 0;
#define CLIENT_MAX 100
int isClosed = 0;

unsigned int sendCount = 0;

unsigned int normalPackCount = 0;
unsigned int ackPackCount = 0;
unsigned int finPackCount = 0;
unsigned int finAckPackCount = 0;

unsigned int writePackCount = 0;
unsigned int writeAckPackCount = 0;
unsigned int writeFinPackCount = 0;
unsigned int writeFinAckPackCount = 0;

unsigned int shutPackCount = 0;
unsigned int shutAckPackCount = 0;

int on_client_connected(tcp_client_t *client){
    int ret = 0;

    return ret;
}

int on_client_closed(tcp_client_t *client){
    int ret = 0;
    printf("[closed]\tclient[%u]\tnormalCount[%u]\tackCount[%u]\tfinCount[%u]\tfinAckCount[%u]\t"
            "writeCount[%u/%u]\twAckCount[%u]\twFinCount[%u]\twFinAckCount[%u]\n",
            getpid(),normalPackCount,ackPackCount,finPackCount,finAckPackCount,
            writePackCount,sendCount,writeAckPackCount,writeFinPackCount,writeFinAckPackCount);
    E(normalPackCount != ackPackCount);
    E(normalPackCount != finPackCount);
    E(normalPackCount != finAckPackCount);
    E(normalPackCount != writePackCount);
    E(normalPackCount != sendCount);
    E(normalPackCount != writeAckPackCount);
    E(normalPackCount != writeFinPackCount);
    E(normalPackCount != writeFinAckPackCount);
    isClosed = 1;
    return ret;
}

int on_recved_pack(void *client,pack_t *pack){
    int ret = 0;
    //    printf("========RECV=======[%u]%s\n",PACK_GET_SIZE(pack),(char *)pack);
    // packCount++;
    
    return ret;
}

int on_normal_pack_recved(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
    R( stimer_set_pack_from_pack(pack,ackPack) ); // write ack time 
    normalPackCount++;
    return ret;
}

int on_ack_pack_recved(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
    R( stimer_set_pack_from_pack(pack,ackPack) ); // write fin time
    ackPackCount++;
    return ret;
}

int on_fin_pack_recved(void *client,pack_t *pack,pack_t *ackPack){
    int ret = 0;
    R( stimer_set_pack_from_pack(pack,ackPack) ); // write fin ack time
    finPackCount++;
    return ret;
}

int on_fin_ack_pack_recved(void *client,pack_t *pack){
    int ret = 0;
    finAckPackCount++;
//    R(stimer_print_pack(pack));
    return ret;
}

int on_normal_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    writePackCount++;
    R( stimer_set_pack(pack) ); // write write time
    return ret;
}

int on_ack_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    writeAckPackCount++;
    R( stimer_set_pack(pack) ); // ack write time
    return ret;
}

int on_fin_pack_before_write (void *client,pack_t *pack){
    int ret = 0;
    writeFinPackCount++;
    R(  stimer_set_pack(pack) ); // fin write time
    return ret;
}
int on_fin_ack_pack_before_write(void *client,pack_t *pack){
    int ret = 0;
    writeFinAckPackCount++;
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

int child_run(){
    int ret = 0;
    int fd = 0;
    struct sockaddr_in addr = {0};
    const char *ip = WS_IP;
    unsigned int port = 14000;

    tcp_client_config_t *config = NULL;
    R(MALLOC_T(&config,tcp_client_config_t));
    
    config->readBufSize = 1024;
    config->writeBufSize = 1024;
    config->onConnected = on_client_connected;
    config->onRecved = pm_on_client_recv;
    config->onClosed = on_client_closed;
    config->onBeforeWrite = pm_on_client_before_write;
    tcp_client_t *client = NULL;

    RL(tcp_client_init(&client,config),"tcp_client_init()\n");
    RL(tcp_client_connect(client,ip,port),"tcp_client_connect()\n");

    pm_t *pm = NULL;
    pm_config_t *pmConfig = NULL;
    R(MALLOC_T(&pmConfig,pm_config_t));
    
    pmConfig->onNormalPackRecved = on_normal_pack_recved;
    pmConfig->onAckPackRecved = on_ack_pack_recved;
    pmConfig->onFinPackRecved = on_fin_pack_recved;
    pmConfig->onFinAckPackRecved = on_fin_ack_pack_recved;

    pmConfig->onNormalPackBeforeWrite = on_normal_pack_before_write;
    pmConfig->onAckPackBeforeWrite = on_ack_pack_before_write;
    pmConfig->onFinPackBeforeWrite = on_fin_pack_before_write;
    pmConfig->onFinAckPackBeforeWrite = on_fin_ack_pack_before_write;
    pmConfig->onShutDownPackBeforeWrite = on_shut_down_pack_before_write;
    pmConfig->onShutDownAckPackBeforeWrite = on_shut_down_ack_pack_before_write;

    R(pm_init(&pm,pmConfig));
    client->userData = pm;

    srand(time(NULL));
    int rnd = rand() % 199 ;
    char buf[1024] = {0};
    int iMax = 1000;
    char *data = NULL;
    unsigned int dataSize = 0;
    stimer_t *t = NULL;

    while(0 < iMax){
        memset(buf,0,1024);
        t = NULL;
        RL(stimer_init(&t),"stimer_init() failed!\n");
        sprintf(buf,"child[%d]\tcount[%d]\thello\t%s",getpid(),1000 - iMax,(char *)t); 
        RL(stimer_free(&t),"stimer_free() failed!\n");
        R(pm_client_send(client,buf,strlen(buf)));
        sendCount++;
        RL(tcp_client_recv(client),"recv error!\n");
        rnd = rand() % ( getpid() % 100 );
        iMax -= rnd;
        sleep(1);
    }

    while(!isClosed){
        R(pm_client_shut_down(client));
        sleep(1);
    }
    
    R(pm_free(&pm));
    R(FREE_T(&pmConfig,pm_config_t));
    R(FREE_T(&config,tcp_client_config_t));
    sleep(1);
    return ret;
}

int forks(){
    int ret = 0;
    int i = 0;
    pid_t cid;
    int currClients = 0;
    int status = 0;

    for(;i < CLIENT_MAX; i++){
        childIndex = i;
        RL(-1 == (cid = fork()),"fork() failed!\n");
        if(0 == cid){
            exit(child_run());
        }else{
            currClients++;
        }
    }

    while(1){
        cid = wait(&status);
        if(-1 != cid){
            currClients--;
            RL(-1 == (cid = fork()),"fork() failed!\n");
            if(0 == cid){
                exit(child_run());
            }else{
                currClients++;
            }
        }
    }
    printf("parent process exit!\n");
    return ret;
}

int main(int argc,char *argv[]){
    int ret = 0;

//        child_run();
    forks();
    mem_show();

    return ret;
}
