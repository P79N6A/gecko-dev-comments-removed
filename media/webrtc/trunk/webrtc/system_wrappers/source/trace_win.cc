









#include "webrtc/system_wrappers/source/trace_win.h"

#include <cassert>
#include <stdarg.h>

#include "Mmsystem.h"

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
TraceWindows::TraceWindows()
    : prev_api_tick_count_(0),
      prev_tick_count_(0) {
}

TraceWindows::~TraceWindows() {
  StopThread();
}

WebRtc_Word32 TraceWindows::AddTime(char* trace_message,
                                    const TraceLevel level) const {
  WebRtc_UWord32 dw_current_time = timeGetTime();
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);

  if (level == kTraceApiCall) {
    WebRtc_UWord32 dw_delta_time = dw_current_time - prev_tick_count_;
    prev_tick_count_ = dw_current_time;

    if (prev_tick_count_ == 0) {
      dw_delta_time = 0;
    }
    if (dw_delta_time > 0x0fffffff) {
      
      dw_delta_time = 0;
    }
    if (dw_delta_time > 99999) {
      dw_delta_time = 99999;
    }

    sprintf(trace_message, "(%2u:%2u:%2u:%3u |%5lu) ", system_time.wHour,
            system_time.wMinute, system_time.wSecond,
            system_time.wMilliseconds, dw_delta_time);
  } else {
    WebRtc_UWord32 dw_delta_time = dw_current_time - prev_api_tick_count_;
    prev_api_tick_count_ = dw_current_time;

    if (prev_api_tick_count_ == 0) {
      dw_delta_time = 0;
    }
    if (dw_delta_time > 0x0fffffff) {
      
      dw_delta_time = 0;
    }
    if (dw_delta_time > 99999) {
      dw_delta_time = 99999;
    }
    sprintf(trace_message, "(%2u:%2u:%2u:%3u |%5lu) ", system_time.wHour,
            system_time.wMinute, system_time.wSecond,
            system_time.wMilliseconds, dw_delta_time);
  }
  return 22;
}

WebRtc_Word32 TraceWindows::AddBuildInfo(char* trace_message) const {
  
  sprintf(trace_message, "Build info: %s", BUILDINFO);
  
  return static_cast<WebRtc_Word32>(strlen(trace_message) + 1);
}

WebRtc_Word32 TraceWindows::AddDateTimeInfo(char* trace_message) const {
  prev_api_tick_count_ = timeGetTime();
  prev_tick_count_ = prev_api_tick_count_;

  SYSTEMTIME sys_time;
  GetLocalTime(&sys_time);

  TCHAR sz_date_str[20];
  TCHAR sz_time_str[20];

  
  GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &sys_time, TEXT("MMM dd yyyy"),
                sz_date_str, 20);

  
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &sys_time, TEXT("HH':'mm':'ss"),
                sz_time_str, 20);

  sprintf(trace_message, "Local Date: %s Local Time: %s", sz_date_str,
          sz_time_str);

  
  return static_cast<WebRtc_Word32>(strlen(trace_message) + 1);
}

}  
