





#if defined(XP_WIN)
# include <io.h>     
#else
# include <unistd.h> 
#endif

#include "vm/ForkJoin.h"

#include "mozilla/ThreadLocal.h"

#include "jscntxt.h"
#include "jslock.h"
#include "jsprf.h"

#include "builtin/TypedObject.h"
#include "jit/BaselineJIT.h"
#include "jit/JitCommon.h"
#include "jit/RematerializedFrame.h"
#ifdef FORKJOIN_SPEW
# include "jit/Ion.h"
# include "jit/JitCompartment.h"
# include "jit/MIR.h"
# include "jit/MIRGraph.h"
#endif
#include "vm/Monitor.h"

#include "gc/ForkJoinNursery-inl.h"
#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::parallel;
using namespace js::jit;

using mozilla::ThreadLocal;







static bool
ExecuteSequentially(JSContext *cx_, HandleValue funVal, uint16_t *sliceStart,
                    uint16_t sliceEnd);

static bool
ForkJoinSequentially(JSContext *cx, CallArgs &args)
{
    RootedValue argZero(cx, args[0]);
    uint16_t sliceStart = uint16_t(args[1].toInt32());
    uint16_t sliceEnd = uint16_t(args[2].toInt32());
    if (!ExecuteSequentially(cx, argZero, &sliceStart, sliceEnd))
        return false;
    MOZ_ASSERT(sliceStart == sliceEnd);
    return true;
}






static bool
ExecuteSequentially(JSContext *cx, HandleValue funVal, uint16_t *sliceStart,
                    uint16_t sliceEnd)
{
    FastInvokeGuard fig(cx, funVal);
    InvokeArgs &args = fig.args();
    if (!args.init(3))
        return false;
    args.setCallee(funVal);
    args.setThis(UndefinedValue());
    args[0].setInt32(0);
    args[1].setInt32(*sliceStart);
    args[2].setInt32(sliceEnd);
    if (!fig.invoke(cx))
        return false;
    *sliceStart = (uint16_t)(args.rval().toInt32());
    return true;
}

ThreadLocal<ForkJoinContext*> ForkJoinContext::tlsForkJoinContext;

 bool
ForkJoinContext::initializeTls()
{
    if (!tlsForkJoinContext.initialized()) {
        if (!tlsForkJoinContext.init())
            return false;
    }
    return true;
}









namespace js {



enum ForkJoinMode {
    
    

    
    
    
    
    ForkJoinModeNormal,

    
    
    
    ForkJoinModeCompile,

    
    
    ForkJoinModeParallel,

    
    
    
    ForkJoinModeRecover,

    
    
    ForkJoinModeBailout,

    NumForkJoinModes
};

class ForkJoinOperation
{
  public:
    
    static const uint32_t MAX_BAILOUTS = 3;
    uint32_t bailouts;

    ForkJoinOperation(JSContext *cx, HandleFunction fun, uint16_t sliceStart,
                      uint16_t sliceEnd, ForkJoinMode mode, HandleObject updatable);
    ExecutionStatus apply();

  private:
    
    
    
    
    
    
    
    
    enum TrafficLight {
        RedLight,
        GreenLight
    };

    struct WorklistData {
        
        
        bool calleesEnqueued;

        
        
        uint32_t warmUpCount;

        
        
        uint32_t stallCount;

        void reset() {
            calleesEnqueued = false;
            warmUpCount = 0;
            stallCount = 0;
        }
    };

    JSContext *cx_;
    HandleFunction fun_;
    HandleObject updatable_;
    uint16_t sliceStart_;
    uint16_t sliceEnd_;
    Vector<ParallelBailoutRecord> bailoutRecords_;
    AutoScriptVector worklist_;
    Vector<WorklistData, 16> worklistData_;
    ForkJoinMode mode_;

    TrafficLight enqueueInitialScript(ExecutionStatus *status);
    TrafficLight compileForParallelExecution(ExecutionStatus *status);
    TrafficLight warmupExecution(bool stopIfComplete, ExecutionStatus *status);
    TrafficLight parallelExecution(ExecutionStatus *status);
    TrafficLight sequentialExecution(bool disqualified, ExecutionStatus *status);
    TrafficLight recoverFromBailout(ExecutionStatus *status);
    TrafficLight fatalError(ExecutionStatus *status);
    bool isInitialScript(HandleScript script);
    bool reportBailoutWarnings();
    bool invalidateBailedOutScripts();
    ExecutionStatus sequentialExecution(bool disqualified);

    TrafficLight appendCallTargetsToWorklist(uint32_t index, ExecutionStatus *status);
    TrafficLight appendCallTargetToWorklist(HandleScript script, ExecutionStatus *status);
    bool addToWorklist(HandleScript script);
    inline bool hasScript(Vector<types::RecompileInfo> &scripts, JSScript *script);
}; 

class ForkJoinShared : public ParallelJob, public Monitor
{
#ifdef JSGC_FJGENERATIONAL
    friend class gc::ForkJoinGCShared;
#endif

    
    

    JSContext *const cx_;                    
    ThreadPool *const threadPool_;           
    HandleFunction fun_;                     
    HandleObject updatable_;                 
    uint16_t sliceStart_;                    
    uint16_t sliceEnd_;                      
    PRLock *cxLock_;                         
    Vector<ParallelBailoutRecord> &records_; 

    
    
    
    

    Vector<Allocator *, 16> allocators_;

    
    
    
    

    bool gcRequested_;              
    JS::gcreason::Reason gcReason_; 
    Zone *gcZone_;                  

    
    
    
    

    
    mozilla::Atomic<bool, mozilla::ReleaseAcquire> abort_;

    
    mozilla::Atomic<bool, mozilla::ReleaseAcquire> fatal_;

  public:
    ForkJoinShared(JSContext *cx,
                   ThreadPool *threadPool,
                   HandleFunction fun,
                   HandleObject updatable,
                   uint16_t sliceStart,
                   uint16_t sliceEnd,
                   Vector<ParallelBailoutRecord> &records);
    ~ForkJoinShared();

    bool init();

    ParallelResult execute();

    
    virtual bool executeFromWorker(ThreadPoolWorker *worker, uintptr_t stackLimit) MOZ_OVERRIDE;

    
    virtual bool executeFromMainThread(ThreadPoolWorker *worker) MOZ_OVERRIDE;

    
    void executePortion(PerThreadData *perThread, ThreadPoolWorker *worker);

    
    
    
    void transferArenasToCompartmentAndProcessGCRequests();


    
    void requestGC(JS::gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason);

    
    void setAbortFlagDueToInterrupt(ForkJoinContext &cx);
    void setAbortFlagAndRequestInterrupt(bool fatal);

    
    void setPendingAbortFatal() { fatal_ = true; }

    JSRuntime *runtime() { return cx_->runtime(); }
    JS::Zone *zone() { return cx_->zone(); }
    JSCompartment *compartment() { return cx_->compartment(); }

    JSContext *acquireJSContext() { PR_Lock(cxLock_); return cx_; }
    void releaseJSContext() { PR_Unlock(cxLock_); }

    HandleObject updatable() { return updatable_; }
};

class AutoEnterWarmup
{
    JSRuntime *runtime_;

  public:
    explicit AutoEnterWarmup(JSRuntime *runtime) : runtime_(runtime) { runtime_->forkJoinWarmup++; }
    ~AutoEnterWarmup() { runtime_->forkJoinWarmup--; }
};

class AutoSetForkJoinContext
{
  public:
    explicit AutoSetForkJoinContext(ForkJoinContext *threadCx) {
        ForkJoinContext::tlsForkJoinContext.set(threadCx);
    }

