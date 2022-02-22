#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TCP_CLIENT
#else
#define MODULE_CURR MODULE_NET_TCP_CLIENT
#endif

int tcp_client_statu_check(tcp_client_define_t *client){
    int ret = 0;
    if(!IS_VALID_FD(client->fd)){
        LOGW("client fd is invalid!\n");
        goto closeTag;
    }else{
        if(0 < errno){
            switch(errno){
                case EINTR:
                case EWOULDBLOCK:
                case EINPROGRESS:
                case EALREADY:
                case ENOBUFS: 
                case ETIMEDOUT:
                    break;
                case EBADF:
                case EHOSTUNREACH:
                case EPIPE:
                case ECONNRESET:
                case ECONNABORTED:
                case ECONNREFUSED:
                case ENETRESET:
                case ENETUNREACH:
                case ENETDOWN:
                case EHOSTDOWN:
                case ENOTCONN:
                case EACCES:
                case ENOTSOCK:
                case EOPNOTSUPP:
                default:
                    goto closeTag;
            }
        }
    }
end:
    errno = 0;
    return ret;
closeTag:
    TCP_CLIENT_SET_CLOSE(client);
    goto end;
}

int tcp_client_close(tcp_client_t **pp){
    int ret = 0;
    tcp_client_t *client = NULL;
    R(NULL == pp);
    R(NULL == (*pp));
    R( NULL == (*pp)->writeBufs );
    R( NULL == (*pp)->readBuf );

    client = *pp;

    if(NULL != client->kq){
        R(kq_client_close(&client->kqClient));
    }else{
        R(close(client->fd));
    }
    LOGI("client[%d] disconnect => %s:%d\n",client->fd,client->host,client->port);
    client->connectStatus = TCCS_CLOSED;
    if(NULL != client->config && NULL != client->config->onClosed){
        R(client->config->onClosed(client));
    }

    R(FREE_TN(&(client->readBuf),str_t,client->readBufSize));
    tcp_write_buffer_t *wBuf = NULL;
    MC_LIST_ITERATOR(client->writeBufs,tcp_write_buffer_t,{
            R(MC_LIST_RM_DATA(client->writeBufs,item,tcp_write_buffer_t));
            R( tcp_write_buffer_free(&item) );
            });
    R(MC_LIST_FREE( &( client->writeBufs ),tcp_write_buffer_t ) );
    R(FREE_T(pp,tcp_client_t));
    return ret;
}

static int __on_tcp_client_connected(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(NULL == client->config);
    if(NULL != client->kq){
        R(kq_client_registe(client->kq,&client->kqClient));
    }
    LOGI("client[%d] connected => %s:%d!\n",client->fd,client->host,client->port);
    if(NULL != client->config->onConnected){
        R(client->config->onConnected(client));
    }
    return ret;
}

int tcp_read(tcp_client_define_t *client,char *buf,unsigned int needReadSize,unsigned int *readedSize){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(0 >= needReadSize);
    R(NULL == readedSize);
    unsigned int enableReadSize = 0,minReadSize = 0,readSize = 0;

    if(NULL != client->ssl){
        errno = 0;
        R(tcp_ssl_recv(client,buf,needReadSize,&readSize));
    }else{
        R(tcp_get_enable_read_size((tcp_client_define_t *)client,&enableReadSize));
        minReadSize = enableReadSize > needReadSize ? needReadSize : enableReadSize;
        errno = 0;
        readSize = recv(client->fd,buf,minReadSize,0);
        if(0 == readSize){
            TCP_CLIENT_SET_CLOSE(client);
        }
    }
    if(client->isNeedClose) R(1);
    if(readSize <= 0 || errno != 0){
        R(tcp_client_statu_check(client));
        if(client->isNeedClose){R(1);}
    }
    *readedSize = readSize;
    return ret;
}

