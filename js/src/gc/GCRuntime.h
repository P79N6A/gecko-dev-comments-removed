





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


#if defined(DEBUG) && !defined(MOZ_B2G)
#define JS_GC_MARKING_VALIDATION
#endif

namespace js {

namespace gc {

typedef Vector<JS::Zone *, 4, SystemAllocPolicy> ZoneVector;

class MarkingValidator;
struct AutoPrepareForTracing;
class AutoTraceSession;

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
    explicit GCRuntime(JSRuntime *rt);
    bool init(uint32_t maxbytes);
    void finish();

    inline int zeal();
    inline bool upcomingZealousGC();
    inline bool needZealousGC();

    template <typename T> bool addRoot(T *rp, const char *name, JSGCRootType rootType);
    void removeRoot(void *rp);
    void setMarkStackLimit(size_t limit);

    void setParameter(JSGCParamKey key, uint32_t value);
    uint32_t getParameter(JSGCParamKey key);

    bool isHeapBusy() { return heapState != js::Idle; }
    bool isHeapMajorCollecting() { return heapState == js::MajorCollecting; }
    bool isHeapMinorCollecting() { return heapState == js::MinorCollecting; }
    bool isHeapCollecting() { return isHeapMajorCollecting() || isHeapMinorCollecting(); }

    
    
    
    
    
    bool isFJMinorCollecting() { return fjCollectionCounter > 0; }
    void incFJMinorCollecting() { fjCollectionCounter++; }
    void decFJMinorCollecting() { fjCollectionCounter--; }

    bool triggerGC(JS::gcreason::Reason reason);
    bool triggerZoneGC(Zone *zone, JS::gcreason::Reason reason);
    void maybeGC(Zone *zone);
    void minorGC(JS::gcreason::Reason reason);
    void minorGC(JSContext *cx, JS::gcreason::Reason reason);
    void gcIfNeeded(JSContext *cx);
    void collect(bool incremental, int64_t budget, JSGCInvocationKind gckind,
                 JS::gcreason::Reason reason);
    void gcSlice(JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis = 0);
    void runDebugGC();
    inline void poke();

    void markRuntime(JSTracer *trc, bool useSavedRoots = false);

    void notifyDidPaint();
    void shrinkBuffers();

#ifdef JS_GC_ZEAL
    const void *addressOfZealMode() { return &zealMode; }
    void setZeal(uint8_t zeal, uint32_t frequency);
    void setNextScheduled(uint32_t count);
    void verifyPreBarriers();
    void verifyPostBarriers();
    void maybeVerifyPreBarriers(bool always);
    void maybeVerifyPostBarriers(bool always);
    bool selectForMarking(JSObject *object);
    void clearSelectedForMarking();
    void setDeterministic(bool enable);
#endif

  public:
    
    js::gc::State state() { return incrementalState; }
    void recordNativeStackTop();
#ifdef JS_THREADSAFE
    void notifyRequestEnd() { conservativeGC.updateForRequestEnd(); }
#endif
    bool isBackgroundSweeping() { return helperState.isBackgroundSweeping(); }
    void waitBackgroundSweepEnd() { helperState.waitBackgroundSweepEnd(); }
    void waitBackgroundSweepOrAllocEnd() { helperState.waitBackgroundSweepOrAllocEnd(); }
    void startBackgroundAllocationIfIdle() { helperState.startBackgroundAllocationIfIdle(); }
    void freeLater(void *p) { helperState.freeLater(p); }

#ifdef DEBUG

    bool onBackgroundThread() { return helperState.onBackgroundThread(); }