    ~AutoSetForkJoinContext() {
        ForkJoinContext::tlsForkJoinContext.set(nullptr);
    }
};

} 








ForkJoinActivation::ForkJoinActivation(JSContext *cx)
  : Activation(cx, ForkJoin),
    prevJitTop_(cx->mainThread().jitTop),
    av_(cx->runtime(), false)
{
    
    
    
    
    
    

    if (JS::IsIncrementalGCInProgress(cx->runtime())) {
        JS::PrepareForIncrementalGC(cx->runtime());
        JS::FinishIncrementalGC(cx->runtime(), JS::gcreason::API);
    }

    cx->runtime()->gc.evictNursery();

    cx->runtime()->gc.waitBackgroundSweepEnd();

    JS_ASSERT(!cx->runtime()->needsIncrementalBarrier());
    JS_ASSERT(!cx->zone()->needsIncrementalBarrier());
}

ForkJoinActivation::~ForkJoinActivation()
{
    cx_->perThreadData->jitTop = prevJitTop_;
}








static const char *ForkJoinModeString(ForkJoinMode mode);

bool
js::ForkJoin(JSContext *cx, CallArgs &args)
{
    JS_ASSERT(args.length() == 5); 
    JS_ASSERT(args[0].isObject());
    JS_ASSERT(args[0].toObject().is<JSFunction>());
    JS_ASSERT(args[1].isInt32());
    JS_ASSERT(args[2].isInt32());
    JS_ASSERT(args[3].isInt32());
    JS_ASSERT(args[3].toInt32() < NumForkJoinModes);
    JS_ASSERT(args[4].isObjectOrNull());

    if (!CanUseExtraThreads())
        return ForkJoinSequentially(cx, args);

    RootedFunction fun(cx, &args[0].toObject().as<JSFunction>());
    uint16_t sliceStart = (uint16_t)(args[1].toInt32());
    uint16_t sliceEnd = (uint16_t)(args[2].toInt32());
    ForkJoinMode mode = (ForkJoinMode)(args[3].toInt32());
    RootedObject updatable(cx, args[4].toObjectOrNull());

    MOZ_ASSERT(sliceStart == args[1].toInt32());
    MOZ_ASSERT(sliceEnd == args[2].toInt32());
    MOZ_ASSERT(sliceStart <= sliceEnd);

    ForkJoinOperation op(cx, fun, sliceStart, sliceEnd, mode, updatable);
    ExecutionStatus status = op.apply();
    if (status == ExecutionFatal)
        return false;

    switch (mode) {
      case ForkJoinModeNormal:
      case ForkJoinModeCompile:
        return true;

      case ForkJoinModeParallel:
        if (status == ExecutionParallel && op.bailouts == 0)
            return true;
        break;

      case ForkJoinModeRecover:
        if (status != ExecutionSequential && op.bailouts > 0)
            return true;
        break;

      case ForkJoinModeBailout:
        if (status != ExecutionParallel)
            return true;
        break;

      case NumForkJoinModes:
        break;
    }

    const char *statusString = "?";
    switch (status) {
      case ExecutionSequential: statusString = "seq"; break;
      case ExecutionParallel: statusString = "par"; break;
      case ExecutionWarmup: statusString = "warmup"; break;
      case ExecutionFatal: statusString = "fatal"; break;
    }

    if (ParallelTestsShouldPass(cx)) {
        JS_ReportError(cx, "ForkJoin: mode=%s status=%s bailouts=%d",
                       ForkJoinModeString(mode), statusString, op.bailouts);
        return false;
    }
    return true;
}

static const char *
ForkJoinModeString(ForkJoinMode mode) {
    switch (mode) {
      case ForkJoinModeNormal: return "normal";
      case ForkJoinModeCompile: return "compile";
      case ForkJoinModeParallel: return "parallel";
      case ForkJoinModeRecover: return "recover";
      case ForkJoinModeBailout: return "bailout";
      case NumForkJoinModes: return "max";
    }
    return "???";
}

ForkJoinOperation::ForkJoinOperation(JSContext *cx, HandleFunction fun, uint16_t sliceStart,
                                     uint16_t sliceEnd, ForkJoinMode mode, HandleObject updatable)
  : bailouts(0),
    cx_(cx),
    fun_(fun),
    updatable_(updatable),
    sliceStart_(sliceStart),
    sliceEnd_(sliceEnd),
    bailoutRecords_(cx),
    worklist_(cx),
    worklistData_(cx),
    mode_(mode)
{ }

