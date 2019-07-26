





#ifndef mozilla_dom_CrashReporterChild_h
#define mozilla_dom_CrashReporterChild_h

#include "mozilla/dom/PCrashReporterChild.h"

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
};

} 
} 

#endif 
