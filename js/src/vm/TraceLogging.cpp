





#include "vm/TraceLogging.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Endian.h"

#include <string.h>

#include "jsapi.h"
#include "jsscript.h"

#include "jit/CompileWrappers.h"
#include "vm/Runtime.h"

#include "jit/IonFrames-inl.h"

using namespace js;
using namespace js::jit;
using mozilla::NativeEndian;

#ifndef TRACE_LOG_DIR
# if defined(_WIN32)
#  define TRACE_LOG_DIR ""
# else
#  define TRACE_LOG_DIR "/tmp/"
# endif
#endif

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
#endif

TraceLogging traceLoggers;

static const char *
TLTextIdString(TraceLoggerTextId id)
{
    switch (id) {
      case TraceLogger_Error:
        return "TraceLogger failed to process text";
#define NAME(textId) case TraceLogger_ ## textId: return #textId;
        TRACELOGGER_TEXT_ID_LIST(NAME)
#undef NAME
      default:
        MOZ_CRASH();
    }
}

TraceLogger::TraceLogger()
 : enabled(0),
   failed(false),
   nextTextId(0),
   treeOffset(0),
   top(nullptr)
{ }

bool
TraceLogger::init(uint32_t loggerId)
{
    if (!pointerMap.init())
        return false;
    if (!tree.init())
        return false;
    if (!stack.init())
        return false;
    if (!events.init())
        return false;

    MOZ_ASSERT(loggerId <= 999);

    char dictFilename[sizeof TRACE_LOG_DIR "tl-dict.100.json"];
    sprintf(dictFilename, TRACE_LOG_DIR "tl-dict.%d.json", loggerId);
    dictFile = fopen(dictFilename, "w");
    if (!dictFile)
        return false;

    char treeFilename[sizeof TRACE_LOG_DIR "tl-tree.100.tl"];
    sprintf(treeFilename, TRACE_LOG_DIR "tl-tree.%d.tl", loggerId);
    treeFile = fopen(treeFilename, "wb");
    if (!treeFile) {
        fclose(dictFile);
        dictFile = nullptr;
        return false;
    }

    char eventFilename[sizeof TRACE_LOG_DIR "tl-event.100.tl"];
    sprintf(eventFilename, TRACE_LOG_DIR "tl-event.%d.tl", loggerId);
    eventFile = fopen(eventFilename, "wb");
    if (!eventFile) {
        fclose(dictFile);
        fclose(treeFile);
        dictFile = nullptr;
        treeFile = nullptr;
        return false;
    }

    uint64_t start = rdtsc() - traceLoggers.startupTime;

    TreeEntry &treeEntry = tree.pushUninitialized();
    treeEntry.setStart(start);
    treeEntry.setStop(0);
    treeEntry.setTextId(0);
    treeEntry.setHasChildren(false);
    treeEntry.setNextId(0);

    StackEntry &stackEntry = stack.pushUninitialized();
    stackEntry.setTreeId(0);
    stackEntry.setLastChildId(0);
    stackEntry.setActive(true);

    int written = fprintf(dictFile, "[");
    if (written < 0)
        fprintf(stderr, "TraceLogging: Error while writing.\n");

    
    for (uint32_t i = 0; i < TraceLogger_LAST; i++) {
        TraceLoggerTextId id = TraceLoggerTextId(i);
        mozilla::DebugOnly<uint32_t> textId = createTextId(TLTextIdString(id));
        MOZ_ASSERT(textId == i);
    }

    enabled = 1;
    return true;
}

bool
TraceLogger::enable()
{
    if (enabled > 0) {
        enabled++;
        return true;
    }

    if (failed)
        return false;

    enabled = 1;
    return true;
}

bool
TraceLogger::enable(JSContext *cx)
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

        startEvent(createTextId(script));
        startEvent(engine);
    }

    return true;
}

bool
TraceLogger::disable()
{
    if (failed)
        return false;

    if (enabled == 0)
        return true;

    if (enabled > 1) {
        enabled--;
        return true;
    }

    MOZ_ASSERT(enabled == 1);
    while (stack.size() > 1)
        stopEvent();

    enabled = 0;

    return true;
}

