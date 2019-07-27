





#ifndef jsfuninlines_h
#define jsfuninlines_h

#include "jsfun.h"

#include "vm/ScopeObject.h"

namespace js {

inline const char*
GetFunctionNameBytes(JSContext* cx, JSFunction* fun, JSAutoByteString* bytes)
{
    JSAtom* atom = fun->atom();
    if (atom)
        return bytes->encodeLatin1(cx, atom);
    return js_anonymous_str;
}

static inline JSObject*
SkipScopeParent(JSObject* parent)
{
    if (!parent)
        return nullptr;
    while (parent->is<ScopeObject>())
        parent = &parent->as<ScopeObject>().enclosingScope();
    return parent;
}

inline bool
CanReuseFunctionForClone(JSContext* cx, HandleFunction fun)
{
    if (!fun->isSingleton())
        return false;
    if (fun->isInterpretedLazy()) {
        LazyScript* lazy = fun->lazyScript();
        if (lazy->hasBeenCloned())
            return false;
        lazy->setHasBeenCloned();
    } else {
        JSScript* script = fun->nonLazyScript();
        if (script->hasBeenCloned())
            return false;
        script->setHasBeenCloned();
    }
    return true;
}

inline JSFunction*
CloneFunctionObjectIfNotSingleton(JSContext* cx, HandleFunction fun, HandleObject parent,
                                  HandleObject proto = nullptr,
                                  NewObjectKind newKind = GenericObject)
{
    











    if (CanReuseFunctionForClone(cx, fun)) {
        RootedObject obj(cx, SkipScopeParent(parent));
        ObjectOpResult succeeded;
        if (proto && !SetPrototype(cx, fun, proto, succeeded))
            return nullptr;
        MOZ_ASSERT(!proto || succeeded);
        fun->setEnvironment(parent);
        return fun;
    }

    
    
    gc::AllocKind finalizeKind = gc::AllocKind::FUNCTION;
    gc::AllocKind extendedFinalizeKind = gc::AllocKind::FUNCTION_EXTENDED;
    gc::AllocKind kind = fun->isExtended()
                         ? extendedFinalizeKind
                         : finalizeKind;

    if (CanReuseScriptForClone(cx->compartment(), fun, parent))
        return CloneFunctionReuseScript(cx, fun, parent, kind, newKind, proto);

    RootedScript script(cx, fun->getOrCreateScript(cx));
    if (!script)
        return nullptr;
    RootedObject staticScope(cx, script->enclosingStaticScope());
    return CloneFunctionAndScript(cx, fun, parent, staticScope, kind, proto);
}

} 

#endif 
