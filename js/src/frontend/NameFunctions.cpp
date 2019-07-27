





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

    ExclusiveContext *cx;
    size_t nparents;                
    ParseNode *parents[MaxParents]; 
    StringBuffer *buf;              

    
    bool call(ParseNode *pn) {
        return pn && pn->isKind(PNK_CALL);
    }

    









    bool appendPropertyReference(JSAtom *name) {
        if (IsIdentifier(name))
            return buf->append('.') && buf->append(name);

        
        JSString *source = QuoteString(cx, name, '"');
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

    



    bool nameExpression(ParseNode *n) {
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

    












    ParseNode *gatherNameable(ParseNode **nameable, size_t *size) {
        *size = 0;

        for (int pos = nparents - 1; pos >= 0; pos--) {
            ParseNode *cur = parents[pos];
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

    




    bool resolveFun(ParseNode *pn, HandleAtom prefix, MutableHandleAtom retAtom) {
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

        
        ParseNode *toName[MaxParents];
        size_t size;
        ParseNode *assignment = gatherNameable(toName, &size);

        
        if (assignment) {
            if (assignment->isAssignment())
                assignment = assignment->pn_left;
            if (!nameExpression(assignment))
                return true;
        }

        




        for (int pos = size - 1; pos >= 0; pos--) {
            ParseNode *node = toName[pos];

            if (node->isKind(PNK_COLON) || node->isKind(PNK_SHORTHAND)) {
                ParseNode *left = node->pn_left;
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

    




    bool isDirectCall(int pos, ParseNode *cur) {
        return pos >= 0 && call(parents[pos]) && parents[pos]->pn_head == cur;
    }

  public:
    explicit NameResolver(ExclusiveContext *cx) : cx(cx), nparents(0), buf(nullptr) {}

    




    bool resolve(ParseNode *cur, HandleAtom prefixArg = js::NullPtr()) {
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
            for (ParseNode *nxt = cur->pn_head; nxt; nxt = nxt->pn_next)
                if (!resolve(nxt, prefix))
                    return false;
            break;
        }
        nparents--;
        return true;
    }
};

} 

bool
frontend::NameFunctions(ExclusiveContext *cx, ParseNode *pn)
{
    NameResolver nr(cx);
    return nr.resolve(pn);
}
