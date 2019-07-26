














#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"

#define WEBRTC_TRACE Trace::Add

namespace webrtc {

class Trace {
 public:
  
  static void CreateTrace();
  
  static void ReturnTrace();
  
  
  

  
  
  
  
  static WebRtc_Word32 SetLevelFilter(const WebRtc_UWord32 filter);

  
  static WebRtc_Word32 LevelFilter(WebRtc_UWord32& filter);

  
  
  
  static WebRtc_Word32 SetTraceFile(const char* file_name,
                                    const bool add_file_counter = false);

  
  static WebRtc_Word32 TraceFile(char file_name[1024]);

  
  
  
  static WebRtc_Word32 SetTraceCallback(TraceCallback* callback);

  
  
  
  
  
  
  
  
  
  
  static void Add(const TraceLevel level,
                  const TraceModule module,
                  const WebRtc_Word32 id,
                  const char* msg, ...);
};

}  

#endif  
