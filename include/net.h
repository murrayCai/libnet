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

typedef enum{
    TCCT_NONE,
    TCCT_TCP,
    TCCT_HTTP,
    TCCT_WEB_SOCKET,
    TCCT_MAX
}tcp_client_connect_type_e;

#define TCP_CLIENT_DEFINE \
    int fd;\
    struct sockaddr_in addr;\
    char *readBuf;\
    unsigned int readBufSize;\
    unsigned int readBufUsed;\
    char *writeBuf;\
    unsigned int writeBufSize;\
    unsigned int writeBufUsed;\
    tcp_client_connect_type_e connectType;\
    unsigned int isWebSocketInited;\
    void *userData


typedef struct {
    TCP_CLIENT_DEFINE;
}tcp_client_define_t;

typedef enum{
    PMCT_NONE,
    PMCT_CLIENT,
    PMCT_SERVER_CLIENT,
    PMCT_MAX
}client_type_e;
#define CLIENT_TYPE_CHECK(t) ( PMCT_NONE >= (t) || PMCT_MAX <= (t) )

/*=========== TCP_SERVER ================*/
typedef struct tcp_server_client_s tcp_server_client_t;
typedef int (*cb_tcp_server_client_recved)(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_server_client_before_write)(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_server_client_closed)(tcp_server_client_t *client);
typedef int (*cb_tcp_server_client_connected)(tcp_server_client_t *client);
typedef struct {
    const char *ip;
    unsigned int port;
    unsigned int backLog;
    unsigned int clientMax;
    unsigned int readPackMax;
    unsigned int writePackMax;
    unsigned int kqTimeout;
    cb_tcp_server_client_recved onClientRecved;
    cb_tcp_server_client_closed onClientClosed;
    cb_tcp_server_client_before_write onClientBeforeWrite;
    cb_tcp_server_client_connected onClientConnected;
}tcp_server_config_t;

typedef struct kevent ev_t;
typedef struct{
    tcp_server_config_t *config;
    struct sockaddr_in addr;
    int kq;
    int fd;
    arr_t *clients;
    ev_t *events;
    time_t netStatStart;
    unsigned int wCount;
    unsigned int rCount;
    double loopTimeMax;
}tcp_server_t;


struct tcp_server_client_s{
    TCP_CLIENT_DEFINE;
    tcp_server_t *server;
    unsigned int index;
    unsigned int enableReadSize;
    unsigned int enableWriteSize;
    struct kevent evRead;
    struct kevent evWrite;
};

typedef int (*cb_tcp_server_run_error)(struct kevent *ev);

int tcp_server_init(tcp_server_t **ts,tcp_server_config_t *config);
int tcp_server_client_close(tcp_server_client_t **pp);
int tcp_server_exit(tcp_server_t **ts);
int tcp_server_run(tcp_server_t *ts,cb_tcp_server_run_error callBack);
int tcp_server_client_write(tcp_server_client_t *client,const char *buf,size_t bufSize);

#define IS_VALID_FD(fd) (-1 != fcntl( (fd),F_GETFD ) )

/*==============TCP_CLIENT=====================*/
typedef struct tcp_client_s tcp_client_t;
typedef int (*cb_tcp_client_connected)(tcp_client_t *client);
typedef int (*cb_tcp_client_closed)(tcp_client_t *client);
typedef int (*cb_tcp_client_recved)(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_client_before_write)(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used);
typedef struct{
    void *userData;
    unsigned int readBufSize;
    unsigned int writeBufSize;
    cb_tcp_client_connected onConnected;
    cb_tcp_client_closed onClosed;
    cb_tcp_client_recved onRecved;
    cb_tcp_client_before_write onBeforeWrite;
}tcp_client_config_t;

struct tcp_client_s{
    TCP_CLIENT_DEFINE;
    tcp_client_config_t *config;
};

int tcp_client_init(tcp_client_t **pp,tcp_client_config_t *config);
int tcp_client_close(tcp_client_t **pp);
int tcp_client_connect(tcp_client_t *client,const char *ip,unsigned int port);
int tcp_client_recv(tcp_client_t *client);
int tcp_client_send(tcp_client_t *client,const char *buf,size_t size);

/*================PACK=====================*/
typedef enum{
    PT_NONE,
    PT_NORMAL,
    PT_ACK,
    PT_FIN,
    PT_FIN_ACK,
    PT_SHUT_DOWN,
    PT_SHUT_DOWN_ACK,
    PT_MAX
}pack_type_e;
#define PACK_TYPE_CHECK(t) ( PT_NONE >= (t) || PT_MAX <= (t) )

