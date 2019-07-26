





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
  : JS::shadow::Zone(rt, &rt->gc.marker),
    allocator(this),
    types(this),
    compartments(),
    gcGrayRoots(),
    gcHeapGrowthFactor(3.0),
    gcMallocBytes(0),
    gcMallocGCTriggered(false),
    gcBytes(0),
    gcTriggerBytes(0),
    data(nullptr),
    isSystem(false),
    usedByExclusiveThread(false),
    active(false),
    jitZone_(nullptr),
    gcState_(NoGC),
    gcScheduled_(false),
    gcPreserveCode_(false),
    ionUsingBarriers_(false)
{
    
    JS_ASSERT(reinterpret_cast<JS::shadow::Zone *>(this) ==
              static_cast<JS::shadow::Zone *>(this));

    setGCMaxMallocBytes(rt->gc.maxMallocBytes * 0.9);
}

Zone::~Zone()
{
    JSRuntime *rt = runtimeFromMainThread();
    if (this == rt->gc.systemZone)
        rt->gc.systemZone = nullptr;

#ifdef JS_ION
    js_delete(jitZone_);
#endif
}

bool Zone::init()
{
    return gcZoneGroupEdges.init();
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
Zone::sweep(FreeOp *fop, bool releaseTypes, bool *oom)
{
    



    if (active)
        releaseTypes = false;

    {
        gcstats::AutoPhase ap(fop->runtime()->gc.stats, gcstats::PHASE_DISCARD_ANALYSIS);
        types.sweep(fop, releaseTypes, oom);
    }

    if (!fop->runtime()->debuggerList.isEmpty())
        sweepBreakpoints(fop);

    active = false;
}

void
Zone::sweepBreakpoints(FreeOp *fop)
{
    




    gcstats::AutoPhase ap1(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_TABLES);
    gcstats::AutoPhase ap2(fop->runtime()->gc.stats, gcstats::PHASE_SWEEP_TABLES_BREAKPOINT);

    JS_ASSERT(isGCSweeping());
    for (ZoneCellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
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
                HeapPtrObject &dbgobj = bp->debugger->toJSObjectRef();
                JS_ASSERT_IF(dbgobj->zone()->isCollecting(), dbgobj->zone()->isGCSweeping());
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
#ifdef JS_ION
    if (!jitZone())
        return;

    if (isPreservingCode()) {
        PurgeJITCaches(this);
    } else {

# ifdef DEBUG
        
        for (ZoneCellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            JS_ASSERT_IF(script->hasBaselineScript(), !script->baselineScript()->active());
        }
# endif

        
        jit::MarkActiveBaselineScripts(this);

        
        jit::InvalidateAll(fop, this);

        for (ZoneCellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
            JSScript *script = i.get<JSScript>();
            jit::FinishInvalidation<SequentialExecution>(fop, script);

            
            
            
            if (script->hasParallelIonScript()) {
                if (jit::ShouldPreserveParallelJITCode(runtimeFromMainThread(), script)) {
                    script->parallelIonScript()->purgeCaches();
                    script->baselineScript()->setActive();
                } else {
                    jit::FinishInvalidation<ParallelExecution>(fop, script);
                }
            }

            



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
    
    
    return usedByExclusiveThread ? 0 : runtimeFromMainThread()->gc.number;
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

JS::Zone *
js::ZoneOfValue(const JS::Value &value)
{
    JS_ASSERT(value.isMarkable());
    if (value.isObject())
        return value.toObject().zone();
    return static_cast<js::gc::Cell *>(value.toGCThing())->tenuredZone();
}
