





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

class AutoLockGC;

namespace gc {

typedef Vector<JS::Zone *, 4, SystemAllocPolicy> ZoneVector;

struct FinalizePhase;
class MarkingValidator;
struct AutoPrepareForTracing;
class AutoTraceSession;

#ifdef JSGC_COMPACTING
struct ArenasToUpdate;
struct MovingTracer;
#endif

class ChunkPool
{
    Chunk *head_;
    size_t count_;

  public:
    ChunkPool() : head_(nullptr), count_(0) {}

    size_t count() const { return count_; }

    Chunk *head() { MOZ_ASSERT(head_); return head_; }
    Chunk *pop();
    void push(Chunk *chunk);
    Chunk *remove(Chunk *chunk);

#ifdef DEBUG
    bool contains(Chunk *chunk) const;
    bool verify() const;
#endif

    
    
    class Iter {
      public:
        explicit Iter(ChunkPool &pool) : current_(pool.head_) {}
        bool done() const { return !current_; }
        void next();
        Chunk *get() const { return current_; }
        operator Chunk *() const { return get(); }
        Chunk *operator->() const { return get(); }
      private:
        Chunk *current_;
    };

  private:
    
    
    ChunkPool(const ChunkPool &) MOZ_DELETE;
    ChunkPool operator=(const ChunkPool &) MOZ_DELETE;
};



class BackgroundAllocTask : public GCParallelTask
{
    
    JSRuntime *runtime;
    ChunkPool &chunkPool_;

    const bool enabled_;

  public:
    BackgroundAllocTask(JSRuntime *rt, ChunkPool &pool);
    bool enabled() const { return enabled_; }

  protected:
    virtual void run() MOZ_OVERRIDE;
};





class GCSchedulingTunables
{
    





    size_t gcMaxBytes_;

    




    size_t gcZoneAllocThresholdBase_;

    
    double zoneAllocThresholdFactor_;

    



    size_t zoneAllocDelayBytes_;

    



    bool dynamicHeapGrowthEnabled_;

    



    uint64_t highFrequencyThresholdUsec_;

    



    uint64_t highFrequencyLowLimitBytes_;
    uint64_t highFrequencyHighLimitBytes_;
    double highFrequencyHeapGrowthMax_;
    double highFrequencyHeapGrowthMin_;

    



    double lowFrequencyHeapGrowth_;

    


    bool dynamicMarkSliceEnabled_;

    


    unsigned minEmptyChunkCount_;
    unsigned maxEmptyChunkCount_;

  public:
    GCSchedulingTunables()
      : gcMaxBytes_(0),
        gcZoneAllocThresholdBase_(30 * 1024 * 1024),
        zoneAllocThresholdFactor_(0.9),
        zoneAllocDelayBytes_(1024 * 1024),
        dynamicHeapGrowthEnabled_(false),
        highFrequencyThresholdUsec_(1000 * 1000),
        highFrequencyLowLimitBytes_(100 * 1024 * 1024),
        highFrequencyHighLimitBytes_(500 * 1024 * 1024),
        highFrequencyHeapGrowthMax_(3.0),
        highFrequencyHeapGrowthMin_(1.5),
        lowFrequencyHeapGrowth_(1.5),
        dynamicMarkSliceEnabled_(false),
        minEmptyChunkCount_(1),
        maxEmptyChunkCount_(30)
    {}

