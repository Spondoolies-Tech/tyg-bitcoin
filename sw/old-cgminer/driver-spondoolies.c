
#include <float.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#include "config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "compat.h"
#include "miner.h"
#include "mg_proto_parser.h"
#include "driver-spondoolies.h"

extern int opt_g_threads;

#define SKIP_REAL_WORK 0

struct device_drv spondoolies_drv;
//int socket_fd;


#ifdef WORDS_BIGENDIAN
#  define swap32tobe(out, in, sz)  ((out == in) ? (void)0 : memmove(out, in, sz))
#  define LOCAL_swap32be(type, var, sz)  ;
#  define swap32tole(out, in, sz)  swap32yes(out, in, sz)
#  define LOCAL_swap32le(type, var, sz)  LOCAL_swap32(type, var, sz)
#else
#  define swap32tobe(out, in, sz)  swap32yes(out, in, sz)
#  define LOCAL_swap32be(type, var, sz)  LOCAL_swap32(type, var, sz)
#  define swap32tole(out, in, sz)  ((out == in) ? (void)0 : memmove(out, in, sz))
#  define LOCAL_swap32le(type, var, sz)  ;
#endif
static inline void swap32yes(void*out, const void*in, size_t sz) {
	size_t swapcounter = 0;
	for (swapcounter = 0; swapcounter < sz; ++swapcounter)
		(((uint32_t*)out)[swapcounter]) = swab32(((uint32_t*)in)[swapcounter]);
}

void send_minergate_pkt(const minergate_packet* mp_req, 
						minergate_packet* mp_rsp,
						int  socket_fd) {
	int 	nbytes;		
	write(socket_fd, (const void*)mp_req, mp_req->data_length + MINERGATE_PACKET_HEADER_SIZE);
	nbytes = read(socket_fd, (void*)mp_rsp, 40000);	
	assert(nbytes > 0);
	//printf("got %d(%d) bytes\n",mp_rsp->data_length, nbytes);
	assert(mp_rsp->magic == 0xcafe);
	assert((mp_rsp->data_length + MINERGATE_PACKET_HEADER_SIZE) == nbytes);
}


static bool spondoolies_prepare(struct thr_info *thr)
{
	//applog(LOG_DEBUG, "SPOND spondoolies_prepare");
	//printf("WALLA prepare !!!!\n");
	struct cgpu_info *spondoolies = thr->cgpu;
	assert(spondoolies);
	struct timeval now;

	cgtime(&now);
/* FIXME: Vladik */
#if NEED_FIX
	get_datestamp(spondoolies->init, &now);
#endif
	//applog(LOG_DEBUG, "SPOND spondoolies_prepare done");

	return true;
}

/*
void do_freq_change(struct cgpu_info* cgpu) {
	struct spond_adapter *a = cgpu->device_data;
	printf("do_freq_change...\n");
	a->adapter_state = ADAPTER_STATE_FREQ_CHANGE;
	spond_do_scaling(a);
	a->adapter_state = ADAPTER_STATE_OPERATIONAL;
	printf("do_freq_change done...\n");

}
*/

void spond_print_stats(struct spond_adapter *a) {
	int i;
	printf("---------- ADAPTER STATS ------------\n");
	printf("state %i\n",a->adapter_state);
//	printf("job write %i, read %i, count %i\n",a->job_write_ptr,a->job_read_ptr, a->job_count);
	for (i = 0; i < MAX_SPOND_JOBS; i++) {
  	//	printf("  job %i: %p\n", i, a->minergate_work[i]);
	}	
	printf("-------------------------------------\n");

}
 

/*
void one_sec_timer_operational(struct cgpu_info* cgpu, int t) {
	struct spond_adapter *a = cgpu->device_data;
		int needs_change = 0;
		
		needs_change = spond_one_sec_timer_scaling(a, t);

		if (needs_change) {
			// let all jobs finish...
			printf("Waiting for all jobs to finish...\n");
			a->adapter_state = ADAPTER_STATE_PRE_FREQ_CHANGE;
			spond_dropwork();
	
	}
}


// called once every second
void one_sec_timer(struct cgpu_info* cgpu, int t) {
	struct spond_adapter *a = cgpu->device_data;
	printf("One SEC timer: %d\n", a->adapter_state);
	if (a->adapter_state == ADAPTER_STATE_OPERATIONAL) {
		one_sec_timer_operational(cgpu, t);
	}
	spond_print_stats(a);

}
*/

