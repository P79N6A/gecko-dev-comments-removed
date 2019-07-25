








































#include "jsdbg.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsgcmark.h"
#include "jsobj.h"
#include "jswrapper.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "vm/Stack-inl.h"

using namespace js;



extern Class DebugFrame_class;

enum {
    JSSLOT_DEBUGFRAME_OWNER,
    JSSLOT_DEBUGFRAME_ARGUMENTS,
    JSSLOT_DEBUGFRAME_COUNT
};

extern Class DebugObject_class;

enum {
    JSSLOT_DEBUGOBJECT_OWNER,
    JSSLOT_DEBUGOBJECT_CCW,  
    JSSLOT_DEBUGOBJECT_COUNT
};



bool
ReportMoreArgsNeeded(JSContext *cx, const char *name, uintN required)
{
    JS_ASSERT(required > 0);
    JS_ASSERT(required <= 10);
    char s[2];
    s[0] = '0' + (required - 1);
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
    JSSLOT_DEBUG_OBJECT_PROTO,
    JSSLOT_DEBUG_COUNT
};

Debug::Debug(JSObject *dbg, JSObject *hooks)
  : object(dbg), hooksObject(hooks), uncaughtExceptionHook(NULL), enabled(true),
    hasDebuggerHandler(false), hasThrowHandler(false)
{
    
    JSRuntime *rt = dbg->compartment()->rt;
    AutoLockGC lock(rt);
    JS_APPEND_LINK(&link, &rt->debuggerList);
}

Debug::~Debug()
{
    JS_ASSERT(object->compartment()->rt->gcRunning);
    if (!debuggees.empty()) {
        
        
        JS_ASSERT(object->compartment()->rt->gcCurrentCompartment == object->compartment());
        for (GlobalObjectSet::Enum e(debuggees); !e.empty(); e.popFront())
            removeDebuggeeGlobal(e.front(), NULL, &e);
    }

    
    JS_REMOVE_LINK(&link);
}

bool
Debug::init(JSContext *cx)
{
    bool ok = frames.init() && objects.init() && debuggees.init();
    if (!ok)
        js_ReportOutOfMemory(cx);
    return ok;
}

JS_STATIC_ASSERT(uintN(JSSLOT_DEBUGFRAME_OWNER) == uintN(JSSLOT_DEBUGOBJECT_OWNER));

Debug *
Debug::fromChildJSObject(JSObject *obj)
{
    JS_ASSERT(obj->clasp == &DebugFrame_class || obj->clasp == &DebugObject_class);
    JSObject *dbgobj = &obj->getReservedSlot(JSSLOT_DEBUGOBJECT_OWNER).toObject();
    return fromJSObject(dbgobj);
}

bool
Debug::getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp)
{
    JS_ASSERT(fp->isScriptFrame());
    FrameMap::AddPtr p = frames.lookupForAdd(fp);
    if (!p) {
        
        JSObject *argsobj;
        if (fp->hasArgs()) {
            uintN argc = fp->numActualArgs();
            JS_ASSERT(uint(argc) == argc);
            argsobj = NewDenseAllocatedArray(cx, uint(argc), NULL);
            Value *argv = fp->actualArgs();
            for (uintN i = 0; i < argc; i++) {
                Value v = argv[i];
                if (!wrapDebuggeeValue(cx, &v))
                    return false;
                argsobj->setDenseArrayElement(i, v);
            }
            if (!argsobj)
                return false;
        } else {
            argsobj = NULL;
        }

        
        JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_FRAME_PROTO).toObject();
        JSObject *frameobj = NewNonFunction<WithProto::Given>(cx, &DebugFrame_class, proto, NULL);
        if (!frameobj || !frameobj->ensureClassReservedSlots(cx))
            return false;
        frameobj->setPrivate(fp);
        frameobj->setReservedSlot(JSSLOT_DEBUGFRAME_OWNER, ObjectValue(*object));
        frameobj->setReservedSlot(JSSLOT_DEBUGFRAME_ARGUMENTS, ObjectOrNullValue(argsobj));

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
    GlobalObject *global = fp->scopeChain().getGlobal();

    
    
    
    if (GlobalObject::DebugVector *debuggers = global->getDebuggers()) {
        for (Debug **p = debuggers->begin(); p != debuggers->end(); p++) {
            Debug *dbg = *p;
            if (FrameMap::Ptr p = dbg->frames.lookup(fp)) {
                JSObject *frameobj = p->value;
                frameobj->setPrivate(NULL);
                dbg->frames.remove(p);
            }
        }
    }
}

