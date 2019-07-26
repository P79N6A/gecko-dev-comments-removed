






#include "mozilla/DebugOnly.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsmath.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#include "assembler/wtf/Platform.h"
#include "gc/Marking.h"
#include "js/MemoryMetrics.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/PolyIC.h"
#include "methodjit/MonoIC.h"
#include "methodjit/Retcon.h"
#include "vm/Debugger.h"
#include "yarr/BumpPointerAllocator.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#ifdef JS_ION
#include "ion/IonCompartment.h"
#include "ion/Ion.h"
#endif

#if ENABLE_YARR_JIT
#include "assembler/jit/ExecutableAllocator.h"
#endif

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;

JSCompartment::JSCompartment(JSRuntime *rt)
  : rt(rt),
    principals(NULL),
    global_(NULL),
    enterCompartmentDepth(0),
#ifdef JSGC_GENERATIONAL
    gcStoreBuffer(&gcNursery),
#endif
    ionUsingBarriers_(false),
    gcScheduled(false),
    gcState(NoGC),
    gcPreserveCode(false),
    gcBytes(0),
    gcTriggerBytes(0),
    gcHeapGrowthFactor(3.0),
    hold(false),
    isSystemCompartment(false),
    lastCodeRelease(0),
    analysisLifoAlloc(LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    typeLifoAlloc(LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    data(NULL),
    active(false),
    scheduledForDestruction(false),
    maybeAlive(true),
    lastAnimationTime(0),
    regExps(rt),
    propertyTree(thisForCtor()),
    gcMallocAndFreeBytes(0),
    gcTriggerMallocAndFreeBytes(0),
    gcIncomingGrayPointers(NULL),
    gcLiveArrayBuffers(NULL),
    gcWeakMapList(NULL),
    gcGrayRoots(),
    gcMallocBytes(0),
    debugModeBits(rt->debugMode ? DebugFromC : 0),
    rngState(0),
    watchpointMap(NULL),
    scriptCountsMap(NULL),
    debugScriptMap(NULL),
    debugScopes(NULL)
#ifdef JS_ION
    , ionCompartment_(NULL)
#endif
{
    
    JS_ASSERT(reinterpret_cast<JS::shadow::Compartment *>(this) ==
              static_cast<JS::shadow::Compartment *>(this));

    setGCMaxMallocBytes(rt->gcMaxMallocBytes * 0.9);
}

JSCompartment::~JSCompartment()
{
#ifdef JS_ION
    js_delete(ionCompartment_);
#endif

    js_delete(watchpointMap);
    js_delete(scriptCountsMap);
    js_delete(debugScriptMap);
    js_delete(debugScopes);
}

bool
JSCompartment::init(JSContext *cx)
{
    





    if (cx)
        cx->runtime->dateTimeInfo.updateTimeZoneAdjustment();

    activeAnalysis = activeInference = false;
    types.init(cx);

    if (!crossCompartmentWrappers.init())
        return false;

    if (!regExps.init(cx))
        return false;

    if (cx)
        InitRandom(cx->runtime, &rngState);

#ifdef JSGC_GENERATIONAL
    



    if (rt->gcVerifyPostData) {
        if (!gcNursery.enable())
            return false;

        if (!gcStoreBuffer.enable())
            return false;
    } else {
        gcNursery.disable();
        gcStoreBuffer.disable();
    }
#endif

    return debuggees.init();
}

void
JSCompartment::setNeedsBarrier(bool needs, ShouldUpdateIon updateIon)
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

#ifdef JS_ION
ion::IonRuntime *
JSRuntime::createIonRuntime(JSContext *cx)
{
    ionRuntime_ = cx->new_<ion::IonRuntime>();

    if (!ionRuntime_)
        return NULL;

    if (!ionRuntime_->initialize(cx)) {
        js_delete(ionRuntime_);
        ionRuntime_ = NULL;

        if (cx->runtime->atomsCompartment->ionCompartment_) {
            js_delete(cx->runtime->atomsCompartment->ionCompartment_);
            cx->runtime->atomsCompartment->ionCompartment_ = NULL;
        }

        return NULL;
    }

    return ionRuntime_;
}

bool
JSCompartment::ensureIonCompartmentExists(JSContext *cx)
{
    using namespace js::ion;
    if (ionCompartment_)
        return true;

    IonRuntime *ionRuntime = cx->runtime->getIonRuntime(cx);
    if (!ionRuntime)
        return false;

    
    ionCompartment_ = cx->new_<IonCompartment>(ionRuntime);

    if (!ionCompartment_)
        return false;

    if (!ionCompartment_->initialize(cx)) {
        js_delete(ionCompartment_);
        ionCompartment_ = NULL;
        return false;
    }

    return true;
}
#endif

static bool
WrapForSameCompartment(JSContext *cx, HandleObject obj, Value *vp)
{
    JS_ASSERT(cx->compartment == obj->compartment());
    if (!cx->runtime->sameCompartmentWrapObjectCallback) {
        vp->setObject(*obj);
        return true;
    }

    JSObject *wrapped = cx->runtime->sameCompartmentWrapObjectCallback(cx, obj);
    if (!wrapped)
        return false;
    vp->setObject(*wrapped);
    return true;
}

bool
JSCompartment::putWrapper(const CrossCompartmentKey &wrapped, const js::Value &wrapper)
{
    JS_ASSERT(wrapped.wrapped);
    JS_ASSERT_IF(wrapped.kind == CrossCompartmentKey::StringWrapper, wrapper.isString());
    JS_ASSERT_IF(wrapped.kind != CrossCompartmentKey::StringWrapper, wrapper.isObject());
    
    
    return crossCompartmentWrappers.put(wrapped, wrapper);
}

bool
JSCompartment::wrap(JSContext *cx, Value *vp, JSObject *existing)
{
    JS_ASSERT(cx->compartment == this);
    JS_ASSERT_IF(existing, existing->compartment() == cx->compartment);
    JS_ASSERT_IF(existing, vp->isObject());
    JS_ASSERT_IF(existing, IsDeadProxyObject(existing));

    unsigned flags = 0;

    JS_CHECK_CHROME_RECURSION(cx, return false);

#ifdef DEBUG
    struct AutoDisableProxyCheck {
        JSRuntime *runtime;
        AutoDisableProxyCheck(JSRuntime *rt) : runtime(rt) {
            runtime->gcDisableStrictProxyCheckingCount++;
        }
        ~AutoDisableProxyCheck() { runtime->gcDisableStrictProxyCheckingCount--; }
    } adpc(rt);
#endif

    
    if (!vp->isMarkable())
        return true;

    if (vp->isString()) {
        JSString *str = vp->toString();

        
        if (str->compartment() == this)
            return true;

        
        if (str->isAtom()) {
            JS_ASSERT(str->compartment() == cx->runtime->atomsCompartment);
            return true;
        }
    }

    






    HandleObject global = cx->global();

    
    if (vp->isObject()) {
        RootedObject obj(cx, &vp->toObject());

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);

        
        if (obj->isStopIteration()) {
            RootedValue vvp(cx, *vp);
            bool result = js_FindClassObject(cx, JSProto_StopIteration, &vvp);
            *vp = vvp;
            return result;
        }

        
        obj = UnwrapObject(&vp->toObject(),  true, &flags);

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);

        if (cx->runtime->preWrapObjectCallback) {
            obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);
            if (!obj)
                return false;
        }

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);
        vp->setObject(*obj);

