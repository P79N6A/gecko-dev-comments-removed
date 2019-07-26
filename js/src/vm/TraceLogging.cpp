





#include "vm/TraceLogging.h"

#include "mozilla/DebugOnly.h"

#include <string.h>

#include "jsapi.h"
#include "jsscript.h"

#include "jit/CompileWrappers.h"
#include "vm/Runtime.h"

using namespace js;

#ifndef TRACE_LOG_DIR
# if defined(_WIN32)
#  define TRACE_LOG_DIR ""
# else
#  define TRACE_LOG_DIR "/tmp/"
# endif
#endif

#if defined(__i386__)
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

    return(result);
}
#endif

TraceLogging traceLoggers;




const char* const text[] = {
    "TraceLogger failed to process text",
    "Bailout",
    "Baseline",
    "GC",
    "GCAllocation",
    "GCSweeping",
    "Interpreter",
    "Invalidation",
    "IonCompilation",
    "IonLinking",
    "IonMonkey",
    "MinorGC",
    "ParserCompileFunction",
    "ParserCompileLazy",
    "ParserCompileScript",
    "TraceLogger",
    "YarrCompile",
    "YarrInterpret",
    "YarrJIT",
    "SplitCriticalEdges",
    "RenumberBlocks",
    "DominatorTree",
    "PhiAnalysis",
    "ApplyTypes",
    "ParallelSafetyAnalysis",
    "AliasAnalysis",
    "GVN",
    "UCE",
    "LICM",
    "RangeAnalysis",
    "EffectiveAddressAnalysis",
    "EliminateDeadCode",
    "EdgeCaseAnalysis",
    "EliminateRedundantChecks"
};

TraceLogger::TraceLogger()
 : enabled(false),
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

    JS_ASSERT(loggerId <= 999);

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
    treeEntry.start = start;
    treeEntry.stop = 0;
    treeEntry.u.s.textId = 0;
    treeEntry.u.s.hasChildren = false;
    treeEntry.nextId = 0;

    StackEntry &stackEntry = stack.pushUninitialized();
    stackEntry.treeId = 0;
    stackEntry.lastChildId = 0;
    stackEntry.active = true;

    int written = fprintf(dictFile, "[");
    if (written < 0)
        fprintf(stderr, "TraceLogging: Error while writing.\n");

    
    for (uint32_t i = 0; i < LAST; i++) {
        mozilla::DebugOnly<uint32_t> textId = createTextId(text[i]);
        JS_ASSERT(textId == i);
    }

    enabled = true;
    return true;
}

