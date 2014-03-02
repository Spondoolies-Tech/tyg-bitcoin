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




int AC_CURRNT_PER_15_HZ[ASIC_VOLTAGE_COUNT] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int DC_CURRNT_PER_15_HZ[ASIC_VOLTAGE_COUNT] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

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


void set_asic_freq(int addr, ASIC_FREQ new_freq) {
  if (vm.hammer[addr].asic_freq != new_freq) {
    printf("Changes ASIC %x frequency from %d to %d\n", addr,vm.hammer[addr].asic_freq*15+210,new_freq*15+210);
    vm.hammer[addr].asic_freq = new_freq;
  }
  set_pll(addr, new_freq);
}



void set_safe_voltage_and_frequency() {
  // for each enabled loop
  for (int l = 0; l < LOOP_COUNT; l++) {
    // Set voltage
    int err; 
    dc2dc_set_voltage(l, ASIC_VOLTAGE_DEFAULT, &err);
    if (err) {
      printf(RED "Failed to set voltage DC2DC\n" RESET);
    } else {
      printf("Set voltage %d DC2DC\n", ASIC_VOLTAGE_DEFAULT);
    }
  }
  hammer_iter hi;
  hammer_iter_init(&hi);
  disable_engines_all_asics();
  while (hammer_iter_next_present(&hi)) {
    set_asic_freq(hi.addr, ASIC_FREQ_SAFE);
  }
  enable_nvm_engines_all_asics_ok(); 
}


void enable_voltage_from_nvm() {
  int l, h, i = 0;
  // for each enabled loop
  disable_engines_all_asics();
  for (l = 0; l < LOOP_COUNT; l++) {
    if (!vm.loop[l].enabled_loop) {
      // Set voltage
      int err;
      dc2dc_set_voltage(l, nvm.loop_voltage[l], &err);
      // passert(err);

      // for each ASIC
      for (h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
        HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
        // Set freq
        if (a->asic_present) {
          set_asic_freq(a->address, ASIC_FREQ_SAFE);
        }
      }
    }
  }
  enable_nvm_engines_all_asics_ok();
}


void set_nvm_dc2dc_voltage() {
  // for each enabled loop
  for (int l = 0; l < LOOP_COUNT; l++) {
    // Set voltage
    int err;
    dc2dc_set_voltage(l, nvm.loop_voltage[l], &err);
    //passert(err);
  }
  enable_nvm_engines_all_asics_ok();
}


void pause_all_mining_engines() {
  passert(vm.asics_shut_down_powersave == 0);
  int some_asics_busy = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY);
  passert(some_asics_busy == 0);
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
    nvm.loop_voltage[i] = ASIC_VOLTAGE_DEFAULT;
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


// returns worst asic
//  Any ASIC is worth then NULL
HAMMER *choose_asic_to_throttle(HAMMER *a, HAMMER *b) {
  if (!a || !can_be_downscaled(a))
    return b;
  if (!b || !can_be_downscaled(b))
    return a;

 

  if (a->asic_temp != b->asic_temp) {
     // Reduce higher temperature because they have higher leakage
     return (a->asic_temp > b->asic_temp) ? a : b;
  }
  
  if (a->corner != b->corner) {
     // Reduce higher corners because they have higher leakage
     return (a->corner > b->corner) ? a : b;
  }

  if (a->asic_freq != b->asic_freq) {
    // Reduce higher frequency
    return (a->asic_freq > b->asic_freq) ? a : b;
  }

  return a;
 
}

// return hottest ASIC
/*
HAMMER *find_asic_to_reduce_ac_current() {
  HAMMER *best = NULL;
  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    best = choose_asic_to_throttle(best, hi.a);
  }

  passert(can_be_downscaled(best));
  return best;
}
*/
HAMMER *find_asic_to_reduce_dc_current(int l) {
  int h;
  // Find hottest ASIC at highest corner.
  HAMMER *best = NULL;
  for (h = 0; h < HAMMERS_PER_LOOP; h++) {
    HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
    if (a->asic_present) {
     printf("%p %p\n",a,best);
     best = choose_asic_to_throttle(best, a);
    }
  }
  printf("%p>, %x\n",best, best->address);
  if(can_be_downscaled(best)) {
     printf("%p>, %x\n",best, best->address);
     return best;
  }
  return NULL;
}



