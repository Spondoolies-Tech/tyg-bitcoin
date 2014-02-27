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
#include "scaling_manager.h"

/*
       Supported temp sensor settings:
   temp    83 / 89 / 95 / 101 / 107 / 113 /119 / 125
   T[2:0]  0    1     2    3     4     5    6     7 
*/

// When ariving here (>=) cut the frequency drastically.
ASIC_TEMP CRITICAL_TEMPERATURE_PER_CORNER[ASIC_CORNER_COUNT] = 
    { ASIC_TEMP_119, ASIC_TEMP_119, ASIC_TEMP_119, ASIC_TEMP_119, ASIC_TEMP_119, ASIC_TEMP_119 };
// When under this (<) raise the frequency slowly
ASIC_TEMP BEST_TEMPERATURE_PER_CORNER[ASIC_CORNER_COUNT] = 
  { ASIC_TEMP_95, ASIC_TEMP_95, ASIC_TEMP_95, ASIC_TEMP_95, ASIC_TEMP_95, ASIC_TEMP_95 };

// When under this (<) raise the frequency slowly
// TODO
ASIC_FREQ MINIMAL_FREQ_PER_CORNER[ASIC_CORNER_COUNT] = {
  ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_225,
  ASIC_FREQ_225
};



int AC_CURRNT_PER_15_HZ[ASIC_VOLTAGE_COUNT] = { 1, 1, 1, 1, 1, 1, 1, 1 };
int DC_CURRNT_PER_15_HZ[ASIC_VOLTAGE_COUNT] = { 1, 1, 1, 1, 1, 1, 1, 1 };

void send_keep_alive();

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
void print_state();
void print_scaling();

MINER_BOX vm = { 0 };
extern SPONDOOLIES_NVM nvm;
extern int enable_scaling;
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

void get_delta_after_asic_freq_change(int loop, int *dc_delta, int *ac_delta) {
 // int loop_addr = asic_addr / HAMMERS_PER_LOOP;
  passert(vm.loop[loop].enabled_loop);
  printf(ANSI_COLOR_RED "ENABLE AC_CURRNT_PER_15_HZ PART 123 TODO" ANSI_COLOR_RESET);
  *dc_delta = AC_CURRNT_PER_15_HZ[nvm.loop_voltage[loop]];
  *ac_delta = DC_CURRNT_PER_15_HZ[nvm.loop_voltage[loop]];
}

int try_increase_asic_freq(HAMMER *h) {
  int ac_delta_15hz;
  int dc_delta_15hz;    
  assert(h->asic_present);
  get_delta_after_asic_freq_change(h->loop_address, 
                          &ac_delta_15hz, 
                          &dc_delta_15hz);

  if (vm.ac2dc_current == 0 ||
      vm.loop[h->loop_address].dc2dc.dc_current_16s == 0 ||
      ((AC2DC_CURRENT_GREEN_LINE - vm.ac2dc_current) < ac_delta_15hz) ||
      ((DC2DC_CURRENT_GREEN_LINE_16S - vm.loop[h->loop_address].dc2dc.dc_current_16s) < dc_delta_15hz)) {
      printf("Failed to increase frequency on ASIC %x - no spare power\n", h->address);
    return 0;
  }
  ASIC_FREQ new_freq = (ASIC_FREQ)(h->asic_freq+1);

  if ((h->temperature > CRITICAL_TEMPERATURE_PER_CORNER[nvm.asic_corner[h->address]]) &&
      (new_freq > h->max_freq)) {
    printf("Failed to increase frequency on ASIC %x - too hot or freq too high\n", h->address);
    return 0;
  }

  printf("Set frequency on %x from %d to %d. \n",
      h->address, h->asic_freq*15+210, new_freq*15+210);
  set_asic_freq(h->address, new_freq);
  h->last_freq_increase_time = time(NULL);
  vm.ac2dc_current += ac_delta_15hz;
  vm.loop[h->loop_address].dc2dc.dc_current_16s += dc_delta_15hz;
  printf("  now dc2dc_16s guessed %d and ac2dc guessed\n",
       vm.loop[h->loop_address].dc2dc.dc_current_16s,
       vm.ac2dc_current);

  return 1;
}



void enable_engines_from_nvm() {
  enable_nvm_engines_all_asics();
}

void set_safe_voltage_and_frequency() {
  // for each enabled loop
  for (int l = 0; l < LOOP_COUNT; l++) {
    // Set voltage
    int err;
    dc2dc_set_voltage(l, ASIC_VOLTAGE_810, &err);
    if (err) {
      printf(ANSI_COLOR_RED "Failed to set voltage DC2DC\n" ANSI_COLOR_RESET);
    }
  }
  hammer_iter hi;
  hammer_iter_init(&hi);
  disable_engines_all_asics();
  while (hammer_iter_next_present(&hi)) {
    set_asic_freq(hi.addr, ASIC_FREQ_225);
  }
  enable_nvm_engines_all_asics();
}


