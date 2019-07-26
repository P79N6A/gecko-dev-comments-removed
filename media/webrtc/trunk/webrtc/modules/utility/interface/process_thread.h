









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_PROCESS_THREAD_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_PROCESS_THREAD_H_

#include "webrtc/typedefs.h"

namespace webrtc {
class Module;

class ProcessThread
{
public:
    static ProcessThread* CreateProcessThread();
    static void DestroyProcessThread(ProcessThread* module);

    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;

    virtual int32_t RegisterModule(Module* module) = 0;
    virtual int32_t DeRegisterModule(const Module* module) = 0;
protected:
    virtual ~ProcessThread();
};
}  
#endif 
