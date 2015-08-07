#include <string.h>
#include <stdio.h>

#include "request.hpp"
#include "msgdecoder.hpp"
#include "utils.hpp"

Request :: Request()
{
  mDecoder = new DefaultMsgDecoder();
  memset(mClientIP, 0, sizeof(mClientIP));
  mClientPort = 0;
  memset(mServerIP, 0, sizeof(mServerIP));
}

Request :: ~Request()
{
  if(NULL != mDecoder) delete mDecoder;
    mDecoder = NULL;
}

MsgDecoder * Request :: getMsgDecoder()
{
  return mDecoder;
}

void Request :: setMsgDecoder(MsgDecoder * decoder)
{
  if(NULL != mDecoder)
    delete mDecoder;

  mDecoder = decoder;
}

void Request :: setClientIP(const char * clientIP)
{
  strlcpy(mClientIP, clientIP, sizeof(mClientIP));
}

const char *Request :: getClientIP()
{
  return mClientIP;
}

void Request :: setClientPort(int port)
{
  mClientPort = port;
}

int Request :: getClientPort()
{
  return mClientPort;
}

void Request :: setServerIP(const char * ip)
{
  strlcpy(mServerIP, ip, sizeof(mServerIP));
}

const char * Request :: getServerIP()
{
  return mServerIP;
}





