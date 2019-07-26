









#ifndef WEBRTC_VOICE_ENGINE_MONITOR_MODULE_H
#define WEBRTC_VOICE_ENGINE_MONITOR_MODULE_H

#include "webrtc/modules/interface/module.h"
#include "webrtc/typedefs.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

class MonitorObserver
{
public:
    virtual void OnPeriodicProcess() = 0;
protected:
    virtual ~MonitorObserver() {}
};


namespace webrtc {
class CriticalSectionWrapper;

namespace voe {

class MonitorModule : public Module
{
public:
    int32_t RegisterObserver(MonitorObserver& observer);

    int32_t DeRegisterObserver();

    MonitorModule();

    virtual ~MonitorModule();
public:	
    int32_t Version(char* version,
                    uint32_t& remainingBufferInBytes,
                    uint32_t& position) const;

    int32_t ChangeUniqueId(int32_t id);

    int32_t TimeUntilNextProcess();

    int32_t Process();
private:
    enum { kAverageProcessUpdateTimeMs = 1000 };
    MonitorObserver* _observerPtr;
    CriticalSectionWrapper&	_callbackCritSect;
    int32_t _lastProcessTime;
};

}  

}  

#endif
