





#include "gc/Zone.h"

#include "jsgc.h"

#ifdef JS_ION
#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/JitCompartment.h"
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
    gcMallocGCTriggered(false),
    gcGrayRoots(),
    data(nullptr),
    types(this)
#ifdef JS_ION
    , jitZone_(nullptr)
#endif
{
    
    JS_ASSERT(reinterpret_cast<JS::shadow::Zone *>(this) ==
              static_cast<JS::shadow::Zone *>(this));

    setGCMaxMallocBytes(rt->gcMaxMallocBytes * 0.9);
}

Zone::~Zone()
{
    if (this == runtimeFromMainThread()->systemZone)
        runtimeFromMainThread()->systemZone = nullptr;

#ifdef JS_ION
    js_delete(jitZone_);
#endif
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
Zone::resetGCMallocBytes()
{
    gcMallocBytes = ptrdiff_t(gcMaxMallocBytes);
    gcMallocGCTriggered = false;
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
    if (!gcMallocGCTriggered)
        gcMallocGCTriggered = TriggerZoneGC(this, JS::gcreason::TOO_MUCH_MALLOC);
}

void
Zone::sweep(FreeOp *fop, bool releaseTypes)
{
    



    if (active)
        releaseTypes = false;

    {
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

    JS_ASSERT(isGCSweeping());
    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        JS_ASSERT(script->zone()->isGCSweeping());
        if (!script->hasAnyBreakpointsOrStepMode())
            continue;

        bool scriptGone = IsScriptAboutToBeFinalized(&script);
        JS_ASSERT(script == i.get<JSScript>());
        for (unsigned i = 0; i < script->length(); i++) {
            BreakpointSite *site = script->getBreakpointSite(script->offsetToPC(i));
            if (!site)
                continue;

            Breakpoint *nextbp;
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = nextbp) {
                nextbp = bp->nextInSite();
                HeapPtrObject& dbgobj = bp->debugger->toJSObjectRef();
                JS_ASSERT(dbgobj->zone()->isGCSweeping());
                bool dying = scriptGone || IsObjectAboutToBeFinalized(&dbgobj);
                JS_ASSERT_IF(!dying, bp->getHandler()->isMarked());
                if (dying)
                    bp->destroy(fop);
            }
        }
    }
}

void
Zone::discardJitCode(FreeOp *fop)
{
#ifdef JS_ION
    if (!jitZone())
        return;

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

        jitZone()->optimizedStubSpace()->free();
    }
#endif
}

uint64_t
Zone::gcNumber()
{
    
    
    return usedByExclusiveThread ? 0 : runtimeFromMainThread()->gcNumber;
}

#ifdef JS_ION
js::jit::JitZone *
Zone::createJitZone(JSContext *cx)
{
    MOZ_ASSERT(!jitZone_);

    if (!cx->runtime()->getJitRuntime(cx))
        return nullptr;

    jitZone_ = cx->new_<js::jit::JitZone>();
    return jitZone_;
}
#endif

JS::Zone *
js::ZoneOfObject(const JSObject &obj)
{
    return obj.zone();
}

JS::Zone *
js::ZoneOfObjectFromAnyThread(const JSObject &obj)
{
    return obj.zoneFromAnyThread();
}


