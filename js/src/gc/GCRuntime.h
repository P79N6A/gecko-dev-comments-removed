





#ifndef gc_GCRuntime_h
#define gc_GCRuntime_h

#include "mozilla/Atomics.h"

#include "jsgc.h"

#include "gc/Heap.h"
#include "gc/Nursery.h"
#include "gc/Statistics.h"
#include "gc/StoreBuffer.h"
#include "gc/Tracer.h"


#if defined(DEBUG) && !defined(MOZ_B2G)
#define JS_GC_MARKING_VALIDATION
#endif

namespace js {

class AutoLockGC;

namespace gc {

typedef Vector<JS::Zone*, 4, SystemAllocPolicy> ZoneVector;

struct FinalizePhase;
class MarkingValidator;
struct AutoPrepareForTracing;
class AutoTraceSession;
struct ArenasToUpdate;
struct MovingTracer;

class ChunkPool
{
    Chunk* head_;
    size_t count_;

  public:
    ChunkPool() : head_(nullptr), count_(0) {}

    size_t count() const { return count_; }

    Chunk* head() { MOZ_ASSERT(head_); return head_; }
    Chunk* pop();
    void push(Chunk* chunk);
    Chunk* remove(Chunk* chunk);

#ifdef DEBUG
    bool contains(Chunk* chunk) const;
    bool verify() const;
#endif

    
    
    class Iter {
      public:
        explicit Iter(ChunkPool& pool) : current_(pool.head_) {}
        bool done() const { return !current_; }
        void next();
        Chunk* get() const { return current_; }
        operator Chunk*() const { return get(); }
        Chunk* operator->() const { return get(); }
      private:
        Chunk* current_;
    };
};



class BackgroundAllocTask : public GCParallelTask
{
    
    JSRuntime* runtime;
    ChunkPool& chunkPool_;

    const bool enabled_;

  public:
    BackgroundAllocTask(JSRuntime* rt, ChunkPool& pool);
    bool enabled() const { return enabled_; }

  protected:
    virtual void run() override;
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
                                 const GCSchedulingTunables& tunables) {
        inHighFrequencyGCMode_ =
            tunables.isDynamicHeapGrowthEnabled() && lastGCTime &&
            lastGCTime + tunables.highFrequencyThresholdUsec() > currentTime;
    }
};

template<typename F>
struct Callback {
    F op;
    void* data;

    Callback()
      : op(nullptr), data(nullptr)
    {}
    Callback(F op, void* data)
      : op(op), data(data)
    {}
};

template<typename F>
class CallbackVector : public Vector<Callback<F>, 4, SystemAllocPolicy> {};

template <typename T, typename Iter0, typename Iter1>
class ChainedIter
{
    Iter0 iter0_;
    Iter1 iter1_;

  public:
    ChainedIter(const Iter0& iter0, const Iter1& iter1)
      : iter0_(iter0), iter1_(iter1)
    {}

    bool done() const { return iter0_.done() && iter1_.done(); }
    void next() {
        MOZ_ASSERT(!done());
        if (!iter0_.done()) {
            iter0_.next();
        } else {
            MOZ_ASSERT(!iter1_.done());
            iter1_.next();
        }
    }
    T get() const {
        MOZ_ASSERT(!done());
        if (!iter0_.done())
            return iter0_.get();
        MOZ_ASSERT(!iter1_.done());
        return iter1_.get();
    }

    operator T() const { return get(); }
    T operator->() const { return get(); }
};

typedef js::HashMap<Value*,
                    const char*,
                    js::DefaultHasher<Value*>,
                    js::SystemAllocPolicy> RootedValueMap;

class GCRuntime
{
  public:
    explicit GCRuntime(JSRuntime* rt);
    bool init(uint32_t maxbytes, uint32_t maxNurseryBytes);
    void finishRoots();
    void finish();

    inline int zeal();
    inline bool upcomingZealousGC();
    inline bool needZealousGC();

    bool addRoot(Value* vp, const char* name);
    void removeRoot(Value* vp);
    void setMarkStackLimit(size_t limit);

    void setParameter(JSGCParamKey key, uint32_t value);
    uint32_t getParameter(JSGCParamKey key, const AutoLockGC& lock);

    bool isHeapBusy() { return heapState != js::Idle; }
    bool isHeapMajorCollecting() { return heapState == js::MajorCollecting; }
    bool isHeapMinorCollecting() { return heapState == js::MinorCollecting; }
    bool isHeapCollecting() { return isHeapMajorCollecting() || isHeapMinorCollecting(); }
    bool isHeapCompacting() { return isHeapMajorCollecting() && state() == COMPACT; }

