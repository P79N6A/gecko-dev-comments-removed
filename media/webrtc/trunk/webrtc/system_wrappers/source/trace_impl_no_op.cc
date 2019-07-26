









#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

void Trace::CreateTrace() {
}

void Trace::ReturnTrace() {
}

WebRtc_Word32 Trace::SetLevelFilter(WebRtc_UWord32 filter) {
  return 0;
}

WebRtc_Word32 Trace::LevelFilter(WebRtc_UWord32& filter) {
  return 0;
}

WebRtc_Word32 Trace::TraceFile(char file_name[1024]) {
  return -1;
}

WebRtc_Word32 Trace::SetTraceFile(const char* file_name,
                                  const bool add_file_counter) {
  return -1;
}

WebRtc_Word32 Trace::SetTraceCallback(TraceCallback* callback) {
  return -1;
}

void Trace::Add(const TraceLevel level, const TraceModule module,
                const WebRtc_Word32 id, const char* msg, ...) {
}

}  
