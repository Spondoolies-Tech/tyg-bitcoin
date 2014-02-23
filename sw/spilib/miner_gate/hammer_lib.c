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
#include "mg_proto_parser.h"
#include "hammer.h"
#include "queue.h"
#include <string.h>	
#include "ac2dc.h"
#include "hammer_lib.h"
#include <pthread.h>
#include <spond_debug.h>	
#include <sys/time.h>
#include "real_time_queue.h"
#include "miner_gate.h"
#include "scaling_manager.h"


extern pthread_mutex_t network_hw_mutex;

int total_devices = 0;
int engines_per_device = 0;
extern int enable_reg_debug;
int cur_leading_zeroes = 0;

RT_JOB* add_to_sw_rt_queue(const RT_JOB *work);
void reset_sw_rt_queue();
RT_JOB* peak_rt_queue(uint8_t hw_id);

extern int rt_queue_size;
extern int rt_queue_sw_write;
extern int rt_queue_hw_done;

void dump_zabbix_stats();



void hammer_iter_init(hammer_iter* e) {
	e->addr = -1;
	e->done = 0;
	e->h    = -1;
	e->l    = 0;
}



int hammer_iter_next_present(hammer_iter* e) {
	int found = 0;
	while (!found) {
		e->h++;
		
		if (e->h == HAMMERS_PER_LOOP) {
			e->l++;
			e->h = 0;
			while (!vm.loop[e->l].enabled_loop) {
				e->l++;
				if (e->l == LOOP_COUNT) {
					e->done = 1;
					return 0;
				}
			}
		}
	
		if (e->l == LOOP_COUNT) {
			e->done = 1;
			return 0;
		}
		
		if (vm.hammer[e->l*HAMMERS_PER_LOOP + e->h].asic_present) {
			found++;
		}

	}
	e->addr = e->l*HAMMERS_PER_LOOP + e->h;
	return 1;
				
}



void loop_iter_init(loop_iter* e) {
	e->done =  0;
	e->l    = -1;
}

int loop_iter_next_enabled(loop_iter* e) {
	int found = 0;
	
	while (!found) {
		e->l++;
		
		while (!vm.loop[e->l].enabled_loop) {
			e->l++;
		}

		if (e->l == LOOP_COUNT) {
			e->done = 1;
			return 0;
		}
	}
	return 1;
				
}





void stop_all_work() {
	 printf("------ Stopping work for scaling!\n");
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
	 printf("------ Starting work again \n");
}




void print_devreg(int reg, const char* name) {
	printf("%2x:  BR:%8x ", reg, read_reg_broadcast(reg));    
	int h, l;


	hammer_iter hi;
	hammer_iter_init(&hi);

    while (hammer_iter_next_present(&hi)) {
        		printf("DEV-%04x:%8x ", 
                    vm.hammer[hi.addr].address,
                    read_reg_device(hi.addr, reg));
	}
	printf("  %s\n", name);
}


void parse_int_register(const char* label, int reg) {
	printf("%s :%x: ", label, reg);
	if (reg & BIT_INTR_NO_ADDR) printf("NO_ADDR ");
	if (reg & BIT_INTR_WIN) printf("WIN ");
	if (reg & BIT_INTR_FIFO_FULL) printf("FIFO_FULL ");
	if (reg & BIT_INTR_FIFO_ERR) printf("FIFO_ERR ");
	if (reg & BIT_INTR_CONDUCTOR_IDLE) printf("CONDUCTOR_IDLE ");
	if (reg & BIT_INTR_CONDUCTOR_BUSY) printf("CONDUCTOR_BUSY ");
	if (reg & BIT_INTR_RESERVED1) printf("RESERVED1 ");
	if (reg & BIT_INTR_FIFO_EMPTY) printf("FIFO_EMPTY ");
	if (reg & BIT_INTR_BIST_FAIL) printf("BIST_FAIL ");
	if (reg & BIT_INTR_THERMAL_SHUTDOWN) printf("THERMAL_SHUTDOWN ");
	if (reg & BIT_INTR_NOT_WIN) printf("NOT_WIN ");
	if (reg & BIT_INTR_0_OVER) printf("0_OVER ");
	if (reg & BIT_INTR_0_UNDER) printf("0_UNDER ");
	if (reg & BIT_INTR_1_OVER) printf("1_OVER ");
	if (reg & BIT_INTR_1_UNDER) printf("1_UNDER ");
	if (reg & BIT_INTR_RESERVED2) printf("RESERVED2 ");
	printf("\n");
}


