






#ifndef Parser_inl_h__
#define Parser_inl_h__

#include "frontend/Parser.h"

#include "frontend/SharedContext-inl.h"

namespace js {
namespace frontend {

template <typename ParseHandler>
inline unsigned
ParseContext<ParseHandler>::blockid()
{
    return topStmt ? topStmt->blockid : bodyid;
}

template <typename ParseHandler>
inline bool
ParseContext<ParseHandler>::atBodyLevel()
{
    return !topStmt;
}

template <typename ParseHandler>
inline
ParseContext<ParseHandler>::ParseContext(Parser<ParseHandler> *prs, SharedContext *sc,
                                      unsigned staticLevel, uint32_t bodyid)
  : sc(sc),
    bodyid(0),           
    blockidGen(bodyid),  
    topStmt(NULL),
    topScopeStmt(NULL),
    blockChain(prs->context),
    staticLevel(staticLevel),
    parenDepth(0),
    yieldCount(0),
    blockNode(ParseHandler::null()),
    decls_(prs->context),
    args_(prs->context),
    vars_(prs->context),
    yieldOffset(0),
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

template <typename ParseHandler>
inline bool
ParseContext<ParseHandler>::init()
{
    if (!frontend::GenerateBlockId(this, this->bodyid))
        return false;

    return decls_.init() && lexdeps.ensureMap(sc->context);
}

template <typename ParseHandler>
inline
ParseContext<ParseHandler>::~ParseContext()
{
    
    
    JS_ASSERT(*parserPC == this);
    *parserPC = this->parent;
    js_delete(funcStmts);
}







template <typename ParseHandler>
static bool
CheckStrictBinding(JSContext *cx, ParseHandler *handler, ParseContext<ParseHandler> *pc,
                   HandlePropertyName name, ParseNode *pn)
{
    if (!pc->sc->needStrictChecks())
        return true;

    if (name == cx->names().eval ||
        name == cx->names().arguments ||
        FindKeyword(name->charsZ(), name->length()))
    {
        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, name, &bytes))
            return false;
        return handler->report(ParseStrictError, pn, JSMSG_BAD_BINDING, bytes.ptr());
    }

    return true;
}

} 
} 

#endif 

