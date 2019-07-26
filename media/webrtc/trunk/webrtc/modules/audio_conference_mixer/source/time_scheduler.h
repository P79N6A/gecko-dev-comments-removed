













#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_TIME_SCHEDULER_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_TIME_SCHEDULER_H_

#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc {
class CriticalSectionWrapper;
class TimeScheduler
{
public:
    TimeScheduler(const uint32_t periodicityInMs);
    ~TimeScheduler();

    
    int32_t UpdateScheduler();

    
    
    int32_t TimeToNextUpdate(int32_t& updateTimeInMS) const;

private:
    CriticalSectionWrapper* _crit;

    bool _isStarted;
    TickTime _lastPeriodMark;

    uint32_t _periodicityInMs;
    int64_t  _periodicityInTicks;
    uint32_t _missedPeriods;
};
}  

#endif 
