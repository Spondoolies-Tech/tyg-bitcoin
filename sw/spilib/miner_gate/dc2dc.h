#ifndef _____DC2DC__45R_H____
#define _____DC2DC__45R_H____





 typedef enum {
    ASIC_VOLTAGE_555   =  0,
    ASIC_VOLTAGE_585   =  1,
    ASIC_VOLTAGE_630   =  2,
    ASIC_VOLTAGE_675   =  3,
    ASIC_VOLTAGE_720   =  4,
    ASIC_VOLTAGE_765   =  5,
    ASIC_VOLTAGE_790   =  6,
    ASIC_VOLTAGE_810   =  7,
    ASIC_VOLTAGE_COUNT =  8
 } DC2DC_VOLTAGE;


void dc2dc_init();

// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function
void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE);

// Returns value like 810, 765 etc`
int dc2dc_get_voltage(int loop);

// return AMPERS
int dc2dc_get_current(int loop);


#endif