void print_state() {
	uint32_t i;
	static int print = 0;
	print++;
	enable_reg_debug = 0;
	dprintf("**************** DEBUG PRINT %x ************\n", print);
	print_devreg(ADDR_CURRENT_NONCE, "CURRENT NONCE");
	print_devreg(ADDR_BR_FIFO_FULL, "FIFO FULL");
	print_devreg(ADDR_BR_FIFO_EMPTY, "FIFO EMPTY");
	print_devreg(ADDR_BR_CONDUCTOR_BUSY, "BUSY");
	print_devreg(ADDR_BR_CONDUCTOR_IDLE, "IDLE");
	dprintf("*****************************\n", print);


print_devreg(ADDR_CHIP_ADDR, "ADDR_CHIP_ADDR");
print_devreg(ADDR_GOT_ADDR , "ADDR_GOT_ADDR ");
print_devreg(ADDR_CONTROL , "ADDR_CONTROL ");
print_devreg(ADDR_COMMAND , "ADDR_COMMAND ");
print_devreg(ADDR_RESETING0, "ADDR_RESETING0");
print_devreg(ADDR_RESETING1, "ADDR_RESETING1");
print_devreg(ADDR_CONTROL_SET0, "ADDR_CONTROL_SET0");
print_devreg(ADDR_CONTROL_SET1, "ADDR_CONTROL_SET1");
print_devreg(ADDR_SW_OVERRIDE_PLL, "ADDR_SW_OVERRIDE_PLL");
print_devreg(ADDR_PLL_RSTN , "ADDR_PLL_RSTN");
print_devreg(ADDR_INTR_MASK , "ADDR_INTR_MASK");
print_devreg(ADDR_INTR_CLEAR , "ADDR_INTR_CLEAR ");
print_devreg(ADDR_INTR_RAW , "ADDR_INTR_RAW ");
print_devreg(ADDR_INTR_SOURCE , "ADDR_INTR_SOURCE ");
print_devreg(ADDR_BR_NO_ADDR , "ADDR_BR_NO_ADDR ");
print_devreg(ADDR_BR_WIN , "ADDR_BR_WIN ");
print_devreg(ADDR_BR_FIFO_FULL , "ADDR_BR_FIFO_FULL ");
print_devreg(ADDR_BR_FIFO_ERROR , "ADDR_BR_FIFO_ERROR ");
print_devreg(ADDR_BR_CONDUCTOR_IDLE , "ADDR_BR_CONDUCTOR_IDLE ");
print_devreg(ADDR_BR_CONDUCTOR_BUSY , "ADDR_BR_CONDUCTOR_BUSY ");
print_devreg(ADDR_BR_CRC_ERROR , "ADDR_BR_CRC_ERROR ");
print_devreg(ADDR_BR_FIFO_EMPTY , "ADDR_BR_FIFO_EMPTY ");
print_devreg(ADDR_BR_BIST_FAIL , "ADDR_BR_BIST_FAIL ");
print_devreg(ADDR_BR_THERMAL_VIOLTION , "ADDR_BR_THERMAL_VIOLTION ");
print_devreg(ADDR_BR_NOT_WIN , "ADDR_BR_NOT_WIN ");
print_devreg(ADDR_BR_0_OVER , "ADDR_BR_0_OVER ");
print_devreg(ADDR_BR_0_UNDER , "ADDR_BR_0_UNDER ");
print_devreg(ADDR_BR_1_OVER , "ADDR_BR_1_OVER ");
print_devreg(ADDR_BR_1_UNDER , "ADDR_BR_1_UNDER ");

print_devreg(ADDR_MID_STATE , "ADDR_MID_STATE ");
print_devreg(ADDR_MERKLE_ROOT , "ADDR_MERKLE_ROOT ");
print_devreg(ADDR_TIMESTEMP , "ADDR_TIMESTEMP ");
print_devreg(ADDR_DIFFICULTY , "ADDR_DIFFICULTY ");
print_devreg(ADDR_WIN_LEADING_0 , "ADDR_WIN_LEADING_0 ");
print_devreg(ADDR_JOB_ID , "ADDR_JOB_ID");


print_devreg(ADDR_WINNER_JOBID_WINNER_ENGINE , "ADDR_WINNER_JOBID_WINNER_ENGINE ");
print_devreg(ADDR_WINNER_NONCE, "ADDR_WINNER_NONCE");
print_devreg(ADDR_CURRENT_NONCE, ANSI_COLOR_CYAN "ADDR_CURRENT_NONCE" ANSI_COLOR_RESET);
print_devreg(ADDR_LEADER_ENGINE, "ADDR_LEADER_ENGINE");
print_devreg(ADDR_VERSION, "ADDR_VERSION");
print_devreg(ADDR_ENGINES_PER_CHIP_BITS, "ADDR_ENGINES_PER_CHIP_BITS");
print_devreg(ADDR_LOOP_LOG2, "ADDR_LOOP_LOG2");
print_devreg(ADDR_ENABLE_ENGINE, "ADDR_ENABLE_ENGINE");
print_devreg(ADDR_FIFO_OUT, "ADDR_FIFO_OUT");
print_devreg(ADDR_CURRENT_NONCE_START, "ADDR_CURRENT_NONCE_START");
print_devreg(ADDR_CURRENT_NONCE_START+1, "ADDR_CURRENT_NONCE_START+1");
print_devreg(ADDR_CURRENT_NONCE_START+1, "ADDR_CURRENT_NONCE_START+2");

print_devreg(ADDR_CURRENT_NONCE_RANGE, "ADDR_CURRENT_NONCE_RANGE");
print_devreg(ADDR_FIFO_OUT_MID_STATE_LSB, "ADDR_FIFO_OUT_MID_STATE_LSB");
print_devreg(ADDR_FIFO_OUT_MID_STATE_MSB, "ADDR_FIFO_OUT_MID_STATE_MSB");
print_devreg(ADDR_FIFO_OUT_MARKEL, "ADDR_FIFO_OUT_MARKEL");
print_devreg(ADDR_FIFO_OUT_TIMESTEMP, "ADDR_FIFO_OUT_TIMESTEMP");
print_devreg(ADDR_FIFO_OUT_DIFFICULTY, "ADDR_FIFO_OUT_DIFFICULTY");
print_devreg(ADDR_FIFO_OUT_WIN_LEADING_0, "ADDR_FIFO_OUT_WIN_LEADING_0");
print_devreg(ADDR_FIFO_OUT_WIN_JOB_ID, "ADDR_FIFO_OUT_WIN_JOB_ID");
print_devreg(ADDR_BIST_NONCE_START, "ADDR_BIST_NONCE_START");
print_devreg(ADDR_BIST_NONCE_RANGE, "ADDR_BIST_NONCE_RANGE");
print_devreg(ADDR_BIST_NONCE_EXPECTED, "ADDR_BIST_NONCE_EXPECTED");
print_devreg(ADDR_EMULATE_BIST_ERROR, "ADDR_EMULATE_BIST_ERROR");
print_devreg(ADDR_BIST_PASS, "ADDR_BIST_PASS");
parse_int_register("ADDR_INTR_RAW", read_reg_broadcast(ADDR_INTR_RAW));
parse_int_register("ADDR_INTR_MASK", read_reg_broadcast(ADDR_INTR_MASK));
parse_int_register("ADDR_INTR_SOURCE", read_reg_broadcast(ADDR_INTR_SOURCE));


	//print_engines();
	enable_reg_debug = 0;
}




