









#ifndef WEBRTC_MODULES_VIDEO_CODING_TIMING_H_
#define WEBRTC_MODULES_VIDEO_CODING_TIMING_H_

#include "typedefs.h"
#include "critical_section_wrapper.h"
#include "codec_timer.h"

namespace webrtc
{

class TickTimeBase;
class VCMTimestampExtrapolator;

class VCMTiming
{
public:
    
    
    VCMTiming(TickTimeBase* clock,
              WebRtc_Word32 vcmId = 0,
              WebRtc_Word32 timingId = 0,
              VCMTiming* masterTiming = NULL);
    ~VCMTiming();

    
    void Reset(WebRtc_Word64 nowMs = -1);
    void ResetDecodeTime();

    
    void SetRenderDelay(WebRtc_UWord32 renderDelayMs);

    
    
    void SetRequiredDelay(WebRtc_UWord32 requiredDelayMs);

    
    void SetMinimumTotalDelay(WebRtc_UWord32 minTotalDelayMs);

    
    
    
    void UpdateCurrentDelay(WebRtc_UWord32 frameTimestamp);

    
    
    
    void UpdateCurrentDelay(WebRtc_Word64 renderTimeMs, WebRtc_Word64 actualDecodeTimeMs);

    
    
    WebRtc_Word32 StopDecodeTimer(WebRtc_UWord32 timeStamp,
                                  WebRtc_Word64 startTimeMs,
                                  WebRtc_Word64 nowMs);

    
    
    void IncomingTimestamp(WebRtc_UWord32 timeStamp, WebRtc_Word64 lastPacketTimeMs);

    
    
    WebRtc_Word64 RenderTimeMs(WebRtc_UWord32 frameTimestamp, WebRtc_Word64 nowMs) const;

    
    
    WebRtc_UWord32 MaxWaitingTime(WebRtc_Word64 renderTimeMs, WebRtc_Word64 nowMs) const;

    
    
    WebRtc_UWord32 TargetVideoDelay() const;

    
    
    bool EnoughTimeToDecode(WebRtc_UWord32 availableProcessingTimeMs) const;

    enum { kDefaultRenderDelayMs = 10 };
    enum { kDelayMaxChangeMsPerS = 100 };

protected:
    WebRtc_Word32 MaxDecodeTimeMs(FrameType frameType = kVideoFrameDelta) const;
    WebRtc_Word64 RenderTimeMsInternal(WebRtc_UWord32 frameTimestamp,
                                       WebRtc_Word64 nowMs) const;
    WebRtc_UWord32 TargetDelayInternal() const;

private:
    CriticalSectionWrapper*       _critSect;
    WebRtc_Word32                 _vcmId;
    TickTimeBase*                 _clock;
    WebRtc_Word32                 _timingId;
    bool                          _master;
    VCMTimestampExtrapolator*     _tsExtrapolator;
    VCMCodecTimer                 _codecTimer;
    WebRtc_UWord32                _renderDelayMs;
    WebRtc_UWord32                _minTotalDelayMs;
    WebRtc_UWord32                _requiredDelayMs;
    WebRtc_UWord32                _currentDelayMs;
    WebRtc_UWord32                _prevFrameTimestamp;
};

} 

#endif 
