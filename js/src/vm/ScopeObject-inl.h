





#ifndef vm_ScopeObject_inl_h
#define vm_ScopeObject_inl_h

#include "vm/ScopeObject.h"

#include "jsinferinlines.h"

namespace js {

inline void
ScopeObject::setAliasedVar(JSContext *cx, ScopeCoordinate sc, PropertyName *name, const Value &v)
{
    JS_ASSERT(is<CallObject>() || is<ClonedBlockObject>());
    JS_STATIC_ASSERT(CallObject::RESERVED_SLOTS == BlockObject::RESERVED_SLOTS);

    
    JS_ASSERT_IF(hasSingletonType(), name);

    setSlot(sc.slot, v);
    if (hasSingletonType())
        types::AddTypePropertyId(cx, this, NameToId(name), v);
}

inline void
CallObject::setAliasedVar(JSContext *cx, AliasedFormalIter fi, PropertyName *name, const Value &v)
{
    JS_ASSERT(name == fi->name());
    setSlot(fi.scopeSlot(), v);
    if (hasSingletonType())
        types::AddTypePropertyId(cx, this, NameToId(name), v);
}

template <AllowGC allowGC>
inline bool
StaticScopeIter<allowGC>::done() const
{
    return !obj;
}

template <AllowGC allowGC>
inline void
StaticScopeIter<allowGC>::operator++(int)
{
    if (obj->template is<StaticBlockObject>()) {
        obj = obj->template as<StaticBlockObject>().enclosingStaticScope();
    } else if (onNamedLambda || !obj->template as<JSFunction>().isNamedLambda()) {
        onNamedLambda = false;
        obj = obj->template as<JSFunction>().nonLazyScript()->enclosingStaticScope();
    } else {
        onNamedLambda = true;
    }
    JS_ASSERT_IF(obj, obj->template is<StaticBlockObject>() || obj->template is<JSFunction>());
    JS_ASSERT_IF(onNamedLambda, obj->template is<JSFunction>());
}

template <AllowGC allowGC>
inline bool
StaticScopeIter<allowGC>::hasDynamicScopeObject() const
{
    return obj->template is<StaticBlockObject>()
           ? obj->template as<StaticBlockObject>().needsClone()
           : obj->template as<JSFunction>().isHeavyweight();
}

template <AllowGC allowGC>
inline Shape *
StaticScopeIter<allowGC>::scopeShape() const
{
    JS_ASSERT(hasDynamicScopeObject());
    JS_ASSERT(type() != NAMED_LAMBDA);
    return type() == BLOCK
           ? block().lastProperty()
           : funScript()->bindings.callObjShape();
}

template <AllowGC allowGC>
inline typename StaticScopeIter<allowGC>::Type
StaticScopeIter<allowGC>::type() const
{
    if (onNamedLambda)
        return NAMED_LAMBDA;
    return obj->template is<StaticBlockObject>() ? BLOCK : FUNCTION;
}

template <AllowGC allowGC>
inline StaticBlockObject &
StaticScopeIter<allowGC>::block() const
{
    JS_ASSERT(type() == BLOCK);
    return obj->template as<StaticBlockObject>();
}

template <AllowGC allowGC>
inline JSScript *
StaticScopeIter<allowGC>::funScript() const
{
    JS_ASSERT(type() == FUNCTION);
    return obj->template as<JSFunction>().nonLazyScript();
}

}  

#endif 
