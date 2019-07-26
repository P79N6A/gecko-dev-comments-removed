













#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_TIME_SCHEDULER_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_TIME_SCHEDULER_H_

#include "tick_util.h"

namespace webrtc {
class CriticalSectionWrapper;
class TimeScheduler
{
public:
    TimeScheduler(const WebRtc_UWord32 periodicityInMs);
    ~TimeScheduler();

    
    WebRtc_Word32 UpdateScheduler();

    
    
    WebRtc_Word32 TimeToNextUpdate(WebRtc_Word32& updateTimeInMS) const;

private:
    CriticalSectionWrapper* _crit;

    bool _isStarted;
    TickTime _lastPeriodMark;

    WebRtc_UWord32 _periodicityInMs;
    WebRtc_Word64  _periodicityInTicks;
    WebRtc_UWord32 _missedPeriods;
};
} 

#endif 
