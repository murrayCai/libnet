#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_DEBUG
#else
#define MODULE_CURR MODULE_NET_DEBUG
#endif

int tcp_write_buffer_print(tcp_write_buffer_t *wBuf){
    int ret = 0;
    R(NULL == wBuf);
    printf("type[%d]\tsize[%u]\twrited[%u]\t:%p\tdata[%p]\n",wBuf->type,wBuf->size,wBuf->writedSize,wBuf,wBuf->data);

    mc_printf_binary(wBuf->data,wBuf->size);

    return ret;
}

int tcp_server_client_print(tcp_server_client_t  *client){
    int ret = 0;
    R(NULL == client);
    
    printf("client write buffer used size : %d\n",client->writeBufs->size);

    if(NULL != client->writeBufs){
        tcp_write_buffer_t *wBuf = NULL;
        int i = 0;
        MC_LIST_FOREACH(wBuf,client->writeBufs){
            printf("wBuf index  : %d\n", i);
            R(tcp_write_buffer_print(wBuf));
        }
    }
    return ret;
}