    size_t gcMaxBytes() const { return gcMaxBytes_; }
    size_t gcZoneAllocThresholdBase() const { return gcZoneAllocThresholdBase_; }
    double zoneAllocThresholdFactor() const { return zoneAllocThresholdFactor_; }
    size_t zoneAllocDelayBytes() const { return zoneAllocDelayBytes_; }
    bool isDynamicHeapGrowthEnabled() const { return dynamicHeapGrowthEnabled_; }
    uint64_t highFrequencyThresholdUsec() const { return highFrequencyThresholdUsec_; }
    uint64_t highFrequencyLowLimitBytes() const { return highFrequencyLowLimitBytes_; }
    uint64_t highFrequencyHighLimitBytes() const { return highFrequencyHighLimitBytes_; }
    double highFrequencyHeapGrowthMax() const { return highFrequencyHeapGrowthMax_; }
    double highFrequencyHeapGrowthMin() const { return highFrequencyHeapGrowthMin_; }
    double lowFrequencyHeapGrowth() const { return lowFrequencyHeapGrowth_; }
    bool isDynamicMarkSliceEnabled() const { return dynamicMarkSliceEnabled_; }
    unsigned minEmptyChunkCount() const { return minEmptyChunkCount_; }
    unsigned maxEmptyChunkCount() const { return maxEmptyChunkCount_; }

    void setParameter(JSGCParamKey key, uint32_t value);
};





class GCSchedulingState
{
    





    bool inHighFrequencyGCMode_;

  public:
    GCSchedulingState()
      : inHighFrequencyGCMode_(false)
    {}

    bool inHighFrequencyGCMode() const { return inHighFrequencyGCMode_; }

