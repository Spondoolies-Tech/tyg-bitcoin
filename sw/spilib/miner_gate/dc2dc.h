#ifndef _____DC2DC__45R_H____
#define _____DC2DC__45R_H____

#define DC2DC_TEMP_GREEN_LINE 90
#define DC2DC_TEMP_RED_LINE 100

#define MIN_COSECUTIVE_JOBS_FOR_DC2DC_MEASUREMENT 200
#define DC2DC_POWER_TRUSTWORTHY      (5*16)
#define DC2DC_CURRENT_GREEN_LINE_16S (40 * 16)
#define DC2DC_CURRENT_RED_LINE_16S   (45 * 16)

typedef enum {
  ASIC_VOLTAGE_0 = 0,
  ASIC_VOLTAGE_555 = 1,
  ASIC_VOLTAGE_585 = 2,
  ASIC_VOLTAGE_630 = 3,
  ASIC_VOLTAGE_675 = 4,
  ASIC_VOLTAGE_720 = 5,
  ASIC_VOLTAGE_765 = 6,
  ASIC_VOLTAGE_790 = 7,
  ASIC_VOLTAGE_810 = 8,
  ASIC_VOLTAGE_COUNT = 9
} DC2DC_VOLTAGE;

#define VOLTAGE_ENUM_TO_MILIVOLTS(ENUM, VALUE)                                 \
  {                                                                            \
    int xxx[ASIC_VOLTAGE_COUNT] = { 0, 555, 585, 630, 675, 720, 765, 790,      \
                                    810 };                                     \
    VALUE = xxx[ENUM];                                                         \
  }

void dc2dc_i2c_close();
void dc2dc_select_i2c(int loop, int *err);
void dc2dc_set_channel(int channel_mask, int *err);

void dc2dc_disable_dc2dc(int loop, int *err);
void dc2dc_enable_dc2dc(int loop, int *err);
void dc2dc_print();
void dc2dc_init();
int update_dc2dc_current_measurments();

// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function
void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v, int *err);

// Returns value like 810, 765 etc`
int dc2dc_get_voltage(int loop, int *err);
int dc2dc_get_temp(int loop, int *err);

// return AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int *err);

#endif
