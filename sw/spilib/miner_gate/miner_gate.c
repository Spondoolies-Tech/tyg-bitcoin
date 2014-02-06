/*
    Minergate server network interface.
    Multithreaded application, listens on socket and handles requests.
    Pushes mining requests from network to lock and returns 
    asynchroniously responces from work_minergate_rsp.

    Each network connection has adaptor structure, that holds all the
    data needed for the connection - last packet with request, 
    next packet with responce etc`.

    
    by Zvisha Shteingart
*/

#include "mg_proto_parser.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h> //for sockets
#include <netinet/in.h> //for internet
#include <pthread.h>
#include "hammer.h"
#include <queue>
#include "spond_debug.h"
#include "squid.h"
#include "i2c.h"
#include "dc2dc.h"
#include "ac2dc.h"
#include <syslog.h>


using namespace std;
pthread_mutex_t network_hw_mutex = PTHREAD_MUTEX_INITIALIZER;
int drop_job_requests = 0;
int noasic = 0;
int enable_scaling = 1;

// SERVER
void compute_hash(const unsigned char * midstate, unsigned int mrkle_root,
        unsigned int timestamp, unsigned int difficulty,
        unsigned int winner_nonce, unsigned char * hash);
int get_leading_zeroes(const unsigned char *hash);
void memprint(const void *m, size_t n);


typedef class {
  public:  
    uint8_t  adapter_id;    
    //uint8_t  adapter_id;
    int connection_fd;
    pthread_t conn_pth;
    minergate_packet* last_req;
    minergate_packet* next_rsp;
    
    //pthread_mutex_t queue_lock;
    queue<minergate_do_job_req> work_minergate_req;
    queue<minergate_do_job_rsp> work_minergate_rsp;

    void minergate_adapter() {
        
    }
} minergate_adapter;


minergate_adapter* adapters[0x100] = {0};


void print_adapter() {
    minergate_adapter *a = adapters[0];
    if (a) {
        printf("Adapter queues: rsp=%d, req=%d\n",
            a->work_minergate_rsp.size(),
            a->work_minergate_req.size());
    }
}

void free_minergate_adapter(minergate_adapter* adapter) {
    close(adapter->connection_fd);
    free(adapter->last_req);
    free(adapter->next_rsp);
    delete adapter;
}



int SWAP32(int x) {
    return ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8)
            | (((x) & 0xff0000) >> 8) | (((x) >> 24) & 0xff));
}


//
// Return the winner (or not winner) job back to the addapter queue.
//
void push_work_rsp(RT_JOB* work) {
    pthread_mutex_lock(&network_hw_mutex);
    minergate_do_job_rsp r;
    uint8_t adapter_id = work->adapter_id;
    minergate_adapter* adapter = adapters[adapter_id];
    if (!adapter) {
        DBG(DBG_NET,"Adapter disconected! Packet to garbage\n");
        pthread_mutex_unlock(&network_hw_mutex);
        return;
    }
    int i;
    r.mrkle_root = work->mrkle_root;
    r.winner_nonce = work->winner_nonce;
    r.work_id_in_sw = work->work_id_in_sw;
    r.res = 0;
    adapter->work_minergate_rsp.push(r);
    passert(adapter->work_minergate_rsp.size() <= MINERGATE_TOTAL_QUEUE*2);
    pthread_mutex_unlock(&network_hw_mutex);
}






// 
// returns success, fills W with new job from adapter queue
// 
int pull_work_req_adapter(RT_JOB* w, minergate_adapter* adapter) {
    minergate_do_job_req r;
    
    if (!adapter->work_minergate_req.empty()) {
        r = adapter->work_minergate_req.front();
        adapter->work_minergate_req.pop();
        w->difficulty = r.difficulty;
        memcpy(w->midstate, r.midstate, sizeof(r.midstate));
        w->adapter_id = adapter->adapter_id;
        w->mrkle_root = r.mrkle_root;
        w->timestamp = r.timestamp;
        w->winner_nonce = 0;
        w->work_id_in_sw = r.work_id_in_sw;
        w->work_state = 0;
        w->leading_zeroes = r.leading_zeroes;
        return 1;
    }
    return 0;
}




