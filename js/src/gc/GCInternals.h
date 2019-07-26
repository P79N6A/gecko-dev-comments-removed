






#ifndef jsgc_internal_h___
#define jsgc_internal_h___

#include "jsapi.h"

namespace js {
namespace gc {

void
MarkRuntime(JSTracer *trc, bool useSavedRoots = false);

class AutoCopyFreeListToArenas {
    JSRuntime *runtime;

  public:
    AutoCopyFreeListToArenas(JSRuntime *rt);
    ~AutoCopyFreeListToArenas();
};

struct AutoFinishGC
{
    AutoFinishGC(JSRuntime *rt);
};





class AutoTraceSession {
  public:
    AutoTraceSession(JSRuntime *rt, HeapState state = Tracing);
    ~AutoTraceSession();

  protected:
    JSRuntime *runtime;

  private:
    AutoTraceSession(const AutoTraceSession&) MOZ_DELETE;
    void operator=(const AutoTraceSession&) MOZ_DELETE;

    js::HeapState prevState;
};

struct AutoPrepareForTracing
{
    AutoFinishGC finish;
    AutoTraceSession session;
    AutoCopyFreeListToArenas copy;

    AutoPrepareForTracing(JSRuntime *rt);
};

} 
} 

#endif 
