
#include "dc2dc.h"
#include "i2c.h"

int volt_to_vtrim[ASIC_VOLTAGE_COUNT] = {
    0xFFC4 ,  0xFFCF, 0xFFE1, 0xFFd4,  0xFFE5,0xfff7, 0x0, 0x8,
};

int volt_to_vmargin[ASIC_VOLTAGE_COUNT] = {
    0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0
};




void dc2dc_init() {
    // Write defaults
    i2c_write_byte(0x1b, 0x00, 0x81); 
    i2c_write_word(0x1b, 0x35, 0xf028);
    i2c_write_word(0x1b, 0x36, 0xf018);
    i2c_write_word(0x1b, 0x38, 0x8816);
    i2c_write_word(0x1b, 0x46, 0xf846);
    i2c_write_word(0x1b, 0x4a, 0xf836);
    i2c_write_byte(0x1b, 0x47, 0x00);
    i2c_write_byte(0x1b, 0xd7, 0x03);
    i2c_write_byte(0x1b, 0x02, 0x02);
    i2c_write(0x1b, 0x15);
    i2c_write(0x1b, 0x03);
}


void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v) {
    printf("%d\n",v);
    assert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
    i2c_write_word(0x1b, 0xd4, volt_to_vtrim[v]);
    i2c_write_byte(0x1b, 0x01, volt_to_vmargin[v]);
}




int dc2dc_get_voltage(int loop) {
    int voltage = ASIC_VOLTAGE_810;
    voltage = i2c_read_word(0x1b, 0x8b)*1000/512;


    return voltage;
}

