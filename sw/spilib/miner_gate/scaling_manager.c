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

DC2DC_VOLTAGE CORNER_TO_VOLTAGE_TABLE[ASIC_CORNER_COUNT] = {
  ASIC_VOLTAGE_765, ASIC_VOLTAGE_765, ASIC_VOLTAGE_720, ASIC_VOLTAGE_630,
  ASIC_VOLTAGE_630, ASIC_VOLTAGE_630
};

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

void loop_disable_hw(int loop_id) {
  printf(ANSI_COLOR_YELLOW "Disabling loop %x...... TODO!!!!\n" ANSI_COLOR_RESET, loop_id);
}

void set_asic_freq(int addr, ASIC_FREQ new_freq) {
  if (vm.hammer[addr].asic_freq != new_freq) {
    set_pll(addr, new_freq);
    printf("Changes ASIC %x frequency from %d to %d\n", addr,vm.hammer[addr].asic_freq*15+210,new_freq*15+210);
    vm.hammer[addr].asic_freq = new_freq;
  } else {
    printf(ANSI_COLOR_YELLOW "Setting ASIC %x to same frequency?\n" ANSI_COLOR_RESET, addr);
  }
}

void get_delta_after_asic_freq_change(int loop, int *dc_delta, int *ac_delta) {
 // int loop_addr = asic_addr / HAMMERS_PER_LOOP;
  passert(vm.loop[loop].enabled_loop);
  printf(ANSI_COLOR_RED "ENABLE AC_CURRNT_PER_15_HZ PART 123 TODO" ANSI_COLOR_RESET);
  *dc_delta = AC_CURRNT_PER_15_HZ[nvm.loop_voltage[loop]];
  *ac_delta = DC_CURRNT_PER_15_HZ[nvm.loop_voltage[loop]];
}

int try_increase_asic_freq(HAMMER *h, 
                      int *ac_spare_power,
                      int *dc_spare_power) {
  int ac_spare_power_15_hz;
  int dc_spare_power_15_hz;                    
  get_delta_after_asic_freq_change(h->address/HAMMERS_PER_LOOP, 
                          &ac_spare_power_15_hz, 
                          &dc_spare_power_15_hz);

  if ((ac_spare_power_15_hz < *ac_spare_power)
      || (ac_spare_power_15_hz < *dc_spare_power)) {
      printf("Failed to increase frequency on ASIC %x - no spare power\n", h->address);
    return 0;
  }
  ASIC_FREQ new_freq = (ASIC_FREQ)(h->asic_freq+1);

  if ((h->temperature > CRITICAL_TEMPERATURE_PER_CORNER[nvm.asic_corner[h->address]]) &&
      (new_freq > nvm.top_freq[h->address])) {
    printf("Failed to increase frequency on ASIC %x - too hot or freq too high\n", h->address);
    return 0;
  }

  DBG(DBG_SCALING, "Set frequency on %x from %d to %d. Points: %d %d\n",
      h->address, h->asic_freq, new_freq, 
      *ac_spare_power, *dc_spare_power);
  set_asic_freq(h->address, new_freq);
  h->last_freq_increase_time = time(NULL);
  *ac_spare_power -= ac_spare_power_15_hz;
  *dc_spare_power -= dc_spare_power_15_hz;
  return 1;
}

void disable_all_engines_all_asics() {
  int i;
  write_reg_broadcast(ADDR_RESETING0, 0xffffffff);
  write_reg_broadcast(ADDR_RESETING1, 0xffffffff);
  write_reg_broadcast(ADDR_ENABLE_ENGINE, 0);
  flush_spi_write();
}

void enable_all_engines_all_asics() {
  int i;
  write_reg_broadcast(ADDR_RESETING0, 0xffffffff);
  write_reg_broadcast(ADDR_RESETING1, 0xffffffff);
  write_reg_broadcast(ADDR_ENABLE_ENGINE, ALL_ENGINES_BITMASK);
  flush_spi_write();
}

void enable_engines_from_nvm() {
  // int l, h, i = 0;
  // for each enabled loop

  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    // for each ASIC
    write_reg_device(hi.addr, ADDR_RESETING0, 0xffffffff);
    write_reg_device(hi.addr, ADDR_RESETING1, 0xffffffff);
    write_reg_device(hi.addr, ADDR_ENABLE_ENGINE, nvm.working_engines[hi.addr]);
  }
  flush_spi_write();
}

void enable_voltage_freq_and_engines_default() {
  // for each enabled loop
  for (int l = 0; l < LOOP_COUNT; l++) {
    // Set voltage
    int err;
    dc2dc_set_voltage(l, ASIC_VOLTAGE_810, &err);
    passert(err);
  }
  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    set_asic_freq(hi.addr, ASIC_FREQ_225);
  }
}

