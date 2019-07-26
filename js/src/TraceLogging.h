





#ifndef TraceLogging_h
#define TraceLogging_h

#include "jsalloc.h"
#ifdef JS_THREADSAFE
# include "jslock.h"
#endif

#include "mozilla/GuardObjects.h"

#include "js/HashTable.h"
#include "js/TypeDecls.h"

class PRThread;
struct JSRuntime;

namespace JS {
    class ReadOnlyCompileOptions;
}

namespace js {














































































class AutoTraceLog;

template <class T>
class ContinuousSpace {
    T *data_;
    uint32_t next_;
    uint32_t capacity_;

  public:
    ContinuousSpace ()
     : data_(nullptr)
    { }

    bool init() {
        capacity_ = 64;
        next_ = 0;
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
        return next_;
    }

    uint32_t nextId() {
        return next_;
    }

    T &next() {
        return data()[next_];
    }

    uint32_t currentId() {
        JS_ASSERT(next_ > 0);
        return next_ - 1;
    }

    T &current() {
        return data()[currentId()];
    }

    bool ensureSpaceBeforeAdd() {
        if (next_ < capacity_)
            return true;

        uint32_t nCapacity = capacity_ * 2;
        T *entries = (T *) js_realloc(data_, nCapacity * sizeof(T));

        if (!entries)
            return false;

        data_ = entries;
        capacity_ = nCapacity;

        return true;
    }

    T &operator[](size_t i) {
        MOZ_ASSERT(i < next_);
        return data()[i];
    }

    void push(T &data) {
        MOZ_ASSERT(next_ < capacity_);
        data()[next_++] = data;
    }

    T &pushUninitialized() {
        return data()[next_++];
    }

    void pop() {
        JS_ASSERT(next_ > 0);
        next_--;
    }

    void clear() {
        next_ = 0;
    }
};

class TraceLogger
{
  public:
    
    
    
    
    enum TextId {
      TL_Error,
      Bailout,
      Baseline,
      GC,
      GCAllocating,
      GCSweeping,
      Interpreter,
      Invalidation,
      IonCompile,
      IonLink,
      IonMonkey,
      MinorGC,
      ParserCompileFunction,
      ParserCompileLazy,
      ParserCompileScript,
      TL,
      YarrCompile,
      YarrInterpret,
      YarrJIT,

      LAST
    };

#ifdef JS_TRACE_LOGGING
  private:
    typedef HashMap<const void *,
                    uint32_t,
                    PointerHasher<const void *, 3>,
                    SystemAllocPolicy> PointerHashMap;

    
    
    struct TreeEntry {
        uint64_t start;
        uint64_t stop;
        union {
            struct {
                uint32_t textId: 31;
                uint32_t hasChildren: 1;
            } s;
            uint32_t value;
        } u;
        uint32_t nextId;

        TreeEntry(uint64_t start, uint64_t stop, uint32_t textId, bool hasChildren,
                  uint32_t nextId)
        {
            this->start = start;
            this->stop = stop;
            this->u.s.textId = textId;
            this->u.s.hasChildren = hasChildren;
            this->nextId = nextId;
        }
    };

    
    
    struct StackEntry {
        uint32_t treeId;
        uint32_t lastChildId;
        StackEntry(uint32_t treeId, uint32_t lastChildId)
          : treeId(treeId), lastChildId(lastChildId)
        { }
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

    bool enabled;
    uint32_t nextTextId;

    PointerHashMap pointerMap;

    ContinuousSpace<TreeEntry> tree;
    ContinuousSpace<StackEntry> stack;
    ContinuousSpace<EventEntry> events;

    uint32_t treeOffset;

  public:
    AutoTraceLog *top;

  private:
    void updateHasChildren(uint32_t treeId, bool hasChildren = false);
    void updateNextId(uint32_t treeId, bool nextId);
    void updateStop(uint32_t treeId, uint64_t timestamp);
    void flush();

  public:
    TraceLogger();
    ~TraceLogger();

    bool init(uint32_t loggerId);

    
    
    
    uint32_t createTextId(const char *text);
    uint32_t createTextId(JSScript *script);
    uint32_t createTextId(const JS::ReadOnlyCompileOptions &script);

    
    void logTimestamp(uint32_t id);

    
    
    
    void startEvent(uint32_t id);
    void stopEvent(uint32_t id);
    void stopEvent();

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

    bool initialized;
    bool enabled;
    ThreadLoggerHashMap threadLoggers;
    uint32_t loggerId;
    FILE *out;

  public:
    uint64_t startupTime;
#ifdef JS_THREADSAFE
    PRLock *lock;
#endif

    TraceLogging();
    ~TraceLogging();

    TraceLogger *forMainThread(JSRuntime *runtime);
    TraceLogger *forThread(PRThread *thread);

  private:
    TraceLogger *create();
    bool lazyInit();
#endif
};

#ifdef JS_TRACE_LOGGING
TraceLogger *TraceLoggerForMainThread(JSRuntime *runtime);
TraceLogger *TraceLoggerForThread(PRThread *thread);
#else
inline TraceLogger *TraceLoggerForMainThread(JSRuntime *runtime) {
    return nullptr;
};
inline TraceLogger *TraceLoggerForThread(PRThread *thread) {
    return nullptr;
};
#endif

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
#ifdef JS_THREADSAFE
        PR_Lock(logging->lock);
#endif
    }
    ~AutoTraceLoggingLock() {
#ifdef JS_THREADSAFE
        PR_Unlock(logging->lock);
#endif
    }
  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif

}  

#endif 
