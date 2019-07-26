





#include "jscntxt.h"

#ifdef JS_THREADSAFE
#  include "prthread.h"
#  include "prprf.h"
#endif

#include "vm/ForkJoin.h"

#if defined(JS_THREADSAFE)
#include "ion/BaselineJIT.h"
#include "vm/Monitor.h"
#endif

#if defined(DEBUG) && defined(JS_THREADSAFE) && defined(JS_ION)
#  include "ion/Ion.h"
#  include "ion/MIR.h"
#  include "ion/MIRGraph.h"
#  include "ion/IonCompartment.h"
#endif 

#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::parallel;
using namespace js::ion;








static bool
ExecuteSequentially(JSContext *cx_, HandleValue funVal, bool *complete);

#if !defined(JS_THREADSAFE) || !defined(JS_ION)
bool
js::ForkJoin(JSContext *cx, CallArgs &args)
{
    RootedValue argZero(cx, args[0]);
    bool complete = false; 
    return ExecuteSequentially(cx, argZero, &complete);
}

uint32_t
js::ForkJoinSlices(JSContext *cx)
{
    return 1; 
}

JSContext *
ForkJoinSlice::acquireContext()
{
    return NULL;
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

bool
ForkJoinSlice::InitializeTLS()
{
    return true;
}

JSRuntime *
ForkJoinSlice::runtime()
{
    MOZ_NOT_REACHED("Not THREADSAFE build");
}

bool
ForkJoinSlice::check()
{
    MOZ_NOT_REACHED("Not THREADSAFE build");
}

void
ForkJoinSlice::requestGC(JS::gcreason::Reason reason)
{
    MOZ_NOT_REACHED("Not THREADSAFE build");
}

void
ForkJoinSlice::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    MOZ_NOT_REACHED("Not THREADSAFE build");
}

void
ParallelBailoutRecord::setCause(ParallelBailoutCause cause,
                                JSScript *outermostScript,
                                JSScript *currentScript,
                                jsbytecode *currentPc)
{
    MOZ_NOT_REACHED("Not THREADSAFE build");
}

void
ParallelBailoutRecord::addTrace(JSScript *script,
                                jsbytecode *pc)
{
    MOZ_NOT_REACHED("Not THREADSAFE build");
}

bool
js::InSequentialOrExclusiveParallelSection()
{
    return true;
}

bool
js::ParallelTestsShouldPass(JSContext *cx)
{
    return false;
}

#endif 