int can_be_upscaled(HAMMER *a) {
  if (!a->asic_present ||
      (a->asic_freq >= MAX_ASIC_FREQ) ||
      (a->asic_temp >= MAX_ASIC_TEMPERATURE) ||
      ((vm.loop[a->loop_address].dc2dc.dc_current_16s + DC2DC_KEEP_FOR_LEAKAGE_16S)
            >= nvm.top_dc2dc_current_16s[a->loop_address]) ||
      (vm.loop[a->loop_address].dc2dc.dc_temp>= DC2DC_TEMP_GREEN_LINE) || 
      (vm.ac2dc_current >= AC2DC_CURRENT_GREEN_LINE) ||
      (vm.ac2dc_temp >= AC2DC_TEMP_GREEN_LINE))

  {
    return 0;
  }

   if ((time(NULL) - a->last_freq_change_time < TRY_ASIC_FREQ_INCREASE_PERIOD_SECS) && 
        !vm.scaling_up_system) {
     return 0;
  }
  return 1;
}


void asic_upscale(HAMMER *a) {
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq+1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = time(NULL);      
}


int can_be_downscaled(HAMMER *a) {
  return (a->asic_freq > ASIC_FREQ_225);
}


void asic_downscale(HAMMER *a, time_t now) {
   passert(vm.engines_disabled == 1);
   printf(RED "xASIC DOWNSCALE %x!\n", a->address);
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq-1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


// returns best asic
HAMMER *choose_asic_to_upscale(HAMMER *a, HAMMER *b) {
  if (!a || !can_be_upscaled(a))
    return b;
  if (!b || !can_be_upscaled(b))
    return a;

  if (a->asic_temp != b->asic_temp) {
    // Increase lower asic_temp because they have lower leakage
    return (a->asic_temp < b->asic_temp) ? a : b;
  }

  if (a->last_freq_change_time != b->last_freq_change_time) {
    // Try someone else for a change
    return (a->last_freq_change_time < b->last_freq_change_time) ? a : b;
  }


  if (a->corner != b->corner) {
     // Upscale lower corners because they have lower leakage
     return (a->corner < b->corner) ? a : b;
  }

  if (a->asic_freq != b->asic_freq) {
    // Increase slower ASICs first
    return (a->asic_freq < b->asic_freq) ? a : b;
  }  
  
  return a;
}


HAMMER *find_asic_to_increase_speed(int l) {
  HAMMER *best = NULL;
  for (int h = 0; h < HAMMERS_PER_LOOP ; h++) { 
    HAMMER *a = &vm.hammer[l*HAMMERS_PER_LOOP+h];
    if (a->asic_present) {
      best = choose_asic_to_upscale(best, a);
    }
  }

  if(best && can_be_upscaled(best))
    return best;
  return NULL;
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
  //dc2dc_set_voltage(0, ASIC_VOLTAGE_, &err);
}


int count_ones(int failed_engines) {
  int c;
  for (c = 0; failed_engines; failed_engines >>= 1)
  {
    c += failed_engines & 1;
  }
  return c;
}


void reduce_dc2dc_strain_if_needed() {
  int any_scaled = 0;
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    int scaled = 0;
    
    while ((vm.loop[l].dc2dc.dc_current_16s  - scaled*16)
              >= nvm.top_dc2dc_current_16s[l]) {
      if (!any_scaled) {
        stop_all_work();
        disable_engines_all_asics();
        any_scaled=1;
      }
      printf(RED "LOOP DOWNSCALE %d\n" RESET, l);
      HAMMER *h = find_asic_to_reduce_dc_current(l);
      assert(h);
      printf(RED "Starin ASIC DOWNSCALE %d\n" RESET, h->address);
      asic_downscale(h, time(NULL));
      scaled++;
    }

    if (scaled) {
      // don't scale till next measurement
      vm.loop[l].dc2dc.dc_current_16s = 0;
    }
  }
  
  if (any_scaled) {
    enable_nvm_engines_all_asics_ok();
  }
}


