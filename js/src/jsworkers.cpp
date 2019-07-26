





#include "jsworkers.h"

#ifdef JS_THREADSAFE

#include "mozilla/DebugOnly.h"

#include "jsnativestack.h"
#include "prmjtime.h"

#include "frontend/BytecodeCompiler.h"
#include "jit/IonBuilder.h"
#include "vm/Debugger.h"
#include "vm/TraceLogging.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;

using mozilla::ArrayLength;
using mozilla::DebugOnly;

namespace js {

GlobalWorkerThreadState gWorkerThreadState;

} 

void
js::EnsureWorkerThreadsInitialized(ExclusiveContext *cx)
{
    
    
    if (!cx->isJSContext())
        return;

    WorkerThreadState().ensureInitialized();
}

static size_t
ThreadCountForCPUCount(size_t cpuCount)
{
    return Max(cpuCount, (size_t)2);
}

void
js::SetFakeCPUCount(size_t count)
{
    
    JS_ASSERT(!WorkerThreadState().threads);

    WorkerThreadState().cpuCount = count;
    WorkerThreadState().threadCount = ThreadCountForCPUCount(count);
}

#ifdef JS_ION

bool
js::StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData)
{
    
    JS_ASSERT(asmData->mir);
    JS_ASSERT(asmData->lir == nullptr);

    AutoLockWorkerThreadState lock;

    
    if (WorkerThreadState().asmJSWorkerFailed())
        return false;

    if (!WorkerThreadState().asmJSWorklist().append(asmData))
        return false;

    WorkerThreadState().notifyOne(GlobalWorkerThreadState::PRODUCER);
    return true;
}

bool
js::StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder)
{
    EnsureWorkerThreadsInitialized(cx);

    AutoLockWorkerThreadState lock;

    if (!WorkerThreadState().ionWorklist().append(builder))
        return false;

    WorkerThreadState().notifyOne(GlobalWorkerThreadState::PRODUCER);
    return true;
}






static void
FinishOffThreadIonCompile(jit::IonBuilder *builder)
{
    WorkerThreadState().ionFinishedList().append(builder);
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
    jit::JitCompartment *jitComp = compartment->jitCompartment();
    if (!jitComp)
        return;

    AutoLockWorkerThreadState lock;

    if (!WorkerThreadState().threads)
        return;

    
    GlobalWorkerThreadState::IonBuilderVector &worklist = WorkerThreadState().ionWorklist();
    for (size_t i = 0; i < worklist.length(); i++) {
        jit::IonBuilder *builder = worklist[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            FinishOffThreadIonCompile(builder);
            WorkerThreadState().remove(worklist, &i);
        }
    }

    
    for (size_t i = 0; i < WorkerThreadState().threadCount; i++) {
        const WorkerThread &helper = WorkerThreadState().threads[i];
        while (helper.ionBuilder &&
               CompiledScriptMatches(compartment, script, helper.ionBuilder->script()))
        {
            helper.ionBuilder->cancel();
            WorkerThreadState().wait(GlobalWorkerThreadState::CONSUMER);
        }
    }

    
    GlobalWorkerThreadState::IonBuilderVector &finished = WorkerThreadState().ionFinishedList();
    for (size_t i = 0; i < finished.length(); i++) {
        jit::IonBuilder *builder = finished[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            jit::FinishOffThreadBuilder(builder);
            WorkerThreadState().remove(finished, &i);
        }
    }
#endif 
}

static const JSClass workerGlobalClass = {
    "internal-worker-global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_DeletePropertyStub,
    JS_PropertyStub,  JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   nullptr,
    nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
};

ParseTask::ParseTask(ExclusiveContext *cx, JSObject *exclusiveContextGlobal, JSContext *initCx,
                     const jschar *chars, size_t length,
                     JS::OffThreadCompileCallback callback, void *callbackData)
  : cx(cx), options(initCx), chars(chars), length(length),
    alloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    exclusiveContextGlobal(initCx, exclusiveContextGlobal), optionsElement(initCx),
    optionsIntroductionScript(initCx), callback(callback), callbackData(callbackData),
    script(nullptr), errors(cx), overRecursed(false)
{
}

bool
ParseTask::init(JSContext *cx, const ReadOnlyCompileOptions &options)
{
    if (!this->options.copy(cx, options))
        return false;

    
    
    optionsElement = this->options.element();
    this->options.setElement(nullptr);
    optionsIntroductionScript = this->options.introductionScript();
    this->options.setIntroductionScript(nullptr);

    return true;
}

