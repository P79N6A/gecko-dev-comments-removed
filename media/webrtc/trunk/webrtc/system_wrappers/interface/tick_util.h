












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

#include "webrtc/typedefs.h"

namespace webrtc {

class TickInterval;


class TickTime {
 public:
  TickTime();
  explicit TickTime(WebRtc_Word64 ticks);

  
  static TickTime Now();

  
  static WebRtc_Word64 MillisecondTimestamp();

  
  static WebRtc_Word64 MicrosecondTimestamp();

  
  WebRtc_Word64 Ticks() const;

  static WebRtc_Word64 MillisecondsToTicks(const WebRtc_Word64 ms);

  static WebRtc_Word64 TicksToMilliseconds(const WebRtc_Word64 ticks);

  
  friend TickTime operator+(const TickTime lhs, const WebRtc_Word64 ticks);
  TickTime& operator+=(const WebRtc_Word64& ticks);

  
  friend TickInterval operator-(const TickTime& lhs, const TickTime& rhs);

  
  
  static void UseFakeClock(WebRtc_Word64 start_millisecond);

  
  static void AdvanceFakeClock(WebRtc_Word64 milliseconds);

 private:
  static WebRtc_Word64 QueryOsForTicks();

  static bool use_fake_clock_;
  static WebRtc_Word64 fake_ticks_;

  WebRtc_Word64 ticks_;
};


class TickInterval {
 public:
  TickInterval();

  WebRtc_Word64 Milliseconds() const;
  WebRtc_Word64 Microseconds() const;

  
  friend TickInterval operator+(const TickInterval& lhs,
                                const TickInterval& rhs);
  TickInterval& operator+=(const TickInterval& rhs);

  
  friend TickInterval operator-(const TickInterval& lhs,
                                const TickInterval& rhs);
  TickInterval& operator-=(const TickInterval& rhs);

  friend bool operator>(const TickInterval& lhs, const TickInterval& rhs);
  friend bool operator<=(const TickInterval& lhs, const TickInterval& rhs);
  friend bool operator<(const TickInterval& lhs, const TickInterval& rhs);
  friend bool operator>=(const TickInterval& lhs, const TickInterval& rhs);

 private:
  explicit TickInterval(WebRtc_Word64 interval);

  friend class TickTime;
  friend TickInterval operator-(const TickTime& lhs, const TickTime& rhs);