static bool
ExecuteSequentially(JSContext *cx, HandleValue funVal, bool *complete)
{
    uint32_t numSlices = ForkJoinSlices(cx);
    FastInvokeGuard fig(cx, funVal);
    bool allComplete = true;
    for (uint32_t i = 0; i < numSlices; i++) {
        InvokeArgs &args = fig.args();
        if (!args.init(3))
            return false;
        args.setCallee(funVal);
        args.setThis(UndefinedValue());
        args[0].setInt32(i);
        args[1].setInt32(numSlices);
        args[2].setBoolean(!!cx->runtime()->parallelWarmup);
        if (!fig.invoke(cx))
            return false;
        allComplete = allComplete & args.rval().toBoolean();
    }
    *complete = allComplete;
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

unsigned ForkJoinSlice::ThreadPrivateIndex;
bool ForkJoinSlice::TLSInitialized;

class ParallelDo
{
  public:
    
    const static uint32_t MAX_BAILOUTS = 3;
    uint32_t bailouts;

    
    ParallelBailoutCause bailoutCause;
    RootedScript bailoutScript;
    jsbytecode *bailoutBytecode;

    ParallelDo(JSContext *cx, HandleObject fun, ForkJoinMode mode);
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

class ForkJoinShared : public TaskExecutor, public Monitor
{
    
    

    JSContext *const cx_;          
    ThreadPool *const threadPool_; 
    HandleObject fun_;             
    const uint32_t numSlices_;     
    PRCondVar *rendezvousEnd_;     
    PRLock *cxLock_;               
    ParallelBailoutRecord *const records_; 

    
    
    
    

    Vector<Allocator *, 16> allocators_;

    
    
    
    

    uint32_t uncompleted_;          
    uint32_t blocked_;              
    uint32_t rendezvousIndex_;      
    bool gcRequested_;              
    JS::gcreason::Reason gcReason_; 
    Zone *gcZone_;                  

    
    
    
    
    

    
    volatile bool abort_;

    
    volatile bool fatal_;

    
    volatile bool rendezvous_;

    
    void executeFromMainThread();

    
    
    void executePortion(PerThreadData *perThread, uint32_t threadId);

    
    
    
    

    friend class AutoRendezvous;

    
    
    void initiateRendezvous(ForkJoinSlice &threadCx);

    
    
    void joinRendezvous(ForkJoinSlice &threadCx);

    
    
    void endRendezvous(ForkJoinSlice &threadCx);

  public:
    ForkJoinShared(JSContext *cx,
                   ThreadPool *threadPool,
                   HandleObject fun,
                   uint32_t numSlices,
                   uint32_t uncompleted,
                   ParallelBailoutRecord *records);
    ~ForkJoinShared();

    bool init();

    ParallelResult execute();

    
    virtual void executeFromWorker(uint32_t threadId, uintptr_t stackLimit);

    
    
    
    
    void transferArenasToCompartmentAndProcessGCRequests();

    
    bool check(ForkJoinSlice &threadCx);

    
    void requestGC(JS::gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason);

    
    void setAbortFlag(bool fatal);

    JSRuntime *runtime() { return cx_->runtime(); }
    JS::Zone *zone() { return cx_->zone(); }

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

class AutoRendezvous
{
  private:
    ForkJoinSlice &threadCx;

  public:
    AutoRendezvous(ForkJoinSlice &threadCx)
        : threadCx(threadCx)
    {
        threadCx.shared->initiateRendezvous(threadCx);
    }

    ~AutoRendezvous() {
        threadCx.shared->endRendezvous(threadCx);
    }
};

class AutoSetForkJoinSlice
{
  public:
    AutoSetForkJoinSlice(ForkJoinSlice *threadCx) {
        PR_SetThreadPrivate(ForkJoinSlice::ThreadPrivateIndex, threadCx);
    }

    ~AutoSetForkJoinSlice() {
        PR_SetThreadPrivate(ForkJoinSlice::ThreadPrivateIndex, NULL);
    }
};

} 








static const char *ForkJoinModeString(ForkJoinMode mode);

bool
js::ForkJoin(JSContext *cx, CallArgs &args)
{
    JS_ASSERT(args[0].isObject()); 
    JS_ASSERT(args[0].toObject().is<JSFunction>());

    ForkJoinMode mode = ForkJoinModeNormal;
    if (args.length() > 1) {
        JS_ASSERT(args[1].isInt32()); 
        JS_ASSERT(args[1].toInt32() < NumForkJoinModes);
        mode = (ForkJoinMode) args[1].toInt32();
    }

    RootedObject fun(cx, &args[0].toObject());
    ParallelDo op(cx, fun, mode);
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

js::ParallelDo::ParallelDo(JSContext *cx,
                           HandleObject fun,
                           ForkJoinMode mode)
  : bailouts(0),
    bailoutCause(ParallelBailoutNone),
    bailoutScript(cx),
    bailoutBytecode(NULL),
    cx_(cx),
    fun_(fun),
    bailoutRecords_(cx),
    worklist_(cx),
    worklistData_(cx),
    mode_(mode)
{ }

ExecutionStatus
js::ParallelDo::apply()
{
    ExecutionStatus status;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT_IF(!ion::IsBaselineEnabled(cx_), !ion::IsEnabled(cx_));
    if (!ion::IsBaselineEnabled(cx_) || !ion::IsEnabled(cx_))
        return sequentialExecution(true);

    SpewBeginOp(cx_, "ParallelDo");

    uint32_t slices = ForkJoinSlices(cx_);

    if (!bailoutRecords_.resize(slices))
        return SpewEndOp(ExecutionFatal);

    for (uint32_t i = 0; i < slices; i++)
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
            return ExecutionFatal;
        }
        break;

      case NumForkJoinModes:
        MOZ_ASSUME_NOT_REACHED("Invalid mode");
    }

    while (bailouts < MAX_BAILOUTS) {
        for (uint32_t i = 0; i < slices; i++)
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

js::ParallelDo::TrafficLight
js::ParallelDo::enqueueInitialScript(ExecutionStatus *status)
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
                 script.get(), script->filename(), script->lineno);
            return GreenLight;
        }

        Spew(SpewOps, "Script %p:%s:%d already compiled, may have uncompiled callees",
             script.get(), script->filename(), script->lineno);
    }

    
    if (addToWorklist(script) == RedLight)
        return fatalError(status);
    return GreenLight;
}

