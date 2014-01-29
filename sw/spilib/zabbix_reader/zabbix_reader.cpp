#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include <string.h>
#include <execinfo.h>
#include <time.h>
//#include <json.h>
#include "zabbix_log.h"



int main(int argc, char *argv[]) {
	if (argc < 2 ) {
		printf("Usage: %s <infile>\n" , argv[0]);
		return 0;
    }

	char *file_name = argv[1];
	
	
}


