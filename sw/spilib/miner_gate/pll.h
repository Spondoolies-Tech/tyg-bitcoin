#ifndef _____PLLLIB_H__B__
#define _____PLLLIB_H__B__

#include <stdint.h>
#include <unistd.h>

#include "nvm.h"

typedef struct {
  uint16_t m_mult;
  uint16_t fvco;
  uint16_t p_div;
} pll_frequency_settings;

#define FREQ_225_0                                                             \
  { 30, 900, 2 }
#define FREQ_232_5                                                             \
  { 31, 930, 2 }
#define FREQ_240_0                                                             \
  { 32, 960, 2 }
#define FREQ_247_5                                                             \
  { 33, 990, 2 }
#define FREQ_255_0                                                             \
  { 34, 1020, 2 }
#define FREQ_262_5                                                             \
  { 35, 1050, 2 }
#define FREQ_270_0                                                             \
  { 36, 1080, 2 }
#define FREQ_277_5                                                             \
  { 37, 1110, 2 }
#define FREQ_285_0                                                             \
  { 38, 1140, 2 }
#define FREQ_292_5                                                             \
  { 39, 1170, 2 }
#define FREQ_300_0                                                             \
  { 40, 1200, 2 }
#define FREQ_307_5                                                             \
  { 41, 1230, 2 }
#define FREQ_315_0                                                             \
  { 42, 1260, 2 }
#define FREQ_322_5                                                             \
  { 43, 1290, 2 }
#define FREQ_330_0                                                             \
  { 44, 1320, 2 }
#define FREQ_337_5                                                             \
  { 45, 1350, 2 }
#define FREQ_345_0                                                             \
  { 46, 1380, 2 }
#define FREQ_352_5                                                             \
  { 47, 1410, 2 }
#define FREQ_360_0                                                             \
  { 48, 1440, 2 }
#define FREQ_375_0                                                             \
  { 25, 750, 1 }
#define FREQ_390_0                                                             \
  { 26, 780, 1 }
#define FREQ_405_0                                                             \
  { 27, 810, 1 }
#define FREQ_420_0                                                             \
  { 28, 840, 1 }
#define FREQ_435_0                                                             \
  { 29, 870, 1 }
#define FREQ_450_0                                                             \
  { 30, 900, 1 }
#define FREQ_465_0                                                             \
  { 31, 930, 1 }
#define FREQ_480_0                                                             \
  { 32, 960, 1 }
#define FREQ_495_0                                                             \
  { 33, 990, 1 }
#define FREQ_510_0                                                             \
  { 34, 1020, 1 }
#define FREQ_525_0                                                             \
  { 35, 1050, 1 }
#define FREQ_540_0                                                             \
  { 36, 1080, 1 }
#define FREQ_555_0                                                             \
  { 37, 1110, 1 }
#define FREQ_570_0                                                             \
  { 38, 1140, 1 }
#define FREQ_585_0                                                             \
  { 39, 1170, 1 }
#define FREQ_600_0                                                             \
  { 40, 1200, 1 }
#define FREQ_615_0                                                             \
  { 41, 1230, 1 }
#define FREQ_630_0                                                             \
  { 42, 1260, 1 }
#define FREQ_645_0                                                             \
  { 43, 1290, 1 }
#define FREQ_660_0                                                             \
  { 44, 1320, 1 }

void set_pll(int addr, ASIC_FREQ freq);

#endif
