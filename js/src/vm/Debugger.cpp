








































#include "vm/Debugger.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jsgcmark.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jswrapper.h"
#include "jsarrayinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/BytecodeEmitter.h"
#include "methodjit/Retcon.h"

#include "vm/Stack-inl.h"

using namespace js;




extern Class DebuggerFrame_class;

enum {
    JSSLOT_DEBUGFRAME_OWNER,
    JSSLOT_DEBUGFRAME_ARGUMENTS,
    JSSLOT_DEBUGFRAME_ONSTEP_HANDLER,
    JSSLOT_DEBUGFRAME_ONPOP_HANDLER,
    JSSLOT_DEBUGFRAME_COUNT
};

extern Class DebuggerArguments_class;

enum {
    JSSLOT_DEBUGARGUMENTS_FRAME,
    JSSLOT_DEBUGARGUMENTS_COUNT
};

extern Class DebuggerEnv_class;

enum {
    JSSLOT_DEBUGENV_OWNER,
    JSSLOT_DEBUGENV_COUNT
};

extern Class DebuggerObject_class;

enum {
    JSSLOT_DEBUGOBJECT_OWNER,
    JSSLOT_DEBUGOBJECT_COUNT
};

extern Class DebuggerScript_class;

enum {
    JSSLOT_DEBUGSCRIPT_OWNER,
    JSSLOT_DEBUGSCRIPT_COUNT
};




bool
ReportMoreArgsNeeded(JSContext *cx, const char *name, unsigned required)
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

#define REQUIRE_ARGC(name, n)                                                 \
    JS_BEGIN_MACRO                                                            \
        if (argc < (n))                                                       \
            return ReportMoreArgsNeeded(cx, name, n);                         \
    JS_END_MACRO

bool
ReportObjectRequired(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_NONNULL_OBJECT);
    return false;
}

bool
ValueToIdentifier(JSContext *cx, const Value &v, jsid *idp)
{
    jsid id;
    if (!ValueToId(cx, v, &id))
        return false;
    if (!JSID_IS_ATOM(id) || !IsIdentifier(JSID_TO_ATOM(id))) {
        js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_UNEXPECTED_TYPE,
                                 JSDVG_SEARCH_STACK, v, NULL, "not an identifier", NULL);
        return false;
    }
    *idp = id;
    return true;
}








class Debugger::FrameRange {
    JSContext *cx;
    StackFrame *fp;

    
    GlobalObject::DebuggerVector *debuggers;

    



    size_t debuggerCount, nextDebugger;

    



    FrameMap::Ptr entry;

  public:
    










    FrameRange(JSContext *cx, StackFrame *fp, GlobalObject *global = NULL)
      : cx(cx), fp(fp) {
        nextDebugger = 0;

        
        if (!global)
            global = &fp->scopeChain().global();

        
        JS_ASSERT(&fp->scopeChain().global() == global);

        
        debuggers = global->getDebuggers();
        if (debuggers) {
            debuggerCount = debuggers->length();
            findNext();
        } else {
            debuggerCount = 0;
        }
    }

    bool empty() const {
        return nextDebugger >= debuggerCount;
    }

    JSObject *frontFrame() const {
        JS_ASSERT(!empty());
        return entry->value;
    }

    Debugger *frontDebugger() const {
        JS_ASSERT(!empty());
        return (*debuggers)[nextDebugger];
    }

    



    void removeFrontFrame() const {
        JS_ASSERT(!empty());
        frontDebugger()->frames.remove(entry);
    }

    void popFront() { 
        JS_ASSERT(!empty());
        nextDebugger++;
        findNext();
    }

  private:
    



    void findNext() {
        while (!empty()) {
            Debugger *dbg = (*debuggers)[nextDebugger];
            entry = dbg->frames.lookup(fp);
            if (entry)
                break;
            nextDebugger++;
        }
    }
};




BreakpointSite::BreakpointSite(JSScript *script, jsbytecode *pc)
  : script(script), pc(pc), scriptGlobal(NULL), enabledCount(0),
    trapHandler(NULL), trapClosure(UndefinedValue())
{
    JS_ASSERT(!script->hasBreakpointsAt(pc));
    JS_INIT_CLIST(&breakpoints);
}





static GlobalObject *
ScriptGlobal(JSContext *cx, JSScript *script, GlobalObject *scriptGlobal)
{
    if (scriptGlobal)
        return scriptGlobal;

    



    for (AllFramesIter i(cx->stack.space()); ; ++i) {
        JS_ASSERT(!i.done());
        if (i.fp()->maybeScript() == script)
            return &i.fp()->scopeChain().global();
    }
    JS_NOT_REACHED("ScriptGlobal: live non-held script not on stack");
}

bool
BreakpointSite::recompile(JSContext *cx, bool forTrap)
{
#ifdef JS_METHODJIT
    if (script->hasJITCode()) {
        Maybe<AutoCompartment> ac;
        if (!forTrap) {
            ac.construct(cx, ScriptGlobal(cx, script, scriptGlobal));
            if (!ac.ref().enter())
                return false;
        }
        mjit::Recompiler::clearStackReferences(cx, script);
        mjit::ReleaseScriptCode(cx, script);
    }
#endif
    return true;
}

bool
BreakpointSite::inc(JSContext *cx)
{
    if (enabledCount == 0 && !trapHandler) {
        if (!recompile(cx, false))
            return false;
    }
    enabledCount++;
    return true;
}

void
BreakpointSite::dec(JSContext *cx)
{
    JS_ASSERT(enabledCount > 0);
    enabledCount--;
    if (enabledCount == 0 && !trapHandler)
        recompile(cx, false);  
}

bool
BreakpointSite::setTrap(JSContext *cx, JSTrapHandler handler, const Value &closure)
{
    if (enabledCount == 0) {
        if (!recompile(cx, true))
            return false;
    }
    trapHandler = handler;
    trapClosure = closure;
    return true;
}

void
BreakpointSite::clearTrap(JSContext *cx, JSTrapHandler *handlerp, Value *closurep)
{
    if (handlerp)
        *handlerp = trapHandler;
    if (closurep)
        *closurep = trapClosure;

    trapHandler = NULL;
    trapClosure = UndefinedValue();
    if (enabledCount == 0) {
        if (!cx->runtime->gcRunning) {
            
            recompile(cx, true);  
        }
        destroyIfEmpty(cx->runtime);
    }
}

void
BreakpointSite::destroyIfEmpty(JSRuntime *rt)
{
    if (JS_CLIST_IS_EMPTY(&breakpoints) && !trapHandler)
        script->destroyBreakpointSite(rt, pc);
}

Breakpoint *
BreakpointSite::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return NULL;
    return Breakpoint::fromSiteLinks(JS_NEXT_LINK(&breakpoints));
}

bool
BreakpointSite::hasBreakpoint(Breakpoint *bp)
{
    for (Breakpoint *p = firstBreakpoint(); p; p = p->nextInSite())
        if (p == bp)
            return true;
    return false;
}

Breakpoint::Breakpoint(Debugger *debugger, BreakpointSite *site, JSObject *handler)
    : debugger(debugger), site(site), handler(handler)
{
    JS_APPEND_LINK(&debuggerLinks, &debugger->breakpoints);
    JS_APPEND_LINK(&siteLinks, &site->breakpoints);
}

Breakpoint *
Breakpoint::fromDebuggerLinks(JSCList *links)
{
    return (Breakpoint *) ((unsigned char *) links - offsetof(Breakpoint, debuggerLinks));
}

Breakpoint *
Breakpoint::fromSiteLinks(JSCList *links)
{
    return (Breakpoint *) ((unsigned char *) links - offsetof(Breakpoint, siteLinks));
}

void
Breakpoint::destroy(JSContext *cx)
{
    if (debugger->enabled)
        site->dec(cx);
    JS_REMOVE_LINK(&debuggerLinks);
    JS_REMOVE_LINK(&siteLinks);
    JSRuntime *rt = cx->runtime;
    site->destroyIfEmpty(rt);
    rt->delete_(this);
}

Breakpoint *
Breakpoint::nextInDebugger()
{
    JSCList *link = JS_NEXT_LINK(&debuggerLinks);
    return (link == &debugger->breakpoints) ? NULL : fromDebuggerLinks(link);
}

Breakpoint *
Breakpoint::nextInSite()
{
    JSCList *link = JS_NEXT_LINK(&siteLinks);
    return (link == &site->breakpoints) ? NULL : fromSiteLinks(link);
}




Debugger::Debugger(JSContext *cx, JSObject *dbg)
  : object(dbg), uncaughtExceptionHook(NULL), enabled(true),
    frames(cx), scripts(cx), objects(cx), environments(cx)
{
    assertSameCompartment(cx, dbg);

    JSRuntime *rt = cx->runtime;
    AutoLockGC lock(rt);
    JS_APPEND_LINK(&link, &rt->debuggerList);
    JS_INIT_CLIST(&breakpoints);
}

Debugger::~Debugger()
{
    JS_ASSERT(debuggees.empty());

    
    JS_ASSERT(object->compartment()->rt->gcRunning);
    JS_REMOVE_LINK(&link);
}

bool
Debugger::init(JSContext *cx)
{
    bool ok = debuggees.init() &&
              frames.init() &&
              scripts.init() &&
              objects.init() &&
              environments.init();
    if (!ok)
        js_ReportOutOfMemory(cx);
    return ok;
}

JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGSCRIPT_OWNER));
JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGOBJECT_OWNER));
JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGENV_OWNER));

Debugger *
Debugger::fromChildJSObject(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &DebuggerFrame_class ||
              obj->getClass() == &DebuggerScript_class ||
              obj->getClass() == &DebuggerObject_class ||
              obj->getClass() == &DebuggerEnv_class);
    JSObject *dbgobj = &obj->getReservedSlot(JSSLOT_DEBUGOBJECT_OWNER).toObject();
    return fromJSObject(dbgobj);
}

bool
Debugger::getScriptFrame(JSContext *cx, StackFrame *fp, Value *vp)
{
    JS_ASSERT(fp->isScriptFrame());
    FrameMap::AddPtr p = frames.lookupForAdd(fp);
    if (!p) {
        
        JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_FRAME_PROTO).toObject();
        JSObject *frameobj =
            NewObjectWithGivenProto(cx, &DebuggerFrame_class, proto, NULL);
        if (!frameobj)
            return false;
        frameobj->setPrivate(fp);
        frameobj->setReservedSlot(JSSLOT_DEBUGFRAME_OWNER, ObjectValue(*object));

        if (!frames.add(p, fp, frameobj)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }
    vp->setObject(*p->value);
    return true;
}

JSObject *
Debugger::getHook(Hook hook) const
{
    JS_ASSERT(hook >= 0 && hook < HookCount);
    const Value &v = object->getReservedSlot(JSSLOT_DEBUG_HOOK_START + hook);
    return v.isUndefined() ? NULL : &v.toObject();
}

bool
Debugger::hasAnyLiveHooks() const
{
    if (!enabled)
        return false;

    if (getHook(OnDebuggerStatement) ||
        getHook(OnExceptionUnwind) ||
        getHook(OnNewScript) ||
        getHook(OnEnterFrame))
    {
        return true;
    }

    
    for (Breakpoint *bp = firstBreakpoint(); bp; bp = bp->nextInDebugger()) {
        if (!IsAboutToBeFinalized(bp->site->script))
            return true;
    }

    for (FrameMap::Range r = frames.all(); !r.empty(); r.popFront()) {
        JSObject *frameObj = r.front().value;
        if (!frameObj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined() ||
            !frameObj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER).isUndefined())
            return true;
    }

    return false;
}

JSTrapStatus
Debugger::slowPathOnEnterFrame(JSContext *cx, Value *vp)
{
    
    AutoValueVector triggered(cx);
    GlobalObject *global = &cx->fp()->scopeChain().global();
    if (GlobalObject::DebuggerVector *debuggers = global->getDebuggers()) {
        for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++) {
            Debugger *dbg = *p;
            JS_ASSERT(dbg->observesFrame(cx->fp()));
            if (dbg->observesEnterFrame() && !triggered.append(ObjectValue(*dbg->toJSObject())))
                return JSTRAP_ERROR;
        }
    }

    
    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debugger *dbg = Debugger::fromJSObject(&p->toObject());
        if (dbg->debuggees.has(global) && dbg->observesEnterFrame()) {
            JSTrapStatus status = dbg->fireEnterFrame(cx, vp);
            if (status != JSTRAP_CONTINUE)
                return status;
        }
    }

    return JSTRAP_CONTINUE;
}






