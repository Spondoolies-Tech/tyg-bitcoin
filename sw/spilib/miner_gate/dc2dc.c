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

// not locked
void dc2dc_init_loop(int loop) {
  
    int err;
    dc2dc_select_i2c(loop, &err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC1 %d\n" RESET, loop);
      dc2dc_i2c_close();
      return;
    }

    i2c_write_byte(I2C_DC2DC, 0x00, 0x81, &err);
    if (err) {
      psyslog(RED "FAILED TO INIT DC2DC2 %d\n" RESET, loop);
      dc2dc_i2c_close();
      return;
    }

    i2c_write_word(I2C_DC2DC, 0x35, 0xf028); 	// VIN ON
    i2c_write_word(I2C_DC2DC, 0x36, 0xf018); 	// VIN OFF(??)
    i2c_write_word(I2C_DC2DC, 0x38, 0x881f); 	// Inductor DCR
    i2c_write_word(I2C_DC2DC, 0x46, 0xf84B); 	// OC Fault
    i2c_write_word(I2C_DC2DC, 0x4a, 0xf846); 	// OC warn
    i2c_write_byte(I2C_DC2DC, 0x47, 0x3C);		// OC fault response
    i2c_write_byte(I2C_DC2DC, 0xd7, 0x03);		// PG limits
    i2c_write_byte(I2C_DC2DC, 0x02, 0x02);		// ON/OFF conditions

    //i2c_write(I2C_DC2DC, 0x15);
    //usleep(1000);
    i2c_write(I2C_DC2DC, 0x03);
    dc2dc_i2c_close();
}

void dc2dc_init() {
  //printf("%s:%d\n",__FILE__, __LINE__);
  
  pthread_mutex_lock(&i2c_mutex);
  int err = 0;
  // static int warned = 0;
  // Write defaults
  for (int loop = 0; loop < LOOP_COUNT; loop++) {
    dc2dc_init_loop(loop);
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
// disengage from scale manager if not needed
#ifdef MINERGATE
  if (vm.loop[loop].enabled_loop) {
#endif
	pthread_mutex_lock(&i2c_mutex);
    dc2dc_select_i2c(loop, err);
    i2c_write_byte(I2C_DC2DC, 0x02, 0x02, err);
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
  #ifdef MINERGATE
  }
#endif
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

  passert(vtrim >= VTRIM_MIN && vtrim <= vm.vtrim_max);

#ifndef __MBTEST__
	  printf("Set VOLTAGE Loop %d Milli:%d Vtrim:%x\n",loop, VTRIM_TO_VOLTAGE_MILLI(vtrim),vtrim);
#endif

  pthread_mutex_lock(&i2c_mutex);

  // printf("%d\n",v);
  // int err = 0;
  dc2dc_select_i2c(loop, err);
//  passert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
  i2c_write_word(I2C_DC2DC, 0xd4, vtrim&0xFFFF);
  i2c_write_byte(I2C_DC2DC, 0x01, 0);

  // disengage from scale manager if not needed
#ifdef MINERGATE
  vm.loop_vtrim[loop] = vtrim;
  vm.loop[loop].dc2dc.last_voltage_change_time = time(NULL);
#endif

  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
}


// returns AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int* overcurrent_err, int* overcurrent_warning ,uint8_t *temp ,int *err) {
  // TODO - select loop!
  // int err = 0;
  passert(err != NULL);
  //printf("%s:%d\n",__FILE__, __LINE__);

  pthread_mutex_lock(&i2c_mutex);
  *overcurrent_err = 0;
  static int warned = 0;
  int current = 0;
  dc2dc_select_i2c(loop, err);
  dc2dc_set_channel(0, err);
  *temp = i2c_read_word(I2C_DC2DC, 0x8e, err) /* *1000/512 */;
  current += (i2c_read_word(I2C_DC2DC, 0x8c) & 0x07FF);
  int problems = i2c_read_word(I2C_DC2DC, 0x7b);
  *overcurrent_err |= (problems & 0x80);
  *overcurrent_warning |= (problems & 0x20);
  if (problems) {
    i2c_write(I2C_DC2DC, 0x03);
  }
  dc2dc_set_channel(1, err);
  int cur2=(i2c_read_word(I2C_DC2DC, 0x8c) & 0x07FF);
  //printf("LOOP:%d: C0=%d, C1=%d\n", loop, current/16, cur2/16);
  current += cur2;
  *temp += i2c_read_word(I2C_DC2DC, 0x8e, err) /* *1000/512 */;
  problems = i2c_read_word(I2C_DC2DC, 0x7b);  
  *overcurrent_err |= (problems & 0x80);
  *overcurrent_warning |= (problems & 0x20);  
  if (problems) {
    i2c_write(I2C_DC2DC, 0x03);
  }
  dc2dc_set_channel(0x81, err);
  *temp = *temp/2;
  if (*err) {
    dc2dc_i2c_close();
    pthread_mutex_unlock(&i2c_mutex);
    return 0;
  }
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  return current;
}

#ifdef MINERGATE
// Return 1 if needs urgent scaling
int update_dc2dc_current_temp_measurments(int loop, int* overcurrent, int* overcurrent_warning) {
  int err;
  int i = loop;
  if (vm.loop[i].enabled_loop) {
  
    if (!vm.asics_shut_down_powersave) {
        int current = dc2dc_get_current_16s_of_amper(i, overcurrent, overcurrent_warning , &vm.loop[i].dc2dc.dc_temp , &err);

        if (*overcurrent != 0){
        	psyslog("DC2DC OC ERROR in LOOP %d !!\n", loop);
/*
        	psyslog("... last 4 previous measures of DC2DC %d !!\n", loop);
        	psyslog("...   %d\n", vm.loop[i].dc2dc.dc_current_16s_arr[0]);
        	psyslog("...   %d\n", vm.loop[i].dc2dc.dc_current_16s_arr[1]);
        	psyslog("...   %d\n", vm.loop[i].dc2dc.dc_current_16s_arr[2]);
        	psyslog("...   %d\n", vm.loop[i].dc2dc.dc_current_16s_arr[3]);
*/
        	psyslog("...   current measure is %d\n", current);
        }

//        vm.loop[i].dc2dc.dc_current_16s_arr[vm.loop[i].dc2dc.dc_current_16s_arr_ptr]= current;
//        vm.loop[i].dc2dc.dc_current_16s_arr_ptr=(vm.loop[i].dc2dc.dc_current_16s_arr_ptr+1)%4;
        // Remove noise
        //printf("READ  %d %d\n",loop,current,current);
        vm.loop[i].dc2dc.dc_current_16s = current; /*
          (vm.loop[i].dc2dc.dc_current_16s_arr[0] + 
           vm.loop[i].dc2dc.dc_current_16s_arr[1] +
           vm.loop[i].dc2dc.dc_current_16s_arr[2] +
           vm.loop[i].dc2dc.dc_current_16s_arr[3]) >> 2;*/
          
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
#endif

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
#if 0
  passert(err != NULL);
  int temp1;
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
  dc2dc_set_channel(1, err);
  dc2dc_i2c_close();
  pthread_mutex_unlock(&i2c_mutex);
  return temp1;
#endif
  return 0; // saveCPU
}


