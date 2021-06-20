#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_WEB_SOCKET
#endif

#define WS_HAND_SHAKE_HEADER "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "

int ws_server_hand_shake(tcp_server_client_t *client,const char *key,unsigned int keySize){
    int ret = 0,errRet = 0;
    R(NULL == client);
    R(NULL == key);
    R( 0 >= keySize );
    char wsKey[64] = {0};
    memcpy(wsKey,key,keySize);

    char mergedKey[256] = {0};
    sprintf(mergedKey,"%s%s",wsKey,"258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    char sha1Content[21] = {0};
    SHA1(sha1Content,mergedKey,strlen(mergedKey));

    char *base64Buf = NULL;
    unsigned int base64BufSize = 0;
    R(base64_encode(sha1Content,20,&base64Buf,&base64BufSize));

    unsigned int bufSize = strlen(WS_HAND_SHAKE_HEADER) + base64BufSize + 5;
    char *buf = NULL;
    G_E(MALLOC_TN(&buf,str_t,bufSize));
    snprintf(buf,bufSize,"%s%s\r\n\r\n",WS_HAND_SHAKE_HEADER,base64Buf);
    G_E(tcp_server_client_write(client,buf,strlen(buf)));

error:
    errRet = ret;
    if(NULL != base64Buf) R(FREE_TN(&base64Buf,str_t,base64BufSize));
    if(NULL != buf) R(FREE_TN(&buf,str_t,bufSize));
    return errRet;
}

int ws_server_send(tcp_server_client_t *client,const char *data,unsigned int dataSize){
    int ret = 0;
    R(NULL == client);
    R(NULL == data);
    R( 0 >= dataSize );

    unsigned int headerSize = 0;
    headerSize = ( 125 >= dataSize ? 2 : (  0xFFFF >= dataSize ? 4 : 10  ) );
    char *buf = NULL;
    unsigned int bufSize = dataSize + headerSize;
    R(MALLOC_TN(&buf,str_t,bufSize));
    buf[0] = 0x81;
    if( 125 >= dataSize ){
        buf[1] = dataSize;
    }else if( 0xFFFF >= dataSize ){
        buf[1] = 126;
        buf[2] = ( dataSize & 0x000000FF );
        buf[3] = ( ( dataSize & 0x0000FF00 ) >> 8 );
    }else{
        buf[1] = 127;
        buf[2] = ( dataSize & 0x000000FF );
        buf[3] = ( ( dataSize & 0x0000FF00 ) >> 8 );
        buf[4] = ( ( dataSize & 0x00FF0000 ) >> 16 );
        buf[5] = ( ( dataSize & 0xFF000000 ) >> 24 );
    }
    memcpy(buf + headerSize, data,dataSize);
    G_E(tcp_server_client_write(client,buf,bufSize));

error:
    if(NULL != buf) R(FREE_TN(&buf,str_t,bufSize));
    return ret;
}


int ws_server_parse(tcp_server_client_t *client,
        const char *data,unsigned int dataSize,
        unsigned int *used,unsigned int *isFin,
        cb_ws_msg_recved onWsMsgRecved){
    int ret = 0;
    R(NULL == client);
    R(NULL == data);
    R(NULL == used);
    R(NULL == isFin);
    R(NULL == onWsMsgRecved);

    char *buf = NULL;
    unsigned int isUseMask = 0;
    const char *headerTail = NULL, *rawData = NULL;
    const char *mask = NULL;
    unsigned int payloadSize = 0,low = 0,high = 0,packSize = 0,i = 0,protocol = 0;

reParse:
    headerTail = NULL;
    rawData = NULL;
    payloadSize = 0;low = 0;high = 0;packSize = 0;i = 0;protocol = 0;
    packSize = 0;

    G( 2 >= dataSize, reread );
    payloadSize = data[1];
    isUseMask = payloadSize >= 128 ? 1 : 0;
    payloadSize = (payloadSize & 0x0000007F);
    if( 125 >= payloadSize ){
        headerTail = data + 2;
    }else if( 126 == payloadSize ){
        G( 8 >= dataSize, reread );
        payloadSize = ( (data[2]) | (data[3] << 8) );
        headerTail = data + 2 + 2;
    }else if( 127 == payloadSize ){
        G( 14 >= dataSize, reread );
        low = ( (data[2]) | (data[3] << 8) | (data[4] << 16) | (data[5] << 24) );
		high = ( (data[6]) | (data[7] << 8) | (data[8] << 16) | (data[9] << 24) );
        R( 0 != high ); // 表示后四个字节有数据int存放不了，太大了，我们不要了。
		payloadSize = low;
		headerTail = data + 2 + 8;
    }
    packSize = headerTail - data + ( isUseMask ? 4 : 0 ) + payloadSize;
    rawData = isUseMask ?  headerTail + 4 : headerTail;
    
    G( packSize > dataSize, reread);
    if( payloadSize > 0 ){
        R(MALLOC_TN(&buf,str_t,payloadSize));
        mask = headerTail;
        if( isUseMask ){
            i = 0;
            for(; i < payloadSize; i++){
                buf[i] = rawData[i] ^ mask[ i % 4 ];
            }
        }else{
            memcpy(buf,rawData,payloadSize);
        }
        R(onWsMsgRecved(client,buf,payloadSize));
        R(FREE_TN(&buf,str_t,payloadSize));
    }
    
    protocol = data[0];
    if(protocol >= 128 || (*isFin) == 1){
        (*isFin) = 1;
    }
    (*used) += packSize;
    data += packSize;
    dataSize -= packSize;
    if( dataSize > 0 ){
        goto reParse;
    }

    return ret;

reread:
    ret = 0;
    if( NULL != buf ) R(FREE_TN(&buf,str_t,payloadSize));
    return ret;
}

