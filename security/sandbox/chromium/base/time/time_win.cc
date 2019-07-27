
































#include "base/time/time.h"

#pragma comment(lib, "winmm.lib")
#include <windows.h>
#include <mmsystem.h>

#include "base/basictypes.h"
#include "base/cpu.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

namespace {



int64 FileTimeToMicroseconds(const FILETIME& ft) {
  
  
  
  return bit_cast<int64, FILETIME>(ft) / 10;
}

void MicrosecondsToFileTime(int64 us, FILETIME* ft) {
  DCHECK_GE(us, 0LL) << "Time is less than 0, negative values are not "
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




const int kMinTimerIntervalHighResMs = 1;
const int kMinTimerIntervalLowResMs = 4;

bool g_high_res_timer_enabled = false;

uint32_t g_high_res_timer_count = 0;

base::LazyInstance<base::Lock>::Leaky g_high_res_lock =
    LAZY_INSTANCE_INITIALIZER;

}  








const int64 Time::kTimeTToMicrosecondsOffset = GG_INT64_C(11644473600000000);


Time Time::Now() {
  if (initial_time == 0)
    InitializeClock();

  
  
  
  
  
  
  
  
  
  
  while (true) {
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
  if (bit_cast<int64, FILETIME>(ft) == 0)
    return Time();
  if (ft.dwHighDateTime == std::numeric_limits<DWORD>::max() &&
      ft.dwLowDateTime == std::numeric_limits<DWORD>::max())
    return Max();
  return Time(FileTimeToMicroseconds(ft));
}

FILETIME Time::ToFileTime() const {
  if (is_null())
    return bit_cast<FILETIME, int64>(0);
  if (is_max()) {
    FILETIME result;
    result.dwHighDateTime = std::numeric_limits<DWORD>::max();
    result.dwLowDateTime = std::numeric_limits<DWORD>::max();
    return result;
  }
  FILETIME utc_ft;
  MicrosecondsToFileTime(us_, &utc_ft);
  return utc_ft;
}


void Time::EnableHighResolutionTimer(bool enable) {
  base::AutoLock lock(g_high_res_lock.Get());
  if (g_high_res_timer_enabled == enable)
    return;
  g_high_res_timer_enabled = enable;
  if (!g_high_res_timer_count)
    return;
  
  
  
  
  
  if (enable) {
    timeEndPeriod(kMinTimerIntervalLowResMs);
    timeBeginPeriod(kMinTimerIntervalHighResMs);
  } else {
    timeEndPeriod(kMinTimerIntervalHighResMs);
    timeBeginPeriod(kMinTimerIntervalLowResMs);
  }
}


bool Time::ActivateHighResolutionTimer(bool activating) {
  
  
  
  const uint32_t max = std::numeric_limits<uint32_t>::max();

  base::AutoLock lock(g_high_res_lock.Get());
  UINT period = g_high_res_timer_enabled ? kMinTimerIntervalHighResMs
                                         : kMinTimerIntervalLowResMs;
  if (activating) {
    DCHECK(g_high_res_timer_count != max);
    ++g_high_res_timer_count;
    if (g_high_res_timer_count == 1)
      timeBeginPeriod(period);
  } else {
    DCHECK(g_high_res_timer_count != 0);
    --g_high_res_timer_count;
    if (g_high_res_timer_count == 0)
      timeEndPeriod(period);
  }
  return (period == kMinTimerIntervalHighResMs);
}


bool Time::IsHighResolutionTimerInUse() {
  base::AutoLock lock(g_high_res_lock.Get());
  return g_high_res_timer_enabled && g_high_res_timer_count > 0;
}


Time Time::FromExploded(bool is_local, const Exploded& exploded) {
  
  
  SYSTEMTIME st;
  st.wYear = static_cast<WORD>(exploded.year);
  st.wMonth = static_cast<WORD>(exploded.month);
  st.wDayOfWeek = static_cast<WORD>(exploded.day_of_week);
  st.wDay = static_cast<WORD>(exploded.day_of_month);
  st.wHour = static_cast<WORD>(exploded.hour);
  st.wMinute = static_cast<WORD>(exploded.minute);
  st.wSecond = static_cast<WORD>(exploded.second);
  st.wMilliseconds = static_cast<WORD>(exploded.millisecond);

  FILETIME ft;
  bool success = true;
  
  if (is_local) {
    SYSTEMTIME utc_st;
    success = TzSpecificLocalTimeToSystemTime(NULL, &st, &utc_st) &&
              SystemTimeToFileTime(&utc_st, &ft);
  } else {
    success = !!SystemTimeToFileTime(&st, &ft);
  }

  if (!success) {
    NOTREACHED() << "Unable to convert time";
    return Time(0);
  }
  return Time(FileTimeToMicroseconds(ft));
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  if (us_ < 0LL) {
    
    ZeroMemory(exploded, sizeof(*exploded));
    return;
  }

  
  FILETIME utc_ft;
  MicrosecondsToFileTime(us_, &utc_ft);

  
  bool success = true;
  
  SYSTEMTIME st = {0};
  if (is_local) {
    SYSTEMTIME utc_st;
    
    
    
    
    success = FileTimeToSystemTime(&utc_ft, &utc_st) &&
              SystemTimeToTzSpecificLocalTime(NULL, &utc_st, &st);
  } else {
    success = !!FileTimeToSystemTime(&utc_ft, &st);
  }

  if (!success) {
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


int64 rollover_ms = 0;


DWORD last_seen_now = 0;







base::Lock rollover_lock;






TimeDelta RolloverProtectedNow() {
  base::AutoLock locked(rollover_lock);
  
  
  DWORD now = tick_function();
  if (now < last_seen_now)
    rollover_ms += 0x100000000I64;  
  last_seen_now = now;
  return TimeDelta::FromMilliseconds(now + rollover_ms);
}

bool IsBuggyAthlon(const base::CPU& cpu) {
  
  
  return cpu.vendor_name() == "AuthenticAMD" && cpu.family() == 15;
}





























class HighResNowSingleton {
 public:
  HighResNowSingleton()
      : ticks_per_second_(0),
        skew_(0) {

    base::CPU cpu;
    if (IsBuggyAthlon(cpu))
      return;

    
    LARGE_INTEGER ticks_per_sec = {0};
    if (!QueryPerformanceFrequency(&ticks_per_sec))
      return; 
    ticks_per_second_ = ticks_per_sec.QuadPart;

    skew_ = UnreliableNow() - ReliableNow();
  }

  bool IsUsingHighResClock() {
    return ticks_per_second_ != 0;
  }

  TimeDelta Now() {
    if (IsUsingHighResClock())
      return TimeDelta::FromMicroseconds(UnreliableNow());

    
    return RolloverProtectedNow();
  }

  int64 GetQPCDriftMicroseconds() {
    if (!IsUsingHighResClock())
      return 0;
    return abs((UnreliableNow() - ReliableNow()) - skew_);
  }

  int64 QPCValueToMicroseconds(LONGLONG qpc_value) {
    if (!ticks_per_second_)
      return 0;
    
    
    if (qpc_value < Time::kQPCOverflowThreshold)
      return qpc_value * Time::kMicrosecondsPerSecond / ticks_per_second_;
    
    
    int64 whole_seconds = qpc_value / ticks_per_second_;
    int64 leftover_ticks = qpc_value - (whole_seconds * ticks_per_second_);
    int64 microseconds = (whole_seconds * Time::kMicrosecondsPerSecond) +
                         ((leftover_ticks * Time::kMicrosecondsPerSecond) /
                          ticks_per_second_);
    return microseconds;
  }

 private:
  
  int64 UnreliableNow() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return QPCValueToMicroseconds(now.QuadPart);
  }

  
  int64 ReliableNow() {
    return RolloverProtectedNow().InMicroseconds();
  }

  int64 ticks_per_second_;  
  int64 skew_;  
};

static base::LazyInstance<HighResNowSingleton>::Leaky
    leaky_high_res_now_singleton = LAZY_INSTANCE_INITIALIZER;

HighResNowSingleton* GetHighResNowSingleton() {
  return leaky_high_res_now_singleton.Pointer();
}

TimeDelta HighResNowWrapper() {
  return GetHighResNowSingleton()->Now();
}

typedef TimeDelta (*NowFunction)(void);

bool CPUReliablySupportsHighResTime() {
  base::CPU cpu;
  if (!cpu.has_non_stop_time_stamp_counter() ||
      !GetHighResNowSingleton()->IsUsingHighResClock())
    return false;

  if (IsBuggyAthlon(cpu))
    return false;

  return true;
}

TimeDelta InitialNowFunction();

volatile NowFunction now_function = InitialNowFunction;

TimeDelta InitialNowFunction() {
  if (!CPUReliablySupportsHighResTime()) {
    InterlockedExchangePointer(
        reinterpret_cast<void* volatile*>(&now_function),
        &RolloverProtectedNow);
    return RolloverProtectedNow();
  }
  InterlockedExchangePointer(
        reinterpret_cast<void* volatile*>(&now_function),
        &HighResNowWrapper);
  return HighResNowWrapper();
}

}  


TimeTicks::TickFunctionType TimeTicks::SetMockTickFunction(
    TickFunctionType ticker) {
  base::AutoLock locked(rollover_lock);
  TickFunctionType old = tick_function;
  tick_function = ticker;
  rollover_ms = 0;
  last_seen_now = 0;
  return old;
}


TimeTicks TimeTicks::Now() {
  return TimeTicks() + now_function();
}


TimeTicks TimeTicks::HighResNow() {
  return TimeTicks() + HighResNowWrapper();
}


bool TimeTicks::IsHighResNowFastAndReliable() {
  return CPUReliablySupportsHighResTime();
}


TimeTicks TimeTicks::ThreadNow() {
  NOTREACHED();
  return TimeTicks();
}


TimeTicks TimeTicks::NowFromSystemTraceTime() {
  return HighResNow();
}


int64 TimeTicks::GetQPCDriftMicroseconds() {
  return GetHighResNowSingleton()->GetQPCDriftMicroseconds();
}


TimeTicks TimeTicks::FromQPCValue(LONGLONG qpc_value) {
  return TimeTicks(GetHighResNowSingleton()->QPCValueToMicroseconds(qpc_value));
}


bool TimeTicks::IsHighResClockWorking() {
  return GetHighResNowSingleton()->IsUsingHighResClock();
}

TimeTicks TimeTicks::UnprotectedNow() {
  if (now_function == HighResNowWrapper) {
    return Now();
  } else {
    return TimeTicks() + TimeDelta::FromMilliseconds(timeGetTime());
  }
}




TimeDelta TimeDelta::FromQPCValue(LONGLONG qpc_value) {
  return TimeDelta(GetHighResNowSingleton()->QPCValueToMicroseconds(qpc_value));
}