void periodic_bist_task() {
  struct timeval tv;
  int now = time(NULL);
  start_stopper(&tv);
  stop_all_work();
  int usec;
  printf(RED "BIST!" RESET);

  {
    hammer_iter hi;
    hammer_iter_init(&hi);
    while (hammer_iter_next_present(&hi)) {
      hi.a->failed_bists = 0;    
      hi.a->passed_last_bist_engines = ALL_ENGINES_BITMASK;
    }
  }
  
  do_bist_ok();
  disable_engines_all_asics();
 
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    if (!vm.loop[l].enabled_loop) {
      continue;
    }
   
    for (int a = 0 ; a < HAMMERS_PER_LOOP; a++) {
      HAMMER *h = &vm.hammer[l*HAMMERS_PER_LOOP+a];
      if (!h->asic_present) {
        continue;
      }
      
      int passed = h->passed_last_bist_engines;
      if ((passed != ALL_ENGINES_BITMASK) || 
          (h->asic_temp >= MAX_ASIC_TEMPERATURE && 
            time(NULL) - h->last_freq_change_time > HOT_ASIC_FREQ_DECREASE_PERIOD_SECS)) {
        vm.scaling_up_system = 0;

        int failed_engines_mask = passed ^ ALL_ENGINES_BITMASK;
        // int failed_engines_count = count_ones(failed_engines_mask);
        printf(RED "Failed asic %x engines passed %x, temp %d\n" RESET, h->address, passed);
        
        if (can_be_downscaled(h)) {
          asic_downscale(h, now);
        } else {
          vm.working_engines[h->address] = vm.working_engines[h->address]&passed;
          if (vm.working_engines[h->address] == 0) {
            // disable ASIC failing bist on all engines.
            disable_asic_forever(h->address);
          }
        }
      } 
      // Increase ASIC speed not durring bring-up
    }

    // try upscale 1 asic in loop
    HAMMER *hh = find_asic_to_increase_speed(l);
    if (hh) {
      if (hh->asic_freq == MAX_ASIC_FREQ &&
          hh->asic_temp == ASIC_TEMP_77) {
        printf(RED "Something wrong with ASIC %x, kill it\n", hh->address);
        //disable_asic_forever(hh->address);
      }
      asic_upscale(hh);
    }   
  }
  enable_nvm_engines_all_asics_ok();
  // measure how long it took
  end_stopper(&tv, "BIST");
  resume_all_work();
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
  hammer_iter hi;
  hammer_iter_init(&hi);

  int hl = -1;
//  printf(GREEN "\nASIC TMP:" RESET);
  while (hammer_iter_next_present(&hi)) {
    if (hi.l != hl) {
      int millivolts;
      VOLTAGE_ENUM_TO_MILIVOLTS(nvm.loop_voltage[hi.l], millivolts) ;
      printf(GREEN "\nloop %d:[V=%d][DC:C=%s%d%s/%d][DC:T=%s%d%s]:" RESET, 
        hi.l, millivolts ,
        ((vm.loop[hi.l].dc2dc.dc_current_16s>=nvm.top_dc2dc_current_16s[hi.l] - 5*16)?RED:GREEN), 
        vm.loop[hi.l].dc2dc.dc_current_16s/16,GREEN,
        nvm.top_dc2dc_current_16s[hi.l]/16,
      ((vm.loop[hi.l].dc2dc.dc_temp>=DC2DC_TEMP_GREEN_LINE)?RED:GREEN), 
        vm.loop[hi.l].dc2dc.dc_temp,GREEN,
        
        vm.loop[hi.l].dc2dc.dc_temp);
      hl = hi.l;
    }
    printf(GREEN "%x|%s%dc%s %s%dhz%s %x|" RESET, 
      hi.addr,
      (hi.a->asic_temp>=MAX_ASIC_TEMPERATURE)?RED:GREEN,((hi.a->asic_temp*6)+77),GREEN,
       (hi.a->asic_freq>=MAX_ASIC_FREQ)?RED:GREEN,hi.a->asic_freq*15+210,GREEN,
      vm.working_engines[hi.addr]);
  }
  printf(RESET "\n");

  
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