bool
Debugger::slowPathOnLeaveFrame(JSContext *cx, bool frameOk)
{
    StackFrame *fp = cx->fp();
    GlobalObject *global = &fp->scopeChain().global();

    
    JSTrapStatus status;
    Value value;
    Debugger::resultToCompletion(cx, frameOk, fp->returnValue(), &status, &value);

    
    AutoObjectVector frames(cx);
    for (FrameRange r(cx, fp, global); !r.empty(); r.popFront()) {
        if (!frames.append(r.frontFrame())) {
            cx->clearPendingException();
            return false;
        }
    }

    
    for (JSObject **p = frames.begin(); p != frames.end(); p++) {
        JSObject *frameobj = *p;
        Debugger *dbg = Debugger::fromChildJSObject(frameobj);

        if (dbg->enabled &&
            !frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER).isUndefined()) {
            const Value &handler = frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER);

            AutoCompartment ac(cx, dbg->object);

            if (!ac.enter()) {
                status = JSTRAP_ERROR;
                break;
            }
                
            Value completion;
            if (!dbg->newCompletionValue(cx, status, value, &completion)) {
                status = dbg->handleUncaughtException(ac, NULL, false);
                break;
            }

            
            Value rval;
            bool hookOk = Invoke(cx, ObjectValue(*frameobj), handler, 1, &completion, &rval);
            Value nextValue;
            JSTrapStatus nextStatus = dbg->parseResumptionValue(ac, hookOk, rval, &nextValue);
            
            



            JS_ASSERT(cx->compartment == global->compartment());
            JS_ASSERT(!cx->isExceptionPending());

            
            if (nextStatus != JSTRAP_CONTINUE) {
                status = nextStatus;
                value = nextValue;
            }
        }
    }

    




    for (FrameRange r(cx, fp, global); !r.empty(); r.popFront()) {
        JSObject *frameobj = r.frontFrame();
        Debugger *dbg = r.frontDebugger();
        JS_ASSERT(dbg == Debugger::fromChildJSObject(frameobj));

        frameobj->setPrivate(NULL);

        
        if (!frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined() &&
            fp->isScriptFrame() &&
            !fp->script()->changeStepModeCount(cx, -1))
        {
            status = JSTRAP_ERROR;
            
        }

        dbg->frames.remove(fp);
    }

    



    if (fp->isEvalFrame()) {
        JSScript *script = fp->script();
        script->clearBreakpointsIn(cx, NULL, NULL);
    }

    
    switch (status) {
      case JSTRAP_RETURN:
        fp->setReturnValue(value);
        return true;

      case JSTRAP_THROW:
        cx->setPendingException(value);
        return false;
        
      case JSTRAP_ERROR:
        JS_ASSERT(!cx->isExceptionPending());
        return false;

      default:
        JS_NOT_REACHED("bad final trap status");
    }
}

bool
Debugger::wrapEnvironment(JSContext *cx, Env *env, Value *rval)
{
    if (!env) {
        rval->setNull();
        return true;
    }

    JSObject *envobj;
    ObjectWeakMap::AddPtr p = environments.lookupForAdd(env);
    if (p) {
        envobj = p->value;
    } else {
        
        JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_ENV_PROTO).toObject();
        envobj = NewObjectWithGivenProto(cx, &DebuggerEnv_class, proto, NULL);
        if (!envobj)
            return false;
        envobj->setPrivate(env);
        envobj->setReservedSlot(JSSLOT_DEBUGENV_OWNER, ObjectValue(*object));
        if (!environments.relookupOrAdd(p, env, envobj)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }
    rval->setObject(*envobj);
    return true;
}

bool
Debugger::wrapDebuggeeValue(JSContext *cx, Value *vp)
{
    assertSameCompartment(cx, object.get());

    if (vp->isObject()) {
        JSObject *obj = &vp->toObject();

        ObjectWeakMap::AddPtr p = objects.lookupForAdd(obj);
        if (p) {
            vp->setObject(*p->value);
        } else {
            
            JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_OBJECT_PROTO).toObject();
            JSObject *dobj =
                NewObjectWithGivenProto(cx, &DebuggerObject_class, proto, NULL);
            if (!dobj)
                return false;
            dobj->setPrivate(obj);
            dobj->setReservedSlot(JSSLOT_DEBUGOBJECT_OWNER, ObjectValue(*object));
            if (!objects.relookupOrAdd(p, obj, dobj)) {
                js_ReportOutOfMemory(cx);
                return false;
            }
            vp->setObject(*dobj);
        }
    } else if (!cx->compartment->wrap(cx, vp)) {
        vp->setUndefined();
        return false;
    }

    return true;
}

bool
Debugger::unwrapDebuggeeValue(JSContext *cx, Value *vp)
{
    assertSameCompartment(cx, object.get(), *vp);
    if (vp->isObject()) {
        JSObject *dobj = &vp->toObject();
        if (dobj->getClass() != &DebuggerObject_class) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_EXPECTED_TYPE,
                                 "Debugger", "Debugger.Object", dobj->getClass()->name);
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

        vp->setObject(*(JSObject *) dobj->getPrivate());
    }
    return true;
}

JSTrapStatus
Debugger::handleUncaughtException(AutoCompartment &ac, Value *vp, bool callHook)
{
    JSContext *cx = ac.context;
    if (cx->isExceptionPending()) {
        if (callHook && uncaughtExceptionHook) {
            Value fval = ObjectValue(*uncaughtExceptionHook);
            Value exc = cx->getPendingException();
            Value rv;
            cx->clearPendingException();
            if (Invoke(cx, ObjectValue(*object), fval, 1, &exc, &rv))
                return vp ? parseResumptionValue(ac, true, rv, vp, false) : JSTRAP_CONTINUE;
        }

        if (cx->isExceptionPending()) {
            JS_ReportPendingException(cx);
            cx->clearPendingException();
        }
    }
    ac.leave();
    return JSTRAP_ERROR;
}

void
Debugger::resultToCompletion(JSContext *cx, bool ok, const Value &rv,
                             JSTrapStatus *status, Value *value)
{
    JS_ASSERT_IF(ok, !cx->isExceptionPending());

    if (ok) {
        *status = JSTRAP_RETURN;
        *value = rv;
    } else if (cx->isExceptionPending()) {
        *status = JSTRAP_THROW;
        *value = cx->getPendingException();
        cx->clearPendingException();
    } else {
        *status = JSTRAP_ERROR;
        value->setUndefined();
    }
}

bool
Debugger::newCompletionValue(JSContext *cx, JSTrapStatus status, Value value, Value *result)
{
    



    assertSameCompartment(cx, object.get());

    jsid key;

    switch (status) {
      case JSTRAP_RETURN:
        key = ATOM_TO_JSID(cx->runtime->atomState.returnAtom);
        break;

      case JSTRAP_THROW:
        key = ATOM_TO_JSID(cx->runtime->atomState.throwAtom);
        break;

      case JSTRAP_ERROR:
        result->setNull();
        return true;

      default:
        JS_NOT_REACHED("bad status passed to Debugger::newCompletionValue");
    }

    
    JSObject *obj = NewBuiltinClassInstance(cx, &ObjectClass);
    if (!obj ||
        !wrapDebuggeeValue(cx, &value) ||
        !DefineNativeProperty(cx, obj, key, value, JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_ENUMERATE, 0, 0))
    {
        return false;
    }

    result->setObject(*obj);
    return true;
}

bool
Debugger::receiveCompletionValue(AutoCompartment &ac, bool ok, Value val, Value *vp)
{
    JSContext *cx = ac.context;

    JSTrapStatus status;
    Value value;
    resultToCompletion(cx, ok, val, &status, &value);
    ac.leave();
    return newCompletionValue(cx, status, value, vp);
}

JSTrapStatus
Debugger::parseResumptionValue(AutoCompartment &ac, bool ok, const Value &rv, Value *vp,
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
    bool okResumption = rv.isObject();
    if (okResumption) {
        obj = &rv.toObject();
        okResumption = obj->isObject();
    }
    if (okResumption) {
        shape = obj->lastProperty();
        okResumption = shape->previous() &&
             !shape->previous()->previous() &&
             (shape->propid() == returnId || shape->propid() == throwId) &&
             shape->isDataDescriptor();
    }
    if (!okResumption) {
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
    return shape->propid() == returnId ? JSTRAP_RETURN : JSTRAP_THROW;
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
            Invoke(cx, ObjectValue(*obj), fval, argc, argv, rval));
}

JSTrapStatus
Debugger::fireDebuggerStatement(JSContext *cx, Value *vp)
{
    JSObject *hook = getHook(OnDebuggerStatement);
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    
    StackFrame *fp = cx->fp();
    AutoCompartment ac(cx, object);
    if (!ac.enter())
        return JSTRAP_ERROR;

    Value argv[1];
    if (!getScriptFrame(cx, fp, argv))
        return handleUncaughtException(ac, vp, false);

    Value rv;
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, argv, &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

JSTrapStatus
Debugger::fireExceptionUnwind(JSContext *cx, Value *vp)
{
    JSObject *hook = getHook(OnExceptionUnwind);
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    StackFrame *fp = cx->fp();
    Value exc = cx->getPendingException();
    cx->clearPendingException();

    AutoCompartment ac(cx, object);
    if (!ac.enter())
        return JSTRAP_ERROR;

    Value argv[2];
    argv[1] = exc;
    if (!getScriptFrame(cx, fp, &argv[0]) || !wrapDebuggeeValue(cx, &argv[1]))
        return handleUncaughtException(ac, vp, false);

    Value rv;
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 2, argv, &rv);
    JSTrapStatus st = parseResumptionValue(ac, ok, rv, vp);
    if (st == JSTRAP_CONTINUE)
        cx->setPendingException(exc);
    return st;
}

JSTrapStatus
Debugger::fireEnterFrame(JSContext *cx, Value *vp)
{
    JSObject *hook = getHook(OnEnterFrame);
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    StackFrame *fp = cx->fp();
    AutoCompartment ac(cx, object);
    if (!ac.enter())
        return JSTRAP_ERROR;

    Value argv[1];
    if (!getScriptFrame(cx, fp, &argv[0]))
        return handleUncaughtException(ac, vp, false);

    Value rv;
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, argv, &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

void
Debugger::fireNewScript(JSContext *cx, JSScript *script)
{
    JSObject *hook = getHook(OnNewScript);
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    AutoCompartment ac(cx, object);
    if (!ac.enter())
        return;

    JSObject *dsobj = wrapScript(cx, script);
    if (!dsobj) {
        handleUncaughtException(ac, NULL, false);
        return;
    }

    Value argv[1];
    argv[0].setObject(*dsobj);
    Value rv;
    if (!Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, argv, &rv))
        handleUncaughtException(ac, NULL, true);
}

JSTrapStatus
Debugger::dispatchHook(JSContext *cx, Value *vp, Hook which)
{
    JS_ASSERT(which == OnDebuggerStatement || which == OnExceptionUnwind);

    







    AutoValueVector triggered(cx);
    GlobalObject *global = &cx->fp()->scopeChain().global();
    if (GlobalObject::DebuggerVector *debuggers = global->getDebuggers()) {
        for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++) {
            Debugger *dbg = *p;
            if (dbg->enabled && dbg->getHook(which)) {
                if (!triggered.append(ObjectValue(*dbg->toJSObject())))
                    return JSTRAP_ERROR;
            }
        }
    }

    



    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debugger *dbg = Debugger::fromJSObject(&p->toObject());
        if (dbg->debuggees.has(global) && dbg->enabled && dbg->getHook(which)) {
            JSTrapStatus st = (which == OnDebuggerStatement)
                              ? dbg->fireDebuggerStatement(cx, vp)
                              : dbg->fireExceptionUnwind(cx, vp);
            if (st != JSTRAP_CONTINUE)
                return st;
        }
    }
    return JSTRAP_CONTINUE;
}

static bool
AddNewScriptRecipients(GlobalObject::DebuggerVector *src, AutoValueVector *dest)
{
    bool wasEmpty = dest->length() == 0;
    for (Debugger **p = src->begin(); p != src->end(); p++) {
        Debugger *dbg = *p;
        Value v = ObjectValue(*dbg->toJSObject());
        if (dbg->observesNewScript() &&
            (wasEmpty || Find(dest->begin(), dest->end(), v) == dest->end()) &&
            !dest->append(v))
        {
            return false;
        }
    }
    return true;
}

void
Debugger::slowPathOnNewScript(JSContext *cx, JSScript *script, GlobalObject *compileAndGoGlobal)
{
    JS_ASSERT(script->compileAndGo == !!compileAndGoGlobal);

    





    AutoValueVector triggered(cx);
    if (script->compileAndGo) {
        if (GlobalObject::DebuggerVector *debuggers = compileAndGoGlobal->getDebuggers()) {
            if (!AddNewScriptRecipients(debuggers, &triggered))
                return;
        }
    } else {
        GlobalObjectSet &debuggees = script->compartment()->getDebuggees();
        for (GlobalObjectSet::Range r = debuggees.all(); !r.empty(); r.popFront()) {
            if (!AddNewScriptRecipients(r.front()->getDebuggers(), &triggered))
                return;
        }
    }

    



    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debugger *dbg = Debugger::fromJSObject(&p->toObject());
        if ((!compileAndGoGlobal || dbg->debuggees.has(compileAndGoGlobal)) &&
            dbg->enabled && dbg->getHook(OnNewScript)) {
            dbg->fireNewScript(cx, script);
        }
    }
}