typedef struct{
    char t[16];
    char num[10];
    char pid[6];
}pack_no_t;
#define PACK_SIZE_NO sizeof(pack_no_t) 

#define PACK_SIZE_SIZE 4
#define PACK_SIZE_TYPE 4
#define PACK_SIZE_DATA_MAX 1024
typedef struct {
    char size[PACK_SIZE_SIZE];
    char type[PACK_SIZE_TYPE];
    pack_no_t no;
    char data[PACK_SIZE_DATA_MAX];
}pack_t;
#define PACK_SIZE_MIN (PACK_SIZE_SIZE + PACK_SIZE_TYPE + PACK_SIZE_NO )
#define PACK_SIZE_MAX (PACK_SIZE_MIN + PACK_SIZE_DATA_MAX)

#define PACK_POS_TYPE(pack) ( (char *)(pack) + PACK_SIZE_SIZE )
#define PACK_POS_NO(pack) ( PACK_POS_TYPE(pack) + PACK_SIZE_TYPE )
#define PACK_POS_DATA(pack) ( PACK_POS_NO(pack) + PACK_SET_NO )

#define PACK_SET_NO(pack,_buf_) \
    memcpy( PACK_POS_NO(pack), ((char *)(_buf_)),PACK_SIZE_NO)

#define PACK_SET_TYPE(pack,type)\
    do{\
        char _bufType_[PACK_SIZE_TYPE + 1] = {0};\
        char fmt[32] = {0};\
        snprintf(fmt,32,"%%0%dd",PACK_SIZE_TYPE);\
        snprintf(_bufType_,PACK_SIZE_TYPE + 1,fmt,(type) );\
        memcpy( ((char *)(pack)) + PACK_SIZE_SIZE , _bufType_ ,PACK_SIZE_TYPE);\
    }while(0)

#define PACK_SET_SIZE(pack,size) \
    do{\
        char _bufSize_[PACK_SIZE_SIZE + 1] = {0};\
        char fmtSize[32] = {0};\
        snprintf(fmtSize,32,"%%0%dd",PACK_SIZE_SIZE);\
        snprintf(_bufSize_,PACK_SIZE_SIZE + 1,fmtSize,(size));\
        memcpy( ((char *)(pack)), _bufSize_,PACK_SIZE_SIZE);\
    }while(0)

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
        char _str[PACK_SIZE_TYPE + 1] = {0}; \
        char *stopAt = NULL;\
        memcpy(_str,((char *)(_buf_)) + PACK_SIZE_SIZE ,PACK_SIZE_TYPE);\
        unsigned int _val = strtol(_str,&stopAt,10);\
        (_val);\
     })

#define ENABLE_BUF_2_PACK(buf,size) \
    ( PACK_SIZE_MIN <= (size) && PACK_GET_SIZE((buf)) <= (size) )

#define PACK_CHECK(pack) ( NULL == (pack) || PACK_SIZE_MIN > PACK_GET_SIZE(pack) || PACK_TYPE_CHECK(PACK_GET_TYPE(pack)) )

int pack_create_from_data(pack_t **pack,const char *data,size_t dataSize);
int pack_create_from_buf(pack_t **pack,const char *buf,size_t size);
int pack_print(pack_t *pack);
int pack_add_data(pack_t *pack,const char *data,unsigned int size);

/*=============PACK_MANAGER================*/

typedef int (*cb_pm_pack_fin_ack_recved)(void *client,pack_t *pack);
typedef int (*cb_pm_pack_recved)(void *client,pack_t *rPack,pack_t *ackPack);
typedef int (*cb_pm_pack_before_write)(void *client,pack_t *pack);

typedef struct {
    cb_pm_pack_recved onNormalPackRecved;
    cb_pm_pack_recved onAckPackRecved;
    cb_pm_pack_recved onFinPackRecved;
    cb_pm_pack_fin_ack_recved onFinAckPackRecved; 

    cb_pm_pack_before_write onNormalPackBeforeWrite;
    cb_pm_pack_before_write onAckPackBeforeWrite;
    cb_pm_pack_before_write onFinPackBeforeWrite;
    cb_pm_pack_before_write onFinAckPackBeforeWrite;
    cb_pm_pack_before_write onShutDownPackBeforeWrite;
    cb_pm_pack_before_write onShutDownAckPackBeforeWrite;
}pm_config_t;

