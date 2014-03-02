

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
#include "defines.h"

//#if TEST_BOARD
extern int loop_to_measure;

// micro ampers
int tb_get_asic_current(int id) {
#if TEST_BOARD == 1
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TESTBOARD_PIN);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x01 << id / 2);
  int addr = 0x4c + id % 2;
  i2c_write_byte(addr, 0x1, 0x1a);
  i2c_write_byte(addr, 0x2, 0x01);
  int error;
  usleep(10000);
  unsigned int s6 = i2c_read_byte(addr, 0x6, &error);
  if (error)
    printf("i2c Error dfjs\n");
  
  usleep(10000);
  unsigned int s7 = i2c_read_byte(addr, 0x7, &error);
  if (error)
    printf("i2c Error dfjs\n");
  if (s7 == 0xff && s6 == 0xff) {
  } 
  unsigned int s8 = ((s7 | (s6 & 0x3f) << 8) * 3884);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0);
  i2c_write(PRIMARY_I2C_SWITCH, 0);

  return s8;
#else 
  return 0;
#endif
}

// micro ampers
int tb_get_asic_voltage(int id) {
#if TEST_BOARD == 1
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TESTBOARD_PIN);
  //usleep(10000); 
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x01 << id / 2);
  int addr = 0x4c + id % 2;
  //usleep(10000); 
  i2c_write_byte(addr, 0x1, 0x1a);
  //usleep(10000);
  i2c_write_byte(addr, 0x2, 0x01);
  int error;
  //usleep(10000);
  unsigned int sa = i2c_read_byte(addr, 0xa, &error);
  if (error)
    printf("i2c Error dfjs\n");
  
  //usleep(10000);
  unsigned int sb = i2c_read_byte(addr, 0xb, &error);
  if (error)
    printf("i2c Error dfjs\n");
  if (sa == 0xff && sb == 0xff) {
  }
  unsigned int s8 = ((sb | (sa & 0x3f) << 8) * 305);
    //usleep(10000);
  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0);

  i2c_write(PRIMARY_I2C_SWITCH, 0);
    //usleep(10000);
  return s8;
#else 
  return 0;
#endif    
}


int max_freq[ENGINES_PER_ASIC][ASIC_VOLTAGE_COUNT][HAMMERS_PER_LOOP];
//int max_voltage[ENGINES_PER_ASIC][ASIC_VOLTAGE_COUNT][HAMMERS_PER_LOOP];
int max_current[ENGINES_PER_ASIC][ASIC_VOLTAGE_COUNT][HAMMERS_PER_LOOP];




void fulltest_test_board() {
  enable_reg_debug = 0;
  ASIC_FREQ f;
  DC2DC_VOLTAGE v;
  uint32_t engine_id = 5;
  int err;
  int test_id = 0;
  
  while(1) {
    hammer_iter hi;
    hammer_iter_init(&hi);
    
    while (hammer_iter_next_present(&hi)) {
      vm.working_engines[hi.addr] = ALL_ENGINES_BITMASK;// 1 << engine_id;
      printf("TESTING_ENGINES::::::%x\n",vm.working_engines[hi.addr]);
      write_reg_broadcast(ADDR_LEADER_ENGINE, engine_id);
    }
    
    // To reset engine we work with
    disable_engines_all_asics();
    if (!enable_nvm_engines_all_asics_ok()) {
      //passert(0);
      //reset_all_asics_full_reset();
      printf(RED "PLL FAIL\n" RESET);
      //continue;
    }
    static ASIC_TEMP temp_measure_temp = ASIC_TEMP_83;

    int some_asics_busy = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY);
    printf("some_asics_busy=%x\n",some_asics_busy);
    passert(some_asics_busy == 0);


    for (v = ASIC_VOLTAGE_585; v < ASIC_VOLTAGE_COUNT; v=(DC2DC_VOLTAGE)(v+1)) {
      passert(read_reg_broadcast(ADDR_BR_THERMAL_VIOLTION) == 0);
      int millivolts;
      VOLTAGE_ENUM_TO_MILIVOLTS(v, millivolts);
      /*
      read_some_asic_temperatures(0, temp_measure_temp, (ASIC_TEMP)(temp_measure_temp+1));
      
      temp_measure_temp=(ASIC_TEMP)(temp_measure_temp+2);
      if (temp_measure_temp >= ASIC_TEMP_COUNT) {
        temp_measure_temp = ASIC_TEMP_83;
      }
      */
      printf("Setting DC2DC to %d milli\n", millivolts);
#if TEST_BOARD != 1
      dc2dc_set_voltage(15, v, &err);
#else
      dc2dc_set_voltage(0, v, &err);
#endif      

      for (f = ASIC_FREQ_225; f < ASIC_FREQ_MAX ; f=(ASIC_FREQ)(f+1)) {
        printf("ENGINE:%d VOLTAGE_SET=%d FREQ_SET=%d\n", engine_id, millivolts, f*15+210); 
        
        //reset_all_asics_full_reset();  
        //usleep(300000); // to cool off
        write_reg_broadcast(ADDR_LEADER_ENGINE, engine_id);
        // Set PLLs
        hammer_iter hi;
        hammer_iter_init(&hi);    
        disable_engines_all_asics();
        while (hammer_iter_next_present(&hi)) {
          printf("Setting PLL to %d\n", f*15+210);
          set_pll(hi.addr,f);
        }
        
        if (!enable_nvm_engines_all_asics_ok()) {
             printf(YELLOW "PLL FAIL TO %d AT %d MV\n" RESET,f*15+210  ,millivolts);
             passert(read_reg_broadcast(ADDR_BR_THERMAL_VIOLTION) == 0);
             reset_all_asics_full_reset();          
             write_reg_broadcast(ADDR_LEADER_ENGINE, engine_id);
             continue;
        }

        // Set engine to test 
        hammer_iter_init(&hi);    
        while (hammer_iter_next_present(&hi)) {
            hi.a->passed_last_bist_engines = ALL_ENGINES_BITMASK;
        }
        
        int ok = do_bist_ok();
       // ok |= do_bist_ok(loop_to_measure);
       // ok |= do_bist_ok(loop_to_measure);
        
        //printf("v= (not) v=%d f=%d\n",millivolts,f*15+210);
        // Print all ASICs after bist
        if (vm.bist_fatal_err) {
          printf(YELLOW "BIST FATALITY TO %dHZ AT %d MV\n" RESET,f*15+210  ,millivolts);
          reset_all_asics_full_reset();          
          write_reg_broadcast(ADDR_LEADER_ENGINE, engine_id);
          continue;
        }
        
        hammer_iter_init(&hi);
        int good = 0;
        
        while (hammer_iter_next_present(&hi)) {
          //int fff = read_reg_device(0, ADDR_DLL_MASTER_MSRMNT);
          //printf("TEST %d %s\n", i, ok?":)":":(");    

          // Update MAX freq
          if (hi.a->passed_last_bist_engines == ALL_ENGINES_BITMASK) {
            good = 1;
            if (f*15+210 > max_freq[engine_id][v][hi.addr]) {
              printf("max_freq[E:%d][V:%d][ADDR:%d] = %d\n",engine_id,millivolts,hi.addr, f*15+210);
              max_freq[engine_id][v][hi.addr] = f*15+210;
              //max_voltage[engine_id][v][hi.addr] = vm.bist_voltage;
              max_current[engine_id][v][hi.addr] = vm.bist_current;
            }
          } 
        }
        test_id++;

       
        if (!good) 
          break;
        //enable_reg_debug = 0;
        //passert(0);
      }
    }

    break;
    if (engine_id == 5) {
      engine_id = 10;
    } else {
      break;
    }
  }

