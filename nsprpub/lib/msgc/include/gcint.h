




































#ifndef gcint_h___
#define gcint_h___

#include "prmon.h"
#include "prgc.h"

extern PRLogModuleInfo *_pr_msgc_lm;
extern GCInfo _pr_gcData;

#if defined(_WIN32) && !defined(DEBUG)
#undef INLINE_LOCK
#endif

#ifdef INLINE_LOCK
#define LOCK_GC()       EnterCriticalSection(&_pr_gcData.lock->mutexHandle)
#define UNLOCK_GC()     LeaveCriticalSection(&_pr_gcData.lock->mutexHandle)
#else
#define LOCK_GC()       PR_EnterMonitor(_pr_gcData.lock)
#define UNLOCK_GC()     PR_ExitMonitor (_pr_gcData.lock)
#define GC_IS_LOCKED()  (PR_GetMonitorEntryCount(_pr_gcData.lock)!=0)
#endif

#ifdef DEBUG
#define _GCTRACE(x, y) if (_pr_gcData.flags & x) GCTrace y
#else
#define _GCTRACE(x, y)
#endif

extern GCBeginGCHook *_pr_beginGCHook;
extern void *_pr_beginGCHookArg;
extern GCBeginGCHook *_pr_endGCHook;
extern void *_pr_endGCHookArg;

extern GCBeginFinalizeHook *_pr_beginFinalizeHook;
extern void *_pr_beginFinalizeHookArg;
extern GCBeginFinalizeHook *_pr_endFinalizeHook;
extern void *_pr_endFinalizeHookArg;

extern int _pr_do_a_dump;
extern FILE *_pr_dump_file;

extern PRLogModuleInfo *_pr_gc_lm;





typedef struct RootFinderStr RootFinder;

struct RootFinderStr {
    RootFinder *next;
    GCRootFinder *func;
    char *name;
    void *arg;
};
extern RootFinder *_pr_rootFinders;

typedef struct CollectorTypeStr {
    GCType gctype;
    PRUint32 flags;
} CollectorType;

#define GC_MAX_TYPES	256
extern CollectorType *_pr_collectorTypes;

#define _GC_TYPE_BUSY   0x1
#define _GC_TYPE_FINAL  0x2
#define _GC_TYPE_WEAK   0x4


#define FREE_MEMORY_TYPEIX 255

extern void _PR_InitGC(PRWord flags);
extern void _MD_InitGC(void);
extern void PR_CALLBACK _PR_ScanFinalQueue(void *notused);




extern void *_MD_GrowGCHeap(PRUint32 *sizep);




extern PRBool _MD_ExtendGCHeap(char *base, PRInt32 oldSize, PRInt32 newSize);




extern void _MD_FreeGCSegment(void *base, PRInt32 len);

#endif 
