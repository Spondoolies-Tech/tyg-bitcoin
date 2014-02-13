
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
#include <stdint.h>
#include <unistd.h>
#include "spond_debug.h"

//#include "i2cbusses.h"


// set voltage test
 //i2c_write_word(0x1b, 0xd4, 0xFFD4);
 //i2c_write_byte(0x1b, 0x01, 0x00);
 //usleep(1000000);
 //return 0;

#define PRIMARY_I2C_SWITCH 	      0x70
#define I2C_DC2DC_SWITCH_GROUP0   0x71 // 0 to 7
#define I2C_DC2DC_SWITCH_GROUP1   0x72 // 8 to 11
#define I2C_DC2DC                 0x1b 




void i2c_init( int * pError = NULL);
uint8_t i2c_read(uint8_t addr, int * pError = NULL);
void i2c_write(uint8_t addr, uint8_t value, int * pError = NULL);
uint8_t i2c_read_byte(uint8_t addr, uint8_t command, int * pError = NULL);
void i2c_write_byte(uint8_t addr, uint8_t command, uint8_t value, int * pError = NULL);
uint16_t i2c_read_word(uint8_t addr, uint8_t command, int * pError = NULL);
void i2c_write_word(uint8_t addr, uint8_t command, uint16_t value, int * pError = NULL);


uint16_t read_mgmt_temp();


#endif 
