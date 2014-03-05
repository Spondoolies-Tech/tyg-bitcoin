#include "squid.h"
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
#include "hammer.h"
#include <time.h>
#include <mg_proto_parser.h>
#include <spond_debug.h>
#include <json.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "zabbix_log.h"

#if 1
MG_ZABBIX_LOG zabbix_log;

void update_zabbix_stats() {

  zabbix_log.magic = MG_ZABBIX_LOG_MAGIC;
  zabbix_log.version = MG_ZABBIX_LOG_VERSION;
  zabbix_log.zabbix_logsize = sizeof(zabbix_log);
  zabbix_log.ac2dc_current = vm.ac2dc_current;
  zabbix_log.ac2dc_temp = vm.ac2dc_temp;

  for (int l = 0; l < LOOP_COUNT; l++) {
    zabbix_log.loops[l].current = vm.loop[l].dc2dc.dc_current_16s;
    zabbix_log.loops[l].temp = vm.loop[l].dc2dc.dc_temp;
    zabbix_log.loops[l].enabled = vm.loop[l].enabled_loop;
  }

  int t = time(NULL);
  for (int l = 0; l < HAMMERS_COUNT; l++) {
    zabbix_log.asics[l].freq = vm.hammer[l].asic_freq;
    zabbix_log.asics[l].temp = vm.hammer[l].asic_temp;
    zabbix_log.asics[l].working_engines = vm.working_engines[l];
    zabbix_log.asics[l].failed_bists = vm.hammer[l].failed_bists;
    zabbix_log.asics[l].freq_time = t - vm.hammer[l].last_freq_change_time;
  }
}

void dump_zabbix_stats() {
  FILE *fp = fopen("/my_nfs/zabbix_log.bin", "w");
  if (fp != NULL) {
    update_zabbix_stats();
    fwrite(&zabbix_log, 1, sizeof(zabbix_log), fp);
    fclose(fp);
  } else {
    printf("Failed to save zabbix log\n");
  }
}

#else
#define MAX_ZABBIX_LOG 200000
#define ZABBIX_VERSION 1

#define zabbix_add(PARAM...)                                                   \
  {                                                                            \
    buf_ptr += sprintf(buf + buf_ptr, PARAM);                                  \
    passert(buf_ptr < MAX_ZABBIX_LOG);                                         \
  }

int buf_ptr = 0;
char buf[MAX_ZABBIX_LOG + 100];
int log_id;

/*
void zabbix_add_int(const char* name, int var) {
  buf_ptr += sprintf(buf+buf_ptr,"%s %d\n",name,var);
  passert(buf_ptr < MAX_ZABBIX_LOG);
}

static void zabbix_add_char(const char* name, char* var) {
  buf_ptr += sprintf(buf,"%s %s\n",name,var);
  passert(buf_ptr < MAX_ZABBIX_LOG);
}
*/

static void zabbix_dump_file(const char *file_name) {
  /*
   // printf("ZAB_LOG:\n%s",buf);
      FILE *fp = fopen(file_name, "w");
      if (fp != NULL)
      {
          fputs(buf, fp);
          fclose(fp);
      }
  
  
      
    buf_ptr = 0;
    passert(dmp == NULL);
  */
}
extern int last_alive_jobs;
void send_keep_alive() {
  static int i = 0;
  FILE *f = fopen("/mnt/alive", "w");
  passert((int)f, "Failed to create alive file");
  fprintf(f, "id:%d \njobs:%d \n", i, last_alive_jobs);
  last_alive_jobs = 0;
  i++;
  fclose(f);
}

void dump_zabbix_stats() {
  JsonNode *dmp = json_mkobject();
  JsonNode *loops = json_mkarray();
  send_keep_alive();
  json_append_member(dmp, "dump_ver", json_mknumber(ZABBIX_VERSION));
  json_append_member(dmp, "log_id", json_mknumber(log_id++));
  json_append_member(dmp, "ac2dc_top_current",
                     json_mknumber(vm.ac2dc_top_current));
  json_append_member(dmp, "ac2dc_temp", json_mknumber(vm.ac2dc_temperature));

  json_append_member(dmp, "loops", loops);
  // TODO - update this hack!
  int h, l;
  for (l = 0; l < LOOP_COUNT; l++) {
    if (vm.loop[l].enabled || 1) {
      JsonNode *loop = json_mkobject();
      json_append_element(loops, loop);
      json_append_member(loop, "loop_id", json_mknumber(l));
      json_append_member(loop, "loop_enbl", json_mknumber(vm.loop[l].enabled));
      //           json_append_member(loop, "loop_brok",
      // json_mknumber(vm.loop[l].broken));
      json_append_member(
          loop, "loop_current",
          json_mknumber(vm.loop[l].dc2dc.dc_current_16s));
      json_append_member(loop, "loop_volt", json_mknumber(nvm.loop_voltage[l]));

      JsonNode *asic_address = json_mkarray();
      JsonNode *asic_enabled = json_mkarray();
      JsonNode *asic_corner = json_mkarray();
      JsonNode *asic_freq = json_mkarray();
      JsonNode *asic_max_freq = json_mkarray();
      JsonNode *asic_engines_on = json_mkarray();
      JsonNode *asic_engins_working = json_mkarray();
      JsonNode *asic_temp = json_mkarray();
      JsonNode *asic_wins = json_mkarray();

      json_append_member(loop, "asic_address", asic_address);
      json_append_member(loop, "asic_enabled", asic_enabled);
      json_append_member(loop, "asic_corner", asic_corner);
      json_append_member(loop, "asic_freq", asic_freq);
      json_append_member(loop, "asic_max_freq", asic_max_freq);
      json_append_member(loop, "asic_engines_on", asic_engines_on);
      json_append_member(loop, "asic_engins_working", asic_engins_working);
      json_append_member(loop, "asic_temp", asic_temp);
      json_append_member(loop, "asic_wins", asic_wins);

      // vm.loop[l].dc2dc.dc_current_16s = 0;

      for (h = 0; h < HAMMERS_PER_LOOP; h++) {
        HAMMER *ham = &vm.hammer[l * HAMMERS_PER_LOOP + h];

        json_append_element(asic_address, json_mknumber(ham->address));
        json_append_element(asic_enabled, json_mknumber(ham->present));
        json_append_element(asic_corner,
                            json_mknumber(nvm.asic_corner[ham->address]));
        json_append_element(asic_freq, json_mknumber(ham->freq));
        json_append_element(asic_max_freq,
                            json_mknumber(ham->max_freq));
        json_append_element(asic_engines_on,
                            json_mknumber(ham->enabled_engines_mask));
        json_append_element(asic_temp, json_mknumber(ham->asic_temp));
        json_append_element(asic_wins, json_mknumber(ham->solved_jobs));

        // vm.loop[l].dc2dc.dc_current_16s += ham->freq;
        // vm.ac2dc_top_current += ham->freq;
      }
    }
  }

  char *p = json_stringify(dmp, NULL);
  // printf("%<<<<\n%s", p);
  FILE *fp = fopen("./zabbix_log", "w");
  if (fp != NULL) {
    fputs(p, fp);
    fclose(fp);
  }
  free(p);
  json_delete(dmp);
  dmp = NULL;
}

void create_zabbix_log();
#endif