void
ParseTask::activate(JSRuntime *rt)
{
    rt->setUsedByExclusiveThread(exclusiveContextGlobal->zone());
    cx->enterCompartment(exclusiveContextGlobal->compartment());
}

void
ParseTask::finish()
{
    if (script) {
        
        
        ScriptSourceObject &sso = script->sourceObject()->as<ScriptSourceObject>();
        sso.initElement(optionsElement);
        sso.initIntroductionScript(optionsIntroductionScript);
    }
}

ParseTask::~ParseTask()
{
    
    js_delete(cx);

    for (size_t i = 0; i < errors.length(); i++)
        js_delete(errors[i]);
}

void
js::CancelOffThreadParses(JSRuntime *rt)
{
    AutoLockWorkerThreadState lock;

    if (!WorkerThreadState().threads)
        return;

    
    
    
    while (true) {
        bool pending = false;
        GlobalWorkerThreadState::ParseTaskVector &worklist = WorkerThreadState().parseWorklist();
        for (size_t i = 0; i < worklist.length(); i++) {
            ParseTask *task = worklist[i];
            if (task->runtimeMatches(rt))
                pending = true;
        }
        if (!pending) {
            bool inProgress = false;
            for (size_t i = 0; i < WorkerThreadState().threadCount; i++) {
                ParseTask *task = WorkerThreadState().threads[i].parseTask;
                if (task && task->runtimeMatches(rt))
                    inProgress = true;
            }
            if (!inProgress)
                break;
        }
        WorkerThreadState().wait(GlobalWorkerThreadState::CONSUMER);
    }

    
    GlobalWorkerThreadState::ParseTaskVector &finished = WorkerThreadState().parseFinishedList();
    while (true) {
        bool found = false;
        for (size_t i = 0; i < finished.length(); i++) {
            ParseTask *task = finished[i];
            if (task->runtimeMatches(rt)) {
                found = true;
                AutoUnlockWorkerThreadState unlock;
                WorkerThreadState().finishParseTask( nullptr, rt, task);
            }
        }
        if (!found)
            break;
    }
}

bool
js::OffThreadParsingMustWaitForGC(JSRuntime *rt)
{
    
    
    
    
    
    
    return rt->activeGCInAtomsZone();
}

bool
js::StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
                              const jschar *chars, size_t length,
                              JS::OffThreadCompileCallback callback, void *callbackData)
{
    
    
    gc::AutoSuppressGC suppress(cx);

    SourceBufferHolder srcBuf(chars, length, SourceBufferHolder::NoOwnership);
    frontend::MaybeCallSourceHandler(cx, options, srcBuf);

    EnsureWorkerThreadsInitialized(cx);

    JS::CompartmentOptions compartmentOptions(cx->compartment()->options());
    compartmentOptions.setZone(JS::FreshZone);
    compartmentOptions.setInvisibleToDebugger(true);
    compartmentOptions.setMergeable(true);

    
    compartmentOptions.setTrace(nullptr);

    JSObject *global = JS_NewGlobalObject(cx, &workerGlobalClass, nullptr,
                                          JS::FireOnNewGlobalHook, compartmentOptions);
    if (!global)
        return false;

    JS_SetCompartmentPrincipals(global->compartment(), cx->compartment()->principals);

    RootedObject obj(cx);

    
    
    
    if (!GetBuiltinConstructor(cx, JSProto_Function, &obj) ||
        !GetBuiltinConstructor(cx, JSProto_Array, &obj) ||
        !GetBuiltinConstructor(cx, JSProto_RegExp, &obj) ||
        !GetBuiltinConstructor(cx, JSProto_Iterator, &obj))
    {
        return false;
    }
    {
        AutoCompartment ac(cx, global);
        if (!GetBuiltinConstructor(cx, JSProto_Function, &obj) ||
            !GetBuiltinConstructor(cx, JSProto_Array, &obj) ||
            !GetBuiltinConstructor(cx, JSProto_RegExp, &obj) ||
            !GetBuiltinConstructor(cx, JSProto_Iterator, &obj))
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
                            callback, callbackData));
    if (!task)
        return false;

    workercx.forget();

    if (!task->init(cx, options))
        return false;

    if (OffThreadParsingMustWaitForGC(cx->runtime())) {
        AutoLockWorkerThreadState lock;
        if (!WorkerThreadState().parseWaitingOnGC().append(task.get()))
            return false;
    } else {
        task->activate(cx->runtime());

        AutoLockWorkerThreadState lock;

        if (!WorkerThreadState().parseWorklist().append(task.get()))
            return false;

        WorkerThreadState().notifyOne(GlobalWorkerThreadState::PRODUCER);
    }

    task.forget();

    return true;
}