bool
Debug::wrapDebuggeeValue(JSContext *cx, Value *vp)
{
    assertSameCompartment(cx, object);

    
    
    if (!cx->compartment->wrap(cx, vp)) {
        vp->setUndefined();
        return false;
    }

    if (vp->isObject()) {
        JSObject *ccwobj = &vp->toObject();
        vp->setUndefined();

        
        
        if (!ccwobj->isCrossCompartmentWrapper()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_STREAMS_CROSSED);
            return false;
        }

        ObjectMap::AddPtr p = objects.lookupForAdd(ccwobj);
        if (p) {
            vp->setObject(*p->value);
        } else {
            
            JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_OBJECT_PROTO).toObject();
            JSObject *dobj = NewNonFunction<WithProto::Given>(cx, &DebugObject_class, proto, NULL);
            if (!dobj || !dobj->ensureClassReservedSlots(cx))
                return false;
            dobj->setReservedSlot(JSSLOT_DEBUGOBJECT_OWNER, ObjectValue(*object));
            dobj->setReservedSlot(JSSLOT_DEBUGOBJECT_CCW, ObjectValue(*ccwobj));
            if (!objects.relookupOrAdd(p, ccwobj, dobj)) {
                js_ReportOutOfMemory(cx);
                return false;
            }
            vp->setObject(*dobj);
        }
    }
    return true;
}

bool
Debug::unwrapDebuggeeValue(JSContext *cx, Value *vp)
{
    assertSameCompartment(cx, object, *vp);
    if (vp->isObject()) {
        JSObject *dobj = &vp->toObject();
        if (dobj->clasp != &DebugObject_class) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_EXPECTED_TYPE,
                                 "Debug", "Debug.Object", dobj->clasp->name);
            return false;
        }

        Value owner = dobj->getReservedSlot(JSSLOT_DEBUGOBJECT_OWNER);
        if (owner.toObjectOrNull() != object) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 owner.isNull()
                                 ? JSMSG_DEBUG_OBJECT_PROTO
                                 : JSMSG_DEBUG_OBJECT_WRONG_OWNER);
            return false;
        }

        *vp = dobj->getReservedSlot(JSSLOT_DEBUGOBJECT_CCW);
    }
    return true;
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
    ac.leave();
    return JSTRAP_ERROR;
}

bool
Debug::newCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp)
{
    JS_ASSERT_IF(ok, !ac.context->isExceptionPending());

    JSContext *cx = ac.context;
    jsid key;
    if (ok) {
        ac.leave();
        key = ATOM_TO_JSID(cx->runtime->atomState.returnAtom);
    } else if (cx->isExceptionPending()) {
        key = ATOM_TO_JSID(cx->runtime->atomState.throwAtom);
        val = cx->getPendingException();
        cx->clearPendingException();
        ac.leave();
    } else {
        ac.leave();
        vp->setNull();
        return true;
    }

    JSObject *obj = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!obj ||
        !wrapDebuggeeValue(cx, &val) ||
        !DefineNativeProperty(cx, obj, key, val, PropertyStub, StrictPropertyStub,
                              JSPROP_ENUMERATE, 0, 0))
    {
        return false;
    }
    vp->setObject(*obj);
    return true;
}

JSTrapStatus
Debug::parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
                            bool callHook)
{
    vp->setUndefined();
    if (!ok)
        return handleUncaughtException(ac, vp, callHook);
    if (rv.isUndefined()) {
        ac.leave();
        return JSTRAP_CONTINUE;
    }
    if (rv.isNull()) {
        ac.leave();
        return JSTRAP_ERROR;
    }

    
    JSContext *cx = ac.context;
    JSObject *obj;
    const Shape *shape;
    jsid returnId = ATOM_TO_JSID(cx->runtime->atomState.returnAtom);
    jsid throwId = ATOM_TO_JSID(cx->runtime->atomState.throwAtom);
    if (!rv.isObject() ||
        !(obj = &rv.toObject())->isObject() ||
        !(shape = obj->lastProperty())->previous() ||
        shape->previous()->previous() ||
        (shape->propid != returnId && shape->propid != throwId) ||
        !shape->isDataDescriptor())
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_BAD_RESUMPTION);
        return handleUncaughtException(ac, vp, callHook);
    }

    if (!js_NativeGet(cx, obj, obj, shape, 0, vp) || !unwrapDebuggeeValue(cx, vp))
        return handleUncaughtException(ac, vp, callHook);

    ac.leave();
    if (!cx->compartment->wrap(cx, vp)) {
        vp->setUndefined();
        return JSTRAP_ERROR;
    }
    return shape->propid == returnId ? JSTRAP_RETURN : JSTRAP_THROW;
}