// TODO - locks!!!
// returns success
int has_work_req_adapter(minergate_adapter* adapter) {
   return (adapter->work_minergate_req.size());
}




// TODO - locks!!!
// returns success
int pull_work_req(RT_JOB* w) {
    // go over adapters...
    // TODO
    pthread_mutex_lock(&network_hw_mutex);
    minergate_adapter * adapter = adapters[0];
    if(adapter) {
        pull_work_req_adapter(w,adapter);
    }
    pthread_mutex_unlock(&network_hw_mutex);
}


int has_work_req() {
    pthread_mutex_lock(&network_hw_mutex);
    minergate_adapter * adapter = adapters[0];
    if(adapter) {
        has_work_req_adapter(adapter);
    }
    pthread_mutex_unlock(&network_hw_mutex);

}


void push_work_req(minergate_do_job_req* req, minergate_adapter* adapter) {
    pthread_mutex_lock(&network_hw_mutex);

    if (drop_job_requests || adapter->work_minergate_req.size() >= (MINERGATE_TOTAL_QUEUE - 10)) {
        minergate_do_job_rsp rsp;
        rsp.mrkle_root = req->mrkle_root;
        rsp.winner_nonce = 0;
        rsp.work_id_in_sw = req->work_id_in_sw;
        rsp.res = 1;
        //printf("returning %d %d\n",req->work_id_in_sw,rsp.work_id_in_sw);
        adapter->work_minergate_rsp.push(rsp);
    } else {
        adapter->work_minergate_req.push(*req); 
    } 
    pthread_mutex_unlock(&network_hw_mutex);
}

// returns success
int pull_work_rsp(minergate_do_job_rsp* r, minergate_adapter* adapter) {
    pthread_mutex_lock(&network_hw_mutex);
    if (!adapter->work_minergate_rsp.empty()) {
        *r = adapter->work_minergate_rsp.front();
        adapter->work_minergate_rsp.pop();
        pthread_mutex_unlock(&network_hw_mutex);
        return 1;
    }
    pthread_mutex_unlock(&network_hw_mutex);
    return 0;
}



// c = mp_rsp, 
int minergate_data_processor(minergate_data* md, void* adaptr, void* c2) {
    // parse JOB requests and store in the SW queue? 
    //DBG(DBG_NET,"Got %d %d %d\n",md->data_id,md->data_length,md->magic);
    minergate_adapter* adapter = (minergate_adapter*)adaptr;
    if (md->data_id == MINERGATE_DATA_ID_DO_JOB_REQ) {     
        int i;
        // Return all previous responces
        int rsp_count = adapter->work_minergate_rsp.size();
        DBG(DBG_NET,"Sending %d minergate_do_job_rsp\n", rsp_count);
        minergate_data* md_res = get_minergate_data(adapter->next_rsp,  
                                 rsp_count*sizeof(minergate_do_job_rsp), 
                                  MINERGATE_DATA_ID_DO_JOB_RSP);
        //adapter->next_rsp;
      //  DBG(DBG_NET,"rsp_count %d\n", rsp_count);
        for (i=0;i<rsp_count;i++) {
            //printf("rsp ");
            minergate_do_job_rsp* rsp = ((minergate_do_job_rsp*)md_res->data) + i;
            int res = pull_work_rsp(rsp, adapter);
            passert(res);
            
        }



        
        //DBG(DBG_NET, "GOT minergate_do_job_req: %x/%x\n", sizeof(minergate_do_job_req), md->data_length);
        int array_size = md->data_length / sizeof(minergate_do_job_req);
        DBG(DBG_NET,"Got %d minergate_do_job_req\n", array_size);
        passert(md->data_length % sizeof(minergate_do_job_req) == 0);
        
        for (i = 0; i < array_size; i++) { // walk the jobs
             //printf("j");
             minergate_do_job_req* work = ((minergate_do_job_req*)md->data) + i;
             //fill_random_work2(&work);
             push_work_req(work, adapter);
            //minergate_do_job_req
            //printf("!!!GOT minergate_do_job\n");
        }

    }
}



