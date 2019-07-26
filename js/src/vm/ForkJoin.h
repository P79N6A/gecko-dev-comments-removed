





#ifndef vm_ForkJoin_h
#define vm_ForkJoin_h

#include "mozilla/ThreadLocal.h"

#include "jscntxt.h"

#include "gc/GCInternals.h"

#include "jit/Ion.h"






































































































































































































namespace js {

class ForkJoinActivation : public Activation
{
    uint8_t *prevJitTop_;

    
    
    
    
    gc::AutoStopVerifyingBarriers av_;

  public:
    explicit ForkJoinActivation(JSContext *cx);
    ~ForkJoinActivation();
};

class ForkJoinContext;

bool ForkJoin(JSContext *cx, CallArgs &args);

struct IonLIRTraceData {
    uint32_t blockIndex;
    uint32_t lirIndex;
    uint32_t execModeInt;
    const char *lirOpName;
    const char *mirOpName;
    JSScript *script;
    jsbytecode *pc;
};




enum ParallelBailoutCause {
    ParallelBailoutNone,

    
    ParallelBailoutCompilationSkipped,

    
    ParallelBailoutCompilationFailure,

    
    
    ParallelBailoutInterrupt,

    
    ParallelBailoutFailedIC,

    
    ParallelBailoutHeapBusy,

    ParallelBailoutMainScriptNotPresent,
    ParallelBailoutCalledToUncompiledScript,
    ParallelBailoutIllegalWrite,
    ParallelBailoutAccessToIntrinsic,
    ParallelBailoutOverRecursed,
    ParallelBailoutOutOfMemory,
    ParallelBailoutUnsupported,
    ParallelBailoutUnsupportedVM,
    ParallelBailoutUnsupportedStringComparison,
    ParallelBailoutRequestedGC,
    ParallelBailoutRequestedZoneGC,
};

struct ParallelBailoutTrace {
    JSScript *script;
    jsbytecode *bytecode;
};


struct ParallelBailoutRecord {
    JSScript *topScript;
    ParallelBailoutCause cause;

    
    
    static const uint32_t MaxDepth = 1;
    uint32_t depth;
    ParallelBailoutTrace trace[MaxDepth];

    void init(JSContext *cx);
    void reset(JSContext *cx);
    void setCause(ParallelBailoutCause cause,
                  JSScript *outermostScript = nullptr,   
                  JSScript *currentScript = nullptr,     
                  jsbytecode *currentPc = nullptr);
    void updateCause(ParallelBailoutCause cause,
                     JSScript *outermostScript,
                     JSScript *currentScript,
                     jsbytecode *currentPc);
    void addTrace(JSScript *script,
                  jsbytecode *pc);
};

struct ForkJoinShared;

class ForkJoinContext : public ThreadSafeContext
{
  public:
    
    ParallelBailoutRecord *const bailoutRecord;

#ifdef DEBUG
    
    IonLIRTraceData traceData;

    
    uint32_t maxWorkerId;
#endif

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    uint8_t *targetRegionStart;
    uint8_t *targetRegionEnd;

    ForkJoinContext(PerThreadData *perThreadData, ThreadPoolWorker *worker,
                    Allocator *allocator, ForkJoinShared *shared,
                    ParallelBailoutRecord *bailoutRecord);

    
    
    uint32_t workerId() const { return worker_->id(); }

    
    bool getSlice(uint16_t *sliceId) { return worker_->getSlice(this, sliceId); }

    
    bool isMainThread() const;

    
    
    
    
    
    
    
    
    void requestGC(JS::gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason);

    
    
    bool setPendingAbortFatal(ParallelBailoutCause cause);

    
    
    bool reportError(ParallelBailoutCause cause, unsigned report) {
        if (report & JSREPORT_ERROR)
            return setPendingAbortFatal(cause);
        return true;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool check();

    
    JSRuntime *runtime();

    
    JSContext *acquireJSContext();
    void releaseJSContext();
    bool hasAcquiredJSContext() const;

    
    static inline ForkJoinContext *current();

    
    static bool initialize();

    
    static size_t offsetOfWorker() {
        return offsetof(ForkJoinContext, worker_);
    }

  private:
    friend class AutoSetForkJoinContext;

    
    static mozilla::ThreadLocal<ForkJoinContext*> tlsForkJoinContext;

    ForkJoinShared *const shared_;

    ThreadPoolWorker *worker_;

    bool acquiredJSContext_;

    
    
    JS::AutoAssertNoGC nogc_;
};










class LockedJSContext
{
#if defined(JS_THREADSAFE) && defined(JS_ION)
    ForkJoinContext *cx_;
#endif
    JSContext *jscx_;

  public:
    explicit LockedJSContext(ForkJoinContext *cx)
#if defined(JS_THREADSAFE) && defined(JS_ION)
      : cx_(cx),
        jscx_(cx->acquireJSContext())
#else
      : jscx_(nullptr)
#endif
    { }

    ~LockedJSContext() {
#if defined(JS_THREADSAFE) && defined(JS_ION)
        cx_->releaseJSContext();
#endif
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
    NumSpewChannels
};

#if defined(DEBUG) && defined(JS_THREADSAFE) && defined(JS_ION)

bool SpewEnabled(SpewChannel channel);
void Spew(SpewChannel channel, const char *fmt, ...);
void SpewBeginOp(JSContext *cx, const char *name);
void SpewBailout(uint32_t count, HandleScript script, jsbytecode *pc,
                 ParallelBailoutCause cause);
ExecutionStatus SpewEndOp(ExecutionStatus status);
void SpewBeginCompile(HandleScript script);
jit::MethodStatus SpewEndCompile(jit::MethodStatus status);
void SpewMIR(jit::MDefinition *mir, const char *fmt, ...);
void SpewBailoutIR(IonLIRTraceData *data);

#else

static inline bool SpewEnabled(SpewChannel channel) { return false; }
static inline void Spew(SpewChannel channel, const char *fmt, ...) { }
static inline void SpewBeginOp(JSContext *cx, const char *name) { }
static inline void SpewBailout(uint32_t count, HandleScript script,
                               jsbytecode *pc, ParallelBailoutCause cause) {}
static inline ExecutionStatus SpewEndOp(ExecutionStatus status) { return status; }
static inline void SpewBeginCompile(HandleScript script) { }
#ifdef JS_ION
static inline jit::MethodStatus SpewEndCompile(jit::MethodStatus status) { return status; }
static inline void SpewMIR(jit::MDefinition *mir, const char *fmt, ...) { }
#endif
static inline void SpewBailoutIR(IonLIRTraceData *data) { }

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