void
js::EnqueuePendingParseTasksAfterGC(JSRuntime *rt)
{
    JS_ASSERT(!OffThreadParsingMustWaitForGC(rt));

    GlobalWorkerThreadState::ParseTaskVector newTasks;
    {
        AutoLockWorkerThreadState lock;
        GlobalWorkerThreadState::ParseTaskVector &waiting = WorkerThreadState().parseWaitingOnGC();

        for (size_t i = 0; i < waiting.length(); i++) {
            ParseTask *task = waiting[i];
            if (task->runtimeMatches(rt)) {
                newTasks.append(task);
                WorkerThreadState().remove(waiting, &i);
            }
        }
    }

    if (newTasks.empty())
        return;

    
    

    for (size_t i = 0; i < newTasks.length(); i++)
        newTasks[i]->activate(rt);

    AutoLockWorkerThreadState lock;

    for (size_t i = 0; i < newTasks.length(); i++)
        WorkerThreadState().parseWorklist().append(newTasks[i]);

    WorkerThreadState().notifyAll(GlobalWorkerThreadState::PRODUCER);
}

static const uint32_t WORKER_STACK_SIZE = 512 * 1024;
static const uint32_t WORKER_STACK_QUOTA = 450 * 1024;

void
GlobalWorkerThreadState::ensureInitialized()
{
    JS_ASSERT(this == &WorkerThreadState());
    AutoLockWorkerThreadState lock;

    if (threads)
        return;

    threads = js_pod_calloc<WorkerThread>(threadCount);
    if (!threads)
        CrashAtUnhandlableOOM("GlobalWorkerThreadState::ensureInitialized");

    for (size_t i = 0; i < threadCount; i++) {
        WorkerThread &helper = threads[i];
        helper.threadData.construct(static_cast<JSRuntime *>(nullptr));
        helper.thread = PR_CreateThread(PR_USER_THREAD,
                                        WorkerThread::ThreadMain, &helper,
                                        PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, WORKER_STACK_SIZE);
        if (!helper.thread || !helper.threadData.ref().init())
            CrashAtUnhandlableOOM("GlobalWorkerThreadState::ensureInitialized");
    }

    resetAsmJSFailureState();
}

GlobalWorkerThreadState::GlobalWorkerThreadState()
{
    mozilla::PodZero(this);

    cpuCount = GetCPUCount();
    threadCount = ThreadCountForCPUCount(cpuCount);

    MOZ_ASSERT(cpuCount > 0, "GetCPUCount() seems broken");

    workerLock = PR_NewLock();
    consumerWakeup = PR_NewCondVar(workerLock);
    producerWakeup = PR_NewCondVar(workerLock);
}

void
GlobalWorkerThreadState::finish()
{
    if (threads) {
        for (size_t i = 0; i < threadCount; i++)
            threads[i].destroy();
        js_free(threads);
    }

    PR_DestroyCondVar(consumerWakeup);
    PR_DestroyCondVar(producerWakeup);
    PR_DestroyLock(workerLock);
}

void
GlobalWorkerThreadState::lock()
{
    JS_ASSERT(!isLocked());
    AssertCurrentThreadCanLock(WorkerThreadStateLock);
    PR_Lock(workerLock);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
GlobalWorkerThreadState::unlock()
{
    JS_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = nullptr;
#endif
    PR_Unlock(workerLock);
}

#ifdef DEBUG
bool
GlobalWorkerThreadState::isLocked()
{
    return lockOwner == PR_GetCurrentThread();
}
#endif

void
GlobalWorkerThreadState::wait(CondVar which, uint32_t millis)
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
GlobalWorkerThreadState::notifyAll(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyAllCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup);
}

void
GlobalWorkerThreadState::notifyOne(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup);
}

bool
GlobalWorkerThreadState::canStartAsmJSCompile()
{
    
    JS_ASSERT(isLocked());
    return !asmJSWorklist().empty() && !numAsmJSFailedJobs;
}

bool
GlobalWorkerThreadState::canStartIonCompile()
{
    
    
    
    
    if (ionWorklist().empty())
        return false;
    for (size_t i = 0; i < threadCount; i++) {
        if (threads[i].ionBuilder)
            return false;
    }
    return true;
}

