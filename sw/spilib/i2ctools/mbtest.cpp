///
///simple util that reads AC2DC VPD info
///and prints it out.
///


#include <stdio.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#include "hammer.h"
#include <spond_debug.h>
#include "squid.h"
#include "i2c.h"
#include "dc2dc.h"
#include "mbtest.h"
#include "mainvpd.h"
#include "hammer_lib.h"
using namespace std;
extern pthread_mutex_t i2c_mutex;

int FIRSTLOOP = TOP_FIRST_LOOP ;
int LASTLOOP = BOTTOM_LAST_LOOP;

int usage(char * app ,int exCode ,const char * errMsg = NULL)
{
    if (NULL != errMsg )
    {
        fprintf (stderr,"==================\n%s\n\n==================\n",errMsg);
    }
    
    printf ("Usage: %s [-top|-bottom|-both]\n\n" , app);

    printf ("       -top : top main board\n");
    printf ("       -bottom : bottom main board\n");
    printf ("       -both : both top & bottom main boards (default)\n");

    if (0 == exCode) // exCode ==0 means - just print usage and get back to app for business. other value - exit with it.
    {
        return 0;
    }
    else 
    {
        exit(exCode);
    }
}


int foo(int topOrBottom ) {
	 int err;
	 i2c_init();
	 uint8_t chr;

	 int I2C_MY_MAIN_BOARD_PIN = (topOrBottom == TOP_BOARD)?PRIMARY_I2C_SWITCH_TOP_MAIN_PIN : PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN;
	 i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN, &err);
	 i2c_write(I2C_DC2DC_SWITCH_GROUP1 , MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);

	 //const char buff[45]{};

	 //const unsigned char * buff = "1234567890123456789012345678901234";

	 //my_i2c_set_address(MAIN_BOARD_I2C_EEPROM_DEV_ADDR,&err);


	 //__i2c_write_block_data(MAIN_BOARD_VPD_ADDR_START,43,buff );

	 for (int i = 0; i < MAIN_BOARD_VPD_ADDR_END ; i++) {

//		 i2c_waddr_write_byte( MAIN_BOARD_I2C_EEPROM_DEV_ADDR ,(uint16_t)(0x00FF & i) , 65 );

		 i2c_write_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR,i,0x65,&err );
		 usleep(200);
//		 i2c_write_word(MAIN_BOARD_I2C_EEPROM_DEV_ADDR,i,0x0000,&err );
		 usleep(3000);
	 }

  i2c_write(PRIMARY_I2C_SWITCH, 0x00);
   pthread_mutex_unlock(&i2c_mutex);

	return 0;
}

int bar(int topOrBottom ) {
	 int err;
	 i2c_init();
	 uint8_t chr;
	 uint16_t wchr;
	 int I2C_MY_MAIN_BOARD_PIN = (topOrBottom == TOP_BOARD)?PRIMARY_I2C_SWITCH_TOP_MAIN_PIN : PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN;
	 i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN, &err);
	 i2c_write(I2C_DC2DC_SWITCH_GROUP1 , MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);

	 for (int i = 0; i < MAIN_BOARD_VPD_ADDR_END ; i++) {
		 //chr = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & i), &err);
		 chr = i2c_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR ,  i, &err);
		 printf(" 0x%X - 0x%X (%c)\n" , i , chr , chr);
		 usleep(3000);

//		 wchr = i2c_read_word(MAIN_BOARD_I2C_EEPROM_DEV_ADDR ,  i, &err);
//		 printf(" 0x%X - 0x%X (%c)\n" , i , chr , chr);
//		 usleep(3000);

	 }

   i2c_write(PRIMARY_I2C_SWITCH, 0x00);
    pthread_mutex_unlock(&i2c_mutex);

	return 0;
}

