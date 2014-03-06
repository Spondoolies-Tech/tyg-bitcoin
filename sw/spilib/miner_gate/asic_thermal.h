#ifndef _____DCASSTHERMALIXT__45R_H____
#define _____DCASSTHERMALIXT__45R_H____

#include "defines.h"

void read_some_asic_temperatures(int loop, ASIC_TEMP temp_measure_temp0, ASIC_TEMP temp_measure_temp1);
void thermal_init();
int read_asic_temperature(int addr, ASIC_TEMP temp_measure_temp0, ASIC_TEMP temp_measure_temp1);
void enable_thermal_shutdown();




#endif
