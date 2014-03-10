#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <execinfo.h>

#include <linux/types.h>
#include <time.h>
#include <syslog.h>
#include <spond_debug.h>

#define SIZE 100
void _pabort(const char *s) {
  int j, nptrs;
  void *buffer[SIZE];
  char **strings;
  psyslog("ERROR, ABORT, DYE!\n");
  psyslog("MINERGATE ERROR: ");
  if (s) {
    //perror(orig_buf);
    psyslog("%s :STACK:", s);
  }
  nptrs = backtrace(buffer, SIZE);
  strings = backtrace_symbols(buffer, nptrs);
  for (j = 0; j < nptrs; j++) {
    psyslog(" %s\n", strings[j]);
  }
  abort();
}

void _passert(int cond, const char *s) {
  if (!cond) {
    pabort(s);
  }
}

void start_stopper(struct timeval *tv) {gettimeofday(tv, NULL);}

void end_stopper(struct timeval *tv, const char *name) {
  int usec;
  struct timeval end;
  gettimeofday(&end, NULL);
  usec = (end.tv_sec - tv->tv_sec) * 1000000;
  usec += (end.tv_usec - tv->tv_usec);
  printf(YELLOW "%s took %d\n" RESET, name, usec);
}