js::ParallelDo::TrafficLight
js::ParallelDo::compileForParallelExecution(ExecutionStatus *status)
{
    
    

    
    
    
    
    
    

    RootedFunction fun(cx_);
    RootedScript script(cx_);

    
    
    const uint32_t stallThreshold = 3;

    
    
    
    
    
    
    while (true) {
        bool offMainThreadCompilationsInProgress = false;
        bool gatheringTypeInformation = false;

        
        for (uint32_t i = 0; i < worklist_.length(); i++) {
            script = worklist_[i];
            fun = script->function();

            
            
            
            
            
            
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
                         script.get(), script->filename(), script->lineno,
                         previousUseCount, currentUseCount);
                } else {
                    uint32_t stallCount = ++worklistData_[i].stallCount;
                    if (stallCount < stallThreshold) {
                        gatheringTypeInformation = true;
                    }

                    Spew(SpewCompile,
                         "Script %p:%s:%d has no baseline script, "
                         "and use count has %u stalls at %d",
                         script.get(), script->filename(), script->lineno,
                         stallCount, previousUseCount);
                }
                continue;
            }

            if (!script->hasParallelIonScript()) {
                
                SpewBeginCompile(script);
                MethodStatus mstatus = ion::CanEnterInParallel(cx_, script);
                SpewEndCompile(mstatus);

                switch (mstatus) {
                  case Method_Error:
                    return fatalError(status);

                  case Method_CantCompile:
                    Spew(SpewCompile,
                         "Script %p:%s:%d cannot be compiled, "
                         "falling back to sequential execution",
                         script.get(), script->filename(), script->lineno);
                    return sequentialExecution(true, status);

                  case Method_Skipped:
                    
                    
                    if (script->isParallelIonCompilingOffThread()) {
                        Spew(SpewCompile,
                             "Script %p:%s:%d compiling off-thread",
                             script.get(), script->filename(), script->lineno);
                        offMainThreadCompilationsInProgress = true;
                        continue;
                    }
                    return sequentialExecution(false, status);

                  case Method_Compiled:
                    Spew(SpewCompile,
                         "Script %p:%s:%d compiled",
                         script.get(), script->filename(), script->lineno);
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
                         script.get(), script->filename(), script->lineno);
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

js::ParallelDo::TrafficLight
js::ParallelDo::appendCallTargetsToWorklist(uint32_t index,
                                            ExecutionStatus *status)
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
                       target->filename(), target->lineno);
        if (appendCallTargetToWorklist(target, status) == RedLight)
            return RedLight;
    }

    return GreenLight;
}

