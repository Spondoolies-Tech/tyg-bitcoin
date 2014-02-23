#ifndef _____AC2DC__4544R_H____
#define _____AC2DC__4544R_H____

#define AC2DC_TEMP_GREEN_LINE 110
#define AC2DC_TEMP_RED_LINE 115

#define MIN_COSECUTIVE_JOBS_FOR_AC2DC_MEASUREMENT 200

#define AC2DC_POWER_TRUSTWORTHY (200)
#define AC2DC_POWER_GREEN_LINE (1040)
#define AC2DC_POWER_RED_LINE (1080)

int ac2dc_get_power();
unsigned char ac2dc_get_eeprom_quick(int offset, int *pError = 0);

typedef struct {
  unsigned char pnr[16];
  unsigned char model[5];
  unsigned char revision[3];
  unsigned char serial[11]; // WW[2]+SER[4]+REV[2]+PLANT[2]
} ac2dc_vpd_info_t;

int ac2dc_get_vpd(ac2dc_vpd_info_t *pVpd);
int ac2dc_get_temperature(); // 0,1,2
void ac2dc_print();
int ac2dc_spare_power();

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


*/
#endif
