#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

//#include "porting.hpp"

#include "msgdecoder.hpp"

class MsgDecoder;

class Request{
  public:
    Request();
    ~Request();

    MsgDecoder *getMsgDecoder();

    void setMsgDecoder(MsgDecoder * decoder);

    void setClientIP(const char *clientIP);

    const char *getClientIP();

    void setClientPort(const int port);

    const int getClientPort();

    void setServerIP(const char *ip);

    const char * getServerIP();

  private:
    MsgDecoder *mDecoder;
    char mClientIP[32];
    int mClientPort;
    char mServerIP[32];

};


#endif





