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

#if 0
int loop_can_down(int l) {
  if (l == -1)
      return 0;
  
  return  
     (vm.loop[l].enabled_loop && 
     (vm.loop_vtrim[l] != VTRIM_LOW) &&
     (now - vm.loop[l].last_ac2dc_scaling_on_loop > AC2DC_SCALING_SAME_LOOP_PERIOD_SECS) &&
     vm.loop[l].asic_hz_sum &&
     vm.loop[l].dc2dc.dc_power_watts_16s);
}


void loop_down(int l) {
  int err;
   if ((vm.loop_vtrim[l] < VTRIM_HIGH)) {
    dc2dc_set_vtrim(l, vm.loop_vtrim[l]-1, &err);
  }
  vm.loop[l].last_ac2dc_scaling_on_loop  = now;
}


// returns worst asic
//  Any ASIC is worth then NULL
int choose_loop_to_down(int a, int b) {
  if ((a==-1) || !loop_can_down(a))
    return b;
  if ((b==-1) || !loop_can_down(b))
    return a;

  if (vm.loop[a].dc2dc.dc_current_16s > vm.loop[a].dc2dc.dc_current_limit_16s) return a;
  if (vm.loop[b].dc2dc.dc_current_16s > vm.loop[b].dc2dc.dc_current_limit_16s) return b;
  


  int performance_a = vm.loop[a].asic_hz_sum /vm.loop[a].dc2dc.dc_power_watts_16s;
  int performance_b = vm.loop[b].asic_hz_sum /vm.loop[b].dc2dc.dc_power_watts_16s;
  
  return (performance_a < performance_b)?a:b;
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
  if (l == -1)
      return 0;
  /*
  printf(" %d %d %d %d %d %d",
    ,now - vm.loop[l].last_ac2dc_scaling_on_loop,
    ,,,,)
*/
  return  
    (vm.loop[l].enabled_loop &&
    (vm.loop_vtrim[l] != VTRIM_HIGH) &&
    ((now - vm.loop[l].last_ac2dc_scaling_on_loop) > AC2DC_SCALING_SAME_LOOP_PERIOD_SECS) &&
     (vm.loop[l].dc2dc.dc_current_limit_16s > DC2DC_CURRENT_TOP_BEFORE_LEARNING_16S-1*16) &&
     vm.loop[l].asic_hz_sum &&
     vm.loop[l].dc2dc.dc_power_watts_16s);
 
}


void loop_up(int l) {
  int err;
  if (vm.loop_vtrim[l] == VTRIM_LOW) {
    dc2dc_set_vtrim(l, VTRIM_START, &err);
  }
   if (vm.loop_vtrim[l] == VTRIM_START) {
    dc2dc_set_vtrim(l, VTRIM_HIGH, &err);
  }
   vm.loop[l].last_ac2dc_scaling_on_loop  = now;
}


// returns best loop or -1
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


void ac2dc_scaling_one_minute() {


#if 0  
  static int counter = 0;
  int l;
  int do_upscaling = 0;
  int loops_on_max_current = 0;
  now = time(NULL);
  counter++;
  //passert(0);
  if (counter%5 != 0) {
    return;
  }
  printf(CYAN "******************************\n******************************\nAC2DC SCALING\n******************************\n" RESET);


  
  
  if ((!vm.asics_shut_down_powersave) &&
       (vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING)) {

    int spare_juice = 0;
    for (l = 0; l < LOOP_COUNT ; l++) {
      // scale down loops with 8 asics, where current near critical or more then 6 hot asics 
      if ((vm.loop[l].asic_count == HAMMERS_PER_LOOP) &&
        (
          (vm.loop[l].dc2dc.dc_current_16s >= vm.loop[l].dc2dc.dc_current_limit_16s-16) ||
          (vm.loop[l].hot_asics_count >= HOT_ASICS_IN_LOOP_FOR_DOWNSCALE))
        ) {
        if (loop_can_down(l)) {
          printf(CYAN "LOOP DOWN:%d\n" RESET, l);
          loop_down(l);
          spare_juice++;
        }
      }
    }

    // add spare to 'downed' loops
    if ((vm.ac2dc_power < AC2DC_SPARE_CURRENT_TO_UPSCALE)) {
      spare_juice+=(AC2DC_POWER_LIMIT-vm.ac2dc_power/5);
    }


    // only scale up if has power
    for (int l = 0; (l < LOOP_COUNT) && spare_juice ; l++) {
      // scale down loops with 8 asics, where current near critical or more then 6 hot asics 
      if ((vm.loop[l].asic_count == HAMMERS_PER_LOOP) &&
         (
          (vm.loop[l].dc2dc.dc_current_16s < vm.loop[l].dc2dc.dc_current_limit_16s-16*2) ||
          (vm.loop[l].hot_asics_count >= COLD_ASICS_IN_LOOP_FOR_UPSCALE))
        ) {
        if (loop_can_up(l)) {
          printf(CYAN "LOOP UP:%d\n" RESET, l);
          loop_up(l);
          spare_juice--;
        }
      }
    }

#if 0

    if (!downed &&
        (vm.ac2dc_power > (AC2DC_POWER_LIMIT - AC2DC_LEFT_CURRENT_TO_DOWNSCALE))) {
       l = find_loop_to_down();
       printf(CYAN "LOOP %d down\n" RESET, l);
      if (l != -1) {
         loop_down(l);
      } 
    }


        l = find_loop_to_up();
        if (l != -1) {
          printf(CYAN "LOOP %d UP\n" RESET, l);
          loop_up(l);
        }

        l = find_loop_to_down();
        if (l != -1) {
           printf(CYAN "LOOP %d down\n" RESET, l);
           loop_down(l);
        }
#endif
  }
#endif  
}
#endif