bool
CallMethodIfPresent(JSContext *cx, JSObject *obj, const char *name, int argc, Value *argv,
                    Value *rval)
{
    rval->setUndefined();
    JSAtom *atom = js_Atomize(cx, name, strlen(name));
    Value fval;
    return atom &&
           js_GetMethod(cx, obj, ATOM_TO_JSID(atom), JSGET_NO_METHOD_BARRIER, &fval) &&
           (!js_IsCallable(fval) ||
            ExternalInvoke(cx, ObjectValue(*obj), fval, argc, argv, rval));
}

bool
Debug::observesDebuggerStatement() const
{
    return enabled && hasDebuggerHandler;
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
        return handleUncaughtException(ac, vp, false);

    Value rv;
    bool ok = CallMethodIfPresent(cx, hooksObject, "debuggerHandler", 1, argv, &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

bool
Debug::observesThrow() const
{
    return enabled && hasThrowHandler;
}

JSTrapStatus
Debug::handleThrow(JSContext *cx, Value *vp)
{
    
    StackFrame *fp = cx->fp();
    Value exc = cx->getPendingException();

    cx->clearPendingException();
    JS_ASSERT(hasThrowHandler);
    AutoCompartment ac(cx, hooksObject);
    if (!ac.enter())
        return JSTRAP_ERROR;

    Value argv[2];
    argv[1] = exc;
    if (!getScriptFrame(cx, fp, &argv[0]) || !wrapDebuggeeValue(cx, &argv[1]))
        return handleUncaughtException(ac, vp, false);

    Value rv;
    bool ok = CallMethodIfPresent(cx, hooksObject, "throw", 2, argv, &rv);
    JSTrapStatus st = parseResumptionValue(ac, ok, rv, vp);
    if (st == JSTRAP_CONTINUE)
        cx->setPendingException(exc);
    return st;
}

JSTrapStatus
Debug::dispatchHook(JSContext *cx, js::Value *vp, DebugObservesMethod observesEvent,
                    DebugHandleMethod handleEvent)
{
    
    
    
    
    
    AutoValueVector triggered(cx);
    GlobalObject *global = cx->fp()->scopeChain().getGlobal();
    if (GlobalObject::DebugVector *debuggers = global->getDebuggers()) {
        for (Debug **p = debuggers->begin(); p != debuggers->end(); p++) {
            Debug *dbg = *p;
            if ((dbg->*observesEvent)()) {
                if (!triggered.append(ObjectValue(*dbg->toJSObject())))
                    return JSTRAP_ERROR;
            }
        }
    }

    
    
    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debug *dbg = Debug::fromJSObject(&p->toObject());
        if (dbg->debuggees.has(global) && (dbg->*observesEvent)()) {
            JSTrapStatus st = (dbg->*handleEvent)(cx, vp);
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
            const GlobalObjectSet &debuggees = dc->getDebuggees();
            for (GlobalObjectSet::Range r = debuggees.all(); !r.empty(); r.popFront()) {
                GlobalObject *global = r.front();

                
                
                const GlobalObject::DebugVector *debuggers = global->getDebuggers();
                for (Debug **p = debuggers->begin(); p != debuggers->end(); p++) {
                    Debug *dbg = *p;
                    JSObject *obj = dbg->toJSObject();

                    
                    
                    if ((!comp || obj->compartment() == comp) && !obj->isMarked()) {
                        if (dbg->hasAnyLiveHooks()) {
                            
                            
                            MarkObject(trc, *obj, "enabled Debug");
                            markedAny = true;
                        }
                    }

                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    if (!comp || obj->compartment() == comp) {
                        for (ObjectMap::Range r = dbg->objects.all(); !r.empty(); r.popFront()) {
                            
                            
                            
                            if (!r.front().value->isMarked() &&
                                (comp || r.front().key->unwrap()->isMarked()))
                            {
                                MarkObject(trc, *r.front().value,
                                           "Debug.Object with live referent");
                                markedAny = true;
                            }
                        }
                    }
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
            JSObject *frameobj = e.front().value;
            JS_ASSERT(frameobj->getPrivate());
            MarkObject(trc, *frameobj, "live Debug.Frame");
        }
    }
}

void
Debug::sweepAll(JSRuntime *rt)
{
    for (JSCList *p = &rt->debuggerList; (p = JS_NEXT_LINK(p)) != &rt->debuggerList;) {
        Debug *dbg = (Debug *) ((unsigned char *) p - offsetof(Debug, link));

        
        
        
        
        
        
        if (!dbg->object->isMarked()) {
            for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront())
                dbg->removeDebuggeeGlobal(e.front(), NULL, &e);
        }

        
        for (ObjectMap::Enum e(dbg->objects); !e.empty(); e.popFront()) {
            JS_ASSERT(e.front().key->isMarked() == e.front().value->isMarked());
            if (!e.front().value->isMarked())
                e.removeFront();
        }
    }

    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++)
        sweepCompartment(*c);
}

void
Debug::detachAllDebuggersFromGlobal(GlobalObject *global, GlobalObjectSet::Enum *compartmentEnum)
{
    const GlobalObject::DebugVector *debuggers = global->getDebuggers();
    JS_ASSERT(!debuggers->empty());
    while (!debuggers->empty())
        debuggers->back()->removeDebuggeeGlobal(global, compartmentEnum, NULL);
}

void
Debug::sweepCompartment(JSCompartment *compartment)
{
    
    GlobalObjectSet &debuggees = compartment->getDebuggees();
    for (GlobalObjectSet::Enum e(debuggees); !e.empty(); e.popFront()) {
        GlobalObject *global = e.front();
        if (!global->isMarked())
            detachAllDebuggersFromGlobal(global, &e);
    }
}

void
Debug::detachFromCompartment(JSCompartment *comp)
{
    for (GlobalObjectSet::Enum e(comp->getDebuggees()); !e.empty(); e.popFront())
        detachAllDebuggersFromGlobal(e.front(), &e);
}

void
Debug::finalize(JSContext *cx, JSObject *obj)
{
    Debug *dbg = (Debug *) obj->getPrivate();
    cx->delete_(dbg);
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

    bool hasDebuggerHandler, hasThrow;
    JSBool found;
    if (!JS_HasProperty(cx, hooksobj, "debuggerHandler", &found))
        return false;
    hasDebuggerHandler = !!found;
    if (!JS_HasProperty(cx, hooksobj, "throw", &found))
        return false;
    hasThrow = !!found;

    dbg->hooksObject = hooksobj;
    dbg->hasDebuggerHandler = hasDebuggerHandler;
    dbg->hasThrowHandler = hasThrow;
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

JSObject *
Debug::unwrapDebuggeeArgument(JSContext *cx, Value *vp)
{
    
    
    
    
    
    
    Value v = JS_ARGV(cx, vp)[0];
    JSObject *obj = NonNullObject(cx, v);
    if (obj) {
        if (obj->clasp == &DebugObject_class) {
            
            if (!unwrapDebuggeeValue(cx, &v))
                return NULL;
            obj = &v.toObject();
        }
        if (obj->isCrossCompartmentWrapper()) {
            obj = &obj->getProxyPrivate().toObject();
        }
    }
    return obj;
}

JSBool
Debug::addDebuggee(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.addDebuggee", 1);
    THISOBJ(cx, vp, Debug, "addDebuggee", thisobj, dbg);
    JSObject *referent = dbg->unwrapDebuggeeArgument(cx, vp);
    if (!referent)
        return false;
    GlobalObject *global = referent->getGlobal();
    if (!dbg->debuggees.has(global) && !dbg->addDebuggeeGlobal(cx, global))
        return false;

    Value v = ObjectValue(*referent);
    if (!dbg->wrapDebuggeeValue(cx, &v))
        return false;
    *vp = v;
    return true;
}

JSBool
Debug::removeDebuggee(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.removeDebuggee", 1);
    THISOBJ(cx, vp, Debug, "removeDebuggee", thisobj, dbg);
    JSObject *referent = dbg->unwrapDebuggeeArgument(cx, vp);
    if (!referent)
        return false;
    GlobalObject *global = referent->getGlobal();
    if (dbg->debuggees.has(global))
        dbg->removeDebuggeeGlobal(global, NULL, NULL);
    vp->setUndefined();
    return true;
}

JSBool
Debug::hasDebuggee(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.hasDebuggee", 1);
    THISOBJ(cx, vp, Debug, "hasDebuggee", thisobj, dbg);
    JSObject *referent = dbg->unwrapDebuggeeArgument(cx, vp);
    if (!referent)
        return false;
    vp->setBoolean(!!dbg->debuggees.lookup(referent->getGlobal()));
    return true;
}

JSBool
Debug::getDebuggees(JSContext *cx, uintN argc, Value *vp)
{
    THISOBJ(cx, vp, Debug, "getDebuggees", thisobj, dbg);
    JSObject *arrobj = NewDenseAllocatedArray(cx, dbg->debuggees.count(), NULL);
    if (!arrobj)
        return false;
    uintN i = 0;
    for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront()) {
        Value v = ObjectValue(*e.front());
        if (!dbg->wrapDebuggeeValue(cx, &v))
            return false;
        arrobj->setDenseArrayElement(i++, v);
    }
    vp->setObject(*arrobj);
    return true;
}

JSBool
Debug::getYoungestFrame(JSContext *cx, uintN argc, Value *vp)
{
    THISOBJ(cx, vp, Debug, "getYoungestFrame", thisobj, dbg);
    StackFrame *fp = cx->fp();
    while (fp && !dbg->observesFrame(fp))
        fp = fp->prev();
    if (!fp) {
        vp->setNull();
        return true;
    }
    return dbg->getScriptFrame(cx, fp, vp);
}

JSBool
Debug::construct(JSContext *cx, uintN argc, Value *vp)
{
    
    Value *argv = vp + 2, *argvEnd = argv + argc;
    for (Value *p = argv; p != argvEnd; p++) {
        
        const Value &arg = *p;
        if (!arg.isObject())
            return ReportObjectRequired(cx);
        JSObject *argobj = &arg.toObject();
        if (!argobj->isCrossCompartmentWrapper()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CCW_REQUIRED, "Debug");
            return false;
        }

        
        if (!argobj->getProxyPrivate().toObject().compartment()->debugMode) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DEBUG_MODE);
            return false;
        }
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
    for (uintN slot = JSSLOT_DEBUG_FRAME_PROTO; slot < JSSLOT_DEBUG_COUNT; slot++)
        obj->setReservedSlot(slot, proto->getReservedSlot(slot));

    JSObject *hooks = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!hooks)
        return false;

    Debug *dbg = cx->new_<Debug>(obj, hooks);
    if (!dbg)
        return false;
    obj->setPrivate(dbg);
    if (!dbg->init(cx)) {
        cx->delete_(dbg);
        return false;
    }

    
    for (Value *p = argv; p != argvEnd; p++) {
        GlobalObject *debuggee = p->toObject().getProxyPrivate().toObject().getGlobal();
        if (!dbg->addDebuggeeGlobal(cx, debuggee))
            return false;
    }

    vp->setObject(*obj);
    return true;
}

