#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TCP_SERVER_CLIENT
#else
#define MODULE_CURR MODULE_NET_TCP_SERVER_CLIENT
#endif

static int tcp_server_client_type_init(tcp_server_client_t *client,unsigned int *isInited,unsigned int *used){
    int ret = 0;
    unsigned int isSuccessHttpParse = 0;
    http_req_t *http = NULL;
    R(NULL == client);
    R(NULL == isInited);
    R(NULL == client->httpReq);
    http = client->httpReq;

    R(http_request_parse(client->readBuf,client->readBufUsed,http));
    if(0 == http->isRecvCompleted) goto reInit;

    /* init client connect type */
    if( http->isHttp ){
        if( NULL != http->webSocketKey.data ){
            client->connectType = TCCT_WEB_SOCKET;
            R(ws_server_hand_shake(client,http->webSocketKey.data,http->webSocketKey.size));
            client->isWebSocketInited = 1;
            *used = http->size;
        }else{
            client->connectType = TCCT_HTTP;
        }
    }else{
        client->connectType = TCCT_TCP;
    }
    *isInited = 1;
    return ret;
reInit:
    *isInited = 0;
    if(http->size > client->readBufSize){
        client->httpRsp->code = 413; 
        LOGI("request entity too large : %u > %u\n",http->size,client->readBufSize);
        R(http_rsp(client));
        client->isWaitClose = 1;
    }
    return 0;
}

int on_tcp_server_client_pack_size_too_large(tcp_server_client_t *client,unsigned int packSize,unsigned int maxSize){
    int ret = 0;
    tcp_server_t *ts = NULL;

    R(NULL == client->server);
    R(NULL == client->server->config);
    ts = client->server;

    if(NULL != ts->config->onClientPackTooLarge){
        R(ts->config->onClientPackTooLarge(client,packSize,maxSize));
    }else{
        TCP_CLIENT_SET_CLOSE(client);
    }
    return ret;
}

int tcp_server_client_close(tcp_server_client_t **pp){
    int ret = 0;
    R(NULL == pp);
    R(NULL == (*pp));
    tcp_server_client_t *client = *(tcp_server_client_t **)pp;
    R(NULL == client->server);
    tcp_server_t *ts = client->server;
    
    LOGI("client[%d] closed : %s[%d] => online[%u]\n",client->fd,client->host,client->port,ts->clients->size);

    R(MC_LIST_RM_DATA(ts->clients,client,tcp_server_client_t));


    if(NULL != ts->config->onClientClosed){
        ts->config->onClientClosed(client);
    }

    if(NULL != client->readBuf){
        R(FREE_TN(&client->readBuf,str_t,client->readBufSize));
    }

    if(NULL != client->writeBufs){
        tcp_write_buffer_t *wBuf = NULL;
        int i = 0;
        while(client->writeBufs->size){
            R(NULL == (wBuf = MC_LIST_FIRST(client->writeBufs)));
            if(NULL != wBuf->f) fclose(wBuf->f);
            R(MC_LIST_RM_DATA(client->writeBufs,wBuf,tcp_write_buffer_t));
            R( tcp_write_buffer_free(&wBuf) );
        }
        R( MC_LIST_FREE( &(client->writeBufs),tcp_write_buffer_t ) );
    }
    client->writeBufs = NULL;
    if(NULL != client->ssl){
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }
    if(NULL != client->httpReq){
        R(http_req_free(&client->httpReq));
    }
    if(NULL != client->httpRsp){
        R(http_rsp_free(&client->httpRsp));
    }
    R(kq_client_close(&client->kqClient));
    R(FREE_T(pp,tcp_server_client_t));
    MC_PRINT_CALL_STACK();
    return ret;
}

int tcp_server_client_ignore_data(tcp_server_client_t *client){
    int ret = 0;
    char buf[1024] = {0};
    unsigned int currNeedReadSize = 0,readSize = 0;
    R(NULL == client);
    
    if(client->needIgnoreDataSize > 0){
        if(client->needIgnoreDataSize > client->readBufUsed){
            client->needIgnoreDataSize -= client->readBufUsed;
            client->readBufUsed = 0;
            currNeedReadSize = 1024 > client->needIgnoreDataSize ? client->needIgnoreDataSize : 1024;
            R(tcp_read((tcp_client_define_t *)client, buf, currNeedReadSize, &readSize));
            client->needIgnoreDataSize -= readSize;
        }else{
            client->readBufUsed -= client->needIgnoreDataSize;
            memcpy(client->readBuf , client->readBuf + client->needIgnoreDataSize, client->readBufUsed);
            client->needIgnoreDataSize = 0;
        }
    }
    return ret;
}

