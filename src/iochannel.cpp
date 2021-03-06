/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <string.h>
#include <assert.h>
#include <event.h>
#include <sys/uio.h>

#include "iochannel.hpp"
#include "buffer.hpp"
#include "utils.hpp"
#include "response.hpp"
#include "session.hpp"
#include "msgblock.hpp"

#include "eventcb.hpp"


IOChannel :: IOChannel()
{
}

IOChannel :: ~IOChannel()
{
}

evbuffer_t * IOChannel :: getEvBuffer(Buffer * buffer)
{
  return buffer->mBuffer;
}

int IOChannel :: transmit(Session * session)
{
  const static int MAX_IOV = 8;
  EventArg * eventArg = (EventArg*)session->getArg();

  ArrayList * outList = session->getOutList();
  size_t outOffset = session->getOutOffset();

  struct iovec iovArray[MAX_IOV];
  memset(iovArray, 0, sizeof(iovArray));

  int iovSize = 0;

  for(int i = 0; i<outList->getCount() && iovSize<MAX_IOV; i++) {
    Message * msg = (Message*)outList->getItem(i);

    if(outOffset >= msg->getMsg()->getSize()) {
      outOffset -= msg->getMsg()->getSize();
    } else {
      iovArray[iovSize].iov_base = (char*)msg->getMsg()->getBuffer() + outOffset;
      iovArray[iovSize++].iov_len = msg->getMsg()->getSize() - outOffset;
      outOffset = 0;
    }

    MsgBlockList * blockList = msg->getFollowBlockList();
    for(int j = 0; j < blockList->getCount() && iovSize < MAX_IOV; j++) {
      MsgBlock * block = (MsgBlock*)blockList->getItem(j);
      if(outOffset >= block->getSize()) {
        outOffset -= block->getSize();
      } else {
        iovArray[iovSize].iov_base = (char*)block->getData() + outOffset;
        iovArray[iovSize++].iov_len = block->getSize() - outOffset;
        outOffset = 0;
      }
    }
  }

  int len = write_vec( iovArray, iovSize );

  if( len > 0 ) {
    outOffset = session->getOutOffset() + len;

    for(; outList->getCount() > 0;) {
      Message * msg = (Message*)outList->getItem(0);
      if(outOffset >= msg->getTotalSize()) {
        msg = (Message*)outList->takeItem(0);
        outOffset = outOffset - msg->getTotalSize();
        int index = msg->getToList()->find(session->getSid());
        if(index >= 0)
            msg->getToList()->take( index );
        msg->getSuccess()->add(session->getSid());

        if(msg->getToList()->getCount() <= 0) {
          eventArg->getOutputResultQueue()->push(msg);
        }
    } else {
      break;
    }
  }

  session->setOutOffset( outOffset );
}

  if(len > 0 && outList->getCount() > 0) {
    int tmpLen = transmit( session );
    if(tmpLen > 0) 
        len += tmpLen;
  }

  return len;
}


//---------------------------------------------------------

IOChannelFactory :: ~IOChannelFactory()
{
}

//---------------------------------------------------------

DefaultIOChannel :: DefaultIOChannel()
{
  mFd = -1;
}

DefaultIOChannel :: ~DefaultIOChannel()
{
  mFd = -1;
}

int DefaultIOChannel :: init(int fd)
{
  mFd = fd;
  return 0;
}

int DefaultIOChannel :: receive(Session * session)
{
  return evbuffer_read(getEvBuffer( session->getInBuffer() ), mFd, -1);
}

int DefaultIOChannel :: write_vec(struct iovec * iovArray, int iovSize)
{
  return writev(mFd, iovArray, iovSize);
}

//---------------------------------------------------------

DefaultIOChannelFactory :: DefaultIOChannelFactory()
{
}

DefaultIOChannelFactory :: ~DefaultIOChannelFactory()
{
}

IOChannel * DefaultIOChannelFactory :: create() const
{
  return new DefaultIOChannel();
}