bool
Debug::addDebuggeeGlobal(JSContext *cx, GlobalObject *obj)
{
    
    
    
    
    Vector<JSCompartment *> visited(cx);
    if (!visited.append(object->compartment()))
        return false;
    JSCompartment *dest = obj->compartment();
    for (size_t i = 0; i < visited.length(); i++) {
        JSCompartment *c = visited[i];
        if (c == dest) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_LOOP);
            return false;
        }

        
        
        for (GlobalObjectSet::Range r = c->getDebuggees().all(); !r.empty(); r.popFront()) {
            GlobalObject::DebugVector *v = r.front()->getDebuggers();
            for (Debug **p = v->begin(); p != v->end(); p++) {
                JSCompartment *next = (*p)->object->compartment();
                if (visited.find(next) == visited.end() && !visited.append(next))
                    return false;
            }
        }
    }

    
    GlobalObject::DebugVector *v = obj->getOrCreateDebuggers(cx);
    if (!v || !v->append(this))
        goto fail1;
    if (!debuggees.put(obj))
        goto fail2;
    if (obj->getDebuggers()->length() == 1 && !obj->compartment()->addDebuggee(obj))
        goto fail3;
    return true;

    
fail3:
    debuggees.remove(obj);
fail2:
    JS_ASSERT(v->back() == this);
    v->popBack();
