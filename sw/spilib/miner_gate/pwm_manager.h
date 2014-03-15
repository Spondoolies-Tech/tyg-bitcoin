#ifndef _____PWMLIB_H__B__
#define _____PWMLIB_H__B__

#include <stdint.h>
#include <unistd.h>
#include <defines.h>

//#include "dc2dc.h"
// DONT USE UNLESS YOU KNOW WHAT YOU DOING
void kill_fan();
void init_pwm();
void set_fan_level(int fan_level_percent);
void leds_init();

#define LIGHT_GREEN  0
#define LIGHT_YELLOW 1
void set_light(int light_id, bool on);


#endif
