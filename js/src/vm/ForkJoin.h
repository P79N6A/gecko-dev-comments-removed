






#ifndef ForkJoin_h__
#define ForkJoin_h__

#include "vm/ThreadPool.h"


























































































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
struct IonTraceData
{
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
    IonTraceData traceData;
#endif

    ForkJoinSlice(PerThreadData *perThreadData, uint32_t sliceId, uint32_t numSlices,
                  Allocator *arenaLists, ForkJoinShared *shared);

    
    bool isMainThread();

    
    
    
    
    
    
    
    bool setFatal();

    
    
    
    
    
    
    
    
    void requestGC(gcreason::Reason reason);
    void requestCompartmentGC(JSCompartment *compartment, gcreason::Reason reason);

    
    
    
    
    
    bool check();

    
    JSRuntime *runtime();

    
    static inline ForkJoinSlice *Current();
    static inline bool InParallelSection();

    static bool Initialize();

  private:
    friend class AutoRendezvous;
    friend class AutoSetForkJoinSlice;

#ifdef JS_THREADSAFE
    
    static unsigned ThreadPrivateIndex;
#endif

#ifdef JS_THREADSAFE
    
    
    
    void triggerAbort();
#endif

    ForkJoinShared *const shared;
};



struct ForkJoinOp
{
  public:
    
    
    
    
    virtual bool parallel(ForkJoinSlice &slice) = 0;
};

} 

 inline js::ForkJoinSlice *
js::ForkJoinSlice::Current()
{
#ifdef JS_THREADSAFE
    return (ForkJoinSlice*) PR_GetThreadPrivate(ThreadPrivateIndex);
#else
    return NULL;
#endif
}

 inline bool
js::ForkJoinSlice::InParallelSection()
{
    return Current() != NULL;
}

#endif 
