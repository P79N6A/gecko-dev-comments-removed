









#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/voice_engine/monitor_module.h"

namespace webrtc  {

namespace voe  {

MonitorModule::MonitorModule() :
    _observerPtr(NULL),
    _callbackCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _lastProcessTime(TickTime::MillisecondTimestamp())
{
}

MonitorModule::~MonitorModule()
{
    delete &_callbackCritSect;
}

int32_t 
MonitorModule::RegisterObserver(MonitorObserver& observer)
{
    CriticalSectionScoped lock(&_callbackCritSect);
    if (_observerPtr)
    {
        return -1;
    }
    _observerPtr = &observer;
    return 0;
}

int32_t 
MonitorModule::DeRegisterObserver()
{
    CriticalSectionScoped lock(&_callbackCritSect);
    if (!_observerPtr)
    {
        return 0;
    }
    _observerPtr = NULL;
    return 0;
}

int32_t 
MonitorModule::Version(char* version,
                       uint32_t& remainingBufferInBytes,
                       uint32_t& position) const
{
    return 0;
}
   
int32_t 
MonitorModule::ChangeUniqueId(const int32_t id)
{
    return 0;
}

int32_t 
MonitorModule::TimeUntilNextProcess()
{
    uint32_t now = TickTime::MillisecondTimestamp();
    int32_t timeToNext =
        kAverageProcessUpdateTimeMs - (now - _lastProcessTime);
    return (timeToNext); 
}

int32_t 
MonitorModule::Process()
{
    _lastProcessTime = TickTime::MillisecondTimestamp();
    if (_observerPtr)
    {
        CriticalSectionScoped lock(&_callbackCritSect);
        _observerPtr->OnPeriodicProcess();
    }
    return 0;
}

}  

}  
