






#include "mozilla/DebugOnly.h"

#include "jscntxt.h"
#include "jsdate.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsmath.h"
#include "jsproxy.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#if ENABLE_YARR_JIT
#include "assembler/jit/ExecutableAllocator.h"
#endif
#include "assembler/wtf/Platform.h"
#include "gc/Marking.h"
#ifdef JS_ION
#include "ion/IonCompartment.h"
#include "ion/Ion.h"
#endif
#include "js/MemoryMetrics.h"
#include "js/RootingAPI.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/PolyIC.h"
#include "methodjit/MonoIC.h"
#include "methodjit/Retcon.h"
#include "vm/Debugger.h"
#include "vm/ForkJoin.h"
#include "yarr/BumpPointerAllocator.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;

JSCompartment::JSCompartment(Zone *zone)
  : zone_(zone),
    rt(zone->rt),
    principals(NULL),
    isSystem(false),
    marked(true),
    global_(NULL),
    enterCompartmentDepth(0),
    lastCodeRelease(0),
    analysisLifoAlloc(ANALYSIS_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    data(NULL),
    lastAnimationTime(0),
    regExps(rt),
    propertyTree(thisForCtor()),
    gcIncomingGrayPointers(NULL),
    gcLiveArrayBuffers(NULL),
    gcWeakMapList(NULL),
    debugModeBits(rt->debugMode ? DebugFromC : 0),
    rngState(0),
    watchpointMap(NULL),
    scriptCountsMap(NULL),
    debugScriptMap(NULL),
    debugScopes(NULL),
    enumerators(NULL),
    compartmentStats(NULL)
#ifdef JS_ION
    , ionCompartment_(NULL)
#endif
{
    rt->numCompartments++;
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
    js_free(enumerators);

    rt->numCompartments--;
}

bool
JSCompartment::init(JSContext *cx)
{
    





    if (cx)
        cx->runtime->dateTimeInfo.updateTimeZoneAdjustment();

    activeAnalysis = false;

    if (!crossCompartmentWrappers.init(0))
        return false;

    if (!regExps.init(cx))
        return false;

    if (cx)
        InitRandom(cx->runtime, &rngState);

    enumerators = NativeIterator::allocateSentinel(cx);
    if (!enumerators)
        return false;

    return debuggees.init(0);
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
WrapForSameCompartment(JSContext *cx, HandleObject obj, MutableHandleValue vp)
{
    JS_ASSERT(cx->compartment == obj->compartment());
    if (!cx->runtime->sameCompartmentWrapObjectCallback) {
        vp.setObject(*obj);
        return true;
    }

    JSObject *wrapped = cx->runtime->sameCompartmentWrapObjectCallback(cx, obj);
    if (!wrapped)
        return false;
    vp.setObject(*wrapped);
    return true;
}

bool
JSCompartment::putWrapper(const CrossCompartmentKey &wrapped, const js::Value &wrapper)
{
    JS_ASSERT(wrapped.wrapped);
    JS_ASSERT(!IsPoisonedPtr(wrapped.wrapped));
    JS_ASSERT(!IsPoisonedPtr(wrapped.debugger));
    JS_ASSERT(!IsPoisonedPtr(wrapper.toGCThing()));
    JS_ASSERT_IF(wrapped.kind == CrossCompartmentKey::StringWrapper, wrapper.isString());
    JS_ASSERT_IF(wrapped.kind != CrossCompartmentKey::StringWrapper, wrapper.isObject());
    return crossCompartmentWrappers.put(wrapped, wrapper);
}

bool
JSCompartment::wrap(JSContext *cx, MutableHandleValue vp, HandleObject existingArg)
{
    JS_ASSERT(cx->compartment == this);
    JS_ASSERT_IF(existingArg, existingArg->compartment() == cx->compartment);
    JS_ASSERT_IF(existingArg, vp.isObject());
    JS_ASSERT_IF(existingArg, IsDeadProxyObject(existingArg));

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

    
    if (!vp.isMarkable())
        return true;

    if (vp.isString()) {
        RawString str = vp.toString();

        
        if (str->zone() == zone())
            return true;

        
        if (str->isAtom()) {
            JS_ASSERT(str->zone() == cx->runtime->atomsCompartment->zone());
            return true;
        }
    }

    






    HandleObject global = cx->global();

    
    if (vp.isObject()) {
        RootedObject obj(cx, &vp.toObject());

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);

        
        if (obj->isStopIteration())
            return js_FindClassObject(cx, JSProto_StopIteration, vp);

        
        obj = UnwrapObject(obj,  true, &flags);

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);

        if (cx->runtime->preWrapObjectCallback) {
            obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);
            if (!obj)
                return false;
        }

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);
        vp.setObject(*obj);

#ifdef DEBUG
        {
            JSObject *outer = GetOuterObject(cx, obj);
            JS_ASSERT(outer && outer == obj);
        }
