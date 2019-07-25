









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_POSIX_H_

#include "critical_section_wrapper.h"
#include "trace_impl.h"

namespace webrtc {
class TracePosix : public TraceImpl
{
public:
    TracePosix();
    virtual ~TracePosix();

    virtual WebRtc_Word32 AddTime(char* traceMessage,
                                  const TraceLevel level) const;

    virtual WebRtc_Word32 AddBuildInfo(char* traceMessage) const;
    virtual WebRtc_Word32 AddDateTimeInfo(char* traceMessage) const;

private:
    volatile mutable WebRtc_UWord32  _prevAPITickCount;
    volatile mutable WebRtc_UWord32  _prevTickCount;
};
} 

#endif 
