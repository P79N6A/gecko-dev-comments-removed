







































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsiter.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "jstracer.h"
#include "jswrapper.h"
#include "assembler/wtf/Platform.h"
#include "yarr/BumpPointerAllocator.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/PolyIC.h"
#include "methodjit/MonoIC.h"

#include "jsgcinlines.h"
#include "jsscopeinlines.h"

#if ENABLE_YARR_JIT
#include "assembler/jit/ExecutableAllocator.h"
#endif

using namespace js;
using namespace js::gc;

JSCompartment::JSCompartment(JSRuntime *rt)
  : rt(rt),
    principals(NULL),
    gcBytes(0),
    gcTriggerBytes(0),
    gcLastBytes(0),
    hold(false),
    data(NULL),
    active(false),
#ifdef JS_METHODJIT
    jaegerCompartment(NULL),
#endif
#if ENABLE_YARR_JIT
    regExpAllocator(NULL),
#endif
    propertyTree(thisForCtor()),
    emptyArgumentsShape(NULL),
    emptyBlockShape(NULL),
    emptyCallShape(NULL),
    emptyDeclEnvShape(NULL),
    emptyEnumeratorShape(NULL),
    emptyWithShape(NULL),
    initialRegExpShape(NULL),
    initialStringShape(NULL),
    debugMode(rt->debugMode),
    mathCache(NULL)
{
    JS_INIT_CLIST(&scripts);

    PodArrayZero(scriptsToGC);
}

JSCompartment::~JSCompartment()
{
#if ENABLE_YARR_JIT
    Foreground::delete_(regExpAllocator);
#endif

#ifdef JS_METHODJIT
    Foreground::delete_(jaegerCompartment);
#endif

    Foreground::delete_(mathCache);

#ifdef DEBUG
    for (size_t i = 0; i != JS_ARRAY_LENGTH(scriptsToGC); ++i)
        JS_ASSERT(!scriptsToGC[i]);
#endif
}

bool
JSCompartment::init()
{
    chunk = NULL;
    for (unsigned i = 0; i < FINALIZE_LIMIT; i++)
        arenas[i].init();
    for (unsigned i = 0; i < FINALIZE_LIMIT; i++)
        freeLists.finalizables[i] = NULL;
    if (!crossCompartmentWrappers.init())
        return false;

#ifdef DEBUG
    if (rt->meterEmptyShapes()) {
        if (!emptyShapes.init())
            return false;
    }
#endif

#ifdef JS_TRACER
    if (!traceMonitor.init(rt))
        return false;
#endif

    regExpAllocator = rt->new_<WTF::BumpPointerAllocator>();
    if (!regExpAllocator)
        return false;

    if (!backEdgeTable.init())
        return false;

#ifdef JS_METHODJIT
    jaegerCompartment = rt->new_<mjit::JaegerCompartment>();
    if (!jaegerCompartment || !jaegerCompartment->Initialize())
        return false;
#endif
        
    return true;
}

#ifdef JS_METHODJIT
size_t
JSCompartment::getMjitCodeSize() const
{
    return jaegerCompartment->execAlloc()->getCodeSize();
}
#endif

bool
JSCompartment::arenaListsAreEmpty()
{
  for (unsigned i = 0; i < FINALIZE_LIMIT; i++) {
       if (!arenas[i].isEmpty())
           return false;
  }
  return true;
}

static bool
IsCrossCompartmentWrapper(JSObject *wrapper)
{
    return wrapper->isWrapper() &&
           !!(JSWrapper::wrapperHandler(wrapper)->flags() & JSWrapper::CROSS_COMPARTMENT);
}

