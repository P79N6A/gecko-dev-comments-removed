









#include "webrtc/system_wrappers/interface/clock.h"

#if defined(_WIN32)

#include <Windows.h>
#include <WinSock.h>
#include <MMSystem.h>
#elif ((defined WEBRTC_LINUX) || (defined WEBRTC_BSD) || (defined WEBRTC_MAC))
#include <sys/time.h>
#include <time.h>
#endif

#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc {

const double kNtpFracPerMs = 4.294967296E6;

int64_t Clock::NtpToMs(uint32_t ntp_secs, uint32_t ntp_frac) {
  const double ntp_frac_ms = static_cast<double>(ntp_frac) / kNtpFracPerMs;
  return 1000 * static_cast<int64_t>(ntp_secs) +
      static_cast<int64_t>(ntp_frac_ms + 0.5);
}

#if defined(_WIN32)

struct reference_point {
  FILETIME      file_time;
  LARGE_INTEGER counterMS;
};

struct WindowsHelpTimer {
  volatile LONG _timeInMs;
  volatile LONG _numWrapTimeInMs;
  reference_point _ref_point;

  volatile LONG _sync_flag;
};

void Synchronize(WindowsHelpTimer* help_timer) {
  const LONG start_value = 0;
  const LONG new_value = 1;
  const LONG synchronized_value = 2;

  LONG compare_flag = new_value;
  while (help_timer->_sync_flag == start_value) {
    const LONG new_value = 1;
    compare_flag = InterlockedCompareExchange(
        &help_timer->_sync_flag, new_value, start_value);
  }
  if (compare_flag != start_value) {
    
    
    while (compare_flag != synchronized_value) {
      ::Sleep(0);
    }
    return;
  }
  
  

  
  timeBeginPeriod(1);
  FILETIME    ft0 = { 0, 0 },
              ft1 = { 0, 0 };
  
  
  
  
  ::GetSystemTimeAsFileTime(&ft0);
  do {
    ::GetSystemTimeAsFileTime(&ft1);

    help_timer->_ref_point.counterMS.QuadPart = ::timeGetTime();
    ::Sleep(0);
  } while ((ft0.dwHighDateTime == ft1.dwHighDateTime) &&
          (ft0.dwLowDateTime == ft1.dwLowDateTime));
  help_timer->_ref_point.file_time = ft1;
  timeEndPeriod(1);
}

void get_time(WindowsHelpTimer* help_timer, FILETIME& current_time) {
  
  DWORD t = timeGetTime();
  
  
  volatile LONG* timeInMsPtr = &help_timer->_timeInMs;
  
  DWORD old = InterlockedExchange(timeInMsPtr, t);
  if(old > t) {
    
    help_timer->_numWrapTimeInMs++;
  }
  LARGE_INTEGER elapsedMS;
  elapsedMS.HighPart = help_timer->_numWrapTimeInMs;
  elapsedMS.LowPart = t;

  elapsedMS.QuadPart = elapsedMS.QuadPart -
      help_timer->_ref_point.counterMS.QuadPart;

  
  
  ULARGE_INTEGER filetime_ref_as_ul;

  filetime_ref_as_ul.HighPart =
      help_timer->_ref_point.file_time.dwHighDateTime;
  filetime_ref_as_ul.LowPart =
      help_timer->_ref_point.file_time.dwLowDateTime;
  filetime_ref_as_ul.QuadPart +=
      (ULONGLONG)((elapsedMS.QuadPart)*1000*10);

  
  current_time.dwHighDateTime = filetime_ref_as_ul.HighPart;
  current_time.dwLowDateTime = filetime_ref_as_ul.LowPart;
}
#endif

class RealTimeClock : public Clock {
  
  
  virtual int64_t TimeInMilliseconds() OVERRIDE {
    return TickTime::MillisecondTimestamp();
  }

  
  
  virtual int64_t TimeInMicroseconds() OVERRIDE {
    return TickTime::MicrosecondTimestamp();
  }

  
  virtual void CurrentNtp(uint32_t& seconds, uint32_t& fractions) OVERRIDE {
    timeval tv = CurrentTimeVal();
    double microseconds_in_seconds;
    Adjust(tv, &seconds, &microseconds_in_seconds);
    fractions = static_cast<uint32_t>(
        microseconds_in_seconds * kMagicNtpFractionalUnit + 0.5);
  }

  
  virtual int64_t CurrentNtpInMilliseconds() OVERRIDE {
    timeval tv = CurrentTimeVal();
    uint32_t seconds;
    double microseconds_in_seconds;
    Adjust(tv, &seconds, &microseconds_in_seconds);
    return 1000 * static_cast<int64_t>(seconds) +
        static_cast<int64_t>(1000.0 * microseconds_in_seconds + 0.5);
  }

