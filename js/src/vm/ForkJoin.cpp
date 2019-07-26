





#if defined(XP_OS2) || defined(XP_WIN)
# include <io.h>     
#else
# include <unistd.h> 
#endif

#include "vm/ForkJoin.h"

#include "mozilla/ThreadLocal.h"

#include "jscntxt.h"
#include "jslock.h"
#include "jsprf.h"

#ifdef JS_THREADSAFE
# include "jit/BaselineJIT.h"
# include "vm/Monitor.h"
#endif

#if defined(DEBUG) && defined(JS_THREADSAFE) && defined(JS_ION)
# include "jit/Ion.h"
# include "jit/JitCompartment.h"
# include "jit/MIR.h"
# include "jit/MIRGraph.h"
#endif 

#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::parallel;
using namespace js::jit;

using mozilla::ThreadLocal;








static bool
ExecuteSequentially(JSContext *cx_, HandleValue funVal, bool *complete,
                    uint16_t sliceStart, uint16_t numSlices);

#if !defined(JS_THREADSAFE) || !defined(JS_ION)
bool
js::ForkJoin(JSContext *cx, CallArgs &args)
{
    RootedValue argZero(cx, args[0]);
    bool complete = false; 
    uint32_t numSlices = args[2].toInt32();
    return ExecuteSequentially(cx, argZero, &complete, 0, numSlices);
}

JSContext *
ForkJoinSlice::acquireContext()
{
    return nullptr;
}

void
ForkJoinSlice::releaseContext()
{
}

bool
ForkJoinSlice::isMainThread() const
{
    return true;
}

JSRuntime *
ForkJoinSlice::runtime()
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

bool
ForkJoinSlice::check()
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

void
ForkJoinSlice::requestGC(JS::gcreason::Reason reason)
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

void
ForkJoinSlice::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

bool
ForkJoinSlice::setPendingAbortFatal(ParallelBailoutCause cause)
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
    return false;
}

void
ParallelBailoutRecord::setCause(ParallelBailoutCause cause,
                                JSScript *outermostScript,
                                JSScript *currentScript,
                                jsbytecode *currentPc)
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

void
js::ParallelBailoutRecord::updateCause(ParallelBailoutCause cause,
                                       JSScript *outermostScript,
                                       JSScript *currentScript,
                                       jsbytecode *currentPc)
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

void
ParallelBailoutRecord::addTrace(JSScript *script,
                                jsbytecode *pc)
{
    MOZ_ASSUME_UNREACHABLE("Not THREADSAFE build");
}

bool
js::InExclusiveParallelSection()
{
    return false;
}

bool
js::ParallelTestsShouldPass(JSContext *cx)
{
    return false;
}

#endif 






static bool
ExecuteSequentially(JSContext *cx, HandleValue funVal, bool *complete,
                    uint16_t sliceStart, uint16_t numSlices)
{
    bool allComplete = true;
    for (uint16_t i = sliceStart; i < numSlices; i++) {
        FastInvokeGuard fig(cx, funVal);
        InvokeArgs &args = fig.args();
        if (!args.init(2))
            return false;
        args.setCallee(funVal);
        args.setThis(UndefinedValue());
        args[0].setInt32(i);
        args[1].setBoolean(!!cx->runtime()->parallelWarmup);
        if (!fig.invoke(cx))
            return false;
        allComplete = allComplete & args.rval().toBoolean();
    }
    *complete = allComplete;
    return true;
}

ThreadLocal<ForkJoinSlice*> ForkJoinSlice::tlsForkJoinSlice;

 bool
ForkJoinSlice::initialize()
{
    if (!tlsForkJoinSlice.initialized()) {
        if (!tlsForkJoinSlice.init())
            return false;
    }
    return true;
}







