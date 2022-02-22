#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TCP_SERVER
#else
#define MODULE_CURR MODULE_NET_TCP_SERVER
#endif

#define KQ_EVENT_SIZE(config) ( ( (config)->clientMax + 1) * 2 )
#define IS_VALID_CLIENT(client) ( NULL != (client)  && NULL != (client)->server )

int tcp_server_stop_accept(tcp_server_t *ts){
    int ret = 0;
    R(NULL == ts);
    if(2 < ts->fd && IS_VALID_FD(ts->fd)){
        R(close(ts->fd));
        ts->fd = 0;
        ts->isNeedStart = 0;
        LOGI("tcp server[%d] stoped!\n",ts->fd);
    }
    return ret;
}

int tcp_server_start_accept(tcp_server_t *ts){
    int ret = 0;
    R(NULL == ts);
    if(0 == ts->isNeedStart) return ret;
    tcp_server_config_t *config = ts->config;
    R(NULL == config);
    if( 2 < ts->fd && IS_VALID_FD(ts->fd) ) return ret;
    R(-1 == (ts->fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)));

    int flag = 1,socketFlags = 0;
    R(setsockopt(ts->fd,SOL_SOCKET,SO_REUSEPORT,&flag,sizeof(flag)));
    R(SET_FD_NONBLOCK(ts->fd)); 

    R(bind(ts->fd,(struct sockaddr *)&(ts->addr),sizeof(struct sockaddr_in)));
    R(listen(ts->fd,config->backLog));

    ts->kqClient.fd = ts->fd;
    R(kq_client_registe(ts->kq,&ts->kqClient));

    LOGI("tcp server[%d] started!\n",ts->fd);
    if(NULL != config->onServerStarted){
        R(config->onServerStarted());
    }

    return ret;
}

static int tcp_server_accept(tcp_server_t *ts){
    int ret = 0,fd = 0;
    struct sockaddr_in addr = {0};
    socklen_t sockLen = sizeof(struct sockaddr_in);
    R(NULL == ts);
    fd = accept(ts->fd,(struct sockaddr *)&addr,&sockLen);
    if(-1 == fd){
        switch(errno){
            case EBADF:
                R(tcp_server_start_accept(ts));
                break;
            case ENOTSOCK:
            case EINVAL:
            case EFAULT:
                R(1);
                break;
            case EINTR:
            case EMFILE:
            case ENFILE:
            case EWOULDBLOCK:
                ts->isNeedReAccept = 1;
                break;
            case ECONNABORTED:
                break;
        }
        errno = 0;
    }else{
        R(tcp_server_client_init(ts,fd,&addr));
    }
    return ret;
}


int on_kq_tcp_server_event_error(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0,errCode = 0;
    R(NULL == kqClient);
    R(NULL == ev);
    tcp_server_t *ts = (tcp_server_t *)kqClient->userData;
    R(NULL == ts);
    errCode = ev->data;
    switch(errCode){
        case EBADF:
            R(tcp_server_start_accept(ts));
            break;
        default:
            errno = errCode;
            LOGE("tcp server error\n");
            break;
    }
    errno = 0;
    return ret;
}

int on_kq_tcp_server_event(kq_client_t *kqClient,kq_ev_t *ev){
    int ret = 0,size = 0, i = 0;
    R(NULL == kqClient);
    R(NULL == ev);
    tcp_server_t *ts = (tcp_server_t *)kqClient->userData;
    R(NULL == ts);
    size = ev->data;
    for(; i < size; i++){
        R(tcp_server_accept(ts));
    }
    return ret;
}

int on_kq_tcp_server_free(kq_client_t *kqClient){
    int ret = 0;
    R(NULL == kqClient);

    return ret;
}

