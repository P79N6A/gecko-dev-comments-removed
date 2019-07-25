






































#include "mozilla/GuardObjects.h"
#include "mozilla/StdInt.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jswrapper.h"
#include "jsweakmap.h"
#include "jswatchpoint.h"

#include "jsobjinlines.h"

using namespace js;
using namespace JS;

JS_FRIEND_API(void)
JS_SetGrayGCRootsTracer(JSRuntime *rt, JSTraceDataOp traceOp, void *data)
{
    rt->gcGrayRootsTraceOp = traceOp;
    rt->gcGrayRootsData = data;
}

JS_FRIEND_API(JSString *)
JS_GetAnonymousString(JSRuntime *rt)
{
    JS_ASSERT(rt->hasContexts());
    return rt->atomState.anonymousAtom;
}

JS_FRIEND_API(JSObject *)
JS_FindCompilationScope(JSContext *cx, JSObject *obj)
{
    



    if (obj->isWrapper())
        obj = UnwrapObject(obj);
    
    



    if (JSObjectOp op = obj->getClass()->ext.innerObject)
        obj = op(cx, obj);
    return obj;
}

JS_FRIEND_API(JSFunction *)
JS_GetObjectFunction(JSObject *obj)
{
    if (obj->isFunction())
        return obj->toFunction();
    return NULL;
}

JS_FRIEND_API(JSObject *)
JS_GetGlobalForFrame(JSStackFrame *fp)
{
    return &Valueify(fp)->scopeChain().global();
}

JS_FRIEND_API(JSBool)
JS_SplicePrototype(JSContext *cx, JSObject *obj, JSObject *proto)
{
    




    CHECK_REQUEST(cx);

    if (!obj->hasSingletonType()) {
        



        return JS_SetPrototype(cx, obj, proto);
    }

    return obj->splicePrototype(cx, proto);
}

JS_FRIEND_API(JSObject *)
JS_NewObjectWithUniqueType(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent)
{
    JSObject *obj = JS_NewObject(cx, clasp, proto, parent);
    if (!obj || !obj->setSingletonType(cx))
        return NULL;
    return obj;
}

JS_FRIEND_API(void)
js::GCForReason(JSContext *cx, gcreason::Reason reason)
{
    js_GC(cx, NULL, GC_NORMAL, reason);
}

JS_FRIEND_API(void)
js::CompartmentGCForReason(JSContext *cx, JSCompartment *comp, gcreason::Reason reason)
{
    
    JS_ASSERT(comp != cx->runtime->atomsCompartment);

    js_GC(cx, comp, GC_NORMAL, reason);
}

JS_FRIEND_API(void)
js::ShrinkingGC(JSContext *cx, gcreason::Reason reason)
{
    js_GC(cx, NULL, GC_SHRINK, reason);
}

JS_FRIEND_API(void)
JS_ShrinkGCBuffers(JSRuntime *rt)
{
    ShrinkGCBuffers(rt);
}

JS_FRIEND_API(JSPrincipals *)
JS_GetCompartmentPrincipals(JSCompartment *compartment)
{
    return compartment->principals;
}

JS_FRIEND_API(JSBool)
JS_WrapPropertyDescriptor(JSContext *cx, js::PropertyDescriptor *desc)
{
    return cx->compartment->wrap(cx, desc);
}

JS_FRIEND_API(void)
JS_TraceShapeCycleCollectorChildren(JSTracer *trc, void *shape)
{
    MarkCycleCollectorChildren(trc, (const Shape *)shape);
}

AutoPreserveCompartment::AutoPreserveCompartment(JSContext *cx
                                                 JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : cx(cx), oldCompartment(cx->compartment)
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
}

AutoPreserveCompartment::~AutoPreserveCompartment()
{
    
    cx->compartment = oldCompartment;
}

AutoSwitchCompartment::AutoSwitchCompartment(JSContext *cx, JSCompartment *newCompartment
                                             JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : cx(cx), oldCompartment(cx->compartment)
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
    cx->setCompartment(newCompartment);
}

AutoSwitchCompartment::AutoSwitchCompartment(JSContext *cx, JSObject *target
                                             JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT)
  : cx(cx), oldCompartment(cx->compartment)
{
    JS_GUARD_OBJECT_NOTIFIER_INIT;
    cx->setCompartment(target->compartment());
}

AutoSwitchCompartment::~AutoSwitchCompartment()
{
    
    cx->compartment = oldCompartment;
}

JS_FRIEND_API(bool)
js::IsSystemCompartment(const JSCompartment *c)
{
    return c->isSystemCompartment;
}

JS_FRIEND_API(bool)
js::IsAtomsCompartmentFor(const JSContext *cx, const JSCompartment *c)
{
    return c == cx->runtime->atomsCompartment;
}

