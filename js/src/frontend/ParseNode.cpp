





#include "frontend/ParseNode-inl.h"

#include "frontend/Parser.h"

#include "jscntxtinlines.h"

using namespace js;
using namespace js::frontend;

using mozilla::IsFinite;

#ifdef DEBUG
void
ParseNode::checkListConsistency()
{
    MOZ_ASSERT(isArity(PN_LIST));
    ParseNode** tail;
    uint32_t count = 0;
    if (pn_head) {
        ParseNode* last = pn_head;
        ParseNode* pn = last;
        while (pn) {
            last = pn;
            pn = pn->pn_next;
            count++;
        }

        tail = &last->pn_next;
    } else {
        tail = &pn_head;
    }
    MOZ_ASSERT(pn_tail == tail);
    MOZ_ASSERT(pn_count == count);
}
#endif


void
ParseNodeAllocator::freeNode(ParseNode* pn)
{
    
    MOZ_ASSERT(pn != freelist);

    





    MOZ_ASSERT(!pn->isUsed());
    MOZ_ASSERT(!pn->isDefn());

#ifdef DEBUG
    
    memset(pn, 0xab, sizeof(*pn));
#endif

    pn->pn_next = freelist;
    freelist = pn;
}

namespace {










class NodeStack {
  public:
    NodeStack() : top(nullptr) { }
    bool empty() { return top == nullptr; }
    void push(ParseNode* pn) {
        pn->pn_next = top;
        top = pn;
    }
    
    void pushList(ParseNode* pn) {
        
        *pn->pn_tail = top;
        top = pn->pn_head;
    }
    ParseNode* pop() {
        MOZ_ASSERT(!empty());
        ParseNode* hold = top; 
        top = top->pn_next;
        return hold;
    }
  private:
    ParseNode* top;
};

} 

enum class PushResult { Recyclable, CleanUpLater };

static PushResult
PushCodeNodeChildren(ParseNode* node, NodeStack* stack)
{
    MOZ_ASSERT(node->isArity(PN_CODE));

    
















    node->pn_funbox = nullptr;
    if (node->pn_body)
        stack->push(node->pn_body);
    node->pn_body = nullptr;

    return PushResult::CleanUpLater;
}

static PushResult
PushNameNodeChildren(ParseNode* node, NodeStack* stack)
{
    MOZ_ASSERT(node->isArity(PN_NAME));

    










    if (!node->isUsed()) {
        if (node->pn_expr)
            stack->push(node->pn_expr);
        node->pn_expr = nullptr;
    }

    if (!node->isUsed() && !node->isDefn())
        return PushResult::Recyclable;

    return PushResult::CleanUpLater;
}

static PushResult
PushListNodeChildren(ParseNode* node, NodeStack* stack)
{
    MOZ_ASSERT(node->isArity(PN_LIST));
    node->checkListConsistency();

    stack->pushList(node);

    return PushResult::Recyclable;
}

static PushResult
PushUnaryNodeChild(ParseNode* node, NodeStack* stack)
{
    MOZ_ASSERT(node->isArity(PN_UNARY));

    stack->push(node->pn_kid);

    return PushResult::Recyclable;
}









