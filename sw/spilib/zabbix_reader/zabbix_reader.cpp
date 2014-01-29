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


MG_ZABBIX_LOG zabbix_log;

int main(int argc, char *argv[]) {
	if (argc < 2 ) {
		printf("Usage: %s <infile>\n" , argv[0]);
		return 0;
    }

	char *file_name = argv[1];
	FILE *fp = fopen(file_name, "r");

	if (fp != NULL)
    {
    	fread(&zabbix_log,1,sizeof(zabbix_log),fp); 
        fclose(fp);
    } else {
		printf("Failed to load zabbix log\n");
		return -1;		
    }

	if (zabbix_log.magic != MG_ZABBIX_LOG_MAGIC) {
		printf("zabbix log MAGIC wrong, %x instead of %x\n", zabbix_log.magic , MG_ZABBIX_LOG_MAGIC);
		return -1;
	}

	if (zabbix_log.version != MG_ZABBIX_LOG_VERSION) {
		printf("zabbix log VERSION wrong, %x instead of %x\n", zabbix_log.version , MG_ZABBIX_LOG_VERSION);
		return -1;
	}

	
}


