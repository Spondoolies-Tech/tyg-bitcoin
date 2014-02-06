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

#if 0
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
#endif


	if (zabbix_log.magic != MG_ZABBIX_LOG_MAGIC) {
		printf("zabbix log MAGIC wrong, %x instead of %x\n", zabbix_log.magic , MG_ZABBIX_LOG_MAGIC);
		return -1;
	}

	if (zabbix_log.version != MG_ZABBIX_LOG_VERSION) {
		printf("zabbix log VERSION wrong, %x instead of %x\n", zabbix_log.version , MG_ZABBIX_LOG_VERSION);
		return -1;
	}


	printf("ac2dc_current %x\n", zabbix_log.ac2dc_current);
	printf("ac2dc_temp %x\n", zabbix_log.ac2dc_temp);
	for(int l = 0; l < LOOP_COUNT ; l++) {
		printf("loop%x.voltage %x\n", l, zabbix_log.loops[l].voltage);
		printf("loop%x.current %x\n", l, zabbix_log.loops[l].current);
		printf("loop%x.temp %x\n", l, zabbix_log.loops[l].temp);
		for(int a = 0; a < (HAMMERS_COUNT/LOOP_COUNT); a++){
			printf("loop%x.asic%x.freq %x\n", l, a, zabbix_log.loops[l].asics[a].freq);
			printf("loop%x.asic%x.temp %x\n", l, a, zabbix_log.loops[l].asics[a].temp);
			printf("loop%x.asic%x.wins %x\n", l, a, zabbix_log.loops[l].asics[a].wins);
		}
	}

	
	
}


