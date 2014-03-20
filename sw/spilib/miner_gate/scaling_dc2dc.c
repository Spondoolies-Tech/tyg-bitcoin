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
  do_bist_ok_rt(0);
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
 
  if (vm.ac2dc_power >= AC2DC_POWER_LIMIT)
   {
    return 0;
  }

  if (vm.loop[a->loop_address].dc2dc.dc_current_16s > 
      vm.loop[a->loop_address].dc2dc.dc_current_limit_16s)
      return 0;

  return 1;
}
void asic_up(HAMMER *a) {
   if(a->freq_wanted < ASIC_FREQ_MAX) {
     ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->freq_wanted+1);
     a->freq_wanted = wanted_freq;
     //set_pll(a->address, wanted_freq);       
     a->last_freq_change_time = now;
   }
   vm.loop[a->loop_address].dc2dc.dc_current_16s += 4;
//   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[0] += 4;
//   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[1] += 4;
//   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[2] += 4;
//   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[3] += 4;
   vm.ac2dc_power++;
   vm.pll_changed=1;

}


int asic_can_down(HAMMER *a) {
  return (a && (a->freq_wanted> MINIMAL_ASIC_FREQ));
}



void asic_down_completly(HAMMER *a) {
   //passert(vm.engines_disabled == 1);
    int fdiff = 3 * ( a->freq_wanted - MINIMAL_ASIC_FREQ );
   ASIC_FREQ wanted_freq = MINIMAL_ASIC_FREQ;
   a->freq_wanted = wanted_freq;
   //set_pll(a->address, wanted_freq);       
   a->last_freq_change_time = now;
   a->agressivly_scale_up = 1;
/*
   vm.loop[a->loop_address].dc2dc.dc_current_16s -= fdiff;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[0] -= fdiff;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[1] -= fdiff;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[2] -= fdiff;
*/

   vm.pll_changed=1;
   
}


void asic_up_fast(HAMMER *a) {
   //passert(vm.engines_disabled == 1);
   ASIC_FREQ wanted_freq = a->freq_thermal_limit;
   int fdiff = 4 * ( a->freq_thermal_limit - a->freq_wanted);
   a->freq_wanted=a->freq_thermal_limit;

   vm.loop[a->loop_address].dc2dc.dc_current_16s += fdiff;
   /*
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[0] += fdiff;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[1] += fdiff;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[2] += fdiff;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[3] += fdiff;
   */
   vm.ac2dc_power+=fdiff/8;

   
   //set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;    
   vm.pll_changed=1;
}


void asic_down(HAMMER *a, int down) {
   //passert(vm.engines_disabled == 1);
   //printf(RED "xASIC DOWNSCALE %x!\n", a->address);
    
    a->freq_wanted  = (ASIC_FREQ)(a->freq_wanted-down);
    passert(a->freq_wanted >= MINIMAL_ASIC_FREQ);
    //set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;      
   
   vm.loop[a->loop_address].dc2dc.dc_current_16s -= 3;
   /*
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[0] -= 3;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[1] -= 3;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[2] -= 3;
   vm.loop[a->loop_address].dc2dc.dc_current_16s_arr[3] -= 3;
  */
   vm.pll_changed=1;
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
  if (a->agressivly_scale_up) return a;
  if (b->agressivly_scale_up) return b;


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
    stop_all_work_rt();
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


void do_bist_fix_loops_rt(int force) {
  static int counter = 0;  
  counter++; 

  
  if (!vm.asics_shut_down_powersave && !vm.thermal_test_mode) { 
      if (force  || 
         ((counter % BIST_PERIOD_SECS) == 0)) {
        printf(MAGENTA "Running BIST\n" RESET);

         struct timeval tv; 
         start_stopper(&tv);
         int failed = do_bist_ok_rt(1);
         end_stopper(&tv,"BIST");
         printf(MAGENTA "Bist failed %d times\n" RESET, failed);
         if (failed) {
           proccess_bist_results = 2;
         }
      }
  }

  //change_dc2dc_voltage_if_needed();
  //resume_asics_if_needed();
}



// Runs from non RT thread once a second
void maybe_change_freqs_nrt() {
  static int counter = 0;  
   now = time(NULL);
   vm.dc2dc_total_power = 0;
   vm.total_mhash = 0;
   int critical_downscale = 0;
  
   // Remove disabled loops and pdate statistics
   for (int l = 0 ; l < LOOP_COUNT ; l++) {
     if (!vm.loop[l].enabled_loop) {
        continue;
     }

     
     if (vm.loop[l].dc2dc.dc_current_16s >= vm.loop[l].dc2dc.dc_current_limit_16s) {
       critical_downscale=1;
       printf("Current critical %d!\n", l);
     }
  
     
     if (vm.loop[l].enabled_loop) {
       vm.loop[l].overheating_asics = 0;
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
             //printf("Running critical BIST for ASIC TEMP on %x\n", h->freq_wanted);
             critical_downscale=1;
           }
           vm.loop[l].asic_count++;
           vm.loop[l].asic_temp_sum += h->asic_temp*6+77;
           vm.loop[l].asic_hz_sum += h->freq_wanted*15+210;
           if (h->freq_bist_limit > h->freq_thermal_limit) {
             vm.loop[l].overheating_asics++; 
           }
         }
       }
       
       vm.dc2dc_total_power += vm.loop[l].dc2dc.dc_power_watts_16s;
       vm.total_mhash += vm.loop[l].asic_hz_sum*ENGINES_PER_ASIC;
     }
   }
   vm.dc2dc_total_power/=16;
   
  
   //return;
   counter++;
   // Run every XX seconds
   if (((counter%15) == 0) ||
       proccess_bist_results ||
       critical_downscale ||
       (vm.ac2dc_power > AC2DC_POWER_LIMIT)) {
       printf(MAGENTA "Running FREQ update\n" RESET);
       //!!!  HERE WE DO FREQUENCY UPDATES!!!
       asic_frequency_update_nrt();
       proccess_bist_results = 0;
       // Dont run for next 7 seconds.
   }
}