//
// Support new minergate client
//
int one_done_sw_rt_queue(RT_JOB *work);

void* connection_handler_thread(void* adptr)
{
   // return 0;
     minergate_adapter* adapter = ( minergate_adapter*)adptr;
//   int connection_fd = (int)cf;
     DBG(DBG_NET,"connection_fd = %d\n", adapter->connection_fd);
     

    // (minergate_adapter*)malloc(sizeof(minergate_adapter));
     int nbytes;
     
     //minergate_data* md1 =    get_minergate_data(adapter->next_rsp,  300, 3);
     //minergate_data* md2 =  get_minergate_data(adapter->next_rsp,  400, 4);
     //Read packet
     while((nbytes = read(adapter->connection_fd, (void*)adapter->last_req, 10000)) > 0) {
         //DBG(DBG_NET,"got:%d - %x\n", nbytes, *((char*)adapter->last_req));        
         if (nbytes) {
              //DBG(DBG_NET,"got req len:%d %d\n", adapter->last_req->data_length + MINERGATE_PACKET_HEADER_SIZE, nbytes);
              passert(adapter->last_req->magic == 0xcafe);
              if ((adapter->last_req->magic == 0xcafe)
                && (adapter->last_req->data_length + MINERGATE_PACKET_HEADER_SIZE == nbytes)) {
                  //DBG(DBG_NET,"MESSAGE FROM CLIENT: %x\n", adapter->last_req->request_id);
                  // Parse request
                  parse_minergate_packet(adapter->last_req, minergate_data_processor, adapter, adapter);                  
                  adapter->next_rsp->request_id = adapter->last_req->request_id;
                  // Send response
                  write(adapter->connection_fd, 
                       (void*)adapter->next_rsp, 
                       adapter->next_rsp->data_length + MINERGATE_PACKET_HEADER_SIZE);

                  // Clear packet.
                  adapter->next_rsp->data_length = 0;
              } else {
                DBG(DBG_NET,"Dropped bad packet!\n");
              }
         }
     }
     adapters[adapter->adapter_id] = NULL;
     free_minergate_adapter(adapter); 
     // Clear the real_time_queue from the old packets
     if (!noasic) {
         RT_JOB work;
         while(one_done_sw_rt_queue(&work)) {
             push_work_rsp(&work);
         }
     }
     adapter = NULL;          
     return 0;
}







int init_socket() {
    struct sockaddr_un address;
    int socket_fd, connection_fd;
    socklen_t address_length;
    pid_t child;

 socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
 if(socket_fd < 0)
 {
  DBG(DBG_NET,"socket() failed\n");
  perror("Err:");
  return 1;
 } 

    unlink(MINERGATE_SOCKET_FILE);

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    sprintf(address.sun_path, MINERGATE_SOCKET_FILE);


    if(bind(socket_fd, 
    (struct sockaddr *) &address, 
    sizeof(struct sockaddr_un)) != 0)
    {
     DBG(DBG_NET,"bind() failed\n");
      perror("Err:");
     return 0;
    }

    if(listen(socket_fd, 5) != 0)
    {
     DBG(DBG_NET,"listen() failed\n");
      perror("Err:");
     return 0;
    }

    return socket_fd;
}

uint32_t read_reg_broadcast_test(uint8_t offset);
extern int assert_serial_failures;

