#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TIMER
#else
#define MODULE_CURR MODULE_NET_TIMER
#endif

typedef enum{
    TS_NONE = '0',
    TS_G,
    TS_W,
    TS_R,
    TS_A,
    TS_AR,
    TS_F,
    TS_FR,
    TS_MAX
}stimer_status_e;
#define STIMER_STATUS_CHECK(ts) ( TS_NONE >= (ts) || TS_MAX <= (ts) )

#define STIMER_POS_G(t) ( (char *)(t) )
#define STIMER_POS_W(t) ( (char *)(t) + 16 )
#define STIMER_POS_R(t) ( (char *)(t) + 16 * 2 )
#define STIMER_POS_ACK(t) ( (char *)(t) + 16 * 3 )
#define STIMER_POS_ACK_R(t) ( (char *)(t) + 16 * 4 )
#define STIMER_POS_FIN(t) ( (char *)(t) + 16 * 5 )
#define STIMER_POS_FIN_R(t) ( (char *)(t) + 16 * 6 )
#define STIMER_POS_STATUS(t) ( (char *)(t) + 16 * 7 )
#define STIMER_POS_TYPE(t,type) ( (char *)(t) + 16 * ( (type) - 1 ) )

#define STIMER_GET_STATUS(t) ( (t)->status )
#define STIMER_SET_STATUS(t,type) ( (t)->status = (char)( (type) + '0' ) )
#define STIMER_NOW_2_BUF(buf)\
    ({\
     struct timeval tv = {0};\
     gettimeofday(&tv,NULL);\
     (stimer_2_buf(&tv,buf));\
     })

int stimer_2_buf(struct timeval *t,const char *dist){
    int ret = 0;
    R(NULL == t);
    R(NULL == dist);
    char buf[17] = {0};
    snprintf(buf, 17, "%010d%06d",(int)(t->tv_sec),(int)(t->tv_usec) );
    memcpy((void *)dist,buf,16);
    return ret;
}

int stimer_init(stimer_t **t){
    int ret = 0;
    R(NULL == t);
    R(NULL != (*t));
    R(MALLOC_T(t,stimer_t));
    memset( (*t),'0',sizeof(stimer_t) );
    R(STIMER_NOW_2_BUF( STIMER_POS_G( (*t) ) ));
    (*t)->status = TS_G;
    memcpy(STIMER_POS_TAG_SUFFIX((*t)),STIMER_TAG_SUFFIX,8);
    return ret;
}

int stimer_free(stimer_t **t){
    int ret = 0;
    R(NULL == t);
    R(NULL == (*t));
    R(FREE_T(t,stimer_t));
    return ret;
}

int stimer_set_now(stimer_t *t,stimer_type_e type){
    int ret = 0;
    R(NULL == t);
    R(STIMER_TYPE_CHECK(type));
    R(STIMER_NOW_2_BUF( STIMER_POS_TYPE(t,type) ));
    STIMER_SET_STATUS(t,type);
    return ret;
}

int stimer_set_by_buf(stimer_t *t,stimer_type_e type,const char *buf){
    int ret = 0;
    R(NULL == t);
    R(NULL == buf);
    R(STIMER_TYPE_CHECK(type));
    memcpy( STIMER_POS_TYPE(t,type),buf,16 ) ;
    return ret;
}

int stimer_get(stimer_t *t,stimer_type_e type,struct timeval *dist){
    int ret = 0;
    R(NULL == t);
    R(NULL == dist);
    R(STIMER_TYPE_CHECK(type));

    char tvSecStr[11] = {0},tvUsecStr[7] = {0};
    char *stopAt = NULL;
    memcpy(tvSecStr, STIMER_POS_TYPE(t,type) ,10);
    memcpy(tvUsecStr,STIMER_POS_TYPE(t,type) + 10,6);
    dist->tv_sec = strtol(tvSecStr,&stopAt,10);
    dist->tv_usec = strtol(tvUsecStr,&stopAt,10);

    return ret;
}

int stimer_diff(stimer_t *t,stimer_type_e from,stimer_type_e to,double *dist){
    int ret = 0;
    R(NULL == t);
    R(NULL == dist);
    R(STIMER_TYPE_CHECK(from));
    R(STIMER_TYPE_CHECK(to));
    struct timeval tv1 = {0};
    struct timeval tv2 = {0};
    R(stimer_get(t,from,&tv1));
    R(stimer_get(t,to,&tv2));
    R(timeval_diff(&tv2,&tv1,dist));
    return ret;
}

