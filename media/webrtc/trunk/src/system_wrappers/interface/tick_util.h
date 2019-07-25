












#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TICK_UTIL_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TICK_UTIL_H_

#if _WIN32
#include <windows.h>
#include <mmsystem.h>
#elif WEBRTC_LINUX
#include <ctime>
#elif WEBRTC_MAC
#include <mach/mach_time.h>
#include <string.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include "typedefs.h"

namespace webrtc {
class TickInterval;

class TickTime
{
public:
    
    static TickTime Now();

    
    static WebRtc_Word64 MillisecondTimestamp();

    
    static WebRtc_Word64 MicrosecondTimestamp();

    WebRtc_Word64 Ticks() const;

    static WebRtc_Word64 MillisecondsToTicks(const WebRtc_Word64 ms);

    static WebRtc_Word64 TicksToMilliseconds(const WebRtc_Word64 ticks);

    
    friend TickTime operator+(const TickTime lhs, const WebRtc_Word64 ticks);
    TickTime& operator+=(const WebRtc_Word64& rhs);


    
    friend TickInterval operator-(const TickTime& lhs, const TickTime& rhs);
private:
    WebRtc_Word64 _ticks;
};

class TickInterval
{
public:
    TickInterval();

    WebRtc_Word64 Milliseconds() const;
    WebRtc_Word64 Microseconds() const;

    
    friend TickInterval operator+(const TickInterval& lhs,
                                  const TickInterval& rhs);
    TickInterval& operator-=(const TickInterval& rhs);

    
    friend TickInterval operator-(const TickInterval& lhs,
                                  const TickInterval& rhs);
    TickInterval& operator+=(const TickInterval& rhs);

    friend bool operator>(const TickInterval& lhs, const TickInterval& rhs);
    friend bool operator<=(const TickInterval& lhs, const TickInterval& rhs);
    friend bool operator<(const TickInterval& lhs, const TickInterval& rhs);
    friend bool operator>=(const TickInterval& lhs, const TickInterval& rhs);

private:
    TickInterval(WebRtc_Word64 interval);

    friend class TickTime;
    friend TickInterval operator-(const TickTime& lhs, const TickTime& rhs);

private:
    WebRtc_Word64 _interval;
};

inline TickInterval operator+(const TickInterval& lhs, const TickInterval& rhs)
{
    return TickInterval(lhs._interval + rhs._interval);
}

inline TickInterval operator-(const TickInterval& lhs, const TickInterval& rhs)
{
    return TickInterval(lhs._interval - rhs._interval);
}

inline TickInterval operator-(const TickTime& lhs,const TickTime& rhs)
{
    return TickInterval(lhs._ticks - rhs._ticks);
}

inline TickTime operator+(const TickTime lhs, const WebRtc_Word64 ticks)
{
    TickTime time = lhs;
    time._ticks += ticks;
    return time;
}
inline bool operator>(const TickInterval& lhs, const TickInterval& rhs)
{
    return lhs._interval > rhs._interval;
}
inline bool operator<=(const TickInterval& lhs, const TickInterval& rhs)
{
    return lhs._interval <= rhs._interval;
}
inline bool operator<(const TickInterval& lhs, const TickInterval& rhs)
{
    return lhs._interval <= rhs._interval;
}
inline bool operator>=(const TickInterval& lhs, const TickInterval& rhs)
{
    return lhs._interval >= rhs._interval;
}

inline TickTime TickTime::Now()
{
    TickTime result;
#if _WIN32
    
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        
        
        
        
        
        LARGE_INTEGER qpcnt;
        QueryPerformanceCounter(&qpcnt);
        result._ticks = qpcnt.QuadPart;
    #else
        static volatile LONG lastTimeGetTime = 0;
        static volatile WebRtc_Word64 numWrapTimeGetTime = 0;
        volatile LONG* lastTimeGetTimePtr = &lastTimeGetTime;
        DWORD now = timeGetTime();
        
        DWORD old = InterlockedExchange(lastTimeGetTimePtr, now);
        if(now < old)
        {
            
            
            
            
            if(old > 0xf0000000 && now < 0x0fffffff) 
            {
                numWrapTimeGetTime++;
            }
        }
        result._ticks = now + (numWrapTimeGetTime<<32);
    #endif
#elif defined(WEBRTC_LINUX)
    struct timespec ts;
    
    #ifdef WEBRTC_CLOCK_TYPE_REALTIME
        clock_gettime(CLOCK_REALTIME, &ts);
    #else
        clock_gettime(CLOCK_MONOTONIC, &ts);
    #endif
    result._ticks = 1000000000LL * static_cast<WebRtc_Word64>(ts.tv_sec) + static_cast<WebRtc_Word64>(ts.tv_nsec);
#elif defined(WEBRTC_MAC)
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
      
      
      kern_return_t retval = mach_timebase_info(&timebase);
      if (retval != KERN_SUCCESS) {
        
        
        asm("int3");
      }
    }
    
