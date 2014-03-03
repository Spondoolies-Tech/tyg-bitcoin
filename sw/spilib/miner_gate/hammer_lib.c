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
#include "asic_thermal.h"

#include <pthread.h>
#include <spond_debug.h>
#include <sys/time.h>
#include "real_time_queue.h"
#include "miner_gate.h"
#include "scaling_manager.h"
#include "asic_testboard.h"
#include "pll.h"

extern pthread_mutex_t network_hw_mutex;
pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hammer_mutex = PTHREAD_MUTEX_INITIALIZER;

int total_devices = 0;
extern int enable_reg_debug;
int cur_leading_zeroes = 20;

RT_JOB *add_to_sw_rt_queue(const RT_JOB *work);
void reset_sw_rt_queue();
RT_JOB *peak_rt_queue(uint8_t hw_id);

extern int rt_queue_size;
extern int rt_queue_sw_write;
extern int rt_queue_hw_done;

void dump_zabbix_stats();

void hammer_iter_init(hammer_iter *e) {
  e->addr = -1;
  e->done = 0;
  e->h = -1;
  e->l = 0;
  e->a = NULL;
}


int hammer_iter_next_present(hammer_iter *e) {
  int found = 0;
  
  while (!found) {
   // printf("l=%d,h=%d\n",e->l,e->h);
    passert(e->l < LOOP_COUNT);
    passert(e->h < HAMMERS_PER_LOOP);
    e->h++;

    if (e->h == HAMMERS_PER_LOOP) {
      e->l++;
      e->h = 0;
      while (!vm.loop[e->l].enabled_loop) {
        e->l++;
        if (e->l == LOOP_COUNT) {
//printf("return 0\n");
          e->done = 1;
          e->a = NULL;          
          return 0;
        }
      }
    }

    if (e->l == LOOP_COUNT) {
      e->done = 1;
      e->a = NULL;
      return 0;
    }

    if (vm.hammer[e->l * HAMMERS_PER_LOOP + e->h].asic_present) {
      found++;
    }
  }
  e->addr = e->l * HAMMERS_PER_LOOP + e->h;  
  e->a = vm.hammer + e->addr;
  return 1;
}

void loop_iter_init(loop_iter *e) {
  e->done = 0;
  e->l = -1;
}

int loop_iter_next_enabled(loop_iter *e) {
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
  // wait to finish real time queue requests
  write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
  write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
  //usleep(100);
  RT_JOB work;

  // Move real-time q to complete.
  while (one_done_sw_rt_queue(&work)) {
    push_work_rsp(&work);
  }

  // Move pending to complete
  // RT_JOB w;
  /*
  while (pull_work_req(&work)) {
    push_work_rsp(&work);
  }*/
 // passert(read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY) == 0);
  vm.stopped_all_work = 1;
}

void resume_all_work() { 
  if (vm.stopped_all_work) {
    vm.stopped_all_work = 0;
  } 
}

void print_devreg(int reg, const char *name) {
  printf("%2x:  BR:%8x ", reg, read_reg_broadcast(reg));
  int h, l;

  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    printf("DEV-%04x:%8x ", hi.a->address,
           read_reg_device(hi.addr, reg));
  }
  printf("  %s\n", name);
}

void parse_int_register(const char *label, int reg) {
  printf("%s :%x: ", label, reg);
  if (reg & BIT_INTR_NO_ADDR)
    printf("NO_ADDR ");
  if (reg & BIT_INTR_WIN)
    printf("WIN ");
  if (reg & BIT_INTR_FIFO_FULL)
    printf("FIFO_FULL ");
  if (reg & BIT_INTR_FIFO_ERR)
    printf("FIFO_ERR ");
  if (reg & BIT_INTR_CONDUCTOR_IDLE)
    printf("CONDUCTOR_IDLE ");
  if (reg & BIT_INTR_CONDUCTOR_BUSY)
    printf("CONDUCTOR_BUSY ");
  if (reg & BIT_INTR_RESERVED1)
    printf("RESERVED1 ");
  if (reg & BIT_INTR_FIFO_EMPTY)
    printf("FIFO_EMPTY ");
  if (reg & BIT_INTR_BIST_FAIL)
    printf("BIST_FAIL ");
  if (reg & BIT_INTR_THERMAL_SHUTDOWN)
    printf("THERMAL_SHUTDOWN ");
  if (reg & BIT_INTR_NOT_WIN)
    printf("NOT_WIN ");
  if (reg & BIT_INTR_0_OVER)
    printf("0_OVER ");
  if (reg & BIT_INTR_0_UNDER)
    printf("0_UNDER ");
  if (reg & BIT_INTR_1_OVER)
    printf("1_OVER ");
  if (reg & BIT_INTR_1_UNDER)
    printf("1_UNDER ");
  if (reg & BIT_INTR_PLL_NOT_READY)
    printf("PLL_NOT_READY ");
  printf("\n");
}

