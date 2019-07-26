









#ifndef WEBRTC_VOICE_ENGINE_STATISTICS_H
#define WEBRTC_VOICE_ENGINE_STATISTICS_H

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"
#include "webrtc/voice_engine/include/voe_errors.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

namespace webrtc {
class CriticalSectionWrapper;

namespace voe {

class Statistics
{
 public:
    enum {KTraceMaxMessageSize = 256};
 public:
    Statistics(uint32_t instanceId);
    ~Statistics();

    int32_t SetInitialized();
    int32_t SetUnInitialized();
    bool Initialized() const;
    int32_t SetLastError(int32_t error) const;
    int32_t SetLastError(int32_t error, TraceLevel level) const;
    int32_t SetLastError(int32_t error,
                         TraceLevel level,
                         const char* msg) const;
    int32_t LastError() const;

 private:
    CriticalSectionWrapper* _critPtr;
    const uint32_t _instanceId;
    mutable int32_t _lastError;
    bool _isInitialized;
};

}  

}  

#endif
