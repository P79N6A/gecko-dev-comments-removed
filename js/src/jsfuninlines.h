






































#ifndef jsfuninlines_h___
#define jsfuninlines_h___

#include "jsfun.h"
#include "jsscript.h"

#include "vm/GlobalObject.h"

inline bool
JSFunction::inStrictMode() const
{
    return script()->strictModeCode;
}

inline JSObject *
JSFunction::environment() const
{
    JS_ASSERT(isInterpreted());
    return u.i.env;
}

inline void
JSFunction::setEnvironment(JSObject *obj)
{
    JS_ASSERT(isInterpreted());
    u.i.env = obj;
}

inline void
JSFunction::setJoinable()
{
    JS_ASSERT(isInterpreted());
    flags |= JSFUN_JOINABLE;
}

inline bool
JSFunction::isClonedMethod() const
{
    return joinable() && isExtended() && toExtended()->extu.methodFunction.obj != NULL;
}

inline JSAtom *
JSFunction::methodAtom() const
{
    return (joinable() && isExtended()) ? toExtended()->extu.methodFunction.property : NULL;
}

inline void
JSFunction::setMethodAtom(JSAtom *atom)
{
    JS_ASSERT(joinable());
    toExtended()->extu.methodFunction.property = atom;
}

inline JSObject *
JSFunction::methodObj() const
{
    JS_ASSERT(joinable());
    return isExtended() ? toExtended()->extu.methodFunction.obj : NULL;
}

inline void
JSFunction::setMethodObj(JSObject& obj)
{
    JS_ASSERT(joinable());
    toExtended()->extu.methodFunction.obj = &obj;
}

inline void
JSFunction::setNativeReserved(size_t which, const js::Value &val)
{
    JS_ASSERT(isNative());
    JS_ASSERT(which < JS_ARRAY_LENGTH(toExtended()->extu.nativeReserved));
    toExtended()->extu.nativeReserved[which] = val;
}

inline const js::Value &
JSFunction::getNativeReserved(size_t which)
{
    JS_ASSERT(isNative());
    JS_ASSERT(which < JS_ARRAY_LENGTH(toExtended()->extu.nativeReserved));
    return toExtended()->extu.nativeReserved[which];
}

inline js::Value *
JSFunction::getFlatClosureUpvars() const
{
    JS_ASSERT(isFlatClosure());
    JS_ASSERT(script()->bindings.countUpvars() == script()->upvars()->length);
    return toExtended()->extu.flatClosureUpvars;
}

inline void
JSFunction::finalizeUpvars()
{
    











    JS_ASSERT(isFlatClosure());
    if (isExtended() && toExtended()->extu.flatClosureUpvars)
        js::Foreground::free_(toExtended()->extu.flatClosureUpvars);
}

inline js::Value
JSFunction::getFlatClosureUpvar(uint32 i) const
{
    JS_ASSERT(i < script()->bindings.countUpvars());
    return getFlatClosureUpvars()[i];
}

inline const js::Value &
JSFunction::getFlatClosureUpvar(uint32 i)
{
    JS_ASSERT(i < script()->bindings.countUpvars());
    return getFlatClosureUpvars()[i];
}

inline void
JSFunction::setFlatClosureUpvar(uint32 i, const js::Value &v)
{
    JS_ASSERT(i < script()->bindings.countUpvars());
    getFlatClosureUpvars()[i] = v;
}

inline void
JSFunction::setFlatClosureUpvars(js::Value *upvars)
{
    JS_ASSERT(isFlatClosure());
    toExtended()->extu.flatClosureUpvars = upvars;
}

 inline size_t
JSFunction::getFlatClosureUpvarsOffset()
{
    return offsetof(js::FunctionExtended, extu.flatClosureUpvars);
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
    while (parent->isInternalScope())
        parent = parent->scopeChain();
    return parent;
}

inline JSFunction *
CloneFunctionObject(JSContext *cx, JSFunction *fun, JSObject *parent,
                    gc::AllocKind kind = JSFunction::FinalizeKind)
{
    JS_ASSERT(parent);
    JSObject *proto = parent->getGlobal()->getOrCreateFunctionPrototype(cx);
    if (!proto)
        return NULL;

    return js_CloneFunctionObject(cx, fun, parent, proto, kind);
}

inline JSFunction *
CloneFunctionObjectIfNotSingleton(JSContext *cx, JSFunction *fun, JSObject *parent)
{
    






    if (fun->hasSingletonType()) {
        if (!fun->setParent(cx, SkipScopeParent(parent)))
            return NULL;
        fun->setEnvironment(parent);
        return fun;
    }

    return CloneFunctionObject(cx, fun, parent);
}

inline JSFunction *
CloneFunctionObject(JSContext *cx, JSFunction *fun)
{
    






    JS_ASSERT(fun->getParent() && fun->getProto());

    if (fun->hasSingletonType())
        return fun;

    return js_CloneFunctionObject(cx, fun, fun->environment(), fun->getProto(),
                                  JSFunction::ExtendedFinalizeKind);
}

} 

#endif 