bool
GlobalWorkerThreadState::canStartParseTask()
{
    
    
    
    
    JS_ASSERT(isLocked());
    if (parseWorklist().empty())
        return false;
    for (size_t i = 0; i < threadCount; i++) {
        if (threads[i].parseTask)
            return false;
    }
    return true;
}

bool
GlobalWorkerThreadState::canStartCompressionTask()
{
    return !compressionWorklist().empty();
}

bool
GlobalWorkerThreadState::canStartGCHelperTask()
{
    return !gcHelperWorklist().empty();
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
GlobalWorkerThreadState::finishParseTask(JSContext *maybecx, JSRuntime *rt, void *token)
{
    ScopedJSDeletePtr<ParseTask> parseTask;

    
    
    {
        AutoLockWorkerThreadState lock;
        ParseTaskVector &finished = parseFinishedList();
        for (size_t i = 0; i < finished.length(); i++) {
            if (finished[i] == token) {
                parseTask = finished[i];
                remove(finished, &i);
                break;
            }
        }
    }
    JS_ASSERT(parseTask);

    
    
    rt->clearUsedByExclusiveThread(parseTask->cx->zone());
    if (!maybecx) {
        return nullptr;
    }
    JSContext *cx = maybecx;
    JS_ASSERT(cx->compartment());

    
    
    Rooted<GlobalObject*> global(cx, &cx->global()->as<GlobalObject>());
    if (!GlobalObject::ensureConstructor(cx, global, JSProto_Object) ||
        !GlobalObject::ensureConstructor(cx, global, JSProto_Array) ||
        !GlobalObject::ensureConstructor(cx, global, JSProto_Function) ||
        !GlobalObject::ensureConstructor(cx, global, JSProto_RegExp) ||
        !GlobalObject::ensureConstructor(cx, global, JSProto_Iterator))
    {
        return nullptr;
    }

    
    
    
    
    for (gc::ZoneCellIter iter(parseTask->cx->zone(), gc::FINALIZE_TYPE_OBJECT);
         !iter.done();
         iter.next())
    {
        types::TypeObject *object = iter.get<types::TypeObject>();
        TaggedProto proto(object->proto());
        if (!proto.isObject())
            continue;

        JSProtoKey key = JS::IdentifyStandardPrototype(proto.toObject());
        if (key == JSProto_Null)
            continue;
        JS_ASSERT(key == JSProto_Object || key == JSProto_Array ||
                  key == JSProto_Function || key == JSProto_RegExp ||
                  key == JSProto_Iterator);

        JSObject *newProto = GetBuiltinPrototypePure(global, key);
        JS_ASSERT(newProto);

        object->setProtoUnchecked(newProto);
    }

    
    gc::MergeCompartments(parseTask->cx->compartment(), cx->compartment());
    parseTask->finish();

    RootedScript script(rt, parseTask->script);
    assertSameCompartment(cx, script);

    
    
    for (size_t i = 0; i < parseTask->errors.length(); i++)
        parseTask->errors[i]->throwError(cx);
    if (parseTask->overRecursed)
        js_ReportOverRecursed(cx);

    if (script) {
        
        GlobalObject *compileAndGoGlobal = nullptr;
        if (script->compileAndGo())
            compileAndGoGlobal = &script->global();
        Debugger::onNewScript(cx, script, compileAndGoGlobal);

        
        CallNewScriptHookForAllScripts(cx, script);
    }

    return script;
}

void
WorkerThread::destroy()
{
    if (thread) {
        {
            AutoLockWorkerThreadState lock;
            terminate = true;

            
            WorkerThreadState().notifyAll(GlobalWorkerThreadState::PRODUCER);
        }

        PR_JoinThread(thread);
    }

    if (!threadData.empty())
        threadData.destroy();
}

#ifdef MOZ_NUWA_PROCESS
extern "C" {
MFBT_API bool IsNuwaProcess();
MFBT_API void NuwaMarkCurrentThread(void (*recreate)(void *), void *arg);
}
#endif


void
WorkerThread::ThreadMain(void *arg)
{
    PR_SetCurrentThreadName("Analysis Helper");

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        JS_ASSERT(NuwaMarkCurrentThread != nullptr);
        NuwaMarkCurrentThread(nullptr, nullptr);
    }
#endif

    static_cast<WorkerThread *>(arg)->threadLoop();
}