int loop_can_down(int l);
int loop_down(int l);



// Runs from LOW priority thread.
// every 7 seconds or so
void asic_frequency_update_nrt(int verbal) {    
    int now = time(NULL);
    int usec;
    int time_from_last_call;
    //struct timeval tv;
    int changed = 0;
    static int last_call = 0;
    static int counter = 0;

    if (last_call == 0) {
      last_call = now;
    }
    time_from_last_call = now - last_call;
    int cnt = 0;
    
    static int time_of_last_voltage_change = 0;
    if (time_of_last_voltage_change == 0) {
      time_of_last_voltage_change = now;
    }

    
    for (int l = 0 ; l < LOOP_COUNT ; l++) {
      if (!vm.loop[l].enabled_loop) {
        continue;
      }
      int upped_fast = 0;     
      
      for (int a = 0 ; a < HAMMERS_PER_LOOP; a++) {
        HAMMER *h = &vm.hammer[l*HAMMERS_PER_LOOP+a];
        if (!h->asic_present) {
          continue;
        }

        // This must be first!! We must have real data here.
        int passed = h->passed_last_bist_engines;        
        if ((passed != ALL_ENGINES_BITMASK)) {
          vm.loop[h->loop_address].asics_failing_bist=1;
          int failed_engines_mask = passed ^ ALL_ENGINES_BITMASK;
          cnt++;
          // It's not only thermaly punished, it's failing bist
          h->agressivly_scale_up = false;          
          //printf("ASIC bist work: %x %x:%x %x:%x\n",h->address,h->freq_hw,h->freq_wanted,h->working_engines,h->passed_last_bist_engines);

          //passert(h);
          if (asic_can_down(h)) {
            if (h->freq_wanted == h->freq_hw) {
              h->freq_thermal_limit = (ASIC_FREQ)(h->freq_wanted-1);
              h->freq_bist_limit = (ASIC_FREQ)(h->freq_wanted-1);
              asic_down(h); 
              //printf("Down with ASIC  %x %x:%x\n",h->address,h->freq_hw,h->freq_wanted);            
              changed++;
            } else {
              // remind thread!
              vm.pll_changed = 1;
            }
          } else {
            printf("Cant down ASIC %x %x:%x",h->address,h->freq_hw,h->freq_wanted);
          }
     
           h->passed_last_bist_engines = ALL_ENGINES_BITMASK;
        } 


        if (h->asic_temp >= MAX_ASIC_TEMPERATURE ) {
          if(h->freq_wanted > ASIC_FREQ_225) {
             // let it cool off
             //printf("TOO HOT:%x\n",h->address);
             h->freq_thermal_limit = (ASIC_FREQ)(h->freq_thermal_limit - 1);
             vm.loop[h->loop_address].crit_temp_downscale++;
             h->last_down_freq_change_time = now;
             asic_down_completly(h);
           }
         } else if (h->agressivly_scale_up && asic_can_up(h,0) && (upped_fast==0)) {
            if (vm.loop[l].dc2dc.dc_current_16s < vm.loop[l].dc2dc.dc_current_limit_16s - 16*1) {
                upped_fast++;
                //printf(MAGENTA "UPPER FAST WITH %d\n", h->address);
                asic_up_fast(h);
                changed++;
           } else {
               //printf(MAGENTA "UPPER NORMAL WITH %d", h->address);
               asic_up(h);
           }
         }
      // try upscale 1 asic in loop
      }

      if (vm.loop[l].dc2dc.dc_current_16s < vm.loop[l].dc2dc.dc_current_limit_16s - 10) {
         if (time_from_last_call > 4) {
           if ((counter%(LOOP_COUNT/2)) == 0) {
              HAMMER* hh = find_asic_to_up(l, 0);
              if (hh) {
                //printf("Upping %x\n", hh->address);
                  asic_up(hh);
                  changed++;
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
           changed++;
         }
      }
      
       last_call = now;
       //printf(MAGENTA "freq changed %d\n" MAGENTA, changed);
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


