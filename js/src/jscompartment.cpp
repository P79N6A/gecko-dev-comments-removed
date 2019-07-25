







































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsiter.h"
#include "jsmath.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#include "assembler/wtf/Platform.h"
#include "js/MemoryMetrics.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/PolyIC.h"
#include "methodjit/MonoIC.h"
#include "vm/Debugger.h"
#include "yarr/BumpPointerAllocator.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"

#if ENABLE_YARR_JIT
#include "assembler/jit/ExecutableAllocator.h"
#endif

using namespace mozilla;
using namespace js;
using namespace js::gc;

JSCompartment::JSCompartment(JSRuntime *rt)
  : rt(rt),
    principals(NULL),
    needsBarrier_(false),
    gcState(NoGCScheduled),
    gcBytes(0),
    gcTriggerBytes(0),
    hold(false),
    typeLifoAlloc(TYPE_LIFO_ALLOC_PRIMARY_CHUNK_SIZE),
    data(NULL),
    active(false),
#ifdef JS_METHODJIT
    jaegerCompartment_(NULL),
#endif
    regExps(rt),
    propertyTree(thisForCtor()),
    emptyTypeObject(NULL),
    gcMallocAndFreeBytes(0),
    gcTriggerMallocAndFreeBytes(0),
    gcMallocBytes(0),
    debugModeBits(rt->debugMode ? DebugFromC : 0),
    mathCache(NULL),
    watchpointMap(NULL)
{
    PodArrayZero(evalCache);
    setGCMaxMallocBytes(rt->gcMaxMallocBytes * 0.9);
}

JSCompartment::~JSCompartment()
{
    



    FreeScriptFilenames(this);

#ifdef JS_METHODJIT
    Foreground::delete_(jaegerCompartment_);
#endif

    Foreground::delete_(mathCache);
    Foreground::delete_(watchpointMap);

#ifdef DEBUG
    for (size_t i = 0; i < ArrayLength(evalCache); ++i)
        JS_ASSERT(!evalCache[i]);
#endif
}

bool
JSCompartment::init(JSContext *cx)
{
    activeAnalysis = activeInference = false;
    types.init(cx);

    newObjectCache.reset();

    if (!crossCompartmentWrappers.init())
        return false;

    if (!regExps.init(cx))
        return false;

    if (!scriptFilenameTable.init())
        return false;

    return debuggees.init();
}

#ifdef JS_METHODJIT
bool
JSCompartment::ensureJaegerCompartmentExists(JSContext *cx)
{
    if (jaegerCompartment_)
        return true;

    mjit::JaegerCompartment *jc = cx->new_<mjit::JaegerCompartment>();
    if (!jc)
        return false;
    if (!jc->Initialize(cx)) {
        cx->delete_(jc);
        return false;
    }
    jaegerCompartment_ = jc;
    return true;
}

size_t
JSCompartment::sizeOfMjitCode() const
{
    if (!jaegerCompartment_)
        return 0;

    size_t method, regexp, unused;
    jaegerCompartment_->execAlloc()->sizeOfCode(&method, &regexp, &unused);
    JS_ASSERT(regexp == 0);
    return method + unused;
}

#endif

bool
JSCompartment::wrap(JSContext *cx, Value *vp)
{
    JS_ASSERT(cx->compartment == this);

    unsigned flags = 0;

    JS_CHECK_RECURSION(cx, return false);

    
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

    






    JSObject *global;
    if (cx->hasfp()) {
        global = &cx->fp()->scopeChain().global();
    } else {
        global = JS_ObjectToInnerObject(cx, cx->globalObject);
        if (!global)
            return false;
    }

    
    if (vp->isObject()) {
        JSObject *obj = &vp->toObject();

        
        if (obj->compartment() == this)
            return true;

        
        if (obj->isStopIteration())
            return js_FindClassObject(cx, NULL, JSProto_StopIteration, vp);

        
        if (!obj->getClass()->ext.innerObject) {
            obj = UnwrapObject(&vp->toObject(), true, &flags);
            vp->setObject(*obj);
            if (obj->compartment() == this)
                return true;

            if (cx->runtime->preWrapObjectCallback) {
                obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);
                if (!obj)
                    return false;
            }

            vp->setObject(*obj);
            if (obj->compartment() == this)
                return true;
        } else {
            if (cx->runtime->preWrapObjectCallback) {
                obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);
                if (!obj)
                    return false;
            }

            JS_ASSERT(!obj->isWrapper() || obj->getClass()->ext.innerObject);
            vp->setObject(*obj);
        }