JSTrapStatus
Debugger::onTrap(JSContext *cx, Value *vp)
{
    StackFrame *fp = cx->fp();
    JSScript *script = fp->script();
    GlobalObject *scriptGlobal = &fp->scopeChain().global();
    jsbytecode *pc = cx->regs().pc;
    BreakpointSite *site = script->getBreakpointSite(pc);
    JSOp op = JSOp(*pc);

    
    Vector<Breakpoint *> triggered(cx);
    for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = bp->nextInSite()) {
        if (!triggered.append(bp))
            return JSTRAP_ERROR;
    }

    for (Breakpoint **p = triggered.begin(); p != triggered.end(); p++) {
        Breakpoint *bp = *p;

        
        if (!site || !site->hasBreakpoint(bp))
            continue;

        Debugger *dbg = bp->debugger;
        if (dbg->enabled && dbg->debuggees.lookup(scriptGlobal)) {
            AutoCompartment ac(cx, dbg->object);
            if (!ac.enter())
                return JSTRAP_ERROR;

            Value argv[1];
            if (!dbg->getScriptFrame(cx, fp, &argv[0]))
                return dbg->handleUncaughtException(ac, vp, false);
            Value rv;
            bool ok = CallMethodIfPresent(cx, bp->handler, "hit", 1, argv, &rv);
            JSTrapStatus st = dbg->parseResumptionValue(ac, ok, rv, vp, true);
            if (st != JSTRAP_CONTINUE)
                return st;

            
            site = script->getBreakpointSite(pc);
        }
    }

    if (site && site->trapHandler) {
        JSTrapStatus st = site->trapHandler(cx, fp->script(), pc, vp, site->trapClosure);
        if (st != JSTRAP_CONTINUE)
            return st;
    }

    
    vp->setInt32(op);
    return JSTRAP_CONTINUE;
}

JSTrapStatus
Debugger::onSingleStep(JSContext *cx, Value *vp)
{
    StackFrame *fp = cx->fp();

    





    Value exception = UndefinedValue();
    bool exceptionPending = cx->isExceptionPending();
    if (exceptionPending) {
        exception = cx->getPendingException();
        cx->clearPendingException();
    }

    
    JS_ASSERT(fp->isScriptFrame());

    



    AutoObjectVector frames(cx);
    for (FrameRange r(cx, fp); !r.empty(); r.popFront()) {
        JSObject *frame = r.frontFrame();
        if (!frame->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined() &&
            !frames.append(frame))
        {
            return JSTRAP_ERROR;
        }
    }

#ifdef DEBUG
    








    {
        uint32_t stepperCount = 0;
        JSScript *trappingScript = fp->script();
        GlobalObject *global = &fp->scopeChain().global();
        if (GlobalObject::DebuggerVector *debuggers = global->getDebuggers()) {
            for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++) {
                Debugger *dbg = *p;
                for (FrameMap::Range r = dbg->frames.all(); !r.empty(); r.popFront()) {
                    StackFrame *frame = r.front().key;
                    JSObject *frameobj = r.front().value;
                    if (frame->isScriptFrame() &&
                        frame->script() == trappingScript &&
                        !frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined())
                    {
                        stepperCount++;
                    }
                }
            }
        }
        if (trappingScript->compileAndGo)
            JS_ASSERT(stepperCount == trappingScript->stepModeCount());
        else
            JS_ASSERT(stepperCount <= trappingScript->stepModeCount());
    }
#endif

    
    for (JSObject **p = frames.begin(); p != frames.end(); p++) {
        JSObject *frame = *p;
        Debugger *dbg = Debugger::fromChildJSObject(frame);
        AutoCompartment ac(cx, dbg->object);
        if (!ac.enter())
            return JSTRAP_ERROR;
        const Value &handler = frame->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER);
        Value rval;
        bool ok = Invoke(cx, ObjectValue(*frame), handler, 0, NULL, &rval);
        JSTrapStatus st = dbg->parseResumptionValue(ac, ok, rval, vp);
        if (st != JSTRAP_CONTINUE)
            return st;
    }

    vp->setUndefined();
    if (exceptionPending)
        cx->setPendingException(exception);
    return JSTRAP_CONTINUE;
}




void
Debugger::markKeysInCompartment(JSTracer *tracer)
{
    JSCompartment *comp = tracer->runtime->gcCurrentCompartment;
    JS_ASSERT(comp);

    




    typedef HashMap<HeapPtrObject, HeapPtrObject, DefaultHasher<HeapPtrObject>, RuntimeAllocPolicy>
        ObjectMap;
    const ObjectMap &objStorage = objects;
    for (ObjectMap::Range r = objStorage.all(); !r.empty(); r.popFront()) {
        const HeapPtrObject &key = r.front().key;
        if (key->compartment() == comp && IsAboutToBeFinalized(key)) {
            HeapPtrObject tmp(key);
            gc::MarkObject(tracer, &tmp, "cross-compartment WeakMap key");
            JS_ASSERT(tmp == key);
        }
    }

    const ObjectMap &envStorage = environments;
    for (ObjectMap::Range r = envStorage.all(); !r.empty(); r.popFront()) {
        const HeapPtrObject &key = r.front().key;
        if (key->compartment() == comp && IsAboutToBeFinalized(key)) {
            HeapPtrObject tmp(key);
            js::gc::MarkObject(tracer, &tmp, "cross-compartment WeakMap key");
            JS_ASSERT(tmp == key);
        }
    }

    typedef HashMap<HeapPtrScript, HeapPtrObject, DefaultHasher<HeapPtrScript>, RuntimeAllocPolicy>
        ScriptMap;
    const ScriptMap &scriptStorage = scripts;
    for (ScriptMap::Range r = scriptStorage.all(); !r.empty(); r.popFront()) {
        const HeapPtrScript &key = r.front().key;
        if (key->compartment() == comp && IsAboutToBeFinalized(key)) {
            HeapPtrScript tmp(key);
            gc::MarkScript(tracer, &tmp, "cross-compartment WeakMap key");
            JS_ASSERT(tmp == key);
        }
    }
}
























void
Debugger::markCrossCompartmentDebuggerObjectReferents(JSTracer *tracer)
{
    JSRuntime *rt = tracer->runtime;
    JSCompartment *comp = rt->gcCurrentCompartment;

    



    for (JSCList *p = &rt->debuggerList; (p = JS_NEXT_LINK(p)) != &rt->debuggerList;) {
        Debugger *dbg = Debugger::fromLinks(p);
        if (dbg->object->compartment() != comp)
            dbg->markKeysInCompartment(tracer);
    }
}











bool
Debugger::markAllIteratively(GCMarker *trc)
{
    bool markedAny = false;

    



    JSRuntime *rt = trc->runtime;
    JSCompartment *comp = rt->gcCurrentCompartment;
    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++) {
        JSCompartment *dc = *c;

        



        if (comp && dc == comp)
            continue;

        const GlobalObjectSet &debuggees = dc->getDebuggees();
        for (GlobalObjectSet::Range r = debuggees.all(); !r.empty(); r.popFront()) {
            GlobalObject *global = r.front();
            if (IsAboutToBeFinalized(global))
                continue;

            



            const GlobalObject::DebuggerVector *debuggers = global->getDebuggers();
            JS_ASSERT(debuggers);
            for (Debugger * const *p = debuggers->begin(); p != debuggers->end(); p++) {
                Debugger *dbg = *p;

                





                HeapPtrObject &dbgobj = dbg->toJSObjectRef();
                if (comp && comp != dbgobj->compartment())
                    continue;

                bool dbgMarked = !IsAboutToBeFinalized(dbgobj);
                if (!dbgMarked && dbg->hasAnyLiveHooks()) {
                    



                    MarkObject(trc, &dbgobj, "enabled Debugger");
                    markedAny = true;
                    dbgMarked = true;
                }

                if (dbgMarked) {
                    
                    for (Breakpoint *bp = dbg->firstBreakpoint(); bp; bp = bp->nextInDebugger()) {
                        if (!IsAboutToBeFinalized(bp->site->script)) {
                            



                            if (IsAboutToBeFinalized(bp->getHandler())) {
                                MarkObject(trc, &bp->getHandlerRef(), "breakpoint handler");
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
Debugger::traceObject(JSTracer *trc, JSObject *obj)
{
    if (Debugger *dbg = Debugger::fromJSObject(obj))
        dbg->trace(trc);
}

void
Debugger::trace(JSTracer *trc)
{
    if (uncaughtExceptionHook)
        MarkObject(trc, &uncaughtExceptionHook, "hooks");

    







    for (FrameMap::Range r = frames.all(); !r.empty(); r.popFront()) {
        HeapPtrObject &frameobj = r.front().value;
        JS_ASSERT(frameobj->getPrivate());
        MarkObject(trc, &frameobj, "live Debugger.Frame");
    }

    
    scripts.trace(trc);

    
    objects.trace(trc);

    
    environments.trace(trc);
}

void
Debugger::sweepAll(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JS_ASSERT(!rt->gcCurrentCompartment);

    for (JSCList *p = &rt->debuggerList; (p = JS_NEXT_LINK(p)) != &rt->debuggerList;) {
        Debugger *dbg = Debugger::fromLinks(p);

        if (IsAboutToBeFinalized(dbg->object)) {
            








            for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront())
                dbg->removeDebuggeeGlobal(cx, e.front(), NULL, &e);
        }

    }

    for (JSCompartment **c = rt->compartments.begin(); c != rt->compartments.end(); c++) {
        
        GlobalObjectSet &debuggees = (*c)->getDebuggees();
        for (GlobalObjectSet::Enum e(debuggees); !e.empty(); e.popFront()) {
            GlobalObject *global = e.front();
            if (IsAboutToBeFinalized(global))
                detachAllDebuggersFromGlobal(cx, global, &e);
        }
    }
}

void
Debugger::detachAllDebuggersFromGlobal(JSContext *cx, GlobalObject *global,
                                       GlobalObjectSet::Enum *compartmentEnum)
{
    const GlobalObject::DebuggerVector *debuggers = global->getDebuggers();
    JS_ASSERT(!debuggers->empty());
    while (!debuggers->empty())
        debuggers->back()->removeDebuggeeGlobal(cx, global, compartmentEnum, NULL);
}

void
Debugger::finalize(JSContext *cx, JSObject *obj)
{
    Debugger *dbg = fromJSObject(obj);
    if (!dbg)
        return;
    if (!dbg->debuggees.empty()) {
        



        JS_ASSERT(cx->runtime->gcCurrentCompartment == dbg->object->compartment());
        for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront())
            dbg->removeDebuggeeGlobal(cx, e.front(), NULL, &e);
    }
    cx->delete_(dbg);
}

Class Debugger::jsclass = {
    "Debugger",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUG_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Debugger::finalize,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    Debugger::traceObject
};

Debugger *
Debugger::fromThisValue(JSContext *cx, const CallArgs &args, const char *fnname)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &Debugger::jsclass) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger", fnname, thisobj->getClass()->name);
        return NULL;
    }

    




    Debugger *dbg = fromJSObject(thisobj);
    if (!dbg) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger", fnname, "prototype object");
    }
    return dbg;
}

#define THIS_DEBUGGER(cx, argc, vp, fnname, args, dbg)                       \
    CallArgs args = CallArgsFromVp(argc, vp);                                \
    Debugger *dbg = Debugger::fromThisValue(cx, args, fnname);               \
    if (!dbg)                                                                \
        return false

JSBool
Debugger::getEnabled(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "get enabled", args, dbg);
    args.rval().setBoolean(dbg->enabled);
    return true;
}

JSBool
Debugger::setEnabled(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.set enabled", 1);
    THIS_DEBUGGER(cx, argc, vp, "set enabled", args, dbg);
    bool enabled = js_ValueToBoolean(args[0]);

    if (enabled != dbg->enabled) {
        for (Breakpoint *bp = dbg->firstBreakpoint(); bp; bp = bp->nextInDebugger()) {
            if (enabled) {
                if (!bp->site->inc(cx)) {
                    



                    for (Breakpoint *bp2 = dbg->firstBreakpoint();
                         bp2 != bp;
                         bp2 = bp2->nextInDebugger())
                    {
                        bp->site->dec(cx);
                    }
                    return false;
                }
            } else {
                bp->site->dec(cx);
            }
        }
    }

    dbg->enabled = enabled;
    args.rval().setUndefined();
    return true;
}

JSBool
Debugger::getHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which)
{
    JS_ASSERT(which >= 0 && which < HookCount);
    THIS_DEBUGGER(cx, argc, vp, "getHook", args, dbg);
    args.rval() = dbg->object->getReservedSlot(JSSLOT_DEBUG_HOOK_START + which);
    return true;
}

JSBool
Debugger::setHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which)
{
    JS_ASSERT(which >= 0 && which < HookCount);
    REQUIRE_ARGC("Debugger.setHook", 1);
    THIS_DEBUGGER(cx, argc, vp, "setHook", args, dbg);
    const Value &v = args[0];
    if (v.isObject()) {
        if (!v.toObject().isCallable()) {
            js_ReportIsNotFunction(cx, vp, JSV2F_SEARCH_STACK);
            return false;
        }
    } else if (!v.isUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_CALLABLE_OR_UNDEFINED);
        return false;
    }
    dbg->object->setReservedSlot(JSSLOT_DEBUG_HOOK_START + which, v);
    args.rval().setUndefined();
    return true;
}

