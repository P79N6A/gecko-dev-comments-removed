






#ifndef TreeContext_inl_h__
#define TreeContext_inl_h__

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

#include "frontend/ParseMaps-inl.h"

namespace js {

inline
SharedContext::SharedContext(JSContext *cx, bool inFunction)
  : context(cx),
    bodyid(0),
    blockidGen(0),
    topStmt(NULL),
    topScopeStmt(NULL),
    blockChain(cx),
    fun_(cx),
    scopeChain_(cx),
    staticLevel(0),
    funbox(NULL),
    bindings(cx),
    bindingsRoot(cx, &bindings),
    inFunction(inFunction),
    inForInit(false),
    cxFlags(cx)
{
}

inline unsigned
SharedContext::blockid()
{
    return topStmt ? topStmt->blockid : bodyid;
}

inline bool
SharedContext::atBodyLevel()
{
    return !topStmt || (topStmt->flags & SIF_BODY_BLOCK);
}

inline bool
SharedContext::needStrictChecks() {
    return context->hasStrictOption() || inStrictMode();
}

inline unsigned
SharedContext::argumentsLocalSlot() const {
    PropertyName *arguments = context->runtime->atomState.argumentsAtom;
    unsigned slot;
    DebugOnly<BindingKind> kind = bindings.lookup(context, arguments, &slot);
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
    return slot;
}

inline
TreeContext::TreeContext(Parser *prs, SharedContext *sc)
  : sc(sc),
    parenDepth(0),
    yieldCount(0),
    blockNode(NULL),
    decls(prs->context),
    yieldNode(NULL),
    functionList(NULL),
    parserTC(&prs->tc),
    lexdeps(prs->context),
    parent(prs->tc),
    innermostWith(NULL),
    funcStmts(NULL),
    hasReturnExpr(false),
    hasReturnVoid(false),
    inDeclDestructuring(false)
{
    prs->tc = this;
}

inline bool
TreeContext::init()
{
    return decls.init() && lexdeps.ensureMap(sc->context);
}




inline
TreeContext::~TreeContext()
{
    
    
    JS_ASSERT(*parserTC == this);
    *parserTC = this->parent;
    sc->context->delete_(funcStmts);
}

} 

#endif 
