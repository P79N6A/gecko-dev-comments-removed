





#ifndef TraceLogging_h
#define TraceLogging_h

#include "mozilla/GuardObjects.h"
#include "mozilla/UniquePtr.h"

#include "jsalloc.h"
#include "jslock.h"

#include "js/HashTable.h"
#include "js/TypeDecls.h"
#include "js/Vector.h"
#include "vm/TraceLoggingGraph.h"
#include "vm/TraceLoggingTypes.h"

struct JSRuntime;

namespace JS {
    class ReadOnlyCompileOptions;
}

namespace js {
class PerThreadData;

namespace jit {
    class CompileRuntime;
}

































class AutoTraceLog;

class TraceLoggerThread
{
#ifdef JS_TRACE_LOGGING
  private:
    typedef HashMap<const void *,
                    uint32_t,
                    PointerHasher<const void *, 3>,
                    SystemAllocPolicy> PointerHashMap;

    uint32_t enabled;
    bool failed;

    mozilla::UniquePtr<TraceLoggerGraph> graph;

    PointerHashMap pointerMap;
    Vector<char *, 0, js::SystemAllocPolicy> extraTextId;

    ContinuousSpace<EventEntry> events;

    
    
    uint32_t iteration_;

  public:
    AutoTraceLog *top;

    TraceLoggerThread();
    bool init();
    ~TraceLoggerThread();

    bool init(uint32_t loggerId);
    void initGraph();

    bool enable();
    bool enable(JSContext *cx);
    bool disable();

    
    
    
    EventEntry *getEventsStartingAt(uint32_t *lastIteration, uint32_t *lastEntryId, size_t *num) {
        EventEntry *start;
        if (iteration_ == *lastIteration) {
            MOZ_ASSERT(events.lastEntryId() >= *lastEntryId);
            *num = events.lastEntryId() - *lastEntryId;
            start = events.data() + *lastEntryId + 1;
        } else {
            *num = events.lastEntryId() + 1;
            start = events.data();
        }

        *lastIteration = iteration_;
        *lastEntryId = events.lastEntryId();
        return start;
    }

    
    
    void extractScriptDetails(uint32_t textId, const char **filename, size_t *filename_len,
                              const char **lineno, size_t *lineno_len, const char **colno,
                              size_t *colno_len);

    bool lostEvents(uint32_t lastIteration, uint32_t lastEntryId) {
        
        if (lastIteration == iteration_) {
            MOZ_ASSERT(lastEntryId <= events.lastEntryId());
            return false;
        }

        
        
        if (lastIteration + 1 == iteration_ && lastEntryId == events.capacity())
            return false;

        return true;
    }

    const char *eventText(uint32_t id);
    bool textIdIsScriptEvent(uint32_t id);

    
    
    
    
    uint32_t createTextId(const char *text);
    uint32_t createTextId(JSScript *script);
    uint32_t createTextId(const JS::ReadOnlyCompileOptions &script);
  private:
    uint32_t createTextId(const char *filename, size_t lineno, size_t colno, const void *p);

  public:
    
    void logTimestamp(uint32_t id);

    
    void startEvent(uint32_t id);
    void stopEvent(uint32_t id);

  private:
    void stopEvent();

