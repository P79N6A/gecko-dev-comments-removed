





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
    isSelfHosting(false),
    marked(true),
#ifdef DEBUG
    firedOnNewGlobalObject(false),
#endif
    global_(nullptr),
    enterCompartmentDepth(0),
    data(nullptr),
    objectMetadataCallback(nullptr),
    lastAnimationTime(0),
    regExps(runtime_),
    globalWriteBarriered(false),
    propertyTree(thisForCtor()),
    selfHostingScriptSource(nullptr),
    gcIncomingGrayPointers(nullptr),
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
    JS_ASSERT_IF(options.mergeable(), options.invisibleToDebugger());
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

    enumerators = NativeIterator::allocateSentinel(cx);
    if (!enumerators)
        return false;

    if (!savedStacks_.init())
        return false;

    return debuggees.init(0);
}

#ifdef JS_ION
jit::JitRuntime *
JSRuntime::createJitRuntime(JSContext *cx)
{
    
    
    AutoLockForExclusiveAccess atomsLock(cx);

    
    
    
    AutoLockForInterrupt lock(this);

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

    if (!zone()->getJitZone(cx))
        return false;

    
    jitCompartment_ = cx->new_<JitCompartment>();

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

#ifdef JSGC_GENERATIONAL






class WrapperMapRef : public BufferableRef
{
    WrapperMap *map;
    CrossCompartmentKey key;

  public:
    WrapperMapRef(WrapperMap *map, const CrossCompartmentKey &key)
      : map(map), key(key) {}

    void mark(JSTracer *trc) {
        CrossCompartmentKey prior = key;
        if (key.debugger)
            Mark(trc, &key.debugger, "CCW debugger");
        if (key.kind != CrossCompartmentKey::StringWrapper)
            Mark(trc, reinterpret_cast<JSObject**>(&key.wrapped), "CCW wrapped object");
        if (key.debugger == prior.debugger && key.wrapped == prior.wrapped)
            return;

        
        WrapperMap::Ptr p = map->lookup(prior);
        if (!p)
            return;

        
        map->rekeyAs(prior, key, key);
    }
};

#ifdef JS_GC_ZEAL
void
JSCompartment::checkWrapperMapAfterMovingGC()
{
    




    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey key = e.front().key();
        JS_ASSERT(!IsInsideNursery(key.debugger));
        JS_ASSERT(!IsInsideNursery(key.wrapped));
        JS_ASSERT(!IsInsideNursery(static_cast<Cell *>(e.front().value().get().toGCThing())));

        WrapperMap::Ptr ptr = crossCompartmentWrappers.lookup(key);
        JS_ASSERT(ptr.found() && &*ptr == &e.front());
    }
}
#endif

#endif