int tcp_do_message(tcp_client_define_t *client){
    int ret = 0;
    tcp_server_t *ts = NULL;
    tcp_server_config_t *serverConfig = NULL;
    tcp_client_config_t *clientConfig = NULL;
    unsigned int used = 0,totalUsed = 0;
    R(NULL == client);
    R(0 >= client->readBufUsed);
    if(PMCT_SERVER_CLIENT == client->type){
        ts = ((tcp_server_client_t *)client)->server;
        R(NULL == ts);
        serverConfig = ts->config;
        R(NULL == serverConfig);
        R(NULL == serverConfig->onClientRecved);
    }else if(PMCT_CLIENT == client->type){
        clientConfig = ((tcp_client_t *)client)->config;
        R(NULL == clientConfig);
        R(NULL == clientConfig->onRecved);
    }

doMessage:
    used = 0;
    if(PMCT_SERVER_CLIENT == client->type){
        R(serverConfig->onClientRecved((tcp_server_client_t *)client,client->readBuf + totalUsed,client->readBufUsed - totalUsed,&used));
    }else{
        R(clientConfig->onRecved((tcp_client_t *)client,client->readBuf + totalUsed,client->readBufUsed - totalUsed,&used));
    }
    if(0 > used){
        R(1);
    }else if( 0 < used ){
        totalUsed += used;
        if(client->readBufUsed > totalUsed){ goto doMessage;}
        else if(totalUsed > client->readBufUsed){ R(1); }
        else { goto endMessage; }
    }else{
endMessage:
        if(totalUsed > 0){
            if(client->readBufUsed > totalUsed){
                memcpy(client->readBuf,client->readBuf + totalUsed,client->readBufUsed - totalUsed);
            }
        }
    }
    client->readBufUsed -= totalUsed;
    R(0 > client->readBufUsed);
    return ret;
}


int tcp_client_connect(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(NULL == client->config);
    if(-1 == connect(client->fd,(struct sockaddr *)&(client->addr),sizeof(struct sockaddr_in))){
        if(client->config->isNonBlock){
            switch(errno){
                case EINPROGRESS:
                case EWOULDBLOCK:
                case EINTR:
                case EALREADY:
                    errno = 0;
                    client->connectStatus = TCCS_CONNECTING;
                    break;
                case EBADF:
                case ENOTSOCK:
                case EINVAL:
                case EADDRNOTAVAIL:
                case EAFNOSUPPORT:
                case ETIMEDOUT:
                case ENETUNREACH:
                case EHOSTUNREACH:
                case EADDRINUSE:
                case EFAULT:
                case EACCES:
                    client->connectStatus = TCCS_CONNECT_ERROR;
                    break;
                case EISCONN:
                    client->connectStatus = TCCS_CONNECTED;
                    break;
                case ECONNREFUSED:
                case ECONNRESET:
                    client->connectStatus = TCCS_CLOSED;
                    break;
            }
        }else{
            client->connectStatus = TCCS_CLOSED;
        }
    }else{
        client->connectStatus = TCCS_CONNECTED;
    }
    switch(client->connectStatus){
        case TCCS_CONNECTED:
            R(__on_tcp_client_connected(client));
            break;
        case TCCS_CONNECT_ERROR:
            if(NULL != client->config->onConnectError){
                R(client->config->onConnectError(client));
            }
            errno = 0;
            R(tcp_client_fd_init(client));
            break;
        case TCCS_CLOSED:
            if(NULL != client->config->onConnectError){
                R(client->config->onConnectError(client));
            }
            errno = 0;
            R(tcp_client_fd_init(client));
            break;
        case TCCS_CONNECTING:
            break;
        default:
            break;
    }
    return ret;
}


