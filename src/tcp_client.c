#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TCP_CLIENT
#endif

int tcp_client_init(tcp_client_t **pp,unsigned int readBufSize,unsigned int writeBufSize,cb_tcp_client_closed cbClosed){
    int ret = 0;
    R(NULL == pp);
    R(NULL != *pp);
    R(NULL == cbClosed);
    R(0 >= readBufSize);
    R(0 >= writeBufSize);
    R(MALLOC_T(pp,tcp_client_t));
    R(MALLOC_TN(&( (*pp)->readBuf ),str_t, readBufSize));
    R(MALLOC_TN(&( (*pp)->writeBuf ),str_t, writeBufSize));
    (*pp)->readBufSize = readBufSize;
    (*pp)->writeBufSize = writeBufSize;
    (*pp)->onClosed = cbClosed;
    return ret;
}


int tcp_client_close(tcp_client_t **pp){
    int ret = 0;
    R(NULL == pp);
    R(NULL == (*pp));

    if((*pp)->readBufSize > 0){
        R(FREE_TN(&(*pp)->readBuf,str_t,(*pp)->readBufSize));
    }
    if((*pp)->writeBufSize > 0){
        R(FREE_TN(&(*pp)->writeBuf,str_t,(*pp)->writeBufSize));
    }
    close((*pp)->fd);
    if(NULL != (*pp)->onClosed){
        R((*pp)->onClosed((*pp)));
    }
    if(NULL != (*pp)->pm){
        R(pack_manager_free( &( (*pp)->pm ) ));
    }
    R(FREE_T(pp,tcp_client_t));
    return ret;
}

int tcp_client_connect(tcp_client_t *client,const char *ip,unsigned int port){
    int ret = 0;
    R(NULL == client);
    R(NULL == ip);

    R(-1 == ( client->fd =  socket(PF_INET,SOCK_STREAM,IPPROTO_TCP) ) );

    client->addr.sin_family = AF_INET;
    client->addr.sin_addr.s_addr = inet_addr(ip);
    client->addr.sin_port = htons(port);

    R(connect(client->fd,(struct sockaddr *)&(client->addr),sizeof(struct sockaddr_in)));

    return ret;
}

int tcp_client_recv(tcp_client_t *client){
    int ret = 0;
    R(NULL == client);
    R(NULL == client->readBuf);
    unsigned int leaveBufSize = client->readBufSize - client->readBufUsed;
    int readSize = recv(client->fd,client->readBuf + client->readBufUsed,leaveBufSize,0);
    if( -1 == readSize ){
        if(EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno){
            ret = 0;
        }else{
            if(!IS_VALID_FD(client->fd)){
                R(tcp_client_close(&client));
            }else{
                R(1);
            }
        }
    }else if( 0 == readSize ){
        R(tcp_client_close(&client));
    }else{
        client->readBufUsed += readSize;
        // set recv time
        unsigned int doneSize = 0;
        R(pack_manager_modify_read_time(client->readBuf,client->readBufUsed,&doneSize));
        R(doneSize != client->readBufUsed);
    }
    return ret;
}

int tcp_client_send(tcp_client_t *client,char *buf,size_t size){
    int ret = 0,errRet = 0;
    R(NULL == client);
    unsigned int leaveBufSize = client->writeBufSize - client->writeBufUsed;
    unsigned int maxWriteSize = client->writeBufUsed + size;
    char *bufTmp = NULL;
    unsigned int isMalloced = 0;
    unsigned int mallocSize = client->writeBufUsed + size + 16;
    if(leaveBufSize < size){
        R(MALLOC_TN( &bufTmp, str_t, mallocSize ));
        isMalloced = 1;
        memcpy(bufTmp,client->writeBuf,client->writeBufUsed);
        memcpy(bufTmp + client->writeBufUsed, buf,size);
    }else{
        memcpy(client->writeBuf + client->writeBufUsed,buf,size);
        client->writeBufUsed += size;
        bufTmp = client->writeBuf;
    }

    // set write time
    unsigned int needWriteSize = 0;
    R(pack_manager_modify_write_time(bufTmp,maxWriteSize,&needWriteSize));
    G_E(0 >= needWriteSize);

    if(!IS_VALID_FD(client->fd)){
        G_E(tcp_client_close(&client));
    }else{
        int writeSize = send(client->fd,bufTmp,needWriteSize,0);
        if( -1 == writeSize ){
            if(EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno){
                ret = 0;
            }else{
                if(ENOTSOCK == errno || ECONNRESET == errno || EBADF == errno){
                    R(tcp_client_close(&client));
                }else{
                    if(!IS_VALID_FD(client->fd)){
                    }else{
                        R(1);
                    }
                }
            }
        }else{
            client->writeBufUsed = maxWriteSize - needWriteSize;
            if(client->writeBufUsed > 0){
                memcpy(client->writeBuf,bufTmp,client->writeBufUsed);
                memset(client->writeBuf + client->writeBufUsed,0,client->writeBufSize - client->writeBufUsed);
            }else{
                memset(client->writeBuf,0,client->writeBufSize);
            }
        }
    }
    if(isMalloced) R(FREE_TN(&bufTmp,str_t,mallocSize));
    return ret;
error:
    errRet = ret;
    if(isMalloced) R(FREE_TN(&bufTmp,str_t,mallocSize));
    return errRet;
}


