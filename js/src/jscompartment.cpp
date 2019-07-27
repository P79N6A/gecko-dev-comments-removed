





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
#include "jit/JitCompartment.h"
#include "js/RootingAPI.h"
#include "proxy/DeadObjectProxy.h"
#include "vm/Debugger.h"
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
    addonId(options.addonIdOrNull()),
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
    debugModeBits(0),
    rngState(0),
    watchpointMap(nullptr),
    scriptCountsMap(nullptr),
    debugScriptMap(nullptr),
    debugScopes(nullptr),
    enumerators(nullptr),
    compartmentStats(nullptr),
    scheduledForDestruction(false),
    maybeAlive(true),
    jitCompartment_(nullptr)
{
    runtime_->numCompartments++;
    JS_ASSERT_IF(options.mergeable(), options.invisibleToDebugger());
}

JSCompartment::~JSCompartment()
{
    js_delete(jitCompartment_);
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

    return true;
}

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

#ifdef JSGC_HASH_TABLE_CHECKS
void
JSCompartment::checkWrapperMapAfterMovingGC()
{
    




    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey key = e.front().key();
        CheckGCThingAfterMovingGC(key.debugger);
        CheckGCThingAfterMovingGC(key.wrapped);
        CheckGCThingAfterMovingGC(static_cast<Cell *>(e.front().value().get().toGCThing()));

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
    bool success = crossCompartmentWrappers.put(wrapped, ReadBarriered<Value>(wrapper));

#ifdef JSGC_GENERATIONAL
    
    JS_ASSERT(!IsInsideNursery(static_cast<gc::Cell *>(wrapper.toGCThing())));

    if (success && (IsInsideNursery(wrapped.wrapped) || IsInsideNursery(wrapped.debugger))) {
        WrapperMapRef ref(&crossCompartmentWrappers, wrapped);
        cx->runtime()->gc.storeBuffer.putGeneric(ref);
    }
#endif

    return success;
}

static JSString *
CopyStringPure(JSContext *cx, JSString *str)
{
    





    size_t len = str->length();
    JSString *copy;
    if (str->isLinear()) {
        
        if (str->hasLatin1Chars()) {
            JS::AutoCheckCannotGC nogc;
            copy = NewStringCopyN<NoGC>(cx, str->asLinear().latin1Chars(nogc), len);
        } else {
            JS::AutoCheckCannotGC nogc;
            copy = NewStringCopyNDontDeflate<NoGC>(cx, str->asLinear().twoByteChars(nogc), len);
        }
        if (copy)
            return copy;

        AutoStableStringChars chars(cx);
        if (!chars.init(cx, str))
            return nullptr;

        return chars.isLatin1()
               ? NewStringCopyN<CanGC>(cx, chars.latin1Range().start().get(), len)
               : NewStringCopyNDontDeflate<CanGC>(cx, chars.twoByteRange().start().get(), len);
    }

    if (str->hasLatin1Chars()) {
        ScopedJSFreePtr<Latin1Char> copiedChars;
        if (!str->asRope().copyLatin1CharsZ(cx, copiedChars))
            return nullptr;

        return NewString<CanGC>(cx, copiedChars.forget(), len);
    }

    ScopedJSFreePtr<char16_t> copiedChars;
    if (!str->asRope().copyTwoByteCharsZ(cx, copiedChars))
        return nullptr;

    return NewStringDontDeflate<CanGC>(cx, copiedChars.forget(), len);
}