void pause_all_mining_engines() {
  assert(vm.asics_shut_down_powersave == 0);
  int some_asics_busy = read_reg_broadcast(BIT_INTR_CONDUCTOR_BUSY);
  assert(some_asics_busy == 0);
  // stop all ASICs
  write_reg_broadcast(ADDR_ENABLE_ENGINE, 0);
  // disable_all_engines_all_asics();
  vm.asics_shut_down_powersave = 1;
  printf("PAUSING ALL MINING!!\n");
}

void unpause_all_mining_engines() {
  assert(vm.asics_shut_down_powersave != 0);
  vm.not_mining_counter = 0;
  hammer_iter hi;
  hammer_iter_init(&hi);

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
        set_asic_freq(a->address, nvm.top_freq[l * HAMMERS_PER_LOOP + h]);
      }
    }
  }
}

int test_serial(int loopid) {
  assert_serial_failures = false;
  // printf("Testing loops: %x\n", (~read_spi(ADDR_SQUID_LOOP_BYPASS))&
  // 0xFFFFFF);
  int i = 0;
  int val1;
  int val2;
  // This printfs used in tests! Don`t remove!!
  printf("Testing loop %d:", loopid);
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
  }

  // printf("Ok %d!\n", i);

  // test for connectivity
  int ver = read_reg_broadcast(ADDR_VERSION);

  // printf("XTesting loop, version = %x\n", ver);
  if (BROADCAST_READ_DATA(ver) != 0x3c) {
    // printf("Failed: Got version: %x!\n", ver);
    printf("BAD DATA\n");
    return 0;
  }

  printf("OK\n");
  assert_serial_failures = true;
  return 1;
}

