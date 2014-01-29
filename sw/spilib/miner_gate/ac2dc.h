#ifndef _____AC2DC__4544R_H____
#define _____AC2DC__4544R_H____


int ac2dc_get_power();
int ac2dc_get_temperature(int id);
unsigned char ac2dc_get_eeprom_quick(int offset,int * pError = 0);

typedef struct {
	unsigned char pnr[16];
	unsigned char model[5];
	unsigned char revision[3];
	unsigned char serial[11];// WW[2]+SER[4]+REV[2]+PLANT[2]
} ac2dc_vpd_info_t;

int ac2dc_get_vpd( ac2dc_vpd_info_t * pVpd);


/*
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



// in takes 0.2second for voltage to be stable.
// Remember to wait after you exit this function
void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE);

// Returns value like 810, 765 etc`
int dc2dc_get_voltage(int loop);
*/
#endif
