









#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

const int Trace::kBoilerplateLength = 71;
const int Trace::kTimestampPosition = 13;
const int Trace::kTimestampLength = 12;
uint32_t Trace::level_filter_ = kTraceNone;

void Trace::CreateTrace() {
}

void Trace::ReturnTrace() {
}

int32_t Trace::TraceFile(char file_name[1024]) {
  return -1;
}

int32_t Trace::SetTraceFile(const char* file_name,
                            const bool add_file_counter) {
  return -1;
}

int32_t Trace::SetTraceCallback(TraceCallback* callback) {
  return -1;
}

void Trace::Add(const TraceLevel level, const TraceModule module,
                const int32_t id, const char* msg, ...) {
}

}  
