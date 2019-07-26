









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_WIN_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_WIN_H_

#include <stdio.h>
#include <windows.h>

#include "webrtc/system_wrappers/source/trace_impl.h"

namespace webrtc {

class TraceWindows : public TraceImpl {
 public:
  TraceWindows();
  virtual ~TraceWindows();

  virtual WebRtc_Word32 AddTime(char* trace_message,
                                const TraceLevel level) const;

  virtual WebRtc_Word32 AddBuildInfo(char* trace_message) const;
  virtual WebRtc_Word32 AddDateTimeInfo(char* trace_message) const;
 private:
  volatile mutable WebRtc_UWord32 prev_api_tick_count_;
  volatile mutable WebRtc_UWord32 prev_tick_count_;
};

}  

#endif  