 private:
  WebRtc_Word64 interval_;
};

inline TickInterval operator+(const TickInterval& lhs,
                              const TickInterval& rhs) {
  return TickInterval(lhs.interval_ + rhs.interval_);
}

inline TickInterval operator-(const TickInterval& lhs,
                              const TickInterval& rhs) {
  return TickInterval(lhs.interval_ - rhs.interval_);
}

inline TickInterval operator-(const TickTime& lhs, const TickTime& rhs) {
  return TickInterval(lhs.ticks_ - rhs.ticks_);
}

inline TickTime operator+(const TickTime lhs, const WebRtc_Word64 ticks) {
  TickTime time = lhs;
  time.ticks_ += ticks;
  return time;
}

inline bool operator>(const TickInterval& lhs, const TickInterval& rhs) {
  return lhs.interval_ > rhs.interval_;
}

inline bool operator<=(const TickInterval& lhs, const TickInterval& rhs) {
  return lhs.interval_ <= rhs.interval_;
}

inline bool operator<(const TickInterval& lhs, const TickInterval& rhs) {
  return lhs.interval_ <= rhs.interval_;
}

inline bool operator>=(const TickInterval& lhs, const TickInterval& rhs) {
  return lhs.interval_ >= rhs.interval_;
}

inline TickTime::TickTime()
    : ticks_(0) {
}

inline TickTime::TickTime(WebRtc_Word64 ticks)
    : ticks_(ticks) {
}

inline TickTime TickTime::Now() {
  if (use_fake_clock_)
    return TickTime(fake_ticks_);
  else
    return TickTime(QueryOsForTicks());
}

inline WebRtc_Word64 TickTime::QueryOsForTicks() {
  TickTime result;
#if _WIN32
  
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  
  
  
  
  
  LARGE_INTEGER qpcnt;
  QueryPerformanceCounter(&qpcnt);
  result.ticks_ = qpcnt.QuadPart;
#else
  static volatile LONG last_time_get_time = 0;
  static volatile WebRtc_Word64 num_wrap_time_get_time = 0;
  volatile LONG* last_time_get_time_ptr = &last_time_get_time;
  DWORD now = timeGetTime();
  
  DWORD old = InterlockedExchange(last_time_get_time_ptr, now);
  if (now < old) {
    
    
    
    
    if (old > 0xf0000000 && now < 0x0fffffff) {
      num_wrap_time_get_time++;
    }
  }
  result.ticks_ = now + (num_wrap_time_get_time << 32);
#endif
#elif defined(WEBRTC_LINUX)
  struct timespec ts;
  
#ifdef WEBRTC_CLOCK_TYPE_REALTIME
  clock_gettime(CLOCK_REALTIME, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
  result.ticks_ = 1000000000LL * static_cast<WebRtc_Word64>(ts.tv_sec) +
      static_cast<WebRtc_Word64>(ts.tv_nsec);
#elif defined(WEBRTC_MAC)
  static mach_timebase_info_data_t timebase;
  if (timebase.denom == 0) {
    
    
    kern_return_t retval = mach_timebase_info(&timebase);
    if (retval != KERN_SUCCESS) {
      
      
#ifndef WEBRTC_IOS
      asm("int3");
#else
      __builtin_trap();
#endif  
    }
  }
  
  result.ticks_ = mach_absolute_time() * timebase.numer / timebase.denom;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  result.ticks_ = 1000000LL * static_cast<WebRtc_Word64>(tv.tv_sec) +
      static_cast<WebRtc_Word64>(tv.tv_usec);
#endif
  return result.ticks_;
}

inline WebRtc_Word64 TickTime::MillisecondTimestamp() {
  WebRtc_Word64 ticks = TickTime::Now().Ticks();
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

inline WebRtc_Word64 TickTime::MicrosecondTimestamp() {
  WebRtc_Word64 ticks = TickTime::Now().Ticks();
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (ticks * 1000) / (qpfreq.QuadPart / 1000);
#else
  return ticks * 1000LL;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
  return ticks / 1000LL;
#else
  return ticks;
#endif
}

inline WebRtc_Word64 TickTime::Ticks() const {
  return ticks_;
}

inline WebRtc_Word64 TickTime::MillisecondsToTicks(const WebRtc_Word64 ms) {
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

inline WebRtc_Word64 TickTime::TicksToMilliseconds(const WebRtc_Word64 ticks) {
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

inline TickTime& TickTime::operator+=(const WebRtc_Word64& ticks) {
  ticks_ += ticks;
  return *this;
}

inline TickInterval::TickInterval() : interval_(0) {
}

inline TickInterval::TickInterval(const WebRtc_Word64 interval)
  : interval_(interval) {
}

inline WebRtc_Word64 TickInterval::Milliseconds() const {
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (interval_ * 1000) / qpfreq.QuadPart;
#else
  
  return interval_;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
  
  return interval_ / 1000000;
#else
  
  return interval_ / 1000;
#endif
}

inline WebRtc_Word64 TickInterval::Microseconds() const {
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (interval_ * 1000000) / qpfreq.QuadPart;
#else
  
  return interval_ * 1000LL;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
  
  return interval_ / 1000;
#else
  
  return interval_;
#endif
}

inline TickInterval& TickInterval::operator+=(const TickInterval& rhs) {
  interval_ += rhs.interval_;
  return *this;
}

inline TickInterval& TickInterval::operator-=(const TickInterval& rhs) {
  interval_ -= rhs.interval_;
  return *this;
}

}  

#endif  
