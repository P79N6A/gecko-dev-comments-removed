





#ifndef TraceLogging_h
#define TraceLogging_h

#include "mozilla/GuardObjects.h"

#include "jsalloc.h"
#include "jslock.h"

#include "js/HashTable.h"
#include "js/TypeDecls.h"
#include "js/Vector.h"

struct JSRuntime;

namespace JS {
    class ReadOnlyCompileOptions;
}

namespace js {
class PerThreadData;

namespace jit {
    class CompileRuntime;
}














































































#define TRACELOGGER_TEXT_ID_LIST(_)                   \
    _(Bailout)                                        \
    _(Baseline)                                       \
    _(BaselineCompilation)                            \
    _(GC)                                             \
    _(GCAllocation)                                   \
    _(GCSweeping)                                     \
    _(Interpreter)                                    \
    _(Invalidation)                                   \
    _(IonCompilation)                                 \
    _(IonCompilationPaused)                           \
    _(IonLinking)                                     \
    _(IonMonkey)                                      \
    _(MinorGC)                                        \
    _(ParserCompileFunction)                          \
    _(ParserCompileLazy)                              \
    _(ParserCompileScript)                            \
    _(TL)                                             \
    _(IrregexpCompile)                                \
    _(IrregexpExecute)                                \
    _(VM)                                             \
                                                      \
    /* Specific passes during ion compilation */      \
    _(FoldTests)                                      \
    _(SplitCriticalEdges)                             \
    _(RenumberBlocks)                                 \
    _(ScalarReplacement)                              \
    _(DominatorTree)                                  \
    _(PhiAnalysis)                                    \
    _(MakeLoopsContiguous)                            \
    _(ApplyTypes)                                     \
    _(ParallelSafetyAnalysis)                         \
    _(AliasAnalysis)                                  \
    _(GVN)                                            \
    _(LICM)                                           \
    _(RangeAnalysis)                                  \
    _(LoopUnrolling)                                  \
    _(EffectiveAddressAnalysis)                       \
    _(EliminateDeadCode)                              \
    _(EdgeCaseAnalysis)                               \
    _(EliminateRedundantChecks)                       \
    _(GenerateLIR)                                    \
    _(RegisterAllocation)                             \
    _(GenerateCode)                                   \

class AutoTraceLog;

template <class T>
class ContinuousSpace {
    T *data_;
    uint32_t size_;
    uint32_t capacity_;

  public:
    ContinuousSpace ()
     : data_(nullptr)
    { }

    bool init() {
        capacity_ = 64;
        size_ = 0;
        data_ = (T *) js_malloc(capacity_ * sizeof(T));
        if (!data_)
            return false;

        return true;
    }

    T *data() {
        return data_;
    }

    uint32_t capacity() {
        return capacity_;
    }

    uint32_t size() {
        return size_;
    }

    bool empty() {
        return size_ == 0;
    }

    uint32_t lastEntryId() {
        MOZ_ASSERT(!empty());
        return size_ - 1;
    }

    T &lastEntry() {
        return data()[lastEntryId()];
    }

    bool hasSpaceForAdd(uint32_t count = 1) {
        if (size_ + count <= capacity_)
            return true;
        return false;
    }

    bool ensureSpaceBeforeAdd(uint32_t count = 1) {
        if (hasSpaceForAdd(count))
            return true;

        uint32_t nCapacity = capacity_ * 2;
        if (size_ + count > nCapacity)
            nCapacity = size_ + count;
        T *entries = (T *) js_realloc(data_, nCapacity * sizeof(T));

        if (!entries)
            return false;

        data_ = entries;
        capacity_ = nCapacity;

        return true;
    }

    T &operator[](size_t i) {
        MOZ_ASSERT(i < size_);
        return data()[i];
    }

    void push(T &data) {
        MOZ_ASSERT(size_ < capacity_);
        data()[size_++] = data;
    }

    T &pushUninitialized() {
        MOZ_ASSERT(size_ < capacity_);
        return data()[size_++];
    }

    void pop() {
        MOZ_ASSERT(!empty());
        size_--;
    }

    void clear() {
        size_ = 0;
    }
};

class TraceLogger
{
  public:
    
    
    enum TextId {
        TL_Error = 0,
#   define DEFINE_TEXT_ID(textId) textId,
        TRACELOGGER_TEXT_ID_LIST(DEFINE_TEXT_ID)
#   undef DEFINE_TEXT_ID
        LAST
    };

#ifdef JS_TRACE_LOGGING
  private:
    typedef HashMap<const void *,
                    uint32_t,
                    PointerHasher<const void *, 3>,
                    SystemAllocPolicy> PointerHashMap;

    
    
