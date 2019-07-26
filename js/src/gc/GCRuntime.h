





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
class AutoPrepareForTracing;

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

template<typename F>
struct Callback {
    F op;
    void *data;

    Callback()
      : op(nullptr), data(nullptr)
    {}
    Callback(F op, void *data)
      : op(op), data(data)
    {}
};

template<typename F>
class CallbackVector : public Vector<Callback<F>, 4, SystemAllocPolicy> {};

class GCRuntime
{
  public:
    GCRuntime(JSRuntime *rt);
    bool init(uint32_t maxbytes);
    void finish();

    void setGCZeal(uint8_t zeal, uint32_t frequency);
    template <typename T> bool addRoot(T *rp, const char *name, JSGCRootType rootType);
    void removeRoot(void *rp);
    void setMarkStackLimit(size_t limit);

    bool isHeapBusy() { return heapState != js::Idle; }
    bool isHeapMajorCollecting() { return heapState == js::MajorCollecting; }
    bool isHeapMinorCollecting() { return heapState == js::MinorCollecting; }
    bool isHeapCollecting() { return isHeapMajorCollecting() || isHeapMinorCollecting(); }

    bool triggerGC(JS::gcreason::Reason reason);
    bool triggerZoneGC(Zone *zone, JS::gcreason::Reason reason);
    void maybeGC(Zone *zone);
    void minorGC(JS::gcreason::Reason reason);
    void minorGC(JSContext *cx, JS::gcreason::Reason reason);
    void gcIfNeeded(JSContext *cx);
    void collect(bool incremental, int64_t budget, JSGCInvocationKind gckind,
                 JS::gcreason::Reason reason);
    void gcSlice(JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis);
    void runDebugGC();

    void markRuntime(JSTracer *trc, bool useSavedRoots = false);

  public:
    
    void recordNativeStackTop();
    void notifyRequestEnd() { conservativeGC.updateForRequestEnd(); }
    bool isBackgroundSweeping() { return helperThread.sweeping(); }
    void waitBackgroundSweepEnd() { helperThread.waitBackgroundSweepEnd(); }
    void waitBackgroundSweepOrAllocEnd() { helperThread.waitBackgroundSweepOrAllocEnd(); }
    void startBackgroundShrink() { helperThread.startBackgroundShrink(); }
    void freeLater(void *p) { helperThread.freeLater(p); }
#ifdef DEBUG
    bool onBackgroundThread() { return helperThread.onBackgroundThread(); }
#endif

#ifdef JS_THREADSAFE
    void assertCanLock() {
        JS_ASSERT(lockOwner != PR_GetCurrentThread());
    }
#endif

    void lockGC() {
#ifdef JS_THREADSAFE
        PR_Lock(lock);
        JS_ASSERT(!lockOwner);
#ifdef DEBUG
        lockOwner = PR_GetCurrentThread();
#endif
#endif
    }

    void unlockGC() {
#ifdef JS_THREADSAFE
        JS_ASSERT(lockOwner == PR_GetCurrentThread());
        lockOwner = nullptr;
        PR_Unlock(lock);
#endif
    }

#ifdef DEBUG
    bool isAllocAllowed() { return noGCOrAllocationCheck == 0; }
    void disallowAlloc() { ++noGCOrAllocationCheck; }
    void allowAlloc() {
        JS_ASSERT(!isAllocAllowed());
        --noGCOrAllocationCheck;
    }
#endif

    void setAlwaysPreserveCode() { alwaysPreserveCode = true; }

  private:
    
    friend class ArenaLists;
    Chunk *pickChunk(Zone *zone);

    inline bool wantBackgroundAllocation() const;

    bool initGCZeal();
    void requestInterrupt(JS::gcreason::Reason reason);
    bool gcCycle(bool incremental, int64_t budget, JSGCInvocationKind gckind,
                 JS::gcreason::Reason reason);
    void budgetIncrementalGC(int64_t *budget);
    void resetIncrementalGC(const char *reason);
    void incrementalCollectSlice(int64_t budget, JS::gcreason::Reason reason,
                                 JSGCInvocationKind gckind);
    void pushZealSelectedObjects();
    bool beginMarkPhase();
    bool shouldPreserveJITCode(JSCompartment *comp, int64_t currentTime);
    void bufferGrayRoots();
    bool drainMarkStack(SliceBudget &sliceBudget, gcstats::Phase phase);
    template <class CompartmentIterT> void markWeakReferences(gcstats::Phase phase);
    void markWeakReferencesInCurrentGroup(gcstats::Phase phase);
    template <class ZoneIterT, class CompartmentIterT> void markGrayReferences();
    void markGrayReferencesInCurrentGroup();
    void beginSweepPhase(bool lastGC);
    void findZoneGroups();
    bool findZoneEdgesForWeakMaps();
    void getNextZoneGroup();
    void endMarkingZoneGroup();
    void beginSweepingZoneGroup();
    bool releaseObservedTypes();
    void endSweepingZoneGroup();
    bool sweepPhase(SliceBudget &sliceBudget);
    void endSweepPhase(JSGCInvocationKind gckind, bool lastGC);
    void sweepZones(FreeOp *fop, bool lastGC);

    void computeNonIncrementalMarkingForValidation();
    void validateIncrementalMarking();
    void finishMarkingValidation();

    void markConservativeStackRoots(JSTracer *trc, bool useSavedRoots);

#ifdef DEBUG
    void checkForCompartmentMismatches();
    void markAllWeakReferences(gcstats::Phase phase);
    void markAllGrayReferences();
#endif

  public:  
    JSRuntime             *rt;

    
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
    int                   finalizePhase;
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

    JSGCCallback          gcCallback;
    void                  *gcCallbackData;

    JS::GCSliceCallback   sliceCallback;
    CallbackVector<JSFinalizeCallback> finalizeCallbacks;

    



    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire>   mallocBytes;

    



    mozilla::Atomic<bool, mozilla::ReleaseAcquire>   mallocGCTriggered;

    





    CallbackVector<JSTraceDataOp> blackRootTracers;
    Callback<JSTraceDataOp> grayRootTracer;

    



    size_t                systemPageSize;

    
    size_t                systemAllocGranularity;

    
    js::ScriptAndCountsVector *scriptAndCountsVector;

  private:
    
    bool                  alwaysPreserveCode;

#ifdef DEBUG
    size_t                noGCOrAllocationCheck;
#endif

    
    PRLock                *lock;
    mozilla::DebugOnly<PRThread *>   lockOwner;

    js::GCHelperThread helperThread;

    ConservativeGCData conservativeGC;

    
    friend class js::GCHelperThread;
    friend class js::gc::MarkingValidator;
};

} 
} 

#endif
