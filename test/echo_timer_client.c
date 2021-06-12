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
    printf("recved(%u) => %s\n",size,buf);
    (*used) = size;
    packCount++;
    stimer_t *t = NULL;
    R( !STIMER_EXISTS(buf,size) );
    R(stimer_get_from_buf(buf,size,&t));
    R(stimer_set_run_status(t));
//    R(stimer_print(t));
    return ret;
}
int on_client_before_write(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    writeCount++;
//    printf("write========[%u]%s\t[%s]\n",size,buf,buf + size - STIMER_TAG_SUFFIX_SIZE - 1);
    stimer_t *t = NULL;
    R( !STIMER_EXISTS(buf,size) );
    R(stimer_get_from_buf(buf,size,&t));
    R(stimer_set_run_status(t));
//    R(stimer_print(t));
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
    int iMax = 1;
    char *data = NULL;
    unsigned int dataSize = 0;

    stimer_t *t = NULL;
    while(0 < iMax){
        memset(buf,0,1024);
        t = NULL;
        RL(stimer_init(&t),"stimer_init() failed!\n");
        
        sprintf(buf,"child[%d]\tcount[%d]\thello\t%s",getpid(),1000 - iMax,(char *)t);
        
        RL(stimer_free(&t),"stimer_free() failed!\n");
        RL(tcp_client_send(client,buf,strlen(buf)),"send failed!\n");
        sendCount++;
        RL(tcp_client_recv(client),"recv failed!\n");
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

    child_run();
//    forks();
    mem_show();

    return ret;
}