void print_state() {
  uint32_t i;
  static int print = 0;
  print++;
  dprintf("**************** DEBUG PRINT %x ************\n", print);
  print_devreg(ADDR_CURRENT_NONCE, "CURRENT NONCE");
  print_devreg(ADDR_BR_FIFO_FULL, "FIFO FULL");
  print_devreg(ADDR_BR_FIFO_EMPTY, "FIFO EMPTY");
  print_devreg(ADDR_BR_CONDUCTOR_BUSY, "BUSY");
  print_devreg(ADDR_BR_CONDUCTOR_IDLE, "IDLE");
  dprintf("*****************************\n", print);

  print_devreg(ADDR_CHIP_ADDR, "ADDR_CHIP_ADDR");
  print_devreg(ADDR_GOT_ADDR, "ADDR_GOT_ADDR ");
  print_devreg(ADDR_CONTROL, "ADDR_CONTROL ");
  print_devreg(ADDR_COMMAND, "ADDR_COMMAND ");
  print_devreg(ADDR_RESETING0, "ADDR_RESETING0");
  print_devreg(ADDR_RESETING1, "ADDR_RESETING1");
  print_devreg(ADDR_CONTROL_SET0, "ADDR_CONTROL_SET0");
  print_devreg(ADDR_CONTROL_SET1, "ADDR_CONTROL_SET1");
  print_devreg(ADDR_SW_OVERRIDE_PLL, "ADDR_SW_OVERRIDE_PLL");
  print_devreg(ADDR_PLL_RSTN, "ADDR_PLL_RSTN");
  print_devreg(ADDR_INTR_MASK, "ADDR_INTR_MASK");
  print_devreg(ADDR_INTR_CLEAR, "ADDR_INTR_CLEAR ");
  print_devreg(ADDR_INTR_RAW, "ADDR_INTR_RAW ");
  print_devreg(ADDR_INTR_SOURCE, "ADDR_INTR_SOURCE ");
  print_devreg(ADDR_BR_NO_ADDR, "ADDR_BR_NO_ADDR ");
  print_devreg(ADDR_BR_WIN, "ADDR_BR_WIN ");
  print_devreg(ADDR_BR_FIFO_FULL, "ADDR_BR_FIFO_FULL ");
  print_devreg(ADDR_BR_FIFO_ERROR, "ADDR_BR_FIFO_ERROR ");
  print_devreg(ADDR_BR_CONDUCTOR_IDLE, "ADDR_BR_CONDUCTOR_IDLE ");
  print_devreg(ADDR_BR_CONDUCTOR_BUSY, "ADDR_BR_CONDUCTOR_BUSY ");
  print_devreg(ADDR_BR_CRC_ERROR, "ADDR_BR_CRC_ERROR ");
  print_devreg(ADDR_BR_FIFO_EMPTY, "ADDR_BR_FIFO_EMPTY ");
  print_devreg(ADDR_BR_BIST_FAIL, "ADDR_BR_BIST_FAIL ");
  print_devreg(ADDR_BR_THERMAL_VIOLTION, "ADDR_BR_THERMAL_VIOLTION ");
  print_devreg(ADDR_BR_NOT_WIN, "ADDR_BR_NOT_WIN ");
  print_devreg(ADDR_BR_0_OVER, "ADDR_BR_0_OVER ");
  print_devreg(ADDR_BR_0_UNDER, "ADDR_BR_0_UNDER ");
  print_devreg(ADDR_BR_1_OVER, "ADDR_BR_1_OVER ");
  print_devreg(ADDR_BR_1_UNDER, "ADDR_BR_1_UNDER ");

  print_devreg(ADDR_MID_STATE, "ADDR_MID_STATE ");
  print_devreg(ADDR_MERKLE_ROOT, "ADDR_MERKLE_ROOT ");
  print_devreg(ADDR_TIMESTEMP, "ADDR_TIMESTEMP ");
  print_devreg(ADDR_DIFFICULTY, "ADDR_DIFFICULTY ");
  print_devreg(ADDR_WIN_LEADING_0, "ADDR_WIN_LEADING_0 ");
  print_devreg(ADDR_JOB_ID, "ADDR_JOB_ID");

  print_devreg(ADDR_WINNER_JOBID_WINNER_ENGINE,
               "ADDR_WINNER_JOBID_WINNER_ENGINE ");
  print_devreg(ADDR_WINNER_NONCE, "ADDR_WINNER_NONCE");
  print_devreg(ADDR_CURRENT_NONCE,
               CYAN "ADDR_CURRENT_NONCE" RESET);
  print_devreg(ADDR_LEADER_ENGINE, "ADDR_LEADER_ENGINE");
  print_devreg(ADDR_VERSION, "ADDR_VERSION");
  print_devreg(ADDR_ENGINES_PER_CHIP_BITS, "ADDR_ENGINES_PER_CHIP_BITS");
  print_devreg(ADDR_LOOP_LOG2, "ADDR_LOOP_LOG2");
  print_devreg(ADDR_ENABLE_ENGINE, "ADDR_ENABLE_ENGINE");
  print_devreg(ADDR_FIFO_OUT, "ADDR_FIFO_OUT");
  print_devreg(ADDR_CURRENT_NONCE_START, "ADDR_CURRENT_NONCE_START");
  print_devreg(ADDR_CURRENT_NONCE_START + 1, "ADDR_CURRENT_NONCE_START+1");
  print_devreg(ADDR_CURRENT_NONCE_START + 1, "ADDR_CURRENT_NONCE_START+2");

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

  // print_engines();
}

