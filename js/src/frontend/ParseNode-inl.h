





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
    JS_ASSERT(isKind(PNK_FUNCTIONDECL) || isKind(PNK_NAME) || isKind(PNK_INTRINSICNAME));
    JSAtom *atom = isKind(PNK_FUNCTIONDECL) ? pn_funbox->fun()->atom() : pn_atom;
    return atom->asPropertyName();
}

inline bool
ParseNode::isConstant()
{
    switch (pn_type) {
      case PNK_NUMBER:
      case PNK_STRING:
      case PNK_NULL:
      case PNK_FALSE:
      case PNK_TRUE:
        return true;
      case PNK_ARRAY:
      case PNK_OBJECT:
        return isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST);
      default:
        return false;
    }
}

struct ParseContext;

inline void
NameNode::initCommon(ParseContext *pc)
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
