/*
 * Copyright 2014 Zvi (Zvisha) Shteingart - Spondoolies-tech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 *
 * Note that changing this SW will void your miners guaranty
 */



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
#include "defines.h"
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
#include "pwm_manager.h"
#include "hammer_lib.h"
#include "miner_gate.h"
#include "scaling_manager.h"
#include "corner_discovery.h"
#include "asic_thermal.h"
#include <syslog.h>
#include "pll.h"
#include "real_time_queue.h"
#include <signal.h>
#include "leds.h"


using namespace std;
pthread_mutex_t network_hw_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sigaction termhandler, inthandler;


// SERVER
void compute_hash(const unsigned char *midstate, unsigned int mrkle_root,
                  unsigned int timestamp, unsigned int difficulty,
                  unsigned int winner_nonce, unsigned char *hash);
int get_leading_zeroes(const unsigned char *hash);
void memprint(const void *m, size_t n);

typedef class {
public:
  uint8_t adapter_id;
  // uint8_t  adapter_id;
  int connection_fd;
  pthread_t conn_pth;
  minergate_req_packet *last_req;
  minergate_rsp_packet *next_rsp;

  // pthread_mutex_t queue_lock;
  queue<minergate_do_job_req> work_minergate_req;
  queue<minergate_do_job_rsp> work_minergate_rsp;

  void minergate_adapter() {}
} minergate_adapter;

minergate_adapter *adapters[0x100] = { 0 };
int kill_app = 0;

void exit_nicely() {
  int err;
  kill_app = 1;
  // Let other threads finish. 
  //set_light(LIGHT_YELLOW, LIGHT_MODE_OFF);
  usleep(1000*1000);
  set_light(LIGHT_GREEN, LIGHT_MODE_OFF);   
  disable_engines_all_asics();
  for (int l = 0 ; l < LOOP_COUNT ; l++) {
    dc2dc_disable_dc2dc(l, &err); 
  }
  set_fan_level(0);
  psyslog("Here comes unexpected death!\n");
  exit(0);  
}

int read_work_mode() {
	FILE* file = fopen ("/etc/mg_work_mode", "r");
	int i = 0;
	if (file <= 0) {
		vm.work_mode = 2;
	} else {
  	fscanf (file, "%d", &vm.work_mode);	  
    if (vm.work_mode < 0 || vm.work_mode > 2) {
      vm.work_mode = 2;
    }
    fclose (file);
	}

  if (vm.work_mode == 0) {
    vm.max_fan_level = FAN_QUIET;
    vm.vtrim_start = VTRIM_START_QUIET;
    vm.vtrim_max = VTRIM_MAX_QUIET;
  } else if (vm.work_mode == 1) {
    vm.max_fan_level = FAN_NORMAL;
    vm.vtrim_start = VTRIM_START_NORMAL;
    vm.vtrim_max = VTRIM_MAX_NORMAL;
  } else if (vm.work_mode == 2) {
    vm.max_fan_level = FAN_TURBO;
    vm.vtrim_start = VTRIM_START_TURBO;
    vm.vtrim_max = VTRIM_MAX_TURBO;
  }


  file = fopen ("/etc/mg_fan_speed_override", "r");
  if (file > 0) {
    int i;
    fscanf(file, "%d", &i);
    if (i >= 0 && i <= 100) {
      vm.max_fan_level = i;
    }
    fclose (file);
  } 


  file = fopen ("/etc/mg_max_voltage", "r");
  if (file > 0) {
    int vtrim;
    fscanf(file, "%d", &vtrim);
    if (vtrim >= 0 && i <= VTRIM_810 - VTRIM_MIN) {
      vm.vtrim_max = VTRIM_MIN+vtrim;
      if (vm.vtrim_start > vm.vtrim_max) {
        vm.vtrim_start = vm.vtrim_max;
      }
    }
    fclose (file);
  } 


   vm.max_ac2dc_power = AC2DC_POWER_LIMIT;
   file = fopen ("/etc/mg_psu_limit", "r");
   if (file > 0) {
    int limit;
    fscanf(file, "%d", &limit);
    if (limit >= 500) {
      vm.max_ac2dc_power = limit;
    }
    fclose (file);
  } 

  printf("WORK MODE = %d\n", vm.work_mode);
	
}


