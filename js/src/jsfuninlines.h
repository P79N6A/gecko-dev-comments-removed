






































#ifndef jsfuninlines_h___
#define jsfuninlines_h___

#include "jsfun.h"
#include "jsscript.h"

inline bool
JSFunction::inStrictMode() const
{
    return script()->strictModeCode;
}

inline JSObject *
JSFunction::callScope() const
{
    JS_ASSERT(isInterpreted());
    return u.i.scope;
}

inline void
JSFunction::setCallScope(JSObject *obj)
{
    JS_ASSERT(isInterpreted());
    u.i.scope = obj;
}

inline void
JSFunction::setJoinable()
{
    JS_ASSERT(isInterpreted());
    setSlot(JSSLOT_FUN_METHOD_ATOM, js::NullValue());
    flags |= JSFUN_JOINABLE;
}

inline bool
JSFunction::isClonedMethod() const
{
    return getFixedSlot(JSSLOT_FUN_METHOD_OBJ).isObject();
}

inline void
JSFunction::setMethodAtom(JSAtom *atom)
{
    JS_ASSERT(joinable());
    setSlot(JSSLOT_FUN_METHOD_ATOM, js::StringValue(atom));
}

inline bool
JSFunction::hasMethodObj(const JSObject& obj) const
{
    return getFixedSlot(JSSLOT_FUN_METHOD_OBJ).isObject() &&
           getFixedSlot(JSSLOT_FUN_METHOD_OBJ).toObject() == obj;
}

inline void
JSFunction::setMethodObj(JSObject& obj)
{
    setFixedSlot(JSSLOT_FUN_METHOD_OBJ, js::ObjectValue(obj));
}

namespace js {

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v)
{
    return v.isObject() && v.toObject().isFunction();
}

static JS_ALWAYS_INLINE bool
IsFunctionObject(const js::Value &v, JSFunction **fun)
{
    if (v.isObject() && v.toObject().isFunction()) {
        *fun = v.toObject().toFunction();
        return true;
    }
    return false;
}

static JS_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v)
{
    JSFunction *fun;
    return IsFunctionObject(v, &fun) && fun->isNative();
}

static JS_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v, JSFunction **fun)
{
    return IsFunctionObject(v, fun) && (*fun)->isNative();
}

static JS_ALWAYS_INLINE bool
IsNativeFunction(const js::Value &v, JSNative native)
{
    JSFunction *fun;
    return IsFunctionObject(v, &fun) && fun->maybeNative() == native;
}









static JS_ALWAYS_INLINE bool
ClassMethodIsNative(JSContext *cx, JSObject *obj, Class *clasp, jsid methodid, JSNative native)
{
    JS_ASSERT(obj->getClass() == clasp);

    Value v;
    if (!HasDataProperty(cx, obj, methodid, &v)) {
        JSObject *proto = obj->getProto();
        if (!proto || proto->getClass() != clasp || !HasDataProperty(cx, proto, methodid, &v))
            return false;
    }

    return js::IsNativeFunction(v, native);
}

extern JS_ALWAYS_INLINE bool
SameTraceType(const Value &lhs, const Value &rhs)
{
    return SameType(lhs, rhs) &&
           (lhs.isPrimitive() ||
            lhs.toObject().isFunction() == rhs.toObject().isFunction());
}


static JS_ALWAYS_INLINE bool
IsConstructing(const Value *vp)
{
#ifdef DEBUG
    JSObject *callee = &JS_CALLEE(cx, vp).toObject();
    if (callee->isFunction()) {
        JSFunction *fun = callee->toFunction();
        JS_ASSERT((fun->flags & JSFUN_CONSTRUCTOR) != 0);
    } else {
        JS_ASSERT(callee->getClass()->construct != NULL);
    }
#endif
    return vp[1].isMagic();
}

inline bool
IsConstructing(CallReceiver call)
{
    return IsConstructing(call.base());
}

static JS_ALWAYS_INLINE bool
IsConstructing_PossiblyWithGivenThisObject(const Value *vp, JSObject **ctorThis)
{
#ifdef DEBUG
    JSObject *callee = &JS_CALLEE(cx, vp).toObject();
    if (callee->isFunction()) {
        JSFunction *fun = callee->toFunction();
        JS_ASSERT((fun->flags & JSFUN_CONSTRUCTOR) != 0);
    } else {
        JS_ASSERT(callee->getClass()->construct != NULL);
    }
#endif
    bool isCtor = vp[1].isMagic();
    if (isCtor)
        *ctorThis = vp[1].getMagicObjectOrNullPayload();
    return isCtor;
}

inline const char *
GetFunctionNameBytes(JSContext *cx, JSFunction *fun, JSAutoByteString *bytes)
{
    if (fun->atom)
        return bytes->encode(cx, fun->atom);
    return js_anonymous_str;
}

extern JSFunctionSpec function_methods[];

extern JSBool
Function(JSContext *cx, uintN argc, Value *vp);

extern bool
IsBuiltinFunctionConstructor(JSFunction *fun);













const Shape *
LookupInterpretedFunctionPrototype(JSContext *cx, JSObject *funobj);

static inline JSObject *
SkipScopeParent(JSObject *parent)
{
    if (!parent)
        return NULL;
    while (parent->isScope())
        parent = parent->getParentMaybeScope();
    return parent;
}

inline JSFunction *
CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                    bool ignoreSingletonClone = false)
{
    JS_ASSERT(parent);
    JSObject *proto;
    if (!js_GetClassPrototype(cx, parent, JSProto_Function, &proto))
        return NULL;

    






    if (ignoreSingletonClone && fun->hasSingletonType()) {
        JS_ASSERT(fun->getProto() == proto);
        if (!fun->setParent(cx, SkipScopeParent(parent)))
            return NULL;
        fun->setCallScope(parent);
        return fun;
    }

    return js_CloneFunctionObject(cx, fun, parent, proto);
}

inline JSFunction *
CloneFunctionObject(JSContext *cx, JSFunction *fun)
{
    






    JS_ASSERT(fun->getParent() && fun->getProto());

    if (fun->hasSingletonType())
        return fun;

    return js_CloneFunctionObject(cx, fun, fun->callScope(), fun->getProto());
}

} 

#endif 