#ifdef DEBUG
        {
            JSObject *outer = GetOuterObject(cx, obj);
            JS_ASSERT(outer && outer == obj);
        }
#endif
    }

    RootedValue key(cx, *vp);

    
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(key)) {
        *vp = p->value;
        if (vp->isObject()) {
            RootedObject obj(cx, &vp->toObject());
            JS_ASSERT(obj->isCrossCompartmentWrapper());
            JS_ASSERT(obj->getParent() == global);
        }
        return true;
    }

    if (vp->isString()) {
        RootedValue orig(cx, *vp);
        Rooted<JSStableString *> str(cx, vp->toString()->ensureStable(cx));
        if (!str)
            return false;
        RootedString wrapped(cx, js_NewStringCopyN(cx, str->chars().get(), str->length()));
        if (!wrapped)
            return false;
        vp->setString(wrapped);
        if (!putWrapper(orig, *vp))
            return false;

        if (str->compartment()->isGCMarking()) {
            





            JSString *tmp = str;
            MarkStringUnbarriered(&rt->gcMarker, &tmp, "wrapped string");
            JS_ASSERT(tmp == str);
        }

        return true;
    }

    RootedObject obj(cx, &vp->toObject());

    JSObject *proto = Proxy::LazyProto;
    if (existing) {
        
        if (!existing->getTaggedProto().isLazy() ||
            existing->getClass() != &ObjectProxyClass ||
            existing->getParent() != global ||
            obj->isCallable())
        {
            existing = NULL;
        }
    }

    




    RootedObject wrapper(cx);
    wrapper = cx->runtime->wrapObjectCallback(cx, existing, obj, proto, global, flags);
    if (!wrapper)
        return false;

    
    
    JS_ASSERT(Wrapper::wrappedObject(wrapper) == &key.get().toObject());

    vp->setObject(*wrapper);

    if (!putWrapper(key, *vp))
        return false;

    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSString **strp)
{
    RootedValue value(cx, StringValue(*strp));
    if (!wrap(cx, value.address()))
        return false;
    *strp = value.get().toString();
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, HeapPtrString *strp)
{
    RootedValue value(cx, StringValue(*strp));
    if (!wrap(cx, value.address()))
        return false;
    *strp = value.get().toString();
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSObject **objp, JSObject *existing)
{
    if (!*objp)
        return true;
    RootedValue value(cx, ObjectValue(**objp));
    if (!wrap(cx, value.address(), existing))
        return false;
    *objp = &value.get().toObject();
    return true;
}

