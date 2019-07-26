





#include "jsworkers.h"

#ifdef JS_WORKER_THREADS
#include "mozilla/DebugOnly.h"

#include "prmjtime.h"

#include "frontend/BytecodeCompiler.h"
#include "jit/ExecutionModeInlines.h"
#include "jit/IonBuilder.h"
#include "vm/Debugger.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"
#include "jsobjinlines.h"

using namespace js;

using mozilla::DebugOnly;

bool
js::EnsureWorkerThreadsInitialized(ExclusiveContext *cx)
{
    
    
    if (!cx->isJSContext()) {
        JS_ASSERT(cx->workerThreadState() != nullptr);
        return true;
    }

    JSRuntime *rt = cx->asJSContext()->runtime();
    if (rt->workerThreadState)
        return true;

    rt->workerThreadState = rt->new_<WorkerThreadState>();
    if (!rt->workerThreadState)
        return false;

    if (!rt->workerThreadState->init(rt)) {
        js_delete(rt->workerThreadState);
        rt->workerThreadState = nullptr;
        return false;
    }

    return true;
}

bool
js::StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData)
{
    
    JS_ASSERT(cx->workerThreadState() != nullptr);
    JS_ASSERT(asmData->mir);
    JS_ASSERT(asmData->lir == nullptr);

    WorkerThreadState &state = *cx->workerThreadState();
    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(state);

    
    if (state.asmJSWorkerFailed())
        return false;

    if (!state.asmJSWorklist.append(asmData))
        return false;

    state.notifyAll(WorkerThreadState::PRODUCER);
    return true;
}

bool
js::StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder)
{
    if (!EnsureWorkerThreadsInitialized(cx))
        return false;

    WorkerThreadState &state = *cx->runtime()->workerThreadState;
    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(state);

    if (!state.ionWorklist.append(builder))
        return false;

    state.notifyAll(WorkerThreadState::PRODUCER);
    return true;
}






static void
FinishOffThreadIonCompile(jit::IonBuilder *builder)
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

    jit::IonCompartment *ion = compartment->ionCompartment();
    if (!ion)
        return;

    AutoLockWorkerThreadState lock(state);

    
    for (size_t i = 0; i < state.ionWorklist.length(); i++) {
        jit::IonBuilder *builder = state.ionWorklist[i];
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
            state.wait(WorkerThreadState::CONSUMER);
        }
    }

    jit::OffThreadCompilationVector &compilations = ion->finishedOffThreadCompilations();

    
    for (size_t i = 0; i < compilations.length(); i++) {
        jit::IonBuilder *builder = compilations[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            jit::FinishOffThreadBuilder(builder);
            compilations[i--] = compilations.back();
            compilations.popBack();
        }
    }
}

static const JSClass workerGlobalClass = {
    "internal-worker-global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_DeletePropertyStub,
    JS_PropertyStub,  JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   nullptr
};

ParseTask::ParseTask(ExclusiveContext *cx, const CompileOptions &options,
                     const jschar *chars, size_t length, JSObject *scopeChain,
                     JS::OffThreadCompileCallback callback, void *callbackData)
  : cx(cx), options(options), chars(chars), length(length),
    alloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE), scopeChain(scopeChain),
    callback(callback), callbackData(callbackData), script(nullptr), errors(cx)
{
    JSRuntime *rt = scopeChain->runtimeFromMainThread();

    if (options.principals())
        JS_HoldPrincipals(options.principals());
    if (options.originPrincipals())
        JS_HoldPrincipals(options.originPrincipals());
    if (!AddObjectRoot(rt, &this->scopeChain, "ParseTask::scopeChain"))
        MOZ_CRASH();
}

ParseTask::~ParseTask()
{
    JSRuntime *rt = scopeChain->runtimeFromMainThread();

    if (options.principals())
        JS_DropPrincipals(rt, options.principals());
    if (options.originPrincipals())
        JS_DropPrincipals(rt, options.originPrincipals());
    JS_RemoveObjectRootRT(rt, &scopeChain);

    
    js_delete(cx);

    for (size_t i = 0; i < errors.length(); i++)
        js_delete(errors[i]);
}

