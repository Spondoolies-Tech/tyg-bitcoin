
#include "ac2dc_const.h"
#include "ac2dc.h"
#include "dc2dc.h"
#include "i2c.h"


int ac2dc_getint(int source) {
	int n = (source & 0xF800) >> 11;
	// This is shitty 2s compliment on 5 bits.
	if (n&0x10) {
		n = (n^0x1F)+1;
	} 
	int y = (source & 0x07FF);
	return (y*1000)/(1<<n);

}

// Return Watts
int ac2dc_get_power() {
	int err = 0;
	static int warned = 0;
	i2c_write(PRIMARY_I2C_SWITCH, 0x01); 
	int power = ac2dc_getint(i2c_read_word(AC2DC_I2C_MGMT_DEVICE, AC2DC_I2C_READ_POUT_WORD, &err));
	i2c_write(PRIMARY_I2C_SWITCH, 0x00);
	if (err) {
		if ((warned++) < 10)
			psyslog("FAILED TO INIT DC2DC\n");
		return 0;
	} 
		
    return power/1000;
}



// Return Watts
int ac2dc_get_temperature(int sensor) {
	static int warned = 0;

	int err = 0;
	i2c_write(PRIMARY_I2C_SWITCH, 0x01); 
	int temp = ac2dc_getint(i2c_read_word(AC2DC_I2C_MGMT_DEVICE, AC2DC_I2C_READ_TEMP1_WORD + sensor, &err));
	i2c_write(PRIMARY_I2C_SWITCH, 0x00);
	if (err) {
		if ((warned++) < 10)
			psyslog("FAILED TO INIT DC2DC\n");
		return 0;
	} 
		
    return temp/1000;
}

int ac2dc_get_vpd( ac2dc_vpd_info_t * pVpd){
	int err = 0;
	int pnr_offset = 0x34;
	int pnr_size = 15;
	int model_offset = 0x57;
	int model_size = 4;
	int serial_offset = 0x5b;
	int serial_size = 10;
	int revision_offset = 0x61;
	int revision_size = 2;

	if (NULL == pVpd )
	{
		printf("call ac2dc_get_vpd performed without allocating sturcture first\n"); 
		return 1;
	}


	i2c_write(PRIMARY_I2C_SWITCH, 0x01,&err); 

	if (err){
		fprintf(stderr , "Failed writing to I2C address 0x%X (err %d)",PRIMARY_I2C_SWITCH,err);
		return err;
	}

	for ( int i = 0 ; i < pnr_size ; i++){
		pVpd->pnr[i] = ac2dc_get_eeprom_quick(pnr_offset + i,&err);
		if (err)
			goto ac2dc_get_eeprom_quick_err;
	}

	for ( int i = 0 ; i < model_size ; i++){
		pVpd->model[i] = ac2dc_get_eeprom_quick(model_offset + i,&err);
		if (err)
			goto ac2dc_get_eeprom_quick_err;
	}

	for ( int i = 0 ; i < revision_size ; i++){
		pVpd->revision[i] = ac2dc_get_eeprom_quick(revision_offset + i,&err);
		if (err)
			goto ac2dc_get_eeprom_quick_err;
	}

	for ( int i = 0 ; i < serial_size ; i++){
		pVpd->serial[i] = ac2dc_get_eeprom_quick(serial_offset + i,&err);
		if (err)
			goto ac2dc_get_eeprom_quick_err;
	}

	i2c_write(PRIMARY_I2C_SWITCH, 0x00);


ac2dc_get_eeprom_quick_err:
	
	if (err){
		fprintf(stderr , "Failed reading AC2DC eeprom (err %d)\n",err);
		return err;
	}

return 0;

}

// this function assumes as a precondition 
// that the i2c bridge is already pointing to the correct device
// needed to read ac2dc eeprom
// no side effect either
// use this funtion when performing serial multiple reads
unsigned char ac2dc_get_eeprom_quick(int offset , int * pError) {
	unsigned char b = (unsigned char)i2c_read_byte(AC2DC_I2C_EEPROM_DEVICE, offset,pError);
	return b;
}


// no precondition as per i2c
// and thus sets switch first,
// and then sets it back
// side effect - it closes the i2c bridge when finishes.
int ac2dc_get_eeprom(int offset,int * pError) {
	// Stub for remo
	int b ;
	i2c_write(PRIMARY_I2C_SWITCH, 0x01 , pError);
	if (pError && *pError)
		return *pError;
		
	b = i2c_read_byte(AC2DC_I2C_EEPROM_DEVICE, offset, pError);
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
    passert(v<ASIC_VOLTAGE_COUNT && v>=0);
    i2c_write_word(0x1b, 0xd4, volt_to_vtrim[v]);
    i2c_write_byte(0x1b, 0x01, volt_to_vmargin[v]);
}




int dc2dc_get_voltage(int loop) {
    int voltage = ASIC_VOLTAGE_810;
    voltage = i2c_read_word(0x1b, 0x8b)*1000/512;


    return voltage;
}
*/

