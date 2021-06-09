#include "net.h"
#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_PACK_MANAGER
#endif

typedef enum{
    PMIT_NONE,
    PMIT_READ,
    PMIT_WRITE,
    PMIT_ERROR,
    PMIT_MAX
}pack_manager_item_type_e;

typedef enum{
    PMT_GLOABL,
    PMT_WRITE_TIME,
    PMT_READ_TIME,
    PMT_ACK_WRITE_TIME,
    PMT_ACK_READ_TIME,
    PMT_FIN_WRITE_TIME,
    PMT_FIN_READ_TIME
}pack_manager_time_e;

typedef struct{
    double g2w; // global time to write data time
    double w2r; // write data time to recved data time
    double r2aw; // recved data time to write ack data time
    double aw2ar; // write ack data time to recved ack data time
    double ar2fw; // recved ack data time to write fin data time
    double fw2fr; // write fin data time to recved fin data time
}pack_time_diff_t;

typedef struct{
    pack_t *pack;
    struct timeval gTime;
    struct timeval wTime;
    struct timeval rTime;
    struct timeval ackWtime;
    struct timeval ackRtime;
    struct timeval finWtime;
    struct timeval finRtime;
    pack_time_diff_t diff;
}pack_manager_item_t;

static pack_t ackPack = {0};
static int isInitAckPack = 0;

static pack_t finPack = {0};
static int isInitFinPack = 0;
int pack_ack_init(){
    int ret = 0;
    if(0 == isInitAckPack){
        memset(&ackPack,0,PACK_SIZE_MAX);
        PACK_SET_SIZE(&ackPack,PACK_SIZE_MIN);
        PACK_SET_TYPE(&ackPack,PT_ACK);
        isInitAckPack = 1;
    }
    return ret;
}

int pack_fin_init(){
    int ret = 0;
    if(0 == isInitFinPack){
        memset(&finPack,0,PACK_SIZE_MAX);
        PACK_SET_SIZE(&finPack,PACK_SIZE_MIN);
        PACK_SET_TYPE(&finPack,PT_FIN);
        isInitFinPack = 1;
    }
    return ret;
}

static pack_t shutDownPack = {0};
static int isInitShutDownPack = 0;
int pack_shut_down_init(){
    int ret = 0;
    if(0 == isInitShutDownPack){
        memset(&shutDownPack,0,PACK_SIZE_MAX);
        PACK_SET_SIZE(&shutDownPack,PACK_SIZE_MIN);
        PACK_SET_TYPE(&shutDownPack,PT_SHUT_DOWN);
        isInitShutDownPack = 1;
    }
    return ret;
}

static pack_t shutDownAckPack = {0};
static int isInitShutDownAckPack = 0;
int pack_shut_down_ack_init(){
    int ret = 0;
    if(0 == isInitShutDownAckPack){
        memset(&shutDownAckPack,0,PACK_SIZE_MAX);
        PACK_SET_SIZE(&shutDownAckPack,PACK_SIZE_MIN);
        PACK_SET_TYPE(&shutDownAckPack,PT_SHUT_DOWN_ACK);
        isInitShutDownAckPack = 1;
    }
    return ret;
}

static int cb_pack_free(void *data){
    int ret = 0;
    R(NULL == data);
    pack_manager_item_t *item = (pack_manager_item_t *)data;
    R(NULL == item->pack);
    R(pack_manager_item_free(&item));
    return ret;
}

#define TSC_WRITE_ACK(client,pack,data,dataSize) \
    do{\
        pack_ack_init();\
        PACK_SET_NO(&ackPack, PACK_NO_POS(pack));\
        PACK_SET_TIME_WRITE_BY_PACK(&ackPack,pack);\
        PACK_SET_TIME_RECV_BY_PACK(&ackPack,pack);\
        if(NULL != (data) && 0 < (dataSize) ){\
            PACK_SET_SIZE(&ackPack,PACK_SIZE_MIN + dataSize);\
            R(tcp_server_client_write(client,((char *)(&ackPack)),PACK_SIZE_MIN));\
            R(tcp_server_client_write(client,((char *)(data)),(dataSize)));\
            PACK_SET_SIZE(&ackPack,PACK_SIZE_MIN);\
        }else{\
            R(tcp_server_client_write(client,((char *)(&ackPack)),PACK_SIZE_MIN));\
        }\
    }while(0)

