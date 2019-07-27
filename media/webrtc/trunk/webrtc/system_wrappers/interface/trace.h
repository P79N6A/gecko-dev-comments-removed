














#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

#if defined(WEBRTC_RESTRICT_LOGGING)

#define WEBRTC_TRACE true ? (void) 0 : Trace::Add
#else
#define WEBRTC_TRACE Trace::Add
#endif

class Trace {
 public:
  
  static const int kBoilerplateLength;
  
  static const int kTimestampPosition;
  
  static const int kTimestampLength;

  
  static void CreateTrace();
  
  static void ReturnTrace();
  
  
  

  
  
  
  
  static void set_level_filter(uint32_t filter) { level_filter_ = filter; }

  
  static uint32_t level_filter() { return level_filter_; }

  
  
  
  static int32_t SetTraceFile(const char* file_name,
                              const bool add_file_counter = false);

  
  static int32_t TraceFile(char file_name[1024]);

  
  
  
  static int32_t SetTraceCallback(TraceCallback* callback);

  
  
  
  
  
  
  
  
  
  
  static void Add(const TraceLevel level,
                  const TraceModule module,
                  const int32_t id,
                  const char* msg, ...);

 private:
  static uint32_t level_filter_;
};

}  

#endif  
