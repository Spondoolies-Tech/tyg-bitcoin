#include "dc2dc.h"
#include "i2c.h"
#include "nvm.h"
#include "hammer.h"
#include <time.h>
#include <pthread.h>

#include "defines.h"
#include "scaling_manager.h"
extern pthread_mutex_t i2c_mutex;



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


static void dc2dc_i2c_close();
static void dc2dc_select_i2c(int loop, int *err);
static void dc2dc_set_channel(int channel_mask, int *err);


void dc2dc_init() {
  //printf("%s:%d\n",__FILE__, __LINE__);
  
  pthread_mutex_lock(&i2c_mutex);
  int err = 0;
  // static int warned = 0;
  // Write defaults
#if TEST_BOARD == 1
  for (int loop = 0; loop < 1; loop++) {
#else
  for (int loop = 0; loop < LOOP_COUNT; loop++) {
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
   // i2c_write(I2C_DC2DC, 0x15);
    usleep(1000);
    i2c_write(I2C_DC2DC, 0x03);
    psyslog("OK INIT DC2DC\n");
    dc2dc_i2c_close();
  }

  pthread_mutex_unlock(&i2c_mutex);
  //printf("W\n");
}

static void dc2dc_set_channel(int channel_mask, int *err) {
  i2c_write_byte(I2C_DC2DC, 0x00, channel_mask, err);
}

void dc2dc_disable_dc2dc(int loop, int *err) {
   //printf("%s:%d\n",__FILE__, __LINE__);
  pthread_mutex_lock(&i2c_mutex);
  //printf("%s:%d\n",__FILE__, __LINE__);
  
#if TEST_BOARD != 1
  dc2dc_select_i2c(loop, err);
  i2c_write_byte(I2C_DC2DC, 0x02, 0x12, err);
  dc2dc_i2c_close();
#endif
  pthread_mutex_unlock(&i2c_mutex);
}






void dc2dc_enable_dc2dc(int loop, int *err) {
  

  //printf("%s:%d\n",__FILE__, __LINE__);
  if (vm.loop[loop].enabled_loop) {
    pthread_mutex_lock(&i2c_mutex);
    dc2dc_select_i2c(loop, err);
    i2c_write_byte(I2C_DC2DC, 0x02, 0x02, err);
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
  }
}

// miner-# i2cset -y 0 0x1b 0x02 0x12
// miner-# i2cset -y 0 0x1b 0x02 0x02

static void dc2dc_i2c_close() {
    int err;
#if TEST_BOARD == 1
      i2c_write(PRIMARY_TESTBOARD_SWITCH, 0); // TOP
#endif
      i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0, &err);    
      i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0, &err);
  
          
      i2c_write(PRIMARY_I2C_SWITCH, 0);
}

static void dc2dc_select_i2c_ex(int top,          // 1 or 0
                         int i2c_group,    // 0 or 1
                         int dc2dc_offset, // 0..7
                         int *err) { // 0x00=first, 0x01=second, 0x81=both
  if (top) {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TOP_MAIN_PIN,err); // TOP
  } else {
    i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN, err); // BOTTOM
  }

  if (i2c_group == 0) {
    //if (dc2dc_offset ==0 && top) return;
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 1 << dc2dc_offset, err); // TOP
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0); // TO
  } else {
    i2c_write(I2C_DC2DC_SWITCH_GROUP1, 1 << dc2dc_offset, err); // TOP    
    i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0);                 // TOP   
  }
}

static void dc2dc_select_i2c(int loop, int *err) { // 1 or 0
  int top = (loop < 12);
  int i2c_group = ((loop % 12) >= 8);
  int dc2dc_offset = loop % 12;
  dc2dc_offset = dc2dc_offset % 8;
  dc2dc_select_i2c_ex(top,          // 1 or 0
                      i2c_group,    // 0 or 1
                      dc2dc_offset, // 0..7
                      err);
}


