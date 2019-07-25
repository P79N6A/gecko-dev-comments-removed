







































#include "frontend/ParseNode.h"

#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"

using namespace js;




#define pn_offsetof(m)  offsetof(ParseNode, m)

JS_STATIC_ASSERT(pn_offsetof(pn_link) == pn_offsetof(dn_uses));

#undef pn_offsetof

void
ParseNode::become(ParseNode *pn2)
{
    JS_ASSERT(!pn_defn);
    JS_ASSERT(!pn2->isDefn());

    JS_ASSERT(!pn_used);
    if (pn2->isUsed()) {
        ParseNode **pnup = &pn2->pn_lexdef->dn_uses;
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

    



    if (this->isKind(PNK_FUNCTION) && isArity(PN_FUNC)) {
        
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
ParseNode::clear()
{
    pn_type = PNK_LIMIT;
    setOp(JSOP_NOP);
    pn_used = pn_defn = false;
    pn_arity = PN_NULLARY;
    pn_parens = false;
}

bool
FunctionBox::joinable() const
{
    return function()->isNullClosure() &&
           (tcflags & (TCF_FUN_USES_ARGUMENTS |
                       TCF_FUN_USES_OWN_NAME |
                       TCF_COMPILE_N_GO)) == TCF_COMPILE_N_GO;
}

bool
FunctionBox::inAnyDynamicScope() const
{
    for (const FunctionBox *funbox = this; funbox; funbox = funbox->parent) {
        if (funbox->tcflags & (TCF_IN_WITH | TCF_FUN_EXTENSIBLE_SCOPE))
            return true;
    }
    return false;
}

bool
FunctionBox::scopeIsExtensible() const
{
    return tcflags & TCF_FUN_EXTENSIBLE_SCOPE;
}

bool
FunctionBox::shouldUnbrand(uintN methods, uintN slowMethods) const
{
    if (slowMethods != 0) {
        for (const FunctionBox *funbox = this; funbox; funbox = funbox->parent) {
            if (!(funbox->tcflags & TCF_FUN_MODULE_PATTERN))
                return true;
            if (funbox->inLoop)
                return true;
        }
    }
    return false;
}


void
ParseNodeAllocator::freeNode(ParseNode *pn)
{
    
    JS_ASSERT(pn != freelist);

    





    JS_ASSERT(!pn->isUsed());
    JS_ASSERT(!pn->isDefn());

    if (pn->isArity(PN_NAMESET) && pn->pn_names.hasMap())
        pn->pn_names.releaseMap(cx);

#ifdef DEBUG
    
    memset(pn, 0xab, sizeof(*pn));
#endif

    pn->pn_next = freelist;
    freelist = pn;
}










class NodeStack {
  public:
    NodeStack() : top(NULL) { }
    bool empty() { return top == NULL; }
    void push(ParseNode *pn) {
        pn->pn_next = top;
        top = pn;
    }
    void pushUnlessNull(ParseNode *pn) { if (pn) push(pn); }
    
    void pushList(ParseNode *pn) {
        
        *pn->pn_tail = top;
        top = pn->pn_head;
    }
    ParseNode *pop() {
        JS_ASSERT(!empty());
        ParseNode *hold = top; 
        top = top->pn_next;
        return hold;
    }
  private:
    ParseNode *top;
};









static bool
PushNodeChildren(ParseNode *pn, NodeStack *stack)
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






void
ParseNodeAllocator::prepareNodeForMutation(ParseNode *pn)
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
                freeNode(pn);
        }
    }
}











ParseNode *
ParseNodeAllocator::freeTree(ParseNode *pn)
{
    if (!pn)
        return NULL;

    ParseNode *savedNext = pn->pn_next;

    NodeStack stack;
    for (;;) {
        if (PushNodeChildren(pn, &stack))
            freeNode(pn);
        if (stack.empty())
            break;
        pn = stack.pop();
    }

    return savedNext;
}





void *
ParseNodeAllocator::allocNode()
{
    if (ParseNode *pn = freelist) {
        freelist = pn->pn_next;
        return pn;
    }

    void *p = cx->tempLifoAlloc().alloc(sizeof (ParseNode));
    if (!p)
        js_ReportOutOfMemory(cx);
    return p;
}



ParseNode *
ParseNode::create(ParseNodeKind kind, ParseNodeArity arity, TreeContext *tc)
{
    Parser *parser = tc->parser;
    const Token &tok = parser->tokenStream.currentToken();
    return parser->new_<ParseNode>(kind, JSOP_NOP, arity, tok.pos);
}

ParseNode *
ParseNode::append(ParseNodeKind kind, JSOp op, ParseNode *left, ParseNode *right)
{
    if (!left || !right)
        return NULL;

    JS_ASSERT(left->isKind(kind) && left->isOp(op) && (js_CodeSpec[op].format & JOF_LEFTASSOC));

    if (left->pn_arity != PN_LIST) {
        ParseNode *pn1 = left->pn_left, *pn2 = left->pn_right;
        left->setArity(PN_LIST);
        left->pn_parens = false;
        left->initList(pn1);
        left->append(pn2);
        if (kind == PNK_ADD) {
            if (pn1->isKind(PNK_STRING))
                left->pn_xflags |= PNX_STRCAT;
            else if (!pn1->isKind(PNK_NUMBER))
                left->pn_xflags |= PNX_CANTFOLD;
            if (pn2->isKind(PNK_STRING))
                left->pn_xflags |= PNX_STRCAT;
            else if (!pn2->isKind(PNK_NUMBER))
                left->pn_xflags |= PNX_CANTFOLD;
        }
    }
    left->append(right);
    left->pn_pos.end = right->pn_pos.end;
    if (kind == PNK_ADD) {
        if (right->isKind(PNK_STRING))
            left->pn_xflags |= PNX_STRCAT;
        else if (!right->isKind(PNK_NUMBER))
            left->pn_xflags |= PNX_CANTFOLD;
    }

    return left;
}

