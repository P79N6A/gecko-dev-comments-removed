





#include "vm/Debugger-inl.h"

#include "mozilla/DebugOnly.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jshashutil.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jswrapper.h"

#include "frontend/BytecodeCompiler.h"
#include "gc/Marking.h"
#include "jit/BaselineJIT.h"
#include "js/Debug.h"
#include "js/GCAPI.h"
#include "js/Vector.h"
#include "vm/ArgumentsObject.h"
#include "vm/DebuggerMemory.h"
#include "vm/SPSProfiler.h"
#include "vm/WrapperObject.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"
#include "jsscriptinlines.h"

#include "vm/ObjectImpl-inl.h"
#include "vm/Stack-inl.h"

using namespace js;

using JS::dbg::Builder;
using js::frontend::IsIdentifier;
using mozilla::ArrayLength;
using mozilla::Maybe;




extern const Class DebuggerFrame_class;

enum {
    JSSLOT_DEBUGFRAME_OWNER,
    JSSLOT_DEBUGFRAME_ARGUMENTS,
    JSSLOT_DEBUGFRAME_ONSTEP_HANDLER,
    JSSLOT_DEBUGFRAME_ONPOP_HANDLER,
    JSSLOT_DEBUGFRAME_COUNT
};

extern const Class DebuggerArguments_class;

enum {
    JSSLOT_DEBUGARGUMENTS_FRAME,
    JSSLOT_DEBUGARGUMENTS_COUNT
};

extern const Class DebuggerEnv_class;

enum {
    JSSLOT_DEBUGENV_OWNER,
    JSSLOT_DEBUGENV_COUNT
};

extern const Class DebuggerObject_class;

enum {
    JSSLOT_DEBUGOBJECT_OWNER,
    JSSLOT_DEBUGOBJECT_COUNT
};

extern const Class DebuggerScript_class;

enum {
    JSSLOT_DEBUGSCRIPT_OWNER,
    JSSLOT_DEBUGSCRIPT_COUNT
};

extern const Class DebuggerSource_class;

enum {
    JSSLOT_DEBUGSOURCE_OWNER,
    JSSLOT_DEBUGSOURCE_COUNT
};




static inline bool
EnsureFunctionHasScript(JSContext *cx, HandleFunction fun)
{
    if (fun->isInterpretedLazy()) {
        AutoCompartment ac(cx, fun);
        return !!fun->getOrCreateScript(cx);
    }
    return true;
}

static inline JSScript *
GetOrCreateFunctionScript(JSContext *cx, HandleFunction fun)
{
    MOZ_ASSERT(fun->isInterpreted());
    if (!EnsureFunctionHasScript(cx, fun))
        return nullptr;
    return fun->nonLazyScript();
}

bool
js::ReportObjectRequired(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT);
    return false;
}

static bool
ValueToIdentifier(JSContext *cx, HandleValue v, MutableHandleId id)
{
    if (!ValueToId<CanGC>(cx, v, id))
        return false;
    if (!JSID_IS_ATOM(id) || !IsIdentifier(JSID_TO_ATOM(id))) {
        RootedValue val(cx, v);
        js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_UNEXPECTED_TYPE,
                                 JSDVG_SEARCH_STACK, val, js::NullPtr(), "not an identifier",
                                 nullptr);
        return false;
    }
    return true;
}








class Debugger::FrameRange
{
    AbstractFramePtr frame;

    
    GlobalObject::DebuggerVector *debuggers;

    



    size_t debuggerCount, nextDebugger;

    



    FrameMap::Ptr entry;

  public:
    










    explicit FrameRange(AbstractFramePtr frame, GlobalObject *global = nullptr)
      : frame(frame)
    {
        nextDebugger = 0;

        
        if (!global)
            global = &frame.script()->global();

        
        JS_ASSERT(&frame.script()->global() == global);

        
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
        return entry->value();
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
            entry = dbg->frames.lookup(frame);
            if (entry)
                break;
            nextDebugger++;
        }
    }
};



BreakpointSite::BreakpointSite(JSScript *script, jsbytecode *pc)
  : script(script), pc(pc), enabledCount(0)
{
    JS_ASSERT(!script->hasBreakpointsAt(pc));
    JS_INIT_CLIST(&breakpoints);
}

void
BreakpointSite::recompile(FreeOp *fop)
{
    if (script->hasBaselineScript())
        script->baselineScript()->toggleDebugTraps(script, pc);
}

void
BreakpointSite::inc(FreeOp *fop)
{
    enabledCount++;
    if (enabledCount == 1)
        recompile(fop);
}

void
BreakpointSite::dec(FreeOp *fop)
{
    JS_ASSERT(enabledCount > 0);
    enabledCount--;
    if (enabledCount == 0)
        recompile(fop);
}

void
BreakpointSite::destroyIfEmpty(FreeOp *fop)
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        script->destroyBreakpointSite(fop, pc);
}

Breakpoint *
BreakpointSite::firstBreakpoint() const
{
    if (JS_CLIST_IS_EMPTY(&breakpoints))
        return nullptr;
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
    JS_ASSERT(handler->compartment() == debugger->object->compartment());
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
Breakpoint::destroy(FreeOp *fop)
{
    if (debugger->enabled)
        site->dec(fop);
    JS_REMOVE_LINK(&debuggerLinks);
    JS_REMOVE_LINK(&siteLinks);
    site->destroyIfEmpty(fop);
    fop->delete_(this);
}

Breakpoint *
Breakpoint::nextInDebugger()
{
    JSCList *link = JS_NEXT_LINK(&debuggerLinks);
    return (link == &debugger->breakpoints) ? nullptr : fromDebuggerLinks(link);
}

Breakpoint *
Breakpoint::nextInSite()
{
    JSCList *link = JS_NEXT_LINK(&siteLinks);
    return (link == &site->breakpoints) ? nullptr : fromSiteLinks(link);
}



Debugger::Debugger(JSContext *cx, JSObject *dbg)
  : object(dbg), uncaughtExceptionHook(nullptr), enabled(true), trackingAllocationSites(false),
    allocationsLogLength(0), maxAllocationsLogLength(DEFAULT_MAX_ALLOCATIONS_LOG_LENGTH),
    frames(cx->runtime()), scripts(cx), sources(cx), objects(cx), environments(cx)
{
    assertSameCompartment(cx, dbg);

    cx->runtime()->debuggerList.insertBack(this);
    JS_INIT_CLIST(&breakpoints);
    JS_INIT_CLIST(&onNewGlobalObjectWatchersLink);
}

Debugger::~Debugger()
{
    JS_ASSERT_IF(debuggees.initialized(), debuggees.empty());
    emptyAllocationsLog();

    






    JS_REMOVE_LINK(&onNewGlobalObjectWatchersLink);
}

bool
Debugger::init(JSContext *cx)
{
    bool ok = debuggees.init() &&
              frames.init() &&
              scripts.init() &&
              sources.init() &&
              objects.init() &&
              environments.init();
    if (!ok)
        js_ReportOutOfMemory(cx);
    return ok;
}

JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGSCRIPT_OWNER));
JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGSOURCE_OWNER));
JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGOBJECT_OWNER));
JS_STATIC_ASSERT(unsigned(JSSLOT_DEBUGFRAME_OWNER) == unsigned(JSSLOT_DEBUGENV_OWNER));

Debugger *
Debugger::fromChildJSObject(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &DebuggerFrame_class ||
              obj->getClass() == &DebuggerScript_class ||
              obj->getClass() == &DebuggerSource_class ||
              obj->getClass() == &DebuggerObject_class ||
              obj->getClass() == &DebuggerEnv_class);
    JSObject *dbgobj = &obj->getReservedSlot(JSSLOT_DEBUGOBJECT_OWNER).toObject();
    return fromJSObject(dbgobj);
}

bool
Debugger::hasMemory() const
{
    return object->getReservedSlot(JSSLOT_DEBUG_MEMORY_INSTANCE).isObject();
}

DebuggerMemory &
Debugger::memory() const
{
    JS_ASSERT(hasMemory());
    return object->getReservedSlot(JSSLOT_DEBUG_MEMORY_INSTANCE).toObject().as<DebuggerMemory>();
}

bool
Debugger::getScriptFrameWithIter(JSContext *cx, AbstractFramePtr frame,
                                 const ScriptFrameIter *maybeIter, MutableHandleValue vp)
{
    MOZ_ASSERT_IF(maybeIter, maybeIter->abstractFramePtr() == frame);

    FrameMap::AddPtr p = frames.lookupForAdd(frame);
    if (!p) {
        
        JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_FRAME_PROTO).toObject();
        JSObject *frameobj =
            NewObjectWithGivenProto(cx, &DebuggerFrame_class, proto, nullptr);
        if (!frameobj)
            return false;

        
        
        if (maybeIter) {
            AbstractFramePtr data = maybeIter->copyDataAsAbstractFramePtr();
            if (!data)
                return false;
            frameobj->setPrivate(data.raw());
        } else {
            frameobj->setPrivate(frame.raw());
        }

        frameobj->setReservedSlot(JSSLOT_DEBUGFRAME_OWNER, ObjectValue(*object));

        if (!frames.add(p, frame, frameobj)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }
    vp.setObject(*p->value());
    return true;
}

JSObject *
Debugger::getHook(Hook hook) const
{
    JS_ASSERT(hook >= 0 && hook < HookCount);
    const Value &v = object->getReservedSlot(JSSLOT_DEBUG_HOOK_START + hook);
    return v.isUndefined() ? nullptr : &v.toObject();
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
        if (IsScriptMarked(&bp->site->script))
            return true;
    }

    for (FrameMap::Range r = frames.all(); !r.empty(); r.popFront()) {
        JSObject *frameObj = r.front().value();
        if (!frameObj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined() ||
            !frameObj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER).isUndefined())
            return true;
    }

    return false;
}

JSTrapStatus
Debugger::slowPathOnEnterFrame(JSContext *cx, AbstractFramePtr frame, MutableHandleValue vp)
{
    
    AutoValueVector triggered(cx);
    Handle<GlobalObject*> global = cx->global();

    if (GlobalObject::DebuggerVector *debuggers = global->getDebuggers()) {
        for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++) {
            Debugger *dbg = *p;
            if (dbg->observesFrame(frame) && dbg->observesEnterFrame() &&
                !triggered.append(ObjectValue(*dbg->toJSObject())))
            {
                return JSTRAP_ERROR;
            }
        }
    }

    
    for (Value *p = triggered.begin(); p != triggered.end(); p++) {
        Debugger *dbg = Debugger::fromJSObject(&p->toObject());
        if (dbg->debuggees.has(global) && dbg->observesEnterFrame()) {
            JSTrapStatus status = dbg->fireEnterFrame(cx, frame, vp);
            if (status != JSTRAP_CONTINUE)
                return status;
        }
    }

    return JSTRAP_CONTINUE;
}

static void
DebuggerFrame_maybeDecrementFrameScriptStepModeCount(FreeOp *fop, AbstractFramePtr frame,
                                                     JSObject *frameobj);

static void
DebuggerFrame_freeScriptFrameIterData(FreeOp *fop, JSObject *obj);






bool
Debugger::slowPathOnLeaveFrame(JSContext *cx, AbstractFramePtr frame, bool frameOk)
{
    Handle<GlobalObject*> global = cx->global();

    
    JSTrapStatus status;
    RootedValue value(cx);
    Debugger::resultToCompletion(cx, frameOk, frame.returnValue(), &status, &value);

    
    AutoObjectVector frames(cx);
    for (FrameRange r(frame, global); !r.empty(); r.popFront()) {
        if (!frames.append(r.frontFrame())) {
            cx->clearPendingException();
            return false;
        }
    }

    
    for (JSObject **p = frames.begin(); p != frames.end(); p++) {
        RootedObject frameobj(cx, *p);
        Debugger *dbg = Debugger::fromChildJSObject(frameobj);

        if (dbg->enabled &&
            !frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER).isUndefined()) {
            RootedValue handler(cx, frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER));

            Maybe<AutoCompartment> ac;
            ac.emplace(cx, dbg->object);

            RootedValue completion(cx);
            if (!dbg->newCompletionValue(cx, status, value, &completion)) {
                status = dbg->handleUncaughtException(ac, false);
                break;
            }

            
            RootedValue rval(cx);
            bool hookOk = Invoke(cx, ObjectValue(*frameobj), handler, 1, completion.address(),
                                 &rval);
            RootedValue nextValue(cx);
            JSTrapStatus nextStatus = dbg->parseResumptionValue(ac, hookOk, rval, &nextValue);

            



            JS_ASSERT(cx->compartment() == global->compartment());
            JS_ASSERT(!cx->isExceptionPending());

            
            if (nextStatus != JSTRAP_CONTINUE) {
                status = nextStatus;
                value = nextValue;
            }
        }
    }

    




    for (FrameRange r(frame, global); !r.empty(); r.popFront()) {
        RootedObject frameobj(cx, r.frontFrame());
        Debugger *dbg = r.frontDebugger();
        JS_ASSERT(dbg == Debugger::fromChildJSObject(frameobj));

        FreeOp *fop = cx->runtime()->defaultFreeOp();
        DebuggerFrame_freeScriptFrameIterData(fop, frameobj);
        DebuggerFrame_maybeDecrementFrameScriptStepModeCount(fop, frame, frameobj);

        dbg->frames.remove(frame);
    }

    



    if (frame.isEvalFrame()) {
        RootedScript script(cx, frame.script());
        script->clearBreakpointsIn(cx->runtime()->defaultFreeOp(), nullptr, nullptr);
    }

    
    switch (status) {
      case JSTRAP_RETURN:
        frame.setReturnValue(value);
        return true;

      case JSTRAP_THROW:
        cx->setPendingException(value);
        return false;

      case JSTRAP_ERROR:
        JS_ASSERT(!cx->isExceptionPending());
        return false;

      default:
        MOZ_CRASH("bad final trap status");
    }
}

bool
Debugger::wrapEnvironment(JSContext *cx, Handle<Env*> env, MutableHandleValue rval)
{
    if (!env) {
        rval.setNull();
        return true;
    }

    



    JS_ASSERT(!env->is<ScopeObject>());

    JSObject *envobj;
    DependentAddPtr<ObjectWeakMap> p(cx, environments, env);
    if (p) {
        envobj = p->value();
    } else {
        
        JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_ENV_PROTO).toObject();
        envobj = NewObjectWithGivenProto(cx, &DebuggerEnv_class, proto, nullptr, TenuredObject);
        if (!envobj)
            return false;
        envobj->setPrivateGCThing(env);
        envobj->setReservedSlot(JSSLOT_DEBUGENV_OWNER, ObjectValue(*object));
        if (!p.add(cx, environments, env, envobj)) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        CrossCompartmentKey key(CrossCompartmentKey::DebuggerEnvironment, object, env);
        if (!object->compartment()->putWrapper(cx, key, ObjectValue(*envobj))) {
            environments.remove(env);
            js_ReportOutOfMemory(cx);
            return false;
        }
    }
    rval.setObject(*envobj);
    return true;
}

bool
Debugger::wrapDebuggeeValue(JSContext *cx, MutableHandleValue vp)
{
    assertSameCompartment(cx, object.get());

    if (vp.isObject()) {
        RootedObject obj(cx, &vp.toObject());

        if (obj->is<JSFunction>()) {
            RootedFunction fun(cx, &obj->as<JSFunction>());
            if (!EnsureFunctionHasScript(cx, fun))
                return false;
        }

        DependentAddPtr<ObjectWeakMap> p(cx, objects, obj);
        if (p) {
            vp.setObject(*p->value());
        } else {
            
            JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_OBJECT_PROTO).toObject();
            JSObject *dobj =
                NewObjectWithGivenProto(cx, &DebuggerObject_class, proto, nullptr, TenuredObject);
            if (!dobj)
                return false;
            dobj->setPrivateGCThing(obj);
            dobj->setReservedSlot(JSSLOT_DEBUGOBJECT_OWNER, ObjectValue(*object));

            if (!p.add(cx, objects, obj, dobj)) {
                js_ReportOutOfMemory(cx);
                return false;
            }

            if (obj->compartment() != object->compartment()) {
                CrossCompartmentKey key(CrossCompartmentKey::DebuggerObject, object, obj);
                if (!object->compartment()->putWrapper(cx, key, ObjectValue(*dobj))) {
                    objects.remove(obj);
                    js_ReportOutOfMemory(cx);
                    return false;
                }
            }

            vp.setObject(*dobj);
        }
    } else if (vp.isMagic()) {
        RootedObject optObj(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
        if (!optObj)
            return false;

        
        
        
        PropertyName *name;
        if (vp.whyMagic() == JS_OPTIMIZED_ARGUMENTS) {
            name = cx->names().missingArguments;
        } else {
            MOZ_ASSERT(vp.whyMagic() == JS_OPTIMIZED_OUT);
            name = cx->names().optimizedOut;
        }

        RootedValue trueVal(cx, BooleanValue(true));
        if (!JSObject::defineProperty(cx, optObj, name, trueVal))
            return false;

        vp.setObject(*optObj);
    } else if (!cx->compartment()->wrap(cx, vp)) {
        vp.setUndefined();
        return false;
    }

    return true;
}

bool
Debugger::unwrapDebuggeeValue(JSContext *cx, MutableHandleValue vp)
{
    assertSameCompartment(cx, object.get(), vp);
    if (vp.isObject()) {
        JSObject *dobj = &vp.toObject();
        if (dobj->getClass() != &DebuggerObject_class) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_EXPECTED_TYPE,
                                 "Debugger", "Debugger.Object", dobj->getClass()->name);
            return false;
        }

        Value owner = dobj->getReservedSlot(JSSLOT_DEBUGOBJECT_OWNER);
        if (owner.isUndefined() || &owner.toObject() != object) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 owner.isUndefined()
                                 ? JSMSG_DEBUG_OBJECT_PROTO
                                 : JSMSG_DEBUG_OBJECT_WRONG_OWNER);
            return false;
        }

        vp.setObject(*static_cast<JSObject*>(dobj->getPrivate()));
    }
    return true;
}





static bool
CheckArgCompartment(JSContext *cx, JSObject *obj, HandleValue v,
                    const char *methodname, const char *propname)
{
    if (v.isObject() && v.toObject().compartment() != obj->compartment()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_COMPARTMENT_MISMATCH,
                             methodname, propname);
        return false;
    }
    return true;
}

bool
Debugger::unwrapPropDescInto(JSContext *cx, HandleObject obj, Handle<PropDesc> wrapped,
                             MutableHandle<PropDesc> unwrapped)
{
    MOZ_ASSERT(!wrapped.isUndefined());

    unwrapped.set(wrapped);

    if (unwrapped.hasValue()) {
        RootedValue value(cx, unwrapped.value());
        if (!unwrapDebuggeeValue(cx, &value) ||
            !CheckArgCompartment(cx, obj, value, "defineProperty", "value"))
        {
            return false;
        }
        unwrapped.setValue(value);
    }

    if (unwrapped.hasGet()) {
        RootedValue get(cx, unwrapped.getterValue());
        if (!unwrapDebuggeeValue(cx, &get) ||
            !CheckArgCompartment(cx, obj, get, "defineProperty", "get"))
        {
            return false;
        }
        unwrapped.setGetter(get);
    }

    if (unwrapped.hasSet()) {
        RootedValue set(cx, unwrapped.setterValue());
        if (!unwrapDebuggeeValue(cx, &set) ||
            !CheckArgCompartment(cx, obj, set, "defineProperty", "set"))
        {
            return false;
        }
        unwrapped.setSetter(set);
    }

    return true;
}

JSTrapStatus
Debugger::handleUncaughtExceptionHelper(Maybe<AutoCompartment> &ac,
                                        MutableHandleValue *vp, bool callHook)
{
    JSContext *cx = ac->context()->asJSContext();
    if (cx->isExceptionPending()) {
        if (callHook && uncaughtExceptionHook) {
            RootedValue exc(cx);
            if (!cx->getPendingException(&exc))
                return JSTRAP_ERROR;
            cx->clearPendingException();
            RootedValue fval(cx, ObjectValue(*uncaughtExceptionHook));
            RootedValue rv(cx);
            if (Invoke(cx, ObjectValue(*object), fval, 1, exc.address(), &rv))
                return vp ? parseResumptionValue(ac, true, rv, *vp, false) : JSTRAP_CONTINUE;
        }

        if (cx->isExceptionPending()) {
            JS_ReportPendingException(cx);
            cx->clearPendingException();
        }
    }
    ac.reset();
    return JSTRAP_ERROR;
}