static PushResult
PushNodeChildren(ParseNode* pn, NodeStack* stack)
{
    switch (pn->getKind()) {
      
      
      case PNK_NOP:
      case PNK_STRING:
      case PNK_TEMPLATE_STRING:
      case PNK_REGEXP:
      case PNK_TRUE:
      case PNK_FALSE:
      case PNK_NULL:
      case PNK_THIS:
      case PNK_ELISION:
      case PNK_GENERATOR:
      case PNK_NUMBER:
      case PNK_BREAK:
      case PNK_CONTINUE:
      case PNK_DEBUGGER:
      case PNK_EXPORT_BATCH_SPEC:
      case PNK_OBJECT_PROPERTY_NAME:
      case PNK_FRESHENBLOCK:
        MOZ_ASSERT(pn->isArity(PN_NULLARY));
        MOZ_ASSERT(!pn->isUsed(), "handle non-trivial cases separately");
        MOZ_ASSERT(!pn->isDefn(), "handle non-trivial cases separately");
        return PushResult::Recyclable;

      
      case PNK_TYPEOF:
      case PNK_VOID:
      case PNK_NOT:
      case PNK_BITNOT:
      case PNK_THROW:
      case PNK_DELETE:
      case PNK_POS:
      case PNK_NEG:
      case PNK_PREINCREMENT:
      case PNK_POSTINCREMENT:
      case PNK_PREDECREMENT:
      case PNK_POSTDECREMENT:
      case PNK_COMPUTED_NAME:
      case PNK_ARRAYPUSH:
      case PNK_SPREAD:
      case PNK_MUTATEPROTO:
      case PNK_EXPORT:
        return PushUnaryNodeChild(pn, stack);

      
      case PNK_SEMI: {
        MOZ_ASSERT(pn->isArity(PN_UNARY));
        if (pn->pn_kid)
            stack->push(pn->pn_kid);
        return PushResult::Recyclable;
      }

      

      
      case PNK_ASSIGN:
      case PNK_ADDASSIGN:
      case PNK_SUBASSIGN:
      case PNK_BITORASSIGN:
      case PNK_BITXORASSIGN:
      case PNK_BITANDASSIGN:
      case PNK_LSHASSIGN:
      case PNK_RSHASSIGN:
      case PNK_URSHASSIGN:
      case PNK_MULASSIGN:
      case PNK_DIVASSIGN:
      case PNK_MODASSIGN:
      
      case PNK_ELEM:
      case PNK_LETEXPR:
      case PNK_IMPORT_SPEC:
      case PNK_EXPORT_SPEC:
      case PNK_COLON:
      case PNK_CASE:
      case PNK_SHORTHAND:
      case PNK_DOWHILE:
      case PNK_WHILE:
      case PNK_SWITCH:
      case PNK_LETBLOCK:
      case PNK_CLASSMETHOD:
      case PNK_FOR: {
        MOZ_ASSERT(pn->isArity(PN_BINARY));
        stack->push(pn->pn_left);
        stack->push(pn->pn_right);
        return PushResult::Recyclable;
      }

      
      case PNK_CLASSNAMES: {
        MOZ_ASSERT(pn->isArity(PN_BINARY));
        if (pn->pn_left)
            stack->push(pn->pn_left);
        stack->push(pn->pn_right);
        return PushResult::Recyclable;
      }

      
      
      
      
      case PNK_WITH: {
        MOZ_ASSERT(pn->isArity(PN_BINARY_OBJ));
        stack->push(pn->pn_left);
        stack->push(pn->pn_right);
        return PushResult::Recyclable;
      }

      
      
      
      case PNK_DEFAULT: {
        MOZ_ASSERT(pn->isArity(PN_BINARY));
        MOZ_ASSERT(pn->pn_left == nullptr);
        stack->push(pn->pn_right);
        return PushResult::Recyclable;
      }

      
      
      
      
      case PNK_YIELD_STAR:
      case PNK_YIELD: {
        MOZ_ASSERT(pn->isArity(PN_BINARY));
        MOZ_ASSERT(pn->pn_right);
        MOZ_ASSERT(pn->pn_right->isKind(PNK_NAME) ||
                   (pn->pn_right->isKind(PNK_ASSIGN) &&
                    pn->pn_right->pn_left->isKind(PNK_NAME) &&
                    pn->pn_right->pn_right->isKind(PNK_GENERATOR)));
        if (pn->pn_left)
            stack->push(pn->pn_left);
        stack->push(pn->pn_right);
        return PushResult::Recyclable;
      }

      
      
      
      case PNK_RETURN: {
        MOZ_ASSERT(pn->isArity(PN_BINARY));
        if (pn->pn_left)
            stack->push(pn->pn_left);
        if (pn->pn_right) {
            MOZ_ASSERT(pn->pn_right->isKind(PNK_NAME));
            MOZ_ASSERT(pn->pn_right->pn_atom->equals(".genrval"));
            MOZ_ASSERT(pn->pn_right->isAssigned());
            stack->push(pn->pn_right);
        }
        return PushResult::Recyclable;
      }

      
      
      case PNK_IMPORT:
      case PNK_EXPORT_FROM: {
        MOZ_ASSERT(pn->isArity(PN_BINARY));
        MOZ_ASSERT_IF(pn->isKind(PNK_IMPORT), pn->pn_left->isKind(PNK_IMPORT_SPEC_LIST));
        MOZ_ASSERT_IF(pn->isKind(PNK_EXPORT_FROM), pn->pn_left->isKind(PNK_EXPORT_SPEC_LIST));
        MOZ_ASSERT(pn->pn_left->isArity(PN_LIST));
        MOZ_ASSERT(pn->pn_right->isKind(PNK_STRING));
        stack->pushList(pn->pn_left);
        stack->push(pn->pn_right);
        return PushResult::Recyclable;
      }

      
      case PNK_CONDITIONAL: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        stack->push(pn->pn_kid1);
        stack->push(pn->pn_kid2);
        stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      
      
      
      
      
      case PNK_FORIN:
      case PNK_FOROF: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        if (pn->pn_kid1)
            stack->push(pn->pn_kid1);
        stack->push(pn->pn_kid2);
        stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      case PNK_FORHEAD: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        if (pn->pn_kid1)
            stack->push(pn->pn_kid1);
        if (pn->pn_kid2)
            stack->push(pn->pn_kid2);
        if (pn->pn_kid3)
            stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      case PNK_CLASS: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        if (pn->pn_kid1)
            stack->push(pn->pn_kid1);
        if (pn->pn_kid2)
            stack->push(pn->pn_kid2);
        stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      
      case PNK_IF: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        stack->push(pn->pn_kid1);
        stack->push(pn->pn_kid2);
        if (pn->pn_kid3)
            stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      
      case PNK_TRY: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        MOZ_ASSERT(pn->pn_kid2 || pn->pn_kid3);
        stack->push(pn->pn_kid1);
        if (pn->pn_kid2)
            stack->push(pn->pn_kid2);
        if (pn->pn_kid3)
            stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      
      
      
      case PNK_CATCH: {
        MOZ_ASSERT(pn->isArity(PN_TERNARY));
        stack->push(pn->pn_kid1);
        if (pn->pn_kid2)
            stack->push(pn->pn_kid2);
        stack->push(pn->pn_kid3);
        return PushResult::Recyclable;
      }

      
      case PNK_OR:
      case PNK_AND:
      case PNK_BITOR:
      case PNK_BITXOR:
      case PNK_BITAND:
      case PNK_STRICTEQ:
      case PNK_EQ:
      case PNK_STRICTNE:
      case PNK_NE:
      case PNK_LT:
      case PNK_LE:
      case PNK_GT:
      case PNK_GE:
      case PNK_INSTANCEOF:
      case PNK_IN:
      case PNK_LSH:
      case PNK_RSH:
      case PNK_URSH:
      case PNK_ADD:
      case PNK_SUB:
      case PNK_STAR:
      case PNK_DIV:
      case PNK_MOD:
      case PNK_COMMA:
      case PNK_NEW:
      case PNK_CALL:
      case PNK_GENEXP:
      case PNK_ARRAY:
      case PNK_OBJECT:
      case PNK_TEMPLATE_STRING_LIST:
      case PNK_TAGGED_TEMPLATE:
      case PNK_CALLSITEOBJ:
      case PNK_VAR:
      case PNK_CONST:
      case PNK_GLOBALCONST:
      case PNK_LET:
      case PNK_CATCHLIST:
      case PNK_STATEMENTLIST:
      case PNK_IMPORT_SPEC_LIST:
      case PNK_EXPORT_SPEC_LIST:
      case PNK_SEQ:
      case PNK_ARGSBODY:
      case PNK_CLASSMETHODLIST:
        return PushListNodeChildren(pn, stack);

      
      
      
      case PNK_ARRAYCOMP: {
#ifdef DEBUG
        MOZ_ASSERT(pn->isKind(PNK_ARRAYCOMP));
        MOZ_ASSERT(pn->isArity(PN_LIST));
        MOZ_ASSERT(pn->pn_count == 1);
        MOZ_ASSERT(pn->pn_head->isKind(PNK_LEXICALSCOPE) || pn->pn_head->isKind(PNK_FOR));
#endif
        return PushListNodeChildren(pn, stack);
      }

      case PNK_LABEL:
      case PNK_DOT:
      case PNK_LEXICALSCOPE:
      case PNK_NAME:
        return PushNameNodeChildren(pn, stack);

      case PNK_FUNCTION:
        return PushCodeNodeChildren(pn, stack);

      case PNK_LIMIT: 
        MOZ_CRASH("invalid node kind");
    }

    MOZ_CRASH("bad ParseNodeKind");
    return PushResult::CleanUpLater;
}