int init_socket() {
	struct sockaddr_un address;
    int socket_fd;
	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	 if(socket_fd < 0)
	 {
	  printf("socket() failed\n");
	  perror("Err:");
	  return 0;
	 }
	 
	 /* start with a clean address structure */
	 memset(&address, 0, sizeof(struct sockaddr_un));
	 
	 address.sun_family = AF_UNIX;
	 sprintf(address.sun_path, MINERGATE_SOCKET_FILE);
	 
	 if(connect(socket_fd, 
				(struct sockaddr *) &address, 
				sizeof(struct sockaddr_un)) != 0)
	 {
	  printf("connect() failed\n");
	  perror("Err:");
	  return 0;
	 }

	return socket_fd;
}


static void spondoolies_detect()
{
	int success;
	struct spond_adapter *a;


	//printf("WALLA detect !!!!\n");
	struct device_drv *drv = &spondoolies_drv;

	
	//usb_detect(&icarus_drv, icarus_detect_one);
	applog(LOG_DEBUG, "SPOND spondoolies_detect");
	applog(LOG_DEBUG, "USB scan devices: checking for %s devices", drv->name);



	/* 
	 *  If opt_g_threads is not set, use default 1 thread on scrypt and
	 *  2 for regular mining */
	 /*
	if (opt_g_threads == -1) {
			opt_g_threads = 5;
	}
	*/
	
	//spond_log("*************** Hello!\n");
	
/* FIXME: Vladik */
#if NEED_FIX
	nDevs = 1;
#endif

	struct cgpu_info *cgpu = calloc(1, sizeof(*cgpu));
	assert(cgpu);
	cgpu->drv = drv;
	cgpu->deven = DEV_ENABLED;
	cgpu->threads = 1;
	cgpu->device_data = calloc(sizeof(struct spond_adapter), 1);
	memset(cgpu->device_data, 0, sizeof(struct spond_adapter));
	if (unlikely(!(cgpu->device_data)))
		quit(1, "Failed to calloc cgpu_info data");
	a = cgpu->device_data;
	a->cgpu = (void*)cgpu;
	a->adapter_state = ADAPTER_STATE_OPERATIONAL;
    a->mp_req = allocate_minergate_packet(
        REQUEST_SIZE*sizeof(minergate_do_job_req) + MINERGATE_DATA_HEADER_SIZE,
        0xca, 0xfe);
    a->m_data_req = get_minergate_data(
        a->mp_req,   
        REQUEST_SIZE*sizeof(minergate_do_job_req), 
        MINERGATE_DATA_ID_DO_JOB_REQ);
    a->mp_rsp = allocate_minergate_packet(
        (2*MINERGATE_TOTAL_QUEUE*sizeof(minergate_do_job_rsp)) + 
        MINERGATE_DATA_HEADER_SIZE, 
        0xca, 0xfe);
     

	a->socket_fd = init_socket();
	if (a->socket_fd < 1) {
        printf("Error connecting to minergate server!");
		assert(0);
	}

	assert(add_cgpu(cgpu));
	applog(LOG_DEBUG, "SPOND spondoolies_detect done");
}


static struct api_data *spondoolies_api_stats(struct cgpu_info *cgpu)
{
	struct api_data *root = NULL;
	//struct SPONDOOLIES_INFO *a = (struct SPONDOOLIES_INFO *)(cgpu->device_data);
	applog(LOG_DEBUG, "SPOND spondoolies_api_stats");

	applog(LOG_DEBUG, "SPOND spondoolies_api_stats done");
	return root;
}





