





#include "vm/TraceLogging.h"

#include "mozilla/DebugOnly.h"

#include <string.h>

#include "jsapi.h"
#include "jsprf.h"
#include "jsscript.h"

#include "jit/BaselineJIT.h"
#include "jit/CompileWrappers.h"
#include "vm/Runtime.h"
#include "vm/TraceLoggingGraph.h"

#include "jit/IonFrames-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;
using mozilla::NativeEndian;

TraceLoggerThreadState traceLoggers;

#if defined(_WIN32)
#include <intrin.h>
static __inline uint64_t
rdtsc(void)
{
    return __rdtsc();
}
#elif defined(__i386__)
static __inline__ uint64_t
rdtsc(void)
{
    uint64_t x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)
static __inline__ uint64_t
rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}
#elif defined(__powerpc__)
static __inline__ uint64_t
rdtsc(void)
{
    uint64_t result=0;
    uint32_t upper, lower,tmp;
    __asm__ volatile(
            "0:                  \n"
            "\tmftbu   %0           \n"
            "\tmftb    %1           \n"
            "\tmftbu   %2           \n"
            "\tcmpw    %2,%0        \n"
            "\tbne     0b         \n"
            : "=r"(upper),"=r"(lower),"=r"(tmp)
            );
    result = upper;
    result = result<<32;
    result = result|lower;

    return result;
}
#else
static __inline__ uint64_t
rdtsc(void)
{
    return 0;
}
#endif

class AutoTraceLoggerThreadStateLock
{
  TraceLoggerThreadState *logging;

