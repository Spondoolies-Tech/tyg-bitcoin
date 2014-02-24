#ifndef _____HAMMERLIBBBB_H____
#define _____HAMMERLIBBBB_H____

#include "nvm.h"
#include "hammer.h"

void pause_all_mining_engines();
void unpause_all_mining_engines();

typedef struct {
  int l;
  int h;
  int addr;
  int done;
} hammer_iter;

typedef struct {
  int l;
  int done;
} loop_iter;

void hammer_iter_init(hammer_iter *e);
int hammer_iter_next_present(hammer_iter *e);
void loop_iter_init(loop_iter *e);
int loop_iter_next_enabled(loop_iter *e);

void stop_all_work();
void resume_all_work();

uint32_t read_reg_broadcast_test(uint8_t offset);
extern int assert_serial_failures;
int one_done_sw_rt_queue(RT_JOB *work);

void *squid_regular_state_machine(void *p);
int init_hammers();
void init_scaling();
int do_bist_ok();
uint32_t crc32(uint32_t crc, const void *buf, size_t size);
void enable_nvm_loops();
void enable_voltage_freq_and_engines_from_nvm();
void enable_voltage_freq_and_engines_default();

void recompute_corners_and_voltage_update_nvm();
void spond_save_nvm();
void create_default_nvm();
void find_bad_engines_update_nvm();
int enable_nvm_loops_ok();
int allocate_addresses_to_devices();
void set_nonce_range_in_engines(unsigned int max_range);
void enable_all_engines_all_asics();
void discover_good_loops_update_nvm();
void periodic_bist_task();

#endif
