









#include "webrtc/system_wrappers/source/trace_posix.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#if defined(_DEBUG)
#define BUILDMODE "d"
#elif defined(DEBUG)
#define BUILDMODE "d"
#elif defined(NDEBUG)
#define BUILDMODE "r"
#else
#define BUILDMODE "?"
#endif
#define BUILDTIME __TIME__
#define BUILDDATE __DATE__

#define BUILDINFO BUILDDATE " " BUILDTIME " " BUILDMODE

namespace webrtc {

TracePosix::TracePosix()
    : crit_sect_(*CriticalSectionWrapper::CreateCriticalSection()) {
  struct timeval system_time_high_res;
  gettimeofday(&system_time_high_res, 0);
  prev_api_tick_count_ = prev_tick_count_ = system_time_high_res.tv_sec;
}

TracePosix::~TracePosix() {
  delete &crit_sect_;
  StopThread();
}

int32_t TracePosix::AddTime(char* trace_message, const TraceLevel level) const {
  struct timeval system_time_high_res;
  if (gettimeofday(&system_time_high_res, 0) == -1) {
    return -1;
  }
  struct tm buffer;
  const struct tm* system_time =
    localtime_r(&system_time_high_res.tv_sec, &buffer);

  const uint32_t ms_time = system_time_high_res.tv_usec / 1000;
  uint32_t prev_tickCount = 0;
  {
    CriticalSectionScoped lock(&crit_sect_);
    if (level == kTraceApiCall) {
      prev_tickCount = prev_tick_count_;
      prev_tick_count_ = ms_time;
    } else {
      prev_tickCount = prev_api_tick_count_;
      prev_api_tick_count_ = ms_time;
    }
  }

  uint32_t dw_delta_time = ms_time - prev_tickCount;
  if (prev_tickCount == 0) {
    dw_delta_time = 0;
  }
  if (dw_delta_time > 0x0fffffff) {
    
    dw_delta_time = 0;
  }
  if (dw_delta_time > 99999) {
    dw_delta_time = 99999;
  }

  sprintf(trace_message, "(%2u:%2u:%2u:%3u |%5lu) ", system_time->tm_hour,
          system_time->tm_min, system_time->tm_sec, ms_time,
          static_cast<unsigned long>(dw_delta_time));
  
  return 22;
}

int32_t TracePosix::AddBuildInfo(char* trace_message) const {
  sprintf(trace_message, "Build info: %s", BUILDINFO);
  
  return strlen(trace_message) + 1;
}

int32_t TracePosix::AddDateTimeInfo(char* trace_message) const {
  time_t t;
  time(&t);
  char buffer[26];  
  sprintf(trace_message, "Local Date: %s", ctime_r(&t, buffer));
  int32_t len = static_cast<int32_t>(strlen(trace_message));

  if ('\n' == trace_message[len - 1]) {
    trace_message[len - 1] = '\0';
    --len;
  }

  
  return len + 1;
}

}  