void dc2dc_set_vtrim(int loop, uint32_t vtrim, int *err) {
  printf("Set VOLTAGE Loop %d Milli:%d Vtrim:%x\n",loop, VTRIM_TO_VOLTAGE_MILLI(vtrim),vtrim);
  assert(vtrim >= VTRIM_MIN && vtrim <= VTRIM_MAX);

  pthread_mutex_lock(&i2c_mutex);

  // printf("%d\n",v);
  // int err = 0;
  dc2dc_select_i2c(loop, err);
//  passert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
  i2c_write_word(I2C_DC2DC, 0xd4, vtrim&0xFFFF);
  i2c_write_byte(I2C_DC2DC, 0x01, 0);
  vm.loop_vtrim[loop] = vtrim;
  vm.loop[loop].dc2dc.last_voltage_change_time = time(NULL);
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
}


// Return 1 if needs urgent scaling
int get_dc2dc_error(int loop) {
  int err; 
  int error_happened = 0;

  static int warned = 0;
  int current = 0;
  
 
  pthread_mutex_lock(&i2c_mutex);
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
    pthread_mutex_unlock(&i2c_mutex);
    return 0;
  }
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);  
  return error_happened;
}



// Return 1 if needs urgent scaling
int update_dc2dc_current_temp_measurments(int loop) {
  int err;
  int i = loop;
  if (vm.loop[i].enabled_loop) {
    vm.loop[i].dc2dc.dc_temp = dc2dc_get_temp(i, &err);
    int current = dc2dc_get_current_16s_of_amper(i, &err);
    if (!vm.asics_shut_down_powersave) {
        vm.loop[i].dc2dc.dc_current_16s =
                        dc2dc_get_current_16s_of_amper(i, &err);
        vm.loop[i].dc2dc.dc_power_watts_16s = 
        vm.loop[i].dc2dc.dc_current_16s*VTRIM_TO_VOLTAGE_MILLI(vm.loop_vtrim[i])/1000;
    } else {
      // This will disable ac2dc scaling
      vm.loop[i].dc2dc.dc_current_16s = 0;
      vm.loop[i].dc2dc.dc_power_watts_16s = 0;
      //printf(GREEN "Loop %d current not saved = %d" RESET, i, vm.loop[i].dc2dc.dc_current_16s *1000/16);
    }
  }
  return 0;
}



// returns AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int *err) {
  // TODO - select loop!
  // int err = 0;
  passert(err != NULL);
  //printf("%s:%d\n",__FILE__, __LINE__);

  pthread_mutex_lock(&i2c_mutex);

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
    pthread_mutex_unlock(&i2c_mutex);
    return 0;
  }
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  return current;
}

int dc2dc_get_voltage(int loop, int *err) {
  //*err = 0;
  passert(err != NULL);
  static int warned = 0;
  int voltage;
  //printf("%s:%d\n",__FILE__, __LINE__);
  
   pthread_mutex_lock(&i2c_mutex);
  dc2dc_select_i2c(loop, err);
  voltage = i2c_read_word(I2C_DC2DC, 0x8b, err) * 1000 / 512;
  if (*err) {
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
    return 0;
  }
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  return voltage;
}

int dc2dc_get_temp(int loop, int *err) {
  // returns max temperature of the 2 sensors
  passert(err != NULL);
  int temp1, temp2;
  //printf("%s:%d\n",__FILE__, __LINE__);

  pthread_mutex_lock(&i2c_mutex);
  dc2dc_select_i2c(loop, err);
  dc2dc_set_channel(0, err);
  temp1 = i2c_read_word(I2C_DC2DC, 0x8e, err) /* *1000/512 */;
  if (*err) {
    dc2dc_i2c_close();
      pthread_mutex_unlock(&i2c_mutex);
    return 0;
  }
  dc2dc_set_channel(1, err);
  temp2 = i2c_read_word(I2C_DC2DC, 0x8e, err) /* *1000/512 */;
  if (*err) {
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
    return 0;
  }
  if (temp2 > temp1)
    temp1 = temp2;
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  return temp1;
}