bool
JSCompartment::putWrapper(JSContext *cx, const CrossCompartmentKey &wrapped, const js::Value &wrapper)
{
    JS_ASSERT(wrapped.wrapped);
    JS_ASSERT(!IsPoisonedPtr(wrapped.wrapped));
    JS_ASSERT(!IsPoisonedPtr(wrapped.debugger));
    JS_ASSERT(!IsPoisonedPtr(wrapper.toGCThing()));
    JS_ASSERT_IF(wrapped.kind == CrossCompartmentKey::StringWrapper, wrapper.isString());
    JS_ASSERT_IF(wrapped.kind != CrossCompartmentKey::StringWrapper, wrapper.isObject());
    bool success = crossCompartmentWrappers.put(wrapped, wrapper);

#ifdef JSGC_GENERATIONAL
    
    JS_ASSERT(!IsInsideNursery(static_cast<gc::Cell *>(wrapper.toGCThing())));

    if (success && (IsInsideNursery(wrapped.wrapped) || IsInsideNursery(wrapped.debugger))) {
        WrapperMapRef ref(&crossCompartmentWrappers, wrapped);
        cx->runtime()->gc.storeBuffer.putGeneric(ref);
    }
#endif

    return success;
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
        *strp = p->value().get().toString();
        return true;
    }

    





    JSString *copy;
    if (str->hasPureChars()) {
        copy = js_NewStringCopyN<CanGC>(cx, str->pureChars(), str->length());
    } else {
        ScopedJSFreePtr<jschar> copiedChars;
        if (!str->copyNonPureCharsZ(cx, copiedChars))
            return false;
        copy = js_NewString<CanGC>(cx, copiedChars.forget(), str->length());
    }

    if (!copy)
        return false;
    if (!putWrapper(cx, key, StringValue(copy)))
        return false;

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
    RootedObject objGlobal(cx, &obj->global());
    JS_ASSERT(global);
    JS_ASSERT(objGlobal);

    const JSWrapObjectCallbacks *cb = cx->runtime()->wrapObjectCallbacks;

    if (obj->compartment() == this) {
        obj.set(GetOuterObject(cx, obj));
        return true;
    }

    
    
    
    
    JS_ASSERT(!cx->runtime()->isSelfHostingGlobal(global) &&
              !cx->runtime()->isSelfHostingGlobal(objGlobal));

    
    unsigned flags = 0;
    obj.set(UncheckedUnwrap(obj,  true, &flags));

    if (obj->compartment() == this) {
        MOZ_ASSERT(obj == GetOuterObject(cx, obj));
        return true;
    }

    
    if (obj->is<StopIterationObject>()) {
        
        
        RootedObject stopIteration(cx);
        if (!GetBuiltinConstructor(cx, JSProto_StopIteration, &stopIteration))
            return false;
        obj.set(stopIteration);
        return true;
    }

    
    
    JS_CHECK_CHROME_RECURSION(cx, return false);
    if (cb->preWrap) {
        obj.set(cb->preWrap(cx, global, obj, flags));
        if (!obj)
            return false;
    }
    MOZ_ASSERT(obj == GetOuterObject(cx, obj));

    if (obj->compartment() == this)
        return true;


    
    RootedValue key(cx, ObjectValue(*obj));
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(key)) {
        obj.set(&p->value().get().toObject());
        JS_ASSERT(obj->is<CrossCompartmentWrapperObject>());
        JS_ASSERT(obj->getParent() == global);
        return true;
    }

    RootedObject proto(cx, TaggedProto::LazyProto);
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

    obj.set(cb->wrap(cx, existing, obj, proto, global, flags));
    if (!obj)
        return false;

    
    
    JS_ASSERT(Wrapper::wrappedObject(obj) == &key.get().toObject());

    return putWrapper(cx, key, ObjectValue(*obj));
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
        Value v = e.front().value();
        if (e.front().key().kind == CrossCompartmentKey::ObjectWrapper) {
            ProxyObject *wrapper = &v.toObject().as<ProxyObject>();

            



            Value referent = wrapper->private_();
            MarkValueRoot(trc, &referent, "cross-compartment wrapper");
            JS_ASSERT(referent == wrapper->private_());
        }
    }
}

void
JSCompartment::trace(JSTracer *trc)
{
    
    
}