JS_FRIEND_API(bool)
js::IsScopeObject(JSObject *obj)
{
    return obj->isScope();
}

JS_FRIEND_API(JSObject *)
js::GetObjectParentMaybeScope(JSObject *obj)
{
    return obj->enclosingScope();
}

JS_FRIEND_API(JSObject *)
js::GetGlobalForObjectCrossCompartment(JSObject *obj)
{
    return &obj->global();
}

JS_FRIEND_API(uint32_t)
js::GetObjectSlotSpan(JSObject *obj)
{
    return obj->slotSpan();
}

JS_FRIEND_API(bool)
js::IsObjectInContextCompartment(const JSObject *obj, const JSContext *cx)
{
    return obj->compartment() == cx->compartment;
}

JS_FRIEND_API(bool)
js::IsOriginalScriptFunction(JSFunction *fun)
{
    return fun->script()->function() == fun;
}

JS_FRIEND_API(JSFunction *)
js::DefineFunctionWithReserved(JSContext *cx, JSObject *obj, const char *name, JSNative call,
                               uintN nargs, uintN attrs)
{
    RootObject objRoot(cx, &obj);

    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAtom *atom = js_Atomize(cx, name, strlen(name));
    if (!atom)
        return NULL;
    return js_DefineFunction(cx, objRoot, ATOM_TO_JSID(atom), call, nargs, attrs,
                             JSFunction::ExtendedFinalizeKind);
}

JS_FRIEND_API(JSFunction *)
js::NewFunctionWithReserved(JSContext *cx, JSNative native, uintN nargs, uintN flags,
                            JSObject *parent, const char *name)
{
    RootObject parentRoot(cx, &parent);

    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    JSAtom *atom;

    CHECK_REQUEST(cx);
    assertSameCompartment(cx, parent);

    if (!name) {
        atom = NULL;
    } else {
        atom = js_Atomize(cx, name, strlen(name));
        if (!atom)
            return NULL;
    }

    return js_NewFunction(cx, NULL, native, nargs, flags, parentRoot, atom,
                          JSFunction::ExtendedFinalizeKind);
}

JS_FRIEND_API(JSFunction *)
js::NewFunctionByIdWithReserved(JSContext *cx, JSNative native, uintN nargs, uintN flags, JSObject *parent,
                                jsid id)
{
    RootObject parentRoot(cx, &parent);

    JS_ASSERT(JSID_IS_STRING(id));
    JS_THREADSAFE_ASSERT(cx->compartment != cx->runtime->atomsCompartment);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, parent);

    return js_NewFunction(cx, NULL, native, nargs, flags, parentRoot, JSID_TO_ATOM(id),
                          JSFunction::ExtendedFinalizeKind);
}

JS_FRIEND_API(JSObject *)
js::InitClassWithReserved(JSContext *cx, JSObject *obj, JSObject *parent_proto,
                          JSClass *clasp, JSNative constructor, uintN nargs,
                          JSPropertySpec *ps, JSFunctionSpec *fs,
                          JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, parent_proto);
    RootObject objRoot(cx, &obj);
    return js_InitClass(cx, objRoot, parent_proto, Valueify(clasp), constructor,
                        nargs, ps, fs, static_ps, static_fs, NULL,
                        JSFunction::ExtendedFinalizeKind);
}

JS_FRIEND_API(const Value &)
js::GetFunctionNativeReserved(JSObject *fun, size_t which)
{
    JS_ASSERT(fun->toFunction()->isNative());
    return fun->toFunction()->getExtendedSlot(which);
}

JS_FRIEND_API(void)
js::SetFunctionNativeReserved(JSObject *fun, size_t which, const Value &val)
{
    JS_ASSERT(fun->toFunction()->isNative());
    fun->toFunction()->setExtendedSlot(which, val);
}

JS_FRIEND_API(void)
js::SetReservedSlotWithBarrier(JSObject *obj, size_t slot, const js::Value &value)
{
    obj->setSlot(slot, value);
}

void
js::SetPreserveWrapperCallback(JSRuntime *rt, PreserveWrapperCallback callback)
{
    rt->preserveWrapperCallback = callback;
}






extern size_t sE4XObjectsCreated;

JS_FRIEND_API(size_t)
JS_GetE4XObjectsCreated(JSContext *)
{
    return sE4XObjectsCreated;
}

extern size_t sSetProtoCalled;

JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext *)
{
    return sSetProtoCalled;
}

extern size_t sCustomIteratorCount;

JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext *cx)
{
    return sCustomIteratorCount;
}

void
js::TraceWeakMaps(WeakMapTracer *trc)
{
    WeakMapBase::traceAllMappings(trc);
    WatchpointMap::traceAll(trc);
}