JSTrapStatus
Debugger::handleUncaughtException(Maybe<AutoCompartment> &ac, MutableHandleValue vp, bool callHook)
{
    return handleUncaughtExceptionHelper(ac, &vp, callHook);
}

JSTrapStatus
Debugger::handleUncaughtException(Maybe<AutoCompartment> &ac, bool callHook)
{
    return handleUncaughtExceptionHelper(ac, nullptr, callHook);
}

void
Debugger::resultToCompletion(JSContext *cx, bool ok, const Value &rv,
                             JSTrapStatus *status, MutableHandleValue value)
{
    JS_ASSERT_IF(ok, !cx->isExceptionPending());

    if (ok) {
        *status = JSTRAP_RETURN;
        value.set(rv);
    } else if (cx->isExceptionPending()) {
        *status = JSTRAP_THROW;
        if (!cx->getPendingException(value))
            *status = JSTRAP_ERROR;
        cx->clearPendingException();
    } else {
        *status = JSTRAP_ERROR;
        value.setUndefined();
    }
}

bool
Debugger::newCompletionValue(JSContext *cx, JSTrapStatus status, Value value_,
                             MutableHandleValue result)
{
    



    assertSameCompartment(cx, object.get());

    RootedId key(cx);
    RootedValue value(cx, value_);

    switch (status) {
      case JSTRAP_RETURN:
        key = NameToId(cx->names().return_);
        break;

      case JSTRAP_THROW:
        key = NameToId(cx->names().throw_);
        break;

      case JSTRAP_ERROR:
        result.setNull();
        return true;

      default:
        MOZ_CRASH("bad status passed to Debugger::newCompletionValue");
    }

    
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
    if (!obj ||
        !wrapDebuggeeValue(cx, &value) ||
        !DefineNativeProperty(cx, obj, key, value, JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_ENUMERATE))
    {
        return false;
    }

    result.setObject(*obj);
    return true;
}

bool
Debugger::receiveCompletionValue(Maybe<AutoCompartment> &ac, bool ok,
                                 HandleValue val,
                                 MutableHandleValue vp)
{
    JSContext *cx = ac->context()->asJSContext();

    JSTrapStatus status;
    RootedValue value(cx);
    resultToCompletion(cx, ok, val, &status, &value);
    ac.reset();
    return newCompletionValue(cx, status, value, vp);
}

JSTrapStatus
Debugger::parseResumptionValue(Maybe<AutoCompartment> &ac, bool ok, const Value &rv, MutableHandleValue vp,
                               bool callHook)
{
    vp.setUndefined();
    if (!ok)
        return handleUncaughtException(ac, vp, callHook);
    if (rv.isUndefined()) {
        ac.reset();
        return JSTRAP_CONTINUE;
    }
    if (rv.isNull()) {
        ac.reset();
        return JSTRAP_ERROR;
    }

    
    JSContext *cx = ac->context()->asJSContext();
    Rooted<JSObject*> obj(cx);
    RootedShape shape(cx);
    RootedId returnId(cx, NameToId(cx->names().return_));
    RootedId throwId(cx, NameToId(cx->names().throw_));
    bool okResumption = rv.isObject();
    if (okResumption) {
        obj = &rv.toObject();
        okResumption = obj->is<JSObject>();
    }
    if (okResumption) {
        shape = obj->lastProperty();
        okResumption = shape->previous() &&
             !shape->previous()->previous() &&
             (shape->propid() == returnId || shape->propid() == throwId) &&
             shape->isDataDescriptor();
    }
    if (!okResumption) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_BAD_RESUMPTION);
        return handleUncaughtException(ac, vp, callHook);
    }

    RootedValue v(cx, vp.get());
    if (!NativeGet(cx, obj, obj, shape, &v) || !unwrapDebuggeeValue(cx, &v))
        return handleUncaughtException(ac, &v, callHook);

    ac.reset();
    if (!cx->compartment()->wrap(cx, &v)) {
        vp.setUndefined();
        return JSTRAP_ERROR;
    }
    vp.set(v);

    return shape->propid() == returnId ? JSTRAP_RETURN : JSTRAP_THROW;
}

static bool
CallMethodIfPresent(JSContext *cx, HandleObject obj, const char *name, int argc, Value *argv,
                    MutableHandleValue rval)
{
    rval.setUndefined();
    JSAtom *atom = Atomize(cx, name, strlen(name));
    if (!atom)
        return false;

    RootedId id(cx, AtomToId(atom));
    RootedValue fval(cx);
    return JSObject::getGeneric(cx, obj, obj, id, &fval) &&
           (!IsCallable(fval) || Invoke(cx, ObjectValue(*obj), fval, argc, argv, rval));
}

JSTrapStatus
Debugger::fireDebuggerStatement(JSContext *cx, MutableHandleValue vp)
{
    RootedObject hook(cx, getHook(OnDebuggerStatement));
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, object);

    ScriptFrameIter iter(cx);

    RootedValue scriptFrame(cx);
    if (!getScriptFrame(cx, iter, &scriptFrame))
        return handleUncaughtException(ac, false);

    RootedValue rv(cx);
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, scriptFrame.address(), &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

JSTrapStatus
Debugger::fireExceptionUnwind(JSContext *cx, MutableHandleValue vp)
{
    RootedObject hook(cx, getHook(OnExceptionUnwind));
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    RootedValue exc(cx);
    if (!cx->getPendingException(&exc))
        return JSTRAP_ERROR;
    cx->clearPendingException();

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, object);

    JS::AutoValueArray<2> argv(cx);
    argv[0].setUndefined();
    argv[1].set(exc);

    ScriptFrameIter iter(cx);
    if (!getScriptFrame(cx, iter, argv[0]) || !wrapDebuggeeValue(cx, argv[1]))
        return handleUncaughtException(ac, false);

    RootedValue rv(cx);
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 2, argv.begin(), &rv);
    JSTrapStatus st = parseResumptionValue(ac, ok, rv, vp);
    if (st == JSTRAP_CONTINUE)
        cx->setPendingException(exc);
    return st;
}

JSTrapStatus
Debugger::fireEnterFrame(JSContext *cx, AbstractFramePtr frame, MutableHandleValue vp)
{
    RootedObject hook(cx, getHook(OnEnterFrame));
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, object);

    RootedValue scriptFrame(cx);
    if (!getScriptFrame(cx, frame, &scriptFrame))
        return handleUncaughtException(ac, false);

    RootedValue rv(cx);
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, scriptFrame.address(), &rv);
    return parseResumptionValue(ac, ok, rv, vp);
}

void
Debugger::fireNewScript(JSContext *cx, HandleScript script)
{
    RootedObject hook(cx, getHook(OnNewScript));
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, object);

    JSObject *dsobj = wrapScript(cx, script);
    if (!dsobj) {
        handleUncaughtException(ac, false);
        return;
    }

    RootedValue scriptObject(cx, ObjectValue(*dsobj));
    RootedValue rv(cx);
    if (!Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, scriptObject.address(), &rv))
        handleUncaughtException(ac, true);
}

JSTrapStatus
Debugger::dispatchHook(JSContext *cx, MutableHandleValue vp, Hook which)
{
    JS_ASSERT(which == OnDebuggerStatement || which == OnExceptionUnwind);

    







    AutoValueVector triggered(cx);
    Handle<GlobalObject*> global = cx->global();
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
AddNewScriptRecipients(GlobalObject::DebuggerVector *src, HandleScript script,
                       AutoValueVector *dest)
{
    bool wasEmpty = dest->length() == 0;
    for (Debugger **p = src->begin(); p != src->end(); p++) {
        Debugger *dbg = *p;
        Value v = ObjectValue(*dbg->toJSObject());
        if (dbg->observesScript(script) && dbg->observesNewScript() &&
            (wasEmpty || Find(dest->begin(), dest->end(), v) == dest->end()) &&
            !dest->append(v))
        {
            return false;
        }
    }
    return true;
}

void
Debugger::slowPathOnNewScript(JSContext *cx, HandleScript script, GlobalObject *compileAndGoGlobal_)
{
    Rooted<GlobalObject*> compileAndGoGlobal(cx, compileAndGoGlobal_);

    JS_ASSERT(script->compileAndGo() == !!compileAndGoGlobal);

    







    AutoValueVector triggered(cx);
    GlobalObject::DebuggerVector *debuggers =
        (script->compileAndGo()
         ? compileAndGoGlobal->getDebuggers()
         : script->compartment()->maybeGlobal()->getDebuggers());
    if (debuggers) {
        if (!AddNewScriptRecipients(debuggers, script, &triggered))
            return;
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
Debugger::onTrap(JSContext *cx, MutableHandleValue vp)
{
    MOZ_ASSERT(cx->compartment()->debugMode());

    ScriptFrameIter iter(cx);
    RootedScript script(cx, iter.script());
    Rooted<GlobalObject*> scriptGlobal(cx, &script->global());
    jsbytecode *pc = iter.pc();
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
            Maybe<AutoCompartment> ac;
            ac.emplace(cx, dbg->object);

            RootedValue scriptFrame(cx);
            if (!dbg->getScriptFrame(cx, iter, &scriptFrame))
                return dbg->handleUncaughtException(ac, false);
            RootedValue rv(cx);
            Rooted<JSObject*> handler(cx, bp->handler);
            bool ok = CallMethodIfPresent(cx, handler, "hit", 1, scriptFrame.address(), &rv);
            JSTrapStatus st = dbg->parseResumptionValue(ac, ok, rv, vp, true);
            if (st != JSTRAP_CONTINUE)
                return st;

            
            site = script->getBreakpointSite(pc);
        }
    }

    
    vp.setInt32(op);
    return JSTRAP_CONTINUE;
}

JSTrapStatus
Debugger::onSingleStep(JSContext *cx, MutableHandleValue vp)
{
    ScriptFrameIter iter(cx);

    





    RootedValue exception(cx, UndefinedValue());
    bool exceptionPending = cx->isExceptionPending();
    if (exceptionPending) {
        if (!cx->getPendingException(&exception))
            return JSTRAP_ERROR;
        cx->clearPendingException();
    }

    



    AutoObjectVector frames(cx);
    for (FrameRange r(iter.abstractFramePtr()); !r.empty(); r.popFront()) {
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
        JSScript *trappingScript = iter.script();
        GlobalObject *global = cx->global();
        if (GlobalObject::DebuggerVector *debuggers = global->getDebuggers()) {
            for (Debugger **p = debuggers->begin(); p != debuggers->end(); p++) {
                Debugger *dbg = *p;
                for (FrameMap::Range r = dbg->frames.all(); !r.empty(); r.popFront()) {
                    AbstractFramePtr frame = r.front().key();
                    JSObject *frameobj = r.front().value();
                    if (frame.script() == trappingScript &&
                        !frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined())
                    {
                        stepperCount++;
                    }
                }
            }
        }
        if (trappingScript->compileAndGo())
            JS_ASSERT(stepperCount == trappingScript->stepModeCount());
        else
            JS_ASSERT(stepperCount <= trappingScript->stepModeCount());
    }
#endif

    
    class PreserveIterValue {
        JSContext *cx;
        RootedValue savedIterValue;

      public:
        explicit PreserveIterValue(JSContext *cx) : cx(cx), savedIterValue(cx, cx->iterValue) {
            cx->iterValue.setMagic(JS_NO_ITER_VALUE);
        }
        ~PreserveIterValue() {
            cx->iterValue = savedIterValue;
        }
    };
    PreserveIterValue piv(cx);

    
    for (JSObject **p = frames.begin(); p != frames.end(); p++) {
        RootedObject frame(cx, *p);
        Debugger *dbg = Debugger::fromChildJSObject(frame);

        Maybe<AutoCompartment> ac;
        ac.emplace(cx, dbg->object);

        const Value &handler = frame->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER);
        RootedValue rval(cx);
        bool ok = Invoke(cx, ObjectValue(*frame), handler, 0, nullptr, &rval);
        JSTrapStatus st = dbg->parseResumptionValue(ac, ok, rval, vp);
        if (st != JSTRAP_CONTINUE)
            return st;
    }

    vp.setUndefined();
    if (exceptionPending)
        cx->setPendingException(exception);
    return JSTRAP_CONTINUE;
}

JSTrapStatus
Debugger::fireNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global, MutableHandleValue vp)
{
    RootedObject hook(cx, getHook(OnNewGlobalObject));
    JS_ASSERT(hook);
    JS_ASSERT(hook->isCallable());

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, object);

    RootedValue wrappedGlobal(cx, ObjectValue(*global));
    if (!wrapDebuggeeValue(cx, &wrappedGlobal))
        return handleUncaughtException(ac, false);

    RootedValue rv(cx);

    
    
    
    
    
    
    bool ok = Invoke(cx, ObjectValue(*object), ObjectValue(*hook), 1, wrappedGlobal.address(), &rv);
    if (ok && !rv.isUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_DEBUG_RESUMPTION_VALUE_DISALLOWED);
        ok = false;
    }
    
    
    
    
    JSTrapStatus status = ok ? JSTRAP_CONTINUE
                             : handleUncaughtException(ac, vp, true);
    JS_ASSERT(!cx->isExceptionPending());
    return status;
}

void
Debugger::slowPathOnNewGlobalObject(JSContext *cx, Handle<GlobalObject *> global)
{
    JS_ASSERT(!JS_CLIST_IS_EMPTY(&cx->runtime()->onNewGlobalObjectWatchers));
    if (global->compartment()->options().invisibleToDebugger())
        return;

    




    AutoObjectVector watchers(cx);
    for (JSCList *link = JS_LIST_HEAD(&cx->runtime()->onNewGlobalObjectWatchers);
         link != &cx->runtime()->onNewGlobalObjectWatchers;
         link = JS_NEXT_LINK(link))
    {
        Debugger *dbg = fromOnNewGlobalObjectWatchersLink(link);
        JS_ASSERT(dbg->observesNewGlobalObject());
        JSObject *obj = dbg->object;
        JS::ExposeObjectToActiveJS(obj);
        if (!watchers.append(obj))
            return;
    }

    JSTrapStatus status = JSTRAP_CONTINUE;
    RootedValue value(cx);

    for (size_t i = 0; i < watchers.length(); i++) {
        Debugger *dbg = fromJSObject(watchers[i]);

        
        
        
        
        
        
        
        
        if (dbg->observesNewGlobalObject()) {
            status = dbg->fireNewGlobalObject(cx, global, &value);
            if (status != JSTRAP_CONTINUE && status != JSTRAP_RETURN)
                break;
        }
    }
    JS_ASSERT(!cx->isExceptionPending());
}

 bool
Debugger::slowPathOnLogAllocationSite(JSContext *cx, HandleSavedFrame frame,
                                      GlobalObject::DebuggerVector &dbgs)
{
    JS_ASSERT(!dbgs.empty());
    mozilla::DebugOnly<Debugger **> begin = dbgs.begin();

    for (Debugger **dbgp = dbgs.begin(); dbgp < dbgs.end(); dbgp++) {
        
        
        JS_ASSERT(dbgs.begin() == begin);

        if ((*dbgp)->trackingAllocationSites &&
            (*dbgp)->enabled &&
            !(*dbgp)->appendAllocationSite(cx, frame))
        {
            return false;
        }
    }

    return true;
}

bool
Debugger::appendAllocationSite(JSContext *cx, HandleSavedFrame frame)
{
    AutoCompartment ac(cx, object);
    RootedObject wrapped(cx, frame);
    if (!cx->compartment()->wrap(cx, &wrapped))
        return false;

    AllocationSite *allocSite = cx->new_<AllocationSite>(wrapped);
    if (!allocSite)
        return false;

    allocationsLog.insertBack(allocSite);

    if (allocationsLogLength >= maxAllocationsLogLength) {
        js_delete(allocationsLog.getFirst());
    } else {
        allocationsLogLength++;
    }

    return true;
}

void
Debugger::emptyAllocationsLog()
{
    while (!allocationsLog.isEmpty())
        js_delete(allocationsLog.getFirst());
    allocationsLogLength = 0;
}





void
Debugger::markKeysInCompartment(JSTracer *trc)
{
    




    objects.markKeys(trc);
    environments.markKeys(trc);
    scripts.markKeys(trc);
    sources.markKeys(trc);
}























void
Debugger::markCrossCompartmentDebuggerObjectReferents(JSTracer *trc)
{
    JSRuntime *rt = trc->runtime();

    



    for (Debugger *dbg = rt->debuggerList.getFirst(); dbg; dbg = dbg->getNext()) {
        if (!dbg->object->zone()->isCollecting())
            dbg->markKeysInCompartment(trc);
    }
}











bool
Debugger::markAllIteratively(GCMarker *trc)
{
    bool markedAny = false;

    



    JSRuntime *rt = trc->runtime();
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        if (c->debugMode()) {
            GlobalObject *global = c->maybeGlobal();
            if (!IsObjectMarked(&global))
                continue;

            



            const GlobalObject::DebuggerVector *debuggers = global->getDebuggers();
            JS_ASSERT(debuggers);
            for (Debugger * const *p = debuggers->begin(); p != debuggers->end(); p++) {
                Debugger *dbg = *p;

                





                HeapPtrObject &dbgobj = dbg->toJSObjectRef();
                if (!dbgobj->zone()->isGCMarking())
                    continue;

                bool dbgMarked = IsObjectMarked(&dbgobj);
                if (!dbgMarked && dbg->hasAnyLiveHooks()) {
                    



                    MarkObject(trc, &dbgobj, "enabled Debugger");
                    markedAny = true;
                    dbgMarked = true;
                }

                if (dbgMarked) {
                    
                    for (Breakpoint *bp = dbg->firstBreakpoint(); bp; bp = bp->nextInDebugger()) {
                        if (IsScriptMarked(&bp->site->script)) {
                            



                            if (!IsObjectMarked(&bp->getHandlerRef())) {
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
Debugger::markAll(JSTracer *trc)
{
    JSRuntime *rt = trc->runtime();
    for (Debugger *dbg = rt->debuggerList.getFirst(); dbg; dbg = dbg->getNext()) {
        GlobalObjectSet &debuggees = dbg->debuggees;
        for (GlobalObjectSet::Enum e(debuggees); !e.empty(); e.popFront()) {
            GlobalObject *global = e.front();

            MarkObjectUnbarriered(trc, &global, "Global Object");
            if (global != e.front())
                e.rekeyFront(global);
        }

        HeapPtrObject &dbgobj = dbg->toJSObjectRef();
        MarkObject(trc, &dbgobj, "Debugger Object");

        dbg->scripts.trace(trc);
        dbg->sources.trace(trc);
        dbg->objects.trace(trc);
        dbg->environments.trace(trc);

        for (Breakpoint *bp = dbg->firstBreakpoint(); bp; bp = bp->nextInDebugger()) {
            MarkScriptUnbarriered(trc, &bp->site->script, "breakpoint script");
            MarkObject(trc, &bp->getHandlerRef(), "breakpoint handler");
        }
    }
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
        RelocatablePtrObject &frameobj = r.front().value();
        JS_ASSERT(MaybeForwarded(frameobj.get())->getPrivate());
        MarkObject(trc, &frameobj, "live Debugger.Frame");
    }

    


    for (AllocationSite *s = allocationsLog.getFirst(); s; s = s->getNext()) {
        if (s->frame)
            MarkObject(trc, &s->frame, "allocation log SavedFrame");
    }

    
    scripts.trace(trc);

    
    sources.trace(trc);

    
    objects.trace(trc);

    
    environments.trace(trc);
}

void
Debugger::sweepAll(FreeOp *fop)
{
    JSRuntime *rt = fop->runtime();

    for (Debugger *dbg = rt->debuggerList.getFirst(); dbg; dbg = dbg->getNext()) {
        if (IsObjectAboutToBeFinalized(&dbg->object)) {
            




            for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront()) {
                
                
                
                dbg->removeDebuggeeGlobalUnderGC(fop, e.front(), &e);
            }
        }
    }
}

void
Debugger::detachAllDebuggersFromGlobal(FreeOp *fop, GlobalObject *global)
{
    const GlobalObject::DebuggerVector *debuggers = global->getDebuggers();
    JS_ASSERT(!debuggers->empty());
    while (!debuggers->empty())
        debuggers->back()->removeDebuggeeGlobalUnderGC(fop, global, nullptr);
}

 void
Debugger::findCompartmentEdges(Zone *zone, js::gc::ComponentFinder<Zone> &finder)
{
    





    for (Debugger *dbg = zone->runtimeFromMainThread()->debuggerList.getFirst();
         dbg;
         dbg = dbg->getNext())
    {
        Zone *w = dbg->object->zone();
        if (w == zone || !w->isGCMarking())
            continue;
        if (dbg->scripts.hasKeyInZone(zone) ||
            dbg->sources.hasKeyInZone(zone) ||
            dbg->objects.hasKeyInZone(zone) ||
            dbg->environments.hasKeyInZone(zone))
        {
            finder.addEdgeTo(w);
        }
    }
}

void
Debugger::finalize(FreeOp *fop, JSObject *obj)
{
    Debugger *dbg = fromJSObject(obj);
    if (!dbg)
        return;
    fop->delete_(dbg);
}

const Class Debugger::jsclass = {
    "Debugger",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUG_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Debugger::finalize,
    nullptr,              
    nullptr,              
    nullptr,              
    Debugger::traceObject
};

Debugger *
Debugger::fromThisValue(JSContext *cx, const CallArgs &args, const char *fnname)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return nullptr;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &Debugger::jsclass) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger", fnname, thisobj->getClass()->name);
        return nullptr;
    }

    




    Debugger *dbg = fromJSObject(thisobj);
    if (!dbg) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger", fnname, "prototype object");
    }
    return dbg;
}

#define THIS_DEBUGGER(cx, argc, vp, fnname, args, dbg)                       \
    CallArgs args = CallArgsFromVp(argc, vp);                                \
    Debugger *dbg = Debugger::fromThisValue(cx, args, fnname);               \
    if (!dbg)                                                                \
        return false

bool
Debugger::getEnabled(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "get enabled", args, dbg);
    args.rval().setBoolean(dbg->enabled);
    return true;
}

