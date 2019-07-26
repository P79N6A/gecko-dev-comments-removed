






#include "jscntxt.h"
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
#include "ion/IonCompartment.h"
#include "ion/Ion.h"

#if ENABLE_YARR_JIT
#include "assembler/jit/ExecutableAllocator.h"
#endif

using namespace mozilla;
using namespace js;
using namespace js::gc;

JSCompartment::JSCompartment(JSRuntime *rt)
  : rt(rt),
    principals(NULL),
    global_(NULL),
    needsBarrier_(false),
    gcState(NoGCScheduled),
    gcPreserveCode(false),
    gcBytes(0),
    gcTriggerBytes(0),
    gcHeapGrowthFactor(3.0),
    hold(false),
    isSystemCompartment(false),
    lastCodeRelease(0),
    typeLifoAlloc(TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    data(NULL),
    active(false),
    lastAnimationTime(0),
    regExps(rt),
    propertyTree(thisForCtor()),
    emptyTypeObject(NULL),
    gcMallocAndFreeBytes(0),
    gcTriggerMallocAndFreeBytes(0),
    gcMallocBytes(0),
    debugModeBits(rt->debugMode ? DebugFromC : 0),
    watchpointMap(NULL),
    scriptCountsMap(NULL),
    sourceMapMap(NULL),
    debugScriptMap(NULL)
#ifdef JS_ION
    , ionCompartment_(NULL)
#endif
{
    setGCMaxMallocBytes(rt->gcMaxMallocBytes * 0.9);
}

JSCompartment::~JSCompartment()
{

#ifdef JS_ION
    Foreground::delete_(ionCompartment_);
#endif

    Foreground::delete_(watchpointMap);
    Foreground::delete_(scriptCountsMap);
    Foreground::delete_(sourceMapMap);
    Foreground::delete_(debugScriptMap);
}

bool
JSCompartment::init(JSContext *cx)
{
    activeAnalysis = activeInference = false;
    types.init(cx);

    if (!crossCompartmentWrappers.init())
        return false;

    if (!regExps.init(cx))
        return false;

    return debuggees.init();
}

void
JSCompartment::setNeedsBarrier(bool needs)
{
#ifdef JS_METHODJIT
    if (needsBarrier_ != needs)
        mjit::ClearAllFrames(this);
#endif

#ifdef JS_ION
    if (needsBarrier_ != needs)
        ion::ToggleBarriers(this, needs);
#endif

    needsBarrier_ = needs;
}

#ifdef JS_ION
bool
JSCompartment::ensureIonCompartmentExists(JSContext *cx)
{
    using namespace js::ion;
    if (ionCompartment_)
        return true;

    
    ionCompartment_ = cx->new_<IonCompartment>();

    if (!ionCompartment_ || !ionCompartment_->initialize(cx)) {
        if (ionCompartment_)
            delete ionCompartment_;
        ionCompartment_ = NULL;
        return false;
    }

    return true;
}
#endif

static bool
WrapForSameCompartment(JSContext *cx, JSObject *obj, Value *vp)
{
    JS_ASSERT(cx->compartment == obj->compartment());
    if (cx->runtime->sameCompartmentWrapObjectCallback) {
        obj = cx->runtime->sameCompartmentWrapObjectCallback(cx, obj);
        if (!obj)
            return false;
    }
    vp->setObject(*obj);
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, Value *vp)
{
    JS_ASSERT(cx->compartment == this);

    unsigned flags = 0;

    JS_CHECK_RECURSION(cx, return false);

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

    






    RootedObject global(cx);
    if (cx->hasfp()) {
        global = &cx->fp()->global();
    } else {
        global = JS_ObjectToInnerObject(cx, cx->globalObject);
        if (!global)
            return false;
    }

    
    if (vp->isObject()) {
        Rooted<JSObject*> obj(cx, &vp->toObject());

        if (obj->compartment() == this)
            return WrapForSameCompartment(cx, obj, vp);

        
        if (obj->isStopIteration()) {
            RootedValue vvp(cx, *vp);
            bool result = js_FindClassObject(cx, NullPtr(), JSProto_StopIteration, &vvp);
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
            if (global->getClass() != &dummy_class && obj->getParent() != global) {
                do {
                    if (!JSObject::setParent(cx, obj, global))
                        return false;
                    obj = obj->getProto();
                } while (obj && obj->isCrossCompartmentWrapper());
            }
        }
        return true;
    }

    if (vp->isString()) {
        RootedValue orig(cx, *vp);
        JSString *str = vp->toString();
        const jschar *chars = str->getChars(cx);
        if (!chars)
            return false;
        JSString *wrapped = js_NewStringCopyN(cx, chars, str->length());
        if (!wrapped)
            return false;
        vp->setString(wrapped);
        return crossCompartmentWrappers.put(orig, *vp);
    }

    RootedObject obj(cx, &vp->toObject());

    









    RootedObject proto(cx, obj->getProto());
    if (!wrap(cx, proto.address()))
        return false;

    




    RootedObject wrapper(cx, cx->runtime->wrapObjectCallback(cx, obj, proto, global, flags));
    if (!wrapper)
        return false;

    
    
    JS_ASSERT(Wrapper::wrappedObject(wrapper) == &key.get().toObject());

    vp->setObject(*wrapper);

    if (wrapper->getProto() != proto && !SetProto(cx, wrapper, proto, false))
        return false;

    if (!crossCompartmentWrappers.put(key, *vp))
        return false;

    if (!JSObject::setParent(cx, wrapper, global))
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
JSCompartment::wrap(JSContext *cx, JSObject **objp)
{
    if (!*objp)
        return true;
    RootedValue value(cx, ObjectValue(**objp));
    if (!wrap(cx, value.address()))
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
    return ValueToId(cx, value.get(), idp);
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
JSCompartment::discardJitCode(FreeOp *fop)
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
    }

#endif 
}

void
JSCompartment::sweep(FreeOp *fop, bool releaseTypes)
{
    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_DISCARD_CODE);
        discardJitCode(fop);
    }

    
    sweepCrossCompartmentWrappers();

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES);

        

        sweepBaseShapeTable();
        sweepInitialShapeTable();
        sweepNewTypeObjectTable(newTypeObjects);
        sweepNewTypeObjectTable(lazyTypeObjects);

        if (emptyTypeObject && !IsTypeObjectMarked(emptyTypeObject.unsafeGet()))
            emptyTypeObject = NULL;

        sweepBreakpoints(fop);

        if (global_ && !IsObjectMarked(&global_))
            global_ = NULL;

