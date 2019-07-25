






































#ifndef ParseNode_inl_h__
#define ParseNode_inl_h__

#include "frontend/ParseNode.h"
#include "frontend/BytecodeEmitter.h"
#include "frontend/TokenStream.h"

namespace js {

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
      case PNK_RB:
      case PNK_RC:
        return isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST);
      default:
        return false;
    }
}

#ifdef DEBUG
inline void
IndentNewLine(int indent)
{
    fputc('\n', stderr);
    for (int i = 0; i < indent; ++i)
        fputc(' ', stderr);
}

inline void
ParseNode::dump(int indent)
{
    switch (pn_arity) {
      case PN_NULLARY:
        ((NullaryNode *) this)->dump();
        break;
      case PN_UNARY:
        ((UnaryNode *) this)->dump(indent);
        break;
      case PN_BINARY:
        ((BinaryNode *) this)->dump(indent);
        break;
      case PN_TERNARY:
        ((TernaryNode *) this)->dump(indent);
        break;
      case PN_FUNC:
        ((FunctionNode *) this)->dump(indent);
        break;
      case PN_LIST:
        ((ListNode *) this)->dump(indent);
        break;
      case PN_NAME:
        ((NameNode *) this)->dump(indent);
        break;
      default:
        fprintf(stderr, "?");
        break;
    }
}

inline void
NullaryNode::dump()
{
    fprintf(stderr, "(%s)", js_CodeName[getOp()]);
}

inline void
UnaryNode::dump(int indent)
{
    const char *name = js_CodeName[getOp()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_kid, indent);
    fprintf(stderr, ")");
}

inline void
BinaryNode::dump(int indent)
{
    const char *name = js_CodeName[getOp()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_left, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_right, indent);
    fprintf(stderr, ")");
}

inline void
TernaryNode::dump(int indent)
{
    const char *name = js_CodeName[getOp()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_kid1, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_kid2, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_kid3, indent);
    fprintf(stderr, ")");
}

inline void
FunctionNode::dump(int indent)
{
    const char *name = js_CodeName[getOp()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_body, indent);
    fprintf(stderr, ")");
}

inline void
ListNode::dump(int indent)
{
    const char *name = js_CodeName[getOp()];
    fprintf(stderr, "(%s ", name);
    if (pn_head != NULL) {
        indent += strlen(name) + 2;
        DumpParseTree(pn_head, indent);
        ParseNode *pn = pn_head->pn_next;
        while (pn != NULL) {
            IndentNewLine(indent);
            DumpParseTree(pn, indent);
            pn = pn->pn_next;
        }
    }
    fprintf(stderr, ")");
}

inline void
NameNode::dump(int indent)
{
    const char *name = js_CodeName[getOp()];
    if (isUsed())
        fprintf(stderr, "(%s)", name);
    else {
        fprintf(stderr, "(%s ", name);
        indent += strlen(name) + 2;
        DumpParseTree(expr(), indent);
        fprintf(stderr, ")");
    }
}
#endif

inline void
NameNode::initCommon(TreeContext *tc)
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
