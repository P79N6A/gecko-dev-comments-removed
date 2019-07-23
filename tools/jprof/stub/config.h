


































#ifndef config_h___
#define config_h___

#define MAX_STACK_CRAWL 500
#define M_LOGFILE "jprof-log"
#define M_MAPFILE "jprof-map"

#if defined(linux) || defined(NTO)
#define USE_BFD
#undef NEED_WRAPPERS

#endif 

#endif 
