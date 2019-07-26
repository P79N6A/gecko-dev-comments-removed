





#ifndef gc_GCInternals_h
#define gc_GCInternals_h

#include "jscntxt.h"
#include "jsworkers.h"

#include "gc/Zone.h"
#include "vm/Runtime.h"

namespace js {
namespace gc {

void
MarkPersistentRootedChains(JSTracer *trc);

class AutoCopyFreeListToArenas
{
    JSRuntime *runtime;
    ZoneSelector selector;

  public:
    AutoCopyFreeListToArenas(JSRuntime *rt, ZoneSelector selector);
    ~AutoCopyFreeListToArenas();
};

struct AutoFinishGC
{
    AutoFinishGC(JSRuntime *rt);
};





class AutoTraceSession
{
  public:
    AutoTraceSession(JSRuntime *rt, HeapState state = Tracing);
    ~AutoTraceSession();

  protected:
    AutoLockForExclusiveAccess lock;
    JSRuntime *runtime;

  private:
    AutoTraceSession(const AutoTraceSession&) MOZ_DELETE;
    void operator=(const AutoTraceSession&) MOZ_DELETE;

    HeapState prevState;
};

struct AutoPrepareForTracing
{
    AutoFinishGC finish;
    AutoTraceSession session;
    AutoCopyFreeListToArenas copy;

    AutoPrepareForTracing(JSRuntime *rt, ZoneSelector selector);
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

#ifdef JS_GC_ZEAL

class AutoStopVerifyingBarriers
{
    GCRuntime *gc;
    bool restartPreVerifier;
    bool restartPostVerifier;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoStopVerifyingBarriers(JSRuntime *rt, bool isShutdown
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : gc(&rt->gc)
    {
        restartPreVerifier = gc->endVerifyPreBarriers() && !isShutdown;
        restartPostVerifier = gc->endVerifyPostBarriers() && !isShutdown &&
            JS::IsGenerationalGCEnabled(rt);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoStopVerifyingBarriers() {
        if (restartPreVerifier)
            gc->startVerifyPreBarriers();
        if (restartPostVerifier)
            gc->startVerifyPostBarriers();
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