unsigned char get_leading_zeroes(const unsigned char *target) {
	unsigned char leading = 0;
	int first_non_zero_chr;
/*
	printf("target -- %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", 
		target[0],target[1],target[2],target[3],
		target[4],target[5],target[6],target[7],
		target[8],target[9],target[10],target[11],
		target[12],target[13],target[14],target[15],
		target[16],target[17],target[18],target[19],
		target[20],target[21],target[22],target[23],
		target[24],target[25],target[26],target[27],
		target[28],target[29],target[30],target[31]);
*/
	
	for (first_non_zero_chr = 31; first_non_zero_chr >= 0;
			first_non_zero_chr--) {
		if (target[first_non_zero_chr] == 0) {
			leading += 8;
		} else { 
			break;
		}
	}

	// j = first non-zero
	uint8_t m = target[first_non_zero_chr];
	while ((m & 0x80) == 0) {
		leading++;
		m = m << 1;
	}
	return leading;
}





#if 0
static int64_t spondoolies_scanhash(
	struct cgpu_info* cgpu,
	struct work *work)
{
    /*
	//printf("WALLA spondoolies_scanhash %d %d !!!!\n", thr->device_thread, thr->id);
	applog(LOG_DEBUG, "SPOND spondoolies_scanhash %d", work->id);
	struct spond_adapter *a = cgpu->device_data;
	bool rc;
	
	//printf("THR 2:%p %p %p\n", work->thr, thr, thr->cgpu);
	// TEST VECTOR
	//uint32_t nonce;
	LOCAL_swap32le(unsigned char, work->midstate, 32/4)
	LOCAL_swap32le(unsigned char, work->data+64, 64/4)
#if (SKIP_REAL_WORK	== 0)

	minergate_do_job_req* spond_w = &a->current_spond_works[work->subid];
	//spond_w->app_data  = work;
	//spond_w->app_data2 = cgpu;
#endif
	static unsigned int last_print_time = 0;
	//printf("--------- WORK: %x %x %x %x\n", *((int*)work->data + 64), *((int*)work->data + 64 + 4), *((int*)work->data + 64 + 8),  *((int*)work->data + 64 + 12));
	//pthread_mutex_lock(&spond_mutex);

	//uint32_t *nonce = (uint32_t *)(work->data + 64 + 12);
	//static int work_id = 1;
#if (SKIP_REAL_WORK	== 0)
	// Initialize and pass data for the library
	swap32yes(spond_w->data_tail64, work->data + 64, 64/4);
    spond_w.difficulty = 0;
    spond_w.timestamp  = 0;      
    spond_w.mrkle_root = 0;
	memcpy(spond_w->midstate, work->midstate, 32);
	spond_w->leading_zeroes = get_leading_zeroes(work->target);
	printf("LEADING ZEROES:%d\n", spond_w->leading_zeroes);
	spond_w->work_id = work->subid;
	spond_w->work_state = SPOND_WORK_STATE_WAITING;
	spond_give_work(spond_w);
	
	//pthread_mutex_unlock(&spond_mutex);
	//if(spond_w->work_state == SPOND_WORK_STATE_FOUND_NONCE) {
	
	
	
#else 
	usleep((rand()%2)*1000+2000+(rand()%500)); // 2-4 milli
#endif


	//work->blk.nonce = max_nonce;
	//static int i = 0;
    applog(LOG_DEBUG, "SPOND spondoolies_scanhash %d:%d done", work->id, work->subid);

	// Gather all threads before BIST
	*/

    

	return 0;	

	
}
#endif
static int64_t spondoolies_scanhash(
	struct cgpu_info* cgpu,
	struct work *work)
{

    return 0;
}



static void spondoolies_shutdown(__maybe_unused struct thr_info *thr)
{
	// TODO: ?
	printf("SPOND:: SHUTDOWN !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d\n", __LINE__);
}

