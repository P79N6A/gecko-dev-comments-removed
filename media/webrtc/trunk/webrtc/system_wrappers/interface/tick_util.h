












#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TICK_UTIL_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TICK_UTIL_H_

#if _WIN32

#include <windows.h>
#include <mmsystem.h>
#elif WEBRTC_LINUX
#include <time.h>
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
  explicit TickTime(int64_t ticks);

  
  static TickTime Now();

  
  static int64_t MillisecondTimestamp();

  
  static int64_t MicrosecondTimestamp();

  
  int64_t Ticks() const;

  static int64_t MillisecondsToTicks(const int64_t ms);

  static int64_t TicksToMilliseconds(const int64_t ticks);

  
  friend TickTime operator+(const TickTime lhs, const int64_t ticks);
  TickTime& operator+=(const int64_t& ticks);

  
  friend TickInterval operator-(const TickTime& lhs, const TickTime& rhs);

  
  
  static void UseFakeClock(int64_t start_millisecond);

  
  static void AdvanceFakeClock(int64_t milliseconds);

 private:
  static int64_t QueryOsForTicks();

  static bool use_fake_clock_;
  static int64_t fake_ticks_;

  int64_t ticks_;
};


class TickInterval {
 public:
  TickInterval();

  int64_t Milliseconds() const;
  int64_t Microseconds() const;

  
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
  explicit TickInterval(int64_t interval);

  friend class TickTime;
  friend TickInterval operator-(const TickTime& lhs, const TickTime& rhs);

 private:
  int64_t interval_;
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

inline TickTime operator+(const TickTime lhs, const int64_t ticks) {
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

inline TickTime::TickTime(int64_t ticks)
    : ticks_(ticks) {
}

inline TickTime TickTime::Now() {
  if (use_fake_clock_)
    return TickTime(fake_ticks_);
  else
    return TickTime(QueryOsForTicks());
}

inline int64_t TickTime::MillisecondTimestamp() {
  int64_t ticks = TickTime::Now().Ticks();
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (ticks * 1000) / qpfreq.QuadPart;
#else
  return ticks;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
  return ticks / 1000000LL;
#else
  return ticks / 1000LL;
#endif
}

inline int64_t TickTime::MicrosecondTimestamp() {
  int64_t ticks = TickTime::Now().Ticks();
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (ticks * 1000) / (qpfreq.QuadPart / 1000);
#else
  return ticks * 1000LL;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
  return ticks / 1000LL;
#else
  return ticks;
#endif
}

inline int64_t TickTime::Ticks() const {
  return ticks_;
}

inline int64_t TickTime::MillisecondsToTicks(const int64_t ms) {
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (qpfreq.QuadPart * ms) / 1000;
#else
  return ms;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
  return ms * 1000000LL;
#else
  return ms * 1000LL;
#endif
}

inline int64_t TickTime::TicksToMilliseconds(const int64_t ticks) {
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (ticks * 1000) / qpfreq.QuadPart;
#else
  return ticks;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
  return ticks / 1000000LL;
#else
  return ticks / 1000LL;
#endif
}

inline TickTime& TickTime::operator+=(const int64_t& ticks) {
  ticks_ += ticks;
  return *this;
}

inline TickInterval::TickInterval() : interval_(0) {
}

inline TickInterval::TickInterval(const int64_t interval)
  : interval_(interval) {
}

inline int64_t TickInterval::Milliseconds() const {
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (interval_ * 1000) / qpfreq.QuadPart;
#else
  
  return interval_;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
  
  return interval_ / 1000000;
#else
  
  return interval_ / 1000;
#endif
}

inline int64_t TickInterval::Microseconds() const {
#if _WIN32
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  LARGE_INTEGER qpfreq;
  QueryPerformanceFrequency(&qpfreq);
  return (interval_ * 1000000) / qpfreq.QuadPart;
#else
  
  return interval_ * 1000LL;
#endif
#elif defined(WEBRTC_LINUX) || defined(WEBRTC_BSD) || defined(WEBRTC_MAC)
  
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