bool
Debugger::setEnabled(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "set enabled", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.set enabled", 1))
        return false;

    bool enabled = ToBoolean(args[0]);

    if (enabled != dbg->enabled) {
        for (Breakpoint *bp = dbg->firstBreakpoint(); bp; bp = bp->nextInDebugger()) {
            if (enabled)
                bp->site->inc(cx->runtime()->defaultFreeOp());
            else
                bp->site->dec(cx->runtime()->defaultFreeOp());
        }

        



        if (dbg->getHook(OnNewGlobalObject)) {
            if (enabled) {
                
                JS_ASSERT(JS_CLIST_IS_EMPTY(&dbg->onNewGlobalObjectWatchersLink));
                JS_APPEND_LINK(&dbg->onNewGlobalObjectWatchersLink,
                               &cx->runtime()->onNewGlobalObjectWatchers);
            } else {
                
                JS_ASSERT(!JS_CLIST_IS_EMPTY(&dbg->onNewGlobalObjectWatchersLink));
                JS_REMOVE_AND_INIT_LINK(&dbg->onNewGlobalObjectWatchersLink);
            }
        }
    }

    dbg->enabled = enabled;
    args.rval().setUndefined();
    return true;
}

bool
Debugger::getHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which)
{
    JS_ASSERT(which >= 0 && which < HookCount);
    THIS_DEBUGGER(cx, argc, vp, "getHook", args, dbg);
    args.rval().set(dbg->object->getReservedSlot(JSSLOT_DEBUG_HOOK_START + which));
    return true;
}

bool
Debugger::setHookImpl(JSContext *cx, unsigned argc, Value *vp, Hook which)
{
    JS_ASSERT(which >= 0 && which < HookCount);
    THIS_DEBUGGER(cx, argc, vp, "setHook", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.setHook", 1))
        return false;
    if (args[0].isObject()) {
        if (!args[0].toObject().isCallable())
            return ReportIsNotFunction(cx, args[0], args.length() - 1);
    } else if (!args[0].isUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_CALLABLE_OR_UNDEFINED);
        return false;
    }
    dbg->object->setReservedSlot(JSSLOT_DEBUG_HOOK_START + which, args[0]);
    args.rval().setUndefined();
    return true;
}

bool
Debugger::getOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnDebuggerStatement);
}

bool
Debugger::setOnDebuggerStatement(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnDebuggerStatement);
}

bool
Debugger::getOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnExceptionUnwind);
}

bool
Debugger::setOnExceptionUnwind(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnExceptionUnwind);
}

bool
Debugger::getOnNewScript(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnNewScript);
}

bool
Debugger::setOnNewScript(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnNewScript);
}

bool
Debugger::getOnEnterFrame(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnEnterFrame);
}

bool
Debugger::setOnEnterFrame(JSContext *cx, unsigned argc, Value *vp)
{
    return setHookImpl(cx, argc, vp, OnEnterFrame);
}

bool
Debugger::getOnNewGlobalObject(JSContext *cx, unsigned argc, Value *vp)
{
    return getHookImpl(cx, argc, vp, OnNewGlobalObject);
}

bool
Debugger::setOnNewGlobalObject(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "setOnNewGlobalObject", args, dbg);
    RootedObject oldHook(cx, dbg->getHook(OnNewGlobalObject));

    if (!setHookImpl(cx, argc, vp, OnNewGlobalObject))
        return false;

    



    if (dbg->enabled) {
        JSObject *newHook = dbg->getHook(OnNewGlobalObject);
        if (!oldHook && newHook) {
            
            JS_ASSERT(JS_CLIST_IS_EMPTY(&dbg->onNewGlobalObjectWatchersLink));
            JS_APPEND_LINK(&dbg->onNewGlobalObjectWatchersLink,
                           &cx->runtime()->onNewGlobalObjectWatchers);
        } else if (oldHook && !newHook) {
            
            JS_ASSERT(!JS_CLIST_IS_EMPTY(&dbg->onNewGlobalObjectWatchersLink));
            JS_REMOVE_AND_INIT_LINK(&dbg->onNewGlobalObjectWatchersLink);
        }
    }

    return true;
}

bool
Debugger::getUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "get uncaughtExceptionHook", args, dbg);
    args.rval().setObjectOrNull(dbg->uncaughtExceptionHook);
    return true;
}

bool
Debugger::setUncaughtExceptionHook(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "set uncaughtExceptionHook", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.set uncaughtExceptionHook", 1))
        return false;
    if (!args[0].isNull() && (!args[0].isObject() || !args[0].toObject().isCallable())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_ASSIGN_FUNCTION_OR_NULL,
                             "uncaughtExceptionHook");
        return false;
    }
    dbg->uncaughtExceptionHook = args[0].toObjectOrNull();
    args.rval().setUndefined();
    return true;
}

bool
Debugger::getMemory(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "get memory", args, dbg);
    Value memoryValue = dbg->object->getReservedSlot(JSSLOT_DEBUG_MEMORY_INSTANCE);

    if (!memoryValue.isObject()) {
        RootedObject memory(cx, DebuggerMemory::create(cx, dbg));
        if (!memory)
            return false;
        memoryValue = ObjectValue(*memory);
    }

    args.rval().set(memoryValue);
    return true;
}

GlobalObject *
Debugger::unwrapDebuggeeArgument(JSContext *cx, const Value &v)
{
    if (!v.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             "argument", "not a global object");
        return nullptr;
    }

    RootedObject obj(cx, &v.toObject());

    
    if (obj->getClass() == &DebuggerObject_class) {
        RootedValue rv(cx, v);
        if (!unwrapDebuggeeValue(cx, &rv))
            return nullptr;
        obj = &rv.toObject();
    }

    
    obj = CheckedUnwrap(obj);
    if (!obj) {
        JS_ReportError(cx, "Permission denied to access object");
        return nullptr;
    }

    
    obj = GetInnerObject(obj);
    if (!obj)
        return nullptr;

    
    if (!obj->is<GlobalObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             "argument", "not a global object");
        return nullptr;
    }

    return &obj->as<GlobalObject>();
}

bool
Debugger::addDebuggee(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "addDebuggee", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.addDebuggee", 1))
        return false;
    Rooted<GlobalObject*> global(cx, dbg->unwrapDebuggeeArgument(cx, args[0]));
    if (!global)
        return false;

    if (!dbg->addDebuggeeGlobal(cx, global))
        return false;

    RootedValue v(cx, ObjectValue(*global));
    if (!dbg->wrapDebuggeeValue(cx, &v))
        return false;
    args.rval().set(v);
    return true;
}

bool
Debugger::addAllGlobalsAsDebuggees(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "addAllGlobalsAsDebuggees", args, dbg);
    for (ZonesIter zone(cx->runtime(), SkipAtoms); !zone.done(); zone.next()) {
        
        
        AutoDebugModeInvalidation invalidate(zone);

        for (CompartmentsInZoneIter c(zone); !c.done(); c.next()) {
            if (c == dbg->object->compartment() || c->options().invisibleToDebugger())
                continue;
            c->scheduledForDestruction = false;
            GlobalObject *global = c->maybeGlobal();
            if (global) {
                Rooted<GlobalObject*> rg(cx, global);
                if (!dbg->addDebuggeeGlobal(cx, rg, invalidate))
                    return false;
            }
        }
    }

    args.rval().setUndefined();
    return true;
}

bool
Debugger::removeDebuggee(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "removeDebuggee", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.removeDebuggee", 1))
        return false;
    Rooted<GlobalObject *> global(cx, dbg->unwrapDebuggeeArgument(cx, args[0]));
    if (!global)
        return false;
    if (dbg->debuggees.has(global)) {
        if (!dbg->removeDebuggeeGlobal(cx, global, nullptr))
            return false;
    }
    args.rval().setUndefined();
    return true;
}

bool
Debugger::removeAllDebuggees(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "removeAllDebuggees", args, dbg);

    for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront()) {
        Rooted<GlobalObject *> global(cx, e.front());
        if (!dbg->removeDebuggeeGlobal(cx, global, &e))
            return false;
    }

    args.rval().setUndefined();
    return true;
}

bool
Debugger::hasDebuggee(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "hasDebuggee", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.hasDebuggee", 1))
        return false;
    GlobalObject *global = dbg->unwrapDebuggeeArgument(cx, args[0]);
    if (!global)
        return false;
    args.rval().setBoolean(!!dbg->debuggees.lookup(global));
    return true;
}

bool
Debugger::getDebuggees(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "getDebuggees", args, dbg);
    RootedObject arrobj(cx, NewDenseFullyAllocatedArray(cx, dbg->debuggees.count()));
    if (!arrobj)
        return false;
    arrobj->ensureDenseInitializedLength(cx, 0, dbg->debuggees.count());
    unsigned i = 0;
    for (GlobalObjectSet::Enum e(dbg->debuggees); !e.empty(); e.popFront()) {
        RootedValue v(cx, ObjectValue(*e.front()));
        if (!dbg->wrapDebuggeeValue(cx, &v))
            return false;
        arrobj->setDenseElement(i++, v);
    }
    args.rval().setObject(*arrobj);
    return true;
}

bool
Debugger::getNewestFrame(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "getNewestFrame", args, dbg);

    
    for (AllFramesIter i(cx); !i.done(); ++i) {
        if (dbg->observesFrame(i)) {
            
            
            if (i.isIon() && !i.ensureHasRematerializedFrame(cx))
                return false;
            AbstractFramePtr frame = i.abstractFramePtr();
            ScriptFrameIter iter(i.activation()->cx(), ScriptFrameIter::GO_THROUGH_SAVED);
            while (!iter.hasUsableAbstractFramePtr() || iter.abstractFramePtr() != frame)
                ++iter;
            return dbg->getScriptFrame(cx, iter, args.rval());
        }
    }
    args.rval().setNull();
    return true;
}

bool
Debugger::clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "clearAllBreakpoints", args, dbg);
    for (GlobalObjectSet::Range r = dbg->debuggees.all(); !r.empty(); r.popFront())
        r.front()->compartment()->clearBreakpointsIn(cx->runtime()->defaultFreeOp(),
                                                     dbg, NullPtr());
    return true;
}

bool
Debugger::construct(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    for (unsigned i = 0; i < args.length(); i++) {
        const Value &arg = args[i];
        if (!arg.isObject())
            return ReportObjectRequired(cx);
        JSObject *argobj = &arg.toObject();
        if (!argobj->is<CrossCompartmentWrapperObject>()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_CCW_REQUIRED,
                                 "Debugger");
            return false;
        }
    }

    
    RootedValue v(cx);
    RootedObject callee(cx, &args.callee());
    if (!JSObject::getProperty(cx, callee, callee, cx->names().prototype, &v))
        return false;
    RootedObject proto(cx, &v.toObject());
    JS_ASSERT(proto->getClass() == &Debugger::jsclass);
    




    RootedObject obj(cx, NewObjectWithGivenProto(cx, &Debugger::jsclass, proto, nullptr));
    if (!obj)
        return false;
    for (unsigned slot = JSSLOT_DEBUG_PROTO_START; slot < JSSLOT_DEBUG_PROTO_STOP; slot++)
        obj->setReservedSlot(slot, proto->getReservedSlot(slot));
    obj->setReservedSlot(JSSLOT_DEBUG_MEMORY_INSTANCE, NullValue());

    
    auto dbg = cx->make_unique<Debugger>(cx, obj.get());
    if (!dbg || !dbg->init(cx))
        return false;

    Debugger *debugger = dbg.release();
    obj->setPrivate(debugger); 

    
    for (unsigned i = 0; i < args.length(); i++) {
        Rooted<GlobalObject*>
            debuggee(cx, &args[i].toObject().as<ProxyObject>().private_().toObject().global());
        if (!debugger->addDebuggeeGlobal(cx, debuggee))
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

bool
Debugger::addDebuggeeGlobal(JSContext *cx, Handle<GlobalObject*> global)
{
    AutoSuppressProfilerSampling suppressProfilerSampling(cx);
    AutoDebugModeInvalidation invalidate(global->compartment());
    return addDebuggeeGlobal(cx, global, invalidate);
}

bool
Debugger::addDebuggeeGlobal(JSContext *cx,
                            Handle<GlobalObject*> global,
                            AutoDebugModeInvalidation &invalidate)
{
    if (debuggees.has(global))
        return true;

    
    
    
    
    JSCompartment *debuggeeCompartment = global->compartment();
    if (debuggeeCompartment->options().invisibleToDebugger()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_DEBUG_CANT_DEBUG_GLOBAL);
        return false;
    }

    





    Vector<JSCompartment *> visited(cx);
    if (!visited.append(object->compartment()))
        return false;
    for (size_t i = 0; i < visited.length(); i++) {
        JSCompartment *c = visited[i];
        if (c == debuggeeCompartment) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_LOOP);
            return false;
        }

        



        if (c->debugMode()) {
            GlobalObject::DebuggerVector *v = c->maybeGlobal()->getDebuggers();
            for (Debugger **p = v->begin(); p != v->end(); p++) {
                JSCompartment *next = (*p)->object->compartment();
                if (Find(visited, next) == visited.end() && !visited.append(next))
                    return false;
            }
        }
    }

    



    bool setMetadataCallback = false;
    if (trackingAllocationSites) {
        if (debuggeeCompartment->hasObjectMetadataCallback()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_OBJECT_METADATA_CALLBACK_ALREADY_SET);
            return false;
        }

        debuggeeCompartment->setObjectMetadataCallback(SavedStacksMetadataCallback);
        setMetadataCallback = true;
    }

    



    AutoCompartment ac(cx, global);
    GlobalObject::DebuggerVector *v = GlobalObject::getOrCreateDebuggers(cx, global);
    if (!v || !v->append(this)) {
        js_ReportOutOfMemory(cx);
    } else {
        if (!debuggees.put(global)) {
            js_ReportOutOfMemory(cx);
        } else {
            if (global->getDebuggers()->length() > 1)
                return true;
            if (debuggeeCompartment->enterDebugMode(cx, invalidate))
                return true;

            
            debuggees.remove(global);
        }
        JS_ASSERT(v->back() == this);
        v->popBack();
    }

    
    if (setMetadataCallback)
        debuggeeCompartment->forgetObjectMetadataCallback();

    return false;
}