    void updateHighFrequencyMode(uint64_t lastGCTime, uint64_t currentTime,
                                 const GCSchedulingTunables &tunables) {
        inHighFrequencyGCMode_ =
            tunables.isDynamicHeapGrowthEnabled() && lastGCTime &&
            lastGCTime + tunables.highFrequencyThresholdUsec() > currentTime;
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
    bool init(uint32_t maxbytes, uint32_t maxNurseryBytes);
    void finish();

    inline int zeal();
    inline bool upcomingZealousGC();
    inline bool needZealousGC();

    template <typename T> bool addRoot(T *rp, const char *name, JSGCRootType rootType);
    void removeRoot(void *rp);
    void setMarkStackLimit(size_t limit);

    void setParameter(JSGCParamKey key, uint32_t value);
    uint32_t getParameter(JSGCParamKey key, const AutoLockGC &lock);

    bool isHeapBusy() { return heapState != js::Idle; }
    bool isHeapMajorCollecting() { return heapState == js::MajorCollecting; }
    bool isHeapMinorCollecting() { return heapState == js::MinorCollecting; }
    bool isHeapCollecting() { return isHeapMajorCollecting() || isHeapMinorCollecting(); }
#ifdef JSGC_COMPACTING
    bool isHeapCompacting() { return isHeapMajorCollecting() && state() == COMPACT; }
#else
    bool isHeapCompacting() { return false; }
#endif

    
    
    
    
    
    bool isFJMinorCollecting() { return fjCollectionCounter > 0; }
    void incFJMinorCollecting() { fjCollectionCounter++; }
    void decFJMinorCollecting() { fjCollectionCounter--; }

    bool triggerGC(JS::gcreason::Reason reason);
    void maybeAllocTriggerZoneGC(Zone *zone, const AutoLockGC &lock);
    bool triggerZoneGC(Zone *zone, JS::gcreason::Reason reason);
    bool maybeGC(Zone *zone);
    void maybePeriodicFullGC();
    void minorGC(JS::gcreason::Reason reason);
    void minorGC(JSContext *cx, JS::gcreason::Reason reason);
    void evictNursery(JS::gcreason::Reason reason = JS::gcreason::EVICT_NURSERY) { minorGC(reason); }
    bool gcIfNeeded(JSContext *cx = nullptr);
    void gc(JSGCInvocationKind gckind, JS::gcreason::Reason reason);
    void gcSlice(JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis = 0);
    void gcFinalSlice(JSGCInvocationKind gckind, JS::gcreason::Reason reason);
    void gcDebugSlice(SliceBudget &budget);

    void runDebugGC();
    inline void poke();

    enum TraceOrMarkRuntime {
        TraceRuntime,
        MarkRuntime
    };
    enum TraceRootsOrUsedSaved {
        TraceRoots,
        UseSavedRoots
    };
    void markRuntime(JSTracer *trc,
                     TraceOrMarkRuntime traceOrMark = TraceRuntime,
                     TraceRootsOrUsedSaved rootsSource = TraceRoots);

    void notifyDidPaint();
    void shrinkBuffers();
    void onOutOfMallocMemory();
    void onOutOfMallocMemory(const AutoLockGC &lock);

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

    size_t maxMallocBytesAllocated() { return maxMallocBytes; }

  public:
    
    js::gc::State state() { return incrementalState; }
    bool isBackgroundSweeping() { return helperState.isBackgroundSweeping(); }
    void waitBackgroundSweepEnd() { helperState.waitBackgroundSweepEnd(); }
    void waitBackgroundSweepOrAllocEnd() {
        helperState.waitBackgroundSweepEnd();
        allocTask.cancel(GCParallelTask::CancelAndWait);
    }

#ifdef JSGC_GENERATIONAL
    void requestMinorGC(JS::gcreason::Reason reason);
#endif

#ifdef DEBUG

    bool onBackgroundThread() { return helperState.onBackgroundThread(); }

    bool currentThreadOwnsGCLock() {
        return lockOwner == PR_GetCurrentThread();
    }

#endif 

    void assertCanLock() {
        MOZ_ASSERT(!currentThreadOwnsGCLock());
    }

    void lockGC() {
        PR_Lock(lock);
        MOZ_ASSERT(!lockOwner);
#ifdef DEBUG
        lockOwner = PR_GetCurrentThread();
#endif
    }

    void unlockGC() {
        MOZ_ASSERT(lockOwner == PR_GetCurrentThread());
        lockOwner = nullptr;
        PR_Unlock(lock);
    }

#ifdef DEBUG
    bool isAllocAllowed() { return noGCOrAllocationCheck == 0; }
    void disallowAlloc() { ++noGCOrAllocationCheck; }
    void allowAlloc() {
        MOZ_ASSERT(!isAllocAllowed());
        --noGCOrAllocationCheck;
    }

    bool isInsideUnsafeRegion() { return inUnsafeRegion != 0; }
    void enterUnsafeRegion() { ++inUnsafeRegion; }
    void leaveUnsafeRegion() {
        MOZ_ASSERT(inUnsafeRegion > 0);
        --inUnsafeRegion;
    }

    bool isStrictProxyCheckingEnabled() { return disableStrictProxyCheckingCount == 0; }
    void disableStrictProxyChecking() { ++disableStrictProxyCheckingCount; }
    void enableStrictProxyChecking() {
        MOZ_ASSERT(disableStrictProxyCheckingCount > 0);
        --disableStrictProxyCheckingCount;
    }
#endif

    void setAlwaysPreserveCode() { alwaysPreserveCode = true; }

    bool isIncrementalGCAllowed() { return incrementalAllowed; }
    void disallowIncrementalGC() { incrementalAllowed = false; }

    bool isIncrementalGCEnabled() { return mode == JSGC_MODE_INCREMENTAL && incrementalAllowed; }
    bool isIncrementalGCInProgress() { return state() != gc::NO_INCREMENTAL && !verifyPreData; }

    bool isGenerationalGCEnabled() { return generationalDisabled == 0; }
    void disableGenerationalGC();
    void enableGenerationalGC();

#ifdef JSGC_COMPACTING
    void disableCompactingGC();
    void enableCompactingGC();
#endif

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
    bool addWeakPointerCallback(JSWeakPointerCallback callback, void *data);
    void removeWeakPointerCallback(JSWeakPointerCallback func);
    JS::GCSliceCallback setSliceCallback(JS::GCSliceCallback callback);

    void setValidate(bool enable);
    void setFullCompartmentChecks(bool enable);

    bool isManipulatingDeadZones() { return manipulatingDeadZones; }
    void setManipulatingDeadZones(bool value) { manipulatingDeadZones = value; }
    unsigned objectsMarkedInDeadZonesCount() { return objectsMarkedInDeadZones; }
    void incObjectsMarkedInDeadZone() {
        MOZ_ASSERT(manipulatingDeadZones);
        ++objectsMarkedInDeadZones;
    }

    JS::Zone *getCurrentZoneGroup() { return currentZoneGroup; }
    void setFoundBlackGrayEdges() { foundBlackGrayEdges = true; }

    uint64_t gcNumber() { return number; }
    void incGcNumber() { ++number; }

    bool isIncrementalGc() { return isIncremental; }
    bool isFullGc() { return isFull; }

    bool shouldCleanUpEverything() { return cleanUpEverything; }

    bool areGrayBitsValid() { return grayBitsValid; }
    void setGrayBitsInvalid() { grayBitsValid = false; }

    bool isGcNeeded() { return minorGCRequested || majorGCRequested; }

    double computeHeapGrowthFactor(size_t lastBytes);
    size_t computeTriggerBytes(double growthFactor, size_t lastBytes);

    JSGCMode gcMode() const { return mode; }
    void setGCMode(JSGCMode m) {
        mode = m;
        marker.setGCMode(mode);
    }

    inline void updateOnFreeArenaAlloc(const ChunkInfo &info);
    inline void updateOnArenaFree(const ChunkInfo &info);

    GCChunkSet::Range allChunks() { return chunkSet.all(); }
    void moveChunkToFreePool(Chunk *chunk, const AutoLockGC &lock);
    bool hasChunk(Chunk *chunk) { return chunkSet.has(chunk); }
    ChunkPool &availableChunks(const AutoLockGC &lock) { return availableChunks_; }
    ChunkPool &emptyChunks(const AutoLockGC &lock) { return emptyChunks_; }
    const ChunkPool &availableChunks(const AutoLockGC &lock) const { return availableChunks_; }
    const ChunkPool &emptyChunks(const AutoLockGC &lock) const { return emptyChunks_; }

#ifdef JS_GC_ZEAL
    void startVerifyPreBarriers();
    bool endVerifyPreBarriers();
    void startVerifyPostBarriers();
    bool endVerifyPostBarriers();
    void finishVerifier();
    bool isVerifyPreBarriersEnabled() const { return !!verifyPreData; }
#else
    bool isVerifyPreBarriersEnabled() const { return false; }
#endif

    template <AllowGC allowGC>
    static void *refillFreeListFromAnyThread(ThreadSafeContext *cx, AllocKind thingKind);
    static void *refillFreeListInGC(Zone *zone, AllocKind thingKind);

    
    void freeUnusedLifoBlocksAfterSweeping(LifoAlloc *lifo);
    void freeAllLifoBlocksAfterSweeping(LifoAlloc *lifo);

    
    void releaseArena(ArenaHeader *aheader, const AutoLockGC &lock);

  private:
    
    friend class ArenaLists;
    Chunk *pickChunk(const AutoLockGC &lock,
                     AutoMaybeStartBackgroundAllocation &maybeStartBGAlloc);
    ArenaHeader *allocateArena(Chunk *chunk, Zone *zone, AllocKind kind, const AutoLockGC &lock);
    inline void arenaAllocatedDuringGC(JS::Zone *zone, ArenaHeader *arena);

    template <AllowGC allowGC>
    static void *refillFreeListFromMainThread(JSContext *cx, AllocKind thingKind);
    static void *refillFreeListOffMainThread(ExclusiveContext *cx, AllocKind thingKind);
    static void *refillFreeListPJS(ForkJoinContext *cx, AllocKind thingKind);

    



    Chunk *expireEmptyChunkPool(bool shrinkBuffers, const AutoLockGC &lock);
    void freeEmptyChunks(JSRuntime *rt, const AutoLockGC &lock);
    void freeChunkList(Chunk *chunkListHead);
    void prepareToFreeChunk(ChunkInfo &info);
    void releaseChunk(Chunk *chunk);

    friend class BackgroundAllocTask;
    friend class AutoMaybeStartBackgroundAllocation;
    inline bool wantBackgroundAllocation(const AutoLockGC &lock) const;
    void startBackgroundAllocTaskIfIdle();

    bool initZeal();
    void requestMajorGC(JS::gcreason::Reason reason);
    void collect(bool incremental, SliceBudget &budget, JSGCInvocationKind gckind,
                 JS::gcreason::Reason reason);
    bool gcCycle(bool incremental, SliceBudget &budget, JSGCInvocationKind gckind,
                 JS::gcreason::Reason reason);
    gcstats::ZoneGCStats scanZonesBeforeGC();
    void budgetIncrementalGC(SliceBudget &budget);
    void resetIncrementalGC(const char *reason);
    void incrementalCollectSlice(SliceBudget &budget, JS::gcreason::Reason reason);
    void pushZealSelectedObjects();
    void purgeRuntime();
    bool beginMarkPhase(JS::gcreason::Reason reason);
    bool shouldPreserveJITCode(JSCompartment *comp, int64_t currentTime,
                               JS::gcreason::Reason reason);
    void bufferGrayRoots();
    bool drainMarkStack(SliceBudget &sliceBudget, gcstats::Phase phase);
    template <class CompartmentIterT> void markWeakReferences(gcstats::Phase phase);
    void markWeakReferencesInCurrentGroup(gcstats::Phase phase);
    template <class ZoneIterT, class CompartmentIterT> void markGrayReferences(gcstats::Phase phase);
    void markGrayReferencesInCurrentGroup(gcstats::Phase phase);
    void markAllWeakReferences(gcstats::Phase phase);
    void markAllGrayReferences(gcstats::Phase phase);

    void beginSweepPhase(bool lastGC);
    void findZoneGroups();
    bool findZoneEdgesForWeakMaps();
    void getNextZoneGroup();
    void endMarkingZoneGroup();
    void beginSweepingZoneGroup();
    bool shouldReleaseObservedTypes();
    void endSweepingZoneGroup();
    bool sweepPhase(SliceBudget &sliceBudget);
    void endSweepPhase(bool lastGC);
    void sweepZones(FreeOp *fop, bool lastGC);
    void decommitAllWithoutUnlocking(const AutoLockGC &lock);
    void decommitArenas(const AutoLockGC &lock);
    void expireChunksAndArenas(bool shouldShrink, const AutoLockGC &lock);
    void sweepBackgroundThings();
    void assertBackgroundSweepingFinished();
    bool shouldCompact();
#ifdef JSGC_COMPACTING
    void sweepTypesAfterCompacting(Zone *zone);
    void sweepZoneAfterCompacting(Zone *zone);
    void compactPhase(bool lastGC);
    ArenaHeader *relocateArenas();
    void updateAllCellPointersParallel(ArenasToUpdate &source);
    void updateAllCellPointersSerial(MovingTracer *trc, ArenasToUpdate &source);
    void updatePointersToRelocatedCells();
    void releaseRelocatedArenas(ArenaHeader *relocatedList);
#ifdef DEBUG
    void protectRelocatedArenas(ArenaHeader *relocatedList);
    void unprotectRelocatedArenas(ArenaHeader *relocatedList);
#endif
#endif
    void finishCollection();

    void computeNonIncrementalMarkingForValidation();
    void validateIncrementalMarking();
    void finishMarkingValidation();

    void markConservativeStackRoots(JSTracer *trc, bool useSavedRoots);

#ifdef DEBUG
    void checkForCompartmentMismatches();
#endif

    void callFinalizeCallbacks(FreeOp *fop, JSFinalizeStatus status) const;
    void callWeakPointerCallbacks() const;

  public:
    JSRuntime *rt;

    
    JS::Zone *systemZone;

    
    js::gc::ZoneVector zones;

#ifdef JSGC_GENERATIONAL
    js::Nursery nursery;
    js::gc::StoreBuffer storeBuffer;
#endif

    js::gcstats::Statistics stats;

    js::GCMarker marker;

    
    HeapUsage usage;

    
    GCSchedulingTunables tunables;
    GCSchedulingState schedulingState;

  private:
    




    js::GCChunkSet chunkSet;

    






    ChunkPool availableChunks_;
    ChunkPool emptyChunks_;

    js::RootedValueMap rootsHash;

    size_t maxMallocBytes;

    


    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> numArenasFreeCommitted;
    void *verifyPreData;
    void *verifyPostData;
    bool chunkAllocationSinceLastGC;
    int64_t nextFullGCTime;
    int64_t lastGCTime;

    JSGCMode mode;

    uint64_t decommitThreshold;

    
    bool cleanUpEverything;

    



    bool grayBitsValid;

    volatile uintptr_t majorGCRequested;
    JS::gcreason::Reason majorGCTriggerReason;

#ifdef JSGC_GENERATIONAL
    bool minorGCRequested;
    JS::gcreason::Reason minorGCTriggerReason;
#endif

    
    uint64_t majorGCNumber;

    
    uint64_t jitReleaseNumber;

    
    uint64_t number;

    
    uint64_t startNumber;

    
    bool isIncremental;

    
    bool isFull;

    
    JSGCInvocationKind invocationKind;

    





    mozilla::DebugOnly<uintptr_t> disableStrictProxyCheckingCount;

    



    js::gc::State incrementalState;

    
    bool lastMarkSlice;

    
    bool sweepOnBackgroundThread;

    
    bool releaseObservedTypes;

    
    bool foundBlackGrayEdges;

    
    JS::Zone *sweepingZones;

    



    js::LifoAlloc freeLifoAlloc;

    
    unsigned zoneGroupIndex;

    


    JS::Zone *zoneGroups;
    JS::Zone *currentZoneGroup;
    bool sweepingTypes;
    unsigned finalizePhase;
    JS::Zone *sweepZone;
    unsigned sweepKindIndex;
    bool abortSweepAfterCurrentGroup;

    


    void startTask(GCParallelTask &task, gcstats::Phase phase);
    void joinTask(GCParallelTask &task, gcstats::Phase phase);

    


    js::gc::ArenaHeader *arenasAllocatedDuringSweep;

#ifdef JS_GC_MARKING_VALIDATION
    js::gc::MarkingValidator *markingValidator;
#endif

    




    volatile uintptr_t interFrameGC;

    
    int64_t sliceBudget;

    



    bool incrementalAllowed;

    


    unsigned generationalDisabled;

#ifdef JSGC_COMPACTING
    




    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> compactingDisabled;
#endif

    




    bool manipulatingDeadZones;

    







    unsigned objectsMarkedInDeadZones;

    bool poked;

    volatile js::HeapState heapState;

    







    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> fjCollectionCounter;

    

























#ifdef JS_GC_ZEAL
    int zealMode;
    int zealFrequency;
    int nextScheduled;
    bool deterministicOnly;
    int incrementalLimit;

    js::Vector<JSObject *, 0, js::SystemAllocPolicy> selectedForMarking;
#endif

    bool validate;
    bool fullCompartmentChecks;

    Callback<JSGCCallback> gcCallback;
    CallbackVector<JSFinalizeCallback> finalizeCallbacks;
    CallbackVector<JSWeakPointerCallback> updateWeakPointerCallbacks;

    



    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire> mallocBytes;

    



    mozilla::Atomic<bool, mozilla::ReleaseAcquire> mallocGCTriggered;

    





    CallbackVector<JSTraceDataOp> blackRootTracers;
    Callback<JSTraceDataOp> grayRootTracer;

    
    bool alwaysPreserveCode;

#ifdef DEBUG
    





    int inUnsafeRegion;

    size_t noGCOrAllocationCheck;

#ifdef JSGC_COMPACTING
    ArenaHeader* relocatedArenasToRelease;
#endif

#endif

    
    PRLock *lock;
    mozilla::DebugOnly<PRThread *> lockOwner;

    BackgroundAllocTask allocTask;
    GCHelperState helperState;

    



    SortedArenaList incrementalSweepList;

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
             zealMode <= ZealIncrementalMultipleSlices) ||
            zealMode == ZealCompactValue)
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
