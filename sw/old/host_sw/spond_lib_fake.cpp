

#include "spond_lib.h"
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <utility>
#include <iomanip> 
#include <pthread.h>


using namespace std;

FINISHED_CB finished_cb;
static ifstream tty_file;
static ofstream tty_file_o;
static ofstream log_o;



unsigned char current_target[SIZE_OF_TARGET_BYTES];
pthread_t poll_thread;

pthread_cond_t     cmd_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t    cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t    pm = PTHREAD_MUTEX_INITIALIZER;

int last_req_id = 0;
char last_resp[200];
int MAX_JOBS_IN_HW = 50;

map<int, SPOND_WORK*> works_map;


void spond_get_board_state(int *free_slots, int *engine_running);


void* poll_board_thread(void* p) {
	while (1) {
		usleep(10);
		pthread_mutex_lock(&pm);
		if (works_map.size()) {
		    pthread_mutex_lock(&cmd_mutex);
			map<int,SPOND_WORK*>::iterator it = works_map.begin();
			SPOND_WORK* w = it->second;
			assert(w);
			w->success = 0;
			w->winner_nonce = 0xffffffff;
			w->work_state = SPOND_WORK_STATE_FINISHED;
			//printf("Fake-end %x\n", w->work_id);
			finished_cb(w);
			works_map.erase(it);
			pthread_mutex_unlock(&cmd_mutex);
		} 
		pthread_mutex_unlock(&pm);
	}
}

void spond_dropwork() {
	pthread_mutex_lock(&pm);	
 	pthread_mutex_lock(&cmd_mutex);
	map<int,SPOND_WORK*>::iterator it;
	while (works_map.size()) {
		it = works_map.begin();
		SPOND_WORK* w = it->second;
		w->success = 0;
		w->winner_nonce = 0xffffffff;
		w->work_state = SPOND_WORK_STATE_FINISHED;
		printf("FORCE Fake-end %x\n", w->work_id);
		finished_cb(w);
		works_map.erase(it);
	}
	pthread_mutex_unlock(&cmd_mutex);	
	pthread_mutex_unlock(&pm);	
}



int spond_connect(const char* tty_dev, FINISHED_CB cb, WON_CB wcb) {
    finished_cb = cb;
	pthread_mutex_init(&cmd_mutex, 0);
	pthread_mutex_init(&pm, 0);
    pthread_create(&poll_thread, NULL, poll_board_thread, NULL);
    int engine_running;
    spond_get_board_state(&MAX_JOBS_IN_HW, &engine_running);
    printf("HW QUEUE LEN = %d\n", MAX_JOBS_IN_HW);	
    return 1;
}


int reverse32(int i) {
	return __bswap_32(i);
}



int spond_give_work(SPOND_WORK *work) {
	static int j = 0;
	pthread_mutex_lock(&pm);	
	printf("!!!fake WORK %d!!!\n", j++);		
	int free_size = MAX_JOBS_IN_HW - works_map.size();
	if (works_map.size() >= MAX_JOBS_IN_HW) {
		printf("!overflow\n");		
		return -1;
	}
	pthread_mutex_lock(&cmd_mutex);
	work->success = 0;
	works_map[rand()] = work;
	//printf("Fake-start %x\n", work->work_id);
	pthread_mutex_unlock(&cmd_mutex);	
	pthread_mutex_unlock(&pm);	
	return free_size;	
}



void spond_get_board_state(int *free_slots, int *engine_running) {
	*free_slots        =  10;
	*engine_running    =  0;
}



void spond_set_target(const unsigned char *target) {
	memcpy(current_target, target,SIZE_OF_TARGET_BYTES); 
}



void spond_log(const char* p) {
	printf(p);
}


