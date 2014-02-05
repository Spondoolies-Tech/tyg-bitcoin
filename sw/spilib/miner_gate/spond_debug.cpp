#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <execinfo.h>

#include <linux/types.h>
#include <time.h>
#include <syslog.h>
#include <spond_debug.h>




#define SIZE 100
void _pabort(const char *s)
{
	int j, nptrs;
    void *buffer[SIZE];
    char **strings;
	char *buf = (char*)malloc(4000);
	char *orig_buf = buf;
	printf("ERROR, ABORT, DYE!\n");
	buf += sprintf(buf, "MINERGATE ERROR: ");
	if (s) {
    	perror(orig_buf);
		buf += sprintf(buf,"%s :STACK:", s);
	}
    nptrs = backtrace(buffer, SIZE);
    strings = backtrace_symbols(buffer, nptrs);
	for (j = 0; j < nptrs; j++) {
		buf += sprintf(buf," %s\n", strings[j]);
	}
	printf("%s\n",orig_buf);
	syslog (LOG_ALERT, "%s\n",orig_buf);
    abort();
}

void _passert(int cond, const char *s)
{
	if (!cond) {
		 pabort(s);
	}
}



