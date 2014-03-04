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
    int spare = vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s;
  return  
     (vm.loop[l].enabled_loop && (spare < (2*16)) &&
     (now - vm.loop[l].last_ac2dc_scaling_on_loop > AC2DC_SCALING_SAME_LOOP_PERIOD_SECS) &&
     (vm.loop[l].dc2dc.dc_current_limit_16s > AC2DC_CURRENT_MINIMAL_FOR_DOWNSCALE_16S));
}


void loop_down(int l) {
  vm.loop[l].last_ac2dc_scaling_on_loop  = now;
  vm.loop[l].dc2dc.dc_current_limit_16s -= AC2DC_CURRENT_JUMP_16S;
}


// returns worst asic
//  Any ASIC is worth then NULL
int choose_loop_to_down(int a, int b) {
  if ((a==-1) || !loop_can_down(a))
    return b;
  if ((b==-1) || !loop_can_down(b))
    return a;

  // Always return coldest loop
  if (vm.loop[a].asic_temp_sum != vm.loop[b].asic_temp_sum) {
    // Increase lower asic_temp because they have lower leakage
    return (vm.loop[a].asic_temp_sum > vm.loop[b].asic_temp_sum) ? a : b;
  }

  return a;
}

// return worst loop ID
int find_loop_to_down() {
  int best = -1;
  for (int l = 0; l < LOOP_COUNT; l++) { 
    if (vm.loop[l].enabled_loop) {
      best = choose_loop_to_down(best, l);
    }
  }

  if((best!=-1) && loop_can_down(best))
    return best;
  return 0;
}



int loop_can_up(int l) {
  int spare = vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s;
  return  
    (vm.loop[l].enabled_loop &&  (spare < (2*16)) &&
    (now - vm.loop[l].last_ac2dc_scaling_on_loop > AC2DC_SCALING_SAME_LOOP_PERIOD_SECS) &&
    (vm.loop[l].dc2dc.dc_current_limit_16s < nvm.top_dc2dc_current_16s[l]));
 
}


void loop_up(int l) {
   vm.loop[l].dc2dc.dc_current_limit_16s += AC2DC_CURRENT_JUMP_16S;
   vm.loop[l].last_ac2dc_scaling_on_loop  = now;
}


// returns best loop or -1
int choose_loop_to_up(int a, int b) {


  if ((a==-1) || !loop_can_up(a))
    return b;
  if ((b==-1) || !loop_can_up(b))
    return a;

  // Always return coldest loop
  int a_spare = vm.loop[a].dc2dc.dc_current_limit_16s - vm.loop[a].dc2dc.dc_current_16s;
  int b_spare = vm.loop[a].dc2dc.dc_current_limit_16s - vm.loop[a].dc2dc.dc_current_16s;  

 

  if (vm.loop[a].asic_temp_sum/vm.loop[a].asic_count!= vm.loop[b].asic_temp_sum/vm.loop[b].asic_count) {
    // Increase lower asic_temp because they have lower leakage
    return (vm.loop[a].asic_temp_sum/vm.loop[a].asic_count < vm.loop[b].asic_temp_sum/vm.loop[b].asic_count) ? a : b;
  }


  // Always return coldest loop
  if (vm.loop[a].asic_temp_sum != vm.loop[b].asic_temp_sum) {
    // Increase lower asic_temp because they have lower leakage
    return (vm.loop[a].asic_temp_sum < vm.loop[b].asic_temp_sum) ? a : b;
  }

  return a;
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


void ac2dc_scaling_one_second() {
  static int counter = 0;
  int l;
  int do_upscaling = 0;
  int loops_on_max_current = 0;
  now = time(NULL);


  // TODO - for now
  return;

  // See if all loops stable on maximum.
  for (l = 0; l < LOOP_COUNT ; l++) {
    if (vm.loop[l].enabled_loop) {
      if ( vm.loop[l].dc2dc.dc_current_limit_16s - vm.loop[l].dc2dc.dc_current_16s < (3*16)) {
        loops_on_max_current++;
      }
    }
  }

  do_upscaling = (loops_on_max_current > 20);

  if ((!vm.asics_shut_down_powersave) &&
       !vm.scaling_up_system && 
       (vm.start_mine_time > AC2DC_UPSCALE_TIME_SECS) &&
       (vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING)) {

          
        if ((vm.ac2dc_current < (AC2DC_CURRENT_LIMIT - AC2DC_SPARE_CURRENT_TO_UPSCALE))&&
            do_upscaling) {
          l = find_loop_to_up();
          if (l != -1) {
            printf(CYAN "LOOP %d UP\n" RESET, l);
            loop_up(l);
          }
        }

        if (vm.ac2dc_current > (AC2DC_CURRENT_LIMIT - AC2DC_LEFT_CURRENT_TO_DOWNSCALE)) {
          l = find_loop_to_down();
          if (l != -1) {
             printf(CYAN "LOOP %d down\n" RESET, l);
             loop_down(l);
          }
        }

        // Regardless switch 2 loops.
        l = find_loop_to_up();
        if (l != -1) {
           printf(CYAN "LOOP2 %d up\n" RESET, l);          
           loop_up(l);
        }
        
        l = find_loop_to_down();
        if (l != -1) {
           printf(CYAN "LOOP2 %d down\n" RESET, l);
           loop_down(l);
        }
    }
}

