









#ifndef WEBRTC_VOICE_ENGINE_STATISTICS_H
#define WEBRTC_VOICE_ENGINE_STATISTICS_H

#include "common_types.h"
#include "typedefs.h"
#include "voice_engine_defines.h"
#include "voe_errors.h"

namespace webrtc {
class CriticalSectionWrapper;

namespace voe {

class Statistics
{
 public:
    enum {KTraceMaxMessageSize = 256};
 public:
    Statistics(const WebRtc_UWord32 instanceId);
    ~Statistics();

    WebRtc_Word32 SetInitialized();
    WebRtc_Word32 SetUnInitialized();
    bool Initialized() const;
    WebRtc_Word32 SetLastError(const WebRtc_Word32 error) const;
    WebRtc_Word32 SetLastError(const WebRtc_Word32 error,
                               const TraceLevel level) const;
    WebRtc_Word32 SetLastError(const WebRtc_Word32 error,
                               const TraceLevel level,
                               const char* msg) const;
    WebRtc_Word32 LastError() const;

 private:
    CriticalSectionWrapper* _critPtr;
    const WebRtc_UWord32 _instanceId;
    mutable WebRtc_Word32 _lastError;
    bool _isInitialized;
};

}  

}  

#endif
