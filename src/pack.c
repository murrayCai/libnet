#include "net.h"
#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_PACK
#endif

static unsigned int packNum = 0;

int pack_no_generate(pack_no_t *p){
    int ret = 0;
    R(NULL == p);
    char num[11] = {0},pid[7] = {0};
    
    snprintf( num, 11, "%010d",packNum++);
    snprintf( pid, 7, "%06d",getpid());
    
    memcpy( ((char *)(p)), TIMEVAL_BUF_GENERATE(), 16);
    memcpy( ((char *)(p)) + 16, num,10);
    memcpy( ((char *)(p)) + 26, pid,6);
    return ret;
}


int pack_free(pack_t **pack){
    int ret = 0;
    R(NULL == pack);
    R(NULL == (*pack));
    R(FREE_T(pack,pack_t));
    return ret;
}

int pack_create_from_data(pack_t **pack,void *data,size_t dataSize){
    int ret = 0;
    R(NULL == pack);
    R(NULL != (*pack));
    R(NULL == data);
    R( 0 >= dataSize || PACK_DATA_SIZE_MAX < dataSize );

    R(MALLOC_T(pack,pack_t));

    char fmtSize[32] = {0},fmtType[32] = {0},fmtMsgCode[32] = {0};
    snprintf(fmtSize,32,"%%0%dd",PACK_SIZE_SIZE);
    snprintf( (char *)(*pack), PACK_SIZE_SIZE + 1, fmtSize,PACK_SIZE_MIN + dataSize);
    
    snprintf(fmtType,32,"%%0%dd",PACK_TYPE_SIZE);
    snprintf( ((char *)(*pack)) + PACK_SIZE_SIZE ,PACK_TYPE_SIZE + 1,fmtType,PT_NORMAL);
    
    R(pack_no_generate(&((*pack)->no)));

    memset(PACK_TIME_GLOBAL_POS((*pack)),'0',PACK_TIME_SIZE);
    PACK_SET_TIME_GLOBAL((*pack));

    memcpy( ((char *)(*pack)) + PACK_SIZE_MIN, data, dataSize );
    return ret;
}


int pack_create_from_buf(pack_t **pack,void *buf,size_t size){
    int ret = 0;
    R(NULL == pack);
    R(NULL != (*pack));
    R(NULL == buf);
    R( PACK_SIZE_MIN > size );
    unsigned int packSize = PACK_GET_SIZE(buf);
    RL(0 == packSize,"buf[%s][%u]\t[%u]\n",buf,size,PACK_GET_SIZE(buf));
    R( packSize >  PACK_SIZE_MAX );
    R(MALLOC_T(pack,pack_t));
    memcpy( (*pack) ,buf,packSize);
    return ret;
}

int pack_print(pack_t *pack){
    int ret = 0;
    printf("size[%u]\ttype[%u]\tpackNo[%s]\tgt[%s]\twt[%s]\trt[%s]\tackwt[%s]\tackrt[%s]\tfinwt[%s]\tfinrt[%s]\tdata[%s]\n",
            PACK_GET_SIZE(pack),
            PACK_GET_TYPE(pack),
            PACK_NO_POS(pack),
            pack->time.gTime,
            pack->time.wTime,
            pack->time.rTime,
            pack->time.ackWtime,
            pack->time.ackRtime,
            pack->time.finWtime,
            pack->time.finRtime,
            pack->data);
    return ret;
}

