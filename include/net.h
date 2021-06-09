#ifndef __NET_H__
#define __NET_H__
#include "mc.h"
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>
#include <string.h>
#include <errno.h>
#define TCP_SERVER_CLIENT_BUF_LV 10
typedef struct pack_manager_s pack_manager_t;

/*=========== TCP_SERVER ================*/
typedef struct tcp_server_client_s tcp_server_client_t;
typedef int (*cb_tcp_server_client_recved)(tcp_server_client_t *client);
typedef int (*cb_tcp_server_client_closed)(tcp_server_client_t *client);
typedef struct {
    const char *ip;
    unsigned int port;
    unsigned int backLog;
    unsigned int clientMax;
    unsigned int readPackMax;
    unsigned int writePackMax;
    unsigned int kqTimeout;
    cb_tcp_server_client_recved cbClientRecved;
    cb_tcp_server_client_closed cbClientClosed;
}tcp_server_config_t;

typedef struct kevent ev_t;
typedef struct{
    tcp_server_config_t *config;
    struct sockaddr_in addr;
    int kq;
    int fd;
    arr_t *clients;
    ev_t *events;
}tcp_server_t;


struct tcp_server_client_s{
    int fd;
    struct sockaddr_in addr;
    tcp_server_t *server;
    unsigned int index;
    unsigned int enableReadSize;
    unsigned int enableWriteSize;
    char *readBuf;
    unsigned int readBufSize;
    unsigned int readBufUsed;
    char *writeBuf;
    unsigned int writeBufSize;
    unsigned int writeBufUsed;
    struct kevent evRead;
    struct kevent evWrite;
    pack_manager_t *pm;
    unsigned int isClosing;
};

typedef int (*cb_tcp_server_run_error)(struct kevent *ev);

int tcp_server_init(tcp_server_t **ts,tcp_server_config_t *config);
int tcp_server_client_close(tcp_server_client_t **pp);
int tcp_server_exit(tcp_server_t **ts);
int tcp_server_run(tcp_server_t *ts,cb_tcp_server_run_error callBack);
int tcp_server_client_write(tcp_server_client_t *client,char *buf,size_t bufSize);

#define IS_VALID_FD(fd) (-1 != fcntl( (fd),F_GETFD ) )

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

/*==============TCP_CLIENT=====================*/
typedef struct tcp_client_s tcp_client_t;
typedef int (*cb_tcp_client_closed)(tcp_client_t *client);
struct tcp_client_s{
    int fd;
    struct sockaddr_in addr;
    char *readBuf;
    unsigned int readBufSize;
    unsigned int readBufUsed;
    char *writeBuf;
    unsigned int writeBufSize;
    unsigned int writeBufUsed;
    pack_manager_t *pm;
    unsigned int isClosing;
    cb_tcp_client_closed onClosed;
};

int tcp_client_init(tcp_client_t **pp,unsigned int rBufSize,unsigned int wBufSize,cb_tcp_client_closed cbClosed);
int tcp_client_close(tcp_client_t **pp);
int tcp_client_connect(tcp_client_t *client,const char *ip,unsigned int port);
int tcp_client_recv(tcp_client_t *client);
int tcp_client_send(tcp_client_t *client,char *buf,size_t size);

/*================PACK=====================*/
#define PACK_NO_SIZE sizeof(pack_no_t) 
#define PACK_SIZE_SIZE 4
#define PACK_TYPE_SIZE 4
#define PACK_TIME_GLOBAL_SIZE 16
#define PACK_TIME_WRITE_SIZE 16
#define PACK_TIME_RECV_SIZE 16
#define PACK_TIME_ACK_WRITE_SIZE 16
#define PACK_TIME_ACK_RECV_SIZE 16
#define PACK_TIME_FIN_WRITE_SIZE 16
#define PACK_TIME_FIN_RECV_SIZE 16
#define PACK_TIME_SIZE sizeof(pack_time_t)

#define PACK_DATA_SIZE_MAX 1024
#define PACK_SIZE_MIN (PACK_SIZE_SIZE + PACK_TYPE_SIZE + PACK_NO_SIZE + PACK_TIME_SIZE )
#define PACK_SIZE_MAX (PACK_SIZE_MIN + PACK_DATA_SIZE_MAX)

