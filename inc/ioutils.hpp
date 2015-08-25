#ifndef __IOUTILS_HPP__
#define __IOUTILS_HPP__

#include <netinet/in.h>

class IOUtils {
  public:
    static void inetNtoa(in_addr * addr, char * ip, int size);

    static int setNonblock(int fd);

    static int setBlock(int fd);

    static int tcpListen(const char * ip, int port, int * fd, int blocking = 1);

    static int initDaemon(const char * workdir = 0);

    static int tcpListen(const char * path, int * fd, int blocking = 1, int mode = 0666);

  private:
    IOUtils();
};

#endif