  public:
    AutoTraceLoggerThreadStateLock(TraceLoggerThreadState *logging MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : logging(logging)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        PR_Lock(logging->lock);
    }
    ~AutoTraceLoggerThreadStateLock() {
        PR_Unlock(logging->lock);
    }
  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

TraceLoggerThread::TraceLoggerThread()
  : enabled(0),
    failed(false),
    graph(),
    iteration_(0),
    top(nullptr)
{ }

bool
TraceLoggerThread::init()
{
    if (!pointerMap.init())
        return false;
    if (!extraTextId.init())
        return false;
    if (!events.init())
        return false;

    
    
    
    if (!events.ensureSpaceBeforeAdd(3))
        return false;

    enabled = 1;
    logTimestamp(TraceLogger_Enable);

    return true;
}

void
TraceLoggerThread::initGraph()
{
    
    
    
    graph.reset(js_new<TraceLoggerGraph>());
    if (!graph.get())
        return;

    uint64_t start = rdtsc() - traceLoggers.startupTime;
    if (!graph->init(start)) {
        graph = nullptr;
        return;
    }

    
    for (uint32_t i = 0; i < TraceLogger_LastTreeItem; i++) {
        TraceLoggerTextId id = TraceLoggerTextId(i);
        graph->addTextId(i, TLTextIdString(id));
    }
    graph->addTextId(TraceLogger_LastTreeItem, "TraceLogger internal");
    for (uint32_t i = TraceLogger_LastTreeItem + 1; i < TraceLogger_Last; i++) {
        TraceLoggerTextId id = TraceLoggerTextId(i);
        graph->addTextId(i, TLTextIdString(id));
    }
}

TraceLoggerThread::~TraceLoggerThread()
{
    if (graph.get()) {
        if (!failed)
            graph->log(events);
        graph = nullptr;
    }

    for (TextIdHashMap::Range r = extraTextId.all(); !r.empty(); r.popFront())
        js_delete(r.front().value());
    extraTextId.finish();
    pointerMap.finish();
}

bool
TraceLoggerThread::enable()
{
    if (enabled > 0) {
        enabled++;
        return true;
    }

    if (failed)
        return false;

    enabled = 1;
    logTimestamp(TraceLogger_Enable);

    return true;
}

bool
TraceLoggerThread::enable(JSContext *cx)
{
    if (!enable())
        return false;

    if (enabled == 1) {
        
        ActivationIterator iter(cx->runtime());
        Activation *act = iter.activation();

        if (!act) {
            failed = true;
            enabled = 0;
            return false;
        }

        JSScript *script = nullptr;
        int32_t engine = 0;

        if (act->isJit()) {
            JitFrameIterator it(iter);

            while (!it.isScripted() && !it.done())
                ++it;

            MOZ_ASSERT(!it.done());
            MOZ_ASSERT(it.isIonJS() || it.isBaselineJS());

            script = it.script();
            engine = it.isIonJS() ? TraceLogger_IonMonkey : TraceLogger_Baseline;
        } else {
            MOZ_ASSERT(act->isInterpreter());
            InterpreterFrame *fp = act->asInterpreter()->current();
            MOZ_ASSERT(!fp->runningInJit());

            script = fp->script();
            engine = TraceLogger_Interpreter;
            if (script->compartment() != cx->compartment()) {
                failed = true;
                enabled = 0;
                return false;
            }
        }

        TraceLoggerEvent event(this, TraceLogger_Scripts, script);
        startEvent(event);
        startEvent(engine);
    }

    return true;
}

bool
TraceLoggerThread::disable()
{
    if (failed)
        return false;

    if (enabled == 0)
        return true;

    if (enabled > 1) {
        enabled--;
        return true;
    }

    logTimestamp(TraceLogger_Disable);
    enabled = 0;

    return true;
}

const char *
TraceLoggerThread::eventText(uint32_t id)
{
    if (id < TraceLogger_Last)
        return TLTextIdString(static_cast<TraceLoggerTextId>(id));

    TextIdHashMap::Ptr p = extraTextId.lookup(id);
    MOZ_ASSERT(p);

    return p->value()->string();
}

bool
TraceLoggerThread::textIdIsScriptEvent(uint32_t id)
{
    if (id < TraceLogger_Last)
        return false;

    
    const char *str = eventText(id);
    return EqualChars(str, "script", 6);
}

void
TraceLoggerThread::extractScriptDetails(uint32_t textId, const char **filename, size_t *filename_len,
                                        const char **lineno, size_t *lineno_len, const char **colno,
                                        size_t *colno_len)
{
    MOZ_ASSERT(textIdIsScriptEvent(textId));

    const char *script = eventText(textId);

    
    MOZ_ASSERT(EqualChars(script, "script ", 7));
    *filename = script + 7;

    
    *lineno = script;
    *colno = script;
    const char *next = script - 1;
    while ((next = strchr(next + 1, ':'))) {
        *lineno = *colno;
        *colno = next;
    }

    MOZ_ASSERT(*lineno && *lineno != script);
    MOZ_ASSERT(*colno && *colno != script);

    
    *lineno = *lineno + 1;
    *colno = *colno + 1;

    *filename_len = *lineno - *filename - 1;
    *lineno_len = *colno - *lineno - 1;
    *colno_len = strlen(*colno);
}

TraceLoggerEventPayload *
TraceLoggerThread::getOrCreateEventPayload(TraceLoggerTextId textId)
{
    TextIdHashMap::AddPtr p = extraTextId.lookupForAdd(textId);
    if (p)
        return p->value();

    TraceLoggerEventPayload *payload = js_new<TraceLoggerEventPayload>(textId, (char *)nullptr);

    if (!extraTextId.add(p, textId, payload))
        return nullptr;

    return payload;
}

TraceLoggerEventPayload *
TraceLoggerThread::getOrCreateEventPayload(const char *text)
{
    PointerHashMap::AddPtr p = pointerMap.lookupForAdd((const void *)text);
    if (p)
        return p->value();

    size_t len = strlen(text);
    char *str = js_pod_malloc<char>(len + 1);
    if (!str)
        return nullptr;

    DebugOnly<size_t> ret = JS_snprintf(str, len + 1, "%s", text);
    MOZ_ASSERT(ret == len);

    uint32_t textId = extraTextId.count() + TraceLogger_Last;

    TraceLoggerEventPayload *payload = js_new<TraceLoggerEventPayload>(textId, str);
    if (!payload) {
        js_free(str);
        return nullptr;
    }

    if (!extraTextId.putNew(textId, payload)) {
        js_delete(payload);
        return nullptr;
    }

    if (!pointerMap.add(p, text, payload))
        return nullptr;

    if (graph.get())
        graph->addTextId(textId, str);

    return payload;
}

TraceLoggerEventPayload *
TraceLoggerThread::getOrCreateEventPayload(TraceLoggerTextId type, const char *filename,
                                           size_t lineno, size_t colno, const void *ptr)
{
    MOZ_ASSERT(type == TraceLogger_Scripts || type == TraceLogger_AnnotateScripts ||
               type == TraceLogger_InlinedScripts);

    if (!filename)
        filename = "<unknown>";

    
    
    if (!traceLoggers.isTextIdEnabled(type))
        return getOrCreateEventPayload(type);

    PointerHashMap::AddPtr p = pointerMap.lookupForAdd(ptr);
    if (p)
        return p->value();

    
    size_t lenFilename = strlen(filename);
    size_t lenLineno = 1;
    for (size_t i = lineno; i /= 10; lenLineno++);
    size_t lenColno = 1;
    for (size_t i = colno; i /= 10; lenColno++);

    size_t len = 7 + lenFilename + 1 + lenLineno + 1 + lenColno;
    char *str = js_pod_malloc<char>(len + 1);
    if (!str)
        return nullptr;

    DebugOnly<size_t> ret =
        JS_snprintf(str, len + 1, "script %s:%u:%u", filename, lineno, colno);
    MOZ_ASSERT(ret == len);

    uint32_t textId = extraTextId.count() + TraceLogger_Last;
    TraceLoggerEventPayload *payload = js_new<TraceLoggerEventPayload>(textId, str);
    if (!payload) {
        js_free(str);
        return nullptr;
    }

    if (!extraTextId.putNew(textId, payload)) {
        js_delete(payload);
        return nullptr;
    }

    if (!pointerMap.add(p, ptr, payload))
        return nullptr;

    if (graph.get())
        graph->addTextId(textId, str);

    return payload;
}

TraceLoggerEventPayload *
TraceLoggerThread::getOrCreateEventPayload(TraceLoggerTextId type, JSScript *script)
{
    return getOrCreateEventPayload(type, script->filename(), script->lineno(), script->column(),
                                   script);
}

TraceLoggerEventPayload *
TraceLoggerThread::getOrCreateEventPayload(TraceLoggerTextId type,
                                           const JS::ReadOnlyCompileOptions &script)
{
    return getOrCreateEventPayload(type, script.filename(), script.lineno, script.column, &script);
}

void
TraceLoggerThread::startEvent(TraceLoggerTextId id) {
    startEvent(uint32_t(id));
}

void
TraceLoggerThread::startEvent(const TraceLoggerEvent &event) {
    if (!event.hasPayload()) {
        startEvent(TraceLogger_Error);
        return;
    }
    startEvent(event.payload()->textId());
}

void
TraceLoggerThread::startEvent(uint32_t id)
{
    MOZ_ASSERT(TLTextIdIsTreeEvent(id) || id == TraceLogger_Error);
    if (!traceLoggers.isTextIdEnabled(id))
       return;

    logTimestamp(id);
}

void
TraceLoggerThread::stopEvent(TraceLoggerTextId id) {
    stopEvent(uint32_t(id));
}

void
TraceLoggerThread::stopEvent(const TraceLoggerEvent &event) {
    if (!event.hasPayload()) {
        stopEvent(TraceLogger_Error);
        return;
    }
    stopEvent(event.payload()->textId());
}

void
TraceLoggerThread::stopEvent(uint32_t id)
{
    MOZ_ASSERT(TLTextIdIsTreeEvent(id) || id == TraceLogger_Error);
    if (!traceLoggers.isTextIdEnabled(id))
        return;

    logTimestamp(TraceLogger_Stop);
}

void
TraceLoggerThread::logTimestamp(TraceLoggerTextId id)
{
    logTimestamp(uint32_t(id));
}

void
TraceLoggerThread::logTimestamp(uint32_t id)
{
    if (enabled == 0)
        return;

    if (!events.ensureSpaceBeforeAdd()) {
        uint64_t start = 0;

        if (graph.get()) {
            start = rdtsc() - traceLoggers.startupTime;
            graph->log(events);
        }

        iteration_++;
        events.clear();

        
        
        if (graph.get()) {
            MOZ_ASSERT(events.capacity() > 2);
            EventEntry &entryStart = events.pushUninitialized();
            entryStart.time = start;
            entryStart.textId = TraceLogger_Internal;

            EventEntry &entryStop = events.pushUninitialized();
            entryStop.time = rdtsc() - traceLoggers.startupTime;
            entryStop.textId = TraceLogger_Stop;
        }

        
        for (TextIdHashMap::Enum e(extraTextId); !e.empty(); e.popFront()) {
            if (e.front().value()->uses() == 0) {
                js_delete(e.front().value());
                e.removeFront();
            }
        }
    }

    uint64_t time = graph.get() ? rdtsc() - traceLoggers.startupTime : 0;

    EventEntry &entry = events.pushUninitialized();
    entry.time = time;
    entry.textId = id;
}

TraceLoggerThreadState::TraceLoggerThreadState()
{
    initialized = false;
    enabled = 0;
    mainThreadEnabled = false;
    offThreadEnabled = false;
    graphSpewingEnabled = false;

    lock = PR_NewLock();
    if (!lock)
        MOZ_CRASH();
}

TraceLoggerThreadState::~TraceLoggerThreadState()
{
    for (size_t i = 0; i < mainThreadLoggers.length(); i++)
        js_delete(mainThreadLoggers[i]);

    mainThreadLoggers.clear();

    if (threadLoggers.initialized()) {
        for (ThreadLoggerHashMap::Range r = threadLoggers.all(); !r.empty(); r.popFront())
            js_delete(r.front().value());

        threadLoggers.finish();
    }

    if (lock) {
        PR_DestroyLock(lock);
        lock = nullptr;
    }

    enabled = 0;
}

static bool
ContainsFlag(const char *str, const char *flag)
{
    size_t flaglen = strlen(flag);
    const char *index = strstr(str, flag);
    while (index) {
        if ((index == str || index[-1] == ',') && (index[flaglen] == 0 || index[flaglen] == ','))
            return true;
        index = strstr(index + flaglen, flag);
    }
    return false;
}

bool
TraceLoggerThreadState::lazyInit()
{
    if (initialized)
        return enabled > 0;

    initialized = true;

    if (!threadLoggers.init())
        return false;

    const char *env = getenv("TLLOG");
    if (!env)
        env = "";

    if (strstr(env, "help")) {
        fflush(nullptr);
        printf(
            "\n"
            "usage: TLLOG=option,option,option,... where options can be:\n"
            "\n"
            "Collections:\n"
            "  Default        Output all default\n"
            "  IonCompiler    Output all information about compilation\n"
            "\n"
            "Specific log items:\n"
        );
        for (uint32_t i = 1; i < TraceLogger_Last; i++) {
            TraceLoggerTextId id = TraceLoggerTextId(i);
            if (!TLTextIdIsToggable(id))
                continue;
            printf("  %s\n", TLTextIdString(id));
        }
        printf("\n");
        exit(0);
        
    }

    for (uint32_t i = 1; i < TraceLogger_Last; i++) {
        TraceLoggerTextId id = TraceLoggerTextId(i);
        if (TLTextIdIsToggable(id))
            enabledTextIds[i] = ContainsFlag(env, TLTextIdString(id));
        else
            enabledTextIds[i] = true;
    }

    if (ContainsFlag(env, "Default")) {
        enabledTextIds[TraceLogger_AnnotateScripts] = true;
        enabledTextIds[TraceLogger_Bailout] = true;
        enabledTextIds[TraceLogger_Baseline] = true;
        enabledTextIds[TraceLogger_BaselineCompilation] = true;
        enabledTextIds[TraceLogger_GC] = true;
        enabledTextIds[TraceLogger_GCAllocation] = true;
        enabledTextIds[TraceLogger_GCSweeping] = true;
        enabledTextIds[TraceLogger_Interpreter] = true;
        enabledTextIds[TraceLogger_IonCompilation] = true;
        enabledTextIds[TraceLogger_IonLinking] = true;
        enabledTextIds[TraceLogger_IonMonkey] = true;
        enabledTextIds[TraceLogger_MinorGC] = true;
        enabledTextIds[TraceLogger_ParserCompileFunction] = true;
        enabledTextIds[TraceLogger_ParserCompileLazy] = true;
        enabledTextIds[TraceLogger_ParserCompileScript] = true;
        enabledTextIds[TraceLogger_IrregexpCompile] = true;
        enabledTextIds[TraceLogger_IrregexpExecute] = true;
        enabledTextIds[TraceLogger_Scripts] = true;
        enabledTextIds[TraceLogger_Engine] = true;
    }

    if (ContainsFlag(env, "IonCompiler")) {
        enabledTextIds[TraceLogger_IonCompilation] = true;
        enabledTextIds[TraceLogger_IonLinking] = true;
        enabledTextIds[TraceLogger_FoldTests] = true;
        enabledTextIds[TraceLogger_SplitCriticalEdges] = true;
        enabledTextIds[TraceLogger_RenumberBlocks] = true;
        enabledTextIds[TraceLogger_DominatorTree] = true;
        enabledTextIds[TraceLogger_PhiAnalysis] = true;
        enabledTextIds[TraceLogger_ApplyTypes] = true;
        enabledTextIds[TraceLogger_ParallelSafetyAnalysis] = true;
        enabledTextIds[TraceLogger_AliasAnalysis] = true;
        enabledTextIds[TraceLogger_GVN] = true;
        enabledTextIds[TraceLogger_LICM] = true;
        enabledTextIds[TraceLogger_RangeAnalysis] = true;
        enabledTextIds[TraceLogger_LoopUnrolling] = true;
        enabledTextIds[TraceLogger_EffectiveAddressAnalysis] = true;
        enabledTextIds[TraceLogger_EliminateDeadCode] = true;
        enabledTextIds[TraceLogger_EdgeCaseAnalysis] = true;
        enabledTextIds[TraceLogger_EliminateRedundantChecks] = true;
        enabledTextIds[TraceLogger_GenerateLIR] = true;
        enabledTextIds[TraceLogger_RegisterAllocation] = true;
        enabledTextIds[TraceLogger_GenerateCode] = true;
        enabledTextIds[TraceLogger_Scripts] = true;
    }

    enabledTextIds[TraceLogger_Interpreter] = enabledTextIds[TraceLogger_Engine];
    enabledTextIds[TraceLogger_Baseline] = enabledTextIds[TraceLogger_Engine];
    enabledTextIds[TraceLogger_IonMonkey] = enabledTextIds[TraceLogger_Engine];

    const char *options = getenv("TLOPTIONS");
    if (options) {
        if (strstr(options, "help")) {
            fflush(nullptr);
            printf(
                "\n"
                "usage: TLOPTIONS=option,option,option,... where options can be:\n"
                "\n"
                "  EnableMainThread        Start logging the main thread immediately.\n"
                "  EnableOffThread         Start logging helper threads immediately.\n"
                "  EnableGraph             Enable spewing the tracelogging graph to a file.\n"
            );
            printf("\n");
            exit(0);
            
        }

        if (strstr(options, "EnableMainThread"))
           mainThreadEnabled = true;
        if (strstr(options, "EnableOffThread"))
           offThreadEnabled = true;
        if (strstr(options, "EnableGraph"))
           graphSpewingEnabled = true;
    }

    startupTime = rdtsc();
    enabled = 1;
    return true;
}

void
TraceLoggerThreadState::enableTextId(JSContext *cx, uint32_t textId)
{
    MOZ_ASSERT(TLTextIdIsToggable(textId));

    if (enabledTextIds[textId])
        return;

    enabledTextIds[textId] = true;
    if (textId == TraceLogger_Engine) {
        enabledTextIds[TraceLogger_IonMonkey] = true;
        enabledTextIds[TraceLogger_Baseline] = true;
        enabledTextIds[TraceLogger_Interpreter] = true;
    }

    ReleaseAllJITCode(cx->runtime()->defaultFreeOp());

    if (textId == TraceLogger_Scripts)
        jit::ToggleBaselineTraceLoggerScripts(cx->runtime(), true);
    if (textId == TraceLogger_Engine)
        jit::ToggleBaselineTraceLoggerEngine(cx->runtime(), true);

}
void
TraceLoggerThreadState::disableTextId(JSContext *cx, uint32_t textId)
{
    MOZ_ASSERT(TLTextIdIsToggable(textId));

    if (!enabledTextIds[textId])
        return;

    enabledTextIds[textId] = false;
    if (textId == TraceLogger_Engine) {
        enabledTextIds[TraceLogger_IonMonkey] = false;
        enabledTextIds[TraceLogger_Baseline] = false;
        enabledTextIds[TraceLogger_Interpreter] = false;
    }

    ReleaseAllJITCode(cx->runtime()->defaultFreeOp());

    if (textId == TraceLogger_Scripts)
        jit::ToggleBaselineTraceLoggerScripts(cx->runtime(), false);
    if (textId == TraceLogger_Engine)
        jit::ToggleBaselineTraceLoggerEngine(cx->runtime(), false);
}


TraceLoggerThread *
js::TraceLoggerForMainThread(CompileRuntime *runtime)
{
    return traceLoggers.forMainThread(runtime);
}

TraceLoggerThread *
TraceLoggerThreadState::forMainThread(CompileRuntime *runtime)
{
    return forMainThread(runtime->mainThread());
}

TraceLoggerThread *
js::TraceLoggerForMainThread(JSRuntime *runtime)
{
    return traceLoggers.forMainThread(runtime);
}

TraceLoggerThread *
TraceLoggerThreadState::forMainThread(JSRuntime *runtime)
{
    return forMainThread(&runtime->mainThread);
}

TraceLoggerThread *
TraceLoggerThreadState::forMainThread(PerThreadData *mainThread)
{
    if (!mainThread->traceLogger) {
        AutoTraceLoggerThreadStateLock lock(this);

        if (!lazyInit())
            return nullptr;

        TraceLoggerThread *logger = create();
        if (!logger)
            return nullptr;

        if (!mainThreadLoggers.append(logger)) {
            js_delete(logger);
            return nullptr;
        }

        mainThread->traceLogger = logger;
        if (!mainThreadEnabled)
            logger->disable();
    }

    return mainThread->traceLogger;
}

TraceLoggerThread *
js::TraceLoggerForCurrentThread()
{
    PRThread *thread = PR_GetCurrentThread();
    return traceLoggers.forThread(thread);
}

TraceLoggerThread *
TraceLoggerThreadState::forThread(PRThread *thread)
{
    AutoTraceLoggerThreadStateLock lock(this);

    if (!lazyInit())
        return nullptr;

    ThreadLoggerHashMap::AddPtr p = threadLoggers.lookupForAdd(thread);
    if (p)
        return p->value();

    TraceLoggerThread *logger = create();
    if (!logger)
        return nullptr;

    if (!threadLoggers.add(p, thread, logger)) {
        js_delete(logger);
        return nullptr;
    }

    if (graphSpewingEnabled)
        logger->initGraph();

    if (!offThreadEnabled)
        logger->disable();

    return logger;
}

TraceLoggerThread *
TraceLoggerThreadState::create()
{
    TraceLoggerThread *logger = js_new<TraceLoggerThread>();
    if (!logger)
        return nullptr;

    if (!logger->init()) {
        js_delete(logger);
        return nullptr;
    }

    return logger;
}

bool
js::TraceLogTextIdEnabled(uint32_t textId)
{
    return traceLoggers.isTextIdEnabled(textId);
}

void
js::TraceLogEnableTextId(JSContext *cx, uint32_t textId)
{
    traceLoggers.enableTextId(cx, textId);
}
void
js::TraceLogDisableTextId(JSContext *cx, uint32_t textId)
{
    traceLoggers.disableTextId(cx, textId);
}

TraceLoggerEvent::TraceLoggerEvent(TraceLoggerThread *logger, TraceLoggerTextId textId)
{
    payload_ = logger->getOrCreateEventPayload(textId);
    if (payload_)
        payload_->use();
}

TraceLoggerEvent::TraceLoggerEvent(TraceLoggerThread *logger, TraceLoggerTextId type,
                                   JSScript *script)
{
    payload_ = logger->getOrCreateEventPayload(type, script);
    if (payload_)
        payload_->use();
}

TraceLoggerEvent::TraceLoggerEvent(TraceLoggerThread *logger, TraceLoggerTextId type,
                                   const JS::ReadOnlyCompileOptions &compileOptions)
{
    payload_ = logger->getOrCreateEventPayload(type, compileOptions);
    if (payload_)
        payload_->use();
}

TraceLoggerEvent::TraceLoggerEvent(TraceLoggerThread *logger, const char *text)
{
    payload_ = logger->getOrCreateEventPayload(text);
    if (payload_)
        payload_->use();
}

TraceLoggerEvent::~TraceLoggerEvent()
{
    if (payload_)
        payload_->release();
}
