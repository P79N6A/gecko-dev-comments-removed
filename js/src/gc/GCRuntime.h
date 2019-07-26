





#ifndef gc_GCRuntime_h
#define gc_GCRuntime_h

#include "jsgc.h"

#include "gc/Heap.h"
#ifdef JSGC_GENERATIONAL
# include "gc/Nursery.h"
#endif
#include "gc/Statistics.h"
#ifdef JSGC_GENERATIONAL
# include "gc/StoreBuffer.h"
#endif
#include "gc/Tracer.h"

namespace js {

struct ScriptAndCounts
{
    
    JSScript *script;
    ScriptCounts scriptCounts;

    PCCounts &getPCCounts(jsbytecode *pc) const {
        return scriptCounts.pcCountsVector[script->pcToOffset(pc)];
    }

    jit::IonScriptCounts *getIonCounts() const {
        return scriptCounts.ionCounts;
    }
};

typedef Vector<ScriptAndCounts, 0, SystemAllocPolicy> ScriptAndCountsVector;

namespace gc {

typedef Vector<JS::Zone *, 4, SystemAllocPolicy> ZoneVector;

class MarkingValidator;

struct ConservativeGCData
{
    



    uintptr_t           *nativeStackTop;

    union {
        jmp_buf         jmpbuf;
        uintptr_t       words[JS_HOWMANY(sizeof(jmp_buf), sizeof(uintptr_t))];
    } registerSnapshot;

    ConservativeGCData() {
        mozilla::PodZero(this);
    }

    ~ConservativeGCData() {
#ifdef JS_THREADSAFE
        



        JS_ASSERT(!hasStackToScan());
#endif
    }

    MOZ_NEVER_INLINE void recordStackTop();

#ifdef JS_THREADSAFE
    void updateForRequestEnd() {
        nativeStackTop = nullptr;
    }
#endif

    bool hasStackToScan() const {
        return !!nativeStackTop;
    }
};

class GCRuntime
{
  public:
    GCRuntime(JSRuntime *rt);

  public:  

    
    JS::Zone              *systemZone;

    
    js::gc::ZoneVector    zones;

    js::gc::SystemPageAllocator pageAllocator;

    




    js::GCChunkSet        chunkSet;

    






    js::gc::Chunk         *systemAvailableChunkListHead;
    js::gc::Chunk         *userAvailableChunkListHead;
    js::gc::ChunkPool     chunkPool;

    js::RootedValueMap    rootsHash;

    
    mozilla::Atomic<size_t, mozilla::ReleaseAcquire>   bytes;

    size_t                maxBytes;
    size_t                maxMallocBytes;

    


    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire>   numArenasFreeCommitted;
    js::GCMarker          marker;
    void                  *verifyPreData;
    void                  *verifyPostData;
    bool                  chunkAllocationSinceLastGC;
    int64_t               nextFullGCTime;
    int64_t               lastGCTime;
    int64_t               jitReleaseTime;

    JSGCMode              mode;

    size_t                allocationThreshold;
    bool                  highFrequencyGC;
    uint64_t              highFrequencyTimeThreshold;
    uint64_t              highFrequencyLowLimitBytes;
    uint64_t              highFrequencyHighLimitBytes;
    double                highFrequencyHeapGrowthMax;
    double                highFrequencyHeapGrowthMin;
    double                lowFrequencyHeapGrowth;
    bool                  dynamicHeapGrowth;
    bool                  dynamicMarkSlice;
    uint64_t              decommitThreshold;

    
    bool                  shouldCleanUpEverything;

    



    bool                  grayBitsValid;

    




    volatile uintptr_t    isNeeded;

    js::gcstats::Statistics stats;

    
    uint64_t              number;

    
    uint64_t              startNumber;

    
    bool                  isIncremental;

    
    bool                  isFull;

    
    JS::gcreason::Reason  triggerReason;

    



    bool                  strictCompartmentChecking;

#ifdef DEBUG
    





    uintptr_t             disableStrictProxyCheckingCount;
#else
    uintptr_t             unused1;
#endif

    



    js::gc::State         incrementalState;

    
    bool                  lastMarkSlice;

    
    bool                  sweepOnBackgroundThread;

    
    bool                  foundBlackGrayEdges;

    
    JS::Zone              *sweepingZones;

    
    unsigned              zoneGroupIndex;

    


    JS::Zone              *zoneGroups;
    JS::Zone              *currentZoneGroup;
    int                   sweepPhase;
    JS::Zone              *sweepZone;
    int                   sweepKindIndex;
    bool                  abortSweepAfterCurrentGroup;

    


    js::gc::ArenaHeader   *arenasAllocatedDuringSweep;

#ifdef DEBUG
    js::gc::MarkingValidator *markingValidator;
#endif

    




    volatile uintptr_t    interFrameGC;

    
    int64_t               sliceBudget;

    



    bool                  incrementalEnabled;

    


    unsigned              generationalDisabled;

    




    bool                  manipulatingDeadZones;

    







    unsigned              objectsMarkedInDeadZones;

    bool                  poke;

    volatile js::HeapState heapState;

#ifdef JSGC_GENERATIONAL
    js::Nursery           nursery;
    js::gc::StoreBuffer   storeBuffer;
#endif

    























#ifdef JS_GC_ZEAL
    int                   zealMode;
    int                   zealFrequency;
    int                   nextScheduled;
    bool                  deterministicOnly;
    int                   incrementalLimit;

    js::Vector<JSObject *, 0, js::SystemAllocPolicy>   selectedForMarking;
#endif

    bool                  validate;
    bool                  fullCompartmentChecks;

    JSGCCallback          callback;
    JS::GCSliceCallback   sliceCallback;
    JSFinalizeCallback    finalizeCallback;

    void                  *callbackData;

    



    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire>   mallocBytes;

    



    mozilla::Atomic<bool, mozilla::ReleaseAcquire>   mallocGCTriggered;

    





    typedef js::Vector<ExtraTracer, 4, js::SystemAllocPolicy> ExtraTracerVector;
    ExtraTracerVector     blackRootTracers;
    ExtraTracer           grayRootTracer;

    



    size_t                systemPageSize;

    
    size_t                systemAllocGranularity;

    
    js::ScriptAndCountsVector *scriptAndCountsVector;

    
    bool                  alwaysPreserveCode;

#ifdef DEBUG
    size_t                noGCOrAllocationCheck;
#endif

    
    PRLock   *lock;
    mozilla::DebugOnly<PRThread *>   lockOwner;

    friend class js::GCHelperThread;

    js::GCHelperThread    helperThread;

    ConservativeGCData    conservativeGC;
};

} 
} 

#endif
