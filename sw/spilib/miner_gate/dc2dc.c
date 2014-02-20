#include "dc2dc.h"
#include "i2c.h"
#include "nvm.h"

int volt_to_vtrim[ASIC_VOLTAGE_COUNT] = {
    0, 0xFFC4 ,  0xFFCF, 0xFFE1, 0xFFd4,  0xFFE5,0xfff7, 0x0, 0x8,
};

int volt_to_vmargin[ASIC_VOLTAGE_COUNT] = {
    0, 0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0
};

 


void dc2dc_init() {
	int err = 0;
	//static int warned = 0;
    // Write defaults
	for (int loop = 0; loop < LOOP_COUNT; loop++) {
		dc2dc_select_i2c(loop,  &err);
		if (err) {
			psyslog("FAILED TO INIT DC2DC 1\n");
			dc2dc_i2c_close();
			continue;
		} 

		
	    i2c_write_byte(I2C_DC2DC, 0x00, 0x81, &err);
		if (err) {
			psyslog("FAILED TO INIT DC2DC 2\n");
			dc2dc_i2c_close();
			continue;
		} 
	
	    i2c_write_word(I2C_DC2DC, 0x35, 0xf028);
	    i2c_write_word(I2C_DC2DC, 0x36, 0xf018);
	    i2c_write_word(I2C_DC2DC, 0x38, 0x881f);
	    i2c_write_word(I2C_DC2DC, 0x46, 0xf846);
	    i2c_write_word(I2C_DC2DC, 0x4a, 0xf836);
	    i2c_write_byte(I2C_DC2DC, 0x47, 0x3C);
	    i2c_write_byte(I2C_DC2DC, 0xd7, 0x03);
	    i2c_write_byte(I2C_DC2DC, 0x02, 0x02);
	    i2c_write(I2C_DC2DC, 0x15);
	    i2c_write(I2C_DC2DC, 0x03);
		psyslog("OK INIT DC2DC 2\n");
		dc2dc_i2c_close();
	}
}




void dc2dc_set_channel(int channel_mask, int* err) {
	i2c_write_byte(I2C_DC2DC, 0x00, channel_mask, err); 
}



void dc2dc_disable_dc2dc(int loop, int* err) {
/*	
	miner-# i2cset -y 0 0x1b 0x02 0x12
	miner-# i2cset -y 0 0x1b 0x02 0x02
*/
	dc2dc_select_i2c(loop, err);

	i2c_write_byte(I2C_DC2DC, 0x02, 0x12, err); 
	dc2dc_i2c_close();
	//i2c_write_byte(I2C_DC2DC, 0x00, channel_mask, err); 

}


void dc2dc_enable_dc2dc(int loop, int* err) {
	dc2dc_select_i2c(loop, err);
	i2c_write_byte(I2C_DC2DC, 0x02, 0x02, err);  
	dc2dc_i2c_close();
}

//miner-# i2cset -y 0 0x1b 0x02 0x12
//miner-# i2cset -y 0 0x1b 0x02 0x02


void dc2dc_i2c_close() {
	i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0); 
	i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0); 
	i2c_write(PRIMARY_I2C_SWITCH, 0);
}


void dc2dc_select_i2c_ex(int top,     // 1 or 0
						  int i2c_group,   // 0 or 1
						  int dc2dc_offset, // 0..7
						  int* err) { //0x00=first, 0x01=second, 0x81=both
	
	if (top) {
		i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TOP_MAIN_PIN); // TOP
	} else {
		i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN); // BOTTOM
	}
	
	if (i2c_group == 0) {
#ifdef ASIC_TESTBOARD		
		i2c_write(PRIMARY_TESTBOARD_SWITCH, 0x40); // TOP
#else  
		i2c_write(I2C_DC2DC_SWITCH_GROUP0, 1<<dc2dc_offset); // TOP
#endif
		i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0); // TO
	} else {
		i2c_write(I2C_DC2DC_SWITCH_GROUP1, 1<<dc2dc_offset); // TOP
		i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0); // TOP
	}
}


void dc2dc_select_i2c(int loop,  int* err) {     // 1 or 0
	int top = (loop < 12); 
	int i2c_group = ((loop%12) >= 8);
	int dc2dc_offset = loop%12;
	dc2dc_offset = dc2dc_offset % 8;
	dc2dc_select_i2c_ex( top,     // 1 or 0
						 i2c_group,   // 0 or 1
						 dc2dc_offset, // 0..7
						 err);

}



void dc2dc_set_voltage(int loop, DC2DC_VOLTAGE v, int* err) {
    //printf("%d\n",v);
   // int err = 0;
    dc2dc_select_i2c(loop, err);
    passert((v < ASIC_VOLTAGE_COUNT) && (v >= 0));
    i2c_write_word(I2C_DC2DC, 0xd4, volt_to_vtrim[v]);
    i2c_write_byte(I2C_DC2DC, 0x01, volt_to_vmargin[v]);
	dc2dc_i2c_close();
}

// returns AMPERS
int dc2dc_get_current_16s_of_amper(int loop, int* err) {
	// TODO - select loop!
	//int err = 0;
	passert(err != NULL);
	static int warned = 0;
    int current = 0;
	dc2dc_select_i2c(loop, err);
	dc2dc_set_channel(0, err);
    current += (i2c_read_word(I2C_DC2DC, 0x8c)&0x07FF);
	dc2dc_set_channel(1, err);
	current += (i2c_read_word(I2C_DC2DC, 0x8c)&0x07FF);
	dc2dc_set_channel(0x81, err);
	if (*err) {
		dc2dc_i2c_close();
		return 0;
	}
	dc2dc_i2c_close();
    return current;
}



int dc2dc_get_voltage(int loop, int *err) {
	//*err = 0;
	passert(err != NULL);
	static int warned = 0;
    int voltage = ASIC_VOLTAGE_810;
	dc2dc_select_i2c(loop, err);
    voltage = i2c_read_word(I2C_DC2DC, 0x8b, err)*1000/512;
	if (*err) {
		dc2dc_i2c_close();
		return 0;
	}
	dc2dc_i2c_close();
    return voltage;
}



int dc2dc_get_temp(int loop, int *err) {
	//returns max temperature of the 2 sensors
	passert(err != NULL);
    int temp1,temp2;
	dc2dc_select_i2c(loop, err);
	dc2dc_set_channel(0, err);
    temp1 = i2c_read_word(I2C_DC2DC, 0x8e, err)/* *1000/512 */;
	if (*err) {dc2dc_i2c_close();return 0;}
	dc2dc_set_channel(1, err);
    temp2 = i2c_read_word(I2C_DC2DC, 0x8e, err)/* *1000/512 */;
	if (*err) {dc2dc_i2c_close();return 0;}
	if (temp2>temp1) temp1=temp2;
	dc2dc_i2c_close();
	return temp1;
   
}