void
Debugger::cleanupDebuggeeGlobalBeforeRemoval(FreeOp *fop, GlobalObject *global,
                                             GlobalObjectSet::Enum *debugEnum)
{
    




    JS_ASSERT(debuggees.has(global));
    JS_ASSERT_IF(debugEnum, debugEnum->front() == global);

    








    for (FrameMap::Enum e(frames); !e.empty(); e.popFront()) {
        AbstractFramePtr frame = e.front().key();
        JSObject *frameobj = e.front().value();
        if (&frame.script()->global() == global) {
            DebuggerFrame_freeScriptFrameIterData(fop, frameobj);
            DebuggerFrame_maybeDecrementFrameScriptStepModeCount(fop, frame, frameobj);
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
    if (debugEnum)
        debugEnum->removeFront();
    else
        debuggees.remove(global);

    
    Breakpoint *nextbp;
    for (Breakpoint *bp = firstBreakpoint(); bp; bp = nextbp) {
        nextbp = bp->nextInDebugger();
        if (bp->site->script->compartment() == global->compartment())
            bp->destroy(fop);
    }
    JS_ASSERT_IF(debuggees.empty(), !firstBreakpoint());

    



    if (trackingAllocationSites)
        global->compartment()->forgetObjectMetadataCallback();
}

bool
Debugger::removeDebuggeeGlobal(JSContext *cx, Handle<GlobalObject *> global,
                               GlobalObjectSet::Enum *debugEnum)
{
    AutoDebugModeInvalidation invalidate(global->compartment());
    return removeDebuggeeGlobal(cx, global, invalidate, debugEnum);
}

bool
Debugger::removeDebuggeeGlobal(JSContext *cx, Handle<GlobalObject *> global,
                               AutoDebugModeInvalidation &invalidate,
                               GlobalObjectSet::Enum *debugEnum)
{
    cleanupDebuggeeGlobalBeforeRemoval(cx->runtime()->defaultFreeOp(), global, debugEnum);

    
    if (global->getDebuggers()->empty())
        return global->compartment()->leaveDebugMode(cx, invalidate);

    return true;
}

void
Debugger::removeDebuggeeGlobalUnderGC(FreeOp *fop, GlobalObject *global,
                                      GlobalObjectSet::Enum *debugEnum)
{
    cleanupDebuggeeGlobalBeforeRemoval(fop, global, debugEnum);

    




    if (global->getDebuggers()->empty())
        global->compartment()->leaveDebugModeUnderGC();
}

static inline ScriptSourceObject *GetSourceReferent(JSObject *obj);





class MOZ_STACK_CLASS Debugger::ScriptQuery
{
  public:
    
    ScriptQuery(JSContext *cx, Debugger *dbg):
        cx(cx), debugger(dbg), compartments(cx->runtime()), url(cx), displayURLString(cx),
        source(cx), innermostForCompartment(cx->runtime())
    {}

    



    bool init() {
        if (!compartments.init() ||
            !innermostForCompartment.init())
        {
            js_ReportOutOfMemory(cx);
            return false;
        }

        return true;
    }

    



    bool parseQuery(HandleObject query) {
        



        RootedValue global(cx);
        if (!JSObject::getProperty(cx, query, query, cx->names().global, &global))
            return false;
        if (global.isUndefined()) {
            matchAllDebuggeeGlobals();
        } else {
            GlobalObject *globalObject = debugger->unwrapDebuggeeArgument(cx, global);
            if (!globalObject)
                return false;

            



            if (debugger->debuggees.has(globalObject)) {
                if (!matchSingleGlobal(globalObject))
                    return false;
            }
        }

        
        if (!JSObject::getProperty(cx, query, query, cx->names().url, &url))
            return false;
        if (!url.isUndefined() && !url.isString()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                                 "query object's 'url' property", "neither undefined nor a string");
            return false;
        }

        
        RootedValue debuggerSource(cx);
        if (!JSObject::getProperty(cx, query, query, cx->names().source, &debuggerSource))
            return false;
        if (!debuggerSource.isUndefined()) {
            if (!debuggerSource.isObject() ||
                debuggerSource.toObject().getClass() != &DebuggerSource_class) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                                     "query object's 'source' property",
                                     "not undefined nor a Debugger.Source object");
                return false;
            }

            source = GetSourceReferent(&debuggerSource.toObject());
        }

        
        RootedValue displayURL(cx);
        if (!JSObject::getProperty(cx, query, query, cx->names().displayURL, &displayURL))
            return false;
        if (!displayURL.isUndefined() && !displayURL.isString()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                                 "query object's 'displayURL' property",
                                 "neither undefined nor a string");
            return false;
        }

        if (displayURL.isString()) {
            displayURLString = displayURL.toString()->ensureLinear(cx);
            if (!displayURLString)
                return false;
        }

        
        RootedValue lineProperty(cx);
        if (!JSObject::getProperty(cx, query, query, cx->names().line, &lineProperty))
            return false;
        if (lineProperty.isUndefined()) {
            hasLine = false;
        } else if (lineProperty.isNumber()) {
            if (displayURL.isUndefined() && url.isUndefined() && !source) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_QUERY_LINE_WITHOUT_URL);
                return false;
            }
            double doubleLine = lineProperty.toNumber();
            if (doubleLine <= 0 || (unsigned int) doubleLine != doubleLine) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_BAD_LINE);
                return false;
            }
            hasLine = true;
            line = doubleLine;
        } else {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                                 "query object's 'line' property",
                                 "neither undefined nor an integer");
            return false;
        }

        
        PropertyName *innermostName = cx->names().innermost;
        RootedValue innermostProperty(cx);
        if (!JSObject::getProperty(cx, query, query, innermostName, &innermostProperty))
            return false;
        innermost = ToBoolean(innermostProperty);
        if (innermost) {
            
            if ((displayURL.isUndefined() && url.isUndefined() && !source) || !hasLine) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_QUERY_INNERMOST_WITHOUT_LINE_URL);
                return false;
            }
        }

        return true;
    }

    
    bool omittedQuery() {
        url.setUndefined();
        hasLine = false;
        innermost = false;
        displayURLString = nullptr;
        return matchAllDebuggeeGlobals();
    }

    



    bool findScripts(AutoScriptVector *v) {
        if (!prepareQuery())
            return false;

        JSCompartment *singletonComp = nullptr;
        if (compartments.count() == 1)
            singletonComp = compartments.all().front();

        
        vector = v;
        oom = false;
        IterateScripts(cx->runtime(), singletonComp, this, considerScript);
        if (oom) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        
        for (JSScript **i = vector->begin(); i != vector->end(); ++i)
            JS::ExposeScriptToActiveJS(*i);

        





        if (innermost) {
            for (CompartmentToScriptMap::Range r = innermostForCompartment.all();
                 !r.empty();
                 r.popFront())
            {
                JS::ExposeScriptToActiveJS(r.front().value());
                if (!v->append(r.front().value())) {
                    js_ReportOutOfMemory(cx);
                    return false;
                }
            }
        }

        return true;
    }

  private:
    
    JSContext *cx;

    
    Debugger *debugger;

    typedef HashSet<JSCompartment *, DefaultHasher<JSCompartment *>, RuntimeAllocPolicy>
        CompartmentSet;

    
    CompartmentSet compartments;

    
    RootedValue url;

    
    JSAutoByteString urlCString;

    

    RootedLinearString displayURLString;

    



    RootedScriptSource source;

    
    bool hasLine;

    
    unsigned int line;

    
    bool innermost;

    typedef HashMap<JSCompartment *, JSScript *, DefaultHasher<JSCompartment *>, RuntimeAllocPolicy>
        CompartmentToScriptMap;

    




    CompartmentToScriptMap innermostForCompartment;

    
    AutoScriptVector *vector;

    
    bool oom;

    bool addCompartment(JSCompartment *comp) {
        {
            
            
            AutoCompartment ac(cx, comp);
            if (!comp->ensureDelazifyScriptsForDebugMode(cx))
                return false;
        }
        return compartments.put(comp);
    }

    
    bool matchSingleGlobal(GlobalObject *global) {
        JS_ASSERT(compartments.count() == 0);
        if (!addCompartment(global->compartment())) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        return true;
    }

    



    bool matchAllDebuggeeGlobals() {
        JS_ASSERT(compartments.count() == 0);
        
        for (GlobalObjectSet::Range r = debugger->debuggees.all(); !r.empty(); r.popFront()) {
            if (!addCompartment(r.front()->compartment())) {
                js_ReportOutOfMemory(cx);
                return false;
            }
        }
        return true;
    }

    



    bool prepareQuery() {
        

        if (url.isString()) {
            if (!urlCString.encodeLatin1(cx, url.toString()))
                return false;
        }

        return true;
    }

    static void considerScript(JSRuntime *rt, void *data, JSScript *script) {
        ScriptQuery *self = static_cast<ScriptQuery *>(data);
        self->consider(script);
    }

    




    void consider(JSScript *script) {
        
        
        
        if (oom || script->selfHosted() || !script->code())
            return;
        JSCompartment *compartment = script->compartment();
        if (!compartments.has(compartment))
            return;
        if (urlCString.ptr()) {
            bool gotFilename = false;
            if (script->filename() && strcmp(script->filename(), urlCString.ptr()) == 0)
                gotFilename = true;

            bool gotSourceURL = false;
            if (!gotFilename && script->scriptSource()->introducerFilename() &&
                strcmp(script->scriptSource()->introducerFilename(), urlCString.ptr()) == 0)
            {
                gotSourceURL = true;
            }
            if (!gotFilename && !gotSourceURL)
                return;
        }
        if (hasLine) {
            if (line < script->lineno() || script->lineno() + js_GetScriptLineExtent(script) < line)
                return;
        }
        if (displayURLString) {
            if (!script->scriptSource() || !script->scriptSource()->hasDisplayURL())
                return;

            const char16_t *s = script->scriptSource()->displayURL();
            if (CompareChars(s, js_strlen(s), displayURLString) != 0)
                return;
        }
        if (source && source != script->sourceObject())
            return;

        if (innermost) {
            











            CompartmentToScriptMap::AddPtr p = innermostForCompartment.lookupForAdd(compartment);
            if (p) {
                
                JSScript *incumbent = p->value();
                if (script->staticLevel() > incumbent->staticLevel())
                    p->value() = script;
            } else {
                



                if (!innermostForCompartment.add(p, compartment, script)) {
                    oom = true;
                    return;
                }
            }
        } else {
            
            if (!vector->append(script)) {
                oom = true;
                return;
            }
        }

        return;
    }
};

bool
Debugger::findScripts(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "findScripts", args, dbg);

    ScriptQuery query(cx, dbg);
    if (!query.init())
        return false;

    if (args.length() >= 1) {
        RootedObject queryObject(cx, NonNullObject(cx, args[0]));
        if (!queryObject || !query.parseQuery(queryObject))
            return false;
    } else {
        if (!query.omittedQuery())
            return false;
    }

    




    AutoScriptVector scripts(cx);

    if (!query.findScripts(&scripts))
        return false;

    RootedObject result(cx, NewDenseFullyAllocatedArray(cx, scripts.length()));
    if (!result)
        return false;

    result->ensureDenseInitializedLength(cx, 0, scripts.length());

    for (size_t i = 0; i < scripts.length(); i++) {
        JSObject *scriptObject = dbg->wrapScript(cx, scripts[i]);
        if (!scriptObject)
            return false;
        result->setDenseElement(i, ObjectValue(*scriptObject));
    }

    args.rval().setObject(*result);
    return true;
}

bool
Debugger::findAllGlobals(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "findAllGlobals", args, dbg);

    RootedObject result(cx, NewDenseEmptyArray(cx));
    if (!result)
        return false;

    for (CompartmentsIter c(cx->runtime(), SkipAtoms); !c.done(); c.next()) {
        if (c->options().invisibleToDebugger())
            continue;

        c->scheduledForDestruction = false;

        GlobalObject *global = c->maybeGlobal();

        if (cx->runtime()->isSelfHostingGlobal(global))
            continue;

        if (global) {
            




            JS::ExposeObjectToActiveJS(global);

            RootedValue globalValue(cx, ObjectValue(*global));
            if (!dbg->wrapDebuggeeValue(cx, &globalValue))
                return false;
            if (!NewbornArrayPush(cx, result, globalValue))
                return false;
        }
    }

    args.rval().setObject(*result);
    return true;
}

bool
Debugger::makeGlobalObjectReference(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER(cx, argc, vp, "makeGlobalObjectReference", args, dbg);
    if (!args.requireAtLeast(cx, "Debugger.makeGlobalObjectReference", 1))
        return false;

    Rooted<GlobalObject *> global(cx, dbg->unwrapDebuggeeArgument(cx, args[0]));
    if (!global)
        return false;

    args.rval().setObject(*global);
    return dbg->wrapDebuggeeValue(cx, args.rval());
}

const JSPropertySpec Debugger::properties[] = {
    JS_PSGS("enabled", Debugger::getEnabled, Debugger::setEnabled, 0),
    JS_PSGS("onDebuggerStatement", Debugger::getOnDebuggerStatement,
            Debugger::setOnDebuggerStatement, 0),
    JS_PSGS("onExceptionUnwind", Debugger::getOnExceptionUnwind,
            Debugger::setOnExceptionUnwind, 0),
    JS_PSGS("onNewScript", Debugger::getOnNewScript, Debugger::setOnNewScript, 0),
    JS_PSGS("onEnterFrame", Debugger::getOnEnterFrame, Debugger::setOnEnterFrame, 0),
    JS_PSGS("onNewGlobalObject", Debugger::getOnNewGlobalObject, Debugger::setOnNewGlobalObject, 0),
    JS_PSGS("uncaughtExceptionHook", Debugger::getUncaughtExceptionHook,
            Debugger::setUncaughtExceptionHook, 0),
    JS_PSG("memory", Debugger::getMemory, 0),
    JS_PS_END
};
const JSFunctionSpec Debugger::methods[] = {
    JS_FN("addDebuggee", Debugger::addDebuggee, 1, 0),
    JS_FN("addAllGlobalsAsDebuggees", Debugger::addAllGlobalsAsDebuggees, 0, 0),
    JS_FN("removeDebuggee", Debugger::removeDebuggee, 1, 0),
    JS_FN("removeAllDebuggees", Debugger::removeAllDebuggees, 0, 0),
    JS_FN("hasDebuggee", Debugger::hasDebuggee, 1, 0),
    JS_FN("getDebuggees", Debugger::getDebuggees, 0, 0),
    JS_FN("getNewestFrame", Debugger::getNewestFrame, 0, 0),
    JS_FN("clearAllBreakpoints", Debugger::clearAllBreakpoints, 1, 0),
    JS_FN("findScripts", Debugger::findScripts, 1, 0),
    JS_FN("findAllGlobals", Debugger::findAllGlobals, 0, 0),
    JS_FN("makeGlobalObjectReference", Debugger::makeGlobalObjectReference, 1, 0),
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
    
    if (JSScript *script = GetScriptReferent(obj)) {
        MarkCrossCompartmentScriptUnbarriered(trc, obj, &script, "Debugger.Script referent");
        obj->setPrivateUnbarriered(script);
    }
}

const Class DebuggerScript_class = {
    "Script",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGSCRIPT_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr,              
    nullptr,              
    nullptr,              
    DebuggerScript_trace
};

JSObject *
Debugger::newDebuggerScript(JSContext *cx, HandleScript script)
{
    assertSameCompartment(cx, object.get());

    JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_SCRIPT_PROTO).toObject();
    JS_ASSERT(proto);
    JSObject *scriptobj = NewObjectWithGivenProto(cx, &DebuggerScript_class, proto, nullptr, TenuredObject);
    if (!scriptobj)
        return nullptr;
    scriptobj->setReservedSlot(JSSLOT_DEBUGSCRIPT_OWNER, ObjectValue(*object));
    scriptobj->setPrivateGCThing(script);

    return scriptobj;
}

JSObject *
Debugger::wrapScript(JSContext *cx, HandleScript script)
{
    assertSameCompartment(cx, object.get());
    JS_ASSERT(cx->compartment() != script->compartment());
    DependentAddPtr<ScriptWeakMap> p(cx, scripts, script);
    if (!p) {
        JSObject *scriptobj = newDebuggerScript(cx, script);
        if (!scriptobj)
            return nullptr;

        if (!p.add(cx, scripts, script, scriptobj)) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }

        CrossCompartmentKey key(CrossCompartmentKey::DebuggerScript, object, script);
        if (!object->compartment()->putWrapper(cx, key, ObjectValue(*scriptobj))) {
            scripts.remove(script);
            js_ReportOutOfMemory(cx);
            return nullptr;
        }
    }

    JS_ASSERT(GetScriptReferent(p->value()) == script);
    return p->value();
}

static JSObject *
DebuggerScript_check(JSContext *cx, const Value &v, const char *clsname, const char *fnname)
{
    if (!v.isObject()) {
        ReportObjectRequired(cx);
        return nullptr;
    }
    JSObject *thisobj = &v.toObject();
    if (thisobj->getClass() != &DebuggerScript_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             clsname, fnname, thisobj->getClass()->name);
        return nullptr;
    }

    



    if (!GetScriptReferent(thisobj)) {
        JS_ASSERT(!GetScriptReferent(thisobj));
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             clsname, fnname, "prototype object");
        return nullptr;
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
    RootedObject obj(cx, DebuggerScript_checkThis(cx, args, fnname));               \
    if (!obj)                                                                       \
        return false;                                                               \
    Rooted<JSScript*> script(cx, GetScriptReferent(obj))

static bool
DebuggerScript_getUrl(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get url)", args, obj, script);

    if (script->filename()) {
        JSString *str;
        if (script->scriptSource()->introducerFilename())
            str = NewStringCopyZ<CanGC>(cx, script->scriptSource()->introducerFilename());
        else
            str = NewStringCopyZ<CanGC>(cx, script->filename());
        if (!str)
            return false;
        args.rval().setString(str);
    } else {
        args.rval().setNull();
    }
    return true;
}

static bool
DebuggerScript_getStartLine(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get startLine)", args, obj, script);
    args.rval().setNumber(uint32_t(script->lineno()));
    return true;
}

static bool
DebuggerScript_getLineCount(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get lineCount)", args, obj, script);

    unsigned maxLine = js_GetScriptLineExtent(script);
    args.rval().setNumber(double(maxLine));
    return true;
}

static bool
DebuggerScript_getSource(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get source)", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    RootedScriptSource source(cx, &UncheckedUnwrap(script->sourceObject())->as<ScriptSourceObject>());
    RootedObject sourceObject(cx, dbg->wrapSource(cx, source));
    if (!sourceObject)
        return false;

    args.rval().setObject(*sourceObject);
    return true;
}

static bool
DebuggerScript_getSourceStart(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get sourceStart)", args, obj, script);
    args.rval().setNumber(uint32_t(script->sourceStart()));
    return true;
}

static bool
DebuggerScript_getSourceLength(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get sourceEnd)", args, obj, script);
    args.rval().setNumber(uint32_t(script->sourceEnd() - script->sourceStart()));
    return true;
}

static bool
DebuggerScript_getStaticLevel(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get staticLevel)", args, obj, script);
    args.rval().setNumber(uint32_t(script->staticLevel()));
    return true;
}

static bool
DebuggerScript_getSourceMapUrl(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get sourceMapURL)", args, obj, script);

    ScriptSource *source = script->scriptSource();
    JS_ASSERT(source);

    if (source->hasSourceMapURL()) {
        JSString *str = JS_NewUCStringCopyZ(cx, source->sourceMapURL());
        if (!str)
            return false;
        args.rval().setString(str);
    } else {
        args.rval().setNull();
    }

    return true;
}

static bool
DebuggerScript_getGlobal(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "(get global)", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    RootedValue v(cx, ObjectValue(script->global()));
    if (!dbg->wrapDebuggeeValue(cx, &v))
        return false;
    args.rval().set(v);
    return true;
}