void restore_nvm_voltage_and_frequency() {
  // for each enabled loop
  for (int l = 0; l < LOOP_COUNT; l++) {
    // Set voltage
    int err;
    dc2dc_set_voltage(l, nvm.loop_voltage[l], &err);
    //passert(err);
  }
  
  hammer_iter hi;
  hammer_iter_init(&hi);
  disable_engines_all_asics();
  while (hammer_iter_next_present(&hi)) {
    set_asic_freq(hi.addr, hi.a->asic_freq);
  }
  enable_nvm_engines_all_asics();
}


void pause_all_mining_engines() {
  assert(vm.asics_shut_down_powersave == 0);
  int some_asics_busy = read_reg_broadcast(BIT_INTR_CONDUCTOR_BUSY);
  assert(some_asics_busy == 0);
  // stop all ASICs
  disable_engines_all_asics();
  
  // disable_engines_all_asics();
  vm.asics_shut_down_powersave = 1;
  printf("PAUSING ALL MINING!!\n");
}

void unpause_all_mining_engines() {
  assert(vm.asics_shut_down_powersave != 0);
  vm.not_mining_counter = 0;
  hammer_iter hi;
  hammer_iter_init(&hi);
  enable_nvm_engines_all_asics();

  while (hammer_iter_next_present(&hi)) {
    // for each ASIC
    write_reg_device(hi.addr, ADDR_RESETING0, 0xffffffff);
    write_reg_device(hi.addr, ADDR_RESETING1, 0xffffffff);
    write_reg_device(hi.addr, ADDR_ENABLE_ENGINE, nvm.working_engines[hi.addr]);
  }
  printf("STARTING ALL MINING!!\n");

  vm.asics_shut_down_powersave = 0;
}

void enable_voltage_freq_and_engines_from_nvm() {
  int l, h, i = 0;
  // for each enabled loop
  disable_engines_all_asics();
  for (l = 0; l < LOOP_COUNT; l++) {
    if (!nvm.loop_brocken[l]) {
      // Set voltage
      int err;
      dc2dc_set_voltage(l, nvm.loop_voltage[l], &err);
      // passert(err);

      // for each ASIC
      for (h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
        HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
        // Set freq
        if (a->asic_present) {
          a->corner = nvm.asic_corner[l * HAMMERS_PER_LOOP + h];
          a->max_freq = nvm.top_freq[l * HAMMERS_PER_LOOP + h];
          set_asic_freq(a->address, MINIMAL_FREQ_PER_CORNER[a->corner]);
        }
      }
    }
  }
  enable_nvm_engines_all_asics();
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

int enable_nvm_loops_ok() {
  write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
  write_spi(ADDR_SQUID_LOOP_RESET, 0);
  write_spi(ADDR_SQUID_COMMAND, 0xF);
  write_spi(ADDR_SQUID_COMMAND, 0x0);
  write_spi(ADDR_SQUID_LOOP_BYPASS, ~(nvm.good_loops));
  write_spi(ADDR_SQUID_LOOP_RESET, 0xFFFFFF); 
  int ok = test_serial(-1);

  if (!ok) {
    return 0;
  }

  for (int i = 0; i < LOOP_COUNT; i++) {
    printf("%sloop %d enabled = %d%s\n",
           (nvm.loop_brocken[i]) ? ANSI_COLOR_RED : ANSI_COLOR_RESET, i,
           !(nvm.loop_brocken[i]), ANSI_COLOR_RESET);
    vm.loop[i].enabled_loop = !(nvm.loop_brocken[i]);
    vm.loop[i].id = i;
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
    nvm.working_engines[i] = ALL_ENGINES_BITMASK;
    nvm.asic_corner[i] = ASIC_CORNER_SS; // all start ass SS corner
    nvm.top_freq[i] = ASIC_FREQ_225;
  }

  nvm.dirty = 1;
}

void init_scaling() {
  // ENABLE ENGINES
  passert(enable_scaling);
}

HAMMER *get_hammer(uint32_t addr) {
  int l, h;
  ASIC_GET_LOOP_OFFSET(addr, l, h);
  HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
  return a;
}


int update_i2c_temperature_measurments() {
  int err;
  vm.ac2dc_temp = ac2dc_get_temperature();

  for (int i = 0; i < LOOP_COUNT; i++) {
    vm.loop[i].dc2dc.dc_temp = dc2dc_get_temp(i, &err);
  }

  return 0;
}

bool can_be_throttled(HAMMER *a) {
  return (a->asic_present &&
          (a->asic_freq > MINIMAL_FREQ_PER_CORNER[a->corner]));
}

// returns worst asic
//  Any ASIC is worth then NULL
HAMMER *choose_asic_to_throttle(HAMMER *a, HAMMER *b) {
  if (!a || !can_be_throttled(a))
    return b;
  if (!b || !can_be_throttled(b))
    return a;

  if (a->corner != b->corner) {
    // Reduce higher corners because they have higher leakage
    return (a->corner > b->corner) ? a : b;
  }

  if (a->asic_freq != b->asic_freq) {
    // Reduce higher frequency
    return (a->asic_freq > b->asic_freq) ? a : b;
  }


  if (a->temperature != b->temperature) {
    // Reduce higher temperature because they have higher leakage
    return (a->temperature > b->temperature) ? a : b;
  }
}

// return hottest ASIC
HAMMER *find_asic_to_reduce_ac_current() {
  HAMMER *best = NULL;
  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    best = choose_asic_to_throttle(best, hi.a);
  }

  passert(can_be_throttled(best));
  return best;
}