// Queues work to actual HW
// returns 1 if found nonce
void push_to_hw_queue(RT_JOB *work) {
	// ASSAF - JOBPUSH
	int old_dbg;
	old_dbg = enable_reg_debug;
	enable_reg_debug = 0;
	static int j = 0;
	int i; 
	//printf("Push!\n");

	for (i = 0; i < 8; i++) {
		write_reg_broadcast((ADDR_MID_STATE + i), work->midstate[i]);
	}
	write_reg_broadcast(ADDR_MERKLE_ROOT, work->mrkle_root);
	write_reg_broadcast(ADDR_TIMESTEMP, work->timestamp);
	write_reg_broadcast(ADDR_DIFFICULTY, work->difficulty);
	/*
	if (current_leading == 0) {
		push_write_reg_broadcast(ADDR_WIN_LEADING_0, work->leading_zeroes);
		current_leading = work->leading_zeroes;
	}
	*/
	passert(work->work_id_in_hw < 0x100);
	write_reg_broadcast(ADDR_JOB_ID, work->work_id_in_hw);
	write_reg_broadcast(ADDR_COMMAND, BIT_CMD_LOAD_JOB);
    flush_spi_write();
	enable_reg_debug = old_dbg;
}






// destribute range between 0 and max_range
void set_nonce_range_in_engines(unsigned int max_range) {
	int d, e;
	unsigned int current_nonce = 0;
	//enable_reg_debug = 0;
	unsigned int engine_size;
	engines_per_device = read_reg_broadcast(ADDR_ENGINES_PER_CHIP_BITS) & 0xFF;
	printf("%d engines found\n", engines_per_device);
	int old_dbg = enable_reg_debug;
	//enable_reg_debug = 0;
	printf("engines %x\n",engines_per_device);
	int total_engines = engines_per_device * total_devices;
	engine_size = ((max_range) / (total_engines));

	//for (d = FIRST_CHIP_ADDR; d < FIRST_CHIP_ADDR+total_devices; d++) {

    //int h, l;
	hammer_iter hi;
	hammer_iter_init(&hi);

    while (hammer_iter_next_present(&hi)) {
        write_reg_device(hi.addr, ADDR_CURRENT_NONCE_RANGE, engine_size);
		for (e = 0; e < engines_per_device; e++) {
		    DBG(DBG_HW,"engine %x:%x got range from 0x%x to 0x%x\n", 
                hi.addr, e, current_nonce, current_nonce + engine_size);
		    write_reg_device(hi.addr, ADDR_CURRENT_NONCE_START + e, current_nonce);
		    //read_reg_device(d, ADDR_CURRENT_NONCE_START + e);			
	        current_nonce += engine_size;
		}
	}
	//enable_reg_debug = 0;
	flush_spi_write();
	//print_state();
}






