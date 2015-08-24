#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <event.h>

#include "buffer.hpp"

Buffer :: Buffer()
{
  mBuffer = evbuffer_new();
}

Buffer :: ~Buffer()
{
  evbuffer_free( mBuffer );
  mBuffer = NULL;
}

int Buffer :: append(const void * buffer, int len)
{
  len = len <= 0 ? strlen( (char*)buffer ) : len;
  return evbuffer_add(mBuffer, (void*)buffer, len);
}

int Buffer :: append(const Buffer * buffer)
{
  if(buffer->getSize() > 0) {
    return append(buffer->getBuffer(), buffer->getSize());
  } else {
    return 0;
  }
}

int Buffer :: printf(const char *fmt, ...)
{
  int ret = 0;

  if( NULL != strchr( fmt, '%' ) ) {
    va_list args;
    va_start(args, fmt);
    ret = evbuffer_add_vprintf(mBuffer, fmt, args);
    va_end(args);
  } else {
    ret = append( fmt );
  }

  return ret;
}

void Buffer :: erase(int len)
{
  evbuffer_drain( mBuffer, len );
}

void Buffer :: reset()
{
  erase(getSize());
}

int Buffer :: truncate(int len)
{
  if( len < (int)getSize() ) {
    EVBUFFER_LENGTH( mBuffer ) = len;
    return 0;
  }

  return -1;
}

void Buffer :: reserve(int len)
{
  evbuffer_expand(mBuffer, len - getSize());
}

int Buffer :: getCapacity()
{
  return mBuffer->totallen;
}

const void *Buffer :: getBuffer() const
{
  if( NULL != EVBUFFER_DATA( mBuffer ) ) {
    evbuffer_expand(mBuffer, 1);
    ((char*)(EVBUFFER_DATA(mBuffer)))[getSize()] = '\0';
    return EVBUFFER_DATA(mBuffer);
    } else {
      return "";
    }
}

const void * Buffer :: getRawBuffer() const
{
  return EVBUFFER_DATA(mBuffer);
}

size_t Buffer :: getSize() const
{
  return EVBUFFER_LENGTH(mBuffer);
}

char * Buffer :: getLine()
{
  return sp_evbuffer_readline(mBuffer);
}

int Buffer :: take(char * buffer, int len)
{
  len = sp_evbuffer_remove( mBuffer, buffer, len - 1);
  buffer[len] = '\0';
  return len;
}

Buffer * Buffer :: take()
{
  Buffer * ret = new Buffer();
  evbuffer_t * tmp = ret->mBuffer;
  ret->mBuffer = mBuffer;
  mBuffer = tmp;
  return ret;
}

const void * Buffer :: find(const void * key, size_t len)
{
  //return (void*)evbuffer_find( mBuffer, (u_char*)key, len );
  evbuffer_t * buffer = mBuffer;
  u_char * what = (u_char*)key;

  size_t remain = buffer->off;
  u_char *search = buffer->buffer;
  u_char *p;

  while (remain >= len) {
    if ((p = (u_char*)memchr(search, *what, (remain - len) + 1)) == NULL)
      break;

    if (memcmp(p, what, len) == 0)
      return (p);

    search = p + 1;
    remain = buffer->off - (size_t)(search - buffer->buffer);
  }

  return (NULL);
}




