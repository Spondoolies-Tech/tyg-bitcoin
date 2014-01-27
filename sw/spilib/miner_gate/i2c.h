
#ifndef _____HAMMER_I2C_H____
#define _____HAMMER_I2C_H____

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
//#include "i2cbusses.h"


// set voltage test
 //i2c_write_word(0x1b, 0xd4, 0xFFD4);
 //i2c_write_byte(0x1b, 0x01, 0x00);
 //usleep(1000000);
 //return 0;

#define PRIMARY_I2C_SWITCH 0x70

void i2c_init();
uint8_t i2c_read(uint8_t addr);
void i2c_write(uint8_t addr, uint8_t value);

uint8_t i2c_read_byte(uint8_t addr, uint8_t command);
void i2c_write_byte(uint8_t addr, uint8_t command, uint8_t value);
uint16_t i2c_read_word(uint8_t addr, uint8_t command);
void i2c_write_word(uint8_t addr, uint8_t command, uint16_t value);


uint16_t read_mgmt_temp();


#endif 