JSBool
Debugger::getOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnDebuggerStatement);
}

JSBool
Debugger::setOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnDebuggerStatement);
}

JSBool
Debugger::getOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnExceptionUnwind);
}

JSBool
Debugger::setOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnExceptionUnwind);
}

JSBool
Debugger::getOnNewScript(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnNewScript);
}

JSBool
Debugger::setOnNewScript(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnNewScript);
}

JSBool
Debugger::getOnEnterFrame(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnEnterFrame);
}

JSBool
Debugger::setOnEnterFrame(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnEnterFrame);
}

JSBool
Debugger::getUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "get uncaughtExceptionHook", args, dbg);
    args.rval().setObjectOrNull(dbg->uncaughtExceptionHook);
    return true;
}

JSBool
Debugger::setUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.set uncaughtExceptionHook", 1);
    THIS_DEBUGGER(cx, argc, vp, "set uncaughtExceptionHook", args, dbg);
    if (!args[0].isNull() && (!args[0].isObject() || !args[0].toObject().isCallable())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_ASSIGN_FUNCTION_OR_NULL,
                             "uncaughtExceptionHook");
        return false;
    }

    dbg->uncaughtExceptionHook = args[0].toObjectOrNull();
    args.rval().setUndefined();
    return true;
}

JSObject *
Debugger::unwrapDebuggeeArgument(JSContext *cx, const Value &v)
{
    







    JSObject *obj = NonNullObject(cx, v);
    if (obj) {
        if (obj->getClass() == &DebuggerObject_class) {
            Value rv = v;
            if (!unwrapDebuggeeValue(cx, &rv))
                return NULL;
            return &rv.toObject();
        }
        if (IsCrossCompartmentWrapper(obj))
            return &GetProxyPrivate(obj).toObject();
    }
    return obj;
}

JSBool
Debugger::addDebuggee(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.addDebuggee", 1);
    THIS_DEBUGGER(cx, argc, vp, "addDebuggee", args, dbg);
    JSObject *referent = dbg->unwrapDebuggeeArgument(cx, args[0]);
    if (!referent)
        return false;
    GlobalObject *global = &referent->global();
    if (!dbg->addDebuggeeGlobal(cx, global))
        return false;

    Value v = ObjectValue(*referent);
    if (!dbg->wrapDebuggeeValue(cx, &v))
        return false;
    args.rval() = v;
    return true;
}

JSBool
Debugger::removeDebuggee(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.removeDebuggee", 1);
    THIS_DEBUGGER(cx, argc, vp, "removeDebuggee", args, dbg);
    JSObject *referent = dbg->unwrapDebuggeeArgument(cx, args[0]);
    if (!referent)
        return false;
    GlobalObject *global = &referent->global();
    if (dbg->debuggees.has(global))
        dbg->removeDebuggeeGlobal(cx, global, NULL, NULL);
    args.rval().setUndefined();
    return true;
}

JSBool
Debugger::hasDebuggee(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.hasDebuggee", 1);
    THIS_DEBUGGER(cx, argc, vp, "hasDebuggee", args, dbg);
    JSObject *referent = dbg->unwrapDebuggeeArgument(cx, args[0]);
    if (!referent)
        return false;
    args.rval().setBoolean(!!dbg->debuggees.lookup(&referent->global()));
    return true;
}

JSBool
Debugger::getDebuggees(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "getDebuggees", args, dbg);
    JSObject *arrobj = NewDenseAllocatedArray(cx, dbg->debuggees.count(), NULL);
    if (!arrobj)
        return false;
    arrobj->ensureDenseArrayInitializedLength(cx, 0, dbg->debuggees.count());
    unsigned i = 0;
    for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront()) {
        Value v = ObjectValue(*e.front());
        if (!dbg->wrapDebuggeeValue(cx, &v))
            return false;
        arrobj->setDenseArrayElement(i++, v);
    }
    args.rval().setObject(*arrobj);
    return true;
}

JSBool
Debugger::getNewestFrame(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "getNewestFrame", args, dbg);

    



    for (AllFramesIter i(cx->stack.space()); !i.done(); ++i) {
        if (dbg->observesFrame(i.fp()))
            return dbg->getScriptFrame(cx, i.fp(), vp);
    }
    args.rval().setNull();
    return true;
}

JSBool
Debugger::clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "clearAllBreakpoints", args, dbg);
    for (GlobalObjectSet::Range r = dbg->debuggees.all(); !r.empty(); r.popFront())
        r.front()->compartment()->clearBreakpointsIn(cx, dbg, NULL);
    return true;
}

JSBool
Debugger::construct(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    for (unsigned i = 0; i < argc; i++) {
        const Value &arg = args[i];
        if (!arg.isObject())
            return ReportObjectRequired(cx);
        JSObject *argobj = &arg.toObject();
        if (!IsCrossCompartmentWrapper(argobj)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CCW_REQUIRED, "Debugger");
            return false;
        }
    }

    
    Value v;
    if (!args.callee().getProperty(cx, cx->runtime->atomState.classPrototypeAtom, &v))
        return false;
    JSObject *proto = &v.toObject();
    JS_ASSERT(proto->getClass() == &Debugger::jsclass);

    




    JSObject *obj = NewObjectWithGivenProto(cx, &Debugger::jsclass, proto, NULL);
    if (!obj)
        return false;
    for (unsigned slot = JSSLOT_DEBUG_PROTO_START; slot < JSSLOT_DEBUG_PROTO_STOP; slot++)
        obj->setReservedSlot(slot, proto->getReservedSlot(slot));

    Debugger *dbg = cx->new_<Debugger>(cx, obj);
    if (!dbg)
        return false;
    obj->setPrivate(dbg);
    if (!dbg->init(cx)) {
        cx->delete_(dbg);
        return false;
    }

    
    for (unsigned i = 0; i < argc; i++) {
        GlobalObject *debuggee = &GetProxyPrivate(&args[i].toObject()).toObject().global();
        if (!dbg->addDebuggeeGlobal(cx, debuggee))
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

bool
Debugger::addDebuggeeGlobal(JSContext *cx, GlobalObject *global)
{
    if (debuggees.has(global))
        return true;

    JSCompartment *debuggeeCompartment = global->compartment();

    





    Vector<JSCompartment *> visited(cx);
    if (!visited.append(object->compartment()))
        return false;
    for (size_t i = 0; i < visited.length(); i++) {
        JSCompartment *c = visited[i];
        if (c == debuggeeCompartment) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_LOOP);
            return false;
        }

        



        for (GlobalObjectSet::Range r = c->getDebuggees().all(); !r.empty(); r.popFront()) {
            GlobalObject::DebuggerVector *v = r.front()->getDebuggers();
            for (Debugger **p = v->begin(); p != v->end(); p++) {
                JSCompartment *next = (*p)->object->compartment();
                if (Find(visited, next) == visited.end() && !visited.append(next))
                    return false;
            }
        }
    }

    
    if (!debuggeeCompartment->debugMode() && debuggeeCompartment->hasScriptsOnStack()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_IDLE);
        return false;
    }

    



    AutoCompartment ac(cx, global);
    if (!ac.enter())
        return false;
    GlobalObject::DebuggerVector *v = global->getOrCreateDebuggers(cx);
    if (!v || !v->append(this)) {
        js_ReportOutOfMemory(cx);
    } else {
        if (!debuggees.put(global)) {
            js_ReportOutOfMemory(cx);
        } else {
            if (global->getDebuggers()->length() > 1)
                return true;
            if (debuggeeCompartment->addDebuggee(cx, global))
                return true;

            
            debuggees.remove(global);
        }
        JS_ASSERT(v->back() == this);
        v->popBack();
    }
    return false;
}

void
Debugger::removeDebuggeeGlobal(JSContext *cx, GlobalObject *global,
                               GlobalObjectSet::Enum *compartmentEnum,
                               GlobalObjectSet::Enum *debugEnum)
{
    





    JS_ASSERT(global->compartment()->getDebuggees().has(global));
    JS_ASSERT_IF(compartmentEnum, compartmentEnum->front() == global);
    JS_ASSERT(debuggees.has(global));
    JS_ASSERT_IF(debugEnum, debugEnum->front() == global);

    








    for (FrameMap::Enum e(frames); !e.empty(); e.popFront()) {
        StackFrame *fp = e.front().key;
        if (&fp->scopeChain().global() == global) {
            e.front().value->setPrivate(NULL);
            e.removeFront();
        }
    }

    GlobalObject::DebuggerVector *v = global->getDebuggers();
    Debugger **p;
    for (p = v->begin(); p != v->end(); p++) {
        if (*p == this)
            break;
    }
    JS_ASSERT(p != v->end());

    



    v->erase(p);
    if (v->empty())
        global->compartment()->removeDebuggee(cx, global, compartmentEnum);
    if (debugEnum)
        debugEnum->removeFront();
    else
        debuggees.remove(global);
}

JSPropertySpec Debugger::properties[] = {
    JS_PSGS("enabled", Debugger::getEnabled, Debugger::setEnabled, 0),
    JS_PSGS("onDebuggerStatement", Debugger::getOnDebuggerStatement,
            Debugger::setOnDebuggerStatement, 0),
    JS_PSGS("onExceptionUnwind", Debugger::getOnExceptionUnwind,
            Debugger::setOnExceptionUnwind, 0),
    JS_PSGS("onNewScript", Debugger::getOnNewScript, Debugger::setOnNewScript, 0),
    JS_PSGS("onEnterFrame", Debugger::getOnEnterFrame, Debugger::setOnEnterFrame, 0),
    JS_PSGS("uncaughtExceptionHook", Debugger::getUncaughtExceptionHook,
            Debugger::setUncaughtExceptionHook, 0),
    JS_PS_END
};

JSFunctionSpec Debugger::methods[] = {
    JS_FN("addDebuggee", Debugger::addDebuggee, 1, 0),
    JS_FN("removeDebuggee", Debugger::removeDebuggee, 1, 0),
    JS_FN("hasDebuggee", Debugger::hasDebuggee, 1, 0),
    JS_FN("getDebuggees", Debugger::getDebuggees, 0, 0),
    JS_FN("getNewestFrame", Debugger::getNewestFrame, 0, 0),
    JS_FN("clearAllBreakpoints", Debugger::clearAllBreakpoints, 1, 0),
    JS_FS_END
};




static inline JSScript *
GetScriptReferent(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &DebuggerScript_class);
    return static_cast<JSScript *>(obj->getPrivate());
}

static void
DebuggerScript_trace(JSTracer *trc, JSObject *obj)
{
    if (!trc->runtime->gcCurrentCompartment) {
        
        if (JSScript *script = GetScriptReferent(obj))
            MarkScriptUnbarriered(trc, script, "Debugger.Script referent");

    }
}

Class DebuggerScript_class = {
    "Script",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGSCRIPT_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    DebuggerScript_trace
};

JSObject *
Debugger::newDebuggerScript(JSContext *cx, JSScript *script)
{
    assertSameCompartment(cx, object.get());

    JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_SCRIPT_PROTO).toObject();
    JS_ASSERT(proto);
    JSObject *scriptobj = NewObjectWithGivenProto(cx, &DebuggerScript_class, proto, NULL);
    if (!scriptobj)
        return NULL;
    scriptobj->setReservedSlot(JSSLOT_DEBUGSCRIPT_OWNER, ObjectValue(*object));
    scriptobj->setPrivate(script);

    return scriptobj;
}

JSObject *
Debugger::wrapScript(JSContext *cx, JSScript *script)
{
    assertSameCompartment(cx, object.get());
    JS_ASSERT(cx->compartment != script->compartment());
    ScriptWeakMap::AddPtr p = scripts.lookupForAdd(script);
    if (!p) {
        JSObject *scriptobj = newDebuggerScript(cx, script);

        
        if (!scriptobj || !scripts.relookupOrAdd(p, script, scriptobj))
            return NULL;
    }

    JS_ASSERT(GetScriptReferent(p->value) == script);
    return p->value;
}

static JSObject *
DebuggerScript_check(JSContext *cx, const Value &v, const char *clsname, const char *fnname)
{
    if (!v.isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &v.toObject();
    if (thisobj->getClass() != &DebuggerScript_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             clsname, fnname, thisobj->getClass()->name);
        return NULL;
    }

    



    if (!GetScriptReferent(thisobj)) {
        JS_ASSERT(!GetScriptReferent(thisobj));
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             clsname, fnname, "prototype object");
        return NULL;
    }

    return thisobj;
}

static JSObject *
DebuggerScript_checkThis(JSContext *cx, const CallArgs &args, const char *fnname)
{
    return DebuggerScript_check(cx, args.thisv(), "Debugger.Script", fnname);
}

#define THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, fnname, args, obj, script)            \
    CallArgs args = CallArgsFromVp(argc, vp);                                       \
    JSObject *obj = DebuggerScript_checkThis(cx, args, fnname);                     \
    if (!obj)                                                                       \
        return false;                                                               \
    JSScript *script = GetScriptReferent(obj)

