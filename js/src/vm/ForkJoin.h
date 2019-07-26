






#ifndef ForkJoin_h__
#define ForkJoin_h__

#include "vm/ThreadPool.h"
#include "jsgc.h"


























































































namespace js {




enum ParallelResult { TP_SUCCESS, TP_RETRY_SEQUENTIALLY, TP_FATAL };

struct ForkJoinOp;



uint32_t
ForkJoinSlices(JSContext *cx);





ParallelResult ExecuteForkJoinOp(JSContext *cx, ForkJoinOp &op);

class PerThreadData;
class ForkJoinShared;
class AutoRendezvous;
class AutoSetForkJoinSlice;

#ifdef DEBUG
struct IonLIRTraceData {
    uint32_t bblock;
    uint32_t lir;
    uint32_t execModeInt;
    const char *lirOpName;
    const char *mirOpName;
    JSScript *script;
    jsbytecode *pc;
};
#endif

struct ForkJoinSlice
{
  public:
    
    PerThreadData *perThreadData;

    
    const uint32_t sliceId;

    
    const uint32_t numSlices;

    
    
    
    Allocator *const allocator;

    
    JSScript *abortedScript;

#ifdef DEBUG
    
    IonLIRTraceData traceData;
#endif

    ForkJoinSlice(PerThreadData *perThreadData, uint32_t sliceId, uint32_t numSlices,
                  Allocator *arenaLists, ForkJoinShared *shared);

    
    bool isMainThread();

    
    
    
    
    
    
    
    
    void requestGC(gcreason::Reason reason);
    void requestZoneGC(JS::Zone *zone, gcreason::Reason reason);

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool check();

    
    JSRuntime *runtime();

    
    static inline ForkJoinSlice *Current();

    
    static bool InitializeTLS();

  private:
    friend class AutoRendezvous;
    friend class AutoSetForkJoinSlice;

#if defined(JS_THREADSAFE) && defined(JS_ION)
    
    static unsigned ThreadPrivateIndex;
    static bool TLSInitialized;
#endif

#if defined(JS_THREADSAFE) && defined(JS_ION)
    
    
    
    void triggerAbort();
#endif

    ForkJoinShared *const shared;
};



struct ForkJoinOp
{
  public:
    
    
    
    
    virtual bool parallel(ForkJoinSlice &slice) = 0;
};

static inline bool
InParallelSection()
{
    return ForkJoinSlice::Current() != NULL;
}

} 

 inline js::ForkJoinSlice *
js::ForkJoinSlice::Current()
{
#if defined(JS_THREADSAFE) && defined(JS_ION)
    return (ForkJoinSlice*) PR_GetThreadPrivate(ThreadPrivateIndex);
#else
    return NULL;
#endif
}

#endif 
