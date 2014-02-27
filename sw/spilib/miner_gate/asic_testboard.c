

#include "defines.h"
#include "i2c.h"
#include "asic_testboard.h"

#include <stdio.h>
#include <stdlib.h>
#include "corner_discovery.h"
#include "scaling_manager.h"
#include <spond_debug.h>
#include "hammer.h"
#include "hammer_lib.h"
#include "squid.h"
#include "pll.h"
#include "asic_thermal.h"

//#if ASIC_TESTBOARD 

// micro ampers
int tb_get_asic_current(int id) {

  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TESTBOARD_PIN);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x01 << id / 2);
  int addr = 0x4c + id % 2;
  i2c_write_byte(addr, 0x1, 0x1a);
  i2c_write_byte(addr, 0x2, 0x01);
  int error;
  usleep(1000);
  unsigned int s6 = i2c_read_byte(addr, 0x6, &error);
  if (error)
    printf("i2c Error dfjs\n");
  unsigned int s7 = i2c_read_byte(addr, 0x7, &error);
  if (error)
    printf("i2c Error dfjs\n");
  if (s7 == 0xff && s6 == 0xff) {
  } 
  unsigned int s8 = ((s7 | (s6 & 0x3f) << 8) * 3884);
  i2c_write(PRIMARY_I2C_SWITCH, 0);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0);

  return s8;
}

// micro ampers
int tb_get_asic_voltage(int id) {

  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TESTBOARD_PIN);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x01 << id / 2);
  int addr = 0x4c + id % 2;
  i2c_write_byte(addr, 0x1, 0x1a);
  i2c_write_byte(addr, 0x2, 0x01);
  int error;
  usleep(1000);
  unsigned int sa = i2c_read_byte(addr, 0xa, &error);
  if (error)
    printf("i2c Error dfjs\n");
  unsigned int sb = i2c_read_byte(addr, 0xb, &error);
  if (error)
    printf("i2c Error dfjs\n");
  if (sa == 0xff && sb == 0xff) {
  }
  unsigned int s8 = ((sb | (sa & 0x3f) << 8) * 305);
  i2c_write(PRIMARY_I2C_SWITCH, 0);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0);
  printf("%d\n", s8);

  return s8;
}


int max_freq[ENGINES_PER_ASIC][ASIC_VOLTAGE_COUNT][HAMMERS_PER_LOOP];
//int failed_tests[]

