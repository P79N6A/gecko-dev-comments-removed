






#include "frontend/NameFunctions.h"
#include "frontend/ParseNode.h"
#include "frontend/SharedContext.h"

#include "jsfun.h"
#include "jsprf.h"

#include "vm/String-inl.h"
#include "vm/StringBuffer.h"

using namespace js;
using namespace js::frontend;

class NameResolver
{
    static const size_t MaxParents = 100;

    JSContext *cx;
    size_t nparents;                
    ParseNode *parents[MaxParents]; 
    StringBuffer *buf;              

    
    bool call(ParseNode *pn) {
        return pn && pn->isKind(PNK_CALL);
    }

    









    bool appendPropertyReference(JSAtom *name) {
        if (IsIdentifier(name))
            return buf->append(".") && buf->append(name);

        
        JSString *source = js_QuoteString(cx, name, '"');
        return source && buf->append("[") && buf->append(source) && buf->append("]");
    }

    
    bool appendNumber(double n) {
        char number[30];
        int digits = JS_snprintf(number, sizeof(number), "%g", n);
        return buf->appendInflated(number, digits);
    }

    
    bool appendNumericPropertyReference(double n) {
        return buf->append("[") && appendNumber(n) && buf->append("]");
    }

    



    bool nameExpression(ParseNode *n) {
        switch (n->getKind()) {
          case PNK_DOT:
            return nameExpression(n->expr()) && appendPropertyReference(n->pn_atom);

          case PNK_NAME:
            return buf->append(n->pn_atom);

          case PNK_ELEM:
            return nameExpression(n->pn_left) &&
                   buf->append("[") &&
                   nameExpression(n->pn_right) &&
                   buf->append("]");

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
              case PNK_FUNCTION: return NULL; 

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
                





                if (pos == 0 || !parents[pos - 1]->isKind(PNK_OBJECT))
                    return NULL;
                pos--;
                

              default:
                
                JS_ASSERT(*size < MaxParents);
                nameable[(*size)++] = cur;
                break;
            }
        }

        return NULL;
    }

    




    JSAtom *resolveFun(ParseNode *pn, HandleAtom prefix) {
        JS_ASSERT(pn != NULL && pn->isKind(PNK_FUNCTION));
        RootedFunction fun(cx, pn->pn_funbox->function());
        if (nparents == 0)
            return NULL;

        StringBuffer buf(cx);
        this->buf = &buf;

        
        if (fun->displayAtom() != NULL) {
            if (prefix == NULL)
                return fun->atom();
            if (!buf.append(prefix) ||
                !buf.append("/") ||
                !buf.append(fun->displayAtom()))
                return NULL;
            return buf.finishAtom();
        }

        
        if (prefix != NULL && (!buf.append(prefix) || !buf.append("/")))
            return NULL;

        
        ParseNode *toName[MaxParents];
        size_t size;
        ParseNode *assignment = gatherNameable(toName, &size);

        
        if (assignment) {
            if (assignment->isAssignment())
                assignment = assignment->pn_left;
            if (!nameExpression(assignment))
                return NULL;
        }

        




        for (int pos = size - 1; pos >= 0; pos--) {
            ParseNode *node = toName[pos];

            if (node->isKind(PNK_COLON)) {
                ParseNode *left = node->pn_left;
                if (left->isKind(PNK_NAME) || left->isKind(PNK_STRING)) {
                    if (!appendPropertyReference(left->pn_atom))
                        return NULL;
                } else if (left->isKind(PNK_NUMBER)) {
                    if (!appendNumericPropertyReference(left->pn_dval))
                        return NULL;
                }
            } else {
                



                if (!buf.empty() && *(buf.end() - 1) != '<' && !buf.append("<"))
                    return NULL;
            }
        }

        




        if (!buf.empty() && *(buf.end() - 1) == '/' && !buf.append("<"))
            return NULL;
        if (buf.empty())
            return NULL;

        UnrootedAtom atom = buf.finishAtom();
        fun->setGuessedAtom(atom);
        return atom;
    }

    




    bool isDirectCall(int pos, ParseNode *cur) {
        return pos >= 0 && call(parents[pos]) && parents[pos]->pn_head == cur;
    }

  public:
    explicit NameResolver(JSContext *cx) : cx(cx), nparents(0), buf(NULL) {}

    




    void resolve(ParseNode *cur, HandleAtom prefixArg = NullPtr()) {
        RootedAtom prefix(cx, prefixArg);
        if (cur == NULL)
            return;

        if (cur->isKind(PNK_FUNCTION) && cur->isArity(PN_FUNC)) {
            RootedAtom prefix2(cx, resolveFun(cur, prefix));
            





            if (!isDirectCall(nparents - 1, cur))
                prefix = prefix2;
        }
        if (nparents >= MaxParents)
            return;
        parents[nparents++] = cur;

        switch (cur->getArity()) {
          case PN_NULLARY:
            break;
          case PN_NAME:
            resolve(cur->maybeExpr(), prefix);
            break;
          case PN_UNARY:
            resolve(cur->pn_kid, prefix);
            break;
          case PN_BINARY:
            resolve(cur->pn_left, prefix);

            





            if (cur->pn_left != cur->pn_right)
                resolve(cur->pn_right, prefix);
            break;
          case PN_TERNARY:
            resolve(cur->pn_kid1, prefix);
            resolve(cur->pn_kid2, prefix);
            resolve(cur->pn_kid3, prefix);
            break;
          case PN_FUNC:
            JS_ASSERT(cur->isKind(PNK_FUNCTION));
            resolve(cur->pn_body, prefix);
            break;
          case PN_LIST:
            for (ParseNode *nxt = cur->pn_head; nxt; nxt = nxt->pn_next)
                resolve(nxt, prefix);
            break;
        }
        nparents--;
    }
};

bool
frontend::NameFunctions(JSContext *cx, ParseNode *pn)
{
    NameResolver nr(cx);
    nr.resolve(pn);
    return true;
}
