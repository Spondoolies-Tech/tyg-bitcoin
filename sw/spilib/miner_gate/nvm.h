#ifndef _____NVMLIB_H__B__
#define _____NVMLIB_H__B__

#include <stdint.h>
#include <unistd.h>
#include "dc2dc.h"
#include "defines.h"

#define NVM_VERSION (0xCAF10000 + sizeof(SPONDOOLIES_NVM))
#define RECOMPUTE_NVM_TIME_SEC (60 * 60 * 24) // once every 24 hours

#define LOOP_COUNT 24
#define HAMMERS_PER_LOOP 8
#define HAMMERS_COUNT (LOOP_COUNT *HAMMERS_PER_LOOP)
#define ALL_ENGINES_BITMASK 0x7FFF
#define NVM_FILE_NAME "/nvm/nvm.bin"

typedef enum {
  ASIC_FREQ_0 = 0,
  ASIC_FREQ_225,
  ASIC_FREQ_240,
  ASIC_FREQ_255,
  ASIC_FREQ_270,
  ASIC_FREQ_285,
  ASIC_FREQ_300,
  ASIC_FREQ_315,
  ASIC_FREQ_330,
  ASIC_FREQ_345,
  ASIC_FREQ_360,
  ASIC_FREQ_375,
  ASIC_FREQ_390,
  ASIC_FREQ_405,
  ASIC_FREQ_420,
  ASIC_FREQ_435,
  ASIC_FREQ_450,
  ASIC_FREQ_465,
  ASIC_FREQ_480,
  ASIC_FREQ_495,
  ASIC_FREQ_510,
  ASIC_FREQ_525,
  ASIC_FREQ_540,
  ASIC_FREQ_555,
  ASIC_FREQ_570,
  ASIC_FREQ_585,
  ASIC_FREQ_600,
  ASIC_FREQ_615,
  ASIC_FREQ_630,
  ASIC_FREQ_645,
  ASIC_FREQ_660,
  ASIC_FREQ_675,
  ASIC_FREQ_690,
  ASIC_FREQ_705,
  ASIC_FREQ_720,
  ASIC_FREQ_735,
  ASIC_FREQ_750,
  ASIC_FREQ_765,
  ASIC_FREQ_780,
  ASIC_FREQ_795,
  ASIC_FREQ_810,
  ASIC_FREQ_825,
  ASIC_FREQ_840,
  ASIC_FREQ_855,
  ASIC_FREQ_870,
  ASIC_FREQ_MAX
} ASIC_FREQ;

typedef enum {
  ASIC_CORNER_NA = 0,
  ASIC_CORNER_SS = 1,
  ASIC_CORNER_SSTT = 2,
  ASIC_CORNER_TT = 3,
  ASIC_CORNER_TTFFG = 4,
  ASIC_CORNER_FFG = 5,
  ASIC_CORNER_COUNT = 6,
} ASIC_CORNER;

#define ASIC_TEMP_MIN 77
#define ASIC_TEMP_DELTA 6
typedef enum {
  ASIC_TEMP_77 = 0, // under the radar
  ASIC_TEMP_83 = 1,
  ASIC_TEMP_89 = 2,
  ASIC_TEMP_95 = 3,
  ASIC_TEMP_101 = 4,
  ASIC_TEMP_107 = 5,
  ASIC_TEMP_113 = 6,
  ASIC_TEMP_119 = 7,
  ASIC_TEMP_125 = 8,
  ASIC_TEMP_COUNT = 9,
} ASIC_TEMP;


typedef struct _spondoolies_nvm {
  // Fore competibility tests.
  uint32_t nvm_version;
  // time(NULL) at store time.
  uint32_t store_time;
  // Values are ASIC_CORNER_XXX
  uint8_t asic_corner[HAMMERS_COUNT];
  // Working engines at each ASIC
  // uint16_t working_engines[HAMMERS_COUNT];
  // Top frequency per ASIC
  ASIC_FREQ top_freq[HAMMERS_COUNT];
  // Each loop voltage
  DC2DC_VOLTAGE loop_voltage[LOOP_COUNT];
  // Save nvm
  uint8_t dirty;

  // Known top working DC2DC current
  uint32_t top_dc2dc_current_16s[LOOP_COUNT];

  
  // This field must be last!
  uint32_t crc32;
} SPONDOOLIES_NVM;

extern SPONDOOLIES_NVM nvm;
int load_nvm_ok();
void spond_save_nvm();
void spond_delete_nvm();
void print_nvm();

#endif
