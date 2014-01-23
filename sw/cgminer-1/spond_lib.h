#ifndef SPONDND_SDSFL_LIB_H
#define SPONDND_SDSFL_LIB_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>


#define SIZE_OF_TARGET_BYTES  32 
#define SPOND_WORK_STATE_WAITING 0
#define SPOND_WORK_STATE_FINISHED    1
#define SPOND_WORK_STATE_FOUND_NONCE 2


typedef struct {
	unsigned char work_id;

	// IN parameters
	unsigned char	leading_zeroes;
	unsigned char	data_tail64[64]; // little endian like in block
	unsigned char	midstate[32];    // not reverted, like output of SHA

	// OUT parameters
	unsigned int   success;
	unsigned int   work_state;
	unsigned int   winner_nonce;
	//unsigned char  winner_hash[32];

	void* app_data;
	void* app_data2;	
} SPOND_WORK;

// Callback with result 
typedef void (*FINISHED_CB)(SPOND_WORK* work);
typedef void (*WON_CB)(SPOND_WORK* work);


#ifdef __cplusplus
// "__cplusplus" is defined whenever it's a C++ compiler,
// not a C compiler, that is doing the compiling.
extern "C" {
#endif

// Just for test
void spond_get_board_state(int *free_slots, int *engine_running);
// Connect to board
int spond_connect(const char* tty_dev, FINISHED_CB done_cb, WON_CB _won_cb);
// returns how much space for WORK availible, -1 for error.
// TODO - fix syncronisation after restart
int spond_give_work(SPOND_WORK *work);
// Set SHA tarrget 
//void spond_set_target(const unsigned char *target); // 32 byte
void bin2str(const unsigned char *p, size_t p_len, char* str);

// Drop all current work
//void spond_dropwork();
void spond_log(const char* p);


#ifdef __cplusplus
}
#endif




#endif