int allocate_addresses_to_devices() {
    int reg;
    int l;

    DBG(DBG_HW,"Unallocate all chip addresses\n");
	write_reg_broadcast(ADDR_GOT_ADDR, 0);
    
    //  validate address reset
    reg = read_reg_broadcast(ADDR_BR_NO_ADDR);
	//printf("1\n");
    if (reg == 0) {
        passert(0);
    }
	//printf("2\n");


    // Do it loop by loop!
    for (l = 0; l < LOOP_COUNT ; l++) {
        if (vm.loop[l].enabled_loop) {
			printf("4\n");
            // Disable all other loops
            unsigned int bypass_loops = (~(1 << l) & 0xFFFFFF);
            printf("Giving address on loop (mask): %x\n", (~bypass_loops) & 0xFFFFFF);
            write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
            
            // Give 8 addresses
            int h = 0;
            int asics_in_loop = 0;
            for (h = 0; h < HAMMERS_PER_LOOP ; h++){
                uint16_t addr = (l*HAMMERS_PER_LOOP+h);
                vm.hammer[l*HAMMERS_PER_LOOP + h].address = addr;
				printf(ANSI_COLOR_MAGENTA "TODO TODO Find bad engines in ASICs!\n" ANSI_COLOR_RESET);
                if (read_reg_broadcast(ADDR_BR_NO_ADDR)) {
                     write_reg_broadcast(ADDR_CHIP_ADDR, addr);
                     total_devices++;
                     asics_in_loop++;
                     vm.hammer[l*HAMMERS_PER_LOOP + h].asic_present = 1;
                     DBG(DBG_HW,"Address allocated: %x\n", addr);
//                   for (total_devices = 0; read_reg_broadcast(ADDR_BR_NO_ADDR); ++total_devices) {
//		             write_reg_broadcast(ADDR_CHIP_ADDR, total_devices + FIRST_CHIP_ADDR);            		
            	} else {
//                   printf("No ASIC found at position %x!\n", addr);
                     vm.hammer[l*HAMMERS_PER_LOOP + h].address = addr; 
                     vm.hammer[l*HAMMERS_PER_LOOP + h].asic_present = 0;
					 nvm.working_engines[l*HAMMERS_PER_LOOP + h] = 0;
					 nvm.top_freq[l*HAMMERS_PER_LOOP + h] = ASIC_FREQ_0;
					 nvm.asic_corner[l*HAMMERS_PER_LOOP + h] = ASIC_CORNER_NA;
                }
            } 
            // Dont remove this print - used by scripts!!!!
            printf("ASICS in loop %d: %d\n",l,asics_in_loop);
            passert(read_reg_broadcast(ADDR_BR_NO_ADDR) == 0);
            write_spi(ADDR_SQUID_LOOP_BYPASS, ~(nvm.good_loops));
        } else {
            printf("ASICS in loop %d: %d\n",l,0);;
        }
		
    }
	//printf("3\n");

    // Validate all got address
    passert(read_reg_broadcast(ADDR_BR_NO_ADDR) == 0);
    DBG(DBG_HW,"Number of ASICs found: %x\n", total_devices);
	return total_devices;    
    
	
	//set_bits_offset(i);
}


 


void memprint(const void *m, size_t n) {
	int i;
	for (i = 0 ; i < n; i++) {
		printf("%02x", ((const unsigned char*)m)[i]);
	}
}


void push_hammer_read(uint32_t addr, uint32_t offset, uint32_t* p_value);

int SWAP32(int x);
void compute_hash(const unsigned char * midstate, unsigned int mrkle_root,
        unsigned int timestamp, unsigned int difficulty,
        unsigned int winner_nonce, unsigned char * hash);
int get_leading_zeroes(const unsigned char *hash);