#if defined(JS_THREADSAFE) && defined(JS_ION)




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

    
    ParallelBailoutCause bailoutCause;
    RootedScript bailoutScript;
    jsbytecode *bailoutBytecode;

    ForkJoinOperation(JSContext *cx, HandleObject fun, ForkJoinMode mode, uint16_t numSlices);
    ExecutionStatus apply();

  private:
    
    
    
    
    
    
    
    
    enum TrafficLight {
        RedLight,
        GreenLight
    };

    struct WorklistData {
        
        
        bool calleesEnqueued;

        
        
        uint32_t useCount;

        
        
        uint32_t stallCount;

        void reset() {
            calleesEnqueued = false;
            useCount = 0;
            stallCount = 0;
        }
    };

    JSContext *cx_;
    HandleObject fun_;
    Vector<ParallelBailoutRecord, 16> bailoutRecords_;
    AutoScriptVector worklist_;
    Vector<WorklistData, 16> worklistData_;
    ForkJoinMode mode_;
    uint16_t warmupSlice_;
    uint16_t numSlices_;

    TrafficLight enqueueInitialScript(ExecutionStatus *status);
    TrafficLight compileForParallelExecution(ExecutionStatus *status);
    TrafficLight warmupExecution(bool stopIfComplete,
                                 ExecutionStatus *status);
    TrafficLight parallelExecution(ExecutionStatus *status);
    TrafficLight sequentialExecution(bool disqualified, ExecutionStatus *status);
    TrafficLight recoverFromBailout(ExecutionStatus *status);
    TrafficLight fatalError(ExecutionStatus *status);
    void determineBailoutCause();
    bool invalidateBailedOutScripts();
    ExecutionStatus sequentialExecution(bool disqualified);

    TrafficLight appendCallTargetsToWorklist(uint32_t index,
                                             ExecutionStatus *status);
    TrafficLight appendCallTargetToWorklist(HandleScript script,
                                            ExecutionStatus *status);
    bool addToWorklist(HandleScript script);
    inline bool hasScript(Vector<types::RecompileInfo> &scripts, JSScript *script);
}; 

class ForkJoinShared : public ParallelJob, public Monitor
{
    
    

    JSContext *const cx_;          
    ThreadPool *const threadPool_; 
    HandleObject fun_;             
    uint16_t numSlices_;           
    PRLock *cxLock_;               
    ParallelBailoutRecord *const records_; 

    
    
    
    

    Vector<Allocator *, 16> allocators_;

    
    
    
    

    bool gcRequested_;              
    JS::gcreason::Reason gcReason_; 
    Zone *gcZone_;                  

    
    
    
    
    

    
    volatile bool abort_;

    
    volatile bool fatal_;

  public:
    ForkJoinShared(JSContext *cx,
                   ThreadPool *threadPool,
                   HandleObject fun,
                   uint16_t numSlices,
                   ParallelBailoutRecord *records);
    ~ForkJoinShared();

    bool init();

    ParallelResult execute();

    
    virtual bool executeFromWorker(uint16_t sliceId, uint32_t workerId,
                                   uintptr_t stackLimit) MOZ_OVERRIDE;

    
    virtual bool executeFromMainThread(uint16_t sliceId) MOZ_OVERRIDE;

    
    void executePortion(PerThreadData *perThread, uint16_t sliceId, uint32_t workerId);

    
    
    
    void transferArenasToCompartmentAndProcessGCRequests();

    
    bool check(ForkJoinSlice &threadCx);

    
    void requestGC(JS::gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason);

    
    void setAbortFlag(bool fatal);

    
    void setPendingAbortFatal() { fatal_ = true; }

    JSRuntime *runtime() { return cx_->runtime(); }
    JS::Zone *zone() { return cx_->zone(); }
    JSCompartment *compartment() { return cx_->compartment(); }

    JSContext *acquireContext() { PR_Lock(cxLock_); return cx_; }
    void releaseContext() { PR_Unlock(cxLock_); }
};

class AutoEnterWarmup
{
    JSRuntime *runtime_;

  public:
    AutoEnterWarmup(JSRuntime *runtime) : runtime_(runtime) { runtime_->parallelWarmup++; }
    ~AutoEnterWarmup() { runtime_->parallelWarmup--; }
};

