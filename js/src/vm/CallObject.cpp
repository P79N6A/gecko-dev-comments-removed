







































#include "jsobjinlines.h"
#include "CallObject.h"

#include "CallObject-inl.h"

namespace js {







CallObject *
CallObject::create(JSContext *cx, JSScript *script, JSObject &scopeChain, JSObject *callee)
{
    Bindings &bindings = script->bindings;
    gc::AllocKind kind = gc::GetGCObjectKind(bindings.lastShape()->numFixedSlots() + 1);

    js::types::TypeObject *type = cx->compartment->getEmptyType(cx);
    if (!type)
        return NULL;

    Value *slots;
    if (!ReserveObjectDynamicSlots(cx, bindings.lastShape(), &slots))
        return NULL;

    JSObject *obj = js_NewGCObject(cx, kind);
    if (!obj)
        return NULL;

    obj->initialize(bindings.lastShape(), type, slots);

    




    JSObject *global = scopeChain.getGlobal();
    if (global != obj->getParentMaybeScope()) {
        JS_ASSERT(obj->getParentMaybeScope() == NULL);
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

    obj->setScopeChain(&scopeChain);

    



    if (obj->lastProperty()->extensibleParents() && !obj->generateOwnShape(cx))
        return NULL;

    CallObject &callobj = obj->asCall();
    callobj.setCallee(callee);
    return &callobj;
}

}
