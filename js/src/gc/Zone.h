





#ifndef gc_Zone_h
#define gc_Zone_h

#include "mozilla/Atomics.h"
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
    




    friend class gc::ArenaLists;

    JS::Zone *zone_;

  public:
    explicit Allocator(JS::Zone *zone);

    js::gc::ArenaLists arenas;
};

typedef Vector<JSCompartment *, 1, SystemAllocPolicy> CompartmentVector;

} 

namespace JS {















































struct Zone : public JS::shadow::Zone,
              public js::gc::GraphNodeBase<JS::Zone>,
              public js::MallocProvider<JS::Zone>
{
  private:
    friend bool js::CurrentThreadCanAccessZone(Zone *zone);

  public:
    js::Allocator                allocator;

    js::CompartmentVector        compartments;

  private:
    bool                         ionUsingBarriers_;

  public:
    bool                         active;  

    bool compileBarriers(bool needsBarrier) const {
        return needsBarrier || runtimeFromMainThread()->gcZeal() == js::gc::ZealVerifierPreValue;
    }

    bool compileBarriers() const {
        return compileBarriers(needsBarrier());
    }

    enum ShouldUpdateIon {
        DontUpdateIon,
        UpdateIon
    };

    void setNeedsBarrier(bool needs, ShouldUpdateIon updateIon);

    const bool *addressOfNeedsBarrier() const {
        return &needsBarrier_;
    }

  public:
    enum GCState {
        NoGC,
        Mark,
        MarkGray,
        Sweep,
        Finished
    };

  private:
    bool                         gcScheduled;
    GCState                      gcState;
    bool                         gcPreserveCode;

  public:
    bool isCollecting() const {
        if (runtimeFromMainThread()->isHeapCollecting())
            return gcState != NoGC;
        else
            return needsBarrier();
    }

    bool isPreservingCode() const {
        return gcPreserveCode;
    }

    



    bool requireGCTracer() const {
        return runtimeFromMainThread()->isHeapMajorCollecting() && gcState != NoGC;
    }

    void setGCState(GCState state) {
        JS_ASSERT(runtimeFromMainThread()->isHeapBusy());
        JS_ASSERT_IF(state != NoGC, canCollect());
        gcState = state;
    }

    void scheduleGC() {
        JS_ASSERT(!runtimeFromMainThread()->isHeapBusy());
        gcScheduled = true;
    }

    void unscheduleGC() {
        gcScheduled = false;
    }

    bool isGCScheduled() {
        return gcScheduled && canCollect();
    }

    void setPreservingCode(bool preserving) {
        gcPreserveCode = preserving;
    }

    bool canCollect() {
        
        if (usedByExclusiveThread)
            return false;
        JSRuntime *rt = runtimeFromAnyThread();
        if (rt->isAtomsZone(this) && rt->exclusiveThreadsPresent())
            return false;
        return true;
    }

    bool wasGCStarted() const {
        return gcState != NoGC;
    }

    bool isGCMarking() {
        if (runtimeFromMainThread()->isHeapCollecting())
            return gcState == Mark || gcState == MarkGray;
        else
            return needsBarrier();
    }

    bool isGCMarkingBlack() {
        return gcState == Mark;
    }

    bool isGCMarkingGray() {
        return gcState == MarkGray;
    }

    bool isGCSweeping() {
        return gcState == Sweep;
    }

    bool isGCFinished() {
        return gcState == Finished;
    }

    
    mozilla::Atomic<size_t, mozilla::ReleaseAcquire> gcBytes;

    size_t                       gcTriggerBytes;
    size_t                       gcMaxMallocBytes;
    double                       gcHeapGrowthFactor;

    bool                         isSystem;

    
    bool usedByExclusiveThread;

    



    uint64_t gcNumber();

    



    bool                         scheduledForDestruction;
    bool                         maybeAlive;

    




    mozilla::Atomic<ptrdiff_t, mozilla::ReleaseAcquire> gcMallocBytes;

    






    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> gcMallocGCTriggered;

    
    js::Vector<js::GrayRoot, 0, js::SystemAllocPolicy> gcGrayRoots;

    
    void *data;

    Zone(JSRuntime *rt);
    ~Zone();

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone> &finder);

    void discardJitCode(js::FreeOp *fop);

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                size_t *typePool,
                                size_t *baselineStubsOptimized);

    void setGCLastBytes(size_t lastBytes, js::JSGCInvocationKind gckind);
    void reduceGCTriggerBytes(size_t amount);

    void resetGCMallocBytes();
    void setGCMaxMallocBytes(size_t value);
    void updateMallocCounter(size_t nbytes) {
        



        gcMallocBytes -= ptrdiff_t(nbytes);
        if (MOZ_UNLIKELY(isTooMuchMalloc()))
            onTooMuchMalloc();
    }

    bool isTooMuchMalloc() const {
        return gcMallocBytes <= 0;
     }

    void onTooMuchMalloc();

    void *onOutOfMemory(void *p, size_t nbytes) {
        return runtimeFromMainThread()->onOutOfMemory(p, nbytes);
    }
    void reportAllocationOverflow() {
        js_ReportAllocationOverflow(nullptr);
    }

    js::types::TypeZone types;

    void sweep(js::FreeOp *fop, bool releaseTypes, bool *oom);

    bool hasMarkedCompartments();

  private:
    void sweepBreakpoints(js::FreeOp *fop);

#ifdef JS_ION
    js::jit::JitZone *jitZone_;
    js::jit::JitZone *createJitZone(JSContext *cx);

  public:
    js::jit::JitZone *getJitZone(JSContext *cx) {
        return jitZone_ ? jitZone_ : createJitZone(cx);
    }
    js::jit::JitZone *jitZone() {
        return jitZone_;
    }
#endif
};

} 

namespace js {







enum ZoneSelector {
    WithAtoms,
    SkipAtoms
};

class ZonesIter {
  private:
    JS::Zone **it, **end;

  public:
    ZonesIter(JSRuntime *rt, ZoneSelector selector) {
        it = rt->gc.zones.begin();
        end = rt->gc.zones.end();

        if (selector == SkipAtoms) {
            JS_ASSERT(rt->isAtomsZone(*it));
            it++;
        }
    }

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
    
    friend class mozilla::Maybe<CompartmentsInZoneIter>;
  private:
    JSCompartment **it, **end;

    CompartmentsInZoneIter()
      : it(nullptr), end(nullptr)
    {}

  public:
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
};





template<class ZonesIterT>
class CompartmentsIterT
{
  private:
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
