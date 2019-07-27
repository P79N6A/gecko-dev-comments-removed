





#ifndef vm_ForkJoin_h
#define vm_ForkJoin_h

#include "mozilla/ThreadLocal.h"

#include <stdarg.h>

#include "jscntxt.h"

#include "gc/ForkJoinNursery.h"
#include "gc/GCInternals.h"

#include "jit/Ion.h"
#include "jit/IonTypes.h"

#ifdef DEBUG
  #define FORKJOIN_SPEW
#endif






































































































































































































































namespace js {

class ForkJoinActivation : public Activation
{
    uint8_t *prevJitTop_;

    
    
    
    
    gc::AutoStopVerifyingBarriers av_;

  public:
    explicit ForkJoinActivation(JSContext *cx);
    ~ForkJoinActivation();

    bool isProfiling() const {
        return false;
    }
};

class ForkJoinContext;

bool ForkJoin(JSContext *cx, CallArgs &args);















enum ParallelBailoutCause {
    ParallelBailoutNone = 0,

    
    
    ParallelBailoutExecution,

    
    
    ParallelBailoutInterrupt,

    
    ParallelBailoutCompilationSkipped,

    
    ParallelBailoutCompilationFailure,

    
    ParallelBailoutMainScriptNotPresent,

    
    ParallelBailoutOverRecursed,

    
    ParallelBailoutOutOfMemory,

    
    
    ParallelBailoutRequestedGC,
    ParallelBailoutRequestedZoneGC
};

namespace jit {
class BailoutStack;
class JitFrameIterator;
class IonBailoutIterator;
class RematerializedFrame;
}


struct ParallelBailoutRecord
{
    
    
    Vector<jit::RematerializedFrame *> *frames_;

    
    ParallelBailoutCause cause;

    
    
    jit::BailoutKind ionBailoutKind;

    ParallelBailoutRecord()
      : frames_(nullptr),
        cause(ParallelBailoutNone),
        ionBailoutKind(jit::Bailout_Inevitable)
    { }

    ~ParallelBailoutRecord();

    bool init(JSContext *cx);
    void reset();

    Vector<jit::RematerializedFrame *> &frames() { MOZ_ASSERT(frames_); return *frames_; }
    bool hasFrames() const { return frames_ && !frames_->empty(); }
    bool bailedOut() const { return cause != ParallelBailoutNone; }

    void joinCause(ParallelBailoutCause cause) {
        if (this->cause <= ParallelBailoutInterrupt &&
            (cause > ParallelBailoutInterrupt || cause > this->cause))
        {
            this->cause = cause;
        }
    }

    void setIonBailoutKind(jit::BailoutKind kind) {
        joinCause(ParallelBailoutExecution);
        ionBailoutKind = kind;
    }

    void rematerializeFrames(ForkJoinContext *cx, jit::JitFrameIterator &frameIter);
    void rematerializeFrames(ForkJoinContext *cx, jit::IonBailoutIterator &frameIter);
};

class ForkJoinShared;

class ForkJoinContext : public ThreadSafeContext
{
  public:
    
    ParallelBailoutRecord *const bailoutRecord;

#ifdef FORKJOIN_SPEW
    
    uint32_t maxWorkerId;
#endif

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    uint8_t *targetRegionStart;
    uint8_t *targetRegionEnd;

    ForkJoinContext(PerThreadData *perThreadData, ThreadPoolWorker *worker,
                    Allocator *allocator, ForkJoinShared *shared,
                    ParallelBailoutRecord *bailoutRecord);

    bool initialize();

    
    
    uint32_t workerId() const { return worker_->id(); }

    
    bool getSlice(uint16_t *sliceId) { return worker_->getSlice(this, sliceId); }

    
    bool isMainThread() const;

    
    
    
    
    
    
    
    
    void requestGC(JS::gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason);

    
    
    bool setPendingAbortFatal(ParallelBailoutCause cause);

    
    
