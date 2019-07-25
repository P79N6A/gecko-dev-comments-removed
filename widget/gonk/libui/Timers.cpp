


















#include "utils_Log.h"
#include "Timers.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#ifdef HAVE_WIN32_THREADS
#include <windows.h>
#endif

nsecs_t systemTime(int clock)
{
#if defined(HAVE_POSIX_CLOCKS)
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[clock], &t);
    return nsecs_t(t.tv_sec)*1000000000LL + t.tv_nsec;
#else
    
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return nsecs_t(t.tv_sec)*1000000000LL + nsecs_t(t.tv_usec)*1000LL;
#endif
}

int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime)
{
    int timeoutDelayMillis;
    if (timeoutTime > referenceTime) {
        uint64_t timeoutDelay = uint64_t(timeoutTime - referenceTime);
        if (timeoutDelay > uint64_t((INT_MAX - 1) * 1000000LL)) {
            timeoutDelayMillis = -1;
        } else {
            timeoutDelayMillis = (timeoutDelay + 999999LL) / 1000000LL;
        }
    } else {
        timeoutDelayMillis = 0;
    }
    return timeoutDelayMillis;
}








using namespace android;


void DurationTimer::start(void)
{
    gettimeofday(&mStartWhen, NULL);
}


void DurationTimer::stop(void)
{
    gettimeofday(&mStopWhen, NULL);
}


long long DurationTimer::durationUsecs(void) const
{
    return (long) subtractTimevals(&mStopWhen, &mStartWhen);
}



 long long DurationTimer::subtractTimevals(const struct timeval* ptv1,
    const struct timeval* ptv2)
{
    long long stop  = ((long long) ptv1->tv_sec) * 1000000LL +
                      ((long long) ptv1->tv_usec);
    long long start = ((long long) ptv2->tv_sec) * 1000000LL +
                      ((long long) ptv2->tv_usec);
    return stop - start;
}


 void DurationTimer::addToTimeval(struct timeval* ptv, long usec)
{
    if (usec < 0) {
        ALOG(LOG_WARN, "", "Negative values not supported in addToTimeval\n");
        return;
    }

    
    if (ptv->tv_usec >= 1000000) {
        ptv->tv_sec += ptv->tv_usec / 1000000;
        ptv->tv_usec %= 1000000;
    }

    ptv->tv_usec += usec % 1000000;
    if (ptv->tv_usec >= 1000000) {
        ptv->tv_usec -= 1000000;
        ptv->tv_sec++;
    }
    ptv->tv_sec += usec / 1000000;
}