int tcp_server_client_recv(tcp_server_client_t *client){
    int ret = 0;
    unsigned int readSize = 0,reReadCount = 0,typeInitUsedSize = 0;
    unsigned int used = 0,remainBufSize = 0,needReadSize = 0,isClientTypeInited = 0,wsNeedDataSize = 0;
    unsigned int wsHeaderParseOk = 0,enableReadSize = 0;
    tcp_server_t *ts = NULL;
    tcp_server_config_t *config = NULL;
    char *ignoreData = NULL;

    R(NULL == client);
    ts = client->server;
    R(NULL == ts);
    config = ts->config;
    R(NULL == config);
                
    if(NULL != client->ssl && 0 == client->isSslHandShaked){
        R(tcp_server_client_ssl_hand_shake(client));
        return 0;
    }

    /*
     * 1. check ignore data
     * 2. read web socket header data 
     * 3. read data and parse web socket data
     * 4. client type init
     * 5. dispath message
     * */
    R(tcp_server_client_ignore_data(client));
    if(client->needIgnoreDataSize > 0) return 0;
    
    needReadSize = client->readBufSize - client->readBufUsed;
    if( TCCT_WEB_SOCKET == client->connectType ){
        if( WSHP_CLOSE == client->wsHeader.statu  ) goto finish;
        if( WSHP_MASK_OK != client->wsHeader.statu  ){
            R(ws_server_header_parse(client,&wsHeaderParseOk));
            if( WSHP_CLOSE == client->wsHeader.statu  ) goto finish;
            if(0 == wsHeaderParseOk) return 0;
        }
        wsNeedDataSize = client->wsHeader.dataSize - client->wsHeader.hadReadDataSize;
        needReadSize  = needReadSize > wsNeedDataSize ? wsNeedDataSize : needReadSize;
    }

    if(0 < needReadSize){
        readSize = 0;
        R(tcp_read((tcp_client_define_t *)client,client->readBuf + client->readBufUsed,needReadSize,&readSize));
        if(0 < readSize){
            client->readBufUsed += readSize;
            client->server->rCount += readSize;
            client->recvTotalSize += readSize;
            if( TCCT_WEB_SOCKET == client->connectType ){
                if(client->wsHeader.dataSize <= 0) goto finish;
                R(client->wsHeader.dataSize < client->wsHeader.hadReadDataSize);
                R(ws_server_data_parse(client,readSize));
            }
        }
    }

    if(client->isWaitClose) client->readBufUsed = 0;
    if(client->readBufUsed == 0) return 0;
    
    if( TCCT_NONE == client->connectType ){
        isClientTypeInited = 0;
        typeInitUsedSize = 0;
        R( tcp_server_client_type_init(client,&isClientTypeInited,&typeInitUsedSize) );
        if(0 == isClientTypeInited) return 0;
        R(typeInitUsedSize > client->readBufUsed);
        if(NULL != ts->config->onClientTypeInited){
            R(ts->config->onClientTypeInited(client));
        }
        client->readBufUsed -= typeInitUsedSize;
    }
    if(client->readBufUsed == 0) return 0;

    R(tcp_do_message((tcp_client_define_t *)client));
    return ret;
finish:
    TCP_CLIENT_SET_CLOSE(client);
    return 0;
}

int on_kq_tcp_server_client_error(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0,errCode = 0;
    tcp_server_client_t *client = NULL;
    R(NULL == ev);
    R(NULL == kqClient);
    client = (tcp_server_client_t *)kqClient->userData;
    R(NULL == client);
    errCode = ev->data;
    errno = errCode;
    R(tcp_client_statu_check((tcp_client_define_t *)client));
    if(client->isNeedClose){
        R(tcp_server_client_close(&client));
    }else{
        LOGW("kq tcp server client[%d] error!\n",client->fd);
    }
    errno = 0;
    return ret;
}

