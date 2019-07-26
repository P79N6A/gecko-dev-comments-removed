





#include "jscompartmentinlines.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"
#include "jsfriendapi.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsproxy.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#include "gc/Marking.h"
#ifdef JS_ION
#include "jit/JitCompartment.h"
#endif
#include "js/RootingAPI.h"
#include "vm/StopIterationObject.h"
#include "vm/WrapperObject.h"

#include "jsatominlines.h"
#include "jsfuninlines.h"
#include "jsgcinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;

JSCompartment::JSCompartment(Zone *zone, const JS::CompartmentOptions &options = JS::CompartmentOptions())
  : options_(options),
    zone_(zone),
    runtime_(zone->runtimeFromMainThread()),
    principals(nullptr),
    isSystem(false),
    marked(true),
#ifdef DEBUG
    firedOnNewGlobalObject(false),
#endif
    global_(nullptr),
    enterCompartmentDepth(0),
    lastCodeRelease(0),
    data(nullptr),
    objectMetadataCallback(nullptr),
    lastAnimationTime(0),
    regExps(runtime_),
    typeReprs(runtime_),
    globalWriteBarriered(false),
    propertyTree(thisForCtor()),
    gcIncomingGrayPointers(nullptr),
    gcLiveArrayBuffers(nullptr),
    gcWeakMapList(nullptr),
    debugModeBits(runtime_->debugMode ? DebugFromC : 0),
    rngState(0),
    watchpointMap(nullptr),
    scriptCountsMap(nullptr),
    debugScriptMap(nullptr),
    debugScopes(nullptr),
    enumerators(nullptr),
    compartmentStats(nullptr)
#ifdef JS_ION
    , jitCompartment_(nullptr)
#endif
{
    runtime_->numCompartments++;
}

JSCompartment::~JSCompartment()
{
#ifdef JS_ION
    js_delete(jitCompartment_);
#endif

    js_delete(watchpointMap);
    js_delete(scriptCountsMap);
    js_delete(debugScriptMap);
    js_delete(debugScopes);
    js_free(enumerators);

    runtime_->numCompartments--;
}

bool
JSCompartment::init(JSContext *cx)
{
    





    if (cx)
        cx->runtime()->dateTimeInfo.updateTimeZoneAdjustment();

    activeAnalysis = false;

    if (!crossCompartmentWrappers.init(0))
        return false;

    if (!regExps.init(cx))
        return false;

    if (!typeReprs.init())
        return false;

    enumerators = NativeIterator::allocateSentinel(cx);
    if (!enumerators)
        return false;

    return debuggees.init(0);
}

#ifdef JS_ION
jit::JitRuntime *
JSRuntime::createJitRuntime(JSContext *cx)
{
    
    
    AutoLockForExclusiveAccess atomsLock(cx);

    
    
    
    AutoLockForOperationCallback lock(this);

    JS_ASSERT(!jitRuntime_);

    jitRuntime_ = cx->new_<jit::JitRuntime>();

    if (!jitRuntime_)
        return nullptr;

    if (!jitRuntime_->initialize(cx)) {
        js_delete(jitRuntime_);
        jitRuntime_ = nullptr;

        JSCompartment *comp = cx->runtime()->atomsCompartment();
        if (comp->jitCompartment_) {
            js_delete(comp->jitCompartment_);
            comp->jitCompartment_ = nullptr;
        }

        return nullptr;
    }

    return jitRuntime_;
}

bool
JSCompartment::ensureJitCompartmentExists(JSContext *cx)
{
    using namespace js::jit;
    if (jitCompartment_)
        return true;

    JitRuntime *jitRuntime = cx->runtime()->getJitRuntime(cx);
    if (!jitRuntime)
        return false;

    
    jitCompartment_ = cx->new_<JitCompartment>(jitRuntime);

    if (!jitCompartment_)
        return false;

    if (!jitCompartment_->initialize(cx)) {
        js_delete(jitCompartment_);
        jitCompartment_ = nullptr;
        return false;
    }

    return true;
}
#endif

