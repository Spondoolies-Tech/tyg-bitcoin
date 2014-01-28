
#include "ac2dc_const.h"
#include "dc2dc.h"
#include "i2c.h"




int ac2dc_get_power() {
	i2c_write(PRIMARY_I2C_SWITCH, 0x01); 
	int power = i2c_read_word(AC2DC_I2C_EEPROM_DEVICE, AC2DC_I2C_READ_POUT_WORD)*1000/512;
	i2c_write(PRIMARY_I2C_SWITCH, 0x00);
    return power;
}


int ac2dc_get_eeprom(int offset) {
	// Stub for remo
	i2c_write(PRIMARY_I2C_SWITCH, 0x01);
	int b = i2c_read_byte(AC2DC_I2C_MGMT_DEVICE, offset);
	i2c_write(PRIMARY_I2C_SWITCH, 0x00);
	return b;
}



/*
int volt_to_vtrim[ASIC_VOLTAGE_COUNT] = {
    0xFFC4 ,  0xFFCF, 0xFFE1, 0xFFd4,  0xFFE5,0xfff7, 0x0, 0x8,
};

int volt_to_vmargin[ASIC_VOLTAGE_COUNT] = {
    0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0
};



void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v) {
    printf("%d\n",v);
    assert(v<ASIC_VOLTAGE_COUNT && v>=0);
    i2c_write_word(0x1b, 0xd4, volt_to_vtrim[v]);
    i2c_write_byte(0x1b, 0x01, volt_to_vmargin[v]);
}




int dc2dc_get_voltage(int loop) {
    int voltage = ASIC_VOLTAGE_810;
    voltage = i2c_read_word(0x1b, 0x8b)*1000/512;


    return voltage;
}
*/

