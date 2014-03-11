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
#include "asic_thermal.h"

#include "scaling_manager.h"
#include "scaling_dc2dc.h"

#include "corner_discovery.h"


void do_leakage_discovery_loop(int l) {
   int err; 
   dc2dc_set_vtrim(l, VTRIM_810, &err);
   
   for (int h = 0 ; h < HAMMERS_PER_LOOP; h++) {
      struct timeval tv;
      int addr = h+l*HAMMERS_PER_LOOP;
      // Heat to 117 degree
      int temp = 0;
      disable_engines_all_asics();
      set_pll(addr, ASIC_FREQ_510);
      // Enable work mode
      enable_all_engines_asic(addr);     
      while (temp != 3) {
        
        do_bist_ok(0);
        temp = read_asic_temperature(addr,ASIC_TEMP_83, ASIC_TEMP_89);
        printf("%x:%x ",addr,temp);
      }
      // Measure current.
      


      disable_engines_asic(addr);
   }   
   dc2dc_set_vtrim(l, VTRIM_MIN, &err);
}

void do_leakage_discovery() {
  // first heat up the ASICs 
  hammer_iter hi;
  hammer_iter_init(&hi);  

  while (hammer_iter_next_present(&hi)) {
    disable_engines_asic(hi.addr);
  }
  //kill_fan();
  set_fan_level(0);

  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    do_leakage_discovery_loop(l);
  }

  
  exit(0);
}

