#include "dc2dc.h"
#include "i2c.h"

int volt_to_vtrim[ASIC_VOLTAGE_COUNT] = {
    0xFFC4 ,  0xFFCF, 0xFFE1, 0xFFd4,  0xFFE5,0xfff7, 0x0, 0x8,
};

int volt_to_vmargin[ASIC_VOLTAGE_COUNT] = {
    0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0
};

 


void dc2dc_init() {
	int err = 0;
	static int warned = 0;
    // Write defaults
    i2c_write_byte(0x1b, 0x00, 0x81, &err);
	if (err) {
		if ((warned++) < 10)
			psyslog("FAILED TO INIT DC2DC\n");
	} else {
	    i2c_write_word(0x1b, 0x35, 0xf028);
	    i2c_write_word(0x1b, 0x36, 0xf018);
	    i2c_write_word(0x1b, 0x38, 0x881f);
	    i2c_write_word(0x1b, 0x46, 0xf846);
	    i2c_write_word(0x1b, 0x4a, 0xf836);
	    i2c_write_byte(0x1b, 0x47, 0x3C);
	    i2c_write_byte(0x1b, 0xd7, 0x03);
	    i2c_write_byte(0x1b, 0x02, 0x02);
	    i2c_write(0x1b, 0x15);
	    i2c_write(0x1b, 0x03);
	}
}



void dc2dc_select_channel(int channel_mask, int* err) { //0x00=first, 0x01=second, 0x81=both
	i2c_write_byte(0x1b, 0x00, channel_mask, err); 
}




void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v) {
    //printf("%d\n",v);
    passert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
    i2c_write_word(0x1b, 0xd4, volt_to_vtrim[v]);
    i2c_write_byte(0x1b, 0x01, volt_to_vmargin[v]);
}

// returns AMPERS
int dc2dc_get_current(int loop) {
	// TODO - select loop!
	int err = 0;
	static int warned = 0;
    int current = 0;
	dc2dc_select_channel(0, &err);
    current += i2c_read_word(0x1b, 0x8c)/16;
	dc2dc_select_channel(1, &err);
	current += i2c_read_word(0x1b, 0x8c)/16;
	dc2dc_select_channel(0x81, &err);
	if (err) {
		if ((warned++) < 10)
			psyslog("FAILED TO GET DC2DC CURRENT\n");
		return 0;
	}
    return current;
}



int dc2dc_get_voltage(int loop) {
	int err = 0;
	static int warned = 0;
    int voltage = ASIC_VOLTAGE_810;
    voltage = i2c_read_word(0x1b, 0x8b, &err)*1000/512;
	if (err) {
		if ((warned++) < 10)
			psyslog("FAILED TO GET DC2DC VOLTAGE\n");
		return 0;
	}
    return voltage;
}


