

#include "spond_lib.h"



void finished_cb_test(SPOND_WORK* work) {
	//printf("Work[%x]  done=%x Nonce=%x\n",work->work_id,work->success, work->nonce);
}



void ping_test() {
	while (1) {
//		ping_board();
//		usleep(10000);
	}
}


void work_test() {
	int i = 0x1000;
	SPOND_WORK sw;
	while (1) {
		sw.work_id = i++;
		((int*)sw.midstate)[0]++;
		int can_take = spond_give_work(&sw);
		if (can_take >= 0)
			printf("can_take %d\n", can_take)	;
		usleep(10000);
	} 
}



int main() {
	int s = spond_connect("/dev/ttyUSB0", finished_cb_test, finished_cb_test);
	assert(s);
	//usleep(1000000);
	work_test();
	//ping_test();
    
    



	return 0;
}

