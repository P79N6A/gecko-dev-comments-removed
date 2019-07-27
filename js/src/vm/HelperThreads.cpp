





#include "vm/HelperThreads.h"

#include "mozilla/DebugOnly.h"

#include "jsnativestack.h"
#include "jsnum.h" 
#include "prmjtime.h"

#include "frontend/BytecodeCompiler.h"
#include "gc/GCInternals.h"
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

GlobalHelperThreadState* gHelperThreadState = nullptr;

} 

bool
js::CreateHelperThreadsState()
{
    MOZ_ASSERT(!gHelperThreadState);
    gHelperThreadState = js_new<GlobalHelperThreadState>();
    return gHelperThreadState != nullptr;
}

void
js::DestroyHelperThreadsState()
{
    MOZ_ASSERT(gHelperThreadState);
    gHelperThreadState->finish();
    js_delete(gHelperThreadState);
    gHelperThreadState = nullptr;
}

void
js::EnsureHelperThreadsInitialized()
{
    MOZ_ASSERT(gHelperThreadState);
    gHelperThreadState->ensureInitialized();
}

static size_t
ThreadCountForCPUCount(size_t cpuCount)
{
    
    
    static const uint32_t EXCESS_THREADS = 4;
    return cpuCount + EXCESS_THREADS;
}

void
js::SetFakeCPUCount(size_t count)
{
    
    MOZ_ASSERT(!HelperThreadState().threads);

    HelperThreadState().cpuCount = count;
    HelperThreadState().threadCount = ThreadCountForCPUCount(count);
}

bool
js::StartOffThreadAsmJSCompile(ExclusiveContext* cx, AsmJSParallelTask* asmData)
{
    
    MOZ_ASSERT(asmData->mir);
    MOZ_ASSERT(asmData->lir == nullptr);

    AutoLockHelperThreadState lock;

    
    if (HelperThreadState().asmJSFailed())
        return false;

    if (!HelperThreadState().asmJSWorklist().append(asmData))
        return false;

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    return true;
}

bool
js::StartOffThreadIonCompile(JSContext* cx, jit::IonBuilder* builder)
{
    AutoLockHelperThreadState lock;

    if (!HelperThreadState().ionWorklist().append(builder))
        return false;

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    return true;
}






static void
FinishOffThreadIonCompile(jit::IonBuilder* builder)
{
    if (!HelperThreadState().ionFinishedList().append(builder))
        CrashAtUnhandlableOOM("FinishOffThreadIonCompile");
}

static inline bool
CompiledScriptMatches(JSCompartment* compartment, JSScript* script, JSScript* target)
{
    if (script)
        return target == script;
    return target->compartment() == compartment;
}

void
js::CancelOffThreadIonCompile(JSCompartment* compartment, JSScript* script)
{
    jit::JitCompartment* jitComp = compartment->jitCompartment();
    if (!jitComp)
        return;

    AutoLockHelperThreadState lock;

    if (!HelperThreadState().threads)
        return;

    
    GlobalHelperThreadState::IonBuilderVector& worklist = HelperThreadState().ionWorklist();
    for (size_t i = 0; i < worklist.length(); i++) {
        jit::IonBuilder* builder = worklist[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            FinishOffThreadIonCompile(builder);
            HelperThreadState().remove(worklist, &i);
        }
    }

    
    for (size_t i = 0; i < HelperThreadState().threadCount; i++) {
        HelperThread& helper = HelperThreadState().threads[i];
        while (helper.ionBuilder &&
               CompiledScriptMatches(compartment, script, helper.ionBuilder->script()))
        {
            helper.ionBuilder->cancel();
            if (helper.pause) {
                helper.pause = false;
                HelperThreadState().notifyAll(GlobalHelperThreadState::PAUSE);
            }
            HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
        }
    }

    
    GlobalHelperThreadState::IonBuilderVector& finished = HelperThreadState().ionFinishedList();
    for (size_t i = 0; i < finished.length(); i++) {
        jit::IonBuilder* builder = finished[i];
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            jit::FinishOffThreadBuilder(nullptr, builder);
            HelperThreadState().remove(finished, &i);
        }
    }

    
    jit::IonBuilder* builder = HelperThreadState().ionLazyLinkList().getFirst();
    while (builder) {
        jit::IonBuilder* next = builder->getNext();
        if (CompiledScriptMatches(compartment, script, builder->script())) {
            builder->script()->setPendingIonBuilder(nullptr, nullptr);
            jit::FinishOffThreadBuilder(nullptr, builder);
        }
        builder = next;
    }
}