  public:
    static unsigned offsetOfEnabled() {
        return offsetof(TraceLoggerThread, enabled);
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

class TraceLoggerThreadState
{
#ifdef JS_TRACE_LOGGING
    typedef HashMap<PRThread *,
                    TraceLoggerThread *,
                    PointerHasher<PRThread *, 3>,
                    SystemAllocPolicy> ThreadLoggerHashMap;
    typedef Vector<TraceLoggerThread *, 1, js::SystemAllocPolicy > MainThreadLoggers;

    bool initialized;
    bool enabled;
    bool enabledTextIds[TraceLogger_Last];
    bool mainThreadEnabled;
    bool offThreadEnabled;
    bool graphSpewingEnabled;
    ThreadLoggerHashMap threadLoggers;
    MainThreadLoggers mainThreadLoggers;

  public:
    uint64_t startupTime;
    PRLock *lock;

    TraceLoggerThreadState();
    ~TraceLoggerThreadState();

    TraceLoggerThread *forMainThread(JSRuntime *runtime);
    TraceLoggerThread *forMainThread(jit::CompileRuntime *runtime);
    TraceLoggerThread *forThread(PRThread *thread);

    bool isTextIdEnabled(uint32_t textId) {
        if (textId < TraceLogger_Last)
            return enabledTextIds[textId];
        return true;
    }
    void enableTextId(JSContext *cx, uint32_t textId);
    void disableTextId(JSContext *cx, uint32_t textId);

  private:
    TraceLoggerThread *forMainThread(PerThreadData *mainThread);
    TraceLoggerThread *create();
    bool lazyInit();
#endif
};

#ifdef JS_TRACE_LOGGING
TraceLoggerThread *TraceLoggerForMainThread(JSRuntime *runtime);
TraceLoggerThread *TraceLoggerForMainThread(jit::CompileRuntime *runtime);
TraceLoggerThread *TraceLoggerForCurrentThread();
#else
inline TraceLoggerThread *TraceLoggerForMainThread(JSRuntime *runtime) {
    return nullptr;
};
inline TraceLoggerThread *TraceLoggerForMainThread(jit::CompileRuntime *runtime) {
    return nullptr;
};
inline TraceLoggerThread *TraceLoggerForCurrentThread() {
    return nullptr;
};
#endif

inline bool TraceLoggerEnable(TraceLoggerThread *logger) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->enable();
#endif
    return false;
}
inline bool TraceLoggerEnable(TraceLoggerThread *logger, JSContext *cx) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->enable(cx);
#endif
    return false;
}
inline bool TraceLoggerDisable(TraceLoggerThread *logger) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->disable();
#endif
    return false;
}

inline uint32_t TraceLogCreateTextId(TraceLoggerThread *logger, JSScript *script) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->createTextId(script);
#endif
    return TraceLogger_Error;
}
inline uint32_t TraceLogCreateTextId(TraceLoggerThread *logger,
                                     const JS::ReadOnlyCompileOptions &compileOptions)
{
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->createTextId(compileOptions);
#endif
    return TraceLogger_Error;
}
inline uint32_t TraceLogCreateTextId(TraceLoggerThread *logger, const char *text) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        return logger->createTextId(text);
#endif
    return TraceLogger_Error;
}
#ifdef JS_TRACE_LOGGING
bool TraceLogTextIdEnabled(uint32_t textId);
void TraceLogEnableTextId(JSContext *cx, uint32_t textId);
void TraceLogDisableTextId(JSContext *cx, uint32_t textId);
#else
inline bool TraceLogTextIdEnabled(uint32_t textId) {
    return false;
}
inline void TraceLogEnableTextId(JSContext *cx, uint32_t textId) {}
inline void TraceLogDisableTextId(JSContext *cx, uint32_t textId) {}
#endif
inline void TraceLogTimestamp(TraceLoggerThread *logger, uint32_t textId) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->logTimestamp(textId);
#endif
}
inline void TraceLogStartEvent(TraceLoggerThread *logger, uint32_t textId) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->startEvent(textId);
#endif
}
inline void TraceLogStopEvent(TraceLoggerThread *logger, uint32_t textId) {
#ifdef JS_TRACE_LOGGING
    if (logger)
        logger->stopEvent(textId);
#endif
}


class AutoTraceLog {
#ifdef JS_TRACE_LOGGING
    TraceLoggerThread *logger;
    uint32_t textId;
    bool executed;
    AutoTraceLog *prev;

  public:
    AutoTraceLog(TraceLoggerThread *logger, uint32_t textId MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
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
    AutoTraceLog(TraceLoggerThread *logger, uint32_t textId MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
#endif

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

}  

#endif
