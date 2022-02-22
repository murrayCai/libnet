#include "net.h"
#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_TCP_SSL
#else
#define MODULE_CURR MODULE_NET_TCP_SSL
#endif

/* scd.19src.com  private key */
#define SSL_PRIVATE_KEY_BASE64 \
"MIIEowIBAAKCAQEAyarU5feJ1jnkSrUwfD2q1JX9MxbMAdzVYx1W4POGqYAsC+5b"\
"CnEcmBOxWaFsk8f07xYsx30eD895PqeqoC84Jb7+7Z7c1G7eEfxE0zhYyMGb2OmA"\
"ggNnqjPq0ySw5PMD+GACztRqdhNVDIGmJNtQ3mdoqoE44F+dqMBcnyWvDz5+CZ4x"\
"I6pFjVTY/0ACwyVce9v+RzhVdm1MHG5J9YdRh86+gwHGOdWoTb4t4rLD87zHSi+z"\
"SuKt92AdkzqzVsFw0YCuSNMA793HI77y7YElH7ByBeHc3afVD7ZbvH6yEnIl4No0"\
"4nYhjmLxN3Sj2k3N9auBfjn83MGQqFNYYb/b/wIDAQABAoIBAFh7xwvbz7OaFQue"\
"clag7Bp4cO8AmqRak5n4Xo027KfkX+8gNvve40/qnA8an7w7qMFadm1U/WKsgqM2"\
"B8xey4DsS4hRAs2Gk5NKn5wZhgMXx9yln0fsXIi2XNslpyPdaKmK3tCHLOr/0no5"\
"TH+xZssejjL5nc38xf0r4bfGXggOeNqJ3fToQna64oDtGDlGKr88Oj+8uGR4QDK0"\
"MJg3a80haIdyMYcPv8DySYdh/hKWVCJkx602hiOd4XJ2gkdSqrHSGpgyLNyFE5VU"\
"JTYrDKu+Lp2XqSrwNqd5AgTAmLnLD5LH4BN8ek+mVnBeAXT0bh2DUEAZDZjCfc8p"\
"ZxH9trECgYEA6qcofmwnUAi5uQ2+38WAY7/uiKiOLsewM1f/AGFolcTODwGbweym"\
"xqPmilpI9uPoLoZBTaif7+0EoFC9+bBNlTEknXOTGXUWDlZNw/binqYDVj101c5Q"\
"PQaRRoNdIT/JzvnRzAoUqfRn3wviYJv5nKnZHQ/uY2AtOLYALVc7i9kCgYEA3AN4"\
"KOdMw7Bp3Q7uiG9WriJKzQ3zLfsuZ9enBCszTsKFIrM4lgd2fQUfVC0ejO4IJ8O8"\
"c6EMU0bemapdjw1DOMWldzQg+bKMQBFq248jwdK9uhOiYBLdQVfSaDnh432limyk"\
"XrzCYocFWlkK+i/8ywxL6umxvn0D0b+16DIC95cCgYEAlGyzcnQa0ccTIviM2aJl"\
"ubK3wP1NIk3xKCjrBWSX8NQRuvA7g4ibXV5i/vRo3rH+NR96wxJ8SBjWKkmoQP3B"\
"tTLmhCmMzDeBggQZIHFTmyJLTguFkY/0tQUocz+4csWUczUV/UgLFg2gfjdYM9+1"\
"HeFZkcZ39afqMsr8hbF92tECgYBHmZlxNlT1GmRZXJKkNBexc9eEBBYv0J65jKT9"\
"5lE8QHGzaT/xE1ysMw6PBl/63TwqNLkPqEmqkFXBDICGHXrS7v96W6yflkuuTfzI"\
"lzy7xymXdOxS99CpgzsmUDADKNDGy7RyYFuiVO6ZCIFaCmPqcIYEXSlLJMQpbop0"\
"llIA4QKBgFWviuxP/pUpmpML0fW3FC+mTdRcDFWiYlPpgGLbVm7kzUzosyXV9ZA+"\
"bZcb0I2TDCwHBSvIWugt7ekLxIRValsQoULHYXgHLSVkpFrBxvLC+5eKUUTHh43a"\
"SkJGm6wUJYZHIIXWdjlYBat0oThW6LLAHeJe2e2pVDvutIar/W6U"