static int on_tcp_server_idle(kq_t *kq){
    int ret = 0;
    tcp_server_t *ts = NULL;
    R(NULL == kq);
    ts = (tcp_server_t *)kq->userData;
    R(NULL == ts);
    R(NULL == ts->config);
    if(ts->isNeedReAccept){
        R(tcp_server_accept(ts));
    }else{
        if(NULL != ts->config->onIdle){
            R(ts->config->onIdle());
        }
    }
    return ret;
}

int tcp_server_init(tcp_server_t **pp,tcp_server_config_t *config){
    int ret = 0,errRet = 0;
    struct timespec kqTimeOut = {0};
    R(NULL == pp);
    R(NULL != (*pp));
    R(NULL == config);
    R(NULL == config->ip);
    R(0 >= config->backLog);
    R(NULL == config->onClientRecved);

    kqTimeOut.tv_nsec = config->kqTimeout;

    R(MALLOC_T(pp,tcp_server_t));
    tcp_server_t *ts = *pp;
    G_E(kq_init(&ts->kq,config->clientMax,TCP_SERVER_KQ_MAX_MS_LOOP_PER,&kqTimeOut));
    ts->config = config;
    
    ts->kq->onIdle = on_tcp_server_idle;
    ts->kq->onReInited = config->onKqReInited;
    ts->kq->onBusy = config->onKqLoopBusy;
    ts->kq->onError = config->onKqError;
    ts->kq->userData = ts;

    ts->kqClient.type = KCT_TCP_SERVER;
    ts->kqClient.userData = ts;
    ts->kqClient.onError = on_kq_tcp_server_event_error;
    ts->kqClient.onEvent = on_kq_tcp_server_event;
    ts->kqClient.onFree = on_kq_tcp_server_free;

    ts->addr.sin_family = AF_INET;
    ts->addr.sin_port = htons(config->port);
    ts->addr.sin_addr.s_addr = inet_addr(config->ip);
    G_E(MC_LIST_INIT(&ts->clients));
    
    if(config->isUseSsl){
        G_E(tcp_server_ssl_init(ts));
    }

    ts->isNeedStart = config->isInitedStartAccept;
    G_E(tcp_server_start_accept(ts));

    return ret;
error:
    errRet = ret;
    if(NULL != ts) {
        if(NULL != ts->kq ) R(kq_free(&ts->kq));
        R(FREE_T(pp,tcp_server_t));
    }
    return errRet;
}


int tcp_server_exit(tcp_server_t **ts){
    int ret = 0;
    R(NULL == ts);
    R(NULL == (*ts));
    R(NULL == (*ts)->config);
    int i = 0;

    tcp_server_config_t *config = (*ts)->config;
    tcp_server_client_t *client = NULL;
    cb_tcp_server_exited cbServerExit = config->onServerExited;

    MC_LIST_FOREACH(client,(*ts)->clients){
        R(tcp_server_client_close(&client));
    }

    R(MC_LIST_FREE(&((*ts)->clients),tcp_server_client_t));
    R(close((*ts)->fd));
    if(NULL != (*ts)->events ){
        R(FREE_TN(&((*ts)->events),ev_t, KQ_EVENT_SIZE(config) ));
    }
    R(FREE_T(ts,tcp_server_t));
    if(NULL != cbServerExit){
        R(cbServerExit());
    }
    return ret;
}


int tcp_server_run(tcp_server_t *ts){
    int ret = 0;
    R(NULL == ts);
    R(NULL == ts->kq);
    R(kq_run(ts->kq));
    return ret;
}

void on_tcp_signal(int signalNo){
    int ret = 0;
}

int tcp_signal_init(sig_t onSignal){
    int ret = 0;
    if(NULL == onSignal){
        signal(SIGTERM,on_tcp_signal);
        signal(SIGINT,on_tcp_signal);
        signal(SIGPIPE,SIG_IGN);
    }else{
        signal(SIGTERM,onSignal);
        signal(SIGINT,onSignal);
        signal(SIGPIPE,onSignal);
    }
    return ret;
}


