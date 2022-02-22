#ifndef __LIB_NET_H__
#define __LIB_NET_H__
#include "mc.h"
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/err.h>


#define TCP_SERVER_CLIENT_BUF_LV 1
#define PACK_SIZE_SIZE 8
#define PACK_SIZE_TYPE 4
#define PACK_SIZE_DATA_MAX 1024
#define TCP_CLIENT_WRITE_BUF_BUSY_SIZE 10
#define TCP_WRITE_BUSY_SECOND_PER 3
#define CLIENT_HOST_SIZE 64
#define TCP_SERVER_KQ_MAX_MS_LOOP_PER 100

typedef enum{
    TCCT_NONE,
    TCCT_TCP,
    TCCT_HTTP,
    TCCT_WEB_SOCKET,
    TCCT_MAX
}tcp_client_connect_type_e;

typedef enum{
    TWBT_NONE,
    TWBT_MEM,
    TWBT_FILE,
    TWBT_MAX
}tcp_write_buffer_type_e;

typedef enum{
    TSKCT_SERVER_CLIENT,
    TSKCT_CLIENT,
    TSKCT_VNODE
}tcp_server_kq_client_type_e;

typedef struct{
    tcp_server_kq_client_type_e type;
    void *data;
}tcp_server_kq_client_t;

typedef struct http_req_s http_req_t;
typedef struct http_rsp_s http_rsp_t;
typedef struct tcp_client_define_s tcp_client_define_t;
typedef struct tcp_server_client_s tcp_server_client_t;
typedef struct tcp_write_buffer_s tcp_write_buffer_t;
typedef int (*cb_tcp_write_buf_completed)(tcp_write_buffer_t *wBuf);
int on_send_file_completed_default(tcp_write_buffer_t *wBuf);
int tcp_free_write_buf_default(tcp_write_buffer_t *wBuf);
int tcp_write_buffer_free(tcp_write_buffer_t **pp);

typedef enum{
    PMCT_NONE,
    PMCT_CLIENT,
    PMCT_SERVER_CLIENT,
    PMCT_MAX
}client_type_e;


#define TCP_CLIENT_DEFINE \
    client_type_e type;\
    tcp_client_connect_type_e connectType;\
    int fd;\
    kq_t *kq;\
    kq_client_t kqClient;\
    struct sockaddr_in addr;\
    char host[CLIENT_HOST_SIZE];\
    unsigned int port;\
    char *readBuf;\
    unsigned int isWaitClose;\
    unsigned int socketRecvBufSize;\
    unsigned int socketSendBufSize;\
    unsigned int readBufSize;\
    unsigned int readBufUsed;\
    unsigned int enableReadSize;\
    unsigned int enableWriteSize;\
    mc_list_t *writeBufs;\
    unsigned int isWebSocketInited;\
    unsigned int isNeedClose;\
    unsigned int sendPackCount;\
    unsigned int sendTotalSize;\
    unsigned int recvTotalSize;\
    time_t lastStaticReadBufSize;\
    time_t lastNotifyWriteBusy;\
    SSL *ssl;\
    unsigned int isSslHandShaked;\
    unsigned int sslEnableReadSize;\
    unsigned int connectStatus;\
    http_req_t *httpReq;\
    http_rsp_t *httpRsp;\
    void *userData

struct tcp_client_define_s{
    TCP_CLIENT_DEFINE;
};
int tcp_client_statu_check(tcp_client_define_t *client);

#define TCP_CLIENT_SET_CLOSE(_c_) \
    ({\
        (_c_)->isNeedClose = 1;\
        LOGI("set close[%d] %s:%d\n",(_c_)->fd,(_c_)->host,(_c_)->port);\
        (KQ_CLIENT_SOCKET_ADD_WRITE_ONCE(&(_c_)->kqClient));\
    })

int tcp_get_write_buf_size(tcp_client_define_t *client,unsigned int *size);
int tcp_get_read_buf_size(tcp_client_define_t *client,unsigned int *size);
int tcp_get_enable_write_size(tcp_client_define_t *,unsigned int *size);
int tcp_get_enable_read_size(tcp_client_define_t *,unsigned int *size);