static bool
DebuggerScript_getChildScripts(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getChildScripts", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    RootedObject result(cx, NewDenseEmptyArray(cx));
    if (!result)
        return false;
    if (script->hasObjects()) {
        





        ObjectArray *objects = script->objects();
        RootedFunction fun(cx);
        RootedScript funScript(cx);
        RootedObject obj(cx), s(cx);
        for (uint32_t i = script->innerObjectsStart(); i < objects->length; i++) {
            obj = objects->vector[i];
            if (obj->is<JSFunction>()) {
                fun = &obj->as<JSFunction>();
                
                if (fun->isNative())
                    continue;
                funScript = GetOrCreateFunctionScript(cx, fun);
                if (!funScript)
                    return false;
                s = dbg->wrapScript(cx, funScript);
                if (!s || !NewbornArrayPush(cx, result, ObjectValue(*s)))
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
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_BAD_OFFSET);
        return false;
    }
    *offsetp = off;
    return true;
}

static bool
DebuggerScript_getOffsetLine(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getOffsetLine", args, obj, script);
    if (!args.requireAtLeast(cx, "Debugger.Script.getOffsetLine", 1))
        return false;
    size_t offset;
    if (!ScriptOffset(cx, script, args[0], &offset))
        return false;
    unsigned lineno = JS_PCToLineNumber(cx, script, script->offsetToPC(offset));
    args.rval().setNumber(lineno);
    return true;
}

namespace {

class BytecodeRangeWithPosition : private BytecodeRange
{
  public:
    using BytecodeRange::empty;
    using BytecodeRange::frontPC;
    using BytecodeRange::frontOpcode;
    using BytecodeRange::frontOffset;

    BytecodeRangeWithPosition(JSContext *cx, JSScript *script)
      : BytecodeRange(cx, script), lineno(script->lineno()), column(0),
        sn(script->notes()), snpc(script->code())
    {
        if (!SN_IS_TERMINATOR(sn))
            snpc += SN_DELTA(sn);
        updatePosition();
        while (frontPC() != script->main())
            popFront();
    }

    void popFront() {
        BytecodeRange::popFront();
        if (!empty())
            updatePosition();
    }

    size_t frontLineNumber() const { return lineno; }
    size_t frontColumnNumber() const { return column; }

  private:
    void updatePosition() {
        



        while (!SN_IS_TERMINATOR(sn) && snpc <= frontPC()) {
            SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
            if (type == SRC_COLSPAN) {
                ptrdiff_t colspan = js_GetSrcNoteOffset(sn, 0);

                if (colspan >= SN_COLSPAN_DOMAIN / 2)
                    colspan -= SN_COLSPAN_DOMAIN;
                JS_ASSERT(ptrdiff_t(column) + colspan >= 0);
                column += colspan;
            } if (type == SRC_SETLINE) {
                lineno = size_t(js_GetSrcNoteOffset(sn, 0));
                column = 0;
            } else if (type == SRC_NEWLINE) {
                lineno++;
                column = 0;
            }

            sn = SN_NEXT(sn);
            snpc += SN_DELTA(sn);
        }
    }

    size_t lineno;
    size_t column;
    jssrcnote *sn;
    jsbytecode *snpc;
};






































class FlowGraphSummary {
  public:
    class Entry {
      public:
        static Entry createWithNoEdges() {
            return Entry(SIZE_MAX, 0);
        }

        static Entry createWithSingleEdge(size_t lineno, size_t column) {
            return Entry(lineno, column);
        }

        static Entry createWithMultipleEdgesFromSingleLine(size_t lineno) {
            return Entry(lineno, SIZE_MAX);
        }

        static Entry createWithMultipleEdgesFromMultipleLines() {
            return Entry(SIZE_MAX, SIZE_MAX);
        }

        Entry() {}

        bool hasNoEdges() const {
            return lineno_ == SIZE_MAX && column_ != SIZE_MAX;
        }

        bool hasSingleEdge() const {
            return lineno_ != SIZE_MAX && column_ != SIZE_MAX;
        }

        bool hasMultipleEdgesFromSingleLine() const {
            return lineno_ != SIZE_MAX && column_ == SIZE_MAX;
        }

        bool hasMultipleEdgesFromMultipleLines() const {
            return lineno_ == SIZE_MAX && column_ == SIZE_MAX;
        }

        bool operator==(const Entry &other) const {
            return lineno_ == other.lineno_ && column_ == other.column_;
        }

        bool operator!=(const Entry &other) const {
            return lineno_ != other.lineno_ || column_ != other.column_;
        }

        size_t lineno() const {
            return lineno_;
        }

        size_t column() const {
            return column_;
        }

      private:
        Entry(size_t lineno, size_t column) : lineno_(lineno), column_(column) {}

        size_t lineno_;
        size_t column_;
    };

    explicit FlowGraphSummary(JSContext *cx) : entries_(cx) {}

    Entry &operator[](size_t index) {
        return entries_[index];
    }

    bool populate(JSContext *cx, JSScript *script) {
        if (!entries_.growBy(script->length()))
            return false;
        unsigned mainOffset = script->pcToOffset(script->main());
        entries_[mainOffset] = Entry::createWithMultipleEdgesFromMultipleLines();
        for (size_t i = mainOffset + 1; i < script->length(); i++)
            entries_[i] = Entry::createWithNoEdges();

        size_t prevLineno = script->lineno();
        size_t prevColumn = 0;
        JSOp prevOp = JSOP_NOP;
        for (BytecodeRangeWithPosition r(cx, script); !r.empty(); r.popFront()) {
            size_t lineno = r.frontLineNumber();
            size_t column = r.frontColumnNumber();
            JSOp op = r.frontOpcode();

            if (FlowsIntoNext(prevOp))
                addEdge(prevLineno, prevColumn, r.frontOffset());

            if (js_CodeSpec[op].type() == JOF_JUMP) {
                addEdge(lineno, column, r.frontOffset() + GET_JUMP_OFFSET(r.frontPC()));
            } else if (op == JSOP_TABLESWITCH) {
                jsbytecode *pc = r.frontPC();
                size_t offset = r.frontOffset();
                ptrdiff_t step = JUMP_OFFSET_LEN;
                size_t defaultOffset = offset + GET_JUMP_OFFSET(pc);
                pc += step;
                addEdge(lineno, column, defaultOffset);

                int32_t low = GET_JUMP_OFFSET(pc);
                pc += JUMP_OFFSET_LEN;
                int ncases = GET_JUMP_OFFSET(pc) - low + 1;
                pc += JUMP_OFFSET_LEN;

                for (int i = 0; i < ncases; i++) {
                    size_t target = offset + GET_JUMP_OFFSET(pc);
                    addEdge(lineno, column, target);
                    pc += step;
                }
            }

            prevLineno = lineno;
            prevColumn = column;
            prevOp = op;
        }

        return true;
    }

  private:
    void addEdge(size_t sourceLineno, size_t sourceColumn, size_t targetOffset) {
        if (entries_[targetOffset].hasNoEdges())
            entries_[targetOffset] = Entry::createWithSingleEdge(sourceLineno, sourceColumn);
        else if (entries_[targetOffset].lineno() != sourceLineno)
            entries_[targetOffset] = Entry::createWithMultipleEdgesFromMultipleLines();
        else if (entries_[targetOffset].column() != sourceColumn)
            entries_[targetOffset] = Entry::createWithMultipleEdgesFromSingleLine(sourceLineno);
    }

    Vector<Entry> entries_;
};

} 

static bool
DebuggerScript_getAllOffsets(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getAllOffsets", args, obj, script);

    



    FlowGraphSummary flowData(cx);
    if (!flowData.populate(cx, script))
        return false;

    
    RootedObject result(cx, NewDenseEmptyArray(cx));
    if (!result)
        return false;
    for (BytecodeRangeWithPosition r(cx, script); !r.empty(); r.popFront()) {
        size_t offset = r.frontOffset();
        size_t lineno = r.frontLineNumber();

        
        if (!flowData[offset].hasNoEdges() && flowData[offset].lineno() != lineno) {
            
            RootedObject offsets(cx);
            RootedValue offsetsv(cx);

            RootedId id(cx, INT_TO_JSID(lineno));

            bool found;
            if (!js::HasOwnProperty(cx, result, id, &found))
                return false;
            if (found && !JSObject::getGeneric(cx, result, result, id, &offsetsv))
                return false;

            if (offsetsv.isObject()) {
                offsets = &offsetsv.toObject();
            } else {
                JS_ASSERT(offsetsv.isUndefined());

                



                RootedId id(cx);
                RootedValue v(cx, NumberValue(lineno));
                offsets = NewDenseEmptyArray(cx);
                if (!offsets ||
                    !ValueToId<CanGC>(cx, v, &id))
                {
                    return false;
                }

                RootedValue value(cx, ObjectValue(*offsets));
                if (!JSObject::defineGeneric(cx, result, id, value))
                    return false;
            }

            
            if (!NewbornArrayPush(cx, offsets, NumberValue(offset)))
                return false;
        }
    }

    args.rval().setObject(*result);
    return true;
}

static bool
DebuggerScript_getAllColumnOffsets(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getAllColumnOffsets", args, obj, script);

    



    FlowGraphSummary flowData(cx);
    if (!flowData.populate(cx, script))
        return false;

    
    RootedObject result(cx, NewDenseEmptyArray(cx));
    if (!result)
        return false;
    for (BytecodeRangeWithPosition r(cx, script); !r.empty(); r.popFront()) {
        size_t lineno = r.frontLineNumber();
        size_t column = r.frontColumnNumber();
        size_t offset = r.frontOffset();

        
        if (!flowData[offset].hasNoEdges() &&
            (flowData[offset].lineno() != lineno ||
             flowData[offset].column() != column)) {
            RootedObject entry(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
            if (!entry)
                return false;

            RootedId id(cx, NameToId(cx->names().lineNumber));
            RootedValue value(cx, NumberValue(lineno));
            if (!JSObject::defineGeneric(cx, entry, id, value))
                return false;

            value = NumberValue(column);
            if (!JSObject::defineProperty(cx, entry, cx->names().columnNumber, value))
                return false;

            id = NameToId(cx->names().offset);
            value = NumberValue(offset);
            if (!JSObject::defineGeneric(cx, entry, id, value))
                return false;

            if (!NewbornArrayPush(cx, result, ObjectValue(*entry)))
                return false;
        }
    }

    args.rval().setObject(*result);
    return true;
}

static bool
DebuggerScript_getLineOffsets(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getLineOffsets", args, obj, script);
    if (!args.requireAtLeast(cx, "Debugger.Script.getLineOffsets", 1))
        return false;

    
    RootedValue linenoValue(cx, args[0]);
    size_t lineno;
    if (!ToNumber(cx, &linenoValue))
        return false;
    {
        double d = linenoValue.toNumber();
        lineno = size_t(d);
        if (lineno != d) {
            JS_ReportErrorNumber(cx,  js_GetErrorMessage, nullptr, JSMSG_DEBUG_BAD_LINE);
            return false;
        }
    }

    



    FlowGraphSummary flowData(cx);
    if (!flowData.populate(cx, script))
        return false;

    
    RootedObject result(cx, NewDenseEmptyArray(cx));
    if (!result)
        return false;
    for (BytecodeRangeWithPosition r(cx, script); !r.empty(); r.popFront()) {
        size_t offset = r.frontOffset();

        
        if (r.frontLineNumber() == lineno &&
            !flowData[offset].hasNoEdges() &&
            flowData[offset].lineno() != lineno)
        {
            if (!NewbornArrayPush(cx, result, NumberValue(offset)))
                return false;
        }
    }

    args.rval().setObject(*result);
    return true;
}

bool
Debugger::observesFrame(AbstractFramePtr frame) const
{
    return observesScript(frame.script());
}

bool
Debugger::observesFrame(const ScriptFrameIter &iter) const
{
    return observesScript(iter.script());
}

bool
Debugger::observesScript(JSScript *script) const
{
    if (!enabled)
        return false;
    return observesGlobal(&script->global()) && (!script->selfHosted() ||
                                                 SelfHostedFramesVisible());
}

 bool
Debugger::replaceFrameGuts(JSContext *cx, AbstractFramePtr from, AbstractFramePtr to,
                           ScriptFrameIter &iter)
{
    for (Debugger::FrameRange r(from); !r.empty(); r.popFront()) {
        RootedObject frameobj(cx, r.frontFrame());
        Debugger *dbg = r.frontDebugger();
        JS_ASSERT(dbg == Debugger::fromChildJSObject(frameobj));

        
        DebuggerFrame_freeScriptFrameIterData(cx->runtime()->defaultFreeOp(), frameobj);
        ScriptFrameIter::Data *data = iter.copyData();
        if (!data)
            return false;
        frameobj->setPrivate(data);

        
        r.removeFrontFrame();

        
        if (!dbg->frames.putNew(to, frameobj)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    return true;
}

 bool
Debugger::handleBaselineOsr(JSContext *cx, InterpreterFrame *from, jit::BaselineFrame *to)
{
    ScriptFrameIter iter(cx);
    JS_ASSERT(iter.abstractFramePtr() == to);
    return replaceFrameGuts(cx, from, to, iter);
}

 bool
Debugger::handleIonBailout(JSContext *cx, jit::RematerializedFrame *from, jit::BaselineFrame *to)
{
    
    
    
    
    
    
    
    
    ScriptFrameIter iter(cx);
    while (iter.abstractFramePtr() != to)
        ++iter;
    return replaceFrameGuts(cx, from, to, iter);
}

 void
Debugger::propagateForcedReturn(JSContext *cx, AbstractFramePtr frame, HandleValue rval)
{
    
    
    
    
    
    
    
    
    
    MOZ_ASSERT(!cx->isExceptionPending());
    cx->setPropagatingForcedReturn();
    frame.setReturnValue(rval);
}

static bool
DebuggerScript_setBreakpoint(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "setBreakpoint", args, obj, script);
    if (!args.requireAtLeast(cx, "Debugger.Script.setBreakpoint", 2))
        return false;
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    if (!dbg->observesScript(script)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_DEBUGGING);
        return false;
    }

    size_t offset;
    if (!ScriptOffset(cx, script, args[0], &offset))
        return false;

    JSObject *handler = NonNullObject(cx, args[1]);
    if (!handler)
        return false;

    jsbytecode *pc = script->offsetToPC(offset);
    BreakpointSite *site = script->getOrCreateBreakpointSite(cx, pc);
    if (!site)
        return false;
    site->inc(cx->runtime()->defaultFreeOp());
    if (cx->runtime()->new_<Breakpoint>(dbg, site, handler)) {
        args.rval().setUndefined();
        return true;
    }
    site->dec(cx->runtime()->defaultFreeOp());
    site->destroyIfEmpty(cx->runtime()->defaultFreeOp());
    return false;
}

static bool
DebuggerScript_getBreakpoints(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "getBreakpoints", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    jsbytecode *pc;
    if (args.length() > 0) {
        size_t offset;
        if (!ScriptOffset(cx, script, args[0], &offset))
            return false;
        pc = script->offsetToPC(offset);
    } else {
        pc = nullptr;
    }

    RootedObject arr(cx, NewDenseEmptyArray(cx));
    if (!arr)
        return false;

    for (unsigned i = 0; i < script->length(); i++) {
        BreakpointSite *site = script->getBreakpointSite(script->offsetToPC(i));
        if (site && (!pc || site->pc == pc)) {
            for (Breakpoint *bp = site->firstBreakpoint(); bp; bp = bp->nextInSite()) {
                if (bp->debugger == dbg &&
                    !NewbornArrayPush(cx, arr, ObjectValue(*bp->getHandler())))
                {
                    return false;
                }
            }
        }
    }
    args.rval().setObject(*arr);
    return true;
}

static bool
DebuggerScript_clearBreakpoint(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "clearBreakpoint", args, obj, script);
    if (!args.requireAtLeast(cx, "Debugger.Script.clearBreakpoint", 1))
        return false;
    Debugger *dbg = Debugger::fromChildJSObject(obj);

    JSObject *handler = NonNullObject(cx, args[0]);
    if (!handler)
        return false;

    script->clearBreakpointsIn(cx->runtime()->defaultFreeOp(), dbg, handler);
    args.rval().setUndefined();
    return true;
}

static bool
DebuggerScript_clearAllBreakpoints(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "clearAllBreakpoints", args, obj, script);
    Debugger *dbg = Debugger::fromChildJSObject(obj);
    script->clearBreakpointsIn(cx->runtime()->defaultFreeOp(), dbg, nullptr);
    args.rval().setUndefined();
    return true;
}

static bool
DebuggerScript_isInCatchScope(JSContext *cx, unsigned argc, Value* vp)
{
    THIS_DEBUGSCRIPT_SCRIPT(cx, argc, vp, "isInCatchScope", args, obj, script);
    if (!args.requireAtLeast(cx, "Debugger.Script.isInCatchScope", 1))
        return false;

    size_t offset;
    if (!ScriptOffset(cx, script, args[0], &offset))
        return false;

    



    offset -= script->mainOffset();

    args.rval().setBoolean(false);
    if (script->hasTrynotes()) {
        JSTryNote* tnBegin = script->trynotes()->vector;
        JSTryNote* tnEnd = tnBegin + script->trynotes()->length;
        while (tnBegin != tnEnd) {
            if (tnBegin->start <= offset &&
                offset <= tnBegin->start + tnBegin->length &&
                tnBegin->kind == JSTRY_CATCH)
            {
                args.rval().setBoolean(true);
                break;
            }
            ++tnBegin;
        }
    }
    return true;
}

static bool
DebuggerScript_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Script");
    return false;
}

static const JSPropertySpec DebuggerScript_properties[] = {
    JS_PSG("url", DebuggerScript_getUrl, 0),
    JS_PSG("startLine", DebuggerScript_getStartLine, 0),
    JS_PSG("lineCount", DebuggerScript_getLineCount, 0),
    JS_PSG("source", DebuggerScript_getSource, 0),
    JS_PSG("sourceStart", DebuggerScript_getSourceStart, 0),
    JS_PSG("sourceLength", DebuggerScript_getSourceLength, 0),
    JS_PSG("staticLevel", DebuggerScript_getStaticLevel, 0),
    JS_PSG("sourceMapURL", DebuggerScript_getSourceMapUrl, 0),
    JS_PSG("global", DebuggerScript_getGlobal, 0),
    JS_PS_END
};

static const JSFunctionSpec DebuggerScript_methods[] = {
    JS_FN("getChildScripts", DebuggerScript_getChildScripts, 0, 0),
    JS_FN("getAllOffsets", DebuggerScript_getAllOffsets, 0, 0),
    JS_FN("getAllColumnOffsets", DebuggerScript_getAllColumnOffsets, 0, 0),
    JS_FN("getLineOffsets", DebuggerScript_getLineOffsets, 1, 0),
    JS_FN("getOffsetLine", DebuggerScript_getOffsetLine, 0, 0),
    JS_FN("setBreakpoint", DebuggerScript_setBreakpoint, 2, 0),
    JS_FN("getBreakpoints", DebuggerScript_getBreakpoints, 1, 0),
    JS_FN("clearBreakpoint", DebuggerScript_clearBreakpoint, 1, 0),
    JS_FN("clearAllBreakpoints", DebuggerScript_clearAllBreakpoints, 0, 0),
    JS_FN("isInCatchScope", DebuggerScript_isInCatchScope, 1, 0),
    JS_FS_END
};




static inline ScriptSourceObject *
GetSourceReferent(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &DebuggerSource_class);
    return static_cast<ScriptSourceObject *>(obj->getPrivate());
}

static void
DebuggerSource_trace(JSTracer *trc, JSObject *obj)
{
    



    if (JSObject *referent = GetSourceReferent(obj)) {
        MarkCrossCompartmentObjectUnbarriered(trc, obj, &referent, "Debugger.Source referent");
        obj->setPrivateUnbarriered(referent);
    }
}

const Class DebuggerSource_class = {
    "Source",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGSOURCE_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr,              
    nullptr,              
    nullptr,              
    DebuggerSource_trace
};

JSObject *
Debugger::newDebuggerSource(JSContext *cx, HandleScriptSource source)
{
    assertSameCompartment(cx, object.get());

    JSObject *proto = &object->getReservedSlot(JSSLOT_DEBUG_SOURCE_PROTO).toObject();
    JS_ASSERT(proto);
    JSObject *sourceobj = NewObjectWithGivenProto(cx, &DebuggerSource_class, proto, nullptr, TenuredObject);
    if (!sourceobj)
        return nullptr;
    sourceobj->setReservedSlot(JSSLOT_DEBUGSOURCE_OWNER, ObjectValue(*object));
    sourceobj->setPrivateGCThing(source);

    return sourceobj;
}

JSObject *
Debugger::wrapSource(JSContext *cx, HandleScriptSource source)
{
    assertSameCompartment(cx, object.get());
    JS_ASSERT(cx->compartment() != source->compartment());
    DependentAddPtr<SourceWeakMap> p(cx, sources, source);
    if (!p) {
        JSObject *sourceobj = newDebuggerSource(cx, source);
        if (!sourceobj)
            return nullptr;

        if (!p.add(cx, sources, source, sourceobj)) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }

        CrossCompartmentKey key(CrossCompartmentKey::DebuggerSource, object, source);
        if (!object->compartment()->putWrapper(cx, key, ObjectValue(*sourceobj))) {
            sources.remove(source);
            js_ReportOutOfMemory(cx);
            return nullptr;
        }
    }

    JS_ASSERT(GetSourceReferent(p->value()) == source);
    return p->value();
}

static bool
DebuggerSource_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Source");
    return false;
}

static JSObject *
DebuggerSource_checkThis(JSContext *cx, const CallArgs &args, const char *fnname)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return nullptr;
    }

    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerSource_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Source", fnname, thisobj->getClass()->name);
        return nullptr;
    }

    if (!GetSourceReferent(thisobj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Frame", fnname, "prototype object");
        return nullptr;
    }

    return thisobj;
}

#define THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, fnname, args, obj, sourceObject)    \
    CallArgs args = CallArgsFromVp(argc, vp);                                       \
    RootedObject obj(cx, DebuggerSource_checkThis(cx, args, fnname));               \
    if (!obj)                                                                       \
        return false;                                                               \
    RootedScriptSource sourceObject(cx, GetSourceReferent(obj));                    \
    if (!sourceObject)                                                              \
        return false;

static bool
DebuggerSource_getText(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get text)", args, obj, sourceObject);

    ScriptSource *ss = sourceObject->source();
    bool hasSourceData = ss->hasSourceData();
    if (!ss->hasSourceData() && !JSScript::loadSource(cx, ss, &hasSourceData))
        return false;

    JSString *str = hasSourceData ? ss->substring(cx, 0, ss->length())
                                  : NewStringCopyZ<CanGC>(cx, "[no source]");
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

static bool
DebuggerSource_getUrl(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get url)", args, obj, sourceObject);

    ScriptSource *ss = sourceObject->source();
    if (ss->filename()) {
        JSString *str = NewStringCopyZ<CanGC>(cx, ss->filename());
        if (!str)
            return false;
        args.rval().setString(str);
    } else {
        args.rval().setNull();
    }
    return true;
}

static bool
DebuggerSource_getDisplayURL(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get url)", args, obj, sourceObject);

    ScriptSource *ss = sourceObject->source();
    JS_ASSERT(ss);

    if (ss->hasDisplayURL()) {
        JSString *str = JS_NewUCStringCopyZ(cx, ss->displayURL());
        if (!str)
            return false;
        args.rval().setString(str);
    } else {
        args.rval().setNull();
    }

    return true;
}

static bool
DebuggerSource_getElement(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get element)", args, obj, sourceObject);

    if (sourceObject->element()) {
        args.rval().setObjectOrNull(sourceObject->element());
        if (!Debugger::fromChildJSObject(obj)->wrapDebuggeeValue(cx, args.rval()))
            return false;
    } else {
        args.rval().setUndefined();
    }
    return true;
}

static bool
DebuggerSource_getElementProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get elementAttributeName)", args, obj, sourceObject);
    args.rval().set(sourceObject->elementAttributeName());
    return Debugger::fromChildJSObject(obj)->wrapDebuggeeValue(cx, args.rval());
}

static bool
DebuggerSource_getIntroductionScript(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get introductionScript)", args, obj, sourceObject);

    RootedScript script(cx, sourceObject->introductionScript());
    if (script) {
        RootedObject scriptDO(cx, Debugger::fromChildJSObject(obj)->wrapScript(cx, script));
        if (!scriptDO)
            return false;
        args.rval().setObject(*scriptDO);
    } else {
        args.rval().setUndefined();
    }
    return true;
}