int get_print_win(int winner_device) {
	// TODO - find out who one!
	//int winner_device = winner_reg >> 16;
	//enable_reg_debug = 1;
	uint32_t winner_nonce;// = read_reg_device(winner_device, ADDR_WINNER_NONCE);
	uint32_t winner_id;// = read_reg_device(winner_device, ADDR_WINNER_JOBID);
	int engine_id;
	push_hammer_read(winner_device, ADDR_WINNER_NONCE, &winner_nonce);
	//push_read_reg_device(winner_device, ADDR_WINNER_JOBID, &winner_id);
	push_hammer_read(winner_device, ADDR_WINNER_JOBID_WINNER_ENGINE, &winner_id);
	write_reg_device(winner_device, ADDR_INTR_CLEAR, BIT_INTR_WIN);
  //  int current = read_reg_broadcast(ADDR_CURRENT_NONCE);
	squid_wait_hammer_reads();
    // Winner ID holds ID and Engine info
    engine_id = winner_id >> 8;
	winner_id = winner_id & 0xFF;
	RT_JOB *work_in_hw = peak_rt_queue(winner_id);

	
	//printf("----%x %x\n", winner_device ,read_reg_device(winner_device, ADDR_WINNER_NONCE));
	// test that this is the "work_in_hw" win.
	if (work_in_hw->work_state == WORK_STATE_HAS_JOB) {
		printf("!!!!!!!  WIN !!!!!!!! %x %x\n", winner_nonce, work_in_hw->work_id_in_sw);		
    	work_in_hw->winner_nonce = winner_nonce;
        // Optional printing
        if (work_in_hw->winner_nonce != 0) {
            if (DBG_WINS) {
               DBG(DBG_WINS,"------ FOUND WIN:----\n");
               DBG(DBG_WINS,"- midstate:\n");         
               int i;
               for (i = 0; i < 8; i++) {
                   DBG(DBG_WINS,"-   %08x\n", work_in_hw->midstate[i]); 
               }
               DBG(DBG_WINS,"- mrkle_root: %08x\n", work_in_hw->mrkle_root);
               DBG(DBG_WINS,"- timestamp : %08x\n", work_in_hw->timestamp);
               DBG(DBG_WINS,"- difficulty: %08x\n", work_in_hw->difficulty);
               DBG(DBG_WINS,"--- NONCE = %08x \n- ", work_in_hw->winner_nonce);    
               static unsigned char hash[32];
               memset(hash,0, 32);
               compute_hash((const unsigned char*)work_in_hw->midstate,
                   SWAP32(work_in_hw->mrkle_root),
                   SWAP32(work_in_hw->timestamp),
                   SWAP32(work_in_hw->difficulty),
                   SWAP32(work_in_hw->winner_nonce),
                   //0x7c2bac1d,
                   hash);
               memprint((void*)hash, 32);
               int leading_zeroes = get_leading_zeroes(hash);
               DBG(DBG_WINS,"\n- Leading Zeroes: %d\n", leading_zeroes);
               DBG(DBG_WINS,"-------------------\n");                
            } else {
                printf("Win 0x%x\n", winner_id);
            }
        }
		return 1;
	} else {
        printf(ANSI_COLOR_RED "------------------  -------- !!!!!  Warning !!!!: Win orphan job 0x%x, nonce=0x%x!!!\n" ANSI_COLOR_RESET, winner_id, winner_nonce);
		psyslog("!!!!!  Warning !!!!: Win orphan job_id 0x%x!!!\n", 
			winner_id);
		assert(0);
		return 0;
	}
	return 1;
}




extern int spi_ioctls_read;
extern int spi_ioctls_write;
/*
void fill_random_work(RT_JOB *work) {
	static int id = 1;
	id++;
	memset(work, 0, sizeof(RT_JOB));
	work->midstate[0] = (rand()<<16) + rand();
	work->midstate[1] = (rand()<<16) + rand();
	work->midstate[2] = (rand()<<16) + rand();
	work->midstate[3] = (rand()<<16) + rand();
	work->midstate[4] = (rand()<<16) + rand();
	work->midstate[5] = (rand()<<16) + rand();
	work->midstate[6] = (rand()<<16) + rand();
	work->midstate[7] = (rand()<<16) + rand();
	work->work_id_in_sw = id;
	work->mrkle_root = (rand()<<16) + rand();
	work->timestamp  = (rand()<<16) + rand();
	work->difficulty = (rand()<<16) + rand();
	//work_in_queue->winner_nonce
}*/

void init_scaling();
int update_top_current_measurments();
int update_i2c_temperature_measurments();

int init_hammers() {
	int i;
	//enable_reg_debug = 1;
	printf("VERSION SW:%x\n", 1);
	enable_reg_debug = 0;
	// Clearing all interrupts
	write_reg_broadcast(ADDR_SW_OVERRIDE_PLL, 0x1);
	write_reg_broadcast(ADDR_PLL_RSTN, 0x1);
	write_reg_broadcast(ADDR_INTR_MASK, 0x0);
	write_reg_broadcast(ADDR_INTR_CLEAR, 0xffff);
	write_reg_broadcast(ADDR_INTR_CLEAR, 0x0);
	write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
	write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
	flush_spi_write();
	//print_state();  
	return 0;
}


