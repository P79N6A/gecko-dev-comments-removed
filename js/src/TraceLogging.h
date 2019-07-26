





#ifndef TraceLogging_h
#define TraceLogging_h

#include <stdint.h>
#include <stdio.h>

#include "jsalloc.h"

#include "js/HashTable.h"
#include "js/TypeDecls.h"

namespace JS {
class CompileOptions;
}

namespace js {

class TraceLogging
{
  public:
    enum Type {
        SCRIPT_START,
        SCRIPT_STOP,
        ION_COMPILE_START,
        ION_COMPILE_STOP,
        YARR_JIT_START,
        YARR_JIT_STOP,
        GC_START,
        GC_STOP,
        MINOR_GC_START,
        MINOR_GC_STOP,
        PARSER_COMPILE_SCRIPT_START,
        PARSER_COMPILE_SCRIPT_STOP,
        PARSER_COMPILE_LAZY_START,
        PARSER_COMPILE_LAZY_STOP,
        PARSER_COMPILE_FUNCTION_START,
        PARSER_COMPILE_FUNCTION_STOP,
        INFO_ENGINE_INTERPRETER,
        INFO_ENGINE_BASELINE,
        INFO_ENGINE_IONMONKEY,
        INFO
    };
    enum Logger {
        DEFAULT,
        ION_BACKGROUND_COMPILER,

        LAST_LOGGER
    };

  private:
    struct Entry {
        uint64_t tick_;
        char* text_;
        uint32_t textId_;
        uint32_t lineno_;
        uint8_t type_;

        Entry(uint64_t tick, char* text, uint32_t textId, uint32_t lineno, Type type)
            : tick_(tick),
              text_(text),
              textId_(textId),
              lineno_(lineno),
              type_((uint8_t)type) {}

        uint64_t tick() const { return tick_; }
        char *text() const { return text_; }
        uint32_t textId() const { return textId_; }
        uint32_t lineno() const { return lineno_; }
        Type type() const { return (Type) type_; }
    };

    typedef HashMap<const char *,
                        uint32_t,
                        PointerHasher<const char *, 3>,
                        SystemAllocPolicy> TextHashMap;

    uint64_t loggingTime;
    TextHashMap textMap;
    uint32_t nextTextId;
    Entry *entries;
    unsigned int curEntry;
    unsigned int numEntries;
    int fileno;
    FILE *out;
    Logger id;

    static bool atexitSet;
    static const char * const typeName[];
    static TraceLogging* loggers[];
    static uint64_t startupTime;
  public:
    TraceLogging(Logger id);
    ~TraceLogging();

    void log(Type type, const char* text = NULL, unsigned int number = 0);
    void log(Type type, const JS::CompileOptions &options);
    void log(Type type, JSScript* script);
    void log(const char* log);
    void flush();

    static TraceLogging* getLogger(Logger id);
    static TraceLogging* defaultLogger() {
        return getLogger(DEFAULT);
    }
    static void releaseLoggers();

  private:
    void grow();
};


void TraceLog(TraceLogging* logger, TraceLogging::Type type, JSScript* script);
void TraceLog(TraceLogging* logger, const char* log);
void TraceLog(TraceLogging* logger, TraceLogging::Type type);


class AutoTraceLog {
    TraceLogging* logger;
    TraceLogging::Type stop;

  public:
    AutoTraceLog(TraceLogging* logger, TraceLogging::Type start, TraceLogging::Type stop,
                 const JS::CompileOptions &options)
      : logger(logger),
        stop(stop)
    {
        logger->log(start, options);
    }

    AutoTraceLog(TraceLogging* logger, TraceLogging::Type start, TraceLogging::Type stop,
                 JSScript* script)
      : logger(logger),
        stop(stop)
    {
        logger->log(start, script);
    }

    AutoTraceLog(TraceLogging* logger, TraceLogging::Type start, TraceLogging::Type stop)
      : logger(logger),
        stop(stop)
    {
        logger->log(start);
    }

    ~AutoTraceLog()
    {
        logger->log(stop);
    }
};

}  

#endif
