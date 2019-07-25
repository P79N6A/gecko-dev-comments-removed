









#include "trace.h"

namespace webrtc {

void Trace::CreateTrace()
{
}

void Trace::ReturnTrace()
{
}

WebRtc_Word32 Trace::SetLevelFilter(WebRtc_UWord32 )
{
    return 0;
}

WebRtc_Word32 Trace::LevelFilter(WebRtc_UWord32& )
{
    return 0;
}

WebRtc_Word32 Trace::TraceFile(
    char[1024])
{
    return -1;
}

WebRtc_Word32 Trace::SetTraceFile(const char* ,
                                  const bool )
{
    return -1;
}

WebRtc_Word32 Trace::SetTraceCallback(TraceCallback* )
{
    return -1;
}

void Trace::Add(const TraceLevel , const TraceModule ,
                const WebRtc_Word32 , const char* , ...)

{
}

} 
