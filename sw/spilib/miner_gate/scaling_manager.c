#include "squid.h"
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include "queue.h"
#include "pll.h"
#include "spond_debug.h"
#include "hammer.h"
#include <sys/time.h>
#include "nvm.h"
#include "ac2dc.h"
#include "dc2dc.h"
#include "hammer_lib.h"
#include "defines.h"

#include "scaling_manager.h"
#include "corner_discovery.h"

/*
       Supported temp sensor settings:
   temp    83 / 89 / 95 / 101 / 107 / 113 /119 / 125
   T[2:0]  0    1     2    3     4     5    6     7 
*/




void send_keep_alive();

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
void print_state();
void print_scaling();

MINER_BOX vm = { 0 };
extern SPONDOOLIES_NVM nvm;
extern int assert_serial_failures;

int test_asic(int addr) {
  assert_serial_failures = false;
  int ver = read_reg_device(addr, ADDR_VERSION);
  assert_serial_failures = true;
  // psyslog("Testing loop, version = %x\n", ver);
  return ((ver & 0xFF) == 0x3c);
}



void pause_all_mining_engines() {
  int err;
  //passert(vm.asics_shut_down_powersave == 0);
  int some_asics_busy = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY);
  set_fan_level(50);
  while(some_asics_busy != 0) {
    int addr = BROADCAST_READ_ADDR(some_asics_busy);
    psyslog(RED "some_asics_busy %x\n" RESET, some_asics_busy);
    disable_asic_forever(addr);
    some_asics_busy = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY);
  }
  // stop all ASICs
  disable_engines_all_asics();
  // disable_engines_all_asics();
  vm.asics_shut_down_powersave = 1;
  psyslog("System has no jobs, going to sleep...\n");
}

void unpause_all_mining_engines() {
  int err;
  psyslog("Got mining request, enable DC2DC!\n");
  set_fan_level(100);  
  vm.not_mining_counter = 0;
  enable_good_engines_all_asics_ok();
  psyslog("Got mining request, waking up done!\n");
  vm.asics_shut_down_powersave = 0;
}


int test_serial(int loopid) {
  /*
  if (loopid < 20 && loopid!= -1) {
    return 0;
  }
  */
  assert_serial_failures = false;
  // psyslog("Testing loops: %x\n", (~read_spi(ADDR_SQUID_LOOP_BYPASS))&
  // 0xFFFFFF);
  int i = 0;
  int val1;
  int val2;
  // This psyslogs used in tests! Don`t remove!!
  psyslog("Testing loop %d:\n", loopid);
  // For benny - TODO remove
  /*psyslog("FAKING IT :)\n");
  for (int j = 0 ; j < 1000 ; j++) {
    write_reg_broadcast(ADDR_VERSION, 0xAAAA);
  }*/

  while ((val1 = read_spi(ADDR_SQUID_STATUS)) &
         BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
    val2 = read_spi(ADDR_SQUID_SERIAL_READ);
    i++;
    if (i > 700) {
      // parse_squid_status(val1);
      // psyslog("ERR (%d) loop is too noisy: ADDR_SQUID_STATUS=%x,
      // ADDR_SQUID_SERIAL_READ=%x\n",i, val1, val2);
      // This psyslogs used in tests! Don`t remove!!
      psyslog("NOISE\n");
      return 0;
    }
    usleep(10);
  }

  // psyslog("Ok %d!\n", i);

  // test for connectivity
  int ver = read_reg_broadcast(ADDR_VERSION);

  // printf("XTesting loop, version = %x\n", ver);
  if (BROADCAST_READ_DATA(ver) != 0x3c) {
    // printf("Failed: Got version: %x!\n", ver);
    psyslog("BAD DATA (%x)\n",ver);
    return 0;
  }

  psyslog("OK\n");
  assert_serial_failures = true;
  return 1;
}

int enable_good_loops_ok() {
  write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
  write_spi(ADDR_SQUID_LOOP_RESET, 0);
  usleep(10000);
  write_spi(ADDR_SQUID_COMMAND, 0xF);
  write_spi(ADDR_SQUID_COMMAND, 0x0);
  write_spi(ADDR_SQUID_LOOP_BYPASS, ~(vm.good_loops));
  write_spi(ADDR_SQUID_LOOP_RESET, 0xFFFFFF); 
  int ok = test_serial(-1);
  if (!ok) {
    return 0;
  }
  for (int i = 0; i < LOOP_COUNT; i++) {
    psyslog("%sloop %d enabled = %d%s\n",
           (!vm.loop[i].enabled_loop) ? RED : RESET, i,
           (vm.loop[i].enabled_loop), RESET);
   
  }

  return 1;
}




//int failed_tests[]
void reset_all_asics_full_reset() {
  passert(read_reg_broadcast(ADDR_BR_THERMAL_VIOLTION) == 0);
  discover_good_loops();
  enable_good_loops_ok();
  init_hammers();             
  allocate_addresses_to_devices();
  vm.bist_fatal_err = 0;
  write_reg_broadcast(ADDR_LEADER_ENGINE, 0);
}


int count_ones(int failed_engines) {
  int c;
  for (c = 0; failed_engines; failed_engines >>= 1)
  {
    c += failed_engines & 1;
  }
  return c;
}



void print_miner_box() { DBG(DBG_SCALING, "MINER AC:%x\n", vm.ac2dc_power); }


