







































#include "jsobjinlines.h"
#include "CallObject.h"

#include "CallObject-inl.h"

namespace js {







CallObject *
CallObject::create(JSContext *cx, JSScript *script, JSObject &scopeChain, JSObject *callee)
{
    Bindings &bindings = script->bindings;
    gc::AllocKind kind = gc::GetGCObjectKind(bindings.lastShape()->numFixedSlots() + 1);

    JSObject *obj = js_NewGCObject(cx, kind);
    if (!obj)
        return NULL;

    
    if (!obj->initCall(cx, bindings, &scopeChain))
        return NULL;

#ifdef DEBUG
    for (Shape::Range r = obj->lastProperty(); !r.empty(); r.popFront()) {
        const Shape &s = r.front();
        if (s.hasSlot()) {
            JS_ASSERT(s.slot() + 1 == obj->slotSpan());
            break;
        }
    }
#endif

    CallObject &callobj = obj->asCall();
    callobj.setCallee(callee);
    return &callobj;
}

}
