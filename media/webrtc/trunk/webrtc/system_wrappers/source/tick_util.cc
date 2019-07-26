









#include "webrtc/system_wrappers/interface/tick_util.h"

#include <assert.h>

namespace webrtc {

bool TickTime::use_fake_clock_ = false;
int64_t TickTime::fake_ticks_ = 0;

void TickTime::UseFakeClock(int64_t start_millisecond) {
  use_fake_clock_ = true;
  fake_ticks_ = MillisecondsToTicks(start_millisecond);
}

void TickTime::AdvanceFakeClock(int64_t milliseconds) {
  assert(use_fake_clock_);
  fake_ticks_ += MillisecondsToTicks(milliseconds);
}

int64_t TickTime::QueryOsForTicks() {
  TickTime result;
#if _WIN32
  
#ifdef USE_QUERY_PERFORMANCE_COUNTER
  
  
  
  
  
  LARGE_INTEGER qpcnt;
  QueryPerformanceCounter(&qpcnt);
  result.ticks_ = qpcnt.QuadPart;
#else
  static volatile LONG last_time_get_time = 0;
  static volatile int64_t num_wrap_time_get_time = 0;
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
  result.ticks_ = 1000000000LL * static_cast<int64_t>(ts.tv_sec) +
      static_cast<int64_t>(ts.tv_nsec);
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
  result.ticks_ = 1000000LL * static_cast<int64_t>(tv.tv_sec) +
      static_cast<int64_t>(tv.tv_usec);
#endif
  return result.ticks_;
}

}  
