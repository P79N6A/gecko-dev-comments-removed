































#ifndef __OGGZ_OFF_T_H__
#define __OGGZ_OFF_T_H__






#ifdef _WIN32
#ifdef WINCE
   typedef long off_t;
#endif

  
   typedef off_t oggz_off_t;

#define PRI_OGGZ_OFF_T "l"

#else
#include <oggz/oggz_off_t_generated.h>
#endif

#endif 