static bool
DebuggerSource_getIntroductionOffset(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get introductionOffset)", args, obj, sourceObject);

    
    
    
    ScriptSource *ss = sourceObject->source();
    if (ss->hasIntroductionOffset() && sourceObject->introductionScript())
        args.rval().setInt32(ss->introductionOffset());
    else
        args.rval().setUndefined();
    return true;
}

static bool
DebuggerSource_getIntroductionType(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGSOURCE_REFERENT(cx, argc, vp, "(get introductionType)", args, obj, sourceObject);

    ScriptSource *ss = sourceObject->source();
    if (ss->hasIntroductionType()) {
        JSString *str = NewStringCopyZ<CanGC>(cx, ss->introductionType());
        if (!str)
            return false;
        args.rval().setString(str);
    } else {
        args.rval().setUndefined();
    }
    return true;
}

static const JSPropertySpec DebuggerSource_properties[] = {
    JS_PSG("text", DebuggerSource_getText, 0),
    JS_PSG("url", DebuggerSource_getUrl, 0),
    JS_PSG("element", DebuggerSource_getElement, 0),
    JS_PSG("displayURL", DebuggerSource_getDisplayURL, 0),
    JS_PSG("introductionScript", DebuggerSource_getIntroductionScript, 0),
    JS_PSG("introductionOffset", DebuggerSource_getIntroductionOffset, 0),
    JS_PSG("introductionType", DebuggerSource_getIntroductionType, 0),
    JS_PSG("elementAttributeName", DebuggerSource_getElementProperty, 0),
    JS_PS_END
};

static const JSFunctionSpec DebuggerSource_methods[] = {
    JS_FS_END
};




static void
UpdateFrameIterPc(FrameIter &iter)
{
    if (iter.abstractFramePtr().isRematerializedFrame()) {
#ifdef DEBUG
        
        
        
        
        
        
        
        
        jit::RematerializedFrame *frame = iter.abstractFramePtr().asRematerializedFrame();
        jit::IonJSFrameLayout *jsFrame = (jit::IonJSFrameLayout *)frame->top();
        jit::JitActivation *activation = iter.activation()->asJit();

        ActivationIterator activationIter(activation->cx()->perThreadData);
        while (activationIter.activation() != activation)
            ++activationIter;

        jit::JitFrameIterator jitIter(activationIter);
        while (!jitIter.isIonJS() || jitIter.jsFrame() != jsFrame)
            ++jitIter;

        jit::InlineFrameIterator ionInlineIter(activation->cx(), &jitIter);
        while (ionInlineIter.frameNo() != frame->frameNo())
            ++ionInlineIter;

        MOZ_ASSERT(ionInlineIter.pc() == iter.pc());
#endif
        return;
    }

    iter.updatePcQuadratic();
}

static void
DebuggerFrame_freeScriptFrameIterData(FreeOp *fop, JSObject *obj)
{
    AbstractFramePtr frame = AbstractFramePtr::FromRaw(obj->getPrivate());
    if (frame.isScriptFrameIterData())
        fop->delete_((ScriptFrameIter::Data *) frame.raw());
    obj->setPrivate(nullptr);
}

static void
DebuggerFrame_maybeDecrementFrameScriptStepModeCount(FreeOp *fop, AbstractFramePtr frame,
                                                     JSObject *frameobj)
{
    
    if (!frameobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER).isUndefined())
        frame.script()->decrementStepModeCount(fop);
}

static void
DebuggerFrame_finalize(FreeOp *fop, JSObject *obj)
{
    DebuggerFrame_freeScriptFrameIterData(fop, obj);
}

const Class DebuggerFrame_class = {
    "Frame", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGFRAME_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, DebuggerFrame_finalize
};

static JSObject *
CheckThisFrame(JSContext *cx, const CallArgs &args, const char *fnname, bool checkLive)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return nullptr;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerFrame_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Frame", fnname, thisobj->getClass()->name);
        return nullptr;
    }

    





    if (!thisobj->getPrivate()) {
        if (thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_OWNER).isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                                 "Debugger.Frame", fnname, "prototype object");
            return nullptr;
        }
        if (checkLive) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_LIVE,
                                 "Debugger.Frame");
            return nullptr;
        }
    }
    return thisobj;
}


















#define THIS_FRAME_THISOBJ(cx, argc, vp, fnname, args, thisobj)                \
    CallArgs args = CallArgsFromVp(argc, vp);                                  \
    RootedObject thisobj(cx, CheckThisFrame(cx, args, fnname, true));          \
    if (!thisobj)                                                              \
        return false

#define THIS_FRAME(cx, argc, vp, fnname, args, thisobj, frame)                 \
    THIS_FRAME_THISOBJ(cx, argc, vp, fnname, args, thisobj);                   \
    AbstractFramePtr frame = AbstractFramePtr::FromRaw(thisobj->getPrivate()); \
    if (frame.isScriptFrameIterData()) {                                       \
        ScriptFrameIter iter(*(ScriptFrameIter::Data *)(frame.raw()));         \
        frame = iter.abstractFramePtr();                                       \
    }

#define THIS_FRAME_ITER(cx, argc, vp, fnname, args, thisobj, maybeIter, iter)  \
    THIS_FRAME_THISOBJ(cx, argc, vp, fnname, args, thisobj);                   \
    Maybe<ScriptFrameIter> maybeIter;                                          \
    {                                                                          \
        AbstractFramePtr f = AbstractFramePtr::FromRaw(thisobj->getPrivate()); \
        if (f.isScriptFrameIterData()) {                                       \
            maybeIter.emplace(*(ScriptFrameIter::Data *)(f.raw()));            \
        } else {                                                               \
            maybeIter.emplace(cx, ScriptFrameIter::ALL_CONTEXTS,               \
                              ScriptFrameIter::GO_THROUGH_SAVED);              \
            ScriptFrameIter &iter = *maybeIter;                                \
            while (iter.isIon() || iter.abstractFramePtr() != f)               \
                ++iter;                                                        \
            AbstractFramePtr data = iter.copyDataAsAbstractFramePtr();         \
            if (!data)                                                         \
                return false;                                                  \
            thisobj->setPrivate(data.raw());                                   \
        }                                                                      \
    }                                                                          \
    ScriptFrameIter &iter = *maybeIter

#define THIS_FRAME_OWNER(cx, argc, vp, fnname, args, thisobj, frame, dbg)      \
    THIS_FRAME(cx, argc, vp, fnname, args, thisobj, frame);                    \
    Debugger *dbg = Debugger::fromChildJSObject(thisobj)

#define THIS_FRAME_OWNER_ITER(cx, argc, vp, fnname, args, thisobj, maybeIter, iter, dbg) \
    THIS_FRAME_ITER(cx, argc, vp, fnname, args, thisobj, maybeIter, iter);               \
    Debugger *dbg = Debugger::fromChildJSObject(thisobj)

static bool
DebuggerFrame_getType(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get type", args, thisobj, frame);

    



    args.rval().setString(frame.isEvalFrame()
                          ? cx->names().eval
                          : frame.isGlobalFrame()
                          ? cx->names().global
                          : cx->names().call);
    return true;
}

static bool
DebuggerFrame_getImplementation(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get implementation", args, thisobj, frame);

    const char *s;
    if (frame.isBaselineFrame())
        s = "baseline";
    else if (frame.isRematerializedFrame())
        s = "ion";
    else
        s = "interpreter";

    JSAtom *str = Atomize(cx, s, strlen(s));
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

static bool
DebuggerFrame_getEnvironment(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_OWNER_ITER(cx, argc, vp, "get environment", args, thisobj, _, iter, dbg);

    Rooted<Env*> env(cx);
    {
        AutoCompartment ac(cx, iter.abstractFramePtr().scopeChain());
        UpdateFrameIterPc(iter);
        env = GetDebugScopeForFrame(cx, iter.abstractFramePtr(), iter.pc());
        if (!env)
            return false;
    }

    return dbg->wrapEnvironment(cx, env, args.rval());
}

static bool
DebuggerFrame_getCallee(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get callee", args, thisobj, frame);
    RootedValue calleev(cx, frame.isNonEvalFunctionFrame() ? frame.calleev() : NullValue());
    if (!Debugger::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, &calleev))
        return false;
    args.rval().set(calleev);
    return true;
}

static bool
DebuggerFrame_getGenerator(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get generator", args, thisobj, frame);
    args.rval().setBoolean(frame.isGeneratorFrame());
    return true;
}

static bool
DebuggerFrame_getConstructing(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_ITER(cx, argc, vp, "get constructing", args, thisobj, _, iter);
    args.rval().setBoolean(iter.isFunctionFrame() && iter.isConstructing());
    return true;
}

static bool
DebuggerFrame_getThis(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_ITER(cx, argc, vp, "get this", args, thisobj, _, iter);
    RootedValue thisv(cx);
    {
        AutoCompartment ac(cx, iter.scopeChain());
        if (!iter.computeThis(cx))
            return false;
        thisv = iter.computedThisValue();
    }
    if (!Debugger::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, &thisv))
        return false;
    args.rval().set(thisv);
    return true;
}

static bool
DebuggerFrame_getOlder(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_ITER(cx, argc, vp, "get this", args, thisobj, _, iter);
    Debugger *dbg = Debugger::fromChildJSObject(thisobj);

    for (++iter; !iter.done(); ++iter) {
        if (dbg->observesFrame(iter)) {
            if (iter.isIon() && !iter.ensureHasRematerializedFrame(cx))
                return false;
            return dbg->getScriptFrame(cx, iter, args.rval());
        }
    }
    args.rval().setNull();
    return true;
}

const Class DebuggerArguments_class = {
    "Arguments", JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGARGUMENTS_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};


static bool
DebuggerArguments_getArg(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    int32_t i = args.callee().as<JSFunction>().getExtendedSlot(0).toInt32();

    
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return false;
    }
    RootedObject argsobj(cx, &args.thisv().toObject());
    if (argsobj->getClass() != &DebuggerArguments_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Arguments", "getArgument", argsobj->getClass()->name);
        return false;
    }

    



    args.setThis(argsobj->getReservedSlot(JSSLOT_DEBUGARGUMENTS_FRAME));
    THIS_FRAME(cx, argc, vp, "get argument", ca2, thisobj, frame);

    



    JS_ASSERT(i >= 0);
    RootedValue arg(cx);
    RootedScript script(cx);
    if (unsigned(i) < frame.numActualArgs()) {
        script = frame.script();
        if (unsigned(i) < frame.numFormalArgs() && script->formalIsAliased(i)) {
            for (AliasedFormalIter fi(script); ; fi++) {
                if (fi.frameIndex() == unsigned(i)) {
                    arg = frame.callObj().aliasedVar(fi);
                    break;
                }
            }
        } else if (script->argsObjAliasesFormals() && frame.hasArgsObj()) {
            arg = frame.argsObj().arg(i);
        } else {
            arg = frame.unaliasedActual(i, DONT_CHECK_ALIASING);
        }
    } else {
        arg.setUndefined();
    }

    if (!Debugger::fromChildJSObject(thisobj)->wrapDebuggeeValue(cx, &arg))
        return false;
    args.rval().set(arg);
    return true;
}

static bool
DebuggerFrame_getArguments(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get arguments", args, thisobj, frame);
    Value argumentsv = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ARGUMENTS);
    if (!argumentsv.isUndefined()) {
        JS_ASSERT(argumentsv.isObjectOrNull());
        args.rval().set(argumentsv);
        return true;
    }

    RootedObject argsobj(cx);
    if (frame.hasArgs()) {
        
        Rooted<GlobalObject*> global(cx, &args.callee().global());
        JSObject *proto = GlobalObject::getOrCreateArrayPrototype(cx, global);
        if (!proto)
            return false;
        argsobj = NewObjectWithGivenProto(cx, &DebuggerArguments_class, proto, global);
        if (!argsobj)
            return false;
        SetReservedSlot(argsobj, JSSLOT_DEBUGARGUMENTS_FRAME, ObjectValue(*thisobj));

        JS_ASSERT(frame.numActualArgs() <= 0x7fffffff);
        unsigned fargc = frame.numActualArgs();
        RootedValue fargcVal(cx, Int32Value(fargc));
        if (!DefineNativeProperty(cx, argsobj, cx->names().length,
                                  fargcVal, nullptr, nullptr,
                                  JSPROP_PERMANENT | JSPROP_READONLY))
        {
            return false;
        }

        Rooted<jsid> id(cx);
        for (unsigned i = 0; i < fargc; i++) {
            RootedFunction getobj(cx);
            getobj = NewFunction(cx, js::NullPtr(), DebuggerArguments_getArg, 0,
                                 JSFunction::NATIVE_FUN, global, js::NullPtr(),
                                 JSFunction::ExtendedFinalizeKind);
            if (!getobj)
                return false;
            id = INT_TO_JSID(i);
            if (!getobj ||
                !DefineNativeProperty(cx, argsobj, id, UndefinedHandleValue,
                                      JS_DATA_TO_FUNC_PTR(PropertyOp, getobj.get()), nullptr,
                                      JSPROP_ENUMERATE | JSPROP_SHARED | JSPROP_GETTER))
            {
                return false;
            }
            getobj->setExtendedSlot(0, Int32Value(i));
        }
    } else {
        argsobj = nullptr;
    }
    args.rval().setObjectOrNull(argsobj);
    thisobj->setReservedSlot(JSSLOT_DEBUGFRAME_ARGUMENTS, args.rval());
    return true;
}

static bool
DebuggerFrame_getScript(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get script", args, thisobj, frame);
    Debugger *debug = Debugger::fromChildJSObject(thisobj);

    RootedObject scriptObject(cx);
    if (frame.isFunctionFrame() && !frame.isEvalFrame()) {
        RootedFunction callee(cx, frame.callee());
        if (callee->isInterpreted()) {
            RootedScript script(cx, callee->nonLazyScript());
            scriptObject = debug->wrapScript(cx, script);
            if (!scriptObject)
                return false;
        }
    } else {
        



        RootedScript script(cx, frame.script());
        scriptObject = debug->wrapScript(cx, script);
        if (!scriptObject)
            return false;
    }
    args.rval().setObjectOrNull(scriptObject);
    return true;
}

static bool
DebuggerFrame_getOffset(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_ITER(cx, argc, vp, "get offset", args, thisobj, _, iter);
    JSScript *script = iter.script();
    UpdateFrameIterPc(iter);
    jsbytecode *pc = iter.pc();
    size_t offset = script->pcToOffset(pc);
    args.rval().setNumber(double(offset));
    return true;
}

static bool
DebuggerFrame_getLive(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject *thisobj = CheckThisFrame(cx, args, "get live", false);
    if (!thisobj)
        return false;
    bool hasFrame = !!thisobj->getPrivate();
    args.rval().setBoolean(hasFrame);
    return true;
}

static bool
IsValidHook(const Value &v)
{
    return v.isUndefined() || (v.isObject() && v.toObject().isCallable());
}

static bool
DebuggerFrame_getOnStep(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get onStep", args, thisobj, frame);
    (void) frame;  
    Value handler = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER);
    JS_ASSERT(IsValidHook(handler));
    args.rval().set(handler);
    return true;
}

static bool
DebuggerFrame_setOnStep(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "set onStep", args, thisobj, frame);
    if (!args.requireAtLeast(cx, "Debugger.Frame.set onStep", 1))
        return false;
    if (!IsValidHook(args[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_CALLABLE_OR_UNDEFINED);
        return false;
    }

    Value prior = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER);
    if (!args[0].isUndefined() && prior.isUndefined()) {
        
        AutoCompartment ac(cx, frame.scopeChain());
        if (!frame.script()->incrementStepModeCount(cx))
            return false;
    } else if (args[0].isUndefined() && !prior.isUndefined()) {
        
        frame.script()->decrementStepModeCount(cx->runtime()->defaultFreeOp());
    }

    
    thisobj->setReservedSlot(JSSLOT_DEBUGFRAME_ONSTEP_HANDLER, args[0]);
    args.rval().setUndefined();
    return true;
}

static bool
DebuggerFrame_getOnPop(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "get onPop", args, thisobj, frame);
    (void) frame;  
    Value handler = thisobj->getReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER);
    JS_ASSERT(IsValidHook(handler));
    args.rval().set(handler);
    return true;
}

static bool
DebuggerFrame_setOnPop(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME(cx, argc, vp, "set onPop", args, thisobj, frame);
    if (!args.requireAtLeast(cx, "Debugger.Frame.set onPop", 1))
        return false;
    (void) frame;  
    if (!IsValidHook(args[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_CALLABLE_OR_UNDEFINED);
        return false;
    }

    thisobj->setReservedSlot(JSSLOT_DEBUGFRAME_ONPOP_HANDLER, args[0]);
    args.rval().setUndefined();
    return true;
}











bool
js::EvaluateInEnv(JSContext *cx, Handle<Env*> env, HandleValue thisv, AbstractFramePtr frame,
                  mozilla::Range<const char16_t> chars, const char *filename, unsigned lineno,
                  MutableHandleValue rval)
{
    assertSameCompartment(cx, env, frame);
    JS_ASSERT_IF(frame, thisv.get() == frame.thisValue());

    JS_ASSERT(!IsPoisonedPtr(chars.start().get()));

    




    CompileOptions options(cx);
    options.setCompileAndGo(true)
           .setForEval(true)
           .setNoScriptRval(false)
           .setFileAndLine(filename, lineno)
           .setCanLazilyParse(false)
           .setIntroductionType("debugger eval");
    RootedScript callerScript(cx, frame ? frame.script() : nullptr);
    SourceBufferHolder srcBuf(chars.start().get(), chars.length(), SourceBufferHolder::NoOwnership);
    RootedScript script(cx, frontend::CompileScript(cx, &cx->tempLifoAlloc(), env, callerScript,
                                                    options, srcBuf,
                                                     nullptr,
                                                     frame ? 1 : 0));
    if (!script)
        return false;

    script->setActiveEval();
    ExecuteType type = !frame ? EXECUTE_DEBUG_GLOBAL : EXECUTE_DEBUG;
    return ExecuteKernel(cx, script, *env, thisv, type, frame, rval.address());
}

enum EvalBindings { EvalHasExtraBindings = true, EvalWithDefaultBindings = false };

static bool
DebuggerGenericEval(JSContext *cx, const char *fullMethodName, const Value &code,
                    EvalBindings evalWithBindings, HandleValue bindings, HandleValue options,
                    MutableHandleValue vp, Debugger *dbg, HandleObject scope,
                    ScriptFrameIter *iter)
{
    
    JS_ASSERT_IF(iter, !scope);
    JS_ASSERT_IF(!iter, scope && scope->is<GlobalObject>());

    
    if (!code.isString()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_EXPECTED_TYPE,
                             fullMethodName, "string", InformalValueTypeName(code));
        return false;
    }
    Rooted<JSFlatString *> flat(cx, code.toString()->ensureFlat(cx));
    if (!flat)
        return false;

    




    AutoIdVector keys(cx);
    AutoValueVector values(cx);
    if (evalWithBindings) {
        RootedObject bindingsobj(cx, NonNullObject(cx, bindings));
        if (!bindingsobj ||
            !GetPropertyNames(cx, bindingsobj, JSITER_OWNONLY, &keys) ||
            !values.growBy(keys.length()))
        {
            return false;
        }
        for (size_t i = 0; i < keys.length(); i++) {
            MutableHandleValue valp = values[i];
            if (!JSObject::getGeneric(cx, bindingsobj, bindingsobj, keys[i], valp) ||
                !dbg->unwrapDebuggeeValue(cx, valp))
            {
                return false;
            }
        }
    }

    
    JSAutoByteString url_bytes;
    char *url = nullptr;
    unsigned lineNumber = 1;

    if (options.isObject()) {
        RootedObject opts(cx, &options.toObject());
        RootedValue v(cx);

        if (!JS_GetProperty(cx, opts, "url", &v))
            return false;
        if (!v.isUndefined()) {
            RootedString url_str(cx, ToString<CanGC>(cx, v));
            if (!url_str)
                return false;
            url = url_bytes.encodeLatin1(cx, url_str);
            if (!url)
                return false;
        }

        if (!JS_GetProperty(cx, opts, "lineNumber", &v))
            return false;
        if (!v.isUndefined()) {
            uint32_t lineno;
            if (!ToUint32(cx, v, &lineno))
                return false;
            lineNumber = lineno;
        }
    }

    Maybe<AutoCompartment> ac;
    if (iter)
        ac.emplace(cx, iter->scopeChain());
    else
        ac.emplace(cx, scope);

    RootedValue thisv(cx);
    Rooted<Env *> env(cx);
    if (iter) {
        
        if (!iter->computeThis(cx))
            return false;
        thisv = iter->computedThisValue();
        env = GetDebugScopeForFrame(cx, iter->abstractFramePtr(), iter->pc());
        if (!env)
            return false;
    } else {
        




        JSObject *thisObj = JSObject::thisObject(cx, scope);
        if (!thisObj)
            return false;
        thisv = ObjectValue(*thisObj);
        env = scope;
    }

    
    if (evalWithBindings) {
        
        env = NewObjectWithGivenProto(cx, &JSObject::class_, nullptr, env);
        if (!env)
            return false;
        RootedId id(cx);
        for (size_t i = 0; i < keys.length(); i++) {
            id = keys[i];
            MutableHandleValue val = values[i];
            if (!cx->compartment()->wrap(cx, val) ||
                !DefineNativeProperty(cx, env, id, val, nullptr, nullptr, 0))
            {
                return false;
            }
        }
    }

    
    RootedValue rval(cx);
    JS::Anchor<JSString *> anchor(flat);
    AbstractFramePtr frame = iter ? iter->abstractFramePtr() : NullFramePtr();
    AutoStableStringChars stableChars(cx);
    if (!stableChars.initTwoByte(cx, flat))
        return false;

    mozilla::Range<const char16_t> chars = stableChars.twoByteRange();
    bool ok = EvaluateInEnv(cx, env, thisv, frame, chars, url ? url : "debugger eval code",
                            lineNumber, &rval);
    return dbg->receiveCompletionValue(ac, ok, rval, vp);
}

