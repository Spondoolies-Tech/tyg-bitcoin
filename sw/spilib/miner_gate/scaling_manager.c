#include "squid.h"
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>
#include "queue.h"
#include "spond_debug.h"
#include "hammer.h"
#include <sys/time.h>
#include "nvm.h"
#include "ac2dc.h"
#include "dc2dc.h"
#include "hammer_lib.h"


/*
       Supported temp sensor settings:
   temp    83 / 89 / 95 / 101 / 107 / 113 /119 / 125
   T[2:0]  0    1     2    3     4     5    6     7 
*/

// When ariving here (>=) cut the frequency drastically.
int8_t CRITICAL_TEMPERATURE_PER_CORNER[ASIC_CORNER_COUNT] = {4,4,4,4,4,4};
// When under this (<) raise the frequency slowly
int8_t BEST_TEMPERATURE_PER_CORNER[ASIC_CORNER_COUNT] = {3,3,3,3,3,3};

// When under this (<) raise the frequency slowly
//TODO
int SAFE_FREQ_PER_CORNER[ASIC_CORNER_COUNT] = {ASIC_FREQ_210, ASIC_FREQ_210, ASIC_FREQ_210,ASIC_FREQ_210,ASIC_FREQ_210,ASIC_FREQ_210};
int AC_CURRNT_PER_15_HZ[ASIC_VOLTAGE_COUNT] = {1,1,1,1,1,1,1,1};
int DC_CURRNT_PER_15_HZ[ASIC_VOLTAGE_COUNT] = {1,1,1,1,1,1,1,1};

DC2DC_VOLTAGE CORNER_TO_VOLTAGE_TABLE[ASIC_CORNER_COUNT] = 
    {ASIC_VOLTAGE_765, ASIC_VOLTAGE_765, ASIC_VOLTAGE_720, 
     ASIC_VOLTAGE_630, ASIC_VOLTAGE_630, ASIC_VOLTAGE_630};


void send_keep_alive();

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
void print_state();
void print_scaling();

MINER_BOX miner_box = {0};
extern SPONDOOLIES_NVM* nvm;
extern int enable_scaling;

int pull_work_req(RT_JOB* w);
extern int drop_job_requests;
int one_done_sw_rt_queue(RT_JOB *work);
int do_bist_ok(bool with_current_measurment);
void push_work_rsp(RT_JOB* work);
extern int assert_serial_failures;


int test_asic(int addr) {
    assert_serial_failures = false;
    int ver = read_reg_device(addr,ADDR_VERSION);
    assert_serial_failures = true;
    //printf("Testing loop, version = %x\n", ver);
    return ((ver & 0xFF) == 0x3c);   
}





void loop_disable_hw(int loop_id) {
    printf("Disabling loop %x...... TODO!!!!\n", loop_id);
}


void try_set_all_asic_freq(uint8_t new_freq) {
    printf("Setting FREQ %d in all ASICs... TODO!\n", new_freq);
}



void try_set_asic_freq(HAMMER* h, uint8_t new_freq, uint32_t*ac_points, uint32_t* dc_points) {
    int ac_points_15_hz = AC_CURRNT_PER_15_HZ[nvm->loop_voltage[h->address/HAMMERS_PER_LOOP]];
    
    if ((new_freq > h->freq) &&
        (h->temperature < BEST_TEMPERATURE_PER_CORNER[nvm->asic_corner[h->address]]) && 
        (h->freq < nvm->top_freq[h->address])) {
         DBG(DBG_SCALING, "Failed to increase frequency on ASIC\n");
    }
    
    DBG(DBG_SCALING, "Set frequency on %x from %d to %d. Points: %d %d\n",
        h->address,h->freq,new_freq,*ac_points,dc_points);
    h->wanted_freq = new_freq;
    h->freq = new_freq;
    //h->last_freq_down_time
    h->last_freq_up_time = time(NULL);
    if (ac_points)
        *ac_points-=20;
    if (dc_points)
        *dc_points-=20;
}




void enable_all_engines_all_asics() {
    int i;
    write_reg_broadcast(ADDR_RESETING0, 0xffffffff);
    write_reg_broadcast(ADDR_RESETING1, 0xffffffff);
    write_reg_broadcast(ADDR_ENABLE_ENGINE, ALL_ENGINES_BITMASK);
    flush_spi_write();
}

    
void enable_engines(int addr, int engines_mask) {
    int i;
    write_reg_device(addr, ADDR_RESETING0, 0xffffffff);
    write_reg_device(addr, ADDR_RESETING1, 0xffffffff);
    write_reg_device(addr, ADDR_ENABLE_ENGINE, engines_mask);
    flush_spi_write();
}