JS_FRIEND_API(void)
JS_SetAccumulateTelemetryCallback(JSRuntime *rt, JSAccumulateTelemetryDataCallback callback)
{
    rt->telemetryCallback = callback;
}

JS_FRIEND_API(void)
JS_SetGCFinishedCallback(JSRuntime *rt, JSGCFinishedCallback callback)
{
    rt->gcFinishedCallback = callback;
}

#ifdef DEBUG
JS_FRIEND_API(void)
js_DumpString(JSString *str)
{
    str->dump();
}

JS_FRIEND_API(void)
js_DumpAtom(JSAtom *atom)
{
    atom->dump();
}

extern void
DumpChars(const jschar *s, size_t n)
{
    if (n == SIZE_MAX) {
        n = 0;
        while (s[n])
            n++;
    }

    fputc('"', stderr);
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\n')
            fprintf(stderr, "\\n");
        else if (s[i] == '\t')
            fprintf(stderr, "\\t");
        else if (s[i] >= 32 && s[i] < 127)
            fputc(s[i], stderr);
        else if (s[i] <= 255)
            fprintf(stderr, "\\x%02x", (unsigned int) s[i]);
        else
            fprintf(stderr, "\\u%04x", (unsigned int) s[i]);
    }
    fputc('"', stderr);
}

