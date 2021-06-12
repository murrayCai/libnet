#include "net.h"
#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_PM
#endif

typedef enum{
    PMIT_NONE,
    PMIT_READ,
    PMIT_WRITE,
    PMIT_ERROR,
    PMIT_MAX
}pm_item_type_e;

static pack_t ackPack = {0};
static int isInitAckPack = 0;
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

static pack_t finPack = {0};
static int isInitFinPack = 0;
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

static pack_t finAckPack = {0};
static int isInitFinAckPack = 0;
int pack_fin_ack_init(){
    int ret = 0;
    if(0 == isInitFinAckPack){
        memset(&finAckPack,0,PACK_SIZE_MAX);
        PACK_SET_SIZE(&finAckPack,PACK_SIZE_MIN);
        PACK_SET_TYPE(&finAckPack,PT_FIN_ACK);
        isInitFinAckPack = 1;
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

static int on_pack_free(void *data){
    int ret = 0;
    R(NULL == data);
    R(FREE_T( ((pack_t **)(&data)),pack_t ));
    return ret;
}

int pm_free(pm_t **pp){
    int ret = 0;
    R(NULL == pp);
    R(NULL == *pp);
    R(mc_list_free(&((*pp)->rPacks)));
    R(mc_list_free(&((*pp)->wPacks)));
    R(FREE_T(pp,pm_t));
    return ret;
}
int pm_init(pm_t **pm,pm_config_t *config){
    int ret = 0;
    int errCode = 0;
    R(NULL == pm);
    R(NULL != *pm);
    R(NULL == config);
    R(NULL == config->onNormalPackRecved);

    R(MALLOC_T(pm,pm_t));
    G_E(mc_list_init(&((*pm)->rPacks),on_pack_free));
    G_E(mc_list_init(&((*pm)->wPacks),on_pack_free));
    (*pm)->config = config;
    return ret;
error:
    errCode = ret;
    if(NULL != (*pm)->rPacks) R(mc_list_free(&((*pm)->rPacks)));
    if(NULL != (*pm)->wPacks) R(mc_list_free(&((*pm)->wPacks)));
    if(NULL != *pm){
        R(FREE_T(pm,pm_t));
    }
    return errCode;
}

#define PM_GET_LIST(pm,type) ( PMIT_WRITE == (type) ? (pm)->wPacks : (pm)->rPacks )

static int pm_add(pm_t *pm,pack_t *pack,pm_item_type_e type){
    int ret = 0;
    R(NULL == pm);
    R(NULL == pack);
    R(PMIT_NONE >= type || PMIT_MAX <= type);
    mc_list_t *list = PM_GET_LIST(pm,type) ;
    R(NULL == list);
    R(mc_list_add(list,pack));
    return ret;
}

static int pm_del(pm_t *pm,pack_t *pack,pm_item_type_e type){
    int ret = 0;
    R(NULL == pm);
    R(NULL == pack);
    R(PMIT_NONE >= type || PMIT_MAX <= type);
    mc_list_t *list = PM_GET_LIST(pm,type) ;
    R(NULL == list);
    char *packNo = PACK_POS_NO(pack);
    R(NULL == packNo);
    int i = 0;
    pack_t *tmp = NULL;
    for(; i < list->size; i++){
        R(mc_list_index((void **)(&tmp),list,0));
        if(0 == strncmp( PACK_POS_NO(tmp), packNo, PACK_SIZE_NO)){
            R(mc_list_del(list,tmp));
            return 0;
        }
    }
    return ret;
}

#define CLIENT_A void *c,client_type_e type
#define CLIENT_A_CHECK() ( NULL == (c) || ( CLIENT_TYPE_CHECK(type)  ) ) 
#define CLIENT_P client,type
#define PM_CLEINT_DEFINE()\
    tcp_client_define_t * client = (tcp_client_define_t *)c;\
    pm_t *pm = (pm_t *)client->userData;


#define PM_ADD_READ(pack) (pm_add(pm,(pack),PMIT_READ))
#define PM_ADD_WRITE(pack) (pm_add(pm,(pack),PMIT_WRITE))
#define PM_DEL_READ(pack) (pm_del(pm,(pack),PMIT_READ))
#define PM_DEL_WRITE(pack) (pm_del(pm,(pack),PMIT_WRITE))

int tcp_server_client_shut_down(tcp_server_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(pack_shut_down_init());
    R(tcp_server_client_write(client,(char *)&shutDownPack,PACK_SIZE_MIN));
    return ret;
}

#define PM_WRITE_DATA(data,size)\
    ( PMCT_CLIENT == (type) ? \
      ( tcp_client_send( (tcp_client_t *)(client), ((char *)(data)),(size)) ) : \
      ( tcp_server_client_write( (tcp_server_client_t *)(client), ((char *)(data)),(size)) ) )

#define PM_WRITE_PACK(pack) PM_WRITE_DATA( (pack), PACK_GET_SIZE(pack) )

static int pm_recv(CLIENT_A,const char *buf,unsigned int size,unsigned int *retUsed){
    int ret = 0;
    R(NULL == buf);
    R(NULL == retUsed);
    R(0 >= size);
    R(CLIENT_A_CHECK());
    PM_CLEINT_DEFINE();
    pack_t *pack = NULL;
    unsigned int packType = 0;
    const char *pos = buf;
    unsigned int unUsedSize = size;
    unsigned int isEnd = 0;
    unsigned int packSize = 0;

redo:
    if(ENABLE_BUF_2_PACK(pos,unUsedSize)){
        pack = NULL;
        packSize = 0;
        R(pack_create_from_buf(&pack,pos,unUsedSize));
        packSize = PACK_GET_SIZE(pack);
        packType = PACK_GET_TYPE(pack);
        if(PT_NORMAL == packType){
            R(PM_ADD_READ(pack));
            // write ack package
            R(pack_ack_init());
            PACK_SET_NO(&ackPack, PACK_POS_NO(pack));
            PACK_SET_SIZE(&ackPack, PACK_SIZE_MIN);
            if(NULL != pm->config->onNormalPackRecved){
                R(pm->config->onNormalPackRecved( (void *)(client), pack, &ackPack ) );
            }
            R(PM_WRITE_PACK(&ackPack));
        }else if(PT_ACK == packType){
            R(pack_fin_init());
            PACK_SET_NO(&finPack, PACK_POS_NO(pack));
            PACK_SET_SIZE(&finPack, PACK_SIZE_MIN);
            if(NULL != pm->config->onAckPackRecved){\
                R(pm->config->onAckPackRecved( (void *)(client), (pack), &finPack ) );
            }
            R(PM_WRITE_PACK(&finPack));
            R(PM_DEL_WRITE(pack));
        }else if(PT_FIN == packType){
            R(pack_fin_ack_init());
            PACK_SET_NO(&finAckPack, PACK_POS_NO(pack));
            PACK_SET_SIZE(&finAckPack, PACK_SIZE_MIN);
            if(NULL != pm->config->onFinPackRecved){
                R(pm->config->onFinPackRecved( (void *)(client), pack, &finAckPack ) );
            }
            R(PM_WRITE_PACK(&finAckPack));
            R(PM_DEL_READ(pack));
        }else if(PT_FIN_ACK == packType){
            if( NULL != pm->config->onFinAckPackRecved ){
                R(pm->config->onFinAckPackRecved ( (void *)(client), pack ) );
            }
        }else if(PT_SHUT_DOWN == packType){
            R(pack_shut_down_ack_init());
            R(PM_WRITE_DATA(&shutDownAckPack,PACK_SIZE_MIN));
            pm->status = PMSDS_REQ;
        }else if(PT_SHUT_DOWN_ACK == packType){
            pm->status = PMSDS_OK;
            isEnd = 1;
        }else{
            R(1);
        }
        if(PT_NORMAL != packType){
            R(FREE_T(&pack,pack_t));
        }
        pos += packSize;
        unUsedSize -= packSize;
        if(0 == isEnd) goto redo;
    }
    (*retUsed) = size - unUsedSize;
    return ret;
}

int pm_on_server_recv(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(NULL == used);
    R(0 >= size);
    R(pm_recv((tcp_client_define_t *)client,PMCT_SERVER_CLIENT,buf,size,used));
    return ret;
}


int pm_on_client_recv(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(NULL == used);
    R(0 >= size);
    R(pm_recv(client,PMCT_CLIENT,buf,size,used));
    return ret;
}

static pm_shut_down(CLIENT_A){
    int ret = 0;
    R(CLIENT_A_CHECK());
    PM_CLEINT_DEFINE();
    R(pack_shut_down_init());
    R(PM_WRITE_DATA(&shutDownPack,PACK_SIZE_MIN));
    pm->status = PMSDS_REQ;
    return ret;
}

int pm_server_client_shut_down(tcp_server_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(pm_shut_down(client,PMCT_SERVER_CLIENT));
    return ret;
}


int pm_client_shut_down(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    pm_t *pm = (pm_t *)(client->userData);
    if(PMSDS_NONE == pm->status){
        R(pm_shut_down(client,PMCT_CLIENT));
    }else {
        R(tcp_client_recv(client));
        if( pm->rPacks->size > 0 || pm->wPacks->size > 0 ){

        }else{
            if( PMSDS_OK == pm->status ){
                R(tcp_client_close( (tcp_client_t **)(&client) ) );
            }
        }
    }
    return ret;
}

static int _pm_send_buf(CLIENT_A,const char *data,unsigned int dataSize){
    int ret = 0;
    R(NULL == data);
    R(0 >= dataSize);
    R(CLIENT_A_CHECK());
    PM_CLEINT_DEFINE();
    pack_t *pack = NULL;
    unsigned int packSize = 0;
    if(PMSDS_NONE == pm->status){
        R(pack_create_from_data(&pack,data,dataSize));
        packSize = PACK_SIZE_MIN + dataSize;
        R(PM_WRITE_DATA(pack,packSize));
        R(PM_ADD_WRITE(pack));
    }
    return ret;
}

int pm_server_client_send(tcp_server_client_t *client,const char *buf,unsigned int size){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(0 > size);
    R(_pm_send_buf(client,PMCT_SERVER_CLIENT,buf,size));
    return ret;
}

int pm_client_send(tcp_client_t *client,const char *buf,unsigned int size){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(0 > size);
    R(_pm_send_buf(client,PMCT_CLIENT,buf,size));
    return ret;
}

int pm_on_before_write(CLIENT_A,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    R(NULL == buf);
    R(NULL == used);
    R(0 >= size);
    R(CLIENT_A_CHECK());
    PM_CLEINT_DEFINE();

    unsigned int packCurrSize = 0;
    unsigned int doneSize = 0;
    unsigned int unDoneSize = size;
    pack_type_e packType = PT_NORMAL;
    const char *pos = buf;

    while( unDoneSize >= PACK_SIZE_MIN ){
        packCurrSize = PACK_GET_SIZE(pos);
        packType = PACK_GET_TYPE(pos);
        if(packCurrSize + doneSize <= size){
            if(PT_NORMAL == packType){
                if(NULL != pm->config->onNormalPackBeforeWrite){
                    R(pm->config->onNormalPackBeforeWrite( (void *)(client), (pack_t *)pos ) );
                }
            }else if(PT_ACK == packType){
                if(NULL != pm->config->onAckPackBeforeWrite){
                    R(pm->config->onAckPackBeforeWrite( (void *)(client), (pack_t *)pos ) );
                }
            }else if( PT_FIN == packType ){
                if(NULL != pm->config->onFinPackBeforeWrite){
                    R(pm->config->onFinPackBeforeWrite( (void *)(client), (pack_t *)pos ) );
                }
            }else if( PT_FIN_ACK == packType ){
                if(NULL != pm->config->onFinAckPackBeforeWrite){
                    R(pm->config->onFinAckPackBeforeWrite( (void *)(client), (pack_t *)pos ) );
                }
            } else if( PT_SHUT_DOWN == packType ){
                if(NULL != pm->config->onShutDownPackBeforeWrite){
                    R(pm->config->onShutDownPackBeforeWrite( (void *)(client), (pack_t *)pos ) );
                }
            }else if ( PT_SHUT_DOWN_ACK == packType ){
                if(NULL != pm->config->onShutDownAckPackBeforeWrite){
                    R(pm->config->onShutDownAckPackBeforeWrite( (void *)(client), (pack_t *)pos ) );
                }
            }
            doneSize += packCurrSize;
            unDoneSize -= packCurrSize;
            pos += packCurrSize;
        }else{
            break;
        }
    }
    *used = doneSize;
    return ret;
}

int pm_on_server_before_write(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(NULL == used);
    R(0 >= size);
    R( pm_on_before_write( client, PMCT_SERVER_CLIENT,buf,size,used ) );
    return ret;
}

int pm_on_client_before_write(tcp_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;
    R(NULL == client);
    R(NULL == buf);
    R(NULL == used);
    R(0 >= size);
    R( pm_on_before_write( client, PMCT_CLIENT,buf,size,used ) );
    return ret;
}


int pm_print(pm_t *pm){
    int ret = 0;
    R(NULL == pm);
    R(NULL == pm->rPacks);
    R(NULL == pm->wPacks);
    int i = 0;
    printf("isClosing : [%u]\n",pm->status);
    printf("================= read packs :\n");
    pack_t *pack = NULL;
    for(;i < pm->rPacks->size;i++){
        R(mc_list_index((void **)&pack,pm->rPacks,i));
        pack_print(pack);
    }
    printf("=================\n\n");
    printf("================= write packs :\n");
    for(i = 0;i < pm->wPacks->size;i++){
        R(mc_list_index((void **)&pack,pm->wPacks,i));
        pack_print(pack);
    }
    printf("================= \n\n");
    return ret;
}