class AutoSetForkJoinSlice
{
  public:
    AutoSetForkJoinSlice(ForkJoinSlice *threadCx) {
        ForkJoinSlice::tlsForkJoinSlice.set(threadCx);
    }

    ~AutoSetForkJoinSlice() {
        ForkJoinSlice::tlsForkJoinSlice.set(nullptr);
    }
};

} 








ForkJoinActivation::ForkJoinActivation(JSContext *cx)
  : Activation(cx, ForkJoin),
    prevIonTop_(cx->mainThread().ionTop),
    av_(cx->runtime(), false)
{
    
    
    
    
    
    

    if (JS::IsIncrementalGCInProgress(cx->runtime())) {
        JS::PrepareForIncrementalGC(cx->runtime());
        JS::FinishIncrementalGC(cx->runtime(), JS::gcreason::API);
    }

    MinorGC(cx->runtime(), JS::gcreason::API);

    cx->runtime()->gcHelperThread.waitBackgroundSweepEnd();

    JS_ASSERT(!cx->runtime()->needsBarrier());
    JS_ASSERT(!cx->zone()->needsBarrier());
}

ForkJoinActivation::~ForkJoinActivation()
{
    cx_->mainThread().ionTop = prevIonTop_;
}








static const char *ForkJoinModeString(ForkJoinMode mode);

bool
js::ForkJoin(JSContext *cx, CallArgs &args)
{
    JS_ASSERT(args.length() == 3); 
    JS_ASSERT(args[0].isObject());
    JS_ASSERT(args[0].toObject().is<JSFunction>());
    JS_ASSERT(args[1].isInt32());
    JS_ASSERT(args[1].toInt32() < NumForkJoinModes);
    JS_ASSERT(args[2].isInt32());

    RootedObject fun(cx, &args[0].toObject());
    ForkJoinMode mode = (ForkJoinMode) args[1].toInt32();
    uint32_t numSlices = args[2].toInt32();
    MOZ_ASSERT(uint32_t(uint16_t(numSlices)) == numSlices);

    ForkJoinOperation op(cx, fun, mode, numSlices);
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
    } else {
        return true;
    }
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

js::ForkJoinOperation::ForkJoinOperation(JSContext *cx, HandleObject fun, ForkJoinMode mode,
                                         uint16_t numSlices)
  : bailouts(0),
    bailoutCause(ParallelBailoutNone),
    bailoutScript(cx),
    bailoutBytecode(nullptr),
    cx_(cx),
    fun_(fun),
    bailoutRecords_(cx),
    worklist_(cx),
    worklistData_(cx),
    mode_(mode),
    warmupSlice_(0),
    numSlices_(numSlices)
{ }