static bool
WrapForSameCompartment(JSContext *cx, MutableHandleObject obj)
{
    JS_ASSERT(cx->compartment() == obj->compartment());
    if (!cx->runtime()->sameCompartmentWrapObjectCallback)
        return true;

    RootedObject wrapped(cx);
    wrapped = cx->runtime()->sameCompartmentWrapObjectCallback(cx, obj);
    if (!wrapped)
        return false;
    obj.set(wrapped);
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
JSCompartment::wrap(JSContext *cx, JSString **strp)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(this));
    JS_ASSERT(cx->compartment() == this);

    
    JSString *str = *strp;
    if (str->zone() == zone())
        return true;

    
    if (str->isAtom()) {
        JS_ASSERT(cx->runtime()->isAtomsZone(str->zone()));
        return true;
    }

    
    RootedValue key(cx, StringValue(str));
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(key)) {
        *strp = p->value.get().toString();
        return true;
    }

    
    Rooted<JSLinearString *> linear(cx, str->ensureLinear(cx));
    if (!linear)
        return false;
    JSString *copy = js_NewStringCopyN<CanGC>(cx, linear->chars(),
                                              linear->length());
    if (!copy)
        return false;
    if (!putWrapper(key, StringValue(copy)))
        return false;

    if (linear->zone()->isGCMarking()) {
        





        JSString *tmp = linear;
        MarkStringUnbarriered(&cx->runtime()->gcMarker, &tmp, "wrapped string");
        JS_ASSERT(tmp == linear);
    }

    *strp = copy;
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, HeapPtrString *strp)
{
    RootedString str(cx, *strp);
    if (!wrap(cx, str.address()))
        return false;
    *strp = str;
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, MutableHandleObject obj, HandleObject existingArg)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(this));
    JS_ASSERT(cx->compartment() == this);
    JS_ASSERT_IF(existingArg, existingArg->compartment() == cx->compartment());
    JS_ASSERT_IF(existingArg, IsDeadProxyObject(existingArg));

    if (!obj)
        return true;
    AutoDisableProxyCheck adpc(cx->runtime());

    






    HandleObject global = cx->global();
    JS_ASSERT(global);

    if (obj->compartment() == this)
        return WrapForSameCompartment(cx, obj);

    
    unsigned flags = 0;
    obj.set(UncheckedUnwrap(obj,  true, &flags));

    if (obj->compartment() == this)
        return WrapForSameCompartment(cx, obj);

    
    if (obj->is<StopIterationObject>()) {
        RootedValue v(cx);
        if (!js_FindClassObject(cx, JSProto_StopIteration, &v))
            return false;
        obj.set(&v.toObject());
        return true;
    }

    

    JS_CHECK_CHROME_RECURSION(cx, return false);
    if (cx->runtime()->preWrapObjectCallback) {
        obj.set(cx->runtime()->preWrapObjectCallback(cx, global, obj, flags));
        if (!obj)
            return false;
    }

    if (obj->compartment() == this)
        return WrapForSameCompartment(cx, obj);

#ifdef DEBUG
    {
        JSObject *outer = GetOuterObject(cx, obj);
        JS_ASSERT(outer && outer == obj);
    }
