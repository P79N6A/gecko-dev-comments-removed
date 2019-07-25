







































#ifndef TreeContext_inl_h__
#define TreeContext_inl_h__

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

namespace js {

inline
TreeContext::TreeContext(Parser *prs)
  : flags(0), bodyid(0), blockidGen(0), parenDepth(0), yieldCount(0),
    topStmt(NULL), topScopeStmt(NULL), blockChain(prs->context), blockNode(NULL),
    decls(prs->context), parser(prs), yieldNode(NULL), argumentsNode(NULL),
    fun_(prs->context), scopeChain_(prs->context),
    lexdeps(prs->context), parent(prs->tc), staticLevel(0), funbox(NULL), functionList(NULL),
    innermostWith(NULL), bindings(prs->context), bindingsRoot(prs->context, &bindings),
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
    return parser->context->hasStrictOption() || inStrictMode();
}

inline unsigned
TreeContext::argumentsLocalSlot() const {
    PropertyName *arguments = parser->context->runtime->atomState.argumentsAtom;
    unsigned slot;
    DebugOnly<BindingKind> kind = bindings.lookup(parser->context, arguments, &slot);
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
    return slot;
}

inline ParseNode *
TreeContext::freeTree(ParseNode *pn)
{
    return parser->freeTree(pn);
}




inline
TreeContext::~TreeContext()
{
    parser->tc = this->parent;
    parser->context->delete_(funcStmts);
}

} 

#endif 
