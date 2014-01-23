#include "i2c.h"
#include "i2c-mydev.h"
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
//#include <linux/i2c-dev.h>


























static int file;
static pthread_mutex_t i2cm  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t i2cm_trans  = PTHREAD_MUTEX_INITIALIZER;


/*

78 extern s32 i2c_smbus_read_byte(struct i2c_client *client);
79 extern s32 i2c_smbus_write_byte(struct i2c_client *client, uint8_t value);
80 extern s32 i2c_smbus_read_byte_data(struct i2c_client *client, uint8_t command);
81 extern s32 i2c_smbus_write_byte_data(struct i2c_client *client,
82                                      uint8_t command, uint8_t value);
83 extern s32 i2c_smbus_read_word_data(struct i2c_client *client, uint8_t command);
84 extern s32 i2c_smbus_write_word_data(struct i2c_client *client,
85                                      uint8_t command, uint16_t value);

*/
    
static char buf[10] = {0};
void passert(int cond, const char *s);


// set the I2C slave address for all subsequent I2C device transfers
static void i2c_set_address(int address)
{
    assert(file);
	if (ioctl(file, I2C_SLAVE, address) < 0) {
		passert(0,"i2c-ff");
	}
}

uint8_t i2c_read(uint8_t addr) {
    uint8_t res;
    pthread_mutex_lock(&i2cm);
    i2c_set_address(addr);
    res = i2c_smbus_read_byte(file);
    pthread_mutex_unlock(&i2cm);
    return res;
}

void i2c_write(uint8_t addr, uint8_t value){
      pthread_mutex_lock(&i2cm);    
      i2c_set_address(addr);
      i2c_smbus_write_byte(file, value);
      pthread_mutex_unlock(&i2cm);        
}

uint8_t i2c_read_byte(uint8_t addr, uint8_t command) {
    uint8_t res;
    pthread_mutex_lock(&i2cm);
    i2c_set_address(addr);
    __s32 r = i2c_smbus_read_byte_data(file, command);
    //printf("i2c[%x:%x] -> %x\n",addr, command, r);
    pthread_mutex_unlock(&i2cm);
    return r;
}

uint16_t i2c_read_w(uint8_t addr) {
    uint16_t res;
    pthread_mutex_lock(&i2cm);
    i2c_set_address(addr);
    res = i2c_smbus_read_word_data(file , 0);
    pthread_mutex_unlock(&i2cm);
    return res;
}

void i2c_write_w(uint8_t addr, uint16_t value){
      pthread_mutex_lock(&i2cm);    
      i2c_set_address(addr);
      i2c_smbus_write_word_data(file,0 ,value);
      pthread_mutex_unlock(&i2cm);        
}


void i2c_write_byte(uint8_t addr, uint8_t command, uint8_t value){
    pthread_mutex_lock(&i2cm);    
    i2c_set_address(addr);
    i2c_smbus_write_byte_data(file, command, value);
    pthread_mutex_unlock(&i2cm);        
}

uint16_t i2c_read_word(uint8_t addr, uint8_t command) {
    pthread_mutex_lock(&i2cm);        
    i2c_set_address(addr);
    __s32 r = i2c_smbus_read_word_data(file, command);
    //printf("i2c[%x:%x] -> %x\n",addr, command, r);
    pthread_mutex_unlock(&i2cm);
    return r;
}

void i2c_write_word(uint8_t addr, uint8_t command, uint16_t value) {
    pthread_mutex_lock(&i2cm);        
    i2c_set_address(addr);
    i2c_smbus_write_word_data(file, command, value);
    pthread_mutex_unlock(&i2cm);        
}






void i2c_init() {
    pthread_mutex_lock(&i2cm);        
    if ((file = open("/dev/i2c-0",O_RDWR)) < 0) {
         printf("Failed to open the i2c bus.\n");
         /* ERROR HANDLING; you can check errno to see what went wrong */
         exit(1);
    }
    pthread_mutex_unlock(&i2cm);        
}


uint16_t read_mgmt_temp() {
    pthread_mutex_lock(&i2cm_trans);
    // set router
    //     ./i2cset -y 0 0x70 0x00 0xff
    i2c_write_byte(0x70, 0x00, 0xff);
    // read value
    //     ./i2cget -y 0 0x48 0x00 w
    uint16_t value = i2c_read_word(0x48, 0x00);
    pthread_mutex_unlock(&i2cm_trans);
    printf("------------------ READ %x\n",value);
    return value;
}
 
