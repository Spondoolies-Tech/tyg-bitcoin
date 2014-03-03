#include "dc2dc.h"
#include "i2c.h"
#include "nvm.h"
#include "hammer.h"
#include <time.h>

#include "defines.h"
#include "scaling_manager.h"

extern MINER_BOX vm;
#if 0
int volt_to_vtrim[ASIC_VOLTAGE_COUNT] = 
  { 0, 0xFFC4, 0xFFCF, 0xFFE1, 0xFFd4, 0xFFd6, 0xFFd8, 
//     550      580     630      675    681    687                               
       0xFFda,  0xFFdc, 0xFFde, 0xFFE1, 0xFFE3,   
//     693       700     705       710    715  
  0xFFE5,  0xfff7, 0x0, 0x8,
//  720      765   790   810 


};
   
int volt_to_vmargin[ASIC_VOLTAGE_COUNT] = { 0, 0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 
                                            0x0,0x0, 0x0,  0x0,  0x0, 0x0, 0x0,
                                            0x0, 0x0 };
#endif



void dc2dc_print() {
  printf("VOLTAGE/CURRENT/TEMP:\n");
  for (int loop = 0; loop < LOOP_COUNT; loop++) {
    int err;
    int volt = dc2dc_get_voltage(loop, &err);
    int temp = dc2dc_get_temp(loop, &err);
    int crnt = dc2dc_get_current_16s_of_amper(loop, &err);

    if (!err) {
      printf("%2i:", loop);
      printf("%3d/", volt);
      printf(RESET);
      if (crnt < DC2DC_CURRENT_WARNING_16S)
        printf(GREEN);
      if (crnt > DC2DC_CURRENT_WARNING_16S)
        printf(RED);
      printf("%3d/", crnt / 16);
      printf(RESET);
      if (temp < DC2DC_TEMP_GREEN_LINE)
        printf(GREEN);
      if (temp > DC2DC_TEMP_RED_LINE)
        printf(RED);
      printf("%3d\n", temp);
      printf(RESET);
    } else {
      printf(RED "%2i:XXX/XXX/XXX\n" RESET, loop);
    }
    if (loop == LOOP_COUNT / 2 - 1) {
      printf("\n");
    }
  }
  printf("\n");
}

void dc2dc_init() {
  int err = 0;
  // static int warned = 0;
  // Write defaults
#if TEST_BOARD == 1
  for (int loop = 0; loop < 1; loop++) {
#else
  for (int loop = 0; loop < LOOP_COUNT; loop++) {
#endif    

#if NO_TOP == 1 
    if (loop < LOOP_COUNT/2) {
      continue;
    }
#endif
    

    dc2dc_select_i2c(loop, &err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC1 %d\n" RESET,
              loop);
      dc2dc_i2c_close();
      continue;
    }

    i2c_write_byte(I2C_DC2DC, 0x00, 0x81, &err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC2 %d\n" RESET,
              loop);
      dc2dc_i2c_close();
      continue;
    }

    i2c_write_word(I2C_DC2DC, 0x35, 0xf028);
    i2c_write_word(I2C_DC2DC, 0x36, 0xf018);
    i2c_write_word(I2C_DC2DC, 0x38, 0x881f);
    i2c_write_word(I2C_DC2DC, 0x46, 0xf846);
    i2c_write_word(I2C_DC2DC, 0x4a, 0xf836);
    i2c_write_byte(I2C_DC2DC, 0x47, 0x3C);
    i2c_write_byte(I2C_DC2DC, 0xd7, 0x03);
    i2c_write_byte(I2C_DC2DC, 0x02, 0x02);
    i2c_write(I2C_DC2DC, 0x15);
    usleep(100000);
    i2c_write(I2C_DC2DC, 0x03);
    psyslog("OK INIT DC2DC\n");
    dc2dc_i2c_close();
  }

#if NO_TOP == 1
  dc2dc_disable_dc2dc(12, &err);
  dc2dc_disable_dc2dc(13, &err);
  dc2dc_disable_dc2dc(14, &err);
  dc2dc_disable_dc2dc(16, &err);
  dc2dc_disable_dc2dc(17, &err);
  dc2dc_disable_dc2dc(18, &err);
  dc2dc_disable_dc2dc(19, &err);
  dc2dc_disable_dc2dc(20, &err);
  dc2dc_disable_dc2dc(21, &err);
  dc2dc_disable_dc2dc(22, &err);
  dc2dc_disable_dc2dc(23, &err);
#endif
  
}

