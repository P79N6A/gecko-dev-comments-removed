






#include "jsapi.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsprf.h"

#include "js/HashTable.h"
#include "gc/GCInternals.h"

#ifdef JS_ION
#include "ion/IonCompartment.h"
#include "ion/Ion.h"
#endif

#include "jsobjinlines.h"
#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

JS::Zone::Zone(JSRuntime *rt)
  : rt(rt),
    allocator(this),
    hold(false),
#ifdef JSGC_GENERATIONAL
    gcNursery(),
    gcStoreBuffer(&gcNursery),
#endif
    ionUsingBarriers_(false),
    active(false),
    gcScheduled(false),
    gcState(NoGC),
    gcPreserveCode(false),
    gcBytes(0),
    gcTriggerBytes(0),
    gcHeapGrowthFactor(3.0),
    isSystem(false),
    scheduledForDestruction(false),
    maybeAlive(true),
    gcMallocBytes(0),
    gcGrayRoots(),
    types(this)
{
    
    JS_ASSERT(reinterpret_cast<JS::shadow::Zone *>(this) ==
              static_cast<JS::shadow::Zone *>(this));

    setGCMaxMallocBytes(rt->gcMaxMallocBytes * 0.9);
}

Zone::~Zone()
{
    if (this == rt->systemZone)
        rt->systemZone = NULL;
}

bool
Zone::init(JSContext *cx)
{
    types.init(cx);
    return true;
}

void
Zone::setNeedsBarrier(bool needs, ShouldUpdateIon updateIon)
{
#ifdef JS_METHODJIT
    
    bool old = compileBarriers();
    if (compileBarriers(needs) != old)
        mjit::ClearAllFrames(this);
#endif

#ifdef JS_ION
    if (updateIon == UpdateIon && needs != ionUsingBarriers_) {
        ion::ToggleBarriers(this, needs);
        ionUsingBarriers_ = needs;
    }
#endif

    needsBarrier_ = needs;
}

void
Zone::markTypes(JSTracer *trc)
{
    




    JS_ASSERT(isPreservingCode());

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        MarkScriptRoot(trc, &script, "mark_types_script");
        JS_ASSERT(script == i.get<JSScript>());
    }

    for (size_t thingKind = FINALIZE_OBJECT0; thingKind < FINALIZE_OBJECT_LIMIT; thingKind++) {
        ArenaHeader *aheader = allocator.arenas.getFirstArena(static_cast<AllocKind>(thingKind));
        if (aheader)
            rt->gcMarker.pushArenaList(aheader);
    }

    for (CellIterUnderGC i(this, FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        types::TypeObject *type = i.get<types::TypeObject>();
        MarkTypeObjectRoot(trc, &type, "mark_types_scan");
        JS_ASSERT(type == i.get<types::TypeObject>());
    }
}

void
Zone::resetGCMallocBytes()
{
    gcMallocBytes = ptrdiff_t(gcMaxMallocBytes);
}

void
Zone::setGCMaxMallocBytes(size_t value)
{
    



    gcMaxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
    resetGCMallocBytes();
}

void
Zone::onTooMuchMalloc()
{
    TriggerZoneGC(this, gcreason::TOO_MUCH_MALLOC);
}

void
Zone::sweep(FreeOp *fop, bool releaseTypes)
{
    



    if (active)
        releaseTypes = false;

    if (!isPreservingCode()) {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);
        types.sweep(fop, releaseTypes);
    }

    active = false;
}

void
Zone::discardJitCode(FreeOp *fop, bool discardConstraints)
{
#ifdef JS_METHODJIT
    







    mjit::ClearAllFrames(this);

    if (isPreservingCode()) {
        PurgeJITCaches(this);
    } else {
# ifdef JS_ION
        
        ion::InvalidateAll(fop, this);
# endif
        for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();

            mjit::ReleaseScriptCode(fop, script);
# ifdef JS_ION
            ion::FinishInvalidation(fop, script);
# endif

            




            script->resetUseCount();
        }

        for (CompartmentsInZoneIter comp(this); !comp.done(); comp.next())
            comp->types.sweepCompilerOutputs(fop, discardConstraints);
    }
#endif 
}