static JSBool
DebuggerScript_getUrl(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getUrl", args, obj, script);

    JSString *str = js_NewStringCopyZ(cx, script->filename);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static JSBool
DebuggerScript_getStartLine(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getStartLine", args, obj, script);
    args.rval().setNumber(script->lineno);
    return true;
}

static JSBool
DebuggerScript_getLineCount(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getLineCount", args, obj, script);

    unsigned maxLine = js_GetScriptLineExtent(script);
    args.rval().setNumber(double(maxLine));
    return true;
}

static JSBool
DebuggerScript_getChildScripts(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getChildScripts", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    JSObject *result = NewDenseEmptyArray(cx);
    if (!result)
        return false;
    if (JSScript::isValidOffset(script->objectsOffset)) {
        




        JSObjectArray *objects = script->objects();
        for (uint32_t i = script->savedCallerFun ? 1 : 0; i < objects->length; i++) {
            JSObject *obj = objects->vector[i];
            if (obj->isFunction()) {
                JSFunction *fun = static_cast<JSFunction *>(obj);
                JSObject *s = dbg->wrapScript(cx, fun->script());
                if (!s || !js_NewbornArrayPush(cx, result, ObjectValue(*s)))
                    return false;
            }
        }
    }
    args.rval().setObject(*result);
    return true;
}

static bool
ScriptOffset(JSContext *cx, JSScript *script, const Value &v, size_t *offsetp)
{
    double d;
    size_t off;

    bool ok = v.isNumber();
    if (ok) {
        d = v.toNumber();
        off = size_t(d);
    }
    if (!ok || off != d || !IsValidBytecodeOffset(cx, script, off)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_BAD_OFFSET);
        return false;
    }
    *offsetp = off;
    return true;
}

static JSBool
DebuggerScript_getOffsetLine(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.Script.getOffsetLine", 1);
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getOffsetLine", args, obj, script);
    size_t offset;
    if (!ScriptOffset(cx, script, args[0], &offset))
        return false;
    unsigned lineno = JS_PCToLineNumber(cx, script, script->code + offset);
    args.rval().setNumber(lineno);
    return true;
}

class BytecodeRangeWithLineNumbers : private BytecodeRange
{
  public:
    using BytecodeRange::empty;
    using BytecodeRange::frontPC;
    using BytecodeRange::frontOpcode;
    using BytecodeRange::frontOffset;

    BytecodeRangeWithLineNumbers(JSScript *script)
      : BytecodeRange(script), lineno(script->lineno), sn(script->notes()), snpc(script->code)
    {
        if (!SN_IS_TERMINATOR(sn))
            snpc += SN_DELTA(sn);
        updateLine();
    }

    void popFront() {
        BytecodeRange::popFront();
        if (!empty())
            updateLine();
    }

    size_t frontLineNumber() const { return lineno; }

  private:
    void updateLine() {
        



        while (!SN_IS_TERMINATOR(sn) && snpc <= frontPC()) {
            SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
            if (type == SRC_SETLINE)
                lineno = size_t(js_GetSrcNoteOffset(sn, 0));
            else if (type == SRC_NEWLINE)
                lineno++;

            sn = SN_NEXT(sn);
            snpc += SN_DELTA(sn);
        }
    }

    size_t lineno;
    jssrcnote *sn;
    jsbytecode *snpc;
};

static const size_t NoEdges = -1;
static const size_t MultipleEdges = -2;




















class FlowGraphSummary : public Vector<size_t> {
  public:
    typedef Vector<size_t> Base;
    FlowGraphSummary(JSContext *cx) : Base(cx) {}

    void addEdge(size_t sourceLine, size_t targetOffset) {
        FlowGraphSummary &self = *this;
        if (self[targetOffset] == NoEdges)
            self[targetOffset] = sourceLine;
        else if (self[targetOffset] != sourceLine)
            self[targetOffset] = MultipleEdges;
    }

    void addEdgeFromAnywhere(size_t targetOffset) {
        (*this)[targetOffset] = MultipleEdges;
    }

    bool populate(JSContext *cx, JSScript *script) {
        if (!growBy(script->length))
            return false;
        FlowGraphSummary &self = *this;
        self[0] = MultipleEdges;
        for (size_t i = 1; i < script->length; i++)
            self[i] = NoEdges;

        size_t prevLine = script->lineno;
        JSOp prevOp = JSOP_NOP;
        for (BytecodeRangeWithLineNumbers r(script); !r.empty(); r.popFront()) {
            size_t lineno = r.frontLineNumber();
            JSOp op = r.frontOpcode();

            if (FlowsIntoNext(prevOp))
                addEdge(prevLine, r.frontOffset());

            if (js_CodeSpec[op].type() == JOF_JUMP) {
                addEdge(lineno, r.frontOffset() + GET_JUMP_OFFSET(r.frontPC()));
            } else if (op == JSOP_TABLESWITCH || op == JSOP_LOOKUPSWITCH) {
                jsbytecode *pc = r.frontPC();
                size_t offset = r.frontOffset();
                ptrdiff_t step = JUMP_OFFSET_LEN;
                size_t defaultOffset = offset + GET_JUMP_OFFSET(pc);
                pc += step;
                addEdge(lineno, defaultOffset);

                int ncases;
                if (op == JSOP_TABLESWITCH) {
                    int32_t low = GET_JUMP_OFFSET(pc);
                    pc += JUMP_OFFSET_LEN;
                    ncases = GET_JUMP_OFFSET(pc) - low + 1;
                    pc += JUMP_OFFSET_LEN;
                } else {
                    ncases = GET_UINT16(pc);
                    pc += UINT16_LEN;
                    JS_ASSERT(ncases > 0);
                }

                for (int i = 0; i < ncases; i++) {
                    if (op == JSOP_LOOKUPSWITCH)
                        pc += UINT32_INDEX_LEN;
                    size_t target = offset + GET_JUMP_OFFSET(pc);
                    addEdge(lineno, target);
                    pc += step;
                }
            }

            prevOp = op;
            prevLine = lineno;
        }

        return true;
    }
};

static JSBool
DebuggerScript_getAllOffsets(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getAllOffsets", args, obj, script);

    



    FlowGraphSummary flowData(cx);
    if (!flowData.populate(cx, script))
        return false;

    
    JSObject *result = NewDenseEmptyArray(cx);
    if (!result)
        return false;
    for (BytecodeRangeWithLineNumbers r(script); !r.empty(); r.popFront()) {
        size_t offset = r.frontOffset();
        size_t lineno = r.frontLineNumber();

        
        if (flowData[offset] != NoEdges && flowData[offset] != lineno) {
            
            JSObject *offsets;
            Value offsetsv;
            if (!result->arrayGetOwnDataElement(cx, lineno, &offsetsv))
                return false;

            jsid id;
            if (offsetsv.isObject()) {
                offsets = &offsetsv.toObject();
            } else {
                JS_ASSERT(offsetsv.isMagic(JS_ARRAY_HOLE));

                



                offsets = NewDenseEmptyArray(cx);
                if (!offsets ||
                    !ValueToId(cx, NumberValue(lineno), &id) ||
                    !result->defineGeneric(cx, id, ObjectValue(*offsets)))
                {
                    return false;
                }
            }

            
            if (!js_NewbornArrayPush(cx, offsets, NumberValue(offset)))
                return false;
        }
    }

    args.rval().setObject(*result);
    return true;
}

static JSBool
DebuggerScript_getLineOffsets(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getLineOffsets", args, obj, script);
    REQUIRE_ARGC("Debugger.Script.getLineOffsets", 1);

    
    size_t lineno;
    bool ok = false;
    if (args[0].isNumber()) {
        double d = args[0].toNumber();
        lineno = size_t(d);
        ok = (lineno == d);
    }
    if (!ok) {
        JS_ReportErrorNumber(cx,  js_GetErrorMessage, NULL, JSMSG_DEBUG_BAD_LINE);
        return false;
    }

    



    FlowGraphSummary flowData(cx);
    if (!flowData.populate(cx, script))
        return false;

    
    JSObject *result = NewDenseEmptyArray(cx);
    if (!result)
        return false;
    for (BytecodeRangeWithLineNumbers r(script); !r.empty(); r.popFront()) {
        size_t offset = r.frontOffset();

        
        if (r.frontLineNumber() == lineno &&
            flowData[offset] != NoEdges &&
            flowData[offset] != lineno)
        {
            if (!js_NewbornArrayPush(cx, result, NumberValue(offset)))
                return false;
        }
    }

    args.rval().setObject(*result);
    return true;
}

static JSBool
DebuggerScript_setBreakpoint(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.Script.setBreakpoint", 2);
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "setBreakpoint", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    GlobalObject *scriptGlobal = script->getGlobalObjectOrNull();
    if (!dbg->observesGlobal(ScriptGlobal(cx, script, scriptGlobal))) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_DEBUGGING);
        return false;
    }

    size_t offset;
    if (!ScriptOffset(cx, script, args[0], &offset))
        return false;

    JSObject *handler = NonNullObject(cx, args[1]);
    if (!handler)
        return false;

    jsbytecode *pc = script->code + offset;
    BreakpointSite *site = script->getOrCreateBreakpointSite(cx, pc, scriptGlobal);
    if (!site)
        return false;
    if (site->inc(cx)) {
        if (cx->runtime->new_<Breakpoint>(dbg, site, handler)) {
            args.rval().setUndefined();
            return true;
        }
        site->dec(cx);
    }
    site->destroyIfEmpty(cx->runtime);
    return false;
}

static JSBool
DebuggerScript_getBreakpoints(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getBreakpoints", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    jsbytecode *pc;
    if (argc > 0) {
        size_t offset;
        if (!ScriptOffset(cx, script, args[0], &offset))
            return false;
        pc = script->code + offset;
    } else {
        pc = NULL;
    }

    JSObject *arr = NewDenseEmptyArray(cx);
    if (!arr)
        return false;

    for (unsigned i = 0; i < script->length; i++) {
        BreakpointSite *site = script->getBreakpointSite(script->code + i);
        if (site && (!pc || site->pc == pc)) {
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = bp->nextInSite()) {
                if (bp->debugger == dbg &&
                    !js_NewbornArrayPush(cx, arr, ObjectValue(*bp->getHandler())))
                {
                    return false;
                }
            }
        }
    }
    args.rval().setObject(*arr);
    return true;
}

static JSBool
DebuggerScript_clearBreakpoint(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.Script.clearBreakpoint", 1);
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "clearBreakpoint", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    JSObject *handler = NonNullObject(cx, args[0]);
    if (!handler)
        return false;

    script->clearBreakpointsIn(cx, dbg, handler);
    args.rval().setUndefined();
    return true;
}

static JSBool
DebuggerScript_clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "clearAllBreakpoints", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);
    script->clearBreakpointsIn(cx, dbg, NULL);
    args.rval().setUndefined();
    return true;
}

static JSBool
DebuggerScript_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debugger.Script");
    return false;
}

static JSPropertySpec DebuggerScript_properties[] = {
    JS_PSG("url", DebuggerScript_getUrl, 0),
    JS_PSG("startLine", DebuggerScript_getStartLine, 0),
    JS_PSG("lineCount", DebuggerScript_getLineCount, 0),
    JS_PS_END
};

static JSFunctionSpec DebuggerScript_methods[] = {
    JS_FN("getChildScripts", DebuggerScript_getChildScripts, 0, 0),
    JS_FN("getAllOffsets", DebuggerScript_getAllOffsets, 0, 0),
    JS_FN("getLineOffsets", DebuggerScript_getLineOffsets, 1, 0),
    JS_FN("getOffsetLine", DebuggerScript_getOffsetLine, 0, 0),
    JS_FN("setBreakpoint", DebuggerScript_setBreakpoint, 2, 0),
    JS_FN("getBreakpoints", DebuggerScript_getBreakpoints, 1, 0),
    JS_FN("clearBreakpoint", DebuggerScript_clearBreakpoint, 1, 0),
    JS_FN("clearAllBreakpoints", DebuggerScript_clearAllBreakpoints, 0, 0),
    JS_FS_END
};




Class DebuggerFrame_class = {
    "Frame", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGFRAME_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

static JSObject *
CheckThisFrame(JSContext *cx, const CallArgs &args, const char *fnname, bool checkLive)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerFrame_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Frame", fnname, thisobj->getClass()->name);
        return NULL;
    }

    





    if (!thisobj->getPrivate()) {
        if (thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_OWNER).isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                                 "Debugger.Frame", fnname, "prototype object");
            return NULL;
        }
        if (checkLive) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_LIVE,
                                 "Debugger.Frame", fnname, "stack frame");
            return NULL;
        }
    }
    return thisobj;
}

#if DEBUG
static bool
StackContains(JSContext *cx, StackFrame *fp)
{
    for (AllFramesIter i(cx->stack.space()); !i.done(); ++i) {
        if (fp == i.fp())
            return true;
    }
    return false;
}
#endif

#define THIS_FRAME(cx, argc, vp, fnname, args, thisobj, fp)                  \
    CallArgs args = CallArgsFromVp(argc, vp);                                \
    JSObject *thisobj = CheckThisFrame(cx, args, fnname, true);              \
    if (!thisobj)                                                            \
        return false;                                                        \
    StackFrame *fp = (StackFrame *) thisobj->getPrivate();                   \
    JS_ASSERT(StackContains(cx, fp))

