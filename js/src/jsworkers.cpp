





#include "jsworkers.h"

#include "mozilla/DebugOnly.h"

#include "prmjtime.h"

#include "frontend/BytecodeCompiler.h"

#ifdef JS_WORKER_THREADS
# include "jit/ExecutionModeInlines.h"
# include "jit/IonBuilder.h"
#endif

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"

#include "vm/ObjectImpl-inl.h"

using namespace js;

using mozilla::DebugOnly;

#ifdef JS_WORKER_THREADS

bool
js::EnsureWorkerThreadsInitialized(JSRuntime *rt)
{
    if (rt->workerThreadState)
        return true;

    rt->workerThreadState = rt->new_<WorkerThreadState>();
    if (!rt->workerThreadState)
        return false;

    if (!rt->workerThreadState->init(rt)) {
        js_delete(rt->workerThreadState);
        rt->workerThreadState = NULL;
        return false;
    }

    return true;
}

bool
js::StartOffThreadAsmJSCompile(JSContext *cx, AsmJSParallelTask *asmData)
{
    
    JS_ASSERT(cx->runtime()->workerThreadState);
    JS_ASSERT(asmData->mir);
    JS_ASSERT(asmData->lir == NULL);

    WorkerThreadState &state = *cx->runtime()->workerThreadState;
    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(cx->runtime());

    
    if (state.asmJSWorkerFailed())
        return false;

    if (!state.asmJSWorklist.append(asmData))
        return false;

    state.notify(WorkerThreadState::WORKER);
    return true;
}

bool
js::StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder)
{
    JSRuntime *rt = cx->runtime();
    if (!EnsureWorkerThreadsInitialized(rt))
        return false;

    WorkerThreadState &state = *cx->runtime()->workerThreadState;
    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(rt);

    if (!state.ionWorklist.append(builder))
        return false;

    state.notify(WorkerThreadState::WORKER);
    return true;
}






static void
FinishOffThreadIonCompile(ion::IonBuilder *builder)
{
    JSCompartment *compartment = builder->script()->compartment();
    JS_ASSERT(compartment->runtimeFromAnyThread()->workerThreadState);
    JS_ASSERT(compartment->runtimeFromAnyThread()->workerThreadState->isLocked());

    compartment->ionCompartment()->finishedOffThreadCompilations().append(builder);
}

static inline bool
CompiledScriptMatches(JSCompartment *compartment, JSScript *script, JSScript *target)
{
    if (script)
        return target == script;
    return target->compartment() == compartment;
}

void
js::CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script)
{
    JSRuntime *rt = compartment->runtimeFromMainThread();

    if (!rt->workerThreadState)
        return;

    WorkerThreadState &state = *rt->workerThreadState;

    ion::IonCompartment *ion = compartment->ionCompartment();
    if (!ion)
        return;

    AutoLockWorkerThreadState lock(rt);

    
    for (size_t i = 0; i < state.ionWorklist.length(); i++) {
        ion::IonBuilder *builder = state.ionWorklist[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            FinishOffThreadIonCompile(builder);
            state.ionWorklist[i--] = state.ionWorklist.back();
            state.ionWorklist.popBack();
        }
    }

    
    for (size_t i = 0; i < state.numThreads; i++) {
        const WorkerThread &helper = state.threads[i];
        while (helper.ionBuilder &&
               CompiledScriptMatches(compartment, script, helper.ionBuilder->script()))
        {
            helper.ionBuilder->cancel();
            state.wait(WorkerThreadState::MAIN);
        }
    }

    ion::OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    
    for (size_t i = 0; i < compilations.length(); i++) {
        ion::IonBuilder *builder = compilations[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            ion::FinishOffThreadBuilder(builder);
            compilations[i--] = compilations.back();
            compilations.popBack();
        }
    }
}

static JSClass workerGlobalClass = {
    "internal-worker-global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_DeletePropertyStub,
    JS_PropertyStub,  JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   NULL
};

ParseTask::ParseTask(JSRuntime *rt, ExclusiveContext *cx, const CompileOptions &options,
                     const jschar *chars, size_t length)
  : runtime(rt), cx(cx), options(options), chars(chars), length(length),
    alloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE), script(NULL)
{
    if (options.principals())
        JS_HoldPrincipals(options.principals());
    if (options.originPrincipals())
        JS_HoldPrincipals(options.originPrincipals());
}