fail1:
    js_ReportOutOfMemory(cx);
    return false;
}

void
Debug::removeDebuggeeGlobal(GlobalObject *global, GlobalObjectSet::Enum *compartmentEnum,
                            GlobalObjectSet::Enum *debugEnum)
{
    
    
    
    
    JS_ASSERT(global->compartment()->getDebuggees().has(global));
    JS_ASSERT_IF(compartmentEnum, compartmentEnum->front() == global);
    JS_ASSERT(debuggees.has(global));
    JS_ASSERT_IF(debugEnum, debugEnum->front() == global);

    
    
    
    
    
    
    
    for (FrameMap::Enum e(frames); !e.empty(); e.popFront()) {
        js::StackFrame *fp = e.front().key;
        if (fp->scopeChain().getGlobal() == global) {
            e.front().value->setPrivate(NULL);
            e.removeFront();
        }
    }

    GlobalObject::DebugVector *v = global->getDebuggers();
    Debug **p;
    for (p = v->begin(); p != v->end(); p++) {
        if (*p == this)
            break;
    }
    JS_ASSERT(p != v->end());

    
    
    v->erase(p);
    if (v->empty()) {
        if (compartmentEnum)
            compartmentEnum->removeFront();
        else
            global->compartment()->removeDebuggee(global);
    }
    if (debugEnum)
        debugEnum->removeFront();
    else
        debuggees.remove(global);
}

