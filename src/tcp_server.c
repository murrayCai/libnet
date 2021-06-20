#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TCP_SERVER
#endif

#define KQ_EVENT_SIZE(config) ( ( (config)->clientMax + 1) * 2 )
#define IS_VALID_CLIENT(client) ( NULL != (client)  && NULL != (client)->server )

#define TSC_WRITE_ENABLE(client) \
    do{\
        EV_SET(&( (client)->evWrite),(client)->fd, EVFILT_WRITE , EV_ENABLE, 0,0,(client));\
        kevent((client)->server->kq,&(client->evWrite),1,NULL,0,NULL);\
    }while(0)

#define TSC_WRITE_DISABLE(client) \
    do{\
        EV_SET(&( (client)->evWrite), (client)->fd, EVFILT_WRITE , EV_DISABLE, 0,0,NULL);\
        kevent( (client)->server->kq,&(client->evWrite),1,NULL,0,NULL);\
    }while(0)



int tcp_server_init(tcp_server_t **ts,tcp_server_config_t *config){
    int ret = 0,errRet = 0;
    R(NULL == ts);
    R(NULL != (*ts));
    R(NULL == config);
    R(NULL == config->ip);
    R(0 >= config->backLog);
    R(NULL == config->onClientRecved);

    R(MALLOC_T(ts,tcp_server_t));
    G_E(MALLOC_TN( &((*ts)->events), ev_t, (config->clientMax + 1) * 2 ));

    G_E(-1 == ((*ts)->fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)));

    int flag = 1,socketFlags = 0;
    G_E(setsockopt((*ts)->fd,SOL_SOCKET,SO_REUSEPORT,&flag,sizeof(flag)));
    G_E(-1 == (socketFlags = fcntl((*ts)->fd,F_GETFL,0)) );
    G_E(-1 == fcntl((*ts)->fd,F_SETFL,socketFlags | O_NONBLOCK));

    (*ts)->addr.sin_family = AF_INET;
    (*ts)->addr.sin_port = htons(config->port);
    (*ts)->addr.sin_addr.s_addr = inet_addr(config->ip);
    G_E(bind((*ts)->fd,(struct sockaddr *)&((*ts)->addr),sizeof(struct sockaddr_in)));
    G_E(listen((*ts)->fd,config->backLog));

    G_E(-1 == ( (*ts)->kq = kqueue()));
    struct kevent ev = {0};
    EV_SET(&ev,(*ts)->fd,EVFILT_READ , EV_ADD,0,0,0);
    G_E(-1 == kevent((*ts)->kq,&ev,1,NULL,0,NULL));

    (*ts)->config = config;
    G_E(arr_init(&(*ts)->clients,config->clientMax));

    return ret;
error:
    errRet = ret;
    if(NULL != (*ts)) {
        if(NULL != (*ts)->events ){
            R(FREE_TN(&((*ts)->events),ev_t, KQ_EVENT_SIZE(config)));
        }
        R(FREE_T(ts,tcp_server_t));
    }
    return errRet;
}

static int tcp_server_client_init(tcp_server_t *ts){
    int ret = 0,errRet = 0;
    socklen_t sockLen = sizeof(struct sockaddr_in);
    R(NULL == ts);
    R(NULL == ts->clients);
    R(NULL == ts->config);
    int fd = 0;
    struct sockaddr_in addr = {0};
    tcp_server_client_t *client = NULL;

    R(0 >= ( fd = accept(ts->fd,(struct sockaddr *)&addr,&sockLen) ) );

    R(MALLOC_T(&client,tcp_server_client_t));
    client->server = ts;
    client->fd = fd;
    memcpy(&client->addr,&addr,sockLen);

    unsigned int readBufSize = ts->config->readPackMax * TCP_SERVER_CLIENT_BUF_LV;
    G_E(MALLOC_TN(&(client->readBuf),str_t,readBufSize));
    client->readBufSize = readBufSize;

    unsigned int writeBufSize = ts->config->writePackMax * TCP_SERVER_CLIENT_BUF_LV;
    G_E(MALLOC_TN(&(client->writeBuf),str_t,writeBufSize));
    client->writeBufSize = writeBufSize;

    EV_SET(&(client->evRead),client->fd, EVFILT_READ , EV_ADD, 0,0,client);
    kevent(ts->kq,&(client->evRead),1,NULL,0,NULL);

    EV_SET(&(client->evWrite),client->fd, EVFILT_WRITE , EV_ADD | EV_DISABLE, 0,0,client);
    kevent(ts->kq,&(client->evWrite),1,NULL,0,NULL);

    client->index = ts->clients->used;
    G_E(arr_add(ts->clients,client));

    if( NULL != ts->config->onClientConnected ){
        G_E(ts->config->onClientConnected(client));
    }
    return ret;
error:
    errRet = ret;
    if(IS_VALID_FD(fd)){
        close(fd);
    }
    if(NULL != client){
        if(NULL != client->readBuf){
            R(FREE_TN(&client->readBuf,str_t,client->readBufSize));
        }
        if(NULL != client->writeBuf){
            R(FREE_TN(&client->writeBuf,str_t,client->writeBufSize));
        }
        R(FREE_T(&client,tcp_server_client_t));
    }
    return errRet;
}

