#ifndef _____DC2SCALINGME__45R_H____
#define _____DC2SCALINGME__45R_H____

#define ASIC_SPEED_RAISE_TIME 10 // seconds between ASIC speed changes UP

void periodic_upscale_task();
void decrease_asics_freqs();
void enable_engines_from_nvm();

#define TIME_FOR_DLL_USECS  1000
#define MIN_COSECUTIVE_JOBS_FOR_SCALING 200

#endif
