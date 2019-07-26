









#ifndef WEBRTC_TEST_TEST_SUPPORT_TRACE_TO_STDERR_H_
#define WEBRTC_TEST_TEST_SUPPORT_TRACE_TO_STDERR_H_

#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
namespace test {



class TraceToStderr : public TraceCallback {
 public:
  TraceToStderr();
  virtual ~TraceToStderr();

  virtual void Print(TraceLevel level, const char* msg_array, int length);
};

}  
}  

#endif  