ParseTask::~ParseTask()
{
    if (options.principals())
        JS_DropPrincipals(runtime, options.principals());
    if (options.originPrincipals())
        JS_DropPrincipals(runtime, options.originPrincipals());

    
    js_delete(cx);
}

bool
js::StartOffThreadParseScript(JSContext *cx, const CompileOptions &options,
                              const jschar *chars, size_t length)
{
    frontend::MaybeCallSourceHandler(cx, options, chars, length);

    JSRuntime *rt = cx->runtime();
    if (!EnsureWorkerThreadsInitialized(rt))
        return false;

    JS::CompartmentOptions compartmentOptions(cx->compartment()->options());
    compartmentOptions.setZone(JS::FreshZone);

    JSObject *global = JS_NewGlobalObject(cx, &workerGlobalClass, NULL,
                                          JS::FireOnNewGlobalHook, compartmentOptions);
    if (!global)
        return false;

    
    
    global->zone()->types.inferenceEnabled = false;

    
    
    {
        AutoCompartment ac(cx, global);

        RootedObject obj(cx);
        if (!js_GetClassObject(cx, global, JSProto_Function, &obj) ||
            !js_GetClassObject(cx, global, JSProto_Array, &obj) ||
            !js_GetClassObject(cx, global, JSProto_RegExp, &obj))
        {
            return false;
        }
    }

    global->zone()->usedByExclusiveThread = true;

    ScopedJSDeletePtr<ExclusiveContext> workercx(
        cx->new_<ExclusiveContext>(cx->runtime(), (PerThreadData *) NULL,
                                   ThreadSafeContext::Context_Exclusive));
    if (!workercx)
        return false;

    workercx->enterCompartment(global->compartment());

    ScopedJSDeletePtr<ParseTask> task(
        cx->new_<ParseTask>(cx->runtime(), workercx.get(), options, chars, length));
    if (!task)
        return false;

    workercx.forget();

    WorkerThreadState &state = *cx->runtime()->workerThreadState;
    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(rt);

    if (!state.parseWorklist.append(task.get()))
        return false;

    task.forget();

    state.notify(WorkerThreadState::WORKER);
    return true;
}

void
js::WaitForOffThreadParsingToFinish(JSRuntime *rt)
{
    if (!rt->workerThreadState)
        return;

    WorkerThreadState &state = *rt->workerThreadState;

    AutoLockWorkerThreadState lock(rt);

    while (true) {
        if (state.parseWorklist.empty()) {
            bool parseInProgress = false;
            for (size_t i = 0; i < state.numThreads; i++)
                parseInProgress |= !!state.threads[i].parseTask;
            if (!parseInProgress)
                break;
        }
        state.wait(WorkerThreadState::MAIN);
    }
}

