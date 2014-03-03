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




int loop_can_down(int a) {
#if 0  
  return (a->asic_freq > ASIC_FREQ_225);
#endif
  return 0;
}


void loop_down(int a, time_t now) {
#if 0  
   passert(vm.engines_disabled == 1);
   printf(RED "xASIC DOWNSCALE %x!\n", a->address);
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq-1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = now;   
#endif   
}


// returns worst asic
//  Any ASIC is worth then NULL
int choose_loop_to_down(int a, int b) {
#if 0
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
#endif 
}

// return worst loop ID
int find_loop_to_down() {
  int h;
  // Find hottest ASIC at highest corner.
  /*
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
  */
}



int loop_can_up(int l) {
#if 0
  if (!a->asic_present ||
      (a->asic_freq >= MAX_ASIC_FREQ) ||
      (a->asic_temp >= MAX_ASIC_TEMPERATURE) 
      /*|| ((vm.loop[a->loop_address].dc2dc.dc_current_16s + DC2DC_KEEP_FOR_LEAKAGE_16S)
            >= nvm.top_dc2dc_current_16s[a->loop_address]) ||
      (vm.loop[a->loop_address].dc2dc.dc_temp>= DC2DC_TEMP_GREEN_LINE) || 
      (vm.ac2dc_current >= AC2DC_CURRENT_GREEN_LINE) ||
      (vm.ac2dc_temp >= AC2DC_TEMP_GREEN_LINE)*/)
  {
    return 0;
  }

   if ((time(NULL) - a->last_freq_change_time < TRY_ASIC_FREQ_INCREASE_PERIOD_SECS) && 
        !vm.scaling_up_system) {
     return 0;
  }

#endif
  return 1;
}


void loop_up(HAMMER *a) {
#if 0  
   ASIC_FREQ wanted_freq = (ASIC_FREQ)(a->asic_freq+1);
   a->asic_freq = wanted_freq;
   set_pll(a->address, wanted_freq);        
   a->last_freq_change_time = time(NULL);      
#endif   
}


// returns best loop or -1
int choose_loop_to_up(int a, int b) {
#if 0
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
#endif  
  return a;
}


int find_loop_to_up() {
#if 0  
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
#endif 
  return 0;
}



void ac2dc_scaling_one_second() {

}