bool
js::StartOffThreadParseScript(JSContext *cx, const CompileOptions &options,
                              const jschar *chars, size_t length, HandleObject scopeChain,
                              JS::OffThreadCompileCallback callback, void *callbackData)
{
    
    
    gc::AutoSuppressGC suppress(cx);

    frontend::MaybeCallSourceHandler(cx, options, chars, length);

    if (!EnsureWorkerThreadsInitialized(cx))
        return false;

    JS::CompartmentOptions compartmentOptions(cx->compartment()->options());
    compartmentOptions.setZone(JS::FreshZone);

    JSObject *global = JS_NewGlobalObject(cx, &workerGlobalClass, nullptr,
                                          JS::FireOnNewGlobalHook, compartmentOptions);
    if (!global)
        return false;

    global->zone()->types.inferenceEnabled = cx->typeInferenceEnabled();
    JS_SetCompartmentPrincipals(global->compartment(), cx->compartment()->principals);

    RootedObject obj(cx);

    
    
    
    if (!js_GetClassObject(cx, cx->global(), JSProto_Function, &obj) ||
        !js_GetClassObject(cx, cx->global(), JSProto_Array, &obj) ||
        !js_GetClassObject(cx, cx->global(), JSProto_RegExp, &obj) ||
        !js_GetClassObject(cx, cx->global(), JSProto_GeneratorFunction, &obj))
    {
        return false;
    }
    {
        AutoCompartment ac(cx, global);
        if (!js_GetClassObject(cx, global, JSProto_Function, &obj) ||
            !js_GetClassObject(cx, global, JSProto_Array, &obj) ||
            !js_GetClassObject(cx, global, JSProto_RegExp, &obj) ||
            !js_GetClassObject(cx, global, JSProto_GeneratorFunction, &obj))
        {
            return false;
        }
    }

    cx->runtime()->setUsedByExclusiveThread(global->zone());

    ScopedJSDeletePtr<ExclusiveContext> workercx(
        cx->new_<ExclusiveContext>(cx->runtime(), (PerThreadData *) nullptr,
                                   ThreadSafeContext::Context_Exclusive));
    if (!workercx)
        return false;

    workercx->enterCompartment(global->compartment());

    ScopedJSDeletePtr<ParseTask> task(
        cx->new_<ParseTask>(workercx.get(), options, chars, length,
                            scopeChain, callback, callbackData));
    if (!task)
        return false;

    workercx.forget();

    WorkerThreadState &state = *cx->runtime()->workerThreadState;
    JS_ASSERT(state.numThreads);

    AutoLockWorkerThreadState lock(state);

    if (!state.parseWorklist.append(task.get()))
        return false;

    task.forget();

    state.notifyAll(WorkerThreadState::PRODUCER);
    return true;
}

void
js::WaitForOffThreadParsingToFinish(JSRuntime *rt)
{
    if (!rt->workerThreadState)
        return;

    WorkerThreadState &state = *rt->workerThreadState;

    AutoLockWorkerThreadState lock(state);

    while (true) {
        if (state.parseWorklist.empty()) {
            bool parseInProgress = false;
            for (size_t i = 0; i < state.numThreads; i++)
                parseInProgress |= !!state.threads[i].parseTask;
            if (!parseInProgress)
                break;
        }
        state.wait(WorkerThreadState::CONSUMER);
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

    consumerWakeup = PR_NewCondVar(workerLock);
    if (!consumerWakeup)
        return false;

    producerWakeup = PR_NewCondVar(workerLock);
    if (!producerWakeup)
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
        if (!helper.thread || !helper.threadData.ref().init()) {
            for (size_t j = 0; j < numThreads; j++)
                threads[j].destroy();
            js_free(threads);
            threads = nullptr;
            numThreads = 0;
            return false;
        }
    }

    resetAsmJSFailureState();
    return true;
}

void
WorkerThreadState::cleanup(JSRuntime *rt)
{
    
    

    
    if (threads) {
        for (size_t i = 0; i < numThreads; i++)
            threads[i].destroy();
        js_free(threads);
        threads = nullptr;
        numThreads = 0;
    }

    
    while (!parseFinishedList.empty())
        finishParseTask( nullptr, rt, parseFinishedList[0]);
}

WorkerThreadState::~WorkerThreadState()
{
    JS_ASSERT(!threads);
    JS_ASSERT(parseFinishedList.empty());

    if (workerLock)
        PR_DestroyLock(workerLock);

    if (consumerWakeup)
        PR_DestroyCondVar(consumerWakeup);

    if (producerWakeup)
        PR_DestroyCondVar(producerWakeup);
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
    lockOwner = nullptr;
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
    lockOwner = nullptr;
#endif
    DebugOnly<PRStatus> status =
        PR_WaitCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup,
                       millis ? PR_MillisecondsToInterval(millis) : PR_INTERVAL_NO_TIMEOUT);
    JS_ASSERT(status == PR_SUCCESS);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
WorkerThreadState::notifyAll(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyAllCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup);
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