#endif

    
    RootedValue key(cx, ObjectValue(*obj));
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(key)) {
        obj.set(&p->value.get().toObject());
        JS_ASSERT(obj->is<CrossCompartmentWrapperObject>());
        JS_ASSERT(obj->getParent() == global);
        return true;
    }

    RootedObject proto(cx, Proxy::LazyProto);
    RootedObject existing(cx, existingArg);
    if (existing) {
        
        if (!existing->getTaggedProto().isLazy() ||
            
            existing->getClass() != &ProxyObject::uncallableClass_ ||
            existing->getParent() != global ||
            obj->isCallable())
        {
            existing = nullptr;
        }
    }

    




    obj.set(cx->runtime()->wrapObjectCallback(cx, existing, obj, proto, global, flags));
    if (!obj)
        return false;

    



    JS_ASSERT(Wrapper::wrappedObject(obj) == &key.get().toObject());

    return putWrapper(key, ObjectValue(*obj));
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
    if (!ValueToId<CanGC>(cx, value, &id))
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
JSCompartment::wrap(JSContext *cx, MutableHandle<PropertyDescriptor> desc)
{
    if (!wrap(cx, desc.object()))
        return false;

    if (desc.hasGetterObject()) {
        if (!wrap(cx, &desc.getter()))
            return false;
    }
    if (desc.hasSetterObject()) {
        if (!wrap(cx, &desc.setter()))
            return false;
    }

    return wrap(cx, desc.value());
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
            ProxyObject *wrapper = &v.toObject().as<ProxyObject>();

            



            Value referent = wrapper->private_();
            MarkValueRoot(trc, &referent, "cross-compartment wrapper");
            JS_ASSERT(referent == wrapper->private_());
        }
    }
}







void
JSCompartment::markAllCrossCompartmentWrappers(JSTracer *trc)
{
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey key = e.front().key;
        MarkGCThingRoot(trc, (void **)&key.wrapped, "CrossCompartmentKey::wrapped");
        if (key.debugger)
            MarkObjectRoot(trc, &key.debugger, "CrossCompartmentKey::debugger");
        MarkValueRoot(trc, e.front().value.unsafeGet(), "CrossCompartmentWrapper");
        if (key.wrapped != e.front().key.wrapped || key.debugger != e.front().key.debugger)
            e.rekeyFront(key);
    }
}

void
JSCompartment::mark(JSTracer *trc)
{
    JS_ASSERT(!trc->runtime->isHeapMinorCollecting());

#ifdef JS_ION
    if (jitCompartment_)
        jitCompartment_->mark(trc, this);
#endif

    



    if (enterCompartmentDepth && global_)
        MarkObjectRoot(trc, global_.unsafeGet(), "on-stack compartment global");
}

void
JSCompartment::sweep(FreeOp *fop, bool releaseTypes)
{
    JS_ASSERT(!activeAnalysis);

    
    sweepCrossCompartmentWrappers();

    JSRuntime *rt = runtimeFromMainThread();

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES);

        

        sweepBaseShapeTable();
        sweepInitialShapeTable();
        sweepNewTypeObjectTable(newTypeObjects);
        sweepNewTypeObjectTable(lazyTypeObjects);
        sweepCallsiteClones();

        if (global_ && IsObjectAboutToBeFinalized(global_.unsafeGet()))
            global_ = nullptr;

#ifdef JS_ION
        if (jitCompartment_)
            jitCompartment_->sweep(fop);
