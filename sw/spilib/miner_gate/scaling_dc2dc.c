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
static int proccess_bist_results = 0;



void do_bist() {
  //struct timeval tv;
  //start_stopper(&tv);
  
  resume_asics_if_needed();
  do_bist_ok(0);
  //end_stopper(&tv, "bist stopper");
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

  if (a->freq_wanted != b->freq_wanted) {
    // Reduce higher frequency
    return (a->freq_wanted > b->freq_wanted) ? a : b;
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
       !a->asic_present /*||
       (a->asic_temp >= HOT_ASIC_TEMPERATURE)*/) {
    return false;
  }

 
  if (a->freq_wanted >= a->freq_thermal_limit) {
    return false;
  }

  
  if ((now - a->last_freq_change_time) < TRY_ASIC_FREQ_INCREASE_PERIOD_SECS)
  {
     return false;
  }

  if (force) {
    return 1;
  }
  
  if ((vm.loop[a->loop_address].dc2dc.dc_temp > DC2DC_TEMP_GREEN_LINE) || 
      (vm.ac2dc_power >= AC2DC_POWER_LIMIT))
   {
    return 0;
  }

  return 1;
}


void asic_up(HAMMER *a) {
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->freq_wanted+1);
   a->freq_wanted = wanted_freq;
   //set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;
}


int asic_can_down(HAMMER *a) {
  return (a && a->freq_wanted > MINIMAL_ASIC_FREQ);
}


void asic_down_completly(HAMMER *a) {
   //passert(vm.engines_disabled == 1);
   ASIC_FREQ wanted_freq = MINIMAL_ASIC_FREQ;
   a->freq_wanted = wanted_freq;
   //set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;
   a->thermaly_punished = 1;
}


void asic_up_completly(HAMMER *a) {
   //passert(vm.engines_disabled == 1);
   ASIC_FREQ wanted_freq = MAX_ASIC_FREQ;
   a->freq_wanted = wanted_freq;
   //set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


void asic_down(HAMMER *a, int down) {
   //passert(vm.engines_disabled == 1);
   //printf(RED "xASIC DOWNSCALE %x!\n", a->address);
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->freq_wanted-down);
   a->freq_wanted = wanted_freq;
   //set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
}


// returns best asic
HAMMER *choose_asic_to_up(HAMMER *a, HAMMER *b, int force) {
  if (!a || !asic_can_up(a, force))
    return b;
  if (!b || !asic_can_up(b, force))
    return a;


/*
  if (a->asic_temp != b->asic_temp) {
    // Increase lower asic_temp because they have lower leakage
    return (a->asic_temp < b->asic_temp) ? a : b;
  }
*/
  if (a->thermaly_punished) return a;
  if (b->thermaly_punished) return b;


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

  if (a->freq_wanted != b->freq_wanted) {
    // Increase slower ASICs first
    return (a->freq_wanted < b->freq_wanted) ? a : b;
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
  counter++;
  
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    if (vm.loop[l].enabled_loop) {
     if (vm.loop[l].dc2dc.kill_me_i_am_bad) {
       // Kill ASICS in bad DC2DCs
       for (int addr = l*HAMMERS_PER_LOOP; addr < l*HAMMERS_PER_LOOP+HAMMERS_PER_LOOP; addr++) {
         disable_asic_forever(addr);
         vm.loop[l].dc2dc.kill_me_i_am_bad = 0;
       }
     }

     if (vm.loop[l].asic_count == 0) {
         int err;
         psyslog("xDisabling DC2DC %d\n", l);
         dc2dc_disable_dc2dc(l, &err);
         vm.loop[l].enabled_loop = 0;
         int loop_bit = 1 << l;
         vm.good_loops &= ~(loop_bit);
         write_spi(ADDR_SQUID_LOOP_BYPASS, ~(vm.good_loops));
     }
    }
  }     

  
  if (!vm.asics_shut_down_powersave && !vm.thermal_test_mode) { 
      if (force  || 
         ((counter % BIST_PERIOD_SECS) == 0)) {
         psyslog(MAGENTA "Running BIST\n" RESET);
         do_bist();
         proccess_bist_results = 1;
      }
  }

  //change_dc2dc_voltage_if_needed();
  //resume_asics_if_needed();
}



