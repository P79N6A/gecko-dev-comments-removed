









#include "webrtc/test/testsupport/trace_to_stderr.h"

#include <cassert>
#include <cstdio>

#include <string>

namespace webrtc {
namespace test {

static const int kLevelFilter = kTraceError | kTraceWarning | kTraceTerseInfo;

TraceToStderr::TraceToStderr() {
  Trace::CreateTrace();
  Trace::SetTraceCallback(this);
  Trace::SetLevelFilter(kLevelFilter);
}

TraceToStderr::~TraceToStderr() {
  Trace::SetTraceCallback(NULL);
  Trace::ReturnTrace();
}

void TraceToStderr::Print(TraceLevel level, const char* msg_array, int length) {
  if (level & kLevelFilter) {
    assert(length > Trace::kBoilerplateLength);
    std::string msg = msg_array;
    std::string msg_time = msg.substr(Trace::kTimestampPosition,
                                      Trace::kTimestampLength);
    std::string msg_log = msg.substr(Trace::kBoilerplateLength);
    fprintf(stderr, "%s %s\n", msg_time.c_str(), msg_log.c_str());
    fflush(stderr);
  }
}

}  
}  