bool
TraceLogger::flush()
{
    MOZ_ASSERT(!failed);

    if (treeFile) {
        
        for (size_t i = 0; i < tree.size(); i++)
            entryToBigEndian(&tree[i]);

        int success = fseek(treeFile, 0, SEEK_END);
        if (success != 0)
            return false;

        size_t bytesWritten = fwrite(tree.data(), sizeof(TreeEntry), tree.size(), treeFile);
        if (bytesWritten < tree.size())
            return false;

        treeOffset += tree.lastEntryId();
        tree.clear();
    }

    if (eventFile) {
        
        for (size_t i = 0; i < events.size(); i++) {
            events[i].time = NativeEndian::swapToBigEndian(events[i].time);
            events[i].textId = NativeEndian::swapToBigEndian(events[i].textId);
        }

        size_t bytesWritten = fwrite(events.data(), sizeof(EventEntry), events.size(), eventFile);
        if (bytesWritten < events.size())
            return false;
        events.clear();
    }

    return true;
}

TraceLogger::~TraceLogger()
{
    
    if (dictFile) {
        int written = fprintf(dictFile, "]");
        if (written < 0)
            fprintf(stderr, "TraceLogging: Error while writing.\n");
        fclose(dictFile);

        dictFile = nullptr;
    }

    if (!failed && treeFile) {
        
        
        
        enabled = 1;
        while (stack.size() > 1)
            stopEvent();
        enabled = 0;
    }

    if (!failed && !flush()) {
        fprintf(stderr, "TraceLogging: Couldn't write the data to disk.\n");
        enabled = 0;
        failed = true;
    }

    if (treeFile) {
        fclose(treeFile);
        treeFile = nullptr;
    }

    if (eventFile) {
        fclose(eventFile);
        eventFile = nullptr;
    }
}

uint32_t
TraceLogger::createTextId(const char *text)
{
    assertNoQuotes(text);

    PointerHashMap::AddPtr p = pointerMap.lookupForAdd((const void *)text);
    if (p)
        return p->value();

    uint32_t textId = nextTextId++;
    if (!pointerMap.add(p, text, textId))
        return TraceLogger_Error;

    int written;
    if (textId > 0)
        written = fprintf(dictFile, ",\n\"%s\"", text);
    else
        written = fprintf(dictFile, "\"%s\"", text);

    if (written < 0)
        return TraceLogger_Error;

    return textId;
}

uint32_t
TraceLogger::createTextId(JSScript *script)
{
    if (!script->filename())
        return createTextId("");

    assertNoQuotes(script->filename());

    
    
    if (!traceLoggers.isTextIdEnabled(TraceLogger_Scripts))
        return TraceLogger_Scripts;

    PointerHashMap::AddPtr p = pointerMap.lookupForAdd(script);
    if (p)
        return p->value();

    uint32_t textId = nextTextId++;
    if (!pointerMap.add(p, script, textId))
        return TraceLogger_Error;

    int written;
    if (textId > 0) {
        written = fprintf(dictFile, ",\n\"script %s:%u:%u\"", script->filename(),
                          (unsigned)script->lineno(), (unsigned)script->column());
    } else {
        written = fprintf(dictFile, "\"script %s:%u:%u\"", script->filename(),
                          (unsigned)script->lineno(), (unsigned)script->column());
    }

    if (written < 0)
        return TraceLogger_Error;

    return textId;
}

uint32_t
TraceLogger::createTextId(const JS::ReadOnlyCompileOptions &compileOptions)
{
    if (!compileOptions.filename())
        return createTextId("");

    assertNoQuotes(compileOptions.filename());

    
    
    if (!traceLoggers.isTextIdEnabled(TraceLogger_Scripts))
        return TraceLogger_Scripts;

    PointerHashMap::AddPtr p = pointerMap.lookupForAdd(&compileOptions);
    if (p)
        return p->value();

    uint32_t textId = nextTextId++;
    if (!pointerMap.add(p, &compileOptions, textId))
        return TraceLogger_Error;

    int written;
    if (textId > 0) {
        written = fprintf(dictFile, ",\n\"script %s:%d:%d\"", compileOptions.filename(),
                          compileOptions.lineno, compileOptions.column);
    } else {
        written = fprintf(dictFile, "\"script %s:%d:%d\"", compileOptions.filename(),
                          compileOptions.lineno, compileOptions.column);
    }

    if (written < 0)
        return TraceLogger_Error;

    return textId;
}

