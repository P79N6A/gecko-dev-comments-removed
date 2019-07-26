





#include "TraceLogging.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#ifndef TRACE_LOG_DIR
# if defined(_WIN32)
#  define TRACE_LOG_DIR ""
# else
#  define TRACE_LOG_DIR "/tmp/"
# endif
#endif

namespace js {

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

const char* TraceLogging::type_name[] = {
    "start,ion_compile",
    "stop,ion_compile",
    "start,ion_cannon",
    "stop,ion_cannon",
    "stop,ion_cannon_bailout",
    "start,ion_side_cannon",
    "stop,ion_side_cannon",
    "stop,ion_side_cannon_bailout",
    "start,yarr_jit_execute",
    "stop,yarr_jit_execute",
    "start,jm_safepoint",
    "stop,jm_safepoint",
    "start,jm_normal",
    "stop,jm_normal",
    "start,jm_compile",
    "stop,jm_compile",
    "start,gc",
    "stop,gc",
    "start,interpreter",
    "stop,interpreter"
};
TraceLogging* TraceLogging::_defaultLogger = NULL;

TraceLogging::TraceLogging()
  : loggingTime(0),
    entries(NULL),
    curEntry(0),
    numEntries(1000000),
    fileno(0),
    out(NULL)
{
}

TraceLogging::~TraceLogging()
{
    if (out != NULL) {
        fclose(out);
        out = NULL;
    }

    if (entries != NULL) {
        flush();
        free(entries);
        entries = NULL;
    }
}

void
TraceLogging::grow()
{
    Entry* nentries = (Entry*) realloc(entries, numEntries*2*sizeof(Entry));

    
    
    if (nentries == NULL) {
        flush();
        return;
    }

    entries = nentries;
    numEntries *= 2;
}

void
TraceLogging::log(Type type, const char* file, unsigned int lineno)
{
    uint64_t now = rdtsc();

    
    if (entries == NULL) {
        entries = (Entry*) malloc(numEntries*sizeof(Entry));
        if (entries == NULL)
            return;
    }

    
    
    char *copy = NULL;
    if (file != NULL)
        copy = strdup(file);

    entries[curEntry++] = Entry(now - loggingTime, copy, lineno, type);

    
    if (curEntry >= numEntries)
        grow();

    
    
    loggingTime += rdtsc()-now;
}

void
TraceLogging::log(Type type, JSScript* script)
{
    this->log(type, script->filename, script->lineno);
}

void
TraceLogging::log(const char* log)
{
    this->log(INFO, log, 0);
}

void
TraceLogging::log(Type type)
{
    this->log(type, NULL, 0);
}

void
TraceLogging::flush()
{
    
    if (out == NULL)
        out = fopen(TRACE_LOG_DIR "tracelogging.log", "w");

    
    for (unsigned int i = 0; i < curEntry; i++) {
        int written;
        if (entries[i].type() == INFO) {
            written = fprintf(out, "INFO,%s\n", entries[i].file());
        } else {
            if (entries[i].file() == NULL) {
                written = fprintf(out, "%llu,%s\n",
                                  (unsigned long long)entries[i].tick(),
                                  type_name[entries[i].type()]);
            } else {
                written = fprintf(out, "%llu,%s,%s:%d\n",
                                  (unsigned long long)entries[i].tick(),
                                  type_name[entries[i].type()],
                                  entries[i].file(),
                                  entries[i].lineno());
            }
        }

        
        
        
        if (written < 0) {
            fclose(out);
            if (fileno >= 9999)
                exit(-1);

            char filename[21 + sizeof(TRACE_LOG_DIR)];
            sprintf (filename, TRACE_LOG_DIR "tracelogging-%d.log", ++fileno);
            out = fopen(filename, "w");
            i--; 
            continue;
        }

        if (entries[i].file() != NULL) {
            free(entries[i].file());
            entries[i].file_ = NULL;
        }
    }
    curEntry = 0;
}

TraceLogging*
TraceLogging::defaultLogger()
{
    if (_defaultLogger == NULL) {
        _defaultLogger = new TraceLogging();
        atexit (releaseDefaultLogger);
    }
    return _defaultLogger;
}

void
TraceLogging::releaseDefaultLogger()
{
    if (_defaultLogger != NULL) {
        delete _defaultLogger;
        _defaultLogger = NULL;
    }
}


void
TraceLog(TraceLogging* logger, TraceLogging::Type type, JSScript* script)
{
    logger->log(type, script);
}
void
TraceLog(TraceLogging* logger, const char* log)
{
    logger->log(log);
}
void
TraceLog(TraceLogging* logger, TraceLogging::Type type)
{
    logger->log(type);
}

}  
