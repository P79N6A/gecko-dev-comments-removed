





#ifndef vm_ScopeObject_inl_h
#define vm_ScopeObject_inl_h

#include "vm/ScopeObject.h"

#include "jsinferinlines.h"

namespace js {

inline void
ScopeObject::setAliasedVar(JSContext *cx, ScopeCoordinate sc, PropertyName *name, const Value &v)
{
    MOZ_ASSERT(is<CallObject>() || is<ClonedBlockObject>());
    JS_STATIC_ASSERT(CallObject::RESERVED_SLOTS == BlockObject::RESERVED_SLOTS);

    
    JS_ASSERT_IF(hasSingletonType(), name);

    if (hasSingletonType()) {
        MOZ_ASSERT(name);
        types::AddTypePropertyId(cx, this, NameToId(name), v);

        
        if (!getSlot(sc.slot()).isUndefined()) {
            Shape *shape = nativeLookup(cx, name);
            shape->setOverwritten();
        }
    }

    setSlot(sc.slot(), v);
}

inline void
CallObject::setAliasedVar(JSContext *cx, AliasedFormalIter fi, PropertyName *name, const Value &v)
{
    MOZ_ASSERT(name == fi->name());
    setSlot(fi.scopeSlot(), v);
    if (hasSingletonType())
        types::AddTypePropertyId(cx, this, NameToId(name), v);
}

inline void
CallObject::setAliasedVarFromArguments(JSContext *cx, const Value &argsValue, jsid id, const Value &v)
{
    setSlot(argsValue.magicUint32(), v);
    if (hasSingletonType())
        types::AddTypePropertyId(cx, this, id, v);
}

inline void
CallObject::setAliasedLexicalsToThrowOnTouch(JSScript *script)
{
    uint32_t aliasedLexicalBegin = script->bindings.aliasedBodyLevelLexicalBegin();
    uint32_t aliasedLexicalEnd = numFixedSlots();
    for (uint32_t slot = aliasedLexicalBegin; slot < aliasedLexicalEnd; slot++)
        initFixedSlot(slot, MagicValue(JS_UNINITIALIZED_LEXICAL));
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
    if (obj->template is<NestedScopeObject>()) {
        obj = obj->template as<NestedScopeObject>().enclosingScopeForStaticScopeIter();
    } else if (onNamedLambda || !obj->template as<JSFunction>().isNamedLambda()) {
        onNamedLambda = false;
        obj = obj->template as<JSFunction>().nonLazyScript()->enclosingStaticScope();
    } else {
        onNamedLambda = true;
    }
    JS_ASSERT_IF(obj, obj->template is<NestedScopeObject>() || obj->template is<JSFunction>());
    JS_ASSERT_IF(onNamedLambda, obj->template is<JSFunction>());
}

template <AllowGC allowGC>
inline bool
StaticScopeIter<allowGC>::hasDynamicScopeObject() const
{
    return obj->template is<StaticBlockObject>()
           ? obj->template as<StaticBlockObject>().needsClone()
           : (obj->template is<StaticWithObject>() ||
              obj->template as<JSFunction>().isHeavyweight());
}

template <AllowGC allowGC>
inline Shape *
StaticScopeIter<allowGC>::scopeShape() const
{
    MOZ_ASSERT(hasDynamicScopeObject());
    MOZ_ASSERT(type() != NAMED_LAMBDA);
    if (type() == BLOCK)
        return block().lastProperty();
    return funScript()->callObjShape();
}

template <AllowGC allowGC>
inline typename StaticScopeIter<allowGC>::Type
StaticScopeIter<allowGC>::type() const
{
    if (onNamedLambda)
        return NAMED_LAMBDA;
    return obj->template is<StaticBlockObject>()
           ? BLOCK
           : (obj->template is<StaticWithObject>() ? WITH : FUNCTION);
}

template <AllowGC allowGC>
inline StaticBlockObject &
StaticScopeIter<allowGC>::block() const
{
    MOZ_ASSERT(type() == BLOCK);
    return obj->template as<StaticBlockObject>();
}

template <AllowGC allowGC>
inline StaticWithObject &
StaticScopeIter<allowGC>::staticWith() const
{
    MOZ_ASSERT(type() == WITH);
    return obj->template as<StaticWithObject>();
}

template <AllowGC allowGC>
inline JSScript *
StaticScopeIter<allowGC>::funScript() const
{
    MOZ_ASSERT(type() == FUNCTION);
    return obj->template as<JSFunction>().nonLazyScript();
}

template <AllowGC allowGC>
inline JSFunction &
StaticScopeIter<allowGC>::fun() const
{
    MOZ_ASSERT(type() == FUNCTION);
    return obj->template as<JSFunction>();
}

}  

#endif 