    bool reportError(unsigned report) {
        if (report & JSREPORT_ERROR)
            return setPendingAbortFatal(ParallelBailoutExecution);
        return true;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool check();

    
    JSRuntime *runtime();

    
    JSContext *acquireJSContext();
    void releaseJSContext();
    bool hasAcquiredJSContext() const;

    
    static inline ForkJoinContext *current();

    
    static bool initializeTls();

    
    static size_t offsetOfWorker() {
        return offsetof(ForkJoinContext, worker_);
    }

#ifdef JSGC_FJGENERATIONAL
    
    gc::ForkJoinNursery &nursery() { return nursery_; }

    
    
    void evacuateLiveData() { nursery_.evacuatingGC(); }

    
    
    static size_t offsetOfFJNursery() {
        return offsetof(ForkJoinContext, nursery_);
    }
#endif

  private:
    friend class AutoSetForkJoinContext;

    
    static mozilla::ThreadLocal<ForkJoinContext*> tlsForkJoinContext;

    ForkJoinShared *const shared_;

#ifdef JSGC_FJGENERATIONAL
    gc::ForkJoinGCShared gcShared_;
    gc::ForkJoinNursery nursery_;
#endif

    ThreadPoolWorker *worker_;

    bool acquiredJSContext_;

    
    
    JS::AutoSuppressGCAnalysis nogc_;
};










class LockedJSContext
{
    ForkJoinContext *cx_;
    JSContext *jscx_;

  public:
    explicit LockedJSContext(ForkJoinContext *cx)
      : cx_(cx),
        jscx_(cx->acquireJSContext())
    { }

    ~LockedJSContext() {
        cx_->releaseJSContext();
    }

    operator JSContext *() { return jscx_; }
    JSContext *operator->() { return jscx_; }
};

bool InExclusiveParallelSection();

bool ParallelTestsShouldPass(JSContext *cx);

void RequestInterruptForForkJoin(JSRuntime *rt, JSRuntime::InterruptMode mode);

bool intrinsic_SetForkJoinTargetRegion(JSContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo intrinsic_SetForkJoinTargetRegionInfo;

bool intrinsic_ClearThreadLocalArenas(JSContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo intrinsic_ClearThreadLocalArenasInfo;




namespace jit {
    class MDefinition;
}

namespace parallel {

enum ExecutionStatus {
    
    ExecutionFatal,

    
    ExecutionSequential,

    
    ExecutionWarmup,

    
    ExecutionParallel
};

enum SpewChannel {
    SpewOps,
    SpewCompile,
    SpewBailouts,
    SpewGC,
    NumSpewChannels
};

#ifdef FORKJOIN_SPEW

bool SpewEnabled(SpewChannel channel);
void Spew(SpewChannel channel, const char *fmt, ...);
void SpewVA(SpewChannel channel, const char *fmt, va_list args);
void SpewBeginOp(JSContext *cx, const char *name);
void SpewBailout(uint32_t count, HandleScript script, jsbytecode *pc,
                 ParallelBailoutCause cause);
ExecutionStatus SpewEndOp(ExecutionStatus status);
void SpewBeginCompile(HandleScript script);
jit::MethodStatus SpewEndCompile(jit::MethodStatus status);
void SpewMIR(jit::MDefinition *mir, const char *fmt, ...);

#else

static inline bool SpewEnabled(SpewChannel channel) { return false; }
static inline void Spew(SpewChannel channel, const char *fmt, ...) { }
static inline void SpewVA(SpewChannel channel, const char *fmt, va_list args) { }
static inline void SpewBeginOp(JSContext *cx, const char *name) { }
static inline void SpewBailout(uint32_t count, HandleScript script,
                               jsbytecode *pc, ParallelBailoutCause cause) {}
static inline ExecutionStatus SpewEndOp(ExecutionStatus status) { return status; }
static inline void SpewBeginCompile(HandleScript script) { }
static inline jit::MethodStatus SpewEndCompile(jit::MethodStatus status) { return status; }
static inline void SpewMIR(jit::MDefinition *mir, const char *fmt, ...) { }

#endif 

} 
} 

 inline js::ForkJoinContext *
js::ForkJoinContext::current()
{
    return tlsForkJoinContext.get();
}

namespace js {

static inline bool
InParallelSection()
{
    return ForkJoinContext::current() != nullptr;
}

} 

#endif 