/*
  printf("MAX FREQ PER VOLTAGE\n");
  for (engine_id = 0; engine_id < ENGINES_PER_ASIC ; engine_id++) {
    printf("ENGINE %d\n", engine_id);
    for(int i = ASIC_VOLTAGE_555 ; i < ASIC_VOLTAGE_COUNT ; i=(DC2DC_VOLTAGE)(i+1)) {
         int millivolts;
        VOLTAGE_ENUM_TO_MILIVOLTS(i, millivolts);
        
      printf("%4d:",millivolts);
      for(int j = 0 ; j < HAMMERS_PER_LOOP; j++) {
        printf("%4d/",max_freq[engine_id][i][j]);
        printf("%4d/",max_voltage[engine_id][i][j]/1000);
        printf("%4d ",max_current[engine_id][i][j]/1000);
      }
      printf("\n");
    }
  }
*/  
  printf("MAX FREQ PER VOLTAGE\n");
  engine_id = 5;
    printf("ENGINE %d\n", engine_id);
    for(int i = ASIC_VOLTAGE_555 ; i < ASIC_VOLTAGE_COUNT ; i=(DC2DC_VOLTAGE)(i+1)) {
       int millivolts;
      VOLTAGE_ENUM_TO_MILIVOLTS(i, millivolts);
        
      printf("%4d ",millivolts);

      for (int j = 121; j < 127; j++) {
       printf("%4d ",max_freq[engine_id][i][j]);
   //  printf("%4d ",max_voltage[engine_id][i][j]/1000);
       printf("%6d |",(max_current[engine_id][i][j]*1000)/16);
      }
      printf("\n");
  }

/*
  engine_id = 10;
     printf("ENGINE %d\n", engine_id);
     for(int i = ASIC_VOLTAGE_555 ; i < ASIC_VOLTAGE_COUNT ; i=(DC2DC_VOLTAGE)(i+1)) {
        int millivolts;
       VOLTAGE_ENUM_TO_MILIVOLTS(i, millivolts);
         
       printf("%4d ",millivolts);
       int j = 0;
        printf("%4d ",max_freq[engine_id][i][j]);
        printf("%4d ",max_voltage[engine_id][i][j]/1000);
        printf("%4d ",max_current[engine_id][i][j]/1000);
       printf("\n");
   }
*/


  printf("BIN:\n");
  for(int j = 0 ; j < HAMMERS_PER_LOOP; j++) {
    printf("%d:%c ",j, 'A' + (max_freq[0][ASIC_VOLTAGE_810][j])/30);
  }
  printf("\nThank you for your cooperation.\nPlease continue to next test.\n");

  exit(0);
  
}



//#endif