#define TSC_WRITE_FIN(client,pack) \
    do{\
        pack_fin_init();\
        PACK_SET_NO(&finPack, PACK_NO_POS(pack));\
        PACK_SET_TIME_WRITE_BY_PACK(&finPack,pack);\
        PACK_SET_TIME_RECV_BY_PACK(&finPack,pack);\
        PACK_SET_TIME_ACK_WRITE_BY_PACK(&finPack,pack);\
        PACK_SET_TIME_ACK_RECV_BY_PACK(&finPack,pack);\
        R(tcp_server_client_write(client,((char *)(&finPack)),PACK_SIZE_MIN));\
    }while(0)

#define TCP_CLIENT_WRITE_ACK(client,pack,data,dataSize) \
    do{\
        pack_ack_init();\
        PACK_SET_NO(&ackPack, PACK_NO_POS(pack) );\
        PACK_SET_TIME_WRITE_BY_PACK(&ackPack,pack);\
        PACK_SET_TIME_RECV_BY_PACK(&ackPack,pack);\
        if(NULL != (data) && 0 < (dataSize)){\
            PACK_SET_SIZE(&ackPack,PACK_SIZE_MIN + dataSize);\
            R(tcp_client_send(client,((char *)(&ackPack)),PACK_SIZE_MIN));\
            R(tcp_client_send(client,((char *)(data)),(dataSize)));\
            PACK_SET_SIZE(&ackPack,PACK_SIZE_MIN);\
        }else{\
            R(tcp_client_send(client,((char *)(&ackPack)),PACK_SIZE_MIN));\
        }\
    }while(0)

#define TCP_CLIENT_WRITE_FIN(client,pack) \
    do{\
        pack_fin_init();\
        PACK_SET_NO(&finPack, PACK_NO_POS(pack));\
        PACK_SET_TIME_WRITE_BY_PACK(&finPack,pack);\
        PACK_SET_TIME_RECV_BY_PACK(&finPack,pack);\
        PACK_SET_TIME_ACK_WRITE_BY_PACK(&finPack,pack);\
        PACK_SET_TIME_ACK_RECV_BY_PACK(&finPack,pack);\
        R(tcp_client_send(client,((char *)(&finPack)),PACK_SIZE_MIN));\
    }while(0)

int pack_manager_item_init(pack_manager_item_t **item,pack_t *pack){
    int ret = 0;
    R(NULL == item);
    R(NULL != (*item));
    R(NULL == pack);
    R(MALLOC_T(item,pack_manager_item_t));
    (*item)->pack = pack;
    gettimeofday(&((*item)->gTime),NULL);
    return ret;
}

int pack_manager_item_free(pack_manager_item_t **item){
    int ret = 0;
    R(NULL == item);
    R(NULL == (*item));
    R(NULL == (*item)->pack);
    R(pack_free( (pack_t **)( &((*item)->pack) ) ));
    R(FREE_T(item,pack_manager_item_t));
    return ret;
}

int pack_manager_init(pack_manager_t **pm){
    int ret = 0;
    int errCode = 0;
    R(NULL == pm);
    R(NULL != *pm);

    R(MALLOC_T(pm,pack_manager_t));
    G_E(mc_list_init(&((*pm)->rPacks),cb_pack_free));
    G_E(mc_list_init(&((*pm)->wPacks),cb_pack_free));
    G_E(mc_list_init(&((*pm)->ePacks),cb_pack_free));

    return ret;
error:
    errCode = ret;
    if(NULL != (*pm)->rPacks) R(mc_list_free(&((*pm)->rPacks)));
    if(NULL != (*pm)->wPacks) R(mc_list_free(&((*pm)->wPacks)));
    if(NULL != (*pm)->ePacks) R(mc_list_free(&((*pm)->ePacks)));
    if(NULL != *pm){
        R(FREE_T(pm,pack_manager_t));
    }
    return errCode;
}

