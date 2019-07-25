






































#ifndef nsTraceMalloc_h___
#define nsTraceMalloc_h___

#include "mozilla/StdInt.h"

#include <stdio.h> 
#include "prtypes.h"

#ifdef XP_WIN
#define setlinebuf(stream) setvbuf((stream), (char *)NULL, _IOLBF, 0)
#endif

PR_BEGIN_EXTERN_C







#define NS_TRACE_MALLOC_MAGIC           "XPCOM\nTMLog08\r\n\032"
#define NS_TRACE_MALLOC_MAGIC_SIZE      16




typedef struct nsTMStats {
    uint32_t calltree_maxstack;
    uint32_t calltree_maxdepth;
    uint32_t calltree_parents;
    uint32_t calltree_maxkids;
    uint32_t calltree_kidhits;
    uint32_t calltree_kidmisses;
    uint32_t calltree_kidsteps;
    uint32_t callsite_recurrences;
    uint32_t backtrace_calls;
    uint32_t backtrace_failures;
    uint32_t btmalloc_failures;
    uint32_t dladdr_failures;
    uint32_t malloc_calls;
    uint32_t malloc_failures;
    uint32_t calloc_calls;
    uint32_t calloc_failures;
    uint32_t realloc_calls;
    uint32_t realloc_failures;
    uint32_t free_calls;
    uint32_t null_free_calls;
} nsTMStats;

#define NS_TMSTATS_STATIC_INITIALIZER {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}





























































#define TM_EVENT_LIBRARY        'L'
#define TM_EVENT_FILENAME       'G'
#define TM_EVENT_METHOD         'N'
#define TM_EVENT_CALLSITE       'S'
#define TM_EVENT_MALLOC         'M'
#define TM_EVENT_CALLOC         'C'
#define TM_EVENT_REALLOC        'R'
#define TM_EVENT_FREE           'F'
#define TM_EVENT_STATS          'Z'
#define TM_EVENT_TIMESTAMP      'T'

PR_EXTERN(void) NS_TraceMallocStartup(int logfd);




PR_EXTERN(int) NS_TraceMallocStartupArgs(int argc, char* argv[]);




PR_EXTERN(PRBool) NS_TraceMallocHasStarted(void);




PR_EXTERN(void) NS_TraceMallocShutdown(void);




PR_EXTERN(void) NS_TraceMallocDisable(void);




PR_EXTERN(void) NS_TraceMallocEnable(void);







PR_EXTERN(int) NS_TraceMallocChangeLogFD(int fd);





PR_EXTERN(void) NS_TraceMallocCloseLogFD(int fd);




PR_EXTERN(void) NS_TraceMallocLogTimestamp(const char *caption);







PR_EXTERN(void)
NS_TraceStack(int skip, FILE *ofp);








PR_EXTERN(int)
NS_TraceMallocDumpAllocations(const char *pathname);




PR_EXTERN(void)
NS_TraceMallocFlushLogfiles(void);




PR_EXTERN(void)
NS_TrackAllocation(void* ptr, FILE *ofp);


typedef struct nsTMStackTraceIDStruct *nsTMStackTraceID;





PR_EXTERN(nsTMStackTraceID)
NS_TraceMallocGetStackTrace(void);




PR_EXTERN(void)
NS_TraceMallocPrintStackTrace(FILE *ofp, nsTMStackTraceID id);

PR_END_EXTERN_C

#endif 