static const JSClass parseTaskGlobalClass = {
    "internal-parse-task-global", JSCLASS_GLOBAL_FLAGS,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
};

ParseTask::ParseTask(ExclusiveContext* cx, JSObject* exclusiveContextGlobal, JSContext* initCx,
                     const char16_t* chars, size_t length,
                     JS::OffThreadCompileCallback callback, void* callbackData)
  : cx(cx), options(initCx), chars(chars), length(length),
    alloc(JSRuntime::TEMP_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    exclusiveContextGlobal(initCx, exclusiveContextGlobal),
    callback(callback), callbackData(callbackData),
    script(nullptr), errors(cx), overRecursed(false)
{
}

bool
ParseTask::init(JSContext* cx, const ReadOnlyCompileOptions& options)
{
    if (!this->options.copy(cx, options))
        return false;

    
    
    
    
    if (cx->compartment()->debuggerObservesAsmJS())
        this->options.asmJSOption = false;

    return true;
}

void
ParseTask::activate(JSRuntime* rt)
{
    rt->setUsedByExclusiveThread(exclusiveContextGlobal->zone());
    cx->enterCompartment(exclusiveContextGlobal->compartment());
}

bool
ParseTask::finish(JSContext* cx)
{
    if (script) {
        
        
        RootedScriptSource sso(cx, &script->sourceObject()->as<ScriptSourceObject>());
        if (!ScriptSourceObject::initFromOptions(cx, sso, options))
            return false;
    }

    return true;
}

ParseTask::~ParseTask()
{
    
    js_delete(cx);

    for (size_t i = 0; i < errors.length(); i++)
        js_delete(errors[i]);
}

void
js::CancelOffThreadParses(JSRuntime* rt)
{
    AutoLockHelperThreadState lock;

    if (!HelperThreadState().threads)
        return;

    
    
    
    while (true) {
        bool pending = false;
        GlobalHelperThreadState::ParseTaskVector& worklist = HelperThreadState().parseWorklist();
        for (size_t i = 0; i < worklist.length(); i++) {
            ParseTask* task = worklist[i];
            if (task->runtimeMatches(rt))
                pending = true;
        }
        if (!pending) {
            bool inProgress = false;
            for (size_t i = 0; i < HelperThreadState().threadCount; i++) {
                ParseTask* task = HelperThreadState().threads[i].parseTask;
                if (task && task->runtimeMatches(rt))
                    inProgress = true;
            }
            if (!inProgress)
                break;
        }
        HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
    }

    
    GlobalHelperThreadState::ParseTaskVector& finished = HelperThreadState().parseFinishedList();
    while (true) {
        bool found = false;
        for (size_t i = 0; i < finished.length(); i++) {
            ParseTask* task = finished[i];
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
js::OffThreadParsingMustWaitForGC(JSRuntime* rt)
{
    
    
    
    
    
    
    return rt->activeGCInAtomsZone();
}

bool
js::StartOffThreadParseScript(JSContext* cx, const ReadOnlyCompileOptions& options,
                              const char16_t* chars, size_t length,
                              JS::OffThreadCompileCallback callback, void* callbackData)
{
    
    
    gc::AutoSuppressGC suppress(cx);

    JS::CompartmentOptions compartmentOptions(cx->compartment()->options());
    compartmentOptions.setZone(JS::FreshZone);
    compartmentOptions.setInvisibleToDebugger(true);
    compartmentOptions.setMergeable(true);

    
    compartmentOptions.setTrace(nullptr);

    JSObject* global = JS_NewGlobalObject(cx, &parseTaskGlobalClass, nullptr,
                                          JS::FireOnNewGlobalHook, compartmentOptions);
    if (!global)
        return false;

    JS_SetCompartmentPrincipals(global->compartment(), cx->compartment()->principals());

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
        cx->new_<ExclusiveContext>(cx->runtime(), (PerThreadData*) nullptr,
                                   ExclusiveContext::Context_Exclusive));
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
js::EnqueuePendingParseTasksAfterGC(JSRuntime* rt)
{
    MOZ_ASSERT(!OffThreadParsingMustWaitForGC(rt));

    GlobalHelperThreadState::ParseTaskVector newTasks;
    {
        AutoLockHelperThreadState lock;
        GlobalHelperThreadState::ParseTaskVector& waiting = HelperThreadState().parseWaitingOnGC();

        for (size_t i = 0; i < waiting.length(); i++) {
            ParseTask* task = waiting[i];
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

static const uint32_t kDefaultHelperStackSize = 512 * 1024;
static const uint32_t kDefaultHelperStackQuota = 450 * 1024;











#if defined(MOZ_TSAN)
static const uint32_t HELPER_STACK_SIZE = 2 * kDefaultHelperStackSize;
static const uint32_t HELPER_STACK_QUOTA = 2 * kDefaultHelperStackQuota;
#else
static const uint32_t HELPER_STACK_SIZE = kDefaultHelperStackSize;
static const uint32_t HELPER_STACK_QUOTA = kDefaultHelperStackQuota;
#endif

void
GlobalHelperThreadState::ensureInitialized()
{
    MOZ_ASSERT(CanUseExtraThreads());

    MOZ_ASSERT(this == &HelperThreadState());
    AutoLockHelperThreadState lock;

    if (threads)
        return;

    threads = js_pod_calloc<HelperThread>(threadCount);
    if (!threads)
        CrashAtUnhandlableOOM("GlobalHelperThreadState::ensureInitialized");

    for (size_t i = 0; i < threadCount; i++) {
        HelperThread& helper = threads[i];
        helper.threadData.emplace(static_cast<JSRuntime*>(nullptr));
        helper.thread = PR_CreateThread(PR_USER_THREAD,
                                        HelperThread::ThreadMain, &helper,
                                        PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, HELPER_STACK_SIZE);
        if (!helper.thread || !helper.threadData->init())
            CrashAtUnhandlableOOM("GlobalHelperThreadState::ensureInitialized");
    }

    resetAsmJSFailureState();
}

GlobalHelperThreadState::GlobalHelperThreadState()
 : cpuCount(0),
   threadCount(0),
   threads(nullptr),
   asmJSCompilationInProgress(false),
   helperLock(nullptr),
#ifdef DEBUG
   lockOwner(nullptr),
#endif
   consumerWakeup(nullptr),
   producerWakeup(nullptr),
   pauseWakeup(nullptr),
   numAsmJSFailedJobs(0),
   asmJSFailedFunction(nullptr)
{
    cpuCount = GetCPUCount();
    threadCount = ThreadCountForCPUCount(cpuCount);

    MOZ_ASSERT(cpuCount > 0, "GetCPUCount() seems broken");

    helperLock = PR_NewLock();
    consumerWakeup = PR_NewCondVar(helperLock);
    producerWakeup = PR_NewCondVar(helperLock);
    pauseWakeup = PR_NewCondVar(helperLock);
}

void
GlobalHelperThreadState::finish()
{
    if (threads) {
        MOZ_ASSERT(CanUseExtraThreads());
        for (size_t i = 0; i < threadCount; i++)
            threads[i].destroy();
        js_free(threads);
    }

    PR_DestroyCondVar(consumerWakeup);
    PR_DestroyCondVar(producerWakeup);
    PR_DestroyCondVar(pauseWakeup);
    PR_DestroyLock(helperLock);

    ionLazyLinkList_.clear();
}

void
GlobalHelperThreadState::lock()
{
    MOZ_ASSERT(!isLocked());
    AssertCurrentThreadCanLock(HelperThreadStateLock);
    PR_Lock(helperLock);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
GlobalHelperThreadState::unlock()
{
    MOZ_ASSERT(isLocked());
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
    MOZ_ASSERT(isLocked());
#ifdef DEBUG
    lockOwner = nullptr;
#endif
    DebugOnly<PRStatus> status =
        PR_WaitCondVar(whichWakeup(which),
                       millis ? PR_MillisecondsToInterval(millis) : PR_INTERVAL_NO_TIMEOUT);
    MOZ_ASSERT(status == PR_SUCCESS);
#ifdef DEBUG
    lockOwner = PR_GetCurrentThread();
#endif
}

void
GlobalHelperThreadState::notifyAll(CondVar which)
{
    MOZ_ASSERT(isLocked());
    PR_NotifyAllCondVar(whichWakeup(which));
}

void
GlobalHelperThreadState::notifyOne(CondVar which)
{
    MOZ_ASSERT(isLocked());
    PR_NotifyCondVar(whichWakeup(which));
}

bool
GlobalHelperThreadState::canStartAsmJSCompile()
{
    
    MOZ_ASSERT(isLocked());
    if (asmJSWorklist().empty() || numAsmJSFailedJobs)
        return false;

    
    
    size_t numAsmJSThreads = 0;
    for (size_t i = 0; i < threadCount; i++) {
        if (threads[i].asmData)
            numAsmJSThreads++;
    }
    if (numAsmJSThreads >= maxAsmJSCompilationThreads())
        return false;

    return true;
}

static bool
IonBuilderHasHigherPriority(jit::IonBuilder* first, jit::IonBuilder* second)
{
    
    

    
    if (first->optimizationInfo().level() != second->optimizationInfo().level())
        return first->optimizationInfo().level() < second->optimizationInfo().level();

    
    if (first->script()->hasIonScript() != second->script()->hasIonScript())
        return !first->script()->hasIonScript();

    
    return first->script()->getWarmUpCount() / first->script()->length() >
           second->script()->getWarmUpCount() / second->script()->length();
}

bool
GlobalHelperThreadState::canStartIonCompile()
{
    return !ionWorklist().empty();
}

jit::IonBuilder*
GlobalHelperThreadState::highestPriorityPendingIonCompile(bool remove )
{
    MOZ_ASSERT(isLocked());

    if (ionWorklist().empty()) {
        MOZ_ASSERT(!remove);
        return nullptr;
    }

    
    size_t index = 0;
    for (size_t i = 1; i < ionWorklist().length(); i++) {
        if (IonBuilderHasHigherPriority(ionWorklist()[i], ionWorklist()[index]))
            index = i;
    }
    jit::IonBuilder* builder = ionWorklist()[index];
    if (remove)
        ionWorklist().erase(&ionWorklist()[index]);
    return builder;
}

HelperThread*
GlobalHelperThreadState::lowestPriorityUnpausedIonCompileAtThreshold()
{
    MOZ_ASSERT(isLocked());

    
    
    
    size_t numBuilderThreads = 0;
    HelperThread* thread = nullptr;
    for (size_t i = 0; i < threadCount; i++) {
        if (threads[i].ionBuilder && !threads[i].pause) {
            numBuilderThreads++;
            if (!thread || IonBuilderHasHigherPriority(thread->ionBuilder, threads[i].ionBuilder))
                thread = &threads[i];
        }
    }
    if (numBuilderThreads < maxIonCompilationThreads())
        return nullptr;
    return thread;
}

HelperThread*
GlobalHelperThreadState::highestPriorityPausedIonCompile()
{
    MOZ_ASSERT(isLocked());

    
    
    HelperThread* thread = nullptr;
    for (size_t i = 0; i < threadCount; i++) {
        if (threads[i].pause) {
            
            MOZ_ASSERT(threads[i].ionBuilder);
            if (!thread || IonBuilderHasHigherPriority(threads[i].ionBuilder, thread->ionBuilder))
                thread = &threads[i];
        }
    }
    return thread;
}

bool
GlobalHelperThreadState::pendingIonCompileHasSufficientPriority()
{
    MOZ_ASSERT(isLocked());

    
    if (!canStartIonCompile())
        return false;

    
    
    HelperThread* lowestPriorityThread = lowestPriorityUnpausedIonCompileAtThreshold();

    
    
    if (!lowestPriorityThread)
        return true;

    
    
    
    if (IonBuilderHasHigherPriority(highestPriorityPendingIonCompile(), lowestPriorityThread->ionBuilder))
        return true;

    
    return false;
}

bool
GlobalHelperThreadState::canStartParseTask()
{
    
    
    
    
    MOZ_ASSERT(isLocked());
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

bool
GlobalHelperThreadState::canStartGCParallelTask()
{
    return !gcParallelWorklist().empty();
}

js::GCParallelTask::~GCParallelTask()
{
    
    
    
    
#ifdef DEBUG
    AutoLockHelperThreadState helperLock;
    MOZ_ASSERT(state == NotStarted);
#endif
}

bool
js::GCParallelTask::startWithLockHeld()
{
    MOZ_ASSERT(HelperThreadState().isLocked());

    
    MOZ_ASSERT(state == NotStarted);

    
    
    
    if (!HelperThreadState().threads)
        return false;

    if (!HelperThreadState().gcParallelWorklist().append(this))
        return false;
    state = Dispatched;

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);

    return true;
}

bool
js::GCParallelTask::start()
{
    AutoLockHelperThreadState helperLock;
    return startWithLockHeld();
}

void
js::GCParallelTask::joinWithLockHeld()
{
    MOZ_ASSERT(HelperThreadState().isLocked());

    if (state == NotStarted)
        return;

    while (state != Finished)
        HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
    state = NotStarted;
    cancel_ = false;
}

void
js::GCParallelTask::join()
{
    AutoLockHelperThreadState helperLock;
    joinWithLockHeld();
}

void
js::GCParallelTask::runFromMainThread(JSRuntime* rt)
{
    MOZ_ASSERT(state == NotStarted);
    MOZ_ASSERT(js::CurrentThreadCanAccessRuntime(rt));
    uint64_t timeStart = PRMJ_Now();
    run();
    duration_ = PRMJ_Now() - timeStart;
}

void
js::GCParallelTask::runFromHelperThread()
{
    MOZ_ASSERT(HelperThreadState().isLocked());

    {
        AutoUnlockHelperThreadState parallelSection;
        uint64_t timeStart = PRMJ_Now();
        run();
        duration_ = PRMJ_Now() - timeStart;
    }

    state = Finished;
    HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);
}

bool
js::GCParallelTask::isRunning() const
{
    MOZ_ASSERT(HelperThreadState().isLocked());
    return state == Dispatched;
}

void
HelperThread::handleGCParallelWorkload()
{
    MOZ_ASSERT(HelperThreadState().isLocked());
    MOZ_ASSERT(HelperThreadState().canStartGCParallelTask());
    MOZ_ASSERT(idle());

    MOZ_ASSERT(!gcParallelTask);
    gcParallelTask = HelperThreadState().gcParallelWorklist().popCopy();
    gcParallelTask->runFromHelperThread();
    gcParallelTask = nullptr;
}

static void
LeaveParseTaskZone(JSRuntime* rt, ParseTask* task)
{
    
    
    task->cx->leaveCompartment(task->cx->compartment());
    rt->clearUsedByExclusiveThread(task->cx->zone());
}

static bool
EnsureConstructor(JSContext* cx, Handle<GlobalObject*> global, JSProtoKey key)
{
    if (!GlobalObject::ensureConstructor(cx, global, key))
        return false;

    return global->getPrototype(key).toObject().setDelegate(cx);
}

JSScript*
GlobalHelperThreadState::finishParseTask(JSContext* maybecx, JSRuntime* rt, void* token)
{
    ScopedJSDeletePtr<ParseTask> parseTask;

    
    
    {
        AutoLockHelperThreadState lock;
        ParseTaskVector& finished = parseFinishedList();
        for (size_t i = 0; i < finished.length(); i++) {
            if (finished[i] == token) {
                parseTask = finished[i];
                remove(finished, &i);
                break;
            }
        }
    }
    MOZ_ASSERT(parseTask);

    if (!maybecx) {
        LeaveParseTaskZone(rt, parseTask);
        return nullptr;
    }

    JSContext* cx = maybecx;
    MOZ_ASSERT(cx->compartment());

    
    
    Rooted<GlobalObject*> global(cx, &cx->global()->as<GlobalObject>());
    if (!EnsureConstructor(cx, global, JSProto_Object) ||
        !EnsureConstructor(cx, global, JSProto_Array) ||
        !EnsureConstructor(cx, global, JSProto_Function) ||
        !EnsureConstructor(cx, global, JSProto_RegExp) ||
        !EnsureConstructor(cx, global, JSProto_Iterator))
    {
        LeaveParseTaskZone(rt, parseTask);
        return nullptr;
    }

    LeaveParseTaskZone(rt, parseTask);

    
    
    
    
    
    
    gc::AutoFinishGC finishGC(rt);
    for (gc::ZoneCellIter iter(parseTask->cx->zone(), gc::AllocKind::OBJECT_GROUP);
         !iter.done();
         iter.next())
    {
        JS::AutoAssertNoAlloc noAlloc(rt);
        ObjectGroup* group = iter.get<ObjectGroup>();
        TaggedProto proto(group->proto());
        if (!proto.isObject())
            continue;

        JSProtoKey key = JS::IdentifyStandardPrototype(proto.toObject());
        if (key == JSProto_Null)
            continue;
        MOZ_ASSERT(key == JSProto_Object || key == JSProto_Array ||
                   key == JSProto_Function || key == JSProto_RegExp ||
                   key == JSProto_Iterator);

        JSObject* newProto = GetBuiltinPrototypePure(global, key);
        MOZ_ASSERT(newProto);

        group->setProtoUnchecked(TaggedProto(newProto));
    }

    
    gc::MergeCompartments(parseTask->cx->compartment(), cx->compartment());
    if (!parseTask->finish(cx))
        return nullptr;

    RootedScript script(rt, parseTask->script);
    assertSameCompartment(cx, script);

    
    
    for (size_t i = 0; i < parseTask->errors.length(); i++)
        parseTask->errors[i]->throwError(cx);
    if (parseTask->overRecursed)
        ReportOverRecursed(cx);
    if (cx->isExceptionPending())
        return nullptr;

    if (script) {
        
        Debugger::onNewScript(cx, script);

        
        
        if (script->scriptSource()->hasCompressedSource())
            script->scriptSource()->updateCompressedSourceSet(rt);
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

    threadData.reset();
}

#ifdef MOZ_NUWA_PROCESS
extern "C" {
MFBT_API bool IsNuwaProcess();
MFBT_API void NuwaMarkCurrentThread(void (*recreate)(void*), void* arg);
}
#endif


void
HelperThread::ThreadMain(void* arg)
{
    PR_SetCurrentThreadName("Analysis Helper");

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        MOZ_ASSERT(NuwaMarkCurrentThread != nullptr);
        NuwaMarkCurrentThread(nullptr, nullptr);
    }
#endif

    
    
    
    
    FIX_FPU();

    static_cast<HelperThread*>(arg)->threadLoop();
}

void
HelperThread::handleAsmJSWorkload()
{
    MOZ_ASSERT(HelperThreadState().isLocked());
    MOZ_ASSERT(HelperThreadState().canStartAsmJSCompile());
    MOZ_ASSERT(idle());

    asmData = HelperThreadState().asmJSWorklist().popCopy();
    bool success = false;

    do {
        AutoUnlockHelperThreadState unlock;
        PerThreadData::AutoEnterRuntime enter(threadData.ptr(), asmData->runtime);

        jit::JitContext jcx(asmData->mir->compartment->runtime(),
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
}

void
HelperThread::handleIonWorkload()
{
    MOZ_ASSERT(HelperThreadState().isLocked());
    MOZ_ASSERT(HelperThreadState().canStartIonCompile());
    MOZ_ASSERT(idle());

    
    
    jit::IonBuilder* builder =
        HelperThreadState().highestPriorityPendingIonCompile( true);

    
    
    
    
    
    if (HelperThread* other = HelperThreadState().lowestPriorityUnpausedIonCompileAtThreshold()) {
        MOZ_ASSERT(other->ionBuilder && !other->pause);
        other->pause = true;
    }

    ionBuilder = builder;
    ionBuilder->setPauseFlag(&pause);

    TraceLoggerThread* logger = TraceLoggerForCurrentThread();
    TraceLoggerEvent event(logger, TraceLogger_AnnotateScripts, ionBuilder->script());
    AutoTraceLog logScript(logger, event);
    AutoTraceLog logCompile(logger, TraceLogger_IonCompilation);

    JSRuntime* rt = ionBuilder->script()->compartment()->runtimeFromAnyThread();

    {
        AutoUnlockHelperThreadState unlock;
        PerThreadData::AutoEnterRuntime enter(threadData.ptr(),
                                              ionBuilder->script()->runtimeFromAnyThread());
        jit::JitContext jctx(jit::CompileRuntime::get(rt),
                             jit::CompileCompartment::get(ionBuilder->script()->compartment()),
                             &ionBuilder->alloc());
        ionBuilder->setBackgroundCodegen(jit::CompileBackEnd(ionBuilder));
    }

    FinishOffThreadIonCompile(ionBuilder);
    ionBuilder = nullptr;
    pause = false;

    
    
    
    
    rt->requestInterrupt(JSRuntime::RequestInterruptCanWait);

    
    HelperThreadState().notifyAll(GlobalHelperThreadState::CONSUMER);

    
    
    
    
    
    
    
    if (HelperThread* other = HelperThreadState().highestPriorityPausedIonCompile()) {
        MOZ_ASSERT(other->ionBuilder && other->pause);

        
        
        jit::IonBuilder* builder = HelperThreadState().highestPriorityPendingIonCompile();
        if (!builder || IonBuilderHasHigherPriority(other->ionBuilder, builder)) {
            other->pause = false;

            
            
            HelperThreadState().notifyAll(GlobalHelperThreadState::PAUSE);
        }
    }
}

static HelperThread*
CurrentHelperThread()
{
    PRThread* prThread = PR_GetCurrentThread();
    HelperThread* thread = nullptr;
    for (size_t i = 0; i < HelperThreadState().threadCount; i++) {
        if (prThread == HelperThreadState().threads[i].thread) {
            thread = &HelperThreadState().threads[i];
            break;
        }
    }
    MOZ_ASSERT(thread);
    return thread;
}

void
js::PauseCurrentHelperThread()
{
    TraceLoggerThread* logger = TraceLoggerForCurrentThread();
    AutoTraceLog logPaused(logger, TraceLogger_IonCompilationPaused);

    HelperThread* thread = CurrentHelperThread();

    AutoLockHelperThreadState lock;
    while (thread->pause)
        HelperThreadState().wait(GlobalHelperThreadState::PAUSE);
}

void
ExclusiveContext::setHelperThread(HelperThread* thread)
{
    helperThread_ = thread;
    perThreadData = thread->threadData.ptr();
}

frontend::CompileError&
ExclusiveContext::addPendingCompileError()
{
    frontend::CompileError* error = js_new<frontend::CompileError>();
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
    MOZ_ASSERT(HelperThreadState().isLocked());
    MOZ_ASSERT(HelperThreadState().canStartParseTask());
    MOZ_ASSERT(idle());

    parseTask = HelperThreadState().parseWorklist().popCopy();
    parseTask->cx->setHelperThread(this);

    {
        AutoUnlockHelperThreadState unlock;
        PerThreadData::AutoEnterRuntime enter(threadData.ptr(),
                                              parseTask->exclusiveContextGlobal->runtimeFromAnyThread());
        SourceBufferHolder srcBuf(parseTask->chars, parseTask->length,
                                  SourceBufferHolder::NoOwnership);
        parseTask->script = frontend::CompileScript(parseTask->cx, &parseTask->alloc,
                                                    NullPtr(), NullPtr(), NullPtr(),
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
    MOZ_ASSERT(HelperThreadState().isLocked());
    MOZ_ASSERT(HelperThreadState().canStartCompressionTask());
    MOZ_ASSERT(idle());

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
js::StartOffThreadCompression(ExclusiveContext* cx, SourceCompressionTask* task)
{
    AutoLockHelperThreadState lock;

    if (!HelperThreadState().compressionWorklist().append(task)) {
        if (JSContext* maybecx = cx->maybeJSContext())
            ReportOutOfMemory(maybecx);
        return false;
    }

    HelperThreadState().notifyOne(GlobalHelperThreadState::PRODUCER);
    return true;
}

bool
GlobalHelperThreadState::compressionInProgress(SourceCompressionTask* task)
{
    MOZ_ASSERT(isLocked());
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
        MOZ_ASSERT(!compressed);
        return true;
    }

    {
        AutoLockHelperThreadState lock;
        while (HelperThreadState().compressionInProgress(this))
            HelperThreadState().wait(GlobalHelperThreadState::CONSUMER);
    }

    if (result == Success) {
        ss->setCompressedSource(cx->isJSContext() ? cx->asJSContext()->runtime() : nullptr,
                                compressed, compressedBytes, compressedHash);

        
        cx->updateMallocCounter(ss->computedSizeOfData());
    } else {
        js_free(compressed);

        if (result == OOM)
            ReportOutOfMemory(cx);
        else if (result == Aborted && !ss->ensureOwnsSource(cx))
            result = OOM;
    }

    ss = nullptr;
    compressed = nullptr;
    MOZ_ASSERT(!active());

    return result != OOM;
}

SourceCompressionTask*
GlobalHelperThreadState::compressionTaskForSource(ScriptSource* ss)
{
    MOZ_ASSERT(isLocked());
    for (size_t i = 0; i < compressionWorklist().length(); i++) {
        SourceCompressionTask* task = compressionWorklist()[i];
        if (task->source() == ss)
            return task;
    }
    for (size_t i = 0; i < threadCount; i++) {
        SourceCompressionTask* task = threads[i].compressionTask;
        if (task && task->source() == ss)
            return task;
    }
    return nullptr;
}

void
HelperThread::handleGCHelperWorkload()
{
    MOZ_ASSERT(HelperThreadState().isLocked());
    MOZ_ASSERT(HelperThreadState().canStartGCHelperTask());
    MOZ_ASSERT(idle());

    MOZ_ASSERT(!gcHelperState);
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
    MOZ_ASSERT(CanUseExtraThreads());

    JS::AutoSuppressGCAnalysis nogc;
    AutoLockHelperThreadState lock;

    js::TlsPerThreadData.set(threadData.ptr());

    
    uintptr_t stackLimit = GetNativeStackBase();
#if JS_STACK_GROWTH_DIRECTION > 0
    stackLimit += HELPER_STACK_QUOTA;
#else
    stackLimit -= HELPER_STACK_QUOTA;
#endif
    for (size_t i = 0; i < ArrayLength(threadData->nativeStackLimit); i++)
        threadData->nativeStackLimit[i] = stackLimit;

    while (true) {
        MOZ_ASSERT(idle());

        
        
        
        bool ionCompile = false;
        while (true) {
            if (terminate)
                return;
            if (HelperThreadState().canStartAsmJSCompile() ||
                (ionCompile = HelperThreadState().pendingIonCompileHasSufficientPriority()) ||
                HelperThreadState().canStartParseTask() ||
                HelperThreadState().canStartCompressionTask() ||
                HelperThreadState().canStartGCHelperTask() ||
                HelperThreadState().canStartGCParallelTask())
            {
                break;
            }
            HelperThreadState().wait(GlobalHelperThreadState::PRODUCER);
        }

        
        if (HelperThreadState().canStartAsmJSCompile())
            handleAsmJSWorkload();
        else if (ionCompile)
            handleIonWorkload();
        else if (HelperThreadState().canStartParseTask())
            handleParseWorkload();
        else if (HelperThreadState().canStartCompressionTask())
            handleCompressionWorkload();
        else if (HelperThreadState().canStartGCHelperTask())
            handleGCHelperWorkload();
        else if (HelperThreadState().canStartGCParallelTask())
            handleGCParallelWorkload();
        else
            MOZ_CRASH("No task to perform");
    }
}
