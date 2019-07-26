






#include "mozilla/FloatingPoint.h"

#include "jslibmath.h"

#include "frontend/FoldConstants.h"
#include "frontend/ParseNode.h"
#include "frontend/Parser.h"
#include "vm/NumericConversions.h"

#include "jsatominlines.h"

#include "vm/String-inl.h"

using namespace js;
using namespace js::frontend;

static ParseNode *
ContainsVarOrConst(ParseNode *pn)
{
    if (!pn)
        return NULL;
    if (pn->isKind(PNK_VAR) || pn->isKind(PNK_CONST))
        return pn;
    switch (pn->getArity()) {
      case PN_LIST:
        for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (ParseNode *pnt = ContainsVarOrConst(pn2))
                return pnt;
        }
        break;
      case PN_TERNARY:
        if (ParseNode *pnt = ContainsVarOrConst(pn->pn_kid1))
            return pnt;
        if (ParseNode *pnt = ContainsVarOrConst(pn->pn_kid2))
            return pnt;
        return ContainsVarOrConst(pn->pn_kid3);
      case PN_BINARY:
        



        if (!pn->isOp(JSOP_NOP))
            return NULL;
        if (ParseNode *pnt = ContainsVarOrConst(pn->pn_left))
            return pnt;
        return ContainsVarOrConst(pn->pn_right);
      case PN_UNARY:
        if (!pn->isOp(JSOP_NOP))
            return NULL;
        return ContainsVarOrConst(pn->pn_kid);
      case PN_NAME:
        return ContainsVarOrConst(pn->maybeExpr());
      default:;
    }
    return NULL;
}





static bool
FoldType(JSContext *cx, ParseNode *pn, ParseNodeKind kind)
{
    if (!pn->isKind(kind)) {
        switch (kind) {
          case PNK_NUMBER:
            if (pn->isKind(PNK_STRING)) {
                double d;
                if (!ToNumber(cx, StringValue(pn->pn_atom), &d))
                    return false;
                pn->pn_dval = d;
                pn->setKind(PNK_NUMBER);
                pn->setOp(JSOP_DOUBLE);
            }
            break;

          case PNK_STRING:
            if (pn->isKind(PNK_NUMBER)) {
                JSString *str = js_NumberToString<CanGC>(cx, pn->pn_dval);
                if (!str)
                    return false;
                pn->pn_atom = AtomizeString<CanGC>(cx, str);
                if (!pn->pn_atom)
                    return false;
                pn->setKind(PNK_STRING);
                pn->setOp(JSOP_STRING);
            }
            break;

          default:;
        }
    }
    return true;
}






static bool
FoldBinaryNumeric(JSContext *cx, JSOp op, ParseNode *pn1, ParseNode *pn2,
                  ParseNode *pn, Parser *parser)
{
    double d, d2;
    int32_t i, j;

    JS_ASSERT(pn1->isKind(PNK_NUMBER) && pn2->isKind(PNK_NUMBER));
    d = pn1->pn_dval;
    d2 = pn2->pn_dval;
    switch (op) {
      case JSOP_LSH:
      case JSOP_RSH:
        i = ToInt32(d);
        j = ToInt32(d2);
        j &= 31;
        d = (op == JSOP_LSH) ? i << j : i >> j;
        break;

      case JSOP_URSH:
        j = ToInt32(d2);
        j &= 31;
        d = ToUint32(d) >> j;
        break;

      case JSOP_ADD:
        d += d2;
        break;

      case JSOP_SUB:
        d -= d2;
        break;

      case JSOP_MUL:
        d *= d2;
        break;

      case JSOP_DIV:
        if (d2 == 0) {
#if defined(XP_WIN)
            
            if (MOZ_DOUBLE_IS_NaN(d2))
                d = js_NaN;
            else
#endif
            if (d == 0 || MOZ_DOUBLE_IS_NaN(d))
                d = js_NaN;
            else if (MOZ_DOUBLE_IS_NEGATIVE(d) != MOZ_DOUBLE_IS_NEGATIVE(d2))
                d = js_NegativeInfinity;
            else
                d = js_PositiveInfinity;
        } else {
            d /= d2;
        }
        break;

      case JSOP_MOD:
        if (d2 == 0) {
            d = js_NaN;
        } else {
            d = js_fmod(d, d2);
        }
        break;

      default:;
    }

    
    if (pn1 != pn)
        parser->freeTree(pn1);
    if (pn2 != pn)
        parser->freeTree(pn2);
    pn->setKind(PNK_NUMBER);
    pn->setOp(JSOP_DOUBLE);
    pn->setArity(PN_NULLARY);
    pn->pn_dval = d;
    return true;
}








