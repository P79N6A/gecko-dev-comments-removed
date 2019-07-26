









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_PROCESS_THREAD_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_PROCESS_THREAD_H_

#include "typedefs.h"

namespace webrtc {
class Module;

class ProcessThread
{
public:
    static ProcessThread* CreateProcessThread();
    static void DestroyProcessThread(ProcessThread* module);

    virtual WebRtc_Word32 Start() = 0;
    virtual WebRtc_Word32 Stop() = 0;

    virtual WebRtc_Word32 RegisterModule(const Module* module) = 0;
    virtual WebRtc_Word32 DeRegisterModule(const Module* module) = 0;
protected:
    virtual ~ProcessThread();
};
} 
#endif 
