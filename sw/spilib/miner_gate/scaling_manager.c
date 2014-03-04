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
  // printf("Testing loop, version = %x\n", ver);
  return ((ver & 0xFF) == 0x3c);
}



void pause_all_mining_engines() {
  passert(vm.asics_shut_down_powersave == 0);
  int some_asics_busy = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY);
  
  if(some_asics_busy != 0) {
    printf(RED "some_asics_busy %x\n" RESET, some_asics_busy);
    passert(0);
  }
  // stop all ASICs
  disable_engines_all_asics();
  // disable_engines_all_asics();
  vm.asics_shut_down_powersave = 1;
  printf("PAUSING ALL MINING!!\n");
}

void unpause_all_mining_engines() {
  passert(vm.asics_shut_down_powersave != 0);
  vm.not_mining_counter = 0;
  enable_nvm_engines_all_asics_ok();
  printf("STARTING ALL MINING!!\n");
  vm.asics_shut_down_powersave = 0;
}


int test_serial(int loopid) {
  assert_serial_failures = false;
  // printf("Testing loops: %x\n", (~read_spi(ADDR_SQUID_LOOP_BYPASS))&
  // 0xFFFFFF);
  int i = 0;
  int val1;
  int val2;
  // This printfs used in tests! Don`t remove!!
  printf("Testing loop %d:\n", loopid);
  // For benny - TODO remove
  /*printf("FAKING IT :)\n");
  for (int j = 0 ; j < 1000 ; j++) {
    write_reg_broadcast(ADDR_VERSION, 0xAAAA);
  }*/

  while ((val1 = read_spi(ADDR_SQUID_STATUS)) &
         BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
    val2 = read_spi(ADDR_SQUID_SERIAL_READ);
    i++;
    if (i > 700) {
      // parse_squid_status(val1);
      // printf("ERR (%d) loop is too noisy: ADDR_SQUID_STATUS=%x,
      // ADDR_SQUID_SERIAL_READ=%x\n",i, val1, val2);
      // This printfs used in tests! Don`t remove!!
      printf("NOISE\n");
      return 0;
    }
    usleep(10);
  }

  // printf("Ok %d!\n", i);

  // test for connectivity
  int ver = read_reg_broadcast(ADDR_VERSION);

  // printf("XTesting loop, version = %x\n", ver);
  if (BROADCAST_READ_DATA(ver) != 0x3c) {
    // printf("Failed: Got version: %x!\n", ver);
    printf("BAD DATA (%x)\n",ver);
    return 0;
  }

  printf("OK\n");
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
    printf("%sloop %d enabled = %d%s\n",
           (!vm.loop[i].enabled_loop) ? RED : RESET, i,
           (vm.loop[i].enabled_loop), RESET);
   
  }

  return 1;
}



void create_default_nvm() {
  memset(&nvm, 0, sizeof(SPONDOOLIES_NVM));
  nvm.store_time = time(NULL);
  nvm.nvm_version = NVM_VERSION;
  int l, h, i = 0;

  // See max rate under ASIC_VOLTAGE_810
  for (i = 0; i < HAMMERS_COUNT; i++) {
    nvm.asic_corner[i] = ASIC_CORNER_SS; // all start ass SS corner
    nvm.top_freq[i] = ASIC_FREQ_SAFE;
  } 

  for (i = 0; i < LOOP_COUNT; i++) {
    nvm.top_dc2dc_current_16s[i] = DC2DC_CURRENT_TOP_BEFORE_LEARNING_16S;
  }
  nvm.dirty = 1;
}

void init_scaling() {
  hammer_iter hi;
  hammer_iter_init(&hi);
  vm.scaling_up_system = 1;
}



