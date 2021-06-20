#include "net.h"


int main(int argc,char *argv[]){
    int ret = 0;
    //char const string[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno";
    //char const expect[] = "7789f0c9ef7bfc40d93311143dfbe69e2017f592";
    char const string[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    char const expect[] = "84983e441c3bd26ebaae4aa1f95129e5e54670f1";
    //unsigned char result[21];
    char result[21];
    char hexresult[41];
    int iterator;
    size_t offset;
    SHA1_CTX ctx;

    SHA1( result, string, strlen(string) );

    /* calculate hash */
 //   SHA1Init(&ctx);
 //   for ( iterator = 0; iterator < 16777216; iterator++) {
 //       SHA1Update( &ctx, (unsigned char const *)string, strlen(string) );
 //   }
 //   SHA1Final(result, &ctx);
 //   printf("result === %s\n",result);

    /* format the hash for comparison */
    for( offset = 0; offset < 20; offset++) {
        sprintf( ( hexresult + (2*offset)), "%02x", result[offset]&0xff);
    }
    
    printf("%s\n",0 == strncmp(hexresult, expect, 40) ? "SUCCESS" : "FAILED");

    return ret;
}