int enable_nvm_loops_ok() {
  write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
  write_spi(ADDR_SQUID_LOOP_RESET, 0);
  write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
  write_spi(ADDR_SQUID_COMMAND, 0xF);
  write_spi(ADDR_SQUID_COMMAND, 0x0);
  write_spi(ADDR_SQUID_LOOP_BYPASS, ~(nvm.good_loops));
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

void discover_good_loops_update_nvm() {
  DBG(DBG_NET, "RESET SQUID\n");

  uint32_t good_loops = 0;
  int i, ret = 0, success;

  write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
  write_spi(ADDR_SQUID_LOOP_RESET, 0);
  write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
  write_spi(ADDR_SQUID_COMMAND, 0xF);
  write_spi(ADDR_SQUID_COMMAND, 0x0);
  write_spi(ADDR_SQUID_LOOP_BYPASS, 0);
  success = test_serial(-1);
  if (success) {
    for (i = 0; i < LOOP_COUNT; i++) {
      // vm.loop[i].present = true;
      nvm.loop_brocken[i] = false;
      ret++;
    }
  } else {
    for (i = 0; i < LOOP_COUNT; i++) {
      // vm.loop[i].id =i;
      unsigned int bypass_loops = (~(1 << i) & 0xFFFFFF);
      write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
      write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
      // write_spi(ADDR_SQUID_LOOP_RESET, 0);
      if (test_serial(i)) {
        // printf("--00--\n");
        nvm.loop_brocken[i] = false;
        // vm.loop[i].present = true;
        good_loops |= 1 << i;
        ret++;
      } else {
        // printf("--11--\n");
        nvm.loop_brocken[i] = true;
        // vm.loop[i].present = false;
        for (int h = i * HAMMERS_PER_LOOP; h < (i + 1) * HAMMERS_PER_LOOP;
             h++) {
          // printf("remove ASIC 0x%x\n", h);
          nvm.asic_ok[h] = 0;
          nvm.working_engines[h] = 0;
          nvm.top_freq[h] = ASIC_FREQ_0;
          nvm.asic_corner[h] = ASIC_CORNER_NA;
        }
        printf("TODO: DISABLE DC2DC HERE!!!\n");
      }
    }
    nvm.good_loops = good_loops;
    write_spi(ADDR_SQUID_LOOP_BYPASS, ~(nvm.good_loops));
    test_serial(-1);
  }
  nvm.dirty = 1;
  printf("Found %d good loops\n", ret);
  passert(ret);
}

void test_asics_in_freq(ASIC_FREQ freq_to_pass, ASIC_CORNER corner_to_set) {
  printf(ANSI_COLOR_MAGENTA "TODO TODO testing freq!\n" ANSI_COLOR_RESET);
}

void find_bad_engines_update_nvm() {
  int i;

  if (!do_bist_ok(false)) {
    // IF FAILED BIST - reduce top speed of failed ASIC.
    printf("INIT BIST FAILED, reseting working engines bitmask!\n");
    hammer_iter hi;
    hammer_iter_init(&hi);

    while (hammer_iter_next_present(&hi)) {
      // Update NVM
      if (nvm.working_engines[hi.addr] !=
          vm.hammer[hi.addr].passed_last_bist_engines) {
        nvm.working_engines[hi.addr] =
            vm.hammer[hi.addr].passed_last_bist_engines;
        nvm.dirty = 1;
        printf("After BIST setting %x enabled engines to %x\n", hi.addr,
               nvm.working_engines[hi.addr]);
      }
    }
  }
}

void recompute_corners_and_voltage_update_nvm() {
  int i;

  printf(ANSI_COLOR_MAGENTA "Checking ASIC speed...!\n" ANSI_COLOR_RESET);
  ASIC_FREQ corner_passage_freq_at_081v[ASIC_CORNER_COUNT] = {
    ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_345, ASIC_FREQ_510,
    ASIC_FREQ_660
  };

  printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
  for (i = ASIC_CORNER_SS; i < ASIC_CORNER_COUNT; i++) {
    test_asics_in_freq(corner_passage_freq_at_081v[i], (ASIC_CORNER)i);
  }

  // We can assume all corners updated. Set all frequencies:
  // printf("->>>--%x- %d %s\n",nvm.good_loops,  __LINE__, __FUNCTION__);
  for (int l = 0; l < LOOP_COUNT; l++) {
    if (nvm.good_loops & 1 << l) {
      // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
      int avarage_corner = 0;
      int working_asic_count = 0;
      // Compute corners to ASICs
      for (int h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
        // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
        if (nvm.working_engines[h]) {
          // printf("->>>---%d %d %s\n", nvm.asic_corner[h],__LINE__,
          // __FUNCTION__);
          avarage_corner += nvm.asic_corner[h];
          working_asic_count++;
        }
      }
      // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
      //  compute avarage corner
      if (working_asic_count) {
        avarage_corner = avarage_corner / working_asic_count;
        nvm.loop_voltage[l] = CORNER_TO_VOLTAGE_TABLE[avarage_corner];
        printf("avarage_corner per loop %d = %d\n", l, avarage_corner);
      } else {
        nvm.loop_voltage[l] = ASIC_VOLTAGE_0;
        printf("CLOSE LOOP???? TODO\n");
      }
    } else {
      nvm.loop_voltage[l] = ASIC_VOLTAGE_0;
      printf("CLOSE LOOP???? TODO\n");
    }
  }
  // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
  nvm.dirty = 1;
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

// Return 1 if needs urgent scaling
int update_top_current_measurments() {
  int err;
  if (!vm.asics_shut_down_powersave) {
    int current = ac2dc_get_power();
    if (current >= AC2DC_POWER_TRUSTWORTHY && 
      vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_AC2DC_MEASUREMENT) {
      vm.ac2dc_current = current;
    } else {
      vm.ac2dc_current = AC2DC_POWER_GREEN_LINE;
    }

    for (int i = 0; i < LOOP_COUNT; i++) {
      int current = dc2dc_get_current_16s_of_amper(i, &err);
      if (current >= DC2DC_POWER_TRUSTWORTHY && 
          vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_DC2DC_MEASUREMENT) {
      vm.loop[i].dc2dc.dc_current_16s_of_amper =
          dc2dc_get_current_16s_of_amper(i, &err);
      } else {
        vm.ac2dc_current = DC2DC_CURRENT_GREEN_LINE_16S;
      }
    }
  }
  return 0;
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
          (a->asic_freq > MINIMAL_FREQ_PER_CORNER[nvm.asic_corner[a->address]]));
}

// returns worst asic
//  Any ASIC is worth then NULL
HAMMER *choose_asic_to_throttle(HAMMER *a, HAMMER *b) {
  if (!a || !can_be_throttled(a))
    return b;
  if (!b || !can_be_throttled(b))
    return a;

  if (nvm.asic_corner[a->address] != nvm.asic_corner[b->address]) {
    // Reduce higher corners because they have higher leakage
    return (nvm.asic_corner[a->address] > nvm.asic_corner[b->address]) ? a : b;
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
    HAMMER *a = &vm.hammer[hi.addr];
    best = choose_asic_to_throttle(best, a);
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


  int ac_spare_power = ac2dc_spare_power();

  // First resolve the DC2DC current issues
  for (l = 0; l < LOOP_COUNT; l++) {   
    if (!vm.loop[l].enabled_loop) 
         continue;
       
    while (vm.loop[l].dc2dc.dc_current_16s_of_amper >= DC2DC_CURRENT_RED_LINE_16S) {
      if (!vm.stopped_all_work) {
        changed = true;
        stop_all_work();
      }

      HAMMER *a = find_asic_to_reduce_dc_current(l);
      passert(a);
      printf("DC2DC OVER LIMIT, throttling ASIC:%d!\n", a->address);
      int dc_delta = DC_CURRNT_PER_15_HZ[nvm.loop_voltage[a->address/HAMMERS_PER_LOOP]];
      int ac_delta = AC_CURRNT_PER_15_HZ[nvm.loop_voltage[a->address/HAMMERS_PER_LOOP]]; 
      dc_delta *= (a->asic_freq - MINIMAL_FREQ_PER_CORNER[nvm.asic_corner[a->address]]);
      ac_delta *= (a->asic_freq - MINIMAL_FREQ_PER_CORNER[nvm.asic_corner[a->address]]);
      if (vm.ac2dc_current > ac_delta) {
        vm.ac2dc_current -= ac_delta;
      } else {
        vm.ac2dc_current = 0;
      }
      if (vm.loop[l].dc2dc.dc_current_16s_of_amper  > dc_delta) {
        vm.loop[l].dc2dc.dc_current_16s_of_amper  -= dc_delta;
      } else {
        vm.loop[l].dc2dc.dc_current_16s_of_amper  = 0;
      }
      set_asic_freq(a->address, MINIMAL_FREQ_PER_CORNER[nvm.asic_corner[a->address]]);
    }
  }

  while (vm.ac2dc_current >= AC2DC_POWER_RED_LINE) {
    if (!vm.stopped_all_work) {
      changed = true;
      stop_all_work();
    }
    HAMMER *a = find_asic_to_reduce_ac_current();
    int ac_delta = AC_CURRNT_PER_15_HZ[nvm.loop_voltage[a->address/HAMMERS_PER_LOOP]]; 
    if (vm.ac2dc_current > ac_delta) {
      vm.ac2dc_current -= ac_delta;
    } else {
      vm.ac2dc_current = 0;
    }
    printf("AC2DC OVER LIMIT, throttling ASIC:%d %d!\n", a->address, a->asic_freq);
    set_asic_freq(a->address, MINIMAL_FREQ_PER_CORNER[nvm.asic_corner[a->address]]);
  }
  resume_all_work();
  if (changed) {
    usleep(TIME_FOR_DLL_USECS);
  }
  end_stopper(&tv, "DOWNSCALE");
#endif
}


bool can_be_upscaled(HAMMER *a) {
  if (!a->asic_present ||
      (a->asic_freq >= nvm.top_freq[a->address]) ||
      (vm.loop[a->address/HAMMERS_PER_LOOP].dc2dc.dc_spare_power < 1)) {
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

  if (nvm.asic_corner[a->address] != nvm.asic_corner[b->address]) {
    // Upscale lower corners because they have lower leakage
    return (nvm.asic_corner[a->address] < nvm.asic_corner[b->address]) ? a : b;
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
  if (vm.asics_shut_down_powersave) {
    return ret;
  }

  int now = time(NULL);
  int h, l;

  // first handle critical system AC2DC current
  vm.ac2dc_spare_current = ac2dc_spare_power();
  if (vm.ac2dc_spare_current < 1) {
    return ret;
  }

  // mark all ASICs we can increase speed
  for (l = 0; l < LOOP_COUNT; l++) {
    // Works for disabled loops too
    vm.loop[l].dc2dc.dc_spare_power = dc2dc_spare_power(l);
    for (h = 0; h < HAMMERS_PER_LOOP; h++) {
      int addr = l*HAMMERS_PER_LOOP + l;
      if (vm.hammer[addr].asic_present &&
          (vm.hammer[addr].asic_freq < nvm.top_freq[addr]) &&
          (vm.loop[l].dc2dc.dc_spare_power) &&
          (now - vm.hammer[addr].last_freq_increase_time) > ASIC_SPEED_RAISE_TIME) {
        vm.hammer[addr].can_scale_up = 1;
      } else {
        vm.hammer[addr].can_scale_up = 0;
      }
    }
  }
  
  HAMMER *a;
  while (vm.ac2dc_spare_current &&
        (a = find_asic_to_increase_speed())) {
    if (!vm.stopped_all_work) {
      ret = 1;
      stop_all_work();
    }
    try_increase_asic_freq(a, 
                           &vm.ac2dc_spare_current,
                           &vm.loop[a->address/HAMMERS_PER_LOOP].dc2dc.dc_spare_power);
    a->can_scale_up = 0;
  }
  resume_all_work();
  if (ret) {
    usleep(TIME_FOR_DLL_USECS);
  }
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
    vm.hammer[hi.addr].failed_bists = 0;
  }

  if (!do_bist_ok(false)) {
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
        h->address, nvm.asic_corner[h->address], h->enabled_engines_mask,
        h->asic_freq, nvm.top_freq[h->address], h->solved_jobs, h->temperature);
  } else {
    DBG(DBG_SCALING, "      HAMMER %04x ----\n", h->address);
  }
}

void print_loop(int l) {
  LOOP *loop = &vm.loop[l];
  DBG(DBG_SCALING, "   LOOP %x AMP:%d\n", l,
      loop->dc2dc.dc_current_16s_of_amper);
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
