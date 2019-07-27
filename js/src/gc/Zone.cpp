





#include "gc/Zone.h"

#include "jsgc.h"

#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "jit/JitCompartment.h"
#include "vm/Debugger.h"
#include "vm/Runtime.h"

#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

JS::Zone::Zone(JSRuntime *rt)
  : JS::shadow::Zone(rt, &rt->gc.marker),
    allocator(this),
    types(this),
    compartments(),
    gcGrayRoots(),
    gcMallocBytes(0),
    gcMallocGCTriggered(false),
    usage(&rt->gc.usage),
    data(nullptr),
    isSystem(false),
    usedByExclusiveThread(false),
    active(false),
    jitZone_(nullptr),
    gcState_(NoGC),
    gcScheduled_(false),
    gcPreserveCode_(false),
    jitUsingBarriers_(false)
{
    
    JS_ASSERT(reinterpret_cast<JS::shadow::Zone *>(this) ==
              static_cast<JS::shadow::Zone *>(this));

    threshold.updateAfterGC(8192, GC_NORMAL, rt->gc.tunables, rt->gc.schedulingState);
    setGCMaxMallocBytes(rt->gc.maxMallocBytesAllocated() * 0.9);
}

Zone::~Zone()
{
    JSRuntime *rt = runtimeFromMainThread();
    if (this == rt->gc.systemZone)
        rt->gc.systemZone = nullptr;

    js_delete(jitZone_);
}

bool Zone::init(bool isSystemArg)
{
    isSystem = isSystemArg;
    return gcZoneGroupEdges.init();
}

void
Zone::setNeedsIncrementalBarrier(bool needs, ShouldUpdateJit updateJit)
{
    if (updateJit == UpdateJit && needs != jitUsingBarriers_) {
        jit::ToggleBarriers(this, needs);
        jitUsingBarriers_ = needs;
    }

    if (needs && runtimeFromMainThread()->isAtomsZone(this))
        JS_ASSERT(!runtimeFromMainThread()->exclusiveThreadsPresent());

    JS_ASSERT_IF(needs, canCollect());
    needsIncrementalBarrier_ = needs;
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
    if (!gcMallocGCTriggered) {
        GCRuntime &gc = runtimeFromAnyThread()->gc;
        gcMallocGCTriggered = gc.triggerZoneGC(this, JS::gcreason::TOO_MUCH_MALLOC);
    }
}

void
Zone::sweep(FreeOp *fop, bool releaseTypes, bool *oom)
{
    



    if (active)
        releaseTypes = false;

    GCRuntime &gc = fop->runtime()->gc;

    {
        gcstats::MaybeAutoPhase ap(gc.stats, !gc.isHeapCompacting(),
                                   gcstats::PHASE_DISCARD_ANALYSIS);
        types.sweep(fop, releaseTypes, oom);
    }

    if (!fop->runtime()->debuggerList.isEmpty()) {
        gcstats::MaybeAutoPhase ap1(gc.stats, !gc.isHeapCompacting(),
                                    gcstats::PHASE_SWEEP_TABLES);
        gcstats::MaybeAutoPhase ap2(gc.stats, !gc.isHeapCompacting(),
                                    gcstats::PHASE_SWEEP_TABLES_BREAKPOINT);
        sweepBreakpoints(fop);
    }
}

void
Zone::sweepBreakpoints(FreeOp *fop)
{
    




    JS_ASSERT(isGCSweepingOrCompacting());
    for (ZoneCellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        JS_ASSERT_IF(isGCSweeping(), script->zone()->isGCSweeping());
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
                HeapPtrObject &dbgobj = bp->debugger->toJSObjectRef();
                JS_ASSERT_IF(isGCSweeping() && dbgobj->zone()->isCollecting(),
                             dbgobj->zone()->isGCSweeping());
                bool dying = scriptGone || IsObjectAboutToBeFinalized(&dbgobj);
                JS_ASSERT_IF(!dying, !IsAboutToBeFinalized(&bp->getHandlerRef()));
                if (dying)
                    bp->destroy(fop);
            }
        }
    }
}

void
Zone::discardJitCode(FreeOp *fop)
{
    if (!jitZone())
        return;

    if (isPreservingCode()) {
        PurgeJITCaches(this);
    } else {

#ifdef DEBUG
        
        for (ZoneCellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            JS_ASSERT_IF(script->hasBaselineScript(), !script->baselineScript()->active());
        }
#endif

        
        jit::MarkActiveBaselineScripts(this);

        
        jit::InvalidateAll(fop, this);

        for (ZoneCellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            jit::FinishInvalidation<SequentialExecution>(fop, script);
            jit::FinishInvalidation<ParallelExecution>(fop, script);

            



            jit::FinishDiscardBaselineScript(fop, script);

            




            script->resetWarmUpCounter();
        }

        jitZone()->optimizedStubSpace()->free();
    }
}

uint64_t
Zone::gcNumber()
{
    
    
    return usedByExclusiveThread ? 0 : runtimeFromMainThread()->gc.gcNumber();
}

js::jit::JitZone *
Zone::createJitZone(JSContext *cx)
{
    MOZ_ASSERT(!jitZone_);

    if (!cx->runtime()->getJitRuntime(cx))
        return nullptr;

    jitZone_ = cx->new_<js::jit::JitZone>();
    return jitZone_;
}

JS::Zone *
js::ZoneOfObjectFromAnyThread(const JSObject &obj)
{
    return obj.zoneFromAnyThread();
}

bool
Zone::hasMarkedCompartments()
{
    for (CompartmentsInZoneIter comp(this); !comp.done(); comp.next()) {
        if (comp->marked)
            return true;
    }
    return false;
}

bool
Zone::canCollect()
{
    
    if (usedByExclusiveThread)
        return false;
    JSRuntime *rt = runtimeFromAnyThread();
    if (rt->isAtomsZone(this) && rt->exclusiveThreadsPresent())
        return false;
    return true;
}

JS::Zone *
js::ZoneOfValue(const JS::Value &value)
{
    JS_ASSERT(value.isMarkable());
    if (value.isObject())
        return value.toObject().zone();
    return js::gc::TenuredCell::fromPointer(value.toGCThing())->zone();
}

bool
js::ZonesIter::atAtomsZone(JSRuntime *rt)
{
    return rt->isAtomsZone(*it);
}