ParseNode *
ParseNode::newBinaryOrAppend(ParseNodeKind kind, JSOp op, ParseNode *left, ParseNode *right,
                             TreeContext *tc)
{
    if (!left || !right)
        return NULL;

    



    if (left->isKind(kind) && left->isOp(op) && (js_CodeSpec[op].format & JOF_LEFTASSOC))
        return append(kind, op, left, right);

    






    if (kind == PNK_ADD &&
        left->isKind(PNK_NUMBER) &&
        right->isKind(PNK_NUMBER) &&
        tc->parser->foldConstants)
    {
        left->pn_dval += right->pn_dval;
        left->pn_pos.end = right->pn_pos.end;
        tc->freeTree(right);
        return left;
    }

    return tc->parser->new_<BinaryNode>(kind, op, left, right);
}

NameNode *
NameNode::create(ParseNodeKind kind, JSAtom *atom, TreeContext *tc)
{
    ParseNode *pn = ParseNode::create(kind, PN_NAME, tc);
    if (pn) {
        pn->pn_atom = atom;
        ((NameNode *)pn)->initCommon(tc);
    }
    return (NameNode *)pn;
}

const char js_argument_str[] = "argument";
const char js_variable_str[] = "variable";
const char js_unknown_str[]  = "unknown";

const char *
Definition::kindString(Kind kind)
{
    static const char *table[] = {
        js_var_str, js_const_str, js_let_str,
        js_function_str, js_argument_str, js_unknown_str
    };

    JS_ASSERT(unsigned(kind) <= unsigned(ARG));
    return table[kind];
}

#if JS_HAS_DESTRUCTURING





static ParseNode *
CloneParseTree(ParseNode *opn, TreeContext *tc)
{
    JS_CHECK_RECURSION(tc->parser->context, return NULL);

    ParseNode *pn = tc->parser->new_<ParseNode>(opn->getKind(), opn->getOp(), opn->getArity(),
                                                opn->pn_pos);
    if (!pn)
        return NULL;
    pn->setInParens(opn->isInParens());
    pn->setDefn(opn->isDefn());
    pn->setUsed(opn->isUsed());

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
        for (ParseNode *opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            ParseNode *pn2;
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
            



            Definition *dn = pn->pn_lexdef;

            pn->pn_link = dn->dn_uses;
            dn->dn_uses = pn;
        } else if (opn->pn_expr) {
            NULLCHECK(pn->pn_expr = CloneParseTree(opn->pn_expr, tc));

            



            if (opn->isDefn()) {
                opn->setDefn(false);
                LinkUseToDef(opn, (Definition *) pn, tc);
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











ParseNode *
js::CloneLeftHandSide(ParseNode *opn, TreeContext *tc)
{
    ParseNode *pn = tc->parser->new_<ParseNode>(opn->getKind(), opn->getOp(), opn->getArity(),
                                                opn->pn_pos);
    if (!pn)
        return NULL;
    pn->setInParens(opn->isInParens());
    pn->setDefn(opn->isDefn());
    pn->setUsed(opn->isUsed());

#if JS_HAS_DESTRUCTURING
    if (opn->isArity(PN_LIST)) {
        JS_ASSERT(opn->isKind(PNK_RB) || opn->isKind(PNK_RC));
        pn->makeEmpty();
        for (ParseNode *opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            ParseNode *pn2;
            if (opn->isKind(PNK_RC)) {
                JS_ASSERT(opn2->isArity(PN_BINARY));
                JS_ASSERT(opn2->isKind(PNK_COLON));

                ParseNode *tag = CloneParseTree(opn2->pn_left, tc);
                if (!tag)
                    return NULL;
                ParseNode *target = CloneLeftHandSide(opn2->pn_right, tc);
                if (!target)
                    return NULL;

                pn2 = tc->parser->new_<BinaryNode>(PNK_COLON, JSOP_INITPROP, opn2->pn_pos, tag, target);
            } else if (opn2->isArity(PN_NULLARY)) {
                JS_ASSERT(opn2->isKind(PNK_COMMA));
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
    JS_ASSERT(opn->isKind(PNK_NAME));

    
    pn->pn_u.name = opn->pn_u.name;
    pn->setOp(JSOP_SETNAME);
    if (opn->isUsed()) {
        Definition *dn = pn->pn_lexdef;

        pn->pn_link = dn->dn_uses;
        dn->dn_uses = pn;
    } else {
        pn->pn_expr = NULL;
        if (opn->isDefn()) {
            
            pn->pn_cookie.makeFree();
            pn->pn_dflags &= ~PND_BOUND;
            pn->setDefn(false);

            LinkUseToDef(pn, (Definition *) opn, tc);
        }
    }
    return pn;
}
