
#include <stdio.h>
#include <stdlib.h>

#include "pll.h"
#include "hammer.h"
#include "defines.h"
#include "squid.h"
#include "hammer_lib.h"

#include "spond_debug.h"

pll_frequency_settings pfs[ASIC_FREQ_MAX] = {
  { 0, 0, 0 }, 
  FREQ_225_0, FREQ_240_0, FREQ_255_0, FREQ_270_0, FREQ_285_0,
  FREQ_300_0, FREQ_315_0, FREQ_330_0, FREQ_345_0, FREQ_360_0, FREQ_375_0,
  FREQ_390_0, FREQ_405_0, FREQ_420_0, FREQ_435_0, FREQ_450_0, FREQ_465_0,
  FREQ_480_0, FREQ_495_0, FREQ_510_0, FREQ_525_0, FREQ_540_0, FREQ_555_0,
  FREQ_570_0, FREQ_585_0, FREQ_600_0, FREQ_615_0, FREQ_630_0, FREQ_645_0,
  FREQ_660_0
};



void enable_all_engines_all_asics() {
  int i;
  write_reg_broadcast(ADDR_RESETING0, 0xffffffff);
  write_reg_broadcast(ADDR_RESETING1, 0xffffffff);
  write_reg_broadcast(ADDR_ENABLE_ENGINE, ALL_ENGINES_BITMASK);
  flush_spi_write();
}



void disable_engines_all_asics() {
  //printf("Disabling engines:\n");
  write_reg_broadcast(ADDR_ENABLE_ENGINE, 0);
  write_reg_broadcast(ADDR_RESETING1, 0);
  write_reg_broadcast(ADDR_RESETING0, 0);
  write_reg_broadcast(ADDR_CLK_ENABLE, 0);
  flush_spi_write();
  vm.engines_disabled = 1;
}



void set_pll(int addr, ASIC_FREQ freq) {
#if HAS_PLL == 1
  passert(vm.engines_disabled == 1);
  write_reg_device(addr, ADDR_DLL_OFFSET_CFG_LOW, 0xC3C1C200);
  write_reg_device(addr, ADDR_DLL_OFFSET_CFG_HIGH, 0x0082C381);
  passert(freq < ASIC_FREQ_MAX);
  pll_frequency_settings *ppfs = &pfs[freq];
  uint32_t pll_config = 0;
  uint32_t M = ppfs->m_mult;
  uint32_t P = ppfs->p_div;
  pll_config = (M - 1) & 0xFF;
  pll_config |= (P - 1) & 0x1F << 13;
  pll_config |= 0x100000;
  write_reg_device(addr, ADDR_PLL_CONFIG, pll_config);
  write_reg_device(addr, ADDR_PLL_ENABLE, 0x0);
  write_reg_device(addr, ADDR_PLL_ENABLE, 0x1);
#if 0
  int i = 0; 
  while (read_reg_device(addr, ADDR_PLL_STATUS) != 3) {
    if (i++ > 100) {
      passert("PLL lock failure\n");
    }
    usleep(50);
  }
#else
  printf(ANSI_COLOR_RED "sPLL!\n" ANSI_COLOR_RESET);
#endif
#endif
}


void enable_nvm_engines_all_asics() {
#if 0
    int i = 0; 
    while (read_reg_broadcast(ADDR_BR_PLL_NOT_READY) != 0) {
      if (i++ > 100) {
        passert("PLL lock failure\n");
      }
      usleep(50);
    }
#endif
   //printf("Enabling engines from NVM:\n");
   hammer_iter hi;
   hammer_iter_init(&hi);
   //passert(vm.engines_disabled == 1);
   while (hammer_iter_next_present(&hi)) {
     // for each ASIC
     write_reg_device(hi.addr, ADDR_CLK_ENABLE, nvm.working_engines[hi.addr]);
     write_reg_device(hi.addr, ADDR_RESETING0, nvm.working_engines[hi.addr]);
     write_reg_device(hi.addr, ADDR_RESETING1, nvm.working_engines[hi.addr] | 0x8000);
     write_reg_device(hi.addr, ADDR_ENABLE_ENGINE, nvm.working_engines[hi.addr]);
   }
   flush_spi_write();
   vm.engines_disabled = 0;
}
