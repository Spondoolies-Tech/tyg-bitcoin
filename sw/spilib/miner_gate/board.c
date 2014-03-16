#include "dc2dc.h"
#include "i2c.h"
#include "nvm.h"
#include "hammer.h"
#include <time.h>
#include <pthread.h>

#include "defines.h"
#include "scaling_manager.h"





int get_mng_board_temp() {
  int err;
  int temp;
  int reg;  
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_MGMT_PIN);
  reg = i2c_read_word(I2C_MGMT_THERMAL_SENSOR, 0x0, &err);
  reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
  temp = (reg*625)/10000;
  i2c_write(PRIMARY_I2C_SWITCH, 0);   

  return temp;
}

int get_top_board_temp() {
  int err;
   int temp;
   int reg;  
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TOP_MAIN_PIN);
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0x80);   
   reg = i2c_read_word(I2C_MAIN_THERMAL_SENSOR, 0x0, &err);
   reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
   temp = (reg*625)/10000;
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0);      
   i2c_write(PRIMARY_I2C_SWITCH, 0);   
   return temp;
}

int get_bottom_board_temp() {
   int err;
   int temp;
   int reg;  
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN);
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0x80);   
   reg = i2c_read_word(I2C_MAIN_THERMAL_SENSOR, 0x0, &err);
   reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
   temp = (reg*625)/10000;
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0);      
   i2c_write(PRIMARY_I2C_SWITCH, 0);   
   return temp;
}




