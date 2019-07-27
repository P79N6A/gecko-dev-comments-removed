





#ifndef jit_BaselineFrame_inl_h
#define jit_BaselineFrame_inl_h

#include "jit/BaselineFrame.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "vm/ScopeObject.h"

namespace js {
namespace jit {

inline void
BaselineFrame::pushOnScopeChain(ScopeObject &scope)
{
    MOZ_ASSERT(*scopeChain() == scope.enclosingScope() ||
               *scopeChain() == scope.as<CallObject>().enclosingScope().as<DeclEnvObject>().enclosingScope());
    scopeChain_ = &scope;
}

inline void
BaselineFrame::popOffScopeChain()
{
    scopeChain_ = &scopeChain_->as<ScopeObject>().enclosingScope();
}

inline void
BaselineFrame::popWith(JSContext *cx)
{
    if (MOZ_UNLIKELY(isDebuggee()))
        DebugScopes::onPopWith(this);

    MOZ_ASSERT(scopeChain()->is<DynamicWithObject>());
    popOffScopeChain();
}

inline bool
BaselineFrame::pushBlock(JSContext *cx, Handle<StaticBlockObject *> block)
{
    MOZ_ASSERT(block->needsClone());

    ClonedBlockObject *clone = ClonedBlockObject::create(cx, block, this);
    if (!clone)
        return false;
    pushOnScopeChain(*clone);

    return true;
}

inline void
BaselineFrame::popBlock(JSContext *cx)
{
    MOZ_ASSERT(scopeChain_->is<ClonedBlockObject>());

    popOffScopeChain();
}

inline CallObject &
BaselineFrame::callObj() const
{
    MOZ_ASSERT(hasCallObj());
    MOZ_ASSERT(fun()->isHeavyweight());

    JSObject *obj = scopeChain();
    while (!obj->is<CallObject>())
        obj = obj->enclosingScope();
    return obj->as<CallObject>();
}

} 
} 

#endif 
