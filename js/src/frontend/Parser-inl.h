






#ifndef Parser_inl_h__
#define Parser_inl_h__

#include "frontend/Parser.h"

namespace js {
namespace frontend {

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
    inDeclDestructuring(false)
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
} 

#endif 

