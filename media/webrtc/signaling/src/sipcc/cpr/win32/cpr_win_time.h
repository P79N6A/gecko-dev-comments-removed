






































#ifndef _CPR_WIN_TIME_H_
#define _CPR_WIN_TIME_H_

#include "cpr_types.h"

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <time.h>

typedef time_t cpr_time_t;

unsigned long gmt_string_to_seconds(char *gmt_string, unsigned long *seconds);
long diff_current_time(unsigned long t1, unsigned long *difference);

#endif
