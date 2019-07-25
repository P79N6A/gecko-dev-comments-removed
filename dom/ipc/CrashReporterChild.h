





































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
    virtual ~CrashReporterChild() {
      MOZ_COUNT_DTOR(CrashReporterChild);
    }
};
} 
} 
