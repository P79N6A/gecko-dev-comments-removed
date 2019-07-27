





#ifndef gc_Zone_h
#define gc_Zone_h

#include "mozilla/Atomics.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jsgc.h"
#include "jsinfer.h"

#include "gc/FindSCCs.h"

namespace js {

namespace jit {
class JitZone;
}




class Allocator
{
  public:
    explicit Allocator(JS::Zone *zone);

    js::gc::ArenaLists arenas;

  private:
    
    
    
    friend class gc::ArenaLists;

    JS::Zone *zone_;
};

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

    void updateAfterGC(size_t lastBytes, JSGCInvocationKind gckind,
                       const GCSchedulingTunables &tunables, const GCSchedulingState &state);
    void updateForRemovedArena(const GCSchedulingTunables &tunables);

  private:
    static double computeZoneHeapGrowthFactorForHeapSize(size_t lastBytes,
                                                         const GCSchedulingTunables &tunables,
                                                         const GCSchedulingState &state);
    static size_t computeZoneTriggerBytes(double growthFactor, size_t lastBytes,
                                          JSGCInvocationKind gckind,
                                          const GCSchedulingTunables &tunables);
};

} 
} 

namespace JS {












































struct Zone : public JS::shadow::Zone,
              public js::gc::GraphNodeBase<JS::Zone>,
              public js::MallocProvider<JS::Zone>
{
    explicit Zone(JSRuntime *rt);
    ~Zone();
    bool init(bool isSystem);

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone> &finder);

    void discardJitCode(js::FreeOp *fop);

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t *typePool,
                                size_t *baselineStubsOptimized);

    void resetGCMallocBytes();
    void setGCMaxMallocBytes(size_t value);
    void updateMallocCounter(size_t nbytes) {
        
        
        gcMallocBytes -= ptrdiff_t(nbytes);
        if (MOZ_UNLIKELY(isTooMuchMalloc()))
            onTooMuchMalloc();
    }

    bool isTooMuchMalloc() const { return gcMallocBytes <= 0; }
    void onTooMuchMalloc();

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtimeFromMainThread()->onOutOfMemory(p, nbytes);
    }
    void reportAllocationOverflow() { js_ReportAllocationOverflow(nullptr); }

    void sweep(js::FreeOp *fop, bool releaseTypes, bool *oom);

    bool hasMarkedCompartments();

    void scheduleGC() { JS_ASSERT(!runtimeFromMainThread()->isHeapBusy()); gcScheduled_ = true; }
    void unscheduleGC() { gcScheduled_ = false; }
    bool isGCScheduled() { return gcScheduled_ && canCollect(); }

    void setPreservingCode(bool preserving) { gcPreserveCode_ = preserving; }
    bool isPreservingCode() const { return gcPreserveCode_; }

    bool canCollect();

    enum GCState {
        NoGC,
        Mark,
        MarkGray,
        Sweep,
        Finished
    };
    void setGCState(GCState state) {
        JS_ASSERT(runtimeFromMainThread()->isHeapBusy());
        JS_ASSERT_IF(state != NoGC, canCollect());
        gcState_ = state;
    }

    bool isCollecting() const {
        if (runtimeFromMainThread()->isHeapCollecting())
            return gcState_ != NoGC;
        else
            return needsIncrementalBarrier();
    }

    
    
    bool requireGCTracer() const {
        return runtimeFromMainThread()->isHeapMajorCollecting() && gcState_ != NoGC;
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

    
    
    uint64_t gcNumber();

    bool compileBarriers() const { return compileBarriers(needsIncrementalBarrier()); }
    bool compileBarriers(bool needsIncrementalBarrier) const {
        return needsIncrementalBarrier ||
               runtimeFromMainThread()->gcZeal() == js::gc::ZealVerifierPreValue;
    }

    enum ShouldUpdateJit { DontUpdateJit, UpdateJit };
    void setNeedsIncrementalBarrier(bool needs, ShouldUpdateJit updateJit);
    const bool *addressOfNeedsIncrementalBarrier() const { return &needsIncrementalBarrier_; }

    js::jit::JitZone *getJitZone(JSContext *cx) { return jitZone_ ? jitZone_ : createJitZone(cx); }
    js::jit::JitZone *jitZone() { return jitZone_; }

#ifdef DEBUG
    
    
    unsigned lastZoneGroupIndex() { return gcLastZoneGroupIndex; }
#endif

  private:
    void sweepBreakpoints(js::FreeOp *fop);
    void sweepCompartments(js::FreeOp *fop, bool keepAtleastOne, bool lastGC);

    js::jit::JitZone *createJitZone(JSContext *cx);

  public:
    js::Allocator allocator;

    js::types::TypeZone types;

    
    typedef js::Vector<JSCompartment *, 1, js::SystemAllocPolicy> CompartmentVector;
    CompartmentVector compartments;

    
    typedef js::Vector<js::GrayRoot, 0, js::SystemAllocPolicy> GrayRootVector;
    GrayRootVector gcGrayRoots;

    
    
    
    
    typedef js::HashSet<Zone *, js::DefaultHasher<Zone *>, js::SystemAllocPolicy> ZoneSet;
    ZoneSet gcZoneGroupEdges;

    
    
    
    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire> gcMallocBytes;

    
    size_t gcMaxMallocBytes;

    
    
    
    
    
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> gcMallocGCTriggered;

    
    js::gc::HeapUsage usage;

    
    js::gc::ZoneHeapThreshold threshold;

    
    void *data;

    bool isSystem;

    bool usedByExclusiveThread;

    
    bool active;

    mozilla::DebugOnly<unsigned> gcLastZoneGroupIndex;

  private:
    js::jit::JitZone *jitZone_;

    GCState gcState_;
    bool gcScheduled_;
    bool gcPreserveCode_;
    bool jitUsingBarriers_;

    friend bool js::CurrentThreadCanAccessZone(Zone *zone);
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
    JS::Zone **it, **end;

  public:
    ZonesIter(JSRuntime *rt, ZoneSelector selector) {
        it = rt->gc.zones.begin();
        end = rt->gc.zones.end();

        if (selector == SkipAtoms) {
            MOZ_ASSERT(atAtomsZone(rt));
            it++;
        }
    }

    bool atAtomsZone(JSRuntime *rt);

    bool done() const { return it == end; }

    void next() {
        JS_ASSERT(!done());
        do {
            it++;
        } while (!done() && (*it)->usedByExclusiveThread);
    }

    JS::Zone *get() const {
        JS_ASSERT(!done());
        return *it;
    }

    operator JS::Zone *() const { return get(); }
    JS::Zone *operator->() const { return get(); }
};