int tcp_client_connect_check(tcp_client_t *client){
    int ret = 0,i = 0,retErr = 0;
    int fd = 0;
    tcp_client_connect_statu_e statu;
    R(NULL == client);
    fd = client->fd;

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int errVal = 0;
    socklen_t errValLen = sizeof(errVal);

    if( -1 == getpeername(fd,(struct sockaddr *)&addr,&len) ){
        switch(errno){
            case ENOTCONN:
                errno = 0;
                R(getsockopt(fd,SOL_SOCKET,SO_ERROR,&errVal,&errValLen));
                if(errVal > 0){
                    errno = errVal;
                    statu = TCCS_CONNECT_ERROR;
                }else{
                    statu = TCCS_CONNECTING;
                }
                break;
            case EINVAL:
                statu = TCCS_CONNECTING;
                break;
            case ECONNRESET:
            case EBADF:
            case EFAULT:
            case ENOBUFS:
            case ENOTSOCK:
            case ECONNREFUSED:
            default:
                statu = TCCS_CONNECT_ERROR;
        }
        errno = 0;
    }else{
        statu = TCCS_CONNECTED;
    }
    client->connectStatus = statu;
    switch(statu){
        case TCCS_CONNECTED:
            R(__on_tcp_client_connected(client));
            break;
        case TCCS_CONNECT_ERROR:
            if(NULL != client->config->onConnectError){
                R(client->config->onConnectError(client));
            }
            R(tcp_client_fd_init(client));
            break;
        case TCCS_CONNECTING:
            /* R(tcp_client_connect(client)); */
            break;
        default:
            break;
    }
    errno = 0;
    return ret;
}

int tcp_write_buffer_free(tcp_write_buffer_t **pp){
    int ret = 0;
    R( NULL == pp );
    R( NULL == (*pp) );
    tcp_write_buffer_t *wBuf = *pp;

    if( NULL != wBuf->onCompleted ){
        R( wBuf->onCompleted( wBuf ) );
    }

    if(wBuf->type == TWBT_MEM){
        if(wBuf->isNeedFree){
            R(NULL == wBuf->data);
            FREE_TN(&wBuf->data,str_t,wBuf->size);
        }
    }else if(wBuf->type == TWBT_FILE){
        R(NULL == wBuf->f);
        fclose(wBuf->f);
    }

    R( FREE_T(pp,tcp_write_buffer_t)  );

    return ret;
}

int on_kq_tcp_client_error(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0,errCode = 0;
    tcp_client_t *client = NULL;
    R(NULL == ev);
    R(NULL == kqClient);
    client = (tcp_client_t *)kqClient->userData;
    R(NULL == client);
    errCode = ev->data;
    errno = errCode;
    R(tcp_client_statu_check((tcp_client_define_t *)client));
    if(client->isNeedClose){
        R(tcp_client_close(&client));
    }else{
        LOGW("kq tcp client[%d] error!\n",client->fd);
    }
    errno = 0;
    return ret;
}

int on_kq_tcp_client_event(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0;
    short filter = 0;
    tcp_client_t *client = NULL;
    R(NULL == kqClient);
    R(NULL == ev);
    client = (tcp_client_t *)kqClient->userData;
    R(NULL == client);
    filter = ev->filter;
    if(client->isNeedClose) goto error;
    switch(filter){
        case EVFILT_READ:
            G_E(tcp_client_recv(client));
            break;
        case EVFILT_WRITE:
            if(client->writeBufs->size > 0){
                G_E(tcp_send_buf((tcp_client_define_t *)client,NULL,0,0,0));
            }
            break;
        default:
            G_E(1);
    }
    if(client->isNeedClose) goto error;
end:
    return ret;
error:
    R(tcp_client_close(&client));
    goto end;
}

int on_kq_tcp_client_eof(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0;
    short filter = 0;
    tcp_client_t *client = NULL;
    R(NULL == kqClient);
    R(NULL == ev);
    client = (tcp_client_t *)kqClient->userData;
    R(NULL == client);
    R(tcp_client_close(&client));
    return ret;
}

int on_kq_tcp_client_free(kq_client_t *kqClient){
    int ret = 0;
    tcp_client_t *client = NULL;
    R(NULL == kqClient);
    client = (tcp_client_t *)kqClient->userData;
    R(NULL == client);
    R(tcp_client_close(&client));
    return ret;
}

