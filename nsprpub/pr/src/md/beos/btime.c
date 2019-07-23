




































#include "primpl.h"
#include <kernel/OS.h>

static bigtime_t start;

PRTime
_MD_now (void)
{
    return (PRTime)real_time_clock_usecs();
}

void
_MD_interval_init (void)
{
    
    start = real_time_clock_usecs();
}

PRIntervalTime
_MD_get_interval (void)
{
    return( (PRIntervalTime) real_time_clock_usecs() / 10 );

#if 0
    

    bigtime_t now = real_time_clock_usecs();
    now -= start;
    now /= 10;
    return (PRIntervalTime)now;
#endif
}

PRIntervalTime
_MD_interval_per_sec (void)
{
    return 100000L;
}
