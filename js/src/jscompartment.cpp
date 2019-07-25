







































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsproxy.h"
#include "jsscope.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/PolyIC.h"
#include "methodjit/MonoIC.h"

#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

JSCompartment::JSCompartment(JSRuntime *rt)
  : rt(rt), principals(NULL), data(NULL), marked(false), active(false), debugMode(rt->debugMode),
    anynameObject(NULL), functionNamespaceObject(NULL)
{
    JS_INIT_CLIST(&scripts);
}

JSCompartment::~JSCompartment()
{
#ifdef JS_METHODJIT
    delete jaegerCompartment;
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
#ifdef JS_GCMETER
    memset(&compartmentStats, 0, sizeof(JSGCArenaStats) * FINALIZE_LIMIT);
#endif
    if (!crossCompartmentWrappers.init())
        return false;

#ifdef JS_METHODJIT
    if (!(jaegerCompartment = new mjit::JaegerCompartment))
        return false;
    return jaegerCompartment->Initialize();
#else
    return true;
#endif
}

bool
JSCompartment::arenaListsAreEmpty()
{
  for (unsigned i = 0; i < FINALIZE_LIMIT; i++) {
       if (!arenas[i].isEmpty())
           return false;
  }
  return true;
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

        
        if (JSString::isStatic(str))
            return true;

        
        if (str->asCell()->compartment() == this)
            return true;

        
        if (str->isAtomized()) {
            JS_ASSERT(str->asCell()->compartment() == cx->runtime->defaultCompartment);
            return true;
        }
    }

    







    JSObject *global;
    if (cx->hasfp()) {
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

            if (cx->runtime->preWrapObjectCallback)
                obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);
            if (!obj)
                return false;

            vp->setObject(*obj);
            if (obj->getCompartment() == this)
                return true;
        } else {
            if (cx->runtime->preWrapObjectCallback)
                obj = cx->runtime->preWrapObjectCallback(cx, global, obj, flags);

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
        if (vp->isObject())
            vp->toObject().setParent(global);
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

bool
JSCompartment::wrapException(JSContext *cx)
{
    JS_ASSERT(cx->compartment == this);

    if (cx->throwing) {
        AutoValueRooter tvr(cx, cx->exception);
        cx->throwing = false;
        cx->exception.setNull();
        if (wrap(cx, tvr.addr())) {
            cx->throwing = true;
            cx->exception = tvr.value();
        }
        return false;
    }
    return true;
}





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

void
JSCompartment::sweep(JSContext *cx, uint32 releaseInterval)
{
    chunk = NULL;
    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        JS_ASSERT_IF(IsAboutToBeFinalized(e.front().key.toGCThing()) &&
                     !IsAboutToBeFinalized(e.front().value.toGCThing()),
                     e.front().key.isString());
        if (IsAboutToBeFinalized(e.front().key.toGCThing()) ||
            IsAboutToBeFinalized(e.front().value.toGCThing())) {
            e.removeFront();
        }
    }

#if defined JS_METHODJIT && defined JS_MONOIC

    






    uint32 counter = 1;
    bool discardScripts = !active && releaseInterval != 0;

    for (JSCList *cursor = scripts.next; cursor != &scripts; cursor = cursor->next) {
        JSScript *script = reinterpret_cast<JSScript *>(cursor);
        if (script->hasJITCode()) {
            mjit::ic::SweepCallICs(script, discardScripts);
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