static int tcp_server_client_recv(tcp_server_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(0 >= client->enableReadSize);
    G(0 >= client->readBufSize - client->readBufUsed,cb);
    tcp_server_t *ts = client->server;
    R(NULL == ts);

    ssize_t readSize = 0;
    unsigned int used = 0; 

    unsigned int needReadSize = (client->enableReadSize > client->readBufSize - client->readBufUsed ) ?
        client->readBufSize - client->readBufUsed : client->enableReadSize;
        readSize = recv(client->fd,client->readBuf + client->readBufUsed,needReadSize,0);

    if( -1 == readSize ){
        if(EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno){
            ret = 0;
        }else{
            R(1);
        }
    }else if( 0 == readSize ){
        if( !IS_VALID_FD(client->fd) ){
            R(tcp_server_client_close(&client));
        }else{
            LOGI("fd[%d] valid and get read event,buf read size 0!\n",client->fd);
        }
    }else{
        client->server->rCount += readSize;
        client->readBufUsed += readSize;
        R( readSize != needReadSize );
cb:
        used = 0;
        ret = 0;
        if(NULL != ts->config->onClientRecved){
            R(ts->config->onClientRecved(client,client->readBuf,client->readBufUsed,&used));
            R(0 > used);
            if(0 < used){
                int unUsedSize = client->readBufUsed - used;
                R(0 > unUsedSize);
                memcpy(client->readBuf,client->readBuf + used,unUsedSize);
                memset(client->readBuf + unUsedSize,0,client->readBufSize - unUsedSize);
                client->readBufUsed = unUsedSize;
            }
        }
    }
    return ret;
}

static int tcp_server_client_send(tcp_server_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(NULL == client->server);

    unsigned int maxWriteSize = client->writeBufUsed > client->enableWriteSize ? client->enableWriteSize : client->writeBufUsed;

    // set write time
    unsigned int needWriteSize = maxWriteSize;
    if(NULL != client->server->config->onClientBeforeWrite){
        R(client->server->config->onClientBeforeWrite(client,client->writeBuf,maxWriteSize,&needWriteSize));
    }
    R(0 >= needWriteSize);

    int sendSize = 0;
    sendSize = send(client->fd,client->writeBuf, needWriteSize, 0);
    if( 0 >= sendSize ){
        if( !IS_VALID_FD(client->fd) ){
            R(tcp_server_client_close(&client));
        }else{
            LOGI("fd[%d] valid and get read event,buf write size 0!\n",client->fd);
        }
    }else{
        client->server->wCount += sendSize;
        unsigned int leaveWriteBufSize = client->writeBufUsed - sendSize;
        if(leaveWriteBufSize > 0){
            memcpy(client->writeBuf,client->writeBuf + sendSize,leaveWriteBufSize);
        }else{
            memset(client->writeBuf,0,client->writeBufSize);
        }
        client->writeBufUsed -= sendSize;
        if(0 == leaveWriteBufSize){
            TSC_WRITE_DISABLE(client);
        }
        R( sendSize != needWriteSize );
    }
    return ret;
}


int tcp_server_client_write(tcp_server_client_t *client,const char *buf,size_t bufSize){
    int ret = 0;
    R(NULL == client);
    R(NULL == client->writeBuf);
    R(bufSize > client->writeBufSize - client->writeBufUsed);
    memcpy(client->writeBuf + client->writeBufUsed,buf,bufSize);
    client->writeBufUsed += bufSize;
    TSC_WRITE_ENABLE(client);
    return ret;
}


static int __cb_reset_client_index(uint index,void *data){
    int ret = 0;
    tcp_server_client_t *client = (tcp_server_client_t *)data;
    client->index = index - 1;
    return ret;
}