void enable_voltage_freq_and_engines_default() {
      // for each enabled loop
      for (int l = 0; l < LOOP_COUNT; l++) {  
        // Set voltage
        if (!nvm->loop_brocken[l]) {
			int err;
			dc2dc_set_voltage(l, ASIC_VOLTAGE_810, &err);
            passert(err);
        } else {
            loop_disable_hw(l);
        }
      }

      try_set_all_asic_freq(ASIC_FREQ_120);
}


void enable_voltage_freq_and_engines_from_nvm() {
   int l, h, i = 0;
   // for each enabled loop
   for (l = 0; l < LOOP_COUNT; l++) {  
     if (!nvm->loop_brocken[l]) {  
        // Set voltage
		int err;
		dc2dc_set_voltage(l, nvm->loop_voltage[l], &err);
		passert(err);

        // for each ASIC
        for (h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
           HAMMER* a = &miner_box.hammer[l*HAMMERS_PER_LOOP + h];
           // Set freq
           try_set_asic_freq(a, nvm->top_freq[l*HAMMERS_PER_LOOP + h] , NULL, NULL);
         }
     }
   }
}




int test_serial(int loopid) {
    assert_serial_failures = false;
    //printf("Testing loops: %x\n", (~read_spi(ADDR_SQUID_LOOP_BYPASS))& 0xFFFFFF);
    int i = 0;
    int val1;
    int val2;
    // This printfs used in tests! Don`t remove!!
    printf("Testing loop %d:", loopid);
	// For benny - TODO remove
	/*printf("FAKING IT :)\n");
	for (int j = 0 ; j < 1000 ; j++) {
		write_reg_broadcast(ADDR_VERSION, 0xAAAA);
	}*/
	

    while((val1 = read_spi(ADDR_SQUID_STATUS)) & BIT_STATUS_SERIAL_Q_RX_NOT_EMPTY) {
        val2 = read_spi(ADDR_SQUID_SERIAL_READ);
        i++;    
        if (i>700) {
            //parse_squid_status(val1);        
            //printf("ERR (%d) loop is too noisy: ADDR_SQUID_STATUS=%x, ADDR_SQUID_SERIAL_READ=%x\n",i, val1, val2); 
            // This printfs used in tests! Don`t remove!!
            printf("NOISE\n");
            return 0;          
        }
    }
	
	
    //printf("Ok %d!\n", i);


    // test for connectivity
    int ver = read_reg_broadcast(ADDR_VERSION);
    
	
    //printf("XTesting loop, version = %x\n", ver);
    if (BROADCAST_READ_DATA(ver) != 0x3c) {
        //printf("Failed: Got version: %x!\n", ver);                
        printf("BAD DATA\n");
        return 0;
    }
	

    printf("OK\n");
    assert_serial_failures = true;
    return 1;
}


int enable_nvm_loops_ok() {
    write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
    write_spi(ADDR_SQUID_LOOP_RESET, 0);
    write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
    write_spi(ADDR_SQUID_COMMAND, 0xF);
    write_spi(ADDR_SQUID_COMMAND, 0x0);   
    write_spi(ADDR_SQUID_LOOP_BYPASS, ~(nvm->good_loops));
    int ok = test_serial(-1);
	

    if (!ok) {
      return 0;
    }
    
	
    for (int i = 0;i<LOOP_COUNT;i++) {
        printf("loop %d enabled = %d\n",i,!(nvm->loop_brocken[i]));
        miner_box.loop[i].enabled = !(nvm->loop_brocken[i]);
        miner_box.loop[i].id =i;
    }
	
	
    return 1;
}
      



