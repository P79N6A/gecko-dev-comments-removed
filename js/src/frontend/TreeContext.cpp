






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
    sc->bindings.trace(trc);
}

bool
frontend::SetStaticLevel(SharedContext *sc, unsigned staticLevel)
{
    



    if (UpvarCookie::isLevelReserved(staticLevel)) {
        JS_ReportErrorNumber(sc->context, js_GetErrorMessage, NULL,
                             JSMSG_TOO_DEEP, js_function_str);
        return false;
    }
    sc->staticLevel = staticLevel;
    return true;
}

bool
frontend::GenerateBlockId(SharedContext *sc, uint32_t &blockid)
{
    if (sc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    blockid = sc->blockidGen++;
    return true;
}

void
frontend::PushStatement(SharedContext *sc, StmtInfo *stmt, StmtType type, ptrdiff_t top)
{
    stmt->type = type;
    stmt->flags = 0;
    stmt->blockid = sc->blockid();
    SET_STATEMENT_TOP(stmt, top);
    stmt->label = NULL;
    stmt->blockObj = NULL;
    stmt->down = sc->topStmt;
    sc->topStmt = stmt;
    if (STMT_LINKS_SCOPE(stmt)) {
        stmt->downScope = sc->topScopeStmt;
        sc->topScopeStmt = stmt;
    } else {
        stmt->downScope = NULL;
    }
}

void
frontend::PushBlockScope(SharedContext *sc, StmtInfo *stmt, StaticBlockObject &blockObj,
                         ptrdiff_t top)
{
    PushStatement(sc, stmt, STMT_BLOCK, top);
    stmt->flags |= SIF_SCOPE;
    blockObj.setEnclosingBlock(sc->blockChain);
    stmt->downScope = sc->topScopeStmt;
    sc->topScopeStmt = stmt;
    sc->blockChain = &blockObj;
    stmt->blockObj = &blockObj;
}

void
frontend::PopStatementSC(SharedContext *sc)
{
    StmtInfo *stmt = sc->topStmt;
    sc->topStmt = stmt->down;
    if (STMT_LINKS_SCOPE(stmt)) {
        sc->topScopeStmt = stmt->downScope;
        if (stmt->flags & SIF_SCOPE)
            sc->blockChain = stmt->blockObj->enclosingBlock();
    }
}

StmtInfo *
frontend::LexicalLookup(SharedContext *sc, JSAtom *atom, int *slotp, StmtInfo *stmt)
{
    if (!stmt)
        stmt = sc->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        if (stmt->type == STMT_WITH)
            break;

        
        if (!(stmt->flags & SIF_SCOPE))
            continue;

        StaticBlockObject &blockObj = *stmt->blockObj;
        const Shape *shape = blockObj.nativeLookup(sc->context, AtomToId(atom));
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

