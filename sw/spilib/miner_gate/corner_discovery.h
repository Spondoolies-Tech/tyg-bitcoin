#ifndef _____DC2CORNERDISC__45R_H____
#define _____DC2CORNERDISC__45R_H____


#include "nvm.h"
#include "hammer.h"
#include "defines.h"


// Functions assumes in NVM loops and present ASICs 
// discovered, but max_freq/corner/engines not detected
void recompute_corners_and_voltage_update_nvm();
void find_bad_engines_update_nvm();
void discover_good_loops();


#endif