bool
WorkerThreadState::canStartParseTask()
{
    
    
    
    
    JS_ASSERT(isLocked());
    if (parseWorklist.empty())
        return false;
    for (size_t i = 0; i < numThreads; i++) {
        if (threads[i].parseTask)
            return false;
    }
    return true;
}

bool
WorkerThreadState::canStartCompressionTask()
{
    return !compressionWorklist.empty();
}

static void
CallNewScriptHookForAllScripts(JSContext *cx, HandleScript script)
{
    
    
    JS_CHECK_RECURSION(cx, return);

    
    if (script->hasObjects()) {
        ObjectArray *objects = script->objects();
        for (size_t i = 0; i < objects->length; i++) {
            JSObject *obj = objects->vector[i];
            if (obj->is<JSFunction>()) {
                JSFunction *fun = &obj->as<JSFunction>();
                if (fun->hasScript()) {
                    RootedScript nested(cx, fun->nonLazyScript());
                    CallNewScriptHookForAllScripts(cx, nested);
                }
            }
        }
    }

    
    RootedFunction function(cx, script->function());
    CallNewScriptHook(cx, script, function);
}

JSScript *
WorkerThreadState::finishParseTask(JSContext *maybecx, JSRuntime *rt, void *token)
{
    ParseTask *parseTask = nullptr;

    
    
    {
        AutoLockWorkerThreadState lock(*rt->workerThreadState);
        for (size_t i = 0; i < parseFinishedList.length(); i++) {
            if (parseFinishedList[i] == token) {
                parseTask = parseFinishedList[i];
                parseFinishedList[i] = parseFinishedList.back();
                parseFinishedList.popBack();
                break;
            }
        }
    }
    JS_ASSERT(parseTask);

    
    
    rt->clearUsedByExclusiveThread(parseTask->cx->zone());

    
    
    
    
    for (gc::CellIter iter(parseTask->cx->zone(), gc::FINALIZE_TYPE_OBJECT);
         !iter.done();
         iter.next())
    {
        types::TypeObject *object = iter.get<types::TypeObject>();
        TaggedProto proto(object->proto);
        if (!proto.isObject())
            continue;

        JSProtoKey key = js_IdentifyClassPrototype(proto.toObject());
        if (key == JSProto_Null)
            continue;

        JSObject *newProto = GetClassPrototypePure(&parseTask->scopeChain->global(), key);
        JS_ASSERT(newProto);

        object->proto = newProto;
    }

    
    gc::MergeCompartments(parseTask->cx->compartment(), parseTask->scopeChain->compartment());

    RootedScript script(rt, parseTask->script);

    
    
    if (maybecx) {
        AutoCompartment ac(maybecx, parseTask->scopeChain);
        for (size_t i = 0; i < parseTask->errors.length(); i++)
            parseTask->errors[i]->throwError(maybecx);

        if (script) {
            
            GlobalObject *compileAndGoGlobal = nullptr;
            if (script->compileAndGo)
                compileAndGoGlobal = &script->global();
            Debugger::onNewScript(maybecx, script, compileAndGoGlobal);

            
            CallNewScriptHookForAllScripts(maybecx, script);
        }
    }

    js_delete(parseTask);
    return script;
}

void
WorkerThread::destroy()
{
    WorkerThreadState &state = *runtime->workerThreadState;

    if (thread) {
        {
            AutoLockWorkerThreadState lock(state);
            terminate = true;

            
            state.notifyAll(WorkerThreadState::PRODUCER);
        }

        PR_JoinThread(thread);
    }

    if (!threadData.empty()) {
        threadData.ref().removeFromThreadList();
        threadData.destroy();
    }
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
        jit::IonContext icx(runtime, asmData->mir->compartment, &asmData->mir->temp());

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
        asmData = nullptr;
        state.noteAsmJSFailure(asmData->func);
        state.notifyAll(WorkerThreadState::CONSUMER);
        return;
    }

    
    state.asmJSFinishedList.append(asmData);
    asmData = nullptr;

    
    state.notifyAll(WorkerThreadState::CONSUMER);
}

void
WorkerThread::handleIonWorkload(WorkerThreadState &state)
{
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartIonCompile());
    JS_ASSERT(idle());

    ionBuilder = state.ionWorklist.popCopy();

    DebugOnly<ExecutionMode> executionMode = ionBuilder->info().executionMode();
    JS_ASSERT(jit::GetIonScript(ionBuilder->script(), executionMode) == ION_COMPILING_SCRIPT);

