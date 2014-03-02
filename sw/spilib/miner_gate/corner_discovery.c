
#include <stdio.h>
#include <stdlib.h>
#include "corner_discovery.h"
#include "scaling_manager.h"
#include <spond_debug.h>
#include "hammer.h"
#include "hammer_lib.h"
#include "squid.h"



DC2DC_VOLTAGE CORNER_TO_VOLTAGE_TABLE[ASIC_CORNER_COUNT] = {
  ASIC_VOLTAGE_765, ASIC_VOLTAGE_765, ASIC_VOLTAGE_720, ASIC_VOLTAGE_630,
  ASIC_VOLTAGE_630, ASIC_VOLTAGE_630
};



void discover_good_loops() {
  DBG(DBG_NET, "RESET SQUID\n");

  uint32_t good_loops = 0;
  int i, ret = 0, success;

  write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
  write_spi(ADDR_SQUID_LOOP_RESET, 0);
  write_spi(ADDR_SQUID_COMMAND, 0xF);
  write_spi(ADDR_SQUID_COMMAND, 0x0);
  write_spi(ADDR_SQUID_LOOP_BYPASS, 0);
  write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);

  success = test_serial(-1);
  if (success) {
    for (i = 0; i < LOOP_COUNT; i++) {
      // vm.loop[i].present = true;
      //nvm.loop_brocken[i] = false;
      ret++;
    }
  } else {
    for (i = 0; i < LOOP_COUNT; i++) {
      vm.loop[i].id = i;
      unsigned int bypass_loops = (~(1 << i) & 0xFFFFFF);
      write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
      //write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
      //write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
      printf("Testing loop::::::\n");
      if (test_serial(i)) {
        // printf("--00--\n");
        vm.loop[i].enabled_loop = 1;
        good_loops |= 1 << i;
        ret++;
      } else {
        // printf("--11--\n");
        vm.loop[i].enabled_loop = 0;
        for (int h = i * HAMMERS_PER_LOOP; h < (i + 1) * HAMMERS_PER_LOOP;
             h++) {
          // printf("remove ASIC 0x%x\n", h);
          vm.hammer[h].asic_present = 0;
          vm.working_engines[h] = 0;
          nvm.top_freq[h] = ASIC_FREQ_0;
          nvm.asic_corner[h] = ASIC_CORNER_NA;
        }
        int err;
        printf("Disabling DC2DC %d\n", i);
        dc2dc_disable_dc2dc(i, &err);
      }
    }
    write_spi(ADDR_SQUID_LOOP_BYPASS, ~(good_loops));
    vm.good_loops = good_loops;
    test_serial(-1);
  }
 
  printf("Found %d good loops\n", ret);
  passert(ret);
}

void test_asics_in_freq(ASIC_FREQ freq_to_pass, ASIC_CORNER corner_to_set) {
  printf(MAGENTA "TODO TODO testing freq!\n" RESET);
}


void find_bad_engines_update_nvm() {
  int i;
   
  if (!do_bist_ok()) {
    // IF FAILED BIST - reduce top speed of failed ASIC.
    printf("INIT BIST FAILED, reseting working engines bitmask!\n");
    hammer_iter hi;
    hammer_iter_init(&hi);

    while (hammer_iter_next_present(&hi)) {
      // Update NVM
      if (vm.working_engines[hi.addr] != hi.a->passed_last_bist_engines) {
        vm.working_engines[hi.addr] = hi.a->passed_last_bist_engines;
        nvm.dirty = 1;
        printf("After BIST setting %x enabled engines to %x\n", 
               hi.addr,
               vm.working_engines[hi.addr]);
      }
    }
    
  }
  set_nvm_dc2dc_voltage();
}

