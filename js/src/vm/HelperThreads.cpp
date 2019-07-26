





#include "vm/HelperThreads.h"

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

GlobalHelperThreadState gHelperThreadState;

} 

void
js::EnsureHelperThreadsInitialized(ExclusiveContext *cx)
{
    
    
    if (!cx->isJSContext())
        return;

    HelperThreadState().ensureInitialized();
}

static size_t
ThreadCountForCPUCount(size_t cpuCount)
{
    return Max(cpuCount, (size_t)2);
}

void
js::SetFakeCPUCount(size_t count)
{
    
    JS_ASSERT(!HelperThreadState().threads);

    HelperThreadState().cpuCount = count;
    HelperThreadState().threadCount = ThreadCountForCPUCount(count);
}

#ifdef JS_ION

bool
js::StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData)
{
    
    JS_ASSERT(asmData->mir);
    JS_ASSERT(asmData->lir == nullptr);

    AutoLockHelperThreadState lock;

    
    if (HelperThreadState().asmJSFailed())
        return false;

    if (!HelperThreadState().asmJSWorklist().append(asmData))
        return false;

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    return true;
}

bool
js::StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder)
{
    EnsureHelperThreadsInitialized(cx);

    AutoLockHelperThreadState lock;

    if (!HelperThreadState().ionWorklist().append(builder))
        return false;

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    return true;
}






static void
FinishOffThreadIonCompile(jit::IonBuilder *builder)
{
    HelperThreadState().ionFinishedList().append(builder);
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

    AutoLockHelperThreadState lock;

    if (!HelperThreadState().threads)
        return;

    
    GlobalHelperThreadState::IonBuilderVector &worklist = HelperThreadState().ionWorklist();
    for (size_t i = 0; i < worklist.length(); i++) {
        jit::IonBuilder *builder = worklist[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            FinishOffThreadIonCompile(builder);
            HelperThreadState().remove(worklist, &i);
        }
    }

    
    for (size_t i = 0; i < HelperThreadState().threadCount; i++) {
        const HelperThread &helper = HelperThreadState().threads[i];
        while (helper.ionBuilder &&
               CompiledScriptMatches(compartment, script, helper.ionBuilder->script()))
        {
            helper.ionBuilder->cancel();
            HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
        }
    }

    
    GlobalHelperThreadState::IonBuilderVector &finished = HelperThreadState().ionFinishedList();
    for (size_t i = 0; i < finished.length(); i++) {
        jit::IonBuilder *builder = finished[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            jit::FinishOffThreadBuilder(builder);
            HelperThreadState().remove(finished, &i);
        }
    }
#endif 
}

