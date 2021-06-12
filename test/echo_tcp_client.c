#include "net.h"

int childIndex = 0;
#define CLIENT_MAX 100

unsigned int packCount = 0;
unsigned int sendCount = 0;
unsigned int writeCount = 0;

int on_client_closed(tcp_client_t *client){
    int ret = 0;
    printf("client[%u] closed,sended[%u]\twrite[%u]\tpack[%u]!\n",getpid(),sendCount,writeCount,packCount);
    return ret;
}

int on_client_recved(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
//    printf("recved(%u) => %s\n",size,buf);
    (*used) = size;
    packCount++;
    return ret;
}
int on_client_before_write(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    writeCount++;
    return ret;
}

int child_run(){
    int ret = 0;
    int fd = 0;
    struct sockaddr_in addr = {0};
    const char *ip = "192.168.10.188";
    unsigned int port = 14000;

    tcp_client_config_t *config = NULL;
    R(MALLOC_T(&config,tcp_client_config_t));
    config->readBufSize = 1024;
    config->writeBufSize = 1024;
    config->onRecved = on_client_recved;
    config->onClosed = on_client_closed;
    config->onBeforeWrite = on_client_before_write;
    tcp_client_t *client = NULL;
    RL(tcp_client_init(&client,config),"tcp_client_init()\n");
    RL(tcp_client_connect(client,ip,port),"tcp_client_connect()\n");

    srand(time(NULL));
    int rnd = rand() % 199 ;
    char buf[1024] = {0};
    int iMax = 1000;
    char *data = NULL;
    unsigned int dataSize = 0;

    while(0 < iMax){
        memset(buf,0,1024);
        sprintf(buf,"child[%d]\tcount[%d]\thello",getpid(),1000 - iMax);
        R(tcp_client_send(client,buf,strlen(buf)));
        sendCount++;
        R(tcp_client_recv(client));
        rnd = rand() % ( getpid() % 100 );
        iMax -= rnd;
    }
    R(tcp_client_close(&client));
    sleep(1);
    R(FREE_T(&config,tcp_client_config_t));
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

//    child_run();
    forks();
    mem_show();

    return ret;
}
