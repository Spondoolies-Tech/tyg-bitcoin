#ifndef _____AC2DCSCALINGME__32R_H____
#define _____AC2DCSCALINGME__32R_H____
#include "defines.h"


void decrease_asics_freqs();
int can_be_downscaled(HAMMER *a);
int can_be_upscaled(HAMMER *a);
// Called each second.
void change_dc2dc_voltage_if_needed();
void asic_frequency_update_with_bist();
void dc2dc_scaling_once_second();


/*
  HAMMER *find_asic_to_increase_speed(int l);
  void asic_upscale(HAMMER *a);
  */



#endif

