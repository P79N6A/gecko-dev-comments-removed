






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
        return pn && pn->isKind(PNK_LP);
    }

    



    bool special(JSAtom *atom) {
        return cx->runtime->atomState.protoAtom == atom ||
               cx->runtime->atomState.classPrototypeAtom == atom;
    }

    



    bool nameExpression(ParseNode *n) {
        switch (n->getKind()) {
            case PNK_DOT:
                return nameExpression(n->expr()) &&
                       (special(n->pn_atom) ||
                        (buf->append(".") && buf->append(n->pn_atom)));

            case PNK_NAME:
                return buf->append(n->pn_atom);

            case PNK_LB:
                return nameExpression(n->pn_left) &&
                       buf->append("[") &&
                       nameExpression(n->pn_right) &&
                       buf->append("]");

            case PNK_NUMBER: {
                char number[30];
                int digits = JS_snprintf(number, sizeof(number), "%g", n->pn_dval);
                return buf->appendInflated(number, digits);
            }

            




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
                default:           break;       

                
                case PNK_COLON:
                case PNK_LP:
                case PNK_NEW:
                    JS_ASSERT(*size < MaxParents);
                    nameable[(*size)++] = cur;
                    break;
            }

            











            if (cur->isKind(PNK_RETURN)) {
                for (int tmp = pos - 1; tmp > 0; tmp--) {
                    if (isDirectCall(tmp, cur)) {
                        pos = tmp;
                        break;
                    } else if (call(cur)) {
                        
                        break;
                    }
                    cur = parents[tmp];
                }
            }
        }

        return NULL;
    }

    




    JSAtom *resolveFun(ParseNode *pn, JSAtom *prefix) {
        JS_ASSERT(pn != NULL && pn->isKind(PNK_FUNCTION));
        JSFunction *fun = pn->pn_funbox->function();
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
                if (!node->pn_left->isKind(PNK_NAME))
                    continue;
                
                if (!special(node->pn_left->pn_atom)) {
                    if (!buf.append(".") || !buf.append(node->pn_left->pn_atom))
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

        fun->setGuessedAtom(buf.finishAtom());
        return fun->displayAtom();
    }

    




    bool isDirectCall(int pos, ParseNode *cur) {
        return pos >= 0 && call(parents[pos]) && parents[pos]->pn_head == cur;
    }

  public:
    NameResolver(JSContext *cx) : cx(cx), nparents(0), buf(NULL) {}

    




    void resolve(ParseNode *cur, JSAtom *prefix = NULL) {
        if (cur == NULL)
            return;

        if (cur->isKind(PNK_FUNCTION) && cur->isArity(PN_FUNC)) {
            JSAtom *prefix2 = resolveFun(cur, prefix);
            





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
