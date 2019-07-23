




































#ifndef prgc_h___
#define prgc_h___




#include "prtypes.h"
#include "prmon.h"
#include "prthread.h"
#include <stdio.h>

#if defined(WIN16)
#define GCPTR __far
#else
#define GCPTR
#endif


PR_BEGIN_EXTERN_C









PR_EXTERN(void) PR_InitGC(
    PRWord flags, PRInt32 initialHeapSize, PRInt32 segmentSize, PRThreadScope scope);




PR_EXTERN(void) PR_ShutdownGC(PRBool finalizeOnExit);





typedef PRInt32 (*PRWalkFun)(void GCPTR* obj, void* data);






typedef struct GCType {
    




    void (PR_CALLBACK *scan)(void GCPTR *obj);

    








    void (PR_CALLBACK *finalize)(void GCPTR *obj);

    



    void (PR_CALLBACK *dump)(FILE *out, void GCPTR *obj, PRBool detailed, PRIntn indentLevel);

    


    void (PR_CALLBACK *summarize)(void GCPTR *obj, PRUint32 bytes);

    


    void (PR_CALLBACK *free)(void *obj);

    


    PRUint32 (PR_CALLBACK *getWeakLinkOffset)(void *obj);

    
    char kindChar;

    



    PRInt32 (PR_CALLBACK *walk)(void GCPTR *obj, PRWalkFun fun, void* data);
} GCType;




 
typedef struct PRSummaryEntry {
    void* clazz;
    PRInt32 instancesCount;
    PRInt32 totalSize;
} PRSummaryEntry;






typedef void (PR_CALLBACK *PRSummaryPrinter)(FILE *out, void* closure);

PR_EXTERN(void) PR_CALLBACK PR_RegisterSummaryPrinter(PRSummaryPrinter fun, void* closure);

typedef void PR_CALLBACK GCRootFinder(void *arg);
typedef void PR_CALLBACK GCBeginFinalizeHook(void *arg);
typedef void PR_CALLBACK GCEndFinalizeHook(void *arg);
typedef void PR_CALLBACK GCBeginGCHook(void *arg);
typedef void PR_CALLBACK GCEndGCHook(void *arg);

typedef enum { PR_GCBEGIN, PR_GCEND } GCLockHookArg;

typedef void PR_CALLBACK GCLockHookFunc(GCLockHookArg arg1, void *arg2);

typedef struct GCLockHook GCLockHook;

struct GCLockHook {
  GCLockHookFunc* func;
  void* arg;
  GCLockHook* next;
  GCLockHook* prev;
};









PR_EXTERN(void) PR_CALLBACK PR_SetBeginGCHook(GCBeginGCHook *hook, void *arg);
PR_EXTERN(void) PR_CALLBACK PR_GetBeginGCHook(GCBeginGCHook **hook, void **arg);
PR_EXTERN(void) PR_CALLBACK PR_SetEndGCHook(GCBeginGCHook *hook, void *arg);
PR_EXTERN(void) PR_CALLBACK PR_GetEndGCHook(GCEndGCHook **hook, void **arg);











PR_EXTERN(int) PR_RegisterGCLockHook(GCLockHookFunc *hook, void *arg);









PR_EXTERN(void) PR_SetBeginFinalizeHook(GCBeginFinalizeHook *hook, void *arg);
PR_EXTERN(void) PR_GetBeginFinalizeHook(GCBeginFinalizeHook **hook, void **arg);
PR_EXTERN(void) PR_SetEndFinalizeHook(GCBeginFinalizeHook *hook, void *arg);
PR_EXTERN(void) PR_GetEndFinalizeHook(GCEndFinalizeHook **hook, void **arg);







PR_EXTERN(PRInt32) PR_RegisterType(GCType *type);






PR_EXTERN(PRStatus) PR_RegisterRootFinder(GCRootFinder func, char *name, void *arg);













PR_EXTERN(PRWord GCPTR *)PR_AllocMemory(
    PRWord bytes, PRInt32 typeIndex, PRWord flags);
PR_EXTERN(PRWord GCPTR *)PR_AllocSimpleMemory(
    PRWord bytes, PRInt32 typeIndex);






PR_EXTERN(void) PR_EnableAllocation(PRBool yesOrNo);


#define PR_ALLOC_CLEAN 0x1
#define PR_ALLOC_DOUBLE 0x2
#define PR_ALLOC_ZERO_HANDLE 0x4              /* XXX yes, it's a hack */




PR_EXTERN(void) PR_GC(void);








PR_EXTERN(void) PR_ForceFinalize(void);





PR_EXTERN(void) PR_DumpGCHeap(FILE *out, PRBool detailed);




PR_EXTERN(void) PR_DumpMemory(PRBool detailed);




PR_EXTERN(void) PR_DumpMemorySummary(void);




PR_EXTERN(void) PR_DumpApplicationHeaps(void);





PR_EXTERN(void) PR_DumpIndent(FILE *out, PRIntn indent);





















































typedef struct GCInfoStr {
    PRWord  flags;         
    PRWord  busyMemory;    
    PRWord  freeMemory;    
    PRWord  allocMemory;   
    PRWord  maxMemory;     
    PRWord *lowSeg;        
    PRWord *highSeg;       

    PRMonitor *lock;
    PRThread  *finalizer;

    void (PR_CALLBACK *liveBlock)(void **base, PRInt32 count);
    void (PR_CALLBACK *livePointer)(void *ptr);
    void (PR_CALLBACK *processRootBlock)(void **base, PRInt32 count);
    void (PR_CALLBACK *processRootPointer)(void *ptr);
    FILE* dumpOutput;
#ifdef GCTIMINGHOOK
    void (*gcTimingHook)(int32 gcTime);
#endif
} GCInfo;

PR_EXTERN(GCInfo *) PR_GetGCInfo(void);
PR_EXTERN(PRBool) PR_GC_In_Heap(void GCPTR *object);






#if !defined(XP_PC) || defined(_WIN32)
#define GC_IN_HEAP(_info, _p) (((PRWord*)(_p) >= (_info)->lowSeg) && \
                               ((PRWord*)(_p) <  (_info)->highSeg))
#else





#define GC_IN_HEAP(_info, _p) PR_GC_In_Heap(_p)
#endif

PR_EXTERN(PRWord) PR_GetObjectHeader(void *ptr);

PR_EXTERN(PRWord) PR_SetObjectHeader(void *ptr, PRWord newUserBits);




#define GC_TRACE    0x0001
#define GC_ROOTS    0x0002
#define GC_LIVE     0x0004
#define GC_ALLOC    0x0008
#define GC_MARK     0x0010
#define GC_SWEEP    0x0020
#define GC_DEBUG    0x0040
#define GC_FINAL    0x0080

#if defined(DEBUG_kipp) || defined(DEBUG_warren)
#define GC_CHECK    0x0100
#endif

#ifdef DEBUG
#define GCTRACE(x, y) if (PR_GetGCInfo()->flags & x) GCTrace y
PR_EXTERN(void) GCTrace(char *fmt, ...);
#else
#define GCTRACE(x, y)
#endif

PR_END_EXTERN_C

#endif 
