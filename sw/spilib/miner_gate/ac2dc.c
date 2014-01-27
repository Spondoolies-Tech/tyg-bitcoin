
#include "dc2dc.h"
#include "i2c.h"

#define AC2DC_EEPROM_DEVICE 0x5f
#define AC2DC_MGMT_DEVICE 0x5f



// MGMT i2c registers
#define AC2DC_I2C_STATUS_WORD 0x79
#define AC2DC_I2C_VOUT_STATUS 0x7A
#define AC2DC_I2C_IOUT_STATUS 0x7B
#define AC2DC_I2C_VIN_STATUS 0x7C
#define AC2DC_I2C_TEMP_STATUS 0x7D
#define AC2DC_I2C_CML_STATUS  0x7E
#define AC2DC_I2C_FANSTATE_STATUS  0x81


#define AC2DC_I2C_READ_VIN_WORD 0x88
#define AC2DC_I2C_READ_IIN_WORD  0x89
#define AC2DC_I2C_READ_VCAP_WORD  0x8a
#define AC2DC_I2C_READ_VOUT_WORD  0x8b
#define AC2DC_I2C_READ_IOUT_WORD  0x8c
#define AC2DC_I2C_READ_TEMP1_WORD  0x8d
#define AC2DC_I2C_READ_TEMP2_WORD  0x8e
#define AC2DC_I2C_READ_TEMP3_WORD  0x8f
#define AC2DC_I2C_READ_FANSPEED_WORD  0x90
#define AC2DC_I2C_READ_POUT_WORD  0x96
#define AC2DC_I2C_READ_PIN_WORD  0x97
















int ac2dc_get_power() {
	i2c_write(PRIMARY_I2C_SWITCH, 0x01); 
	int power = i2c_read_word(AC2DC_EEPROM_DEVICE, AC2DC_I2C_READ_POUT_WORD)*1000/512;
	i2c_write(PRIMARY_I2C_SWITCH, 0x00);
    return power;
}


int ac2dc_get_eeprom(int offset) {
	// Stub for remo
	i2c_write(PRIMARY_I2C_SWITCH, 0x01);
	int b = i2c_read_byte(AC2DC_MGMT_DEVICE, offset);
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