int stimer_set_run_status(stimer_t *t){
    int ret = 0;
    R(NULL == t);
    R(STIMER_STATUS_CHECK( t->status ));
    switch(t->status){
        case TS_NONE : R(1); break;
        case TS_G : R(stimer_set_now(t,T_W)); break;
        case TS_W : R(stimer_set_now(t,T_R)); break;
        case TS_R : R(stimer_set_now(t,T_A)); break;
        case TS_A : R(stimer_set_now(t,T_AR)); break;
        case TS_AR : R(stimer_set_now(t,T_F)); break;
        case TS_F : R(stimer_set_now(t,T_FR)); break;
        default: R(2);
    }
    return ret;
}

int stimer_set_pack(pack_t *pack){
    int ret = 0;
    R(NULL == pack);
    unsigned int packSize = PACK_GET_SIZE(pack);
    R( !STIMER_EXISTS((char *)pack,packSize) );
    
    stimer_t *st = (stimer_t *)STIMER_POS_FROM_BUF( pack,packSize );
    R(stimer_set_run_status( st ));

    return ret;
}

int stimer_set_pack_from_pack(pack_t *old,pack_t *new){
    int ret = 0;
    R(NULL == old);
    R(NULL == new);
    unsigned int oldPackSize = PACK_GET_SIZE(old);
    R( !STIMER_EXISTS((char *)old,oldPackSize) );

    unsigned int newPackSize = PACK_GET_SIZE(new);
    if( !STIMER_EXISTS( (char *)new, newPackSize ) ){
        stimer_t *oldSt = (stimer_t *)STIMER_POS_FROM_BUF( old,oldPackSize );
        R(STIMER_CHECK(oldSt));
        R(pack_add_data(new,(void *)oldSt,sizeof(stimer_t)));
        newPackSize = PACK_GET_SIZE(new);
    }
    
    stimer_t *newSt = (stimer_t *)STIMER_POS_FROM_BUF( new,newPackSize );
    R(stimer_set_run_status( newSt ));

    return ret;
}

int stimer_get_from_buf(const char *buf,unsigned int size,stimer_t **dist){
    int ret = 0;
    R( !STIMER_EXISTS(buf,size) );
    (*dist) = (stimer_t *)STIMER_POS_FROM_BUF(buf,size);
    return ret;
}

int stimer_print(stimer_t *t){
    int ret = 0;
    R(NULL == t);
    char g[17] = {0},w[17] = {0},r[17] = {0},ack[17] = {0}, ackR[17] = {0}, fin[17] = {0}, finR[17] = {0};
    memcpy(g,t->g,16);
    memcpy(w,t->w,16);
    memcpy(r,t->r,16);
    memcpy(ack,t->ack,16);
    memcpy(ackR,t->ackR,16);
    memcpy(fin,t->fin,16);
    memcpy(finR,t->finR,16);
    printf("timer : g[%s]\tw[%s]\tr[%s]\tack[%s]\tackr[%s]\tfin[%s]\tfinr[%s]\tstatus[%c]\n",
            g,w,r,ack,ackR,fin,finR,t->status
            );
    return ret;
} 

int stimer_print_pack(pack_t *pack){
    int ret = 0;
    R(NULL == pack);
    unsigned int packSize = PACK_GET_SIZE(pack);
    RL(!STIMER_EXISTS( pack, packSize) ,"[%u]\t(%s)\t(%s)\t(%s)\n",packSize,(char *)pack,((char *)pack) + packSize, ((char *)pack) + packSize - STIMER_TAG_SUFFIX_SIZE ) ;
    char no[sizeof(pack_no_t) + 1] = {0};
    memcpy(no,PACK_POS_NO(pack),sizeof(pack_no_t));
    printf("pack[%s]\t",no);
    
    stimer_t *st = NULL;
    R(stimer_get_from_buf((const char *)(pack),packSize,&st));
    stimer_print(st);

    return ret;
}

int stimer_send(void *c,client_type_e t, const char *data,unsigned int size){
    int ret = 0,errRet = 0;
    R(NULL == c);
    R(NULL == data);
    R(CLIENT_TYPE_CHECK(t));
    R(0 >= size);


    unsigned int needSendSize = size + sizeof(stimer_t);
    char *buf = NULL;
    R(MALLOC_TN(&buf,str_t,needSendSize));
    memcpy(buf,data,size);

    stimer_t *st = NULL;
    G_E(stimer_init(&st));
    memcpy(buf + size, (char *)st, sizeof(stimer_t));
    
    if( PMCT_SERVER_CLIENT == t ){
        R(pm_server_client_send((tcp_server_client_t *)c,buf,needSendSize));
    }else if( PMCT_CLIENT == t ){
        R(pm_client_send((tcp_client_t *)c,buf,needSendSize));
    }else{
        G_E(1);
    }

    R(FREE_T(&st,stimer_t));
    R(FREE_TN(&buf,str_t,needSendSize));

    return ret;
error:
    errRet = ret;
    if(NULL != buf) R(FREE_TN(&buf,str_t,needSendSize));
    if(NULL != st) R(FREE_T(&st,stimer_t));
    return errRet;
}