bool
TraceLogger::flush()
{
    JS_ASSERT(!failed);

    if (treeFile) {
        
        for (size_t i = 0; i < tree.size(); i++)
            entryToBigEndian(&tree[i]);

        int success = fseek(treeFile, 0, SEEK_END);
        if (success != 0)
            return false;

        size_t bytesWritten = fwrite(tree.data(), sizeof(TreeEntry), tree.size(), treeFile);
        if (bytesWritten < tree.size())
            return false;

        treeOffset += tree.currentId();
        tree.clear();
    }

    if (eventFile) {
        
        for (size_t i = 0; i < events.size(); i++) {
            events[i].time = htobe64(events[i].time);
            events[i].textId = htobe64(events[i].textId);
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
        
        
        
        enabled = true;
        while (stack.size() > 0)
            stopEvent();
        enabled = false;
    }

    if (!failed && !flush()) {
        fprintf(stderr, "TraceLogging: Couldn't write the data to disk.\n");
        enabled = false;
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
        return TraceLogger::TL_Error;

    int written;
    if (textId > 0)
        written = fprintf(dictFile, ",\n\"%s\"", text);
    else
        written = fprintf(dictFile, "\"%s\"", text);

    if (written < 0)
        return TraceLogger::TL_Error;

    return textId;
}

uint32_t
TraceLogger::createTextId(JSScript *script)
{
    assertNoQuotes(script->filename());

    PointerHashMap::AddPtr p = pointerMap.lookupForAdd(script);
    if (p)
        return p->value();

    uint32_t textId = nextTextId++;
    if (!pointerMap.add(p, script, textId))
        return TraceLogger::TL_Error;

    int written;
    if (textId > 0) {
        written = fprintf(dictFile, ",\n\"script %s:%d:%d\"", script->filename(),
                          script->lineno(), script->column());
    } else {
        written = fprintf(dictFile, "\"script %s:%d:%d\"", script->filename(),
                          script->lineno(), script->column());
    }

    if (written < 0)
        return TraceLogger::TL_Error;

    return textId;
}

uint32_t
TraceLogger::createTextId(const JS::ReadOnlyCompileOptions &compileOptions)
{
    assertNoQuotes(compileOptions.filename());

    PointerHashMap::AddPtr p = pointerMap.lookupForAdd(&compileOptions);
    if (p)
        return p->value();

    uint32_t textId = nextTextId++;
    if (!pointerMap.add(p, &compileOptions, textId))
        return TraceLogger::TL_Error;

    int written;
    if (textId > 0) {
        written = fprintf(dictFile, ",\n\"script %s:%d:%d\"", compileOptions.filename(),
                          compileOptions.lineno, compileOptions.column);
    } else {
        written = fprintf(dictFile, "\"script %s:%d:%d\"", compileOptions.filename(),
                          compileOptions.lineno, compileOptions.column);
    }

    if (written < 0)
        return TraceLogger::TL_Error;

    return textId;
}

void
TraceLogger::logTimestamp(uint32_t id)
{
    if (!enabled)
        return;

    if (!events.ensureSpaceBeforeAdd()) {
        fprintf(stderr, "TraceLogging: Disabled a tracelogger due to OOM.\n");
        enabled = false;
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
    entry->start = htobe64(entry->start);
    entry->stop = htobe64(entry->stop);
    entry->u.value = htobe32((entry->u.s.textId << 1) + entry->u.s.hasChildren);
    entry->nextId = htobe32(entry->nextId);
}

void
TraceLogger::entryToSystemEndian(TreeEntry *entry)
{
    entry->start = be64toh(entry->start);
    entry->stop = be64toh(entry->stop);

    uint32_t data = be32toh(entry->u.value);
    entry->u.s.textId = data >> 1;
    entry->u.s.hasChildren = data & 0x1;

    entry->nextId = be32toh(entry->nextId);
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
        entry.u.s.hasChildren = hasChildren;
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].u.s.hasChildren = hasChildren;
    return true;
}

bool
TraceLogger::updateNextId(uint32_t treeId, uint32_t nextId)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.nextId = nextId;
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].nextId = nextId;
    return true;
}

bool
TraceLogger::updateStop(uint32_t treeId, uint64_t timestamp)
{
    if (treeId < treeOffset) {
        TreeEntry entry;
        if (!getTreeEntry(treeId, &entry))
            return false;
        entry.stop = timestamp;
        if (!saveTreeEntry(treeId, &entry))
            return false;
        return true;
    }

    tree[treeId - treeOffset].stop = timestamp;
    return true;
}

void
TraceLogger::startEvent(uint32_t id)
{
    if (!enabled)
        return;

    if (!stack.ensureSpaceBeforeAdd()) {
        fprintf(stderr, "TraceLogging: Failed to allocate space to keep track of the stack.\n");
        enabled = false;
        failed = true;
        return;
    }

    if (!tree.ensureSpaceBeforeAdd()) {
        uint64_t start = rdtsc() - traceLoggers.startupTime;
        if (!flush()) {
            fprintf(stderr, "TraceLogging: Couldn't write the data to disk.\n");
            enabled = false;
            failed = true;
            return;
        }

        
        
        if (!startEvent(TraceLogger::TL, start)) {
            fprintf(stderr, "TraceLogging: Failed to start an event.\n");
            enabled = false;
            failed = true;
            return;
        }
        stopEvent();
    }

    uint64_t start = rdtsc() - traceLoggers.startupTime;
    if (!startEvent(id, start)) {
        fprintf(stderr, "TraceLogging: Failed to start an event.\n");
        enabled = false;
        failed = true;
        return;
    }
}

