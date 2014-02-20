#include "pll.h"



pll_frequency_settings pfs[ASIC_FREQ_MAX] = {
    	{0,0,0,0},
        FREQ_225_0 ,
        FREQ_240_0 ,
        FREQ_255_0 ,
        FREQ_270_0 ,
        FREQ_285_0 ,
        FREQ_300_0 ,
        FREQ_315_0 ,
        FREQ_330_0 ,
        FREQ_345_0 ,
        FREQ_360_0 ,
        FREQ_375_0 ,
        FREQ_390_0 ,
        FREQ_405_0 ,
        FREQ_420_0 ,
        FREQ_435_0 ,
        FREQ_450_0 ,
        FREQ_465_0 ,
        FREQ_480_0 ,
        FREQ_495_0 ,
        FREQ_510_0 ,
        FREQ_525_0 ,
        FREQ_540_0 ,
        FREQ_555_0 ,
        FREQ_570_0 ,
        FREQ_585_0 ,
        FREQ_600_0 ,
        FREQ_615_0 ,
        FREQ_630_0 ,
        FREQ_645_0 ,
        FREQ_660_0     
};

void set_pll(int addr, ASIC_FREQ freq) {
	write_reg_device(addr, ADDR_CURRENT_NONCE, );
/*
ADDR_DLL_OFFSET_CFG_LOW
ADDR_DLL_OFFSET_CFG_HIGH


Write dll_offset_cfg_low  address 0x26 :       0xC3C1C200
Write  dll_offset_cfg_high  address 0x27:      0x0082C381

Construct pll_config value:
      pll_config [7:0] = (M-1);
      pll_config [12:8] = (N-1);
      pll_config [18:13] = (P-1);
      pll_config [19] = 0;
      pll_config [20] = 1;


Write pll_config value to pll_config register 0x11
ADDR_PLL_CONFIG

 

Write 0 to pll_en address 0x12
ADDR_PLL_ENABLE

 

Write 1 to pll_en address 0x12
ADDR_PLL_ENABLE

 

Wait 1mS

 

Read pll status:

1)      Broadcast read to 0x54 – should be 0

2)      Unicast pll status 0x1b 0- should be 3
ADDR_PLL_STATUS


*/




}


