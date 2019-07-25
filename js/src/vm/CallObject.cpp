







































#include "jsobjinlines.h"
#include "CallObject.h"

#include "CallObject-inl.h"

namespace js {







CallObject *
CallObject::create(JSContext *cx, JSScript *script, JSObject &scopeChain, JSObject *callee)
{
    Bindings &bindings = script->bindings;
    size_t argsVars = bindings.countArgsAndVars();
    size_t slots = RESERVED_SLOTS + argsVars;
    gc::AllocKind kind = gc::GetGCObjectKind(slots);

    





    if (cx->typeInferenceEnabled() && gc::GetGCKindSlots(kind) < slots) {
        kind = gc::GetGCObjectKind(RESERVED_SLOTS);
        JS_ASSERT(gc::GetGCKindSlots(kind) == RESERVED_SLOTS);
    }

    JSObject *obj = js_NewGCObject(cx, kind);
    if (!obj)
        return NULL;

    
    if (!obj->initCall(cx, bindings, &scopeChain))
        return NULL;
    obj->makeVarObj();

    
    if (!obj->ensureInstanceReservedSlots(cx, argsVars))
        return NULL;

#ifdef DEBUG
    for (Shape::Range r = obj->lastProp; !r.empty(); r.popFront()) {
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
