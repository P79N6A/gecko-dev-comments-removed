









#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

void Trace::CreateTrace() {
}

void Trace::ReturnTrace() {
}

int32_t Trace::SetLevelFilter(uint32_t filter) {
  return 0;
}

int32_t Trace::LevelFilter(uint32_t& filter) {
  return 0;
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
