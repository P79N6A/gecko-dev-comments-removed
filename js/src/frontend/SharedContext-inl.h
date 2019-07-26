






#ifndef SharedContext_inl_h__
#define SharedContext_inl_h__

#include "frontend/Parser.h"
#include "frontend/SharedContext.h"

namespace js {
namespace frontend {

inline
SharedContext::SharedContext(JSContext *cx, bool strict)
  : context(cx),
    anyCxFlags(),
    strict(strict)
{
}

inline bool
SharedContext::needStrictChecks()
{
    return context->hasStrictOption() || strict;
}

inline GlobalSharedContext *
SharedContext::asGlobalSharedContext()
{
    JS_ASSERT(isGlobalSharedContext());
    return static_cast<GlobalSharedContext*>(this);
}

inline ModuleBox *
SharedContext::asModuleBox()
{
    JS_ASSERT(isModuleBox());
    return static_cast<ModuleBox*>(this);
}

inline FunctionBox *
SharedContext::asFunctionBox()
{
    JS_ASSERT(isFunctionBox());
    return static_cast<FunctionBox*>(this);
}

GlobalSharedContext::GlobalSharedContext(JSContext *cx, JSObject *scopeChain, bool strict)
  : SharedContext(cx, strict),
    scopeChain_(cx, scopeChain)
{
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
    RootedId id(ct->sc->context, AtomToId(atom));

    if (!stmt)
        stmt = ct->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        




        if (stmt->type == STMT_WITH)
            break;

        
        if (!stmt->isBlockScope)
            continue;

        StaticBlockObject &blockObj = *stmt->blockObj;
        RawShape shape = blockObj.nativeLookup(ct->sc->context, id);
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
