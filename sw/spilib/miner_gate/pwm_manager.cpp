
#include "pwm_manager.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <spond_debug.h>

/*


*/

int pwm_values[] = {12000, 22000, 32000};
void init_pwm() {
	//cd /sys/devices/bone_capemgr.*

	FILE *f; 
	 printf("--------------- %d\n", __LINE__);
    f = fopen("/sys/devices/bone_capemgr.9/slots", "w");
	//usleep(2000);
	fprintf(f, "am33xx_pwm");
	//usleep(2000);
	fclose(f);
	 printf("--------------- %d\n", __LINE__);
	//usleep(2000);
	//sleep(1);
	f = fopen("/sys/devices/bone_capemgr.9/slots", "w");
	fprintf(f, "bone_pwm_P9_31");
	//usleep(2000);
	fclose(f);
	//usleep(2000);
	printf("--------------- %d\n", __LINE__);

	int val = pwm_values[FAN_LEVEL_HIGH];
	f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.11/period", "w");
	if (!f) {
		f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.12/period", "w");
	}
	if (!f) {
		f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.13/period", "w");
	}
	if (!f) {
		f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.14/period", "w");
	}
	passert(f != NULL);
	
	fprintf(f, "40000");
	fclose(f);

	 printf("--------------- %d\n", __LINE__);
	//pwm_values

	f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.11/duty", "w");
	if (!f) {
		f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.12/duty", "w");
	}
	if (!f) {
		f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.13/duty", "w");
	}
	if (!f) {
		f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.14/duty", "w");
	}
	fprintf(f, "%d", val);
	fclose(f);
	 printf("--------------- %d\n", __LINE__);
	//echo am33xx_pwm > "/sys/devices/bone_capemgr.9/slots"
	//echo bone_pwm_P9_31 > "/sys/devices/bone_capemgr.9/slots"
	
}


void set_fan_level(FAN_LEVEL fan_level) {
	//echo 40000 > "/sys/devices/ocp.3/pwm_test_P9_31.13/period"
	//echo val > "/sys/devices/ocp.3/pwm_test_P9_31.13/duty"
	FILE *f; 
	passert(fan_level < FAN_LEVEL_COUNT);
	
	int val = pwm_values[fan_level];

	f = fopen("/sys/devices/ocp.3/pwm_test_P9_31.13/duty", "w");
	fprintf(f, "%d", val);
	fclose(f);
}

/*
cd /sys/devices/bone_capemgr.*
echo am33xx_pwm > slots
echo bone_pwm_P9_31 > slots

cd /sys/devices/ocp.*
cd pwm_test_P9_31.*

our period should be 40000:
echo 40000 > period

for 100%:
echo 40000 > duty

for 80%:
echo 32000 > duty

for 30% (minimum):
echo 12000 > duty
*/