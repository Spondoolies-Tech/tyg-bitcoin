#ifndef _____PWMLIB_H__B__
#define _____PWMLIB_H__B__

#include <stdint.h>
#include <unistd.h>
//#include "dc2dc.h"

#define ASIC_TEMPERATURE_TO_SET_FANS_LOW ASIC_TEMP_83
#define ASIC_TEMPERATURE_TO_SET_FANS_HIGH ASIC_TEMP_89

typedef enum _FAN_LEVEL {
  FAN_LEVEL_LOW = 0,
  FAN_LEVEL_MEDIUM = 1,
  FAN_LEVEL_HIGH = 2,
  FAN_LEVEL_COUNT = 3

} FAN_LEVEL;

void init_pwm();
void set_fan_level(FAN_LEVEL fan_level);
void auto_select_fan_level();

#endif