void discover_good_loops_update_nvm() {
    DBG(DBG_NET,"RESET SQUID\n");
    
    uint32_t good_loops = 0;
    int i, ret = 0, success;

    write_spi(ADDR_SQUID_LOOP_BYPASS, 0xFFFFFF);
    write_spi(ADDR_SQUID_LOOP_RESET, 0);
    write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
    write_spi(ADDR_SQUID_COMMAND, 0xF);
    write_spi(ADDR_SQUID_COMMAND, 0x0);   
    write_spi(ADDR_SQUID_LOOP_BYPASS, 0);
    success = test_serial(-1);
    if (success) {
         for (i = 0;i<LOOP_COUNT;i++) {
            //miner_box.loop[i].enabled = true;             
            nvm->loop_brocken[i] = false;            
            ret++;
         }
    } else {
        for (i = 0;i<LOOP_COUNT;i++) {
            //miner_box.loop[i].id =i;
            unsigned int bypass_loops = (~(1 << i) & 0xFFFFFF);
            write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
            write_spi(ADDR_SQUID_LOOP_RESET, 0xffffff);
           // write_spi(ADDR_SQUID_LOOP_RESET, 0);   
            if (test_serial(i)) {
               //printf("--00--\n");
               nvm->loop_brocken[i] = false;
               //miner_box.loop[i].enabled = true;
               good_loops |= 1<<i; 
               ret ++;
            } else {
               //printf("--11--\n");
               nvm->loop_brocken[i] = true; 
               //miner_box.loop[i].enabled = false;
               for (int h = i*HAMMERS_PER_LOOP; h < (i+1)*HAMMERS_PER_LOOP;h++) {
				  //printf("remove ASIC 0x%x\n", h);
				  nvm->working_engines[h] = 0;
				  nvm->top_freq[h] = ASIC_FREQ_0;
				  nvm->asic_corner[h] = ASIC_CORNER_NA;
			   }
			   printf("TODO: DISABLE DC2DC HERE!!!\n");
            }
        }
        nvm->good_loops = good_loops;
        write_spi(ADDR_SQUID_LOOP_BYPASS, ~(nvm->good_loops));
        test_serial(-1);
    }
    nvm->dirty = 1;   
    printf("Found %d good loops\n",ret);
    passert(ret);

}



void test_asics_in_freq(ASIC_FREQ freq_to_pass, ASIC_CORNER corner_to_set) {
    printf(ANSI_COLOR_MAGENTA "TODO TODO testing freq!\n" ANSI_COLOR_RESET);
}

void find_bad_engines_update_nvm() {    
    int i;
     
   if (!do_bist_ok(false)) {
       // IF FAILED BIST - reduce top speed of failed ASIC.
       printf("INIT BIST FAILED, reseting working engines bitmask!\n");
       for (int l = 0; l < LOOP_COUNT; l++) {
           for (int h = 0; h < HAMMERS_PER_LOOP; h++) {
               if (miner_box.hammer[l*HAMMERS_PER_LOOP + h].passed_last_bist_engines != ALL_ENGINES_BITMASK) {
                   // Update NVM
                   nvm->working_engines[l*HAMMERS_PER_LOOP + h] = miner_box.hammer[l*HAMMERS_PER_LOOP + h].passed_last_bist_engines;
               }
           }
       }
   } 
   nvm->dirty = 1;   
}


void recompute_corners_and_voltage_update_nvm() {
  int i;

  printf(ANSI_COLOR_MAGENTA "Checking ASIC speed...!\n" ANSI_COLOR_RESET);
  ASIC_FREQ corner_passage_freq_at_081v[ASIC_CORNER_COUNT] = {ASIC_FREQ_105,ASIC_FREQ_105, ASIC_FREQ_225, ASIC_FREQ_345, ASIC_FREQ_510, ASIC_FREQ_705};
    
	printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
  for (i = ASIC_CORNER_SS; i < ASIC_CORNER_COUNT; i++) {
     test_asics_in_freq(corner_passage_freq_at_081v[i], (ASIC_CORNER)i);
  }

  
  // We can assume all corners updated. Set all frequencies:
  //printf("->>>--%x- %d %s\n",nvm->good_loops,  __LINE__, __FUNCTION__);
  for (int l = 0; l < LOOP_COUNT; l++) {  
  	if (nvm->good_loops & 1<<l) {
		//printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
        int avarage_corner = 0;
        int working_asic_count = 0;
        // Compute corners to ASICs
        for (int h = 0; h < HAMMERS_PER_LOOP; h++, i++) {
			//printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
            if (nvm->working_engines[h]) {
				//printf("->>>---%d %d %s\n", nvm->asic_corner[h],__LINE__, __FUNCTION__);
                avarage_corner+=nvm->asic_corner[h];
                working_asic_count++;
            }
        }
  		//printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
        //  compute avarage corner
        if (working_asic_count) {
        	avarage_corner = avarage_corner/working_asic_count;
        	nvm->loop_voltage[l] = CORNER_TO_VOLTAGE_TABLE[avarage_corner];
        	printf("avarage_corner per loop %d = %d\n", l, avarage_corner);
        } else {
			nvm->loop_voltage[l] = ASIC_VOLTAGE_0;
			printf("CLOSE LOOP???? TODO\n");
        }
  	 } else {
		nvm->loop_voltage[l] = ASIC_VOLTAGE_0;
		printf("CLOSE LOOP???? TODO\n");
  	 }
   }
   //printf("->>>--- %d %s\n", __LINE__, __FUNCTION__);
   nvm->dirty = 1;
}

