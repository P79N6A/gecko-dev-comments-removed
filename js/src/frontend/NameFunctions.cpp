





#include "frontend/NameFunctions.h"

#include "jsfun.h"
#include "jsprf.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/ParseNode.h"
#include "frontend/SharedContext.h"
#include "vm/StringBuffer.h"

using namespace js;
using namespace js::frontend;

namespace {

class NameResolver
{
    static const size_t MaxParents = 100;

    ExclusiveContext* cx;
    size_t nparents;                
    ParseNode* parents[MaxParents]; 
    StringBuffer* buf;              

    
    bool call(ParseNode* pn) {
        return pn && pn->isKind(PNK_CALL);
    }

    









    bool appendPropertyReference(JSAtom* name) {
        if (IsIdentifier(name))
            return buf->append('.') && buf->append(name);

        
        JSString* source = QuoteString(cx, name, '"');
        return source && buf->append('[') && buf->append(source) && buf->append(']');
    }

    
    bool appendNumber(double n) {
        char number[30];
        int digits = JS_snprintf(number, sizeof(number), "%g", n);
        return buf->append(number, digits);
    }

    
    bool appendNumericPropertyReference(double n) {
        return buf->append("[") && appendNumber(n) && buf->append(']');
    }

    



    bool nameExpression(ParseNode* n) {
        switch (n->getKind()) {
          case PNK_DOT:
            return nameExpression(n->expr()) && appendPropertyReference(n->pn_atom);

          case PNK_NAME:
            return buf->append(n->pn_atom);

          case PNK_THIS:
            return buf->append("this");

          case PNK_ELEM:
            return nameExpression(n->pn_left) &&
                   buf->append('[') &&
                   nameExpression(n->pn_right) &&
                   buf->append(']');

          case PNK_NUMBER:
            return appendNumber(n->pn_dval);

          default:
            




            return false;
        }
    }

    












    ParseNode* gatherNameable(ParseNode** nameable, size_t* size) {
        *size = 0;

        for (int pos = nparents - 1; pos >= 0; pos--) {
            ParseNode* cur = parents[pos];
            if (cur->isAssignment())
                return cur;

            switch (cur->getKind()) {
              case PNK_NAME:     return cur;  
              case PNK_THIS:     return cur;  
              case PNK_FUNCTION: return nullptr; 

              case PNK_RETURN:
                











                for (int tmp = pos - 1; tmp > 0; tmp--) {
                    if (isDirectCall(tmp, cur)) {
                        pos = tmp;
                        break;
                    } else if (call(cur)) {
                        
                        break;
                    }
                    cur = parents[tmp];
                }
                break;

              case PNK_COLON:
              case PNK_SHORTHAND:
                



                pos--;
                

              default:
                
                MOZ_ASSERT(*size < MaxParents);
                nameable[(*size)++] = cur;
                break;
            }
        }

        return nullptr;
    }

    




    bool resolveFun(ParseNode* pn, HandleAtom prefix, MutableHandleAtom retAtom) {
        MOZ_ASSERT(pn != nullptr && pn->isKind(PNK_FUNCTION));
        RootedFunction fun(cx, pn->pn_funbox->function());

        StringBuffer buf(cx);
        this->buf = &buf;

        retAtom.set(nullptr);

        
        if (fun->displayAtom() != nullptr) {
            if (prefix == nullptr) {
                retAtom.set(fun->displayAtom());
                return true;
            }
            if (!buf.append(prefix) ||
                !buf.append('/') ||
                !buf.append(fun->displayAtom()))
                return false;
            retAtom.set(buf.finishAtom());
            return !!retAtom;
        }

        
        if (prefix != nullptr && (!buf.append(prefix) || !buf.append('/')))
            return false;

        
        ParseNode* toName[MaxParents];
        size_t size;
        ParseNode* assignment = gatherNameable(toName, &size);

        
        if (assignment) {
            if (assignment->isAssignment())
                assignment = assignment->pn_left;
            if (!nameExpression(assignment))
                return true;
        }

        




        for (int pos = size - 1; pos >= 0; pos--) {
            ParseNode* node = toName[pos];

            if (node->isKind(PNK_COLON) || node->isKind(PNK_SHORTHAND)) {
                ParseNode* left = node->pn_left;
                if (left->isKind(PNK_OBJECT_PROPERTY_NAME) || left->isKind(PNK_STRING)) {
                    if (!appendPropertyReference(left->pn_atom))
                        return false;
                } else if (left->isKind(PNK_NUMBER)) {
                    if (!appendNumericPropertyReference(left->pn_dval))
                        return false;
                } else {
                    MOZ_ASSERT(left->isKind(PNK_COMPUTED_NAME));
                }
            } else {
                



                if (!buf.empty() && buf.getChar(buf.length() - 1) != '<' && !buf.append('<'))
                    return false;
            }
        }

        




        if (!buf.empty() && buf.getChar(buf.length() - 1) == '/' && !buf.append('<'))
            return false;

        if (buf.empty())
            return true;

        retAtom.set(buf.finishAtom());
        if (!retAtom)
            return false;
        fun->setGuessedAtom(retAtom);
        return true;
    }

    




    bool isDirectCall(int pos, ParseNode* cur) {
        return pos >= 0 && call(parents[pos]) && parents[pos]->pn_head == cur;
    }

  public:
    explicit NameResolver(ExclusiveContext* cx) : cx(cx), nparents(0), buf(nullptr) {}

    