struct tcp_write_buffer_s{
    tcp_write_buffer_type_e type;
    char *data;
    FILE *f;
    unsigned int size;
    unsigned int writedSize;
    unsigned int isNeedFree;
    unsigned int isNeedTimeStamp;
    tcp_client_define_t *client;
    cb_tcp_write_buf_completed onCompleted;
    MC_LIST_ENTRY(tcp_write_buffer_t);
};
#define R_VALID_TWB(wb) \
    do{ \
        R(NULL == (wb)); \
        if( (wb)->type == TWBT_MEM ){ \
            R(NULL == (wb)->data); \
            R(NULL != (wb)->f); \
        }else if( TWBT_FILE == (wb)->type ){ \
            R(NULL == (wb)->f); \
            R(NULL != (wb)->data); \
        } \
        R(0 >= (wb)->size); \
        R((wb)->size < (wb)->writedSize ); \
    }while(0)

int tcp_read(tcp_client_define_t *client,char *buf,unsigned int needReadSize,unsigned int *readedSize);
#define TCP_READ(_c_,_b_,_s_,_r_)\
    tcp_read((tcp_client_define_t *)(_c_),(_b_),(_s_),(_r_))

int tcp_send_buf(tcp_client_define_t *client,char *buf,unsigned int size,unsigned int isNeedFree,unsigned int isNeedTimeStamp);
int tcp_send_file(tcp_client_define_t *client,const char *path,cb_tcp_write_buf_completed onCompleted);

#define CLIENT_TYPE_CHECK(t) ( PMCT_NONE >= (t) || PMCT_MAX <= (t) )

/*=========== TCP_SERVER ================*/
typedef int (*cb_tcp_server_client_recved)(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_server_client_before_write)(tcp_client_define_t *client,char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_server_client_writed)(tcp_server_client_t *client,const char *buf);
typedef int (*cb_tcp_server_client_closed)(tcp_server_client_t *client);
typedef int (*cb_tcp_server_client_connected)(tcp_server_client_t *client);
typedef int (*cb_tcp_server_client_pack_too_large)(tcp_server_client_t *client,unsigned int size,unsigned int maxSize);
typedef int (*cb_tcp_server_client_type_inited)(tcp_server_client_t *client);
typedef int (*cb_tcp_server_idle)();
typedef int (*cb_tcp_server_exited)();
typedef int (*cb_tcp_server_started)();
typedef int (*cb_tcp_server_client_write_busy)(tcp_server_client_t *);
typedef int (*cb_tcp_server_client_write_idle)(tcp_server_client_t *);
typedef int (*cb_kq_loop_busy)(unsigned int millisecond);
typedef struct {
    const char *ip;
    unsigned int port;
    unsigned int backLog;
    unsigned int clientMax;
    unsigned int readPackMax;
    unsigned int writePackMax;
    unsigned int kqTimeout;
    unsigned int isUseSsl;
    const char *sslKeyPath;
    const char *sslServerPath;
    const char *sslChainPath;
    unsigned int isInitedStartAccept;
    cb_kq_re_inited onKqReInited;
    cb_kq_busy onKqLoopBusy;
    cb_kq_error onKqError;
    cb_tcp_server_idle onIdle;
    cb_tcp_server_client_recved onClientRecved;
    cb_tcp_server_client_closed onClientClosed;
    cb_tcp_server_client_before_write onBeforeWrite;
    cb_tcp_server_client_connected onClientConnected;
    cb_tcp_server_client_writed onClientWrited;
    cb_tcp_server_client_pack_too_large onClientPackTooLarge;
    cb_tcp_server_client_type_inited onClientTypeInited;
    cb_tcp_server_exited onServerExited;
    cb_tcp_server_started onServerStarted;
    cb_tcp_server_client_write_busy onWriteBusy;
    cb_tcp_server_client_write_idle onWriteIdle;
}tcp_server_config_t;

typedef struct kevent ev_t;
typedef struct{
    tcp_server_config_t *config;
    struct sockaddr_in addr;
    kq_t *kq;
    kq_client_t kqClient;
    int fd;
    mc_list_t *clients;
    ev_t *events;
    time_t netStatStart;
    unsigned int wCount;
    unsigned int rCount;
    unsigned int isNeedReAccept;
    unsigned int isNeedStart;
    double loopTimeMax;
    int rv;
    SSL_CTX *sslCtx;
    const SSL_METHOD *sslMethod;
}tcp_server_t;

typedef enum{
    WSHP_NONE = 0,
    WSHP_PRFIX_OK = 1,
    WSHP_PAYLOAD_OK = 2,
    WSHP_MASK_OK = 4,
    WSHP_CLOSE = 5
}ws_header_parse_statu_e;

