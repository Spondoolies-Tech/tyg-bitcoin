#ifndef _____DC2SCALINGME__45R_H____
#define _____DC2SCALINGME__45R_H____

#define ASIC_SPEED_RAISE_TIME 10 // seconds between ASIC speed changes UP

void periodic_upscale_task();
void peridic_current_throttle_task();
void enable_engines_from_nvm();

#endif
