#ifndef _____DC2DCSCALINGME__45R_H____
#define _____DC2DCSCALINGME__45R_H____
#include "defines.h"


void decrease_asics_freqs();
int can_be_downscaled(HAMMER *a);
int can_be_upscaled(HAMMER *a);
// Called each second.
void change_dc2dc_voltage_if_needed();
void asic_frequency_update_with_bist();
void dc2dc_scaling_once_second();
void pause_asics_if_needed();

#endif