typedef enum{
    PMSDS_NONE,
    PMSDS_REQ, // at this status, could not write anything
    PMSDS_RSP, // at this status, could not write anything, should wait be closed
    PMSDS_OK, // at this status, should close this client
    PMSDS_MAX
}pm_shut_down_statu_e;

typedef struct {
    pm_config_t *config;
    mc_list_t *rPacks;
    mc_list_t *wPacks;
    pm_shut_down_statu_e status;
}pm_t;
int pm_init(pm_t **pp,pm_config_t *config);
int pm_free(pm_t **pp);

int pm_on_server_recv(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used);
int pm_on_client_recv(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used);

int pm_on_server_before_write(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used);
int pm_on_client_before_write(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used);

int pm_server_client_send(tcp_server_client_t *client,const char *buf,unsigned int size);
int pm_client_send(tcp_client_t *client,const char *buf,unsigned int size);

int pm_server_client_shut_down(tcp_server_client_t *client);
int pm_client_shut_down(tcp_client_t *client);

int pm_print(pm_t *pm);


/*=================== timer ==========================*/
typedef enum{
    T_NONE,
    T_G,
    T_W,
    T_R,
    T_A,
    T_AR,
    T_F,
    T_FR,
    T_MAX
}stimer_type_e;
#define STIMER_TYPE_CHECK(t) ( T_NONE >= (t) || T_MAX <= (t))
#define STIMER_TAG_SUFFIX "timer_tt"
#define STIMER_TAG_SUFFIX_SIZE 8
typedef struct{
    char g[16];
    char w[16];
    char r[16];
    char ack[16];
    char ackR[16];
    char fin[16];
    char finR[16];
    char status;
    char tagSuffix[STIMER_TAG_SUFFIX_SIZE];
}stimer_t;
#define STIMER_POS_TAG_SUFFIX(t) ( (char *)(t) + 16 * 7 + 1 )
#define STIMER_POS_FROM_BUF(buf,size) ( ((const char *)(buf)) + (size) - sizeof(stimer_t) )
#define STIMER_CHECK(st) ( NULL == (st) || 0 != memcmp( STIMER_POS_TAG_SUFFIX(st) , STIMER_TAG_SUFFIX, STIMER_TAG_SUFFIX_SIZE ) )

#define STIMER_EXISTS(buf,size) ( NULL != (buf) && sizeof(stimer_t) <= (size) && 0 == memcmp( ((const char *)(buf)) + (size) - STIMER_TAG_SUFFIX_SIZE , STIMER_TAG_SUFFIX, STIMER_TAG_SUFFIX_SIZE ) )


int stimer_init(stimer_t **t);

int stimer_free(stimer_t **t);

int stimer_set_now(stimer_t *t,stimer_type_e type);

int stimer_set_by_buf(stimer_t *t,stimer_type_e type,const char *buf);

int stimer_get(stimer_t *t,stimer_type_e type,struct timeval *dist);

int stimer_diff(stimer_t *t,stimer_type_e from,stimer_type_e to,double *dist);

#define STIMER_DIFF_G2W(t,dist) timer_diff((t),T_G,T_W,(dist))
#define STIMER_DIFF_W2R(t,dist) timer_diff((t),T_W,T_R,(dist))
#define STIMER_DIFF_R2A(t,dist) timer_diff((t),T_R,T_A,(dist))
#define STIMER_DIFF_A2AR(t,dist) timer_diff((t),T_A,T_AR,(dist))
#define STIMER_DIFF_AR2F(t,dist) timer_diff((t),T_AR,T_F,(dist))
#define STIMER_DIFF_F2FR(t,dist) timer_diff((t),T_F,T_FR,(dist))

int stimer_set_run_status(stimer_t *t);

int stimer_get_from_buf(const char *buf,unsigned int size,stimer_t **dist);
int stimer_set_pack_from_pack(pack_t *old,pack_t *new);
int stimer_set_pack(pack_t *pack);

int stimer_print(stimer_t *t);
int stimer_print_pack(pack_t *pack);
int stimer_send(void *c,client_type_e t, const char *data,unsigned int size);
#define STIMER_SERVER_SEND(c,d,s) (stimer_send((c),PMCT_SERVER_CLIENT,(d),(s)))
#define STIMER_CLIENT_SEND(c,d,s) (stimer_send((c),PMCT_CLIENT,(d),(s)))

