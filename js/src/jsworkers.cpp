





#include "jsworkers.h"

#ifdef JS_THREADSAFE

#include "mozilla/DebugOnly.h"

#include "jsnativestack.h"
#include "prmjtime.h"

#include "frontend/BytecodeCompiler.h"
#include "jit/ExecutionModeInlines.h"
#include "jit/IonBuilder.h"
#include "vm/Debugger.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;

using mozilla::ArrayLength;
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

    rt->workerThreadState = rt->new_<WorkerThreadState>(rt);
    if (!rt->workerThreadState)
        return false;

    if (!rt->workerThreadState->init()) {
        js_delete(rt->workerThreadState);
        rt->workerThreadState = nullptr;
        return false;
    }

    return true;
}

#ifdef JS_ION

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

    state.notifyOne(WorkerThreadState::PRODUCER);
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

    cx->runtime()->addCompilationThread();

    state.notifyAll(WorkerThreadState::PRODUCER);
    return true;
}






static void
FinishOffThreadIonCompile(jit::IonBuilder *builder)
{
    JSCompartment *compartment = builder->script()->compartment();
    JS_ASSERT(compartment->runtimeFromAnyThread()->workerThreadState);
    JS_ASSERT(compartment->runtimeFromAnyThread()->workerThreadState->isLocked());

    compartment->jitCompartment()->finishedOffThreadCompilations().append(builder);
}

#endif 

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
#ifdef JS_ION
    JSRuntime *rt = compartment->runtimeFromMainThread();

    if (!rt->workerThreadState)
        return;

    WorkerThreadState &state = *rt->workerThreadState;

    jit::JitCompartment *jitComp = compartment->jitCompartment();
    if (!jitComp)
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

    jit::OffThreadCompilationVector &compilations = jitComp->finishedOffThreadCompilations();

    
    for (size_t i = 0; i < compilations.length(); i++) {
        jit::IonBuilder *builder = compilations[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            jit::FinishOffThreadBuilder(builder);
            compilations[i--] = compilations.back();
            compilations.popBack();
        }
    }
#endif 
}

static const JSClass workerGlobalClass = {
    "internal-worker-global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_DeletePropertyStub,
    JS_PropertyStub,  JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   nullptr
};

ParseTask::ParseTask(ExclusiveContext *cx, JSObject *exclusiveContextGlobal, JSContext *initCx,
                     const jschar *chars, size_t length, JSObject *scopeChain,
                     JS::OffThreadCompileCallback callback, void *callbackData)
  : cx(cx), options(initCx), chars(chars), length(length),
    alloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE), scopeChain(scopeChain),
    exclusiveContextGlobal(exclusiveContextGlobal), callback(callback),
    callbackData(callbackData), script(nullptr), errors(cx), overRecursed(false)
{
    JSRuntime *rt = scopeChain->runtimeFromMainThread();

    if (!AddObjectRoot(rt, &this->scopeChain, "ParseTask::scopeChain"))
        MOZ_CRASH();
    if (!AddObjectRoot(rt, &this->exclusiveContextGlobal, "ParseTask::exclusiveContextGlobal"))
        MOZ_CRASH();
}

bool
ParseTask::init(JSContext *cx, const ReadOnlyCompileOptions &options)
{
    return this->options.copy(cx, options);
}

void
ParseTask::activate(JSRuntime *rt)
{
    rt->setUsedByExclusiveThread(exclusiveContextGlobal->zone());
    cx->enterCompartment(exclusiveContextGlobal->compartment());
}

ParseTask::~ParseTask()
{
    JSRuntime *rt = scopeChain->runtimeFromMainThread();

    JS_RemoveObjectRootRT(rt, &scopeChain);
    JS_RemoveObjectRootRT(rt, &exclusiveContextGlobal);

    
    js_delete(cx);

    for (size_t i = 0; i < errors.length(); i++)
        js_delete(errors[i]);
}

