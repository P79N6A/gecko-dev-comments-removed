









#include "webrtc/modules/audio_conference_mixer/source/time_scheduler.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {
TimeScheduler::TimeScheduler(const uint32_t periodicityInMs)
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

int32_t TimeScheduler::UpdateScheduler()
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
    int64_t amassedMs = amassedTicks.Milliseconds();

    
    int32_t periodsToClaim = static_cast<int32_t>(amassedMs /
        static_cast<int32_t>(_periodicityInMs));

    
    
    
    if(periodsToClaim < 1)
    {
        periodsToClaim = 1;
    }

    
    
    
    
    for(int32_t i = 0; i < periodsToClaim; i++)
    {
        _lastPeriodMark += _periodicityInTicks;
    }

    
    
    _missedPeriods += periodsToClaim - 1;
    return 0;
}

int32_t TimeScheduler::TimeToNextUpdate(
    int32_t& updateTimeInMS) const
{
    CriticalSectionScoped cs(_crit);
    
    
    if(_missedPeriods > 0)
    {
        updateTimeInMS = 0;
        return 0;
    }

    
    
    TickTime tickNow = TickTime::Now();
    TickInterval ticksSinceLastUpdate = tickNow - _lastPeriodMark;
    const int32_t millisecondsSinceLastUpdate =
        static_cast<int32_t>(ticksSinceLastUpdate.Milliseconds());

    updateTimeInMS = _periodicityInMs - millisecondsSinceLastUpdate;
    updateTimeInMS =  (updateTimeInMS < 0) ? 0 : updateTimeInMS;
    return 0;
}
}  
