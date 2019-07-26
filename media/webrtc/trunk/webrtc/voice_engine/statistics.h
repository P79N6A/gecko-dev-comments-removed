









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
    Statistics(const uint32_t instanceId);
    ~Statistics();

    int32_t SetInitialized();
    int32_t SetUnInitialized();
    bool Initialized() const;
    int32_t SetLastError(const int32_t error) const;
    int32_t SetLastError(const int32_t error, const TraceLevel level) const;
    int32_t SetLastError(const int32_t error,
                         const TraceLevel level,
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
