



































#include "base/time.h"

#pragma comment(lib, "winmm.lib")
#include <windows.h>
#include <mmsystem.h>

#include "base/basictypes.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/cpu.h"
#include "base/singleton.h"
#include "base/system_monitor.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

namespace {



int64 FileTimeToMicroseconds(const FILETIME& ft) {
  
  
  
  return bit_cast<int64, FILETIME>(ft) / 10;
}

void MicrosecondsToFileTime(int64 us, FILETIME* ft) {
  DCHECK(us >= 0) << "Time is less than 0, negative values are not "
      "representable in FILETIME";

  
  
  *ft = bit_cast<FILETIME, int64>(us * 10);
}

int64 CurrentWallclockMicroseconds() {
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  return FileTimeToMicroseconds(ft);
}


const int kMaxMillisecondsToAvoidDrift = 60 * Time::kMillisecondsPerSecond;

int64 initial_time = 0;
TimeTicks initial_ticks;

void InitializeClock() {
  initial_ticks = TimeTicks::Now();
  initial_time = CurrentWallclockMicroseconds();
}

}  








const int64 Time::kTimeTToMicrosecondsOffset = GG_INT64_C(11644473600000000);


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

    return Time(elapsed + initial_time);
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
    ZeroMemory(exploded, sizeof(exploded));
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






class NowSingleton : public base::SystemMonitor::PowerObserver {
 public:
  NowSingleton()
    : rollover_(TimeDelta::FromMilliseconds(0)),
      last_seen_(0),
      hi_res_clock_enabled_(false) {
    base::SystemMonitor* system = base::SystemMonitor::Get();
    system->AddObserver(this);
    UseHiResClock(!system->BatteryPower());
  }

  ~NowSingleton() {
    UseHiResClock(false);
    base::SystemMonitor* monitor = base::SystemMonitor::Get();
    if (monitor)
      monitor->RemoveObserver(this);
  }

  TimeDelta Now() {
    AutoLock locked(lock_);
    
    
    DWORD now = tick_function();
    if (now < last_seen_)
      rollover_ += TimeDelta::FromMilliseconds(0x100000000I64);  
    last_seen_ = now;
    return TimeDelta::FromMilliseconds(now) + rollover_;
  }

  
  void OnPowerStateChange(base::SystemMonitor* system) {
    UseHiResClock(!system->BatteryPower());
  }

  void OnSuspend(base::SystemMonitor* system) {}
  void OnResume(base::SystemMonitor* system) {}

 private:
  
  void UseHiResClock(bool enabled) {
    if (enabled == hi_res_clock_enabled_)
      return;
    if (enabled)
      timeBeginPeriod(1);
    else
      timeEndPeriod(1);
    hi_res_clock_enabled_ = enabled;
  }

  Lock lock_;  
  TimeDelta rollover_;  
  DWORD last_seen_;  
  bool hi_res_clock_enabled_;

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
      int64 now = UnreliableNow();

      
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

  
  int64 UnreliableNow() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<int64>(now.QuadPart / ticks_per_microsecond_);
  }

  
  int64 ReliableNow() {
    return Singleton<NowSingleton>::get()->Now().InMicroseconds();
  }

  
  
  float ticks_per_microsecond_;  
  int64 skew_;  

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