int tcp_server_client_close(tcp_server_client_t **pp){
    int ret = 0;
    R(NULL == pp);
    R(NULL == (*pp));
    tcp_server_client_t *client = *(tcp_server_client_t **)pp;
    R(NULL == client->server);
    tcp_server_t *ts = client->server;

    unsigned int index = client->index;

    struct kevent evRead = {client->fd,EVFILT_READ,EV_DELETE,0,0,NULL};
    R(-1 == kevent(ts->kq,&evRead,1,NULL,0,NULL));

    struct kevent evWrite = {client->fd,EVFILT_WRITE,EV_DELETE,0,0,NULL};
    R(-1 == kevent(ts->kq,&evWrite,1,NULL,0,NULL));
    
    TSC_WRITE_DISABLE(client);

    R(arr_foreach(ts->clients,index,__cb_reset_client_index));
    R(arr_delete_index_then_resort(ts->clients,index));
    
    R(close(client->fd));

    if(NULL != ts->config->onClientClosed){
        ts->config->onClientClosed(client);
    }

    if(NULL != client->readBuf){
        R(FREE_TN(&client->readBuf,str_t,client->readBufSize));
    }
    if(NULL != client->writeBuf){
        R(FREE_TN(&client->writeBuf,str_t,client->writeBufSize));
    }
    R(FREE_T(pp,tcp_server_client_t));
    return ret;
}


int tcp_server_exit(tcp_server_t **ts){
    int ret = 0;
    R(NULL == ts);
    R(NULL == (*ts));
    R(NULL == (*ts)->config);
    int i = 0;

    tcp_server_config_t *config = (*ts)->config;
    tcp_server_client_t *client = NULL;

    while((*ts)->clients->used){
        R(ARR_LAST((void **)&client,(*ts)->clients));
        R(tcp_server_client_close(&client));
    }

    R(arr_free(&((*ts)->clients)));
    R(close((*ts)->fd));
    if(NULL != (*ts)->events ){
        R(FREE_TN(&((*ts)->events),ev_t, KQ_EVENT_SIZE(config) ));
    }
    R(FREE_T(ts,tcp_server_t));
    return ret;
}

int tcp_server_run(tcp_server_t *ts,cb_tcp_server_run_error callBack){
    int ret = 0;
    R(NULL == ts);
    R(NULL == ts->clients);
    R(NULL == ts->config);
    R(NULL == callBack);
    int i = 0;
    int evSize = 0;
    struct timespec kqTimeOut = {0};
    kqTimeOut.tv_sec = ts->config->kqTimeout;
    evSize = kevent(ts->kq,NULL,0,ts->events,KQ_EVENT_SIZE(ts->config),&kqTimeOut);
    struct timeval start = {0};
    struct timeval end = {0};
    double tDiff = 0;
    gettimeofday(&start,NULL);
    if( -1 == evSize ){

    }else if(evSize > 0){
        for(i = 0; i < evSize; i++){
            struct kevent ev = ts->events[i];
            tcp_server_client_t *client = (tcp_server_client_t *)ev.udata;
            if(ev.flags & EV_ERROR){
                int err = ev.data;
                if(EBADF == err){
                    if(!IS_VALID_FD(ev.ident)){
                        if( ev.ident == ts->fd ){
                            G_E(tcp_server_exit(&ts));
                        }else{
                            G_E(tcp_server_client_close(&client));
                        }
                    }else{
                    }
                }else{
                    if( ev.ident == ts->fd ){
                        G_E(tcp_server_exit(&ts));
                    }else{
                        G_E(tcp_server_client_close(&client));
                    }
                }
            }else{
                if(ev.ident == ts->fd){
                    G_E(tcp_server_client_init(ts));
                }else{
                    if(!IS_VALID_CLIENT(client)){
                        LOGI("client is invalid [%d] \t event[%d] !\n",i,ev.filter);
                        continue;
                    }
                    if(!IS_VALID_FD(ev.ident)){
                        G_E(tcp_server_client_close(&client));
                    }else{
                        if(ev.filter == EVFILT_READ){
                            if( (EV_EOF & ev.flags) ){
                                G_E(tcp_server_client_close(&client));
                            }else{
                                client->enableReadSize = (unsigned int)ev.data;
                                G_E(tcp_server_client_recv(client));
                            }
                        }else if(ev.filter == EVFILT_WRITE){
                            if( (EV_EOF & ev.flags) ){
                                G_E(tcp_server_client_close(&client));
                            }else{
                                client->enableWriteSize = (unsigned int)ev.data;
                                if(client->enableWriteSize <= 0){
                                    LOGW("client[%d] socket write buf is full!\n");
                                }else{
                                    if(client->writeBufUsed > 0){
                                        G_E(tcp_server_client_send(client));
                                    }else{
                                        // nothing to write
                                    }
                                }
                            }
                        }else{
                            G_E(1);
                        }
                    }
                }
            }
            continue;
error:
            callBack(&ev);
            MC_ERR_CLEAR();
        }
    }
    gettimeofday(&end,NULL);
    timeval_diff(&end,&start,&tDiff);
    if( tDiff >= ts->loopTimeMax ){
        ts->loopTimeMax = tDiff;
    }
    return ret;
}