JSPropertySpec Debug::properties[] = {
    JS_PSGS("hooks", Debug::getHooks, Debug::setHooks, 0),
    JS_PSGS("enabled", Debug::getEnabled, Debug::setEnabled, 0),
    JS_PSGS("uncaughtExceptionHook", Debug::getUncaughtExceptionHook,
            Debug::setUncaughtExceptionHook, 0),
    JS_PS_END
};

JSFunctionSpec Debug::methods[] = {
    JS_FN("addDebuggee", Debug::addDebuggee, 1, 0),
    JS_FN("removeDebuggee", Debug::removeDebuggee, 1, 0),
    JS_FN("hasDebuggee", Debug::hasDebuggee, 1, 0),
    JS_FN("getDebuggees", Debug::getDebuggees, 0, 0),
    JS_FN("getYoungestFrame", Debug::getYoungestFrame, 0, 0),
    JS_FS_END
};



Class DebugFrame_class = {
    "Frame", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGFRAME_COUNT),
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub, FinalizeStub,
};

static JSObject *
CheckThisFrame(JSContext *cx, Value *vp, const char *fnname, bool checkLive)
{
    if (!vp[1].isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &vp[1].toObject();
    if (thisobj->getClass() != &DebugFrame_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debug.Frame", fnname, thisobj->getClass()->name);
        return NULL;
    }

    
    
    if (!thisobj->getPrivate()) {
        if (thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_OWNER).isUndefined()) {
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

static JSBool
DebugFrame_getType(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get type", thisobj, fp);

    
    
    vp->setString(fp->isEvalFrame()
                  ? cx->runtime->atomState.evalAtom
                  : fp->isGlobalFrame()
                  ? cx->runtime->atomState.globalAtom
                  : cx->runtime->atomState.callAtom);
    return true;
}

static JSBool
DebugFrame_getCallee(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get callee", thisobj, fp);
    *vp = (fp->isFunctionFrame() && !fp->isEvalFrame()) ? fp->calleev() : NullValue();
    return Debug::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, vp);
}

static JSBool
DebugFrame_getGenerator(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get generator", thisobj, fp);
    vp->setBoolean(fp->isGeneratorFrame());
    return true;
}

static JSBool
DebugFrame_getConstructing(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get constructing", thisobj, fp);
    vp->setBoolean(fp->isFunctionFrame() && fp->isConstructing());
    return true;
}

static JSBool
DebugFrame_getThis(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get this", thisobj, fp);
    {
        AutoCompartment ac(cx, &fp->scopeChain());
        if (!ac.enter())
            return false;
        if (!ComputeThis(cx, fp))
            return false;
        *vp = fp->thisValue();
    }
    return Debug::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, vp);
}

static JSBool
DebugFrame_getOlder(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get this", thisobj, thisfp);
    Debug *dbg = Debug::fromChildJSObject(thisobj);
    for (StackFrame *fp = thisfp->prev(); fp; fp = fp->prev()) {
        if (!fp->isDummyFrame() && dbg->observesFrame(fp))
            return dbg->getScriptFrame(cx, fp, vp);
    }
    vp->setNull();
    return true;
}

JSBool
DebugFrame_getArguments(JSContext *cx, uintN argc, Value *vp)
{
    THIS_FRAME(cx, vp, "get arguments", thisobj, fp);
    (void) fp;  
    *vp = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ARGUMENTS);
    return true;
}

