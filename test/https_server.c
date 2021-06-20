#include "net.h"
//#include <openssl/x509.h>
//#include <openssl/ssl.h>
//#include <openssl/err.h>

#define WS_PORT 443
#define WS_IP "192.168.10.188"
#define WS_BACKLOG 5
#define HTML_BODY "<html><head></head><body>hello</body></html>"
http_resource_t *resources = NULL;
#define HTML_DIR_PAGE "/mc/projects/libnet/test/html/pages"
#define HTML_DIR_CSS "/mc/projects/libnet/test/html/css"
#define HTML_DIR_JS "/mc/projects/libnet/test/html/js"

http_req_t http = {0};
// SSL *ssl = NULL;

int on_client_recved(tcp_server_client_t *client,const char *buf,unsigned int size,unsigned int *used){
    int ret = 0;

    printf("%s\n",buf);
    //SSL_set_fd(ssl,client->fd);
    //SSL_write(ssl,"hello https",strlen("hello https"));
    tcp_server_client_write(client,"test",strlen("test"));
    /*    
     *    while(1){
     char buf[1024] = {0};
     int size = SSL_read(ssl,buf,1024);
     buf[size] = '\0';
     printf("read : %s\n",buf);
     }
     */
    (*used) = http.size;
    return ret;
}

int on_client_closed(tcp_server_client_t *client){
    int ret = 0;
    printf("client[%u] closed!\n",client->fd);
    return ret;
}

int on_server_error(struct kevent *ev){
    int ret = 0;
    LOGE("[%d]\tfilter[%d/%d/%d]\tflags[%u]\tfflags[%u]\n",ev->ident,ev->filter, EVFILT_READ,EVFILT_WRITE, ev->flags,ev->fflags); 
    call_stack_print();
    mc_call_stack_clear();
    return ret;
}

int on_client_connected(tcp_server_client_t *client){
    int ret = 0;

    return ret;
}

int isExit = 0;
void on_signal(int sigNo){
    isExit = 1;
}

int main(int argc,char *argv[]){
    int ret = 0;
    signal(SIGTERM,on_signal);
    signal(SIGINT,on_signal);

  //  SSL_library_init();
  //  OpenSSL_add_ssl_algorithms();
  //  SSL_load_error_strings();

  //  SSL_METHOD *method = (SSL_METHOD *)SSLv23_server_method();
  //  SSL_CTX *ctx = SSL_CTX_new(method);
  //  if (!ctx) {
  //      printf("Error creating the context.\n");
  //      exit(0);
  //  }

  //  SSL_CTX_set_verify(ctx,SSL_VERIFY_NONE,NULL);
    //    SSL_CTX_load_verify_locations(ctx,ROOTCERTF,NULL);
  //  ssl = SSL_new(ctx);
  //  if(!ssl) {
  //      printf("Error creating SSL structure.\n");
  //      exit(0);
  //  }


    http_resource_config_t httpResourceConfig = {
        1024,32,32
    };

    RL(http_resource_init(&resources,&httpResourceConfig),"http_resource_init() failed!\n");
    RL(http_resource_load_from_dir(resources,HRT_PAGE,HTML_DIR_PAGE),"http_resource_load_from_dir() failed!\n");
    RL(http_resource_load_from_dir(resources,HRT_CSS,HTML_DIR_CSS),"http_resource_load_from_dir() failed!\n");
    RL(http_resource_load_from_dir(resources,HRT_JS,HTML_DIR_JS),"http_resource_load_from_dir() failed!\n");

    tcp_server_t *server = NULL;
    tcp_server_config_t config = {
        WS_IP,WS_PORT,WS_BACKLOG,
        10000,1024,1024,5,
        on_client_recved,
        on_client_closed,
        NULL,
        on_client_connected
    };
    RL(tcp_server_init(&server,&config),"tcp_server_init() failed!\n");
    while(1){
        RL(tcp_server_run(server,on_server_error),"tcp_server_run() failed!\n");
        if(isExit){
            R(tcp_server_exit(&server));
            R(http_resource_free(&resources));
            break;
        }
    }
    mem_show();
    return ret;
}
