





#ifndef gc_Zone_h
#define gc_Zone_h

#include "mozilla/Atomics.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"

#include "gc/FindSCCs.h"
#include "gc/GCRuntime.h"
#include "js/TracingAPI.h"
#include "vm/TypeInference.h"

namespace js {

namespace jit {
class JitZone;
}

namespace gc {


class ZoneHeapThreshold
{
    
    double gcHeapGrowthFactor_;

    
    size_t gcTriggerBytes_;

  public:
    ZoneHeapThreshold()
      : gcHeapGrowthFactor_(3.0),
        gcTriggerBytes_(0)
    {}

    double gcHeapGrowthFactor() const { return gcHeapGrowthFactor_; }
    size_t gcTriggerBytes() const { return gcTriggerBytes_; }
    double allocTrigger(bool highFrequencyGC) const;

    void updateAfterGC(size_t lastBytes, JSGCInvocationKind gckind,
                       const GCSchedulingTunables& tunables, const GCSchedulingState& state);
    void updateForRemovedArena(const GCSchedulingTunables& tunables);

  private:
    static double computeZoneHeapGrowthFactorForHeapSize(size_t lastBytes,
                                                         const GCSchedulingTunables& tunables,
                                                         const GCSchedulingState& state);
    static size_t computeZoneTriggerBytes(double growthFactor, size_t lastBytes,
                                          JSGCInvocationKind gckind,
                                          const GCSchedulingTunables& tunables);
};

} 
} 

namespace JS {










































struct Zone : public JS::shadow::Zone,
              public js::gc::GraphNodeBase<JS::Zone>,
              public js::MallocProvider<JS::Zone>
{
    explicit Zone(JSRuntime* rt);
    ~Zone();
    bool init(bool isSystem);

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone>& finder);

    void discardJitCode(js::FreeOp* fop);

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t* typePool,
                                size_t* baselineStubsOptimized);

    void resetGCMallocBytes();
    void setGCMaxMallocBytes(size_t value);
    void updateMallocCounter(size_t nbytes) {
        
        
        gcMallocBytes -= ptrdiff_t(nbytes);
        if (MOZ_UNLIKELY(isTooMuchMalloc()))
            onTooMuchMalloc();
    }

    bool isTooMuchMalloc() const { return gcMallocBytes <= 0; }
    void onTooMuchMalloc();

    void* onOutOfMemory(js::AllocFunction allocFunc, size_t nbytes, void* reallocPtr = nullptr) {
        return runtimeFromMainThread()->onOutOfMemory(allocFunc, nbytes, reallocPtr);
    }
    void reportAllocationOverflow() { js::ReportAllocationOverflow(nullptr); }

    void beginSweepTypes(js::FreeOp* fop, bool releaseTypes);

    bool hasMarkedCompartments();

    void scheduleGC() { MOZ_ASSERT(!runtimeFromMainThread()->isHeapBusy()); gcScheduled_ = true; }
    void unscheduleGC() { gcScheduled_ = false; }
    bool isGCScheduled() { return gcScheduled_ && canCollect(); }

    void setPreservingCode(bool preserving) { gcPreserveCode_ = preserving; }
    bool isPreservingCode() const { return gcPreserveCode_; }

    bool canCollect();

    void notifyObservingDebuggers();

    enum GCState {
        NoGC,
        Mark,
        MarkGray,
        Sweep,
        Finished,
        Compact
    };
    void setGCState(GCState state) {
        MOZ_ASSERT(runtimeFromMainThread()->isHeapBusy());
        MOZ_ASSERT_IF(state != NoGC, canCollect());
        gcState_ = state;
        if (state == Finished)
            notifyObservingDebuggers();
    }

    bool isCollecting() const {
        if (runtimeFromMainThread()->isHeapCollecting())
            return gcState_ != NoGC;
        else
            return needsIncrementalBarrier();
    }

    bool isCollectingFromAnyThread() const {
        if (runtimeFromAnyThread()->isHeapCollecting())
            return gcState_ != NoGC;
        else
            return needsIncrementalBarrier();
    }

    
    
    bool requireGCTracer() const {
        JSRuntime* rt = runtimeFromAnyThread();
        return rt->isHeapMajorCollecting() && !rt->isHeapCompacting() && gcState_ != NoGC;
    }

    bool isGCMarking() {
        if (runtimeFromMainThread()->isHeapCollecting())
            return gcState_ == Mark || gcState_ == MarkGray;
        else
            return needsIncrementalBarrier();
    }

    bool wasGCStarted() const { return gcState_ != NoGC; }
    bool isGCMarkingBlack() { return gcState_ == Mark; }
    bool isGCMarkingGray() { return gcState_ == MarkGray; }
    bool isGCSweeping() { return gcState_ == Sweep; }
    bool isGCFinished() { return gcState_ == Finished; }
    bool isGCCompacting() { return gcState_ == Compact; }
    bool isGCSweepingOrCompacting() { return gcState_ == Sweep || gcState_ == Compact; }

    
    
    uint64_t gcNumber();

    bool compileBarriers() const { return compileBarriers(needsIncrementalBarrier()); }
    bool compileBarriers(bool needsIncrementalBarrier) const {
        return needsIncrementalBarrier ||
               runtimeFromMainThread()->gcZeal() == js::gc::ZealVerifierPreValue;
    }

    enum ShouldUpdateJit { DontUpdateJit, UpdateJit };
    void setNeedsIncrementalBarrier(bool needs, ShouldUpdateJit updateJit);
    const bool* addressOfNeedsIncrementalBarrier() const { return &needsIncrementalBarrier_; }

    js::jit::JitZone* getJitZone(JSContext* cx) { return jitZone_ ? jitZone_ : createJitZone(cx); }
    js::jit::JitZone* jitZone() { return jitZone_; }