/*============ HTTP ==================*/
#define HRT_PAGE_SUFFIX ".html"
#define HRT_CSS_SUFFIX ".css"
#define HRT_JS_SUFFIX ".js"
typedef enum{
    HRT_NONE,
    HRT_PAGE,
    HRT_JS,
    HRT_CSS,
    HRT_MAX
}http_resource_type_e;

typedef struct{
    const char *buf;
    unsigned int size;
    const char *url;
    unsigned int page;
    const char *tag;
    const char *header;
    const char *cookies;
    const char *webSocketKey;
    unsigned int webSocketKeySize;
    http_resource_type_e type;
}http_req_t;

int http_req_parse(const char *buf, unsigned int size, http_req_t *http);
int http_req_print(http_req_t *http);
#define IS_HTTP_REQ(b,s,h) \
    ({\
        int r = 0;\
        if( 0 == http_req_parse( (b),(s),(h) )){\
            r = 1;\
        }else{\
            MC_ERR_CLEAR();\
        }\
        (r);\
     })


typedef struct{
    unsigned int pageInitCount;
    unsigned int jsInitCount;
    unsigned int cssInitCount;
}http_resource_config_t;

typedef struct{
    const char *data;
    unsigned int dataSize;
    unsigned int headerSize;
    time_t expire;
}http_resource_item_t;

typedef struct{
    arr_t *page;
    arr_t *js;
    arr_t *css;
}http_resource_t;
#define HTTP_RESOURCE_TYPE_CHECK(t) ( HRT_NONE >= (t) && HRT_MAX <= (t) )

int http_resource_init(http_resource_t **resource,http_resource_config_t *config);
int http_resource_free(http_resource_t **resource);
int http_resource_add(http_resource_t *resource,http_resource_type_e type,unsigned int index,const char *data,unsigned int dataSize,unsigned int headerSize);

int http_resource_get(http_resource_t *resource,http_resource_type_e type,unsigned int index,http_resource_item_t **dist);
int http_resource_print(http_resource_t *resource);
#define HTTP_RESOURCE_GET_PAGE_404(r,d) \
    http_resource_get( (r), HRT_PAGE, 404,(d) )
#define HTTP_RESOURCE_GET_PAGE_OR_404(r,i,d) \
    ({\
        int _r = 0;\
        if(http_resource_get( (r),HRT_PAGE,(i),(d) )){\
            MC_ERR_CLEAR();\
            _r = HTTP_RESOURCE_GET_PAGE_404( (r),(d) );\
        }\
        (_r);\
    })

#define HTTP_RESOURCE_GET_CSS(r,i,d) \
    http_resource_get( (r),HRT_CSS,(i),(d) )

#define HTTP_RESOURCE_GET_JS(r,i,d) \
    http_resource_get( (r),HRT_JS,(i),(d) )

int http_resource_load_from_dir(http_resource_t *resource,http_resource_type_e type, const char *dirPath);


int http_rsp_generate(http_resource_type_e type,const char *data,unsigned int dataSize,char **dist,unsigned int *distSize);

int http_rsp(void *client,client_type_e ct,http_req_t *req,http_resource_t *resources);
#define HTTP_SERVER_RSP(client,req,res) \
    http_rsp( (client),PMCT_SERVER_CLIENT,(req),(res) )
#define HTTP_CLIENT_RSP(client,req,res) \
    http_rsp( (client),PMCT_CLIENT,(req),(res) )

/*================SHA1========================*/
#include "stdint.h"

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
    );

void SHA1Init(
    SHA1_CTX * context
    );

void SHA1Update(
    SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

void SHA1Final(
    unsigned char digest[20],
    SHA1_CTX * context
    );

void SHA1(
    char *hash_out,
    const char *str,
    int len);
/*==================BASE64==========================*/
int base64_encode(const char *str, unsigned int strSize,char **dist,unsigned int *distSize);
int base64_decode(const char *code, unsigned int codeSize, char **dist,unsigned int *distSize);

/*====================WEBSOCKET=====================*/

int ws_server_send(tcp_server_client_t *client,const char *data,unsigned int dataSize);

int ws_server_hand_shake(tcp_server_client_t *client,const char *key,unsigned int keySize);

typedef int (*cb_ws_msg_recved)(tcp_server_client_t *client,const char *msg,unsigned int msgSize);

int ws_server_parse(tcp_server_client_t *client,
        const char *data,unsigned int dataSize,
        unsigned int *used,unsigned int *isFin,
        cb_ws_msg_recved onWsMsgRecved);
#endif




