


















#ifndef _LIBS_UTILS_TIMERS_H
#define _LIBS_UTILS_TIMERS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>




#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t nsecs_t;       

static inline nsecs_t seconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000000000;
}

static inline nsecs_t milliseconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000000;
}

static inline nsecs_t microseconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000;
}

static inline nsecs_t nanoseconds_to_seconds(nsecs_t secs)
{
    return secs/1000000000;
}

static inline nsecs_t nanoseconds_to_milliseconds(nsecs_t secs)
{
    return secs/1000000;
}

static inline nsecs_t nanoseconds_to_microseconds(nsecs_t secs)
{
    return secs/1000;
}

static inline nsecs_t s2ns(nsecs_t v)  {return seconds_to_nanoseconds(v);}
static inline nsecs_t ms2ns(nsecs_t v) {return milliseconds_to_nanoseconds(v);}
static inline nsecs_t us2ns(nsecs_t v) {return microseconds_to_nanoseconds(v);}
static inline nsecs_t ns2s(nsecs_t v)  {return nanoseconds_to_seconds(v);}
static inline nsecs_t ns2ms(nsecs_t v) {return nanoseconds_to_milliseconds(v);}
static inline nsecs_t ns2us(nsecs_t v) {return nanoseconds_to_microseconds(v);}

static inline nsecs_t seconds(nsecs_t v)      { return s2ns(v); }
static inline nsecs_t milliseconds(nsecs_t v) { return ms2ns(v); }
static inline nsecs_t microseconds(nsecs_t v) { return us2ns(v); }

enum {
    SYSTEM_TIME_REALTIME = 0,  
    SYSTEM_TIME_MONOTONIC = 1, 
    SYSTEM_TIME_PROCESS = 2,   
    SYSTEM_TIME_THREAD = 3,    
    SYSTEM_TIME_BOOTTIME = 4   
};


#ifdef __cplusplus
nsecs_t systemTime(int clock = SYSTEM_TIME_MONOTONIC);
#else
nsecs_t systemTime(int clock);
#endif 









int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime);

#ifdef __cplusplus
} 
#endif




#ifdef __cplusplus

namespace android {





class DurationTimer {
public:
    DurationTimer() {}
    ~DurationTimer() {}

    
    void start();
    
    void stop();
    
    long long durationUsecs() const;

    
    
    static long long subtractTimevals(const struct timeval* ptv1,
        const struct timeval* ptv2);

    
    static void addToTimeval(struct timeval* ptv, long usec);

private:
    struct timeval  mStartWhen;
    struct timeval  mStopWhen;
};

}; 
#endif 

#endif
