







































#ifndef BytecodeEmitter_inl_h__
#define BytecodeEmitter_inl_h__

#include "frontend/ParseNode.h"
#include "frontend/TokenStream.h"

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






inline
TreeContext::~TreeContext()
{
    parser->tc = this->parent;
    parser->context->delete_(funcStmts);
}

} 

#endif 