#ifdef DEBUG
        {
            JSObject *outer = obj;
            OBJ_TO_OUTER_OBJECT(cx, outer);
            JS_ASSERT(outer && outer == obj);
        }
#endif
    }

    
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(*vp)) {
        *vp = p->value;
        if (vp->isObject()) {
            JSObject *obj = &vp->toObject();
            JS_ASSERT(obj->isCrossCompartmentWrapper());
            if (global->getClass() != &dummy_class && obj->getParent() != global) {
                do {
                    if (!obj->setParent(cx, global))
                        return false;
                    obj = obj->getProto();
                } while (obj && obj->isCrossCompartmentWrapper());
            }
        }
        return true;
    }

    if (vp->isString()) {
        Value orig = *vp;
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

    JSObject *obj = &vp->toObject();

    









    JSObject *proto = obj->getProto();
    if (!wrap(cx, &proto))
        return false;

    




    JSObject *wrapper = cx->runtime->wrapObjectCallback(cx, obj, proto, global, flags);
    if (!wrapper)
        return false;

    vp->setObject(*wrapper);

    if (wrapper->getProto() != proto && !SetProto(cx, wrapper, proto, false))
        return false;

    if (!crossCompartmentWrappers.put(GetProxyPrivate(wrapper), *vp))
        return false;

    if (!wrapper->setParent(cx, global))
        return false;
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSString **strp)
{
    AutoValueRooter tvr(cx, StringValue(*strp));
    if (!wrap(cx, tvr.addr()))
        return false;
    *strp = tvr.value().toString();
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, HeapPtrString *strp)
{
    AutoValueRooter tvr(cx, StringValue(*strp));
    if (!wrap(cx, tvr.addr()))
        return false;
    *strp = tvr.value().toString();
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSObject **objp)
{
    if (!*objp)
        return true;
    AutoValueRooter tvr(cx, ObjectValue(**objp));
    if (!wrap(cx, tvr.addr()))
        return false;
    *objp = &tvr.value().toObject();
    return true;
}

bool
JSCompartment::wrapId(JSContext *cx, jsid *idp)
{
    if (JSID_IS_INT(*idp))
        return true;
    AutoValueRooter tvr(cx, IdToValue(*idp));
    if (!wrap(cx, tvr.addr()))
        return false;
    return ValueToId(cx, tvr.value(), idp);
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
        Value tmp = e.front().key;
        MarkValueRoot(trc, &tmp, "cross-compartment wrapper");
        JS_ASSERT(tmp == e.front().key);
    }
}

void
JSCompartment::markTypes(JSTracer *trc)
{
    




    JS_ASSERT(activeAnalysis);

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        MarkScriptRoot(trc, &script, "mark_types_script");
        JS_ASSERT(script == i.get<JSScript>());
    }

    for (size_t thingKind = FINALIZE_OBJECT0;
         thingKind < FINALIZE_OBJECT_LIMIT;
         thingKind++) {
        for (CellIterUnderGC i(this, AllocKind(thingKind)); !i.done(); i.next()) {
            JSObject *object = i.get<JSObject>();
            if (object->hasSingletonType()) {
                MarkObjectRoot(trc, &object, "mark_types_singleton");
                JS_ASSERT(object == i.get<JSObject>());
            }
        }
    }

    for (CellIterUnderGC i(this, FINALIZE_TYPE_OBJECT); !i.done(); i.next()) {
        types::TypeObject *type = i.get<types::TypeObject>();
        MarkTypeObjectRoot(trc, &type, "mark_types_scan");
        JS_ASSERT(type == i.get<types::TypeObject>());
    }
}

void
JSCompartment::discardJitCode(JSContext *cx)
{
    



#ifdef JS_METHODJIT
    mjit::ClearAllFrames(this);

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        mjit::ReleaseScriptCode(cx, script);

        




        script->resetUseCount();
    }
#endif
}

