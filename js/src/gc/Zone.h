





#ifndef gc_Zone_h
#define gc_Zone_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/Util.h"

#include "jscntxt.h"
#include "jsgc.h"
#include "jsinfer.h"

#include "gc/FindSCCs.h"

namespace js {






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

    bool                         hold;

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

    const bool *AddressOfNeedsBarrier() const {
        return &needsBarrier_;
    }

  public:
    enum CompartmentGCState {
        NoGC,
        Mark,
        MarkGray,
        Sweep,
        Finished
    };

  private:
    bool                         gcScheduled;
    CompartmentGCState           gcState;
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

    void setGCState(CompartmentGCState state) {
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
        JSRuntime *rt = runtimeFromMainThread();
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

    volatile size_t              gcBytes;
    size_t                       gcTriggerBytes;
    size_t                       gcMaxMallocBytes;
    double                       gcHeapGrowthFactor;

    bool                         isSystem;

    
    bool usedByExclusiveThread;

    



    bool                         scheduledForDestruction;
    bool                         maybeAlive;

    




    ptrdiff_t                    gcMallocBytes;

    
    js::Vector<js::GrayRoot, 0, js::SystemAllocPolicy> gcGrayRoots;

    Zone(JSRuntime *rt);
    ~Zone();
    bool init(JSContext *cx);

    void findOutgoingEdges(js::gc::ComponentFinder<JS::Zone> &finder);

    void discardJitCode(js::FreeOp *fop);

    void addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf, size_t *typePool);

    void setGCLastBytes(size_t lastBytes, js::JSGCInvocationKind gckind);
    void reduceGCTriggerBytes(size_t amount);

    void resetGCMallocBytes();
    void setGCMaxMallocBytes(size_t value);
    void updateMallocCounter(size_t nbytes) {
        



        ptrdiff_t oldCount = gcMallocBytes;
        ptrdiff_t newCount = oldCount - ptrdiff_t(nbytes);
        gcMallocBytes = newCount;
        if (JS_UNLIKELY(newCount <= 0 && oldCount > 0))
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

    void markTypes(JSTracer *trc);

    js::types::TypeZone types;

    void sweep(js::FreeOp *fop, bool releaseTypes);

  private:
    void sweepBreakpoints(js::FreeOp *fop);
};

} 

namespace js {

class ZonesIter {
  private:
    JS::Zone **it, **end;

  public:
    ZonesIter(JSRuntime *rt) {
        it = rt->zones.begin();
        end = rt->zones.end();
    }

    bool done() const { return it == end; }

    void next() {
        JS_ASSERT(!done());
        it++;
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
  private:
    JSCompartment **it, **end;

  public:
    CompartmentsInZoneIter(JS::Zone *zone) {
        it = zone->compartments.begin();
        end = zone->compartments.end();
    }

    bool done() const { return it == end; }
    void next() {
        JS_ASSERT(!done());
        it++;
    }

    JSCompartment *get() const { return *it; }

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
    CompartmentsIterT(JSRuntime *rt)
      : zone(rt)
    {
        JS_ASSERT(!zone.done());
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

} 

#endif 