struct CompartmentsInZoneIter
{
    explicit CompartmentsInZoneIter(JS::Zone *zone) {
        it = zone->compartments.begin();
        end = zone->compartments.end();
    }

    bool done() const {
        JS_ASSERT(it);
        return it == end;
    }
    void next() {
        JS_ASSERT(!done());
        it++;
    }

    JSCompartment *get() const {
        JS_ASSERT(it);
        return *it;
    }

    operator JSCompartment *() const { return get(); }
    JSCompartment *operator->() const { return get(); }

  private:
    JSCompartment **it, **end;

    CompartmentsInZoneIter()
      : it(nullptr), end(nullptr)
    {}

    
    friend class mozilla::Maybe<CompartmentsInZoneIter>;
};



template<class ZonesIterT>
class CompartmentsIterT
{
    ZonesIterT zone;
    mozilla::Maybe<CompartmentsInZoneIter> comp;

  public:
    explicit CompartmentsIterT(JSRuntime *rt)
      : zone(rt)
    {
        if (zone.done())
            comp.construct();
        else
            comp.construct(zone);
    }

    CompartmentsIterT(JSRuntime *rt, ZoneSelector selector)
      : zone(rt, selector)
    {
        if (zone.done())
            comp.construct();
        else
            comp.construct(zone);
    }

    bool done() const { return zone.done(); }

    void next() {
        JS_ASSERT(!done());
        JS_ASSERT(!comp.ref().done());
        comp.ref().next();
        if (comp.ref().done()) {
            comp.destroy();
            zone.next();
            if (!zone.done())
                comp.construct(zone);
        }
    }

    JSCompartment *get() const {
        JS_ASSERT(!done());
        return comp.ref();
    }

    operator JSCompartment *() const { return get(); }
    JSCompartment *operator->() const { return get(); }
};

typedef CompartmentsIterT<ZonesIter> CompartmentsIter;


Zone *
ZoneOfValue(const JS::Value &value);

} 

#endif 
