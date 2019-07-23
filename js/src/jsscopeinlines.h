






































#ifndef jsscopeinlines_h___
#define jsscopeinlines_h___

#include "jscntxt.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsscope.h"

inline void
JSScope::extend(JSContext *cx, JSScopeProperty *sprop)
{
    js_LeaveTraceIfGlobalObject(cx, object);
    shape = (!lastProp || shape == lastProp->shape)
            ? sprop->shape
            : js_GenerateShape(cx, false);
    ++entryCount;
    lastProp = sprop;

    jsuint index;
    if (js_IdIsIndex(sprop->id, &index))
        setIndexedProperties();

    if (sprop->isMethod())
        setMethodBarrier();
}





inline bool
JSScope::methodReadBarrier(JSContext *cx, JSScopeProperty *sprop, jsval *vp)
{
    JS_ASSERT(hasMethodBarrier());
    JS_ASSERT(has(sprop));
    JS_ASSERT(sprop->isMethod());
    JS_ASSERT(sprop->methodValue() == *vp);
    JS_ASSERT(object->getClass() == &js_ObjectClass);

    JSObject *funobj = JSVAL_TO_OBJECT(*vp);
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);
    JS_ASSERT(FUN_OBJECT(fun) == funobj && FUN_NULL_CLOSURE(fun));

    funobj = js_CloneFunctionObject(cx, fun, OBJ_GET_PARENT(cx, funobj));
    if (!funobj)
        return false;
    *vp = OBJECT_TO_JSVAL(funobj);
    return js_SetPropertyHelper(cx, object, sprop->id, 0, vp);
}

inline bool
JSScope::methodWriteBarrier(JSContext *cx, JSScopeProperty *sprop, jsval v)
{
    if (branded()) {
        jsval prev = LOCKED_OBJ_GET_SLOT(object, sprop->slot);

        if (prev != v && VALUE_IS_FUNCTION(cx, prev))
            return methodShapeChange(cx, sprop, v);
    }
    return true;
}

inline bool
JSScope::methodWriteBarrier(JSContext *cx, uint32 slot, jsval v)
{
    if (branded()) {
        jsval prev = LOCKED_OBJ_GET_SLOT(object, slot);

        if (prev != v && VALUE_IS_FUNCTION(cx, prev))
            return methodShapeChange(cx, slot, v);
    }
    return true;
}

inline void
JSScope::trace(JSTracer *trc)
{
    JSContext *cx = trc->context;
    JSScopeProperty *sprop = lastProp;
    uint8 regenFlag = cx->runtime->gcRegenShapesScopeFlag;
    if (IS_GC_MARKING_TRACER(trc) && cx->runtime->gcRegenShapes && hasRegenFlag(regenFlag)) {
        



        uint32 newShape;

        if (sprop) {
            if (!(sprop->flags & SPROP_FLAG_SHAPE_REGEN)) {
                sprop->shape = js_RegenerateShapeForGC(cx);
                sprop->flags |= SPROP_FLAG_SHAPE_REGEN;
            }
            newShape = sprop->shape;
        }
        if (!sprop || hasOwnShape()) {
            newShape = js_RegenerateShapeForGC(cx);
            JS_ASSERT_IF(sprop, newShape != sprop->shape);
        }
        shape = newShape;
        flags ^= JSScope::SHAPE_REGEN;

        
        for (JSScope *empty = emptyScope;
             empty && empty->hasRegenFlag(regenFlag);
             empty = empty->emptyScope) {
            empty->shape = js_RegenerateShapeForGC(cx);
            empty->flags ^= JSScope::SHAPE_REGEN;
        }
    }
    if (sprop) {
        JS_ASSERT(has(sprop));

        
        do {
            if (hadMiddleDelete() && !has(sprop))
                continue;
            sprop->trace(trc);
        } while ((sprop = sprop->parent) != NULL);
    }
}

#endif 