 protected:
  virtual timeval CurrentTimeVal() const = 0;

  static void Adjust(const timeval& tv, uint32_t* adjusted_s,
                     double* adjusted_us_in_s) {
    *adjusted_s = tv.tv_sec + kNtpJan1970;
    *adjusted_us_in_s = tv.tv_usec / 1e6;

    if (*adjusted_us_in_s >= 1) {
      *adjusted_us_in_s -= 1;
      ++*adjusted_s;
    } else if (*adjusted_us_in_s < -1) {
      *adjusted_us_in_s += 1;
      --*adjusted_s;
    }
  }
};

#if defined(_WIN32)
class WindowsRealTimeClock : public RealTimeClock {
 public:
  WindowsRealTimeClock(WindowsHelpTimer* helpTimer)
      : _helpTimer(helpTimer) {}

  virtual ~WindowsRealTimeClock() {}

 protected:
  virtual timeval CurrentTimeVal() const OVERRIDE {
    const uint64_t FILETIME_1970 = 0x019db1ded53e8000;

    FILETIME StartTime;
    uint64_t Time;
    struct timeval tv;

    
    
    get_time(_helpTimer, StartTime);

    Time = (((uint64_t) StartTime.dwHighDateTime) << 32) +
           (uint64_t) StartTime.dwLowDateTime;

    
    Time -= FILETIME_1970;

    tv.tv_sec = (uint32_t)(Time / (uint64_t)10000000);
    tv.tv_usec = (uint32_t)((Time % (uint64_t)10000000) / 10);
    return tv;
  }

  WindowsHelpTimer* _helpTimer;
};

#elif ((defined WEBRTC_LINUX) || (defined WEBRTC_BSD) || (defined WEBRTC_MAC))
class UnixRealTimeClock : public RealTimeClock {
 public:
  UnixRealTimeClock() {}

  virtual ~UnixRealTimeClock() {}

 protected:
  virtual timeval CurrentTimeVal() const OVERRIDE {
    struct timeval tv;
    struct timezone tz;
    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;
    gettimeofday(&tv, &tz);
    return tv;
  }
};
#endif


#if defined(_WIN32)













static WindowsHelpTimer *SyncGlobalHelpTimer() {
  static WindowsHelpTimer global_help_timer = {0, 0, {{ 0, 0}, 0}, 0};
  Synchronize(&global_help_timer);
  return &global_help_timer;
}
#endif

Clock* Clock::GetRealTimeClock() {
#if defined(_WIN32)
  static WindowsRealTimeClock clock(SyncGlobalHelpTimer());
  return &clock;
#elif ((defined WEBRTC_LINUX) || (defined WEBRTC_BSD) || (defined WEBRTC_MAC))
  static UnixRealTimeClock clock;
  return &clock;
#else
  return NULL;
#endif
}

SimulatedClock::SimulatedClock(int64_t initial_time_us)
    : time_us_(initial_time_us) {}

int64_t SimulatedClock::TimeInMilliseconds() {
  return (time_us_ + 500) / 1000;
}

int64_t SimulatedClock::TimeInMicroseconds() {
  return time_us_;
}

void SimulatedClock::CurrentNtp(uint32_t& seconds, uint32_t& fractions) {
  seconds = (TimeInMilliseconds() / 1000) + kNtpJan1970;
  fractions = (uint32_t)((TimeInMilliseconds() % 1000) *
      kMagicNtpFractionalUnit / 1000);
}

int64_t SimulatedClock::CurrentNtpInMilliseconds() {
  return TimeInMilliseconds() + 1000 * static_cast<int64_t>(kNtpJan1970);
}

void SimulatedClock::AdvanceTimeMilliseconds(int64_t milliseconds) {
  AdvanceTimeMicroseconds(1000 * milliseconds);
}

void SimulatedClock::AdvanceTimeMicroseconds(int64_t microseconds) {
  time_us_ += microseconds;
}

};  
