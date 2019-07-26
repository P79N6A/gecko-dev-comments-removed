





#include "TraceLogging.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "jsapi.h"
#include "jsscript.h"

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

const char* const TraceLogging::typeName[] = {
    "1,s",  
    "0,s",  
    "1,c",  
    "0,c",  
    "1,r",  
    "0,r",  
    "1,G",  
    "0,G",  
    "1,g",  
    "0,g",  
    "1,gS", 
    "0,gS", 
    "1,gA", 
    "0,gA", 
    "1,ps", 
    "0,ps", 
    "1,pl", 
    "0,pl", 
    "1,pf", 
    "0,pf", 
    "e,i",  
    "e,b",  
    "e,o"   
};
TraceLogging* TraceLogging::loggers[] = {nullptr, nullptr, nullptr};
bool TraceLogging::atexitSet = false;
uint64_t TraceLogging::startupTime = 0;

TraceLogging::TraceLogging(Logger id)
  : nextTextId(1),
    entries(nullptr),
    curEntry(0),
    numEntries(1000000),
    fileno(0),
    out(nullptr),
    id(id)
{
    textMap.init();
}

TraceLogging::~TraceLogging()
{
    if (entries) {
        flush();
        free(entries);
        entries = nullptr;
    }

    if (out) {
        fclose(out);
        out = nullptr;
    }
}

void
TraceLogging::grow()
{
    Entry* nentries = (Entry*) realloc(entries, numEntries*2*sizeof(Entry));

    
    
    if (!nentries) {
        flush();
        return;
    }

    entries = nentries;
    numEntries *= 2;
}

void
TraceLogging::log(Type type, const char* text , unsigned int number )
{
    uint64_t now = rdtsc() - startupTime;

    
    if (!entries) {
        entries = (Entry*) malloc(numEntries*sizeof(Entry));
        if (!entries)
            return;
    }

    uint32_t textId = 0;
    char *text_ = nullptr;

    if (text) {
        TextHashMap::AddPtr p = textMap.lookupForAdd(text);
        if (!p) {
            
            text_ = strdup(text);
            if (!text_)
                return;
            textId = nextTextId++;
            if (!textMap.add(p, text, textId))
                return;
        } else {
            textId = p->value;
        }
    }

    entries[curEntry++] = Entry(now, text_, textId, number, type);

    
    if (curEntry >= numEntries)
        grow();
}

void
TraceLogging::log(Type type, const JS::CompileOptions &options)
{
    this->log(type, options.filename, options.lineno);
}

void
TraceLogging::log(Type type, JSScript* script)
{
    this->log(type, script->filename(), script->lineno);
}

void
TraceLogging::log(const char* log)
{
    this->log(INFO, log, 0);
}

void
TraceLogging::flush()
{
    
    if (!out) {
        switch(id) {
          case DEFAULT:
            out = fopen(TRACE_LOG_DIR "tracelogging.log", "w");
            break;
          case ION_BACKGROUND_COMPILER:
            out = fopen(TRACE_LOG_DIR "tracelogging-compile.log", "w");
            break;
          case GC_BACKGROUND:
            out = fopen(TRACE_LOG_DIR "tracelogging-gc.log", "w");
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("Bad trigger");
            return;
        }
    }

    
    for (unsigned int i = 0; i < curEntry; i++) {
        Entry entry = entries[i];
        int written;
        if (entry.type() == INFO) {
            written = fprintf(out, "I,%s\n", entry.text());
        } else {
            if (entry.textId() > 0) {
                if (entry.text()) {
                    written = fprintf(out, "%llu,%s,%s,%d\n",
                                      (unsigned long long)entry.tick(),
                                      typeName[entry.type()],
                                      entry.text(),
                                      entry.lineno());
                } else {
                    written = fprintf(out, "%llu,%s,%d,%d\n",
                                      (unsigned long long)entry.tick(),
                                      typeName[entry.type()],
                                      entry.textId(),
                                      entry.lineno());
                }
            } else {
                written = fprintf(out, "%llu,%s\n",
                                  (unsigned long long)entry.tick(),
                                  typeName[entry.type()]);
            }
        }

        
        if (written < 0) {
            fprintf(stderr, "Writing tracelog to disk failed,");
            fprintf(stderr, "probably because the file would've exceeded the maximum size of 2GB");
            fclose(out);
            exit(-1);
        }

        if (entries[i].text() != nullptr) {
            free(entries[i].text());
            entries[i].text_ = nullptr;
        }
    }
    curEntry = 0;
}

TraceLogging*
TraceLogging::getLogger(Logger id)
{
    if (!loggers[id]) {
        loggers[id] = new TraceLogging(id);
        if (!atexitSet) {
            startupTime = rdtsc();
            atexit (releaseLoggers);
            atexitSet = true;
        }
    }

    return loggers[id];
}

void
TraceLogging::releaseLoggers()
{
    for (size_t i = 0; i < LAST_LOGGER; i++) {
        if (!loggers[i])
            continue;

        delete loggers[i];
        loggers[i] = nullptr;
    }
}


void
js::TraceLog(TraceLogging* logger, TraceLogging::Type type, JSScript* script)
{
    logger->log(type, script);
}

void
js::TraceLog(TraceLogging* logger, const char* log)
{
    logger->log(log);
}

void
js::TraceLog(TraceLogging* logger, TraceLogging::Type type)
{
    logger->log(type);
}