    bool resolve(ParseNode* cur, HandleAtom prefixArg = nullptr) {
        RootedAtom prefix(cx, prefixArg);
        if (cur == nullptr)
            return true;

        if (cur->isKind(PNK_FUNCTION) && cur->isArity(PN_CODE)) {
            RootedAtom prefix2(cx);
            if (!resolveFun(cur, prefix, &prefix2))
                return false;

            





            if (!isDirectCall(nparents - 1, cur))
                prefix = prefix2;
        }
        if (nparents >= MaxParents)
            return true;
        parents[nparents++] = cur;

        switch (cur->getKind()) {
          
          
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
          case PNK_FRESHENBLOCK:
          case PNK_SUPERPROP:
            MOZ_ASSERT(cur->isArity(PN_NULLARY));
            goto done;

          
          case PNK_TYPEOF:
          case PNK_VOID:
          case PNK_NOT:
          case PNK_BITNOT:
          case PNK_THROW:
          case PNK_DELETE:
          case PNK_NEG:
          case PNK_POS:
          case PNK_PREINCREMENT:
          case PNK_POSTINCREMENT:
          case PNK_PREDECREMENT:
          case PNK_POSTDECREMENT:
          case PNK_COMPUTED_NAME:
          case PNK_ARRAYPUSH:
          case PNK_SPREAD:
          case PNK_MUTATEPROTO:
          case PNK_SUPERELEM:
          case PNK_EXPORT:
            MOZ_ASSERT(cur->isArity(PN_UNARY));
            if (!resolve(cur->pn_kid, prefix))
                return false;
            goto done;

          
          case PNK_SEMI:
            MOZ_ASSERT(cur->isArity(PN_UNARY));
            if (ParseNode* expr = cur->pn_kid) {
                if (!resolve(expr, prefix))
                    return false;
            }
            goto done;

          
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
          case PNK_IMPORT_SPEC:
          case PNK_EXPORT_SPEC:
          case PNK_ELEM:
          case PNK_LETEXPR:
          case PNK_COLON:
          case PNK_CASE:
          case PNK_SHORTHAND:
          case PNK_DOWHILE:
          case PNK_WHILE:
          case PNK_SWITCH:
          case PNK_LETBLOCK:
          case PNK_FOR:
          case PNK_CLASSMETHOD:
            MOZ_ASSERT(cur->isArity(PN_BINARY));
            if (!resolve(cur->pn_left, prefix))
                return false;
            if (!resolve(cur->pn_right, prefix))
                return false;
            goto done;

          case PNK_WITH:
            MOZ_ASSERT(cur->isArity(PN_BINARY_OBJ));
            if (!resolve(cur->pn_left, prefix))
                return false;
            if (!resolve(cur->pn_right, prefix))
                return false;
            goto done;

          case PNK_CLASSNAMES:
          case PNK_OBJECT_PROPERTY_NAME:
          case PNK_CLASS:
          case PNK_CLASSMETHODLIST:
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
          case PNK_LET:
          case PNK_GLOBALCONST:
          case PNK_CATCHLIST:
          case PNK_STATEMENTLIST:
          case PNK_IMPORT_SPEC_LIST:
          case PNK_EXPORT_SPEC_LIST:
          case PNK_SEQ:
          case PNK_ARGSBODY:
          case PNK_DEFAULT:
          case PNK_FORHEAD:
          case PNK_CONDITIONAL:
          case PNK_FORIN:
          case PNK_FOROF:
          case PNK_IF:
          case PNK_TRY:
          case PNK_CATCH:
          case PNK_YIELD_STAR:
          case PNK_YIELD:
          case PNK_RETURN:
          case PNK_LABEL:
          case PNK_DOT:
          case PNK_LEXICALSCOPE:
          case PNK_ARRAYCOMP:
          case PNK_IMPORT:
          case PNK_EXPORT_FROM:
          case PNK_NAME:
            break; 

          case PNK_FUNCTION:
            MOZ_ASSERT(cur->isArity(PN_CODE));
            if (!resolve(cur->pn_body, prefix))
                return false;
            goto done;

          case PNK_LIMIT: 
            MOZ_CRASH("invalid node kind");
        }

        switch (cur->getArity()) {
          case PN_NULLARY:
            break;
          case PN_NAME:
            if (!resolve(cur->maybeExpr(), prefix))
                return false;
            break;
          case PN_UNARY:
            if (!resolve(cur->pn_kid, prefix))
                return false;
            break;
          case PN_BINARY:
          case PN_BINARY_OBJ:
            if (!resolve(cur->pn_left, prefix))
                return false;

            





            if (cur->pn_left != cur->pn_right)
                if (!resolve(cur->pn_right, prefix))
                    return false;
            break;
          case PN_TERNARY:
            if (!resolve(cur->pn_kid1, prefix))
                return false;
            if (!resolve(cur->pn_kid2, prefix))
                return false;
            if (!resolve(cur->pn_kid3, prefix))
                return false;
            break;
          case PN_CODE:
            MOZ_ASSERT(cur->isKind(PNK_FUNCTION));
            if (!resolve(cur->pn_body, prefix))
                return false;
            break;
          case PN_LIST:
            for (ParseNode* nxt = cur->pn_head; nxt; nxt = nxt->pn_next)
                if (!resolve(nxt, prefix))
                    return false;
            break;
        }

      done:
        nparents--;
        return true;
    }
};

} 

bool
frontend::NameFunctions(ExclusiveContext* cx, ParseNode* pn)
{
    NameResolver nr(cx);
    return nr.resolve(pn);
}
