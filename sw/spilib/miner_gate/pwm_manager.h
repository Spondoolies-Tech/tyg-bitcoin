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
void auto_select_fan_level();

#endif
