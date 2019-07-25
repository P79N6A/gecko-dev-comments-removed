







































#include "frontend/ParseNode.h"

#include "frontend/CodeGenerator.h"
#include "frontend/Parser.h"

#include "jsscriptinlines.h"

#include "frontend/ParseNode-inl.h"

using namespace js;




#define pn_offsetof(m)  offsetof(JSParseNode, m)

JS_STATIC_ASSERT(pn_offsetof(pn_link) == pn_offsetof(dn_uses));

#undef pn_offsetof

void
JSParseNode::become(JSParseNode *pn2)
{
    JS_ASSERT(!pn_defn);
    JS_ASSERT(!pn2->isDefn());

    JS_ASSERT(!pn_used);
    if (pn2->isUsed()) {
        JSParseNode **pnup = &pn2->pn_lexdef->dn_uses;
        while (*pnup != pn2)
            pnup = &(*pnup)->pn_link;
        *pnup = this;
        pn_link = pn2->pn_link;
        pn_used = true;
        pn2->pn_link = NULL;
        pn2->pn_used = false;
    }

    pn_type = pn2->pn_type;
    pn_op = pn2->pn_op;
    pn_arity = pn2->pn_arity;
    pn_parens = pn2->pn_parens;
    pn_u = pn2->pn_u;

    



    if (this->isKind(TOK_FUNCTION) && isArity(PN_FUNC)) {
        
        JS_ASSERT(pn_funbox->node == pn2);
        pn_funbox->node = this;
    } else if (pn_arity == PN_LIST && !pn_head) {
        
        JS_ASSERT(pn_count == 0);
        JS_ASSERT(pn_tail == &pn2->pn_head);
        pn_tail = &pn_head;
    }

    pn2->clear();
}

void
JSParseNode::clear()
{
    pn_type = TOK_EOF;
    setOp(JSOP_NOP);
    pn_used = pn_defn = false;
    pn_arity = PN_NULLARY;
    pn_parens = false;
}


bool
JSFunctionBox::joinable() const
{
    return function()->isNullClosure() &&
           (tcflags & (TCF_FUN_USES_ARGUMENTS |
                       TCF_FUN_USES_OWN_NAME |
                       TCF_COMPILE_N_GO)) == TCF_COMPILE_N_GO;
}

bool
JSFunctionBox::inAnyDynamicScope() const
{
    for (const JSFunctionBox *funbox = this; funbox; funbox = funbox->parent) {
        if (funbox->tcflags & (TCF_IN_WITH | TCF_FUN_EXTENSIBLE_SCOPE))
            return true;
    }
    return false;
}

bool
JSFunctionBox::scopeIsExtensible() const
{
    return tcflags & TCF_FUN_EXTENSIBLE_SCOPE;
}

bool
JSFunctionBox::shouldUnbrand(uintN methods, uintN slowMethods) const
{
    if (slowMethods != 0) {
        for (const JSFunctionBox *funbox = this; funbox; funbox = funbox->parent) {
            if (!(funbox->tcflags & TCF_FUN_MODULE_PATTERN))
                return true;
            if (funbox->inLoop)
                return true;
        }
    }
    return false;
}

namespace js {


void
AddNodeToFreeList(JSParseNode *pn, js::Parser *parser)
{
    
    JS_ASSERT(pn != parser->nodeList);

    





    JS_ASSERT(!pn->isUsed());
    JS_ASSERT(!pn->isDefn());

    if (pn->isArity(PN_NAMESET) && pn->pn_names.hasMap())
        pn->pn_names.releaseMap(parser->context);

#ifdef DEBUG
    
    memset(pn, 0xab, sizeof(*pn));
#endif

    pn->pn_next = parser->nodeList;
    parser->nodeList = pn;
}










class NodeStack {
  public:
    NodeStack() : top(NULL) { }
    bool empty() { return top == NULL; }
    void push(JSParseNode *pn) {
        pn->pn_next = top;
        top = pn;
    }
    void pushUnlessNull(JSParseNode *pn) { if (pn) push(pn); }
    
