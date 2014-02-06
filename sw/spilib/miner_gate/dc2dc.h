#ifndef _____DC2DC__45R_H____
#define _____DC2DC__45R_H____





 typedef enum {
    ASIC_VOLTAGE_0   =    0,
    ASIC_VOLTAGE_555   =  1,
    ASIC_VOLTAGE_585   =  2,
    ASIC_VOLTAGE_630   =  3,
    ASIC_VOLTAGE_675   =  4,
    ASIC_VOLTAGE_720   =  5,
    ASIC_VOLTAGE_765   =  6,
    ASIC_VOLTAGE_790   =  7,
    ASIC_VOLTAGE_810   =  8,
    ASIC_VOLTAGE_COUNT =  9
 } DC2DC_VOLTAGE;


#define VOLTAGE_ENUM_TO_MILIVOLTS(ENUM, VALUE) {int xxx[ASIC_VOLTAGE_COUNT] = {0,555,585,630,675,720,765,790,810}; VALUE=xxx[ENUM];}


void dc2dc_init();

// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function
void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE);

// Returns value like 810, 765 etc`
int dc2dc_get_voltage(int loop);

// return AMPERS
int dc2dc_get_current(int loop);


#endif