bool
JSCompartment::wrap(JSContext *cx, JSString **strp)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(this));
    JS_ASSERT(cx->compartment() == this);

    
    JSString *str = *strp;
    if (str->zoneFromAnyThread() == zone())
        return true;

    
    if (str->isAtom()) {
        JS_ASSERT(str->isPermanentAtom() ||
                  cx->runtime()->isAtomsZone(str->zone()));
        return true;
    }

    
    RootedValue key(cx, StringValue(str));
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(CrossCompartmentKey(key))) {
        *strp = p->value().get().toString();
        return true;
    }

    
    JSString *copy = CopyStringPure(cx, str);
    if (!copy)
        return false;
    if (!putWrapper(cx, CrossCompartmentKey(key), StringValue(copy)))
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

    
    RootedObject objectPassedToWrap(cx, obj);
    obj.set(UncheckedUnwrap(obj,  true));

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
        obj.set(cb->preWrap(cx, global, obj, objectPassedToWrap));
        if (!obj)
            return false;
    }
    MOZ_ASSERT(obj == GetOuterObject(cx, obj));

    if (obj->compartment() == this)
        return true;


    
    RootedValue key(cx, ObjectValue(*obj));
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(CrossCompartmentKey(key))) {
        obj.set(&p->value().get().toObject());
        JS_ASSERT(obj->is<CrossCompartmentWrapperObject>());
        JS_ASSERT(obj->getParent() == global);
        return true;
    }

    RootedObject existing(cx, existingArg);
    if (existing) {
        
        if (!existing->getTaggedProto().isLazy() ||
            
            existing->isCallable() ||
            existing->getParent() != global ||
            obj->isCallable())
        {
            existing = nullptr;
        }
    }

    obj.set(cb->wrap(cx, existing, obj, global));
    if (!obj)
        return false;

    
    
    JS_ASSERT(Wrapper::wrappedObject(obj) == &key.get().toObject());

    return putWrapper(cx, CrossCompartmentKey(key), ObjectValue(*obj));
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
JSCompartment::wrap(JSContext *cx, MutableHandle<PropDesc> desc)
{
    if (desc.isUndefined())
        return true;

    JSCompartment *comp = cx->compartment();

    if (desc.hasValue()) {
        RootedValue value(cx, desc.value());
        if (!comp->wrap(cx, &value))
            return false;
        desc.setValue(value);
    }
    if (desc.hasGet()) {
        RootedValue get(cx, desc.getterValue());
        if (!comp->wrap(cx, &get))
            return false;
        desc.setGetter(get);
    }
    if (desc.hasSet()) {
        RootedValue set(cx, desc.setterValue());
        if (!comp->wrap(cx, &set))
            return false;
        desc.setSetter(set);
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
    savedStacks_.trace(trc);
}

void
JSCompartment::markRoots(JSTracer *trc)
{
    JS_ASSERT(!trc->runtime()->isHeapMinorCollecting());

    if (jitCompartment_)
        jitCompartment_->mark(trc, this);

    



    if (enterCompartmentDepth && global_)
        MarkObjectRoot(trc, global_.unsafeGet(), "on-stack compartment global");
}

void
JSCompartment::sweep(FreeOp *fop, bool releaseTypes)
{
    JS_ASSERT(!activeAnalysis);
    JSRuntime *rt = runtimeFromMainThread();

    {
        gcstats::MaybeAutoPhase ap(rt->gc.stats, !rt->isHeapCompacting(),
                                   gcstats::PHASE_SWEEP_TABLES_WRAPPER);
        sweepCrossCompartmentWrappers();
    }

    

    sweepBaseShapeTable();
    sweepInitialShapeTable();
    {
        gcstats::MaybeAutoPhase ap(rt->gc.stats, !rt->isHeapCompacting(),
                                   gcstats::PHASE_SWEEP_TABLES_TYPE_OBJECT);
        sweepNewTypeObjectTable(newTypeObjects);
        sweepNewTypeObjectTable(lazyTypeObjects);
    }
    sweepCallsiteClones();
    savedStacks_.sweep(rt);

    if (global_ && IsObjectAboutToBeFinalized(global_.unsafeGet())) {
        if (debugMode())
            Debugger::detachAllDebuggersFromGlobal(fop, global_);
        global_.set(nullptr);
    }

    if (selfHostingScriptSource &&
        IsObjectAboutToBeFinalized((JSObject **) selfHostingScriptSource.unsafeGet()))
    {
        selfHostingScriptSource.set(nullptr);
    }

    if (jitCompartment_)
        jitCompartment_->sweep(fop, this);

    




    regExps.sweep(rt);

    if (debugScopes)
        debugScopes->sweep(rt);

    
    WeakMapBase::sweepCompartment(this);

    
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

#ifdef JSGC_COMPACTING




void
JSCompartment::fixupCrossCompartmentWrappers(JSTracer *trc)
{
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        Value val = e.front().value();
        if (IsForwarded(val)) {
            val = Forwarded(val);
            e.front().value().set(val);
        }

        
        
        CrossCompartmentKey key = e.front().key();
        if (key.debugger)
            key.debugger = MaybeForwarded(key.debugger);
        if (key.wrapped && IsForwarded(key.wrapped)) {
            key.wrapped = Forwarded(key.wrapped);
            e.rekeyFront(key, key);
        }

        if (!zone()->isCollecting() && val.isObject()) {
            
            JSObject *obj = &val.toObject();
            const Class *clasp = obj->getClass();
            if (clasp->trace)
                clasp->trace(trc, obj);
        }
    }
}

void JSCompartment::fixupAfterMovingGC()
{
    fixupGlobal();
    fixupNewTypeObjectTable(newTypeObjects);
    fixupNewTypeObjectTable(lazyTypeObjects);
    fixupInitialShapeTable();
}

void
JSCompartment::fixupGlobal()
{
    GlobalObject *global = *global_.unsafeGet();
    if (global)
        global_.set(MaybeForwarded(global));
}

#endif 

void
JSCompartment::purge()
{
    dtoaCache.purge();
}

void
JSCompartment::clearTables()
{
    global_.set(nullptr);

    
    
    
    JS_ASSERT(crossCompartmentWrappers.empty());
    JS_ASSERT_IF(callsiteClones.initialized(), callsiteClones.empty());
    JS_ASSERT(!jitCompartment_);
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
JSCompartment::updateJITForDebugMode(JSContext *maybecx, AutoDebugModeInvalidation &invalidate)
{
    
    
    
    
    return jit::UpdateForDebugMode(maybecx, this, invalidate);
}

bool
JSCompartment::enterDebugMode(JSContext *cx)
{
    AutoDebugModeInvalidation invalidate(this);
    return enterDebugMode(cx, invalidate);
}

bool
JSCompartment::enterDebugMode(JSContext *cx, AutoDebugModeInvalidation &invalidate)
{
    if (!debugMode()) {
        debugModeBits |= DebugMode;
        if (!updateJITForDebugMode(cx, invalidate))
            return false;
    }
    return true;
}

bool
JSCompartment::leaveDebugMode(JSContext *cx)
{
    AutoDebugModeInvalidation invalidate(this);
    return leaveDebugMode(cx, invalidate);
}

bool
JSCompartment::leaveDebugMode(JSContext *cx, AutoDebugModeInvalidation &invalidate)
{
    if (debugMode()) {
        leaveDebugModeUnderGC();
        if (!updateJITForDebugMode(cx, invalidate))
            return false;
    }
    return true;
}

void
JSCompartment::leaveDebugModeUnderGC()
{
    if (debugMode()) {
        debugModeBits &= ~DebugMode;
        DebugScopes::onCompartmentLeaveDebugMode(this);
    }
}

void
JSCompartment::clearBreakpointsIn(FreeOp *fop, js::Debugger *dbg, HandleObject handler)
{
    for (gc::ZoneCellIter i(zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->compartment() == this && script->hasAnyBreakpointsOrStepMode())
            script->clearBreakpointsIn(fop, dbg, handler);
    }
}

void
JSCompartment::addSizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf,
                                      size_t *tiAllocationSiteTables,
                                      size_t *tiArrayTypeTables,
                                      size_t *tiObjectTypeTables,
                                      size_t *compartmentObject,
                                      size_t *compartmentTables,
                                      size_t *crossCompartmentWrappersArg,
                                      size_t *regexpCompartment,
                                      size_t *savedStacksSet)
{
    *compartmentObject += mallocSizeOf(this);
    types.addSizeOfExcludingThis(mallocSizeOf, tiAllocationSiteTables,
                                 tiArrayTypeTables, tiObjectTypeTables);
    *compartmentTables += baseShapes.sizeOfExcludingThis(mallocSizeOf)
                        + initialShapes.sizeOfExcludingThis(mallocSizeOf)
                        + newTypeObjects.sizeOfExcludingThis(mallocSizeOf)
                        + lazyTypeObjects.sizeOfExcludingThis(mallocSizeOf);
    *crossCompartmentWrappersArg += crossCompartmentWrappers.sizeOfExcludingThis(mallocSizeOf);
    *regexpCompartment += regExps.sizeOfExcludingThis(mallocSizeOf);
    *savedStacksSet += savedStacks_.sizeOfExcludingThis(mallocSizeOf);
}

void
JSCompartment::adoptWorkerAllocator(Allocator *workerAllocator)
{
    zone()->allocator.arenas.adoptArenas(runtimeFromMainThread(), &workerAllocator->arenas);
}
