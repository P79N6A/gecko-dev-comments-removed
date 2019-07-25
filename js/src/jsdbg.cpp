








































#include "jsdbg.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jswrapper.h"
#include "jsobjinlines.h"

using namespace js;

static bool
NotImplemented(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "API");
    return false;
}

bool
ReportMoreArgsNeeded(JSContext *cx, const char *name, uintN required)
{
    JS_ASSERT(required < 10);
    char s[2];
    s[0] = '0' + required;
    s[1] = '\0';
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                         name, s, required == 1 ? "" : "s");
    return false;
}

#define REQUIRE_ARGC(name, n) \
    JS_BEGIN_MACRO \
        if (argc < n) \
            return ReportMoreArgsNeeded(cx, name, n); \
    JS_END_MACRO

bool
ReportObjectRequired(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
    return false;
}

JSObject *
CheckThisClass(JSContext *cx, Value *vp, Class *clasp, const char *fnname)
{
    if (!vp[1].isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &vp[1].toObject();
    if (thisobj->getClass() != clasp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             clasp->name, fnname, thisobj->getClass()->name);
        return NULL;
    }

    
    
    if ((clasp->flags & JSCLASS_HAS_PRIVATE) && !thisobj->getPrivate()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             clasp->name, fnname, "prototype object");
        return NULL;
    }
    return thisobj;
}

#define THISOBJ(cx, vp, classname, fnname, thisobj, private)                 \
    JSObject *thisobj =                                                      \
        CheckThisClass(cx, vp, &js::classname::jsclass, fnname);             \
    if (!thisobj)                                                            \
        return false;                                                        \
    js::classname *private = (classname *) thisobj->getPrivate();



Debug::Debug(JSObject *dbg, JSObject *hooks, JSCompartment *compartment)
  : object(dbg), debuggeeCompartment(compartment), hooksObject(hooks),
    enabled(true), hasDebuggerHandler(false)
{
}

JSTrapStatus
Debug::fireUncaughtExceptionHook(JSContext *cx)
{
    
    JS_ReportPendingException(cx);
    JS_ClearPendingException(cx);
    return JSTRAP_ERROR;
}

JSTrapStatus
Debug::parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp)
{
    vp->setUndefined();
    if (!ok)
        return fireUncaughtExceptionHook(ac.context);
    if (rv.isUndefined())
        return JSTRAP_CONTINUE;
    if (rv.isNull())
        return JSTRAP_ERROR;

    
    JSContext *cx = ac.context;
    JSObject *obj;
    const Shape *shape;
    jsid returnId = ATOM_TO_JSID(cx->runtime->atomState.returnAtom);
    jsid throwId = ATOM_TO_JSID(cx->runtime->atomState.throwAtom);
    if (!rv.isObject() ||
        !(obj = &rv.toObject())->isObject() ||
        !(shape = obj->lastProperty())->previous() ||
        shape->previous()->previous() ||
        (shape->id != returnId && shape->id != throwId) ||
        !shape->isDataDescriptor())
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_BAD_RESUMPTION);
        return fireUncaughtExceptionHook(cx);
    }

    if (!js_NativeGet(cx, obj, obj, shape, 0, vp))
        return fireUncaughtExceptionHook(cx);

    
    
    if (vp->isObject()) {
        vp->setUndefined();
        NotImplemented(cx);
        return fireUncaughtExceptionHook(cx);
    }

    ac.leave();
    if (!ac.origin->wrap(cx, vp)) {
        
        
        vp->setUndefined();
        cx->clearPendingException();
        return JSTRAP_ERROR;
    }
    return shape->id == returnId ? JSTRAP_RETURN : JSTRAP_THROW;
}

bool
CallMethodIfPresent(JSContext *cx, JSObject *obj, const char *name, int argc, Value *argv,
                    Value *rval)
{
    rval->setUndefined();
    JSAtom *atom = js_Atomize(cx, name, strlen(name), 0);
    Value fval;
    return atom &&
           js_GetMethod(cx, obj, ATOM_TO_JSID(atom), JSGET_NO_METHOD_BARRIER, &fval) &&
           (!js_IsCallable(fval) ||
            ExternalInvoke(cx, ObjectValue(*obj), fval, argc, argv, rval));
}

JSTrapStatus
Debug::onDebuggerStatement(JSContext *cx, Value *vp)
{
    if (!hasDebuggerHandler)
        return JSTRAP_CONTINUE;

    AutoCompartment ac(cx, hooksObject);
    if (!ac.enter())
        return JSTRAP_ERROR;

    
    Value rv;
    bool ok = CallMethodIfPresent(cx, hooksObject, "debuggerHandler", 0, NULL, &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

JSTrapStatus
JSCompartment::dispatchDebuggerStatement(JSContext *cx, js::Value *vp)
{
    
    
    
    
    
    AutoValueVector triggered(cx);
    for (Debug **p = debuggers.begin(); p != debuggers.end(); p++) {
        Debug *dbg = *p;
        if (dbg->observesDebuggerStatement()) {
            if (!triggered.append(ObjectValue(*dbg->toJSObject())))
                return JSTRAP_ERROR;
        }
    }

    
    
    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debug *dbg = Debug::fromJSObject(&p->toObject());
        if (dbg->observesCompartment(this) && dbg->observesDebuggerStatement()) {
            JSTrapStatus st = dbg->onDebuggerStatement(cx, vp);
            if (st != JSTRAP_CONTINUE)
                return st;
        }
    }
    return JSTRAP_CONTINUE;
}




bool
Debug::mark(GCMarker *trc, JSCompartment *comp, JSGCInvocationKind gckind)
{
    
    
    bool markedAny = false;
    JSRuntime *rt = trc->context->runtime;
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++) {
        JSCompartment *dc = *c;

        
        
        
        
        if (comp ? dc != comp : !dc->isAboutToBeCollected(gckind)) {
            const JSCompartment::DebugVector &debuggers = dc->getDebuggers();
            for (Debug **p = debuggers.begin(); p != debuggers.end(); p++) {
                Debug *dbg = *p;
                JSObject *obj = dbg->toJSObject();

                
                
                if ((!comp || obj->compartment() == comp) &&
                    !obj->isMarked() &&
                    dbg->hasAnyLiveHooks())
                {
                    
                    
                    MarkObject(trc, *obj, "enabled Debug");
                    markedAny = true;
                }
            }
        }
    }
    return markedAny;
}