int on_kq_tcp_server_client_eof(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0,errCode = 0;
    tcp_server_client_t *client = NULL;
    R(NULL == ev);
    R(NULL == kqClient);
    client = (tcp_server_client_t *)kqClient->userData;
    R(NULL == client);
    errCode = ev->data;
    errno = errCode;
    R(tcp_server_client_close(&client));
    errno = 0;
    return ret;
}

int on_kq_tcp_server_client_event(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0,retErr = 0;
    short filter = 0;
    tcp_server_client_t *client = NULL;
    unsigned int enableReadSize = 0;
    int enableSslReadSize = 0;
    R(NULL == kqClient);
    R(NULL == ev);
    client = (tcp_server_client_t *)kqClient->userData;
    R(NULL == client);
    filter = ev->filter;
    if(client->isNeedClose) goto error;
    switch(filter){
        case EVFILT_READ:
            G_E(tcp_server_client_recv(client));
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
    return retErr;
error:
    retErr = ret;
    R(tcp_server_client_close(&client));
    goto end;
}

int on_kq_tcp_server_client_free(kq_client_t *kqClient){
    int ret = 0;
    tcp_server_client_t *client = NULL;
    R(NULL == kqClient);
    client = (tcp_server_client_t *)kqClient->userData;
    R(NULL == client);
    R(tcp_server_client_close(&client));
    return ret;
}

int tcp_server_client_init(tcp_server_t *ts,int fd,struct sockaddr_in *addr){
    int ret = 0,errRet = 0;
    struct linger soLinger = {1,0};
    tcp_server_client_t *client = NULL;
    tcp_server_config_t *config = NULL;
    unsigned int readBufSize = 0;

    R(NULL == ts);
    R(NULL == ts->clients);
    config = ts->config;
    R(NULL == config);
    R(NULL == addr);

    R(MALLOC_T(&client,tcp_server_client_t));
    client->server = ts;
    client->fd = fd;
    client->type = PMCT_SERVER_CLIENT;
    client->port = ntohs(addr->sin_port);
    G_E(0 == inet_ntoa_r(addr->sin_addr,client->host,CLIENT_HOST_SIZE));
    readBufSize = config->readPackMax * TCP_SERVER_CLIENT_BUF_LV;
    
    R(SET_FD_NONBLOCK(ts->fd)); 

    client->kqClient.fd = fd;
    client->kqClient.type = KCT_TCP_SERVER_CLIENT;
    client->kqClient.userData = client;
    client->kqClient.onError = on_kq_tcp_server_client_error;
    client->kqClient.onEvent = on_kq_tcp_server_client_event;
    client->kqClient.onFree = on_kq_tcp_server_client_free;
    client->kqClient.onEof = on_kq_tcp_server_client_eof;
    G_E(kq_client_registe(ts->kq,&client->kqClient));

    memcpy(&client->addr,addr,sizeof(struct sockaddr_in));
    G_E(setsockopt(fd,SOL_SOCKET,SO_LINGER,&soLinger,sizeof(soLinger)));
    G_E(tcp_get_read_buf_size((tcp_client_define_t *)client,&client->socketRecvBufSize));
    G_E(tcp_get_write_buf_size((tcp_client_define_t *)client,&client->socketSendBufSize));

    G_E(MALLOC_TN(&(client->readBuf),str_t,readBufSize));
    client->readBufSize = readBufSize;

    G_E(MC_LIST_INIT(&(client->writeBufs)));

    G_E(MC_LIST_PUSH(ts->clients,client,tcp_server_client_t));
    G_E(http_req_new(&client->httpReq));
    G_E(http_rsp_new(&client->httpRsp));

    client->connectStatus = TCCS_CONNECTED;
    LOGI("client[%d] connected : %s[%d] => online[%u]\n",fd,client->host,client->port,ts->clients->size);
    if( NULL != ts->config->onClientConnected ){
        G_E(ts->config->onClientConnected(client));
    }
    if(config->isUseSsl){
        G_E(tcp_server_client_ssl_hand_shake(client));
    }
    return ret;
error:
    errRet = ret;
    if(IS_VALID_FD(fd)) close(fd);
    if(NULL != client){
        if(NULL != client->readBuf){
            R(FREE_TN(&client->readBuf,str_t,client->readBufSize));
        }
        if(NULL != client->writeBufs){
            R( MC_LIST_FREE( &(client->writeBufs) ,tcp_write_buffer_t) );
        }
        R(FREE_T(&client,tcp_server_client_t));
    }
    return errRet;
}
