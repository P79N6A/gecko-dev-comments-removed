





#ifndef vm_ForkJoin_h
#define vm_ForkJoin_h

#include "jscntxt.h"
#include "vm/ThreadPool.h"
#include "jsgc.h"
#include "ion/Ion.h"



























































































































































































namespace js {

struct ForkJoinSlice;

bool ForkJoin(JSContext *cx, CallArgs &args);



uint32_t ForkJoinSlices(JSContext *cx);

#ifdef DEBUG
struct IonLIRTraceData {
    uint32_t bblock;
    uint32_t lir;
    uint32_t execModeInt;
    const char *lirOpName;
    const char *mirOpName;
    JSScript *script;
    jsbytecode *pc;
};
#endif






enum ParallelResult { TP_SUCCESS, TP_RETRY_SEQUENTIALLY, TP_RETRY_AFTER_GC, TP_FATAL };




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
    ParallelBailoutUnsupportedStringComparison,
    ParallelBailoutUnsupportedSparseArray,
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
                  JSScript *outermostScript,   
                  JSScript *currentScript,     
                  jsbytecode *currentPc);
    void addTrace(JSScript *script,
                  jsbytecode *pc);
};

struct ForkJoinShared;

struct ForkJoinSlice : ThreadSafeContext
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

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool check();

    
    JSRuntime *runtime();

    
    JSContext *acquireContext();
    void releaseContext();

    
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
      : cx_(NULL)
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
    return current != NULL;
#else
    return false;
#endif
}

bool ParallelTestsShouldPass(JSContext *cx);




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
ion::MethodStatus SpewEndCompile(ion::MethodStatus status);
void SpewMIR(ion::MDefinition *mir, const char *fmt, ...);
void SpewBailoutIR(uint32_t bblockId, uint32_t lirId,
                   const char *lir, const char *mir, JSScript *script, jsbytecode *pc);

#else

static inline bool SpewEnabled(SpewChannel channel) { return false; }
static inline void Spew(SpewChannel channel, const char *fmt, ...) { }
static inline void SpewBeginOp(JSContext *cx, const char *name) { }
static inline void SpewBailout(uint32_t count, HandleScript script,
                               jsbytecode *pc, ParallelBailoutCause cause) {}
static inline ExecutionStatus SpewEndOp(ExecutionStatus status) { return status; }
static inline void SpewBeginCompile(HandleScript script) { }
#ifdef JS_ION
static inline ion::MethodStatus SpewEndCompile(ion::MethodStatus status) { return status; }
static inline void SpewMIR(ion::MDefinition *mir, const char *fmt, ...) { }
#endif
static inline void SpewBailoutIR(uint32_t bblockId, uint32_t lirId,
                                 const char *lir, const char *mir,
                                 JSScript *script, jsbytecode *pc) { }

#endif 

} 
} 

 inline js::ForkJoinSlice *
js::ForkJoinSlice::Current()
{
#if defined(JS_THREADSAFE) && defined(JS_ION)
    return (ForkJoinSlice*) PR_GetThreadPrivate(ThreadPrivateIndex);
#else
    return NULL;
#endif
}

#endif 