HAMMER *find_asic_to_reduce_dc_current(int l) {
  int h;
  // Find hottest ASIC at highest corner.
  HAMMER *best = NULL;
  for (h = 0; h < HAMMERS_PER_LOOP; h++) {
    HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
    best = choose_asic_to_throttle(best, a);
  }
  
  if(can_be_throttled(best))
     return best;
  return NULL;
}


void decrease_asics_freqs() {
  int l;
  int current_reduced = 0;
  int changed = false;
#if 1
  struct timeval tv;
  start_stopper(&tv);

  // First resolve the DC2DC current issues
  disable_engines_all_asics();
  for (l = 0; l < LOOP_COUNT; l++) {   
    if (!vm.loop[l].enabled_loop) 
         continue;
       
    while (vm.loop[l].dc2dc.dc_current_16s >= DC2DC_CURRENT_RED_LINE_16S) {
      if (!vm.stopped_all_work) {
        changed = true;
        stop_all_work();
      }

      HAMMER *a = find_asic_to_reduce_dc_current(l);
      passert(a);
      printf("DC2DC OVER LIMIT, throttling ASIC:%d!\n", a->address);
      int dc_delta = DC_CURRNT_PER_15_HZ[nvm.loop_voltage[a->loop_address]];
      int ac_delta = AC_CURRNT_PER_15_HZ[nvm.loop_voltage[a->loop_address]]; 
      if (vm.ac2dc_current > ac_delta) {
        vm.ac2dc_current -= ac_delta;
      } else {
        vm.ac2dc_current = 0;
      }
      if (vm.loop[l].dc2dc.dc_current_16s  > dc_delta) {
        vm.loop[l].dc2dc.dc_current_16s  -= dc_delta;
      } else {
        vm.loop[l].dc2dc.dc_current_16s  = 0;
      }
      passert(a->asic_freq - MINIMAL_FREQ_PER_CORNER[a->corner] >= 1);
      set_asic_freq(a->address, (ASIC_FREQ)(a->asic_freq-1));
    }
  }

  while (vm.ac2dc_current > 0 && 
         vm.ac2dc_current >= AC2DC_CURRENT_RED_LINE) {
    if (!vm.stopped_all_work) {
      changed = true;
      stop_all_work();
    }
    HAMMER *a = find_asic_to_reduce_ac_current();
    int ac_delta = AC_CURRNT_PER_15_HZ[nvm.loop_voltage[a->loop_address]]; 
    // We dont care about DC2DC delta here
    if (vm.ac2dc_current > ac_delta) {
      vm.ac2dc_current -= ac_delta;
    } else {
      vm.ac2dc_current = 0;
    }
    printf("AC2DC OVER LIMIT, throttling ASIC:%d %d!\n", a->address, a->asic_freq);
    
    passert(a->asic_freq - MINIMAL_FREQ_PER_CORNER[a->corner] >= 1);
    set_asic_freq(a->address, (ASIC_FREQ)(a->asic_freq-1));
  }
  vm.ac2dc_current = 0; // to update later
  enable_nvm_engines_all_asics();
  resume_all_work();
  if (changed) {
    usleep(TIME_FOR_DLL_USECS);
  }
  end_stopper(&tv, "DOWNSCALE");
#endif
}


bool can_be_upscaled(HAMMER *a) {
  if (!a->asic_present ||
      (a->asic_freq >= a->max_freq) ||
      ((DC2DC_CURRENT_GREEN_LINE_16S - vm.loop[a->loop_address].dc2dc.dc_current_16s) > 
         DC_CURRNT_PER_15_HZ[a->corner])) {
    a->can_scale_up = 0;
    return 0;
  }
  return 1;
}


