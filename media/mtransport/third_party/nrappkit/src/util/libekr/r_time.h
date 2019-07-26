


















































































#ifndef _r_time_h
#define _r_time_h

#include <csi_platform.h>

#ifndef WIN32
#include <sys/time.h>
#include <time.h>
#endif

int r_timeval_diff(struct timeval *t1,struct timeval *t0, struct timeval *diff);
int r_timeval_add(struct timeval *t1,struct timeval *t2, struct timeval *sum);
int r_timeval_cmp(struct timeval *t1,struct timeval *t2);

UINT8 r_timeval2int(struct timeval *tv);
int r_int2timeval(UINT8 t,struct timeval *tv);
UINT8 r_gettimeint(void);


int r_timeval_diff_usec(struct timeval *t1, struct timeval *t0, INT8 *diff);


int r_timeval_diff_ms(struct timeval *t1, struct timeval *t0, INT8 *diff);

#endif