static bool
DebuggerFrame_eval(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_ITER(cx, argc, vp, "eval", args, thisobj, _, iter);
    if (!args.requireAtLeast(cx, "Debugger.Frame.prototype.eval", 1))
        return false;
    Debugger *dbg = Debugger::fromChildJSObject(thisobj);
    UpdateFrameIterPc(iter);
    return DebuggerGenericEval(cx, "Debugger.Frame.prototype.eval",
                               args[0], EvalWithDefaultBindings, JS::UndefinedHandleValue,
                               args.get(1), args.rval(), dbg, js::NullPtr(), &iter);
}

static bool
DebuggerFrame_evalWithBindings(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_FRAME_ITER(cx, argc, vp, "evalWithBindings", args, thisobj, _, iter);
    if (!args.requireAtLeast(cx, "Debugger.Frame.prototype.evalWithBindings", 2))
        return false;
    Debugger *dbg = Debugger::fromChildJSObject(thisobj);
    UpdateFrameIterPc(iter);
    return DebuggerGenericEval(cx, "Debugger.Frame.prototype.evalWithBindings",
                               args[0], EvalHasExtraBindings, args[1], args.get(2),
                               args.rval(), dbg, js::NullPtr(), &iter);
}

static bool
DebuggerFrame_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Frame");
    return false;
}

static const JSPropertySpec DebuggerFrame_properties[] = {
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
    JS_PSG("implementation", DebuggerFrame_getImplementation, 0),
    JS_PSGS("onStep", DebuggerFrame_getOnStep, DebuggerFrame_setOnStep, 0),
    JS_PSGS("onPop", DebuggerFrame_getOnPop, DebuggerFrame_setOnPop, 0),
    JS_PS_END
};

static const JSFunctionSpec DebuggerFrame_methods[] = {
    JS_FN("eval", DebuggerFrame_eval, 1, 0),
    JS_FN("evalWithBindings", DebuggerFrame_evalWithBindings, 1, 0),
    JS_FS_END
};




static void
DebuggerObject_trace(JSTracer *trc, JSObject *obj)
{
    



    if (JSObject *referent = (JSObject *) obj->getPrivate()) {
        MarkCrossCompartmentObjectUnbarriered(trc, obj, &referent, "Debugger.Object referent");
        obj->setPrivateUnbarriered(referent);
    }
}

const Class DebuggerObject_class = {
    "Object",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGOBJECT_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr,              
    nullptr,              
    nullptr,              
    DebuggerObject_trace
};

static JSObject *
DebuggerObject_checkThis(JSContext *cx, const CallArgs &args, const char *fnname)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return nullptr;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerObject_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Object", fnname, thisobj->getClass()->name);
        return nullptr;
    }

    




    if (!thisobj->getPrivate()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Object", fnname, "prototype object");
        return nullptr;
    }
    return thisobj;
}

#define THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, fnname, args, obj)            \
    CallArgs args = CallArgsFromVp(argc, vp);                                 \
    RootedObject obj(cx, DebuggerObject_checkThis(cx, args, fnname));         \
    if (!obj)                                                                 \
        return false;                                                         \
    obj = (JSObject *) obj->getPrivate();                                     \
    JS_ASSERT(obj)

#define THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, fnname, args, dbg, obj) \
    CallArgs args = CallArgsFromVp(argc, vp);                                 \
    RootedObject obj(cx, DebuggerObject_checkThis(cx, args, fnname));         \
    if (!obj)                                                                 \
        return false;                                                         \
    Debugger *dbg = Debugger::fromChildJSObject(obj);                         \
    obj = (JSObject *) obj->getPrivate();                                     \
    JS_ASSERT(obj)

static bool
DebuggerObject_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Object");
    return false;
}

static bool
DebuggerObject_getProto(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get proto", args, dbg, refobj);
    RootedObject proto(cx);
    {
        AutoCompartment ac(cx, refobj);
        if (!JSObject::getProto(cx, refobj, &proto))
            return false;
    }
    RootedValue protov(cx, ObjectOrNullValue(proto));
    if (!dbg->wrapDebuggeeValue(cx, &protov))
        return false;
    args.rval().set(protov);
    return true;
}

static bool
DebuggerObject_getClass(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get class", args, refobj);
    const char *className;
    {
        AutoCompartment ac(cx, refobj);
        className = JSObject::className(cx, refobj);
    }
    JSAtom *str = Atomize(cx, className, strlen(className));
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static bool
DebuggerObject_getCallable(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get callable", args, refobj);
    args.rval().setBoolean(refobj->isCallable());
    return true;
}

static bool
DebuggerObject_getName(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get name", args, dbg, obj);
    if (!obj->is<JSFunction>()) {
        args.rval().setUndefined();
        return true;
    }

    JSString *name = obj->as<JSFunction>().atom();
    if (!name) {
        args.rval().setUndefined();
        return true;
    }

    RootedValue namev(cx, StringValue(name));
    if (!dbg->wrapDebuggeeValue(cx, &namev))
        return false;
    args.rval().set(namev);
    return true;
}

static bool
DebuggerObject_getDisplayName(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get display name", args, dbg, obj);
    if (!obj->is<JSFunction>()) {
        args.rval().setUndefined();
        return true;
    }

    JSString *name = obj->as<JSFunction>().displayAtom();
    if (!name) {
        args.rval().setUndefined();
        return true;
    }

    RootedValue namev(cx, StringValue(name));
    if (!dbg->wrapDebuggeeValue(cx, &namev))
        return false;
    args.rval().set(namev);
    return true;
}

static bool
DebuggerObject_getParameterNames(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get parameterNames", args, dbg, obj);
    if (!obj->is<JSFunction>()) {
        args.rval().setUndefined();
        return true;
    }

    RootedFunction fun(cx, &obj->as<JSFunction>());

    
    if (!dbg->observesGlobal(&fun->global())) {
        args.rval().setUndefined();
        return true;
    }

    RootedObject result(cx, NewDenseFullyAllocatedArray(cx, fun->nargs()));
    if (!result)
        return false;
    result->ensureDenseInitializedLength(cx, 0, fun->nargs());

    if (fun->isInterpreted()) {
        RootedScript script(cx, GetOrCreateFunctionScript(cx, fun));
        if (!script)
            return false;

        JS_ASSERT(fun->nargs() == script->bindings.numArgs());

        if (fun->nargs() > 0) {
            BindingVector bindings(cx);
            if (!FillBindingVector(script, &bindings))
                return false;
            for (size_t i = 0; i < fun->nargs(); i++) {
                Value v;
                if (bindings[i].name()->length() == 0)
                    v = UndefinedValue();
                else
                    v = StringValue(bindings[i].name());
                result->setDenseElement(i, v);
            }
        }
    } else {
        for (size_t i = 0; i < fun->nargs(); i++)
            result->setDenseElement(i, UndefinedValue());
    }

    args.rval().setObject(*result);
    return true;
}

static bool
DebuggerObject_getScript(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get script", args, dbg, obj);

    if (!obj->is<JSFunction>()) {
        args.rval().setUndefined();
        return true;
    }

    RootedFunction fun(cx, &obj->as<JSFunction>());
    if (fun->isBuiltin()) {
        args.rval().setUndefined();
        return true;
    }

    RootedScript script(cx, GetOrCreateFunctionScript(cx, fun));
    if (!script)
        return false;

    
    if (!dbg->observesScript(script)) {
        args.rval().setNull();
        return true;
    }

    RootedObject scriptObject(cx, dbg->wrapScript(cx, script));
    if (!scriptObject)
        return false;

    args.rval().setObject(*scriptObject);
    return true;
}

static bool
DebuggerObject_getEnvironment(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get environment", args, dbg, obj);

    
    if (!obj->is<JSFunction>() || !obj->as<JSFunction>().isInterpreted()) {
        args.rval().setUndefined();
        return true;
    }

    
    if (!dbg->observesGlobal(&obj->global())) {
        args.rval().setNull();
        return true;
    }

    Rooted<Env*> env(cx);
    {
        AutoCompartment ac(cx, obj);
        RootedFunction fun(cx, &obj->as<JSFunction>());
        env = GetDebugScopeForFunction(cx, fun);
        if (!env)
            return false;
    }

    return dbg->wrapEnvironment(cx, env, args.rval());
}

static bool
DebuggerObject_getIsBoundFunction(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get isBoundFunction", args, refobj);

    args.rval().setBoolean(refobj->isBoundFunction());
    return true;
}

static bool
DebuggerObject_getBoundTargetFunction(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get boundFunctionTarget", args, dbg, refobj);

    if (!refobj->isBoundFunction()) {
        args.rval().setUndefined();
        return true;
    }

    args.rval().setObject(*refobj->as<JSFunction>().getBoundFunctionTarget());
    return dbg->wrapDebuggeeValue(cx, args.rval());
}

static bool
DebuggerObject_getBoundThis(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get boundThis", args, dbg, refobj);

    if (!refobj->isBoundFunction()) {
        args.rval().setUndefined();
        return true;
    }
    args.rval().set(refobj->as<JSFunction>().getBoundFunctionThis());
    return dbg->wrapDebuggeeValue(cx, args.rval());
}

static bool
DebuggerObject_getBoundArguments(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get boundArguments", args, dbg, refobj);

    if (!refobj->isBoundFunction()) {
        args.rval().setUndefined();
        return true;
    }

    Rooted<JSFunction*> fun(cx, &refobj->as<JSFunction>());
    size_t length = fun->getBoundFunctionArgumentCount();
    AutoValueVector boundArgs(cx);
    if (!boundArgs.resize(length))
        return false;
    for (size_t i = 0; i < length; i++) {
        boundArgs[i].set(fun->getBoundFunctionArgument(i));
        if (!dbg->wrapDebuggeeValue(cx, boundArgs[i]))
            return false;
    }

    JSObject *aobj = NewDenseCopiedArray(cx, boundArgs.length(), boundArgs.begin());
    if (!aobj)
        return false;
    args.rval().setObject(*aobj);
    return true;
}

static bool
DebuggerObject_getGlobal(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "get global", args, dbg, obj);

    RootedValue v(cx, ObjectValue(obj->global()));
    if (!dbg->wrapDebuggeeValue(cx, &v))
        return false;
    args.rval().set(v);
    return true;
}

static bool
DebuggerObject_getAllocationSite(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "get allocationSite", args, obj);

    RootedObject metadata(cx, obj->getMetadata());
    if (!cx->compartment()->wrap(cx, &metadata))
        return false;
    args.rval().setObjectOrNull(metadata);
    return true;
}

static bool
DebuggerObject_getOwnPropertyDescriptor(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "getOwnPropertyDescriptor", args, dbg, obj);

    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, args.get(0), &id))
        return false;

    
    Rooted<PropertyDescriptor> desc(cx);
    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, obj);

        ErrorCopier ec(ac);
        if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
            return false;
    }

    if (desc.object()) {
        
        if (!dbg->wrapDebuggeeValue(cx, desc.value()))
            return false;

        if (desc.hasGetterObject()) {
            RootedValue get(cx, ObjectOrNullValue(desc.getterObject()));
            if (!dbg->wrapDebuggeeValue(cx, &get))
                return false;
            desc.setGetterObject(get.toObjectOrNull());
        }
        if (desc.hasSetterObject()) {
            RootedValue set(cx, ObjectOrNullValue(desc.setterObject()));
            if (!dbg->wrapDebuggeeValue(cx, &set))
                return false;
            desc.setSetterObject(set.toObjectOrNull());
        }
    }

    return NewPropertyDescriptorObject(cx, desc, args.rval());
}

static bool
DebuggerObject_getOwnPropertyNames(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "getOwnPropertyNames", args, obj);

    AutoIdVector keys(cx);
    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, obj);
        ErrorCopier ec(ac);
        if (!GetPropertyNames(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN, &keys))
            return false;
    }

    AutoValueVector vals(cx);
    if (!vals.resize(keys.length()))
        return false;

    for (size_t i = 0, len = keys.length(); i < len; i++) {
         jsid id = keys[i];
         if (JSID_IS_INT(id)) {
             JSString *str = Int32ToString<CanGC>(cx, JSID_TO_INT(id));
             if (!str)
                 return false;
             vals[i].setString(str);
         } else if (JSID_IS_ATOM(id)) {
             vals[i].setString(JSID_TO_STRING(id));
         } else {
             MOZ_ASSERT_UNREACHABLE("GetPropertyNames must return only string and int jsids");
         }
    }

    JSObject *aobj = NewDenseCopiedArray(cx, vals.length(), vals.begin());
    if (!aobj)
        return false;
    args.rval().setObject(*aobj);
    return true;
}

static bool
DebuggerObject_defineProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "defineProperty", args, dbg, obj);
    if (!args.requireAtLeast(cx, "Debugger.Object.defineProperty", 2))
        return false;

    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, args[0], &id))
        return false;

    Rooted<PropDesc> desc(cx);
    if (!desc.initialize(cx, args[1], false))
        return false;

    if (!dbg->unwrapPropDescInto(cx, obj, desc, &desc))
        return false;
    if (!desc.checkGetter(cx) || !desc.checkSetter(cx))
        return false;

    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, obj);
        if (!cx->compartment()->wrap(cx, &desc))
            return false;

        ErrorCopier ec(ac);
        bool dummy;
        if (!DefineProperty(cx, obj, id, desc, true, &dummy))
            return false;
    }

    args.rval().setUndefined();
    return true;
}

static bool
DebuggerObject_defineProperties(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "defineProperties", args, dbg, obj);
    if (!args.requireAtLeast(cx, "Debugger.Object.defineProperties", 1))
        return false;

    RootedValue arg(cx, args[0]);
    RootedObject props(cx, ToObject(cx, arg));
    if (!props)
        return false;

    AutoIdVector ids(cx);
    AutoPropDescVector descs(cx);
    if (!ReadPropertyDescriptors(cx, props, false, &ids, &descs))
        return false;
    size_t n = ids.length();

    for (size_t i = 0; i < n; i++) {
        if (!dbg->unwrapPropDescInto(cx, obj, descs[i], descs[i]))
            return false;
        if (!descs[i].checkGetter(cx) || !descs[i].checkSetter(cx))
            return false;
    }

    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, obj);
        for (size_t i = 0; i < n; i++) {
            if (!cx->compartment()->wrap(cx, descs[i]))
                return false;
        }

        ErrorCopier ec(ac);
        for (size_t i = 0; i < n; i++) {
            bool dummy;
            if (!DefineProperty(cx, obj, ids[i], descs[i], true, &dummy))
                return false;
        }
    }

    args.rval().setUndefined();
    return true;
}





static bool
DebuggerObject_deleteProperty(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "deleteProperty", args, obj);
    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, args.get(0), &id))
        return false;

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, obj);
    ErrorCopier ec(ac);

    bool succeeded;
    if (!JSObject::deleteGeneric(cx, obj, id, &succeeded))
        return false;
    args.rval().setBoolean(succeeded);
    return true;
}

enum SealHelperOp { Seal, Freeze, PreventExtensions };

static bool
DebuggerObject_sealHelper(JSContext *cx, unsigned argc, Value *vp, SealHelperOp op, const char *name)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, name, args, obj);

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, obj);
    ErrorCopier ec(ac);
    bool ok;
    if (op == Seal) {
        ok = JSObject::seal(cx, obj);
    } else if (op == Freeze) {
        ok = JSObject::freeze(cx, obj);
    } else {
        JS_ASSERT(op == PreventExtensions);
        bool extensible;
        if (!JSObject::isExtensible(cx, obj, &extensible))
            return false;
        if (!extensible) {
            args.rval().setUndefined();
            return true;
        }
        ok = JSObject::preventExtensions(cx, obj);
    }
    if (!ok)
        return false;
    args.rval().setUndefined();
    return true;
}

static bool
DebuggerObject_seal(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_sealHelper(cx, argc, vp, Seal, "seal");
}

static bool
DebuggerObject_freeze(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_sealHelper(cx, argc, vp, Freeze, "freeze");
}

static bool
DebuggerObject_preventExtensions(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_sealHelper(cx, argc, vp, PreventExtensions, "preventExtensions");
}

static bool
DebuggerObject_isSealedHelper(JSContext *cx, unsigned argc, Value *vp, SealHelperOp op,
                              const char *name)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, name, args, obj);

    Maybe<AutoCompartment> ac;
    ac.emplace(cx, obj);
    ErrorCopier ec(ac);
    bool r;
    if (op == Seal) {
        if (!JSObject::isSealed(cx, obj, &r))
            return false;
    } else if (op == Freeze) {
        if (!JSObject::isFrozen(cx, obj, &r))
            return false;
    } else {
        if (!JSObject::isExtensible(cx, obj, &r))
            return false;
    }
    args.rval().setBoolean(r);
    return true;
}

static bool
DebuggerObject_isSealed(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_isSealedHelper(cx, argc, vp, Seal, "isSealed");
}

static bool
DebuggerObject_isFrozen(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_isSealedHelper(cx, argc, vp, Freeze, "isFrozen");
}

static bool
DebuggerObject_isExtensible(JSContext *cx, unsigned argc, Value *vp)
{
    return DebuggerObject_isSealedHelper(cx, argc, vp, PreventExtensions, "isExtensible");
}

enum ApplyOrCallMode { ApplyMode, CallMode };