int tcp_client_fd_init(tcp_client_t *client){
    int ret = 0,fd = 0;
    tcp_client_config_t *config = NULL;
    R(NULL == client);
    config = client->config;
    R(NULL == config);
    R(-1 == ( fd =  socket(PF_INET,SOCK_STREAM,IPPROTO_TCP) ) );
    client->fd = fd;
    if(NULL != client->kq){
        client->kqClient.fd = client->fd;
    }
    if(config->isNonBlock) R(SET_FD_NONBLOCK(fd));

    if(config->recvTimeout.tv_sec > 0 || config->recvTimeout.tv_usec > 0 ){
        R(-1 == setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&config->recvTimeout,sizeof(struct timeval)));
    }

    if(config->sendTimeout.tv_sec > 0 || config->sendTimeout.tv_usec > 0){
        R( -1 == setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&config->sendTimeout,sizeof(struct timeval)));
    }

    return ret;
}

int tcp_client_init(tcp_client_t **pp,tcp_client_config_t *config,const char *ip,unsigned int port,kq_t *kq){
    int ret = 0,errRet = 0,fd = 0;
    tcp_client_t *client = NULL;
    R(NULL == pp);
    R(NULL != *pp);
    R(NULL == config);
    R(NULL == ip);

    R(0 >= config->readBufSize);
    R(0 >= config->writeBufSize);
    R(MALLOC_T(&client,tcp_client_t));
    G_E(MALLOC_TN(&( client->readBuf ),str_t, config->readBufSize));
    mc_list_t *wBufs = NULL;
    G_E(MC_LIST_INIT(&wBufs));


    client->addr.sin_family = AF_INET;
    client->addr.sin_addr.s_addr = inet_addr(ip);
    client->addr.sin_port = htons(port);

    strncpy(client->host,ip,CLIENT_HOST_SIZE);
    client->port = port;
    client->config = config;


    client->readBufSize = config->readBufSize;
    client->writeBufs = wBufs;
    client->type = PMCT_CLIENT;

    client->kqClient.type = KCT_TCP_CLIENT;
    client->kqClient.userData = client;
    client->kqClient.onError = on_kq_tcp_client_error;
    client->kqClient.onEvent = on_kq_tcp_client_event;
    client->kqClient.onFree = on_kq_tcp_client_free;
    client->kqClient.onEof = on_kq_tcp_client_eof;
    client->kq = kq;

    G_E(tcp_client_fd_init(client));

    *pp = client;
    return ret;
error:
    errRet = ret;
    if(NULL != (*pp)){
        if( NULL != (*pp)->readBuf ){
            R(FREE_TN(&( (*pp)->readBuf ),str_t, config->readBufSize));
        }
        R(FREE_T(pp,tcp_client_t));
    }
    return errRet;
}



int tcp_client_recv(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(NULL == client->readBuf);
    unsigned int leaveBufSize = 0,enableReadSize = 0, readSize = 0;
    char *readBufPtr = NULL;
reRead:
    leaveBufSize = client->readBufSize - client->readBufUsed;
    if(leaveBufSize > 0){
        readBufPtr = client->readBuf + client->readBufUsed;
        R(tcp_read((tcp_client_define_t *)client,readBufPtr,leaveBufSize,&readSize));
        client->readBufUsed += readSize;
    }
    if(0 >= client->readBufUsed) return ret;
    R(tcp_do_message((tcp_client_define_t *)client));
    R(tcp_get_enable_read_size((tcp_client_define_t *)client,&enableReadSize));
    if(enableReadSize > 0 && client->readBufUsed < client->readBufSize){
        goto reRead;
    }
    return ret;
}

int on_send_file_completed_default(tcp_write_buffer_t *wBuf){
    int ret = 0;
    R(NULL == wBuf);
    R(NULL == wBuf->f);
    R(wBuf->size != wBuf->writedSize);
    fclose(wBuf->f);
    return ret;
}

