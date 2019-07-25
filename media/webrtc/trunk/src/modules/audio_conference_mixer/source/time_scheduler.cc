









#include "critical_section_wrapper.h"
#include "time_scheduler.h"

namespace webrtc {
TimeScheduler::TimeScheduler(const WebRtc_UWord32 periodicityInMs)
    : _crit(CriticalSectionWrapper::CreateCriticalSection()),
      _isStarted(false),
      _lastPeriodMark(),
      _periodicityInMs(periodicityInMs),
      _periodicityInTicks(TickTime::MillisecondsToTicks(periodicityInMs)),
      _missedPeriods(0)
 {
 }

TimeScheduler::~TimeScheduler()
{
    delete _crit;
}

WebRtc_Word32 TimeScheduler::UpdateScheduler()
{
    CriticalSectionScoped cs(_crit);
    if(!_isStarted)
    {
        _isStarted = true;
        _lastPeriodMark = TickTime::Now();
        return 0;
    }
    
    
    if(_missedPeriods > 0)
    {
        _missedPeriods--;
        return 0;
    }

    
    TickTime tickNow = TickTime::Now();
    TickInterval amassedTicks = tickNow - _lastPeriodMark;
    WebRtc_Word64 amassedMs = amassedTicks.Milliseconds();

    
    WebRtc_Word32 periodsToClaim = (WebRtc_Word32)amassedMs /
        ((WebRtc_Word32)_periodicityInMs);

    
    
    
    if(periodsToClaim < 1)
    {
        periodsToClaim = 1;
    }

    
    
    
    
    for(WebRtc_Word32 i = 0; i < periodsToClaim; i++)
    {
        _lastPeriodMark += _periodicityInTicks;
    }

    
    
    _missedPeriods += periodsToClaim - 1;
    return 0;
}

WebRtc_Word32 TimeScheduler::TimeToNextUpdate(
    WebRtc_Word32& updateTimeInMS) const
{
    CriticalSectionScoped cs(_crit);
    
    
    if(_missedPeriods > 0)
    {
        updateTimeInMS = 0;
        return 0;
    }

    
    
    TickTime tickNow = TickTime::Now();
    TickInterval ticksSinceLastUpdate = tickNow - _lastPeriodMark;
    const WebRtc_Word32 millisecondsSinceLastUpdate =
        (WebRtc_Word32) ticksSinceLastUpdate.Milliseconds();

    updateTimeInMS = _periodicityInMs - millisecondsSinceLastUpdate;
    updateTimeInMS =  (updateTimeInMS < 0) ? 0 : updateTimeInMS;
    return 0;
}
} 
