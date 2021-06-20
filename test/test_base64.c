#include "net.h"

int main(int argc,char *argv[]){
    int ret = 0;
    char *buf =NULL;
    unsigned int bufSize = 0;
    if(strcmp(argv[1],"-d") == 0) 
    { 
        RL(base64_decode(argv[2],strlen(argv[2]),&buf,&bufSize),"base64_decode() failed!\n");
    }
    else  
    {
        RL(base64_encode(argv[1],strlen(argv[1]),&buf,&bufSize),"base64_encode() failed!\n");
    }
    printf("[%d]%s\n",bufSize,buf);  
    R(FREE_TN(&buf,str_t,bufSize));

    return ret;
}