    bool currentThreadOwnsGCLock() {
#ifdef JS_THREADSAFE
        return lockOwner == PR_GetCurrentThread();
#else
        return true;
#endif
    }

#endif 

#ifdef JS_THREADSAFE
    void assertCanLock() {
        JS_ASSERT(!currentThreadOwnsGCLock());
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

    bool isInsideUnsafeRegion() { return inUnsafeRegion != 0; }
    void enterUnsafeRegion() { ++inUnsafeRegion; }
    void leaveUnsafeRegion() {
        JS_ASSERT(inUnsafeRegion > 0);
        --inUnsafeRegion;
    }
#endif

    void setAlwaysPreserveCode() { alwaysPreserveCode = true; }

    bool isIncrementalGCEnabled() { return incrementalEnabled; }
    void disableIncrementalGC() { incrementalEnabled = false; }

    bool isGenerationalGCEnabled() { return generationalDisabled == 0; }
    void disableGenerationalGC();
    void enableGenerationalGC();

    void setGrayRootsTracer(JSTraceDataOp traceOp, void *data);
    bool addBlackRootsTracer(JSTraceDataOp traceOp, void *data);
    void removeBlackRootsTracer(JSTraceDataOp traceOp, void *data);

    void setMaxMallocBytes(size_t value);
    void resetMallocBytes();
    bool isTooMuchMalloc() const { return mallocBytes <= 0; }
    void updateMallocCounter(JS::Zone *zone, size_t nbytes);
    void onTooMuchMalloc();

    void setGCCallback(JSGCCallback callback, void *data);
    bool addFinalizeCallback(JSFinalizeCallback callback, void *data);
    void removeFinalizeCallback(JSFinalizeCallback func);
    JS::GCSliceCallback setSliceCallback(JS::GCSliceCallback callback);

    void setValidate(bool enable);
    void setFullCompartmentChecks(bool enable);

    bool isManipulatingDeadZones() { return manipulatingDeadZones; }
    void setManipulatingDeadZones(bool value) { manipulatingDeadZones = value; }
    unsigned objectsMarkedInDeadZonesCount() { return objectsMarkedInDeadZones; }
    void incObjectsMarkedInDeadZone() {
        JS_ASSERT(manipulatingDeadZones);
        ++objectsMarkedInDeadZones;
    }

    JS::Zone *getCurrentZoneGroup() { return currentZoneGroup; }
    void setFoundBlackGrayEdges() { foundBlackGrayEdges = true; }

#ifdef JS_GC_ZEAL
    void startVerifyPreBarriers();
    bool endVerifyPreBarriers();
    void startVerifyPostBarriers();
    bool endVerifyPostBarriers();
    void finishVerifier();
#endif

  private:
    
    friend class ArenaLists;
    Chunk *pickChunk(Zone *zone, AutoMaybeStartBackgroundAllocation &maybeStartBackgroundAllocation);
    inline void arenaAllocatedDuringGC(JS::Zone *zone, ArenaHeader *arena);

    inline bool wantBackgroundAllocation() const;

    bool initZeal();
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
    void decommitArenasFromAvailableList(Chunk **availableListHeadp);
    void decommitArenas();
    void expireChunksAndArenas(bool shouldShrink);
    void sweepBackgroundThings(bool onBackgroundThread);
    void assertBackgroundSweepingFinished();

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

#ifdef JSGC_GENERATIONAL
    js::Nursery           nursery;
    js::gc::StoreBuffer   storeBuffer;
#endif

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

  private:
    



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

#ifdef JS_GC_MARKING_VALIDATION
    js::gc::MarkingValidator *markingValidator;
#endif

    




    volatile uintptr_t    interFrameGC;

    
    int64_t               sliceBudget;

    



    bool                  incrementalEnabled;

    


    unsigned              generationalDisabled;

    




    bool                  manipulatingDeadZones;

    







    unsigned              objectsMarkedInDeadZones;

    bool                  poked;

    volatile js::HeapState heapState;

    







    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> fjCollectionCounter;

    























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

    Callback<JSGCCallback>  gcCallback;
    CallbackVector<JSFinalizeCallback> finalizeCallbacks;

    



    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire>   mallocBytes;

    



    mozilla::Atomic<bool, mozilla::ReleaseAcquire>   mallocGCTriggered;

    





    CallbackVector<JSTraceDataOp> blackRootTracers;
    Callback<JSTraceDataOp> grayRootTracer;

#ifdef DEBUG
    





    int inUnsafeRegion;
#endif

    
    bool                  alwaysPreserveCode;

#ifdef DEBUG
    size_t                noGCOrAllocationCheck;
#endif

    
    PRLock                *lock;
    mozilla::DebugOnly<PRThread *>   lockOwner;

    GCHelperState helperState;

    ConservativeGCData conservativeGC;

    friend class js::GCHelperState;
    friend class js::gc::MarkingValidator;
    friend class js::gc::AutoTraceSession;
};

#ifdef JS_GC_ZEAL
inline int
GCRuntime::zeal() {
    return zealMode;
}

inline bool
GCRuntime::upcomingZealousGC() {
    return nextScheduled == 1;
}

inline bool
GCRuntime::needZealousGC() {
    if (nextScheduled > 0 && --nextScheduled == 0) {
        if (zealMode == ZealAllocValue ||
            zealMode == ZealGenerationalGCValue ||
            (zealMode >= ZealIncrementalRootsThenFinish &&
             zealMode <= ZealIncrementalMultipleSlices))
        {
            nextScheduled = zealFrequency;
        }
        return true;
    }
    return false;
}
#else
inline int GCRuntime::zeal() { return 0; }
inline bool GCRuntime::upcomingZealousGC() { return false; }
inline bool GCRuntime::needZealousGC() { return false; }
#endif

} 
} 

#endif