void
TraceLogger::logTimestamp(uint32_t id)
{
    if (enabled == 0)
        return;

    if (!events.ensureSpaceBeforeAdd()) {
        fprintf(stderr, "TraceLogging: Disabled a tracelogger due to OOM.\n");
        enabled = 0;
        return;
    }

    uint64_t time = rdtsc() - traceLoggers.startupTime;

    EventEntry &entry = events.pushUninitialized();
    entry.time = time;
    entry.textId = id;
}

void
TraceLogger::entryToBigEndian(TreeEntry *entry)
{
    entry->start_ = NativeEndian::swapToBigEndian(entry->start_);
    entry->stop_ = NativeEndian::swapToBigEndian(entry->stop_);
    uint32_t data = (entry->u.s.textId_ << 1) + entry->u.s.hasChildren_;
    entry->u.value_ = NativeEndian::swapToBigEndian(data);
    entry->nextId_ = NativeEndian::swapToBigEndian(entry->nextId_);
}

void
TraceLogger::entryToSystemEndian(TreeEntry *entry)
{
    entry->start_ = NativeEndian::swapFromBigEndian(entry->start_);
    entry->stop_ = NativeEndian::swapFromBigEndian(entry->stop_);

    uint32_t data = NativeEndian::swapFromBigEndian(entry->u.value_);
    entry->u.s.textId_ = data >> 1;
    entry->u.s.hasChildren_ = data & 0x1;

    entry->nextId_ = NativeEndian::swapFromBigEndian(entry->nextId_);
}

bool
TraceLogger::getTreeEntry(uint32_t treeId, TreeEntry *entry)
{
    
    if (treeId >= treeOffset) {
        *entry = tree[treeId];
        return true;
    }

    int success = fseek(treeFile, treeId * sizeof(TreeEntry), SEEK_SET);
    if (success != 0)
        return false;

    size_t itemsRead = fread((void *)entry, sizeof(TreeEntry), 1, treeFile);
    if (itemsRead < 1)
        return false;

    entryToSystemEndian(entry);
    return true;
}

bool
TraceLogger::saveTreeEntry(uint32_t treeId, TreeEntry *entry)
{
    int success = fseek(treeFile, treeId * sizeof(TreeEntry), SEEK_SET);
    if (success != 0)
        return false;

    entryToBigEndian(entry);

    size_t itemsWritten = fwrite(entry, sizeof(TreeEntry), 1, treeFile);
    if (itemsWritten < 1)
        return false;

    return true;
}

bool
TraceLogger::updateHasChildren(uint32_t treeId, bool hasChildren)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.setHasChildren(hasChildren);
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].setHasChildren(hasChildren);
    return true;
}

bool
TraceLogger::updateNextId(uint32_t treeId, uint32_t nextId)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.setNextId(nextId);
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].setNextId(nextId);
    return true;
}

bool
TraceLogger::updateStop(uint32_t treeId, uint64_t timestamp)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.setStop(timestamp);
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].setStop(timestamp);
    return true;
}

void
TraceLogger::startEvent(uint32_t id)
{
    if (failed || enabled == 0)
        return;

    if (!tree.hasSpaceForAdd()){
        uint64_t start = rdtsc() - traceLoggers.startupTime;
        if (!tree.ensureSpaceBeforeAdd()) {
            if (!flush()) {
                fprintf(stderr, "TraceLogging: Couldn't write the data to disk.\n");
                enabled = 0;
                failed = true;
                return;
            }
        }

        
        
        if (!startEvent(TraceLogger_Internal, start)) {
            fprintf(stderr, "TraceLogging: Failed to start an event.\n");
            enabled = 0;
            failed = true;
            return;
        }
        stopEvent();
    }

    uint64_t start = rdtsc() - traceLoggers.startupTime;
    if (!startEvent(id, start)) {
        fprintf(stderr, "TraceLogging: Failed to start an event.\n");
        enabled = 0;
        failed = true;
        return;
    }
}

