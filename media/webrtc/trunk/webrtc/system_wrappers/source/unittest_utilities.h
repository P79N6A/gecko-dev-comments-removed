









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_UNITTEST_UTILITIES_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_UNITTEST_UTILITIES_H_




#include <stdio.h>
#include <string.h>

#include "system_wrappers/interface/trace.h"

namespace webrtc {

class TestTraceCallback : public TraceCallback {
 public:
  virtual void Print(TraceLevel level, const char* msg, int length) {
    if (msg) {
      char* cmd_print = new char[length+1];
      memcpy(cmd_print, msg, length);
      cmd_print[length] = '\0';
      printf("%s\n", cmd_print);
      fflush(stdout);
      delete[] cmd_print;
    }
  }
};











class ScopedTracing {
 public:
  explicit ScopedTracing(bool logOn) {
    logging_ = logOn;
    StartTrace();
  }

  ~ScopedTracing() {
    StopTrace();
  }

 private:
  void StartTrace() {
    if (logging_) {
      Trace::CreateTrace();
      Trace::SetLevelFilter(webrtc::kTraceAll);
      Trace::SetTraceCallback(&trace_);
    }
  }

  void StopTrace() {
    if (logging_) {
      Trace::ReturnTrace();
    }
  }

 private:
  bool logging_;
  TestTraceCallback trace_;
};

}  

#endif  
