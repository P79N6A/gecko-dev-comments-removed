





#ifndef gc_GCInternals_h
#define gc_GCInternals_h

#include "jscntxt.h"

#include "gc/Zone.h"
#include "vm/HelperThreads.h"
#include "vm/Runtime.h"

namespace js {
namespace gc {

void
MarkPersistentRootedChains(JSTracer* trc);

class AutoCopyFreeListToArenas
{
    JSRuntime* runtime;
    ZoneSelector selector;

  public:
    AutoCopyFreeListToArenas(JSRuntime* rt, ZoneSelector selector);
    ~AutoCopyFreeListToArenas();
};

struct AutoFinishGC
{
    explicit AutoFinishGC(JSRuntime* rt);
};





class AutoTraceSession
{
  public:
    explicit AutoTraceSession(JSRuntime* rt, HeapState state = Tracing);
    ~AutoTraceSession();

  protected:
    AutoLockForExclusiveAccess lock;
    JSRuntime* runtime;

  private:
    AutoTraceSession(const AutoTraceSession&) = delete;
    void operator=(const AutoTraceSession&) = delete;

    HeapState prevState;
};

struct AutoPrepareForTracing
{
    AutoFinishGC finish;
    AutoTraceSession session;
    AutoCopyFreeListToArenas copy;

    AutoPrepareForTracing(JSRuntime* rt, ZoneSelector selector);
};

class IncrementalSafety
{
    const char* reason_;

    explicit IncrementalSafety(const char* reason) : reason_(reason) {}

  public:
    static IncrementalSafety Safe() { return IncrementalSafety(nullptr); }
    static IncrementalSafety Unsafe(const char* reason) { return IncrementalSafety(reason); }

    explicit operator bool() const {
        return reason_ == nullptr;
    }

    const char* reason() {
        MOZ_ASSERT(reason_);
        return reason_;
    }
};

IncrementalSafety
IsIncrementalGCSafe(JSRuntime* rt);

#ifdef JS_GC_ZEAL

class AutoStopVerifyingBarriers
{
    GCRuntime* gc;
    bool restartPreVerifier;
    bool restartPostVerifier;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoStopVerifyingBarriers(JSRuntime* rt, bool isShutdown
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : gc(&rt->gc)
    {
        restartPreVerifier = gc->endVerifyPreBarriers() && !isShutdown;
        restartPostVerifier = gc->endVerifyPostBarriers() && !isShutdown &&
            JS::IsGenerationalGCEnabled(rt);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoStopVerifyingBarriers() {
        
        
        
        
        gcstats::Phase outer = gc->stats.currentPhase();
        if (outer != gcstats::PHASE_NONE)
            gc->stats.endPhase(outer);
        MOZ_ASSERT((gc->stats.currentPhase() == gcstats::PHASE_NONE) ||
                   (gc->stats.currentPhase() == gcstats::PHASE_GC_BEGIN) ||
                   (gc->stats.currentPhase() == gcstats::PHASE_GC_END));

        if (restartPreVerifier)
            gc->startVerifyPreBarriers();
        if (restartPostVerifier)
            gc->startVerifyPostBarriers();

        if (outer != gcstats::PHASE_NONE)
            gc->stats.beginPhase(outer);
    }
};
#else
struct AutoStopVerifyingBarriers
{
    AutoStopVerifyingBarriers(JSRuntime*, bool) {}
};
#endif

#ifdef JSGC_HASH_TABLE_CHECKS
void
CheckHashTablesAfterMovingGC(JSRuntime* rt);
#endif

struct MovingTracer : JS::CallbackTracer {
    explicit MovingTracer(JSRuntime* rt) : CallbackTracer(rt, Visit, TraceWeakMapKeysValues) {}

    static void Visit(JS::CallbackTracer* jstrc, void** thingp, JSGCTraceKind kind);
    static bool IsMovingTracer(JSTracer* trc) {
        return trc->isCallbackTracer() && trc->asCallbackTracer()->hasCallback(Visit);
    }
};

class AutoMaybeStartBackgroundAllocation
{
  private:
    JSRuntime* runtime;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoMaybeStartBackgroundAllocation(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
      : runtime(nullptr)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void tryToStartBackgroundAllocation(JSRuntime* rt) {
        runtime = rt;
    }

    ~AutoMaybeStartBackgroundAllocation() {
        if (runtime)
            runtime->gc.startBackgroundAllocTaskIfIdle();
    }
};


struct AutoSetThreadIsSweeping
{
#ifdef DEBUG
    explicit AutoSetThreadIsSweeping(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
      : threadData_(js::TlsPerThreadData.get())
    {
        MOZ_ASSERT(!threadData_->gcSweeping);
        threadData_->gcSweeping = true;
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoSetThreadIsSweeping() {
        MOZ_ASSERT(threadData_->gcSweeping);
        threadData_->gcSweeping = false;
    }

  private:
    PerThreadData* threadData_;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
#else
    AutoSetThreadIsSweeping() {}
#endif
};

} 
} 

#endif 