void create_default_nvm(int from_scratch, int check_loops) {
   memset(nvm, 0, sizeof(SPONDOOLIES_NVM));
   nvm->store_time = time(NULL);
   nvm->nvm_version = NVM_VERSION;
   int l, h, i = 0;

   // Select the testing voltage and set it
   /*
   printf("Setting voltage 0.81V in all loops as default\n");
   for (i = 0; i < LOOP_COUNT; i++) {
	   	   int err;
		   dc2dc_set_voltage(l, nvm->loop_voltage[l], &err);
		   passert(err);
   }
*/
    // See max rate under ASIC_VOLTAGE_810 
	nvm->corners_computed = 0; 
	nvm->bad_engines_found = 0;
	for (i = 0; i < HAMMERS_COUNT; i++) {
		nvm->working_engines[i] = 0xFF;	
		nvm->asic_corner[i] = ASIC_CORNER_SS; // all start ass SS corner
		nvm->top_freq[i] = ASIC_FREQ_150;
	}


   // Find good loops 
   if (check_loops) {
     discover_good_loops_update_nvm();
   }

   
   nvm->dirty = 1;
}


    
    
void init_scaling() {
    // ENABLE ENGINES
    passert (enable_scaling);
}
     


    
void stop_all_work() {
     drop_job_requests = 1;
     // wait to finish real time queue requests
     write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
     write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
     write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
     usleep(100);
     RT_JOB work;
    
     // Move real-time q to complete.
     while(one_done_sw_rt_queue(&work)) {
         push_work_rsp(&work);
     };
    
     // Move pending to complete
     //RT_JOB w;
     while (pull_work_req(&work)) {
         push_work_rsp(&work);
     }
     
     passert(read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY) == 0);
}



void resume_all_work() {
    drop_job_requests = 0;
}


uint32_t ac_current_handler() {
    // TODO
    int spare_ac2dc_current = (AC2DC_POWER_RED_LINE - miner_box.ac2dc_current);
    return spare_ac2dc_current;

}


uint32_t compute_spare_dc_points(int l) {
    // TODO
    uint32_t spare_dc2dc_current = (DC2DC_CURRENT_GREEN_LINE_16S - miner_box.loop[l].dc2dc.dc_current_16s_of_amper);
    return spare_dc2dc_current;
}




void update_asic_freq() {
    int now = time(NULL);
    int h, l;

    
    // first handle critical system AC2DC current
    // 
    DBG(DBG_SCALING,"HERE:!@\n");
    uint32_t ac_points = ac_current_handler();
    if (ac_points < 1) {
        DBG(DBG_SCALING,"update_asic_freq: NO AC2DC POINTS\n");
        return;
    }
    DBG(DBG_SCALING,"SET FREQUENCY!\n");

    for (l = 0; l < LOOP_COUNT; l++) {
        // TODO
        uint32_t dc_points = compute_spare_dc_points(l);
        if (dc_points) {
            for (h = 0; h < HAMMERS_PER_LOOP; h++) {            
                HAMMER* a = & miner_box.hammer[l*HAMMERS_PER_LOOP + h];
                if (a->present) {
                    DBG(DBG_SCALING,"SET FREQUENCY 2!\n");
                    // Critical temperature handling
                    if (a->temperature >= CRITICAL_TEMPERATURE_PER_CORNER[nvm->asic_corner[a->address]]) {
                        DBG(DBG_SCALING,"SET FREQUENCY 3!\n");

                        if (a->freq > SAFE_FREQ_PER_CORNER[nvm->asic_corner[a->address]]) {
                            DBG(DBG_SCALING,"SET FREQUENCY 4!\n");
                            // Bring down to safe frequency per this corner
                            try_set_asic_freq(a, 
                                          SAFE_FREQ_PER_CORNER[nvm->asic_corner[a->address]], 
                                          &ac_points,
                                          &dc_points);
                        } else {
                            DBG(DBG_SCALING,"HOT ASIC COOLING DOWN!\n");
                        }
                    } else if ((a->temperature < BEST_TEMPERATURE_PER_CORNER[nvm->asic_corner[a->address]]) &&
                               (a->freq < nvm->top_freq[a->address])) {
                          DBG(DBG_SCALING,"SET FREQUENCY 6!\n");
                         // try and bring it up?
                          try_set_asic_freq(a, 
                                            nvm->top_freq[a->address], 
                                            &ac_points,
                                            &dc_points);
                         
                    }

                
         
                }
                
                // check if needs some up/down scaling
                // We assume temperature already measured   
            }
        }
    }
}