#define THIS_FRAME_OWNER(cx, argc, vp, fnname, args, thisobj, fp, dbg)       \
    THIS_FRAME(cx, argc, vp, fnname, args, thisobj, fp);                     \
    Debugger *dbg = Debugger::fromChildJSObject(thisobj)

static JSBool
DebuggerFrame_getType(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get type", args, thisobj, fp);

    



    args.rval().setString(fp->isEvalFrame()
                          ? cx->runtime->atomState.evalAtom
                          : fp->isGlobalFrame()
                          ? cx->runtime->atomState.globalAtom
                          : cx->runtime->atomState.callAtom);
    return true;
}

static Env *
Frame_GetEnv(JSContext *cx, StackFrame *fp)
{
    assertSameCompartment(cx, fp);
    if (fp->isNonEvalFunctionFrame() && !fp->hasCallObj() && !CallObject::createForFunction(cx, fp))
        return NULL;
    return GetScopeChain(cx, fp);
}

static JSBool
DebuggerFrame_getEnvironment(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_OWNER(cx, argc, vp, "get environment", args, thisobj, fp, dbg);

    Env *env;
    {
        AutoCompartment ac(cx, &fp->scopeChain());
        if (!ac.enter())
            return false;
        env = Frame_GetEnv(cx, fp);
        if (!env)
            return false;
    }

    return dbg->wrapEnvironment(cx, env, &args.rval());
}

static JSBool
DebuggerFrame_getCallee(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get callee", args, thisobj, fp);
    Value calleev = (fp->isFunctionFrame() && !fp->isEvalFrame()) ? fp->calleev() : NullValue();
    if (!Debugger::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, &calleev))
        return false;
    args.rval() = calleev;
    return true;
}

static JSBool
DebuggerFrame_getGenerator(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get generator", args, thisobj, fp);
    args.rval().setBoolean(fp->isGeneratorFrame());
    return true;
}

static JSBool
DebuggerFrame_getConstructing(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get constructing", args, thisobj, fp);
    args.rval().setBoolean(fp->isFunctionFrame() && fp->isConstructing());
    return true;
}

static JSBool
DebuggerFrame_getThis(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get this", args, thisobj, fp);
    Value thisv;
    {
        AutoCompartment ac(cx, &fp->scopeChain());
        if (!ac.enter())
            return false;
        if (!ComputeThis(cx, fp))
            return false;
        thisv = fp->thisValue();
    }
    if (!Debugger::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, &thisv))
        return false;
    args.rval() = thisv;
    return true;
}

static JSBool
DebuggerFrame_getOlder(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get this", args, thisobj, thisfp);
    Debugger *dbg = Debugger::fromChildJSObject(thisobj);
    for (StackFrame *fp = thisfp->prev(); fp; fp = fp->prev()) {
        if (dbg->observesFrame(fp))
            return dbg->getScriptFrame(cx, fp, vp);
    }
    args.rval().setNull();
    return true;
}

Class DebuggerArguments_class = {
    "Arguments", JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGARGUMENTS_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};


static JSBool
DebuggerArguments_getArg(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    int32_t i = args.callee().toFunction()->getExtendedSlot(0).toInt32();

    
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return false;
    }
    JSObject *argsobj = &args.thisv().toObject();
    if (argsobj->getClass() != &DebuggerArguments_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Arguments", "getArgument", argsobj->getClass()->name);
        return false;
    }

    



    args.thisv() = argsobj->getReservedSlot(JSSLOT_DEBUGARGUMENTS_FRAME);
    THIS_FRAME(cx, argc, vp, "get argument", ca2, thisobj, fp);

    



    JS_ASSERT(i >= 0);
    Value arg;
    if (unsigned(i) < fp->numActualArgs())
        arg = fp->canonicalActualArg(i);
    else
        arg.setUndefined();

    if (!Debugger::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, &arg))
        return false;
    args.rval() = arg;
    return true;
}

static JSBool
DebuggerFrame_getArguments(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get arguments", args, thisobj, fp);
    Value argumentsv = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ARGUMENTS);
    if (!argumentsv.isUndefined()) {
        JS_ASSERT(argumentsv.isObjectOrNull());
        args.rval() = argumentsv;
        return true;
    }

    JSObject *argsobj;
    if (fp->hasArgs()) {
        
        RootedVar<GlobalObject*> global(cx);
        global = &args.callee().global();
        JSObject *proto = global->getOrCreateArrayPrototype(cx);
        if (!proto)
            return false;
        argsobj = NewObjectWithGivenProto(cx, &DebuggerArguments_class, proto, global);
        if (!argsobj)
            return false;
        SetReservedSlot(argsobj, JSSLOT_DEBUGARGUMENTS_FRAME, ObjectValue(*thisobj));

        JS_ASSERT(fp->numActualArgs() <= 0x7fffffff);
        int32_t fargc = int32_t(fp->numActualArgs());
        if (!DefineNativeProperty(cx, argsobj, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom),
                                  Int32Value(fargc), NULL, NULL,
                                  JSPROP_PERMANENT | JSPROP_READONLY, 0, 0))
        {
            return false;
        }

        for (int32_t i = 0; i < fargc; i++) {
            JSFunction *getobj =
                js_NewFunction(cx, NULL, DebuggerArguments_getArg, 0, 0, global, NULL,
                               JSFunction::ExtendedFinalizeKind);
            if (!getobj ||
                !DefineNativeProperty(cx, argsobj, INT_TO_JSID(i), UndefinedValue(),
                                      JS_DATA_TO_FUNC_PTR(PropertyOp, getobj), NULL,
                                      JSPROP_ENUMERATE | JSPROP_SHARED | JSPROP_GETTER, 0, 0))
            {
                return false;
            }
            getobj->setExtendedSlot(0, Int32Value(i));
        }
    } else {
        argsobj = NULL;
    }
    args.rval() = ObjectOrNullValue(argsobj);
    thisobj->setReservedSlot(JSSLOT_DEBUGFRAME_ARGUMENTS, args.rval());
    return true;
}

static JSBool
DebuggerFrame_getScript(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get script", args, thisobj, fp);
    Debugger *debug = Debugger::fromChildJSObject(thisobj);

    JSObject *scriptObject = NULL;
    if (fp->isFunctionFrame() && !fp->isEvalFrame()) {
        JSFunction *callee = fp->callee().toFunction();
        if (callee->isInterpreted()) {
            scriptObject = debug->wrapScript(cx, callee->script());
            if (!scriptObject)
                return false;
        }
    } else if (fp->isScriptFrame()) {
        



        JSScript *script = fp->script();
        scriptObject = debug->wrapScript(cx, script);
        if (!scriptObject)
            return false;
    }
    args.rval().setObjectOrNull(scriptObject);
    return true;
}

static JSBool
DebuggerFrame_getOffset(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get offset", args, thisobj, fp);
    if (fp->isScriptFrame()) {
        JSScript *script = fp->script();
        jsbytecode *pc = fp->pcQuadratic(cx);
        JS_ASSERT(script->code <= pc);
        JS_ASSERT(pc < script->code + script->length);
        size_t offset = pc - script->code;
        args.rval().setNumber(double(offset));
    } else {
        args.rval().setUndefined();
    }
    return true;
}

static JSBool
DebuggerFrame_getLive(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject *thisobj = CheckThisFrame(cx, args, "get live", false);
    if (!thisobj)
        return false;
    StackFrame *fp = (StackFrame *) thisobj->getPrivate();
    args.rval().setBoolean(!!fp);
    return true;
}

static bool
IsValidHook(const Value &v)
{
    return v.isUndefined() || (v.isObject() && v.toObject().isCallable());
}

static JSBool
DebuggerFrame_getOnStep(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get onStep", args, thisobj, fp);
    (void) fp;  
    Value handler = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER);
    JS_ASSERT(IsValidHook(handler));
    args.rval() = handler;
    return true;
}

static JSBool
DebuggerFrame_setOnStep(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.Frame.set onStep", 1);
    THIS_FRAME(cx, argc, vp, "set onStep", args, thisobj, fp);
    if (!fp->isScriptFrame()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_SCRIPT_FRAME);
        return false;
    }
    if (!IsValidHook(args[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_CALLABLE_OR_UNDEFINED);
        return false;
    }

    Value prior = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER);
    int delta = !args[0].isUndefined() - !prior.isUndefined();
    if (delta != 0) {
        
        AutoCompartment ac(cx, &fp->scopeChain());
        if (!ac.enter())
            return false;
        if (!fp->script()->changeStepModeCount(cx, delta))
            return false;
    }

    
    thisobj->setReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER, args[0]);
    args.rval().setUndefined();
    return true;
}

static JSBool
DebuggerFrame_getOnPop(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get onPop", args, thisobj, fp);
    (void) fp;  
    Value handler = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER);
    JS_ASSERT(IsValidHook(handler));
    args.rval() = handler;
    return true;
}

static JSBool
DebuggerFrame_setOnPop(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.Frame.set onPop", 1);
    THIS_FRAME(cx, argc, vp, "set onPop", args, thisobj, fp);
    if (!fp->isScriptFrame()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NOT_SCRIPT_FRAME);
        return false;
    }
    if (!IsValidHook(args[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_CALLABLE_OR_UNDEFINED);
        return false;
    }

    thisobj->setReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER, args[0]);
    args.rval().setUndefined();
    return true;
}

namespace js {

JSBool
EvaluateInEnv(JSContext *cx, Env *env, StackFrame *fp, const jschar *chars,
              unsigned length, const char *filename, unsigned lineno, Value *rval)
{
    assertSameCompartment(cx, env, fp);

    if (fp) {
        
        if (!ComputeThis(cx, fp))
            return false;
    }

    





    JSPrincipals *prin = fp->scopeChain().principals(cx);
    JSScript *script = frontend::CompileScript(cx, env, fp, prin, prin,
                                               TCF_COMPILE_N_GO | TCF_NEED_SCRIPT_GLOBAL,
                                               chars, length, filename, lineno,
                                               cx->findVersion(), NULL,
                                               UpvarCookie::UPVAR_LEVEL_LIMIT);

    if (!script)
        return false;

    return ExecuteKernel(cx, script, *env, fp->thisValue(), EXECUTE_DEBUG, fp, rval);
}

}

enum EvalBindingsMode { WithoutBindings, WithBindings };

static JSBool
DebuggerFrameEval(JSContext *cx, unsigned argc, Value *vp, EvalBindingsMode mode)
{
    if (mode == WithBindings)
        REQUIRE_ARGC("Debugger.Frame.evalWithBindings", 2);
    else
        REQUIRE_ARGC("Debugger.Frame.eval", 1);
    THIS_FRAME(cx, argc, vp, mode == WithBindings ? "evalWithBindings" : "eval",
               args, thisobj, fp);
    Debugger *dbg = Debugger::fromChildJSObject(thisobj);

    
    if (!args[0].isString()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_EXPECTED_TYPE,
                             "Debugger.Frame.eval", "string", InformalValueTypeName(args[0]));
        return false;
    }
    JSLinearString *linearStr = args[0].toString()->ensureLinear(cx);
    if (!linearStr)
        return false;

    




    AutoIdVector keys(cx);
    AutoValueVector values(cx);
    if (mode == WithBindings) {
        JSObject *bindingsobj = NonNullObject(cx, args[1]);
        if (!bindingsobj ||
            !GetPropertyNames(cx, bindingsobj, JSITER_OWNONLY, &keys) ||
            !values.growBy(keys.length()))
        {
            return false;
        }
        for (size_t i = 0; i < keys.length(); i++) {
            Value *valp = &values[i];
            if (!bindingsobj->getGeneric(cx, bindingsobj, keys[i], valp) ||
                !dbg->unwrapDebuggeeValue(cx, valp))
            {
                return false;
            }
        }
    }

    AutoCompartment ac(cx, &fp->scopeChain());
    if (!ac.enter())
        return false;

    Env *env = JS_GetFrameScopeChain(cx, Jsvalify(fp));
    if (!env)
        return false;

    
    if (mode == WithBindings) {
        
        env = NewObjectWithGivenProto(cx, &ObjectClass, NULL, env);
        if (!env)
            return false;
        for (size_t i = 0; i < keys.length(); i++) {
            if (!cx->compartment->wrap(cx, &values[i]) ||
                !DefineNativeProperty(cx, env, keys[i], values[i], NULL, NULL, 0, 0, 0))
            {
                return false;
            }
        }
    }

    
    Value rval;
    JS::Anchor<JSString *> anchor(linearStr);
    bool ok = EvaluateInEnv(cx, env, fp, linearStr->chars(), linearStr->length(),
                            "debugger eval code", 1, &rval);
    return dbg->receiveCompletionValue(ac, ok, rval, vp);
}

static JSBool
DebuggerFrame_eval(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerFrameEval(cx, argc, vp, WithoutBindings);
}

static JSBool
DebuggerFrame_evalWithBindings(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerFrameEval(cx, argc, vp, WithBindings);
}

static JSBool
DebuggerFrame_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debugger.Frame");
    return false;
}