void
ParseNodeAllocator::prepareNodeForMutation(ParseNode* pn)
{
    
    if (pn->isArity(PN_NULLARY))
        return;

    
    NodeStack stack;
    PushNodeChildren(pn, &stack);

    
    
    while (!stack.empty()) {
        pn = stack.pop();
        if (PushNodeChildren(pn, &stack) == PushResult::Recyclable)
            freeNode(pn);
    }
}





ParseNode*
ParseNodeAllocator::freeTree(ParseNode* pn)
{
    if (!pn)
        return nullptr;

    ParseNode* savedNext = pn->pn_next;

    NodeStack stack;
    for (;;) {
        if (PushNodeChildren(pn, &stack) == PushResult::Recyclable)
            freeNode(pn);
        if (stack.empty())
            break;
        pn = stack.pop();
    }

    return savedNext;
}





void*
ParseNodeAllocator::allocNode()
{
    if (ParseNode* pn = freelist) {
        freelist = pn->pn_next;
        return pn;
    }

    void* p = alloc.alloc(sizeof (ParseNode));
    if (!p)
        ReportOutOfMemory(cx);
    return p;
}

ParseNode*
ParseNode::appendOrCreateList(ParseNodeKind kind, JSOp op, ParseNode* left, ParseNode* right,
                              FullParseHandler* handler, ParseContext<FullParseHandler>* pc)
{
    
    
    
    
    
    if (!pc->useAsmOrInsideUseAsm()) {
        
        
        
        
        
        if (left->isKind(kind) && left->isOp(op) && (js_CodeSpec[op].format & JOF_LEFTASSOC)) {
            ListNode* list = &left->as<ListNode>();

            list->append(right);
            list->pn_pos.end = right->pn_pos.end;

            return list;
        }
    }

    ParseNode* list = handler->new_<ListNode>(kind, op, left);
    if (!list)
        return nullptr;

    list->append(right);
    return list;
}

