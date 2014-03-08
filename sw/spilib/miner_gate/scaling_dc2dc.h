#ifndef _____DC2DCSCALINGME__45R_H____
#define _____DC2DCSCALINGME__45R_H____
#include "defines.h"


void decrease_asics_freqs();
int asic_can_down(HAMMER *a);
int asic_can_up(HAMMER *a,  int force);
void asic_up(HAMMER *a);

// Called each second.
void change_dc2dc_voltage_if_needed();
void asic_scaling_once_second(int force);
void pause_asics_if_needed();
void asic_frequency_update(int verbal = 0);
void resume_asics_if_needed();


#endif