ExecutionStatus
ForkJoinOperation::apply()
{
    ExecutionStatus status;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT_IF(!IsBaselineEnabled(cx_), !IsIonEnabled(cx_));
    if (!IsBaselineEnabled(cx_) || !IsIonEnabled(cx_))
        return sequentialExecution(true);

    SpewBeginOp(cx_, "ForkJoinOperation");

    
    unsigned numWorkers = cx_->runtime()->threadPool.numWorkers();

    if (!bailoutRecords_.resize(numWorkers))
        return SpewEndOp(ExecutionFatal);

    for (uint32_t i = 0; i < numWorkers; i++) {
        if (!bailoutRecords_[i].init(cx_))
            return SpewEndOp(ExecutionFatal);
    }

    if (enqueueInitialScript(&status) == RedLight)
        return SpewEndOp(status);

    Spew(SpewOps, "Execution mode: %s", ForkJoinModeString(mode_));
    switch (mode_) {
      case ForkJoinModeNormal:
      case ForkJoinModeCompile:
      case ForkJoinModeBailout:
        break;

      case ForkJoinModeParallel:
      case ForkJoinModeRecover:
        
        
        
        
        if (ParallelTestsShouldPass(cx_) && worklist_.length() != 0) {
            JS_ReportError(cx_, "ForkJoin: compilation required in par or bailout mode");
            return SpewEndOp(ExecutionFatal);
        }
        break;

      case NumForkJoinModes:
        MOZ_CRASH("Invalid mode");
    }

    while (bailouts < MAX_BAILOUTS) {
        for (uint32_t i = 0; i < numWorkers; i++)
            bailoutRecords_[i].reset();

        if (compileForParallelExecution(&status) == RedLight)
            return SpewEndOp(status);

        JS_ASSERT(worklist_.length() == 0);
        if (parallelExecution(&status) == RedLight)
            return SpewEndOp(status);

        if (recoverFromBailout(&status) == RedLight)
            return SpewEndOp(status);
    }

    
    return SpewEndOp(sequentialExecution(true));
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::enqueueInitialScript(ExecutionStatus *status)
{
    
    

    
    if (!fun_->is<JSFunction>())
        return sequentialExecution(true, status);

    RootedFunction callee(cx_, &fun_->as<JSFunction>());

    if (!callee->isInterpreted() || !callee->isSelfHostedBuiltin())
        return sequentialExecution(true, status);

    
    
    
    RootedScript script(cx_, callee->getOrCreateScript(cx_));
    if (!script)
        return RedLight;

    if (script->hasParallelIonScript()) {
        
        JitCompartment *jitComp = cx_->compartment()->jitCompartment();
        if (!jitComp->notifyOfActiveParallelEntryScript(cx_, script)) {
            *status = ExecutionFatal;
            return RedLight;
        }

        if (!script->parallelIonScript()->hasUncompiledCallTarget()) {
            Spew(SpewOps, "Script %p:%s:%d already compiled, no uncompiled callees",
                 script.get(), script->filename(), script->lineno());
            return GreenLight;
        }

        Spew(SpewOps, "Script %p:%s:%d already compiled, may have uncompiled callees",
             script.get(), script->filename(), script->lineno());
    }

    
    if (addToWorklist(script) == RedLight)
        return fatalError(status);
    return GreenLight;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::compileForParallelExecution(ExecutionStatus *status)
{
    
    

    
    
    
    
    
    

    RootedFunction fun(cx_);
    RootedScript script(cx_);

    
    
    const uint32_t stallThreshold = 3;

    
    
    
    
    
    
    while (true) {
        bool offMainThreadCompilationsInProgress = false;
        bool gatheringTypeInformation = false;

        
        for (uint32_t i = 0; i < worklist_.length(); i++) {
            script = worklist_[i];
            script->ensureNonLazyCanonicalFunction(cx_);
            fun = script->functionNonDelazifying();

            
            
            
            
            
            
            if (!script->hasBaselineScript()) {
                uint32_t previousWarmUpCount = worklistData_[i].warmUpCount;
                uint32_t currentWarmUpCount = script->getWarmUpCount();
                if (previousWarmUpCount < currentWarmUpCount) {
                    worklistData_[i].warmUpCount = currentWarmUpCount;
                    worklistData_[i].stallCount = 0;
                    gatheringTypeInformation = true;

                    Spew(SpewCompile,
                         "Script %p:%s:%d has no baseline script, "
                         "but warm-up counter grew from %d to %d",
                         script.get(), script->filename(), script->lineno(),
                         previousWarmUpCount, currentWarmUpCount);
                } else {
                    uint32_t stallCount = ++worklistData_[i].stallCount;
                    if (stallCount < stallThreshold) {
                        gatheringTypeInformation = true;
                    }

                    Spew(SpewCompile,
                         "Script %p:%s:%d has no baseline script, "
                         "and warm-up counter has %u stalls at %d",
                         script.get(), script->filename(), script->lineno(),
                         stallCount, previousWarmUpCount);
                }
                continue;
            }

            if (!script->hasParallelIonScript()) {
                
                SpewBeginCompile(script);
                MethodStatus mstatus = CanEnterInParallel(cx_, script);
                SpewEndCompile(mstatus);

                switch (mstatus) {
                  case Method_Error:
                    return fatalError(status);

                  case Method_CantCompile:
                    Spew(SpewCompile,
                         "Script %p:%s:%d cannot be compiled, "
                         "falling back to sequential execution",
                         script.get(), script->filename(), script->lineno());
                    return sequentialExecution(true, status);

                  case Method_Skipped:
                    
                    
                    if (script->isParallelIonCompilingOffThread()) {
                        Spew(SpewCompile,
                             "Script %p:%s:%d compiling off-thread",
                             script.get(), script->filename(), script->lineno());
                        offMainThreadCompilationsInProgress = true;
                        continue;
                    }
                    return sequentialExecution(false, status);

                  case Method_Compiled:
                    Spew(SpewCompile,
                         "Script %p:%s:%d compiled",
                         script.get(), script->filename(), script->lineno());
                    JS_ASSERT(script->hasParallelIonScript());

                    if (isInitialScript(script)) {
                        JitCompartment *jitComp = cx_->compartment()->jitCompartment();
                        if (!jitComp->notifyOfActiveParallelEntryScript(cx_, script)) {
                            *status = ExecutionFatal;
                            return RedLight;
                        }
                    }

                    break;
                }
            }

            
            
            
            
            
            JS_ASSERT(script->hasParallelIonScript());
            if (appendCallTargetsToWorklist(i, status) == RedLight)
                return RedLight;
        }

        
        
        
        
        
        
        
        
        
        
        if (offMainThreadCompilationsInProgress || gatheringTypeInformation) {
            bool stopIfComplete = (mode_ != ForkJoinModeCompile);
            if (warmupExecution(stopIfComplete, status) == RedLight)
                return RedLight;
            continue;
        }

        
        
        
        
        
        bool allScriptsPresent = true;
        for (uint32_t i = 0; i < worklist_.length(); i++) {
            if (!worklist_[i]->hasParallelIonScript()) {
                if (worklistData_[i].stallCount < stallThreshold) {
                    worklistData_[i].reset();
                    allScriptsPresent = false;

                    Spew(SpewCompile,
                         "Script %p:%s:%d is not stalled, "
                         "but no parallel ion script found, "
                         "restarting loop",
                         script.get(), script->filename(), script->lineno());
                }
            }
        }

        if (allScriptsPresent)
            break;
    }

    Spew(SpewCompile, "Compilation complete (final worklist length %d)",
         worklist_.length());

    
    
    
    
    
    for (uint32_t i = 0; i < worklist_.length(); i++) {
        if (worklist_[i]->hasParallelIonScript()) {
            JS_ASSERT(worklistData_[i].calleesEnqueued);
            worklist_[i]->parallelIonScript()->clearHasUncompiledCallTarget();
        } else {
            JS_ASSERT(worklistData_[i].stallCount >= stallThreshold);
        }
    }
    worklist_.clear();
    worklistData_.clear();
    return GreenLight;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::appendCallTargetsToWorklist(uint32_t index, ExecutionStatus *status)
{
    
    

    JS_ASSERT(worklist_[index]->hasParallelIonScript());

    
    
    if (worklistData_[index].calleesEnqueued)
        return GreenLight;
    worklistData_[index].calleesEnqueued = true;

    
    RootedScript target(cx_);
    IonScript *ion = worklist_[index]->parallelIonScript();
    for (uint32_t i = 0; i < ion->callTargetEntries(); i++) {
        target = ion->callTargetList()[i];
        parallel::Spew(parallel::SpewCompile,
                       "Adding call target %s:%u",
                       target->filename(), target->lineno());
        if (appendCallTargetToWorklist(target, status) == RedLight)
            return RedLight;
    }

    return GreenLight;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::appendCallTargetToWorklist(HandleScript script, ExecutionStatus *status)
{
    
    

    JS_ASSERT(script);

    
    if (!script->canParallelIonCompile()) {
        Spew(SpewCompile, "Skipping %p:%s:%u, canParallelIonCompile() is false",
             script.get(), script->filename(), script->lineno());
        return sequentialExecution(true, status);
    }

    if (script->hasParallelIonScript()) {
        
        if (script->parallelIonScript()->bailoutExpected()) {
            Spew(SpewCompile, "Skipping %p:%s:%u, bailout expected",
                 script.get(), script->filename(), script->lineno());
            return sequentialExecution(false, status);
        }
    }

    if (!addToWorklist(script))
        return fatalError(status);

    return GreenLight;
}

bool
ForkJoinOperation::addToWorklist(HandleScript script)
{
    for (uint32_t i = 0; i < worklist_.length(); i++) {
        if (worklist_[i] == script) {
            Spew(SpewCompile, "Skipping %p:%s:%u, already in worklist",
                 script.get(), script->filename(), script->lineno());
            return true;
        }
    }

    Spew(SpewCompile, "Enqueued %p:%s:%u",
         script.get(), script->filename(), script->lineno());

    
    
    
    
    if (!worklist_.append(script))
        return false;

    
    if (!worklistData_.append(WorklistData()))
        return false;
    worklistData_[worklistData_.length() - 1].reset();

    return true;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::sequentialExecution(bool disqualified, ExecutionStatus *status)
{
    

    *status = sequentialExecution(disqualified);
    return RedLight;
}

ExecutionStatus
ForkJoinOperation::sequentialExecution(bool disqualified)
{
    

    Spew(SpewOps, "Executing sequential execution (disqualified=%d).",
         disqualified);

    if (sliceStart_ == sliceEnd_)
        return ExecutionSequential;

    RootedValue funVal(cx_, ObjectValue(*fun_));
    if (!ExecuteSequentially(cx_, funVal, &sliceStart_, sliceEnd_))
        return ExecutionFatal;
    MOZ_ASSERT(sliceStart_ == sliceEnd_);
    return ExecutionSequential;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::fatalError(ExecutionStatus *status)
{
    

    *status = ExecutionFatal;
    return RedLight;
}

static const char *
BailoutExplanation(ParallelBailoutCause cause)
{
    switch (cause) {
      case ParallelBailoutNone:
        return "no bailout";
      case ParallelBailoutInterrupt:
        return "interrupted";
      case ParallelBailoutExecution:
        return "";
      case ParallelBailoutCompilationSkipped:
        return "compilation failed (method skipped)";
      case ParallelBailoutCompilationFailure:
        return "compilation failed";
      case ParallelBailoutMainScriptNotPresent:
        return "main script JIT code was collected";
      case ParallelBailoutOverRecursed:
        return "stack limit exceeded";
      case ParallelBailoutOutOfMemory:
        return "out of memory";
      case ParallelBailoutRequestedGC:
        return "requested GC of common heap";
      case ParallelBailoutRequestedZoneGC:
        return "requested zone GC of common heap";
      default:
        MOZ_CRASH("Invalid ParallelBailoutCause");
    }
}

static const char *
IonBailoutKindExplanation(ParallelBailoutCause cause, BailoutKind kind)
{
    if (cause != ParallelBailoutExecution)
        return "";

    switch (kind) {
      
      case Bailout_Inevitable:
        return "inevitable";
      case Bailout_DuringVMCall:
        return "on vm reentry";
      case Bailout_NonJSFunctionCallee:
        return "non-scripted callee";
      case Bailout_DynamicNameNotFound:
        return "dynamic name not found";
      case Bailout_StringArgumentsEval:
        return "string contains 'arguments' or 'eval'";
      case Bailout_Overflow:
      case Bailout_OverflowInvalidate:
        return "integer overflow";
      case Bailout_Round:
        return "unhandled input to rounding function";
      case Bailout_NonPrimitiveInput:
        return "trying to convert non-primitive input to number or string";
      case Bailout_PrecisionLoss:
        return "precision loss when converting to int32";
      case Bailout_TypeBarrierO:
        return "tripped type barrier: unexpected object";
      case Bailout_TypeBarrierV:
        return "tripped type barrier: unexpected value";
      case Bailout_MonitorTypes:
        return "wrote value of unexpected type to property";
      case Bailout_Hole:
        return "saw unexpected array hole";
      case Bailout_NegativeIndex:
        return "negative array index";
      case Bailout_ObjectIdentityOrTypeGuard:
        return "saw unexpected object type barrier";
      case Bailout_NonInt32Input:
        return "can't unbox: expected int32";
      case Bailout_NonNumericInput:
        return "can't unbox: expected number";
      case Bailout_NonBooleanInput:
        return "can't unbox: expected boolean";
      case Bailout_NonObjectInput:
        return "can't unbox: expected object";
      case Bailout_NonStringInput:
      case Bailout_NonStringInputInvalidate:
        return "can't unbox: expected string";
      case Bailout_NonSymbolInput:
        return "can't unbox: expected symbol";
      case Bailout_GuardThreadExclusive:
        return "tried to write to non-thread local value";
      case Bailout_ParallelUnsafe:
        return "unsafe";
      case Bailout_InitialState:
        return "during function prologue";
      case Bailout_DoubleOutput:
        return "integer arithmetic overflowed to double";
      case Bailout_ArgumentCheck:
        return "unexpected argument type";
      case Bailout_BoundsCheck:
        return "out of bounds element access";
      case Bailout_Neutered:
        return "neutered typed object access";
      case Bailout_ShapeGuard:
        return "saw unexpected shape";
      case Bailout_IonExceptionDebugMode:
        
      default:
        MOZ_CRASH("Invalid BailoutKind");
    }
}

bool
ForkJoinOperation::isInitialScript(HandleScript script)
{
    return fun_->is<JSFunction>() && (fun_->as<JSFunction>().nonLazyScript() == script);
}

static const char *
ValueToChar(JSContext *cx, HandleValue val, JSAutoByteString &bytes)
{
    if (val.isMagic()) {
        switch (val.whyMagic()) {
          case JS_OPTIMIZED_OUT: return "<optimized out>";
          default: return "<unknown magic?>";
        }
    }

    RootedString str(cx, ToString<CanGC>(cx, val));
    if (!str)
        return nullptr;
    return bytes.encodeUtf8(cx, str);
}

bool
ForkJoinOperation::reportBailoutWarnings()
{
    Sprinter sp(cx_);
    if (SpewEnabled(SpewBailouts)) {
        sp.init();
        sp.printf("Bailed out of parallel operation");
    }

    for (uint32_t threadId = 0; threadId < bailoutRecords_.length(); threadId++) {
        ParallelBailoutRecord &record = bailoutRecords_[threadId];
        ParallelBailoutCause cause = record.cause;
        BailoutKind ionBailoutKind = record.ionBailoutKind;
        if (cause == ParallelBailoutNone)
            continue;

        if (record.hasFrames()) {
            Vector<RematerializedFrame *> &frames = record.frames();

            if (!SpewEnabled(SpewBailouts)) {
                RematerializedFrame *frame = frames[0];
                RootedScript bailoutScript(cx_, frame->script());
                SpewBailout(bailouts, bailoutScript, frame->pc(), cause);
                JS_ReportWarning(cx_, "Bailed out of parallel operation: %s%s at %s:%u",
                                 BailoutExplanation(cause),
                                 IonBailoutKindExplanation(cause, ionBailoutKind),
                                 bailoutScript->filename(),
                                 PCToLineNumber(bailoutScript, frame->pc()));
                return true;
            }

            sp.printf("\n  in thread %u: %s%s",
                      threadId, BailoutExplanation(cause),
                      IonBailoutKindExplanation(cause, ionBailoutKind));

            for (uint32_t frameIndex = 0; frameIndex < frames.length(); frameIndex++) {
                RematerializedFrame *frame = frames[frameIndex];
                RootedScript script(cx_, frame->script());

                
                sp.printf("\n    at ");
                if (JSFunction *fun = frame->maybeFun()) {
                    if (fun->displayAtom()) {
                        JSAutoByteString displayBytes;
                        RootedString displayAtom(cx_, fun->displayAtom());
                        const char *displayChars = displayBytes.encodeUtf8(cx_, displayAtom);
                        if (!displayChars)
                            return false;
                        sp.printf("%s", displayChars);
                    } else {
                        sp.printf("<anonymous>");
                    }
                } else {
                    sp.printf("<global>");
                }
                sp.printf(" (%s:%u)", script->filename(), PCToLineNumber(script, frame->pc()));

                
                BindingVector bindings(cx_);
                if (!FillBindingVector(script, &bindings))
                    return false;

                unsigned scopeSlot = 0;
                for (unsigned i = 0; i < bindings.length(); i++) {
                    JSAutoByteString nameBytes;
                    const char *nameChars = nullptr;
                    RootedPropertyName bindingName(cx_, bindings[i].name());
                    nameChars = nameBytes.encodeUtf8(cx_, bindingName);
                    if (!nameChars)
                        return false;

                    RootedValue arg(cx_);
                    if (bindings[i].aliased()) {
                        arg = frame->callObj().getSlot(scopeSlot);
                        scopeSlot++;
                    } else if (i < frame->numFormalArgs()) {
                        if (script->argsObjAliasesFormals() && frame->hasArgsObj())
                            arg = frame->argsObj().arg(i);
                        else
                            arg = frame->unaliasedActual(i, DONT_CHECK_ALIASING);
                    } else {
                        arg = frame->unaliasedLocal(i - frame->numFormalArgs(), DONT_CHECK_ALIASING);
                    }

                    JSAutoByteString valueBytes;
                    const char *valueChars = ValueToChar(cx_, arg, valueBytes);
                    if (!valueChars)
                        return false;

                    sp.printf("\n      %s %s = %s",
                              bindings[i].kind() == Binding::ARGUMENT ? "arg" : "var",
                              nameChars, valueChars);
                }
            }
        }
    }

    if (SpewEnabled(SpewBailouts))
        JS_ReportWarning(cx_, "%s", sp.string());

    return true;
}

bool
ForkJoinOperation::invalidateBailedOutScripts()
{
    Vector<types::RecompileInfo> invalid(cx_);
    for (uint32_t i = 0; i < bailoutRecords_.length(); i++) {
        switch (bailoutRecords_[i].cause) {
          
          case ParallelBailoutNone:
            continue;

          
          
          case ParallelBailoutInterrupt:
            continue;

          
          default:
            break;
        }

        if (!bailoutRecords_[i].hasFrames())
            continue;

        
        RootedScript script(cx_, bailoutRecords_[i].frames()[0]->script());

        
        if (!script->hasParallelIonScript() || hasScript(invalid, script))
            continue;

        Spew(SpewBailouts, "Invalidating script %p:%s:%d due to cause %d",
             script.get(), script->filename(), script->lineno(),
             bailoutRecords_[i].cause);

        types::RecompileInfo co = script->parallelIonScript()->recompileInfo();

        if (!invalid.append(co))
            return false;

        
        
        if (!addToWorklist(script))
            return false;
    }

    Invalidate(cx_, invalid);

    return true;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::warmupExecution(bool stopIfComplete, ExecutionStatus *status)
{
    
    

    if (sliceStart_ == sliceEnd_) {
        Spew(SpewOps, "Warmup execution finished all the work.");

        if (stopIfComplete) {
            *status = ExecutionWarmup;
            return RedLight;
        }

        
        
        
        
        if (!CheckForInterrupt(cx_)) {
            *status = ExecutionFatal;
            return RedLight;
        }

        return GreenLight;
    }

    Spew(SpewOps, "Executing warmup from slice %d.", sliceStart_);

    AutoEnterWarmup warmup(cx_->runtime());
    RootedValue funVal(cx_, ObjectValue(*fun_));
    if (!ExecuteSequentially(cx_, funVal, &sliceStart_, sliceStart_ + 1)) {
        *status = ExecutionFatal;
        return RedLight;
    }

    return GreenLight;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::parallelExecution(ExecutionStatus *status)
{
    
    

    
    
    
    JS_ASSERT(ForkJoinContext::current() == nullptr);

    if (sliceStart_ == sliceEnd_) {
        Spew(SpewOps, "Warmup execution finished all the work.");
        *status = ExecutionWarmup;
        return RedLight;
    }

    ForkJoinActivation activation(cx_);
    ThreadPool *threadPool = &cx_->runtime()->threadPool;
    ForkJoinShared shared(cx_, threadPool, fun_, updatable_, sliceStart_, sliceEnd_,
                          bailoutRecords_);
    if (!shared.init()) {
        *status = ExecutionFatal;
        return RedLight;
    }

    switch (shared.execute()) {
      case TP_SUCCESS:
        *status = ExecutionParallel;
        return RedLight;

      case TP_FATAL:
        *status = ExecutionFatal;
        return RedLight;

      case TP_RETRY_SEQUENTIALLY:
      case TP_RETRY_AFTER_GC:
        break; 
    }

    return GreenLight;
}

ForkJoinOperation::TrafficLight
ForkJoinOperation::recoverFromBailout(ExecutionStatus *status)
{
    
    

    bailouts += 1;

    if (!reportBailoutWarnings())
        return fatalError(status);

    
    
    RootedScript mainScript(cx_, fun_->nonLazyScript());
    if (!addToWorklist(mainScript))
        return fatalError(status);

    
    
    if (!invalidateBailedOutScripts())
        return fatalError(status);

    if (warmupExecution(true, status) == RedLight)
        return RedLight;

    return GreenLight;
}

bool
ForkJoinOperation::hasScript(Vector<types::RecompileInfo> &scripts, JSScript *script)
{
    for (uint32_t i = 0; i < scripts.length(); i++) {
        if (scripts[i] == script->parallelIonScript()->recompileInfo())
            return true;
    }
    return false;
}


template <uint32_t maxArgc>
class ParallelIonInvoke
{
    EnterJitCode enter_;
    void *jitcode_;
    void *calleeToken_;
    Value argv_[maxArgc + 2];
    uint32_t argc_;

  public:
    Value *args;

    ParallelIonInvoke(JSRuntime *rt,
                      HandleFunction callee,
                      uint32_t argc)
      : argc_(argc),
        args(argv_ + 2)
    {
        JS_ASSERT(argc <= maxArgc + 2);

        
        argv_[0] = ObjectValue(*callee);
        argv_[1] = UndefinedValue();

        
        IonScript *ion = callee->nonLazyScript()->parallelIonScript();
        JitCode *code = ion->method();
        jitcode_ = code->raw();
        enter_ = rt->jitRuntime()->enterIon();
        calleeToken_ = CalleeToToken(callee,  false);
    }

    bool invoke(ForkJoinContext *cx) {
        JitActivation activation(cx);
        Value result = Int32Value(argc_);
        CALL_GENERATED_CODE(enter_, jitcode_, argc_ + 1, argv_ + 1, nullptr, calleeToken_,
                            nullptr, 0, &result);
        return !result.isMagic();
    }
};





ForkJoinShared::ForkJoinShared(JSContext *cx,
                               ThreadPool *threadPool,
                               HandleFunction fun,
                               HandleObject updatable,
                               uint16_t sliceStart,
                               uint16_t sliceEnd,
                               Vector<ParallelBailoutRecord> &records)
  : cx_(cx),
    threadPool_(threadPool),
    fun_(fun),
    updatable_(updatable),
    sliceStart_(sliceStart),
    sliceEnd_(sliceEnd),
    cxLock_(nullptr),
    records_(records),
    allocators_(cx),
    gcRequested_(false),
    gcReason_(JS::gcreason::NUM_REASONS),
    gcZone_(nullptr),
    abort_(false),
    fatal_(false)
{
}

bool
ForkJoinShared::init()
{
    
    
    
    
    
    
    
    
    

    if (!Monitor::init())
        return false;

    cxLock_ = PR_NewLock();
    if (!cxLock_)
        return false;

    for (unsigned i = 0; i < threadPool_->numWorkers(); i++) {
        Allocator *allocator = cx_->new_<Allocator>(cx_->zone());
        if (!allocator)
            return false;

        if (!allocators_.append(allocator)) {
            js_delete(allocator);
            return false;
        }
    }

    return true;
}

ForkJoinShared::~ForkJoinShared()
{
    if (cxLock_)
        PR_DestroyLock(cxLock_);

    while (allocators_.length() > 0)
        js_delete(allocators_.popCopy());
}

ParallelResult
ForkJoinShared::execute()
{
    
    
    
    if (cx_->runtime()->interruptPar)
        return TP_RETRY_SEQUENTIALLY;

    AutoLockMonitor lock(*this);

    ParallelResult jobResult = TP_SUCCESS;
    {
        AutoUnlockMonitor unlock(*this);

        
        jobResult = threadPool_->executeJob(cx_, this, sliceStart_, sliceEnd_);
    }

    
    
    transferArenasToCompartmentAndProcessGCRequests();

    if (jobResult == TP_FATAL)
        return TP_FATAL;

    
    if (abort_) {
        if (fatal_)
            return TP_FATAL;
        return TP_RETRY_SEQUENTIALLY;
    }

#ifdef FORKJOIN_SPEW
    Spew(SpewOps, "Completed parallel job [slices: %d, threads: %d, stolen: %d (work stealing:%s)]",
         sliceEnd_ - sliceStart_,
         threadPool_->numWorkers(),
#ifdef DEBUG
         threadPool_->stolenSlices(),
#else
         0,
#endif
         threadPool_->workStealing() ? "ON" : "OFF");
#endif

    
    return jobResult;
}

void
ForkJoinShared::transferArenasToCompartmentAndProcessGCRequests()
{
    JSCompartment *comp = cx_->compartment();
    for (unsigned i = 0; i < threadPool_->numWorkers(); i++)
        comp->adoptWorkerAllocator(allocators_[i]);

    if (gcRequested_) {
        Spew(SpewGC, "Triggering garbage collection in SpiderMonkey heap");
        gc::GCRuntime &gc = cx_->runtime()->gc;
        if (!gcZone_)
            gc.triggerGC(gcReason_);
        else
            gc.triggerZoneGC(gcZone_, gcReason_);
        gcRequested_ = false;
        gcZone_ = nullptr;
    }
}

bool
ForkJoinShared::executeFromWorker(ThreadPoolWorker *worker, uintptr_t stackLimit)
{
    PerThreadData thisThread(cx_->runtime());
    if (!thisThread.init()) {
        setAbortFlagAndRequestInterrupt(true);
        return false;
    }
    TlsPerThreadData.set(&thisThread);

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    stackLimit = Simulator::StackLimit();
#endif

    
    
    thisThread.jitStackLimit = stackLimit;
    executePortion(&thisThread, worker);
    TlsPerThreadData.set(nullptr);

    return !abort_;
}

bool
ForkJoinShared::executeFromMainThread(ThreadPoolWorker *worker)
{
    
    
    PerThreadData *oldData = TlsPerThreadData.get();
    PerThreadData thisThread(cx_->runtime());
    if (!thisThread.init()) {
        setAbortFlagAndRequestInterrupt(true);
        return false;
    }
    TlsPerThreadData.set(&thisThread);

    
    
    
    
    
    
    
    
    
    
    
    
    
    thisThread.jitStackLimit = GetNativeStackLimit(cx_);
    executePortion(&thisThread, worker);
    TlsPerThreadData.set(oldData);

    return !abort_;
}

void
ForkJoinShared::executePortion(PerThreadData *perThread, ThreadPoolWorker *worker)
{
    
    

    Allocator *allocator = allocators_[worker->id()];
    ForkJoinContext cx(perThread, worker, allocator, this, &records_[worker->id()]);
    if (!cx.initialize()) {
        setAbortFlagAndRequestInterrupt(true);
        return;
    }
    AutoSetForkJoinContext autoContext(&cx);

    
    
    
    JS::AutoSuppressGCAnalysis nogc;

#ifdef FORKJOIN_SPEW
    
    cx.maxWorkerId = threadPool_->numWorkers();
#endif

    Spew(SpewOps, "Up");

    
    
    IonContext icx(CompileRuntime::get(cx_->runtime()),
                   CompileCompartment::get(cx_->compartment()),
                   nullptr);

    JS_ASSERT(!cx.bailoutRecord->bailedOut());

    if (!fun_->nonLazyScript()->hasParallelIonScript()) {
        
        
        
        
        Spew(SpewOps, "Down (Script no longer present)");
        cx.bailoutRecord->joinCause(ParallelBailoutMainScriptNotPresent);
        setAbortFlagAndRequestInterrupt(false);
    } else {
        ParallelIonInvoke<3> fii(runtime(), fun_, 3);

        fii.args[0] = Int32Value(worker->id());
        fii.args[1] = Int32Value(sliceStart_);
        fii.args[2] = Int32Value(sliceEnd_);

        bool ok = fii.invoke(&cx);
        JS_ASSERT(ok == !cx.bailoutRecord->bailedOut());
        if (!ok) {
            setAbortFlagAndRequestInterrupt(false);
#ifdef JSGC_FJGENERATIONAL
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            cx.evacuateLiveData();
#endif
        } else {
#ifdef JSGC_FJGENERATIONAL
            cx.evacuateLiveData();
#endif
        }
    }

    Spew(SpewOps, "Down");
}

void
ForkJoinShared::setAbortFlagDueToInterrupt(ForkJoinContext &cx)
{
    JS_ASSERT(cx_->runtime()->interruptPar);
    
    
    
    JS_ASSERT(!cx_->runtime()->gc.isGcNeeded());

    if (!abort_) {
        cx.bailoutRecord->joinCause(ParallelBailoutInterrupt);
        setAbortFlagAndRequestInterrupt(false);
    }
}

void
ForkJoinShared::setAbortFlagAndRequestInterrupt(bool fatal)
{
    AutoLockMonitor lock(*this);

    abort_ = true;
    fatal_ = fatal_ || fatal;

    
    
    cx_->runtime()->requestInterrupt(JSRuntime::RequestInterruptAnyThreadForkJoin);
}

void
ForkJoinShared::requestGC(JS::gcreason::Reason reason)
{
    AutoLockMonitor lock(*this);

    gcZone_ = nullptr;
    gcReason_ = reason;
    gcRequested_ = true;
}

void
ForkJoinShared::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    AutoLockMonitor lock(*this);

    if (gcRequested_ && gcZone_ != zone) {
        
        
        gcZone_ = nullptr;
        gcReason_ = reason;
        gcRequested_ = true;
    } else {
        
        gcZone_ = zone;
        gcReason_ = reason;
        gcRequested_ = true;
    }
}

#ifdef JSGC_FJGENERATIONAL

JSRuntime*
js::gc::ForkJoinGCShared::runtime()
{
    return shared_->runtime();
}

JS::Zone*
js::gc::ForkJoinGCShared::zone()
{
    return shared_->zone();
}

JSObject*
js::gc::ForkJoinGCShared::updatable()
{
    return shared_->updatable();
}

js::gc::ForkJoinNurseryChunk *
js::gc::ForkJoinGCShared::allocateNurseryChunk()
{
    return shared_->threadPool_->getChunk();
}

void
js::gc::ForkJoinGCShared::freeNurseryChunk(js::gc::ForkJoinNurseryChunk *p)
{
    shared_->threadPool_->putFreeChunk(p);
}

void
js::gc::ForkJoinGCShared::spewGC(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SpewVA(SpewGC, fmt, ap);
    va_end(ap);
}

#endif 





ForkJoinContext::ForkJoinContext(PerThreadData *perThreadData, ThreadPoolWorker *worker,
                                 Allocator *allocator, ForkJoinShared *shared,
                                 ParallelBailoutRecord *bailoutRecord)
  : ThreadSafeContext(shared->runtime(), perThreadData, Context_ForkJoin),
    bailoutRecord(bailoutRecord),
    targetRegionStart(nullptr),
    targetRegionEnd(nullptr),
    shared_(shared),
#ifdef JSGC_FJGENERATIONAL
    gcShared_(shared),
    nursery_(const_cast<ForkJoinContext*>(this), &this->gcShared_, allocator),
#endif
    worker_(worker),
    acquiredJSContext_(false),
    nogc_()
{
    



    zone_ = shared->zone();

    



    compartment_ = shared->compartment();

    allocator_ = allocator;
}

bool ForkJoinContext::initialize()
{
#ifdef JSGC_FJGENERATIONAL
    if (!nursery_.initialize())
        return false;
#endif
    return true;
}

bool
ForkJoinContext::isMainThread() const
{
    return worker_->isMainThread();
}

JSRuntime *
ForkJoinContext::runtime()
{
    return shared_->runtime();
}

JSContext *
ForkJoinContext::acquireJSContext()
{
    JSContext *cx = shared_->acquireJSContext();
    JS_ASSERT(!acquiredJSContext_);
    acquiredJSContext_ = true;
    return cx;
}

void
ForkJoinContext::releaseJSContext()
{
    JS_ASSERT(acquiredJSContext_);
    acquiredJSContext_ = false;
    return shared_->releaseJSContext();
}

bool
ForkJoinContext::hasAcquiredJSContext() const
{
    return acquiredJSContext_;
}

bool
ForkJoinContext::check()
{
    if (runtime()->interruptPar) {
        shared_->setAbortFlagDueToInterrupt(*this);
        return false;
    }
    return true;
}

void
ForkJoinContext::requestGC(JS::gcreason::Reason reason)
{
    shared_->requestGC(reason);
    bailoutRecord->joinCause(ParallelBailoutRequestedGC);
    shared_->setAbortFlagAndRequestInterrupt(false);
}

void
ForkJoinContext::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    shared_->requestZoneGC(zone, reason);
    bailoutRecord->joinCause(ParallelBailoutRequestedZoneGC);
    shared_->setAbortFlagAndRequestInterrupt(false);
}

bool
ForkJoinContext::setPendingAbortFatal(ParallelBailoutCause cause)
{
    shared_->setPendingAbortFatal();
    bailoutRecord->joinCause(cause);
    return false;
}




ParallelBailoutRecord::~ParallelBailoutRecord()
{
    reset();
    js_delete(frames_);
}

bool
ParallelBailoutRecord::init(JSContext *cx)
{
    MOZ_ASSERT(!frames_);
    frames_ = cx->new_<Vector<RematerializedFrame *> >(cx);
    return !!frames_;
}

void
ParallelBailoutRecord::reset()
{
    RematerializedFrame::FreeInVector(frames());
    cause = ParallelBailoutNone;
}

template <class T>
static void
RematerializeFramesWithIter(ForkJoinContext *cx, T &frameIter,
                            Vector<RematerializedFrame *> &frames)
{
    
    
    
    

    MOZ_ASSERT(frames.empty());

    for (; !frameIter.done(); ++frameIter) {
        if (!frameIter.isIonJS())
            continue;

        InlineFrameIterator inlineIter(cx, &frameIter);
        Vector<RematerializedFrame *> inlineFrames(cx);

        if (!RematerializedFrame::RematerializeInlineFrames(cx, frameIter.fp(),
                                                            inlineIter, inlineFrames))
        {
            RematerializedFrame::FreeInVector(inlineFrames);
            RematerializedFrame::FreeInVector(frames);
            return;
        }

        
        while (!inlineFrames.empty()) {
            if (!frames.append(inlineFrames.popCopy())) {
                RematerializedFrame::FreeInVector(inlineFrames);
                RematerializedFrame::FreeInVector(frames);
                return;
            }
        }
    }
}

void
ParallelBailoutRecord::rematerializeFrames(ForkJoinContext *cx, JitFrameIterator &frameIter)
{
    RematerializeFramesWithIter(cx, frameIter, frames());
}

void
ParallelBailoutRecord::rematerializeFrames(ForkJoinContext *cx, IonBailoutIterator &frameIter)
{
    RematerializeFramesWithIter(cx, frameIter, frames());
}







#ifdef FORKJOIN_SPEW

static const char *
ExecutionStatusToString(ExecutionStatus status)
{
    switch (status) {
      case ExecutionFatal:
        return "fatal";
      case ExecutionSequential:
        return "sequential";
      case ExecutionWarmup:
        return "warmup";
      case ExecutionParallel:
        return "parallel";
    }
    return "(unknown status)";
}

static const char *
MethodStatusToString(MethodStatus status)
{
    switch (status) {
      case Method_Error:
        return "error";
      case Method_CantCompile:
        return "can't compile";
      case Method_Skipped:
        return "skipped";
      case Method_Compiled:
        return "compiled";
    }
    return "(unknown status)";
}

static unsigned
NumberOfDigits(unsigned n)
{
    if (n == 0)
        return 1;
    unsigned d = 0;
    while (n != 0) {
        d++;
        n /= 10;
    }
    return d;
}

static const size_t BufferSize = 4096;

class ParallelSpewer
{
    uint32_t depth;
    bool colorable;
    bool active[NumSpewChannels];

    const char *color(const char *colorCode) {
        if (!colorable)
            return "";
        return colorCode;
    }

    const char *reset() { return color("\x1b[0m"); }
    const char *bold() { return color("\x1b[1m"); }
    const char *red() { return color("\x1b[31m"); }
    const char *green() { return color("\x1b[32m"); }
    const char *yellow() { return color("\x1b[33m"); }
    const char *cyan() { return color("\x1b[36m"); }
    const char *workerColor(uint32_t id) {
        static const char *colors[] = {
            "\x1b[7m\x1b[31m", "\x1b[7m\x1b[32m", "\x1b[7m\x1b[33m",
            "\x1b[7m\x1b[34m", "\x1b[7m\x1b[35m", "\x1b[7m\x1b[36m",
            "\x1b[7m\x1b[37m",
            "\x1b[31m", "\x1b[32m", "\x1b[33m",
            "\x1b[34m", "\x1b[35m", "\x1b[36m",
            "\x1b[37m"
        };
        return color(colors[id % 14]);
    }

  public:
    ParallelSpewer()
      : depth(0)
    {
        const char *env;

        mozilla::PodArrayZero(active);
        env = getenv("PAFLAGS");
        if (env) {
            if (strstr(env, "ops"))
                active[SpewOps] = true;
            if (strstr(env, "compile"))
                active[SpewCompile] = true;
            if (strstr(env, "bailouts"))
                active[SpewBailouts] = true;
            if (strstr(env, "gc"))
                active[SpewGC] = true;
            if (strstr(env, "full")) {
                for (uint32_t i = 0; i < NumSpewChannels; i++)
                    active[i] = true;
            }
        }

        env = getenv("TERM");
        if (env && isatty(fileno(stderr))) {
            if (strcmp(env, "xterm-color") == 0 || strcmp(env, "xterm-256color") == 0)
                colorable = true;
        }
    }

    bool isActive(js::parallel::SpewChannel channel) {
        return active[channel];
    }

    void spewVA(js::parallel::SpewChannel channel, const char *fmt, va_list ap) {
        if (!active[channel])
            return;

        
        
        char buf[BufferSize];

        if (ForkJoinContext *cx = ForkJoinContext::current()) {
            
            
            char bufbuf[BufferSize];
            JS_snprintf(bufbuf, BufferSize, "[%%sParallel:%%0%du%%s] ",
                        NumberOfDigits(cx->maxWorkerId));
            JS_snprintf(buf, BufferSize, bufbuf, workerColor(cx->workerId()),
                        cx->workerId(), reset());
        } else {
            JS_snprintf(buf, BufferSize, "[Parallel:M] ");
        }

        for (uint32_t i = 0; i < depth; i++)
            JS_snprintf(buf + strlen(buf), BufferSize, "  ");

        JS_vsnprintf(buf + strlen(buf), BufferSize, fmt, ap);
        JS_snprintf(buf + strlen(buf), BufferSize, "\n");

        fprintf(stderr, "%s", buf);
    }

    void spew(js::parallel::SpewChannel channel, const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        spewVA(channel, fmt, ap);
        va_end(ap);
    }

    void beginOp(JSContext *cx, const char *name) {
        if (!active[SpewOps])
            return;

        if (cx) {
            jsbytecode *pc;
            RootedScript script(cx, cx->currentScript(&pc));
            if (script && pc) {
                NonBuiltinScriptFrameIter iter(cx);
                if (iter.done()) {
                    spew(SpewOps, "%sBEGIN %s%s (%s:%u)", bold(), name, reset(),
                         script->filename(), PCToLineNumber(script, pc));
                } else {
                    spew(SpewOps, "%sBEGIN %s%s (%s:%u -> %s:%u)", bold(), name, reset(),
                         iter.script()->filename(), PCToLineNumber(iter.script(), iter.pc()),
                         script->filename(), PCToLineNumber(script, pc));
                }
            } else {
                spew(SpewOps, "%sBEGIN %s%s", bold(), name, reset());
            }
        } else {
            spew(SpewOps, "%sBEGIN %s%s", bold(), name, reset());
        }

        depth++;
    }

    void endOp(ExecutionStatus status) {
        if (!active[SpewOps])
            return;

        JS_ASSERT(depth > 0);
        depth--;

        const char *statusColor;
        switch (status) {
          case ExecutionFatal:
            statusColor = red();
            break;
          case ExecutionSequential:
            statusColor = yellow();
            break;
          case ExecutionParallel:
            statusColor = green();
            break;
          default:
            statusColor = reset();
            break;
        }

        spew(SpewOps, "%sEND %s%s%s", bold(),
             statusColor, ExecutionStatusToString(status), reset());
    }

    void bailout(uint32_t count, HandleScript script,
                 jsbytecode *pc, ParallelBailoutCause cause) {
        if (!active[SpewOps])
            return;

        const char *filename = "";
        unsigned line=0, column=0;
        if (script) {
            line = PCToLineNumber(script, pc, &column);
            filename = script->filename();
        }

        spew(SpewOps, "%s%sBAILOUT %d%s: %s (%d) at %s:%d:%d", bold(), yellow(), count, reset(),
             BailoutExplanation(cause), cause, filename, line, column);
    }

    void beginCompile(HandleScript script) {
        if (!active[SpewCompile])
            return;

        spew(SpewCompile, "COMPILE %p:%s:%u", script.get(), script->filename(), script->lineno());
        depth++;
    }

    void endCompile(MethodStatus status) {
        if (!active[SpewCompile])
            return;

        JS_ASSERT(depth > 0);
        depth--;

        const char *statusColor;
        switch (status) {
          case Method_Error:
          case Method_CantCompile:
            statusColor = red();
            break;
          case Method_Skipped:
            statusColor = yellow();
            break;
          case Method_Compiled:
            statusColor = green();
            break;
          default:
            statusColor = reset();
            break;
        }

        spew(SpewCompile, "END %s%s%s", statusColor, MethodStatusToString(status), reset());
    }

    void spewMIR(MDefinition *mir, const char *fmt, va_list ap) {
        if (!active[SpewCompile])
            return;

        char buf[BufferSize];
        JS_vsnprintf(buf, BufferSize, fmt, ap);

        JSScript *script = mir->block()->info().script();
        spew(SpewCompile, "%s%s%s: %s (%s:%u)", cyan(), mir->opName(), reset(), buf,
             script->filename(), PCToLineNumber(script, mir->trackedPc()));
    }
};


static ParallelSpewer spewer;

bool
parallel::SpewEnabled(SpewChannel channel)
{
    return spewer.isActive(channel);
}

void
parallel::Spew(SpewChannel channel, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    spewer.spewVA(channel, fmt, ap);
    va_end(ap);
}

void
parallel::SpewVA(SpewChannel channel, const char *fmt, va_list ap)
{
    spewer.spewVA(channel, fmt, ap);
}

void
parallel::SpewBeginOp(JSContext *cx, const char *name)
{
    spewer.beginOp(cx, name);
}

ExecutionStatus
parallel::SpewEndOp(ExecutionStatus status)
{
    spewer.endOp(status);
    return status;
}

void
parallel::SpewBailout(uint32_t count, HandleScript script,
                      jsbytecode *pc, ParallelBailoutCause cause)
{
    spewer.bailout(count, script, pc, cause);
}

void
parallel::SpewBeginCompile(HandleScript script)
{
    spewer.beginCompile(script);
}

MethodStatus
parallel::SpewEndCompile(MethodStatus status)
{
    spewer.endCompile(status);
    return status;
}

void
parallel::SpewMIR(MDefinition *mir, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    spewer.spewMIR(mir, fmt, ap);
    va_end(ap);
}

#endif 

bool
js::InExclusiveParallelSection()
{
    return InParallelSection() && ForkJoinContext::current()->hasAcquiredJSContext();
}

bool
js::ParallelTestsShouldPass(JSContext *cx)
{
    return IsIonEnabled(cx) &&
           IsBaselineEnabled(cx) &&
           !js_JitOptions.eagerCompilation &&
           js_JitOptions.baselineWarmUpThreshold != 0 &&
           cx->runtime()->gcZeal() == 0;
}

void
js::RequestInterruptForForkJoin(JSRuntime *rt, JSRuntime::InterruptMode mode)
{
    if (mode != JSRuntime::RequestInterruptAnyThreadDontStopIon)
        rt->interruptPar = true;
}

bool
js::intrinsic_SetForkJoinTargetRegion(JSContext *cx, unsigned argc, Value *vp)
{
    
    
    
    return true;
}

static bool
intrinsic_SetForkJoinTargetRegionPar(ForkJoinContext *cx, unsigned argc, Value *vp)
{
    
    
    
    
    
    
    
    
    
    
    

    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(argc == 3);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    JS_ASSERT(args[1].isInt32());
    JS_ASSERT(args[2].isInt32());

    uint8_t *mem = args[0].toObject().as<TypedObject>().typedMem();
    int32_t start = args[1].toInt32();
    int32_t end = args[2].toInt32();

    cx->targetRegionStart = mem + start;
    cx->targetRegionEnd = mem + end;
    return true;
}

JS_JITINFO_NATIVE_PARALLEL(js::intrinsic_SetForkJoinTargetRegionInfo,
                           intrinsic_SetForkJoinTargetRegionPar);

bool
js::intrinsic_ClearThreadLocalArenas(JSContext *cx, unsigned argc, Value *vp)
{
    return true;
}

static bool
intrinsic_ClearThreadLocalArenasPar(ForkJoinContext *cx, unsigned argc, Value *vp)
{
    cx->allocator()->arenas.wipeDuringParallelExecution(cx->runtime());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL(js::intrinsic_ClearThreadLocalArenasInfo,
                           intrinsic_ClearThreadLocalArenasPar);
