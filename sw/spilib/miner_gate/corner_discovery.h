#ifndef _____DC2CORNERDISC__45R_H____
#define _____DC2CORNERDISC__45R_H____


#include "nvm.h"
#include "hammer.h"
#include "defines.h"


// Functions assumes in NVM loops and present ASICs 
// discovered, but max_freq/corner/engines not detected
void discover_good_loops();
void compute_corners();
const char* corner_to_collor(ASIC_CORNER c);
void enable_voltage_freq(int vtrim, ASIC_FREQ f);
void set_working_voltage_discover_top_speeds() ;



#endif