void
ReplaceNode(ParseNode **pnp, ParseNode *pn)
{
    pn->pn_next = (*pnp)->pn_next;
    *pnp = pn;
}

enum Truthiness { Truthy, Falsy, Unknown };

static Truthiness
Boolish(ParseNode *pn)
{
    switch (pn->getOp()) {
      case JSOP_DOUBLE:
        return (pn->pn_dval != 0 && !MOZ_DOUBLE_IS_NaN(pn->pn_dval)) ? Truthy : Falsy;

      case JSOP_STRING:
        return (pn->pn_atom->length() > 0) ? Truthy : Falsy;

#if JS_HAS_GENERATOR_EXPRS
      case JSOP_CALL:
      {
        




        if (pn->pn_count != 1)
            return Unknown;
        ParseNode *pn2 = pn->pn_head;
        if (!pn2->isKind(PNK_FUNCTION))
            return Unknown;
        if (!(pn2->pn_funbox->inGenexpLambda))
            return Unknown;
        return Truthy;
      }
#endif

      case JSOP_DEFFUN:
      case JSOP_LAMBDA:
      case JSOP_TRUE:
        return Truthy;

      case JSOP_NULL:
      case JSOP_FALSE:
        return Falsy;

      default:
        return Unknown;
    }
}

bool
frontend::FoldConstants(JSContext *cx, ParseNode **pnp, Parser *parser, bool inGenexpLambda,
                        bool inCond)
{
    ParseNode *pn = *pnp;
    ParseNode *pn1 = NULL, *pn2 = NULL, *pn3 = NULL;

    JS_CHECK_RECURSION(cx, return false);

    switch (pn->getArity()) {
      case PN_FUNC:
        if (!FoldConstants(cx, &pn->pn_body, parser, pn->pn_funbox->inGenexpLambda))
            return false;
        break;

      case PN_LIST:
      {
        
        bool cond = inCond && (pn->isKind(PNK_OR) || pn->isKind(PNK_AND));

        
        ParseNode **listp = &pn->pn_head;
        if ((pn->isKind(PNK_CALL) || pn->isKind(PNK_NEW)) && (*listp)->isInParens())
            listp = &(*listp)->pn_next;

        for (; *listp; listp = &(*listp)->pn_next) {
            if (!FoldConstants(cx, listp, parser, inGenexpLambda, cond))
                return false;
        }

        
        pn->pn_tail = listp;

        
        pn1 = pn->pn_head;
        pn2 = NULL;
        break;
      }

      case PN_TERNARY:
        
        if (pn->pn_kid1) {
            if (!FoldConstants(cx, &pn->pn_kid1, parser, inGenexpLambda, pn->isKind(PNK_IF)))
                return false;
        }
        pn1 = pn->pn_kid1;

        if (pn->pn_kid2) {
            if (!FoldConstants(cx, &pn->pn_kid2, parser, inGenexpLambda, pn->isKind(PNK_FORHEAD)))
                return false;
            if (pn->isKind(PNK_FORHEAD) && pn->pn_kid2->isOp(JSOP_TRUE)) {
                parser->freeTree(pn->pn_kid2);
                pn->pn_kid2 = NULL;
            }
        }
        pn2 = pn->pn_kid2;

        if (pn->pn_kid3) {
            if (!FoldConstants(cx, &pn->pn_kid3, parser, inGenexpLambda))
                return false;
        }
        pn3 = pn->pn_kid3;
        break;

      case PN_BINARY:
        
        if (pn->isKind(PNK_OR) || pn->isKind(PNK_AND)) {
            if (!FoldConstants(cx, &pn->pn_left, parser, inGenexpLambda, inCond))
                return false;
            if (!FoldConstants(cx, &pn->pn_right, parser, inGenexpLambda, inCond))
                return false;
        } else {
            
            if (pn->pn_left) {
                bool isWhile = pn->isKind(PNK_WHILE);
                if (!FoldConstants(cx, &pn->pn_left, parser, inGenexpLambda, isWhile))
                    return false;
            }
            if (!FoldConstants(cx, &pn->pn_right, parser, inGenexpLambda, pn->isKind(PNK_DOWHILE)))
                return false;
        }
        pn1 = pn->pn_left;
        pn2 = pn->pn_right;
        break;

      case PN_UNARY:
        








        if (pn->isOp(JSOP_TYPEOF) && !pn->pn_kid->isKind(PNK_NAME))
            pn->setOp(JSOP_TYPEOFEXPR);

        if (pn->pn_kid) {
            if (!FoldConstants(cx, &pn->pn_kid, parser, inGenexpLambda, pn->isOp(JSOP_NOT)))
                return false;
        }
        pn1 = pn->pn_kid;
        break;

      case PN_NAME:
        





        if (!pn->isUsed()) {
            ParseNode **lhsp = &pn->pn_expr;
            while (*lhsp && (*lhsp)->isArity(PN_NAME) && !(*lhsp)->isUsed())
                lhsp = &(*lhsp)->pn_expr;
            if (*lhsp && !FoldConstants(cx, lhsp, parser, inGenexpLambda))
                return false;
            pn1 = *lhsp;
        }
        break;

      case PN_NULLARY:
        break;
    }

    switch (pn->getKind()) {
      case PNK_IF:
        if (ContainsVarOrConst(pn2) || ContainsVarOrConst(pn3))
            break;
        

      case PNK_CONDITIONAL:
        
        switch (pn1->getKind()) {
          case PNK_NUMBER:
            if (pn1->pn_dval == 0 || MOZ_DOUBLE_IS_NaN(pn1->pn_dval))
                pn2 = pn3;
            break;
          case PNK_STRING:
            if (pn1->pn_atom->length() == 0)
                pn2 = pn3;
            break;
          case PNK_TRUE:
            break;
          case PNK_FALSE:
          case PNK_NULL:
            pn2 = pn3;
            break;
          default:
            
            return true;
        }

#if JS_HAS_GENERATOR_EXPRS
        
        if (!pn2 && inGenexpLambda)
            break;
#endif

        if (pn2 && !pn2->isDefn()) {
            ReplaceNode(pnp, pn2);
            pn = pn2;
        }
        if (!pn2 || (pn->isKind(PNK_SEMI) && !pn->pn_kid)) {
            






            pn->setKind(PNK_STATEMENTLIST);
            pn->setArity(PN_LIST);
            pn->makeEmpty();
        }
        if (pn3 && pn3 != pn2)
            parser->freeTree(pn3);
        break;

      case PNK_OR:
      case PNK_AND:
        if (inCond) {
            if (pn->isArity(PN_LIST)) {
                ParseNode **listp = &pn->pn_head;
                JS_ASSERT(*listp == pn1);
                uint32_t orig = pn->pn_count;
                do {
                    Truthiness t = Boolish(pn1);
                    if (t == Unknown) {
                        listp = &pn1->pn_next;
                        continue;
                    }
                    if ((t == Truthy) == pn->isKind(PNK_OR)) {
                        for (pn2 = pn1->pn_next; pn2; pn2 = pn3) {
                            pn3 = pn2->pn_next;
                            parser->freeTree(pn2);
                            --pn->pn_count;
                        }
                        pn1->pn_next = NULL;
                        break;
                    }
                    JS_ASSERT((t == Truthy) == pn->isKind(PNK_AND));
                    if (pn->pn_count == 1)
                        break;
                    *listp = pn1->pn_next;
                    parser->freeTree(pn1);
                    --pn->pn_count;
                } while ((pn1 = *listp) != NULL);

                
                pn1 = pn->pn_head;
                if (pn->pn_count == 2) {
                    pn2 = pn1->pn_next;
                    pn1->pn_next = NULL;
                    JS_ASSERT(!pn2->pn_next);
                    pn->setArity(PN_BINARY);
                    pn->pn_left = pn1;
                    pn->pn_right = pn2;
                } else if (pn->pn_count == 1) {
                    ReplaceNode(pnp, pn1);
                    pn = pn1;
                } else if (orig != pn->pn_count) {
                    
                    pn2 = pn1->pn_next;
                    for (; pn1; pn2 = pn1, pn1 = pn1->pn_next)
                        ;
                    pn->pn_tail = &pn2->pn_next;
                }
            } else {
                Truthiness t = Boolish(pn1);
                if (t != Unknown) {
                    if ((t == Truthy) == pn->isKind(PNK_OR)) {
                        parser->freeTree(pn2);
                        ReplaceNode(pnp, pn1);
                        pn = pn1;
                    } else {
                        JS_ASSERT((t == Truthy) == pn->isKind(PNK_AND));
                        parser->freeTree(pn1);
                        ReplaceNode(pnp, pn2);
                        pn = pn2;
                    }
                }
            }
        }
        break;

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
        






        goto do_binary_op;

      case PNK_ADDASSIGN:
        JS_ASSERT(pn->isOp(JSOP_ADD));
        
      case PNK_ADD:
        if (pn->isArity(PN_LIST)) {
            




            JS_ASSERT(pn->pn_count > 2);
            if (pn->pn_xflags & PNX_CANTFOLD)
                return true;
            if (pn->pn_xflags != PNX_STRCAT)
                goto do_binary_op;

            
            size_t length = 0;
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                if (!FoldType(cx, pn2, PNK_STRING))
                    return false;
                
                if (!pn2->isKind(PNK_STRING))
                    return true;
                length += pn2->pn_atom->length();
            }

            
            jschar *chars = cx->pod_malloc<jschar>(length + 1);
            if (!chars)
                return false;
            chars[length] = 0;
            JSString *str = js_NewString<CanGC>(cx, chars, length);
            if (!str) {
                js_free(chars);
                return false;
            }

            
            for (pn2 = pn1; pn2; pn2 = parser->freeTree(pn2)) {
                JSAtom *atom = pn2->pn_atom;
                size_t length2 = atom->length();
                js_strncpy(chars, atom->chars(), length2);
                chars += length2;
            }
            JS_ASSERT(*chars == 0);

            
            pn->pn_atom = AtomizeString<CanGC>(cx, str);
            if (!pn->pn_atom)
                return false;
            pn->setKind(PNK_STRING);
            pn->setOp(JSOP_STRING);
            pn->setArity(PN_NULLARY);
            break;
        }

        
        JS_ASSERT(pn->isArity(PN_BINARY));
        if (pn1->isKind(PNK_STRING) || pn2->isKind(PNK_STRING)) {
            if (!FoldType(cx, !pn1->isKind(PNK_STRING) ? pn1 : pn2, PNK_STRING))
                return false;
            if (!pn1->isKind(PNK_STRING) || !pn2->isKind(PNK_STRING))
                return true;
            RootedString left(cx, pn1->pn_atom);
            RootedString right(cx, pn2->pn_atom);
            RootedString str(cx, ConcatStrings<CanGC>(cx, left, right));
            if (!str)
                return false;
            pn->pn_atom = AtomizeString<CanGC>(cx, str);
            if (!pn->pn_atom)
                return false;
            pn->setKind(PNK_STRING);
            pn->setOp(JSOP_STRING);
            pn->setArity(PN_NULLARY);
            parser->freeTree(pn1);
            parser->freeTree(pn2);
            break;
        }

        
        goto do_binary_op;

      case PNK_SUB:
      case PNK_STAR:
      case PNK_LSH:
      case PNK_RSH:
      case PNK_URSH:
      case PNK_DIV:
      case PNK_MOD:
      do_binary_op:
        if (pn->isArity(PN_LIST)) {
            JS_ASSERT(pn->pn_count > 2);
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                if (!FoldType(cx, pn2, PNK_NUMBER))
                    return false;
            }
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                
                if (!pn2->isKind(PNK_NUMBER))
                    break;
            }
            if (!pn2) {
                JSOp op = pn->getOp();

                pn2 = pn1->pn_next;
                pn3 = pn2->pn_next;
                if (!FoldBinaryNumeric(cx, op, pn1, pn2, pn, parser))
                    return false;
                while ((pn2 = pn3) != NULL) {
                    pn3 = pn2->pn_next;
                    if (!FoldBinaryNumeric(cx, op, pn, pn2, pn, parser))
                        return false;
                }
            }
        } else {
            JS_ASSERT(pn->isArity(PN_BINARY));
            if (!FoldType(cx, pn1, PNK_NUMBER) ||
                !FoldType(cx, pn2, PNK_NUMBER)) {
                return false;
            }
            if (pn1->isKind(PNK_NUMBER) && pn2->isKind(PNK_NUMBER)) {
                if (!FoldBinaryNumeric(cx, pn->getOp(), pn1, pn2, pn, parser))
                    return false;
            }
        }
        break;

      case PNK_TYPEOF:
      case PNK_VOID:
      case PNK_NOT:
      case PNK_BITNOT:
      case PNK_POS:
      case PNK_NEG:
        if (pn1->isKind(PNK_NUMBER)) {
            double d;

            
            d = pn1->pn_dval;
            switch (pn->getOp()) {
              case JSOP_BITNOT:
                d = ~ToInt32(d);
                break;

              case JSOP_NEG:
                d = -d;
                break;

              case JSOP_POS:
                break;

              case JSOP_NOT:
                if (d == 0 || MOZ_DOUBLE_IS_NaN(d)) {
                    pn->setKind(PNK_TRUE);
                    pn->setOp(JSOP_TRUE);
                } else {
                    pn->setKind(PNK_FALSE);
                    pn->setOp(JSOP_FALSE);
                }
                pn->setArity(PN_NULLARY);
                

              default:
                
                return true;
            }
            pn->setKind(PNK_NUMBER);
            pn->setOp(JSOP_DOUBLE);
            pn->setArity(PN_NULLARY);
            pn->pn_dval = d;
            parser->freeTree(pn1);
        } else if (pn1->isKind(PNK_TRUE) || pn1->isKind(PNK_FALSE)) {
            if (pn->isOp(JSOP_NOT)) {
                ReplaceNode(pnp, pn1);
                pn = pn1;
                if (pn->isKind(PNK_TRUE)) {
                    pn->setKind(PNK_FALSE);
                    pn->setOp(JSOP_FALSE);
                } else {
                    pn->setKind(PNK_TRUE);
                    pn->setOp(JSOP_TRUE);
                }
            }
        }
        break;

      default:;
    }

    if (inCond) {
        Truthiness t = Boolish(pn);
        if (t != Unknown) {
            





            parser->allocator.prepareNodeForMutation(pn);
            if (t == Truthy) {
                pn->setKind(PNK_TRUE);
                pn->setOp(JSOP_TRUE);
            } else {
                pn->setKind(PNK_FALSE);
                pn->setOp(JSOP_FALSE);
            }
            pn->setArity(PN_NULLARY);
        }
    }

    return true;
}
