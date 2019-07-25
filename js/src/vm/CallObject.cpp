







































#include "jsobjinlines.h"
#include "CallObject.h"

#include "CallObject-inl.h"

namespace js {







CallObject *
CallObject::create(JSContext *cx, JSScript *script, JSObject &scopeChain, JSObject *callee)
{
    Bindings &bindings = script->bindings;
    gc::AllocKind kind = gc::GetGCObjectKind(bindings.lastShape()->numFixedSlots() + 1);

    RootedVarTypeObject type(cx);

    type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    HeapValue *slots;
    if (!PreallocateObjectDynamicSlots(cx, bindings.lastShape(), &slots))
        return NULL;

    RootedVarShape shape(cx);
    shape = bindings.lastShape();

    JSObject *obj = JSObject::create(cx, kind, shape, type, slots);
    if (!obj)
        return NULL;

    




    JSObject *global = scopeChain.getGlobal();
    if (global != obj->getParent()) {
        JS_ASSERT(obj->getParent() == NULL);
        if (!obj->setParent(cx, global))
            return NULL;
    }

#ifdef DEBUG
    for (Shape::Range r = obj->lastProperty(); !r.empty(); r.popFront()) {
        const Shape &s = r.front();
        if (s.hasSlot()) {
            JS_ASSERT(s.slot() + 1 == obj->slotSpan());
            break;
        }
    }
#endif

    JS_ASSERT(obj->isCall());
    JS_ASSERT(!obj->inDictionaryMode());

    if (!obj->setInternalScopeChain(cx, &scopeChain))
        return NULL;

    



    if (obj->lastProperty()->extensibleParents() && !obj->generateOwnShape(cx))
        return NULL;

    CallObject &callobj = obj->asCall();
    callobj.initCallee(callee);
    return &callobj;
}

}
