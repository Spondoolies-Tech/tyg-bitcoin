#ifndef _____ZABLOG_LALALR_H____
#define _____ZABLOG_LALALR_H____


#include "nvm.h"

#define MG_ZABBIX_LOG_MAGIC   0xfefa
#define MG_ZABBIX_LOG_VERSION 0x1



typedef struct mg_asic_log {
	uint8_t freq; // ASIC_FREQ
	uint8_t temp; // ASIC_TEMP
} __attribute__((__packed__ )) MG_ASIC_LOG ;



typedef struct mg_loop_log {
	uint8_t voltage; // DC2DC_VOLTAGE
	uint8_t current; // ???
	uint8_t temp;    // ???
} __attribute__((__packed__ )) MG_LOOP_LOG ;


typedef struct mg_zabbix_log {
	uint16_t magic;
	uint16_t version;	
	uint16_t reserved;
	uint16_t zabbix_logsize;
	uint32_t time;    // seconds from some hippie period 
	uint32_t dump_id; // autoincrement

	uint8_t ac2dc_current;
	uint8_t ac2dc_temp;

	MG_LOOP_LOG loops[LOOP_COUNT];
	MG_ASIC_LOG asics[HAMMERS_COUNT];


} __attribute__((__packed__ )) MG_ZABBIX_LOG;






#endif
