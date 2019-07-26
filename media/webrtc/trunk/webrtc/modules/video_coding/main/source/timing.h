









#ifndef WEBRTC_MODULES_VIDEO_CODING_TIMING_H_
#define WEBRTC_MODULES_VIDEO_CODING_TIMING_H_

#include "typedefs.h"
#include "critical_section_wrapper.h"
#include "codec_timer.h"

namespace webrtc
{

class Clock;
class VCMTimestampExtrapolator;

class VCMTiming
{
public:
    
    
    VCMTiming(Clock* clock,
              int32_t vcmId = 0,
              int32_t timingId = 0,
              VCMTiming* masterTiming = NULL);
    ~VCMTiming();

    
    void Reset(int64_t nowMs = -1);
    void ResetDecodeTime();

    
    void SetRenderDelay(uint32_t renderDelayMs);

    
    
    void SetRequiredDelay(uint32_t requiredDelayMs);

    
    void SetMinimumTotalDelay(uint32_t minTotalDelayMs);

    
    
    
    void UpdateCurrentDelay(uint32_t frameTimestamp);

    
    
    
    void UpdateCurrentDelay(int64_t renderTimeMs, int64_t actualDecodeTimeMs);

    
    
    int32_t StopDecodeTimer(uint32_t timeStamp,
                                  int64_t startTimeMs,
                                  int64_t nowMs);

    
    
    void IncomingTimestamp(uint32_t timeStamp, int64_t lastPacketTimeMs);

    
    
    int64_t RenderTimeMs(uint32_t frameTimestamp, int64_t nowMs) const;

    
    
    uint32_t MaxWaitingTime(int64_t renderTimeMs, int64_t nowMs) const;

    
    
    uint32_t TargetVideoDelay() const;

    
    
    bool EnoughTimeToDecode(uint32_t availableProcessingTimeMs) const;

    
    void SetMaxVideoDelay(int maxVideoDelayMs);

    enum { kDefaultRenderDelayMs = 10 };
    enum { kDelayMaxChangeMsPerS = 100 };

protected:
    int32_t MaxDecodeTimeMs(FrameType frameType = kVideoFrameDelta) const;
    int64_t RenderTimeMsInternal(uint32_t frameTimestamp,
                                       int64_t nowMs) const;
    uint32_t TargetDelayInternal() const;

private:
    CriticalSectionWrapper* _critSect;
    int32_t _vcmId;
    Clock* _clock;
    int32_t _timingId;
    bool _master;
    VCMTimestampExtrapolator* _tsExtrapolator;
    VCMCodecTimer _codecTimer;
    uint32_t _renderDelayMs;
    uint32_t _minTotalDelayMs;
    uint32_t _requiredDelayMs;
    uint32_t _currentDelayMs;
    uint32_t _prevFrameTimestamp;
    int _maxVideoDelayMs;
};

} 

#endif 
