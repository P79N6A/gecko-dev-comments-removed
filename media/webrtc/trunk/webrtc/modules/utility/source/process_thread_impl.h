









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_PROCESS_THREAD_IMPL_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_PROCESS_THREAD_IMPL_H_

#include "webrtc/modules/utility/interface/process_thread.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/list_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class ProcessThreadImpl : public ProcessThread
{
public:
    ProcessThreadImpl();
    virtual ~ProcessThreadImpl();

    virtual int32_t Start();
    virtual int32_t Stop();

    virtual int32_t RegisterModule(const Module* module);
    virtual int32_t DeRegisterModule(const Module* module);

protected:
    static bool Run(void* obj);

    bool Process();

private:
    EventWrapper&           _timeEvent;
    CriticalSectionWrapper* _critSectModules;
    ListWrapper             _modules;
    ThreadWrapper*          _thread;
};
}  

#endif 