    void pushList(JSParseNode *pn) {
        
        *pn->pn_tail = top;
        top = pn->pn_head;
    }
    JSParseNode *pop() {
        JS_ASSERT(!empty());
        JSParseNode *hold = top; 
        top = top->pn_next;
        return hold;
    }
  private:
    JSParseNode *top;
};

} 









static bool
PushNodeChildren(JSParseNode *pn, NodeStack *stack)
{
    switch (pn->getArity()) {
      case PN_FUNC:
        

















        pn->pn_funbox = NULL;
        stack->pushUnlessNull(pn->pn_body);
        pn->pn_body = NULL;
        return false;

      case PN_NAME:
        











        if (!pn->isUsed()) {
            stack->pushUnlessNull(pn->pn_expr);
            pn->pn_expr = NULL;
        }
        return !pn->isUsed() && !pn->isDefn();

      case PN_LIST:
        stack->pushList(pn);
        break;
      case PN_TERNARY:
        stack->pushUnlessNull(pn->pn_kid1);
        stack->pushUnlessNull(pn->pn_kid2);
        stack->pushUnlessNull(pn->pn_kid3);
        break;
      case PN_BINARY:
        if (pn->pn_left != pn->pn_right)
            stack->pushUnlessNull(pn->pn_left);
        stack->pushUnlessNull(pn->pn_right);
        break;
      case PN_UNARY:
        stack->pushUnlessNull(pn->pn_kid);
        break;
      case PN_NULLARY:
        



        return !pn->isUsed() && !pn->isDefn();
      default:
        ;
    }

    return true;
}

namespace js {






void
PrepareNodeForMutation(JSParseNode *pn, JSTreeContext *tc)
{
    if (!pn->isArity(PN_NULLARY)) {
        if (pn->isArity(PN_FUNC)) {
            












            if (pn->pn_funbox)
                pn->pn_funbox->node = NULL;
        }

        
        NodeStack stack;
        PushNodeChildren(pn, &stack);
        



        while (!stack.empty()) {
            pn = stack.pop();
            if (PushNodeChildren(pn, &stack))
                AddNodeToFreeList(pn, tc->parser);
        }
    }
}











JSParseNode *
RecycleTree(JSParseNode *pn, JSTreeContext *tc)
{
    if (!pn)
        return NULL;

    JSParseNode *savedNext = pn->pn_next;

    NodeStack stack;
    for (;;) {
        if (PushNodeChildren(pn, &stack))
            AddNodeToFreeList(pn, tc->parser);
        if (stack.empty())
            break;
        pn = stack.pop();
    }

    return savedNext;
}





JSParseNode *
NewOrRecycledNode(JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = tc->parser->nodeList;
    if (!pn) {
        JSContext *cx = tc->parser->context;
        pn = cx->tempLifoAlloc().new_<JSParseNode>();
        if (!pn)
            js_ReportOutOfMemory(cx);
    } else {
        tc->parser->nodeList = pn->pn_next;
    }

    if (pn) {
        pn->setUsed(false);
        pn->setDefn(false);
        memset(&pn->pn_u, 0, sizeof pn->pn_u);
        pn->pn_next = NULL;
    }
    return pn;
}

} 



JSParseNode *
JSParseNode::create(JSParseNodeArity arity, JSTreeContext *tc)
{
    const Token &tok = tc->parser->tokenStream.currentToken();
    return create(arity, tok.type, JSOP_NOP, tok.pos, tc);
}

JSParseNode *
JSParseNode::create(JSParseNodeArity arity, TokenKind type, JSOp op, const TokenPos &pos,
                    JSTreeContext *tc)
{
    JSParseNode *pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    pn->init(type, op, arity);
    pn->pn_pos = pos;
    return pn;
}

