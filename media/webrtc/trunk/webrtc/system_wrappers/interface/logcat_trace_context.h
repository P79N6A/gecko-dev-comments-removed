









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_LOGCAT_TRACE_CONTEXT_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_LOGCAT_TRACE_CONTEXT_H_

#include "webrtc/system_wrappers/interface/trace.h"

#ifndef ANDROID
#error This file only makes sense to include on Android!
#endif

namespace webrtc {



class LogcatTraceContext : public webrtc::TraceCallback {
 public:
  LogcatTraceContext();
  virtual ~LogcatTraceContext();

  
  virtual void Print(TraceLevel level, const char* message, int length);
};

}  

#endif  