    bool triggerGC(JS::gcreason::Reason reason);
    void maybeAllocTriggerZoneGC(Zone* zone, const AutoLockGC& lock);
    bool triggerZoneGC(Zone* zone, JS::gcreason::Reason reason);
    bool maybeGC(Zone* zone);
    void maybePeriodicFullGC();
    void minorGC(JS::gcreason::Reason reason) {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_MINOR_GC);
        minorGCImpl(reason, nullptr);
    }
    void minorGC(JSContext* cx, JS::gcreason::Reason reason);
    void evictNursery(JS::gcreason::Reason reason = JS::gcreason::EVICT_NURSERY) {
        gcstats::AutoPhase ap(stats, gcstats::PHASE_EVICT_NURSERY);
        minorGCImpl(reason, nullptr);
    }
    bool gcIfRequested(JSContext* cx = nullptr);
    void gc(JSGCInvocationKind gckind, JS::gcreason::Reason reason);
    void startGC(JSGCInvocationKind gckind, JS::gcreason::Reason reason, int64_t millis = 0);
    void gcSlice(JS::gcreason::Reason reason, int64_t millis = 0);
    void finishGC(JS::gcreason::Reason reason);
    void abortGC();
    void startDebugGC(JSGCInvocationKind gckind, SliceBudget& budget);
    void debugGCSlice(SliceBudget& budget);

    void triggerFullGCForAtoms() {
        MOZ_ASSERT(fullGCForAtomsRequested_);
        fullGCForAtomsRequested_ = false;
        triggerGC(JS::gcreason::ALLOC_TRIGGER);
    }

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
    void markRuntime(JSTracer* trc,
                     TraceOrMarkRuntime traceOrMark = TraceRuntime,
                     TraceRootsOrUsedSaved rootsSource = TraceRoots);

    void notifyDidPaint();
    void shrinkBuffers();
    void onOutOfMallocMemory();
    void onOutOfMallocMemory(const AutoLockGC& lock);

#ifdef JS_GC_ZEAL
    const void* addressOfZealMode() { return &zealMode; }
    void getZeal(uint8_t* zeal, uint32_t* frequency, uint32_t* nextScheduled);
    void setZeal(uint8_t zeal, uint32_t frequency);
    bool parseAndSetZeal(const char* str);
    void setNextScheduled(uint32_t count);
    void verifyPreBarriers();
    void verifyPostBarriers();
    void maybeVerifyPreBarriers(bool always);
    void maybeVerifyPostBarriers(bool always);
    bool selectForMarking(JSObject* object);
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

    void requestMinorGC(JS::gcreason::Reason reason);

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
    bool isIncrementalGCInProgress() { return state() != gc::NO_INCREMENTAL; }

    bool isGenerationalGCEnabled() { return generationalDisabled == 0; }
    void disableGenerationalGC();
    void enableGenerationalGC();

    void disableCompactingGC();
    void enableCompactingGC();
    bool isCompactingGCEnabled();

    void setGrayRootsTracer(JSTraceDataOp traceOp, void* data);
    bool addBlackRootsTracer(JSTraceDataOp traceOp, void* data);
    void removeBlackRootsTracer(JSTraceDataOp traceOp, void* data);

    void setMaxMallocBytes(size_t value);
    int32_t getMallocBytes() const { return mallocBytesUntilGC; }
    void resetMallocBytes();
    bool isTooMuchMalloc() const { return mallocBytesUntilGC <= 0; }
    void updateMallocCounter(JS::Zone* zone, size_t nbytes);
    void onTooMuchMalloc();

    void setGCCallback(JSGCCallback callback, void* data);
    bool addFinalizeCallback(JSFinalizeCallback callback, void* data);
    void removeFinalizeCallback(JSFinalizeCallback func);
    bool addWeakPointerCallback(JSWeakPointerCallback callback, void* data);
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

    JS::Zone* getCurrentZoneGroup() { return currentZoneGroup; }
    void setFoundBlackGrayEdges() { foundBlackGrayEdges = true; }

    uint64_t gcNumber() { return number; }
    void incGcNumber() { ++number; }

    uint64_t minorGCCount() { return minorGCNumber; }
    void incMinorGcNumber() { ++minorGCNumber; }

    uint64_t majorGCCount() { return majorGCNumber; }
    void incMajorGcNumber() { ++majorGCNumber; }

    bool isIncrementalGc() { return isIncremental; }
    bool isFullGc() { return isFull; }