JSParseNode *
JSParseNode::newBinaryOrAppend(TokenKind tt, JSOp op, JSParseNode *left, JSParseNode *right,
                               JSTreeContext *tc)
{
    JSParseNode *pn, *pn1, *pn2;

    if (!left || !right)
        return NULL;

    



    if (left->isKind(tt) &&
        left->isOp(op) &&
        (js_CodeSpec[op].format & JOF_LEFTASSOC)) {
        if (left->pn_arity != PN_LIST) {
            pn1 = left->pn_left, pn2 = left->pn_right;
            left->setArity(PN_LIST);
            left->pn_parens = false;
            left->initList(pn1);
            left->append(pn2);
            if (tt == TOK_PLUS) {
                if (pn1->isKind(TOK_STRING))
                    left->pn_xflags |= PNX_STRCAT;
                else if (!pn1->isKind(TOK_NUMBER))
                    left->pn_xflags |= PNX_CANTFOLD;
                if (pn2->isKind(TOK_STRING))
                    left->pn_xflags |= PNX_STRCAT;
                else if (!pn2->isKind(TOK_NUMBER))
                    left->pn_xflags |= PNX_CANTFOLD;
            }
        }
        left->append(right);
        left->pn_pos.end = right->pn_pos.end;
        if (tt == TOK_PLUS) {
            if (right->isKind(TOK_STRING))
                left->pn_xflags |= PNX_STRCAT;
            else if (!right->isKind(TOK_NUMBER))
                left->pn_xflags |= PNX_CANTFOLD;
        }
        return left;
    }

    






    if (tt == TOK_PLUS &&
        left->isKind(TOK_NUMBER) &&
        right->isKind(TOK_NUMBER) &&
        tc->parser->foldConstants) {
        left->pn_dval += right->pn_dval;
        left->pn_pos.end = right->pn_pos.end;
        RecycleTree(right, tc);
        return left;
    }

    pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    pn->init(tt, op, PN_BINARY);
    pn->pn_pos.begin = left->pn_pos.begin;
    pn->pn_pos.end = right->pn_pos.end;
    pn->pn_left = left;
    pn->pn_right = right;
    return pn;
}

namespace js {

NameNode *
NameNode::create(JSAtom *atom, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = JSParseNode::create(PN_NAME, tc);
    if (pn) {
        pn->pn_atom = atom;
        ((NameNode *)pn)->initCommon(tc);
    }
    return (NameNode *)pn;
}

} 

const char js_argument_str[] = "argument";
const char js_variable_str[] = "variable";
const char js_unknown_str[]  = "unknown";

const char *
JSDefinition::kindString(Kind kind)
{
    static const char *table[] = {
        js_var_str, js_const_str, js_let_str,
        js_function_str, js_argument_str, js_unknown_str
    };

    JS_ASSERT(unsigned(kind) <= unsigned(ARG));
    return table[kind];
}

#if JS_HAS_DESTRUCTURING





static JSParseNode *
CloneParseTree(JSParseNode *opn, JSTreeContext *tc)
{
    JS_CHECK_RECURSION(tc->parser->context, return NULL);

    JSParseNode *pn, *pn2, *opn2;

    pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    pn->setKind(opn->getKind());
    pn->setOp(opn->getOp());
    pn->setUsed(opn->isUsed());
    pn->setDefn(opn->isDefn());
    pn->setArity(opn->getArity());
    pn->setInParens(opn->isInParens());
    pn->pn_pos = opn->pn_pos;

    switch (pn->getArity()) {
#define NULLCHECK(e)    JS_BEGIN_MACRO if (!(e)) return NULL; JS_END_MACRO

      case PN_FUNC:
        NULLCHECK(pn->pn_funbox =
                  tc->parser->newFunctionBox(opn->pn_funbox->object, pn, tc));
        NULLCHECK(pn->pn_body = CloneParseTree(opn->pn_body, tc));
        pn->pn_cookie = opn->pn_cookie;
        pn->pn_dflags = opn->pn_dflags;
        pn->pn_blockid = opn->pn_blockid;
        break;

      case PN_LIST:
        pn->makeEmpty();
        for (opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            NULLCHECK(pn2 = CloneParseTree(opn2, tc));
            pn->append(pn2);
        }
        pn->pn_xflags = opn->pn_xflags;
        break;

      case PN_TERNARY:
        NULLCHECK(pn->pn_kid1 = CloneParseTree(opn->pn_kid1, tc));
        NULLCHECK(pn->pn_kid2 = CloneParseTree(opn->pn_kid2, tc));
        NULLCHECK(pn->pn_kid3 = CloneParseTree(opn->pn_kid3, tc));
        break;

      case PN_BINARY:
        NULLCHECK(pn->pn_left = CloneParseTree(opn->pn_left, tc));
        if (opn->pn_right != opn->pn_left)
            NULLCHECK(pn->pn_right = CloneParseTree(opn->pn_right, tc));
        else
            pn->pn_right = pn->pn_left;
        pn->pn_pval = opn->pn_pval;
        pn->pn_iflags = opn->pn_iflags;
        break;

      case PN_UNARY:
        NULLCHECK(pn->pn_kid = CloneParseTree(opn->pn_kid, tc));
        pn->pn_num = opn->pn_num;
        pn->pn_hidden = opn->pn_hidden;
        break;

      case PN_NAME:
        
        pn->pn_u = opn->pn_u;
        if (opn->isUsed()) {
            



            JSDefinition *dn = pn->pn_lexdef;

            pn->pn_link = dn->dn_uses;
            dn->dn_uses = pn;
        } else if (opn->pn_expr) {
            NULLCHECK(pn->pn_expr = CloneParseTree(opn->pn_expr, tc));

            



            if (opn->isDefn()) {
                opn->setDefn(false);
                LinkUseToDef(opn, (JSDefinition *) pn, tc);
            }
        }
        break;

      case PN_NAMESET:
        pn->pn_names = opn->pn_names;
        NULLCHECK(pn->pn_tree = CloneParseTree(opn->pn_tree, tc));
        break;

      case PN_NULLARY:
        
        pn->pn_u = opn->pn_u;
        break;

#undef NULLCHECK
    }
    return pn;
}