    struct TreeEntry {
        uint64_t start_;
        uint64_t stop_;
        union {
            struct {
                uint32_t textId_: 31;
                uint32_t hasChildren_: 1;
            } s;
            uint32_t value_;
        } u;
        uint32_t nextId_;

        TreeEntry(uint64_t start, uint64_t stop, uint32_t textId, bool hasChildren,
                  uint32_t nextId)
        {
            start_ = start;
            stop_ = stop;
            u.s.textId_ = textId;
            u.s.hasChildren_ = hasChildren;
            nextId_ = nextId;
        }
        TreeEntry()
        { }
        uint64_t start() {
            return start_;
        }
        uint64_t stop() {
            return stop_;
        }
        uint32_t textId() {
            return u.s.textId_;
        }
        bool hasChildren() {
            return u.s.hasChildren_;
        }
        uint32_t nextId() {
            return nextId_;
        }
        void setStart(uint64_t start) {
            start_ = start;
        }
        void setStop(uint64_t stop) {
            stop_ = stop;
        }
        void setTextId(uint32_t textId) {
            MOZ_ASSERT(textId < uint32_t(1<<31) );
            u.s.textId_ = textId;
        }
        void setHasChildren(bool hasChildren) {
            u.s.hasChildren_ = hasChildren;
        }
        void setNextId(uint32_t nextId) {
            nextId_ = nextId;
        }
    };

    
    
    
    struct StackEntry {
        uint32_t treeId_;
        uint32_t lastChildId_;
        struct {
            uint32_t textId_: 31;
            uint32_t active_: 1;
        } s;
        StackEntry(uint32_t treeId, uint32_t lastChildId, bool active = true)
          : treeId_(treeId), lastChildId_(lastChildId)
        {
            s.textId_ = 0;
            s.active_ = active;
        }
        uint32_t treeId() {
            return treeId_;
        }
        uint32_t lastChildId() {
            return lastChildId_;
        }
        uint32_t textId() {
            return s.textId_;
        }
        bool active() {
            return s.active_;
        }
        void setTreeId(uint32_t treeId) {
            treeId_ = treeId;
        }
        void setLastChildId(uint32_t lastChildId) {
            lastChildId_ = lastChildId;
        }
        void setTextId(uint32_t textId) {
            MOZ_ASSERT(textId < uint32_t(1<<31) );
            s.textId_ = textId;
        }
        void setActive(bool active) {
            s.active_ = active;
        }
    };

    
    
    struct EventEntry {
        uint64_t time;
        uint32_t textId;
        EventEntry(uint64_t time, uint32_t textId)
          : time(time), textId(textId)
        { }
    };

    FILE *dictFile;
    FILE *treeFile;
    FILE *eventFile;

    uint32_t enabled;
    bool failed;
    uint32_t nextTextId;

    PointerHashMap pointerMap;

    ContinuousSpace<TreeEntry> tree;
    ContinuousSpace<StackEntry> stack;
    ContinuousSpace<EventEntry> events;

    uint32_t treeOffset;

  public:
    AutoTraceLog *top;

  private:
    
    
    void entryToBigEndian(TreeEntry *entry);
    void entryToSystemEndian(TreeEntry *entry);

    
    bool getTreeEntry(uint32_t treeId, TreeEntry *entry);
    bool saveTreeEntry(uint32_t treeId, TreeEntry *entry);

    
    StackEntry &getActiveAncestor();

    
    
    bool startEvent(uint32_t id, uint64_t timestamp);

    
    
    bool updateHasChildren(uint32_t treeId, bool hasChildren = true);
    bool updateNextId(uint32_t treeId, uint32_t nextId);
    bool updateStop(uint32_t treeId, uint64_t timestamp);

    
    bool flush();

  public:
    TraceLogger();
    ~TraceLogger();

    bool init(uint32_t loggerId);

    bool enable();
    bool enable(JSContext *cx);
    bool disable();

    
    
    
    uint32_t createTextId(const char *text);
    uint32_t createTextId(JSScript *script);
    uint32_t createTextId(const JS::ReadOnlyCompileOptions &script);

    
    void logTimestamp(uint32_t id);

    
    
    
    void startEvent(uint32_t id);
    void stopEvent(uint32_t id);
    void stopEvent();

    static unsigned offsetOfEnabled() {
        return offsetof(TraceLogger, enabled);
    }

  private:
    void assertNoQuotes(const char *text) {
#ifdef DEBUG
        const char *quote = strchr(text, '"');
        MOZ_ASSERT(!quote);
#endif
    }
#endif
};

class TraceLogging
{
#ifdef JS_TRACE_LOGGING
    typedef HashMap<PRThread *,
                    TraceLogger *,
                    PointerHasher<PRThread *, 3>,
                    SystemAllocPolicy> ThreadLoggerHashMap;
    typedef Vector<TraceLogger *, 1, js::SystemAllocPolicy > MainThreadLoggers;