void fill_minergate_work(minergate_do_job_req* work, struct work *cg_work) {
	static int id = 1;
	id++;
	memset(work, 0, sizeof(minergate_do_job_req));
	//work->
	LOCAL_swap32le(unsigned char, cg_work->midstate, 32/4)
	LOCAL_swap32le(unsigned char, cg_work->data+64, 64/4)
	uint32_t x[64/4];
	swap32yes(x, cg_work->data + 64, 64/4);
    memcpy(work->midstate, cg_work->midstate, 32);
    // TODO - test
    work->mrkle_root = ntohl(x[0]);
	work->timestamp  = ntohl(x[1]);
	work->difficulty = ntohl(x[2]);
    work->leading_zeroes = get_leading_zeroes(cg_work->target);
    work->work_id_in_sw = cg_work->subid;

/*
    printf("work: MKL:%x TS:%X DIF:%x\n",
        work->mrkle_root,
        work->timestamp,
        work->difficulty);
*/
    /*
	work->midstate[0] = work->;
	work->midstate[1] = (rand()<<16) + rand();
	work->midstate[2] = (rand()<<16) + rand();
	work->midstate[3] = (rand()<<16) + rand();
	work->midstate[4] = (rand()<<16) + rand();
	work->midstate[5] = (rand()<<16) + rand();
	work->midstate[6] = (rand()<<16) + rand();
	work->midstate[7] = (rand()<<16) + rand();
	work->work_id_in_sw = id%0xFFFF;
	work->mrkle_root = (rand()<<16) + rand();
    work->leading_zeroes = leading_zeroes;
	work->timestamp  = (rand()<<16) + rand();
	work->difficulty = (rand()<<16) + rand();
	//work_in_queue->winner_nonce
	*/
}



int minergate_data_processor(minergate_data* md, void* c, void* c2) {
	//printf("Got %d %d %d\n",md->data_id,md->data_length,md->magic);
    struct spond_adapter* a = c;
    
    if (md->data_id == MINERGATE_DATA_ID_DO_JOB_RSP) {        
           //DBG(DBG_NET, "GOT minergate_do_job_req: %x/%x\n", sizeof(minergate_do_job_req), md->data_length);
           int array_size = md->data_length / sizeof(minergate_do_job_rsp);
           assert(md->data_length % sizeof(minergate_do_job_rsp) == 0);
           int i;
           for (i = 0; i < array_size; i++) { // walk the jobs
               //printf("j");
               minergate_do_job_rsp* work = ((minergate_do_job_rsp*)md->data) + i;
               int job_id =  work->work_id_in_sw;
               //printf("!!!! HERE %d!\n",job_id);
            
               struct work *cg_work = a->minergate_work[job_id].cgminer_work;
               //a->minergate_work[job_id].cgminer_work.nonce1
                //printf("!!!! HERE %d! %p %p\n",job_id, a->cgpu, a->minergate_work[job_id].cgminer_work);
                if ((a->minergate_work[job_id].cgminer_work)) {
                    a->works_in_minergate--;
                    a->works_in_driver--;
                    assert(a->minergate_work[job_id].state == SPONDWORK_STATE_IN_MINERGATE);
                    if (a->minergate_work[job_id].merkel_root == work->mrkle_root) {
                        
                        //printf("!!!! HERE GOOD!! %d %x!\n",job_id, work->winner_nonce);
                        
                        if (work->winner_nonce) {
                           int r = submit_nonce(cg_work->thr, cg_work, work->winner_nonce);
                           printf("!!!!!!!  WIN !!!!!!!!!!!!!!!!!!!!!\nSUBMISSION WORK RSP:%d\n", r);
                        }

                        //printf("!!!! <) %p %p \n",
                        //a->cgpu, a->minergate_work[job_id].cgminer_work,
                        //a->minergate_work[job_id].cgminer_work->subid);
                        work_completed(a->cgpu, a->minergate_work[job_id].cgminer_work);
                        
                        // a->minergate_work[job_id].cgminer_work = NULL;
                    } else {
                        printf("!!!! HERE BAD!! %d! (%x %x)\n",job_id,
                            a->minergate_work[job_id].merkel_root, 
                            work->mrkle_root);
                        //assert(0);
                    }
                    //
                    //printf("!!!! HERE %d!\n",job_id);

                    a->minergate_work[job_id].cgminer_work = NULL;
                    //a->minergate_work[a->current_job_id].job_id = 0;
                    //a->minergate_work[a->current_job_id].start_time = time();
                    a->minergate_work[job_id].state = SPONDWORK_STATE_EMPTY;
                    //printf("!!!! HERE FULL! id:%d res:%d!\n",job_id, work->res);     
                } else {
                    printf("!HERE EMPTY! id:%d res:%d!\n",job_id, work->res);
                    //assert(0);
                }
           }
       }
}