void dc2dc_set_channel(int channel_mask, int *err) {
  i2c_write_byte(I2C_DC2DC, 0x00, channel_mask, err);
}

void dc2dc_disable_dc2dc(int loop, int *err) {
#if NO_TOP == 1 
    if (loop < LOOP_COUNT/2) {
      return;
    }
#endif

  
#if TEST_BOARD != 1
  dc2dc_select_i2c(loop, err);
  i2c_write_byte(I2C_DC2DC, 0x02, 0x12, err);
  dc2dc_i2c_close();
#endif
}

void dc2dc_enable_dc2dc(int loop, int *err) {
#if NO_TOP == 1 
    if (loop != 15) {
      return;
    }
#endif  
  dc2dc_select_i2c(loop, err);
  i2c_write_byte(I2C_DC2DC, 0x02, 0x02, err);
  dc2dc_i2c_close();
}

// miner-# i2cset -y 0 0x1b 0x02 0x12
// miner-# i2cset -y 0 0x1b 0x02 0x02

void dc2dc_i2c_close() {
#if TEST_BOARD == 1
      i2c_write(PRIMARY_TESTBOARD_SWITCH, 0); // TOP
#else
      i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0);
      i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0);
#endif    
  i2c_write(PRIMARY_I2C_SWITCH, 0);
}

void dc2dc_select_i2c_ex(int top,          // 1 or 0
                         int i2c_group,    // 0 or 1
                         int dc2dc_offset, // 0..7
                         int *err) { // 0x00=first, 0x01=second, 0x81=both
//#if TEST_BOARD == 1
//  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TOP_MAIN_PIN,err); // TOP
//  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x40, err); //
//#else 
  if (top) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TOP_MAIN_PIN,err); // TOP
  } else {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN, err); // BOTTOM
  }
//#endif

  if (i2c_group == 0) {
#if TEST_BOARD == 1
    i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x40); // TOP
#else
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 1 << dc2dc_offset); // TOP
#endif
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0); // TO
  } else {
#if TEST_BOARD != 1  
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 1 << dc2dc_offset); // TOP
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0);                 // TOP
#endif    
  }
}

void dc2dc_select_i2c(int loop, int *err) { // 1 or 0
#if NO_TOP == 1 
      if (loop < LOOP_COUNT/2) {
        return;
      }
#endif


  int top = (loop < 12);
  int i2c_group = ((loop % 12) >= 8);
  int dc2dc_offset = loop % 12;
  dc2dc_offset = dc2dc_offset % 8;
  dc2dc_select_i2c_ex(top,          // 1 or 0
                      i2c_group,    // 0 or 1
                      dc2dc_offset, // 0..7
                      err);
}
/*
void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v, int *err) {
  int millis;
  VOLTAGE_ENUM_TO_MILIVOLTS(v, millis);   
  printf("Set VOLTAGE Loop %d Milli:%d Enum:%d\n",loop, millis, v);
#if NO_TOP == 1 
    if (loop != 15) {
      *err = 1;
      return;
      
    }
#endif  
  // printf("%d\n",v);
  // int err = 0;
  dc2dc_select_i2c(loop, err);
  passert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
  i2c_write_word(I2C_DC2DC, 0xd4, volt_to_vtrim[v]);
  i2c_write_byte(I2C_DC2DC, 0x01, volt_to_vmargin[v]);
  dc2dc_i2c_close();
}
*/

void dc2dc_set_vtrim(int loop, uint32_t vtrim, int *err) {
  pause_asics_if_needed();
  assert(vtrim >= VTRIM_MIN && vtrim <= VTRIM_MAX);
  printf("Set VOLTAGE Loop %d Milli:%d Vtrim:%d\n",loop, VTRIM_TO_VOLTAGE_MILLI(vtrim),vtrim);
#if NO_TOP == 1 
    if (loop != 15) {
      *err = 1;
      return;
      
    }
#endif  
  // printf("%d\n",v);
  // int err = 0;
  dc2dc_select_i2c(loop, err);
//  passert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
  i2c_write_word(I2C_DC2DC, 0xd4, vtrim&0xFFFF);
  i2c_write_byte(I2C_DC2DC, 0x01, 0);
  vm.loop_vtrim[loop] = vtrim;
  vm.loop[loop].dc2dc.last_voltage_change_time = time(NULL);
  dc2dc_i2c_close();
}