js::ParallelDo::TrafficLight
js::ParallelDo::appendCallTargetToWorklist(HandleScript script,
                                           ExecutionStatus *status)
{
    
    

    JS_ASSERT(script);

    
    if (!script->canParallelIonCompile()) {
        Spew(SpewCompile, "Skipping %p:%s:%u, canParallelIonCompile() is false",
             script.get(), script->filename(), script->lineno);
        return sequentialExecution(true, status);
    }

    if (script->hasParallelIonScript()) {
        
        if (script->parallelIonScript()->bailoutExpected()) {
            Spew(SpewCompile, "Skipping %p:%s:%u, bailout expected",
                 script.get(), script->filename(), script->lineno);
            return sequentialExecution(false, status);
        }
    }

    if (!addToWorklist(script))
        return fatalError(status);

    return GreenLight;
}

bool
js::ParallelDo::addToWorklist(HandleScript script)
{
    for (uint32_t i = 0; i < worklist_.length(); i++) {
        if (worklist_[i] == script) {
            Spew(SpewCompile, "Skipping %p:%s:%u, already in worklist",
                 script.get(), script->filename(), script->lineno);
            return true;
        }
    }

    Spew(SpewCompile, "Enqueued %p:%s:%u",
         script.get(), script->filename(), script->lineno);

    
    
    
    
    if (!worklist_.append(script))
        return false;

    
    if (!worklistData_.append(WorklistData()))
        return false;
    worklistData_[worklistData_.length() - 1].reset();

    return true;
}

js::ParallelDo::TrafficLight
js::ParallelDo::sequentialExecution(bool disqualified, ExecutionStatus *status)
{
    

    *status = sequentialExecution(disqualified);
    return RedLight;
}

ExecutionStatus
js::ParallelDo::sequentialExecution(bool disqualified)
{
    

    Spew(SpewOps, "Executing sequential execution (disqualified=%d).",
         disqualified);

    bool complete = false;
    RootedValue funVal(cx_, ObjectValue(*fun_));
    if (!ExecuteSequentially(cx_, funVal, &complete))
        return ExecutionFatal;

    
    
    JS_ASSERT(complete);
    return ExecutionSequential;
}

js::ParallelDo::TrafficLight
js::ParallelDo::fatalError(ExecutionStatus *status)
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
        return "at runtime, the behavior changed, invalidating compiled code (IC update)";
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
      case ParallelBailoutUnsupportedStringComparison:
        return "unsupported string comparison";
      case ParallelBailoutUnsupportedSparseArray:
        return "unsupported sparse array";
      case ParallelBailoutRequestedGC:
        return "requested GC";
      case ParallelBailoutRequestedZoneGC:
        return "requested zone GC";
      default:
        return "no known reason";
    }
}