    bool shouldCleanUpEverything() { return cleanUpEverything; }

    bool areGrayBitsValid() { return grayBitsValid; }
    void setGrayBitsInvalid() { grayBitsValid = false; }

    bool minorGCRequested() const { return minorGCTriggerReason != JS::gcreason::NO_REASON; }
    bool majorGCRequested() const { return majorGCTriggerReason != JS::gcreason::NO_REASON; }
    bool isGcNeeded() { return minorGCRequested() || majorGCRequested(); }

    bool fullGCForAtomsRequested() { return fullGCForAtomsRequested_; }

    double computeHeapGrowthFactor(size_t lastBytes);
    size_t computeTriggerBytes(double growthFactor, size_t lastBytes);

    JSGCMode gcMode() const { return mode; }
    void setGCMode(JSGCMode m) {
        mode = m;
        marker.setGCMode(mode);
    }

    inline void updateOnFreeArenaAlloc(const ChunkInfo& info);
    inline void updateOnArenaFree(const ChunkInfo& info);

    ChunkPool& fullChunks(const AutoLockGC& lock) { return fullChunks_; }
    ChunkPool& availableChunks(const AutoLockGC& lock) { return availableChunks_; }
    ChunkPool& emptyChunks(const AutoLockGC& lock) { return emptyChunks_; }
    const ChunkPool& fullChunks(const AutoLockGC& lock) const { return fullChunks_; }
    const ChunkPool& availableChunks(const AutoLockGC& lock) const { return availableChunks_; }
    const ChunkPool& emptyChunks(const AutoLockGC& lock) const { return emptyChunks_; }
    typedef ChainedIter<Chunk*, ChunkPool::Iter, ChunkPool::Iter> NonEmptyChunksIter;
    NonEmptyChunksIter allNonEmptyChunks() {
        return NonEmptyChunksIter(ChunkPool::Iter(availableChunks_), ChunkPool::Iter(fullChunks_));
    }

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

    
    void freeUnusedLifoBlocksAfterSweeping(LifoAlloc* lifo);
    void freeAllLifoBlocksAfterSweeping(LifoAlloc* lifo);

    
    void releaseArena(ArenaHeader* aheader, const AutoLockGC& lock);

    void releaseHeldRelocatedArenas();

    
    template <AllowGC allowGC>
    bool checkAllocatorState(JSContext* cx, AllocKind kind);
    template <AllowGC allowGC>
    JSObject* tryNewNurseryObject(JSContext* cx, size_t thingSize, size_t nDynamicSlots,
                                  const Class* clasp);
    template <AllowGC allowGC>
    static JSObject* tryNewTenuredObject(ExclusiveContext* cx, AllocKind kind, size_t thingSize,
                                         size_t nDynamicSlots);
    template <typename T, AllowGC allowGC>
    static T* tryNewTenuredThing(ExclusiveContext* cx, AllocKind kind, size_t thingSize);
    static void* refillFreeListInGC(Zone* zone, AllocKind thingKind);

  private:
    enum IncrementalProgress
    {
        NotFinished = 0,
        Finished
    };

    void minorGCImpl(JS::gcreason::Reason reason, Nursery::ObjectGroupList* pretenureGroups);

    
    friend class ArenaLists;
    Chunk* pickChunk(const AutoLockGC& lock,
                     AutoMaybeStartBackgroundAllocation& maybeStartBGAlloc);
    ArenaHeader* allocateArena(Chunk* chunk, Zone* zone, AllocKind kind, const AutoLockGC& lock);
    void arenaAllocatedDuringGC(JS::Zone* zone, ArenaHeader* arena);

    
    bool gcIfNeededPerAllocation(JSContext* cx);
    template <typename T>
    static void checkIncrementalZoneState(ExclusiveContext* cx, T* t);
    template <AllowGC allowGC>
    static void* refillFreeListFromAnyThread(ExclusiveContext* cx, AllocKind thingKind,
                                             size_t thingSize);
    template <AllowGC allowGC>
    static void* refillFreeListFromMainThread(JSContext* cx, AllocKind thingKind,
                                              size_t thingSize);
    static void* tryRefillFreeListFromMainThread(JSContext* cx, AllocKind thingKind);
    static void* refillFreeListOffMainThread(ExclusiveContext* cx, AllocKind thingKind);

    



    ChunkPool expireEmptyChunkPool(bool shrinkBuffers, const AutoLockGC& lock);
    void freeEmptyChunks(JSRuntime* rt, const AutoLockGC& lock);
    void prepareToFreeChunk(ChunkInfo& info);

