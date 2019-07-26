





#ifndef jsfuninlines_h
#define jsfuninlines_h

#include "jsfun.h"

#include "jsscript.h"

#include "vm/GlobalObject.h"

#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

inline void
JSFunction::initAtom(JSAtom *atom)
{
    atom_.init(atom);
}

inline void
JSFunction::setGuessedAtom(JSAtom *atom)
{
    JS_ASSERT(atom_ == NULL);
    JS_ASSERT(atom != NULL);
    JS_ASSERT(!hasGuessedAtom());
    atom_ = atom;
    flags |= HAS_GUESSED_ATOM;
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
JSFunction::initializeExtended()
{
    JS_ASSERT(isExtended());

    JS_ASSERT(mozilla::ArrayLength(toExtended()->extendedSlots) == 2);
    toExtended()->extendedSlots[0].init(js::UndefinedValue());
    toExtended()->extendedSlots[1].init(js::UndefinedValue());
}

inline void
JSFunction::initExtendedSlot(size_t which, const js::Value &val)
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which].init(val);
}

inline void
JSFunction::setExtendedSlot(size_t which, const js::Value &val)
{
    JS_ASSERT(which < mozilla::ArrayLength(toExtended()->extendedSlots));
    toExtended()->extendedSlots[which] = val;
}

namespace js {

inline const char *
GetFunctionNameBytes(JSContext *cx, JSFunction *fun, JSAutoByteString *bytes)
{
    JSAtom *atom = fun->atom();
    if (atom)
        return bytes->encodeLatin1(cx, atom);
    return js_anonymous_str;
}

static inline JSObject *
SkipScopeParent(JSObject *parent)
{
    if (!parent)
        return NULL;
    while (parent->is<ScopeObject>())
        parent = &parent->as<ScopeObject>().enclosingScope();
    return parent;
}

inline bool
CanReuseFunctionForClone(JSContext *cx, HandleFunction fun)
{
    if (!fun->hasSingletonType())
        return false;
    if (fun->isInterpretedLazy()) {
        LazyScript *lazy = fun->lazyScript();
        if (lazy->hasBeenCloned())
            return false;
        lazy->setHasBeenCloned();
    } else {
        JSScript *script = fun->nonLazyScript();
        if (script->hasBeenCloned)
            return false;
        script->hasBeenCloned = true;
    }
    return true;
}

inline JSFunction *
CloneFunctionObjectIfNotSingleton(JSContext *cx, HandleFunction fun, HandleObject parent,
                                  NewObjectKind newKind = GenericObject)
{
    











    if (CanReuseFunctionForClone(cx, fun)) {
        RootedObject obj(cx, SkipScopeParent(parent));
        if (!JSObject::setParent(cx, fun, obj))
            return NULL;
        fun->setEnvironment(parent);
        return fun;
    }

    
    
    gc::AllocKind finalizeKind = JSFunction::FinalizeKind;
    gc::AllocKind extendedFinalizeKind = JSFunction::ExtendedFinalizeKind;
    gc::AllocKind kind = fun->isExtended()
                         ? extendedFinalizeKind
                         : finalizeKind;
    return CloneFunctionObject(cx, fun, parent, kind, newKind);
}

} 

inline JSScript *
JSFunction::existingScript()
{
    JS_ASSERT(isInterpreted());
    if (isInterpretedLazy()) {
        js::LazyScript *lazy = lazyScript();
        JSScript *script = lazy->maybeScript();
        JS_ASSERT(script);

        if (zone()->needsBarrier())
            js::LazyScript::writeBarrierPre(lazy);

        flags &= ~INTERPRETED_LAZY;
        flags |= INTERPRETED;
        initScript(script);
    }
    JS_ASSERT(hasScript());
    return u.i.s.script_;
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

inline JSObject *
JSFunction::getBoundFunctionTarget() const
{
    JS_ASSERT(isBoundFunction());

    
    return getParent();
}

#endif 