bool
JSCompartment::wrap(JSContext *cx, Value *vp)
{
    JS_ASSERT(cx->compartment == this);

    uintN flags = 0;

    JS_CHECK_RECURSION(cx, return false);

    
    if (!vp->isMarkable())
        return true;

    if (vp->isString()) {
        JSString *str = vp->toString();

        
        if (str->isStaticAtom())
            return true;

        
        if (str->compartment() == this)
            return true;

        
        if (str->isAtom()) {
            JS_ASSERT(str->compartment() == cx->runtime->atomsCompartment);
            return true;
        }
    }

    







    JSObject *global;
    if (cx->running()) {
        global = cx->fp()->scopeChain().getGlobal();
    } else {
        global = cx->globalObject;
        OBJ_TO_INNER_OBJECT(cx, global);
        if (!global)
            return false;
    }

    
    if (vp->isObject()) {
        JSObject *obj = &vp->toObject();

        
        if (obj->compartment() == this)
            return true;

        
        if (obj->getClass() == &js_StopIterationClass)
            return js_FindClassObject(cx, NULL, JSProto_StopIteration, vp);

        
        if (!obj->getClass()->ext.innerObject) {
            obj = vp->toObject().unwrap(&flags);
            vp->setObject(*obj);
            if (obj->getCompartment() == this)
                return true;

            if (cx->runtime->preWrapObjectCallback) {
                obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);
                if (!obj)
                    return false;
            }

            vp->setObject(*obj);
            if (obj->getCompartment() == this)
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
            JS_ASSERT(IsCrossCompartmentWrapper(obj));
            if (obj->getParent() != global) {
                do {
                    obj->setParent(global);
                    obj = obj->getProto();
                } while (obj && IsCrossCompartmentWrapper(obj));
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

    wrapper->setProto(proto);
    if (!crossCompartmentWrappers.put(wrapper->getProxyPrivate(), *vp))
        return false;

    wrapper->setParent(global);
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
    jsint length = props.length();
    for (size_t n = 0; n < size_t(length); ++n) {
        if (!wrapId(cx, &vector[n]))
            return false;
    }
    return true;
}

#if defined JS_METHODJIT && defined JS_MONOIC




static inline bool
ScriptPoolDestroyed(JSContext *cx, mjit::JITScript *jit,
                    uint32 releaseInterval, uint32 &counter)
{
    JSC::ExecutablePool *pool = jit->code.m_executablePool;
    if (pool->m_gcNumber != cx->runtime->gcNumber) {
        




        pool->m_destroy = false;
        pool->m_gcNumber = cx->runtime->gcNumber;
        if (--counter == 0) {
            pool->m_destroy = true;
            counter = releaseInterval;
        }
    }
    return pool->m_destroy;
}
#endif






void
JSCompartment::markCrossCompartmentWrappers(JSTracer *trc)
{
    JS_ASSERT(trc->context->runtime->gcCurrentCompartment);

    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront())
        MarkValue(trc, e.front().key, "cross-compartment wrapper");
}

void
JSCompartment::sweep(JSContext *cx, uint32 releaseInterval)
{
    chunk = NULL;

    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        JS_ASSERT_IF(IsAboutToBeFinalized(cx, e.front().key.toGCThing()) &&
                     !IsAboutToBeFinalized(cx, e.front().value.toGCThing()),
                     e.front().key.isString());
        if (IsAboutToBeFinalized(cx, e.front().key.toGCThing()) ||
            IsAboutToBeFinalized(cx, e.front().value.toGCThing())) {
            e.removeFront();
        }
    }

    
    if (emptyArgumentsShape && IsAboutToBeFinalized(cx, emptyArgumentsShape))
        emptyArgumentsShape = NULL;
    if (emptyBlockShape && IsAboutToBeFinalized(cx, emptyBlockShape))
        emptyBlockShape = NULL;
    if (emptyCallShape && IsAboutToBeFinalized(cx, emptyCallShape))
        emptyCallShape = NULL;
    if (emptyDeclEnvShape && IsAboutToBeFinalized(cx, emptyDeclEnvShape))
        emptyDeclEnvShape = NULL;
    if (emptyEnumeratorShape && IsAboutToBeFinalized(cx, emptyEnumeratorShape))
        emptyEnumeratorShape = NULL;
    if (emptyWithShape && IsAboutToBeFinalized(cx, emptyWithShape))
        emptyWithShape = NULL;

    if (initialRegExpShape && IsAboutToBeFinalized(cx, initialRegExpShape))
        initialRegExpShape = NULL;
    if (initialStringShape && IsAboutToBeFinalized(cx, initialStringShape))
        initialStringShape = NULL;

#ifdef JS_TRACER
    traceMonitor.sweep(cx);
#endif

#if defined JS_METHODJIT && defined JS_MONOIC

    






    uint32 counter = 1;
    bool discardScripts = !active && releaseInterval != 0;

    for (JSCList *cursor = scripts.next; cursor != &scripts; cursor = cursor->next) {
        JSScript *script = reinterpret_cast<JSScript *>(cursor);
        if (script->hasJITCode()) {
            mjit::ic::SweepCallICs(cx, script, discardScripts);
            if (discardScripts) {
                if (script->jitNormal &&
                    ScriptPoolDestroyed(cx, script->jitNormal, releaseInterval, counter)) {
                    mjit::ReleaseScriptCode(cx, script);
                    continue;
                }
                if (script->jitCtor &&
                    ScriptPoolDestroyed(cx, script->jitCtor, releaseInterval, counter)) {
                    mjit::ReleaseScriptCode(cx, script);
                }
            }
        }
    }

#endif 

    active = false;
}

void
JSCompartment::purge(JSContext *cx)
{
    freeLists.purge();
    dtoaCache.purge();

    
    js_DestroyScriptsToGC(cx, this);

    nativeIterCache.purge();
    toSourceCache.destroyIfConstructed();

#ifdef JS_TRACER
    



    if (cx->runtime->gcRegenShapes)
        traceMonitor.needFlush = JS_TRUE;
#endif

#ifdef JS_METHODJIT
    for (JSScript *script = (JSScript *)scripts.next;
         &script->links != &scripts;
         script = (JSScript *)script->links.next) {
        if (script->hasJITCode()) {
# if defined JS_POLYIC
            mjit::ic::PurgePICs(cx, script);
# endif
# if defined JS_MONOIC
            



            if (cx->runtime->gcRegenShapes)
                mjit::ic::PurgeMICs(cx, script);
# endif
        }
    }
#endif
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

size_t
JSCompartment::backEdgeCount(jsbytecode *pc) const
{
    if (BackEdgeMap::Ptr p = backEdgeTable.lookup(pc))
        return p->value;

    return 0;
}

size_t
JSCompartment::incBackEdgeCount(jsbytecode *pc)
{
    if (BackEdgeMap::Ptr p = backEdgeTable.lookupWithDefault(pc, 0))
        return ++p->value;
    return 1;  
}