int ssl_info_print(){
    int ret = 0;
	printf("OPENSSL_VERSION_NUMBER:\n\t\t\t0x%lx\n", OPENSSL_VERSION_NUMBER);
	printf("OPENSSL_VERSION_TEXT:\n\t\t\t%s\n", OPENSSL_VERSION_TEXT);
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
	printf("SSLeay:\n\t\t\t0x%x\n", SSLeay());
	printf("SSLeay_version(SSLEAY_VERSION):\n\t\t\t%s\n", SSLeay_version(SSLEAY_VERSION));
	printf("SSLeay_version(SSLEAY_CFLAGS):\n\t\t\t%s\n", SSLeay_version(SSLEAY_CFLAGS));
	printf("SSLeay_version(SSLEAY_BUILT_ON):\n\t\t\t%s\n", SSLeay_version(SSLEAY_BUILT_ON));
	printf("SSLeay_version(SSLEAY_PLATFORM):\n\t\t\t%s\n", SSLeay_version(SSLEAY_PLATFORM));
	printf("SSLeay_version(SSLEAY_DIR):\n\t\t\t%s\n", SSLeay_version(SSLEAY_DIR));
#else
	printf("OpenSSL_version_num:\n\t\t\t0x%lx\n", OpenSSL_version_num());
	printf("OpenSSL_version(OPENSSL_VERSION):\n\t\t\t%s\n", OpenSSL_version(OPENSSL_VERSION));
	printf("OpenSSL_version(OPENSSL_CFLAGS):\n\t\t\t%s\n", OpenSSL_version(OPENSSL_CFLAGS));
	printf("OpenSSL_version(OPENSSL_BUILT_ON):\n\t\t\t%s\n", OpenSSL_version(OPENSSL_BUILT_ON));
	printf("OpenSSL_version(OPENSSL_PLATFORM):\n\t\t\t%s\n", OpenSSL_version(OPENSSL_PLATFORM));
	printf("OpenSSL_version(OPENSSL_DIR):\n\t\t\t%s\n", OpenSSL_version(OPENSSL_DIR));
	printf("OpenSSL_version(OPENSSL_ENGINES_DIR):\n\t\t\t%s\n", OpenSSL_version(OPENSSL_ENGINES_DIR));
#endif
    return ret;
}