// Queues work to actual HW
// returns 1 if found nonce
void push_to_hw_queue(RT_JOB *work) {
  // ASSAF - JOBPUSH
  
  static int j = 0;
  int i;
  // printf("Push!\n");

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
}


// destribute range between 0 and max_range
void set_nonce_range_in_engines(unsigned int max_range) {
  int d, e;
  unsigned int current_nonce = 0;
  unsigned int engine_size;
  int total_engines = ENGINES_PER_ASIC * HAMMERS_COUNT;
  engine_size = ((max_range) / (total_engines));

  // for (d = FIRST_CHIP_ADDR; d < FIRST_CHIP_ADDR+total_devices; d++) {

  // int h, l;
  hammer_iter hi;
  hammer_iter_init(&hi);

  while (hammer_iter_next_present(&hi)) {
    write_reg_device(hi.addr, ADDR_CURRENT_NONCE_RANGE, engine_size);
    for (e = 0; e < ENGINES_PER_ASIC; e++) {
      //DBG(DBG_HW, "engine %x:%x got range from 0x%x to 0x%x\n", hi.addr, e,
       //   current_nonce, current_nonce + engine_size);
      write_reg_device(hi.addr, ADDR_CURRENT_NONCE_START + e, current_nonce);
      // read_reg_device(d, ADDR_CURRENT_NONCE_START + e);
      current_nonce += engine_size;
    }
  }
  //   
  flush_spi_write();
  // print_state();
}

int allocate_addresses_to_devices() {
  int reg;
  int l;
  total_devices = 0;

  // Give resets to all ASICs
  DBG(DBG_HW, "Unallocate all chip addresses\n");
  write_reg_broadcast(ADDR_GOT_ADDR, 0);

  //  validate address reset worked
  reg = read_reg_broadcast(ADDR_BR_NO_ADDR);
  if (reg == 0) {
    passert(0);
  }

  for (l = 0; l < LOOP_COUNT; l++) {
    // Only in loops discovered in "enable_good_loops_ok()"
    if (vm.loop[l].enabled_loop) {
      // Disable all other loops
      unsigned int bypass_loops = (~(1 << l) & 0xFFFFFF);
      printf("Giving address on loop (mask): %x\n", (~bypass_loops) & 0xFFFFFF);
      write_spi(ADDR_SQUID_LOOP_BYPASS, bypass_loops);
      
      // Give 8 addresses
      int h = 0;
      int asics_in_loop = 0;
      for (h = 0; h < HAMMERS_PER_LOOP; h++) {
        int addr = l * HAMMERS_PER_LOOP + h;
        vm.hammer[addr].address = addr;
        vm.hammer[addr].loop_address = l;
          
        if (read_reg_broadcast(ADDR_BR_NO_ADDR)) {
          write_reg_broadcast(ADDR_CHIP_ADDR, addr);
         
          total_devices++;
          asics_in_loop++;
          vm.working_engines[addr] = ALL_ENGINES_BITMASK;
          vm.hammer[addr].asic_present = 1;
          vm.hammer[addr].corner = nvm.asic_corner[addr];
          printf(MAGENTA"Address allocated: %x\n"RESET, addr);
          if (addr == 0x78) {
            disable_asic_forever(addr);
          }
        } else {
          vm.hammer[addr].address = addr;
          vm.hammer[addr].loop_address = l;
          vm.hammer[addr].asic_present = 0;
          vm.working_engines[addr] = 0;
          nvm.top_freq[addr] = ASIC_FREQ_0;
          nvm.asic_corner[addr] = ASIC_CORNER_NA;
        }
      }
      // Dont remove this print - used by scripts!!!!
      printf("%sASICS in loop %d: %d%s\n",
             (asics_in_loop == 8) ? RESET : RED, l,
             asics_in_loop, RESET);
      passert(read_reg_broadcast(ADDR_BR_NO_ADDR) == 0);
    } else {
      printf(RED "ASICS in loop %d: 0\n" RESET, l);
      ;
    }
  }
  // printf("3\n");
  write_spi(ADDR_SQUID_LOOP_BYPASS, (~(vm.good_loops))&0xFFFFFF);

  // Validate all got address
  passert(read_reg_broadcast(ADDR_BR_NO_ADDR) == 0);
//  passert(read_reg_broadcast(ADDR_VERSION) != 0);
  DBG(DBG_HW, "Number of ASICs found: %x (LOOPS=%X) %X\n", 
  total_devices,(~(vm.good_loops))&0xFFFFFF,read_reg_broadcast(ADDR_VERSION));
  return total_devices;

  // set_bits_offset(i);
}