int mainboard_get_vpd(int topOrBottom ,  mainboard_vpd_info_t *pVpd) {
	int rc = 0;
	int err = 0;

	if (NULL == pVpd) {
		psyslog("call ac2dc_get_vpd performed without allocating sturcture first\n");
		return 1;
	}

  	 pthread_mutex_lock(&i2c_mutex);

	 i2c_init();

	 int I2C_MY_MAIN_BOARD_PIN = (topOrBottom == TOP_BOARD)?PRIMARY_I2C_SWITCH_TOP_MAIN_PIN : PRIMARY_I2C_SWITCH_BOTTOM_MAIN_PIN;

	 i2c_write(PRIMARY_I2C_SWITCH, I2C_MY_MAIN_BOARD_PIN, &err);

	 if (err) {
		 fprintf(stderr, "Failed writing to I2C address 0x%X (err %d)",PRIMARY_I2C_SWITCH, err);

		 pthread_mutex_unlock(&i2c_mutex);
		 return err;
	 }


	 i2c_write(I2C_DC2DC_SWITCH_GROUP1 , MAIN_BOARD_I2C_SWITCH_EEPROM_PIN , &err);

	 if (err) {
		 fprintf(stderr, "Failed writing to I2C address 0x%X (err %d)",I2C_DC2DC_SWITCH_GROUP1, err);
		 pthread_mutex_unlock(&i2c_mutex);
		 return err;
	 }

	 for (int i = 0; i < MAIN_BOARD_VPD_PNR_ADDR_LENGTH ; i++) {
		 pVpd->pnr[i] = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & (MAIN_BOARD_VPD_PNR_ADDR_START + i)), &err);
		 if (err)
			 goto get_eeprom_err;
	 }

	  for (int i = 0; i < MAIN_BOARD_VPD_PNRREV_ADDR_LENGTH; i++) {
		pVpd->revision[i] = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & (MAIN_BOARD_VPD_PNRREV_ADDR_START + i)), &err);
		if (err)
		  goto get_eeprom_err;
	  }

	  for (int i = 0; i < MAIN_BOARD_VPD_SERIAL_ADDR_LENGTH; i++) {
		pVpd->serial[i] = i2c_waddr_read_byte(MAIN_BOARD_I2C_EEPROM_DEV_ADDR , (uint16_t)(0x00FF & (MAIN_BOARD_VPD_SERIAL_ADDR_START + i)), &err);
		if (err)
		  goto get_eeprom_err;
	  }


get_eeprom_err:

  if (err) {
    fprintf(stderr, RED            "Failed reading AC2DC eeprom (err %d)\n" RESET, err);
    rc =  err;
  }
    i2c_write(PRIMARY_I2C_SWITCH, 0x00);
     pthread_mutex_unlock(&i2c_mutex);

	return rc;
}

int EnableLoops() {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		dc2dc_enable_dc2dc(i, &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d ENABLE FAIL\n", i);
		} else {
//			fprintf(stdout, "LOOP %2d ENABLE PASS\n", i);
		}
	}
	return rc;
}

int GetLoopCurrent() {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		int loopcur = -1;
		loopcur = dc2dc_get_current_16s_of_amper(i, &err , &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d CURRENT MEASURE FAIL\n", i);
		} else {
			//fprintf(stdout, "LOOP %2d CURRENT MEASURE PASS -%5d\n", i, loopcur);
		}
	}
	return rc;
}

int GetLoopVoltage(int baseline = 0) {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		int loopvoltage = -1;
		loopvoltage = dc2dc_get_voltage(i, &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d VOLTAGE MEASURE FAIL\n", i);
		} else {
			if (baseline != 0){
				float precision = ((float)loopvoltage/(float)baseline);
				if (precision < 1.02 && precision > 0.98 ){
					//fprintf(stdout, "LOOP %2d SET VOLTAGE PASS [Target:%d , Measure:%d (Ratio=%4.3f)]\n", i, loopvoltage , baseline , precision);
				}
				else
				{
					rc = rc | (1 << i) ; // setting bitmap for error!
					fprintf(stdout, "LOOP %2d SET VOLTAGE FAIL [Target:%d , Measure:%d (Ratio=%4.3f)]\n", i, loopvoltage , baseline , precision);
				}
			}
			else{
				//fprintf(stdout, "LOOP %2d VOLTAGE MEASURE PASS -%5d\n", i, loopvoltage);
			}

		}
	}
	return rc;
}

int GetLoopsTemp(){
	int rc = 0;
	int err = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		err = 0;
		int looptemp = -1;

		looptemp = dc2dc_get_temp(i , &err);

		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d GET TEMP FAIL\n", i);
		} else {
			//fprintf(stdout, "LOOP %2d GET TEMP PASS (%d)\n", i , looptemp);
		}
	}
	return rc;
}

int SetLoopsVtrim(int vt) {
	int rc = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		int err = 0;
		int loopvoltage = -1;
		dc2dc_set_vtrim(i, vt, &err);
		if (0 != err) {
			rc = rc | (1 << i) ; // setting bitmap for error!
			fprintf(stdout, "LOOP %2d SET VOLTAGE FAIL\n", i);
		} else {
		//	fprintf(stdout, "LOOP %2d SET VOLTAGE PASS\n", i);
		}
	}
	return rc;
}