int tcp_send_file(tcp_client_define_t *client,const char *path,cb_tcp_write_buf_completed onCompleted){
    int ret = 0;
    tcp_server_client_t *sClient = NULL;
    R( NULL == client );
    R( NULL == path );
    R( access(path,F_OK | R_OK) );

    FILE *f = fopen(path,"r");
    R(NULL == f);
    fseek(f,0,SEEK_END);
    size_t fSize = ftell(f);
    fseek(f,0,SEEK_SET);

    tcp_write_buffer_t *wBuf = NULL;
    R( MALLOC_T(&wBuf,tcp_write_buffer_t) );
    wBuf->client = client;
    wBuf->f = f;
    wBuf->size = fSize;
    wBuf->type = TWBT_FILE;
    wBuf->onCompleted = NULL == onCompleted ? on_send_file_completed_default : onCompleted;

    R( MC_LIST_PUSH(client->writeBufs,wBuf,tcp_write_buffer_t) );
    R(KQ_CLIENT_SOCKET_ENABLE_WRITE(&client->kqClient));
    return ret;
}

int tcp_get_enable_write_size(tcp_client_define_t *client,unsigned int *size){
    int ret = 0;
    R(NULL == client);
    R(NULL == size);
    int socketWBufSize;
    socklen_t len = sizeof(socketWBufSize);

    R(getsockopt(client->fd,SOL_SOCKET,SO_SNDBUF,(void *)&socketWBufSize,&len));
    int socketWBufUsedSize = 0;
    R(ioctl(client->fd,FIONWRITE,&socketWBufUsedSize));

    *size = socketWBufSize - socketWBufUsedSize;

    return ret;
}

int tcp_get_write_buf_size(tcp_client_define_t *client,unsigned int *size){
    int ret = 0;
    R(NULL == client);
    R(NULL == size);
    int socketRBufSize;
    socklen_t len = sizeof(socketRBufSize);

    R(getsockopt(client->fd,SOL_SOCKET,SO_SNDBUF,(void *)&socketRBufSize,&len));
    *size = socketRBufSize;

    return ret;
}

int tcp_get_read_buf_size(tcp_client_define_t *client,unsigned int *size){
    int ret = 0;
    R(NULL == client);
    R(NULL == size);
    int socketRBufSize;
    socklen_t len = sizeof(socketRBufSize);

    R(getsockopt(client->fd,SOL_SOCKET,SO_RCVBUF,(void *)&socketRBufSize,&len));
    *size = socketRBufSize;

    return ret;
}

int tcp_get_enable_read_size(tcp_client_define_t *client,unsigned int *size){
    int ret = 0;
    R(NULL == client);
    R(NULL == size);

    int rBufUsedSize = 0;
    R(ioctl(client->fd,FIONREAD,&rBufUsedSize));

    *size = rBufUsedSize;

    return ret;
}

int on_tcp_writed_callback(tcp_client_define_t *client,const char *buf,unsigned int size){
    int ret = 0;
    R(NULL == client);
    tcp_server_client_t *sClient = NULL;
    tcp_client_t *cClient = NULL;
    if( PMCT_SERVER_CLIENT == client->type ){
        sClient = (tcp_server_client_t *)client;
        if( NULL != (sClient->server->config->onClientWrited ) ){
            R(sClient->server->config->onClientWrited(sClient,buf));
        }
        sClient->server->wCount += size;
    }else if( PMCT_CLIENT == client->type ){
        cClient = (tcp_client_t *)client;
        if(NULL != cClient->config->onWrited){
            R( cClient->config->onWrited(cClient,buf) );
        }
    }else{
        R(1);
    }
    client->sendPackCount++;
    return ret;
}