TraceLogger::StackEntry &
TraceLogger::getActiveAncestor()
{
    uint32_t parentId = stack.currentId();
    while (!stack[parentId].active)
        parentId--;
    return stack[parentId];
}

bool
TraceLogger::startEvent(uint32_t id, uint64_t timestamp)
{
    
    
    
    if (!traceLoggers.isTextIdEnabled(id)) {
        StackEntry &stackEntry = stack.pushUninitialized();
        stackEntry.active = false;
        return true;
    }

    
    
    
    
    StackEntry &parent = getActiveAncestor();
#ifdef DEBUG
    TreeEntry entry;
    if (!getTreeEntry(parent.treeId, &entry))
        return false;
#endif

    if (parent.lastChildId == 0) {
        JS_ASSERT(entry.u.s.hasChildren == 0);
        JS_ASSERT(parent.treeId == tree.currentId() + treeOffset);

        if (!updateHasChildren(parent.treeId))
            return false;
    } else {
        JS_ASSERT(entry.u.s.hasChildren == 1);

        if (!updateNextId(parent.lastChildId, tree.nextId() + treeOffset))
            return false;
    }

    
    TreeEntry &treeEntry = tree.pushUninitialized();
    treeEntry.start = timestamp;
    treeEntry.stop = 0;
    treeEntry.u.s.textId = id;
    treeEntry.u.s.hasChildren = false;
    treeEntry.nextId = 0;

    
    StackEntry &stackEntry = stack.pushUninitialized();
    stackEntry.treeId = tree.currentId() + treeOffset;
    stackEntry.lastChildId = 0;
    stackEntry.active = true;

    
    parent.lastChildId = tree.currentId() + treeOffset;

    return true;
}

void
TraceLogger::stopEvent(uint32_t id)
{
#ifdef DEBUG
    TreeEntry entry;
    JS_ASSERT(getTreeEntry(stack.current().treeId, &entry));
    JS_ASSERT(entry.u.s.textId == id);
#endif
    stopEvent();
}

void
TraceLogger::stopEvent()
{
    if (!enabled)
        return;

    if (stack.current().active) {
        uint64_t stop = rdtsc() - traceLoggers.startupTime;
        if (!updateStop(stack.current().treeId, stop)) {
            fprintf(stderr, "TraceLogging: Failed to stop an event.\n");
            enabled = false;
            failed = true;
            return;
        }
    }
    stack.pop();
}

TraceLogging::TraceLogging()
{
    initialized = false;
    enabled = false;
    loggerId = 0;

#ifdef JS_THREADSAFE
    lock = PR_NewLock();
    if (!lock)
        MOZ_CRASH();
#endif
}