#endif
    }

    RootedValue key(cx, vp);

    
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(key)) {
        vp.set(p->value);
        if (vp.isObject()) {
            DebugOnly<RawObject> obj = &vp.toObject();
            JS_ASSERT(obj->isCrossCompartmentWrapper());
            JS_ASSERT(obj->getParent() == global);
        }
        return true;
    }

    if (vp.isString()) {
        Rooted<JSLinearString *> str(cx, vp.toString()->ensureLinear(cx));
        if (!str)
            return false;

        RawString wrapped = js_NewStringCopyN<CanGC>(cx, str->chars(), str->length());
        if (!wrapped)
            return false;

        vp.setString(wrapped);
        if (!putWrapper(key, vp))
            return false;

        if (str->zone()->isGCMarking()) {
            





            JSString *tmp = str;
            MarkStringUnbarriered(&rt->gcMarker, &tmp, "wrapped string");
            JS_ASSERT(tmp == str);
        }

        return true;
    }

    RootedObject proto(cx, Proxy::LazyProto);
    RootedObject obj(cx, &vp.toObject());
    RootedObject existing(cx, existingArg);
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

    vp.setObject(*wrapper);
    return putWrapper(key, vp);
}

bool
JSCompartment::wrap(JSContext *cx, JSString **strp)
{
    RootedValue value(cx, StringValue(*strp));
    if (!wrap(cx, &value))
        return false;
    *strp = value.get().toString();
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, HeapPtrString *strp)
{
    RootedValue value(cx, StringValue(*strp));
    if (!wrap(cx, &value))
        return false;
    *strp = value.get().toString();
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSObject **objp, JSObject *existingArg)
{
    if (!*objp)
        return true;
    RootedValue value(cx, ObjectValue(**objp));
    RootedObject existing(cx, existingArg);
    if (!wrap(cx, &value, existing))
        return false;
    *objp = &value.get().toObject();
    return true;
}

bool
JSCompartment::wrapId(JSContext *cx, jsid *idp)
{
    MOZ_ASSERT(*idp != JSID_VOID, "JSID_VOID is an out-of-band sentinel value");
    if (JSID_IS_INT(*idp))
        return true;
    RootedValue value(cx, IdToValue(*idp));
    if (!wrap(cx, &value))
        return false;
    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, value.get(), &id))
        return false;

    *idp = id;
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, PropertyOp *propp)
{
    RootedValue value(cx, CastAsObjectJsval(*propp));
    if (!wrap(cx, &value))
        return false;
    *propp = CastAsPropertyOp(value.toObjectOrNull());
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, StrictPropertyOp *propp)
{
    RootedValue value(cx, CastAsObjectJsval(*propp));
    if (!wrap(cx, &value))
        return false;
    *propp = CastAsStrictPropertyOp(value.toObjectOrNull());
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, PropertyDescriptor *desc)
{
    if (!wrap(cx, &desc->obj))
        return false;

    if (desc->attrs & JSPROP_GETTER) {
        if (!wrap(cx, &desc->getter))
            return false;
    }
    if (desc->attrs & JSPROP_SETTER) {
        if (!wrap(cx, &desc->setter))
            return false;
    }

    RootedValue value(cx, desc->value);
    if (!wrap(cx, &value))
        return false;
    desc->value = value.get();
    return true;
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
    JS_ASSERT(!zone()->isCollecting());

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
JSCompartment::sweep(FreeOp *fop, bool releaseTypes)
{
    JS_ASSERT(!activeAnalysis);

    
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

    if (!zone()->isPreservingCode()) {
        JS_ASSERT(!types.constrainedOutputs);
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_FREE_TI_ARENA);
        rt->freeLifoAlloc.transferFrom(&analysisLifoAlloc);
    } else {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);
        types.sweepShapes(fop);
    }

    NativeIterator *ni = enumerators->next();
    while (ni != enumerators) {
        JSObject *iterObj = ni->iterObj();
        NativeIterator *next = ni->next();
        if (gc::IsObjectAboutToBeFinalized(&iterObj))
            ni->unlink();
        ni = next;
    }
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

    for (gc::CellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this)
            script->debugMode = enabled;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!rt->isHeapBusy())
        dmgc.scheduleGC(zone());
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
    for (gc::CellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this && script->hasAnyBreakpointsOrStepMode())
            script->clearBreakpointsIn(fop, dbg, handler);
    }
}

void
JSCompartment::clearTraps(FreeOp *fop)
{
    for (gc::CellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this && script->hasAnyBreakpointsOrStepMode())
            script->clearTraps(fop);
    }
}

void
JSCompartment::sweepBreakpoints(FreeOp *fop)
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES_BREAKPOINT);

    if (rt->debuggerList.isEmpty())
        return;

    for (CellIterUnderGC i(zone(), FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() != this || !script->hasAnyBreakpointsOrStepMode())
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
                                   JS::TypeInferenceSizes *tiSizes, size_t *shapesCompartmentTables,
                                   size_t *crossCompartmentWrappersArg, size_t *regexpCompartment,
                                   size_t *debuggeesSet, size_t *baselineOptimizedStubs)
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
#ifdef JS_ION
    *baselineOptimizedStubs = ionCompartment()
        ? ionCompartment()->optimizedStubSpace()->sizeOfExcludingThis(mallocSizeOf)
        : 0;
#else
    *baselineOptimizedStubs = 0;
#endif
}

void
JSCompartment::adoptWorkerAllocator(Allocator *workerAllocator)
{
    zone()->allocator.arenas.adoptArenas(rt, &workerAllocator->arenas);
}