int pack_manager_free(pack_manager_t **pp){
    int ret = 0;
    R(NULL == pp);
    R(NULL == *pp);
    R(mc_list_free(&((*pp)->rPacks)));
    R(mc_list_free(&((*pp)->wPacks)));
    R(mc_list_free(&((*pp)->ePacks)));
    R(FREE_T(pp,pack_manager_t));
    return ret;
}

int pack_manager_modify_read_time(char *buf,unsigned int bufSize,unsigned int *doneSize){
    int ret = 0;
    R(NULL == buf);
    R(NULL == doneSize);
    (*doneSize) = 0;
    char *pos = buf;
    unsigned int packCurrSize = 0;
    unsigned int unDoneSize = bufSize;
    pack_type_e packType = PT_NORMAL;
    while( unDoneSize >= PACK_SIZE_MIN ){
        packCurrSize = PACK_GET_SIZE(pos);
        packType = PACK_GET_TYPE(pos);
        if(packCurrSize + (*doneSize) <= bufSize){
            if(PT_NORMAL == packType){
                PACK_SET_TIME_RECV(pos);
            }else if(PT_ACK == packType){
                PACK_SET_TIME_ACK_RECV(pos);
            }else if( PT_FIN == packType ){
                PACK_SET_TIME_FIN_RECV(pos);
            }
            (*doneSize) += packCurrSize;
            unDoneSize -= packCurrSize;
            pos += packCurrSize;
        }else{
            break;
        }
    }
    return ret;
}

int pack_manager_modify_write_time(char *buf,unsigned int maxWriteSize,unsigned int *needWriteSize){
    int ret = 0;
    R(NULL == buf);
    R(NULL == needWriteSize);
    (*needWriteSize) = 0;
    char *pos = buf;
    unsigned int packCurrSize = 0;
    unsigned int unDoneSize = maxWriteSize;
    pack_type_e packType = PT_NORMAL;
    while( unDoneSize >= PACK_SIZE_MIN ){
        packCurrSize = PACK_GET_SIZE(pos);
        packType = PACK_GET_TYPE(pos);
        if(packCurrSize + (*needWriteSize) <= maxWriteSize){
            if(PT_NORMAL == packType){
                PACK_SET_TIME_WRITE(pos);
            }else if( PT_ACK == packType ){
                PACK_SET_TIME_ACK_WRITE(pos);
            }else if( PT_FIN == packType ){
                PACK_SET_TIME_FIN_WRITE(pos);
            }
            (*needWriteSize) += packCurrSize;
            unDoneSize -= packCurrSize;
            pos += packCurrSize;
        }else{
            break;
        }
    }

    return ret;
}

int pack_manager_item_print(pack_manager_item_t *item){
    int ret = 0;
    pack_print(item->pack);
    printf("gTime[%s]\twTime[%s]\trTime[%s]\tackWTime[%s]\tackRtime[%s]\tfinWtime[%s]\tfinRTime[%s]\t"
            "g2w[%f]\tw2r[%f]\tr2aw[%f]\taw2ar[%f]\tar2fw[%f]\tfw2fr[%f]\n",
            TIMEVAL_2_BUF(item->gTime),
            TIMEVAL_2_BUF(item->wTime),
            TIMEVAL_2_BUF(item->rTime),
            TIMEVAL_2_BUF(item->ackWtime),
            TIMEVAL_2_BUF(item->ackRtime),
            TIMEVAL_2_BUF(item->finWtime),
            TIMEVAL_2_BUF(item->finRtime),
            item->diff.g2w,
            item->diff.w2r,
            item->diff.r2aw,
            item->diff.aw2ar,
            item->diff.ar2fw,
            item->diff.fw2fr
          );

    return ret;
}

