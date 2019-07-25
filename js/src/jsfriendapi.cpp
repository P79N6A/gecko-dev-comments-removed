






































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jswrapper.h"

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
        return obj->toFunction();
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
    return (obj->slotsAndStructSize() - obj->structSize()) / sizeof(Value);
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

bool
js::IsScopeObject(const JSObject *obj)
{
    return obj->isScope();
}

JS_FRIEND_API(JSObject *)
js::GetObjectParentMaybeScope(const JSObject *obj)
{
    return obj->getParentOrScopeChain();
}

JS_FRIEND_API(JSObject *)
js::GetObjectGlobal(JSObject *obj)
{
    return obj->getGlobal();
}

JS_FRIEND_API(uint32)
js::GetObjectSlotSpan(const JSObject *obj)
{
    return obj->slotSpan();
}

JS_FRIEND_API(bool)
js::IsOriginalScriptFunction(JSFunction *fun)
{
    return fun->script()->function() == fun;
}

JS_FRIEND_API(const Value &)
js::GetFunctionNativeReserved(JSObject *fun, size_t which)
{
    return fun->toFunction()->getNativeReserved(which);
}

JS_FRIEND_API(void)
js::SetFunctionNativeReserved(JSObject *fun, size_t which, const Value &val)
{
    fun->toFunction()->setNativeReserved(which, val);
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

JS_FRIEND_API(void)
JS_SetAccumulateTelemetryCallback(JSRuntime *rt, JSAccumulateTelemetryDataCallback callback)
{
    rt->telemetryCallback = callback;
}
