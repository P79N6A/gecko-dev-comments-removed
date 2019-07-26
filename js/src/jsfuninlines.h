






#ifndef jsfuninlines_h___
#define jsfuninlines_h___

#include "jsfun.h"
#include "jsscript.h"

#include "vm/GlobalObject.h"

#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

inline bool
JSFunction::strict() const
{
    return nonLazyScript()->strict;
}

inline void
JSFunction::initAtom(JSAtom *atom)
{
    atom_.init(atom);
}

inline void
JSFunction::setGuessedAtom(js::UnrootedAtom atom)
{
    JS_ASSERT(atom_ == NULL);
    JS_ASSERT(atom != NULL);
    JS_ASSERT(!hasGuessedAtom());
    atom_ = atom;
    flags |= HAS_GUESSED_ATOM;
}

inline JSObject *
JSFunction::environment() const
{
    JS_ASSERT(isInterpreted());
    return u.i.env_;
}

inline void
JSFunction::setEnvironment(JSObject *obj)
{
    JS_ASSERT(isInterpreted());
    *(js::HeapPtrObject *)&u.i.env_ = obj;
}

inline void
JSFunction::initEnvironment(JSObject *obj)
{
    JS_ASSERT(isInterpreted());
    ((js::HeapPtrObject *)&u.i.env_)->init(obj);
}

inline void
JSFunction::initNative(js::Native native, const JSJitInfo *data)
{
    JS_ASSERT(native);
    u.n.native = native;
    u.n.jitinfo = data;
}

inline const JSJitInfo *
JSFunction::jitInfo() const
{
    JS_ASSERT(isNative());
    return u.n.jitinfo;
}

inline void
JSFunction::setJitInfo(const JSJitInfo *data)
{
    JS_ASSERT(isNative());
    u.n.jitinfo = data;
}

inline void
JSFunction::initializeExtended()
{
    JS_ASSERT(isExtended());

    JS_ASSERT(mozilla::ArrayLength(toExtended()->extendedSlots) == 2);
    toExtended()->extendedSlots[0].init(js::UndefinedValue());
    toExtended()->extendedSlots[1].init(js::UndefinedValue());
}

inline void
JSFunction::setExtendedSlot(size_t which, const js::Value &val)
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which] = val;
}

inline const js::Value &
JSFunction::getExtendedSlot(size_t which) const
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    return toExtended()->extendedSlots[which];
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
ClassMethodIsNative(JSContext *cx, HandleObject obj, Class *clasp, HandleId methodid, JSNative native)
{
    JS_ASSERT(!obj->isProxy());
    JS_ASSERT(obj->getClass() == clasp);

    RootedValue v(cx);
    if (!HasDataProperty(cx, obj, methodid, v.address())) {
        RootedObject proto(cx, obj->getProto());
        if (!proto || proto->getClass() != clasp || !HasDataProperty(cx, proto, methodid, v.address()))
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
        JS_ASSERT(fun->isNativeConstructor());
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

inline const char *
GetFunctionNameBytes(JSContext *cx, JSFunction *fun, JSAutoByteString *bytes)
{
    JSAtom *atom = fun->atom();
    if (atom)
        return bytes->encode(cx, atom);
    return js_anonymous_str;
}

extern JSBool
Function(JSContext *cx, unsigned argc, Value *vp);

extern bool
IsBuiltinFunctionConstructor(JSFunction *fun);

static inline JSObject *
SkipScopeParent(JSObject *parent)
{
    if (!parent)
        return NULL;
    while (parent->isScope())
        parent = &parent->asScope().enclosingScope();
    return parent;
}

inline JSFunction *
CloneFunctionObject(JSContext *cx, HandleFunction fun, HandleObject parent,
                    gc::AllocKind kind = JSFunction::FinalizeKind)
{
    JS_ASSERT(parent);
    RootedObject proto(cx, parent->global().getOrCreateFunctionPrototype(cx));
    if (!proto)
        return NULL;

    return js_CloneFunctionObject(cx, fun, parent, proto, kind);
}

inline JSFunction *
CloneFunctionObjectIfNotSingleton(JSContext *cx, HandleFunction fun, HandleObject parent)
{
    






    if (fun->hasSingletonType()) {
        Rooted<JSObject*> obj(cx, SkipScopeParent(parent));
        if (!JSObject::setParent(cx, fun, obj))
            return NULL;
        fun->setEnvironment(parent);
        return fun;
    }

    return CloneFunctionObject(cx, fun, parent);
}

} 

inline void
JSFunction::setScript(JSScript *script_)
{
    JS_ASSERT(isInterpreted());
    mutableScript() = script_;
}

inline void
JSFunction::initScript(JSScript *script_)
{
    JS_ASSERT(isInterpreted());
    mutableScript().init(script_);
}

#endif 
