
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
      // passert(err);
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


const char* corner_to_collor(ASIC_CORNER c) {
  const static char* color[] = {RESET, CYAN_BOLD ,CYAN, GREEN,  RED, RED_BOLD};
  return color[c];
}

/*
void compute_corners() {
  enable_voltage_freq(ASIC_FREQ_810);
  hammer_iter hi;
  hammer_iter_init(&hi);

  int bist_ok;
  do {
    resume_asics_if_needed();
    bist_ok = do_bist_ok_rt(0);
    pause_asics_if_needed();
    asic_frequency_update_nrt();
 } while (!bist_ok);

   while (hammer_iter_next_present(&hi)) {
      if (hi.a->freq >= CORNER_DISCOVERY_FREQ_FF)
        hi.a->corner = ASIC_CORNER_FFG;
      else if (hi.a->freq >= CORNER_DISCOVERY_FREQ_TF)
        hi.a->corner = ASIC_CORNER_TTFFG;
      else if (hi.a->freq >= CORNER_DISCOVERY_FREQ_TT)
        hi.a->corner = ASIC_CORNER_TT;
      else if (hi.a->freq >= CORNER_DISCOVERY_FREQ_TS)
        hi.a->corner = ASIC_CORNER_SSTT;
      else 
        hi.a->corner = ASIC_CORNER_SS;
      
   }
   
   resume_asics_if_needed();
}
*/

/*
int set_working_voltage_discover_top_speeds();

void set_working_voltage_discover_top_speeds() {
   set_working_voltage_discover_top_speeds();
}
*/



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
    //write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
    //write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
    //printf("Testing loop::::::\n");
    if (test_serial(i)) { // TODOZ
      // printf("--00--\n");
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
      // printf("--11--\n");
      vm.loop[i].enabled_loop = 0;
      vm.loop[i].dc2dc.max_vtrim_currentwise = 0;
      vm.loop_vtrim[i] = 0;
      for (int h = i * HAMMERS_PER_LOOP; h < (i + 1) * HAMMERS_PER_LOOP; h++) {
        // printf("remove ASIC 0x%x\n", h);
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
  // Devide current between availible loops
  
  

  
  passert(ret);
}