JS_FRIEND_API(void)
js_DumpChars(const jschar *s, size_t n)
{
    fprintf(stderr, "jschar * (%p) = ", (void *) s);
    DumpChars(s, n);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpObject(JSObject *obj)
{
    obj->dump();
}

struct DumpingChildInfo {
    void *node;
    JSGCTraceKind kind;

    DumpingChildInfo (void *n, JSGCTraceKind k)
        : node(n), kind(k)
    {}
};

typedef HashSet<void *, DefaultHasher<void *>, ContextAllocPolicy> PtrSet;

struct JSDumpHeapTracer : public JSTracer {
    PtrSet visited;
    FILE   *output;
    Vector<DumpingChildInfo, 0, ContextAllocPolicy> nodes;
    char   buffer[200];
    bool   rootTracing;

    JSDumpHeapTracer(JSContext *cx, FILE *fp)
        : visited(cx), output(fp), nodes(cx)
    {}
};

static void
DumpHeapVisitChild(JSTracer *trc, void *thing, JSGCTraceKind kind);

static void
DumpHeapPushIfNew(JSTracer *trc, void *thing, JSGCTraceKind kind)
{
    JS_ASSERT(trc->callback == DumpHeapPushIfNew ||
              trc->callback == DumpHeapVisitChild);
    JSDumpHeapTracer *dtrc = static_cast<JSDumpHeapTracer *>(trc);

    



    if (dtrc->rootTracing) {
        fprintf(dtrc->output, "%p %s\n", thing,
                JS_GetTraceEdgeName(dtrc, dtrc->buffer, sizeof(dtrc->buffer)));
    }

    PtrSet::AddPtr ptrEntry = dtrc->visited.lookupForAdd(thing);
    if (ptrEntry || !dtrc->visited.add(ptrEntry, thing))
        return;

    dtrc->nodes.append(DumpingChildInfo(thing, kind));
}

static void
DumpHeapVisitChild(JSTracer *trc, void *thing, JSGCTraceKind kind)
{
    JS_ASSERT(trc->callback == DumpHeapVisitChild);
    JSDumpHeapTracer *dtrc = static_cast<JSDumpHeapTracer *>(trc);
    const char *edgeName = JS_GetTraceEdgeName(dtrc, dtrc->buffer, sizeof(dtrc->buffer));
    fprintf(dtrc->output, "> %p %s\n", (void *)thing, edgeName);
    DumpHeapPushIfNew(dtrc, thing, kind);
}

void
js::DumpHeapComplete(JSContext *cx, FILE *fp)
{
    JSDumpHeapTracer dtrc(cx, fp);
    JS_TracerInit(&dtrc, cx, DumpHeapPushIfNew);
    if (!dtrc.visited.init(10000))
        return;

    
    dtrc.rootTracing = true;
    TraceRuntime(&dtrc);
    fprintf(dtrc.output, "==========\n");

    
    dtrc.rootTracing = false;
    dtrc.callback = DumpHeapVisitChild;

    while (!dtrc.nodes.empty()) {
        DumpingChildInfo dci = dtrc.nodes.popCopy();
        JS_PrintTraceThingInfo(dtrc.buffer, sizeof(dtrc.buffer),
                               &dtrc, dci.node, dci.kind, JS_TRUE);
        fprintf(fp, "%p %s\n", dci.node, dtrc.buffer);
        JS_TraceChildren(&dtrc, dci.node, dci.kind);
    }

    dtrc.visited.finish();
}

#endif

namespace js {

JS_FRIEND_API(bool)
IsIncrementalBarrierNeeded(JSRuntime *rt)
{
    return !!rt->gcIncrementalTracer && !rt->gcRunning;
}

JS_FRIEND_API(bool)
IsIncrementalBarrierNeeded(JSContext *cx)
{
    return IsIncrementalBarrierNeeded(cx->runtime);
}

extern JS_FRIEND_API(void)
IncrementalReferenceBarrier(void *ptr)
{
    if (!ptr)
        return;
    JS_ASSERT(!static_cast<gc::Cell *>(ptr)->compartment()->rt->gcRunning);
    uint32_t kind = gc::GetGCThingTraceKind(ptr);
    if (kind == JSTRACE_OBJECT)
        JSObject::writeBarrierPre((JSObject *) ptr);
    else if (kind == JSTRACE_STRING)
        JSString::writeBarrierPre((JSString *) ptr);
    else
        JS_NOT_REACHED("invalid trace kind");
}

extern JS_FRIEND_API(void)
IncrementalValueBarrier(const Value &v)
{
    HeapValue::writeBarrierPre(v);
}

 void
AutoLockGC::LockGC(JSRuntime *rt)
{
    JS_ASSERT(rt);
    JS_LOCK_GC(rt);
}

 void
AutoLockGC::UnlockGC(JSRuntime *rt)
{
    JS_ASSERT(rt);
    JS_UNLOCK_GC(rt);
}

void
AutoLockGC::lock(JSRuntime *rt)
{
    JS_ASSERT(rt);
    JS_ASSERT(!runtime);
    runtime = rt;
    JS_LOCK_GC(rt);
}

JS_FRIEND_API(const JSStructuredCloneCallbacks *)
GetContextStructuredCloneCallbacks(JSContext *cx)
{
    return cx->runtime->structuredCloneCallbacks;
}

JS_FRIEND_API(JSVersion)
VersionSetXML(JSVersion version, bool enable)
{
    return enable ? JSVersion(uint32_t(version) | VersionFlags::HAS_XML)
                  : JSVersion(uint32_t(version) & ~VersionFlags::HAS_XML);
}

JS_FRIEND_API(bool)
CanCallContextDebugHandler(JSContext *cx)
{
    return cx->debugHooks && cx->debugHooks->debuggerHandler;
}

JS_FRIEND_API(JSTrapStatus)
CallContextDebugHandler(JSContext *cx, JSScript *script, jsbytecode *bc, Value *rval)
{
    if (!CanCallContextDebugHandler(cx))
        return JSTRAP_RETURN;

    return cx->debugHooks->debuggerHandler(cx, script, bc, rval,
                                           cx->debugHooks->debuggerHandlerData);
}

#ifdef JS_THREADSAFE
void *
GetOwnerThread(const JSContext *cx)
{
    return cx->runtime->ownerThread();
}

JS_FRIEND_API(unsigned)
GetContextOutstandingRequests(const JSContext *cx)
{
    return cx->outstandingRequests;
}

JS_FRIEND_API(PRLock *)
GetRuntimeGCLock(const JSRuntime *rt)
{
    return rt->gcLock;
}

AutoSkipConservativeScan::AutoSkipConservativeScan(JSContext *cx
                                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : context(cx)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;

    JSRuntime *rt = context->runtime;
    JS_ASSERT(rt->requestDepth >= 1);
    JS_ASSERT(!rt->conservativeGC.requestThreshold);
    if (rt->requestDepth == 1)
        rt->conservativeGC.requestThreshold = 1;
}

AutoSkipConservativeScan::~AutoSkipConservativeScan()
{
    JSRuntime *rt = context->runtime;
    if (rt->requestDepth == 1)
        rt->conservativeGC.requestThreshold = 0;
}
#endif

JS_FRIEND_API(JSCompartment *)
GetContextCompartment(const JSContext *cx)
{
    return cx->compartment;
}

JS_FRIEND_API(bool)
HasUnrootedGlobal(const JSContext *cx)
{
    return cx->hasRunOption(JSOPTION_UNROOTED_GLOBAL);
}

JS_FRIEND_API(void)
SetActivityCallback(JSRuntime *rt, ActivityCallback cb, void *arg)
{
    rt->activityCallback = cb;
    rt->activityCallbackArg = arg;
}

JS_FRIEND_API(bool)
IsContextRunningJS(JSContext *cx)
{
    return !cx->stack.empty();
}

JS_FRIEND_API(void)
TriggerOperationCallback(JSRuntime *rt)
{
    rt->triggerOperationCallback();
}

JS_FRIEND_API(const CompartmentVector&)
GetRuntimeCompartments(JSRuntime *rt)
{
    return rt->compartments;
}

JS_FRIEND_API(size_t)
SizeOfJSContext()
{
    return sizeof(JSContext);
}

} 
