









#include <assert.h>
#include <stdio.h>

#include "webrtc/voice_engine/statistics.h"

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

namespace voe {

Statistics::Statistics(uint32_t instanceId) :
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

int32_t Statistics::SetInitialized()
{
    _isInitialized = true;
    return 0;
}

int32_t Statistics::SetUnInitialized()
{
    _isInitialized = false;
    return 0;
}

bool Statistics::Initialized() const
{
    return _isInitialized;
}

int32_t Statistics::SetLastError(int32_t error) const
{
    CriticalSectionScoped cs(_critPtr);
    _lastError = error;
    return 0;
}

int32_t Statistics::SetLastError(int32_t error,
                                 TraceLevel level) const
{
    CriticalSectionScoped cs(_critPtr);
    _lastError = error;
    WEBRTC_TRACE(level, kTraceVoice, VoEId(_instanceId,-1),
                 "error code is set to %d",
                 _lastError);
    return 0;
}

int32_t Statistics::SetLastError(
    int32_t error,
    TraceLevel level, const char* msg) const
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

int32_t Statistics::LastError() const
{
    CriticalSectionScoped cs(_critPtr);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
               "LastError() => %d", _lastError);
    return _lastError;
}

}  

}  
