






#ifndef Parser_inl_h__
#define Parser_inl_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

inline unsigned
ParseContext::blockid()
{
    return topStmt ? topStmt->blockid : bodyid;
}

inline bool
ParseContext::atBodyLevel()
{
    return !topStmt;
}

inline
ParseContext::ParseContext(Parser *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid)
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
    queuedStrictModeError(NULL),
    parserPC(&prs->pc),
    lexdeps(prs->context),
    parent(prs->pc),
    funcStmts(NULL),
    funHasReturnExpr(false),
    funHasReturnVoid(false),
    parsingForInit(false),
    parsingWith(prs->pc ? prs->pc->parsingWith : false), 
    inDeclDestructuring(false),
    funBecameStrict(false)
{
    prs->pc = this;
}

inline bool
ParseContext::init()
{
    if (!frontend::GenerateBlockId(this, this->bodyid))
        return false;

    return decls_.init() && lexdeps.ensureMap(sc->context);
}

inline void
ParseContext::setQueuedStrictModeError(CompileError *e)
{
    JS_ASSERT(!queuedStrictModeError);
    queuedStrictModeError = e;
}

inline
ParseContext::~ParseContext()
{
    
    
    JS_ASSERT(*parserPC == this);
    *parserPC = this->parent;
    js_delete(funcStmts);
}

} 
} 

#endif 