#endif

        




        regExps.sweep(rt);

        if (debugScopes)
            debugScopes->sweep(rt);

        
        WeakMapBase::sweepCompartment(this);
    }

    if (zone()->isPreservingCode()) {
        gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);
        types.sweepShapes(fop);
    } else {
        JS_ASSERT(!types.constrainedOutputs);
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
    JSRuntime *rt = runtimeFromMainThread();

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
JSCompartment::clearTables()
{
    global_ = nullptr;

    regExps.clearTables();

    
    
    
    JS_ASSERT(crossCompartmentWrappers.empty());
    JS_ASSERT_IF(callsiteClones.initialized(), callsiteClones.empty());
#ifdef JS_ION
    JS_ASSERT(!jitCompartment_);
#endif
    JS_ASSERT(!debugScopes);
    JS_ASSERT(!gcWeakMapList);
    JS_ASSERT(enumerators->next() == enumerators);

    if (baseShapes.initialized())
        baseShapes.clear();
    if (initialShapes.initialized())
        initialShapes.clear();
    if (newTypeObjects.initialized())
        newTypeObjects.clear();
    if (lazyTypeObjects.initialized())
        lazyTypeObjects.clear();
}

void
JSCompartment::setObjectMetadataCallback(js::ObjectMetadataCallback callback)
{
    
    
    ReleaseAllJITCode(runtime_->defaultFreeOp());

    
    
    
    if (callback)
        JS::DisableGenerationalGC(runtime_);
    else
        JS::EnableGenerationalGC(runtime_);

    objectMetadataCallback = callback;
}

bool
JSCompartment::hasScriptsOnStack()
{
    for (ActivationIterator iter(runtimeFromMainThread()); !iter.done(); ++iter) {
        if (iter.activation()->compartment() == this)
            return true;
    }

    return false;
}

static bool
AddInnerLazyFunctionsFromScript(JSScript *script, AutoObjectVector &lazyFunctions)
{
    if (!script->hasObjects())
        return true;
    ObjectArray *objects = script->objects();
    for (size_t i = script->innerObjectsStart(); i < objects->length; i++) {
        JSObject *obj = objects->vector[i];
        if (obj->is<JSFunction>() && obj->as<JSFunction>().isInterpretedLazy()) {
            if (!lazyFunctions.append(obj))
                return false;
        }
    }
    return true;
}

static bool
CreateLazyScriptsForCompartment(JSContext *cx)
{
    AutoObjectVector lazyFunctions(cx);

    
    
    
    for (gc::CellIter i(cx->zone(), JSFunction::FinalizeKind); !i.done(); i.next()) {
        JSObject *obj = i.get<JSObject>();
        if (obj->compartment() == cx->compartment() && obj->is<JSFunction>()) {
            JSFunction *fun = &obj->as<JSFunction>();
            if (fun->isInterpretedLazy()) {
                LazyScript *lazy = fun->lazyScriptOrNull();
                if (lazy && lazy->sourceObject() && !lazy->maybeScript()) {
                    if (!lazyFunctions.append(fun))
                        return false;
                }
            }
        }
    }

    
    
    
    for (size_t i = 0; i < lazyFunctions.length(); i++) {
        JSFunction *fun = &lazyFunctions[i]->as<JSFunction>();

        
        
        if (!fun->isInterpretedLazy())
            continue;

        JSScript *script = fun->getOrCreateScript(cx);
        if (!script)
            return false;
        if (!AddInnerLazyFunctionsFromScript(script, lazyFunctions))
            return false;
    }

    
    for (gc::CellIter i(cx->zone(), JSFunction::FinalizeKind); !i.done(); i.next()) {
        JSObject *obj = i.get<JSObject>();
        if (obj->compartment() == cx->compartment() && obj->is<JSFunction>()) {
            JSFunction *fun = &obj->as<JSFunction>();
            if (fun->isInterpretedLazy()) {
                LazyScript *lazy = fun->lazyScriptOrNull();
                if (lazy && lazy->maybeScript())
                    fun->existingScript();
            }
        }
    }

    return true;
}

bool
JSCompartment::setDebugModeFromC(JSContext *cx, bool b, AutoDebugModeInvalidation &invalidate)
{
    bool enabledBefore = debugMode();
    bool enabledAfter = (debugModeBits & ~unsigned(DebugFromC)) || b;

    
    
    
    
    
    
    
    
    
    
    bool onStack = false;
    if (enabledBefore != enabledAfter) {
        onStack = hasScriptsOnStack();
        if (b && onStack) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_IDLE);
            return false;
        }
        if (enabledAfter && !CreateLazyScriptsForCompartment(cx))
            return false;
    }

    debugModeBits = (debugModeBits & ~unsigned(DebugFromC)) | (b ? DebugFromC : 0);
    JS_ASSERT(debugMode() == enabledAfter);
    if (enabledBefore != enabledAfter) {
        updateForDebugMode(cx->runtime()->defaultFreeOp(), invalidate);
        if (!enabledAfter)
            DebugScopes::onCompartmentLeaveDebugMode(this);
    }
    return true;
}