int  parse_squid_status(int v) {
    if(v & BIT_STATUS_SERIAL_Q_TX_FULL       ) printf("BIT_STATUS_SERIAL_Q_TX_FULL       ");
    if(v & BIT_STATUS_SERIAL_Q_TX_NOT_EMPTY  ) printf("BIT_STATUS_SERIAL_Q_TX_NOT_EMPTY  ");
    if(v & BIT_STATUS_SERIAL_Q_TX_EMPTY      ) printf("BIT_STATUS_SERIAL_Q_TX_EMPTY      ");
    if(v & BIT_STATUS_SERIAL_Q_RX_FULL       ) printf("BIT_STATUS_SERIAL_Q_RX_FULL       ");
    if(v & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY  ) printf("BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY  ");
    if(v & BIT_STATUS_SERIAL_Q_RX_EMPTY      ) printf("BIT_STATUS_SERIAL_Q_RX_EMPTY      ");
    if(v & BIT_STATUS_SERVICE_Q_FULL         ) printf("BIT_STATUS_SERVICE_Q_FULL         ");
    if(v & BIT_STATUS_SERVICE_Q_NOT_EMPTY    ) printf("BIT_STATUS_SERVICE_Q_NOT_EMPTY    ");
    if(v & BIT_STATUS_SERVICE_Q_EMPTY        ) printf("BIT_STATUS_SERVICE_Q_EMPTY        ");
    if(v & BIT_STATUS_FIFO_SERIAL_TX_ERR     ) printf("BIT_STATUS_FIFO_SERIAL_TX_ERR     ");
    if(v & BIT_STATUS_FIFO_SERIAL_RX_ERR     ) printf("BIT_STATUS_FIFO_SERIAL_RX_ERR     ");
    if(v & BIT_STATUS_FIFO_SERVICE_ERR       ) printf("BIT_STATUS_FIFO_SERVICE_ERR       ");
    if(v & BIT_STATUS_CHAIN_EMPTY            ) printf("BIT_STATUS_CHAIN_EMPTY            ");
    if(v & BIT_STATUS_LOOP_TIMEOUT_ERROR     ) printf("BIT_STATUS_LOOP_TIMEOUT_ERROR     ");
    if(v & BIT_STATUS_LOOP_CORRUPTION_ERROR  ) printf("BIT_STATUS_LOOP_CORRUPTION_ERROR  ");
    if(v & BIT_STATUS_ILLEGAL_ACCESS         ) printf("BIT_STATUS_ILLEGAL_ACCESS         ");
    printf("\n");
}








void* squid_regular_state_machine(void* p);
int init_hammers();
void init_scaling();
int do_bist_ok(bool with_current_measurment);
uint32_t crc32(uint32_t crc, const void *buf, size_t size);
void enable_nvm_loops();
void enable_voltage_freq_and_engines_from_nvm();
void enable_voltage_freq_and_engines_default(); 

void recompute_corners_and_voltage_update_nvm();
void set_voltage(int loop_id, DC2DC_VOLTAGE v);
void spond_save_nvm();
void create_default_nvm(int from_scratch, int check_loops);
void find_bad_engines_update_nvm();
int enable_nvm_loops_ok();
int allocate_addresses_to_devices();
void set_nonce_range_in_engines(unsigned int max_range);
void enable_all_engines_all_asics();


void reset_squid() {
    FILE *f = fopen("/sys/class/gpio/export", "w"); 
    if (!f) return;
    fprintf(f, "47");
    fclose(f);
    f = fopen("/sys/class/gpio/gpio47/direction", "w");
    if (!f) return;    
    fprintf(f, "out");
    fclose(f);    
    f = fopen("/sys/class/gpio/gpio47/value", "w");
    if (!f) return;    
    fprintf(f, "0");
    usleep(10000);
    fprintf(f, "1");
    usleep(20000);
    fclose(f);
    /*
    cd /sys/class/gpio
    echo 47 > export
    cd gpio47
    echo out > direction
    echo 0 > value
    echo 1 > value
    */
}