static JSBool
DebugFrame_getLive(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *thisobj = CheckThisFrame(cx, vp, "get live", false);
    if (!thisobj)
        return false;
    StackFrame *fp = (StackFrame *) thisobj->getPrivate();
    vp->setBoolean(!!fp);
    return true;
}

static JSBool
DebugFrame_eval(JSContext *cx, uintN argc, Value *vp)
{
    REQUIRE_ARGC("Debug.Frame.eval", 1);
    THIS_FRAME(cx, vp, "eval", thisobj, fp);
    Debug *dbg = Debug::fromChildJSObject(&vp[1].toObject());

    if (!vp[2].isString()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_EXPECTED_TYPE,
                             "Debug.Frame.eval", "string", InformalValueTypeName(vp[2]));
        return false;
    }
    JSLinearString *linearStr = vp[2].toString()->ensureLinear(cx);
    if (!linearStr)
        return false;
    JS::Anchor<JSString *> anchor(linearStr);

    AutoCompartment ac(cx, &fp->scopeChain());
    if (!ac.enter())
        return false;
    Value rval;
    bool ok = JS_EvaluateUCInStackFrame(cx, Jsvalify(fp), linearStr->chars(), linearStr->length(),
                                        "debugger eval code", 1, Jsvalify(&rval));
    return dbg->newCompletionValue(ac, ok, rval, vp);
}

static JSBool
DebugFrame_construct(JSContext *cx, uintN argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debug.Frame");
    return false;
}

static JSPropertySpec DebugFrame_properties[] = {
    JS_PSG("type", DebugFrame_getType, 0),
    JS_PSG("this", DebugFrame_getThis, 0),
    JS_PSG("older", DebugFrame_getOlder, 0),
    JS_PSG("live", DebugFrame_getLive, 0),
    JS_PSG("callee", DebugFrame_getCallee, 0),
    JS_PSG("generator", DebugFrame_getGenerator, 0),
    JS_PSG("constructing", DebugFrame_getConstructing, 0),
    JS_PSG("arguments", DebugFrame_getArguments, 0),
    JS_PS_END
};

static JSFunctionSpec DebugFrame_methods[] = {
    JS_FN("eval", DebugFrame_eval, 1, 0),
    JS_FS_END
};



Class DebugObject_class = {
    "Object", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGOBJECT_COUNT),
    PropertyStub, PropertyStub, PropertyStub, StrictPropertyStub,
    EnumerateStub, ResolveStub, ConvertStub, FinalizeStub,
};

static JSObject *
DebugObject_checkThis(JSContext *cx, Value *vp, const char *fnname)
{
    if (!vp[1].isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &vp[1].toObject();
    if (thisobj->clasp != &DebugObject_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debug.Object", fnname, thisobj->getClass()->name);
        return NULL;
    }

    
    
    if (thisobj->getReservedSlot(JSSLOT_DEBUGOBJECT_CCW).isUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debug.Object", fnname, "prototype object");
        return NULL;
    }
    return thisobj;
}

#define THIS_DEBUGOBJECT_CCW(cx, vp, fnname, obj)                            \
    JSObject *obj = DebugObject_checkThis(cx, vp, fnname);                   \
    if (!obj)                                                                \
        return false;                                                        \
    obj = &obj->getReservedSlot(JSSLOT_DEBUGOBJECT_CCW).toObject();          \
    JS_ASSERT(obj->isCrossCompartmentWrapper())

#define THIS_DEBUGOBJECT_REFERENT(cx, vp, fnname, obj)                       \
    THIS_DEBUGOBJECT_CCW(cx, vp, fnname, obj);                               \
    JS_ASSERT(obj->isCrossCompartmentWrapper());                             \
    obj = JSWrapper::wrappedObject(obj)

static JSBool
DebugObject_construct(JSContext *cx, uintN argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debug.Object");
    return false;
}

static JSBool
DebugObject_getProto(JSContext *cx, uintN argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, vp, "get proto", refobj);
    vp->setObjectOrNull(refobj->getProto());
    return Debug::fromChildJSObject(&vp[1].toObject())->wrapDebuggeeValue(cx, vp);
}

static JSBool
DebugObject_getClass(JSContext *cx, uintN argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, vp, "get class", refobj);
    const char *s = refobj->clasp->name;
    JSAtom *str = js_Atomize(cx, s, strlen(s));
    if (!str)
        return false;
    vp->setString(str);
    return true;
}