void
WorkerThread::handleAsmJSWorkload()
{
#ifdef JS_ION
    JS_ASSERT(WorkerThreadState().isLocked());
    JS_ASSERT(WorkerThreadState().canStartAsmJSCompile());
    JS_ASSERT(idle());

    asmData = WorkerThreadState().asmJSWorklist().popCopy();
    bool success = false;

    do {
        AutoUnlockWorkerThreadState unlock;
        PerThreadData::AutoEnterRuntime enter(threadData.addr(), asmData->runtime);

        jit::IonContext icx(asmData->mir->compartment->runtime(),
                            asmData->mir->compartment,
                            &asmData->mir->alloc());

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

    
    if (!success) {
        WorkerThreadState().noteAsmJSFailure(asmData->func);
        WorkerThreadState().notifyAll(GlobalWorkerThreadState::CONSUMER);
        asmData = nullptr;
        return;
    }

    
    WorkerThreadState().asmJSFinishedList().append(asmData);
    asmData = nullptr;

    
    WorkerThreadState().notifyAll(GlobalWorkerThreadState::CONSUMER);
#else
    MOZ_CRASH();
#endif 
}

void
WorkerThread::handleIonWorkload()
{
#ifdef JS_ION
    JS_ASSERT(WorkerThreadState().isLocked());
    JS_ASSERT(WorkerThreadState().canStartIonCompile());
    JS_ASSERT(idle());

    
    GlobalWorkerThreadState::IonBuilderVector &ionWorklist = WorkerThreadState().ionWorklist();
    size_t highest = 0;
    for (size_t i = 1; i < ionWorklist.length(); i++) {
        if (ionWorklist[i]->script()->getUseCount() >
            ionWorklist[highest]->script()->getUseCount())
        {
            highest = i;
        }
    }
    ionBuilder = ionWorklist[highest];

    
    
    if (highest != ionWorklist.length() - 1)
        ionWorklist[highest] = ionWorklist.popCopy();
    else
        ionWorklist.popBack();

    TraceLogger *logger = TraceLoggerForCurrentThread();
    AutoTraceLog logScript(logger, TraceLogCreateTextId(logger, ionBuilder->script()));
    AutoTraceLog logCompile(logger, TraceLogger::IonCompilation);

    JSRuntime *rt = ionBuilder->script()->compartment()->runtimeFromAnyThread();

    {
        AutoUnlockWorkerThreadState unlock;
        PerThreadData::AutoEnterRuntime enter(threadData.addr(),
                                              ionBuilder->script()->runtimeFromAnyThread());
        jit::IonContext ictx(jit::CompileRuntime::get(rt),
                             jit::CompileCompartment::get(ionBuilder->script()->compartment()),
                             &ionBuilder->alloc());
        ionBuilder->setBackgroundCodegen(jit::CompileBackEnd(ionBuilder));
    }

    FinishOffThreadIonCompile(ionBuilder);
    ionBuilder = nullptr;

    
    
    
    
    rt->requestInterrupt(JSRuntime::RequestInterruptAnyThreadDontStopIon);

    
    WorkerThreadState().notifyAll(GlobalWorkerThreadState::CONSUMER);
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
WorkerThread::handleParseWorkload()
{
    JS_ASSERT(WorkerThreadState().isLocked());
    JS_ASSERT(WorkerThreadState().canStartParseTask());
    JS_ASSERT(idle());

    parseTask = WorkerThreadState().parseWorklist().popCopy();
    parseTask->cx->setWorkerThread(this);

    {
        AutoUnlockWorkerThreadState unlock;
        PerThreadData::AutoEnterRuntime enter(threadData.addr(),
                                              parseTask->exclusiveContextGlobal->runtimeFromAnyThread());
        SourceBufferHolder srcBuf(parseTask->chars, parseTask->length,
                                  SourceBufferHolder::NoOwnership);
        parseTask->script = frontend::CompileScript(parseTask->cx, &parseTask->alloc,
                                                    NullPtr(), NullPtr(),
                                                    parseTask->options,
                                                    srcBuf);
    }

    
    parseTask->callback(parseTask, parseTask->callbackData);

    
    
    WorkerThreadState().parseFinishedList().append(parseTask);

    parseTask = nullptr;

    
    WorkerThreadState().notifyAll(GlobalWorkerThreadState::CONSUMER);
}

void
WorkerThread::handleCompressionWorkload()
{
    JS_ASSERT(WorkerThreadState().isLocked());
    JS_ASSERT(WorkerThreadState().canStartCompressionTask());
    JS_ASSERT(idle());

    compressionTask = WorkerThreadState().compressionWorklist().popCopy();
    compressionTask->workerThread = this;

    {
        AutoUnlockWorkerThreadState unlock;
        compressionTask->result = compressionTask->work();
    }

    compressionTask->workerThread = nullptr;
    compressionTask = nullptr;

    
    WorkerThreadState().notifyAll(GlobalWorkerThreadState::CONSUMER);
}

bool
js::StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task)
{
    EnsureWorkerThreadsInitialized(cx);

    AutoLockWorkerThreadState lock;

    if (!WorkerThreadState().compressionWorklist().append(task)) {
        if (JSContext *maybecx = cx->maybeJSContext())
            js_ReportOutOfMemory(maybecx);
        return false;
    }

    WorkerThreadState().notifyOne(GlobalWorkerThreadState::PRODUCER);
    return true;
}

bool
GlobalWorkerThreadState::compressionInProgress(SourceCompressionTask *task)
{
    JS_ASSERT(isLocked());
    for (size_t i = 0; i < compressionWorklist().length(); i++) {
        if (compressionWorklist()[i] == task)
            return true;
    }
    for (size_t i = 0; i < threadCount; i++) {
        if (threads[i].compressionTask == task)
            return true;
    }
    return false;
}

bool
SourceCompressionTask::complete()
{
    if (!active()) {
        JS_ASSERT(!compressed);
        return true;
    }

    {
        AutoLockWorkerThreadState lock;
        while (WorkerThreadState().compressionInProgress(this))
            WorkerThreadState().wait(GlobalWorkerThreadState::CONSUMER);
    }

    if (result == Success) {
        ss->setCompressedSource(compressed, compressedBytes);

        
        cx->updateMallocCounter(ss->computedSizeOfData());
    } else {
        js_free(compressed);

        if (result == OOM)
            js_ReportOutOfMemory(cx);
        else if (result == Aborted && !ss->ensureOwnsSource(cx))
            result = OOM;
    }

    ss = nullptr;
    compressed = nullptr;
    JS_ASSERT(!active());

    return result != OOM;
}

SourceCompressionTask *
GlobalWorkerThreadState::compressionTaskForSource(ScriptSource *ss)
{
    JS_ASSERT(isLocked());
    for (size_t i = 0; i < compressionWorklist().length(); i++) {
        SourceCompressionTask *task = compressionWorklist()[i];
        if (task->source() == ss)
            return task;
    }
    for (size_t i = 0; i < threadCount; i++) {
        SourceCompressionTask *task = threads[i].compressionTask;
        if (task && task->source() == ss)
            return task;
    }
    return nullptr;
}

void
WorkerThread::handleGCHelperWorkload()
{
    JS_ASSERT(WorkerThreadState().isLocked());
    JS_ASSERT(WorkerThreadState().canStartGCHelperTask());
    JS_ASSERT(idle());

    JS_ASSERT(!gcHelperState);
    gcHelperState = WorkerThreadState().gcHelperWorklist().popCopy();

    {
        AutoUnlockWorkerThreadState unlock;
        gcHelperState->work();
    }

    gcHelperState = nullptr;
}

void
WorkerThread::threadLoop()
{
    JS::AutoAssertNoGC nogc;
    AutoLockWorkerThreadState lock;

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
            if (WorkerThreadState().canStartIonCompile() ||
                WorkerThreadState().canStartAsmJSCompile() ||
                WorkerThreadState().canStartParseTask() ||
                WorkerThreadState().canStartCompressionTask() ||
                WorkerThreadState().canStartGCHelperTask())
            {
                break;
            }
            WorkerThreadState().wait(GlobalWorkerThreadState::PRODUCER);
        }

        
        if (WorkerThreadState().canStartAsmJSCompile())
            handleAsmJSWorkload();
        else if (WorkerThreadState().canStartIonCompile())
            handleIonWorkload();
        else if (WorkerThreadState().canStartParseTask())
            handleParseWorkload();
        else if (WorkerThreadState().canStartCompressionTask())
            handleCompressionWorkload();
        else if (WorkerThreadState().canStartGCHelperTask())
            handleGCHelperWorkload();
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

void
js::CancelOffThreadParses(JSRuntime *rt)
{
}

bool
js::StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
                              const jschar *chars, size_t length,
                              JS::OffThreadCompileCallback callback, void *callbackData)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compilation not available in non-THREADSAFE builds");
}

bool
js::StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task)
{
    MOZ_ASSUME_UNREACHABLE("Off thread compression not available");
}

bool
SourceCompressionTask::complete()
{
    JS_ASSERT(!ss);
    return true;
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