// returns best asic
HAMMER *choose_asic_to_upscale(HAMMER *a, HAMMER *b) {
  if (!a || !can_be_upscaled(a))
    return b;
  if (!b || !can_be_upscaled(b))
    return a;

  if (a->corner != b->corner) {
    // Upscale lower corners because they have lower leakage
    return (a->corner < b->corner) ? a : b;
  }

  if (a->asic_freq != b->asic_freq) {
    // Increase slower ASICs first
    return (a->asic_freq < b->asic_freq) ? a : b;
  }


  if (a->temperature != b->temperature) {
    // Increase lower temperature because they have lower leakage
    return (a->temperature < b->temperature) ? a : b;
  }
}


HAMMER *find_asic_to_increase_speed() {
  HAMMER *best = NULL;
  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    HAMMER *a = &vm.hammer[hi.addr];
    best = choose_asic_to_upscale(best, a);
  }

  if(can_be_upscaled(best))
    return best;
  return NULL;
}


int increase_asics_freqs() {
  int ret = 0; 
  // Don't increase speed when paused
  if (vm.cosecutive_jobs < MIN_COSECUTIVE_JOBS_FOR_SCALING) {
    printf("No scaling when no jobs\n");
    return ret;
  }

  int now = time(NULL);
  int h, l;

  // first handle critical system AC2DC current
  if ((AC2DC_CURRENT_GREEN_LINE - vm.ac2dc_current) <= 0) {
    return ret;
  }

  
  // mark all ASICs we can increase speed
  for (l = 0; l < LOOP_COUNT; l++) {
    // Works for disabled loops too
    // vm.loop[l].dc2dc.dc_spare_power = dc2dc_spare_power(l);
    for (h = 0; h < HAMMERS_PER_LOOP; h++) {
      int addr = l*HAMMERS_PER_LOOP + l;
      if (vm.hammer[addr].asic_present &&
          (vm.hammer[addr].asic_freq < vm.hammer[addr].max_freq) &&
          (vm.loop[l].dc2dc.dc_current_16s < DC2DC_CURRENT_GREEN_LINE_16S) &&
          (now - vm.hammer[addr].last_freq_increase_time) > ASIC_SPEED_RAISE_TIME) {
        vm.hammer[addr].can_scale_up = 1;
      } else {
        vm.hammer[addr].can_scale_up = 0;
      }
    }
  }
  
  HAMMER *a;
  disable_engines_all_asics();
  while (vm.ac2dc_current < AC2DC_CURRENT_GREEN_LINE &&
        (a = find_asic_to_increase_speed())) {
    if (!vm.stopped_all_work) {
      ret = 1;
      stop_all_work();
    }
    try_increase_asic_freq(a);
    a->can_scale_up = 0;
  }
  enable_nvm_engines_all_asics();
  resume_all_work();
  return ret;
}



//
void set_optimal_voltage() {
  // Load from NVM?
}

void periodic_upscale_task() {
  struct timeval tv;
  start_stopper(&tv);
  int usec;
#ifdef DBG_SCALING
  print_scaling();
#endif
  int changed = increase_asics_freqs();
  // measure how long it took
  end_stopper(&tv, "UPSCALE");
}


void periodic_bist_task() {
  struct timeval tv;
  start_stopper(&tv);
  stop_all_work();
  int usec;
#ifdef DBG_SCALING
  print_scaling();
#endif

  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    hi.a->failed_bists = 0;    
    hi.a->passed_last_bist_engines = ALL_ENGINES_BITMASK;
  }

  if (!do_bist_ok()) {
    spond_delete_nvm();
    passert(0);
  }

  // measure how long it took
  end_stopper(&tv, "BIST");
  resume_all_work();
}



// Callibration SW

void print_asic(HAMMER *h) {
  if (h->asic_present) {
    DBG(DBG_SCALING,
        "      HAMMER %04x C:%d ENG:0x%x Hz:%d MaxHz:%d WINS:%04d T:%x\n",
        h->address, h->corner, h->enabled_engines_mask,
        h->asic_freq, h->max_freq, h->solved_jobs, h->temperature);
  } else {
    DBG(DBG_SCALING, "      HAMMER %04x ----\n", h->address);
  }
}

void print_loop(int l) {
  LOOP *loop = &vm.loop[l];
  DBG(DBG_SCALING, "   LOOP %x AMP:%d\n", l,
      loop->dc2dc.dc_current_16s);
}

void print_miner_box() { DBG(DBG_SCALING, "MINER AC:%x\n", vm.ac2dc_current); }

void print_scaling() {
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
}