bool
JSCompartment::wrapId(JSContext *cx, jsid *idp)
{
    if (JSID_IS_INT(*idp))
        return true;
    RootedValue value(cx, IdToValue(*idp));
    if (!wrap(cx, value.address()))
        return false;
    RootedId id(cx);
    if (!ValueToId(cx, value.get(), &id))
        return false;

    *idp = id;
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, PropertyOp *propp)
{
    Value v = CastAsObjectJsval(*propp);
    if (!wrap(cx, &v))
        return false;
    *propp = CastAsPropertyOp(v.toObjectOrNull());
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, StrictPropertyOp *propp)
{
    Value v = CastAsObjectJsval(*propp);
    if (!wrap(cx, &v))
        return false;
    *propp = CastAsStrictPropertyOp(v.toObjectOrNull());
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, PropertyDescriptor *desc)
{
    return wrap(cx, &desc->obj) &&
           (!(desc->attrs & JSPROP_GETTER) || wrap(cx, &desc->getter)) &&
           (!(desc->attrs & JSPROP_SETTER) || wrap(cx, &desc->setter)) &&
           wrap(cx, &desc->value);
}

bool
JSCompartment::wrap(JSContext *cx, AutoIdVector &props)
{
    jsid *vector = props.begin();
    int length = props.length();
    for (size_t n = 0; n < size_t(length); ++n) {
        if (!wrapId(cx, &vector[n]))
            return false;
    }
    return true;
}






void
JSCompartment::markCrossCompartmentWrappers(JSTracer *trc)
{
    JS_ASSERT(!isCollecting());

    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        Value v = e.front().value;
        if (e.front().key.kind == CrossCompartmentKey::ObjectWrapper) {
            JSObject *wrapper = &v.toObject();

            



            Value referent = GetProxyPrivate(wrapper);
            MarkValueRoot(trc, &referent, "cross-compartment wrapper");
            JS_ASSERT(referent == GetProxyPrivate(wrapper));

            if (IsFunctionProxy(wrapper)) {
                Value call = GetProxyCall(wrapper);
                MarkValueRoot(trc, &call, "cross-compartment wrapper");
                JS_ASSERT(call == GetProxyCall(wrapper));
            }
        }
    }
}

void
JSCompartment::mark(JSTracer *trc)
{
#ifdef JS_ION
    if (ionCompartment_)
        ionCompartment_->mark(trc, this);
#endif

    



    if (enterCompartmentDepth && global_)
        MarkObjectRoot(trc, global_.unsafeGet(), "on-stack compartment global");
}

void
JSCompartment::markTypes(JSTracer *trc)
{
    




    JS_ASSERT(activeAnalysis || isPreservingCode());

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        MarkScriptRoot(trc, &script, "mark_types_script");
        JS_ASSERT(script == i.get<JSScript>());
    }

    for (size_t thingKind = FINALIZE_OBJECT0; thingKind < FINALIZE_OBJECT_LIMIT; thingKind++) {
        ArenaHeader *aheader = arenas.getFirstArena(static_cast<AllocKind>(thingKind));
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
JSCompartment::discardJitCode(FreeOp *fop, bool discardConstraints)
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

        types.sweepCompilerOutputs(fop, discardConstraints);
    }

#endif 
}