void
JSCompartment::markRoots(JSTracer *trc)
{
    JS_ASSERT(!trc->runtime()->isHeapMinorCollecting());

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
        gcstats::AutoPhase ap(rt->gc.stats, gcstats::PHASE_SWEEP_TABLES);

        

        sweepBaseShapeTable();
        sweepInitialShapeTable();
        sweepNewTypeObjectTable(newTypeObjects);
        sweepNewTypeObjectTable(lazyTypeObjects);
        sweepCallsiteClones();
        savedStacks_.sweep(rt);

        if (global_ && IsObjectAboutToBeFinalized(global_.unsafeGet()))
            global_ = nullptr;

        if (selfHostingScriptSource &&
            IsObjectAboutToBeFinalized((JSObject **) selfHostingScriptSource.unsafeGet()))
        {
            selfHostingScriptSource = nullptr;
        }

#ifdef JS_ION
        if (jitCompartment_)
            jitCompartment_->sweep(fop);
#endif

        




        regExps.sweep(rt);

        if (debugScopes)
            debugScopes->sweep(rt);

        
        WeakMapBase::sweepCompartment(this);
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

    gcstats::AutoPhase ap1(rt->gc.stats, gcstats::PHASE_SWEEP_TABLES);
    gcstats::AutoPhase ap2(rt->gc.stats, gcstats::PHASE_SWEEP_TABLES_WRAPPER);

    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey key = e.front().key();
        bool keyDying = IsCellAboutToBeFinalized(&key.wrapped);
        bool valDying = IsValueAboutToBeFinalized(e.front().value().unsafeGet());
        bool dbgDying = key.debugger && IsObjectAboutToBeFinalized(&key.debugger);
        if (keyDying || valDying || dbgDying) {
            JS_ASSERT(key.kind != CrossCompartmentKey::StringWrapper);
            e.removeFront();
        } else if (key.wrapped != e.front().key().wrapped ||
                   key.debugger != e.front().key().debugger)
        {
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

    
    
    
    JS_ASSERT(crossCompartmentWrappers.empty());
    JS_ASSERT_IF(callsiteClones.initialized(), callsiteClones.empty());
#ifdef JS_ION
    JS_ASSERT(!jitCompartment_);
#endif
    JS_ASSERT(!debugScopes);
    JS_ASSERT(!gcWeakMapList);
    JS_ASSERT(enumerators->next() == enumerators);
    JS_ASSERT(regExps.empty());

    types.clearTables();
    if (baseShapes.initialized())
        baseShapes.clear();
    if (initialShapes.initialized())
        initialShapes.clear();
    if (newTypeObjects.initialized())
        newTypeObjects.clear();
    if (lazyTypeObjects.initialized())
        lazyTypeObjects.clear();
    if (savedStacks_.initialized())
        savedStacks_.clear();
}

void
JSCompartment::setObjectMetadataCallback(js::ObjectMetadataCallback callback)
{
    
    
    ReleaseAllJITCode(runtime_->defaultFreeOp());

    objectMetadataCallback = callback;
}

bool
JSCompartment::hasScriptsOnStack()
{
    for (ActivationIterator iter(runtimeFromMainThread()); !iter.done(); ++iter) {
        if (iter->compartment() == this)
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

    
    
    
    
    
    
    for (gc::ZoneCellIter i(cx->zone(), gc::FINALIZE_LAZY_SCRIPT); !i.done(); i.next()) {
        LazyScript *lazy = i.get<LazyScript>();
        JSFunction *fun = lazy->functionNonDelazifying();
        if (fun->compartment() == cx->compartment() &&
            lazy->sourceObject() && !lazy->maybeScript() &&
            !lazy->hasUncompiledEnclosingScript())
        {
            MOZ_ASSERT(fun->isInterpretedLazy());
            MOZ_ASSERT(lazy == fun->lazyScriptOrNull());
            if (!lazyFunctions.append(fun))
                return false;
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

    return true;
}

bool
JSCompartment::ensureDelazifyScriptsForDebugMode(JSContext *cx)
{
    MOZ_ASSERT(cx->compartment() == this);
    if ((debugModeBits & DebugNeedDelazification) && !CreateLazyScriptsForCompartment(cx))
        return false;
    debugModeBits &= ~DebugNeedDelazification;
    return true;
}

bool
JSCompartment::setDebugModeFromC(JSContext *cx, bool b, AutoDebugModeInvalidation &invalidate)
{
    bool enabledBefore = debugMode();
    bool enabledAfter = (debugModeBits & DebugModeFromMask & ~DebugFromC) || b;

    
    
    
    
    
    
    
    bool onStack = false;
    if (enabledBefore != enabledAfter) {
        onStack = hasScriptsOnStack();
        if (b && onStack) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_IDLE);
            return false;
        }
    }

    debugModeBits = (debugModeBits & ~DebugFromC) | (b ? DebugFromC : 0);
    JS_ASSERT(debugMode() == enabledAfter);
    if (enabledBefore != enabledAfter) {
        
        
        if (!updateJITForDebugMode(nullptr, invalidate))
            return false;
        if (!enabledAfter)
            DebugScopes::onCompartmentLeaveDebugMode(this);
    }
    return true;
}

bool
JSCompartment::updateJITForDebugMode(JSContext *maybecx, AutoDebugModeInvalidation &invalidate)
{
#ifdef JS_ION
    
    
    
    
    if (!jit::UpdateForDebugMode(maybecx, this, invalidate))
        return false;
#endif
    return true;
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
    if (!debuggees.put(global)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    debugModeBits |= DebugFromJS;
    if (!wasEnabled && !updateJITForDebugMode(cx, invalidate))
        return false;
    return true;
}

bool
JSCompartment::removeDebuggee(JSContext *cx,
                              js::GlobalObject *global,
                              js::GlobalObjectSet::Enum *debuggeesEnum)
{
    AutoDebugModeInvalidation invalidate(this);
    return removeDebuggee(cx, global, invalidate, debuggeesEnum);
}

bool
JSCompartment::removeDebuggee(JSContext *cx,
                              js::GlobalObject *global,
                              AutoDebugModeInvalidation &invalidate,
                              js::GlobalObjectSet::Enum *debuggeesEnum)
{
    bool wasEnabled = debugMode();
    removeDebuggeeUnderGC(cx->runtime()->defaultFreeOp(), global, invalidate, debuggeesEnum);
    if (wasEnabled && !debugMode() && !updateJITForDebugMode(cx, invalidate))
        return false;
    return true;
}

void
JSCompartment::removeDebuggeeUnderGC(FreeOp *fop,
                                     js::GlobalObject *global,
                                     js::GlobalObjectSet::Enum *debuggeesEnum)
{
    AutoDebugModeInvalidation invalidate(this);
    removeDebuggeeUnderGC(fop, global, invalidate, debuggeesEnum);
}

void
JSCompartment::removeDebuggeeUnderGC(FreeOp *fop,
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
        if (wasEnabled && !debugMode())
            DebugScopes::onCompartmentLeaveDebugMode(this);
    }
}

void
JSCompartment::clearBreakpointsIn(FreeOp *fop, js::Debugger *dbg, JSObject *handler)
{
    for (gc::ZoneCellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this && script->hasAnyBreakpointsOrStepMode())
            script->clearBreakpointsIn(fop, dbg, handler);
    }
}

void
JSCompartment::clearTraps(FreeOp *fop)
{
    MinorGC(fop->runtime(), JS::gcreason::EVICT_NURSERY);
    for (gc::ZoneCellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this && script->hasAnyBreakpointsOrStepMode())
            script->clearTraps(fop);
    }
}