static void sighandler(int sig)
{
  /* Restore signal handlers so we can still quit if kill_work fails */  
  sigaction(SIGTERM, &termhandler, NULL);
  sigaction(SIGINT, &inthandler, NULL);
  exit_nicely();
}


void print_adapter(FILE *f ) {
  minergate_adapter *a = adapters[0];
  if (a) {
    fprintf(f, "Adapter queues: rsp=%d, req=%d\n", 
            a->work_minergate_rsp.size(),
           a->work_minergate_req.size());
  } else {
    fprintf(f, "No Adapter\n");
  }
}

void free_minergate_adapter(minergate_adapter *adapter) {
  close(adapter->connection_fd);
  free(adapter->last_req);
  free(adapter->next_rsp);
  delete adapter;
}

int SWAP32(int x) {
  return ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) |
          (((x) & 0xff0000) >> 8) | (((x) >> 24) & 0xff));
}

//
// Return the winner (or not winner) job back to the addapter queue.
//
void push_work_rsp(RT_JOB *work) {
  pthread_mutex_lock(&network_hw_mutex);
  minergate_do_job_rsp r;
  uint8_t adapter_id = work->adapter_id;
  minergate_adapter *adapter = adapters[adapter_id];
  if (!adapter) {
    DBG(DBG_NET, "Adapter disconected! Packet to garbage\n");
    pthread_mutex_unlock(&network_hw_mutex);
    return;
  }
  int i;
  r.mrkle_root = work->mrkle_root;
  r.winner_nonce = work->winner_nonce;
  r.work_id_in_sw = work->work_id_in_sw;
  r.res = 0;
  adapter->work_minergate_rsp.push(r);
  //passert(adapter->work_minergate_rsp.size() <= MINERGATE_TOTAL_QUEUE * 2);
  pthread_mutex_unlock(&network_hw_mutex);
}

