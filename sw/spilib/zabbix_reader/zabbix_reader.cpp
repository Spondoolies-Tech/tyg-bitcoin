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


	printf("ac2dc.current %d\n", zabbix_log.ac2dc_current);
	printf("ac2dc.temp %d\n", zabbix_log.ac2dc_temp);

	for(int l = 0; l < LOOP_COUNT ; l++) {
		printf("loop%d.voltage %d\n", l, zabbix_log.loops[l].voltage);
		printf("loop%d.current %d\n", l, zabbix_log.loops[l].current);
		printf("loop%d.temp %d\n", l, zabbix_log.loops[l].temp);
		printf("loop%d.enabled %x\n", l, zabbix_log.loops[l].enabled);
		for(int a = 0; a < (HAMMERS_PER_LOOP); a++){
			printf("loop%d.asic%d.freq %d\n", l, a, zabbix_log.asics[a*l].freq);
			printf("loop%d.asic%d.working_engines %d\n", l, a, zabbix_log.asics[a*l].working_engines);
			printf("loop%d.asic%d.failed_bists %x\n", l, a, zabbix_log.asics[a*l].failed_bists);
			printf("loop%d.asic%d.freq_time %d\n", l, a, zabbix_log.asics[a*l].freq_time);
			printf("loop%d.asic%d.temp %d\n", l, a, zabbix_log.asics[a*l].temp);
			printf("loop%d.asic%d.wins %x\n", l, a, zabbix_log.asics[a*l].wins);
		}
	}
	
}


