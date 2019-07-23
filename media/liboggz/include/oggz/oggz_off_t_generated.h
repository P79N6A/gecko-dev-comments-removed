































#ifndef __OGGZ_OFF_T_GENERATED_H__
#define __OGGZ_OFF_T_GENERATED_H__

























#include <sys/types.h>

#if defined(__APPLE__) || defined(SOLARIS) || defined(OS2)
typedef off_t oggz_off_t;
#else
typedef loff_t oggz_off_t;
#endif

#define PRI_OGGZ_OFF_T "PRId64"

#endif 