void
js::ParallelDo::determineBailoutCause()
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
js::ParallelDo::invalidateBailedOutScripts()
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
             script.get(), script->filename(), script->lineno);

        switch (bailoutRecords_[i].cause) {
          
          
          case ParallelBailoutInterrupt: continue;

          
          case ParallelBailoutIllegalWrite: continue;

          
          default: break;
        }

        
        if (hasScript(invalid, script))
            continue;

        Spew(SpewBailouts, "Invalidating script %p:%s:%d due to cause %d",
             script.get(), script->filename(), script->lineno,
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

js::ParallelDo::TrafficLight
js::ParallelDo::warmupExecution(bool stopIfComplete,
                                ExecutionStatus *status)
{
    
    

    Spew(SpewOps, "Executing warmup.");

    AutoEnterWarmup warmup(cx_->runtime());
    RootedValue funVal(cx_, ObjectValue(*fun_));
    bool complete;
    if (!ExecuteSequentially(cx_, funVal, &complete)) {
        *status = ExecutionFatal;
        return RedLight;
    }

    if (complete && stopIfComplete) {
        Spew(SpewOps, "Warmup execution finished all the work.");
        *status = ExecutionWarmup;
        return RedLight;
    }

    return GreenLight;
}

class AutoEnterParallelSection
{
  private:
    JSContext *cx_;
    uint8_t *prevIonTop_;

  public:
    AutoEnterParallelSection(JSContext *cx)
      : cx_(cx),
        prevIonTop_(cx->mainThread().ionTop)
    {
        
        
        
        
        
        

        if (JS::IsIncrementalGCInProgress(cx->runtime())) {
            JS::PrepareForIncrementalGC(cx->runtime());
            JS::FinishIncrementalGC(cx->runtime(), JS::gcreason::API);
        }

        MinorGC(cx->runtime(), JS::gcreason::API);

        cx->runtime()->gcHelperThread.waitBackgroundSweepEnd();
    }

    ~AutoEnterParallelSection() {
        cx_->mainThread().ionTop = prevIonTop_;
    }
};

js::ParallelDo::TrafficLight
js::ParallelDo::parallelExecution(ExecutionStatus *status)
{
    
    

    
    
    
    JS_ASSERT(ForkJoinSlice::Current() == NULL);

    AutoEnterParallelSection enter(cx_);

    ThreadPool *threadPool = &cx_->runtime()->threadPool;
    uint32_t numSlices = ForkJoinSlices(cx_);

    RootedObject rootedFun(cx_, fun_);
    ForkJoinShared shared(cx_, threadPool, rootedFun, numSlices, numSlices - 1,
                          &bailoutRecords_[0]);
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

js::ParallelDo::TrafficLight
js::ParallelDo::recoverFromBailout(ExecutionStatus *status)
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
js::ParallelDo::hasScript(Vector<types::RecompileInfo> &scripts, JSScript *script)
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
    EnterIonCode enter_;
    void *jitcode_;
    void *calleeToken_;
    Value argv_[maxArgc + 2];
    uint32_t argc_;

  public:
    Value *args;

    ParallelIonInvoke(JSCompartment *compartment,
                      HandleFunction callee,
                      uint32_t argc)
      : argc_(argc),
        args(argv_ + 2)
    {
        JS_ASSERT(argc <= maxArgc + 2);

        
        argv_[0] = ObjectValue(*callee);
        argv_[1] = UndefinedValue();

        
        IonScript *ion = callee->nonLazyScript()->parallelIonScript();
        IonCode *code = ion->method();
        jitcode_ = code->raw();
        enter_ = compartment->ionCompartment()->enterJIT();
        calleeToken_ = CalleeToParallelToken(callee);
    }

    bool invoke(PerThreadData *perThread) {
        RootedValue result(perThread);
        enter_(jitcode_, argc_ + 1, argv_ + 1, NULL, calleeToken_, NULL, 0, result.address());
        return !result.isMagic();
    }
};





ForkJoinShared::ForkJoinShared(JSContext *cx,
                               ThreadPool *threadPool,
                               HandleObject fun,
                               uint32_t numSlices,
                               uint32_t uncompleted,
                               ParallelBailoutRecord *records)
  : cx_(cx),
    threadPool_(threadPool),
    fun_(fun),
    numSlices_(numSlices),
    rendezvousEnd_(NULL),
    cxLock_(NULL),
    records_(records),
    allocators_(cx),
    uncompleted_(uncompleted),
    blocked_(0),
    rendezvousIndex_(0),
    gcRequested_(false),
    gcReason_(JS::gcreason::NUM_REASONS),
    gcZone_(NULL),
    abort_(false),
    fatal_(false),
    rendezvous_(false)
{
}

bool
ForkJoinShared::init()
{
    
    
    
    
    
    
    
    
    

    if (!Monitor::init())
        return false;

    rendezvousEnd_ = PR_NewCondVar(lock_);
    if (!rendezvousEnd_)
        return false;

    cxLock_ = PR_NewLock();
    if (!cxLock_)
        return false;

    for (unsigned i = 0; i < numSlices_; i++) {
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
    if (rendezvousEnd_)
        PR_DestroyCondVar(rendezvousEnd_);

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

    
    {
        AutoUnlockMonitor unlock(*this);
        if (!threadPool_->submitAll(cx_, this))
            return TP_FATAL;
        executeFromMainThread();
    }

    
    while (uncompleted_ > 0)
        lock.wait();

    transferArenasToCompartmentAndProcessGCRequests();

    
    if (abort_) {
        if (fatal_)
            return TP_FATAL;
        else
            return TP_RETRY_SEQUENTIALLY;
    }

    
    return TP_SUCCESS;
}

void
ForkJoinShared::transferArenasToCompartmentAndProcessGCRequests()
{
    JSCompartment *comp = cx_->compartment();
    for (unsigned i = 0; i < numSlices_; i++)
        comp->adoptWorkerAllocator(allocators_[i]);

    if (gcRequested_) {
        if (!gcZone_)
            TriggerGC(cx_->runtime(), gcReason_);
        else
            TriggerZoneGC(gcZone_, gcReason_);
        gcRequested_ = false;
        gcZone_ = NULL;
    }
}

void
ForkJoinShared::executeFromWorker(uint32_t workerId, uintptr_t stackLimit)
{
    JS_ASSERT(workerId < numSlices_ - 1);

    PerThreadData thisThread(cx_->runtime());
    TlsPerThreadData.set(&thisThread);

    
    
    thisThread.ionStackLimit = stackLimit;
    executePortion(&thisThread, workerId);
    TlsPerThreadData.set(NULL);

    AutoLockMonitor lock(*this);
    uncompleted_ -= 1;
    if (blocked_ == uncompleted_) {
        
        
        
        lock.notify();
    }
}

void
ForkJoinShared::executeFromMainThread()
{
    executePortion(&cx_->mainThread(), numSlices_ - 1);
}

void
ForkJoinShared::executePortion(PerThreadData *perThread,
                               uint32_t threadId)
{
    
    

    Allocator *allocator = allocators_[threadId];
    ForkJoinSlice slice(perThread, threadId, numSlices_, allocator,
                        this, &records_[threadId]);
    AutoSetForkJoinSlice autoContext(&slice);

    Spew(SpewOps, "Up");

    
    
    IonContext icx(cx_->compartment(), NULL);

    JS_ASSERT(slice.bailoutRecord->topScript == NULL);

    RootedObject fun(perThread, fun_);
    JS_ASSERT(fun->is<JSFunction>());
    RootedFunction callee(perThread, &fun->as<JSFunction>());
    if (!callee->nonLazyScript()->hasParallelIonScript()) {
        
        
        
        
        Spew(SpewOps, "Down (Script no longer present)");
        slice.bailoutRecord->setCause(ParallelBailoutMainScriptNotPresent,
                                      NULL, NULL, NULL);
        setAbortFlag(false);
    } else {
        ParallelIonInvoke<3> fii(cx_->compartment(), callee, 3);

        fii.args[0] = Int32Value(slice.sliceId);
        fii.args[1] = Int32Value(slice.numSlices);
        fii.args[2] = BooleanValue(false);

        bool ok = fii.invoke(perThread);
        JS_ASSERT(ok == !slice.bailoutRecord->topScript);
        if (!ok)
            setAbortFlag(false);
    }

    Spew(SpewOps, "Down");
}

bool
ForkJoinShared::check(ForkJoinSlice &slice)
{
    JS_ASSERT(cx_->runtime()->interrupt);

    if (abort_)
        return false;

    if (slice.isMainThread()) {
        JS_ASSERT(!cx_->runtime()->gcIsNeeded);

        if (cx_->runtime()->interrupt) {
            
            
            
            JS_ASSERT(!cx_->runtime()->gcIsNeeded);

            
            
            
            
            
            slice.bailoutRecord->setCause(ParallelBailoutInterrupt,
                                          NULL, NULL, NULL);
            setAbortFlag(false);
            return false;
        }
    } else if (rendezvous_) {
        joinRendezvous(slice);
    }

    return true;
}

void
ForkJoinShared::initiateRendezvous(ForkJoinSlice &slice)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(slice.isMainThread());
    JS_ASSERT(!rendezvous_ && blocked_ == 0);
    JS_ASSERT(cx_->runtime()->interrupt);

    AutoLockMonitor lock(*this);

    
    rendezvous_ = true;

    
    while (blocked_ != uncompleted_)
        lock.wait();
}

void
ForkJoinShared::joinRendezvous(ForkJoinSlice &slice)
{
    JS_ASSERT(!slice.isMainThread());
    JS_ASSERT(rendezvous_);

    AutoLockMonitor lock(*this);
    const uint32_t index = rendezvousIndex_;
    blocked_ += 1;

    
    if (blocked_ == uncompleted_)
        lock.notify();

    
    
    
    
    while (rendezvousIndex_ == index)
        PR_WaitCondVar(rendezvousEnd_, PR_INTERVAL_NO_TIMEOUT);
}

void
ForkJoinShared::endRendezvous(ForkJoinSlice &slice)
{
    JS_ASSERT(slice.isMainThread());

    AutoLockMonitor lock(*this);
    rendezvous_ = false;
    blocked_ = 0;
    rendezvousIndex_++;

    
    PR_NotifyAllCondVar(rendezvousEnd_);
}

void
ForkJoinShared::setAbortFlag(bool fatal)
{
    AutoLockMonitor lock(*this);

    abort_ = true;
    fatal_ = fatal_ || fatal;

    cx_->runtime()->triggerOperationCallback();
}

void
ForkJoinShared::requestGC(JS::gcreason::Reason reason)
{
    AutoLockMonitor lock(*this);

    gcZone_ = NULL;
    gcReason_ = reason;
    gcRequested_ = true;
}

void
ForkJoinShared::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    AutoLockMonitor lock(*this);

    if (gcRequested_ && gcZone_ != zone) {
        
        
        gcZone_ = NULL;
        gcReason_ = reason;
        gcRequested_ = true;
    } else {
        
        gcZone_ = zone;
        gcReason_ = reason;
        gcRequested_ = true;
    }
}





