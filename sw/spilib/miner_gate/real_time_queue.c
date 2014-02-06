#include "squid.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include <unistd.h>

#include "hammer.h"
#include "squid.h"
#include "queue.h"
#include <string.h>	
#include <spond_debug.h>	
#include <sys/time.h>



#define MAX_PACKETS_IN_RT_QUEUE 15
// hash table
RT_JOB rt_queue[0x100] = {0};
int rt_queue_sw_write;
int rt_queue_hw_done;
int rt_queue_size = MAX_PACKETS_IN_RT_QUEUE + 5; // to check that init was called



int one_done_sw_rt_queue(RT_JOB *work);
void push_work_rsp(RT_JOB* work);


RT_JOB* peak_rt_queue(uint8_t hw_id) {
    return &rt_queue[hw_id];   
}


void reset_sw_rt_queue() {
	memset(&rt_queue,0,sizeof(rt_queue));
    // start with 3, why not? :)
	rt_queue_sw_write = rt_queue_hw_done = 3;
	rt_queue_size = 0;
}

// Returns pointer for RT_JOB to give HW.
RT_JOB* add_to_sw_rt_queue(const RT_JOB *work) {
	if(rt_queue_size == MAX_PACKETS_IN_RT_QUEUE) {
		RT_JOB old_w;
		one_done_sw_rt_queue(&old_w);
	    push_work_rsp(&old_w);
	}
	passert(rt_queue[rt_queue_sw_write].work_state == WORK_STATE_FREE);
	RT_JOB *work_in_queue = &rt_queue[rt_queue_sw_write];
	*work_in_queue = *work;
	work_in_queue->work_id_in_hw = rt_queue_sw_write;
	work_in_queue->work_state = WORK_STATE_HAS_JOB;
	// to make sure...
	passert(work_in_queue->winner_nonce == 0);
	work_in_queue->work_id_in_hw = rt_queue_sw_write;
	rt_queue_sw_write = (rt_queue_sw_write+1)%0x100;
	rt_queue_size++;
    passert((rt_queue_sw_write - rt_queue_hw_done)%0x100 <= MAX_PACKETS_IN_RT_QUEUE);
    passert(rt_queue_size <= MAX_PACKETS_IN_RT_QUEUE);
        
	return work_in_queue;
}



int one_done_sw_rt_queue(RT_JOB *work) {
	if (rt_queue_size) {
		*work = rt_queue[rt_queue_hw_done];
		passert(rt_queue[rt_queue_hw_done].work_state == WORK_STATE_HAS_JOB);
        rt_queue[rt_queue_hw_done].work_state = WORK_STATE_FREE;	
		rt_queue_hw_done = (rt_queue_hw_done+1)%0x100;
		rt_queue_size--;
        passert((rt_queue_sw_write - rt_queue_hw_done)%0x100 <= MAX_PACKETS_IN_RT_QUEUE);
        passert(rt_queue_size <= MAX_PACKETS_IN_RT_QUEUE);
		return 1;
	}
	return 0;
}