typedef struct {
    ws_header_parse_statu_e statu;
    char buf[32];
    unsigned int bufSize;
    unsigned int dataSize;
    unsigned int isFin;
    unsigned int isUseMask;
    unsigned int payloadSizeTag;
    unsigned int hadReadDataSize;
    char mask[4];
    unsigned int headerSize;
}ws_header_t;

struct tcp_server_client_s{
    TCP_CLIENT_DEFINE;
    tcp_server_t *server;
    unsigned int needIgnoreDataSize;
    ws_header_t wsHeader;
    MC_LIST_ENTRY(tcp_server_client_t);
};

int tcp_signal_init(sig_t onSignal);
int tcp_server_init(tcp_server_t **ts,tcp_server_config_t *config);
int tcp_server_start_accept(tcp_server_t *ts);
int tcp_server_stop_accept(tcp_server_t *ts);
int tcp_server_client_close(tcp_server_client_t **pp);
int tcp_server_exit(tcp_server_t **ts);
int tcp_server_run(tcp_server_t *ts);
int tcp_server_client_send_wbuf(tcp_server_client_t *client,tcp_write_buffer_t *wBuf);
int tcp_server_client_recv(tcp_server_client_t *client);
int on_tcp_server_client_pack_size_too_large(tcp_server_client_t *,unsigned int,unsigned int);

int tcp_server_ssl_init(tcp_server_t *);
int tcp_server_client_ssl_hand_shake(tcp_server_client_t *client);
int tcp_ssl_recv(tcp_client_define_t *client,char *buf,unsigned int needReadSize,unsigned int *readedSize);
int tcp_ssl_send(tcp_client_define_t *client,const char *buf,unsigned int bufSize,unsigned int *sendedSize);
int tcp_ssl_send_file(tcp_client_define_t *client,int fd,off_t offset,size_t size,unsigned int *sendedSize);
int tcp_do_message(tcp_client_define_t *);

#define TSC_READER_BUFFER_SIZE(client) ((client)->server->config->readPackMax * TCP_SERVER_CLIENT_BUF_LV)

/*==============TCP_CLIENT=====================*/
typedef struct tcp_client_s tcp_client_t;
typedef int (*cb_tcp_client_connected)(tcp_client_t *client);
typedef int (*cb_tcp_client_connect_error)(tcp_client_t *client);
typedef int (*cb_tcp_client_closed)(tcp_client_t *client);
typedef int (*cb_tcp_client_recved)(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_client_before_write)(tcp_client_t *client,char *buf,unsigned int size,unsigned int *used);
typedef int (*cb_tcp_client_writed)(tcp_client_t *client,const char *buf);
typedef int (*cb_tcp_client_write_busy)(tcp_client_t *);
typedef int (*cb_tcp_client_write_idle)(tcp_client_t *);
typedef struct{
    void *userData;
    unsigned int readBufSize;
    unsigned int writeBufSize;
    struct timeval recvTimeout;
    struct timeval sendTimeout;
    int isNonBlock;
    cb_tcp_client_connected onConnected;
    cb_tcp_client_connect_error onConnectError;
    cb_tcp_client_closed onClosed;
    cb_tcp_client_recved onRecved;
    cb_tcp_client_before_write onBeforeWrite;
    cb_tcp_client_writed onWrited;
    cb_tcp_client_write_busy onWriteBusy;
    cb_tcp_client_write_idle onWriteIdle;
}tcp_client_config_t;

struct tcp_client_s{
    TCP_CLIENT_DEFINE;
    tcp_client_config_t *config;
};

int tcp_client_init(tcp_client_t **pp,tcp_client_config_t *config,const char *ip,unsigned int port,kq_t *kq);
int tcp_client_close(tcp_client_t **pp);
int tcp_client_connect(tcp_client_t *client);
int tcp_client_recv(tcp_client_t *client);
int tcp_client_send_wbuf(tcp_client_t *client,tcp_write_buffer_t *buf);
#define TCP_CLIENT_SEND_BUF(c,b,s,f,_needTime_) \
    tcp_send_buf((tcp_client_define_t *)(c),(b),(s),(f),(_needTime_))

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
    PMSDS_REQ, /* at this status, could not write anything*/
    PMSDS_RSP, /* at this status, could not write anything, should wait be closed */
    PMSDS_OK, /* at this status, should close this client */
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
#define HTTP_BUF_SIZE_MIN 5

