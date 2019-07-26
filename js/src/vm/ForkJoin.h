





#ifndef vm_ForkJoin_h
#define vm_ForkJoin_h

#include "jscntxt.h"

#include "gc/GCInternals.h"

#include "jit/Ion.h"



























































































































































































namespace js {

class ForkJoinActivation : public Activation
{
    uint8_t *prevIonTop_;

    
    
    
    
    gc::AutoStopVerifyingBarriers av_;

  public:
    ForkJoinActivation(JSContext *cx);
    ~ForkJoinActivation();
};

class ForkJoinSlice;

bool ForkJoin(JSContext *cx, CallArgs &args);



uint32_t ForkJoinSlices(JSContext *cx);

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

class ForkJoinSlice : public ThreadSafeContext
{
  public:
    
    const uint32_t sliceId;

    
    const uint32_t numSlices;

    
    ParallelBailoutRecord *const bailoutRecord;

#ifdef DEBUG
    
    IonLIRTraceData traceData;
#endif

    ForkJoinSlice(PerThreadData *perThreadData, uint32_t sliceId, uint32_t numSlices,
                  Allocator *allocator, ForkJoinShared *shared,
                  ParallelBailoutRecord *bailoutRecord);

    
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

    
    JSContext *acquireContext();
    void releaseContext();
    bool hasAcquiredContext() const;

    
    static inline ForkJoinSlice *Current();

    
    static bool InitializeTLS();

  private:
    friend class AutoRendezvous;
    friend class AutoSetForkJoinSlice;

#if defined(JS_THREADSAFE) && defined(JS_ION)
    
    static unsigned ThreadPrivateIndex;
    static bool TLSInitialized;
#endif

    ForkJoinShared *const shared;

    bool acquiredContext_;
};










class LockedJSContext
{
#if defined(JS_THREADSAFE) && defined(JS_ION)
    ForkJoinSlice *slice_;
#endif
    JSContext *cx_;

  public:
    LockedJSContext(ForkJoinSlice *slice)
#if defined(JS_THREADSAFE) && defined(JS_ION)
      : slice_(slice),
        cx_(slice->acquireContext())
#else
      : cx_(nullptr)
#endif
    { }

    ~LockedJSContext() {
#if defined(JS_THREADSAFE) && defined(JS_ION)
        slice_->releaseContext();
#endif
    }

    operator JSContext *() { return cx_; }
    JSContext *operator->() { return cx_; }
};

static inline bool
InParallelSection()
{
#ifdef JS_THREADSAFE
    ForkJoinSlice *current = ForkJoinSlice::Current();
    return current != nullptr;
#else
    return false;
#endif
}

bool InExclusiveParallelSection();

bool ParallelTestsShouldPass(JSContext *cx);




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

 inline js::ForkJoinSlice *
js::ForkJoinSlice::Current()
{
#if defined(JS_THREADSAFE) && defined(JS_ION)
    return (ForkJoinSlice*) PR_GetThreadPrivate(ThreadPrivateIndex);
#else
    return nullptr;
#endif
}

#endif 
