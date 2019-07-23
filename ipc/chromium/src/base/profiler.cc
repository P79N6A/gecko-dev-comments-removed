



#include "base/profiler.h"
#include "base/string_util.h"




#ifdef QUANTIFY


#define PURIFY_PRIVATE_INCLUDE
#include "base/third_party/purify/pure.h"
#endif 

namespace base {

void Profiler::StartRecording() {
#ifdef QUANTIFY
  QuantifyStartRecordingData();
#endif
}

void Profiler::StopRecording() {
#ifdef QUANTIFY
  QuantifyStopRecordingData();
#endif
}

void Profiler::ClearData() {
#ifdef QUANTIFY
  QuantifyClearData();
#endif
}

void Profiler::SetThreadName(const char *name) {
#ifdef QUANTIFY
  
  char buffer[512];
  base::snprintf(buffer, sizeof(buffer)-1, "%s", name);
  QuantifySetThreadName(buffer);
#endif
}

}  
