








































#include "jsdbg.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsgcmark.h"
#include "jsobj.h"
#include "jswrapper.h"
#include "jsobjinlines.h"

using namespace js;



extern Class Frame_class;

enum {
    JSSLOT_FRAME_OWNER,
    JSSLOT_FRAME_COUNT
};



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



enum {
    JSSLOT_DEBUG_FRAME_PROTO,
    JSSLOT_DEBUG_COUNT
};

Debug::Debug(JSObject *dbg, JSObject *hooks, JSCompartment *compartment)
  : object(dbg), debuggeeCompartment(compartment), hooksObject(hooks),
    uncaughtExceptionHook(NULL), enabled(true), hasDebuggerHandler(false)
{
}

bool
Debug::init()
{
    return frames.init();
}

bool
Debug::getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp)
{
    FrameMap::AddPtr p = frames.lookupForAdd(fp);
    if (!p) {
        JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_FRAME_PROTO).toObject();
        JSObject *frameobj = NewNonFunction<WithProto::Given>(cx, &Frame_class, proto, NULL);
        if (!frameobj || !frameobj->ensureClassReservedSlots(cx))
            return false;
        frameobj->setPrivate(fp);
        frameobj->setReservedSlot(JSSLOT_FRAME_OWNER, ObjectValue(*object));
        if (!frames.add(p, fp, frameobj)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }
    vp->setObject(*p->value);
    return true;
}

void
Debug::slowPathLeaveStackFrame(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    JSCompartment *compartment = cx->compartment;
    const JSCompartment::DebugVector &debuggers = compartment->getDebuggers();
    for (Debug **p = debuggers.begin(); p != debuggers.end(); p++) {
        Debug *dbg = *p;
        if (FrameMap::Ptr p = dbg->frames.lookup(fp)) {
            JSObject *frameobj = p->value;
            frameobj->setPrivate(NULL);
            dbg->frames.remove(p);
        }
    }
}

JSTrapStatus
Debug::handleUncaughtException(AutoCompartment &ac, Value *vp, bool callHook)
{
    JSContext *cx = ac.context;
    if (cx->isExceptionPending()) {
        if (callHook && uncaughtExceptionHook) {
            Value fval = ObjectValue(*uncaughtExceptionHook);
            Value exc = cx->getPendingException();
            Value rv;
            cx->clearPendingException();
            if (ExternalInvoke(cx, ObjectValue(*object), fval, 1, &exc, &rv))
                return parseResumptionValue(ac, true, rv, vp, false);
        }

        if (cx->isExceptionPending()) {
            JS_ReportPendingException(cx);
            cx->clearPendingException();
        }
    }
    return JSTRAP_ERROR;
}

JSTrapStatus
Debug::parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
                            bool callHook)
{
    vp->setUndefined();
    if (!ok)
        return handleUncaughtException(ac, vp, callHook);
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
        return handleUncaughtException(ac, vp, callHook);
    }

    if (!js_NativeGet(cx, obj, obj, shape, 0, vp))
        return handleUncaughtException(ac, vp, callHook);

    
    
    if (vp->isObject()) {
        vp->setUndefined();
        NotImplemented(cx);
        return handleUncaughtException(ac, vp, callHook);
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
Debug::handleDebuggerStatement(JSContext *cx, Value *vp)
{
    
    StackFrame *fp = cx->fp();

    JS_ASSERT(hasDebuggerHandler);
    AutoCompartment ac(cx, hooksObject);
    if (!ac.enter())
        return JSTRAP_ERROR;

    Value argv[1];
    if (!getScriptFrame(cx, fp, argv))
        return JSTRAP_ERROR;

    Value rv;
    bool ok = CallMethodIfPresent(cx, hooksObject, "debuggerHandler", 1, argv, &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

JSTrapStatus
Debug::dispatchDebuggerStatement(JSContext *cx, js::Value *vp)
{
    
    
    
    
    
    AutoValueVector triggered(cx);
    JSCompartment *compartment = cx->compartment;
    const JSCompartment::DebugVector &debuggers = compartment->getDebuggers();
    for (Debug **p = debuggers.begin(); p != debuggers.end(); p++) {
        Debug *dbg = *p;
        if (dbg->observesDebuggerStatement()) {
            if (!triggered.append(ObjectValue(*dbg->toJSObject())))
                return JSTRAP_ERROR;
        }
    }

    
    
    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debug *dbg = Debug::fromJSObject(&p->toObject());
        if (dbg->observesCompartment(compartment) && dbg->observesDebuggerStatement()) {
            JSTrapStatus st = dbg->handleDebuggerStatement(cx, vp);
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
        MarkObject(trc, *dbg->hooksObject, "hooks");
        if (dbg->uncaughtExceptionHook)
            MarkObject(trc, *dbg->uncaughtExceptionHook, "hooks");

        
        
        for (FrameMap::Enum e(dbg->frames); !e.empty(); e.popFront()) {
            if (e.front().value->getPrivate())
                MarkObject(trc, *obj, "live Debug.Frame");
        }
    }
}

void
Debug::sweepAll(JSRuntime *rt)
{
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
        sweepCompartment(*c);
}

void
Debug::sweepCompartment(JSCompartment *compartment)
{
    
    const JSCompartment::DebugVector &debuggers = compartment->getDebuggers();
    for (Debug **p = debuggers.begin(); p != debuggers.end(); p++) {
        Debug *dbg = *p;
        for (FrameMap::Enum e(dbg->frames); !e.empty(); e.popFront()) {
            if (!e.front().value->isMarked())
                e.removeFront();
        }
    }
}

void
Debug::finalize(JSContext *cx, JSObject *obj)
{
    Debug *dbg = (Debug *) obj->getPrivate();
    if (dbg && dbg->debuggeeCompartment)
        dbg->detachFrom(dbg->debuggeeCompartment);
}

void
Debug::detachFrom(JSCompartment *c)
{
    JS_ASSERT(c == debuggeeCompartment);
    c->removeDebug(this);
    debuggeeCompartment = NULL;
}

Class Debug::jsclass = {
    "Debug", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUG_COUNT),
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
    vp->setObject(*dbg->hooksObject);
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
Debug::getUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp)
{
    THISOBJ(cx, vp, Debug, "get uncaughtExceptionHook", thisobj, dbg);
    vp->setObjectOrNull(dbg->uncaughtExceptionHook);
    return true;
}

JSBool
Debug::setUncaughtExceptionHook(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.set uncaughtExceptionHook", 1);
    THISOBJ(cx, vp, Debug, "set uncaughtExceptionHook", thisobj, dbg);
    if (!vp[2].isNull() && (!vp[2].isObject() || !vp[2].toObject().isCallable())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_ASSIGN_FUNCTION_OR_NULL,
                             "uncaughtExceptionHook");
        return false;
    }

    dbg->uncaughtExceptionHook = vp[2].toObjectOrNull();
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

    
    JSCompartment *debuggeeCompartment = argobj->getProxyPrivate().toObject().compartment();
    if (!debuggeeCompartment->debugMode) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DEBUG_MODE);
        return false;
    }

    
    Value v;
    jsid prototypeId = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    if (!vp[0].toObject().getProperty(cx, prototypeId, &v))
        return false;
    JSObject *proto = &v.toObject();
    JS_ASSERT(proto->getClass() == &Debug::jsclass);

    
    
    JSObject *obj = NewNonFunction<WithProto::Given>(cx, &Debug::jsclass, proto, NULL);
    if (!obj || !obj->ensureClassReservedSlots(cx))
        return false;
    obj->setReservedSlot(JSSLOT_DEBUG_FRAME_PROTO,
                         proto->getReservedSlot(JSSLOT_DEBUG_FRAME_PROTO));
    JSObject *hooks = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!hooks)
        return false;
    Debug *dbg = cx->new_<Debug>(obj, hooks, debuggeeCompartment);
    if (!dbg)
        return false;
    if (!dbg->init() || !debuggeeCompartment->addDebug(dbg)) {
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
    JS_PSGS("uncaughtExceptionHook", Debug::getUncaughtExceptionHook,
            Debug::setUncaughtExceptionHook, 0),
    JS_PS_END
};



