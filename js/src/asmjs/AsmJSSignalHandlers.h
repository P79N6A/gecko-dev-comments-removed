

















#ifndef asmjs_AsmJSSignalHandlers_h
#define asmjs_AsmJSSignalHandlers_h

struct JSRuntime;

#ifdef XP_MACOSX
# include <mach/mach.h>
# include "jslock.h"
#endif

namespace js {





bool
EnsureSignalHandlersInstalled(JSRuntime *rt);


extern void
InterruptRunningJitCode(JSRuntime *rt);







#ifdef XP_MACOSX
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
