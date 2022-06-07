#pragma once

#include <unix.h>
#include <string>
#include <openssl/md5.h>

class Token{
public:
    Token(std::string username, std::string tokensalt)
    :_username(username),_tokensalt(tokensalt)
    {
        char timeStamp[40] = {0};
        time_t now = time(NULL);
        struct tm *pTm = localtime(&now);
        sprintf(timeStamp,"%4d-%02d-%02d %02d:%02d:%02d",
            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
        std::string tokenGen = _username + timeStamp + _tokensalt;
        unsigned char md[16];
        MD5((const unsigned char *)tokenGen.c_str(),tokenGen.length(),md);
        char frag[3] = {0};
        for(int i = 0;i < 16;++i){
            sprintf(frag,"%02x", md[i]);
            _token = _token + frag;
        }
        char timeDate[40] = {0};
        sprintf(timeDate, "%02d%02d%02d%02d", pTm->tm_mon+1, pTm->tm_mday, pTm->tm_hour, pTm->tm_min);
        _token = _token + timeDate;
    }
    
    std::string get_token() const
    {
        return _token;
    }

private:
    std::string _username;
    std::string _tokensalt;
    std::string _token;// MD5(username+timeStamp+tokensalt) + "mmddhhmm"
};