    friend class BackgroundAllocTask;
    friend class AutoMaybeStartBackgroundAllocation;
    inline bool wantBackgroundAllocation(const AutoLockGC& lock) const;
    void startBackgroundAllocTaskIfIdle();

    void requestMajorGC(JS::gcreason::Reason reason);
    SliceBudget defaultBudget(JS::gcreason::Reason reason, int64_t millis);
    void collect(bool incremental, SliceBudget budget, JS::gcreason::Reason reason);
    bool gcCycle(bool incremental, SliceBudget& budget, JS::gcreason::Reason reason);
    gcstats::ZoneGCStats scanZonesBeforeGC();
    void budgetIncrementalGC(SliceBudget& budget);
    void resetIncrementalGC(const char* reason);
    void incrementalCollectSlice(SliceBudget& budget, JS::gcreason::Reason reason);
    void pushZealSelectedObjects();
    void purgeRuntime();
    bool beginMarkPhase(JS::gcreason::Reason reason);
    bool shouldPreserveJITCode(JSCompartment* comp, int64_t currentTime,
                               JS::gcreason::Reason reason);
    void bufferGrayRoots();
    IncrementalProgress drainMarkStack(SliceBudget& sliceBudget, gcstats::Phase phase);
    template <class CompartmentIterT> void markWeakReferences(gcstats::Phase phase);
    void markWeakReferencesInCurrentGroup(gcstats::Phase phase);
    template <class ZoneIterT, class CompartmentIterT> void markGrayReferences(gcstats::Phase phase);
    void markBufferedGrayRoots(JS::Zone* zone);
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
    IncrementalProgress sweepPhase(SliceBudget& sliceBudget);
    void endSweepPhase(bool lastGC);
    void sweepZones(FreeOp* fop, bool lastGC);
    void decommitAllWithoutUnlocking(const AutoLockGC& lock);
    void decommitArenas(AutoLockGC& lock);
    void expireChunksAndArenas(bool shouldShrink, AutoLockGC& lock);
    void queueZonesForBackgroundSweep(ZoneList& zones);
    void sweepBackgroundThings(ZoneList& zones, LifoAlloc& freeBlocks, ThreadType threadType);
    void assertBackgroundSweepingFinished();
    bool shouldCompact();
    IncrementalProgress beginCompactPhase();
    IncrementalProgress compactPhase(JS::gcreason::Reason reason, SliceBudget& sliceBudget);
    void endCompactPhase(JS::gcreason::Reason reason);
    void sweepTypesAfterCompacting(Zone* zone);
    void sweepZoneAfterCompacting(Zone* zone);
    bool relocateArenas(Zone* zone, JS::gcreason::Reason reason, SliceBudget& sliceBudget);
    void updateAllCellPointersParallel(MovingTracer* trc, Zone* zone);
    void updateAllCellPointersSerial(MovingTracer* trc, Zone* zone);
    void updatePointersToRelocatedCells(Zone* zone);
    void releaseRelocatedArenas();
    void releaseRelocatedArenasWithoutUnlocking(const AutoLockGC& lock);
#ifdef DEBUG
    void protectRelocatedArenas();
    void unprotectRelocatedArenas();
#endif
    void finishCollection(JS::gcreason::Reason reason);

    void computeNonIncrementalMarkingForValidation();
    void validateIncrementalMarking();
    void finishMarkingValidation();

#ifdef DEBUG
    void checkForCompartmentMismatches();
#endif

    void callFinalizeCallbacks(FreeOp* fop, JSFinalizeStatus status) const;
    void callWeakPointerCallbacks() const;

  public:
    JSRuntime* rt;

    
    JS::Zone* systemZone;

    
    js::gc::ZoneVector zones;

    js::Nursery nursery;
    js::gc::StoreBuffer storeBuffer;

    js::gcstats::Statistics stats;

    js::GCMarker marker;

    
    HeapUsage usage;

    
    GCSchedulingTunables tunables;
    GCSchedulingState schedulingState;

  private:
    
    
    
    
    ChunkPool             emptyChunks_;

    
    
    
    
    
    
    ChunkPool             availableChunks_;

    
    
    ChunkPool             fullChunks_;

    RootedValueMap rootsHash;

    size_t maxMallocBytes;

    


    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> numArenasFreeCommitted;
    void* verifyPreData;
    void* verifyPostData;
    bool chunkAllocationSinceLastGC;
    int64_t nextFullGCTime;
    int64_t lastGCTime;

    JSGCMode mode;

    mozilla::Atomic<size_t, mozilla::ReleaseAcquire> numActiveZoneIters;

