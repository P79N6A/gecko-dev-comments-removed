







































#include "frontend/ParseNode.h"
#include "frontend/TreeContext.h"

#include "jsatominlines.h"

#include "frontend/TreeContext-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::frontend;

void
TreeContext::trace(JSTracer *trc)
{
    bindings.trace(trc);
}

bool
frontend::SetStaticLevel(TreeContext *tc, unsigned staticLevel)
{
    



    if (UpvarCookie::isLevelReserved(staticLevel)) {
        JS_ReportErrorNumber(tc->context, js_GetErrorMessage, NULL,
                             JSMSG_TOO_DEEP, js_function_str);
        return false;
    }
    tc->staticLevel = staticLevel;
    return true;
}

bool
frontend::GenerateBlockId(TreeContext *tc, uint32_t &blockid)
{
    if (tc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(tc->context, js_GetErrorMessage, NULL,
                             JSMSG_NEED_DIET, "program");
        return false;
    }
    blockid = tc->blockidGen++;
    return true;
}

void
frontend::PushStatement(TreeContext *tc, StmtInfo *stmt, StmtType type, ptrdiff_t top)
{
    stmt->type = type;
    stmt->flags = 0;
    stmt->blockid = tc->blockid();
    SET_STATEMENT_TOP(stmt, top);
    stmt->label = NULL;
    stmt->blockObj = NULL;
    stmt->down = tc->topStmt;
    tc->topStmt = stmt;
    if (STMT_LINKS_SCOPE(stmt)) {
        stmt->downScope = tc->topScopeStmt;
        tc->topScopeStmt = stmt;
    } else {
        stmt->downScope = NULL;
    }
}

void
frontend::PushBlockScope(TreeContext *tc, StmtInfo *stmt, StaticBlockObject &blockObj, ptrdiff_t top)
{
    PushStatement(tc, stmt, STMT_BLOCK, top);
    stmt->flags |= SIF_SCOPE;
    blockObj.setEnclosingBlock(tc->blockChain);
    stmt->downScope = tc->topScopeStmt;
    tc->topScopeStmt = stmt;
    tc->blockChain = &blockObj;
    stmt->blockObj = &blockObj;
}

void
frontend::PopStatementTC(TreeContext *tc)
{
    StmtInfo *stmt = tc->topStmt;
    tc->topStmt = stmt->down;
    if (STMT_LINKS_SCOPE(stmt)) {
        tc->topScopeStmt = stmt->downScope;
        if (stmt->flags & SIF_SCOPE)
            tc->blockChain = stmt->blockObj->enclosingBlock();
    }
}

StmtInfo *
frontend::LexicalLookup(TreeContext *tc, JSAtom *atom, int *slotp, StmtInfo *stmt)
{
    if (!stmt)
        stmt = tc->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        if (stmt->type == STMT_WITH)
            break;

        
        if (!(stmt->flags & SIF_SCOPE))
            continue;

        StaticBlockObject &blockObj = *stmt->blockObj;
        const Shape *shape = blockObj.nativeLookup(tc->context, AtomToId(atom));
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

