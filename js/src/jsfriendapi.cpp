






































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jswrapper.h"
#include "jsweakmap.h"

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
    JS_ASSERT(rt->state == JSRTS_UP);
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
        return obj->getFunctionPrivate();
    return NULL;
}

JS_FRIEND_API(JSObject *)
JS_GetGlobalForFrame(JSStackFrame *fp)
{
    return Valueify(fp)->scopeChain().getGlobal();
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

JS_FRIEND_API(uint32)
JS_ObjectCountDynamicSlots(JSObject *obj)
{
    if (obj->hasSlotsArray())
        return obj->numDynamicSlots(obj->numSlots());
    return 0;
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

#ifdef DEBUG
JS_FRIEND_API(void)
js::CheckReservedSlot(const JSObject *obj, size_t slot)
{
    CheckSlot(obj, slot);
    JS_ASSERT(slot < JSSLOT_FREE(obj->getClass()));
}

JS_FRIEND_API(void)
js::CheckSlot(const JSObject *obj, size_t slot)
{
    JS_ASSERT(slot < obj->numSlots());
}
#endif






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
    JS_TRACER_INIT(&dtrc, cx, DumpHeapPushIfNew);
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