#define PACK_NO_POS(pack) ( (char *)(pack) + PACK_SIZE_SIZE + PACK_TYPE_SIZE )
#define PACK_DATA_POS(pack) ( (char *)(pack) + PACK_SIZE_MIN )
#define PACK_TIME_GLOBAL_POS(pack) ( PACK_NO_POS(pack) + PACK_NO_SIZE )
#define PACK_TIME_WRITE_POS(pack) ( PACK_TIME_GLOBAL_POS(pack) + PACK_TIME_GLOBAL_SIZE )
#define PACK_TIME_RECV_POS(pack) ( PACK_TIME_WRITE_POS(pack) + PACK_TIME_WRITE_SIZE )
#define PACK_TIME_ACK_WRITE_POS(pack) ( PACK_TIME_RECV_POS(pack) + PACK_TIME_RECV_SIZE )
#define PACK_TIME_ACK_RECV_POS(pack) ( PACK_TIME_ACK_WRITE_POS(pack) + PACK_TIME_ACK_WRITE_SIZE )
#define PACK_TIME_FIN_WRITE_POS(pack) ( PACK_TIME_ACK_RECV_POS(pack) + PACK_TIME_ACK_RECV_SIZE )
#define PACK_TIME_FIN_RECV_POS(pack) ( PACK_TIME_FIN_WRITE_POS(pack) + PACK_TIME_FIN_WRITE_SIZE )


#define PACK_SET_NO(pack,_buf_) \
    memcpy( PACK_NO_POS(pack), ((char *)(_buf_)),PACK_NO_SIZE)

#define PACK_SET_TYPE(pack,type)\
    do{\
        char _bufType_[PACK_TYPE_SIZE + 1] = {0};\
        char fmt[32] = {0};\
        snprintf(fmt,32,"%%0%dd",PACK_TYPE_SIZE);\
        snprintf(_bufType_,PACK_TYPE_SIZE + 1,fmt,(type) );\
        memcpy( ((char *)(pack)) + PACK_SIZE_SIZE , _bufType_ ,PACK_TYPE_SIZE);\
    }while(0)

#define PACK_SET_SIZE(pack,size) \
    do{\
        char _bufSize_[PACK_SIZE_SIZE + 1] = {0};\
        char fmtSize[32] = {0};\
        snprintf(fmtSize,32,"%%0%dd",PACK_SIZE_SIZE);\
        snprintf(_bufSize_,PACK_SIZE_SIZE + 1,fmtSize,(size));\
        memcpy( ((char *)(pack)), _bufSize_,PACK_SIZE_SIZE);\
    }while(0)

#define TIMEVAL_2_BUF(_tv_) \
    ({\
        char buf[17] = {0};\
        snprintf( buf, 17, "%010d%06d",(int)((_tv_).tv_sec),(int)((_tv_).tv_usec ));\
        (buf);\
     })

#define TIMEVAL_BUF_GENERATE() \
    ({\
        struct timeval t = {0};\
        gettimeofday(&t,NULL);\
        (TIMEVAL_2_BUF(t));\
     })

#define PACK_SET_TIME_GLOBAL(pack) \
    memcpy(PACK_TIME_GLOBAL_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_GLOBAL_SIZE)

#define PACK_SET_TIME_WRITE(pack) \
    memcpy(PACK_TIME_WRITE_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_WRITE_SIZE)

#define PACK_SET_TIME_RECV(pack) \
    memcpy(PACK_TIME_RECV_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_RECV_SIZE)

#define PACK_SET_TIME_ACK_WRITE(pack) \
    memcpy(PACK_TIME_ACK_WRITE_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_ACK_WRITE_SIZE)

#define PACK_SET_TIME_ACK_RECV(pack) \
    memcpy(PACK_TIME_ACK_RECV_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_ACK_RECV_SIZE)

#define PACK_SET_TIME_FIN_WRITE(pack) \
    memcpy(PACK_TIME_FIN_WRITE_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_FIN_WRITE_SIZE)

#define PACK_SET_TIME_FIN_RECV(pack) \
    memcpy(PACK_TIME_FIN_RECV_POS(pack),TIMEVAL_BUF_GENERATE(),PACK_TIME_FIN_RECV_SIZE)

#define PACK_SET_TIME_GLOBAL_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_GLOBAL_POS(pack), PACK_TIME_GLOBAL_POS(pack1) ,PACK_TIME_GLOBAL_SIZE)

#define PACK_SET_TIME_WRITE_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_WRITE_POS(pack),PACK_TIME_WRITE_POS(pack1),PACK_TIME_WRITE_SIZE)

#define PACK_SET_TIME_RECV_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_RECV_POS(pack),PACK_TIME_RECV_POS(pack1),PACK_TIME_RECV_SIZE)

#define PACK_SET_TIME_ACK_WRITE_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_ACK_WRITE_POS(pack),PACK_TIME_ACK_WRITE_POS(pack1),PACK_TIME_ACK_WRITE_SIZE)

#define PACK_SET_TIME_ACK_RECV_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_ACK_RECV_POS(pack),PACK_TIME_ACK_RECV_POS(pack1),PACK_TIME_ACK_RECV_SIZE)

#define PACK_SET_TIME_FIN_WRITE_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_FIN_WRITE_POS(pack),PACK_TIME_FIN_WRITE_POS(pack1),PACK_TIME_FIN_WRITE_SIZE)

#define PACK_SET_TIME_FIN_RECV_BY_PACK(pack,pack1) \
    memcpy(PACK_TIME_FIN_RECV_POS(pack),PACK_TIME_FIN_RECV_POS(pack1),PACK_TIME_FIN_RECV_SIZE)

