









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_WINDOWS_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_WINDOWS_H_

#include "trace_impl.h"
#include <stdio.h>
#include <windows.h>

namespace webrtc {
class TraceWindows : public TraceImpl
{
public:
    TraceWindows();
    virtual ~TraceWindows();

    virtual WebRtc_Word32 AddTime(char* traceMessage,
                                  const TraceLevel level) const;

    virtual WebRtc_Word32 AddBuildInfo(char* traceMessage) const;
    virtual WebRtc_Word32 AddDateTimeInfo(char* traceMessage) const;
private:
    volatile mutable WebRtc_UWord32    _prevAPITickCount;
    volatile mutable WebRtc_UWord32   _prevTickCount;
};
} 

#endif 
