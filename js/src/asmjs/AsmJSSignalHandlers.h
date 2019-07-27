

















#ifndef asmjs_AsmJSSignalHandlers_h
#define asmjs_AsmJSSignalHandlers_h

#if defined(XP_MACOSX) && defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
# include <mach/mach.h>
# include "jslock.h"
#endif

struct JSRuntime;

namespace js {





bool
EnsureSignalHandlersInstalled(JSRuntime *rt);


extern void
InterruptRunningJitCode(JSRuntime *rt);

#if defined(XP_MACOSX) && defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)






class AsmJSMachExceptionHandler
{
    bool installed_;
    PRThread *thread_;
    mach_port_t port_;

    void uninstall();

  public:
    AsmJSMachExceptionHandler();
    ~AsmJSMachExceptionHandler() { uninstall(); }
    mach_port_t port() const { return port_; }
    bool installed() const { return installed_; }
    bool install(JSRuntime *rt);
};
#endif

} 

#endif 
