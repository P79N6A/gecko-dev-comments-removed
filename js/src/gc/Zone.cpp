





#include "gc/Zone.h"

#include "jsgc.h"

#ifdef JS_ION
#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/IonCompartment.h"
#endif
#include "vm/Debugger.h"
#include "vm/Runtime.h"

#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

JS::Zone::Zone(JSRuntime *rt)
  : JS::shadow::Zone(rt, &rt->gcMarker),
    allocator(this),
    hold(false),
    ionUsingBarriers_(false),
    active(false),
    gcScheduled(false),
    gcState(NoGC),
    gcPreserveCode(false),
    gcBytes(0),
    gcTriggerBytes(0),
    gcHeapGrowthFactor(3.0),
    isSystem(false),
    usedByExclusiveThread(false),
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
    if (this == runtimeFromMainThread()->systemZone)
        runtimeFromMainThread()->systemZone = nullptr;
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
#ifdef JS_ION
    if (updateIon == UpdateIon && needs != ionUsingBarriers_) {
        jit::ToggleBarriers(this, needs);
        ionUsingBarriers_ = needs;
    }
#endif

    if (needs && runtimeFromMainThread()->isAtomsZone(this))
        JS_ASSERT(!runtimeFromMainThread()->exclusiveThreadsPresent());

    JS_ASSERT_IF(needs, canCollect());
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
            trc->runtime->gcMarker.pushArenaList(aheader);
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
        gcstats::AutoPhase ap(fop->runtime()->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);
        types.sweep(fop, releaseTypes);
    }

    if (!fop->runtime()->debuggerList.isEmpty())
        sweepBreakpoints(fop);

    active = false;
}

void
Zone::sweepBreakpoints(FreeOp *fop)
{
    




    gcstats::AutoPhase ap1(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_TABLES);
    gcstats::AutoPhase ap2(fop->runtime()->gcStats, gcstats::PHASE_SWEEP_TABLES_BREAKPOINT);

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (!script->hasAnyBreakpointsOrStepMode())
            continue;
        bool scriptGone = IsScriptAboutToBeFinalized(&script);
        JS_ASSERT(script == i.get<JSScript>());
        for (unsigned i = 0; i < script->length; i++) {
            BreakpointSite *site = script->getBreakpointSite(script->code + i);
            if (!site)
                continue;
            Breakpoint *nextbp;
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = nextbp) {
                nextbp = bp->nextInSite();
                if (scriptGone || IsObjectAboutToBeFinalized(&bp->debugger->toJSObjectRef()))
                    bp->destroy(fop);
            }
        }
    }
}

void
Zone::discardJitCode(FreeOp *fop)
{
#ifdef JS_ION
    if (isPreservingCode()) {
        PurgeJITCaches(this);
    } else {

# ifdef DEBUG
        
        for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            JS_ASSERT_IF(script->hasBaselineScript(), !script->baselineScript()->active());
        }
# endif

        
        jit::MarkActiveBaselineScripts(this);

        
        jit::InvalidateAll(fop, this);

        for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            jit::FinishInvalidation(fop, script);

            



            jit::FinishDiscardBaselineScript(fop, script);

            




            script->resetUseCount();
        }

        for (CompartmentsInZoneIter comp(this); !comp.done(); comp.next()) {
            
            if (comp->ionCompartment())
                comp->ionCompartment()->optimizedStubSpace()->free();

            comp->types.clearCompilerOutputs(fop);
        }
    }
#endif
}

JS::Zone *
js::ZoneOfObject(const JSObject &obj)
{
    return obj.zone();
}