// returns 1 on success
int do_bist_ok(bool with_current_measurment) {
	printf("Doing bist!\n");
    static int bist_id = 0;
    bist_id = 1-bist_id; // 0,1,0,1,0,1...

	int old_dbg;
	int poll_counter=0;
	old_dbg = enable_reg_debug;
	enable_reg_debug = 0;
	// Enter BIST mode
	write_reg_broadcast(ADDR_CONTROL_SET1, BIT_CTRL_BIST_MODE);
	poll_counter=0;

	// Give BIST job
	if (bist_id == 0) {
		write_reg_broadcast(ADDR_BIST_NONCE_START, 0x1DAC0000);
		write_reg_broadcast(ADDR_BIST_NONCE_RANGE, 0x00010000);
		write_reg_broadcast(ADDR_BIST_NONCE_EXPECTED, 0x1DAC2B7C); // 0x1DAC2B7C
		write_reg_broadcast(ADDR_MID_STATE+0, 0xBC909A33);
		write_reg_broadcast(ADDR_MID_STATE+1, 0x6358BFF0);
		write_reg_broadcast(ADDR_MID_STATE+2, 0x90CCAC7D);
		write_reg_broadcast(ADDR_MID_STATE+3, 0x1E59CAA8);
		write_reg_broadcast(ADDR_MID_STATE+4, 0xC3C8D8E9);
		write_reg_broadcast(ADDR_MID_STATE+5, 0x4F0103C8);
		write_reg_broadcast(ADDR_MID_STATE+6, 0x96B18736);
		write_reg_broadcast(ADDR_MID_STATE+7, 0x4719F91B);
		write_reg_broadcast(ADDR_MERKLE_ROOT, 0x4B1E5E4A);
		write_reg_broadcast(ADDR_TIMESTEMP, 0x29AB5F49);
		write_reg_broadcast(ADDR_DIFFICULTY, 0xFFFF001D);
		write_reg_broadcast(ADDR_WIN_LEADING_0, 0x26);
		write_reg_broadcast(ADDR_JOB_ID, 0xEE);
		write_reg_broadcast(ADDR_COMMAND, BIT_CMD_LOAD_JOB);
		flush_spi_write();
	} else if (bist_id == 1) {
		write_reg_broadcast(ADDR_BIST_NONCE_START, 0x227D0000);
		write_reg_broadcast(ADDR_BIST_NONCE_RANGE, 0x00010000);
		write_reg_broadcast(ADDR_BIST_NONCE_EXPECTED, 0x227D01DA /*0x227D01DA*/);

		write_reg_broadcast(ADDR_MID_STATE+0, 0xD83B1B22);
		write_reg_broadcast(ADDR_MID_STATE+1, 0xD5CEFEB4);
		write_reg_broadcast(ADDR_MID_STATE+2, 0x17C68B5C);
		write_reg_broadcast(ADDR_MID_STATE+3, 0x4EB2B911);
		write_reg_broadcast(ADDR_MID_STATE+4, 0x3C3D668F);
		write_reg_broadcast(ADDR_MID_STATE+5, 0x5CCA92CA);
		write_reg_broadcast(ADDR_MID_STATE+6, 0x80E63EEC);
		write_reg_broadcast(ADDR_MID_STATE+7, 0x77415443);
		write_reg_broadcast(ADDR_MERKLE_ROOT, 0x31863FE8);
		write_reg_broadcast(ADDR_TIMESTEMP, 0x68538051);
		write_reg_broadcast(ADDR_DIFFICULTY, 0x3DAA011A);
		write_reg_broadcast(ADDR_WIN_LEADING_0, 0x20);
		write_reg_broadcast(ADDR_JOB_ID, 0xEE);
		write_reg_broadcast(ADDR_COMMAND, BIT_CMD_LOAD_JOB);
		flush_spi_write();
	} else {
		passert(0);
	}

    // TODO - update this timing to work.
    if (with_current_measurment) {
        update_top_current_measurments();
    }

	// POLL RESULT
	enable_reg_debug = 0;
	poll_counter=0;
	//usleep(10);
	int res;
    int i = 0;
	while((res = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY))) {
        i++;
        //printf("RES = %x\n",res );
        //res = read_reg_broadcast(ADDR_CURRENT_NONCE);
        //printf("RES = %x\n",res );
        usleep(1);
        if (i%3000 == 0) {
            printf("Waiting on bist ADDR_BR_CONDUCTOR_BUSY %x\n", res);
            print_state();
        }
	}

	enable_reg_debug = 0;

	// Exit BIST
	int bist_fail;
    int failed = 0;
    while (bist_fail = read_reg_broadcast(ADDR_BR_BIST_FAIL)) {
        uint16_t failed_addr = BROADCAST_READ_ADDR(bist_fail);
        HAMMER *h = ASIC_GET_BY_ADDR(failed_addr);
        h->failed_bists++;
        h->passed_last_bist_engines = read_reg_device(failed_addr, ADDR_BIST_PASS);
        //printf("Writing %x to %x because read %d\n", BIT_INTR_BIST_FAIL, failed_addr, bist_fail);
        write_reg_device(failed_addr, ADDR_INTR_CLEAR, BIT_INTR_BIST_FAIL);
    	write_reg_device(failed_addr, ADDR_INTR_CLEAR, BIT_INTR_WIN);        
    	write_reg_device(failed_addr, ADDR_CONTROL_SET0, BIT_CTRL_BIST_MODE);         
        //print_devreg(ADDR_INTR_RAW , "ADDR_INTR_RAW ");
        
        failed = 1;
    }

    write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
	write_reg_broadcast(ADDR_INTR_CLEAR, BIT_INTR_BIST_FAIL);
	write_reg_broadcast(ADDR_INTR_CLEAR, BIT_INTR_WIN);
	write_reg_broadcast(ADDR_CONTROL_SET0, BIT_CTRL_BIST_MODE);
    write_reg_broadcast(ADDR_WIN_LEADING_0, cur_leading_zeroes);
	flush_spi_write();
	enable_reg_debug = old_dbg;
	return !failed;
}








