
#include "i2c.h"
#include "asic_testboard.h"




// micro ampers
int tb_get_asic_current(int id) {
	 
	  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TESTBOARD_PIN);
	  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x01 << id/2);
	  int addr = 0x4c+id%2;
	  i2c_write_byte(addr, 0x1, 0x1a);
	  i2c_write_byte(addr, 0x2, 0x01);
	  int error;
	  usleep(1000);
	  unsigned int s6 = i2c_read_byte(addr,0x6,&error);
	  if (error) printf("i2c Error dfjs\n");
	  unsigned int s7 = i2c_read_byte(addr,0x7,&error);
	  if (error) printf("i2c Error dfjs\n");
	  if (s7 == 0xff && s6 == 0xff) {}
	  unsigned int s8 = ((s7 | (s6 & 0x3f) << 8) * 3884);
	  i2c_write(PRIMARY_I2C_SWITCH, 0);
      i2c_write(PRIMARY_TESTBOARD_SWITCH, 0);

	  return s8;


}




// micro ampers
int tb_get_asic_voltage(int id) {

	  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TESTBOARD_PIN);
	  i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x01 << id/2);
	  int addr = 0x4c+id%2;
	  i2c_write_byte(addr, 0x1, 0x1a);
	  i2c_write_byte(addr, 0x2, 0x01);
	  int error;
	  usleep(1000);
	  unsigned int sa = i2c_read_byte(addr,0xa,&error);
	  if (error) printf("i2c Error dfjs\n");
	  unsigned int sb = i2c_read_byte(addr,0xb,&error);
	  if (error) printf("i2c Error dfjs\n");
	  if (sa == 0xff && sb == 0xff) {}
	  unsigned int s8 = ((sb | (sa & 0x3f) << 8) * 305);
	  i2c_write(PRIMARY_I2C_SWITCH, 0);
      i2c_write(PRIMARY_TESTBOARD_SWITCH, 0);
	  printf("%d\n",s8);

	  return s8;


}