bool
JSCompartment::isDiscardingJitCode(JSTracer *trc)
{
    if (!IS_GC_MARKING_TRACER(trc))
        return false;

    return !gcPreserveCode;
}

void
JSCompartment::sweep(FreeOp *fop, bool releaseTypes)
{
    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_DISCARD_CODE);
        discardJitCode(fop, !activeAnalysis && !gcPreserveCode);
    }

    
    sweepCrossCompartmentWrappers();

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES);

        

        sweepBaseShapeTable();
        sweepInitialShapeTable();
        sweepNewTypeObjectTable(newTypeObjects);
        sweepNewTypeObjectTable(lazyTypeObjects);
        sweepBreakpoints(fop);
        sweepCallsiteClones();

        if (global_ && IsObjectAboutToBeFinalized(global_.unsafeGet()))
            global_ = NULL;

#ifdef JS_ION
        if (ionCompartment_)
            ionCompartment_->sweep(fop);
#endif

        




        regExps.sweep(rt);

        if (debugScopes)
            debugScopes->sweep(rt);

        
        WeakMapBase::sweepCompartment(this);
    }

    if (!activeAnalysis && !gcPreserveCode) {
        JS_ASSERT(!types.constrainedOutputs);
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);

        



        LifoAlloc oldAlloc(typeLifoAlloc.defaultChunkSize());
        oldAlloc.steal(&typeLifoAlloc);

        



        if (active)
            releaseTypes = false;

        




        if (types.inferenceEnabled) {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_DISCARD_TI);

            for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
                RawScript script = i.get<JSScript>();
                if (script->types) {
                    types::TypeScript::Sweep(fop, script);

                    if (releaseTypes) {
                        script->types->destroy();
                        script->types = NULL;
                    }
                }
            }
        }

        {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_SWEEP_TYPES);
            types.sweep(fop);
        }

        {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_CLEAR_SCRIPT_ANALYSIS);
            for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
                script->clearAnalysis();
                script->clearPropertyReadTypes();
            }
        }

        {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_FREE_TI_ARENA);
            rt->freeLifoAlloc.transferFrom(&analysisLifoAlloc);
            rt->freeLifoAlloc.transferFrom(&oldAlloc);
        }
    }

    active = false;
}






void
JSCompartment::sweepCrossCompartmentWrappers()
{
    gcstats::AutoPhase ap1(rt->gcStats, gcstats::PHASE_SWEEP_TABLES);
    gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_SWEEP_TABLES_WRAPPER);

    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey key = e.front().key;
        bool keyDying = IsCellAboutToBeFinalized(&key.wrapped);
        bool valDying = IsValueAboutToBeFinalized(e.front().value.unsafeGet());
        bool dbgDying = key.debugger && IsObjectAboutToBeFinalized(&key.debugger);
        if (keyDying || valDying || dbgDying) {
            JS_ASSERT(key.kind != CrossCompartmentKey::StringWrapper);
            e.removeFront();
        } else if (key.wrapped != e.front().key.wrapped || key.debugger != e.front().key.debugger) {
            e.rekeyFront(key);
        }
    }
}

void
JSCompartment::purge()
{
    dtoaCache.purge();
}

void
JSCompartment::resetGCMallocBytes()
{
    gcMallocBytes = ptrdiff_t(gcMaxMallocBytes);
}

void
JSCompartment::setGCMaxMallocBytes(size_t value)
{
    



    gcMaxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
    resetGCMallocBytes();
}

void
JSCompartment::onTooMuchMalloc()
{
    TriggerCompartmentGC(this, gcreason::TOO_MUCH_MALLOC);
}


bool
JSCompartment::hasScriptsOnStack()
{
    for (AllFramesIter afi(rt); !afi.done(); ++afi) {
#ifdef JS_ION
        
        if (afi.isIon())
            continue;
#endif
        if (afi.interpFrame()->script()->compartment() == this)
            return true;
    }
#ifdef JS_ION
    for (ion::IonActivationIterator iai(rt); iai.more(); ++iai) {
        if (iai.activation()->compartment() == this)
            return true;
    }
#endif
    return false;
}