bool
WorkerThreadState::init(JSRuntime *rt)
{
    if (!rt->useHelperThreads()) {
        numThreads = 0;
        return true;
    }

    workerLock = PR_NewLock();
    if (!workerLock)
        return false;

    mainWakeup = PR_NewCondVar(workerLock);
    if (!mainWakeup)
        return false;

    helperWakeup = PR_NewCondVar(workerLock);
    if (!helperWakeup)
        return false;

    numThreads = rt->helperThreadCount();

    threads = (WorkerThread*) rt->calloc_(sizeof(WorkerThread) * numThreads);
    if (!threads) {
        numThreads = 0;
        return false;
    }

    for (size_t i = 0; i < numThreads; i++) {
        WorkerThread &helper = threads[i];
        helper.runtime = rt;
        helper.threadData.construct(rt);
        helper.threadData.ref().addToThreadList();
        helper.thread = PR_CreateThread(PR_USER_THREAD,
                                        WorkerThread::ThreadMain, &helper,
                                        PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
        if (!helper.thread) {
            for (size_t j = 0; j < numThreads; j++)
                threads[j].destroy();
            js_delete(threads);
            threads = NULL;
            numThreads = 0;
            return false;
        }
    }

    resetAsmJSFailureState();
    return true;
}

WorkerThreadState::~WorkerThreadState()
{
    



    if (threads) {
        for (size_t i = 0; i < numThreads; i++)
            threads[i].destroy();
        js_delete(threads);
    }

    if (workerLock)
        PR_DestroyLock(workerLock);

    if (mainWakeup)
        PR_DestroyCondVar(mainWakeup);

    if (helperWakeup)
        PR_DestroyCondVar(helperWakeup);
}

void
WorkerThreadState::lock()
{
    JS_ASSERT(!isLocked());
    PR_Lock(workerLock);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
WorkerThreadState::unlock()
{
    JS_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = NULL;
#endif
    PR_Unlock(workerLock);
}

#ifdef DEBUG
bool
WorkerThreadState::isLocked()
{
    return lockOwner == PR_GetCurrentThread();
}
#endif

void
WorkerThreadState::wait(CondVar which, uint32_t millis)
{
    JS_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = NULL;
#endif
    DebugOnly<PRStatus> status =
        PR_WaitCondVar((which == MAIN) ? mainWakeup : helperWakeup,
                       millis ? PR_MillisecondsToInterval(millis) : PR_INTERVAL_NO_TIMEOUT);
    JS_ASSERT(status == PR_SUCCESS);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
WorkerThreadState::notify(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyCondVar((which == MAIN) ? mainWakeup : helperWakeup);
}

void
WorkerThreadState::notifyAll(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyAllCondVar((which == MAIN) ? mainWakeup : helperWakeup);
}

bool
WorkerThreadState::canStartAsmJSCompile()
{
    
    JS_ASSERT(isLocked());
    return (!asmJSWorklist.empty() && !numAsmJSFailedJobs);
}

bool
WorkerThreadState::canStartIonCompile()
{
    
    
    
    
    if (ionWorklist.empty())
        return false;
    for (size_t i = 0; i < numThreads; i++) {
        if (threads[i].ionBuilder)
            return false;
    }
    return true;
}

void
WorkerThread::destroy()
{
    WorkerThreadState &state = *runtime->workerThreadState;

    if (thread) {
        {
            AutoLockWorkerThreadState lock(runtime);
            terminate = true;

            
            state.notifyAll(WorkerThreadState::WORKER);
        }

        PR_JoinThread(thread);
    }

    if (!threadData.empty())
        threadData.ref().removeFromThreadList();
}


void
WorkerThread::ThreadMain(void *arg)
{
    PR_SetCurrentThreadName("Analysis Helper");
    static_cast<WorkerThread *>(arg)->threadLoop();
}

void
WorkerThread::handleAsmJSWorkload(WorkerThreadState &state)
{
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartAsmJSCompile());
    JS_ASSERT(idle());

    asmData = state.asmJSWorklist.popCopy();
    bool success = false;

    state.unlock();
    do {
        ion::IonContext icx(runtime, asmData->mir->compartment, &asmData->mir->temp());

        int64_t before = PRMJ_Now();

        if (!OptimizeMIR(asmData->mir))
            break;

        asmData->lir = GenerateLIR(asmData->mir);
        if (!asmData->lir)
            break;

        int64_t after = PRMJ_Now();
        asmData->compileTime = (after - before) / PRMJ_USEC_PER_MSEC;

        success = true;
    } while(0);
    state.lock();

    
    if (!success) {
        asmData = NULL;
        state.noteAsmJSFailure(asmData->func);
        state.notify(WorkerThreadState::MAIN);
        return;
    }

    
    state.asmJSFinishedList.append(asmData);
    asmData = NULL;

    
    state.notify(WorkerThreadState::MAIN);
}

void
WorkerThread::handleIonWorkload(WorkerThreadState &state)
{
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartIonCompile());
    JS_ASSERT(idle());

    ionBuilder = state.ionWorklist.popCopy();

    DebugOnly<ion::ExecutionMode> executionMode = ionBuilder->info().executionMode();
    JS_ASSERT(GetIonScript(ionBuilder->script(), executionMode) == ION_COMPILING_SCRIPT);

    state.unlock();
    {
        ion::IonContext ictx(runtime, ionBuilder->script()->compartment(), &ionBuilder->temp());
        ionBuilder->setBackgroundCodegen(ion::CompileBackEnd(ionBuilder));
    }
    state.lock();

    FinishOffThreadIonCompile(ionBuilder);
    ionBuilder = NULL;

    
    state.notify(WorkerThreadState::MAIN);

    
    
    runtime->triggerOperationCallback();
}

void
ExclusiveContext::setWorkerThread(WorkerThread *workerThread)
{
    this->workerThread = workerThread;
    this->perThreadData = workerThread->threadData.addr();
}

void
WorkerThread::handleParseWorkload(WorkerThreadState &state)
{
    JS_ASSERT(state.isLocked());
    JS_ASSERT(!state.parseWorklist.empty());
    JS_ASSERT(idle());

    parseTask = state.parseWorklist.popCopy();
    parseTask->cx->setWorkerThread(this);

    {
        AutoUnlockWorkerThreadState unlock(runtime);
        parseTask->script = frontend::CompileScript(parseTask->cx, &parseTask->alloc,
                                                    NullPtr(), NullPtr(),
                                                    parseTask->options,
                                                    parseTask->chars, parseTask->length);
    }

    state.parseFinishedList.append(parseTask);
    parseTask = NULL;

    
    state.notify(WorkerThreadState::MAIN);
}

void
WorkerThread::threadLoop()
{
    WorkerThreadState &state = *runtime->workerThreadState;
    AutoLockWorkerThreadState lock(runtime);

    js::TlsPerThreadData.set(threadData.addr());

    while (true) {
        JS_ASSERT(!ionBuilder && !asmData);

        
        while (!state.canStartIonCompile() &&
               !state.canStartAsmJSCompile() &&
               state.parseWorklist.empty())
        {
            if (state.shouldPause)
                pause();
            if (terminate)
                return;
            state.wait(WorkerThreadState::WORKER);
        }

        
        if (state.canStartAsmJSCompile())
            handleAsmJSWorkload(state);
        else if (state.canStartIonCompile())
            handleIonWorkload(state);
        else if (!state.parseWorklist.empty())
            handleParseWorkload(state);
    }
}

AutoPauseWorkersForGC::AutoPauseWorkersForGC(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : runtime(rt), needsUnpause(false)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;

    if (!runtime->workerThreadState)
        return;

    JS_ASSERT(CurrentThreadCanAccessRuntime(runtime));

    WorkerThreadState &state = *runtime->workerThreadState;
    if (!state.numThreads)
        return;

    AutoLockWorkerThreadState lock(runtime);

    
    if (state.shouldPause) {
        JS_ASSERT(state.numPaused == state.numThreads);
        return;
    }

    needsUnpause = true;

    state.shouldPause = 1;

    while (state.numPaused != state.numThreads) {
        state.notifyAll(WorkerThreadState::WORKER);
        state.wait(WorkerThreadState::MAIN);
    }
}

AutoPauseWorkersForGC::~AutoPauseWorkersForGC()
{
    if (!needsUnpause)
        return;

    WorkerThreadState &state = *runtime->workerThreadState;
    AutoLockWorkerThreadState lock(runtime);

    state.shouldPause = 0;

    
    state.notifyAll(WorkerThreadState::WORKER);
}

void
WorkerThread::pause()
{
    WorkerThreadState &state = *runtime->workerThreadState;
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.shouldPause);

    JS_ASSERT(state.numPaused < state.numThreads);
    state.numPaused++;

    
    if (state.numPaused == state.numThreads)
        state.notify(WorkerThreadState::MAIN);

    while (state.shouldPause)
        state.wait(WorkerThreadState::WORKER);

    state.numPaused--;
}

#else 

bool
js::StartOffThreadAsmJSCompile(JSContext *cx, AsmJSParallelTask *asmData)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

bool
js::StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

void
js::CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script)
{
}

bool
js::StartOffThreadParseScript(JSContext *cx, const CompileOptions &options,
                              const jschar *chars, size_t length)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

void
js::WaitForOffThreadParsingToFinish(JSRuntime *rt)
{
}

AutoPauseWorkersForGC::AutoPauseWorkersForGC(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

AutoPauseWorkersForGC::~AutoPauseWorkersForGC()
{
}

#endif 