    uint64_t decommitThreshold;

    
    bool cleanUpEverything;

    
    
    
    
    
    
    enum class GrayBufferState {
        Unused,
        Okay,
        Failed
    };
    GrayBufferState grayBufferState;
    bool hasBufferedGrayRoots() const { return grayBufferState == GrayBufferState::Okay; }

    
    void resetBufferedGrayRoots() const;

    
    void clearBufferedGrayRoots() {
        grayBufferState = GrayBufferState::Unused;
        resetBufferedGrayRoots();
    }

    



    bool grayBitsValid;

    mozilla::Atomic<JS::gcreason::Reason, mozilla::Relaxed> majorGCTriggerReason;

    JS::gcreason::Reason minorGCTriggerReason;

    
    bool fullGCForAtomsRequested_;

    
    uint64_t minorGCNumber;

    
    uint64_t majorGCNumber;

    
    uint64_t jitReleaseNumber;

    
    uint64_t number;

    
    uint64_t startNumber;

    
    bool isIncremental;

    
    bool isFull;

    
    bool isCompacting;

    
    JSGCInvocationKind invocationKind;

    





    mozilla::DebugOnly<uintptr_t> disableStrictProxyCheckingCount;

    



    js::gc::State incrementalState;

    
    bool lastMarkSlice;

    
    bool sweepOnBackgroundThread;

    
    bool releaseObservedTypes;

    
    bool foundBlackGrayEdges;

    
    ZoneList backgroundSweepZones;

    



    js::LifoAlloc freeLifoAlloc;

    
    unsigned zoneGroupIndex;

    


    JS::Zone* zoneGroups;
    JS::Zone* currentZoneGroup;
    bool sweepingTypes;
    unsigned finalizePhase;
    JS::Zone* sweepZone;
    unsigned sweepKindIndex;
    bool abortSweepAfterCurrentGroup;

    


    void startTask(GCParallelTask& task, gcstats::Phase phase);
    void joinTask(GCParallelTask& task, gcstats::Phase phase);

    


    js::gc::ArenaHeader* arenasAllocatedDuringSweep;

    


    bool startedCompacting;
    js::gc::ZoneList zonesToMaybeCompact;
    ArenaHeader* relocatedArenasToRelease;

#ifdef JS_GC_MARKING_VALIDATION
    js::gc::MarkingValidator* markingValidator;
#endif

    




    bool interFrameGC;

    
    int64_t sliceBudget;

    



    bool incrementalAllowed;

    


    unsigned generationalDisabled;

    


    bool compactingEnabled;

    



    unsigned compactingDisabledCount;

    




    bool manipulatingDeadZones;

    







    unsigned objectsMarkedInDeadZones;

    bool poked;

    volatile js::HeapState heapState;

    

























#ifdef JS_GC_ZEAL
    int zealMode;
    int zealFrequency;
    int nextScheduled;
    bool deterministicOnly;
    int incrementalLimit;

    js::Vector<JSObject*, 0, js::SystemAllocPolicy> selectedForMarking;
#endif

    bool validate;
    bool fullCompartmentChecks;

    Callback<JSGCCallback> gcCallback;
    CallbackVector<JSFinalizeCallback> finalizeCallbacks;
    CallbackVector<JSWeakPointerCallback> updateWeakPointerCallbacks;

    



    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire> mallocBytesUntilGC;

    



    mozilla::Atomic<bool, mozilla::ReleaseAcquire> mallocGCTriggered;

    





    CallbackVector<JSTraceDataOp> blackRootTracers;
    Callback<JSTraceDataOp> grayRootTracer;

    
    bool alwaysPreserveCode;

#ifdef DEBUG
    





    int inUnsafeRegion;

    size_t noGCOrAllocationCheck;
#endif

    
    PRLock* lock;
    mozilla::DebugOnly<PRThread*> lockOwner;

    BackgroundAllocTask allocTask;
    GCHelperState helperState;

    



    SortedArenaList incrementalSweepList;

    friend class js::GCHelperState;
    friend class js::gc::MarkingValidator;
    friend class js::gc::AutoTraceSession;
    friend class AutoEnterIteration;
};


class AutoEnterIteration {
    GCRuntime* gc;

  public:
    explicit AutoEnterIteration(GCRuntime* gc_) : gc(gc_) {
        ++gc->numActiveZoneIters;
    }

    ~AutoEnterIteration() {
        MOZ_ASSERT(gc->numActiveZoneIters);
        --gc->numActiveZoneIters;
    }
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
