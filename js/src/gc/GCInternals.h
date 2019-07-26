





#ifndef gc_GCInternals_h
#define gc_GCInternals_h

#include "jsworkers.h"

#include "vm/Runtime.h"

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
    AutoPauseWorkersForGC pause;
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
    static IncrementalSafety Safe() { return IncrementalSafety(nullptr); }
    static IncrementalSafety Unsafe(const char *reason) { return IncrementalSafety(reason); }

    typedef void (IncrementalSafety::* ConvertibleToBool)();
    void nonNull() {}

    operator ConvertibleToBool() const {
        return reason_ == nullptr ? &IncrementalSafety::nonNull : 0;
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

class AutoStopVerifyingBarriers
{
    JSRuntime *runtime;
    bool restartPreVerifier;
    bool restartPostVerifier;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoStopVerifyingBarriers(JSRuntime *rt, bool isShutdown
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : runtime(rt)
    {
        restartPreVerifier = !isShutdown && rt->gcVerifyPreData;
        restartPostVerifier = !isShutdown && rt->gcVerifyPostData && rt->gcGenerationalEnabled;
        if (rt->gcVerifyPreData)
            EndVerifyPreBarriers(rt);
        if (rt->gcVerifyPostData)
            EndVerifyPostBarriers(rt);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoStopVerifyingBarriers() {
        if (restartPreVerifier)
            StartVerifyPreBarriers(runtime);
        if (restartPostVerifier)
            StartVerifyPostBarriers(runtime);
    }
};
#else
struct AutoStopVerifyingBarriers
{
    AutoStopVerifyingBarriers(JSRuntime *, bool) {}
};
#endif

} 
} 

#endif 