TraceLogger::StackEntry &
TraceLogger::getActiveAncestor()
{
    uint32_t parentId = stack.lastEntryId();
    while (!stack[parentId].active())
        parentId--;
    return stack[parentId];
}

bool
TraceLogger::startEvent(uint32_t id, uint64_t timestamp)
{
    if (!stack.ensureSpaceBeforeAdd())
        return false;

    
    
    
    if (!traceLoggers.isTextIdEnabled(id)) {
        StackEntry &stackEntry = stack.pushUninitialized();
        stackEntry.setActive(false);
        return true;
    }

    
    
    
    
    StackEntry &parent = getActiveAncestor();
#ifdef DEBUG
    TreeEntry entry;
    if (!getTreeEntry(parent.treeId(), &entry))
        return false;
#endif

    if (parent.lastChildId() == 0) {
        MOZ_ASSERT(!entry.hasChildren());
        MOZ_ASSERT(parent.treeId() == tree.lastEntryId() + treeOffset);

        if (!updateHasChildren(parent.treeId()))
            return false;
    } else {
        MOZ_ASSERT(entry.hasChildren());

        if (!updateNextId(parent.lastChildId(), tree.size() + treeOffset))
            return false;
    }

    
    TreeEntry &treeEntry = tree.pushUninitialized();
    treeEntry.setStart(timestamp);
    treeEntry.setStop(0);
    treeEntry.setTextId(id);
    treeEntry.setHasChildren(false);
    treeEntry.setNextId(0);

    
    StackEntry &stackEntry = stack.pushUninitialized();
    stackEntry.setTreeId(tree.lastEntryId() + treeOffset);
    stackEntry.setLastChildId(0);
    stackEntry.setActive(true);

    
    parent.setLastChildId(tree.lastEntryId() + treeOffset);

    return true;
}

void
TraceLogger::stopEvent(uint32_t id)
{
#ifdef DEBUG
    if (id != TraceLogger_Scripts && id != TraceLogger_Engine &&
        stack.size() > 1 && stack.lastEntry().active())
    {
        TreeEntry entry;
        MOZ_ASSERT(getTreeEntry(stack.lastEntry().treeId(), &entry));
        MOZ_ASSERT(entry.textId() == id);
    }
#endif
    stopEvent();
}

void
TraceLogger::stopEvent()
{
    if (enabled > 0 && stack.lastEntry().active()) {
        uint64_t stop = rdtsc() - traceLoggers.startupTime;
        if (!updateStop(stack.lastEntry().treeId(), stop)) {
            fprintf(stderr, "TraceLogging: Failed to stop an event.\n");
            enabled = 0;
            failed = true;
            return;
        }
    }
    if (stack.size() == 1) {
        if (enabled == 0)
            return;

        
        enabled = 1;
        disable();
        return;
    }
    stack.pop();
}

TraceLogging::TraceLogging()
{
    initialized = false;
    enabled = 0;
    mainThreadEnabled = false;
    offThreadEnabled = false;
    loggerId = 0;

    lock = PR_NewLock();
    if (!lock)
        MOZ_CRASH();
}

