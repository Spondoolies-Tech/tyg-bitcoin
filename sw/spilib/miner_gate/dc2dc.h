#ifndef _____DC2DC__45R_H____
#define _____DC2DC__45R_H____

#include "defines.h"


typedef enum {
  ASIC_VOLTAGE_0 ,
  ASIC_VOLTAGE_555 ,
  ASIC_VOLTAGE_585 ,
  ASIC_VOLTAGE_630 ,
  ASIC_VOLTAGE_675 ,
  ASIC_VOLTAGE_681 ,
  ASIC_VOLTAGE_687 ,
  ASIC_VOLTAGE_693 ,
  ASIC_VOLTAGE_700 , 
  ASIC_VOLTAGE_705 , 
  ASIC_VOLTAGE_710 , 
  ASIC_VOLTAGE_715 , 
  ASIC_VOLTAGE_720 ,
  ASIC_VOLTAGE_765 ,
  ASIC_VOLTAGE_790 ,
  ASIC_VOLTAGE_810 ,
  ASIC_VOLTAGE_COUNT 
} DC2DC_VOLTAGE;


#define VTRIM_MIN 0xFFd4  // 0.675
#define VTRIM_MEDIUM 0xffe3 
#define VTRIM_MAX 0xfff7  // 0.765

#define VTIRM_TO_VOLTAGE(XX)    (675 + (XX-VTRIM_MIN)*(3))  


#define VOLTAGE_ENUM_TO_MILIVOLTS(ENUM, VALUE)                                 \
  {                                                                            \
    int xxx[ASIC_VOLTAGE_COUNT] = { 0, 555, 585, 630, 675, 681, 687, 693, 700, \
                                    705, 710, 715, 720, 765, 790,     \
                                    810 };                                     \
    VALUE = xxx[ENUM];                                                         \
  }

void dc2dc_i2c_close();
int get_dc2dc_error(int loop);
void dc2dc_select_i2c(int loop, int *err);
void dc2dc_set_channel(int channel_mask, int *err);

void dc2dc_disable_dc2dc(int loop, int *err);
void dc2dc_enable_dc2dc(int loop, int *err);
void dc2dc_print();
void dc2dc_init();
int update_dc2dc_current_temp_measurments(int loop);

// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function
//void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v, int *err);
void dc2dc_set_vtrim(int loop, int vtrim, int *err);


// Returns value like 810, 765 etc`
int dc2dc_get_voltage(int loop, int *err);
int dc2dc_get_temp(int loop, int *err);

// return AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int *err);

#endif