    bool initialized;
    bool enabled;
    bool enabledTextIds[TraceLogger::LAST];
    bool mainThreadEnabled;
    bool offThreadEnabled;
    ThreadLoggerHashMap threadLoggers;
    MainThreadLoggers mainThreadLoggers;
    uint32_t loggerId;
    FILE *out;

  public:
    uint64_t startupTime;
    PRLock *lock;

    TraceLogging();
    ~TraceLogging();

    TraceLogger *forMainThread(JSRuntime *runtime);
    TraceLogger *forMainThread(jit::CompileRuntime *runtime);
    TraceLogger *forThread(PRThread *thread);

    bool isTextIdEnabled(uint32_t textId) {
        if (textId < TraceLogger::LAST)
            return enabledTextIds[textId];
        return true;
    }

  private:
    TraceLogger *forMainThread(PerThreadData *mainThread);
    TraceLogger *create();
    bool lazyInit();
#endif
};

#ifdef JS_TRACE_LOGGING
TraceLogger *TraceLoggerForMainThread(JSRuntime *runtime);
TraceLogger *TraceLoggerForMainThread(jit::CompileRuntime *runtime);
TraceLogger *TraceLoggerForCurrentThread();
#else
inline TraceLogger *TraceLoggerForMainThread(JSRuntime *runtime) {
    return nullptr;
};
inline TraceLogger *TraceLoggerForMainThread(jit::CompileRuntime *runtime) {
    return nullptr;
};
inline TraceLogger *TraceLoggerForCurrentThread() {
    return nullptr;
};
#endif

inline bool TraceLoggerEnable(TraceLogger *logger) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->enable();
#endif
    return false;
}
inline bool TraceLoggerEnable(TraceLogger *logger, JSContext *cx) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->enable(cx);
#endif
    return false;
}
inline bool TraceLoggerDisable(TraceLogger *logger) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->disable();
#endif
    return false;
}

inline uint32_t TraceLogCreateTextId(TraceLogger *logger, JSScript *script) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->createTextId(script);
#endif
    return TraceLogger::TL_Error;
}
inline uint32_t TraceLogCreateTextId(TraceLogger *logger,
                                     const JS::ReadOnlyCompileOptions &compileOptions)
{
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->createTextId(compileOptions);
#endif
    return TraceLogger::TL_Error;
}
inline uint32_t TraceLogCreateTextId(TraceLogger *logger, const char *text) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->createTextId(text);
#endif
    return TraceLogger::TL_Error;
}
#ifdef JS_TRACE_LOGGING
bool TraceLogTextIdEnabled(uint32_t textId);
#else
inline bool TraceLogTextIdEnabled(uint32_t textId) {
    return false;
}
#endif
inline void TraceLogTimestamp(TraceLogger *logger, uint32_t textId) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->logTimestamp(textId);
#endif
}
inline void TraceLogStartEvent(TraceLogger *logger, uint32_t textId) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->startEvent(textId);
#endif
}
inline void TraceLogStopEvent(TraceLogger *logger, uint32_t textId) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->stopEvent(textId);
#endif
}
inline void TraceLogStopEvent(TraceLogger *logger) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->stopEvent();
#endif
}


class AutoTraceLog {
#ifdef JS_TRACE_LOGGING
    TraceLogger *logger;
    uint32_t textId;
    bool executed;
    AutoTraceLog *prev;

  public:
    AutoTraceLog(TraceLogger *logger, uint32_t textId MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : logger(logger),
        textId(textId),
        executed(false)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (logger) {
            TraceLogStartEvent(logger, textId);

            prev = logger->top;
            logger->top = this;
        }
    }

    ~AutoTraceLog()
    {
        if (logger) {
            while (this != logger->top)
                logger->top->stop();
            stop();
        }
    }
  private:
    void stop() {
        if (!executed) {
            executed = true;
            TraceLogStopEvent(logger, textId);
        }

        if (logger->top == this)
            logger->top = prev;
    }
#else
  public:
    AutoTraceLog(TraceLogger *logger, uint32_t textId MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
#endif

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef JS_TRACE_LOGGING
class AutoTraceLoggingLock
{
  TraceLogging *logging;

  public:
    AutoTraceLoggingLock(TraceLogging *logging MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : logging(logging)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        PR_Lock(logging->lock);
    }
    ~AutoTraceLoggingLock() {
        PR_Unlock(logging->lock);
    }
  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

}  

#endif