extern int rt_queue_size;
extern int rt_queue_sw_write;
extern int rt_queue_hw_done;

void print_adapter(FILE *f ) ;

void print_scaling() {
  int err;
  
  FILE *f = fopen("/var/log/asics", "w");
  if (!f) {
    psyslog("Failed to save ASIC state\n");
    return;
  }

  //hammer_iter_init(&hi);
  int total_hash_power=0;
  int theoretical_hash_power=0; 
  int wanted_hash_power=0;
  int total_loops=0;
  int total_asics=0;
  int expected_rate=0;
  fprintf(f, GREEN "\n----------\nAC2DC power=%d[%d], temp=%d\n" RESET,
      vm.ac2dc_power,
      AC2DC_POWER_LIMIT,
      vm.ac2dc_temp
    );
  int total_watt=0;
  fprintf(f, GREEN "L |Vtrm|vlt|Wt|"  "A /Li|Tmp/"  "Tmp|A|H(H-)" RESET); 
  for (int addr = 0; addr < HAMMERS_COUNT ; addr++) {
    
    hammer_iter hi;    
    hi.addr = addr;
    hi.a = &vm.hammer[addr];
    hi.l = addr/HAMMERS_PER_LOOP;
    hi.h = addr%HAMMERS_PER_LOOP;
    if (hi.h == HAMMERS_PER_LOOP/2) {fprintf(f, "\n-------------------------------------------------");}
    if (hi.h == 0) {
      total_loops++;
      if (!vm.loop[hi.l].enabled_loop) {
        fprintf(f, "\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      } else {    
        DC2DC* dc2dc = &vm.loop[hi.l].dc2dc;

        fprintf(f, GREEN 
          "\n%2d|%4x(M:%4x)|%3d|%2d|"  
          "%s%3d%s/%3d|%s%3d%s|"   
          "%3d|%3d|%2d(%2d)" RESET, 
          hi.l, 
          vm.loop_vtrim[hi.l]&0xffff,
          vm.loop[hi.l].dc2dc.max_vtrim_currentwise&0xffff,
          VTRIM_TO_VOLTAGE_MILLI(vm.loop_vtrim[hi.l]),
          dc2dc->dc_power_watts_16s/16,
          
        ((dc2dc->dc_current_16s>=DC2DC_INITIAL_CURRENT_16S - 1*16)?RED:GREEN), dc2dc->dc_current_16s,GREEN,
          dc2dc->dc_current_limit_16s,
        ((dc2dc->dc_temp>=DC2DC_TEMP_GREEN_LINE)?RED:GREEN), dc2dc->dc_temp,GREEN,
        
          (vm.loop[hi.l].asic_count) ? vm.loop[hi.l].asic_temp_sum/vm.loop[hi.l].asic_count : 0,
          vm.loop[hi.l].crit_temp,
          vm.loop[hi.l].asic_hz_sum*15/1000,
          vm.loop[hi.l].overheating_asics
          );

        total_watt+=dc2dc->dc_power_watts_16s;
      }
    }
    
    if (!hi.a->asic_present) {
      fprintf(f, "|xxxxxxxxxxxxxxxxxxxxxxxxxxx");           
      continue;
    }

    total_hash_power += hi.a->freq_hw*15+210;  
    wanted_hash_power += hi.a->freq_wanted*15+210;
    theoretical_hash_power += hi.a->freq_thermal_limit*15+210;

    total_asics++;

    fprintf(f, GREEN "|%2x:%s%3dc%s %s%3dhz%s(%3d/%3d) %s%x" RESET, 
      hi.addr,
      (hi.a->asic_temp>=MAX_ASIC_TEMPERATURE-1)?((hi.a->asic_temp>=MAX_ASIC_TEMPERATURE)?RED:YELLOW):GREEN,((hi.a->asic_temp*6)+77),GREEN,
    corner_to_collor(hi.a->corner),hi.a->freq_wanted*15+210,GREEN,
       hi.a->freq_thermal_limit*15+210,
       hi.a->freq_bist_limit*15+210, 
       (vm.hammer[hi.addr].working_engines!=0x7FFF)?GREEN_BOLD:GREEN, vm.hammer[hi.addr].working_engines);
  }
  // print last loop
  // print total hash power
  fprintf(f, RESET "\n[H:HW:%dGh/Th:%dGh/W:%dGh,W:%d,L:%d,A:%d,ER:%d,EP:%d]\n",
                (total_hash_power*15)/1000,
                theoretical_hash_power*15/1000,
                wanted_hash_power*15/1000,
                total_watt/16,
                total_loops,
                total_asics,
                (total_hash_power*15*192)/1000/total_asics,
                (vm.ac2dc_power-70)/total_asics*192+70
  );

   fprintf(f, "Pushed %d jobs , in queue %d jobs!\n",
             vm.last_second_jobs, rt_queue_size);
   vm.last_second_jobs = 0;
    fprintf(f, "wins:%d, leading-zeroes:%d idle:%d/%d :)\n", vm.solved_jobs_total,
           vm.cur_leading_zeroes, vm.idle_probs, vm.busy_probs);
    print_adapter(f);
  //   fprintf("Adapter queues: rsp=%d, req=%d\n", 
  //          a->work_minergate_rsp.size(),
  //         a->work_minergate_req.size());

  fclose(f);
}