#endif 

namespace js {











JSParseNode *
CloneLeftHandSide(JSParseNode *opn, JSTreeContext *tc)
{
    JSParseNode *pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    pn->setKind(opn->getKind());
    pn->setOp(opn->getOp());
    pn->setUsed(opn->isUsed());
    pn->setDefn(opn->isDefn());
    pn->setArity(opn->getArity());
    pn->setInParens(opn->isInParens());
    pn->pn_pos = opn->pn_pos;

#if JS_HAS_DESTRUCTURING
    if (opn->isArity(PN_LIST)) {
        JS_ASSERT(opn->isKind(TOK_RB) || opn->isKind(TOK_RC));
        pn->makeEmpty();
        for (JSParseNode *opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            JSParseNode *pn2;
            if (opn->isKind(TOK_RC)) {
                JS_ASSERT(opn2->isArity(PN_BINARY));
                JS_ASSERT(opn2->isKind(TOK_COLON));

                JSParseNode *tag = CloneParseTree(opn2->pn_left, tc);
                if (!tag)
                    return NULL;
                JSParseNode *target = CloneLeftHandSide(opn2->pn_right, tc);
                if (!target)
                    return NULL;
                pn2 = BinaryNode::create(TOK_COLON, JSOP_INITPROP, opn2->pn_pos, tag, target, tc);
            } else if (opn2->isArity(PN_NULLARY)) {
                JS_ASSERT(opn2->isKind(TOK_COMMA));
                pn2 = CloneParseTree(opn2, tc);
            } else {
                pn2 = CloneLeftHandSide(opn2, tc);
            }

            if (!pn2)
                return NULL;
            pn->append(pn2);
        }
        pn->pn_xflags = opn->pn_xflags;
        return pn;
    }
#endif

    JS_ASSERT(opn->isArity(PN_NAME));
    JS_ASSERT(opn->isKind(TOK_NAME));

    
    pn->pn_u.name = opn->pn_u.name;
    pn->setOp(JSOP_SETNAME);
    if (opn->isUsed()) {
        JSDefinition *dn = pn->pn_lexdef;

        pn->pn_link = dn->dn_uses;
        dn->dn_uses = pn;
    } else {
        pn->pn_expr = NULL;
        if (opn->isDefn()) {
            
            pn->pn_cookie.makeFree();
            pn->pn_dflags &= ~PND_BOUND;
            pn->setDefn(false);

            LinkUseToDef(pn, (JSDefinition *) opn, tc);
        }
    }
    return pn;
}

} 