/* move jobs from  get_queued() to a->minergate_work() */
// returns true if queue full.
struct timeval last_force_queue = {0};   
static bool spondoolies_queue_full(struct cgpu_info *cgpu)
{
    // Only once every 1/10 second do work.
    struct spond_adapter* a = cgpu->device_data;
    bool queue_full = false; // queue not full


    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned int usec;
        
    usec=(tv.tv_sec-last_force_queue.tv_sec)*1000000;
    usec+=(tv.tv_usec-last_force_queue.tv_usec);

    if (usec < REQUEST_PERIOD) {
        usleep(1000);
        return true;
    }
    //int i=0;
    
    
    // 1/10 second passed - print!
    //printf("Send Request (%d):\n", usec);
    struct work *work;
    //for (i = 0; i < REQUEST_SIZE; i++) {
    a->current_job_id = (a->current_job_id+1)%0x100;
    work = get_queued(cgpu);
    a->works_in_driver++;
    if (!work) {
    	queue_full = true;
		return queue_full;
	}

    // Get pointer for the request
    minergate_do_job_req* minergate_work = 
        ((minergate_do_job_req*)(a->m_data_req->data)) + a->works_pending_tx;

    work->subid = a->current_job_id;
    if (a->minergate_work[a->current_job_id].cgminer_work) {
        assert(a->minergate_work[a->current_job_id].state == SPONDWORK_STATE_IN_MINERGATE);
        work_completed(cgpu, a->minergate_work[a->current_job_id].cgminer_work);
        printf("X");
        a->works_in_driver--;
        a->works_in_minergate--;
        a->minergate_work[a->current_job_id].cgminer_work = NULL;
        //a->minergate_work[a->current_job_id].job_id = 0;
        //a->minergate_work[a->current_job_id].start_time = time();
        a->minergate_work[a->current_job_id].state = SPONDWORK_STATE_EMPTY;
    }
    a->minergate_work[a->current_job_id].cgminer_work = work;
    a->minergate_work[a->current_job_id].state = SPONDWORK_STATE_IN_MINERGATE;
    //printf("Prepare %d %d\n", a->works_pending_tx, a->current_job_id);
    fill_minergate_work(minergate_work, work);
    a->minergate_work[a->current_job_id].merkel_root = minergate_work->mrkle_root;
/*        printf("WRK: MID:%x MID:%x  LEAD:%d %x\n", 
        work->midstate[0], 
        minergate_work->midstate[0],
        minergate_work->leading_zeroes,
        minergate_work->work_id_in_sw);
*/
    a->works_pending_tx++;


    if (a->works_pending_tx == REQUEST_SIZE) {
        // TODO - send packet
        //printf("%d %d %d\n",a->works_in_minergate,a->works_pending_tx,a->works_in_driver );
        assert(a->works_in_minergate + a->works_pending_tx == a->works_in_driver);
        //   printf("x M:%d A:%d T:%d\n",
        //     a->works_in_minergate, a->works_pending_tx, a->works_in_driver);
         
        // printf("a %d\n",a->mp_req->data_length/sizeof(minergate_do_job_req));
         send_minergate_pkt(a->mp_req,  a->mp_rsp, a->socket_fd);
         last_force_queue = tv;
         assert(!a->parse_resp);
         a->parse_resp = 1;
         a->works_in_minergate += a->works_pending_tx;
         a->works_pending_tx = 0;
        // printf("b GOT:%d\n",a->mp_rsp->data_length/sizeof(minergate_do_job_rsp));
         return true;
    }
    //printf("-");
    return false;
//}
    
    return true;
}