static JSBool
DebugObject_getCallable(JSContext *cx, uintN argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, vp, "get callable", refobj);
    vp->setBoolean(refobj->isCallable());
    return true;
}

static JSBool
DebugObject_getName(JSContext *cx, uintN argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, vp, "get name", obj);
    if (obj->isFunction()) {
        if (JSString *name = obj->getFunctionPrivate()->atom) {
            vp->setString(name);
            return Debug::fromChildJSObject(&vp[1].toObject())->wrapDebuggeeValue(cx, vp);
        }
    }
    vp->setNull();
    return true;
}

static JSBool
DebugObject_apply(JSContext *cx, uintN argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, vp, "apply", obj);
    Debug *dbg = Debug::fromChildJSObject(&vp[1].toObject());

    
    
    if (!obj->isCallable()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debug.Object", "apply", obj->getClass()->name);
        return false;
    }

    
    
    Value calleev = vp[1];
    Value thisv = argc > 0 ? vp[2] : UndefinedValue();
    AutoValueVector argv(cx);
    if (!dbg->unwrapDebuggeeValue(cx, &calleev) || !dbg->unwrapDebuggeeValue(cx, &thisv))
        return false;
    if (argc >= 2 && !vp[3].isNullOrUndefined()) {
        if (!vp[3].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS, js_apply_str);
            return false;
        }
        JSObject *argsobj = &vp[3].toObject();
        uintN length;
        if (!js_GetLengthProperty(cx, argsobj, &length))
            return false;
        length = uintN(JS_MIN(length, JS_ARGS_LENGTH_MAX));

        if (!argv.growBy(length) || !GetElements(cx, argsobj, length, argv.begin()))
            return false;
        for (uintN i = 0; i < length; i++) {
            if (!dbg->unwrapDebuggeeValue(cx, &argv[i]))
                return false;
        }
    }

    
    
    AutoCompartment ac(cx, obj);
    if (!ac.enter() || !cx->compartment->wrap(cx, &calleev) || !cx->compartment->wrap(cx, &thisv))
        return false;
    for (Value *p = argv.begin(); p != argv.end(); ++p) {
        if (!cx->compartment->wrap(cx, p))
            return false;
    }

    
    
    Value rval;
    bool ok = ExternalInvoke(cx, thisv, calleev, argv.length(), argv.begin(), &rval);
    return dbg->newCompletionValue(ac, ok, rval, vp);
}

static JSPropertySpec DebugObject_properties[] = {
    JS_PSG("proto", DebugObject_getProto, 0),
    JS_PSG("class", DebugObject_getClass, 0),
    JS_PSG("callable", DebugObject_getCallable, 0),
    JS_PSG("name", DebugObject_getName, 0),
    JS_PS_END
};

static JSFunctionSpec DebugObject_methods[] = {
    JS_FN("apply", DebugObject_apply, 0, 0),
    JS_FS_END
};



extern JS_PUBLIC_API(JSBool)
JS_DefineDebugObject(JSContext *cx, JSObject *obj)
{
    JSObject *objProto;
    if (!js_GetClassPrototype(cx, obj, JSProto_Object, &objProto))
        return false;

    JSObject *debugCtor;
    JSObject *debugProto = js_InitClass(cx, obj, objProto, &Debug::jsclass, Debug::construct, 1,
                                        Debug::properties, Debug::methods, NULL, NULL, &debugCtor);
    if (!debugProto || !debugProto->ensureClassReservedSlots(cx))
        return false;

    JSObject *frameCtor;
    JSObject *frameProto = js_InitClass(cx, debugCtor, objProto, &DebugFrame_class,
                                        DebugFrame_construct, 0,
                                        DebugFrame_properties, DebugFrame_methods, NULL, NULL,
                                        &frameCtor);
    if (!frameProto)
        return false;

    JSObject *objectProto = js_InitClass(cx, debugCtor, objProto, &DebugObject_class,
                                         DebugObject_construct, 0,
                                         DebugObject_properties, DebugObject_methods, NULL, NULL);
    if (!objectProto)
        return false;

    debugProto->setReservedSlot(JSSLOT_DEBUG_FRAME_PROTO, ObjectValue(*frameProto));
    debugProto->setReservedSlot(JSSLOT_DEBUG_OBJECT_PROTO, ObjectValue(*objectProto));
    return true;
}
