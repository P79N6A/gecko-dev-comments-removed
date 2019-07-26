





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
    return context->hasExtraWarningsOption() || strict;
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

} 

#endif 