TraceLogging::~TraceLogging()
{
    if (out) {
        fprintf(out, "]");
        fclose(out);
        out = nullptr;
    }

    if (threadLoggers.initialized()) {
        for (ThreadLoggerHashMap::Range r = threadLoggers.all(); !r.empty(); r.popFront())
            delete r.front().value();

        threadLoggers.finish();
    }

    for (size_t i = 0; i < mainThreadLoggers.length(); i++)
        delete mainThreadLoggers[i];

    mainThreadLoggers.clear();

    if (lock) {
        PR_DestroyLock(lock);
        lock = nullptr;
    }

    enabled = false;
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
        return enabled;

    initialized = true;

    out = fopen(TRACE_LOG_DIR "tl-data.json", "w");
    if (!out)
        return false;
    fprintf(out, "[");

    if (!threadLoggers.init())
        return false;

    const char *env = getenv("TLLOG");
    if (!env)
        return false;

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
        for (uint32_t i = 1; i < TraceLogger::LAST; i++) {
            printf("  %s\n", text[i]);
        }
        printf("\n");
        exit(0);
        
    }

    for (uint32_t i = 1; i < TraceLogger::LAST; i++)
        enabledTextIds[i] = ContainsFlag(env, text[i]);

    enabledTextIds[TraceLogger::TL_Error] = true;
    enabledTextIds[TraceLogger::TL] = true;

    if (ContainsFlag(env, "Default") || strlen(env) == 0) {
        enabledTextIds[TraceLogger::Bailout] = true;
        enabledTextIds[TraceLogger::Baseline] = true;
        enabledTextIds[TraceLogger::GC] = true;
        enabledTextIds[TraceLogger::GCAllocation] = true;
        enabledTextIds[TraceLogger::GCSweeping] = true;
        enabledTextIds[TraceLogger::Interpreter] = true;
        enabledTextIds[TraceLogger::IonCompilation] = true;
        enabledTextIds[TraceLogger::IonLinking] = true;
        enabledTextIds[TraceLogger::IonMonkey] = true;
        enabledTextIds[TraceLogger::MinorGC] = true;
        enabledTextIds[TraceLogger::ParserCompileFunction] = true;
        enabledTextIds[TraceLogger::ParserCompileLazy] = true;
        enabledTextIds[TraceLogger::ParserCompileScript] = true;
        enabledTextIds[TraceLogger::YarrCompile] = true;
        enabledTextIds[TraceLogger::YarrInterpret] = true;
        enabledTextIds[TraceLogger::YarrJIT] = true;
    }

    if (ContainsFlag(env, "IonCompiler") || strlen(env) == 0) {
        enabledTextIds[TraceLogger::IonCompilation] = true;
        enabledTextIds[TraceLogger::IonLinking] = true;
        enabledTextIds[TraceLogger::SplitCriticalEdges] = true;
        enabledTextIds[TraceLogger::RenumberBlocks] = true;
        enabledTextIds[TraceLogger::DominatorTree] = true;
        enabledTextIds[TraceLogger::PhiAnalysis] = true;
        enabledTextIds[TraceLogger::ApplyTypes] = true;
        enabledTextIds[TraceLogger::ParallelSafetyAnalysis] = true;
        enabledTextIds[TraceLogger::AliasAnalysis] = true;
        enabledTextIds[TraceLogger::GVN] = true;
        enabledTextIds[TraceLogger::UCE] = true;
        enabledTextIds[TraceLogger::LICM] = true;
        enabledTextIds[TraceLogger::RangeAnalysis] = true;
        enabledTextIds[TraceLogger::EffectiveAddressAnalysis] = true;
        enabledTextIds[TraceLogger::EliminateDeadCode] = true;
        enabledTextIds[TraceLogger::EdgeCaseAnalysis] = true;
        enabledTextIds[TraceLogger::EliminateRedundantChecks] = true;
    }

    startupTime = rdtsc();
    enabled = true;
    return true;
}

TraceLogger *
js::TraceLoggerForMainThread(jit::CompileRuntime *runtime)
{
    return traceLoggers.forMainThread(runtime);
}

TraceLogger *
TraceLogging::forMainThread(jit::CompileRuntime *runtime)
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
    }

    return mainThread->traceLogger;
}

TraceLogger *
js::TraceLoggerForThread(PRThread *thread)
{
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


    fprintf(out, "{\"tree\":\"tl-tree.%d.tl\", \"events\":\"tl-event.%d.tl\", \"dict\":\"tl-dict.%d.json\", \"treeFormat\":\"64,64,31,1,32\"}",
            loggerId, loggerId, loggerId);

    loggerId++;

    TraceLogger *logger = new TraceLogger();
    if (!logger)
        return nullptr;

    if (!logger->init(loggerId)) {
        delete logger;
        return nullptr;
    }

    return logger;
}