static JSPropertySpec DebuggerFrame_properties[] = {
    JS_PSG("arguments", DebuggerFrame_getArguments, 0),
    JS_PSG("callee", DebuggerFrame_getCallee, 0),
    JS_PSG("constructing", DebuggerFrame_getConstructing, 0),
    JS_PSG("environment", DebuggerFrame_getEnvironment, 0),
    JS_PSG("generator", DebuggerFrame_getGenerator, 0),
    JS_PSG("live", DebuggerFrame_getLive, 0),
    JS_PSG("offset", DebuggerFrame_getOffset, 0),
    JS_PSG("older", DebuggerFrame_getOlder, 0),
    JS_PSG("script", DebuggerFrame_getScript, 0),
    JS_PSG("this", DebuggerFrame_getThis, 0),
    JS_PSG("type", DebuggerFrame_getType, 0),
    JS_PSGS("onStep", DebuggerFrame_getOnStep, DebuggerFrame_setOnStep, 0),
    JS_PSGS("onPop", DebuggerFrame_getOnPop, DebuggerFrame_setOnPop, 0),
    JS_PS_END
};

static JSFunctionSpec DebuggerFrame_methods[] = {
    JS_FN("eval", DebuggerFrame_eval, 1, 0),
    JS_FN("evalWithBindings", DebuggerFrame_evalWithBindings, 1, 0),
    JS_FS_END
};




static void
DebuggerObject_trace(JSTracer *trc, JSObject *obj)
{
    if (!trc->runtime->gcCurrentCompartment) {
        



        if (JSObject *referent = (JSObject *) obj->getPrivate())
            MarkObjectUnbarriered(trc, referent, "Debugger.Object referent");
    }
}

Class DebuggerObject_class = {
    "Object",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGOBJECT_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    DebuggerObject_trace
};

static JSObject *
DebuggerObject_checkThis(JSContext *cx, const CallArgs &args, const char *fnname)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerObject_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Object", fnname, thisobj->getClass()->name);
        return NULL;
    }

    




    if (!thisobj->getPrivate()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Object", fnname, "prototype object");
        return NULL;
    }
    return thisobj;
}

#define THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, fnname, args, obj)            \
    CallArgs args = CallArgsFromVp(argc, vp);                                 \
    JSObject *obj = DebuggerObject_checkThis(cx, args, fnname);               \
    if (!obj)                                                                 \
        return false;                                                         \
    obj = (JSObject *) obj->getPrivate();                                     \
    JS_ASSERT(obj)

#define THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, fnname, args, dbg, obj) \
    CallArgs args = CallArgsFromVp(argc, vp);                                 \
    JSObject *obj = DebuggerObject_checkThis(cx, args, fnname);               \
    if (!obj)                                                                 \
        return false;                                                         \
    Debugger *dbg = Debugger::fromChildJSObject(obj);                         \
    obj = (JSObject *) obj->getPrivate();                                     \
    JS_ASSERT(obj)

static JSBool
DebuggerObject_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debugger.Object");
    return false;
}

static JSBool
DebuggerObject_getProto(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get proto", args, dbg, refobj);
    Value protov = ObjectOrNullValue(refobj->getProto());
    if (!dbg->wrapDebuggeeValue(cx, &protov))
        return false;
    args.rval() = protov;
    return true;
}

static JSBool
DebuggerObject_getClass(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get class", args, refobj);
    const char *s = refobj->getClass()->name;
    JSAtom *str = js_Atomize(cx, s, strlen(s));
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static JSBool
DebuggerObject_getCallable(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get callable", args, refobj);
    args.rval().setBoolean(refobj->isCallable());
    return true;
}

static JSBool
DebuggerObject_getName(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get name", args, dbg, obj);
    if (!obj->isFunction()) {
        args.rval().setUndefined();
        return true;
    }

    JSString *name = obj->toFunction()->atom;
    if (!name) {
        args.rval().setUndefined();
        return true;
    }

    Value namev = StringValue(name);
    if (!dbg->wrapDebuggeeValue(cx, &namev))
        return false;
    args.rval() = namev;
    return true;
}

static JSBool
DebuggerObject_getParameterNames(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get parameterNames", args, obj);
    if (!obj->isFunction()) {
        args.rval().setUndefined();
        return true;
    }

    const JSFunction *fun = obj->toFunction();
    JSObject *result = NewDenseAllocatedArray(cx, fun->nargs, NULL);
    if (!result)
        return false;
    result->ensureDenseArrayInitializedLength(cx, 0, fun->nargs);

    if (fun->isInterpreted()) {
        JS_ASSERT(fun->nargs == fun->script()->bindings.countArgs());

        if (fun->nargs > 0) {
            Vector<JSAtom *> names(cx);
            if (!fun->script()->bindings.getLocalNameArray(cx, &names))
                return false;

            for (size_t i = 0; i < fun->nargs; i++) {
                JSAtom *name = names[i];
                result->setDenseArrayElement(i, name ? StringValue(name) : UndefinedValue());
            }
        }
    } else {
        for (size_t i = 0; i < fun->nargs; i++)
            result->setDenseArrayElement(i, UndefinedValue());
    }

    args.rval().setObject(*result);
    return true;
}

static JSBool
DebuggerObject_getScript(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get script", args, dbg, obj);

    if (!obj->isFunction()) {
        args.rval().setUndefined();
        return true;
    }

    JSFunction *fun = obj->toFunction();
    if (!fun->isInterpreted()) {
        args.rval().setUndefined();
        return true;
    }

    JSObject *scriptObject = dbg->wrapScript(cx, fun->script());
    if (!scriptObject)
        return false;

    args.rval().setObject(*scriptObject);
    return true;
}

static JSBool
DebuggerObject_getEnvironment(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get environment", args, dbg, obj);

    
    if (!obj->isFunction() || !obj->toFunction()->isInterpreted()) {
        args.rval().setUndefined();
        return true;
    }

    Env *env = obj->toFunction()->environment();
    return dbg->wrapEnvironment(cx, env, &args.rval());
}

static JSBool
DebuggerObject_getOwnPropertyDescriptor(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "getOwnPropertyDescriptor", args, dbg, obj);

    jsid id;
    if (!ValueToId(cx, argc >= 1 ? args[0] : UndefinedValue(), &id))
        return false;

    
    AutoPropertyDescriptorRooter desc(cx);
    {
        AutoCompartment ac(cx, obj);
        if (!ac.enter() || !cx->compartment->wrapId(cx, &id))
            return false;

        ErrorCopier ec(ac, dbg->toJSObject());
        if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
            return false;
    }

    if (desc.obj) {
        
        if (!dbg->wrapDebuggeeValue(cx, &desc.value))
            return false;
        if (desc.attrs & JSPROP_GETTER) {
            Value get = ObjectOrNullValue(CastAsObject(desc.getter));
            if (!dbg->wrapDebuggeeValue(cx, &get))
                return false;
            desc.getter = CastAsPropertyOp(get.toObjectOrNull());
        }
        if (desc.attrs & JSPROP_SETTER) {
            Value set = ObjectOrNullValue(CastAsObject(desc.setter));
            if (!dbg->wrapDebuggeeValue(cx, &set))
                return false;
            desc.setter = CastAsStrictPropertyOp(set.toObjectOrNull());
        }
    }

    return NewPropertyDescriptorObject(cx, &desc, &args.rval());
}

static JSBool
DebuggerObject_getOwnPropertyNames(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "getOwnPropertyNames", args, dbg, obj);

    AutoIdVector keys(cx);
    {
        AutoCompartment ac(cx, obj);
        if (!ac.enter())
            return false;

        ErrorCopier ec(ac, dbg->toJSObject());
        if (!GetPropertyNames(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN, &keys))
            return false;
    }

    AutoValueVector vals(cx);
    if (!vals.resize(keys.length()))
        return false;

    for (size_t i = 0, len = keys.length(); i < len; i++) {
         jsid id = keys[i];
         if (JSID_IS_INT(id)) {
             JSString *str = js_IntToString(cx, JSID_TO_INT(id));
             if (!str)
                 return false;
             vals[i].setString(str);
         } else if (JSID_IS_ATOM(id)) {
             vals[i].setString(JSID_TO_STRING(id));
             if (!cx->compartment->wrap(cx, &vals[i]))
                 return false;
         } else {
             vals[i].setObject(*JSID_TO_OBJECT(id));
             if (!dbg->wrapDebuggeeValue(cx, &vals[i]))
                 return false;
         }
    }

    JSObject *aobj = NewDenseCopiedArray(cx, vals.length(), vals.begin());
    if (!aobj)
        return false;
    args.rval().setObject(*aobj);
    return true;
}

static bool
CheckArgCompartment(JSContext *cx, JSObject *obj, const Value &v,
                    const char *methodname, const char *propname)
{
    if (v.isObject() && v.toObject().compartment() != obj->compartment()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_COMPARTMENT_MISMATCH,
                             methodname, propname);
        return false;
    }
    return true;
}





static bool
UnwrapPropDesc(JSContext *cx, Debugger *dbg, JSObject *obj, PropDesc *desc)
{
    return (!desc->hasValue || (dbg->unwrapDebuggeeValue(cx, &desc->value) &&
                                CheckArgCompartment(cx, obj, desc->value, "defineProperty",
                                                    "value"))) &&
           (!desc->hasGet || (dbg->unwrapDebuggeeValue(cx, &desc->get) &&
                              CheckArgCompartment(cx, obj, desc->get, "defineProperty", "get") &&
                              desc->checkGetter(cx))) &&
           (!desc->hasSet || (dbg->unwrapDebuggeeValue(cx, &desc->set) &&
                              CheckArgCompartment(cx, obj, desc->set, "defineProperty", "set") &&
                              desc->checkSetter(cx)));
}






static bool
WrapIdAndPropDesc(JSContext *cx, JSObject *obj, jsid *idp, PropDesc *desc)
{
    JSCompartment *comp = cx->compartment;
    return comp->wrapId(cx, idp) &&
           comp->wrap(cx, &desc->value) &&
           comp->wrap(cx, &desc->get) &&
           comp->wrap(cx, &desc->set) &&
           (!IsProxy(obj) || desc->makeObject(cx));
}

static JSBool
DebuggerObject_defineProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "defineProperty", args, dbg, obj);
    REQUIRE_ARGC("Debugger.Object.defineProperty", 2);

    jsid id;
    if (!ValueToId(cx, args[0], &id))
        return JS_FALSE;

    const Value &descval = args[1];
    AutoPropDescArrayRooter descs(cx);
    PropDesc *desc = descs.append();
    if (!desc || !desc->initialize(cx, descval, false))
        return false;

    desc->pd.setUndefined();
    if (!UnwrapPropDesc(cx, dbg, obj, desc))
        return false;

    {
        AutoCompartment ac(cx, obj);
        if (!ac.enter() || !WrapIdAndPropDesc(cx, obj, &id, desc))
            return false;

        ErrorCopier ec(ac, dbg->toJSObject());
        bool dummy;
        if (!DefineProperty(cx, obj, id, *desc, true, &dummy))
            return false;
    }

    args.rval().setUndefined();
    return true;
}

static JSBool
DebuggerObject_defineProperties(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "defineProperties", args, dbg, obj);
    REQUIRE_ARGC("Debugger.Object.defineProperties", 1);
    JSObject *props = ToObject(cx, &args[0]);
    if (!props)
        return false;

    AutoIdVector ids(cx);
    AutoPropDescArrayRooter descs(cx);
    if (!ReadPropertyDescriptors(cx, props, false, &ids, &descs))
        return false;
    size_t n = ids.length();

    for (size_t i = 0; i < n; i++) {
        if (!UnwrapPropDesc(cx, dbg, obj, &descs[i]))
            return false;
    }

    {
        AutoCompartment ac(cx, obj);
        if (!ac.enter())
            return false;
        for (size_t i = 0; i < n; i++) {
            if (!WrapIdAndPropDesc(cx, obj, &ids[i], &descs[i]))
                return false;
        }

        ErrorCopier ec(ac, dbg->toJSObject());
        for (size_t i = 0; i < n; i++) {
            bool dummy;
            if (!DefineProperty(cx, obj, ids[i], descs[i], true, &dummy))
                return false;
        }
    }

    args.rval().setUndefined();
    return true;
}





static JSBool
DebuggerObject_deleteProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "deleteProperty", args, dbg, obj);
    Value nameArg = argc > 0 ? args[0] : UndefinedValue();

    AutoCompartment ac(cx, obj);
    if (!ac.enter() || !cx->compartment->wrap(cx, &nameArg))
        return false;

    ErrorCopier ec(ac, dbg->toJSObject());
    return obj->deleteByValue(cx, nameArg, &args.rval(), false);
}

enum SealHelperOp { Seal, Freeze, PreventExtensions };

