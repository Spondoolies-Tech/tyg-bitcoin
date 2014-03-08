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
#include <pthread.h>

static int now;



void do_bist() {
  //struct timeval tv;
  //start_stopper(&tv);
  
  resume_asics_if_needed();
  do_bist_ok(0);

  //end_stopper(&tv, "bist stopper");

}


void set_safe_voltage_and_frequency() {
  enable_voltage_freq(VTRIM_START, ASIC_FREQ_660);
  enable_good_engines_all_asics_ok(); 
}

/*
void enable_voltage_from_nvm() {
  int l, h, i = 0;
  // for each enabled loop
  
  disable_engines_all_asics();
  for (l = 0; l < LOOP_COUNT; l++) {
    if (vm.loop[l].enabled_loop) {
      // Set voltage
      int err;
      // dc2dc_set_voltage(l, vm.loop_vtrim[l], &err);
      dc2dc_set_vtrim(l, vm.loop_vtrim[l], &err);
      // passert(err);

      // for each ASIC
      for (h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
        HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
        // Set freq
        if (a->asic_present) {
          set_asic_freq(a->address, MAX_ASIC_FREQ);
        }
      }
    }
  }
  enable_good_engines_all_asics_ok();
}
*/


// returns worst asic
//  Any ASIC is worth then NULL
HAMMER *choose_asic_to_down(HAMMER *a, HAMMER *b) {
  if (!a || !asic_can_down(a))
    return b;
  if (!b || !asic_can_down(b))
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
HAMMER *find_asic_to_down(int l) {
  int h;
  // Find hottest ASIC at highest corner.
  HAMMER *best = NULL;
  for (h = 0; h < HAMMERS_PER_LOOP; h++) {
    HAMMER *a = &vm.hammer[l * HAMMERS_PER_LOOP + h];
    if (a->asic_present) {
     best = choose_asic_to_down(best, a);
    }
  }
  
  if(best && asic_can_down(best)) {
     return best;
  }
  return NULL;
}



int asic_can_up(HAMMER *a, int force) {
  if (!a ||
      !a->asic_present ||
      (a->asic_freq >= a->top_freq) ||
      /*(a->asic_temp >= HOT_ASIC_TEMPERATURE) ||*/
      (vm.loop[a->loop_address].dc2dc.dc_current_16s > vm.loop[a->loop_address].dc2dc.dc_current_limit_16s)) {
     return false;
  }

  if (force) {
    return true;
  }
  if ((vm.loop[a->loop_address].dc2dc.dc_temp > DC2DC_TEMP_GREEN_LINE) || 
      (vm.ac2dc_power >= AC2DC_POWER_LIMIT) || 
      (((now - a->last_freq_change_time) < TRY_ASIC_FREQ_INCREASE_PERIOD_SECS))
  ) {
    return 0;
  }

  return 1;
}


void asic_up(HAMMER *a) {
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq+1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


int asic_can_down(HAMMER *a) {
  return (a && a->asic_freq > MINIMAL_ASIC_FREQ);
}


void asic_down_completly(HAMMER *a) {
   passert(vm.engines_disabled == 1);
   ASIC_FREQ wanted_freq = MINIMAL_ASIC_FREQ;
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


void asic_down_one(HAMMER *a) {
   passert(vm.engines_disabled == 1);
   //printf(RED "xASIC DOWNSCALE %x!\n", a->address);
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq-1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


// returns best asic
HAMMER *choose_asic_to_up(HAMMER *a, HAMMER *b, int force) {
  if (!a || !asic_can_up(a, force))
    return b;
  if (!b || !asic_can_up(b, force))
    return a;



  if (a->asic_temp != b->asic_temp) {
    // Increase lower asic_temp because they have lower leakage
    return (a->asic_temp < b->asic_temp) ? a : b;
  }


  if (a->last_freq_change_time != b->last_freq_change_time) {
    // Increase lower asic_temp because they have lower leakage
    return (a->last_freq_change_time < b->last_freq_change_time) ? a : b;
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


HAMMER *find_asic_to_up(int l, int force) {
  HAMMER *best = NULL;
  for (int h = 0; h < HAMMERS_PER_LOOP ; h++) { 
    HAMMER *a = &vm.hammer[l*HAMMERS_PER_LOOP+h];
    if (a->asic_present) {
      best = choose_asic_to_up(best, a, force);
    }
  }

  if(best && asic_can_up(best, force))
    return best;
  return NULL;
}

void pause_asics_if_needed() {
  if (vm.engines_disabled == 0) {
    //printf("Disabling hehe\n");
    stop_all_work();
    disable_engines_all_asics();
  }
}


void resume_asics_if_needed() {
  if (vm.engines_disabled != 0) {
    //printf("Resuming hehe\n");
    enable_good_engines_all_asics_ok();
    resume_all_work();
  }
}

void asic_scaling_once_second(int force) {
  static int counter = 0;  
  now = time(NULL);
  struct timeval tv;
  start_stopper(&tv);
  vm.dc2dc_total_power = 0;
  vm.total_mhash = 0;

  // Remove disabled loops and pdate statistics
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    if (vm.loop[l].enabled_loop) {
      vm.loop[l].asic_count = 0;
      vm.loop[l].asic_temp_sum = 0;
      vm.loop[l].asic_hz_sum = 0;
      for (int i = 0 ; i < HAMMERS_PER_LOOP ; i++) {
        HAMMER* h = &vm.hammer[l*HAMMERS_PER_LOOP+i];
        if (h->asic_present) {
          vm.loop[l].asic_count++;
          vm.loop[l].asic_temp_sum += h->asic_temp*6+77;
          vm.loop[l].asic_hz_sum += h->asic_freq*15+210;
        }
      }
      vm.dc2dc_total_power += vm.loop[l].dc2dc.dc_power_watts_16s;
      vm.total_mhash += vm.loop[l].asic_hz_sum*ENGINES_PER_ASIC;
      if (vm.loop[l].asic_count == 0) {
        vm.loop[l].enabled_loop = 0;
      }
    }
  }
  vm.dc2dc_total_power/=16;
  end_stopper(&tv, "SCALING1");
  

  //return;
  start_stopper(&tv);
  counter++;
  if (!vm.asics_shut_down_powersave) { 
      //Once every 10 seconds upscale ASICs if can
      if (force  || 
         ((counter % BIST_PERIOD_SECS) == 0)) {
         //printf("bbb\n");
         do_bist();
      }
            
      if (force  || 
        ((counter % BIST_PERIOD_SECS) == 1)) {
         asic_frequency_update();
      } 

  }
  end_stopper(&tv, "SCALING2");

  //change_dc2dc_voltage_if_needed();
  //resume_asics_if_needed();
}

#if 0
void change_dc2dc_voltage_if_needed() {
  int any_scaled = 0;
  int err;
  // DOWNSCALE!!!!
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    if (!vm.loop[l].enabled_loop) {
      continue;
    }
    if ((((vm.loop[l].dc2dc.dc_current_16s)) >= vm.loop[l].dc2dc.dc_current_limit_16s)
        // When ASICs not fail bist for a long time, try to bring down voltage)
  
        ) {
      // Prefer to downscale VOLTAGE
      if (/*vm.loop_vtrim[l] > VTRIM_MIN*/ 0) { //TODOZ
        printf(MAGENTA "LOOP DOWNSCALE VTRIM %d\n" RESET, l);
        dc2dc_set_vtrim(l,vm.loop_vtrim[l]-1,&err);
        vm.loop[l].asics_failing_bist=0;
      } else { 
        printf(RED "LOOP DOWNSCALE %d\n" RESET, l);
        HAMMER *h = find_asic_to_down(l);
        assert(h);
        printf(RED "Starin ASIC DOWNSCALE %d\n" RESET, h->address);
        pause_asics_if_needed();
        asic_down_one(h);
      }
    }

    // UPSCALE!!!!
#if 0
    if ((vm.loop[l].dc2dc.dc_current_16s != 0) &&
        (!vm.scaling_up_system) &&
        (now - vm.start_mine_time > DC2DC_UPSCALE_TIME_AFTER_BOOT_SECS) && // XX seconds after mining 
        ((now - vm.loop[l].dc2dc.last_voltage_change_time) > DC2DC_UPSCALE_TIME_SECS) && // 30 seconds after last voltage change
        // TODO - add AD2DC constains
         (vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING) &&
         (vm.loop[l].dc2dc.dc_current_16s < vm.loop[l].dc2dc.dc_current_limit_16s - DC2DC_SAFE_TO_INCREASE_CURRENT_16S) &&
         /*(vm.loop[l].asic_temp_sum/vm.loop[l].asic_count < TOP_TEMPERATURE_FOR_VOLTAGE_SCALING_C) && */
          vm.loop[l].asics_failing_bist) {
      if (vm.loop_vtrim[l] < VTRIM_MAX) {
        printf(MAGENTA "RAISING VTRIM DC2DC %d\n" RESET,l);
        dc2dc_set_vtrim(l,vm.loop_vtrim[l]+1,&err);
        vm.loop[l].asics_failing_bist = 0;
      }
    }
#endif
    if (any_scaled) {
      // don't scale till next measurement
      vm.loop[l].dc2dc.dc_current_16s = 0;
    }
  }
}
#endif



void asic_frequency_update(int verbal) {    
    int now = time(NULL);
    int usec;
    //struct timeval tv;
    static int pool_to_up = 0;
    pool_to_up++;
    static int counter = 0;
 // printf(RED "BIST!" RESET);
  //  end_stopper(&tv,"BIST PART");
  //  start_stopper(&tv);
    int cnt = 0;
    pause_asics_if_needed();
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
        if (h->asic_temp >= MAX_ASIC_TEMPERATURE) {
          if (asic_can_down(h)) {
            h->top_freq = h->asic_freq-1;
            // let it cool off
            asic_down_completly(h);
          }
        }

        
        
        if ((passed != ALL_ENGINES_BITMASK)) {
          vm.loop[h->loop_address].asics_failing_bist=1;
          
          int failed_engines_mask = passed ^ ALL_ENGINES_BITMASK;
          // int failed_engines_count = count_ones(failed_engines_mask);
          //printf(RED "Failed asic %x engines passed %x, temp %d\n" RESET, h->address, passed,h->asic_temp*6+77);
          cnt++;

          if (verbal) {
            printf("%x->%d ",h->address,h->asic_freq);
          }

          //assert(h);
          if (asic_can_down(h)) {
            h->top_freq = h->asic_freq-1;
            asic_down_one(h);
          } else {
            h->top_freq = MAX_ASIC_FREQ;
            vm.working_engines[h->address] = vm.working_engines[h->address]&passed;
            if (vm.working_engines[h->address] == 0) {
              // disable ASIC failing bist on all engines.
              disable_asic_forever(h->address);
            }
          }
     
          h->failed_bists = 0;    
          h->passed_last_bist_engines = ALL_ENGINES_BITMASK;
               
        } 
        // Increase ASIC speed not durring bring-up
      }
      // try upscale 1 asic in loop

      
      HAMMER *hh;
      // grow faster when LOW_DC2DC_POWER 
      if ((vm.loop[l].dc2dc.dc_power_watts_16s < LOW_DC2DC_POWER) ||
          (counter%LOOP_COUNT==l)) {
        hh = find_asic_to_up(l, 0);
        if (hh) {
          asic_up(hh);
        }
      }

      if (vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s < 16) {
         hh = find_asic_to_down(l);
         if(hh) {
           asic_down_one(hh);
         }
         if (vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s < 32) {
            hh = find_asic_to_down(l);
            if(hh) {
              asic_down_one(hh);
            }
         }
      }
     
    }
    
     printf(CYAN "Failed in bist %d ASICs\n" RESET, cnt);
     //end_stopper(&tv, "go over bad stopper");
     resume_asics_if_needed();
#if 0

      if (vm.loop[l].dc2dc.dc_current_16s >= vm.loop[l].dc2dc.dc_current_limit_16s) {
        hh = find_asic_to_down(l);
         if(hh) {
           asic_down_one(hh);
         }
      }
    }
    

      if ((counter++%10) == 0) {
        hh = find_asic_to_up(l, 1);
        if (hh) {
          asic_up(hh);
        }
         
        hh = find_asic_to_down(l);
        if(hh) {
          asic_down_one(hh);
        }
      }
    }
#endif   
}  


