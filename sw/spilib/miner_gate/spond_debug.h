#ifndef ___SPOND_DBG____
#define ___SPOND_DBG____



#define DBG_NET 0
#define DBG_WINS 0
#define DBG_HW 1
#define DBG_SCALING 0
#define DBG_HASH 1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#define DBG(DBG_TYPE, STR...)  if (DBG_TYPE) {printf(STR);}

 


#endif 
 