#ifndef _____DC2DC__45R_H____
#define _____DC2DC__45R_H____

#include "defines.h"
#include <stdint.h>
#include <unistd.h>


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


#define VTRIM_MIN 0x0FFc5  // 0.635
#define VTRIM_MEDIUM 0x0ffdd //
#define VTRIM_MAX 0x10008  // 0.810

#define VTRIM_TO_VOLTAGE_MILLI(XX)    ((63500 + (XX-VTRIM_MIN)*(266))/100)  

/*
#define VOLTAGE_ENUM_TO_MILIVOLTS(ENUM, VALUE)                                 \
  {                                                                            \
    int xxx[ASIC_VOLTAGE_COUNT] = { 0, 555, 585, 630, 675, 681, 687, 693, 700, \
                                    705, 710, 715, 720, 765, 790,     \
                                    810 };                                     \
    VALUE = xxx[ENUM];                                                         \
  }
*/
  int get_dc2dc_error(int loop);

void dc2dc_disable_dc2dc(int loop, int *err);
void dc2dc_enable_dc2dc(int loop, int *err);
void dc2dc_init();
int update_dc2dc_current_temp_measurments(int loop);

// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function
//void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v, int *err);
void dc2dc_set_vtrim(int loop, uint32_t vtrim, int *err);


// Returns value like 810, 765 etc`
int dc2dc_get_voltage(int loop, int *err);
int dc2dc_get_temp(int loop, int *err);

// return AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int *err);

#endif
