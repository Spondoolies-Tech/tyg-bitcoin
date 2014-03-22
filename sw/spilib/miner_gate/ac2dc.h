/*
 * Copyright 2014 Zvi (Zvisha) Shteingart - Spondoolies-tech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 *
 * Note that changing this SW will void your miners guaranty
 */

#ifndef _____AC2DC__4544R_H____
#define _____AC2DC__4544R_H____
#include "defines.h"


unsigned char ac2dc_get_eeprom_quick(int offset, int *pError = 0);

typedef struct {
  unsigned char pnr[16];
  unsigned char model[5];
  unsigned char revision[3];
  unsigned char serial[11]; // WW[2]+SER[4]+REV[2]+PLANT[2]
} ac2dc_vpd_info_t;

int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd);
int update_ac2dc_power_measurments();
void ac2dc_init();


#endif