int tcp_server_ssl_init(tcp_server_t *ts){
    int ret = 0;
    R(NULL == ts);
    tcp_server_config_t *config = ts->config;
    R(NULL == config);
    R(NULL == config->sslKeyPath);
    R(NULL == config->sslServerPath);
    /* R(NULL == config->sslChainPath); */
    R(access(config->sslServerPath,F_OK | R_OK));
    R(access(config->sslKeyPath,F_OK | R_OK));
    R(1 != SSL_library_init());
    SSL_load_error_strings();
    /* R(NULL == (ts->sslMethod = SSLv23_server_method())); */
    R(NULL == (ts->sslMethod = TLS_server_method() ));
    R(NULL == (ts->sslCtx = SSL_CTX_new(ts->sslMethod)));
    /* R(1 != SSL_CTX_use_certificate_file(ts->sslCtx, config->sslServerPath, SSL_FILETYPE_PEM));*/
    /* R(1 != SSL_CTX_use_certificate_ASN1(ts->sslCtx, strlen(SSL_PRIVATE_KEY), (const unsigned char *)SSL_PRIVATE_KEY)); */

    R(1 != SSL_CTX_use_certificate_chain_file(ts->sslCtx,config->sslServerPath));

    /*
    buf_t *asn1StrBuf = NULL;
    R(SSL_Base64_decode(SSL_PRIVATE_KEY_BASE64,strlen(SSL_PRIVATE_KEY_BASE64),&asn1StrBuf));
    R(1 != SSL_CTX_use_RSAPrivateKey_ASN1(ts->sslCtx,(const unsigned char *)asn1StrBuf->data,asn1StrBuf->used));
    */
    R(1 != SSL_CTX_use_RSAPrivateKey_file(ts->sslCtx,config->sslKeyPath,SSL_FILETYPE_PEM));
    R(1 != SSL_CTX_check_private_key(ts->sslCtx));
    int mode = 0;
	mode |= SSL_MODE_ENABLE_PARTIAL_WRITE;
	mode |= SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
	SSL_CTX_set_mode(ts->sslCtx, mode);
    SSL_CTX_set_security_level(ts->sslCtx,0);
/*    R(0 == SSL_CTX_set_cipher_list(ts->sslCtx, "ADH:@STRENGTH"));
    R(0 == SSL_CTX_set_cipher_list(ts->sslCtx, "ADH:@SECLEVEL=0")); */
    SSL_CTX_set_verify(ts->sslCtx, SSL_VERIFY_NONE, NULL);
/*    R(FREE_TN(&asn1Str,str_t,strlen(SSL_PRIVATE_KEY_BASE64))); */
/*    ssl_info_print(); */
    return ret;
}

int tcp_print_ssl_error(const char *str, size_t len, void *u){
    LOGE("SSL Error[%zu] : %s",len,str);
    return 0;
}

int tcp_server_client_ssl_hand_shake(tcp_server_client_t *client){
    int ret = 0;
    int fd = 0,sslRet = 0,sslErrno = 0;
    R(NULL == client);
    tcp_server_t *ts = client->server;
    R(NULL == ts);
    tcp_server_config_t *config = ts->config;
    R(NULL == config);
    R(NULL == ts->sslCtx);
    fd = client->fd;
    R(0 != client->isSslHandShaked);

    if(config->isUseSsl){
        if(NULL == client->ssl){
            R(NULL == (client->ssl = SSL_new(ts->sslCtx)));
            R(1 != SSL_set_fd(client->ssl,fd));
            SSL_set_security_level(client->ssl,0);
/*            R(0 == SSL_set_cipher_list(client->ssl, "ADH:@STRENGTH"));
            R(0 == SSL_set_cipher_list(client->ssl, "ADH:@SECLEVEL=0"));*/
            SSL_set_accept_state(client->ssl);
        }
/*        R(SET_FD_BLOCK(fd)); */
        if(1 != (sslRet = SSL_accept(client->ssl))){
            sslErrno = SSL_get_error(client->ssl,sslRet);
            switch(sslErrno){
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_X509_LOOKUP:
                    errno = 0;
                    return ret;
                case SSL_ERROR_SYSCALL:
                    R(tcp_client_statu_check((tcp_client_define_t *)client));
                    /* should take care the system call  */
                    break;
                case SSL_ERROR_ZERO_RETURN:
                    errno = 0;
                    TCP_CLIENT_SET_CLOSE(client);
                    break;
                case SSL_ERROR_SSL:
                    TCP_CLIENT_SET_CLOSE(client);
                default:
                    ERR_print_errors_cb(tcp_print_ssl_error,NULL);
                    LOGE("ssl_recv hand shake : %d,%d\n",sslErrno,sslRet);
                    R(1);
            }
        }else{
            client->isSslHandShaked = 1;
        }
    /*    R(SET_FD_NONBLOCK(fd)); */
    }
    return ret;
}

