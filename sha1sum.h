#pragma once

#include <string>
#include <unix.h>
#include <openssl/sha.h>

std::string sha1File(const char *path){
    int fd = open(path,O_RDONLY);
    char buf[4096];
    SHA_CTX sha_ctx;
    SHA1_Init(&sha_ctx);
    while(1){
        bzero(buf,sizeof(buf));
        int ret =read(fd,buf,sizeof(buf));
        if(ret == 0){
            break;
        }
        SHA1_Update(&sha_ctx,buf,ret);
    }
    unsigned char md[20];
    SHA1_Final(md,&sha_ctx);
    char frag[3] = {0};//"12" "ab" 
    std::string sha1Res;
    for(int i = 0; i < 20; ++i){
        sprintf(frag,"%02x",md[i]);
        sha1Res = sha1Res + frag;
    }
    return sha1Res;
}