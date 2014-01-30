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
		printf("Usage: %s <outfile>\n" , argv[0]);
		return 0;
    }


	zabbix_log.magic = MG_ZABBIX_LOG_MAGIC;
		zabbix_log.version = MG_ZABBIX_LOG_VERSION;
		zabbix_log.ac2dc_current = 0xbc;
		zabbix_log.ac2dc_temp = 0xbd;

		for(int l = 0; l < LOOP_COUNT ; l++) {
			zabbix_log.loops[l].voltage = 1;
			zabbix_log.loops[l].current = 1;
			zabbix_log.loops[l].temp = 1;
		}

		for(int l = 0; l < HAMMERS_COUNT ; l++) {
			zabbix_log.asics[l].freq = 1;
			zabbix_log.asics[l].temp = 1;
		}




	char *file_name = argv[1];
	FILE *fp = fopen(file_name, "w");

	if (fp != NULL)
    {
    	fwrite(&zabbix_log,1,sizeof(zabbix_log),fp); 
        fclose(fp);
    } else {
		printf("Failed to load zabbix log\n");
		return -1;		
    }
	return 0;
}