void fulltest_test_board() {
  enable_reg_debug = 0;
  ASIC_FREQ f;
  DC2DC_VOLTAGE v;
  uint32_t engine_id;
  int err;
  int i = 0;
  for (engine_id = 0; engine_id < 2 ; engine_id++) {

    hammer_iter hi;
    hammer_iter_init(&hi);
    while (hammer_iter_next_present(&hi)) {
      nvm.working_engines[hi.addr] = 0x3;
      write_reg_broadcast(ADDR_LEADER_ENGINE, engine_id);
    }
    // To reset engine we work with
    disable_engines_all_asics();
    enable_nvm_engines_all_asics();
    static ASIC_TEMP temp_measure_temp = ASIC_TEMP_83;

    for (v = ASIC_VOLTAGE_555; v < ASIC_VOLTAGE_COUNT ; v=(DC2DC_VOLTAGE)(v+1)) {
      read_some_asic_temperatures(0, temp_measure_temp, (ASIC_TEMP)(temp_measure_temp+1));
      
      temp_measure_temp=(ASIC_TEMP)(temp_measure_temp+2);
      if (temp_measure_temp >= ASIC_TEMP_COUNT) {
        temp_measure_temp = ASIC_TEMP_83;
      }

      
      dc2dc_set_voltage(0, ASIC_VOLTAGE_555, &err);
      int millivolts;
      VOLTAGE_ENUM_TO_MILIVOLTS(v, millivolts);
          
      for (f = ASIC_FREQ_225; f < ASIC_FREQ_MAX ; f=(ASIC_FREQ)(f+1)) {
        hammer_iter hi;
        hammer_iter_init(&hi);    
        disable_engines_all_asics();
        while (hammer_iter_next_present(&hi)) {
           set_pll(hi.addr,f);
        }
        enable_nvm_engines_all_asics();

        
        hammer_iter_init(&hi);    
        while (hammer_iter_next_present(&hi)) {
            hi.a->passed_last_bist_engines = ALL_ENGINES_BITMASK;
        }
        
        int ok = do_bist_ok();
        hammer_iter_init(&hi);    
        while (hammer_iter_next_present(&hi)) {
          printf("TEST %d %s\n", i, ok?":)":":(");    
          printf("ENGINE:%d VOLTAGE:%d FREQ:%d\n", engine_id, millivolts, f*15+210); 
          printf("BIST PASSED ENGINES: ");
          hammer_iter_init(&hi);    
          while (hammer_iter_next_present(&hi)) {
              printf("%x:%x ", hi.addr%HAMMERS_PER_LOOP, hi.a->passed_last_bist_engines);
              if (hi.a->passed_last_bist_engines == ALL_ENGINES_BITMASK) {
                //printf("*");
                max_freq[engine_id][v][hi.addr] = f*15+210;
              }
          }
          printf("\n");
        }
        i++;
        //enable_reg_debug = 0;
        //assert(0);
      }
    }
  }


  printf("MAX FREQ PER VOLTAGE\n");
  for (engine_id = 0; engine_id < ENGINES_PER_ASIC ; engine_id++) {
    printf("ENGINE %d\n", engine_id);
    for(int i = ASIC_VOLTAGE_555 ; i < ASIC_VOLTAGE_COUNT ; i=(DC2DC_VOLTAGE)(i+1)) {
         int millivolts;
        VOLTAGE_ENUM_TO_MILIVOLTS(i, millivolts);
        
      printf("%4d:",millivolts);
      for(int j = 0 ; j < HAMMERS_PER_LOOP; j++) {
        printf("%4d ",max_freq[engine_id][i][j]);
      }
      printf("\n");
    }
  }
  
  printf("BIN:\n");
  for(int j = 0 ; j < HAMMERS_PER_LOOP; j++) {
    printf("%d:%c ",j, 'A' + (max_freq[0][ASIC_VOLTAGE_810][j])/30);
  }
  printf("\nThank you for your cooperation.\nPlease continue to next test.\n");

  exit(0);
  

#if 0
  int i;
  printf(ANSI_COLOR_MAGENTA "Checking ASIC speed...!\n" ANSI_COLOR_RESET);
  ASIC_FREQ corner_passage_freq_at_081v[ASIC_CORNER_COUNT] = {
    ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_225, ASIC_FREQ_345, ASIC_FREQ_510,
    ASIC_FREQ_660
  };

  printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
  for (i = ASIC_CORNER_SS; i < ASIC_CORNER_COUNT; i++) {
    test_asics_in_freq(corner_passage_freq_at_081v[i], (ASIC_CORNER)i);
  }

  // We can assume all corners updated. Set all frequencies:
  // printf("->>>--%x- %d %s\n",nvm.good_loops,  __LINE__, __FUNCTION__);
  for (int l = 0; l < LOOP_COUNT; l++) {
    if (nvm.good_loops & 1 << l) {
      // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
      int avarage_corner = 0;
      int working_asic_count = 0;
      // Compute corners to ASICs
      for (int h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
        // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
        if (nvm.working_engines[h]) {
          avarage_corner += nvm.asic_corner[h];
          working_asic_count++;
        }
      }
      // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
      //  compute avarage corner
      if (working_asic_count) {
        avarage_corner = avarage_corner / working_asic_count;
        nvm.loop_voltage[l] = CORNER_TO_VOLTAGE_TABLE[avarage_corner];
        printf("avarage_corner per loop %d = %d\n", l, avarage_corner);
      } else {
        nvm.loop_voltage[l] = ASIC_VOLTAGE_0;
        printf("CLOSE LOOP???? TODO\n");
      }
    } else {
      nvm.loop_voltage[l] = ASIC_VOLTAGE_0;
      printf("CLOSE LOOP???? TODO\n");
    }
  }
  // printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
  nvm.dirty = 1;
#endif
}



//#endif