HAMMER* get_hammer(uint32_t addr) {
    int l, h;
    ASIC_GET_LOOP_OFFSET(addr, l, h);
    HAMMER* a = &miner_box.hammer[l*HAMMERS_PER_LOOP + h];
    return a;

}



// Return 1 if needs urgent scaling

int update_top_current_measurments() {
	int err;
    miner_box.ac2dc_current = ac2dc_get_power(); 
	/*
    for (int i = 0; i < LOOP_COUNT; i++) {
        miner_box.loop[i].dc2dc.dc_current_16s_of_amper = dc2dc_get_current_16s_of_amper(i, &err);
    }
    */
    return 0;
}


int update_temperature_measurments() {
	int err;
    miner_box.ac2dc_temp = ac2dc_get_temperature(); 
	
    for (int i = 0; i < LOOP_COUNT; i++) {
        miner_box.loop[i].dc2dc.dc_temp = dc2dc_get_temp(i, &err);
    }
    
    return 0;
}



bool can_be_throttled(HAMMER* a) {
    return (a->present && (a->freq > SAFE_FREQ_PER_CORNER[nvm->asic_corner[a->address]]));
}



// returns worst asic
//  Any ASIC is worth then NULL
HAMMER* choose_asic_to_disable(HAMMER* a, HAMMER* b) {
    if (!a || !can_be_throttled(a)) return b;
    if (!b || !can_be_throttled(b)) return a;

    if (nvm->asic_corner[a->address] != nvm->asic_corner[b->address]) {
        // Reduce higher corners because they have higher leakage
        return (nvm->asic_corner[a->address] > nvm->asic_corner[b->address])?a:b;
    }
  
    if (a->temperature != b->temperature) {
        // Reduce higher temperature because they have higher leakage
        return (a->temperature > b->temperature)?a:b;
    }

    if (a->freq != b->freq) {
        // Reduce higher frequency
        return (a->freq > b->freq)?a:b;
    }
}


// return hottest ASIC 
HAMMER* find_asic_to_reduce_ac_current() {
    int h, l;
    HAMMER* best = NULL;
    for (l = 0; l < LOOP_COUNT; l++) {
        if (miner_box.loop[l].enabled) {
            for (h = 0; h < HAMMERS_PER_LOOP; h++) {
                HAMMER* a = &miner_box.hammer[l*HAMMERS_PER_LOOP + h];
                best = choose_asic_to_disable(best, a);
                DBG(DBG_SCALING,"CHOOSE: BEST:%d A:%d\n",best->address,a->address);
            }
        }
    }
    passert(can_be_throttled(best));
    return best;
}


HAMMER* find_asic_to_reduce_dc_current(int l) {
    int h;
    // Find hottest ASIC at highest corner.
    HAMMER* best = NULL;
    for (h = 0; h < HAMMERS_PER_LOOP; h++) {
      HAMMER* a = &miner_box.hammer[l*HAMMERS_PER_LOOP + h];
      best = choose_asic_to_disable(best, a);
    }
    passert(can_be_throttled(best));
    return best;
}



// Put all LOOPs below critical.
// No BIST since 
void solve_current_problems() {
  // measure current
  // do_bist_ok(1);
  int l;

#if 1
  for (l = 0; l < LOOP_COUNT; l++) {
     while (miner_box.loop[l].dc2dc.dc_current_16s_of_amper >= DC2DC_CURRENT_RED_LINE_16S) {
       HAMMER* a = find_asic_to_reduce_dc_current(l);
       DBG(DBG_SCALING,"DC2DC OVER LIMIT, killing ASIC:%d!\n", a->address);
       try_set_asic_freq(a, SAFE_FREQ_PER_CORNER[nvm->asic_corner[a->address]], 
            &miner_box.ac2dc_current, &miner_box.loop[l].dc2dc.dc_current_16s_of_amper);
     }
  }

 
  while (miner_box.ac2dc_current >= AC2DC_POWER_RED_LINE) {
  	   printf("miner_box.ac2dc_current = %d\n", miner_box.ac2dc_current);
       HAMMER* a = find_asic_to_reduce_ac_current();
       DBG(DBG_SCALING,"AC2DC OVER LIMIT, killing ASIC:%d %d!\n", a->address, a->freq);
       try_set_asic_freq(a, SAFE_FREQ_PER_CORNER[nvm->asic_corner[a->address]], 
            &miner_box.ac2dc_current, &miner_box.loop[HAMMER_TO_LOOP(a)].dc2dc.dc_current_16s_of_amper);
  }
#endif
}




