





































#include "mozilla/dom/PCrashReporterParent.h"

namespace mozilla {
namespace dom {
class CrashReporterParent :
    public PCrashReporterParent
{
public:
    CrashReporterParent();
    virtual ~CrashReporterParent();

 protected:
  virtual void ActorDestroy(ActorDestroyReason why);

  virtual bool
    RecvAddLibraryMappings(const InfallibleTArray<Mapping>& m);
};
} 
} 