int pull_work_req(RT_JOB* w);
void print_adapter();
int has_work_req() ;


void read_some_asic_temperatures() {
    static ASIC_TEMP temp_measure_temp = ASIC_TEMP_83;
    static int temp_loop = 0;
    uint32_t val[HAMMERS_PER_LOOP]; 
    //printf("T:%d L:%d\n",temp_measure_temp, temp_loop);
    if (vm.loop[temp_loop].enabled_loop) {
       // Push temperature measurment to the whole loop
       //read_reg_device(uint16_t cpu,uint8_t offset)
       write_reg_broadcast(ADDR_TS_SET_0, temp_measure_temp);                    
       write_reg_broadcast(ADDR_COMMAND, BIT_CMD_TS_RESET_0);                    
       for (int h = 0; h < HAMMERS_PER_LOOP; h++) {
           HAMMER *a = &vm.hammer[temp_loop*HAMMERS_PER_LOOP + h];
           val[h] = 0;
           if (a->asic_present) {
               push_hammer_read(a->address,ADDR_TS_DATA_0, &(val[h]));
           }
       }
    }
	squid_wait_hammer_reads();
    if (vm.loop[temp_loop].enabled_loop) {
       for (int h = 0; h < HAMMERS_PER_LOOP; h++) {
           HAMMER *a = &vm.hammer[temp_loop*HAMMERS_PER_LOOP + h];  
           if (a->asic_present) {
               if (val[h]) { // This ASIC is at least this temperature high
                   if (a->temperature < temp_measure_temp) {
                       a->temperature = (ASIC_TEMP)(temp_measure_temp + 1);
                   }
               } else { // ASIC is colder then this temp!
                   if (a->temperature >= temp_measure_temp) {
                       a->temperature = temp_measure_temp;
                   }
               }
               DBG(DBG_SCALING,"UPDATED a[%x] to %d\n",a->address, a->temperature);
           }
       }
    }


    // Progress loop and (maybe) temperature
    temp_loop = (temp_loop+1)%LOOP_COUNT;
    if (temp_loop == 0) {
         temp_measure_temp = (ASIC_TEMP)((temp_measure_temp+1) % ASIC_TEMP_COUNT);
    }

}

int last_second_jobs;
int last_alive_jobs;


void ten_second_tasks() {
    periodic_bist_task();

}


void once_second_tasks() {
	static int counter=0;

	
	pthread_mutex_lock(&network_hw_mutex);
	//static int not_mining_counter = 0;
	// See if we can stop engines
	vm.not_mining_counter++;

	if (vm.not_mining_counter >= IDLE_TIME_TO_PAUSE_ENGINES) {
		if (!vm.asics_shut_down_powersave) {
			pause_all_mining_engines();
		}
	}
	pthread_mutex_unlock(&network_hw_mutex);
    update_top_current_measurments();
	auto_select_fan_level();
	
	//update_i2c_temperature_measurments();
	//write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
	//int current_nonce =  read_reg_device(0, ADDR_CURRENT_NONCE);
	//int nnce_start =  read_reg_device(0, ADDR_CURRENT_NONCE_START);
	//int job_id =  read_reg_device(0, ADDR_CURRENT_NONCE_RANGE);
	int no_addr =  read_reg_broadcast(ADDR_BR_NO_ADDR);
	if (no_addr) {
		printf("Error: lost address!\n");
		print_state();
		passert(0);
	}
	printf("Pushed %d jobs (%d:%d) (%d-%d), in queue %d jobs!\n", 
    	last_second_jobs,
    	spi_ioctls_read,
    	spi_ioctls_write, 
    	rt_queue_sw_write , 
    	rt_queue_hw_done ,
    	rt_queue_size);
	printf("wins:%d, leading-zeroes:%d idle:%d/%d\n",
		vm.solved_jobs_total,cur_leading_zeroes, vm.idle_probs, 
		vm.busy_probs);

	spi_ioctls_write = spi_ioctls_read = 0;
    //parse_int_register("ADDR_INTR_SOURCE", read_reg_broadcast(ADDR_INTR_SOURCE));
    last_second_jobs = 0;            
    print_adapter();
	//dc2dc_print();
    // Once every X seconds.
    periodic_scaling_task();

    if (nvm.dirty) {
        spond_save_nvm();
    }
    // print_state();
    dump_zabbix_stats();
	//pthread_mutex_unlock(&network_hw_mutex);

	if (++counter%10 == 0) {
	   ten_second_tasks();
    }
}





