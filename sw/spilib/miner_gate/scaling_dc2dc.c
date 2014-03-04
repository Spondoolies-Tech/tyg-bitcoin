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

static int now;

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
    dc2dc_set_vtrim(l, VTRIM_START, &err);
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
          set_asic_freq(a->address, ASIC_FREQ_SAFE);
        }
      }
    }
  }
  enable_nvm_engines_all_asics_ok();
}



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
     printf("%p %p\n",a,best);
     best = choose_asic_to_down(best, a);
    }
  }
  printf("%p>, %x\n",best, best->address);
  if(asic_can_down(best)) {
     printf("%p>, %x\n",best, best->address);
     return best;
  }
  return NULL;
}



int asic_can_up(HAMMER *a) {
  if (!a->asic_present ||
      (a->asic_freq >= MAX_ASIC_FREQ) ||
      (a->asic_temp >= MAX_ASIC_TEMPERATURE) 
      /*|| ((vm.loop[a->loop_address].dc2dc.dc_current_16s + DC2DC_KEEP_FOR_LEAKAGE_16S)
            >= nvm.top_dc2dc_current_16s[a->loop_address]) ||
      (vm.loop[a->loop_address].dc2dc.dc_temp>= DC2DC_TEMP_GREEN_LINE) || 
      */ || 
      (vm.ac2dc_current >= AC2DC_CURRENT_LIMIT))
  {
    return 0;
  }

   if (((now - a->last_freq_change_time) < TRY_ASIC_FREQ_INCREASE_PERIOD_SECS) && 
        !vm.scaling_up_system) {
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
  return (a->asic_freq > ASIC_FREQ_225);
}


void asic_down(HAMMER *a) {
   passert(vm.engines_disabled == 1);
   printf(RED "xASIC DOWNSCALE %x!\n", a->address);
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq-1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


// returns best asic
HAMMER *choose_asic_to_up(HAMMER *a, HAMMER *b) {
  if (!a || !asic_can_up(a))
    return b;
  if (!b || !asic_can_up(b))
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


HAMMER *find_asic_to_up(int l) {
  HAMMER *best = NULL;
  for (int h = 0; h < HAMMERS_PER_LOOP ; h++) { 
    HAMMER *a = &vm.hammer[l*HAMMERS_PER_LOOP+h];
    if (a->asic_present) {
      best = choose_asic_to_up(best, a);
    }
  }

  if(best && asic_can_up(best))
    return best;
  return NULL;
}

void pause_asics_if_needed() {
  if (vm.engines_disabled == 0) {
    stop_all_work();
    disable_engines_all_asics();
  }
}


void resume_asics_if_needed() {
  if (vm.engines_disabled != 0) {
    usleep(200); // needed? TODO
    enable_nvm_engines_all_asics_ok();
    resume_all_work();
  }
}


void dc2dc_scaling_once_second() {
  static int counter = 0;  
  now = time(NULL);
  struct timeval tv;
  start_stopper(&tv);

  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    if (vm.loop[l].enabled_loop) {
      vm.loop[l].asic_count = 0;      
      vm.loop[l].asic_temp_sum = 0;
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
    }
  }


  ac2dc_scaling_one_second();

  counter++;
  change_dc2dc_voltage_if_needed();

  if (!vm.asics_shut_down_powersave) {    
      //Once every 10 seconds upscale ASICs if can
      if ( (((counter % BIST_PERIOD_SECS_RAMPUP) == 0) && vm.scaling_up_system) || 
           ((counter % BIST_PERIOD_SECS) == 0)) {
         asic_frequency_update_with_bist();
      }
  }
  resume_asics_if_needed();
  end_stopper(&tv, "SCALING");

}


void change_dc2dc_voltage_if_needed() {
  int any_scaled = 0;
  int err;
  printf(MAGENTA "change_dc2dc_voltage_if_needed 0\n" RESET);
  // DOWNSCALE!!!!
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    if (!vm.loop[l].enabled_loop) {
      continue;
    }
    
    if (((vm.loop[l].dc2dc.dc_current_16s))
              >= vm.loop[l].dc2dc.dc_current_limit_16s) {
      printf(MAGENTA "change_dc2dc_voltage_if_needed 3\n" RESET);
      // Prefer to downscale VOLTAGE
      if (vm.loop_vtrim[l] > VTRIM_MIN) {
        printf(MAGENTA "LOOP DOWNSCALE VTRIM %d\n" RESET, l);
        dc2dc_set_vtrim(l,vm.loop_vtrim[l]-1,&err);
      } else {
        printf(RED "LOOP DOWNSCALE %d\n" RESET, l);
        HAMMER *h = find_asic_to_down(l);
        assert(h);
        printf(RED "Starin ASIC DOWNSCALE %d\n" RESET, h->address);
        asic_down(h);
      }
    }

    // UPSCALE!!!!
    if ((vm.loop[l].dc2dc.dc_current_16s != 0) &&
        (!vm.scaling_up_system) &&
        (vm.start_mine_time > DC2DC_UPSCALE_TIME_SECS) && // 30 seconds after mining 
        ((now - vm.loop[l].dc2dc.last_voltage_change_time) > DC2DC_UPSCALE_TIME_SECS) && // 30 seconds after last voltage change
        // TODO - add AD2DC constains
         (vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING) &&
         (vm.loop[l].dc2dc.dc_current_16s < 
              vm.loop[l].dc2dc.dc_current_16s - DC2DC_SAFE_TO_INCREASE_CURRENT_16S)) {
      if (vm.loop_vtrim[l] < VTRIM_MAX) {
        printf(MAGENTA "RAISING VTRIM DC2DC %d\n" RESET,l);
        dc2dc_set_vtrim(l,vm.loop_vtrim[l]+1,&err);
      }
    }

    if (any_scaled) {
      // don't scale till next measurement
      vm.loop[l].dc2dc.dc_current_16s = 0;
    }
  }
  
}





void asic_frequency_update_with_bist() {    
    pause_asics_if_needed();
    int usec;
    printf(RED "BIST!" RESET);
    hammer_iter hi;
    hammer_iter_init(&hi);
    while (hammer_iter_next_present(&hi)) {
      hi.a->failed_bists = 0;    
      hi.a->passed_last_bist_engines = ALL_ENGINES_BITMASK;
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
              now - h->last_freq_change_time > HOT_ASIC_FREQ_DECREASE_PERIOD_SECS)) {
          vm.scaling_up_system = 0;

          int failed_engines_mask = passed ^ ALL_ENGINES_BITMASK;
          // int failed_engines_count = count_ones(failed_engines_mask);
          printf(RED "Failed asic %x engines passed %x, temp %d\n" RESET, h->address, passed);
          
          if (asic_can_down(h)) {
            asic_down(h);
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
      HAMMER *hh = find_asic_to_up(l);
      if (hh) {
        if (hh->asic_freq == MAX_ASIC_FREQ &&
            hh->asic_temp == ASIC_TEMP_77) {
          printf(RED "Something wrong with ASIC %x, kill it\n", hh->address);
          //disable_asic_forever(hh->address);
        }
        asic_up(hh);
      }   
    }
}  