void memprint(const void *m, size_t n) {
  int i;
  for (i = 0; i < n; i++) {
    printf("%02x", ((const unsigned char *)m)[i]);
  }
}


int SWAP32(int x);
void compute_hash(const unsigned char *midstate, unsigned int mrkle_root,
                  unsigned int timestamp, unsigned int difficulty,
                  unsigned int winner_nonce, unsigned char *hash);
int get_leading_zeroes(const unsigned char *hash);

int get_print_win(int winner_device) {
  // TODO - find out who one!
  // int winner_device = winner_reg >> 16;
  // enable_reg_debug = 1;
  uint32_t winner_nonce; // = read_reg_device(winner_device, ADDR_WINNER_NONCE);
  uint32_t winner_id;    // = read_reg_device(winner_device, ADDR_WINNER_JOBID);
  int engine_id;
  push_hammer_read(winner_device, ADDR_WINNER_NONCE, &winner_nonce);
  // push_read_reg_device(winner_device, ADDR_WINNER_JOBID, &winner_id);
  push_hammer_read(winner_device, ADDR_WINNER_JOBID_WINNER_ENGINE, &winner_id);
  write_reg_device(winner_device, ADDR_INTR_CLEAR, BIT_INTR_WIN);
  //  int current = read_reg_broadcast(ADDR_CURRENT_NONCE);
  squid_wait_hammer_reads();
  // Winner ID holds ID and Engine info
  engine_id = winner_id >> 8;
  winner_id = winner_id & 0xFF;
  RT_JOB *work_in_hw = peak_rt_queue(winner_id);

  // printf("----%x %x\n", winner_device ,read_reg_device(winner_device,
  // ADDR_WINNER_NONCE));
  // test that this is the "work_in_hw" win.
  if (work_in_hw->work_state == WORK_STATE_HAS_JOB) {
 
    printf("!!!!!!!  WIN !!!!!!!! %x %x\n", winner_nonce, work_in_hw->work_id_in_sw);
    work_in_hw->winner_nonce = winner_nonce;
    // Optional printing
    if (work_in_hw->winner_nonce != 0) {
#if 0      
      if (DBG_WINS) {
        DBG(DBG_WINS, "------ FOUND WIN:----\n");
        DBG(DBG_WINS, "- midstate:\n");
        int i;
        for (i = 0; i < 8; i++) {
          DBG(DBG_WINS, "-   %08x\n", work_in_hw->midstate[i]);
        }
        DBG(DBG_WINS, "- mrkle_root: %08x\n", work_in_hw->mrkle_root);
        DBG(DBG_WINS, "- timestamp : %08x\n", work_in_hw->timestamp);
        DBG(DBG_WINS, "- difficulty: %08x\n", work_in_hw->difficulty);
        DBG(DBG_WINS, "--- NONCE = %08x \n- ", work_in_hw->winner_nonce);
        static unsigned char hash[32];
        memset(hash, 0, 32);
        compute_hash(
            (const unsigned char *)work_in_hw->midstate,
            SWAP32(work_in_hw->mrkle_root), SWAP32(work_in_hw->timestamp),
            SWAP32(work_in_hw->difficulty), SWAP32(work_in_hw->winner_nonce),
            // 0x7c2bac1d,
            hash);
        memprint((void *)hash, 32);
        int leading_zeroes = get_leading_zeroes(hash);
        DBG(DBG_WINS, "\n- Leading Zeroes: %d\n", leading_zeroes);
        DBG(DBG_WINS, "-------------------\n");
#endif  
    }
    return 1;
  } else {
    printf(
        RED "------------------  -------- !!!!!  Warning !!!!: Win "
                       "orphan job 0x%x (now-job %x, last-job% x), nonce=0x%x!!!\n" RESET,
                      
        winner_id, vm.newest_hw_job_id, vm.oldest_hw_job_id,  winner_nonce);
    psyslog("!!!!!  Warning !!!!: Win orphan job_id 0x%x!!!\n", winner_id);
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
int update_i2c_temperature_measurments();

int init_hammers() {
  int i;
  // enable_reg_debug = 1;
  printf("VERSION SW:%x\n", 1);
  // FOR FPGA ONLY!!! TODO
  /*
  printf(RED "FPGA TODO REMOVE!\n" RESET);
  write_reg_broadcast(ADDR_LEADER_ENGINE, 0x0);
  write_reg_broadcast(ADDR_SW_OVERRIDE_PLL, 0x1);
  write_reg_broadcast(ADDR_PLL_RSTN, 0x1);
  */
  
  // Clearing all interrupts
  write_reg_broadcast(ADDR_INTR_MASK, 0x0);
  write_reg_broadcast(ADDR_INTR_CLEAR, 0xffff);
  write_reg_broadcast(ADDR_INTR_CLEAR, 0x0);


  flush_spi_write();
  // print_state();
  return 0;
}


typedef struct {
  uint32_t nonce_winner;
  uint32_t midstate[8];
  uint32_t mrkl;
  uint32_t timestamp;
  uint32_t difficulty;
  uint32_t leading;
} BIST_VECTOR;


#define TOTAL_BISTS 8
BIST_VECTOR bist_tests[TOTAL_BISTS] = 
{
{0x1DAC2B7C,
  {0xBC909A33,0x6358BFF0,0x90CCAC7D,0x1E59CAA8,0xC3C8D8E9,0x4F0103C8,0x96B18736,0x4719F91B},
    0x4B1E5E4A,0x29AB5F49,0xFFFF001D,0x26},

{0x227D01DA,
  {0xD83B1B22,0xD5CEFEB4,0x17C68B5C,0x4EB2B911,0x3C3D668F,0x5CCA92CA,0x80E63EEC,0x77415443},
    0x31863FE8,0x68538051,0x3DAA011A,0x20},
    
{0x481df739,
  {0x4a41f9ac,0x30309e3f,0x06d49e05,0x938f5ceb,0x90ef75e3,0x94fb77e4,0x9e851664,0x3a89aa95},
  0x73ae0eb3, 0xb8d6fb46, 0xf72e17e4, 36},


{0xbf9d8004,
  { 0x55a41303,0xeea9d4d3,0x39aa38f4,0x985a231f,0xbea0d01f,0x23218acc,0x08c65f3f,0x9535c3f7},
  0x86fb069c, 0xb0a01a68, 0xaf7f89b0, 36},


{0xbbff18c5,
  {0x02c397d8,0xececa9bc,0xc31e67f0,0x9a6c1b50,0x684206bc,0xd09e1e48,0x32a30c61,0x4f7f9717},
0xb9a63c7e,0x0fa4fe30,0x7ebf0578,34},



{0x331f38d8,
  { 0x72608c6a,0xb1dae622,0x74e0fd9d,0x80af02b4,0x0d3c4f66,0x3a0868d6,0x30d6d1cd,0x54b0e059},
  0x4dc7d05e, 0x45716f49, 0x25b433d9, 36},

{0x339bc585,
  { 0xa40ebbec,0x6aa3645e,0xc0ef8027,0xb83c0010,0x0eeda850,0xbc407eae,0x0d892b47,0x6b1f7541},
  0xd6b747c9, 0xd12764f2, 0xd026d972, 35},

{0xbbed44a1 ,
  {   0x0dd441c1,0x5802c0da,0xf2951ee4,0xd77909da,0xb25c02cb,0x009b6349,0xfe2d9807,0x257609a8 },
  0x5064218c, 0x57332e1b, 0xcef3abae, 35}


};




// returns 1 on success
int do_bist_ok() {
  // Choose random bist.
  static int bist_id = 2;

  int next_bist;
  do {
    next_bist = rand()%TOTAL_BISTS;
  } while (next_bist == bist_id);
  bist_id = next_bist;


  int poll_counter = 0;
     
  // Enter BIST mode
  write_reg_broadcast(ADDR_CONTROL_SET1, BIT_CTRL_BIST_MODE);
  poll_counter = 0;


  // Give BIST job
  write_reg_broadcast(ADDR_BIST_NONCE_START, bist_tests[bist_id].nonce_winner - 150); // 
  write_reg_broadcast(ADDR_BIST_NONCE_RANGE, 300);
  write_reg_broadcast(ADDR_BIST_NONCE_EXPECTED, bist_tests[bist_id].nonce_winner); // 0x1DAC2B7C
  write_reg_broadcast(ADDR_MID_STATE + 0, bist_tests[bist_id].midstate[0]);
  write_reg_broadcast(ADDR_MID_STATE + 1, bist_tests[bist_id].midstate[1]);
  write_reg_broadcast(ADDR_MID_STATE + 2, bist_tests[bist_id].midstate[2]);
  write_reg_broadcast(ADDR_MID_STATE + 3, bist_tests[bist_id].midstate[3]);
  write_reg_broadcast(ADDR_MID_STATE + 4, bist_tests[bist_id].midstate[4]);
  write_reg_broadcast(ADDR_MID_STATE + 5, bist_tests[bist_id].midstate[5]);
  write_reg_broadcast(ADDR_MID_STATE + 6, bist_tests[bist_id].midstate[6]);
  write_reg_broadcast(ADDR_MID_STATE + 7, bist_tests[bist_id].midstate[7]);
  write_reg_broadcast(ADDR_MERKLE_ROOT, bist_tests[bist_id].mrkl);
  write_reg_broadcast(ADDR_TIMESTEMP, bist_tests[bist_id].timestamp);
  write_reg_broadcast(ADDR_DIFFICULTY, bist_tests[bist_id].difficulty);
  write_reg_broadcast(ADDR_WIN_LEADING_0, bist_tests[bist_id].leading);
  write_reg_broadcast(ADDR_JOB_ID, 0xEE);
  write_reg_broadcast(ADDR_COMMAND, BIT_CMD_LOAD_JOB);
  flush_spi_write();


  // POLL RESULT    
  poll_counter = 0;
  //usleep(200000);
  int res;
  int i = 0;
  vm.bist_current = 0;
  
  int err;
  //vm.bist_current = tb_get_asic_current(loop_to_measure);
  while ((res = read_reg_broadcast(ADDR_BR_CONDUCTOR_BUSY))) {
    usleep(1);
    if ((i % 10000) == 0) {
      printf(RED "Waiting on bist ADDR_BR_CONDUCTOR_BUSY %x %x\n" RESET, res, i);
      vm.bist_fatal_err = 1;
      break;
    }
  }    


  
  static hammer_iter hi = {-6,-6,-6,-6,NULL};

  if (hi.addr == -6 || !hammer_iter_next_present(&hi)) {
    hammer_iter_init(&hi);
    hammer_iter_next_present(&hi);
  } 
    
  
  uint32_t single_win_test;
  push_hammer_read(hi.addr, ADDR_BR_WIN, &single_win_test);
  //read_reg_device(hi.addr, ADDR_BR_WIN);
 
  // Exit BIST
  int bist_fail;
  int failed = 0;
  //assert(read_reg_broadcast(ADDR_BR_WIN));
  while (bist_fail = read_reg_broadcast(ADDR_BR_BIST_FAIL)) {
    uint16_t failed_addr = BROADCAST_READ_ADDR(bist_fail);
    HAMMER *h = ASIC_GET_BY_ADDR(failed_addr);
    h->failed_bists++;
    h->passed_last_bist_engines = read_reg_device(failed_addr, ADDR_BIST_PASS);
    // printf("Writing %x to %x because read %d\n", BIT_INTR_BIST_FAIL,
    // failed_addr, bist_fail);
    printf(RED "Failed Bist %x::%x\n" RESET, failed_addr, h->passed_last_bist_engines);
    write_reg_device(failed_addr, ADDR_INTR_CLEAR, BIT_INTR_BIST_FAIL);
    write_reg_device(failed_addr, ADDR_INTR_CLEAR, BIT_INTR_WIN);
    write_reg_device(failed_addr, ADDR_CONTROL_SET0, BIT_CTRL_BIST_MODE);
    // print_devreg(ADDR_INTR_RAW , "ADDR_INTR_RAW ");
    failed++;
    //passert(0);
  }

  if (!single_win_test) { // Not won - kill it!
    disable_asic_forever(hi.addr);
  }

  /*if (!failed) {
    printf(GREEN "Passed BIST %d:\n" RESET, bist_id);
  }*/

  // write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
  // write_reg_broadcast(ADDR_INTR_CLEAR, BIT_INTR_BIST_FAIL);
  write_reg_broadcast(ADDR_INTR_CLEAR, BIT_INTR_WIN);
  write_reg_broadcast(ADDR_CONTROL_SET0, BIT_CTRL_BIST_MODE);
  write_reg_broadcast(ADDR_WIN_LEADING_0, cur_leading_zeroes);
  flush_spi_write();
  return !failed;
}

int pull_work_req(RT_JOB *w);
void print_adapter();
int has_work_req();

int last_second_jobs;
int last_alive_jobs;


void one_minute_tasks() {
  
}


void ten_second_tasks() {  
}
/*
pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&i2c_mutex);
*/

void once_second_tasks() {
  static int counter = 0;
   
  auto_select_fan_level();


  if (vm.cosecutive_jobs >= MIN_COSECUTIVE_JOBS_FOR_SCALING) {
      if ((vm.start_mine_time == 0) &&
          (!vm.scaling_up_system)) {
        vm.start_mine_time = time(NULL);
        printf(MAGENTA "Starting mining timer for voltage scaling\n" RESET);
      }
  } else {
    vm.start_mine_time = 0;
  }
  
  pthread_mutex_lock(&network_hw_mutex);
  // static int not_mining_counter = 0;
  // See if we can stop engines
  vm.not_mining_counter++;

  if (vm.not_mining_counter >= IDLE_TIME_TO_PAUSE_ENGINES) {
    if (!vm.asics_shut_down_powersave) {
      pause_all_mining_engines();
    }
  } 

  pthread_mutex_unlock(&network_hw_mutex);

  // update_i2c_temperature_measurments();
  // write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
  // int current_nonce =  read_reg_device(0, ADDR_CURRENT_NONCE);
  // int nnce_start =  read_reg_device(0, ADDR_CURRENT_NONCE_START);
  // int job_id =  read_reg_device(0, ADDR_CURRENT_NONCE_RANGE);
  int no_addr = read_reg_broadcast(ADDR_BR_NO_ADDR);
  if (no_addr) {
    printf("Error: lost address!\n");
    print_state();
    passert(0);
  }

  printf("Pushed %d jobs (%d:%d) (%d-%d), in queue %d jobs!\n",
         last_second_jobs, spi_ioctls_read, spi_ioctls_write, rt_queue_sw_write,
         rt_queue_hw_done, rt_queue_size);
  printf("wins:%d, leading-zeroes:%d idle:%d/%d\n", vm.solved_jobs_total,
         cur_leading_zeroes, vm.idle_probs, vm.busy_probs);

  spi_ioctls_write = spi_ioctls_read = 0;
  // parse_int_register("ADDR_INTR_SOURCE",
  // read_reg_broadcast(ADDR_INTR_SOURCE));
  last_second_jobs = 0;
  print_adapter();
  // dc2dc_print();

  // Decrease frequencies if needed
  //decrease_asics_freqs();
  
  if (nvm.dirty) {
    spond_save_nvm();
  }
  // print_state();
  //dump_zabbix_stats();
  // pthread_mutex_unlock(&network_hw_mutex);

  // each second
  change_dc2dc_voltage_if_needed();

  if (!vm.asics_shut_down_powersave) {    
      //Once every 10 seconds upscale ASICs if can
      if ( (((counter % BIST_PERIOD_SECS_RAMPUP) == 0) && vm.scaling_up_system) || 
           ((counter % BIST_PERIOD_SECS) == 0)) {
        periodic_bist_task();
      }
  }




  static int loop_for_err;
 
  if (counter % 10 == 0) {
    ten_second_tasks(); 
  }

  
  if (counter % 60 == 2) {
    one_minute_tasks();
  }
  ++counter;

  print_scaling();
}


// 40 times per second
// RUN FROM DIFFERENT THREAD
void update_vm_with_dc2dc_current_and_temperature() {
  static int counter = 0;
  counter++;
  pthread_mutex_lock(&i2c_mutex);
  if ((counter % 20) == 1)  {
    // 2 times a second
    update_ac2dc_current_measurments();
    int err;
  } 


  {
    // 38 times a second, poll 1 dc2dc 2 times a second.
    static int loop = 0;
    if (vm.loop[loop].enabled_loop) {
      update_dc2dc_current_temp_measurments(loop);
    }
    int e = get_dc2dc_error(loop); 
    if (e) {
      printf(RED "DC2DC error on loop %d\n" RESET, loop);
      // If top current in last 20 seconds:
      nvm.top_dc2dc_current_16s[loop] = nvm.top_dc2dc_current_16s[loop] - 8;
      if (nvm.top_dc2dc_current_16s[loop] >= DC2DC_MINIMAL_TOP_16S) {
         nvm.top_dc2dc_current_16s[loop] = nvm.top_dc2dc_current_16s[loop] - 8;
      } else {
        nvm.top_dc2dc_current_16s[loop] = DC2DC_MINIMAL_TOP_16S;
      }
      nvm.dirty = 1;
      printf(RED "Saving NVM max DC2DC current: %d=%d\n" RESET,  loop, nvm.top_dc2dc_current_16s[loop]);
      vm.loop[loop].dc2dc.dc_fail_time = time(NULL);
      print_nvm();
      spond_save_nvm();
    }
   

    
    // once 20 seconds - if DC failed, save NVM, restart
    if (counter%(40) == 0) {
      //printf(YELLOW "Testing dcdcdc\n" RESET);
      int l;
      for(l = 0; l < LOOP_COUNT ; l++) {
        if (vm.loop[loop].dc2dc.dc_fail_time) {
          if (time(NULL) - vm.loop[loop].dc2dc.dc_fail_time > TIME_TO_RUN_AFTER_DC_PROBLEM) {
            spond_save_nvm();
            print_nvm();
            usleep(1000000);
            assert(0);
          }
        }
      }
    }
    loop = (loop+1)%LOOP_COUNT;
    pthread_mutex_unlock(&i2c_mutex);
  } 
}



void check_for_dc2dc_errors() {
  
}



// 30 times per second
void update_vm_with_asic_current_and_temperature() {
  {
    // 40 times a second poll 1 loop ASICs for 2 temperatures
    // We measure 1 loop on all temperatures over 1/8 second
    // We complete full cycle of all loops all temperatures over 3 seconds    
    static int temp_measure_loop = 0;
    static ASIC_TEMP temp_measure_temp = ASIC_TEMP_83;

    read_some_asic_temperatures(temp_measure_loop, temp_measure_temp, (ASIC_TEMP)(temp_measure_temp+1));
    // Progress loop and (maybe) temperature

    temp_measure_loop = (temp_measure_loop + 1) % LOOP_COUNT;
    if (temp_measure_loop == 0) {
     temp_measure_temp=(ASIC_TEMP)(temp_measure_temp+2);
     if (temp_measure_temp >= ASIC_TEMP_COUNT) {
       temp_measure_temp = ASIC_TEMP_83;
     }
    }
    if (temp_measure_temp >= ASIC_TEMP_83 &&
       temp_measure_loop == 0) {
     printf("Temperature cycle - done.\n");
    }
  }
}

// 30 times a second
void once_33_msec_tasks() {
  // printf("-"); 
  static int counter = 0;
  uint32_t reg;
  while ((reg = read_reg_broadcast(ADDR_BR_WIN))) {
    uint16_t winner_device = BROADCAST_READ_ADDR(reg);
    vm.hammer[winner_device].solved_jobs++;
    vm.solved_jobs_total++;
    // printf("\nSW:%x
    // HW:%x\n",(actual_work)?actual_work->work_id_in_hw:0,jobid);
    // printf("reg:%x  \n", reg);
    int ok = get_print_win(winner_device);
    // Move on!
    // write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
  }

  
  if (++counter % 30 == 0) {
    once_second_tasks();
  } else {
    update_vm_with_asic_current_and_temperature();
  }
}


// 666 times a second
void once_1500_usec_tasks() {
  static int counter = 0;
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
    vm.newest_hw_job_id = actual_work->work_id_in_hw;
    push_to_hw_queue(actual_work);
    last_second_jobs++;
    last_alive_jobs++;
    if (vm.cosecutive_jobs < MAX_CONSECUTIVE_JOBS_TO_COUNT) {    
      vm.cosecutive_jobs++;
    }
  } else {
    if (vm.cosecutive_jobs > 0) {
      vm.cosecutive_jobs--;
    }
  }

  if (++counter % 22 == 0) {
    once_33_msec_tasks();
  }
  // Move latest job to complete queue
}



// never returns - thread
void *dc2dc_state_machine(void *p) {
  while (1) {
    // sleep 1/40th second
    usleep(1000000/40);
    update_vm_with_dc2dc_current_and_temperature();

     pthread_mutex_lock(&hammer_mutex);
     if (read_reg_broadcast(ADDR_BR_CONDUCTOR_IDLE)) {
        vm.idle_probs++;
      } else {
        vm.busy_probs++;
      }
       pthread_mutex_unlock(&hammer_mutex);
        
  }
}


// never returns - thread
void *squid_regular_state_machine(void *p) {
  reset_sw_rt_queue();
    
  int loop = 0;
  printf("Starting squid_regular_state_machine!\n");
  flush_spi_write();

  // Do BIST before start
  hammer_iter hi;
  hammer_iter_init(&hi);
  while (hammer_iter_next_present(&hi)) {
    hi.a->failed_bists = 0;
    hi.a->passed_last_bist_engines = ALL_ENGINES_BITMASK;
  }

  struct timeval tv;
  struct timeval last_job_pushed;
  // struct timeval last_print;
  // struct timeval last_test_win;
  struct timeval last_force_queue;
  gettimeofday(&last_job_pushed, NULL);
  // gettimeofday(&last_test_win, NULL);
  // gettimeofday(&last_print, NULL);
  gettimeofday(&last_force_queue, NULL);
  gettimeofday(&tv, NULL);
  int usec;
  //    
  for (;;) {
    gettimeofday(&tv, NULL);
    usec = (tv.tv_sec - last_job_pushed.tv_sec) * 1000000;
    usec += (tv.tv_usec - last_job_pushed.tv_usec);

    if (usec >= JOB_PUSH_PERIOD_US) { // new job every 2.5 msecs = 400 per second
      pthread_mutex_lock(&hammer_mutex);
      once_1500_usec_tasks();
      last_job_pushed = tv;
      int drift = usec - JOB_PUSH_PERIOD_US;
      if (last_job_pushed.tv_usec > drift) {
        last_job_pushed.tv_usec -= drift;
      }
      pthread_mutex_unlock(&hammer_mutex);
    }

    // force queue every 10 msecs
    // NEEDED ONLY ON FPGA!! TODO REMOVE!!
    /*
    usec = (tv.tv_sec - last_force_queue.tv_sec) * 1000000;
    usec += (tv.tv_usec - last_force_queue.tv_usec);
    if (usec >= JOB_PUSH_PERIOD_US*4) {
      // printf("-");
      // move job forward every 10 milli
      // TODO - remove!!
      write_reg_broadcast(ADDR_COMMAND, BIT_CMD_END_JOB);
      last_force_queue = tv;
    }
    */

    // print_state();
    usleep(JOB_PUSH_PERIOD_US/10);
  }
}