TraceLogging::~TraceLogging()
{
    if (out) {
        fprintf(out, "]");
        fclose(out);
        out = nullptr;
    }

    for (size_t i = 0; i < mainThreadLoggers.length(); i++)
        delete mainThreadLoggers[i];

    mainThreadLoggers.clear();

    if (threadLoggers.initialized()) {
        for (ThreadLoggerHashMap::Range r = threadLoggers.all(); !r.empty(); r.popFront())
            delete r.front().value();

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
TraceLogging::lazyInit()
{
    if (initialized)
        return enabled > 0;

    initialized = true;

    out = fopen(TRACE_LOG_DIR "tl-data.json", "w");
    if (!out)
        return false;
    fprintf(out, "[");

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
        for (uint32_t i = 1; i < TraceLogger_LAST; i++) {
            TraceLoggerTextId id = TraceLoggerTextId(i);
            if (!TraceLogger::textIdIsToggable(id))
                continue;
            printf("  %s\n", TLTextIdString(id));
        }
        printf("\n");
        exit(0);
        
    }

    for (uint32_t i = 1; i < TraceLogger_LAST; i++) {
        TraceLoggerTextId id = TraceLoggerTextId(i);
        if (TraceLogger::textIdIsToggable(id))
            enabledTextIds[i] = ContainsFlag(env, TLTextIdString(id));
        else
            enabledTextIds[i] = true;
    }

    if (ContainsFlag(env, "Default")) {
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
            );
            printf("\n");
            exit(0);
            
        }

        if (strstr(options, "EnableMainThread"))
           mainThreadEnabled = true;
        if (strstr(options, "EnableOffThread"))
           offThreadEnabled = true;
    }

    startupTime = rdtsc();
    enabled = 1;
    return true;
}

TraceLogger *
js::TraceLoggerForMainThread(CompileRuntime *runtime)
{
    return traceLoggers.forMainThread(runtime);
}

TraceLogger *
TraceLogging::forMainThread(CompileRuntime *runtime)
{
    return forMainThread(runtime->mainThread());
}

TraceLogger *
js::TraceLoggerForMainThread(JSRuntime *runtime)
{
    return traceLoggers.forMainThread(runtime);
}

TraceLogger *
TraceLogging::forMainThread(JSRuntime *runtime)
{
    return forMainThread(&runtime->mainThread);
}

TraceLogger *
TraceLogging::forMainThread(PerThreadData *mainThread)
{
    if (!mainThread->traceLogger) {
        AutoTraceLoggingLock lock(this);

        if (!lazyInit())
            return nullptr;

        TraceLogger *logger = create();
        mainThread->traceLogger = logger;

        if (!mainThreadLoggers.append(logger))
            return nullptr;

        if (!mainThreadEnabled)
            logger->disable();
    }

    return mainThread->traceLogger;
}

TraceLogger *
js::TraceLoggerForCurrentThread()
{
    PRThread *thread = PR_GetCurrentThread();
    return traceLoggers.forThread(thread);
}

TraceLogger *
TraceLogging::forThread(PRThread *thread)
{
    AutoTraceLoggingLock lock(this);

    if (!lazyInit())
        return nullptr;

    ThreadLoggerHashMap::AddPtr p = threadLoggers.lookupForAdd(thread);
    if (p)
        return p->value();

    TraceLogger *logger = create();
    if (!logger)
        return nullptr;

    if (!threadLoggers.add(p, thread, logger)) {
        delete logger;
        return nullptr;
    }

    if (!offThreadEnabled)
        logger->disable();

    return logger;
}

TraceLogger *
TraceLogging::create()
{
    if (loggerId > 999) {
        fprintf(stderr, "TraceLogging: Can't create more than 999 different loggers.");
        return nullptr;
    }

    if (loggerId > 0) {
        int written = fprintf(out, ",\n");
        if (written < 0)
            fprintf(stderr, "TraceLogging: Error while writing.\n");
    }

    loggerId++;

    int written = fprintf(out, "{\"tree\":\"tl-tree.%d.tl\", \"events\":\"tl-event.%d.tl\", \"dict\":\"tl-dict.%d.json\", \"treeFormat\":\"64,64,31,1,32\"}",
                          loggerId, loggerId, loggerId);
    if (written < 0)
        fprintf(stderr, "TraceLogging: Error while writing.\n");


    TraceLogger *logger = new TraceLogger();
    if (!logger)
        return nullptr;

    if (!logger->init(loggerId)) {
        delete logger;
        return nullptr;
    }

    return logger;
}

bool
js::TraceLogTextIdEnabled(uint32_t textId)
{
    return traceLoggers.isTextIdEnabled(textId);
}