// 
void set_optimal_voltage() {
    // Load from NVM?
    
}



// Stop handling requests
// Called from the main HW handling thread every 1 second
void periodic_scaling_task() {

	if (!enable_scaling) {
		return;
	}

    struct timeval tv;
    struct timeval now;
    static struct timeval last_scaling = {0};    

    gettimeofday(&tv, NULL);	
    unsigned int usec=(tv.tv_sec-last_scaling.tv_sec)*1000000;
    usec+=(tv.tv_usec-last_scaling.tv_usec);
    bool critical_current = false;

    if (miner_box.ac2dc_current >= AC2DC_POWER_RED_LINE) {
        critical_current = true;
    }
    
    for (int i = 0; i < LOOP_COUNT; i++) {          
        if (miner_box.loop[i].dc2dc.dc_current_16s_of_amper >= DC2DC_CURRENT_RED_LINE_16S) {
		   critical_current = true; 
        }
    }
    
    if (critical_current) {
        // Only brings rate down
        solve_current_problems();
    }


    // Every X seconds.
    if ((usec >= 5*1000*1000)) {
        stop_all_work();
#ifdef DBG_SCALING
        print_scaling();
#endif
    
        while(1) {
            update_asic_freq();

            // We always assume BIST fails because ASIC too fast.
            for (int l = 0; l < LOOP_COUNT; l++) {
               for (int h = 0; h < HAMMERS_PER_LOOP; h++) {
                   miner_box.hammer[l*HAMMERS_PER_LOOP + h].failed_bists = 0;
               }
            }

            
            if (!do_bist_ok(false)) {
                // IF FAILED BIST - reduce top speed of failed ASIC.
                printf("BIST FAILED!\n");
                for (int l = 0; l < LOOP_COUNT; l++) {
                    for (int h = 0; h < HAMMERS_PER_LOOP; h++) {
                        if (miner_box.hammer[l*HAMMERS_PER_LOOP + h].failed_bists) {
                            // Update NVM
                            nvm->top_freq[l*HAMMERS_PER_LOOP + h]--;
                            nvm->dirty = 1;
                        }
                    }
                }
            } else {
                break;
            }
        }


        
        // measure how long it took
        gettimeofday(&last_scaling, NULL);  
        usec=(last_scaling.tv_sec-tv.tv_sec)*1000000;
        usec+=(last_scaling.tv_usec-tv.tv_usec);
        DBG(DBG_SCALING, "SCALING TOOK: %d usecs\n",usec);
        
        resume_all_work();
    }
}

// Callibration SW




void print_asic(HAMMER* h) {
    if (h->present) {
        DBG(DBG_SCALING,"      HAMMER %04x C:%d ENG:0x%x:0x%x Hz:%d MaxHz:%d WINS:%04d T:%x\n",
            h->address,
            nvm->asic_corner[h->address],
            h->working_engines_mask, 
            h->enabled_engines_mask, 
            h->freq, 
            nvm->top_freq[h->address],
            h->solved_jobs,
            h->temperature);
    } else {
        DBG(DBG_SCALING,"      HAMMER %04x ----\n", h->address);
    }
}


void print_loop(int l) {
    LOOP* loop = & miner_box.loop[l];
    DBG(DBG_SCALING,"   LOOP %x AMP:%d\n" , l, loop->dc2dc.dc_current_16s_of_amper);

}


void print_miner_box() {
    DBG(DBG_SCALING,"MINER AC:%x\n", miner_box.ac2dc_current);
}



void print_scaling() {
    print_miner_box();
    int h, l;
    for (l = 0; l < LOOP_COUNT; l++) {
        // TODO
        if (miner_box.loop[l].enabled) {
            print_loop(l);
            for (h = 0; h < HAMMERS_PER_LOOP; h++) {
                HAMMER* a = &miner_box.hammer[l*HAMMERS_PER_LOOP + h];
                print_asic(a);
            }
        } else {
            DBG(DBG_SCALING,"    LOOP %x ----\n", l);
        }
        
    }
}