// Runs from non RT thread
void asic_scaling_compute_results() {
  static int counter = 0;  
   now = time(NULL);
   vm.dc2dc_total_power = 0;
   vm.total_mhash = 0;
   int critical_downscale = 0;
  
   // Remove disabled loops and pdate statistics
   for (int l = 0 ; l < LOOP_COUNT ; l++) {
     if (vm.loop[l].dc2dc.dc_current_16s > vm.loop[l].dc2dc.dc_current_limit_16s) {
       critical_downscale=1;
     }
  
     
     if (vm.loop[l].enabled_loop) {
       vm.loop[l].unused_frequecy = 0;
       vm.loop[l].asic_count = 0;
       vm.loop[l].asic_temp_sum = 0;
       vm.loop[l].asic_hz_sum = 0;
       vm.loop[l].power_throttled = 0;
       for (int i = 0 ; i < HAMMERS_PER_LOOP ; i++) {
         HAMMER* h = &vm.hammer[l*HAMMERS_PER_LOOP+i];
         if (h->asic_present) {
           if (h->asic_temp >= MAX_ASIC_TEMPERATURE && 
               h->freq_wanted > MINIMAL_ASIC_FREQ &&
               (now - h->last_down_freq_change_time) > 20) {
             printf("Running critical BIST for ASIC TEMP on %x\n", h->freq_wanted);
             critical_downscale=1;
           }
           vm.loop[l].asic_count++;
           vm.loop[l].asic_temp_sum += h->asic_temp*6+77;
           vm.loop[l].asic_hz_sum += h->freq_wanted*15+210;
           vm.loop[l].unused_frequecy += h->freq_bist_limit - h->freq_thermal_limit; 
         }
       }
       
       vm.dc2dc_total_power += vm.loop[l].dc2dc.dc_power_watts_16s;
       vm.total_mhash += vm.loop[l].asic_hz_sum*ENGINES_PER_ASIC;
     }
   }
   vm.dc2dc_total_power/=16;
   
  
   //return;
   counter++;
   if (((counter%5) == 0) ||
       proccess_bist_results ||
       critical_downscale ||
       vm.ac2dc_power > AC2DC_POWER_LIMIT) {
       psyslog(MAGENTA "Running FREQ update\n" RESET);
       asic_frequency_update();
       proccess_bist_results = 0;
   } 
  
   

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
        passert(h);
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


// Runs from LOW priority thread.
void asic_frequency_update(int verbal) {    
    int now = time(NULL);
    int usec;
    int time_from_last_call;
    //struct timeval tv;
    static int pool_to_up = 0;

    static int last_call = 0;
    
    static int counter = 0;
    printf(RED "FR UP!" RESET);

    if (last_call == 0) {
      last_call = now;
    }
    time_from_last_call = now - last_call;
    int cnt = 0;
    //pause_asics_if_needed();
    
    static int time_of_last_voltage_change = 0;
    if (time_of_last_voltage_change == 0) {
      time_of_last_voltage_change = now;
    }
    
    if (now - time_of_last_voltage_change > 60*2) {
      //printf("Do volt scaling :)\n");
      time_of_last_voltage_change = now;
      //ac2dc_scaling_one_minute();
    }

    
    for (int l = 0 ; l < LOOP_COUNT ; l++) {
      if (!vm.loop[l].enabled_loop) {
        continue;
      }
     
      for (int a = 0 ; a < HAMMERS_PER_LOOP; a++) {
        HAMMER *h = &vm.hammer[l*HAMMERS_PER_LOOP+a];
        if (!h->asic_present) {
          continue;
        }

       if (h->asic_temp >= MAX_ASIC_TEMPERATURE ) {
         if(h->freq_wanted > ASIC_FREQ_345) {
            // let it cool off
            h->freq_thermal_limit = h->freq_thermal_limit - 1;
            vm.loop[h->loop_address].crit_temp++;
            h->last_down_freq_change_time = now;
            asic_down_completly(h);
          }
        } else if (h->thermaly_punished && 
                    asic_can_up(h,0)) {
          asic_up(h);
        }

       

        
        int passed = h->passed_last_bist_engines;        
        if ((passed != ALL_ENGINES_BITMASK)) {
          vm.loop[h->loop_address].asics_failing_bist=1;
          
          int failed_engines_mask = passed ^ ALL_ENGINES_BITMASK;
          cnt++;

          

          //passert(h);
          if (asic_can_down(h)) {
            h->freq_thermal_limit = (ASIC_FREQ)(h->freq_wanted-1);
            h->freq_bist_limit = (ASIC_FREQ)(h->freq_wanted-1);
            asic_down(h);
          } else {
            h->freq_thermal_limit = ASIC_FREQ_660;
            h->freq_bist_limit = ASIC_FREQ_660;
            asic_up_completly(h);
            vm.hammer[h->address].working_engines = vm.hammer[h->address].working_engines&passed;
            if (vm.hammer[h->address].working_engines == 0) {
              // disable ASIC failing bist on all engines.
              disable_asic_forever(h->address);
            }
          }
     
          h->failed_bists = 0;    
          h->passed_last_bist_engines = ALL_ENGINES_BITMASK;
               
        } 
/*
        if (h->thermaly_punished && asic_can_up(h,0)) {
            asic_up(h);
        }
        // Increase ASIC speed not durring bring-up
*/
      }
      // try upscale 1 asic in loop

      
        
      if (vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s < 16) {
         printf("Downscaling for DC2DC");
         HAMMER* hh = find_asic_to_down(l);
         if(hh) {
           asic_down(hh);
         }
       } else {
       if (time_from_last_call > 4) {
         if ((counter%(LOOP_COUNT/2)) == 0) {
            HAMMER* hh = find_asic_to_up(l, 0);
            if (hh) {
                asic_up(hh);
            }
         }
       }
       }
    }

    
    if (vm.ac2dc_power >= AC2DC_POWER_LIMIT)  {
        int l = rand()%LOOP_COUNT;
        while (!vm.loop[l].enabled_loop) {
          l = rand()%LOOP_COUNT;
        }
         HAMMER *hh = find_asic_to_down(l);
         if(hh) {
           asic_down(hh);
         }
    }
      
       last_call = now;
     //end_stopper(&tv, "go over bad stopper");
     //resume_asics_if_needed();
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


