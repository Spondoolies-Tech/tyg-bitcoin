#include "ac2dc_const.h"
#include "ac2dc.h"
#include "dc2dc.h"
#include "i2c.h"
#include "hammer.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 

extern MINER_BOX vm;
extern pthread_mutex_t i2c_mutex;

void do_stupid_i2c_workaround() {
   //system("i2cdetect -y -r 0 >> /dev/null");
}
static int murata = 0;
static int eeprom_addr[2] = {AC2DC_EMERSON_I2C_EEPROM_DEVICE, AC2DC_MURATA_I2C_EEPROM_DEVICE};
static int mgmt_addr[2] = {AC2DC_EMERSON_I2C_MGMT_DEVICE, AC2DC_MURATA_I2C_MGMT_DEVICE};

int ac2dc_getint(int source) {
  int n = (source & 0xF800) >> 11;
  int negative = false;
 // printf("N=%d\n", n);
  // This is shitty 2s compliment on 5 bits.
  if (n & 0x10) {
    negative = true;
    n = (n ^ 0x1F) + 1;
  }
  int y = (source & 0x07FF);
   // printf("Y=%d\n", y);
   // printf("RES:%d\n", (y * 1000) / (1 << n));
  if (negative)
    return (y * 1000) / (1 << n);
  else
    return (y * 1000) * (1 << n);
}

// Return Watts
static int ac2dc_get_power() {
  int err = 0;
  static int warned = 0;
  int r;

  do_stupid_i2c_workaround();
  r = i2c_read_word(mgmt_addr[murata], AC2DC_I2C_READ_POUT_WORD, &err);
  if (err) {
    psyslog("RESET I2C BUS?\n");
    system("echo 111 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio111/direction");
    system("echo 0 > /sys/class/gpio/gpio111/value");
    usleep(1000000);
    system("echo 111 > /sys/class/gpio/export");
    passert(0);
  }
  int power = ac2dc_getint(r); //TODOZ

  if (err) {
    if ((warned++) < 10)
      psyslog( "FAILED TO INIT AC2DC\n" );
    if ((warned) == 9)
      psyslog( "FAILED TO INIT AC2DC, giving up :(\n" );
     //pthread_mutex_unlock(&i2c_mutex);
    return 100;
  }
  //pthread_mutex_unlock(&i2c_mutex);

  return power;
}


void ac2dc_init() {
  int err;
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PIN);
  int res = i2c_read_word(AC2DC_EMERSON_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP1_WORD, &err);
  if (!err) {
    psyslog("EMERSON AC2DC LOCATED\n");
    murata = 0;
  } else {
    psyslog("MURATA AC2DC LOCATED\n");
    murata = 1;
  }
  i2c_write(PRIMARY_I2C_SWITCH, 0);
}


// Return Watts
static int ac2dc_get_temperature() {
  static int warned = 0;
  //psyslog("%s:%d\n",__FILE__, __LINE__);


  int err = 0;
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PIN);
  do_stupid_i2c_workaround();
  int res = i2c_read_word(mgmt_addr[murata], AC2DC_I2C_READ_TEMP1_WORD, &err);
  if (err) {

  }

  int temp1 = ac2dc_getint(res);
  if (err) {
    psyslog(RED "ERR reading AC2DC temp\n" RESET);
    // pthread_mutex_unlock(&i2c_mutex);
    return AC2DC_TEMP_GREEN_LINE - 1;
  }
  do_stupid_i2c_workaround();
  int temp2 = ac2dc_getint(
      i2c_read_word(mgmt_addr[murata], AC2DC_I2C_READ_TEMP2_WORD, &err));
  do_stupid_i2c_workaround(); 
  int temp3 = ac2dc_getint(
      i2c_read_word(mgmt_addr[murata], AC2DC_I2C_READ_TEMP3_WORD, &err));
  if (temp2 > temp1)
    temp1 = temp2;
  if (temp3 > temp1)
    temp1 = temp3;

  //i2c_write(PRIMARY_I2C_SWITCH, 0x00);

  return temp1 / 1000;
}

