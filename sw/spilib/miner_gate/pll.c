
#include <stdio.h>
#include <stdlib.h>

#include "pll.h"
#include "hammer.h"
#include "squid.h"
#include "spond_debug.h"


pll_frequency_settings pfs[ASIC_FREQ_MAX] = {
    	{0,0,0},
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
#ifdef HAS_PLL
	write_reg_device(addr, ADDR_DLL_OFFSET_CFG_LOW, 0xC3C1C200);
	write_reg_device(addr, ADDR_DLL_OFFSET_CFG_LOW, 0x0082C381);
	passert(freq < ASIC_FREQ_MAX);
	pll_frequency_settings *ppfs = &pfs[freq];
	uint32_t pll_config = 0;
	uint32_t M = ppfs->m_mult;
	uint32_t N = 1;
	uint32_t P = ppfs->p_div;

	
	pll_config = (M-1)&0xFF;
	pll_config |= (N-1)&0xF << 8;
	pll_config |= (P-1)&0x1F << 13;
	write_reg_device(addr, ADDR_PLL_CONFIG, 0x0082C381);
	write_reg_device(addr, ADDR_PLL_ENABLE, 0x0);
	write_reg_device(addr, ADDR_PLL_ENABLE, 0x1);

	while(
		(read_reg_device(addr,0x54) != 0) &&
		(read_reg_device(addr,ADDR_PLL_STATUS) != 3)) {
		printf("Waiting PLL TODO make global\n");
		usleep(50);
	}
#endif
	//write_reg_device(addr, ADDR_CURRENT_NONCE, );
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