bool
js::StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
                              const jschar *chars, size_t length, HandleObject scopeChain,
                              JS::OffThreadCompileCallback callback, void *callbackData)
{
    
    
    gc::AutoSuppressGC suppress(cx);

    frontend::MaybeCallSourceHandler(cx, options, chars, length);

    if (!EnsureWorkerThreadsInitialized(cx))
        return false;

    JS::CompartmentOptions compartmentOptions(cx->compartment()->options());
    compartmentOptions.setZone(JS::FreshZone);
    compartmentOptions.setInvisibleToDebugger(true);
    compartmentOptions.setMergeable(true);

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

    ScopedJSDeletePtr<ExclusiveContext> workercx(
        cx->new_<ExclusiveContext>(cx->runtime(), (PerThreadData *) nullptr,
                                   ThreadSafeContext::Context_Exclusive));
    if (!workercx)
        return false;

    ScopedJSDeletePtr<ParseTask> task(
        cx->new_<ParseTask>(workercx.get(), global, cx, chars, length,
                            scopeChain, callback, callbackData));
    if (!task)
        return false;

    workercx.forget();

    if (!task->init(cx, options))
        return false;

    WorkerThreadState &state = *cx->runtime()->workerThreadState;
    JS_ASSERT(state.numThreads);

    
    
    
    
    
    
    if (cx->runtime()->activeGCInAtomsZone()) {
        if (!state.parseWaitingOnGC.append(task.get()))
            return false;
    } else {
        task->activate(cx->runtime());

        AutoLockWorkerThreadState lock(state);

        if (!state.parseWorklist.append(task.get()))
            return false;

        state.notifyAll(WorkerThreadState::PRODUCER);
    }

    task.forget();

    return true;
}