#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::getLogger(TraceLogging::ION_BACKGROUND_COMPILER),
                        TraceLogging::ION_COMPILE_START,
                        TraceLogging::ION_COMPILE_STOP,
                        ionBuilder->script());
#endif

    state.unlock();
    {
        jit::IonContext ictx(runtime, ionBuilder->script()->compartment(), &ionBuilder->temp());
        ionBuilder->setBackgroundCodegen(jit::CompileBackEnd(ionBuilder));
    }
    state.lock();

    FinishOffThreadIonCompile(ionBuilder);
    ionBuilder = nullptr;

    
    state.notifyAll(WorkerThreadState::CONSUMER);

    
    
    
    
    runtime->triggerOperationCallback(JSRuntime::TriggerCallbackAnyThreadDontStopIon);
}

void
ExclusiveContext::setWorkerThread(WorkerThread *workerThread)
{
    workerThread_ = workerThread;
    perThreadData = workerThread->threadData.addr();
}

frontend::CompileError &
ExclusiveContext::addPendingCompileError()
{
    frontend::CompileError *error = js_new<frontend::CompileError>();
    if (!error)
        MOZ_CRASH();
    if (!workerThread()->parseTask->errors.append(error))
        MOZ_CRASH();
    return *error;
}

void
WorkerThread::handleParseWorkload(WorkerThreadState &state)
{
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartParseTask());
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

    
    parseTask->callback(parseTask, parseTask->callbackData);

    
    
    state.parseFinishedList.append(parseTask);

    parseTask = nullptr;

    
    state.notifyAll(WorkerThreadState::CONSUMER);
}

void
WorkerThread::handleCompressionWorkload(WorkerThreadState &state)
{
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartCompressionTask());
    JS_ASSERT(idle());

    compressionTask = state.compressionWorklist.popCopy();
    compressionTask->workerThread = this;

    {
        AutoUnlockWorkerThreadState unlock(runtime);
        if (!compressionTask->compress())
            compressionTask->setOOM();
    }

    compressionTask->workerThread = nullptr;
    compressionTask = nullptr;

    
    state.notifyAll(WorkerThreadState::CONSUMER);
}

bool
js::StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task)
{
    if (!EnsureWorkerThreadsInitialized(cx))
        return false;

    WorkerThreadState &state = *cx->workerThreadState();
    AutoLockWorkerThreadState lock(state);

    if (!state.compressionWorklist.append(task))
        return false;

    state.notifyAll(WorkerThreadState::PRODUCER);
    return true;
}

bool
WorkerThreadState::compressionInProgress(SourceCompressionTask *task)
{
    JS_ASSERT(isLocked());
    for (size_t i = 0; i < compressionWorklist.length(); i++) {
        if (compressionWorklist[i] == task)
            return true;
    }
    for (size_t i = 0; i < numThreads; i++) {
        if (threads[i].compressionTask == task)
            return true;
    }
    return false;
}

