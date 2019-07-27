





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
        MOZ_ASSERT(pn != nullptr);
        MOZ_ASSERT(pn->isKind(PNK_FUNCTION));
        MOZ_ASSERT(pn->isArity(PN_CODE));
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

    bool resolveTemplateLiteral(ParseNode* node, HandleAtom prefix) {
        MOZ_ASSERT(node->isKind(PNK_TEMPLATE_STRING_LIST));
        ParseNode* element = node->pn_head;
        while (true) {
            MOZ_ASSERT(element->isKind(PNK_TEMPLATE_STRING));

            element = element->pn_next;
            if (!element)
                return true;

            if (!resolve(element, prefix))
                return false;

            element = element->pn_next;
        }
    }

    bool resolveTaggedTemplate(ParseNode* node, HandleAtom prefix) {
        MOZ_ASSERT(node->isKind(PNK_TAGGED_TEMPLATE));

        ParseNode* element = node->pn_head;

        
        
        if (!resolve(element, prefix))
            return false;

        
        
        element = element->pn_next;
#ifdef DEBUG
        {
            MOZ_ASSERT(element->isKind(PNK_CALLSITEOBJ));
            ParseNode* array = element->pn_head;
            MOZ_ASSERT(array->isKind(PNK_ARRAY));
            for (ParseNode* kid = array->pn_head; kid; kid = kid->pn_next)
                MOZ_ASSERT(kid->isKind(PNK_TEMPLATE_STRING));
            for (ParseNode* next = array->pn_next; next; next = next->pn_next)
                MOZ_ASSERT(next->isKind(PNK_TEMPLATE_STRING));
        }
#endif

        
        ParseNode* interpolated = element->pn_next;
        for (; interpolated; interpolated = interpolated->pn_next) {
            if (!resolve(interpolated, prefix))
                return false;
        }

        return true;
    }

  public:
    explicit NameResolver(ExclusiveContext* cx) : cx(cx), nparents(0), buf(nullptr) {}

    




    bool resolve(ParseNode* cur, HandleAtom prefixArg = nullptr) {
        RootedAtom prefix(cx, prefixArg);
        if (cur == nullptr)
            return true;

        MOZ_ASSERT(cur->isKind(PNK_FUNCTION) == cur->isArity(PN_CODE));
        if (cur->isKind(PNK_FUNCTION)) {
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
          case PNK_OBJECT_PROPERTY_NAME:
          case PNK_NEWTARGET:
            MOZ_ASSERT(cur->isArity(PN_NULLARY));
            break;

          case PNK_TYPEOFNAME:
            MOZ_ASSERT(cur->isArity(PN_UNARY));
            MOZ_ASSERT(cur->pn_kid->isKind(PNK_NAME));
            MOZ_ASSERT(!cur->pn_kid->maybeExpr());
            break;

          
          case PNK_TYPEOFEXPR:
          case PNK_VOID:
          case PNK_NOT:
          case PNK_BITNOT:
          case PNK_THROW:
          case PNK_DELETENAME:
          case PNK_DELETEPROP:
          case PNK_DELETESUPERPROP:
          case PNK_DELETEELEM:
          case PNK_DELETESUPERELEM:
          case PNK_DELETEEXPR:
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
          case PNK_EXPORT_DEFAULT:
            MOZ_ASSERT(cur->isArity(PN_UNARY));
            if (!resolve(cur->pn_kid, prefix))
                return false;
            break;

          
          case PNK_SEMI:
            MOZ_ASSERT(cur->isArity(PN_UNARY));
            if (ParseNode* expr = cur->pn_kid) {
                if (!resolve(expr, prefix))
                    return false;
            }
            break;

          
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
            break;

          case PNK_WITH:
            MOZ_ASSERT(cur->isArity(PN_BINARY_OBJ));
            if (!resolve(cur->pn_left, prefix))
                return false;
            if (!resolve(cur->pn_right, prefix))
                return false;
            break;

          case PNK_DEFAULT:
            MOZ_ASSERT(cur->isArity(PN_BINARY));
            MOZ_ASSERT(!cur->pn_left);
            if (!resolve(cur->pn_right, prefix))
                return false;
            break;

          case PNK_YIELD_STAR:
            MOZ_ASSERT(cur->isArity(PN_BINARY));
            MOZ_ASSERT(cur->pn_right->isKind(PNK_NAME));
            MOZ_ASSERT(!cur->pn_right->isAssigned());
            if (!resolve(cur->pn_left, prefix))
                return false;
            break;

          case PNK_YIELD:
            MOZ_ASSERT(cur->isArity(PN_BINARY));
            if (cur->pn_left) {
                if (!resolve(cur->pn_left, prefix))
                    return false;
            }
            MOZ_ASSERT((cur->pn_right->isKind(PNK_NAME) && !cur->pn_right->isAssigned()) ||
                       (cur->pn_right->isKind(PNK_ASSIGN) &&
                        cur->pn_right->pn_left->isKind(PNK_NAME) &&
                        cur->pn_right->pn_right->isKind(PNK_GENERATOR)));
            break;

          case PNK_RETURN:
            MOZ_ASSERT(cur->isArity(PN_BINARY));
            if (ParseNode* returnValue = cur->pn_left) {
                if (!resolve(returnValue, prefix))
                    return false;
            }
#ifdef DEBUG
            if (ParseNode* internalAssignForGenerators = cur->pn_right) {
                MOZ_ASSERT(internalAssignForGenerators->isKind(PNK_NAME));
                MOZ_ASSERT(internalAssignForGenerators->pn_atom == cx->names().dotGenRVal);
                MOZ_ASSERT(internalAssignForGenerators->isAssigned());
            }
#endif
            break;

          case PNK_IMPORT:
          case PNK_EXPORT_FROM:
            MOZ_ASSERT(cur->isArity(PN_BINARY));
            
            
            
            if (!resolve(cur->pn_left, prefix))
                return false;
            MOZ_ASSERT(cur->pn_right->isKind(PNK_STRING));
            break;

          
          case PNK_CONDITIONAL:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            if (!resolve(cur->pn_kid1, prefix))
                return false;
            if (!resolve(cur->pn_kid2, prefix))
                return false;
            if (!resolve(cur->pn_kid3, prefix))
                return false;
            break;

          
          
          
          
          
          
          case PNK_FORIN:
          case PNK_FOROF:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            if (ParseNode* decl = cur->pn_kid1) {
                if (!resolve(decl, prefix))
                    return false;
            }
            if (!resolve(cur->pn_kid2, prefix))
                return false;
            if (!resolve(cur->pn_kid3, prefix))
                return false;
            break;

          
          
          case PNK_FORHEAD:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            if (ParseNode* init = cur->pn_kid1) {
                if (!resolve(init, prefix))
                    return false;
            }
            if (ParseNode* cond = cur->pn_kid2) {
                if (!resolve(cond, prefix))
                    return false;
            }
            if (ParseNode* step = cur->pn_kid3) {
                if (!resolve(step, prefix))
                    return false;
            }
            break;

          
          
          
          case PNK_CLASS:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            MOZ_ASSERT_IF(cur->pn_kid1, cur->pn_kid1->isKind(PNK_CLASSNAMES));
            MOZ_ASSERT_IF(cur->pn_kid1, cur->pn_kid1->isArity(PN_BINARY));
            MOZ_ASSERT_IF(cur->pn_kid1 && cur->pn_kid1->pn_left,
                          cur->pn_kid1->pn_left->isKind(PNK_NAME));
            MOZ_ASSERT_IF(cur->pn_kid1 && cur->pn_kid1->pn_left,
                          !cur->pn_kid1->pn_left->maybeExpr());
            MOZ_ASSERT_IF(cur->pn_kid1, cur->pn_kid1->pn_right->isKind(PNK_NAME));
            MOZ_ASSERT_IF(cur->pn_kid1, !cur->pn_kid1->pn_right->maybeExpr());
            if (cur->pn_kid2) {
                if (!resolve(cur->pn_kid2, prefix))
                    return false;
            }
            if (!resolve(cur->pn_kid3, prefix))
                return false;
            break;

          
          
          case PNK_IF:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            if (!resolve(cur->pn_kid1, prefix))
                return false;
            if (!resolve(cur->pn_kid2, prefix))
                return false;
            if (cur->pn_kid3) {
                if (!resolve(cur->pn_kid3, prefix))
                    return false;
            }
            break;

          
          
          
          case PNK_TRY:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            if (!resolve(cur->pn_kid1, prefix))
                return false;
            MOZ_ASSERT(cur->pn_kid2 || cur->pn_kid3);
            if (ParseNode* catchList = cur->pn_kid2) {
                MOZ_ASSERT(catchList->isKind(PNK_CATCHLIST));
                if (!resolve(catchList, prefix))
                    return false;
            }
            if (ParseNode* finallyBlock = cur->pn_kid3) {
                if (!resolve(finallyBlock, prefix))
                    return false;
            }
            break;

          
          
          
          
          case PNK_CATCH:
            MOZ_ASSERT(cur->isArity(PN_TERNARY));
            if (!resolve(cur->pn_kid1, prefix))
                return false;
            if (cur->pn_kid2) {
                if (!resolve(cur->pn_kid2, prefix))
                    return false;
            }
            if (!resolve(cur->pn_kid3, prefix))
                return false;
            break;

          
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
          case PNK_STATEMENTLIST:
          case PNK_ARGSBODY:
          
          
          case PNK_VAR:
          case PNK_CONST:
          case PNK_LET:
          case PNK_GLOBALCONST:
            MOZ_ASSERT(cur->isArity(PN_LIST));
            for (ParseNode* element = cur->pn_head; element; element = element->pn_next) {
                if (!resolve(element, prefix))
                    return false;
            }
            break;

          
          
          
          case PNK_ARRAYCOMP:
            MOZ_ASSERT(cur->isArity(PN_LIST));
            MOZ_ASSERT(cur->pn_count == 1);
            MOZ_ASSERT(cur->pn_head->isKind(PNK_LEXICALSCOPE) || cur->pn_head->isKind(PNK_FOR));
            if (!resolve(cur->pn_head, prefix))
                return false;
            break;

          case PNK_OBJECT:
          case PNK_CLASSMETHODLIST:
            MOZ_ASSERT(cur->isArity(PN_LIST));
            for (ParseNode* element = cur->pn_head; element; element = element->pn_next) {
                if (!resolve(element, prefix))
                    return false;
            }
            break;

          
          
          case PNK_TEMPLATE_STRING_LIST:
            MOZ_ASSERT(cur->isArity(PN_LIST));
            if (!resolveTemplateLiteral(cur, prefix))
                return false;
            break;

          case PNK_TAGGED_TEMPLATE:
            MOZ_ASSERT(cur->isArity(PN_LIST));
            if (!resolveTaggedTemplate(cur, prefix))
                return false;
            break;

          
          
          
          case PNK_IMPORT_SPEC_LIST: {
          case PNK_EXPORT_SPEC_LIST:
            MOZ_ASSERT(cur->isArity(PN_LIST));
#ifdef DEBUG
            bool isImport = cur->isKind(PNK_IMPORT_SPEC_LIST);
            ParseNode* item = cur->pn_head;
            if (!isImport && item && item->isKind(PNK_EXPORT_BATCH_SPEC)) {
                MOZ_ASSERT(item->isArity(PN_NULLARY));
                break;
            }
            for (; item; item = item->pn_next) {
                MOZ_ASSERT(item->isKind(isImport ? PNK_IMPORT_SPEC : PNK_EXPORT_SPEC));
                MOZ_ASSERT(item->isArity(PN_BINARY));
                MOZ_ASSERT(item->pn_left->isKind(PNK_NAME));
                MOZ_ASSERT(!item->pn_left->maybeExpr());
                MOZ_ASSERT(item->pn_right->isKind(PNK_NAME));
                MOZ_ASSERT(!item->pn_right->maybeExpr());
            }
#endif
            break;
          }

          case PNK_CATCHLIST: {
            MOZ_ASSERT(cur->isArity(PN_LIST));
            for (ParseNode* catchNode = cur->pn_head; catchNode; catchNode = catchNode->pn_next) {
                MOZ_ASSERT(catchNode->isKind(PNK_LEXICALSCOPE));
                MOZ_ASSERT(catchNode->expr()->isKind(PNK_CATCH));
                MOZ_ASSERT(catchNode->expr()->isArity(PN_TERNARY));
                if (!resolve(catchNode->expr(), prefix))
                    return false;
            }
            break;
          }

          case PNK_LABEL:
          case PNK_DOT:
            MOZ_ASSERT(cur->isArity(PN_NAME));
            if (!resolve(cur->expr(), prefix))
                return false;
            break;

          case PNK_LEXICALSCOPE:
          case PNK_NAME:
            MOZ_ASSERT(cur->isArity(PN_NAME));
            if (!resolve(cur->maybeExpr(), prefix))
                return false;
            break;

          case PNK_FUNCTION:
            MOZ_ASSERT(cur->isArity(PN_CODE));
            if (!resolve(cur->pn_body, prefix))
                return false;
            break;

          

          case PNK_IMPORT_SPEC: 
          case PNK_EXPORT_SPEC: 
          case PNK_CALLSITEOBJ: 
          case PNK_CLASSNAMES:  
            MOZ_CRASH("should have been handled by a parent node");

          case PNK_LIMIT: 
            MOZ_CRASH("invalid node kind");
        }

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