const char*
Definition::kindString(Kind kind)
{
    static const char * const table[] = {
        "", js_var_str, js_const_str, js_const_str, js_let_str, "argument", js_function_str, "unknown"
    };

    MOZ_ASSERT(unsigned(kind) <= unsigned(ARG));
    return table[kind];
}

namespace js {
namespace frontend {





template <>
ParseNode*
Parser<FullParseHandler>::cloneParseTree(ParseNode* opn)
{
    JS_CHECK_RECURSION(context, return nullptr);

    if (opn->isKind(PNK_COMPUTED_NAME)) {
        report(ParseError, false, opn, JSMSG_COMPUTED_NAME_IN_PATTERN);
        return null();
    }

    ParseNode* pn = handler.new_<ParseNode>(opn->getKind(), opn->getOp(), opn->getArity(),
                                            opn->pn_pos);
    if (!pn)
        return nullptr;
    pn->setInParens(opn->isInParens());
    pn->setDefn(opn->isDefn());
    pn->setUsed(opn->isUsed());

    switch (pn->getArity()) {
#define NULLCHECK(e)    JS_BEGIN_MACRO if (!(e)) return nullptr; JS_END_MACRO

      case PN_CODE:
        NULLCHECK(pn->pn_funbox = newFunctionBox(pn, opn->pn_funbox->function(), pc,
                                                 Directives( opn->pn_funbox->strict()),
                                                 opn->pn_funbox->generatorKind()));
        NULLCHECK(pn->pn_body = cloneParseTree(opn->pn_body));
        pn->pn_cookie = opn->pn_cookie;
        pn->pn_dflags = opn->pn_dflags;
        pn->pn_blockid = opn->pn_blockid;
        break;

      case PN_LIST:
        pn->makeEmpty();
        for (ParseNode* opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            ParseNode* pn2;
            NULLCHECK(pn2 = cloneParseTree(opn2));
            pn->append(pn2);
        }
        pn->pn_xflags = opn->pn_xflags;
        break;

      case PN_TERNARY:
        if (opn->pn_kid1)
            NULLCHECK(pn->pn_kid1 = cloneParseTree(opn->pn_kid1));
        if (opn->pn_kid2)
            NULLCHECK(pn->pn_kid2 = cloneParseTree(opn->pn_kid2));
        if (opn->pn_kid3)
            NULLCHECK(pn->pn_kid3 = cloneParseTree(opn->pn_kid3));
        break;

      case PN_BINARY:
      case PN_BINARY_OBJ:
        if (opn->pn_left)
            NULLCHECK(pn->pn_left = cloneParseTree(opn->pn_left));
        if (opn->pn_right) {
            if (opn->pn_right != opn->pn_left)
                NULLCHECK(pn->pn_right = cloneParseTree(opn->pn_right));
            else
                pn->pn_right = pn->pn_left;
        }
        if (opn->isArity(PN_BINARY)) {
            pn->pn_iflags = opn->pn_iflags;
        } else {
            MOZ_ASSERT(opn->isArity(PN_BINARY_OBJ));
            pn->pn_binary_obj = opn->pn_binary_obj;
        }
        break;

      case PN_UNARY:
        if (opn->pn_kid)
            NULLCHECK(pn->pn_kid = cloneParseTree(opn->pn_kid));
        break;

      case PN_NAME:
        
        pn->pn_u = opn->pn_u;
        if (opn->isUsed()) {
            



            Definition* dn = pn->pn_lexdef;

            pn->pn_link = dn->dn_uses;
            pn->pn_dflags = opn->pn_dflags;
            dn->dn_uses = pn;
        } else if (opn->pn_expr) {
            NULLCHECK(pn->pn_expr = cloneParseTree(opn->pn_expr));

            



            if (opn->isDefn()) {
                opn->setDefn(false);
                handler.linkUseToDef(opn, (Definition*) pn);
            }
        }
        break;

      case PN_NULLARY:
        pn->pn_u = opn->pn_u;
        break;

#undef NULLCHECK
    }
    return pn;
}

template <>
ParseNode*
Parser<FullParseHandler>::cloneLeftHandSide(ParseNode* opn);






template <>
ParseNode*
Parser<FullParseHandler>::cloneDestructuringDefault(ParseNode* opn)
{
    MOZ_ASSERT(opn->isKind(PNK_ASSIGN));

    report(ParseError, false, opn, JSMSG_DEFAULT_IN_PATTERN);
    return null();
}











template <>
ParseNode*
Parser<FullParseHandler>::cloneLeftHandSide(ParseNode* opn)
{
    ParseNode* pn = handler.new_<ParseNode>(opn->getKind(), opn->getOp(), opn->getArity(),
                                            opn->pn_pos);
    if (!pn)
        return nullptr;
    pn->setInParens(opn->isInParens());
    pn->setDefn(opn->isDefn());
    pn->setUsed(opn->isUsed());

    if (opn->isArity(PN_LIST)) {
        MOZ_ASSERT(opn->isKind(PNK_ARRAY) || opn->isKind(PNK_OBJECT));
        pn->makeEmpty();
        for (ParseNode* opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            ParseNode* pn2;
            if (opn->isKind(PNK_OBJECT)) {
                if (opn2->isKind(PNK_MUTATEPROTO)) {
                    ParseNode* target = cloneLeftHandSide(opn2->pn_kid);
                    if (!target)
                        return nullptr;
                    pn2 = handler.new_<UnaryNode>(PNK_MUTATEPROTO, JSOP_NOP, opn2->pn_pos, target);
                } else {
                    MOZ_ASSERT(opn2->isArity(PN_BINARY));
                    MOZ_ASSERT(opn2->isKind(PNK_COLON) || opn2->isKind(PNK_SHORTHAND));

                    ParseNode* tag = cloneParseTree(opn2->pn_left);
                    if (!tag)
                        return nullptr;
                    ParseNode* target;
                    if (opn2->pn_right->isKind(PNK_ASSIGN)) {
                        target = cloneDestructuringDefault(opn2->pn_right);
                    } else {
                        target = cloneLeftHandSide(opn2->pn_right);
                    }
                    if (!target)
                        return nullptr;

                    pn2 = handler.new_<BinaryNode>(opn2->getKind(), JSOP_INITPROP, opn2->pn_pos, tag, target);
                }
            } else if (opn2->isArity(PN_NULLARY)) {
                MOZ_ASSERT(opn2->isKind(PNK_ELISION));
                pn2 = cloneParseTree(opn2);
            } else if (opn2->isKind(PNK_SPREAD)) {
                ParseNode* target = cloneLeftHandSide(opn2->pn_kid);
                if (!target)
                    return nullptr;
                pn2 = handler.new_<UnaryNode>(PNK_SPREAD, JSOP_NOP, opn2->pn_pos, target);
            } else if (opn2->isKind(PNK_ASSIGN)) {
                pn2 = cloneDestructuringDefault(opn2);
            } else {
                pn2 = cloneLeftHandSide(opn2);
            }

            if (!pn2)
                return nullptr;
            pn->append(pn2);
        }
        pn->pn_xflags = opn->pn_xflags;
        return pn;
    }

    MOZ_ASSERT(opn->isArity(PN_NAME));
    MOZ_ASSERT(opn->isKind(PNK_NAME));

    
    pn->pn_u.name = opn->pn_u.name;
    pn->setOp(JSOP_SETNAME);
    if (opn->isUsed()) {
        Definition* dn = pn->pn_lexdef;

        pn->pn_link = dn->dn_uses;
        dn->dn_uses = pn;
    } else {
        pn->pn_expr = nullptr;
        if (opn->isDefn()) {
            
            pn->pn_cookie.makeFree();
            pn->pn_dflags &= ~(PND_LEXICAL | PND_BOUND);
            pn->setDefn(false);

            handler.linkUseToDef(pn, (Definition*) opn);
        }
    }
    return pn;
}

} 
} 