bool
SourceCompressionTask::complete()
{
    JS_ASSERT_IF(!ss, !chars);
    if (active()) {
        WorkerThreadState &state = *cx->workerThreadState();
        AutoLockWorkerThreadState lock(state);

        {
            AutoPauseCurrentWorkerThread maybePause(cx);
            while (state.compressionInProgress(this))
                state.wait(WorkerThreadState::CONSUMER);
        }

        ss->ready_ = true;

        
        if (!oom)
            cx->updateMallocCounter(ss->computedSizeOfData());

        ss = nullptr;
        chars = nullptr;
    }
    if (oom) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

SourceCompressionTask *
WorkerThreadState::compressionTaskForSource(ScriptSource *ss)
{
    JS_ASSERT(isLocked());
    for (size_t i = 0; i < compressionWorklist.length(); i++) {
        SourceCompressionTask *task = compressionWorklist[i];
        if (task->source() == ss)
            return task;
    }
    for (size_t i = 0; i < numThreads; i++) {
        SourceCompressionTask *task = threads[i].compressionTask;
        if (task && task->source() == ss)
            return task;
    }
    return nullptr;
}

const jschar *
ScriptSource::getOffThreadCompressionChars(ExclusiveContext *cx)
{
    

    if (ready()) {
        
        return nullptr;
    }

    WorkerThreadState &state = *cx->workerThreadState();
    AutoLockWorkerThreadState lock(state);

    
    
    if (SourceCompressionTask *task = state.compressionTaskForSource(this))
        return task->uncompressedChars();

    
    
    ready_ = true;

    return nullptr;
}

void
WorkerThread::threadLoop()
{
    WorkerThreadState &state = *runtime->workerThreadState;
    AutoLockWorkerThreadState lock(state);

    js::TlsPerThreadData.set(threadData.addr());

    while (true) {
        JS_ASSERT(!ionBuilder && !asmData);

        
        while (true) {
            if (state.shouldPause)
                pause();
            if (terminate)
                return;
            if (state.canStartIonCompile() ||
                state.canStartAsmJSCompile() ||
                state.canStartParseTask() ||
                state.canStartCompressionTask())
            {
                break;
            }
            state.wait(WorkerThreadState::PRODUCER);
        }

        
        if (state.canStartAsmJSCompile())
            handleAsmJSWorkload(state);
        else if (state.canStartIonCompile())
            handleIonWorkload(state);
        else if (state.canStartParseTask())
            handleParseWorkload(state);
        else if (state.canStartCompressionTask())
            handleCompressionWorkload(state);
        else
            MOZ_ASSUME_UNREACHABLE("No task to perform");
    }
}

AutoPauseWorkersForTracing::AutoPauseWorkersForTracing(JSRuntime *rt
                                                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : runtime(rt), needsUnpause(false), oldExclusiveThreadsPaused(rt->exclusiveThreadsPaused)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;

    rt->exclusiveThreadsPaused = true;

    if (!runtime->workerThreadState)
        return;

    JS_ASSERT(CurrentThreadCanAccessRuntime(runtime));

    WorkerThreadState &state = *runtime->workerThreadState;
    if (!state.numThreads)
        return;

    AutoLockWorkerThreadState lock(state);

    
    if (state.shouldPause) {
        JS_ASSERT(state.numPaused == state.numThreads);
        return;
    }

    needsUnpause = true;

    state.shouldPause = 1;

    while (state.numPaused != state.numThreads) {
        state.notifyAll(WorkerThreadState::PRODUCER);
        state.wait(WorkerThreadState::CONSUMER);
    }
}

AutoPauseWorkersForTracing::~AutoPauseWorkersForTracing()
{
    runtime->exclusiveThreadsPaused = oldExclusiveThreadsPaused;

    if (!needsUnpause)
        return;

    WorkerThreadState &state = *runtime->workerThreadState;
    AutoLockWorkerThreadState lock(state);

    state.shouldPause = 0;

    
    state.notifyAll(WorkerThreadState::PRODUCER);
}

AutoPauseCurrentWorkerThread::AutoPauseCurrentWorkerThread(ExclusiveContext *cx
                                                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : cx(cx)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;

    
    
    
    
    if (cx->workerThread()) {
        WorkerThreadState &state = *cx->workerThreadState();
        JS_ASSERT(state.isLocked());

        state.numPaused++;
        if (state.numPaused == state.numThreads)
            state.notifyAll(WorkerThreadState::CONSUMER);
    }
}

AutoPauseCurrentWorkerThread::~AutoPauseCurrentWorkerThread()
{
    if (cx->workerThread()) {
        WorkerThreadState &state = *cx->workerThreadState();
        JS_ASSERT(state.isLocked());

        state.numPaused--;

        
        
        if (state.shouldPause)
            cx->workerThread()->pause();
    }
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
        state.notifyAll(WorkerThreadState::CONSUMER);

    while (state.shouldPause)
        state.wait(WorkerThreadState::PRODUCER);

    state.numPaused--;
}

#else 

using namespace js;

bool
js::StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

bool
js::StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

void
js::CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script)
{
}

bool
js::StartOffThreadParseScript(JSContext *cx, const CompileOptions &options,
                              const jschar *chars, size_t length, HandleObject scopeChain,
                              JS::OffThreadCompileCallback callback, void *callbackData)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

void
js::WaitForOffThreadParsingToFinish(JSRuntime *rt)
{
}

bool
js::StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compression not available");
}

bool
SourceCompressionTask::complete()
{
    JS_ASSERT(!active() && !oom);
    return true;
}

const jschar *
ScriptSource::getOffThreadCompressionChars(ExclusiveContext *cx)
{
    JS_ASSERT(ready());
    return nullptr;
}

AutoPauseWorkersForTracing::AutoPauseWorkersForTracing(JSRuntime *rt
                                                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

AutoPauseWorkersForTracing::~AutoPauseWorkersForTracing()
{
}

AutoPauseCurrentWorkerThread::AutoPauseCurrentWorkerThread(ExclusiveContext *cx
                                                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
}

AutoPauseCurrentWorkerThread::~AutoPauseCurrentWorkerThread()
{
}

frontend::CompileError &
ExclusiveContext::addPendingCompileError()
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available.");
}

#endif 
