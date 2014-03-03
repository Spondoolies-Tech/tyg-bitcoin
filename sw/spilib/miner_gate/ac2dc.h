#ifndef _____AC2DC__4544R_H____
#define _____AC2DC__4544R_H____
#include "defines.h"


int ac2dc_get_power();
unsigned char ac2dc_get_eeprom_quick(int offset, int *pError = 0);

typedef struct {
  unsigned char pnr[16];
  unsigned char model[5];
  unsigned char revision[3];
  unsigned char serial[11]; // WW[2]+SER[4]+REV[2]+PLANT[2]
} ac2dc_vpd_info_t;

int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd);
int ac2dc_get_temperature(); // 0,1,2
void ac2dc_print();
int update_ac2dc_current_measurments();


#endif
