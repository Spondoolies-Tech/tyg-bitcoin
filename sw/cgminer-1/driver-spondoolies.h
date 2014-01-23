
#ifndef SPONDA_HFILE
#define SPONDA_HFILE


#include "miner.h"
#include "miner_gate_lib.h"


#define MAX_SPOND_JOBS 10

typedef enum adapter_state {
	ADAPTER_STATE_INIT,
	ADAPTER_STATE_OPERATIONAL,
} ADAPTER_STATE; 



typedef enum spond_work_state {
	SPONDWORK_STATE_EMPTY,
    SPONDWORK_STATE_IN_MINERGATE,
//    SPONDWORK_STATE_COMPLETE,
} SPONDWORK_STATE; 



typedef struct {
    struct work      *cgminer_work;
    SPONDWORK_STATE  state;
    uint32_t         merkel_root;
    time_t           start_time;
    int              job_id;
} spond_driver_work;

struct spond_adapter {
	pthread_mutex_t lock;
	// Lock the job queue
	pthread_mutex_t qlock;
	pthread_cond_t qcond;
	ADAPTER_STATE adapter_state;
	void* cgpu;



    int works_pending_tx;
    int works_in_driver;
    int works_in_minergate;
    int current_job_id; // 0 to 0xFF
    int socket_fd;
    minergate_packet* mp_req;// = allocate_minergate_packet(10000, 0xca, 0xfe);
    minergate_data* m_data_req;
    int parse_resp;
    minergate_packet* mp_rsp;// = allocate_minergate_packet(10000, 0xca, 0xfe);
    spond_driver_work minergate_work[0x100]; 

};


// returns non-zero if needs to change ASICs.
int spond_one_sec_timer_scaling(struct spond_adapter *a, int t);
int spond_do_scaling(struct spond_adapter *a);

extern void one_sec_spondoolies_watchdog(int uptime);

#define REQUEST_PERIOD 50000  // 20 times per second
#define REQUEST_SIZE   25      // 45 jobs per request



#endif