int tcp_ssl_recv(tcp_client_define_t *client,char *buf,unsigned int needReadSize,unsigned int *readedSize){
    int ret = 0,sslRet = 0;
    int sslErrno = 0;
    R(NULL == client);
    R(NULL == client->ssl);
    R(NULL == buf);
    R(0 >= needReadSize);

    sslRet = SSL_read(client->ssl,buf,needReadSize);
    if( 0 < sslRet ){
        *readedSize = sslRet;
    }else{
        sslErrno = SSL_get_error(client->ssl,sslRet);
        switch(sslErrno){
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_X509_LOOKUP:
                errno = 0;
                break;
            case SSL_ERROR_SYSCALL:
                R(tcp_client_statu_check(client));
                /* should take care the system call  */
                break;
            case SSL_ERROR_ZERO_RETURN:
                errno = 0;
                TCP_CLIENT_SET_CLOSE(client);
                break;
            case SSL_ERROR_SSL:
                TCP_CLIENT_SET_CLOSE(client);
            default:
                ERR_print_errors_cb(tcp_print_ssl_error,NULL);
                LOGE("ssl_recv error : %d,%d\n",sslErrno,sslRet);
                R(1);
        }
        *readedSize = 0;
    }
    client->sslEnableReadSize = SSL_pending(client->ssl);
    /* DP("ssl enable read size : %d\n",client->sslEnableReadSize); */
    /* DP("client readed size : %u/%d/%u\n",*readedSize,needReadSize,client->readBufUsed); */
    return ret;
}

int tcp_ssl_send(tcp_client_define_t *client,const char *buf,unsigned int bufSize,unsigned int *sendedSize){
    int ret = 0,sslRet = 0,sslErrno = 0;
    R(NULL == client);
    R(NULL == client->ssl);
    R(NULL == buf);
    R(0 >= bufSize);
    R(NULL == sendedSize);

    sslRet = SSL_write(client->ssl,buf,bufSize);
    if( 0 < sslRet ){
        *sendedSize = sslRet;
    }else{
        sslErrno = SSL_get_error(client->ssl,sslRet);
        switch(sslErrno){
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_X509_LOOKUP:
                errno = 0;
                break;
            case SSL_ERROR_SYSCALL:
                R(tcp_client_statu_check(client));
                /* should take care the system call  */
                break;
            case SSL_ERROR_ZERO_RETURN:
                errno = 0;
                TCP_CLIENT_SET_CLOSE(client);
                break;
            case SSL_ERROR_SSL:
                TCP_CLIENT_SET_CLOSE(client);
            default:
                ERR_print_errors_cb(tcp_print_ssl_error,NULL);
                LOGE("ssl_send error : %d,%d => %p[%u]\n",sslErrno,sslRet,buf,bufSize);
                R(1);
        }
        *sendedSize = 0;
    }
    return ret;
}

int tcp_ssl_send_file(tcp_client_define_t *client,int fd,off_t offset,size_t size,unsigned int *sendedSize){
    int ret = 0,sslRet = 0,sslErrno = 0;
    R(NULL == client);
    R(NULL == client->ssl);
    R(0 >= size);
    R(NULL == sendedSize);

/*    sslRet = SSL_sendfile(client->ssl,fd,offset,size,0); */
    if( 0 < sslRet ){
        *sendedSize = sslRet;
    }else{
        sslErrno = SSL_get_error(client->ssl,sslRet);
        switch(sslErrno){
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_X509_LOOKUP:
                errno = 0;
                break;
            case SSL_ERROR_SYSCALL:
                R(tcp_client_statu_check(client));
                /* should take care the system call  */
                break;
            case SSL_ERROR_ZERO_RETURN:
                errno = 0;
                TCP_CLIENT_SET_CLOSE(client);
                break;
            case SSL_ERROR_SSL:
                TCP_CLIENT_SET_CLOSE(client);
            default:
                ERR_print_errors_cb(tcp_print_ssl_error,NULL);
                LOGE("ssl_recv error : %d,%d\n",sslErrno,sslRet);
                R(1);
        }
        *sendedSize = 0;
    }
    return ret;
}