int on_tcp_write_busy_callback(tcp_client_define_t *client){
    int ret = 0;
    R(NULL == client);
    tcp_server_client_t *sClient = NULL;
    tcp_client_t *cClient = NULL;
    if( PMCT_SERVER_CLIENT == client->type ){
        sClient = (tcp_server_client_t *)client;
        if( NULL != (sClient->server->config->onWriteBusy ) ){
            R(sClient->server->config->onWriteBusy(sClient));
        }
    }else if( PMCT_CLIENT == client->type ){
        cClient = (tcp_client_t *)client;
        if(NULL != (cClient->config->onWriteBusy)){
            R(cClient->config->onWriteBusy(cClient));
        }
    }else{
        R(1);
    }
    return ret;
}

int on_tcp_write_idle_callback(tcp_client_define_t *client){
    int ret = 0;
    R(NULL == client);
    tcp_server_client_t *sClient = NULL;
    tcp_client_t *cClient = NULL;
    if( PMCT_SERVER_CLIENT == client->type ){
        sClient = (tcp_server_client_t *)client;
        if( NULL != (sClient->server->config->onWriteIdle ) ){
            R(sClient->server->config->onWriteIdle(sClient));
        }
    }else if( PMCT_CLIENT == client->type ){
        cClient = (tcp_client_t *)client;
        if(NULL != (cClient->config->onWriteIdle)){
            R(cClient->config->onWriteIdle(cClient));
        }
    }else{
        R(1);
    }
    return ret;
}

static int _tcp_send(tcp_client_define_t *client,char *buf,unsigned int bufSize,unsigned int writtedSize,unsigned int *sendedSize){
    int ret = 0;
    unsigned int enableWriteSize = 0,currNeedWriteSize = 0,remainSize = 0,usedSize = 0;
    unsigned int actalSendSize = 0;
    R(NULL == client);
    R(NULL == buf);
    R(NULL == sendedSize);
    R( 0 >= bufSize );
    R( writtedSize >= bufSize );

    if(0 == writtedSize){
        if(PMCT_CLIENT == client->type){
            tcp_client_t *clientClient = (tcp_client_t *)client;
            if(NULL != clientClient->config->onBeforeWrite){
                R(clientClient->config->onBeforeWrite(clientClient,buf,bufSize,&usedSize));
            }
        }else if(PMCT_SERVER_CLIENT == client->type){
            tcp_server_client_t *serverClient = (tcp_server_client_t *)client;
            tcp_server_t *ts = serverClient->server;
            R(NULL == ts);
            if(NULL != ts->config->onBeforeWrite){
                R(ts->config->onBeforeWrite(client,buf,bufSize,&usedSize));
            }
        }else{
            R(1);
        }
    }

    remainSize = bufSize - writtedSize - usedSize;
    if(0 == remainSize){
        *sendedSize = bufSize - writtedSize;
        return ret;
    }

    if(NULL != client->ssl){
        R(tcp_ssl_send(client,buf,bufSize,&actalSendSize));
    }else{
        R(tcp_get_enable_write_size(client,&enableWriteSize));
        currNeedWriteSize = enableWriteSize > remainSize ? remainSize : enableWriteSize;
        errno = 0;
        actalSendSize = send(client->fd, buf + writtedSize + usedSize, currNeedWriteSize, 0);
    }
    if(client->isNeedClose) R(1);
    if(actalSendSize <= 0 || 0 != errno){
        R(tcp_client_statu_check(client));
        if(client->isNeedClose){
            R(1);
        }
    }
    *sendedSize = actalSendSize;
    return ret;
}