#define TIMEVAL_BUF_2_TIMEVAL(buf,pTv) \
    do{ \
        char tvSecStr[11] = {0},tvUsecStr[7] = {0};\
        char *stopAt = NULL;\
        memcpy(tvSecStr, (buf) ,10);\
        memcpy(tvUsecStr,(buf) + 10,6);\
        (pTv)->tv_sec = strtol(tvSecStr,&stopAt,10);\
        (pTv)->tv_usec = strtol(tvUsecStr,&stopAt,10);\
    }while(0)

#define PACK_NO_2_TIMEVAL(pack,_tv) TIMEVAL_BUF_2_TIMEVAL(PACK_NO_POS(pack),(_tv))

typedef enum{
    PT_NORMAL,
    PT_ACK,
    PT_FIN,
    PT_SHUT_DOWN,
    PT_SHUT_DOWN_ACK
}pack_type_e;

typedef struct{
    char t[16];
    char num[10];
    char pid[6];
}pack_no_t;

typedef struct{
    char gTime[PACK_TIME_GLOBAL_SIZE];
    char wTime[PACK_TIME_WRITE_SIZE];
    char rTime[PACK_TIME_RECV_SIZE];
    char ackWtime[PACK_TIME_ACK_WRITE_SIZE];
    char ackRtime[PACK_TIME_ACK_RECV_SIZE];
    char finWtime[PACK_TIME_FIN_WRITE_SIZE];
    char finRtime[PACK_TIME_FIN_RECV_SIZE];
}pack_time_t;

typedef struct {
    char size[4];
    char type[4];
    pack_no_t no;
    pack_time_t time;
    char data[PACK_DATA_SIZE_MAX];
}pack_t;

int pack_free(pack_t **pack);
int pack_create_from_data(pack_t **pack,void *data,size_t dataSize);
int pack_create_from_buf(pack_t **pack,void *buf,size_t size);
int pack_print(pack_t *pack);

#define PACK_GET_SIZE(_buf_) \
    ({\
        char packSizeStr[PACK_SIZE_SIZE + 1] = {0}; \
        char *stopAt = NULL;\
        memcpy(packSizeStr,((char *)(_buf_)),PACK_SIZE_SIZE);\
        unsigned int packSize = strtol(packSizeStr,&stopAt,10);\
        (packSize);\
     })

#define PACK_GET_TYPE(_buf_) \
    ({\
        char _str[PACK_TYPE_SIZE + 1] = {0}; \
        char *stopAt = NULL;\
        memcpy(_str,((char *)(_buf_)) + PACK_SIZE_SIZE ,PACK_TYPE_SIZE);\
        unsigned int _val = strtol(_str,&stopAt,10);\
        (_val);\
     })

#define ENABLE_BUF_2_PACK(buf,size) \
    ( PACK_SIZE_MIN <= (size) && PACK_GET_SIZE((buf)) <= (size) )
#endif


/*=============PACK_MANAGER================*/

#define PACK_G2W_TIMEOUT_MAX 1.0
#define PACK_W2R_TIMEOUT_MAX 1
#define PACK_R2AW_TIMEOUT_MAX 1
#define PACK_AW2AR_TIMEOUT_MAX 1
#define PACK_AR2FW_TIMEOUT_MAX 1
#define PACK_FW2FR_TIMEOUT_MAX 1

typedef int (*cb_recved_data)(void *client,char *data,unsigned int size,char **rspData,unsigned int *rspDataSize);

struct pack_manager_s{
    mc_list_t *rPacks;
    mc_list_t *wPacks;
    mc_list_t *ePacks;
    unsigned int g2wTimeOutCount;
    unsigned int w2rTimeOutCount;
    unsigned int r2awTimeOutCount;
    unsigned int aw2arTimeOutCount;
    unsigned int ar2fwTimeOutCount;
    unsigned int fw2frTimeOutCount;
};
int pack_manager_init(pack_manager_t **pm);
int pack_manager_free(pack_manager_t **pp);

int pack_manager_tcp_server_client_recv(tcp_server_client_t *,cb_recved_data);
int pack_manager_tcp_client_recv(tcp_client_t *client,cb_recved_data);
int pack_manager_tcp_server_client_send(tcp_server_client_t *client,char *data,unsigned int dataSize);
int pack_manager_tcp_client_send(tcp_client_t *client,char *data,unsigned int dataSize);

int pack_manager_modify_read_time(char *buf,unsigned int bufSize,unsigned int *doneSize);

int pack_manager_modify_write_time(char *buf,unsigned int maxWriteSize,unsigned int *needWriteSize);

int pack_manager_print(pack_manager_t *pm);
int tcp_server_client_shut_down(tcp_server_client_t *client);
int tcp_client_shut_down(tcp_client_t *client);
