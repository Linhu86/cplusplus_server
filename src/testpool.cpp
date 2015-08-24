/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include "threadpool.hpp"

extern int errno;

void threadFunc(void *arg)
{
  int seconds = *((int *) arg);

  fprintf(stdout, "  in threadFunc %d\n", seconds);
  fprintf(stdout, "  thread#%ld\n", pthread_self());
  sleep(seconds);
  fprintf(stdout, "  done threadFunc %d\n", seconds);
}

int main(int argc, char ** argv)
{
  ThreadPool pool(1);

  int arg1 = 3;
  int arg2 = 6;
  int arg3 = 7;

  fprintf(stdout, "**main** dispatch 3\n");
  pool.dispatch(threadFunc, (void*)&arg1);
  fprintf(stdout, "**main** dispatch 6\n");
  pool.dispatch(threadFunc, (void*)&arg2);
  fprintf(stdout, "**main** dispatch 7\n");
  pool.dispatch(threadFunc, (void*)&arg3);

  fprintf(stdout, "**main** done first\n");
  sleep(20);

  return 0;
}