#ifdef DEBUG

static const char * const parseNodeNames[] = {
#define STRINGIFY(name) #name,
    FOR_EACH_PARSE_NODE_KIND(STRINGIFY)
#undef STRINGIFY
};

void
frontend::DumpParseTree(ParseNode* pn, int indent)
{
    if (pn == nullptr)
        fprintf(stderr, "#NULL");
    else
        pn->dump(indent);
}

static void
IndentNewLine(int indent)
{
    fputc('\n', stderr);
    for (int i = 0; i < indent; ++i)
        fputc(' ', stderr);
}

void
ParseNode::dump()
{
    dump(0);
    fputc('\n', stderr);
}

void
ParseNode::dump(int indent)
{
    switch (pn_arity) {
      case PN_NULLARY:
        ((NullaryNode*) this)->dump();
        break;
      case PN_UNARY:
        ((UnaryNode*) this)->dump(indent);
        break;
      case PN_BINARY:
        ((BinaryNode*) this)->dump(indent);
        break;
      case PN_BINARY_OBJ:
        ((BinaryObjNode*) this)->dump(indent);
        break;
      case PN_TERNARY:
        ((TernaryNode*) this)->dump(indent);
        break;
      case PN_CODE:
        ((CodeNode*) this)->dump(indent);
        break;
      case PN_LIST:
        ((ListNode*) this)->dump(indent);
        break;
      case PN_NAME:
        ((NameNode*) this)->dump(indent);
        break;
      default:
        fprintf(stderr, "#<BAD NODE %p, kind=%u, arity=%u>",
                (void*) this, unsigned(getKind()), unsigned(pn_arity));
        break;
    }
}