void
JSCompartment::sweep(JSContext *cx, bool releaseTypes)
{
    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        JS_ASSERT_IF(IsAboutToBeFinalized(e.front().key) &&
                     !IsAboutToBeFinalized(e.front().value),
                     e.front().key.isString());
        if (IsAboutToBeFinalized(e.front().key) ||
            IsAboutToBeFinalized(e.front().value)) {
            e.removeFront();
        }
    }

    

    regExps.sweep(rt);

    sweepBaseShapeTable(cx);
    sweepInitialShapeTable(cx);
    sweepNewTypeObjectTable(cx, newTypeObjects);
    sweepNewTypeObjectTable(cx, lazyTypeObjects);

    if (emptyTypeObject && IsAboutToBeFinalized(emptyTypeObject))
        emptyTypeObject = NULL;

    newObjectCache.reset();

    sweepBreakpoints(cx);

    {
        gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_DISCARD_CODE);
        discardJitCode(cx);
    }

    if (!activeAnalysis) {
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
                    types::TypeScript::Sweep(cx, script);

                    if (releaseTypes) {
                        script->types->destroy();
                        script->types = NULL;
                        script->typesPurged = true;
                    }
                }
            }
        }

        {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_SWEEP_TYPES);
            types.sweep(cx);
        }

        {
            gcstats::AutoPhase ap2(rt->gcStats, gcstats::PHASE_CLEAR_SCRIPT_ANALYSIS);
            for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
                script->clearAnalysis();
            }
        }
    }

    active = false;
}

void
JSCompartment::purge()
{
    dtoaCache.purge();

    





    for (size_t i = 0; i < ArrayLength(evalCache); ++i) {
        for (JSScript **listHeadp = &evalCache[i]; *listHeadp; ) {
            JSScript *script = *listHeadp;
            JS_ASSERT(GetGCThingTraceKind(script) == JSTRACE_SCRIPT);
            *listHeadp = NULL;
            listHeadp = &script->evalHashLink();
        }
    }

    nativeIterCache.purge();
    toSourceCache.destroyIfConstructed();
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


MathCache *
JSCompartment::allocMathCache(JSContext *cx)
{
    JS_ASSERT(!mathCache);
    mathCache = cx->new_<MathCache>();
    if (!mathCache)
        js_ReportOutOfMemory(cx);
    return mathCache;
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
JSCompartment::setDebugModeFromC(JSContext *cx, bool b)
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
    if (enabledBefore != enabledAfter)
        updateForDebugMode(cx);
    return true;
}

void
JSCompartment::updateForDebugMode(JSContext *cx)
{
    for (ContextIter acx(rt); !acx.done(); acx.next()) {
        if (acx->compartment == this) 
            acx->updateJITEnabled();
    }

#ifdef JS_METHODJIT
    bool enabled = debugMode();

    if (enabled)
        JS_ASSERT(!hasScriptsOnStack());
    else if (hasScriptsOnStack())
        return;

    



    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->debugMode != enabled) {
            mjit::ReleaseScriptCode(cx, script);
            script->clearAnalysis();
            script->debugMode = enabled;
        }
    }
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
    if (!wasEnabled)
        updateForDebugMode(cx);
    return true;
}

void
JSCompartment::removeDebuggee(JSContext *cx,
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
        if (wasEnabled && !debugMode())
            updateForDebugMode(cx);
    }
}

void
JSCompartment::clearBreakpointsIn(JSContext *cx, js::Debugger *dbg, JSObject *handler)
{
    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->hasAnyBreakpointsOrStepMode())
            script->clearBreakpointsIn(cx, dbg, handler);
    }
}

void
JSCompartment::clearTraps(JSContext *cx)
{
    for (gc::CellIter i(this, gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (script->hasAnyBreakpointsOrStepMode())
            script->clearTraps(cx);
    }
}

void
JSCompartment::sweepBreakpoints(JSContext *cx)
{
    if (JS_CLIST_IS_EMPTY(&cx->runtime->debuggerList))
        return;

    for (CellIterUnderGC i(this, FINALIZE_SCRIPT); !i.done(); i.next()) {
        JSScript *script = i.get<JSScript>();
        if (!script->hasAnyBreakpointsOrStepMode())
            continue;
        bool scriptGone = IsAboutToBeFinalized(script);
        for (unsigned i = 0; i < script->length; i++) {
            BreakpointSite *site = script->getBreakpointSite(script->code + i);
            if (!site)
                continue;
            
            
            Breakpoint *nextbp;
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = nextbp) {
                nextbp = bp->nextInSite();
                if (scriptGone || IsAboutToBeFinalized(bp->debugger->toJSObject()))
                    bp->destroy(cx);
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