bool
JSCompartment::setDebugModeFromC(JSContext *cx, bool b, AutoDebugModeGC &dmgc)
{
    bool enabledBefore = debugMode();
    bool enabledAfter = (debugModeBits & ~unsigned(DebugFromC)) || b;

    
    
    
    
    
    
    
    
    
    
    bool onStack = false;
    if (enabledBefore != enabledAfter) {
        onStack = hasScriptsOnStack();
        if (b && onStack) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_IDLE);
            return false;
        }
    }

    debugModeBits = (debugModeBits & ~unsigned(DebugFromC)) | (b ? DebugFromC : 0);
    JS_ASSERT(debugMode() == enabledAfter);
    if (enabledBefore != enabledAfter) {
        updateForDebugMode(cx->runtime->defaultFreeOp(), dmgc);
        if (!enabledAfter)
            DebugScopes::onCompartmentLeaveDebugMode(this);
    }
    return true;
}

void
JSCompartment::updateForDebugMode(FreeOp *fop, AutoDebugModeGC &dmgc)
{
    for (ContextIter acx(rt); !acx.done(); acx.next()) {
        if (acx->compartment == this)
            acx->updateJITEnabled();
    }

#ifdef JS_METHODJIT
    bool enabled = debugMode();

    JS_ASSERT_IF(enabled, !hasScriptsOnStack());

    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        script->debugMode = enabled;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!rt->isHeapBusy())
        dmgc.scheduleGC(this);
#endif
}

bool
JSCompartment::addDebuggee(JSContext *cx, js::GlobalObject *global)
{
    AutoDebugModeGC dmgc(cx->runtime);
    return addDebuggee(cx, global, dmgc);
}

bool
JSCompartment::addDebuggee(JSContext *cx,
                           js::GlobalObject *global,
                           AutoDebugModeGC &dmgc)
{
    bool wasEnabled = debugMode();
    if (!debuggees.put(global)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    debugModeBits |= DebugFromJS;
    if (!wasEnabled) {
        updateForDebugMode(cx->runtime->defaultFreeOp(), dmgc);
    }
    return true;
}

void
JSCompartment::removeDebuggee(FreeOp *fop,
                              js::GlobalObject *global,
                              js::GlobalObjectSet::Enum *debuggeesEnum)
{
    AutoDebugModeGC dmgc(rt);
    return removeDebuggee(fop, global, dmgc, debuggeesEnum);
}

void
JSCompartment::removeDebuggee(FreeOp *fop,
                              js::GlobalObject *global,
                              AutoDebugModeGC &dmgc,
                              js::GlobalObjectSet::Enum *debuggeesEnum)
{
    bool wasEnabled = debugMode();
    JS_ASSERT(debuggees.has(global));
    if (debuggeesEnum)
        debuggeesEnum->removeFront();
    else
        debuggees.remove(global);

    if (debuggees.empty()) {
        debugModeBits &= ~DebugFromJS;
        if (wasEnabled && !debugMode()) {
            DebugScopes::onCompartmentLeaveDebugMode(this);
            updateForDebugMode(fop, dmgc);
        }
    }
}

void
JSCompartment::clearBreakpointsIn(FreeOp *fop, js::Debugger *dbg, JSObject *handler)
{
    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->hasAnyBreakpointsOrStepMode())
            script->clearBreakpointsIn(fop, dbg, handler);
    }
}

void
JSCompartment::clearTraps(FreeOp *fop)
{
    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->hasAnyBreakpointsOrStepMode())
            script->clearTraps(fop);
    }
}

void
JSCompartment::sweepBreakpoints(FreeOp *fop)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES_BREAKPOINT);

    if (rt->debuggerList.isEmpty())
        return;

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
JSCompartment::sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf, size_t *compartmentObject,
                                   TypeInferenceSizes *tiSizes, size_t *shapesCompartmentTables,
                                   size_t *crossCompartmentWrappersArg, size_t *regexpCompartment,
                                   size_t *debuggeesSet)
{
    *compartmentObject = mallocSizeOf(this);
    sizeOfTypeInferenceData(tiSizes, mallocSizeOf);
    *shapesCompartmentTables = baseShapes.sizeOfExcludingThis(mallocSizeOf)
                             + initialShapes.sizeOfExcludingThis(mallocSizeOf)
                             + newTypeObjects.sizeOfExcludingThis(mallocSizeOf)
                             + lazyTypeObjects.sizeOfExcludingThis(mallocSizeOf);
    *crossCompartmentWrappersArg = crossCompartmentWrappers.sizeOfExcludingThis(mallocSizeOf);
    *regexpCompartment = regExps.sizeOfExcludingThis(mallocSizeOf);
    *debuggeesSet = debuggees.sizeOfExcludingThis(mallocSizeOf);
}
