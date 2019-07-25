













#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_TRACE_H_

#include "common_types.h"
#include "typedefs.h"

#define WEBRTC_TRACE Trace::Add

namespace webrtc {
class Trace
{
public:

    
    static void CreateTrace();
    
    static void ReturnTrace();
    
    
    

    
    
    
    
    static WebRtc_Word32 SetLevelFilter(const WebRtc_UWord32 filter);

    
    static WebRtc_Word32 LevelFilter(WebRtc_UWord32& filter);

    
    
    
    static WebRtc_Word32 SetTraceFile(const char* fileName,
                                      const bool addFileCounter = false);

    
    static WebRtc_Word32 TraceFile(char fileName[1024]);

    
    
    
    static WebRtc_Word32 SetTraceCallback(TraceCallback* callback);

    
    
    
    
    
    
    
    
    
    
    static void Add(const TraceLevel level,
                    const TraceModule module,
                    const WebRtc_Word32 id,
                    const char* msg, ...);
};
} 
#endif 
