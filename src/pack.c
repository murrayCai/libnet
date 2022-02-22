#include "net.h"
#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_PACK
#else
#define MODULE_CURR MODULE_NET_PACK
#endif

static unsigned int packNum = 0;

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

static int pack_no_generate(pack_no_t *p){
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

int pack_create_from_data(pack_t **pack,const char *data,size_t dataSize){
    int ret = 0;
    R(NULL == pack);
    R(NULL != (*pack));
    R(NULL == data);
    R( 0 >= dataSize || PACK_SIZE_DATA_MAX < dataSize );

    R(MALLOC_T(pack,pack_t));

    char fmtSize[32] = {0},fmtType[32] = {0},fmtMsgCode[32] = {0};
    snprintf(fmtSize,32,"%%0%dd",PACK_SIZE_SIZE);
    snprintf( (char *)(*pack), PACK_SIZE_SIZE + 1, fmtSize,PACK_SIZE_MIN + dataSize);

    snprintf(fmtType,32,"%%0%dd",PACK_SIZE_TYPE);
    snprintf( ((char *)(*pack)) + PACK_SIZE_SIZE ,PACK_SIZE_TYPE + 1,fmtType,PT_NORMAL);

    R(pack_no_generate(&((*pack)->no)));

    memcpy( ((char *)(*pack)) + PACK_SIZE_MIN, data, dataSize );
    return ret;
}

int pack_create_from_buf(pack_t **pack,const char *buf,size_t size){
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
    printf("size[%u]\ttype[%u]\tpackNo[%s]\tdata[%s]\n",
            PACK_GET_SIZE(pack),
            PACK_GET_TYPE(pack),
            PACK_POS_NO(pack),
            pack->data);
    return ret;
}

int pack_add_data(pack_t *pack,const char *data,unsigned int size){
    int ret = 0;
    R(NULL == pack);
    R(NULL == data);
    R(0 >= size);
    unsigned int packSize = PACK_GET_SIZE(pack);
    memcpy( ( (char *)pack ) + packSize,data,size );
    PACK_SET_SIZE(pack,packSize + size);
    return ret;
}