static int _tcp_send_file(tcp_client_define_t *client,int fd,unsigned int fSize,unsigned int hadWritedSize,unsigned int *writtedSize){
    int ret = 0;
    unsigned int enableWriteSize = 0,maxWriteSize = 0,currNeedWriteSize = 0;
    char *fBuf = NULL;
    R(NULL == client);
    R(NULL == writtedSize);
    R( 0 >= fSize );
    R( hadWritedSize > fSize );
    R(!IS_VALID_FD(fd));
    maxWriteSize = fSize - hadWritedSize;

    if(NULL != client->ssl){
        R(tcp_ssl_send_file(client,fd,hadWritedSize,maxWriteSize,writtedSize));
    }else{
        R(tcp_get_enable_write_size(client,&enableWriteSize));
        currNeedWriteSize = maxWriteSize > enableWriteSize ? enableWriteSize : maxWriteSize;
        R(MALLOC_TN(&fBuf,str_t,currNeedWriteSize));
        R(currNeedWriteSize != read(fd,fBuf,currNeedWriteSize));
        R(currNeedWriteSize != send(client->fd, fBuf, currNeedWriteSize, 0));
        R(FREE_TN(&fBuf,str_t,currNeedWriteSize));
        *writtedSize = currNeedWriteSize;
    }

    return ret;
}

int tcp_send_buf(tcp_client_define_t *client,char *buf,unsigned int size,unsigned int isNeedFree,unsigned int isNeedTimeStamp){
    int ret = 0,retErr = 0;
    R( NULL == client );
    unsigned int enableWriteSize = 0,currWrittedSize = 0,writeMaxSize = 0;
    tcp_write_buffer_t *wBuf = NULL;
    char *fBuf = NULL,*wBufData = NULL;
    tcp_server_client_t *sClient = NULL;
    struct timeval tv = {0};
    uint32_t ms = 0,second = 0;

    if(NULL != buf || 0 < size){
        if(!isNeedFree){
            R(MALLOC_TN(&wBufData,str_t,size));
            memcpy(wBufData,buf,size);
        }else{
            wBufData = buf;
        }
        R( MALLOC_T(&wBuf,tcp_write_buffer_t) );
        wBuf->client = client;
        wBuf->f = NULL;
        wBuf->data = wBufData;
        wBuf->size = size;
        wBuf->type = TWBT_MEM;
        wBuf->isNeedFree = 1;
        wBuf->writedSize = currWrittedSize;
        wBuf->isNeedTimeStamp = isNeedTimeStamp;
        if( MC_LIST_PUSH(client->writeBufs,wBuf,tcp_write_buffer_t) ){
            R(tcp_write_buffer_free(&wBuf));
        }
    }

    if(TCP_CLIENT_WRITE_BUF_BUSY_SIZE < client->writeBufs->size){
        if(time(NULL) > client->lastNotifyWriteBusy + TCP_WRITE_BUSY_SECOND_PER){
            client->lastNotifyWriteBusy = time(NULL);
            R(on_tcp_write_busy_callback(client));
        }
        printf("write busy : %u\n",client->writeBufs->size);
    }else{
        if( 0 == client->writeBufs->size ){
            R(on_tcp_write_idle_callback(client));
        }
    }

    MC_LIST_ITERATOR(client->writeBufs,tcp_write_buffer_t,{
            if(item->type == TWBT_MEM){
                G_E(_tcp_send(client,item->data,item->size,item->writedSize,&currWrittedSize));
            }else if(item->type == TWBT_FILE){
                G_E(_tcp_send_file(client,fileno(item->f),item->size,item->writedSize,&currWrittedSize));
            }else{
                G_E(1);
            }
            if(0 == currWrittedSize) goto end;
            item->writedSize += currWrittedSize;
            client->sendTotalSize += currWrittedSize;
            currWrittedSize = 0;
            G_E(item->size < item->writedSize);
            if(item->size == item->writedSize){
                G_E(on_tcp_writed_callback(client,item->data,item->size));
                G_E(MC_LIST_RM_DATA(client->writeBufs,item,tcp_write_buffer_t));
                G_E(tcp_write_buffer_free(&item));
            }else{
                goto end;
            }
    });

end:
    if(client->writeBufs->size > 0){
        R(KQ_CLIENT_SOCKET_ADD_WRITE_ONCE(&client->kqClient));
    }
    return ret;
error:
    retErr = ret;
    if(isNeedFree) FREE_TN(&buf,str_t,size);
    return retErr;
}