static bool
ApplyOrCall(JSContext *cx, unsigned argc, Value *vp, ApplyOrCallMode mode)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "apply", args, dbg, obj);

    



    RootedValue calleev(cx, ObjectValue(*obj));
    if (!obj->isCallable()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Object", "apply", obj->getClass()->name);
        return false;
    }

    



    RootedValue thisv(cx, args.get(0));
    if (!dbg->unwrapDebuggeeValue(cx, &thisv))
        return false;
    unsigned callArgc = 0;
    Value *callArgv = nullptr;
    AutoValueVector argv(cx);
    if (mode == ApplyMode) {
        if (args.length() >= 2 && !args[1].isNullOrUndefined()) {
            if (!args[1].isObject()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_APPLY_ARGS,
                                     js_apply_str);
                return false;
            }
            RootedObject argsobj(cx, &args[1].toObject());
            if (!GetLengthProperty(cx, argsobj, &callArgc))
                return false;
            callArgc = unsigned(Min(callArgc, ARGS_LENGTH_MAX));
            if (!argv.growBy(callArgc) || !GetElements(cx, argsobj, callArgc, argv.begin()))
                return false;
            callArgv = argv.begin();
        }
    } else {
        callArgc = args.length() > 0 ? unsigned(Min(args.length() - 1, ARGS_LENGTH_MAX)) : 0;
        callArgv = args.array() + 1;
    }

    AutoArrayRooter callArgvRooter(cx, callArgc, callArgv);
    for (unsigned i = 0; i < callArgc; i++) {
        if (!dbg->unwrapDebuggeeValue(cx, callArgvRooter.handleAt(i)))
            return false;
    }

    



    Maybe<AutoCompartment> ac;
    ac.emplace(cx, obj);
    if (!cx->compartment()->wrap(cx, &calleev) || !cx->compartment()->wrap(cx, &thisv))
        return false;

    RootedValue arg(cx);
    for (unsigned i = 0; i < callArgc; i++) {
        if (!cx->compartment()->wrap(cx, callArgvRooter.handleAt(i)))
             return false;
    }

    



    RootedValue rval(cx);
    bool ok = Invoke(cx, thisv, calleev, callArgc, callArgv, &rval);
    return dbg->receiveCompletionValue(ac, ok, rval, args.rval());
}

static bool
DebuggerObject_apply(JSContext *cx, unsigned argc, Value *vp)
{
    return ApplyOrCall(cx, argc, vp, ApplyMode);
}

static bool
DebuggerObject_call(JSContext *cx, unsigned argc, Value *vp)
{
    return ApplyOrCall(cx, argc, vp, CallMode);
}

static bool
DebuggerObject_makeDebuggeeValue(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "makeDebuggeeValue", args, dbg, referent);
    if (!args.requireAtLeast(cx, "Debugger.Object.prototype.makeDebuggeeValue", 1))
        return false;

    RootedValue arg0(cx, args[0]);

    
    if (arg0.isObject()) {
        
        
        {
            AutoCompartment ac(cx, referent);
            if (!cx->compartment()->wrap(cx, &arg0))
                return false;
        }

        
        
        if (!dbg->wrapDebuggeeValue(cx, &arg0))
            return false;
    }

    args.rval().set(arg0);
    return true;
}

static bool
RequireGlobalObject(JSContext *cx, HandleValue dbgobj, HandleObject referent)
{
    RootedObject obj(cx, referent);

    if (!obj->is<GlobalObject>()) {
        const char *isWrapper = "";
        const char *isWindowProxy = "";

        
        if (obj->is<WrapperObject>()) {
            obj = js::UncheckedUnwrap(obj);
            isWrapper = "a wrapper around ";
        }

        
        if (IsOuterObject(obj)) {
            obj = JS_ObjectToInnerObject(cx, obj);
            isWindowProxy = "a WindowProxy referring to ";
        }

        if (obj->is<GlobalObject>()) {
            js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_DEBUG_WRAPPER_IN_WAY,
                                     JSDVG_SEARCH_STACK, dbgobj, js::NullPtr(),
                                     isWrapper, isWindowProxy);
        } else {
            js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_DEBUG_BAD_REFERENT,
                                     JSDVG_SEARCH_STACK, dbgobj, js::NullPtr(),
                                     "a global object", nullptr);
        }
        return false;
    }

    return true;
}

static bool
DebuggerObject_evalInGlobal(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "evalInGlobal", args, dbg, referent);
    if (!args.requireAtLeast(cx, "Debugger.Object.prototype.evalInGlobal", 1))
        return false;
    if (!RequireGlobalObject(cx, args.thisv(), referent))
        return false;

    return DebuggerGenericEval(cx, "Debugger.Object.prototype.evalInGlobal",
                               args[0], EvalWithDefaultBindings, JS::UndefinedHandleValue,
                               args.get(1), args.rval(), dbg, referent, nullptr);
}

static bool
DebuggerObject_evalInGlobalWithBindings(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "evalInGlobalWithBindings", args, dbg, referent);
    if (!args.requireAtLeast(cx, "Debugger.Object.prototype.evalInGlobalWithBindings", 2))
        return false;
    if (!RequireGlobalObject(cx, args.thisv(), referent))
        return false;

    return DebuggerGenericEval(cx, "Debugger.Object.prototype.evalInGlobalWithBindings",
                               args[0], EvalHasExtraBindings, args[1], args.get(2),
                               args.rval(), dbg, referent, nullptr);
}

static bool
DebuggerObject_unwrap(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_OWNER_REFERENT(cx, argc, vp, "unwrap", args, dbg, referent);
    JSObject *unwrapped = UnwrapOneChecked(referent);
    if (!unwrapped) {
        args.rval().setNull();
        return true;
    }

    args.rval().setObject(*unwrapped);
    if (!dbg->wrapDebuggeeValue(cx, args.rval()))
        return false;
    return true;
}

static bool
DebuggerObject_unsafeDereference(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGOBJECT_REFERENT(cx, argc, vp, "unsafeDereference", args, referent);
    args.rval().setObject(*referent);
    if (!cx->compartment()->wrap(cx, args.rval()))
        return false;

    
    JS_ASSERT(!IsInnerObject(&args.rval().toObject()));

    return true;
}

static const JSPropertySpec DebuggerObject_properties[] = {
    JS_PSG("proto", DebuggerObject_getProto, 0),
    JS_PSG("class", DebuggerObject_getClass, 0),
    JS_PSG("callable", DebuggerObject_getCallable, 0),
    JS_PSG("name", DebuggerObject_getName, 0),
    JS_PSG("displayName", DebuggerObject_getDisplayName, 0),
    JS_PSG("parameterNames", DebuggerObject_getParameterNames, 0),
    JS_PSG("script", DebuggerObject_getScript, 0),
    JS_PSG("environment", DebuggerObject_getEnvironment, 0),
    JS_PSG("isBoundFunction", DebuggerObject_getIsBoundFunction, 0),
    JS_PSG("boundTargetFunction", DebuggerObject_getBoundTargetFunction, 0),
    JS_PSG("boundThis", DebuggerObject_getBoundThis, 0),
    JS_PSG("boundArguments", DebuggerObject_getBoundArguments, 0),
    JS_PSG("global", DebuggerObject_getGlobal, 0),
    JS_PSG("allocationSite", DebuggerObject_getAllocationSite, 0),
    JS_PS_END
};

static const JSFunctionSpec DebuggerObject_methods[] = {
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
    JS_FN("makeDebuggeeValue", DebuggerObject_makeDebuggeeValue, 1, 0),
    JS_FN("evalInGlobal", DebuggerObject_evalInGlobal, 1, 0),
    JS_FN("evalInGlobalWithBindings", DebuggerObject_evalInGlobalWithBindings, 2, 0),
    JS_FN("unwrap", DebuggerObject_unwrap, 0, 0),
    JS_FN("unsafeDereference", DebuggerObject_unsafeDereference, 0, 0),
    JS_FS_END
};




static void
DebuggerEnv_trace(JSTracer *trc, JSObject *obj)
{
    



    if (Env *referent = (JSObject *) obj->getPrivate()) {
        MarkCrossCompartmentObjectUnbarriered(trc, obj, &referent, "Debugger.Environment referent");
        obj->setPrivateUnbarriered(referent);
    }
}

const Class DebuggerEnv_class = {
    "Environment",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGENV_COUNT),
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr,
    nullptr,              
    nullptr,              
    nullptr,              
    DebuggerEnv_trace
};

static JSObject *
DebuggerEnv_checkThis(JSContext *cx, const CallArgs &args, const char *fnname,
                      bool requireDebuggee = true)
{
    if (!args.thisv().isObject()) {
        ReportObjectRequired(cx);
        return nullptr;
    }
    JSObject *thisobj = &args.thisv().toObject();
    if (thisobj->getClass() != &DebuggerEnv_class) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Environment", fnname, thisobj->getClass()->name);
        return nullptr;
    }

    




    if (!thisobj->getPrivate()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "Debugger.Environment", fnname, "prototype object");
        return nullptr;
    }

    



    if (requireDebuggee) {
        Rooted<Env*> env(cx, static_cast<Env *>(thisobj->getPrivate()));
        if (!Debugger::fromChildJSObject(thisobj)->observesGlobal(&env->global())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NOT_DEBUGGEE,
                                 "Debugger.Environment", "environment");
            return nullptr;
        }
    }

    return thisobj;
}

#define THIS_DEBUGENV(cx, argc, vp, fnname, args, envobj, env)                \
    CallArgs args = CallArgsFromVp(argc, vp);                                 \
    JSObject *envobj = DebuggerEnv_checkThis(cx, args, fnname);               \
    if (!envobj)                                                              \
        return false;                                                         \
    Rooted<Env*> env(cx, static_cast<Env *>(envobj->getPrivate()));           \
    JS_ASSERT(env);                                                           \
    JS_ASSERT(!env->is<ScopeObject>())

#define THIS_DEBUGENV_OWNER(cx, argc, vp, fnname, args, envobj, env, dbg)     \
    THIS_DEBUGENV(cx, argc, vp, fnname, args, envobj, env);                   \
    Debugger *dbg = Debugger::fromChildJSObject(envobj)

static bool
DebuggerEnv_construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Environment");
    return false;
}

static bool
IsDeclarative(Env *env)
{
    return env->is<DebugScopeObject>() && env->as<DebugScopeObject>().isForDeclarative();
}

static bool
IsWith(Env *env)
{
    return env->is<DebugScopeObject>() &&
           env->as<DebugScopeObject>().scope().is<DynamicWithObject>();
}

static bool
DebuggerEnv_getType(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV(cx, argc, vp, "get type", args, envobj, env);

    
    const char *s;
    if (IsDeclarative(env))
        s = "declarative";
    else if (IsWith(env))
        s = "with";
    else
        s = "object";

    JSAtom *str = Atomize(cx, s, strlen(s), InternAtom);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static bool
DebuggerEnv_getParent(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get parent", args, envobj, env, dbg);

    
    Rooted<Env*> parent(cx, env->enclosingScope());
    return dbg->wrapEnvironment(cx, parent, args.rval());
}

static bool
DebuggerEnv_getObject(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get type", args, envobj, env, dbg);

    



    if (IsDeclarative(env)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_NO_SCOPE_OBJECT);
        return false;
    }

    JSObject *obj;
    if (IsWith(env)) {
        obj = &env->as<DebugScopeObject>().scope().as<DynamicWithObject>().object();
    } else {
        obj = env;
        JS_ASSERT(!obj->is<DebugScopeObject>());
    }

    args.rval().setObject(*obj);
    if (!dbg->wrapDebuggeeValue(cx, args.rval()))
        return false;
    return true;
}

static bool
DebuggerEnv_getCallee(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "get callee", args, envobj, env, dbg);

    args.rval().setNull();

    if (!env->is<DebugScopeObject>())
        return true;

    JSObject &scope = env->as<DebugScopeObject>().scope();
    if (!scope.is<CallObject>())
        return true;

    CallObject &callobj = scope.as<CallObject>();
    if (callobj.isForEval())
        return true;

    args.rval().setObject(callobj.callee());
    if (!dbg->wrapDebuggeeValue(cx, args.rval()))
        return false;
    return true;
}

static bool
DebuggerEnv_getInspectable(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject *envobj = DebuggerEnv_checkThis(cx, args, "get inspectable", false);
    if (!envobj)
        return false;
    Rooted<Env*> env(cx, static_cast<Env *>(envobj->getPrivate()));
    JS_ASSERT(env);
    JS_ASSERT(!env->is<ScopeObject>());

    Debugger *dbg = Debugger::fromChildJSObject(envobj);

    args.rval().setBoolean(dbg->observesGlobal(&env->global()));
    return true;
}

static bool
DebuggerEnv_names(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV(cx, argc, vp, "names", args, envobj, env);

    AutoIdVector keys(cx);
    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, env);
        ErrorCopier ec(ac);
        if (!GetPropertyNames(cx, env, JSITER_HIDDEN, &keys))
            return false;
    }

    RootedObject arr(cx, NewDenseEmptyArray(cx));
    if (!arr)
        return false;
    RootedId id(cx);
    for (size_t i = 0, len = keys.length(); i < len; i++) {
        id = keys[i];
        if (JSID_IS_ATOM(id) && IsIdentifier(JSID_TO_ATOM(id))) {
            if (!NewbornArrayPush(cx, arr, StringValue(JSID_TO_STRING(id))))
                return false;
        }
    }
    args.rval().setObject(*arr);
    return true;
}

static bool
DebuggerEnv_find(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "find", args, envobj, env, dbg);
    if (!args.requireAtLeast(cx, "Debugger.Environment.find", 1))
        return false;

    RootedId id(cx);
    if (!ValueToIdentifier(cx, args[0], &id))
        return false;

    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, env);

        
        ErrorCopier ec(ac);
        RootedShape prop(cx);
        RootedObject pobj(cx);
        for (; env && !prop; env = env->enclosingScope()) {
            if (!JSObject::lookupGeneric(cx, env, id, &pobj, &prop))
                return false;
            if (prop)
                break;
        }
    }

    return dbg->wrapEnvironment(cx, env, args.rval());
}

static bool
DebuggerEnv_getVariable(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "getVariable", args, envobj, env, dbg);
    if (!args.requireAtLeast(cx, "Debugger.Environment.getVariable", 1))
        return false;

    RootedId id(cx);
    if (!ValueToIdentifier(cx, args[0], &id))
        return false;

    RootedValue v(cx);
    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, env);

        
        ErrorCopier ec(ac);

        
        
        
        
        if (env->is<DebugScopeObject>()) {
            if (!env->as<DebugScopeObject>().getMaybeSentinelValue(cx, id, &v))
                return false;
        } else {
            if (!JSObject::getGeneric(cx, env, env, id, &v))
                return false;
        }
    }

    if (!dbg->wrapDebuggeeValue(cx, &v))
        return false;
    args.rval().set(v);
    return true;
}

static bool
DebuggerEnv_setVariable(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGENV_OWNER(cx, argc, vp, "setVariable", args, envobj, env, dbg);
    if (!args.requireAtLeast(cx, "Debugger.Environment.setVariable", 2))
        return false;

    RootedId id(cx);
    if (!ValueToIdentifier(cx, args[0], &id))
        return false;

    RootedValue v(cx, args[1]);
    if (!dbg->unwrapDebuggeeValue(cx, &v))
        return false;

    {
        Maybe<AutoCompartment> ac;
        ac.emplace(cx, env);
        if (!cx->compartment()->wrap(cx, &v))
            return false;

        
        ErrorCopier ec(ac);

        
        bool has;
        if (!JSObject::hasProperty(cx, env, id, &has))
            return false;
        if (!has) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEBUG_VARIABLE_NOT_FOUND);
            return false;
        }

        
        if (!JSObject::setGeneric(cx, env, env, id, &v, true))
            return false;
    }

    args.rval().setUndefined();
    return true;
}

static const JSPropertySpec DebuggerEnv_properties[] = {
    JS_PSG("type", DebuggerEnv_getType, 0),
    JS_PSG("object", DebuggerEnv_getObject, 0),
    JS_PSG("parent", DebuggerEnv_getParent, 0),
    JS_PSG("callee", DebuggerEnv_getCallee, 0),
    JS_PSG("inspectable", DebuggerEnv_getInspectable, 0),
    JS_PS_END
};

static const JSFunctionSpec DebuggerEnv_methods[] = {
    JS_FN("names", DebuggerEnv_names, 0, 0),
    JS_FN("find", DebuggerEnv_find, 1, 0),
    JS_FN("getVariable", DebuggerEnv_getVariable, 1, 0),
    JS_FN("setVariable", DebuggerEnv_setVariable, 2, 0),
    JS_FS_END
};





Builder::Builder(JSContext *cx, js::Debugger *debugger)
  : debuggerObject(cx, debugger->toJSObject().get()),
    debugger(debugger)
{ }


#if DEBUG
void
Builder::assertBuilt(JSObject *obj)
{
    
    
    
    
    MOZ_ASSERT_IF(obj, debuggerObject->compartment() == obj->compartment());
}
#endif

bool
Builder::Object::definePropertyToTrusted(JSContext *cx, const char *name,
                                         JS::MutableHandleValue trusted)
{
    
    MOZ_ASSERT(value);

    JSAtom *atom = Atomize(cx, name, strlen(name));
    if (!atom)
        return false;
    RootedId id(cx, AtomToId(atom));

    return JSObject::defineGeneric(cx, value, id, trusted);
}

bool
Builder::Object::defineProperty(JSContext *cx, const char *name, JS::HandleValue propval_)
{
    AutoCompartment ac(cx, debuggerObject());

    RootedValue propval(cx, propval_);
    if (!debugger()->wrapDebuggeeValue(cx, &propval))
        return false;

    return definePropertyToTrusted(cx, name, &propval);
}

bool
Builder::Object::defineProperty(JSContext *cx, const char *name, JS::HandleObject propval_)
{
    RootedValue propval(cx, ObjectOrNullValue(propval_));
    return defineProperty(cx, name, propval);
}

bool
Builder::Object::defineProperty(JSContext *cx, const char *name, Builder::Object &propval_)
{
    AutoCompartment ac(cx, debuggerObject());

    RootedValue propval(cx, ObjectOrNullValue(propval_.value));
    return definePropertyToTrusted(cx, name, &propval);
}

Builder::Object
Builder::newObject(JSContext *cx)
{
    AutoCompartment ac(cx, debuggerObject);

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_));

    
    return Object(cx, *this, obj);
}




extern JS_PUBLIC_API(bool)
JS_DefineDebuggerObject(JSContext *cx, HandleObject obj)
{
    RootedObject
        objProto(cx),
        debugCtor(cx),
        debugProto(cx),
        frameProto(cx),
        scriptProto(cx),
        sourceProto(cx),
        objectProto(cx),
        envProto(cx),
        memoryProto(cx);
    objProto = obj->as<GlobalObject>().getOrCreateObjectPrototype(cx);
    if (!objProto)
        return false;
    debugProto = js_InitClass(cx, obj,
                              objProto, &Debugger::jsclass, Debugger::construct,
                              1, Debugger::properties, Debugger::methods, nullptr, nullptr,
                              debugCtor.address());
    if (!debugProto)
        return false;

    frameProto = js_InitClass(cx, debugCtor, objProto, &DebuggerFrame_class,
                              DebuggerFrame_construct, 0,
                              DebuggerFrame_properties, DebuggerFrame_methods,
                              nullptr, nullptr);
    if (!frameProto)
        return false;

    scriptProto = js_InitClass(cx, debugCtor, objProto, &DebuggerScript_class,
                               DebuggerScript_construct, 0,
                               DebuggerScript_properties, DebuggerScript_methods,
                               nullptr, nullptr);
    if (!scriptProto)
        return false;

    sourceProto = js_InitClass(cx, debugCtor, sourceProto, &DebuggerSource_class,
                               DebuggerSource_construct, 0,
                               DebuggerSource_properties, DebuggerSource_methods,
                               nullptr, nullptr);
    if (!sourceProto)
        return false;

    objectProto = js_InitClass(cx, debugCtor, objProto, &DebuggerObject_class,
                               DebuggerObject_construct, 0,
                               DebuggerObject_properties, DebuggerObject_methods,
                               nullptr, nullptr);
    if (!objectProto)
        return false;
    envProto = js_InitClass(cx, debugCtor, objProto, &DebuggerEnv_class,
                            DebuggerEnv_construct, 0,
                            DebuggerEnv_properties, DebuggerEnv_methods,
                            nullptr, nullptr);
    if (!envProto)
        return false;
    memoryProto = js_InitClass(cx, debugCtor, objProto, &DebuggerMemory::class_,
                               DebuggerMemory::construct, 0, DebuggerMemory::properties,
                               DebuggerMemory::methods, nullptr, nullptr);
    if (!memoryProto)
        return false;

    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_FRAME_PROTO, ObjectValue(*frameProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_OBJECT_PROTO, ObjectValue(*objectProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_SCRIPT_PROTO, ObjectValue(*scriptProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_SOURCE_PROTO, ObjectValue(*sourceProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_ENV_PROTO, ObjectValue(*envProto));
    debugProto->setReservedSlot(Debugger::JSSLOT_DEBUG_MEMORY_PROTO, ObjectValue(*memoryProto));
    return true;
}
