



































#include "base/time.h"

#pragma comment(lib, "winmm.lib")
#include <windows.h>
#include <mmsystem.h>

#include "base/basictypes.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/cpu.h"
#include "base/singleton.h"
#include "mozilla/Casting.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;
using mozilla::BitwiseCast;

namespace {



int64_t FileTimeToMicroseconds(const FILETIME& ft) {
  
  
  
  return BitwiseCast<int64_t>(ft) / 10;
}

void MicrosecondsToFileTime(int64_t us, FILETIME* ft) {
  DCHECK(us >= 0) << "Time is less than 0, negative values are not "
      "representable in FILETIME";

  
  
  *ft = BitwiseCast<FILETIME>(us * 10);
}

int64_t CurrentWallclockMicroseconds() {
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  return FileTimeToMicroseconds(ft);
}


const int kMaxMillisecondsToAvoidDrift = 60 * Time::kMillisecondsPerSecond;

int64_t initial_time = 0;
TimeTicks initial_ticks;

void InitializeClock() {
  initial_ticks = TimeTicks::Now();
  initial_time = CurrentWallclockMicroseconds();
}

}  








const int64_t Time::kTimeTToMicrosecondsOffset = GG_INT64_C(11644473600000000);


Time Time::Now() {
  if (initial_time == 0)
    InitializeClock();

  
  
  
  
  
  
  
  
  
  
  while(true) {
    TimeTicks ticks = TimeTicks::Now();

    
    TimeDelta elapsed = ticks - initial_ticks;

    
    if (elapsed.InMilliseconds() > kMaxMillisecondsToAvoidDrift) {
      InitializeClock();
      continue;
    }

    return Time(elapsed + Time(initial_time));
  }
}


Time Time::NowFromSystemTime() {
  
  InitializeClock();
  return Time(initial_time);
}


Time Time::FromFileTime(FILETIME ft) {
  return Time(FileTimeToMicroseconds(ft));
}

FILETIME Time::ToFileTime() const {
  FILETIME utc_ft;
  MicrosecondsToFileTime(us_, &utc_ft);
  return utc_ft;
}


Time Time::FromExploded(bool is_local, const Exploded& exploded) {
  
  
  SYSTEMTIME st;
  st.wYear = exploded.year;
  st.wMonth = exploded.month;
  st.wDayOfWeek = exploded.day_of_week;
  st.wDay = exploded.day_of_month;
  st.wHour = exploded.hour;
  st.wMinute = exploded.minute;
  st.wSecond = exploded.second;
  st.wMilliseconds = exploded.millisecond;

  
  FILETIME ft;
  if (!SystemTimeToFileTime(&st, &ft)) {
    NOTREACHED() << "Unable to convert time";
    return Time(0);
  }

  
  if (is_local) {
    FILETIME utc_ft;
    LocalFileTimeToFileTime(&ft, &utc_ft);
    return Time(FileTimeToMicroseconds(utc_ft));
  }
  return Time(FileTimeToMicroseconds(ft));
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  
  FILETIME utc_ft;
  MicrosecondsToFileTime(us_, &utc_ft);

  
  BOOL success = TRUE;
  FILETIME ft;
  if (is_local)
    success = FileTimeToLocalFileTime(&utc_ft, &ft);
  else
    ft = utc_ft;

  
  SYSTEMTIME st;
  if (!success || !FileTimeToSystemTime(&ft, &st)) {
    NOTREACHED() << "Unable to convert time, don't know why";
    ZeroMemory(exploded, sizeof(*exploded));
    return;
  }

  exploded->year = st.wYear;
  exploded->month = st.wMonth;
  exploded->day_of_week = st.wDayOfWeek;
  exploded->day_of_month = st.wDay;
  exploded->hour = st.wHour;
  exploded->minute = st.wMinute;
  exploded->second = st.wSecond;
  exploded->millisecond = st.wMilliseconds;
}


namespace {




DWORD timeGetTimeWrapper() {
  return timeGetTime();
}


DWORD (*tick_function)(void) = &timeGetTimeWrapper;






class NowSingleton {
 public:
  NowSingleton()
    : rollover_(TimeDelta::FromMilliseconds(0)),
      last_seen_(0) {
  }

  TimeDelta Now() {
    AutoLock locked(lock_);
    
    
    DWORD now = tick_function();
    if (now < last_seen_)
      rollover_ += TimeDelta::FromMilliseconds(GG_LONGLONG(0x100000000));  
    last_seen_ = now;
    return TimeDelta::FromMilliseconds(now) + rollover_;
  }

 private:
  Lock lock_;  
  TimeDelta rollover_;  
  DWORD last_seen_;  

  DISALLOW_COPY_AND_ASSIGN(NowSingleton);
};





























class HighResNowSingleton {
 public:
  HighResNowSingleton()
    : ticks_per_microsecond_(0.0),
      skew_(0) {
    InitializeClock();

    
    
    base::CPU cpu;
    if (cpu.vendor_name() == "AuthenticAMD" && cpu.family() == 15)
      DisableHighResClock();
  }

  bool IsUsingHighResClock() {
    return ticks_per_microsecond_ != 0.0;
  }

  void DisableHighResClock() {
    ticks_per_microsecond_ = 0.0;
  }

  TimeDelta Now() {
    
    const int kMaxTimeDrift = 50 * Time::kMicrosecondsPerMillisecond;

    if (IsUsingHighResClock()) {
      int64_t now = UnreliableNow();

      
      DCHECK(now - ReliableNow() - skew_ < kMaxTimeDrift);

      return TimeDelta::FromMicroseconds(now);
    }

    
    return Singleton<NowSingleton>::get()->Now();
  }

 private:
  
  void InitializeClock() {
    LARGE_INTEGER ticks_per_sec = {0};
    if (!QueryPerformanceFrequency(&ticks_per_sec))
      return;  
    ticks_per_microsecond_ = static_cast<float>(ticks_per_sec.QuadPart) /
      static_cast<float>(Time::kMicrosecondsPerSecond);

    skew_ = UnreliableNow() - ReliableNow();
  }

  
  int64_t UnreliableNow() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<int64_t>(now.QuadPart / ticks_per_microsecond_);
  }

  
  int64_t ReliableNow() {
    return Singleton<NowSingleton>::get()->Now().InMicroseconds();
  }

  
  
  float ticks_per_microsecond_;  
  int64_t skew_;  

  DISALLOW_COPY_AND_ASSIGN(HighResNowSingleton);
};

}  


TimeTicks::TickFunctionType TimeTicks::SetMockTickFunction(
    TickFunctionType ticker) {
  TickFunctionType old = tick_function;
  tick_function = ticker;
  return old;
}


TimeTicks TimeTicks::Now() {
  return TimeTicks() + Singleton<NowSingleton>::get()->Now();
}


TimeTicks TimeTicks::HighResNow() {
  return TimeTicks() + Singleton<HighResNowSingleton>::get()->Now();
}