// Return 1 if needs urgent scaling
int get_dc2dc_error(int loop) {
   // TODO - select loop!
  // int err = 0;
#if NO_TOP == 1 
  if (loop != 15) {
    return 0;
  }
#endif    

  int err; 
  int error_happened = 0;

  static int warned = 0;
  int current = 0;
  dc2dc_select_i2c(loop, &err);
  dc2dc_set_channel(0, &err);
  error_happened |= (i2c_read_word(I2C_DC2DC, 0x7b) & 0x80);
  if (error_happened) {
    printf(RED "DC2DC ERR0 %x\n" RESET,error_happened);
    i2c_write(I2C_DC2DC,3,&err);
  }
  dc2dc_set_channel(1, &err);
  error_happened |= (i2c_read_word(I2C_DC2DC, 0x7b) & 0x80);
  if (error_happened) {
    printf(RED "DC2DC ERR1 %x\n" RESET,error_happened);
    i2c_write(I2C_DC2DC,3,&err);
  }
  dc2dc_set_channel(0x81,&err);
  if (err) {
    dc2dc_i2c_close();
    return 0;
  }
  dc2dc_i2c_close();
  return error_happened;
}



// Return 1 if needs urgent scaling
int update_dc2dc_current_temp_measurments(int loop) {
  int err;
  int i = loop;
#if NO_TOP == 1 
  if (i != 15) {
    return 0;
  }
#endif    
  if (vm.loop[i].enabled_loop) {
    vm.loop[i].dc2dc.dc_temp = dc2dc_get_temp(i, &err);
    int current = dc2dc_get_current_16s_of_amper(i, &err);
    if (!vm.asics_shut_down_powersave &&
        current >= DC2DC_POWER_TRUSTWORTHY && 
        vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_DC2DC_MEASUREMENT) {
        vm.loop[i].dc2dc.dc_current_16s =
                        dc2dc_get_current_16s_of_amper(i, &err);
        vm.loop[i].dc2dc.dc_power_watts_16s = 
        vm.loop[i].dc2dc.dc_current_16s*VTRIM_TO_VOLTAGE_MILLI(vm.loop_vtrim[i])/1000;
    } else {
      // This will disable ac2dc scaling
      vm.loop[i].dc2dc.dc_current_16s = 0;
      vm.loop[i].dc2dc.dc_power_watts_16s = 0;
      printf(GREEN "Loop %d current not saved = %d" RESET, i, vm.loop[i].dc2dc.dc_current_16s *1000/16);
    }
  }
  return 0;
}



// returns AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int *err) {
  // TODO - select loop!
  // int err = 0;
#if NO_TOP == 1 
      if (loop != 15) {
        return 0;
      }
#endif    

  
  passert(err != NULL);
  static int warned = 0;
  int current = 0;
  dc2dc_select_i2c(loop, err);
  dc2dc_set_channel(0, err);
  current += (i2c_read_word(I2C_DC2DC, 0x8c) & 0x07FF);
  dc2dc_set_channel(1, err);
  current += (i2c_read_word(I2C_DC2DC, 0x8c) & 0x07FF);
  dc2dc_set_channel(0x81, err);
  if (*err) {
    dc2dc_i2c_close();
    return 0;
  }
  dc2dc_i2c_close();
  return current;
}

int dc2dc_get_voltage(int loop, int *err) {
#if NO_TOP == 1 
      if (loop < LOOP_COUNT/2) {
        return 0;
      }
#endif    
  
  //*err = 0;
  passert(err != NULL);
  static int warned = 0;
  int voltage;
  dc2dc_select_i2c(loop, err);
  voltage = i2c_read_word(I2C_DC2DC, 0x8b, err) * 1000 / 512;
  if (*err) {
    dc2dc_i2c_close();
    return 0;
  }
  dc2dc_i2c_close();
  return voltage;
}

int dc2dc_get_temp(int loop, int *err) {
#if NO_TOP == 1 
      if (loop != 15) {
        return 0;
      }
#endif    
  
  // returns max temperature of the 2 sensors
  passert(err != NULL);
  int temp1, temp2;
  dc2dc_select_i2c(loop, err);
  dc2dc_set_channel(0, err);
  temp1 = i2c_read_word(I2C_DC2DC, 0x8e, err) /* *1000/512 */;
  if (*err) {
    dc2dc_i2c_close();
    return 0;
  }
  dc2dc_set_channel(1, err);
  temp2 = i2c_read_word(I2C_DC2DC, 0x8e, err) /* *1000/512 */;
  if (*err) {
    dc2dc_i2c_close();
    return 0;
  }
  if (temp2 > temp1)
    temp1 = temp2;
  dc2dc_i2c_close();
  return temp1;
}