int update_i2c_temperature_measurments() {
  int err;
  vm.ac2dc_temp = ac2dc_get_temperature();

  for (int i = 0; i < LOOP_COUNT; i++) {
    vm.loop[i].dc2dc.dc_temp = dc2dc_get_temp(i, &err);
  }

  return 0;
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




// Callibration SW
void print_asic(HAMMER *h) {
  if (h->asic_present) {
    DBG(DBG_SCALING,
        "      HAMMER %04x C:%d ENG:0x%x Hz:%d  WINS:%04d T:%x\n",
        h->address, h->corner, h->enabled_engines_mask,
        h->asic_freq, h->solved_jobs, h->asic_temp);
  } else {
    DBG(DBG_SCALING, "      HAMMER %04x ----\n", h->address);
  }
}

/*
void print_loop(int l) {
  LOOP *loop = &vm.loop[l];
  DBG(DBG_SCALING, "   LOOP %x AMP:%d\n", l, loop->dc2dc.dc_current_16s);
}
*/

void print_miner_box() { DBG(DBG_SCALING, "MINER AC:%x\n", vm.ac2dc_current); }

void print_scaling() {
  int err;
  hammer_iter hi;
  hammer_iter_init(&hi);
  int total_hash_power=0;
 // int loop_hash_power=0;  
  int hl = -1;
  printf(GREEN "\n----------\nAC2DC current=%d, temp=%d\n" RESET,
      vm.ac2dc_current,
      vm.ac2dc_temp
    );
  
  printf(GREEN "L |Vtrm|vlt|vlt|Wt|"  "A /Li/Li|Tmp/"  "Tmp|A|gH" RESET); 
  while (hammer_iter_next_present(&hi)) {
    if (hi.l != hl) {
      DC2DC* dc2dc = &vm.loop[hi.l].dc2dc;
      int volt = 0;//dc2dc_get_voltage(hi.l, &err);
     
      printf(GREEN 
        "\n%2d|%4x|%3d|%3d|%2d|"  
        "%s%2d%s/%2d/%2d|%s%3d%s|"   
        "%3d|%d|%2d|" RESET, 
        hi.l, 
        vm.loop_vtrim[hi.l]&0xffff,
        VTRIM_TO_VOLTAGE_MILLI(vm.loop_vtrim[hi.l]),
        volt,
        dc2dc->dc_power_watts_16s/16,
        
      ((dc2dc->dc_current_16s>=nvm.top_dc2dc_current_16s[hi.l] - 3*16)?RED:GREEN), dc2dc->dc_current_16s/16,GREEN,
        dc2dc->dc_current_limit_16s/16,
        nvm.top_dc2dc_current_16s[hi.l]/16,
      ((dc2dc->dc_temp>=DC2DC_TEMP_GREEN_LINE)?RED:GREEN), dc2dc->dc_temp,GREEN,
      
        vm.loop[hi.l].asic_temp_sum/vm.loop[hi.l].asic_count,
        vm.loop[hi.l].asic_count,
        vm.loop[hi.l].asic_hz_sum/1000
        );
      hl = hi.l;
      total_hash_power += hi.a->asic_freq*15+220;      
   
    } else {
    
      total_hash_power += hi.a->asic_freq*15+220;            
    }
    printf(GREEN "|%x:%s%3dc%s %s%3dhz%s %x" RESET, 
      hi.addr,
      (hi.a->asic_temp>=MAX_ASIC_TEMPERATURE)?RED:GREEN,((hi.a->asic_temp*6)+77),GREEN,
       (hi.a->asic_freq>=MAX_ASIC_FREQ)?RED:GREEN,hi.a->asic_freq*15+210,GREEN,
      vm.working_engines[hi.addr]);
  }
  // print last loop
  // print total hash power
  printf(RESET "\n[H:%dGh]\n",(total_hash_power*15)/1000);

  
  /*
  print_miner_box();
  int h, l;
  for (l = 0; l < LOOP_COUNT; l++) {
    // TODO
    if (vm.loop[l].enabled_loop) {
      print_loop(l);
      for (h = 0; h < HAMMERS_PER_LOOP; h++) {
        HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
        print_asic(a);
      }
    } else {
      DBG(DBG_SCALING, "    LOOP %x ----\n", l);
    }
  }
  */
}