// Return completed work to submit_nonce() and work_completed() 
// struct timeval last_force_queue = {0};  

static int64_t spond_scanhash(struct thr_info *thr)
{
	struct cgpu_info *cgpu = thr->cgpu;
	struct spond_adapter *a = cgpu->device_data;
	//applog(LOG_DEBUG, "SPOND spond_scanhash %d", thr->id);
    if(a->parse_resp) {
        parse_minergate_packet(a->mp_rsp, minergate_data_processor, a, a);
        a->parse_resp = 0;
    }

        
    //printf("+");


//    minergate_packet* mp_req = allocate_minergate_packet(10000, 0xca, 0xfe);

	
	return 0;
}

// Drop all current work
void spond_dropwork(struct spond_adapter *a) {
    int job_id;
    printf("---------------------------- DROP WORK!!!!!-----------------\n");
    for (job_id=0; job_id<0x100;job_id++) {
        if ((a->minergate_work[job_id].cgminer_work) ||
           (a->minergate_work[job_id].state == SPONDWORK_STATE_IN_MINERGATE)) {

                
                //printf("!!!! HERE GOOD!! %d %x!\n",job_id, work->winner_nonce);
                a->works_in_minergate--;
                a->works_in_driver--;
                //printf("!!!! <) %p %p \n",
                //a->cgpu, a->minergate_work[job_id].cgminer_work,
                //a->minergate_work[job_id].cgminer_work->subid);
                work_completed(a->cgpu, a->minergate_work[job_id].cgminer_work);
       
                // a->minergate_work[job_id].cgminer_work = NULL;
           
            a->minergate_work[job_id].cgminer_work = NULL;
            //a->minergate_work[a->current_job_id].job_id = 0;
            //a->minergate_work[a->current_job_id].start_time = time();
            a->minergate_work[job_id].state = SPONDWORK_STATE_EMPTY;
        }
    }
}


// Remove all work from queue
static void spond_flush_work(struct cgpu_info *cgpu)
{
	struct spond_adapter *a = cgpu->device_data;
	int i;
	spond_dropwork(a);
}



struct device_drv spondoolies_drv = {
	.drv_id = DRIVER_SPONDOOLIES,
	.dname = "Spondoolies",
	.name = "SPN",
	.drv_detect = spondoolies_detect,
	.get_api_stats = spondoolies_api_stats,
	.thread_prepare = spondoolies_prepare,
	//.scanhash = spondoolies_scanhash,
	.thread_shutdown = spondoolies_shutdown,	
	.hash_work = hash_queued_work,	
	.queue_full = spondoolies_queue_full,	
	.scanwork = spond_scanhash,
	.flush_work = spond_flush_work,
};



// runs once every second and puts all stats about current 
// execution in a json file
void spondoolies_drop_stats_to_file(int uptime) {
	int error;
	FILE* f = NULL;
	struct pool *cp;
	f = fopen("spondoolies_stats.json", "w");
	cp = current_pool();

	
	assert(f);

	json_t *top = json_object();
	json_object_set_new(top, "uptime" , json_integer(uptime));

	json_t *pool = json_object();
	json_object_set_new(pool, "rpc_url" , json_string(cp->rpc_url));
	json_object_set_new(pool, "rpc_proxy" , json_string(cp->rpc_proxy));
	json_object_set_new(pool, "accepted" , json_integer(cp->accepted));
	json_object_set_new(pool, "rejected" , json_integer(cp->rejected));	
	json_object_set_new(pool, "solved" , json_integer(cp->solved));	
	json_object_set_new(pool, "submit_fail" , json_integer(cp->submit_fail));		
	json_object_set_new(pool, "stale_shares" , json_integer(cp->stale_shares));			
	json_object_set_new(pool, "discarded_work" , json_integer(cp->discarded_work));				
	json_object_set_new(top, "pool", pool);

	
	error = json_dumpf(top, f, JSON_ENCODE_ANY);
	
	assert(error == 0);
	fclose(f);
}




void one_sec_spondoolies_watchdog(int uptime) {
     spondoolies_drop_stats_to_file(uptime);
}

