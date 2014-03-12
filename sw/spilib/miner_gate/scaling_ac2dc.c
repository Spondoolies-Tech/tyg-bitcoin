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

static int now; // cahce time

int loop_can_down(int l) {
  if (l == -1)
      return 0;
  
  return  
     (vm.loop[l].enabled_loop && 
     (vm.loop_vtrim[l] > VTRIM_MIN) &&
     (now - vm.loop[l].last_ac2dc_scaling_on_loop > AC2DC_SCALING_SAME_LOOP_PERIOD_SECS));
}


void loop_down(int l) {
  int err;
   printf("vtrim=%x\n",vm.loop_vtrim[l]);
   dc2dc_set_vtrim(l, vm.loop_vtrim[l]-1, &err);
   vm.loop[l].last_ac2dc_scaling_on_loop  = now;

   for (int h = l*HAMMERS_PER_LOOP; h< l*HAMMERS_PER_LOOP+HAMMERS_PER_LOOP;h++) {
    if (vm.hammer[h].asic_present) {
      if (vm.hammer[h].freq_thermal_limit < ASIC_FREQ_MAX-1) 
        {
          vm.hammer[h].freq_thermal_limit = vm.hammer[h].freq_bist_limit;
          /*
          if (vm.hammer[h].freq_thermal_limit > MINIMAL_ASIC_FREQ + 1) {
            asic_down(&vm.hammer[h],1);
          }
          */
        }
      }
    }
   
}



int loop_can_up(int l) {
  if (l == -1)
      return 0;
  
  return  
    (vm.loop[l].enabled_loop &&
    (vm.loop_vtrim[l] < VTRIM_HIGH) &&
    ((now - vm.loop[l].last_ac2dc_scaling_on_loop) > AC2DC_SCALING_SAME_LOOP_PERIOD_SECS));
 
}


void loop_up(int l) {
  int err;  
  //printf("1\n");
  dc2dc_set_vtrim(l, vm.loop_vtrim[l]+1, &err);
  vm.loop[l].last_ac2dc_scaling_on_loop  = now;
   //printf("3\n");
  for (int h = l*HAMMERS_PER_LOOP; h< l*HAMMERS_PER_LOOP+HAMMERS_PER_LOOP;h++) {
    if (vm.hammer[h].asic_present) {
      if (vm.hammer[h].freq_thermal_limit < ASIC_FREQ_MAX-1) 
        {
          vm.hammer[h].freq_thermal_limit = vm.hammer[h].freq_thermal_limit+2;
          vm.hammer[h].freq_bist_limit    = vm.hammer[h].freq_thermal_limit;
          //set_asic_freq(h,vm.hammer[h].freq_thermal_limit - 4);
          asic_down(&vm.hammer[h],2);
        }
      }
    }
}


// returns best loop or -1
/*
int choose_loop_to_up(int a, int b) {

  if ((a==-1) || !loop_can_up(a))
    return b;
  if ((b==-1) || !loop_can_up(b))
    return a;


  int performance_a = vm.loop[a].asic_hz_sum /vm.loop[a].dc2dc.dc_power_watts_16s;
  int performance_b = vm.loop[b].asic_hz_sum /vm.loop[b].dc2dc.dc_power_watts_16s;
  
  return (performance_a > performance_b)?a:b;
}


int find_loop_to_up() {
  int best = -1;
  for (int l = 0; l < LOOP_COUNT; l++) { 
    if (vm.loop[l].enabled_loop) {
      best = choose_loop_to_up(best, l);
    }
  }

  if((best!=-1) && loop_can_up(best))
    return best;
  return -1;
}
*/



int asic_frequency_update_fast() {    
  pause_asics_if_needed();
  int one_ok = 0;
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
      if ((passed == ALL_ENGINES_BITMASK)) {
          one_ok = 1;
          h->freq_wanted= h->freq_wanted+1;
          h->freq_thermal_limit = h->freq_wanted;
          h->freq_bist_limit = h->freq_wanted;          
          set_pll(h->address, h->freq_wanted);    
      } else {
        h->failed_bists = 0;    
        h->passed_last_bist_engines = ALL_ENGINES_BITMASK;
      }
    }
  }
  return one_ok; 
}



void set_working_voltage_discover_top_speeds() {
  ASIC_FREQ n = ASIC_FREQ_225;
  int one_ok;
  enable_voltage_freq(ASIC_FREQ_225);
  do {
    n = n + 1;
    resume_asics_if_needed();
    do_bist_ok(0);
    one_ok = asic_frequency_update_fast();
 } while (one_ok);
 resume_asics_if_needed();
 //assert(0);
}







// Called from low-priority thread.
void ac2dc_scaling_one_minute() {
  for (int l = 0; l < LOOP_COUNT; l++) {
     int changed = 0;
    now=time(NULL);
    if ((!vm.asics_shut_down_powersave) && (vm.loop[l].enabled_loop) &&
         (vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING)) {
      
        int temperature_high = (vm.loop[l].asic_temp_sum / vm.loop[l].asic_count > 110);
        int too_hot_for_frequency = (vm.loop[l].unused_frequecy > 10);
        int fully_utilized = (vm.loop[l].unused_frequecy == 0); // h->freq_thermal_limit - h->freq
        int free_current = (vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s);
        int tmp_scaled=0;
       
       
        //printf(CYAN "ac2dc %d %d %d!\n" RESET, l, vm.loop[l].unused_frequecy, free_current);

         if (free_current >= 3*16) {
          // has unused freq - scale down.
         if (vm.loop[l].unused_frequecy > 3) {
            psyslog(CYAN "LOOP DOWN:%d\n" RESET, l);
            if (loop_can_down(l)) {
              changed = 1;
              loop_down(l);
            }
         } else if (vm.loop[l].unused_frequecy == 0  && 
                    !temperature_high &&
                    vm.ac2dc_power < AC2DC_POWER_LIMIT) 
            // scale up
                psyslog(CYAN "LOOP UP:%d\n" RESET, l);
                if (loop_can_up(l)) {
                  changed = 1;
                  loop_up(l);
                  vm.ac2dc_power += 10;
                  for (int i = 0; i < HAMMERS_PER_LOOP; i++) {
                      vm.hammer[i+l*HAMMERS_PER_LOOP].try_higher = 1;
                    }
                  }
         }
          }
         vm.loop[l].dc2dc.last_voltage_change_time = time(NULL);
       }
    }