#ifdef JS_ION
        if (ionCompartment_)
            ionCompartment_->sweep(fop);
#endif

        
        regExps.sweep(rt);
    }

    if (!activeAnalysis && !gcPreserveCode) {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DISCARD_ANALYSIS);

        



        LifoAlloc oldAlloc(typeLifoAlloc.defaultChunkSize());
        oldAlloc.steal(&typeLifoAlloc);

        



        if (active)
            releaseTypes = false;

        




        if (types.inferenceEnabled) {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_DISCARD_TI);

            for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
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
            }

            if (types.constrainedOutputs) {
                fop->delete_(types.constrainedOutputs);
                types.constrainedOutputs = NULL;
            }
        }

        {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_FREE_TI_ARENA);
            oldAlloc.freeAll();
        }
    }

    active = false;
}






void
JSCompartment::sweepCrossCompartmentWrappers()
{
    gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_SWEEP_TABLES);

    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        CrossCompartmentKey key = e.front().key;
        bool keyMarked = IsCellMarked(&key.wrapped);
        bool valMarked = IsValueMarked(e.front().value.unsafeGet());
        bool dbgMarked = !key.debugger || IsObjectMarked(&key.debugger);
        JS_ASSERT_IF(!keyMarked && valMarked, key.kind == CrossCompartmentKey::StringWrapper);
        if (!keyMarked || !valMarked || !dbgMarked)
            e.removeFront();
        else
            e.rekeyFront(key);
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
    for (AllFramesIter i(rt->stackSpace); !i.done(); ++i) {
        JSScript *script = i.fp()->maybeScript();
        if (script && script->compartment() == this)
            return true;
    }
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
            cx->runtime->debugScopes->onCompartmentLeaveDebugMode(this);
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
    bool wasEnabled = debugMode();
    if (!debuggees.put(global)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    debugModeBits |= DebugFromJS;
    if (!wasEnabled) {
        AutoDebugModeGC dmgc(cx->runtime);
        updateForDebugMode(cx->runtime->defaultFreeOp(), dmgc);
    }
    return true;
}

void
JSCompartment::removeDebuggee(FreeOp *fop,
                              js::GlobalObject *global,
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
            AutoDebugModeGC dmgc(rt);
            fop->runtime()->debugScopes->onCompartmentLeaveDebugMode(this);
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
    if (JS_CLIST_IS_EMPTY(&rt->debuggerList))
        return;

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (!script->hasAnyBreakpointsOrStepMode())
            continue;
        bool scriptGone = !IsScriptMarked(&script);
        JS_ASSERT(script == i.get<JSScript>());
        for (unsigned i = 0; i < script->length; i++) {
            BreakpointSite *site = script->getBreakpointSite(script->code + i);
            if (!site)
                continue;
            
            
            Breakpoint *nextbp;
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = nextbp) {
                nextbp = bp->nextInSite();
                if (scriptGone || !IsObjectMarked(&bp->debugger->toJSObjectRef()))
                    bp->destroy(fop);
            }
        }
    }
}

size_t
JSCompartment::sizeOfShapeTable(JSMallocSizeOfFun mallocSizeOf)
{
    return baseShapes.sizeOfExcludingThis(mallocSizeOf)
         + initialShapes.sizeOfExcludingThis(mallocSizeOf)
         + newTypeObjects.sizeOfExcludingThis(mallocSizeOf)
         + lazyTypeObjects.sizeOfExcludingThis(mallocSizeOf);
}