void
JSCompartment::updateForDebugMode(FreeOp *fop, AutoDebugModeInvalidation &invalidate)
{
    JSRuntime *rt = runtimeFromMainThread();

    for (ContextIter acx(rt); !acx.done(); acx.next()) {
        if (acx->compartment() == this)
            acx->updateJITEnabled();
    }

#ifdef JS_ION
    MOZ_ASSERT(invalidate.isFor(this));
    JS_ASSERT_IF(debugMode(), !hasScriptsOnStack());

    
    
    
    
    
    
    
    invalidate.scheduleInvalidation(debugMode());
#endif
}

bool
JSCompartment::addDebuggee(JSContext *cx, js::GlobalObject *global)
{
    AutoDebugModeInvalidation invalidate(this);
    return addDebuggee(cx, global, invalidate);
}

bool
JSCompartment::addDebuggee(JSContext *cx,
                           GlobalObject *globalArg,
                           AutoDebugModeInvalidation &invalidate)
{
    Rooted<GlobalObject*> global(cx, globalArg);

    bool wasEnabled = debugMode();
    if (!wasEnabled && !CreateLazyScriptsForCompartment(cx))
        return false;
    if (!debuggees.put(global)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    debugModeBits |= DebugFromJS;
    if (!wasEnabled)
        updateForDebugMode(cx->runtime()->defaultFreeOp(), invalidate);
    return true;
}

void
JSCompartment::removeDebuggee(FreeOp *fop,
                              js::GlobalObject *global,
                              js::GlobalObjectSet::Enum *debuggeesEnum)
{
    AutoDebugModeInvalidation invalidate(this);
    return removeDebuggee(fop, global, invalidate, debuggeesEnum);
}

void
JSCompartment::removeDebuggee(FreeOp *fop,
                              js::GlobalObject *global,
                              AutoDebugModeInvalidation &invalidate,
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
            updateForDebugMode(fop, invalidate);
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
    MinorGC(fop->runtime(), JS::gcreason::EVICT_NURSERY);
    for (gc::CellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this && script->hasAnyBreakpointsOrStepMode())
            script->clearTraps(fop);
    }
}

void
JSCompartment::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                      size_t *tiPendingArrays,
                                      size_t *tiAllocationSiteTables,
                                      size_t *tiArrayTypeTables,
                                      size_t *tiObjectTypeTables,
                                      size_t *compartmentObject,
                                      size_t *shapesCompartmentTables,
                                      size_t *crossCompartmentWrappersArg,
                                      size_t *regexpCompartment,
                                      size_t *debuggeesSet,
                                      size_t *baselineStubsOptimized)
{
    *compartmentObject += mallocSizeOf(this);
    types.addSizeOfExcludingThis(mallocSizeOf, tiPendingArrays, tiAllocationSiteTables,
                                 tiArrayTypeTables, tiObjectTypeTables);
    *shapesCompartmentTables += baseShapes.sizeOfExcludingThis(mallocSizeOf)
                              + initialShapes.sizeOfExcludingThis(mallocSizeOf)
                              + newTypeObjects.sizeOfExcludingThis(mallocSizeOf)
                              + lazyTypeObjects.sizeOfExcludingThis(mallocSizeOf);
    *crossCompartmentWrappersArg += crossCompartmentWrappers.sizeOfExcludingThis(mallocSizeOf);
    *regexpCompartment += regExps.sizeOfExcludingThis(mallocSizeOf);
    *debuggeesSet += debuggees.sizeOfExcludingThis(mallocSizeOf);
#ifdef JS_ION
    if (jitCompartment()) {
        *baselineStubsOptimized +=
            jitCompartment()->optimizedStubSpace()->sizeOfExcludingThis(mallocSizeOf);
    }
#endif
}

void
JSCompartment::adoptWorkerAllocator(Allocator *workerAllocator)
{
    zone()->allocator.arenas.adoptArenas(runtimeFromMainThread(), &workerAllocator->arenas);
}