void
NullaryNode::dump()
{
    switch (getKind()) {
      case PNK_TRUE:  fprintf(stderr, "#true");  break;
      case PNK_FALSE: fprintf(stderr, "#false"); break;
      case PNK_NULL:  fprintf(stderr, "#null");  break;

      case PNK_NUMBER: {
        ToCStringBuf cbuf;
        const char* cstr = NumberToCString(nullptr, &cbuf, pn_dval);
        if (!IsFinite(pn_dval))
            fputc('#', stderr);
        if (cstr)
            fprintf(stderr, "%s", cstr);
        else
            fprintf(stderr, "%g", pn_dval);
        break;
      }

      case PNK_STRING:
        pn_atom->dumpCharsNoNewline();
        break;

      default:
        fprintf(stderr, "(%s)", parseNodeNames[getKind()]);
    }
}

void
UnaryNode::dump(int indent)
{
    const char* name = parseNodeNames[getKind()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_kid, indent);
    fprintf(stderr, ")");
}

void
BinaryNode::dump(int indent)
{
    const char* name = parseNodeNames[getKind()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_left, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_right, indent);
    fprintf(stderr, ")");
}

void
BinaryObjNode::dump(int indent)
{
    const char* name = parseNodeNames[getKind()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_left, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_right, indent);
    fprintf(stderr, ")");
}