static JSBool
DebuggerObject_sealHelper(JSContext *cx, unsigned argc, Value *vp, SealHelperOp op, const char *name)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, name, args, dbg, obj);

    AutoCompartment ac(cx, obj);
    if (!ac.enter())
        return false;

    ErrorCopier ec(ac, dbg->toJSObject());
    bool ok;
    if (op == Seal) {
        ok = obj->seal(cx);
    } else if (op == Freeze) {
        ok = obj->freeze(cx);
    } else {
        JS_ASSERT(op == PreventExtensions);
        if (!obj->isExtensible()) {
            args.rval().setUndefined();
            return true;
        }
        AutoIdVector props(cx);
        ok = obj->preventExtensions(cx, &props);
    }
    if (!ok)
        return false;
    args.rval().setUndefined();
    return true;
}

static JSBool
DebuggerObject_seal(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_sealHelper(cx, argc, vp, Seal, "seal");
}

static JSBool
DebuggerObject_freeze(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_sealHelper(cx, argc, vp, Freeze, "freeze");
}

static JSBool
DebuggerObject_preventExtensions(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_sealHelper(cx, argc, vp, PreventExtensions, "preventExtensions");
}

static JSBool
DebuggerObject_isSealedHelper(JSContext *cx, unsigned argc, Value *vp, SealHelperOp op,
                              const char *name)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, name, args, dbg, obj);

    AutoCompartment ac(cx, obj);
    if (!ac.enter())
        return false;

    ErrorCopier ec(ac, dbg->toJSObject());
    bool r;
    if (op == Seal) {
        if (!obj->isSealed(cx, &r))
            return false;
    } else if (op == Freeze) {
        if (!obj->isFrozen(cx, &r))
            return false;
    } else {
        r = obj->isExtensible();
    }
    args.rval().setBoolean(r);
    return true;
}

static JSBool
DebuggerObject_isSealed(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_isSealedHelper(cx, argc, vp, Seal, "isSealed");
}

static JSBool
DebuggerObject_isFrozen(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_isSealedHelper(cx, argc, vp, Freeze, "isFrozen");
}

static JSBool
DebuggerObject_isExtensible(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_isSealedHelper(cx, argc, vp, PreventExtensions, "isExtensible");
}

enum ApplyOrCallMode { ApplyMode, CallMode };

static JSBool
ApplyOrCall(JSContext *cx, unsigned argc, Value *vp, ApplyOrCallMode mode)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "apply", args, dbg, obj);

    



    Value calleev = ObjectValue(*obj);
    if (!obj->isCallable()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Object", "apply", obj->getClass()->name);
        return false;
    }

    



    Value thisv = argc > 0 ? args[0] : UndefinedValue();
    if (!dbg->unwrapDebuggeeValue(cx, &thisv))
        return false;
    unsigned callArgc = 0;
    Value *callArgv = NULL;
    AutoValueVector argv(cx);
    if (mode == ApplyMode) {
        if (argc >= 2 && !args[1].isNullOrUndefined()) {
            if (!args[1].isObject()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_APPLY_ARGS,
                                     js_apply_str);
                return false;
            }
            JSObject *argsobj = &args[1].toObject();
            if (!js_GetLengthProperty(cx, argsobj, &callArgc))
                return false;
            callArgc = unsigned(JS_MIN(callArgc, StackSpace::ARGS_LENGTH_MAX));
            if (!argv.growBy(callArgc) || !GetElements(cx, argsobj, callArgc, argv.begin()))
                return false;
            callArgv = argv.begin();
        }
    } else {
        callArgc = argc > 0 ? unsigned(JS_MIN(argc - 1, StackSpace::ARGS_LENGTH_MAX)) : 0;
        callArgv = args.array() + 1;
    }
    for (unsigned i = 0; i < callArgc; i++) {
        if (!dbg->unwrapDebuggeeValue(cx, &callArgv[i]))
            return false;
    }

    



    AutoCompartment ac(cx, obj);
    if (!ac.enter() || !cx->compartment->wrap(cx, &calleev) || !cx->compartment->wrap(cx, &thisv))
        return false;
    for (unsigned i = 0; i < callArgc; i++) {
        if (!cx->compartment->wrap(cx, &callArgv[i]))
            return false;
    }

    



    Value rval;
    bool ok = Invoke(cx, thisv, calleev, callArgc, callArgv, &rval);
    return dbg->receiveCompletionValue(ac, ok, rval, &args.rval());
}

static JSBool
DebuggerObject_apply(JSContext *cx, unsigned argc, Value *vp)
{
    return ApplyOrCall(cx, argc, vp, ApplyMode);
}

static JSBool
DebuggerObject_call(JSContext *cx, unsigned argc, Value *vp)
{
    return ApplyOrCall(cx, argc, vp, CallMode);
}

static JSPropertySpec DebuggerObject_properties[] = {
    JS_PSG("proto", DebuggerObject_getProto, 0),
    JS_PSG("class", DebuggerObject_getClass, 0),
    JS_PSG("callable", DebuggerObject_getCallable, 0),
    JS_PSG("name", DebuggerObject_getName, 0),
    JS_PSG("parameterNames", DebuggerObject_getParameterNames, 0),
    JS_PSG("script", DebuggerObject_getScript, 0),
    JS_PSG("environment", DebuggerObject_getEnvironment, 0),
    JS_PS_END
};

static JSFunctionSpec DebuggerObject_methods[] = {
    JS_FN("getOwnPropertyDescriptor", DebuggerObject_getOwnPropertyDescriptor, 1, 0),
    JS_FN("getOwnPropertyNames", DebuggerObject_getOwnPropertyNames, 0, 0),
    JS_FN("defineProperty", DebuggerObject_defineProperty, 2, 0),
    JS_FN("defineProperties", DebuggerObject_defineProperties, 1, 0),
    JS_FN("deleteProperty", DebuggerObject_deleteProperty, 1, 0),
    JS_FN("seal", DebuggerObject_seal, 0, 0),
    JS_FN("freeze", DebuggerObject_freeze, 0, 0),
    JS_FN("preventExtensions", DebuggerObject_preventExtensions, 0, 0),
    JS_FN("isSealed", DebuggerObject_isSealed, 0, 0),
    JS_FN("isFrozen", DebuggerObject_isFrozen, 0, 0),
    JS_FN("isExtensible", DebuggerObject_isExtensible, 0, 0),
    JS_FN("apply", DebuggerObject_apply, 0, 0),
    JS_FN("call", DebuggerObject_call, 0, 0),
    JS_FS_END
};




static void
DebuggerEnv_trace(JSTracer *trc, JSObject *obj)
{
    if (!trc->runtime->gcCurrentCompartment) {
        



        if (Env *referent = (JSObject *) obj->getPrivate())
            MarkObjectUnbarriered(trc, referent, "Debugger.Environment referent");
    }
}

Class DebuggerEnv_class = {
    "Environment",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGENV_COUNT),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, NULL,
    NULL,                 
    NULL,                 
    NULL,                 
    NULL,                 
    DebuggerEnv_trace
};

static JSObject *
DebuggerEnv_checkThis(JSContext *cx, const CallArgs &args, const char *fnname)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return NULL;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerEnv_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Environment", fnname, thisobj->getClass()->name);
        return NULL;
    }

    




    if (!thisobj->getPrivate()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Environment", fnname, "prototype object");
        return NULL;
    }
    return thisobj;
}

#define THIS_DEBUGENV(cx, argc, vp, fnname, args, envobj, env)                \
    CallArgs args = CallArgsFromVp(argc, vp);                                 \
    JSObject *envobj = DebuggerEnv_checkThis(cx, args, fnname);               \
    if (!envobj)                                                              \
        return false;                                                         \
    Env *env = static_cast<Env *>(envobj->getPrivate());                      \
    JS_ASSERT(env)

#define THIS_DEBUGENV_OWNER(cx, argc, vp, fnname, args, envobj, env, dbg)     \
    THIS_DEBUGENV(cx, argc, vp, fnname, args, envobj, env);                   \
    Debugger *dbg = Debugger::fromChildJSObject(envobj)

static JSBool
DebuggerEnv_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NO_CONSTRUCTOR, "Debugger.Environment");
    return false;
}

static JSBool
DebuggerEnv_getType(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV(cx, argc, vp, "get type", args, envobj, env);

    
    const char *s;
    if (env->isCall() || env->isBlock() || env->isDeclEnv())
        s = "declarative";
    else
        s = "object";

    JSAtom *str = js_Atomize(cx, s, strlen(s), InternAtom, NormalEncoding);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static JSBool
DebuggerEnv_getParent(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get parent", args, envobj, env, dbg);

    
    Env *parent = env->enclosingScope();
    return dbg->wrapEnvironment(cx, parent, &args.rval());
}

static JSBool
DebuggerEnv_getObject(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get type", args, envobj, env, dbg);

    



    if (env->isCall() || env->isBlock() || env->isDeclEnv()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEBUG_NO_SCOPE_OBJECT);
        return false;
    }
    JSObject *obj = env->isWith() ? env->getProto() : env;

    Value rval = ObjectValue(*obj);
    if (!dbg->wrapDebuggeeValue(cx, &rval))
        return false;
    args.rval() = rval;
    return true;
}

static JSBool
DebuggerEnv_names(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get type", args, envobj, env, dbg);

    AutoIdVector keys(cx);
    {
        AutoCompartment ac(cx, env);
        if (!ac.enter())
            return false;

        ErrorCopier ec(ac, dbg->toJSObject());
        if (!GetPropertyNames(cx, env, JSITER_HIDDEN, &keys))
            return false;
    }

    JSObject *arr = NewDenseEmptyArray(cx);
    if (!arr)
        return false;
    for (size_t i = 0, len = keys.length(); i < len; i++) {
         jsid id = keys[i];
         if (JSID_IS_ATOM(id) && IsIdentifier(JSID_TO_ATOM(id))) {
             if (!cx->compartment->wrapId(cx, &id))
                 return false;
             if (!js_NewbornArrayPush(cx, arr, StringValue(JSID_TO_STRING(id))))
                 return false;
         }
    }
    args.rval().setObject(*arr);
    return true;
}

static JSBool
DebuggerEnv_find(JSContext *cx, unsigned argc, Value *vp)
{
    REQUIRE_ARGC("Debugger.Environment.find", 1);
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get type", args, envobj, env, dbg);

    jsid id;
    if (!ValueToIdentifier(cx, args[0], &id))
        return false;

    {
        AutoCompartment ac(cx, env);
        if (!ac.enter() || !cx->compartment->wrapId(cx, &id))
            return false;

        
        ErrorCopier ec(ac, dbg->toJSObject());
        JSProperty *prop = NULL;
        JSObject *pobj;
        for (; env && !prop; env = env->enclosingScope()) {
            if (!env->lookupGeneric(cx, id, &pobj, &prop))
                return false;
            if (prop)
                break;
        }
    }

    return dbg->wrapEnvironment(cx, env, &args.rval());
}

static JSPropertySpec DebuggerEnv_properties[] = {
    JS_PSG("type", DebuggerEnv_getType, 0),
    JS_PSG("object", DebuggerEnv_getObject, 0),
    JS_PSG("parent", DebuggerEnv_getParent, 0),
    JS_PS_END
};

static JSFunctionSpec DebuggerEnv_methods[] = {
    JS_FN("names", DebuggerEnv_names, 0, 0),
    JS_FN("find", DebuggerEnv_find, 1, 0),
    JS_FS_END
};





extern JS_PUBLIC_API(JSBool)
JS_DefineDebuggerObject(JSContext *cx, JSObject *obj)
{
    RootObject objRoot(cx, &obj);

    RootedVarObject
        objProto(cx),
        debugCtor(cx),
        debugProto(cx),
        frameProto(cx),
        scriptProto(cx),
        objectProto(cx);

    objProto = obj->asGlobal().getOrCreateObjectPrototype(cx);
    if (!objProto)
        return false;


    debugProto = js_InitClass(cx, objRoot,
                              objProto, &Debugger::jsclass, Debugger::construct,
                              1, Debugger::properties, Debugger::methods, NULL, NULL,
                              debugCtor.address());
    if (!debugProto)
        return false;

    frameProto = js_InitClass(cx, debugCtor, objProto, &DebuggerFrame_class,
                              DebuggerFrame_construct, 0,
                              DebuggerFrame_properties, DebuggerFrame_methods,
                              NULL, NULL);
    if (!frameProto)
        return false;

    scriptProto = js_InitClass(cx, debugCtor, objProto, &DebuggerScript_class,
                               DebuggerScript_construct, 0,
                               DebuggerScript_properties, DebuggerScript_methods,
                               NULL, NULL);
    if (!scriptProto)
        return false;

    objectProto = js_InitClass(cx, debugCtor, objProto, &DebuggerObject_class,
                               DebuggerObject_construct, 0,
                               DebuggerObject_properties, DebuggerObject_methods,
                               NULL, NULL);
    if (!objectProto)
        return false;

    JSObject *envProto = js_InitClass(cx, debugCtor, objProto, &DebuggerEnv_class,
                                      DebuggerEnv_construct, 0,
                                      DebuggerEnv_properties, DebuggerEnv_methods,
                                      NULL, NULL);
    if (!envProto)
        return false;

    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_FRAME_PROTO, ObjectValue(*frameProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_OBJECT_PROTO, ObjectValue(*objectProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_SCRIPT_PROTO, ObjectValue(*scriptProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_ENV_PROTO, ObjectValue(*envProto));
    return true;
}
