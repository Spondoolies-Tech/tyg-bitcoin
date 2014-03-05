
#include <stdio.h>
#include <stdlib.h>

#include "pll.h"
#include "hammer.h"
#include "defines.h"
#include "squid.h"
#include "hammer_lib.h"
#include "scaling_manager.h"

#include "spond_debug.h"

pll_frequency_settings pfs[ASIC_FREQ_MAX] = {
  { 0, 0, 0 }, 
  FREQ_225_0, FREQ_240_0, FREQ_255_0, FREQ_270_0, FREQ_285_0,
  FREQ_300_0, FREQ_315_0, FREQ_330_0, FREQ_345_0, FREQ_360_0, FREQ_375_0,
  FREQ_390_0, FREQ_405_0, FREQ_420_0, FREQ_435_0, FREQ_450_0, FREQ_465_0,
  FREQ_480_0, FREQ_495_0, FREQ_510_0, FREQ_525_0, FREQ_540_0, FREQ_555_0,
  FREQ_570_0, FREQ_585_0, FREQ_600_0, FREQ_615_0, FREQ_630_0, FREQ_645_0,
  FREQ_660_0, FREQ_675_0, FREQ_690_0, FREQ_705_0, FREQ_720_0, FREQ_735_0,
  FREQ_750_0, FREQ_765_0, FREQ_780_0, FREQ_795_0, FREQ_810_0, FREQ_825_0, 
  FREQ_840_0, FREQ_855_0, FREQ_870_0, 
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


void disable_engines_asic(int addr) {
  //printf("Disabling engines:\n");
  write_reg_device(addr,ADDR_ENABLE_ENGINE, 0);
  write_reg_device(addr,ADDR_RESETING1, 0);
  write_reg_device(addr,ADDR_RESETING0, 0);
  write_reg_device(addr,ADDR_CLK_ENABLE, 0);
  flush_spi_write();
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
  pll_config |= ((P - 1) & 0x1F) << 13;
  pll_config |= 0x100000;
  write_reg_device(addr, ADDR_PLL_CONFIG, pll_config);
  write_reg_device(addr, ADDR_PLL_ENABLE, 0x0);
  write_reg_device(addr, ADDR_PLL_ENABLE, 0x1);
#if 0
  int i = 0; 
  while ((read_reg_device(addr, ADDR_PLL_STATUS) & 3)!= 3) {
    if (i++ > 100) {
      passert("PLL lock failure\n");
    }
    usleep(50);
  }
#else
  //printf(RED "sPLL!\n" RESET);
#endif
#endif
}

void disable_asic_forever(int addr) {
  vm.working_engines[addr] = 0;
  vm.hammer[addr].asic_present = 0;
  psyslog("Disabing ASIC forever %x\n", addr);
  write_reg_device(addr, ADDR_CONTROL_SET1, BIT_CTRL_DISABLE_TX);
}


int enable_nvm_engines_all_asics_ok() {
#if 1
    int i = 0; 
    int reg;
    int killed_pll=0;
    while ((reg = read_reg_broadcast(ADDR_BR_PLL_NOT_READY)) != 0) {
      if (i++ > 200) {
        printf(RED "F*cking PLL %x stuck, killing ASIC\n" RESET, reg);
        //return 0;
        int addr = BROADCAST_READ_ADDR(reg);
        disable_asic_forever(addr);
        killed_pll++;
      }
      usleep(50);
    }
#endif
   //printf("Enabling engines from NVM:\n");
   if (killed_pll) {
     passert(test_serial(-1)); 
   }
   hammer_iter hi;
   hammer_iter_init(&hi);
   //passert(vm.engines_disabled == 1);
   while (hammer_iter_next_present(&hi)) {
     // for each ASIC
     write_reg_device(hi.addr, ADDR_CLK_ENABLE, vm.working_engines[hi.addr]);
     write_reg_device(hi.addr, ADDR_RESETING0, vm.working_engines[hi.addr]);
     write_reg_device(hi.addr, ADDR_RESETING1, vm.working_engines[hi.addr] | 0x8000);
     write_reg_device(hi.addr, ADDR_ENABLE_ENGINE, vm.working_engines[hi.addr]);
   }
   flush_spi_write();
   vm.engines_disabled = 0;
   return 1;
}