typedef enum{
    HMT_NONE,
    HMT_GET,
    HMT_POST,
    HMT_HEAD,
    HMT_PUT,
    HMT_DELETE,
    HMT_OPTIONS,
    HMT_TRACE,
    HMT_NOT_HTTP,
    HMT_WANT_READ,
    HMT_MAX
}http_method_type_e;
#define HTTP_METHOD_VALID(_m_) ((_m_) >= HMT_MAX || (_m_) <= HMT_NONE)
#define HTTP_METHOD_SIZE(_m_)\
    ({\
     int _size_ = 0;\
     switch(_m_){\
        case HMT_GET:\
        case HMT_PUT:\
        _size_ = 4;\
        break;\
        case HMT_POST:\
        case HMT_HEAD:\
        _size_ = 5;\
        break;\
        case HMT_TRACE:\
        _size_ = 6;\
        break;\
        case HMT_DELETE:\
        _size_ = 7;\
        break;\
        case HMT_OPTIONS:\
        _size_ = 8;\
        break;\
        default : break;\
     }\
     (_size_);\
     })

typedef enum{
    HRT_NONE,
    HRT_PAGE,
    HRT_JS,
    HRT_CSS,
    HRT_MAX
}http_resource_type_e;

#define HTTP_BODY_SIZE_STR_BYTES_MAX 5 
typedef enum{
    HRPPE_NONE,
    HRPPE_METHOD_DONE,
    HRPPE_FIRST_LINE_DONE,
    HRPPE_HEADER_DONE,
    HRPPE_PARSE_DONE,
    HRPPE_MAX
}http_req_parse_process_e;

struct http_req_s{
    http_req_parse_process_e process;
    unsigned int isHttp;
    unsigned int isWebSocket;
    unsigned int isRecvCompleted;
    mc_list_t *headerList;
    mc_list_t *cookieList;
    http_method_type_e method;
    buf_t path;
    buf_t version;
    buf_t webSocketKey;
    buf_t host;
    buf_t body;
    unsigned int bodySize;
    const char *buf;
    unsigned int size;
};


int http_req_new(http_req_t **);
int http_req_free(http_req_t **);
int http_request_parse(const char *buf, unsigned int , http_req_t *);
int http_request_print(http_req_t *http);

struct http_rsp_s{
    unsigned int code;
    buf_t buf;
    buf_t data;
    mc_list_t *headerList;
    unsigned int hadSendSize;
    unsigned int totalSize;
};
int http_rsp_new(http_rsp_t **pp);
int http_rsp_free(http_rsp_t **pp);
int http_rsp_by_code(tcp_server_client_t *client);
int http_rsp(tcp_server_client_t *client);

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
} MC_SHA1_CTX;

void MC_SHA1Transform(uint32_t state[5],const unsigned char buffer[64]);
void MC_SHA1Init(MC_SHA1_CTX * context);
void MC_SHA1Update(MC_SHA1_CTX *,const unsigned char *data,uint32_t len);
void MC_SHA1Final(unsigned char digest[20],MC_SHA1_CTX * context);
void MC_SHA1(char *hash_out,const char *str,int len);
/*==================BASE64==========================*/
int base64_encode(const char *str, unsigned int strSize,char **dist,unsigned int *distSize);
int base64_decode(const char *code, unsigned int codeSize, char **dist,unsigned int *distSize);

/*====================WEBSOCKET=====================*/

int ws_server_get_header_size(char *data,unsigned int size,unsigned int *headerSize);
int ws_server_data_parse(tcp_server_client_t *client,unsigned int size);
int ws_server_header_parse(tcp_server_client_t *client,unsigned int *isParsed);
int ws_server_send(tcp_server_client_t *client,char *data,unsigned int dataSize,int isBinary,int isNeedFree,int isNeedTimeStamp);

int ws_server_hand_shake(tcp_server_client_t *client,const char *key,unsigned int keySize);

typedef int (*cb_ws_msg_recved)(tcp_server_client_t *client,const char *msg,unsigned int msgSize,unsigned int *used);

int ws_server_parse(tcp_server_client_t *client,
        const char *data,unsigned int dataSize,
        unsigned int *used,unsigned int *isFin,
        cb_ws_msg_recved onWsMsgRecved);

/*=====================DEBUG=====================*/

int tcp_server_client_print(tcp_server_client_t*);
int tcp_write_buffer_print(tcp_write_buffer_t *);

typedef enum{
    TCCS_CLOSED = 0,
    TCCS_CONNECTING = 1,
    TCCS_CONNECTED = 2,
    TCCS_CONNECT_ERROR = 3
}tcp_client_connect_statu_e;

int tcp_client_connect_check(tcp_client_t *);

#endif
