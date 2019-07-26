









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODEC_TIMER_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODEC_TIMER_H_

#include "typedefs.h"
#include "module_common_types.h"

namespace webrtc
{


#define MAX_HISTORY_SIZE 20
#define SHORT_FILTER_MS 1000

class VCMShortMaxSample
{
public:
    VCMShortMaxSample() : shortMax(0), timeMs(-1) {};

    WebRtc_Word32     shortMax;
    WebRtc_Word64     timeMs;
};

class VCMCodecTimer
{
public:
    VCMCodecTimer();

    
    WebRtc_Word32 StopTimer(WebRtc_Word64 startTimeMs, WebRtc_Word64 nowMs);

    
    void Reset();

    
    WebRtc_Word32 RequiredDecodeTimeMs(FrameType frameType) const;

private:
    void UpdateMaxHistory(WebRtc_Word32 decodeTime, WebRtc_Word64 now);
    void MaxFilter(WebRtc_Word32 newTime, WebRtc_Word64 nowMs);
    void ProcessHistory(WebRtc_Word64 nowMs);

    WebRtc_Word32                     _filteredMax;
    bool                              _firstDecodeTime;
    WebRtc_Word32                     _shortMax;
    VCMShortMaxSample                 _history[MAX_HISTORY_SIZE];

};

} 

#endif 
