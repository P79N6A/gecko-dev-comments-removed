






































#ifndef ParseNode_inl_h__
#define ParseNode_inl_h__

#include "frontend/ParseNode.h"
#include "frontend/CodeGenerator.h"
#include "frontend/TokenStream.h"

inline bool
JSParseNode::isConstant()
{
    using namespace js;

    switch (pn_type) {
      case TOK_NUMBER:
      case TOK_STRING:
        return true;
      case TOK_PRIMARY:
        switch (pn_op) {
          case JSOP_NULL:
          case JSOP_FALSE:
          case JSOP_TRUE:
            return true;
          default:
            return false;
        }
      case TOK_RB:
      case TOK_RC:
        return isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST);
      default:
        return false;
    }
}

namespace js {

inline void
NameNode::initCommon(JSTreeContext *tc)
{
    pn_expr = NULL;
    pn_cookie.makeFree();
    pn_dflags = (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
                ? PND_BLOCKCHILD
                : 0;
    pn_blockid = tc->blockid();
}

} 

#endif 