void
js::EnqueuePendingParseTasksAfterGC(JSRuntime *rt)
{
    JS_ASSERT(!rt->activeGCInAtomsZone());

    if (!rt->workerThreadState || rt->workerThreadState->parseWaitingOnGC.empty())
        return;

    
    

    WorkerThreadState &state = *rt->workerThreadState;

    for (size_t i = 0; i < state.parseWaitingOnGC.length(); i++)
        state.parseWaitingOnGC[i]->activate(rt);

    AutoLockWorkerThreadState lock(state);

    JS_ASSERT(state.parseWorklist.empty());
    state.parseWorklist.swap(state.parseWaitingOnGC);

    state.notifyAll(WorkerThreadState::PRODUCER);
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

#ifdef XP_WIN



static const uint32_t WORKER_STACK_SIZE = 0;
#else
static const uint32_t WORKER_STACK_SIZE = 512 * 1024;
#endif

static const uint32_t WORKER_STACK_QUOTA = 450 * 1024;

bool
WorkerThreadState::init()
{
    JS_ASSERT(numThreads == 0);

    if (!runtime->useHelperThreads())
        return true;

    workerLock = PR_NewLock();
    if (!workerLock)
        return false;

    consumerWakeup = PR_NewCondVar(workerLock);
    if (!consumerWakeup)
        return false;

    producerWakeup = PR_NewCondVar(workerLock);
    if (!producerWakeup)
        return false;

    threads = (WorkerThread*) js_pod_calloc<WorkerThread>(runtime->workerThreadCount());
    if (!threads)
        return false;

    for (size_t i = 0; i < runtime->workerThreadCount(); i++) {
        WorkerThread &helper = threads[i];
        helper.runtime = runtime;
        helper.threadData.construct(runtime);
        helper.threadData.ref().addToThreadList();
        helper.thread = PR_CreateThread(PR_USER_THREAD,
                                        WorkerThread::ThreadMain, &helper,
                                        PR_PRIORITY_NORMAL, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, WORKER_STACK_SIZE);
        if (!helper.thread || !helper.threadData.ref().init()) {
            for (size_t j = 0; j < runtime->workerThreadCount(); j++)
                threads[j].destroy();
            js_free(threads);
            threads = nullptr;
            return false;
        }
    }

    numThreads = runtime->workerThreadCount();
    resetAsmJSFailureState();
    return true;
}

void
WorkerThreadState::cleanup()
{
    
    

    
    if (threads) {
        for (size_t i = 0; i < numThreads; i++)
            threads[i].destroy();
        js_free(threads);
        threads = nullptr;
        numThreads = 0;
    }

    
    while (!parseFinishedList.empty())
        finishParseTask( nullptr, runtime, parseFinishedList[0]);
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
    runtime->assertCanLock(JSRuntime::WorkerThreadStateLock);
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

void
WorkerThreadState::notifyOne(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup);
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

    
    RootedFunction function(cx, script->functionNonDelazifying());
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
        TaggedProto proto(object->proto());
        if (!proto.isObject())
            continue;

        JSProtoKey key = js_IdentifyClassPrototype(proto.toObject());
        if (key == JSProto_Null)
            continue;

        JSObject *newProto = GetClassPrototypePure(&parseTask->scopeChain->global(), key);
        JS_ASSERT(newProto);

        
        
        object->setProtoUnchecked(newProto);
    }

    
    gc::MergeCompartments(parseTask->cx->compartment(), parseTask->scopeChain->compartment());

    RootedScript script(rt, parseTask->script);

    
    
    if (maybecx) {
        AutoCompartment ac(maybecx, parseTask->scopeChain);
        for (size_t i = 0; i < parseTask->errors.length(); i++)
            parseTask->errors[i]->throwError(maybecx);
        if (parseTask->overRecursed)
            js_ReportOverRecursed(maybecx);

        if (script) {
            
            GlobalObject *compileAndGoGlobal = nullptr;
            if (script->compileAndGo())
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
#ifdef JS_ION
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartAsmJSCompile());
    JS_ASSERT(idle());

    asmData = state.asmJSWorklist.popCopy();
    bool success = false;

    state.unlock();
    do {
        jit::IonContext icx(jit::CompileRuntime::get(runtime), asmData->mir->compartment, &asmData->mir->alloc());

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
        state.noteAsmJSFailure(asmData->func);
        state.notifyAll(WorkerThreadState::CONSUMER);
        asmData = nullptr;
        return;
    }

    
    state.asmJSFinishedList.append(asmData);
    asmData = nullptr;

    
    state.notifyAll(WorkerThreadState::CONSUMER);
#else
    MOZ_CRASH();
#endif 
}

void
WorkerThread::handleIonWorkload(WorkerThreadState &state)
{
#ifdef JS_ION
    JS_ASSERT(state.isLocked());
    JS_ASSERT(state.canStartIonCompile());
    JS_ASSERT(idle());

    ionBuilder = state.ionWorklist.popCopy();

    DebugOnly<ExecutionMode> executionMode = ionBuilder->info().executionMode();

#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::getLogger(TraceLogging::ION_BACKGROUND_COMPILER),
                        TraceLogging::ION_COMPILE_START,
                        TraceLogging::ION_COMPILE_STOP,
                        ionBuilder->script());
#endif

    state.unlock();
    {
        jit::IonContext ictx(jit::CompileRuntime::get(runtime),
                             jit::CompileCompartment::get(ionBuilder->script()->compartment()),
                             &ionBuilder->alloc());
        AutoEnterIonCompilation ionCompiling;
        bool succeeded = ionBuilder->build();
        ionBuilder->clearForBackEnd();
        if (succeeded)
            ionBuilder->setBackgroundCodegen(jit::CompileBackEnd(ionBuilder));
    }
    state.lock();

    FinishOffThreadIonCompile(ionBuilder);
    ionBuilder = nullptr;

    
    
    
    
    runtime->triggerOperationCallback(JSRuntime::TriggerCallbackAnyThreadDontStopIon);

    
    state.notifyAll(WorkerThreadState::CONSUMER);
#else
    MOZ_CRASH();
#endif 
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
ExclusiveContext::addPendingOverRecursed()
{
    if (workerThread()->parseTask)
        workerThread()->parseTask->overRecursed = true;
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
        if (!compressionTask->work())
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

        while (state.compressionInProgress(this))
            state.wait(WorkerThreadState::CONSUMER);

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
    JS::AutoAssertNoGC nogc;
    WorkerThreadState &state = *runtime->workerThreadState;
    AutoLockWorkerThreadState lock(state);

    js::TlsPerThreadData.set(threadData.addr());

    
    uintptr_t stackLimit = GetNativeStackBase();
#if JS_STACK_GROWTH_DIRECTION > 0
    stackLimit += WORKER_STACK_QUOTA;
#else
    stackLimit -= WORKER_STACK_QUOTA;
#endif
    for (size_t i = 0; i < ArrayLength(threadData.ref().nativeStackLimit); i++)
        threadData.ref().nativeStackLimit[i] = stackLimit;

    while (true) {
        JS_ASSERT(!ionBuilder && !asmData);

        
        while (true) {
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

#else 

using namespace js;

#ifdef JS_ION

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

#endif 

void
js::CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script)
{
}

bool
js::StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
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

frontend::CompileError &
ExclusiveContext::addPendingCompileError()
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available.");
}

void
ExclusiveContext::addPendingOverRecursed()
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available.");
}

#endif 