void
Debug::trace(JSTracer *trc, JSObject *obj)
{
    if (Debug *dbg = (Debug *) obj->getPrivate()) {
        if (dbg->hooksObject)
            MarkObject(trc, *dbg->hooksObject, "hooks");
    }
}

void
JSCompartment::removeDebug(Debug *dbg)
{
    for (Debug **p = debuggers.begin(); p != debuggers.end(); p++) {
        if (*p == dbg) {
            debuggers.erase(p);
            return;
        }
    }
    JS_NOT_REACHED("JSCompartment::removeDebug");
}

void
Debug::finalize(JSContext *cx, JSObject *obj)
{
    Debug *dbg = (Debug *) obj->getPrivate();
    if (dbg && dbg->debuggeeCompartment) {
        dbg->debuggeeCompartment->removeDebug(dbg);
        dbg->debuggeeCompartment = NULL;
    }
}

Class Debug::jsclass = {
    "Debug", JSCLASS_HAS_PRIVATE,
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub, Debug::finalize,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    Debug::trace
};

JSBool
Debug::getHooks(JSContext *cx, uintN argc, Value *vp)
{
    THISOBJ(cx, vp, Debug, "get hooks", thisobj, dbg);
    vp->setObjectOrNull(dbg->hooksObject);
    return true;
}

JSBool
Debug::setHooks(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.set hooks", 1);
    THISOBJ(cx, vp, Debug, "set hooks", thisobj, dbg);
    if (!vp[2].isObject())
        return ReportObjectRequired(cx);
    JSObject *hooksobj = &vp[2].toObject();

    JSBool found;
    if (!JS_HasProperty(cx, hooksobj, "debuggerHandler", &found))
        return false;
    dbg->hasDebuggerHandler = !!found;

    dbg->hooksObject = hooksobj;
    vp->setUndefined();
    return true;
}

JSBool
Debug::getEnabled(JSContext *cx, uintN argc, Value *vp)
{
    THISOBJ(cx, vp, Debug, "get enabled", thisobj, dbg);
    vp->setBoolean(dbg->enabled);
    return true;
}

JSBool
Debug::setEnabled(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.set enabled", 1);
    THISOBJ(cx, vp, Debug, "set enabled", thisobj, dbg);
    dbg->enabled = js_ValueToBoolean(vp[2]);
    vp->setUndefined();
    return true;
}

JSBool
Debug::construct(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug", 1);

    
    const Value &arg = vp[2];
    if (!arg.isObject())
        return ReportObjectRequired(cx);
    JSObject *argobj = &arg.toObject();
    if (!argobj->isWrapper() ||
        (!argobj->getWrapperHandler()->flags() & JSWrapper::CROSS_COMPARTMENT))
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CCW_REQUIRED, "Debug");
        return false;
    }

    
    Value v;
    jsid prototypeId = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    if (!vp[0].toObject().getProperty(cx, prototypeId, &v))
        return false;
    JSObject *proto = &v.toObject();
    JS_ASSERT(proto->getClass() == &Debug::jsclass);

    
    JSObject *obj = NewNonFunction<WithProto::Given>(cx, &Debug::jsclass, proto, NULL);
    if (!obj)
        return false;
    JSObject *hooks = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!hooks)
        return false;
    JSCompartment *debuggeeCompartment = argobj->getProxyPrivate().toObject().compartment();
    Debug *dbg = cx->new_<Debug>(obj, hooks, debuggeeCompartment);
    if (!dbg)
        return false;
    if (!debuggeeCompartment->addDebug(dbg)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    obj->setPrivate(dbg);

    vp->setObject(*obj);
    return true;
}

JSPropertySpec Debug::properties[] = {
    JS_PSGS("hooks", Debug::getHooks, Debug::setHooks, 0),
    JS_PSGS("enabled", Debug::getEnabled, Debug::setEnabled, 0),
    JS_PS_END
};
    

extern JS_PUBLIC_API(JSBool)
JS_DefineDebugObject(JSContext *cx, JSObject *obj)
{
    JSObject *objProto;
    if (!js_GetClassPrototype(cx, obj, JSProto_Object, &objProto))
        return NULL;

    return !!js_InitClass(cx, obj, objProto, &Debug::jsclass, Debug::construct, 1,
                          Debug::properties, NULL, NULL, NULL);
}
