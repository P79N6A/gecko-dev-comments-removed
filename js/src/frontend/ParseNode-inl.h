





#ifndef ParseNode_inl_h__
#define ParseNode_inl_h__

#include "frontend/ParseNode.h"

namespace js {
namespace frontend {

inline bool
UpvarCookie::set(JSContext *cx, unsigned newLevel, uint16_t newSlot)
{
    
    
    
    
    if (newLevel >= FREE_LEVEL) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_DEEP, js_function_str);
        return false;
    }
    level_ = newLevel;
    slot_ = newSlot;
    return true;
}

inline PropertyName *
ParseNode::name() const
{
    JS_ASSERT(isKind(PNK_FUNCTION) || isKind(PNK_NAME));
    JSAtom *atom = isKind(PNK_FUNCTION) ? pn_funbox->function()->atom() : pn_atom;
    return atom->asPropertyName();
}

inline JSAtom *
ParseNode::atom() const
{
    JS_ASSERT(isKind(PNK_MODULE) || isKind(PNK_STRING));
    return isKind(PNK_MODULE) ? pn_modulebox->module()->atom() : pn_atom;
}

inline void
NameNode::initCommon(ParseContext<FullParseHandler> *pc)
{
    pn_expr = NULL;
    pn_cookie.makeFree();
    pn_dflags = (!pc->topStmt || pc->topStmt->type == STMT_BLOCK)
                ? PND_BLOCKCHILD
                : 0;
    pn_blockid = pc->blockid();
}

} 
} 

#endif 
