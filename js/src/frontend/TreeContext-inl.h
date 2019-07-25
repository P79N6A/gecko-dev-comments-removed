






#ifndef TreeContext_inl_h__
#define TreeContext_inl_h__

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

#include "frontend/ParseMaps-inl.h"

namespace js {

inline
SharedContext::SharedContext(JSContext *cx, JSObject *scopeChain, JSFunction *fun,
                             FunctionBox *funbox)
  : context(cx),
    fun_(cx, fun),
    funbox_(funbox),
    scopeChain_(cx, scopeChain),
    bindings(),
    bindingsRoot(cx, &bindings),
    cxFlags(cx)
{
    JS_ASSERT((fun && !scopeChain_) || (!fun && !funbox));
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
SharedContext::needStrictChecks() {
    return context->hasStrictOption() || inStrictMode();
}

inline unsigned
SharedContext::argumentsLocal() const
{
    PropertyName *arguments = context->runtime->atomState.argumentsAtom;
    unsigned slot;
    DebugOnly<BindingKind> kind = bindings.lookup(context, arguments, &slot);
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
    return slot;
}

inline
TreeContext::TreeContext(Parser *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid)
  : sc(sc),
    bodyid(0),           
    blockidGen(bodyid),  
    topStmt(NULL),
    topScopeStmt(NULL),
    blockChain(prs->context),
    staticLevel(staticLevel),
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
    inForInit(false),
    inDeclDestructuring(false)
{
    prs->tc = this;
}

inline bool
TreeContext::init()
{
    if (!frontend::GenerateBlockId(this, this->bodyid))
        return false;

    return decls.init() && lexdeps.ensureMap(sc->context);
}




inline
TreeContext::~TreeContext()
{
    
    
    JS_ASSERT(*parserTC == this);
    *parserTC = this->parent;
    sc->context->delete_(funcStmts);
}

template <class ContextT>
void
frontend::PushStatement(ContextT *ct, typename ContextT::StmtInfo *stmt, StmtType type)
{
    stmt->type = type;
    stmt->flags = 0;
    stmt->label = NULL;
    stmt->blockObj = NULL;
    stmt->down = ct->topStmt;
    ct->topStmt = stmt;
    if (STMT_LINKS_SCOPE(stmt)) {
        stmt->downScope = ct->topScopeStmt;
        ct->topScopeStmt = stmt;
    } else {
        stmt->downScope = NULL;
    }
}

template <class ContextT>
void
frontend::FinishPushBlockScope(ContextT *ct, typename ContextT::StmtInfo *stmt,
                               StaticBlockObject &blockObj)
{
    stmt->flags |= SIF_SCOPE;
    blockObj.setEnclosingBlock(ct->blockChain);
    stmt->downScope = ct->topScopeStmt;
    ct->topScopeStmt = stmt;
    ct->blockChain = &blockObj;
    stmt->blockObj = &blockObj;
}

template <class ContextT>
void
frontend::FinishPopStatement(ContextT *ct)
{
    typename ContextT::StmtInfo *stmt = ct->topStmt;
    ct->topStmt = stmt->down;
    if (STMT_LINKS_SCOPE(stmt)) {
        ct->topScopeStmt = stmt->downScope;
        if (stmt->flags & SIF_SCOPE)
            ct->blockChain = stmt->blockObj->enclosingBlock();
    }
}

template <class ContextT>
typename ContextT::StmtInfo *
frontend::LexicalLookup(ContextT *ct, JSAtom *atom, int *slotp, typename ContextT::StmtInfo *stmt)
{
    if (!stmt)
        stmt = ct->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        if (stmt->type == STMT_WITH)
            break;

        
        if (!(stmt->flags & SIF_SCOPE))
            continue;

        StaticBlockObject &blockObj = *stmt->blockObj;
        const Shape *shape = blockObj.nativeLookup(ct->sc->context, AtomToId(atom));
        if (shape) {
            JS_ASSERT(shape->hasShortID());

            if (slotp)
                *slotp = blockObj.stackDepth() + shape->shortid();
            return stmt;
        }
    }

    if (slotp)
        *slotp = -1;
    return stmt;
}

} 

#endif 