Class Frame_class = {
    "Frame", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_FRAME_COUNT),
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub, FinalizeStub,
};

JSObject *
CheckThisFrame(JSContext *cx, Value *vp, const char *fnname, bool checkLive)
{
    if (!vp[1].isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &vp[1].toObject();
    if (thisobj->getClass() != &Frame_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debug.Frame", fnname, thisobj->getClass()->name);
        return NULL;
    }

    
    
    if (!thisobj->getPrivate()) {
        if (thisobj->getReservedSlot(JSSLOT_FRAME_OWNER).isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                                 "Debug.Frame", fnname, "prototype object");
            return NULL;
        }
        if (checkLive) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_FRAME_NOT_LIVE,
                                 "Debug.Frame", fnname);
            return NULL;
        }
    }
    return thisobj;
}

#define THIS_FRAME(cx, vp, fnname, thisobj, fp)                              \
    JSObject *thisobj = CheckThisFrame(cx, vp, fnname, true);                \
    if (!thisobj)                                                            \
        return false;                                                        \
    StackFrame *fp = (StackFrame *) thisobj->getPrivate()

JSBool
Frame_getType(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get type", thisobj, fp);

    
    
    vp->setString(fp->isEvalFrame()
                  ? cx->runtime->atomState.evalAtom
                  : fp->isGlobalFrame()
                  ? cx->runtime->atomState.globalAtom
                  : cx->runtime->atomState.callAtom);
    return true;
}

JSBool
Frame_getGenerator(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get generator", thisobj, fp);
    vp->setBoolean(fp->isGeneratorFrame());
    return true;
}

JSBool
Frame_getLive(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *thisobj = CheckThisFrame(cx, vp, "get live", false);
    if (!thisobj)
        return false;
    StackFrame *fp = (StackFrame *) thisobj->getPrivate();
    vp->setBoolean(!!fp);
    return true;
}

JSBool
Frame_construct(JSContext *cx, uintN argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debug.Frame");
    return false;
}

JSPropertySpec Frame_properties[] = {
    JS_PSG("type", Frame_getType, 0),
    JS_PSG("generator", Frame_getGenerator, 0),
    JS_PSG("live", Frame_getLive, 0),
    JS_PS_END
};



extern JS_PUBLIC_API(JSBool)
JS_DefineDebugObject(JSContext *cx, JSObject *obj)
{
    JSObject *objProto;
    if (!js_GetClassPrototype(cx, obj, JSProto_Object, &objProto))
        return false;

    JSObject *debugCtor;
    JSObject *debugProto = js_InitClass(cx, obj, objProto, &Debug::jsclass, Debug::construct, 1,
                                        Debug::properties, NULL, NULL, NULL, &debugCtor);
    if (!debugProto || !debugProto->ensureClassReservedSlots(cx))
        return false;

    JSObject *frameCtor;
    JSObject *frameProto = js_InitClass(cx, debugCtor, objProto, &Frame_class, Frame_construct, 0,
                                        Frame_properties, NULL, NULL, NULL, &frameCtor);
    if (!frameProto)
        return false;
    debugProto->setReservedSlot(JSSLOT_DEBUG_FRAME_PROTO, ObjectValue(*frameProto));

    return true;
}