    result._ticks = mach_absolute_time() * timebase.numer / timebase.denom;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    result._ticks = 1000000LL * static_cast<WebRtc_Word64>(tv.tv_sec) + static_cast<WebRtc_Word64>(tv.tv_usec);
#endif
    return result;
}

inline WebRtc_Word64 TickTime::MillisecondTimestamp()
{
    TickTime now = TickTime::Now();
#if _WIN32
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (now._ticks * 1000) / qpfreq.QuadPart;
    #else
        return now._ticks;
    #endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
    return now._ticks / 1000000LL;
#else
    return now._ticks / 1000LL;
#endif
}

inline WebRtc_Word64 TickTime::MicrosecondTimestamp()
{
    TickTime now = TickTime::Now();

#if _WIN32
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (now._ticks * 1000) / (qpfreq.QuadPart/1000);
    #else
        return now._ticks *1000LL;
    #endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
    return now._ticks / 1000LL;
#else
    return now._ticks;
#endif
}

inline WebRtc_Word64 TickTime::Ticks() const
{
    return _ticks;
}

inline WebRtc_Word64 TickTime::MillisecondsToTicks(const WebRtc_Word64 ms)
{
#if _WIN32
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (qpfreq.QuadPart * ms) / 1000;
    #else
        return ms;
    #endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
    return ms * 1000000LL;
#else
    return ms * 1000LL;
#endif
}

inline WebRtc_Word64 TickTime::TicksToMilliseconds(const WebRtc_Word64 ticks)
{
#if _WIN32
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (ticks * 1000) / qpfreq.QuadPart;
    #else
        return ticks;
    #endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
    return ticks / 1000000LL;
#else
    return ticks / 1000LL;
#endif
}

inline TickTime& TickTime::operator+=(const WebRtc_Word64& ticks)
{
    _ticks += ticks;
    return *this;
}

inline TickInterval::TickInterval() : _interval(0)
{
}

inline TickInterval::TickInterval(const WebRtc_Word64 interval)
    : _interval(interval)
{
}

inline WebRtc_Word64 TickInterval::Milliseconds() const
{
#if _WIN32
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (_interval * 1000) / qpfreq.QuadPart;
    #else
	
        return _interval;
    #endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
    
    return _interval / 1000000;
#else
    
    return _interval / 1000;
#endif
}

inline WebRtc_Word64 TickInterval::Microseconds() const
{
#if _WIN32
    #ifdef USE_QUERY_PERFORMANCE_COUNTER
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (_interval * 1000000) / qpfreq.QuadPart;
    #else
	
        return _interval *1000LL;
    #endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
    
    return _interval / 1000;
#else
    
    return _interval;
#endif
}

inline TickInterval& TickInterval::operator+=(const TickInterval& rhs)
{
    _interval += rhs._interval;
    return *this;
}

inline TickInterval& TickInterval::operator-=(const TickInterval& rhs)
{
    _interval -= rhs._interval;
    return *this;
}
} 

#endif 