static const JSClass parseTaskGlobalClass = {
    "internal-parse-task-global", JSCLASS_GLOBAL_FLAGS,
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
    AutoLockHelperThreadState lock;

    if (!HelperThreadState().threads)
        return;

    
    
    
    while (true) {
        bool pending = false;
        GlobalHelperThreadState::ParseTaskVector &worklist = HelperThreadState().parseWorklist();
        for (size_t i = 0; i < worklist.length(); i++) {
            ParseTask *task = worklist[i];
            if (task->runtimeMatches(rt))
                pending = true;
        }
        if (!pending) {
            bool inProgress = false;
            for (size_t i = 0; i < HelperThreadState().threadCount; i++) {
                ParseTask *task = HelperThreadState().threads[i].parseTask;
                if (task && task->runtimeMatches(rt))
                    inProgress = true;
            }
            if (!inProgress)
                break;
        }
        HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
    }

    
    GlobalHelperThreadState::ParseTaskVector &finished = HelperThreadState().parseFinishedList();
    while (true) {
        bool found = false;
        for (size_t i = 0; i < finished.length(); i++) {
            ParseTask *task = finished[i];
            if (task->runtimeMatches(rt)) {
                found = true;
                AutoUnlockHelperThreadState unlock;
                HelperThreadState().finishParseTask( nullptr, rt, task);
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

    EnsureHelperThreadsInitialized(cx);

    JS::CompartmentOptions compartmentOptions(cx->compartment()->options());
    compartmentOptions.setZone(JS::FreshZone);
    compartmentOptions.setInvisibleToDebugger(true);
    compartmentOptions.setMergeable(true);

    
    compartmentOptions.setTrace(nullptr);

    JSObject *global = JS_NewGlobalObject(cx, &parseTaskGlobalClass, nullptr,
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

    ScopedJSDeletePtr<ExclusiveContext> helpercx(
        cx->new_<ExclusiveContext>(cx->runtime(), (PerThreadData *) nullptr,
                                   ThreadSafeContext::Context_Exclusive));
    if (!helpercx)
        return false;

    ScopedJSDeletePtr<ParseTask> task(
        cx->new_<ParseTask>(helpercx.get(), global, cx, chars, length,
                            callback, callbackData));
    if (!task)
        return false;

    helpercx.forget();

    if (!task->init(cx, options))
        return false;

    if (OffThreadParsingMustWaitForGC(cx->runtime())) {
        AutoLockHelperThreadState lock;
        if (!HelperThreadState().parseWaitingOnGC().append(task.get()))
            return false;
    } else {
        task->activate(cx->runtime());

        AutoLockHelperThreadState lock;

        if (!HelperThreadState().parseWorklist().append(task.get()))
            return false;

        HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    }

    task.forget();

    return true;
}

void
js::EnqueuePendingParseTasksAfterGC(JSRuntime *rt)
{
    JS_ASSERT(!OffThreadParsingMustWaitForGC(rt));

    GlobalHelperThreadState::ParseTaskVector newTasks;
    {
        AutoLockHelperThreadState lock;
        GlobalHelperThreadState::ParseTaskVector &waiting = HelperThreadState().parseWaitingOnGC();

        for (size_t i = 0; i < waiting.length(); i++) {
            ParseTask *task = waiting[i];
            if (task->runtimeMatches(rt)) {
                newTasks.append(task);
                HelperThreadState().remove(waiting, &i);
            }
        }
    }

    if (newTasks.empty())
        return;

    
    

    for (size_t i = 0; i < newTasks.length(); i++)
        newTasks[i]->activate(rt);

    AutoLockHelperThreadState lock;

    for (size_t i = 0; i < newTasks.length(); i++)
        HelperThreadState().parseWorklist().append(newTasks[i]);

    HelperThreadState().notifyAll(GlobalHelperThreadState::PRODUCER);
}

static const uint32_t HELPER_STACK_SIZE = 512 * 1024;
static const uint32_t HELPER_STACK_QUOTA = 450 * 1024;

void
GlobalHelperThreadState::ensureInitialized()
{
    JS_ASSERT(this == &HelperThreadState());
    AutoLockHelperThreadState lock;

    if (threads)
        return;

    threads = js_pod_calloc<HelperThread>(threadCount);
    if (!threads)
        CrashAtUnhandlableOOM("GlobalHelperThreadState::ensureInitialized");

    for (size_t i = 0; i < threadCount; i++) {
        HelperThread &helper = threads[i];
        helper.threadData.construct(static_cast<JSRuntime *>(nullptr));
        helper.thread = PR_CreateThread(PR_USER_THREAD,
                                        HelperThread::ThreadMain, &helper,
                                        PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, HELPER_STACK_SIZE);
        if (!helper.thread || !helper.threadData.ref().init())
            CrashAtUnhandlableOOM("GlobalHelperThreadState::ensureInitialized");
    }

    resetAsmJSFailureState();
}

GlobalHelperThreadState::GlobalHelperThreadState()
{
    mozilla::PodZero(this);

    cpuCount = GetCPUCount();
    threadCount = ThreadCountForCPUCount(cpuCount);

    MOZ_ASSERT(cpuCount > 0, "GetCPUCount() seems broken");

    helperLock = PR_NewLock();
    consumerWakeup = PR_NewCondVar(helperLock);
    producerWakeup = PR_NewCondVar(helperLock);
}

void
GlobalHelperThreadState::finish()
{
    if (threads) {
        for (size_t i = 0; i < threadCount; i++)
            threads[i].destroy();
        js_free(threads);
    }

    PR_DestroyCondVar(consumerWakeup);
    PR_DestroyCondVar(producerWakeup);
    PR_DestroyLock(helperLock);
}

void
GlobalHelperThreadState::lock()
{
    JS_ASSERT(!isLocked());
    AssertCurrentThreadCanLock(HelperThreadStateLock);
    PR_Lock(helperLock);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
GlobalHelperThreadState::unlock()
{
    JS_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = nullptr;
#endif
    PR_Unlock(helperLock);
}

#ifdef DEBUG
bool
GlobalHelperThreadState::isLocked()
{
    return lockOwner == PR_GetCurrentThread();
}
#endif

void
GlobalHelperThreadState::wait(CondVar which, uint32_t millis)
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
GlobalHelperThreadState::notifyAll(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyAllCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup);
}

void
GlobalHelperThreadState::notifyOne(CondVar which)
{
    JS_ASSERT(isLocked());
    PR_NotifyCondVar((which == CONSUMER) ? consumerWakeup : producerWakeup);
}

bool
GlobalHelperThreadState::canStartAsmJSCompile()
{
    
    JS_ASSERT(isLocked());
    return !asmJSWorklist().empty() && !numAsmJSFailedJobs;
}

bool
GlobalHelperThreadState::canStartIonCompile()
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
GlobalHelperThreadState::canStartParseTask()
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
GlobalHelperThreadState::canStartCompressionTask()
{
    return !compressionWorklist().empty();
}

bool
GlobalHelperThreadState::canStartGCHelperTask()
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
GlobalHelperThreadState::finishParseTask(JSContext *maybecx, JSRuntime *rt, void *token)
{
    ScopedJSDeletePtr<ParseTask> parseTask;

    
    
    {
        AutoLockHelperThreadState lock;
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

        object->setProtoUnchecked(TaggedProto(newProto));
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
HelperThread::destroy()
{
    if (thread) {
        {
            AutoLockHelperThreadState lock;
            terminate = true;

            
            HelperThreadState().notifyAll(GlobalHelperThreadState::PRODUCER);
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
HelperThread::ThreadMain(void *arg)
{
    PR_SetCurrentThreadName("Analysis Helper");

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        JS_ASSERT(NuwaMarkCurrentThread != nullptr);
        NuwaMarkCurrentThread(nullptr, nullptr);
    }
#endif

    static_cast<HelperThread *>(arg)->threadLoop();
}

void
HelperThread::handleAsmJSWorkload()
{
#ifdef JS_ION
    JS_ASSERT(HelperThreadState().isLocked());
    JS_ASSERT(HelperThreadState().canStartAsmJSCompile());
    JS_ASSERT(idle());

    asmData = HelperThreadState().asmJSWorklist().popCopy();
    bool success = false;

    do {
        AutoUnlockHelperThreadState unlock;
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
        HelperThreadState().noteAsmJSFailure(asmData->func);
        HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);
        asmData = nullptr;
        return;
    }

    
    HelperThreadState().asmJSFinishedList().append(asmData);
    asmData = nullptr;

    
    HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);
#else
    MOZ_CRASH();
#endif 
}

void
HelperThread::handleIonWorkload()
{
#ifdef JS_ION
    JS_ASSERT(HelperThreadState().isLocked());
    JS_ASSERT(HelperThreadState().canStartIonCompile());
    JS_ASSERT(idle());

    
    GlobalHelperThreadState::IonBuilderVector &ionWorklist = HelperThreadState().ionWorklist();
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
        AutoUnlockHelperThreadState unlock;
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

    
    HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);
#else
    MOZ_CRASH();
#endif 
}

void
ExclusiveContext::setHelperThread(HelperThread *thread)
{
    helperThread_ = thread;
    perThreadData = thread->threadData.addr();
}

frontend::CompileError &
ExclusiveContext::addPendingCompileError()
{
    frontend::CompileError *error = js_new<frontend::CompileError>();
    if (!error)
        MOZ_CRASH();
    if (!helperThread()->parseTask->errors.append(error))
        MOZ_CRASH();
    return *error;
}

void
ExclusiveContext::addPendingOverRecursed()
{
    if (helperThread()->parseTask)
        helperThread()->parseTask->overRecursed = true;
}

void
HelperThread::handleParseWorkload()
{
    JS_ASSERT(HelperThreadState().isLocked());
    JS_ASSERT(HelperThreadState().canStartParseTask());
    JS_ASSERT(idle());

    parseTask = HelperThreadState().parseWorklist().popCopy();
    parseTask->cx->setHelperThread(this);

    {
        AutoUnlockHelperThreadState unlock;
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

    
    
    HelperThreadState().parseFinishedList().append(parseTask);

    parseTask = nullptr;

    
    HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);
}

void
HelperThread::handleCompressionWorkload()
{
    JS_ASSERT(HelperThreadState().isLocked());
    JS_ASSERT(HelperThreadState().canStartCompressionTask());
    JS_ASSERT(idle());

    compressionTask = HelperThreadState().compressionWorklist().popCopy();
    compressionTask->helperThread = this;

    {
        AutoUnlockHelperThreadState unlock;
        compressionTask->result = compressionTask->work();
    }

    compressionTask->helperThread = nullptr;
    compressionTask = nullptr;

    
    HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);
}

bool
js::StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task)
{
    EnsureHelperThreadsInitialized(cx);

    AutoLockHelperThreadState lock;

    if (!HelperThreadState().compressionWorklist().append(task)) {
        if (JSContext *maybecx = cx->maybeJSContext())
            js_ReportOutOfMemory(maybecx);
        return false;
    }

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    return true;
}

bool
GlobalHelperThreadState::compressionInProgress(SourceCompressionTask *task)
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
        AutoLockHelperThreadState lock;
        while (HelperThreadState().compressionInProgress(this))
            HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
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
GlobalHelperThreadState::compressionTaskForSource(ScriptSource *ss)
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
HelperThread::handleGCHelperWorkload()
{
    JS_ASSERT(HelperThreadState().isLocked());
    JS_ASSERT(HelperThreadState().canStartGCHelperTask());
    JS_ASSERT(idle());

    JS_ASSERT(!gcHelperState);
    gcHelperState = HelperThreadState().gcHelperWorklist().popCopy();

    {
        AutoUnlockHelperThreadState unlock;
        gcHelperState->work();
    }

    gcHelperState = nullptr;
}

void
HelperThread::threadLoop()
{
    JS::AutoSuppressGCAnalysis nogc;
    AutoLockHelperThreadState lock;

    js::TlsPerThreadData.set(threadData.addr());

    
    uintptr_t stackLimit = GetNativeStackBase();
#if JS_STACK_GROWTH_DIRECTION > 0
    stackLimit += HELPER_STACK_QUOTA;
#else
    stackLimit -= HELPER_STACK_QUOTA;
#endif
    for (size_t i = 0; i < ArrayLength(threadData.ref().nativeStackLimit); i++)
        threadData.ref().nativeStackLimit[i] = stackLimit;

    while (true) {
        JS_ASSERT(!ionBuilder && !asmData);

        
        while (true) {
            if (terminate)
                return;
            if (HelperThreadState().canStartIonCompile() ||
                HelperThreadState().canStartAsmJSCompile() ||
                HelperThreadState().canStartParseTask() ||
                HelperThreadState().canStartCompressionTask() ||
                HelperThreadState().canStartGCHelperTask())
            {
                break;
            }
            HelperThreadState().wait(GlobalHelperThreadState::PRODUCER);
        }

        
        if (HelperThreadState().canStartAsmJSCompile())
            handleAsmJSWorkload();
        else if (HelperThreadState().canStartIonCompile())
            handleIonWorkload();
        else if (HelperThreadState().canStartParseTask())
            handleParseWorkload();
        else if (HelperThreadState().canStartCompressionTask())
            handleCompressionWorkload();
        else if (HelperThreadState().canStartGCHelperTask())
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