//
// returns success, fills W with new job from adapter queue
//
int pull_work_req_adapter(RT_JOB *w, minergate_adapter *adapter) {
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

// returns success
int has_work_req_adapter(minergate_adapter *adapter) {
  return (adapter->work_minergate_req.size());
}

// returns success
int pull_work_req(RT_JOB *w) {
  // go over adapters...
  // TODO
  pthread_mutex_lock(&network_hw_mutex);
 int ret = false;
  minergate_adapter *adapter = adapters[0];
  if (adapter) {
      ret =pull_work_req_adapter(w, adapter);
  }
  pthread_mutex_unlock(&network_hw_mutex);
  return ret;
}

int has_work_req() {
  pthread_mutex_lock(&network_hw_mutex);
  minergate_adapter *adapter = adapters[0];
  if (adapter) {
    has_work_req_adapter(adapter);
  }
  pthread_mutex_unlock(&network_hw_mutex);
}

void push_work_req(minergate_do_job_req *req, minergate_adapter *adapter) {
  pthread_mutex_lock(&network_hw_mutex);

  if (adapter->work_minergate_req.size() >= (MINERGATE_TOTAL_QUEUE - 10)) {
    minergate_do_job_rsp rsp;
    rsp.mrkle_root = req->mrkle_root;
    rsp.winner_nonce = 0;
    rsp.work_id_in_sw = req->work_id_in_sw;
    rsp.res = 1;
    // printf("returning %d %d\n",req->work_id_in_sw,rsp.work_id_in_sw);
    adapter->work_minergate_rsp.push(rsp);
  } else {
    adapter->work_minergate_req.push(*req);
  }
  pthread_mutex_unlock(&network_hw_mutex);
}

// returns success
int pull_work_rsp(minergate_do_job_rsp *r, minergate_adapter *adapter) {
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
extern pthread_mutex_t hammer_mutex;
//
// Support new minergate client
//
void *connection_handler_thread(void *adptr) {
  psyslog("New adapter connected!\n");
  minergate_adapter *adapter = (minergate_adapter *)adptr;
  // DBG(DBG_NET,"connection_fd = %d\n", adapter->connection_fd);
  set_light(LIGHT_GREEN, LIGHT_MODE_FAST_BLINK);

  adapter->adapter_id = 0;
  adapters[0] = adapter;
  adapter->last_req = allocate_minergate_packet_req(0xca, 0xfe);
  adapter->next_rsp = allocate_minergate_packet_rsp(0xca, 0xfe);

  vm.idle_probs = 0;
  vm.busy_probs = 0;
  vm.solved_jobs_total = 0;


  RT_JOB work;

  while (one_done_sw_rt_queue(&work)) {
    push_work_rsp(&work);
  }
  int nbytes;

  // Read packet
  struct timeval now;      
  struct timeval last_time; 
  gettimeofday(&now, NULL);
  gettimeofday(&last_time, NULL);
  while ((nbytes = read(adapter->connection_fd, (void *)adapter->last_req,
                        sizeof(minergate_req_packet))) > 0) {
    struct timeval now;      
    struct timeval last_time; 
    int usec;
    if (nbytes) {
      passert(adapter->last_req->magic == 0xcaf4);
      gettimeofday(&now, NULL);

      usec = (now.tv_sec - last_time.tv_sec) * 1000000;
      usec += (now.tv_usec - last_time.tv_usec);
  

      pthread_mutex_lock(&hammer_mutex);     
      pthread_mutex_lock(&network_hw_mutex);
      vm.not_mining_time = 0;
      if (vm.asics_shut_down_powersave) {
        unpause_all_mining_engines();
      }
      pthread_mutex_unlock(&network_hw_mutex);
      pthread_mutex_unlock(&hammer_mutex);

      // Reset packet.
      int i;
      // Return all previous responces
      int rsp_count = adapter->work_minergate_rsp.size();
      DBG(DBG_NET, "Sending %d minergate_do_job_rsp\n", rsp_count);
      if (rsp_count > MAX_RESPONDS) {
        rsp_count = MAX_RESPONDS;
      }

      for (i = 0; i < rsp_count; i++) {
        minergate_do_job_rsp *rsp = adapter->next_rsp->rsp + i;
        int res = pull_work_rsp(rsp, adapter);
        passert(res);
      }
      adapter->next_rsp->rsp_count = rsp_count;
      int mhashes_done = (vm.total_mhash>>10)*(usec>>10);
      adapter->next_rsp->gh_div_10_rate = mhashes_done>>10;  

      int array_size = adapter->last_req->req_count;
      for (i = 0; i < array_size; i++) { // walk the jobs
        minergate_do_job_req *work = adapter->last_req->req + i;
        push_work_req(work, adapter);
      }

      // parse_minergate_packet(adapter->last_req, minergate_data_processor,
      // adapter, adapter);
      adapter->next_rsp->request_id = adapter->last_req->request_id;
      // Send response
      write(adapter->connection_fd, (void *)adapter->next_rsp,
            sizeof(minergate_rsp_packet));

      // Clear packet.
      adapter->next_rsp->rsp_count = 0;
      last_time = now;
    }
  }
  adapters[adapter->adapter_id] = NULL;  
  set_light(LIGHT_GREEN, LIGHT_MODE_SLOW_BLINK);
  free_minergate_adapter(adapter);
  // Clear the real_time_queue from the old packets
  adapter = NULL;
  return 0;
}

int init_socket() {
  struct sockaddr_un address;
  int socket_fd, connection_fd;
  socklen_t address_length;
  pid_t child;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    DBG(DBG_NET, "socket() failed\n");
    perror("Err:");
    return 1;
  }

  unlink(MINERGATE_SOCKET_FILE);

  /* start with a clean address structure */
  memset(&address, 0, sizeof(struct sockaddr_un));

  address.sun_family = AF_UNIX;
  sprintf(address.sun_path, MINERGATE_SOCKET_FILE);

  if (bind(socket_fd, (struct sockaddr *)&address,
           sizeof(struct sockaddr_un)) !=
      0) {
    perror("Err:");
    return 0;
  }

  if (listen(socket_fd, 5) != 0) {
    perror("Err:");
    return 0;
  }

  return socket_fd;
}



void test_asic_reset() {
  // Reset ASICs
  write_reg_broadcast(ADDR_GOT_ADDR, 0);

  // If someone not reseted (has address) - we have a problem
  int reg = read_reg_broadcast(ADDR_BR_NO_ADDR);
  if (reg == 0) {
    // Don't remove - used by tests
    printf("got reply from ASIC 0x%x\n", BROADCAST_READ_ADDR(reg));
    printf("RESET BAD\n");
  } else {
    // Don't remove - used by tests
    printf("RESET GOOD\n");
  }
  return;
}


void enable_sinal_handler() {
  struct sigaction handler;
  handler.sa_handler = &sighandler;
  handler.sa_flags = 0;
  sigemptyset(&handler.sa_mask);
  sigaction(SIGTERM, &handler, &termhandler);
  sigaction(SIGINT, &handler, &inthandler);

}
 
void reset_squid() {
  FILE *f = fopen("/sys/class/gpio/export", "w");
  if (!f)
    return;
  fprintf(f, "47");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio47/direction", "w");
  if (!f)
    return;
  fprintf(f, "out");
  fclose(f);
  f = fopen("/sys/class/gpio/gpio47/value", "w");
  if (!f)
    return;
  fprintf(f, "0");
  usleep(10000);
  fprintf(f, "1");
  usleep(20000);
  fclose(f);
}






int main(int argc, char *argv[]) {
  printf(RESET);
  int testreset_mode = 0;
  int init_mode = 0;
  int s;
  srand (time(NULL));
  enable_reg_debug = 0;
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog("minergate", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  syslog(LOG_NOTICE, "minergate started");
 
  enable_sinal_handler();

  if ((argc > 1) && strcmp(argv[1], "--help") == 0) {
    printf("--testreset = Test asic reset!!!\n");
    printf("--silent-test = Work with 10 ASICs!!!\n");
    printf("--thermal-test = Only test thermal!!!\n");    
    return 0;
  }


  if ((argc > 1) && strcmp(argv[1], "--silent-test") == 0) {
    vm.silent_mode = 1;
    printf("Test mode, using few ASICs !!!\n");
  }

 
  if ((argc > 1) && strcmp(argv[1], "--thermal-test") == 0) {
    vm.thermal_test_mode = 1;
    printf("Test mode, using few ASICs !!!\n");
  }

  if ((argc > 1) && strcmp(argv[1], "--testreset") == 0) {
    testreset_mode = 1;
    printf("Test reset mode!!!\n");
  }


  struct sockaddr_un address;
  int socket_fd, connection_fd;
  socklen_t address_length = sizeof(address);
  pthread_t main_thread;
  pthread_t dc2dc_thread;
  // pthread_t conn_pth;
  pid_t child;
  psyslog("Read work mode\n");
  read_work_mode();
  // Must be done after "read_work_mode"
  psyslog("Read  NVM\n");
  load_nvm_ok();
  psyslog("reset_squid\n");
  reset_squid();
  psyslog("init_spi\n");
  init_spi();
  psyslog("i2c_init\n");
  i2c_init();
  psyslog("dc2dc_init\n");
  dc2dc_init();
  psyslog("ac2dc_init\n");
  ac2dc_init();
  psyslog("init_pwm\n");
  init_pwm();
  psyslog("set_fan_level\n");
  set_fan_level(vm.max_fan_level);
  //exit(0);
  reset_sw_rt_queue();
  leds_init();
  //set_light(LIGHT_YELLOW, LIGHT_MODE_ON);
  set_light(LIGHT_GREEN, LIGHT_MODE_SLOW_BLINK);


  // test SPI
  int q_status = read_spi(ADDR_SQUID_PONG);
  passert((q_status == 0xDEADBEEF),
          "ERROR: no 0xdeadbeef in squid pong register!\n");



  // Find good loops
  // Update vm.good_loops
  // Set ASICS on all disabled loops to asic_ok=0
  discover_good_loops();

 

  //set loops in FPGA
  if (!enable_good_loops_ok()) {
    psyslog("LOOP TEST FAILED, RESTARTING:%x\n", vm.good_loops);
    passert(0);
  }



  psyslog("enable_good_loops_ok done %d\n", __LINE__);
  // Allocates addresses, sets nonce range.
  // Reset all hammers
  init_hammers();

  
  int addr;
   //passert(read_reg_broadcast(ADDR_VERSION)&0xFF == 0x3c);
 
   while (addr = BROADCAST_READ_ADDR(read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY))) {
      psyslog(RED "CONDUCTOR BUZY IN %x (%X)\n" RESET, addr,read_reg_broadcast(ADDR_VERSION));
      disable_asic_forever_rt(addr);
   }
    
  // Give addresses to devices.
  allocate_addresses_to_devices(); 
  //passert(read_reg_broadcast(ADDR_VERSION)&0xFF == 0x3c);
  // Set nonce ranges
  set_nonce_range_in_engines(0xFFFFFFFF);

  // Set default frequencies.
  // Set all voltage to ASIC_VOLTAGE_810
  // Set all frequency to ASIC_FREQ_225
  set_safe_voltage_and_frequency();
  // Set all engines to 0x7FFF
  psyslog("hammer initialisation done %d\n", __LINE__);
  thermal_init();
  

  // Enables NVM engines in ASICs.
  //  printf("enable_voltage_from_nvm\n");
  //  enable_voltage_from_nvm();
    
  psyslog("init_scaling done, ready to mine, saving NVM\n");

  if (testreset_mode) {
    test_asic_reset();
    return 0;
  }


  //compute_corners();
  set_working_voltage_discover_top_speeds();

  
  psyslog("Opening socket for cgminer\n");
  // test HAMMER read
  // passert(read_reg_broadcast(ADDR_VERSION), "No version found in ASICs");
  socket_fd = init_socket();
  passert(socket_fd > 0);

  psyslog("Starting HW thread\n");

  s = pthread_create(&main_thread, NULL, squid_regular_state_machine_rt,
                     (void *)NULL);
  passert(s == 0);
  s = pthread_create(&dc2dc_thread, NULL, i2c_state_machine_nrt,
                     (void *)NULL);
  passert(s == 0);


  minergate_adapter *adapter = new minergate_adapter;
  passert((int)adapter);
  while ((adapter->connection_fd =
              accept(socket_fd, (struct sockaddr *)&address, &address_length)) >
         -1) {
    // Only 1 thread supportd so far...
    psyslog("New adapter connected %d %x!\n", adapter->connection_fd, adapter);
    s = pthread_create(&adapter->conn_pth, NULL, connection_handler_thread,
                       (void *)adapter);
    passert(s == 0);

    adapter = new minergate_adapter;
    passert((int)adapter);
  }
  psyslog("Err %d:", adapter->connection_fd);
  passert(0, "Err");

  close(socket_fd);
  unlink(MINERGATE_SOCKET_FILE);
  return 0;
}

