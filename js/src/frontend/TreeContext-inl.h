






#ifndef TreeContext_inl_h__
#define TreeContext_inl_h__

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

#include "frontend/ParseMaps-inl.h"

namespace js {
namespace frontend {

inline
SharedContext::SharedContext(JSContext *cx, JSObject *scopeChain, JSFunction *fun,
                             FunctionBox *funbox, StrictMode::StrictModeState sms)
  : context(cx),
    fun_(cx, fun),
    funbox_(funbox),
    scopeChain_(cx, scopeChain),
    cxFlags(cx),
    strictModeState(sms)
{
    JS_ASSERT((fun && !scopeChain_) || (!fun && !funbox));
}

inline bool
SharedContext::inStrictMode()
{
    JS_ASSERT(strictModeState != StrictMode::UNKNOWN);
    JS_ASSERT_IF(inFunction() && funbox(), funbox()->strictModeState == strictModeState);
    return strictModeState == StrictMode::STRICT;
}

inline bool
SharedContext::needStrictChecks()
{
    return context->hasStrictOption() || strictModeState != StrictMode::NOTSTRICT;
}

inline unsigned
TreeContext::blockid()
{
    return topStmt ? topStmt->blockid : bodyid;
}

inline bool
TreeContext::atBodyLevel()
{
    return !topStmt || topStmt->isFunctionBodyBlock;
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
    decls_(prs->context),
    args_(prs->context),
    vars_(prs->context),
    yieldNode(NULL),
    functionList(NULL),
    queuedStrictModeError(NULL),
    parserTC(&prs->tc),
    lexdeps(prs->context),
    parent(prs->tc),
    innermostWith(NULL),
    funcStmts(NULL),
    hasReturnExpr(false),
    hasReturnVoid(false),
    inForInit(false),
    inDeclDestructuring(false),
    hasDuplicateArgument_(false)
{
    prs->tc = this;
}

inline bool
TreeContext::init()
{
    if (!frontend::GenerateBlockId(this, this->bodyid))
        return false;

    return decls_.init() && lexdeps.ensureMap(sc->context);
}

inline void
TreeContext::setQueuedStrictModeError(CompileError *e)
{
    JS_ASSERT(!queuedStrictModeError);
    queuedStrictModeError = e;
}

inline
TreeContext::~TreeContext()
{
    
    
    JS_ASSERT(*parserTC == this);
    *parserTC = this->parent;
    sc->context->delete_(funcStmts);
    if (queuedStrictModeError) {
        
        
        if (parent && parent->sc->strictModeState == StrictMode::UNKNOWN &&
            !parent->queuedStrictModeError)
            parent->queuedStrictModeError = queuedStrictModeError;
        else
            sc->context->delete_(queuedStrictModeError);
    }
}

} 

template <class ContextT>
void
frontend::PushStatement(ContextT *ct, typename ContextT::StmtInfo *stmt, StmtType type)
{
    stmt->type = type;
    stmt->isBlockScope = false;
    stmt->isForLetBlock = false;
    stmt->label = NULL;
    stmt->blockObj = NULL;
    stmt->down = ct->topStmt;
    ct->topStmt = stmt;
    if (stmt->linksScope()) {
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
    stmt->isBlockScope = true;
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
    if (stmt->linksScope()) {
        ct->topScopeStmt = stmt->downScope;
        if (stmt->isBlockScope)
            ct->blockChain = stmt->blockObj->enclosingBlock();
    }
}









template <class ContextT>
typename ContextT::StmtInfo *
frontend::LexicalLookup(ContextT *ct, HandleAtom atom, int *slotp, typename ContextT::StmtInfo *stmt)
{
    if (!stmt)
        stmt = ct->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        




        if (stmt->type == STMT_WITH)
            break;

        
        if (!stmt->isBlockScope)
            continue;

        StaticBlockObject &blockObj = *stmt->blockObj;
        Shape *shape = blockObj.nativeLookup(ct->sc->context, AtomToId(atom));
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