int main(int argc, char *argv[])
{
 int test_mode = 0;
 int testreset_mode = 0;
 int init_mode = 0;
 int s;
 
 setlogmask (LOG_UPTO (LOG_INFO));
 openlog ("minergate", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
 syslog (LOG_NOTICE, "minergate started");
// syslog (LOG_INFO, "A tree falls in a forest");
// syslog (LOG_ALERT, "Real issue - A tree falls in a forest");
 
	
 if ((argc > 1) && strcmp(argv[1],"--help") == 0) {
    test_mode = 1;
	printf ("--testreset = Test asic reset!!!\n");
    printf ("--test = Test mode, remove NVM!!!\n");
	printf ("--noscale = No scaling mode!!!\n");
	printf ("--noasic = NO HW MODE!!!\n");
	printf ("--init = Re-init mode!!!\n");
    return 0;
 }


 if ((argc > 1) && strcmp(argv[1],"--test") == 0) {
    test_mode = 1;
    printf ("Test mode, remove NVM!!!\n");
    spond_delete_nvm();
 }

 
 if ((argc > 1) && strcmp(argv[1],"--testreset") == 0) {
    testreset_mode = 1;
    printf ("Test reset mode, remove NVM!!!\n");
    spond_delete_nvm();
 }

 if ((argc > 1) && strcmp(argv[1],"--noscale") == 0) {
    enable_scaling = 0;
    printf ("No scaling mode!!!\n");
 }


 if ((argc > 1) && strcmp(argv[1],"--noasic") == 0) {
    printf ("NO HW MODE!!!!\n");
    noasic = 1;
 }

 if ((argc > 1) && strcmp(argv[1],"--init") == 0) {
    printf ("Re-init mode!!!\n");
    // Delete NVM
    spond_delete_nvm();
 }

  
 struct sockaddr_un address;
 int socket_fd, connection_fd;
 socklen_t address_length = sizeof(address);
 pthread_t main_thread;
 //pthread_t conn_pth;
 pid_t child;
 reset_squid();
 init_spi();
 i2c_init();
 dc2dc_init();
 //read_mgmt_temp();
/*
 while (1) {
 	printf("Current power::: = %d\n",ac2dc_get_power());
		printf("Current tmp0::: = %d\n",ac2dc_get_temperature(0));
			printf("Current tmp1::: = %d\n",ac2dc_get_temperature(1));
				printf("Current tmp2::: = %d\n",ac2dc_get_temperature(2));
	sleep(1);
 }
 */
 
 /*
 int power = ac2dc_get_temperature(0);
 printf("Current temp1 = %d\n",power);
 power = ac2dc_get_temperature(1);
 printf("Current temp1 = %d\n",power);
 power = ac2dc_get_temperature(2);
 printf("Current temp1 = %d\n",power);
*/			
 // test SPI 
 int q_status = read_spi(ADDR_SQUID_PONG);
 passert((q_status == 0xDEADBEEF), "ERROR: no 0xdeadbeef in squid pong register!\n");


 // set voltage test
 /*
    dc2dc_set_voltage(0, ASIC_VOLTAGE_810);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_790);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_765);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_720);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_675);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_630);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_585);
    sleep(3);
    dc2dc_set_voltage(0, ASIC_VOLTAGE_555); 
    sleep(3);
 */
	 printf("--------------- %d\n", __LINE__);

 if (!load_nvm_ok() || !enable_scaling) {
    // Init NVM.
    // Finds good loops using broadcast reads. No addresses given.
    printf("CREATING NEW NVM!\n");
    create_default_nvm(1, !noasic);
    spond_save_nvm();
 } 

 printf("--------------- %d\n", __LINE__);


 if (!noasic) {
     if (!enable_nvm_loops_ok()) {
         // reinitialise
         printf("LOOP TEST FAILED, DELETE NVM AND RESTART\n");
         spond_delete_nvm();
         // Exits on error.
         passert(0);
     }
 }
 
 printf("--------------- %d\n", __LINE__);
 // Allocates addresses, sets nonce range.
 if (!noasic) {
     init_hammers(); 
     allocate_addresses_to_devices();
     
     // Loads NVM, sets freq`, enables engines... 
     //passert(total_devices);
     set_nonce_range_in_engines(0xFFFFFFFF); 

     // Set default frequencies.
     enable_voltage_freq_and_engines_default();
     enable_all_engines_all_asics();


     // UPDATE NVM DATA by running some tests.
     if (!nvm->bad_engines_found) {
         // Sets bad engines in NVM
         find_bad_engines_update_nvm();
     }
     if (!nvm->corners_computed) {
         // Sets asic Corner and loop voltages
         recompute_corners_and_voltage_update_nvm();
     }
 }
 printf("--------------- %d\n", __LINE__);

 // Save NVM unless running without scaling
 spond_save_nvm();

 printf("--------------- %d\n", __LINE__);

 if ((!noasic) && enable_scaling) {
     // Enables NVM engines in ASICs.
     enable_voltage_freq_and_engines_from_nvm();     
     printf("init_scaling\n");
     init_scaling();
 }

 printf("--------------- %d\n", __LINE__);


 if (testreset_mode) {
	// Reset ASICs
	write_reg_broadcast(ADDR_GOT_ADDR, 0);
	
	// If someone not reseted (has address) - we have a problem
	int reg = read_reg_broadcast(ADDR_BR_NO_ADDR);
    if (reg == 0) {
		// Don't remove - used by tests
		printf("got reply from ASIC 0x%x\n", BROADCAST_READ_ADDR(reg));
        printf("RESET BAD\n");
		return 1;
    } else {
  	    // Don't remove - used by tests
		printf("RESET GOOD\n");
		return 0;
    }

	
 }

 
 // test HAMMER read
 //passert(read_reg_broadcast(ADDR_VERSION), "No version found in ASICs");
 socket_fd = init_socket();
 passert(socket_fd > 0);
 printf("done\n");

 printf("--------------- %d\n", __LINE__);

 if (test_mode) {
    if (!noasic) {
        printf("Running BISTs... :");
        printf(do_bist_ok(false)?" OK":" FAIL");        
        printf(do_bist_ok(false)?" OK":" FAIL");        
        printf(do_bist_ok(false)?" OK\n":" FAIL\n");            
    }
    for (int l = 0; l < LOOP_COUNT; l++) {
        for (int h = 0; h < HAMMERS_PER_LOOP; h++) {
            if (!miner_box.loop[l].enabled) {
                // DONT REMOVE THIS PRINT!! USED BY TESTS!!
                printf("Hammer %02d %02d DISCONNECTED\n", l, h);
            } else if (!miner_box.hammer[l*HAMMERS_PER_LOOP+h].present) {
                // DONT REMOVE THIS PRINT!! USED BY TESTS!!
                printf("Hammer %02d %02d MISSING\n", l, h);
            } else if (miner_box.hammer[l*HAMMERS_PER_LOOP + h].failed_bists) {
                // DONT REMOVE THIS PRINT!! USED BY TESTS!!
                printf("Hammer %02d %02d BIST_FAIL\n", l, h);
            } else {
                // DONT REMOVE THIS PRINT!! USED BY TESTS!!
                printf("Hammer %02d %02d OK\n", l, h);
            }
        }
    }
    return 0;
 }

 if (!noasic) {
    s = pthread_create(&main_thread,NULL,squid_regular_state_machine,(void*)NULL);
    passert (s == 0);
 }
 while((connection_fd = accept(socket_fd, 
                              (struct sockaddr *) &address,
                               &address_length)) > -1) {
    // Only 1 thread supportd so far...    
    minergate_adapter* adapter = new minergate_adapter;
    passert((int)adapter);
    adapter->adapter_id = 0;
    adapters[0] = adapter;
    adapter->connection_fd = connection_fd;
    adapter->last_req = allocate_minergate_packet(
        MINERGATE_TOTAL_QUEUE*sizeof(minergate_do_job_req) + MINERGATE_DATA_HEADER_SIZE, 0xca, 0xfe);
    adapter->next_rsp = allocate_minergate_packet(
        (2*MINERGATE_TOTAL_QUEUE*sizeof(minergate_do_job_rsp)) + MINERGATE_DATA_HEADER_SIZE, 0xca, 0xfe);


    s = pthread_create(&adapter->conn_pth,NULL,connection_handler_thread,(void*)adapter);
    passert (s == 0);
 }
 printf("Err %d:", connection_fd);
 passert(0,"Err");

 close(socket_fd);
 unlink(MINERGATE_SOCKET_FILE);
 return 0;
}

