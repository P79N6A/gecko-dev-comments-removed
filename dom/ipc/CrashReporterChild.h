





































#include "mozilla/dom/PCrashReporterChild.h"
#include "mozilla/Util.h"
#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#include "nsXULAppAPI.h"
#endif

namespace mozilla {
namespace dom {
class CrashReporterChild :
    public PCrashReporterChild
{
 public:
    CrashReporterChild() {
        MOZ_COUNT_CTOR(CrashReporterChild);
    }
    ~CrashReporterChild() {
        MOZ_COUNT_DTOR(CrashReporterChild);
    }

    static PCrashReporterChild* GetCrashReporter();

    template<class Toplevel>
    static void CreateCrashReporter(Toplevel* actor) {
#ifdef MOZ_CRASHREPORTER
        MOZ_ASSERT(actor->ManagedPCrashReporterChild().Length() == 0);
        actor->SendPCrashReporterConstructor(
                CrashReporter::CurrentThreadId(),
                XRE_GetProcessType());
#endif
    }
};
} 
} 
