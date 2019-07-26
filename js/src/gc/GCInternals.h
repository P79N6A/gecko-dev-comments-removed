






#ifndef jsgc_internal_h___
#define jsgc_internal_h___

#include "jsapi.h"

namespace js {
namespace gc {

void
MarkRuntime(JSTracer *trc, bool useSavedRoots = false);

void
BufferGrayRoots(GCMarker *gcmarker);

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

class IncrementalSafety
{
    const char *reason_;

    IncrementalSafety(const char *reason) : reason_(reason) {}

  public:
    static IncrementalSafety Safe() { return IncrementalSafety(NULL); }
    static IncrementalSafety Unsafe(const char *reason) { return IncrementalSafety(reason); }

    typedef void (IncrementalSafety::* ConvertibleToBool)();
    void nonNull() {}

    operator ConvertibleToBool() const {
        return reason_ == NULL ? &IncrementalSafety::nonNull : 0;
    }

    const char *reason() {
        JS_ASSERT(reason_);
        return reason_;
    }
};

IncrementalSafety
IsIncrementalGCSafe(JSRuntime *rt);

#ifdef JSGC_ROOT_ANALYSIS
void *
GetAddressableGCThing(JSRuntime *rt, uintptr_t w);
#endif

#ifdef JS_GC_ZEAL
void
StartVerifyPreBarriers(JSRuntime *rt);

void
EndVerifyPreBarriers(JSRuntime *rt);

void
StartVerifyPostBarriers(JSRuntime *rt);

void
EndVerifyPostBarriers(JSRuntime *rt);

void
FinishVerifier(JSRuntime *rt);
#endif 

} 
} 

#endif 
