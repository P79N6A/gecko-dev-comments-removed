














#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"

#if !defined(WEBRTC_LOGGING)
#define WEBRTC_TRACE (true) ? (void)0 : Trace::Add
#else
#define WEBRTC_TRACE Trace::Add
#endif

namespace webrtc {

class Trace {
 public:
  
  static const int kBoilerplateLength;
  
  static const int kTimestampPosition;
  
  static const int kTimestampLength;

  
  static void CreateTrace();
  
  static void ReturnTrace();
  
  
  

  
  
  
  
  static int32_t SetLevelFilter(const uint32_t filter);

  
  static int32_t LevelFilter(uint32_t& filter);

  
  
  
  static int32_t SetTraceFile(const char* file_name,
                              const bool add_file_counter = false);

  
  static int32_t TraceFile(char file_name[1024]);

  
  
  
  static int32_t SetTraceCallback(TraceCallback* callback);

  
  
  
  
  
  
  
  
  
  
  static void Add(const TraceLevel level,
                  const TraceModule module,
                  const int32_t id,
                  const char* msg, ...);
};

}  

#endif  