static int pack_manager_del_pack_by_no(pack_manager_t *pm,pack_t *pack,pack_manager_item_type_e type){
    int ret = 0;
    R(NULL == pm);
    R(NULL == pack);
    R(PMIT_NONE >= type || PMIT_MAX <= type);

    char *packNo = PACK_NO_POS(pack);
    R(NULL == packNo);
    int i = 0;
    pack_manager_item_t *item = NULL;
    mc_list_t *list = PMIT_WRITE == type ? pm->wPacks : ( PMIT_READ == type ? pm->rPacks : pm->ePacks );
    for(; i < list->size; i++){
        R(mc_list_index((void **)(&item),list,0));
        if(0 == strncmp( PACK_NO_POS(item->pack), packNo, PACK_NO_SIZE)){
            if( PMIT_WRITE == type ){
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_WRITE_POS(pack),&(item->wTime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_RECV_POS(pack),&(item->rTime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_ACK_WRITE_POS(pack),&(item->ackWtime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_ACK_RECV_POS(pack),&(item->ackRtime));
            }else if( PMIT_READ == type ){
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_WRITE_POS(pack),&(item->wTime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_RECV_POS(pack),&(item->rTime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_ACK_WRITE_POS(pack),&(item->ackWtime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_ACK_RECV_POS(pack),&(item->ackRtime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_FIN_WRITE_POS(pack),&(item->finWtime));
                TIMEVAL_BUF_2_TIMEVAL( PACK_TIME_FIN_RECV_POS(pack),&(item->finRtime));
            }else{
            }
            
            R(timeval_diff( &(item->gTime),&(item->wTime),&(item->diff.g2w) ));
            R(timeval_diff( &(item->wTime),&(item->rTime),&(item->diff.w2r) ));
            R(timeval_diff( &(item->rTime),&(item->ackWtime),&(item->diff.r2aw) ));
            R(timeval_diff( &(item->ackWtime),&(item->ackRtime),&(item->diff.aw2ar) ));
            R(timeval_diff( &(item->ackRtime),&(item->finWtime),&(item->diff.ar2fw) ));
            R(timeval_diff( &(item->finWtime),&(item->finRtime),&(item->diff.fw2fr) ));

            if(item->diff.g2w > PACK_G2W_TIMEOUT_MAX) pm->g2wTimeOutCount++;
            if(item->diff.w2r > PACK_W2R_TIMEOUT_MAX) pm->w2rTimeOutCount++;
            if(item->diff.r2aw > PACK_R2AW_TIMEOUT_MAX) pm->r2awTimeOutCount++;
            if(item->diff.aw2ar > PACK_AW2AR_TIMEOUT_MAX) pm->aw2arTimeOutCount++;
            if(item->diff.ar2fw > PACK_AR2FW_TIMEOUT_MAX) pm->ar2fwTimeOutCount++;
            if(item->diff.fw2fr > PACK_FW2FR_TIMEOUT_MAX) pm->fw2frTimeOutCount++;

            // pack_manager_item_print(item);
            R(mc_list_del(list,item));
            return 0;
        }
    }
    return ret;
}

static int pack_manager_add_pack(pack_manager_t *pm,pack_t *pack,pack_manager_item_type_e type){
    int ret = 0;
    R(NULL == pm);
    R(NULL == pack);
    R(PMIT_NONE >= type || PMIT_MAX <= type);
    pack_manager_item_t *item = NULL;
    R(pack_manager_item_init(&item,pack));
    mc_list_t *list = PMIT_WRITE == type ? pm->wPacks : ( PMIT_READ == type ? pm->rPacks : pm->ePacks );
    R(mc_list_add(list,item));
    return ret;
}

int pack_manager_tcp_server_client_recv(tcp_server_client_t *client,cb_recved_data callback){
    int ret = 0;
    R(NULL == client);
    R(NULL == callback);
    pack_t *pack = NULL;
    unsigned int packSize = 0;
    unsigned int packType = 0;
    if(NULL == client->pm){
        R(pack_manager_init(&client->pm));
    }
    char *rspData = NULL;
    unsigned int rspDataSize = 0;
    char *wtPos = NULL;
redo:
    if(ENABLE_BUF_2_PACK(client->readBuf,client->readBufUsed)){
        rspData = NULL;
        rspDataSize = 0;
        pack = NULL;
        R(pack_create_from_buf(&pack,client->readBuf,client->readBufUsed));
        packSize = PACK_GET_SIZE(pack);
        if(client->readBufUsed > packSize){
            memcpy(client->readBuf,client->readBuf + packSize,client->readBufUsed - packSize);
            memset(client->readBuf + (client->readBufUsed - packSize) ,0, client->readBufSize - (client->readBufUsed - packSize));
        }else{
            memset(client->readBuf,0,client->readBufSize);
        }
        client->readBufUsed -= packSize;
        packType = PACK_GET_TYPE(pack);
        if(PT_NORMAL == packType){
            R(pack_manager_add_pack(client->pm,pack,PMIT_READ));
            R(callback((void *)client, ((char *)pack) + PACK_SIZE_MIN, packSize - PACK_SIZE_MIN,&rspData,&rspDataSize));
            TSC_WRITE_ACK(client,pack,rspData,rspDataSize);
        } else if(PT_ACK == packType){
            R(pack_manager_del_pack_by_no(client->pm, pack ,PMIT_WRITE));
            TSC_WRITE_FIN(client,pack);
            if( PACK_SIZE_MIN < packSize ){ // when have extend data
                R(callback((void *)client, ((char *)pack) + PACK_SIZE_MIN, packSize - PACK_SIZE_MIN,&rspData,&rspDataSize));
                if( NULL != rspData && 0 < rspDataSize ){
                    R(pack_manager_tcp_server_client_send(client,rspData,rspDataSize));
                }
            }
            R(FREE_T(&pack,pack_t));
        } else if(PT_FIN == packType){
            R(pack_manager_del_pack_by_no(client->pm, pack ,PMIT_READ));
            R(FREE_T(&pack,pack_t));
        }else if(PT_SHUT_DOWN == packType){
            R(tcp_server_client_shut_down_ack(client));
            R(FREE_T(&pack,pack_t));
        }else if(PT_SHUT_DOWN_ACK == packType){
            R(tcp_server_client_close(&client));
            R(FREE_T(&pack,pack_t));
            goto end;
        }else{
            R(1);
        }
        goto redo;
    }
end:
    return ret;
}



int pack_manager_tcp_client_recv(tcp_client_t *client,cb_recved_data callback){
    int ret = 0;
    R(NULL == client);
    R(NULL == callback);
    pack_t *pack = NULL;
    unsigned int packSize = 0;
    unsigned int packType = 0;
    if(NULL == client->pm){
        R(pack_manager_init(&client->pm));
    }
    char *rspData = NULL;
    unsigned int rspDataSize = 0;
    R(tcp_client_recv(client));
redo:
    if(ENABLE_BUF_2_PACK(client->readBuf,client->readBufUsed)){
        rspData = NULL;
        rspDataSize = 0;
        pack = NULL;
        R(pack_create_from_buf(&pack,client->readBuf,client->readBufUsed));
        packSize = PACK_GET_SIZE(pack);
        if(client->readBufUsed > packSize){
            memcpy(client->readBuf,client->readBuf + packSize,client->readBufUsed - packSize);
            memset(client->readBuf + (client->readBufUsed - packSize) ,0, client->readBufSize - (client->readBufUsed - packSize));
        }else{
            memset(client->readBuf,0,client->readBufSize);
        }
        client->readBufUsed -= packSize;
        packType = PACK_GET_TYPE(pack);

        if(PT_NORMAL == packType){
            R(pack_manager_add_pack(client->pm,pack,PMIT_READ));
            R(callback((void *)client, ((char *)pack) + PACK_SIZE_MIN, packSize - PACK_SIZE_MIN,&rspData,&rspDataSize));
            TCP_CLIENT_WRITE_ACK(client,pack,rspData,rspDataSize);
        }else if(PT_ACK == packType){
            R(pack_manager_del_pack_by_no(client->pm, pack ,PMIT_WRITE));
            TCP_CLIENT_WRITE_FIN(client,pack);
            if( PACK_SIZE_MIN < packSize ){ // when have extend data
                R(callback((void *)client, ((char *)pack) + PACK_SIZE_MIN, packSize - PACK_SIZE_MIN,&rspData,&rspDataSize));
                if(NULL != rspData && 0 < rspDataSize){
                    R(pack_manager_tcp_client_send(client,rspData,rspDataSize));
                }
            }
            R(FREE_T(&pack,pack_t));
        }else if(PT_FIN == packType){
            R(pack_manager_del_pack_by_no(client->pm, pack ,PMIT_READ));
            R(FREE_T(&pack,pack_t));
        }else if(PT_SHUT_DOWN == packType){
            R(tcp_client_shut_down_ack(client));
            R(FREE_T(&pack,pack_t));
        }else if(PT_SHUT_DOWN_ACK == packType){
            R(tcp_client_close(&client));
            R(FREE_T(&pack,pack_t));
            goto end;
        }else{
            R(1);
        }
        goto redo;
    }
end:
    return ret;
}

int pack_manager_tcp_server_client_send(tcp_server_client_t *client,char *data,unsigned int dataSize){
    int ret = 0;
    R(NULL == client);
    R(NULL == data);
    R(0 >= dataSize);
    pack_t *pack = NULL;
    unsigned int packSize = 0;
    if(0 == client->isClosing){
        if(NULL == client->pm){
            R(pack_manager_init(&client->pm));
        }
        R(pack_create_from_data(&pack,data,dataSize));
        packSize = PACK_SIZE_MIN + dataSize;
        R(tcp_server_client_write(client,(char *)pack,packSize));
        R(pack_manager_add_pack(client->pm,pack,PMIT_WRITE));
    }
    return ret;
}

int pack_manager_tcp_client_send(tcp_client_t *client,char *data,unsigned int dataSize){
    int ret = 0;
    R(NULL == client);
    R(NULL == data);
    R(0 >= dataSize);
    pack_t *pack = NULL;
    unsigned int packSize = 0;
    if(0 == client->isClosing){
        if(NULL == client->pm){
            R(pack_manager_init(&client->pm));
        }
        R(pack_create_from_data(&pack,data,dataSize));
        packSize = PACK_SIZE_MIN + dataSize;
        R(tcp_client_send(client,(char *)pack,packSize));
        R(pack_manager_add_pack(client->pm,pack,PMIT_WRITE));
    }
    return ret;
}


int pack_manager_print(pack_manager_t *pm){
    int ret = 0;
    R(NULL == pm);
    R(NULL == pm->rPacks);
    R(NULL == pm->wPacks);
    R(NULL == pm->ePacks);
    int i = 0;
    pack_manager_item_t *item = NULL;
    printf("g2w timeout count : [%u]\t",pm->g2wTimeOutCount);
    printf("w2r timeout count : [%u]\t",pm->w2rTimeOutCount);
    printf("r2aw timeout count : [%u]\t",pm->r2awTimeOutCount);
    printf("aw2ar timeout count : [%u]\t",pm->aw2arTimeOutCount);
    printf("ar2fw timeout count : [%u]\t",pm->ar2fwTimeOutCount);
    printf("fw2fr timeout count : [%u]\n",pm->fw2frTimeOutCount);
    printf("================= read packs :\n");
    for(;i < pm->rPacks->size;i++){
        R(mc_list_index((void **)&item,pm->rPacks,i));
        pack_print(item->pack);
    }
    printf("=================\n\n");
    printf("================= write packs :\n");
    for(i = 0;i < pm->wPacks->size;i++){
        R(mc_list_index((void **)&item,pm->wPacks,i));
        pack_print(item->pack);
    }
    printf("================= \n\n");
    printf("================= error packs :\n");
    for(i = 0;i < pm->ePacks->size;i++){
        R(mc_list_index((void **)&item,pm->ePacks,i));
        pack_print(item->pack);
    }
    printf("================= \n\n");
    return ret;
}

int tcp_server_client_shut_down(tcp_server_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(pack_shut_down_init());
    R(tcp_server_client_write(client,(char *)&shutDownPack,PACK_SIZE_MIN));
    client->isClosing = 1;
    return ret;
}

int tcp_server_client_shut_down_ack(tcp_server_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(pack_shut_down_ack_init());
    R(tcp_server_client_write(client,(char *)&shutDownAckPack,PACK_SIZE_MIN));
    client->isClosing = 2;
    return ret;
}

int tcp_client_shut_down(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(pack_shut_down_init());
    R(tcp_client_send(client,(char *)&shutDownPack,PACK_SIZE_MIN));
    client->isClosing = 1;
    return ret;
}

int tcp_client_shut_down_ack(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(pack_shut_down_ack_init());
    R(tcp_client_send(client,(char *)&shutDownAckPack,PACK_SIZE_MIN));
    client->isClosing = 2;
    return ret;
}
