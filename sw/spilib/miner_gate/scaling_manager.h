#ifndef _____DC2SCALINGME__45R_H____
#define _____DC2SCALINGME__45R_H____

#define ASIC_SPEED_RAISE_TIME 10 // seconds between ASIC speed changes UP

void decrease_asics_freqs();
int test_serial(int loopid);
void print_scaling();
int can_be_downscaled(HAMMER *a);
int can_be_upscaled(HAMMER *a);
void reset_all_asics_full_reset();


#endif
