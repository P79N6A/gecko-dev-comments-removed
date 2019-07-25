







































#ifndef TreeContext_inl_h__
#define TreeContext_inl_h__

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

namespace js {

inline
TreeContext::TreeContext(Parser *prs)
  : context(prs->context), flags(0), bodyid(0), blockidGen(0), parenDepth(0), yieldCount(0),
    topStmt(NULL), topScopeStmt(NULL), blockChain(context), blockNode(NULL),
    decls(context), yieldNode(NULL), argumentsNode(NULL), parserTC(&prs->tc),
    fun_(context), scopeChain_(context),
    lexdeps(context), parent(prs->tc), staticLevel(0), funbox(NULL), functionList(NULL),
    innermostWith(NULL), bindings(context), bindingsRoot(context, &bindings),
    funcStmts(NULL)
{
    prs->tc = this;
}

inline unsigned
TreeContext::blockid()
{
    return topStmt ? topStmt->blockid : bodyid;
}

inline bool
TreeContext::atBodyLevel()
{
    return !topStmt || (topStmt->flags & SIF_BODY_BLOCK);
}

inline bool
TreeContext::needStrictChecks() {
    return context->hasStrictOption() || inStrictMode();
}

inline unsigned
TreeContext::argumentsLocalSlot() const {
    PropertyName *arguments = context->runtime->atomState.argumentsAtom;
    unsigned slot;
    DebugOnly<BindingKind> kind = bindings.lookup(context, arguments, &slot);
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
    return slot;
}




inline
TreeContext::~TreeContext()
{
    
    
    JS_ASSERT(*parserTC == this);
    *parserTC = this->parent;
    context->delete_(funcStmts);
}

} 

#endif 