void
TernaryNode::dump(int indent)
{
    const char* name = parseNodeNames[getKind()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_kid1, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_kid2, indent);
    IndentNewLine(indent);
    DumpParseTree(pn_kid3, indent);
    fprintf(stderr, ")");
}

void
CodeNode::dump(int indent)
{
    const char* name = parseNodeNames[getKind()];
    fprintf(stderr, "(%s ", name);
    indent += strlen(name) + 2;
    DumpParseTree(pn_body, indent);
    fprintf(stderr, ")");
}

void
ListNode::dump(int indent)
{
    const char* name = parseNodeNames[getKind()];
    fprintf(stderr, "(%s [", name);
    if (pn_head != nullptr) {
        indent += strlen(name) + 3;
        DumpParseTree(pn_head, indent);
        ParseNode* pn = pn_head->pn_next;
        while (pn != nullptr) {
            IndentNewLine(indent);
            DumpParseTree(pn, indent);
            pn = pn->pn_next;
        }
    }
    fprintf(stderr, "])");
}

template <typename CharT>
static void
DumpName(const CharT* s, size_t len)
{
    if (len == 0)
        fprintf(stderr, "#<zero-length name>");

    for (size_t i = 0; i < len; i++) {
        char16_t c = s[i];
        if (c > 32 && c < 127)
            fputc(c, stderr);
        else if (c <= 255)
            fprintf(stderr, "\\x%02x", unsigned(c));
        else
            fprintf(stderr, "\\u%04x", unsigned(c));
    }
}

void
NameNode::dump(int indent)
{
    if (isKind(PNK_NAME) || isKind(PNK_DOT)) {
        if (isKind(PNK_DOT))
            fprintf(stderr, "(.");

        if (!pn_atom) {
            fprintf(stderr, "#<null name>");
        } else {
            JS::AutoCheckCannotGC nogc;
            if (pn_atom->hasLatin1Chars())
                DumpName(pn_atom->latin1Chars(nogc), pn_atom->length());
            else
                DumpName(pn_atom->twoByteChars(nogc), pn_atom->length());
        }

        if (isKind(PNK_DOT)) {
            fputc(' ', stderr);
            DumpParseTree(expr(), indent + 2);
            fputc(')', stderr);
        }
        return;
    }

    MOZ_ASSERT(!isUsed());
    const char* name = parseNodeNames[getKind()];
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

ObjectBox::ObjectBox(JSObject* object, ObjectBox* traceLink)
  : object(object),
    traceLink(traceLink),
    emitLink(nullptr)
{
    MOZ_ASSERT(!object->is<JSFunction>());
    MOZ_ASSERT(object->isTenured());
}

ObjectBox::ObjectBox(JSFunction* function, ObjectBox* traceLink)
  : object(function),
    traceLink(traceLink),
    emitLink(nullptr)
{
    MOZ_ASSERT(object->is<JSFunction>());
    MOZ_ASSERT(asFunctionBox()->function() == function);
    MOZ_ASSERT(object->isTenured());
}

FunctionBox*
ObjectBox::asFunctionBox()
{
    MOZ_ASSERT(isFunctionBox());
    return static_cast<FunctionBox*>(this);
}

void
ObjectBox::trace(JSTracer* trc)
{
    ObjectBox* box = this;
    while (box) {
        TraceRoot(trc, &box->object, "parser.object");
        if (box->isFunctionBox())
            box->asFunctionBox()->bindings.trace(trc);
        box = box->traceLink;
    }
}