#ifdef DEBUG
    
    
    unsigned lastZoneGroupIndex() { return gcLastZoneGroupIndex; }
#endif

  private:
    void sweepBreakpoints(js::FreeOp* fop);
    void sweepCompartments(js::FreeOp* fop, bool keepAtleastOne, bool lastGC);

    js::jit::JitZone* createJitZone(JSContext* cx);

    bool isQueuedForBackgroundSweep() {
        return isOnList();
    }

  public:
    js::gc::ArenaLists arenas;

    js::TypeZone types;

    
    typedef js::Vector<JSCompartment*, 1, js::SystemAllocPolicy> CompartmentVector;
    CompartmentVector compartments;

    
    typedef js::Vector<js::gc::Cell*, 0, js::SystemAllocPolicy> GrayRootVector;
    GrayRootVector gcGrayRoots;

    
    
    
    
    ZoneSet gcZoneGroupEdges;

    
    
    
    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire> gcMallocBytes;

    
    size_t gcMaxMallocBytes;

    
    
    
    
    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> gcMallocGCTriggered;

    
    js::gc::HeapUsage usage;

    
    js::gc::ZoneHeapThreshold threshold;

    
    
    size_t gcDelayBytes;

    
    void* data;

    bool isSystem;

    bool usedByExclusiveThread;

    
    bool active;

    mozilla::DebugOnly<unsigned> gcLastZoneGroupIndex;

  private:
    js::jit::JitZone* jitZone_;

    GCState gcState_;
    bool gcScheduled_;
    bool gcPreserveCode_;
    bool jitUsingBarriers_;

    
    friend class js::gc::ZoneList;
    static Zone * const NotOnList;
    Zone* listNext_;
    bool isOnList() const;
    Zone* nextZone() const;

    friend bool js::CurrentThreadCanAccessZone(Zone* zone);
    friend class js::gc::GCRuntime;
};

} 

namespace js {





enum ZoneSelector {
    WithAtoms,
    SkipAtoms
};

class ZonesIter
{
    gc::AutoEnterIteration iterMarker;
    JS::Zone** it;
    JS::Zone** end;

  public:
    ZonesIter(JSRuntime* rt, ZoneSelector selector) : iterMarker(&rt->gc) {
        it = rt->gc.zones.begin();
        end = rt->gc.zones.end();

        if (selector == SkipAtoms) {
            MOZ_ASSERT(atAtomsZone(rt));
            it++;
        }
    }

    bool atAtomsZone(JSRuntime* rt);

    bool done() const { return it == end; }

    void next() {
        MOZ_ASSERT(!done());
        do {
            it++;
        } while (!done() && (*it)->usedByExclusiveThread);
    }

    JS::Zone* get() const {
        MOZ_ASSERT(!done());
        return *it;
    }

    operator JS::Zone*() const { return get(); }
    JS::Zone* operator->() const { return get(); }
};

struct CompartmentsInZoneIter
{
    explicit CompartmentsInZoneIter(JS::Zone* zone) : zone(zone) {
        it = zone->compartments.begin();
    }

    bool done() const {
        MOZ_ASSERT(it);
        return it < zone->compartments.begin() ||
               it >= zone->compartments.end();
    }
    void next() {
        MOZ_ASSERT(!done());
        it++;
    }

    JSCompartment* get() const {
        MOZ_ASSERT(it);
        return *it;
    }

    operator JSCompartment*() const { return get(); }
    JSCompartment* operator->() const { return get(); }

  private:
    JS::Zone* zone;
    JSCompartment** it;

    CompartmentsInZoneIter()
      : zone(nullptr), it(nullptr)
    {}

    
    friend class mozilla::Maybe<CompartmentsInZoneIter>;
};



template<class ZonesIterT>
class CompartmentsIterT
{
    gc::AutoEnterIteration iterMarker;
    ZonesIterT zone;
    mozilla::Maybe<CompartmentsInZoneIter> comp;

  public:
    explicit CompartmentsIterT(JSRuntime* rt)
      : iterMarker(&rt->gc), zone(rt)
    {
        if (zone.done())
            comp.emplace();
        else
            comp.emplace(zone);
    }

    CompartmentsIterT(JSRuntime* rt, ZoneSelector selector)
      : iterMarker(&rt->gc), zone(rt, selector)
    {
        if (zone.done())
            comp.emplace();
        else
            comp.emplace(zone);
    }

    bool done() const { return zone.done(); }

    void next() {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(!comp.ref().done());
        comp->next();
        if (comp->done()) {
            comp.reset();
            zone.next();
            if (!zone.done())
                comp.emplace(zone);
        }
    }

    JSCompartment* get() const {
        MOZ_ASSERT(!done());
        return *comp;
    }

    operator JSCompartment*() const { return get(); }
    JSCompartment* operator->() const { return get(); }
};

typedef CompartmentsIterT<ZonesIter> CompartmentsIter;


Zone*
ZoneOfValue(const JS::Value& value);

} 

#endif 
