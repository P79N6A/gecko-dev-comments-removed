






#ifndef jstaskset_h___
#define jstaskset_h___

#include "threadpool.h"















































































namespace js {





enum ParallelResult { TP_SUCCESS, TP_RETRY_SEQUENTIALLY, TP_FATAL };

struct ForkJoinOp;





ParallelResult ExecuteForkJoinOp(JSContext *cx, ForkJoinOp &op);

class ForkJoinShared;
class AutoRendezvous;
class AutoSetForkJoinSlice;
namespace gc { struct ArenaLists; }

struct ForkJoinSlice
{
public:
    
    PerThreadData *perThreadData;

    
    const size_t sliceId;

    
    const size_t numSlices;

    
    uintptr_t ionStackLimit;

    
    
    
    gc::ArenaLists *const arenaLists;

    ForkJoinSlice(PerThreadData *perThreadData, size_t sliceId, size_t numSlices,
                  uintptr_t stackLimit, gc::ArenaLists *arenaLists,
                  ForkJoinShared *shared);

    
    bool isMainThread();

    
    
    
    
    
    
    
    bool setFatal();

    
    
    
    
    
    bool check();

    
    JSRuntime *runtime();

    static inline ForkJoinSlice *current();
    static bool Initialize();

private:
    friend class AutoRendezvous;
    friend class AutoSetForkJoinSlice;

    static PRUintn ThreadPrivateIndex; 

    ForkJoinShared *const shared;
};



struct ForkJoinOp
{
public:
    
    
    
    
    
    virtual bool pre(size_t numSlices) = 0;

    
    
    
    
    
    virtual bool parallel(ForkJoinSlice &slice) = 0;

    
    
    
    
    virtual bool post(size_t numSlices) = 0;
};



static inline bool InParallelSection() {
#   ifdef JS_THREADSAFE_ION
    return ForkJoinSlice::current() != NULL;
#   else
    return false;
#   endif
}

#endif

}