#ifndef MINERGATE
int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd) {

	int rc = 0;
	int err = 0;
  int pnr_offset = 0x34;
  int pnr_size = 15;
  int model_offset = 0x57;
  int model_size = 4;
  int serial_offset = 0x5b;
  int serial_size = 10;
  int revision_offset = 0x61;
  int revision_size = 2;


  if (NULL == pVpd) {
    psyslog("call ac2dc_get_vpd performed without allocating sturcture first\n");
    return 1;
  }
  //psyslog("%s:%d\n",__FILE__, __LINE__);
  

  pthread_mutex_lock(&i2c_mutex);

  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PIN  , &err);

  if (err) {
    fprintf(stderr, "Failed writing to I2C address 0x%X (err %d)",
            PRIMARY_I2C_SWITCH, err);
     pthread_mutex_unlock(&i2c_mutex);
    return err;
  }

  for (int i = 0; i < pnr_size; i++) {
    pVpd->pnr[i] = ac2dc_get_eeprom_quick(pnr_offset + i, &err);
    if (err)
      goto ac2dc_get_eeprom_quick_err;
  }

  for (int i = 0; i < model_size; i++) {
    pVpd->model[i] = ac2dc_get_eeprom_quick(model_offset + i, &err);
    if (err)
      goto ac2dc_get_eeprom_quick_err;
  }

  for (int i = 0; i < revision_size; i++) {
    pVpd->revision[i] = ac2dc_get_eeprom_quick(revision_offset + i, &err);
    if (err)
      goto ac2dc_get_eeprom_quick_err;
  }


  for (int i = 0; i < serial_size; i++) {
    pVpd->serial[i] = ac2dc_get_eeprom_quick(serial_offset + i, &err);
    if (err)
      goto ac2dc_get_eeprom_quick_err;
  }


ac2dc_get_eeprom_quick_err:

  if (err) {
    fprintf(stderr, RED            "Failed reading AC2DC eeprom (err %d)\n" RESET, err);
    rc =  err;
  }

  i2c_write(PRIMARY_I2C_SWITCH, 0x00);
  pthread_mutex_unlock(&i2c_mutex);

  return rc;
}
#endif

// this function assumes as a precondition
// that the i2c bridge is already pointing to the correct device
// needed to read ac2dc eeprom
// no side effect either
// use this funtion when performing serial multiple reads
#ifndef MINERGATE
unsigned char ac2dc_get_eeprom_quick(int offset, int *pError) {


  //pthread_mutex_lock(&i2c_mutex); no lock here !!

  unsigned char b =
      (unsigned char)i2c_read_byte(eeprom_addr[murata], offset, pError);

  return b;
}
#endif

// no precondition as per i2c
// and thus sets switch first,
// and then sets it back
// side effect - it closes the i2c bridge when finishes.
#ifndef MINERGATE
int ac2dc_get_eeprom(int offset, int *pError) {
  // Stub for remo
  
  //printf("%s:%d\n",__FILE__, __LINE__);
   pthread_mutex_lock(&i2c_mutex);
  int b;
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PIN, pError);
  if (pError && *pError) {
     pthread_mutex_unlock(&i2c_mutex);
    return *pError;
  }

  b = i2c_read_byte(eeprom_addr[murata], offset, pError);
  i2c_write(PRIMARY_I2C_SWITCH, 0x00);
   pthread_mutex_unlock(&i2c_mutex);
  return b;
}
#endif

void reset_i2c() {
  /*
  FILE *f = fopen("/sys/class/gpio/export", "w");
  if (!f)
    return;
  fprintf(f, "111");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio111/direction", "w");
  if (!f)
    return;
  fprintf(f, "out");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio47/value", "w");
  if (!f)
    return;
  fprintf(f, "0");
  usleep(1000000);
  fprintf(f, "1");
  usleep(1000000);
  fclose(f);
  */
}



#ifdef MINERGATE
int update_ac2dc_power_measurments() {
  int err;
  static int counter = 0;
  counter++;
  pthread_mutex_lock(&i2c_mutex);
 
  reset_i2c();
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PIN);
  //do_stupid_i2c_workaround();
#if 0//AC2DC_BUG == 0  
  vm.ac2dc_temp = ac2dc_get_temperature();
#endif

  int power_guessed = (vm.dc2dc_total_power*1000/790)+60;// ac2dc_get_power()/1000; //TODOZ

  int power = power_guessed;
#if AC2DC_BUG == 0  
  power = ac2dc_get_power()/1000;
#endif
  //printf(CYAN"power=%d(guessed=%d)\n" RESET, power,power_guessed);

  if (
    !vm.asics_shut_down_powersave &&
    power >= AC2DC_CURRENT_TRUSTWORTHY && 
    vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_AC2DC_MEASUREMENT) {
      vm.ac2dc_power = power;
    } else {
      //printf(GREEN "CONSEC = %d\n" RESET, vm.cosecutive_jobs); 
      vm.ac2dc_power = 0;
    }
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_AC2DC_PIN);  
  pthread_mutex_unlock(&i2c_mutex);  
  return 0;
}
#endif

