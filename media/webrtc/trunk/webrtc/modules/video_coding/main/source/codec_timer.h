









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODEC_TIMER_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODEC_TIMER_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc
{


#define MAX_HISTORY_SIZE 10
#define SHORT_FILTER_MS 1000

class VCMShortMaxSample
{
public:
    VCMShortMaxSample() : shortMax(0), timeMs(-1) {};

    int32_t     shortMax;
    int64_t     timeMs;
};

class VCMCodecTimer
{
public:
    VCMCodecTimer();

    
    int32_t StopTimer(int64_t startTimeMs, int64_t nowMs);

    
    void Reset();

    
    int32_t RequiredDecodeTimeMs(FrameType frameType) const;

private:
    void UpdateMaxHistory(int32_t decodeTime, int64_t now);
    void MaxFilter(int32_t newTime, int64_t nowMs);
    void ProcessHistory(int64_t nowMs);

    int32_t                     _filteredMax;
    
    int32_t                     _ignoredSampleCount;
    int32_t                     _shortMax;
    VCMShortMaxSample           _history[MAX_HISTORY_SIZE];

};

}  

#endif 
