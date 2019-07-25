









#include <cassert>
#include <stdio.h>

#include "statistics.h"

#include "trace.h"
#include "critical_section_wrapper.h"

namespace webrtc {

namespace voe {

Statistics::Statistics(const WebRtc_UWord32 instanceId) :
    _critPtr(CriticalSectionWrapper::CreateCriticalSection()),
    _instanceId(instanceId),
    _lastError(0),
    _isInitialized(false)
{
}
	
Statistics::~Statistics()
{
    if (_critPtr)
    {
        delete _critPtr;
        _critPtr = NULL;
    }
}

WebRtc_Word32 Statistics::SetInitialized()
{
    _isInitialized = true;
    return 0;
}

WebRtc_Word32 Statistics::SetUnInitialized()
{
    _isInitialized = false;
    return 0;
}

bool Statistics::Initialized() const
{
    return _isInitialized;
}

WebRtc_Word32 Statistics::SetLastError(const WebRtc_Word32 error) const
{
    CriticalSectionScoped cs(_critPtr);
    _lastError = error;
    return 0;
}

WebRtc_Word32 Statistics::SetLastError(const WebRtc_Word32 error,
                                       const TraceLevel level) const
{
    CriticalSectionScoped cs(_critPtr);
    _lastError = error;
    WEBRTC_TRACE(level, kTraceVoice, VoEId(_instanceId,-1),
                 "error code is set to %d",
                 _lastError);
    return 0;
}

WebRtc_Word32 Statistics::SetLastError(
    const WebRtc_Word32 error,
    const TraceLevel level, const char* msg) const
{
    CriticalSectionScoped cs(_critPtr);
    char traceMessage[KTraceMaxMessageSize];
    assert(strlen(msg) < KTraceMaxMessageSize);
    _lastError = error;
    sprintf(traceMessage, "%s (error=%d)", msg, error);
    WEBRTC_TRACE(level, kTraceVoice, VoEId(_instanceId,-1), "%s",
                 traceMessage);
    return 0;
}

WebRtc_Word32 Statistics::LastError() const
{
    CriticalSectionScoped cs(_critPtr);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
               "LastError() => %d", _lastError);
    return _lastError;
}

}  

}  