ExecutionStatus
js::ForkJoinOperation::apply()
{
    ExecutionStatus status;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT_IF(!jit::IsBaselineEnabled(cx_), !jit::IsIonEnabled(cx_));
    if (!jit::IsBaselineEnabled(cx_) || !jit::IsIonEnabled(cx_))
        return sequentialExecution(true);

    SpewBeginOp(cx_, "ForkJoinOperation");

    
    unsigned numWorkersWithMain = cx_->runtime()->threadPool.numWorkers() + 1;

    if (!bailoutRecords_.resize(numWorkersWithMain))
        return SpewEndOp(ExecutionFatal);

    for (uint32_t i = 0; i < numWorkersWithMain; i++)
        bailoutRecords_[i].init(cx_);

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
        MOZ_ASSUME_UNREACHABLE("Invalid mode");
    }

    while (bailouts < MAX_BAILOUTS) {
        for (uint32_t i = 0; i < numWorkersWithMain; i++)
            bailoutRecords_[i].reset(cx_);

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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::enqueueInitialScript(ExecutionStatus *status)
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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::compileForParallelExecution(ExecutionStatus *status)
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
                uint32_t previousUseCount = worklistData_[i].useCount;
                uint32_t currentUseCount = script->getUseCount();
                if (previousUseCount < currentUseCount) {
                    worklistData_[i].useCount = currentUseCount;
                    worklistData_[i].stallCount = 0;
                    gatheringTypeInformation = true;

                    Spew(SpewCompile,
                         "Script %p:%s:%d has no baseline script, "
                         "but use count grew from %d to %d",
                         script.get(), script->filename(), script->lineno(),
                         previousUseCount, currentUseCount);
                } else {
                    uint32_t stallCount = ++worklistData_[i].stallCount;
                    if (stallCount < stallThreshold) {
                        gatheringTypeInformation = true;
                    }

                    Spew(SpewCompile,
                         "Script %p:%s:%d has no baseline script, "
                         "and use count has %u stalls at %d",
                         script.get(), script->filename(), script->lineno(),
                         stallCount, previousUseCount);
                }
                continue;
            }

            if (!script->hasParallelIonScript()) {
                
                SpewBeginCompile(script);
                MethodStatus mstatus = jit::CanEnterInParallel(cx_, script);
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

        if (allScriptsPresent) {
            
            
            
            
            if (mode_ != ForkJoinModeNormal) {
                StopAllOffThreadCompilations(cx_->compartment());
                if (!js_HandleExecutionInterrupt(cx_))
                    return fatalError(status);
            }
            break;
        }
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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::appendCallTargetsToWorklist(uint32_t index, ExecutionStatus *status)
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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::appendCallTargetToWorklist(HandleScript script, ExecutionStatus *status)
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
js::ForkJoinOperation::addToWorklist(HandleScript script)
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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::sequentialExecution(bool disqualified, ExecutionStatus *status)
{
    

    *status = sequentialExecution(disqualified);
    return RedLight;
}

ExecutionStatus
js::ForkJoinOperation::sequentialExecution(bool disqualified)
{
    

    Spew(SpewOps, "Executing sequential execution (disqualified=%d).",
         disqualified);

    bool complete = false;
    RootedValue funVal(cx_, ObjectValue(*fun_));
    if (!ExecuteSequentially(cx_, funVal, &complete, 0, numSlices_))
        return ExecutionFatal;

    
    
    JS_ASSERT(complete);
    return ExecutionSequential;
}

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::fatalError(ExecutionStatus *status)
{
    

    *status = ExecutionFatal;
    return RedLight;
}

static const char *
BailoutExplanation(ParallelBailoutCause cause)
{
    switch (cause) {
      case ParallelBailoutNone:
        return "no particular reason";
      case ParallelBailoutCompilationSkipped:
        return "compilation failed (method skipped)";
      case ParallelBailoutCompilationFailure:
        return "compilation failed";
      case ParallelBailoutInterrupt:
        return "interrupted";
      case ParallelBailoutFailedIC:
        return "failed to attach stub to IC";
      case ParallelBailoutHeapBusy:
        return "heap busy flag set during interrupt";
      case ParallelBailoutMainScriptNotPresent:
        return "main script not present";
      case ParallelBailoutCalledToUncompiledScript:
        return "called to uncompiled script";
      case ParallelBailoutIllegalWrite:
        return "illegal write";
      case ParallelBailoutAccessToIntrinsic:
        return "access to intrinsic";
      case ParallelBailoutOverRecursed:
        return "over recursed";
      case ParallelBailoutOutOfMemory:
        return "out of memory";
      case ParallelBailoutUnsupported:
        return "unsupported";
      case ParallelBailoutUnsupportedVM:
        return "unsupported operation in VM call";
      case ParallelBailoutUnsupportedStringComparison:
        return "unsupported string comparison";
      case ParallelBailoutRequestedGC:
        return "requested GC";
      case ParallelBailoutRequestedZoneGC:
        return "requested zone GC";
      default:
        return "no known reason";
    }
}

void
js::ForkJoinOperation::determineBailoutCause()
{
    bailoutCause = ParallelBailoutNone;
    for (uint32_t i = 0; i < bailoutRecords_.length(); i++) {
        if (bailoutRecords_[i].cause == ParallelBailoutNone)
            continue;

        if (bailoutRecords_[i].cause == ParallelBailoutInterrupt)
            continue;

        bailoutCause = bailoutRecords_[i].cause;
        const char *causeStr = BailoutExplanation(bailoutCause);
        if (bailoutRecords_[i].depth) {
            bailoutScript = bailoutRecords_[i].trace[0].script;
            bailoutBytecode = bailoutRecords_[i].trace[0].bytecode;

            const char *filename = bailoutScript->filename();
            int line = JS_PCToLineNumber(cx_, bailoutScript, bailoutBytecode);
            JS_ReportWarning(cx_, "Bailed out of parallel operation: %s at %s:%d",
                             causeStr, filename, line);

            Spew(SpewBailouts, "Bailout from thread %d: cause %d at loc %s:%d",
                 i,
                 bailoutCause,
                 bailoutScript->filename(),
                 PCToLineNumber(bailoutScript, bailoutBytecode));
        } else {
            JS_ReportWarning(cx_, "Bailed out of parallel operation: %s",
                             causeStr);

            Spew(SpewBailouts, "Bailout from thread %d: cause %d, unknown loc",
                 i,
                 bailoutCause);
        }
    }
}

bool
js::ForkJoinOperation::invalidateBailedOutScripts()
{
    Vector<types::RecompileInfo> invalid(cx_);
    for (uint32_t i = 0; i < bailoutRecords_.length(); i++) {
        RootedScript script(cx_, bailoutRecords_[i].topScript);

        
        if (!script || !script->hasParallelIonScript())
            continue;

        Spew(SpewBailouts,
             "Bailout from thread %d: cause %d, topScript %p:%s:%d",
             i,
             bailoutRecords_[i].cause,
             script.get(), script->filename(), script->lineno());

        switch (bailoutRecords_[i].cause) {
          
          
          case ParallelBailoutInterrupt: continue;

          
          case ParallelBailoutIllegalWrite: continue;

          
          default: break;
        }

        
        if (hasScript(invalid, script))
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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::warmupExecution(bool stopIfComplete, ExecutionStatus *status)
{
    
    

    Spew(SpewOps, "Executing warmup of slice %u.", warmupSlice_);

    AutoEnterWarmup warmup(cx_->runtime());
    RootedValue funVal(cx_, ObjectValue(*fun_));
    bool complete;
    uint32_t warmupTo = Min<uint16_t>(warmupSlice_ + 1, numSlices_);
    if (!ExecuteSequentially(cx_, funVal, &complete, warmupSlice_, warmupTo)) {
        *status = ExecutionFatal;
        return RedLight;
    }

    if (complete) {
        warmupSlice_ = warmupTo;
        if (warmupSlice_ == numSlices_) {
            if (stopIfComplete) {
                Spew(SpewOps, "Warmup execution finished all the work.");
                *status = ExecutionWarmup;
                return RedLight;
            }

            
            
            
            
            if (!js_HandleExecutionInterrupt(cx_)) {
                *status = ExecutionFatal;
                return RedLight;
            }
        }
    }

    return GreenLight;
}

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::parallelExecution(ExecutionStatus *status)
{
    
    

    
    
    
    JS_ASSERT(ForkJoinSlice::current() == nullptr);

    ForkJoinActivation activation(cx_);

    ThreadPool *threadPool = &cx_->runtime()->threadPool;

    RootedObject rootedFun(cx_, fun_);
    ForkJoinShared shared(cx_, threadPool, rootedFun, numSlices_, &bailoutRecords_[0]);
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

js::ForkJoinOperation::TrafficLight
js::ForkJoinOperation::recoverFromBailout(ExecutionStatus *status)
{
    
    

    bailouts += 1;
    determineBailoutCause();

    SpewBailout(bailouts, bailoutScript, bailoutBytecode, bailoutCause);

    
    
    RootedScript mainScript(cx_, fun_->as<JSFunction>().nonLazyScript());
    if (!addToWorklist(mainScript))
        return fatalError(status);

    
    
    if (!invalidateBailedOutScripts())
        return fatalError(status);

    if (warmupExecution(true, status) == RedLight)
        return RedLight;

    return GreenLight;
}

bool
js::ForkJoinOperation::hasScript(Vector<types::RecompileInfo> &scripts, JSScript *script)
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
        calleeToken_ = CalleeToToken(callee);
    }

    bool invoke(PerThreadData *perThread) {
        RootedValue result(perThread);
        enter_(jitcode_, argc_ + 1, argv_ + 1, nullptr, calleeToken_, nullptr, 0,
               result.address());
        return !result.isMagic();
    }
};





ForkJoinShared::ForkJoinShared(JSContext *cx,
                               ThreadPool *threadPool,
                               HandleObject fun,
                               uint16_t numSlices,
                               ParallelBailoutRecord *records)
  : cx_(cx),
    threadPool_(threadPool),
    fun_(fun),
    numSlices_(numSlices),
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

    for (unsigned i = 0; i < (threadPool_->numWorkers() + 1); i++) {
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
    PR_DestroyLock(cxLock_);

    while (allocators_.length() > 0)
        js_delete(allocators_.popCopy());
}

ParallelResult
ForkJoinShared::execute()
{
    
    
    
    if (cx_->runtime()->interrupt)
        return TP_RETRY_SEQUENTIALLY;

    AutoLockMonitor lock(*this);

    ParallelResult jobResult = TP_SUCCESS;
    {
        AutoUnlockMonitor unlock(*this);

        
        jobResult = threadPool_->executeJob(cx_, this, numSlices_);
        if (jobResult == TP_FATAL)
            return TP_FATAL;
    }

    transferArenasToCompartmentAndProcessGCRequests();

    
    if (abort_) {
        if (fatal_)
            return TP_FATAL;
        return TP_RETRY_SEQUENTIALLY;
    }

#ifdef DEBUG
    Spew(SpewOps, "Completed parallel job [slices %d, threads: %d (+1), stolen: %d (work stealing:%s)]",
         numSlices_,
         threadPool_->numWorkers(),
         threadPool_->stolenSlices(),
         threadPool_->workStealing() ? "ON" : "OFF");
#endif

    
    return jobResult;
}

void
ForkJoinShared::transferArenasToCompartmentAndProcessGCRequests()
{
    JSCompartment *comp = cx_->compartment();
    for (unsigned i = 0; i < (threadPool_->numWorkers() + 1); i++)
        comp->adoptWorkerAllocator(allocators_[i]);

    if (gcRequested_) {
        if (!gcZone_)
            TriggerGC(cx_->runtime(), gcReason_);
        else
            TriggerZoneGC(gcZone_, gcReason_);
        gcRequested_ = false;
        gcZone_ = nullptr;
    }
}

bool
ForkJoinShared::executeFromWorker(uint16_t sliceId, uint32_t workerId, uintptr_t stackLimit)
{
    JS_ASSERT(sliceId <= numSlices_);

    PerThreadData thisThread(cx_->runtime());
    if (!thisThread.init()) {
        setAbortFlag(true);
        return false;
    }
    TlsPerThreadData.set(&thisThread);

    
    
    thisThread.ionStackLimit = stackLimit;
    executePortion(&thisThread, sliceId, workerId);
    TlsPerThreadData.set(nullptr);

    return !abort_;
}

bool
ForkJoinShared::executeFromMainThread(uint16_t sliceId)
{
    executePortion(&cx_->mainThread(), sliceId, threadPool_->numWorkers());
    return !abort_;
}

void
ForkJoinShared::executePortion(PerThreadData *perThread, uint16_t sliceId, uint32_t workerId)
{
    
    

    
    
    
    JS::AutoAssertNoGC nogc(runtime());

    Allocator *allocator = allocators_[workerId];
    ForkJoinSlice slice(perThread, sliceId, workerId, allocator, this, &records_[workerId]);
    AutoSetForkJoinSlice autoContext(&slice);

#ifdef DEBUG
    
    slice.maxSliceId = numSlices_ - 1;
    slice.maxWorkerId = threadPool_->numWorkers();
#endif

    Spew(SpewOps, "Slice up");

    
    
    IonContext icx(CompileRuntime::get(cx_->runtime()),
                   CompileCompartment::get(cx_->compartment()),
                   nullptr);

    JS_ASSERT(slice.bailoutRecord->topScript == nullptr);

    RootedObject fun(perThread, fun_);
    JS_ASSERT(fun->is<JSFunction>());
    RootedFunction callee(perThread, &fun->as<JSFunction>());
    if (!callee->nonLazyScript()->hasParallelIonScript()) {
        
        
        
        
        Spew(SpewOps, "Down (Script no longer present)");
        slice.bailoutRecord->setCause(ParallelBailoutMainScriptNotPresent);
        setAbortFlag(false);
    } else {
        ParallelIonInvoke<2> fii(cx_->runtime(), callee, 2);

        fii.args[0] = Int32Value(slice.sliceId);
        fii.args[1] = BooleanValue(false);

        bool ok = fii.invoke(perThread);
        JS_ASSERT(ok == !slice.bailoutRecord->topScript);
        if (!ok)
            setAbortFlag(false);
    }

    Spew(SpewOps, "Slice down");
}

bool
ForkJoinShared::check(ForkJoinSlice &slice)
{
    JS_ASSERT(cx_->runtime()->interrupt);

    if (abort_)
        return false;

    
    
    
    if (slice.isMainThread() || !threadPool_->isMainThreadActive()) {
        JS_ASSERT(!cx_->runtime()->gcIsNeeded);

        if (cx_->runtime()->interrupt) {
            
            
            
            JS_ASSERT(!cx_->runtime()->gcIsNeeded);

            slice.bailoutRecord->setCause(ParallelBailoutInterrupt);
            setAbortFlag(false);
            return false;
        }
    }

    return true;
}

void
ForkJoinShared::setAbortFlag(bool fatal)
{
    AutoLockMonitor lock(*this);

    abort_ = true;
    fatal_ = fatal_ || fatal;

    
    
    cx_->runtime()->triggerOperationCallback(JSRuntime::TriggerCallbackAnyThreadDontStopIon);
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





ForkJoinSlice::ForkJoinSlice(PerThreadData *perThreadData,
                             uint16_t sliceId, uint32_t workerId,
                             Allocator *allocator, ForkJoinShared *shared,
                             ParallelBailoutRecord *bailoutRecord)
  : ThreadSafeContext(shared->runtime(), perThreadData, Context_ForkJoin),
    sliceId(sliceId),
    workerId(workerId),
    bailoutRecord(bailoutRecord),
    shared(shared),
    acquiredContext_(false),
    nogc_(shared->runtime())
{
    



    zone_ = shared->zone();

    



    compartment_ = shared->compartment();

    allocator_ = allocator;
}

bool
ForkJoinSlice::isMainThread() const
{
    return perThreadData == &shared->runtime()->mainThread;
}

JSRuntime *
ForkJoinSlice::runtime()
{
    return shared->runtime();
}

JSContext *
ForkJoinSlice::acquireContext()
{
    JSContext *cx = shared->acquireContext();
    JS_ASSERT(!acquiredContext_);
    acquiredContext_ = true;
    return cx;
}

void
ForkJoinSlice::releaseContext()
{
    JS_ASSERT(acquiredContext_);
    acquiredContext_ = false;
    return shared->releaseContext();
}

bool
ForkJoinSlice::hasAcquiredContext() const
{
    return acquiredContext_;
}

bool
ForkJoinSlice::check()
{
    if (runtime()->interrupt)
        return shared->check(*this);
    else
        return true;
}

void
ForkJoinSlice::requestGC(JS::gcreason::Reason reason)
{
    shared->requestGC(reason);
    bailoutRecord->setCause(ParallelBailoutRequestedGC);
    shared->setAbortFlag(false);
}

void
ForkJoinSlice::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    shared->requestZoneGC(zone, reason);
    bailoutRecord->setCause(ParallelBailoutRequestedZoneGC);
    shared->setAbortFlag(false);
}

bool
ForkJoinSlice::setPendingAbortFatal(ParallelBailoutCause cause)
{
    shared->setPendingAbortFatal();
    bailoutRecord->setCause(cause);
    return false;
}




void
js::ParallelBailoutRecord::init(JSContext *cx)
{
    reset(cx);
}

void
js::ParallelBailoutRecord::reset(JSContext *cx)
{
    topScript = nullptr;
    cause = ParallelBailoutNone;
    depth = 0;
}

void
js::ParallelBailoutRecord::setCause(ParallelBailoutCause cause,
                                    JSScript *outermostScript,
                                    JSScript *currentScript,
                                    jsbytecode *currentPc)
{
    this->cause = cause;
    updateCause(cause, outermostScript, currentScript, currentPc);
}

void
js::ParallelBailoutRecord::updateCause(ParallelBailoutCause cause,
                                       JSScript *outermostScript,
                                       JSScript *currentScript,
                                       jsbytecode *currentPc)
{
    JS_ASSERT_IF(outermostScript, currentScript);
    JS_ASSERT_IF(outermostScript, outermostScript->hasParallelIonScript());
    JS_ASSERT_IF(currentScript, outermostScript);
    JS_ASSERT_IF(!currentScript, !currentPc);

    if (this->cause == ParallelBailoutNone)
        this->cause = cause;

    if (outermostScript)
        this->topScript = outermostScript;

    if (currentScript)
        addTrace(currentScript, currentPc);
}

void
js::ParallelBailoutRecord::addTrace(JSScript *script,
                                    jsbytecode *pc)
{
    
    
    
    if (topScript == nullptr && script != nullptr)
        topScript = script;

    if (depth < MaxDepth) {
        trace[depth].script = script;
        trace[depth].bytecode = pc;
        depth += 1;
    }
}







#ifdef DEBUG

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

        if (ForkJoinSlice *slice = ForkJoinSlice::current()) {
            
            
            char bufbuf[BufferSize];
            JS_snprintf(bufbuf, BufferSize, "[%%sParallel:%%0%du(%%0%du)%%s] ",
                        NumberOfDigits(slice->maxWorkerId), NumberOfDigits(slice->maxSliceId));
            JS_snprintf(buf, BufferSize, bufbuf, workerColor(slice->workerId),
                        slice->workerId, slice->sliceId, reset());
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
            JSScript *script = cx->currentScript(&pc);
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

        spew(SpewOps, "%s%sBAILOUT %d%s: %d at %s:%d:%d", bold(), yellow(), count, reset(), cause, filename, line, column);
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

    void spewBailoutIR(IonLIRTraceData *data) {
        if (!active[SpewBailouts])
            return;

        
        
        
        if (data->mirOpName && data->script) {
            spew(SpewBailouts, "%sBailout%s: %s / %s%s%s (block %d lir %d) (%s:%u)", yellow(), reset(),
                 data->lirOpName, cyan(), data->mirOpName, reset(),
                 data->blockIndex, data->lirIndex, data->script->filename(),
                 PCToLineNumber(data->script, data->pc));
        }
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

void
parallel::SpewBailoutIR(IonLIRTraceData *data)
{
    spewer.spewBailoutIR(data);
}

#endif 

bool
js::InExclusiveParallelSection()
{
    return InParallelSection() && ForkJoinSlice::current()->hasAcquiredContext();
}

bool
js::ParallelTestsShouldPass(JSContext *cx)
{
    return jit::IsIonEnabled(cx) &&
           jit::IsBaselineEnabled(cx) &&
           !jit::js_JitOptions.eagerCompilation &&
           jit::js_JitOptions.baselineUsesBeforeCompile != 0 &&
           cx->runtime()->gcZeal() == 0;
}

#endif 