int formatBadLoopsString ( int badloops ,  char * formatted){
	int index = 0;
	for (int i = FIRSTLOOP ; i <= LASTLOOP; i++) {
		if (1 == ((badloops >> i ) & 0x00000001)){ // loop i is bad
			sprintf(formatted+index  , "%2d, ",i);
			index += 4;
		}
	}
	// terminate string (allow reuse)
	formatted[index] = '\0';

	// better terminate without the ", " in the end.
	if (index > 1){
		formatted[index-2] = '\0';
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int err;


	bool callUsage = false;
	bool badParm = false;

	int topOrBottom = -1; // default willbe TOP, start with -1, to rule out ambiguity.

	char badloopsformattedstring[100];

	for (int i = 1 ; i < argc ; i++){
	if ( 0 == strcmp(argv[i],"-h"))
	  callUsage = true;

	else if ( 0 == strcmp(argv[i],"-top")){
	  if (topOrBottom == -1 || topOrBottom == TOP_BOARD)
		  topOrBottom = TOP_BOARD;
	  else
		  badParm = true;
	}
	else if ( 0 == strcmp(argv[i],"-bottom")){
	  if (topOrBottom == -1 || topOrBottom == BOTTOM_BOARD)
		  topOrBottom = BOTTOM_BOARD;
	  else
		  badParm = true;
	}
	else
	  badParm = true;
	}

	if(badParm)
	{
		usage(argv[0],1,"Bad arguments");
	}

	// applying default as top.
	if (-1 == topOrBottom)
	  topOrBottom = BOTH_MAIN_BOARDS;

	switch (topOrBottom){
		case TOP_BOARD:
			FIRSTLOOP = TOP_FIRST_LOOP;
			LASTLOOP = TOP_LAST_LOOP;
			break;
		case BOTTOM_BOARD:
			FIRSTLOOP = BOTTOM_FIRST_LOOP;
			LASTLOOP = BOTTOM_LAST_LOOP;
			break;
		case BOTH_MAIN_BOARDS:
			FIRSTLOOP = TOP_FIRST_LOOP;
			LASTLOOP = BOTTOM_LAST_LOOP;
			break;
	}



	if (callUsage)
	return usage(argv[0] , 0);


	i2c_init(&err);

	dc2dc_init();

	int failedloops;
	printf ("+============  ENABLE  LOOPS  =================\n");
	failedloops =  EnableLoops();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL ENABLE LOOPS - %d loops failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS ENABLE LOOPS\n");
	printf ("-----------------------------------------------\n\n");

	printf ("+============  MEASURE CURRENT =================\n");
	failedloops = GetLoopCurrent();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL MEASURE CURRENT - %d loops failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS MEASURE CURRENT\n");
	printf ("-----------------------------------------------\n\n");

	printf ("+============  MEASURE VOLTAGE =================\n");
	failedloops = GetLoopVoltage();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL MEASURE VOLTAGE - %d loops failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS MEASURE VOLTAGE\n");
	printf ("-----------------------------------------------\n\n");


	printf ("+============  SET VOLTAGE %d ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN));
	failedloops = SetLoopsVtrim(VTRIM_MIN);
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d loops failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN), count_bits(failedloops) , badloopsformattedstring);
	}
	usleep(200000);
	failedloops = GetLoopVoltage(VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN));
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d loops failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN), count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS SET VOLTAGE %d\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MIN));
	printf ("-----------------------------------------------\n\n");


	printf ("+============  SET VOLTAGE %d ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH));
	failedloops = SetLoopsVtrim(VTRIM_HIGH);
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d loops failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH), count_bits(failedloops) , badloopsformattedstring);
	}
	usleep(200000);
	failedloops = GetLoopVoltage(VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH));
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d loops failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH), count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS SET VOLTAGE %d\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_HIGH));
	printf ("-----------------------------------------------\n\n");

	printf ("+============  SET VOLTAGE %d ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM));
	failedloops = SetLoopsVtrim(VTRIM_MEDIUM);
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d loops failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM), count_bits(failedloops) , badloopsformattedstring);
	}
	usleep(200000);
	failedloops = GetLoopVoltage(VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM + 20));
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL SET VOLTAGE %d - %d loops failed (%s)\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM), count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS SET VOLTAGE %d\n" ,VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM));
	printf ("-----------------------------------------------\n\n");

	printf ("+============  GET DC2DC TEMP ===============\n", VTRIM_TO_VOLTAGE_MILLI(VTRIM_MEDIUM));
	failedloops = GetLoopsTemp();
	if (failedloops > 0){
		formatBadLoopsString(failedloops , badloopsformattedstring);
		printf ("TEST-FAIL GET DC2DC TEMP - %d loops failed (%s)\n" , count_bits(failedloops) , badloopsformattedstring);
	}
	else
		printf ("TEST-PASS GET DC2DC TEMP\n");
	printf ("-----------------------------------------------\n\n");


//	if (do_bist_ok_rt(0)){
//		printf ("TEST-PASS ASICs SELF TEST\n");
//	}else{
//		printf ("TEST-FAIL ASICs SELF TEST\n");
//	}


	return rc;
}