ForkJoinSlice::ForkJoinSlice(PerThreadData *perThreadData,
                             uint32_t sliceId, uint32_t numSlices,
                             Allocator *allocator, ForkJoinShared *shared,
                             ParallelBailoutRecord *bailoutRecord)
  : ThreadSafeContext(shared->runtime(), perThreadData, Context_ForkJoin),
    sliceId(sliceId),
    numSlices(numSlices),
    bailoutRecord(bailoutRecord),
    shared(shared),
    acquiredContext_(false)
{
    



    zone_ = shared->zone();
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

bool
ForkJoinSlice::InitializeTLS()
{
    if (!TLSInitialized) {
        TLSInitialized = true;
        PRStatus status = PR_NewThreadPrivateIndex(&ThreadPrivateIndex, NULL);
        return status == PR_SUCCESS;
    }
    return true;
}

void
ForkJoinSlice::requestGC(JS::gcreason::Reason reason)
{
    shared->requestGC(reason);
    bailoutRecord->setCause(ParallelBailoutRequestedGC,
                            NULL, NULL, NULL);
    shared->setAbortFlag(false);
}

void
ForkJoinSlice::requestZoneGC(JS::Zone *zone, JS::gcreason::Reason reason)
{
    shared->requestZoneGC(zone, reason);
    bailoutRecord->setCause(ParallelBailoutRequestedZoneGC,
                            NULL, NULL, NULL);
    shared->setAbortFlag(false);
}



uint32_t
js::ForkJoinSlices(JSContext *cx)
{
    
    return cx->runtime()->threadPool.numWorkers() + 1;
}




void
js::ParallelBailoutRecord::init(JSContext *cx)
{
    reset(cx);
}

void
js::ParallelBailoutRecord::reset(JSContext *cx)
{
    topScript = NULL;
    cause = ParallelBailoutNone;
    depth = 0;
}

void
js::ParallelBailoutRecord::setCause(ParallelBailoutCause cause,
                                    JSScript *outermostScript,
                                    JSScript *currentScript,
                                    jsbytecode *currentPc)
{
    JS_ASSERT_IF(outermostScript, currentScript);
    JS_ASSERT_IF(outermostScript, outermostScript->hasParallelIonScript());
    JS_ASSERT_IF(currentScript, outermostScript);
    JS_ASSERT_IF(!currentScript, !currentPc);

    this->cause = cause;

    if (outermostScript) {
        this->topScript = outermostScript;
    }

    if (currentScript) {
        addTrace(currentScript, currentPc);
    }
}

void
js::ParallelBailoutRecord::addTrace(JSScript *script,
                                    jsbytecode *pc)
{
    
    
    
    if (topScript == NULL && script != NULL)
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
    const char *sliceColor(uint32_t id) {
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
        if (env) {
            if (strcmp(env, "xterm-color") == 0 || strcmp(env, "xterm-256color") == 0)
                colorable = true;
        }
    }

    bool isActive(SpewChannel channel) {
        return active[channel];
    }

    void spewVA(SpewChannel channel, const char *fmt, va_list ap) {
        if (!active[channel])
            return;

        
        
        char buf[BufferSize];

        if (ForkJoinSlice *slice = ForkJoinSlice::Current()) {
            PR_snprintf(buf, BufferSize, "[%sParallel:%u%s] ",
                        sliceColor(slice->sliceId), slice->sliceId, reset());
        } else {
            PR_snprintf(buf, BufferSize, "[Parallel:M] ");
        }

        for (uint32_t i = 0; i < depth; i++)
            PR_snprintf(buf + strlen(buf), BufferSize, "  ");

        PR_vsnprintf(buf + strlen(buf), BufferSize, fmt, ap);
        PR_snprintf(buf + strlen(buf), BufferSize, "\n");

        fprintf(stderr, "%s", buf);
    }

    void spew(SpewChannel channel, const char *fmt, ...) {
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

        spew(SpewCompile, "COMPILE %p:%s:%u", script.get(), script->filename(), script->lineno);
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
        PR_vsnprintf(buf, BufferSize, fmt, ap);

        JSScript *script = mir->block()->info().script();
        spew(SpewCompile, "%s%s%s: %s (%s:%u)", cyan(), mir->opName(), reset(), buf,
             script->filename(), PCToLineNumber(script, mir->trackedPc()));
    }

    void spewBailoutIR(uint32_t bblockId, uint32_t lirId,
                       const char *lir, const char *mir, JSScript *script, jsbytecode *pc) {
        if (!active[SpewBailouts])
            return;

        
        
        
        if (mir && script) {
            spew(SpewBailouts, "%sBailout%s: %s / %s%s%s (block %d lir %d) (%s:%u)", yellow(), reset(),
                 lir, cyan(), mir, reset(),
                 bblockId, lirId,
                 script->filename(), PCToLineNumber(script, pc));
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
parallel::SpewBailoutIR(uint32_t bblockId, uint32_t lirId,
                        const char *lir, const char *mir,
                        JSScript *script, jsbytecode *pc)
{
    spewer.spewBailoutIR(bblockId, lirId, lir, mir, script, pc);
}

#endif 

bool
js::InSequentialOrExclusiveParallelSection()
{
    return !InParallelSection() || ForkJoinSlice::Current()->hasAcquiredContext();
}

bool
js::ParallelTestsShouldPass(JSContext *cx)
{
    return ion::IsEnabled(cx) &&
           ion::IsBaselineEnabled(cx) &&
           !ion::js_IonOptions.eagerCompilation &&
           ion::js_IonOptions.baselineUsesBeforeCompile != 0 &&
           cx->runtime()->gcZeal() == 0;
}

#endif 
