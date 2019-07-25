






































#ifndef jsscopeinlines_h___
#define jsscopeinlines_h___

#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsscope.h"

inline JSEmptyScope *
JSScope::createEmptyScope(JSContext *cx, JSClass *clasp)
{
    JS_ASSERT(!emptyScope);
    emptyScope = cx->create<JSEmptyScope>(cx, ops, clasp);
    return emptyScope;
}

inline JSEmptyScope *
JSScope::getEmptyScope(JSContext *cx, JSClass *clasp)
{
    if (emptyScope) {
        JS_ASSERT(clasp == emptyScope->clasp);
        return emptyScope->hold();
    }
    return createEmptyScope(cx, clasp);
}

inline bool
JSScope::ensureEmptyScope(JSContext *cx, JSClass *clasp)
{
    if (emptyScope) {
        JS_ASSERT(clasp == emptyScope->clasp);
        return true;
    }
    if (!createEmptyScope(cx, clasp))
        return false;

    
    JS_ASSERT(emptyScope->nrefs == 2);
    emptyScope->nrefs = 1;
    return true;
}

inline void
JSScope::updateShape(JSContext *cx)
{
    JS_ASSERT(object);
    js::LeaveTraceIfGlobalObject(cx, object);
    shape = (hasOwnShape() || !lastProp) ? js_GenerateShape(cx, false) : lastProp->shape;
}

inline void
JSScope::updateFlags(const JSScopeProperty *sprop, bool isDefinitelyAtom)
{
    jsuint index;
    if (!isDefinitelyAtom && js_IdIsIndex(sprop->id, &index))
        setIndexedProperties();

    if (sprop->isMethod())
        setMethodBarrier();
}

inline void
JSScope::extend(JSContext *cx, JSScopeProperty *sprop, bool isDefinitelyAtom)
{
    ++entryCount;
    setLastProperty(sprop);
    updateShape(cx);
    updateFlags(sprop, isDefinitelyAtom);
}





inline bool
JSScope::methodReadBarrier(JSContext *cx, JSScopeProperty *sprop, jsval *vp)
{
    JS_ASSERT(hasMethodBarrier());
    JS_ASSERT(hasProperty(sprop));
    JS_ASSERT(sprop->isMethod());
    JS_ASSERT(sprop->methodValue() == *vp);
    JS_ASSERT(object->getClass() == &js_ObjectClass);

    JSObject *funobj = JSVAL_TO_OBJECT(*vp);
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
    JS_ASSERT(FUN_OBJECT(fun) == funobj && FUN_NULL_CLOSURE(fun));

    funobj = CloneFunctionObject(cx, fun, funobj->getParent());
    if (!funobj)
        return false;
    *vp = OBJECT_TO_JSVAL(funobj);
    return !!js_SetPropertyHelper(cx, object, sprop->id, 0, vp);
}

inline bool
JSScope::methodWriteBarrier(JSContext *cx, JSScopeProperty *sprop, jsval v)
{
    if (flags & (BRANDED | METHOD_BARRIER)) {
        jsval prev = object->lockedGetSlot(sprop->slot);

        if (prev != v && VALUE_IS_FUNCTION(cx, prev))
            return methodShapeChange(cx, sprop);
    }
    return true;
}

inline bool
JSScope::methodWriteBarrier(JSContext *cx, uint32 slot, jsval v)
{
    if (flags & (BRANDED | METHOD_BARRIER)) {
        jsval prev = object->lockedGetSlot(slot);

        if (prev != v && VALUE_IS_FUNCTION(cx, prev))
            return methodShapeChange(cx, slot);
    }
    return true;
}

inline void
JSScope::trace(JSTracer *trc)
{
    JSContext *cx = trc->context;
    JSScopeProperty *sprop = lastProp;
    uint8 regenFlag = cx->runtime->gcRegenShapesScopeFlag;

    if (IS_GC_MARKING_TRACER(trc) && cx->runtime->gcRegenShapes && !hasRegenFlag(regenFlag)) {
        



        uint32 newShape;

        if (sprop) {
            if (!sprop->hasRegenFlag()) {
                sprop->shape = js_RegenerateShapeForGC(cx);
                sprop->setRegenFlag();
            }
            newShape = sprop->shape;
        }
        if (!sprop || hasOwnShape()) {
            newShape = js_RegenerateShapeForGC(cx);
            JS_ASSERT_IF(sprop, newShape != sprop->shape);
        }
        shape = newShape;
        flags ^= JSScope::SHAPE_REGEN;

        
        JSScope *empty = emptyScope;
        if (empty) {
            JS_ASSERT(!empty->emptyScope);
            if (!empty->hasRegenFlag(regenFlag)) {
                uint32 newEmptyShape = js_RegenerateShapeForGC(cx);

                JS_PROPERTY_TREE(cx).emptyShapeChange(empty->shape, newEmptyShape);
                empty->shape = newEmptyShape;
                empty->flags ^= JSScope::SHAPE_REGEN;
            }
        }
    }

    if (sprop) {
        JS_ASSERT(hasProperty(sprop));

        
        do {
            sprop->trace(trc);
        } while ((sprop = sprop->parent) != NULL);
    }
}

inline
JSScopeProperty::JSScopeProperty(jsid id, JSPropertyOp getter, JSPropertyOp setter,
                                 uint32 slot, uintN attrs, uintN flags, intN shortid)
  : id(id), rawGetter(getter), rawSetter(setter), slot(slot), attrs(uint8(attrs)),
    flags(uint8(flags)), shortid(int16(shortid))
{
    JS_ASSERT_IF(getter && (attrs & JSPROP_GETTER), getterObj->isCallable());
    JS_ASSERT_IF(setter && (attrs & JSPROP_SETTER), setterObj->isCallable());
}

inline JSDHashNumber
JSScopeProperty::hash() const
{
    JSDHashNumber hash = 0;

    
    JS_ASSERT_IF(isMethod(), !rawSetter || rawSetter == js_watch_set);
    if (rawGetter)
        hash = JS_ROTATE_LEFT32(hash, 4) ^ jsuword(rawGetter);
    if (rawSetter)
        hash = JS_ROTATE_LEFT32(hash, 4) ^ jsuword(rawSetter);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ (flags & PUBLIC_FLAGS);
    hash = JS_ROTATE_LEFT32(hash, 4) ^ attrs;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ shortid;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ slot;
    hash = JS_ROTATE_LEFT32(hash, 4) ^ id;
    return hash;
}

inline bool
JSScopeProperty::matches(const JSScopeProperty *p) const
{
    JS_ASSERT(!JSVAL_IS_NULL(id));
    JS_ASSERT(!JSVAL_IS_NULL(p->id));
    return id == p->id &&
           matchesParamsAfterId(p->rawGetter, p->rawSetter, p->slot, p->attrs, p->flags,
                                p->shortid);
}

inline bool
JSScopeProperty::matchesParamsAfterId(JSPropertyOp agetter, JSPropertyOp asetter, uint32 aslot,
                                      uintN aattrs, uintN aflags, intN ashortid) const
{
    JS_ASSERT(!JSVAL_IS_NULL(id));
    return rawGetter == agetter &&
           rawSetter == asetter &&
           slot == aslot &&
           attrs == aattrs &&
           ((flags ^ aflags) & PUBLIC_FLAGS) == 0 &&
           shortid == ashortid;
}

#endif 
