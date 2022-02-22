#include "net.h"

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_WEB_SOCKET
#else
#define MODULE_CURR MODULE_NET_WEB_SOCKET
#endif

#define WS_HAND_SHAKE_HEADER "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "
#define WS_HAND_SHAKE_TAILER "Sec-WebSocket-Origin: null\r\nSec-WebSocket-Location: ws://192.168.10.188:16678/"

int ws_server_hand_shake(tcp_server_client_t *client,const char *key,unsigned int keySize){
    int ret = 0,errRet = 0;
    R(NULL == client);
    R(NULL == key);
    R( 0 >= keySize );
    char wsKey[64] = {0};
    memcpy(wsKey,key,keySize);

    char mergedKey[256] = {0};
    sprintf(mergedKey,"%s%s",wsKey,"258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    // DP("merged key : [%d]%s\n", keySize, wsKey);

    char sha1Content[21] = {0};
    MC_SHA1(sha1Content,mergedKey,strlen(mergedKey));

    char *base64Buf = NULL;
    unsigned int base64BufSize = 0;
    R(base64_encode(sha1Content,20,&base64Buf,&base64BufSize));

    unsigned int bufSize = strlen(WS_HAND_SHAKE_HEADER) + base64BufSize + 4;
    char *buf = NULL;
    G_E(MALLOC_TN(&buf,str_t,bufSize));
    memcpy(buf,WS_HAND_SHAKE_HEADER,strlen(WS_HAND_SHAKE_HEADER));
    memcpy(buf + strlen(WS_HAND_SHAKE_HEADER),base64Buf,base64BufSize);
    memcpy(buf + strlen(WS_HAND_SHAKE_HEADER) + base64BufSize,"\r\n\r\n",4);
    G_E(tcp_send_buf((tcp_client_define_t *)client,buf,bufSize,1,0));

end:
    if(NULL != base64Buf) R(FREE_TN(&base64Buf,str_t,base64BufSize));
    return errRet;
error:
    errRet = ret;
    if(NULL != buf) R(FREE_TN(&buf,str_t,bufSize));
    goto end;
}

int ws_server_send_close(tcp_server_client_t *client){
    int ret = 0;
    char buf[2] = {0x88,0x00};
    R(tcp_send_buf((tcp_client_define_t *)client,buf,2,0,0));
    client->isWaitClose = 1;
    return ret;
}

int ws_server_send(tcp_server_client_t *client,char *data,unsigned int dataSize,int isBinary,int isNeedFree,int isNeedTimeStamp){
    int ret = 0,retErr = 0;
    R(NULL == client);
    R(NULL == data);
    R( 0 >= dataSize );

    unsigned int headerSize = 0;
    headerSize = ( 125 >= dataSize ? 2 : (  0xFFFF >= dataSize ? 4 : 10  ) );
    char *header = NULL;
    R(MALLOC_TN(&header,str_t,headerSize));
    header[0] = isBinary ? 0x82 : 0x81;
    int nLen = 0;
    if( dataSize < 126 ){
        header[1] = dataSize;
    }else if( dataSize < 0xFFFF ){
        nLen = htons( (uint16_t)dataSize );
        header[1] = 126;
        memcpy( header + 2, &nLen, 2 );
    }else{
        G_E(1); /* too big package */
    }
    G_E(tcp_send_buf((tcp_client_define_t *)client,header,headerSize,1,0));
    G_E(tcp_send_buf((tcp_client_define_t *)client,data,dataSize,isNeedFree,isNeedTimeStamp));
    
    return ret;
error:
    retErr = ret;
    if(isNeedFree){
        FREE_TN(&data,str_t,dataSize);
    }
    if(NULL != header) R(FREE_TN(&header,str_t,headerSize));
    return retErr;
}

int ws_server_data_parse(tcp_server_client_t *client,unsigned int size){
    int ret = 0;
    char *dataBuf = NULL;
    unsigned int i = 0;
    R(NULL == client);
    R(size > client->wsHeader.dataSize - client->wsHeader.hadReadDataSize);
    R(size > client->readBufUsed);
    dataBuf = client->readBuf + client->readBufUsed - size;
    for(; i < size; i++){
        dataBuf[i] = dataBuf[i] ^ client->wsHeader.mask[ (i + client->wsHeader.hadReadDataSize) % 4 ];
    }

    client->wsHeader.hadReadDataSize += size;

    R(client->wsHeader.hadReadDataSize > client->wsHeader.dataSize);
    if(client->wsHeader.dataSize == client->wsHeader.hadReadDataSize){
        memset(&client->wsHeader,0,sizeof(ws_header_t));
    }

    return ret;
}

static int ws_server_read_head(tcp_server_client_t *client){
    int ret = 0;
    unsigned int readSize = 0;
    ws_header_t *h = NULL;
    R(NULL == client);
    h = &client->wsHeader;
    if(h->bufSize < 2){
        R(TCP_READ(client,h->buf + h->bufSize,2 - h->bufSize,&readSize));
        if(readSize == 0) return 0;
        h->bufSize += readSize;
        if(h->bufSize < 2) return 0;
    }
    h->headerSize = 2 + 
        ( (126 ==  (h->buf[1] & 0x7fu)) ? 2 : 0 ) + 
        ( ((h->buf[1] >> 7) & 1) ? 4 : 0 );
    readSize = 0;
    R(TCP_READ(client,h->buf + h->bufSize, h->headerSize - h->bufSize,&readSize));
    if(0 == readSize) return 0;
    h->bufSize += readSize;
    if(h->bufSize < h->headerSize) return 0;
    return ret;
}

int ws_server_header_parse(tcp_server_client_t *client,unsigned int *isParsed){
    int ret = 0,retErr = 0;
    unsigned int isUseMask = 0,maskPrefixSize = 0;
    char *data = client->wsHeader.buf;
    ws_header_t *h = NULL;
    char payloadSizeTag = 0;
    uint16_t wsDataSize = 0,hx = 0;
    unsigned int readSize = 0;
    R(NULL == client);
    R(NULL == isParsed);
    h = &client->wsHeader;
    if(0 == h->bufSize || 0 == h->headerSize || h->headerSize > h->bufSize) R(ws_server_read_head(client));
    if(0 == h->bufSize || 0 == h->headerSize || h->headerSize > h->bufSize) goto reRead;

    switch(client->wsHeader.statu){
        case WSHP_NONE : goto parsePrefix;
        case WSHP_PRFIX_OK: goto parsePayloadSizeTag;
        case WSHP_PAYLOAD_OK: goto parseMask;
        default : R(1);
    }

parsePrefix:
    client->wsHeader.isFin = (data[0] >> 7) & 1;
    client->wsHeader.isUseMask = (data[1] >> 7) & 1;
    client->wsHeader.payloadSizeTag = (data[1] & 0x7fu);

    if(data[0] & 0x8){
        client->wsHeader.statu = WSHP_CLOSE;
        goto close;
    }else{
        client->wsHeader.statu = WSHP_PRFIX_OK;
    }

parsePayloadSizeTag:
    if( 125 >= client->wsHeader.payloadSizeTag){
        wsDataSize = client->wsHeader.payloadSizeTag;
    }else if( 126 == client->wsHeader.payloadSizeTag ){
        memcpy(&hx,data + 2,sizeof(uint16_t));
        wsDataSize = ntohs(hx);
    }else{
        R(1);
    }
    client->wsHeader.dataSize = wsDataSize;
    client->wsHeader.statu = WSHP_PAYLOAD_OK;

parseMask:
    if(client->wsHeader.isUseMask){
        memcpy(client->wsHeader.mask,h->buf + h->bufSize - 4,4);
    }
    client->wsHeader.statu = WSHP_MASK_OK;
    client->wsHeader.hadReadDataSize = 0;
    *isParsed = 1;

    return ret;
reRead:
    *isParsed = 0;
    return 0;
close:
    R(ws_server_send_close(client));
    return 0;
}

int ws_server_parse(tcp_server_client_t *client,
        const char *data,unsigned int dataSize,
        unsigned int *used,unsigned int *isFin,
        cb_ws_msg_recved onWsMsgRecved){
    int ret = 0;
    char *buf = NULL;
    unsigned int isUseMask = 0;
    const char *headerTail = NULL, *rawData = NULL;
    const char *mask = NULL;
    char payloadSizeTag = 0;
    uint16_t wsDataSize = 0;
    unsigned int low = 0,high = 0,packSize = 0,i = 0,protocol = 0,dataUsed = 0;
    R(NULL == client);
    R(NULL == data);
    R(NULL == used);
    R(NULL == isFin);
    R(NULL == onWsMsgRecved);


reParse:
    headerTail = NULL;
    rawData = NULL;
    wsDataSize = 0,low = 0;high = 0;
    packSize = 0;i = 0;protocol = 0;

    if( 2 >= dataSize ){
        goto reread;
    }
    (*isFin) = (data[0] >> 7) & 1;
    isUseMask = (data[1] >> 7) & 1;
    payloadSizeTag = (data[1] & 0x7fu);
    if( 125 >= payloadSizeTag ){
        headerTail = data + 2;
        wsDataSize = payloadSizeTag;
    }else if( 126 == payloadSizeTag ){
        G( 8 >= dataSize, reread );
        uint16_t h;
        memcpy(&h,data + 2,sizeof(uint16_t));
        wsDataSize = ntohs(h);
        printf("%d/%d\n",h,wsDataSize);
        headerTail = data + 2 + 2;
    }else if( 127 == payloadSizeTag ){
        TCP_CLIENT_SET_CLOSE(client);
        return ret;
        /*
        G( 14 >= dataSize, reread );
        low = ( (data[2]) | (data[3] << 8) | (data[4] << 16) | (data[5] << 24) );
		high = ( (data[6]) | (data[7] << 8) | (data[8] << 16) | (data[9] << 24) );
        R( 0 != high ); // 表示后四个字节有数据int存放不了，太大了，我们不要了。
		payloadSize = low;
		headerTail = data + 2 + 8;
        */
    }
    packSize = headerTail - data + ( isUseMask ? 4 : 0 ) + wsDataSize;
    rawData = isUseMask ?  headerTail + 4 : headerTail;

    if( packSize > client->readBufSize ){
        R(on_tcp_server_client_pack_size_too_large(client,packSize,client->readBufSize));
    }

    if( packSize > dataSize ){
        goto reread;
    }
    if( wsDataSize > 0 ){
        dataUsed = 0;
        mask = headerTail;
        if( isUseMask ){
            R(MALLOC_TN(&buf,str_t,wsDataSize));
            i = 0;
            for(; i < wsDataSize; i++){
                buf[i] = rawData[i] ^ mask[ i % 4 ];
            }
            R(onWsMsgRecved(client,buf,wsDataSize,&dataUsed));
            R(FREE_TN(&buf,str_t,wsDataSize));
        }else{
            R(onWsMsgRecved(client,rawData,wsDataSize,&dataUsed));
        }
        R(dataUsed != wsDataSize);
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
    if( NULL != buf ) R(FREE_TN(&buf,str_t,wsDataSize));
    return ret;
}