void
JSCompartment::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                      size_t *tiAllocationSiteTables,
                                      size_t *tiArrayTypeTables,
                                      size_t *tiObjectTypeTables,
                                      size_t *compartmentObject,
                                      size_t *shapesCompartmentTables,
                                      size_t *crossCompartmentWrappersArg,
                                      size_t *regexpCompartment,
                                      size_t *debuggeesSet,
                                      size_t *savedStacksSet)
{
    *compartmentObject += mallocSizeOf(this);
    types.addSizeOfExcludingThis(mallocSizeOf, tiAllocationSiteTables,
                                 tiArrayTypeTables, tiObjectTypeTables);
    *shapesCompartmentTables += baseShapes.sizeOfExcludingThis(mallocSizeOf)
                              + initialShapes.sizeOfExcludingThis(mallocSizeOf)
                              + newTypeObjects.sizeOfExcludingThis(mallocSizeOf)
                              + lazyTypeObjects.sizeOfExcludingThis(mallocSizeOf);
    *crossCompartmentWrappersArg += crossCompartmentWrappers.sizeOfExcludingThis(mallocSizeOf);
    *regexpCompartment += regExps.sizeOfExcludingThis(mallocSizeOf);
    *debuggeesSet += debuggees.sizeOfExcludingThis(mallocSizeOf);
    *savedStacksSet += savedStacks_.sizeOfExcludingThis(mallocSizeOf);
}

void
JSCompartment::adoptWorkerAllocator(Allocator *workerAllocator)
{
    zone()->allocator.arenas.adoptArenas(runtimeFromMainThread(), &workerAllocator->arenas);
}
