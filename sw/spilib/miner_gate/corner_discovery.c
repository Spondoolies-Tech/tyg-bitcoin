/*
 * Copyright 2014 Zvi (Zvisha) Shteingart - Spondoolies-tech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 *
 * Note that changing this SW will void your miners guaranty
 */

#include <stdio.h>
#include <stdlib.h>
#include "corner_discovery.h"
#include "scaling_manager.h"
#include <spond_debug.h>
#include "hammer.h"
#include "hammer_lib.h"
#include "squid.h"
#include "pll.h"


void enable_voltage_freq(ASIC_FREQ f) {
  int l, h, i = 0;
  // for each enabled loop
  
  disable_engines_all_asics();
  for (l = 0; l < LOOP_COUNT; l++) {
    if (vm.loop[l].enabled_loop) {
      // Set voltage
      int err;
      // dc2dc_set_voltage(l, vm.loop_vtrim[l], &err);
      if (vm.thermal_test_mode) {
        dc2dc_set_vtrim(l, VTRIM_674, &err);
      } else if (vm.silent_mode) {
        dc2dc_set_vtrim(l, VTRIM_MIN, &err);
      } else {
        dc2dc_set_vtrim(l, vm.loop_vtrim[l], &err);
      }

      // for each ASIC
      for (h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
        HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
        // Set freq
        if (a->asic_present) {
          set_pll(a->address, f);
        }
      }
    }
  }
  enable_good_engines_all_asics_ok();
}




void set_safe_voltage_and_frequency() {
  enable_voltage_freq(ASIC_FREQ_405);
  enable_good_engines_all_asics_ok(); 
}




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

 
  for (i = 0; i < LOOP_COUNT; i++) {
    vm.loop[i].id = i;
    unsigned int bypass_loops = (~(1 << i) & 0xFFFFFF);
    write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
    if (test_serial(i)) {
      vm.loop[i].enabled_loop = 1;
      if (vm.thermal_test_mode) {
        vm.loop[i].dc2dc.max_vtrim_currentwise = VTRIM_674;
        vm.loop_vtrim[i] = VTRIM_674;
      } else {
        vm.loop[i].dc2dc.max_vtrim_currentwise = vm.vtrim_max;
        vm.loop_vtrim[i] = vm.vtrim_start;
      }
      vm.loop[i].dc2dc.dc_current_limit_16s = DC2DC_INITIAL_CURRENT_16S;
      good_loops |= 1 << i;
      ret++;
    } else {
      vm.loop[i].enabled_loop = 0;
      vm.loop[i].dc2dc.max_vtrim_currentwise = 0;
      vm.loop_vtrim[i] = 0;
      for (int h = i * HAMMERS_PER_LOOP; h < (i + 1) * HAMMERS_PER_LOOP; h++) {
        vm.hammer[h].asic_present = 0;
        vm.hammer[h].working_engines = 0;
      }
      int err;
      printf("Disabling DC2DC %d\n", i);
      dc2dc_disable_dc2dc(i, &err);
    }
  }
  write_spi(ADDR_SQUID_LOOP_BYPASS, ~(good_loops));
  vm.good_loops = good_loops;
  test_serial(-1); 
  psyslog("Found %d good loops\n", ret);
  passert(ret);
}



