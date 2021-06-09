#include "net.h"

int childIndex = 0;
#define CLIENT_MAX 100

int on_recved_data(void *client,char *data,unsigned int dataSize,char **rspData,unsigned int *rspDataSize){
    int ret = 0;

//    printf("recved => size[%u]\tdata[%s]\n",dataSize,data);
//    R(pack_manager_tcp_server_client_send(client,data,dataSize));

    return ret;
}

unsigned int isClosed = 0;

int on_client_closed(tcp_client_t *client){
    int ret = 0;
    pack_manager_print(client->pm);
    isClosed = 1;
    return ret;
}

int child_run(){
    int ret = 0;
    int fd = 0;
    struct sockaddr_in addr = {0};
    const char *ip = "192.168.10.188";
    unsigned int port = 14000;

    tcp_client_t *client = NULL;
    RL(tcp_client_init(&client,1024,1024,on_client_closed),"tcp_client_init()\n");
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
        R(pack_manager_tcp_client_send(client,buf,strlen(buf)));
    //pack_manager_print(client->pm);
        RL(pack_manager_tcp_client_recv(client,on_recved_data),"recv error!\n");
    //pack_manager_print(client->pm);
        rnd = rand() % ( getpid() % 100 ) ;
        iMax -= rnd;
    }
    R(tcp_client_shut_down(client));
    while(!isClosed){
        sleep(1);
        RL(pack_manager_tcp_client_recv(client,on_recved_data),"recv error!\n");
    }
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

//    child_run();
    forks();
    mem_show();

    return ret;
}
