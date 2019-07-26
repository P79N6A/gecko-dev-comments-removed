





#ifndef jit_BaselineFrame_inl_h
#define jit_BaselineFrame_inl_h

#ifdef JS_ION

#include "jit/BaselineFrame.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "vm/ScopeObject.h"

namespace js {
namespace jit {

inline void
BaselineFrame::pushOnScopeChain(ScopeObject &scope)
{
    JS_ASSERT(*scopeChain() == scope.enclosingScope() ||
              *scopeChain() == scope.as<CallObject>().enclosingScope().as<DeclEnvObject>().enclosingScope());
    scopeChain_ = &scope;
}

inline void
BaselineFrame::popOffScopeChain()
{
    scopeChain_ = &scopeChain_->as<ScopeObject>().enclosingScope();
}

inline bool
BaselineFrame::pushBlock(JSContext *cx, Handle<StaticBlockObject *> block)
{
    JS_ASSERT_IF(hasBlockChain(), blockChain() == *block->enclosingBlock());

    if (block->needsClone()) {
        ClonedBlockObject *clone = ClonedBlockObject::create(cx, block, this);
        if (!clone)
            return false;

        pushOnScopeChain(*clone);
    }

    setBlockChain(*block);
    return true;
}

inline void
BaselineFrame::popBlock(JSContext *cx)
{
    JS_ASSERT(hasBlockChain());

    if (cx->compartment()->debugMode())
        DebugScopes::onPopBlock(cx, this);

    if (blockChain_->needsClone()) {
        JS_ASSERT(scopeChain_->as<ClonedBlockObject>().staticBlock() == *blockChain_);
        popOffScopeChain();
    }

    setBlockChain(*blockChain_->enclosingBlock());
}

inline CallObject &
BaselineFrame::callObj() const
{
    JS_ASSERT(hasCallObj());
    JS_ASSERT(fun()->isHeavyweight());

    JSObject *obj = scopeChain();
    while (!obj->is<CallObject>())
        obj = obj->enclosingScope();
    return obj->as<CallObject>();
}

} 
} 

#endif 

#endif 