void once_25000_usec_tasks() {
	//printf("-");
	static int counter=0;
	uint32_t reg;
	while((reg = read_reg_broadcast(ADDR_BR_WIN))) {
	  uint16_t winner_device = BROADCAST_READ_ADDR(reg);
	  vm.hammer[winner_device].solved_jobs++;
	  vm.solved_jobs_total++;
	  //printf("\nSW:%x HW:%x\n",(actual_work)?actual_work->work_id_in_hw:0,jobid);
	  printf ("reg:%x  \n",reg);
	  int ok = get_print_win(winner_device);
	  // Move on!
	  //write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
	}
	if (read_reg_broadcast(ADDR_BR_CONDUCTOR_IDLE)) {
	  vm.idle_probs++;
	} else {
	  vm.busy_probs++;

	};

	// Here it is important to have right time.
	read_some_asic_temperatures();

	if (++counter%40 == 0) {
	   once_second_tasks();
    }
}



void once_2500_usec_tasks() {
   static int counter=0;
   int last_hw_job_id;
   RT_JOB work;    			   
   RT_JOB *actual_work = NULL;
   int has_request = pull_work_req(&work);
   if (has_request) {
	   // Update leading zeroes?
	   vm.not_mining_counter = 0;
	   if (work.leading_zeroes != cur_leading_zeroes) {
		   cur_leading_zeroes = work.leading_zeroes;
		   write_reg_broadcast(ADDR_WIN_LEADING_0, cur_leading_zeroes);
	   }
	   actual_work = add_to_sw_rt_queue(&work);
	   // write_reg_device(0, ADDR_CURRENT_NONCE_START, rand() + rand()<<16);
	   // write_reg_device(0, ADDR_CURRENT_NONCE_START + 1, rand() + rand()<<16);				 
	   push_to_hw_queue(actual_work);
	   last_hw_job_id = actual_work->work_id_in_hw;
	   assert(last_hw_job_id <= 0x100);
	   last_second_jobs++;
	   last_alive_jobs++;
   }

   if (++counter%10 == 0) {
	   once_25000_usec_tasks();
   }
   // Move latest job to complete queue
}





// never returns - thread
void* squid_regular_state_machine(void* p) {
	reset_sw_rt_queue();
	enable_reg_debug = 0;
	int loop = 0;
	printf("Starting squid_regular_state_machine!\n");
	flush_spi_write();
    
	if (!do_bist_ok(true)) {
        printf("Init BIST failure!\n");
        passert(0);
	}
	
	struct timeval tv;
	struct timeval last_job_pushed;
	//struct timeval last_print;
    //struct timeval last_test_win;    
	struct timeval last_force_queue;    
	gettimeofday(&last_job_pushed, NULL);	
	//gettimeofday(&last_test_win, NULL);
	//gettimeofday(&last_print, NULL);            
	gettimeofday(&last_force_queue, NULL);    
	gettimeofday(&tv, NULL);	
	int usec;
//	enable_reg_debug = 0;
	for (;;) {
		gettimeofday(&tv, NULL);
	    usec=(tv.tv_sec-last_job_pushed.tv_sec)*1000000;
		usec+=(tv.tv_usec-last_job_pushed.tv_usec);
        
		if (usec >= 2500) { // new job every 2.5 msecs = 400 per second
			once_2500_usec_tasks();
			 last_job_pushed = tv;
		   int drift = usec - 2500;
		   if (last_job_pushed.tv_usec > drift) {
			   last_job_pushed.tv_usec -= drift;
		   }	  
		}


        // force queue every 10 msecs
        // NEEDED ONLY ON FPGA!! TODO REMOVE!!
		usec=(tv.tv_sec-last_force_queue.tv_sec)*1000000;
		usec+=(tv.tv_usec-last_force_queue.tv_usec);
		if (usec >= 10000) { 
    		// printf("-");
			// move job forward every 10 milli
			// TODO - remove!!
			write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
            last_force_queue = tv;
		}

                
		//print_state();
		usleep(250);
	}
}














