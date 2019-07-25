





















































#include "frontend/Parser.h"

#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/BytecodeGenerator.h"
#include "frontend/FoldConstants.h"
#include "frontend/ParseMaps.h"
#include "frontend/TokenStream.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#if JS_HAS_DESTRUCTURING
#include "jsdhash.h"
#endif

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "vm/RegExpObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::frontend;





#define MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, __flags)                                     \
    JS_BEGIN_MACRO                                                                          \
        if (tokenStream.getToken((__flags)) != tt) {                                        \
            reportErrorNumber(NULL, JSREPORT_ERROR, errno);                                 \
            return NULL;                                                                    \
        }                                                                                   \
    JS_END_MACRO
#define MUST_MATCH_TOKEN(tt, errno) MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, 0)

Parser::Parser(JSContext *cx, JSPrincipals *prin, StackFrame *cfp, bool foldConstants)
  : js::AutoGCRooter(cx),
    context(cx),
    tokenStream(cx),
    principals(NULL),
    callerFrame(cfp),
    callerVarObj(cfp ? &cfp->varObj() : NULL),
    allocator(cx),
    functionCount(0),
    traceListHead(NULL),
    tc(NULL),
    keepAtoms(cx->runtime),
    foldConstants(foldConstants)
{
    cx->activeCompilations++;
    PodArrayZero(tempFreeList);
    setPrincipals(prin);
    JS_ASSERT_IF(cfp, cfp->isScriptFrame());
}

bool
Parser::init(const jschar *base, size_t length, const char *filename, uintN lineno,
             JSVersion version)
{
    JSContext *cx = context;
    if (!cx->ensureParseMapPool())
        return false;
    tempPoolMark = cx->tempLifoAlloc().mark();
    if (!tokenStream.init(base, length, filename, lineno, version)) {
        cx->tempLifoAlloc().release(tempPoolMark);
        return false;
    }
    return true;
}

Parser::~Parser()
{
    JSContext *cx = context;

    if (principals)
        JSPRINCIPALS_DROP(cx, principals);
    cx->tempLifoAlloc().release(tempPoolMark);
    cx->activeCompilations--;
}

void
Parser::setPrincipals(JSPrincipals *prin)
{
    JS_ASSERT(!principals);
    if (prin)
        JSPRINCIPALS_HOLD(context, prin);
    principals = prin;
}

ObjectBox *
Parser::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj);

    






    ObjectBox *objbox = context->tempLifoAlloc().new_<ObjectBox>();
    if (!objbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }
    objbox->traceLink = traceListHead;
    traceListHead = objbox;
    objbox->emitLink = NULL;
    objbox->object = obj;
    objbox->isFunctionBox = false;
    return objbox;
}

FunctionBox *
Parser::newFunctionBox(JSObject *obj, ParseNode *fn, TreeContext *tc)
{
    JS_ASSERT(obj);
    JS_ASSERT(obj->isFunction());

    






    FunctionBox *funbox = context->tempLifoAlloc().newPod<FunctionBox>();
    if (!funbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }
    funbox->traceLink = traceListHead;
    traceListHead = funbox;
    funbox->emitLink = NULL;
    funbox->object = obj;
    funbox->isFunctionBox = true;
    funbox->node = fn;
    funbox->siblings = tc->functionList;
    tc->functionList = funbox;
    ++tc->parser->functionCount;
    funbox->kids = NULL;
    funbox->parent = tc->funbox;
    funbox->methods = NULL;
    new (&funbox->bindings) Bindings(context);
    funbox->queued = false;
    funbox->inLoop = false;
    for (StmtInfo *stmt = tc->topStmt; stmt; stmt = stmt->down) {
        if (STMT_IS_LOOP(stmt)) {
            funbox->inLoop = true;
            break;
        }
    }
    funbox->level = tc->staticLevel;
    funbox->tcflags = (TCF_IN_FUNCTION | (tc->flags & (TCF_COMPILE_N_GO | TCF_STRICT_MODE_CODE)));
    if (tc->innermostWith)
        funbox->tcflags |= TCF_IN_WITH;
    return funbox;
}

void
Parser::trace(JSTracer *trc)
{
    ObjectBox *objbox = traceListHead;
    while (objbox) {
        MarkObject(trc, *objbox->object, "parser.object");
        if (objbox->isFunctionBox)
            static_cast<FunctionBox *>(objbox)->bindings.trace(trc);
        objbox = objbox->traceLink;
    }

    for (TreeContext *tc = this->tc; tc; tc = tc->parent)
        tc->trace(trc);
}






























void
Parser::cleanFunctionList(FunctionBox **funboxHead)
{
    FunctionBox **link = funboxHead;
    while (FunctionBox *box = *link) {
        if (!box->node) {
            



            *link = box->siblings;
        } else if (!box->node->pn_funbox) {
            



            *link = box->siblings;
            allocator.freeNode(box->node);
        } else {
            

            
            {
                ParseNode **methodLink = &box->methods;
                while (ParseNode *method = *methodLink) {
                    
                    JS_ASSERT(method->isArity(PN_FUNC));
                    if (!method->pn_funbox) {
                        
                        *methodLink = method->pn_link;
                    } else {
                        
                        methodLink = &method->pn_link;
                    }
                }
            }

            
            cleanFunctionList(&box->kids);

            
            link = &box->siblings;
        }
    }
}

static bool
GenerateBlockIdForStmtNode(ParseNode *pn, TreeContext *tc)
{
    JS_ASSERT(tc->topStmt);
    JS_ASSERT(STMT_MAYBE_SCOPE(tc->topStmt));
    JS_ASSERT(pn->isKind(TOK_LC) || pn->isKind(TOK_LEXICALSCOPE));
    if (!GenerateBlockId(tc, tc->topStmt->blockid))
        return false;
    pn->pn_blockid = tc->topStmt->blockid;
    return true;
}




ParseNode *
Parser::parse(JSObject *chain)
{
    







    TreeContext globaltc(this);
    if (!globaltc.init(context))
        return NULL;
    globaltc.setScopeChain(chain);
    if (!GenerateBlockId(&globaltc, globaltc.bodyid))
        return NULL;

    ParseNode *pn = statements();
    if (pn) {
        if (!tokenStream.matchToken(TOK_EOF)) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (foldConstants) {
            if (!FoldConstants(context, pn, &globaltc))
                pn = NULL;
        }
    }
    return pn;
}

JS_STATIC_ASSERT(UpvarCookie::FREE_LEVEL == JS_BITMASK(JSFB_LEVEL_BITS));







#define ENDS_IN_OTHER   0
#define ENDS_IN_RETURN  1
#define ENDS_IN_BREAK   2

static int
HasFinalReturn(ParseNode *pn)
{
    ParseNode *pn2, *pn3;
    uintN rv, rv2, hasDefault;

    switch (pn->getKind()) {
      case TOK_LC:
        if (!pn->pn_head)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->last());

      case TOK_IF:
        if (!pn->pn_kid3)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->pn_kid2) & HasFinalReturn(pn->pn_kid3);

      case TOK_WHILE:
        pn2 = pn->pn_left;
        if (pn2->isKind(TOK_PRIMARY) && pn2->isOp(JSOP_TRUE))
            return ENDS_IN_RETURN;
        if (pn2->isKind(TOK_NUMBER) && pn2->pn_dval)
            return ENDS_IN_RETURN;
        return ENDS_IN_OTHER;

      case TOK_DO:
        pn2 = pn->pn_right;
        if (pn2->isKind(TOK_PRIMARY)) {
            if (pn2->isOp(JSOP_FALSE))
                return HasFinalReturn(pn->pn_left);
            if (pn2->isOp(JSOP_TRUE))
                return ENDS_IN_RETURN;
        }
        if (pn2->isKind(TOK_NUMBER)) {
            if (pn2->pn_dval == 0)
                return HasFinalReturn(pn->pn_left);
            return ENDS_IN_RETURN;
        }
        return ENDS_IN_OTHER;

      case TOK_FOR:
        pn2 = pn->pn_left;
        if (pn2->isArity(PN_TERNARY) && !pn2->pn_kid2)
            return ENDS_IN_RETURN;
        return ENDS_IN_OTHER;

      case TOK_SWITCH:
        rv = ENDS_IN_RETURN;
        hasDefault = ENDS_IN_OTHER;
        pn2 = pn->pn_right;
        if (pn2->isKind(TOK_LEXICALSCOPE))
            pn2 = pn2->expr();
        for (pn2 = pn2->pn_head; rv && pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(TOK_DEFAULT))
                hasDefault = ENDS_IN_RETURN;
            pn3 = pn2->pn_right;
            JS_ASSERT(pn3->isKind(TOK_LC));
            if (pn3->pn_head) {
                rv2 = HasFinalReturn(pn3->last());
                if (rv2 == ENDS_IN_OTHER && pn2->pn_next)
                    ;
                else
                    rv &= rv2;
            }
        }
        
        rv &= hasDefault;
        return rv;

      case TOK_BREAK:
        return ENDS_IN_BREAK;

      case TOK_WITH:
        return HasFinalReturn(pn->pn_right);

      case TOK_RETURN:
        return ENDS_IN_RETURN;

      case TOK_COLON:
      case TOK_LEXICALSCOPE:
        return HasFinalReturn(pn->expr());

      case TOK_THROW:
        return ENDS_IN_RETURN;

      case TOK_TRY:
        
        if (pn->pn_kid3) {
            rv = HasFinalReturn(pn->pn_kid3);
            if (rv == ENDS_IN_RETURN)
                return rv;
        }

        
        rv = HasFinalReturn(pn->pn_kid1);
        if (pn->pn_kid2) {
            JS_ASSERT(pn->pn_kid2->isArity(PN_LIST));
            for (pn2 = pn->pn_kid2->pn_head; pn2; pn2 = pn2->pn_next)
                rv &= HasFinalReturn(pn2);
        }
        return rv;

      case TOK_CATCH:
        
        return HasFinalReturn(pn->pn_kid3);

      case TOK_LET:
        
        if (!pn->isArity(PN_BINARY))
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->pn_right);

      default:
        return ENDS_IN_OTHER;
    }
}

static JSBool
ReportBadReturn(JSContext *cx, TreeContext *tc, ParseNode *pn, uintN flags, uintN errnum,
                uintN anonerrnum)
{
    JSAutoByteString name;
    if (tc->fun()->atom) {
        if (!js_AtomToPrintableString(cx, tc->fun()->atom, &name))
            return false;
    } else {
        errnum = anonerrnum;
    }
    return ReportCompileErrorNumber(cx, TS(tc->parser), pn, flags, errnum, name.ptr());
}

static JSBool
CheckFinalReturn(JSContext *cx, TreeContext *tc, ParseNode *pn)
{
    JS_ASSERT(tc->inFunction());
    return HasFinalReturn(pn) == ENDS_IN_RETURN ||
           ReportBadReturn(cx, tc, pn, JSREPORT_WARNING | JSREPORT_STRICT,
                           JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE);
}





static bool
CheckStrictAssignment(JSContext *cx, TreeContext *tc, ParseNode *lhs)
{
    if (tc->needStrictChecks() && lhs->isKind(TOK_NAME)) {
        JSAtom *atom = lhs->pn_atom;
        JSAtomState *atomState = &cx->runtime->atomState;
        if (atom == atomState->evalAtom || atom == atomState->argumentsAtom) {
            JSAutoByteString name;
            if (!js_AtomToPrintableString(cx, atom, &name) ||
                !ReportStrictModeError(cx, TS(tc->parser), tc, lhs, JSMSG_DEPRECATED_ASSIGN,
                                       name.ptr())) {
                return false;
            }
        }
    }
    return true;
}







bool
CheckStrictBinding(JSContext *cx, TreeContext *tc, PropertyName *name, ParseNode *pn)
{
    if (!tc->needStrictChecks())
        return true;

    JSAtomState *atomState = &cx->runtime->atomState;
    if (name == atomState->evalAtom ||
        name == atomState->argumentsAtom ||
        FindKeyword(name->charsZ(), name->length()))
    {
        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, name, &bytes))
            return false;
        return ReportStrictModeError(cx, TS(tc->parser), tc, pn, JSMSG_BAD_BINDING, bytes.ptr());
    }

    return true;
}

static bool
ReportBadParameter(JSContext *cx, TreeContext *tc, JSAtom *name, uintN errorNumber)
{
    Definition *dn = tc->decls.lookupFirst(name);
    JSAutoByteString bytes;
    return js_AtomToPrintableString(cx, name, &bytes) &&
           ReportStrictModeError(cx, TS(tc->parser), tc, dn, errorNumber, bytes.ptr());
}

namespace js {







bool
CheckStrictParameters(JSContext *cx, TreeContext *tc)
{
    JS_ASSERT(tc->inFunction());

    if (!tc->needStrictChecks() || tc->bindings.countArgs() == 0)
        return true;

    JSAtom *argumentsAtom = cx->runtime->atomState.argumentsAtom;
    JSAtom *evalAtom = cx->runtime->atomState.evalAtom;

    
    HashMap<JSAtom *, bool> parameters(cx);
    if (!parameters.init(tc->bindings.countArgs()))
        return false;

    
    for (Shape::Range r = tc->bindings.lastVariable(); !r.empty(); r.popFront()) {
        jsid id = r.front().propid;
        if (!JSID_IS_ATOM(id))
            continue;

        JSAtom *name = JSID_TO_ATOM(id);

        if (name == argumentsAtom || name == evalAtom) {
            if (!ReportBadParameter(cx, tc, name, JSMSG_BAD_BINDING))
                return false;
        }

        if (tc->inStrictMode() && FindKeyword(name->charsZ(), name->length())) {
            



            JS_ALWAYS_TRUE(!ReportBadParameter(cx, tc, name, JSMSG_RESERVED_ID));
            return false;
        }

        



        if (HashMap<JSAtom *, bool>::AddPtr p = parameters.lookupForAdd(name)) {
            if (!p->value && !ReportBadParameter(cx, tc, name, JSMSG_DUPLICATE_FORMAL))
                return false;
            p->value = true;
        } else {
            if (!parameters.add(p, name, false))
                return false;
        }
    }

    return true;
}

} 

ParseNode *
Parser::functionBody()
{
    JS_ASSERT(tc->inFunction());

    StmtInfo stmtInfo;
    PushStatement(tc, &stmtInfo, STMT_BLOCK, -1);
    stmtInfo.flags = SIF_BODY_BLOCK;

    uintN oldflags = tc->flags;
    tc->flags &= ~(TCF_RETURN_EXPR | TCF_RETURN_VOID);

    ParseNode *pn;
#if JS_HAS_EXPR_CLOSURES
    if (tokenStream.currentToken().type == TOK_LC) {
        pn = statements();
    } else {
        pn = UnaryNode::create(tc);
        if (pn) {
            pn->pn_kid = assignExpr();
            if (!pn->pn_kid) {
                pn = NULL;
            } else {
                if (tc->flags & TCF_FUN_IS_GENERATOR) {
                    ReportBadReturn(context, tc, pn, JSREPORT_ERROR,
                                    JSMSG_BAD_GENERATOR_RETURN,
                                    JSMSG_BAD_ANON_GENERATOR_RETURN);
                    pn = NULL;
                } else {
                    pn->setKind(TOK_RETURN);
                    pn->setOp(JSOP_RETURN);
                    pn->pn_pos.end = pn->pn_kid->pn_pos.end;
                }
            }
        }
    }
#else
    pn = statements();
#endif

    if (pn) {
        JS_ASSERT(!(tc->topStmt->flags & SIF_SCOPE));
        PopStatementTC(tc);

        
        if (context->hasStrictOption() && (tc->flags & TCF_RETURN_EXPR) &&
            !CheckFinalReturn(context, tc, pn)) {
            pn = NULL;
        }
    }

    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}


static Definition *
MakePlaceholder(ParseNode *pn, TreeContext *tc)
{
    Definition *dn = (Definition *) NameNode::create(pn->pn_atom, tc);
    if (!dn)
        return NULL;

    dn->setKind(TOK_NAME);
    dn->setOp(JSOP_NOP);
    dn->setDefn(true);
    dn->pn_dflags |= PND_PLACEHOLDER;
    return dn;
}

static bool
Define(ParseNode *pn, JSAtom *atom, TreeContext *tc, bool let = false)
{
    JS_ASSERT(!pn->isUsed());
    JS_ASSERT_IF(pn->isDefn(), pn->isPlaceholder());

    bool foundLexdep = false;
    Definition *dn = NULL;

    if (let)
        dn = tc->decls.lookupFirst(atom);

    if (!dn) {
        dn = tc->lexdeps.lookupDefn(atom);
        foundLexdep = !!dn;
    }

    if (dn && dn != pn) {
        ParseNode **pnup = &dn->dn_uses;
        ParseNode *pnu;
        uintN start = let ? pn->pn_blockid : tc->bodyid;

        while ((pnu = *pnup) != NULL && pnu->pn_blockid >= start) {
            JS_ASSERT(pnu->isUsed());
            pnu->pn_lexdef = (Definition *) pn;
            pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
            pnup = &pnu->pn_link;
        }

        if (pnu != dn->dn_uses) {
            *pnup = pn->dn_uses;
            pn->dn_uses = dn->dn_uses;
            dn->dn_uses = pnu;

            if ((!pnu || pnu->pn_blockid < tc->bodyid) && foundLexdep)
                tc->lexdeps->remove(atom);
        }
    }

    Definition *toAdd = (Definition *) pn;
    bool ok = let ? tc->decls.addShadow(atom, toAdd) : tc->decls.addUnique(atom, toAdd);
    if (!ok)
        return false;
    pn->setDefn(true);
    pn->pn_dflags &= ~PND_PLACEHOLDER;
    if (!tc->parent)
        pn->pn_dflags |= PND_TOPLEVEL;
    return true;
}

static void
ForgetUse(ParseNode *pn)
{
    if (!pn->isUsed()) {
        JS_ASSERT(!pn->isDefn());
        return;
    }

    ParseNode **pnup = &pn->lexdef()->dn_uses;
    ParseNode *pnu;
    while ((pnu = *pnup) != pn)
        pnup = &pnu->pn_link;
    *pnup = pn->pn_link;
    pn->setUsed(false);
}

static ParseNode *
MakeAssignment(ParseNode *pn, ParseNode *rhs, TreeContext *tc)
{
    ParseNode *lhs = tc->parser->new_<ParseNode>(*pn);
    if (!lhs)
        return NULL;

    if (pn->isUsed()) {
        Definition *dn = pn->pn_lexdef;
        ParseNode **pnup = &dn->dn_uses;

        while (*pnup != pn)
            pnup = &(*pnup)->pn_link;
        *pnup = lhs;
        lhs->pn_link = pn->pn_link;
        pn->pn_link = NULL;
    }

    pn->setKind(TOK_ASSIGN);
    pn->setOp(JSOP_NOP);
    pn->setArity(PN_BINARY);
    pn->setInParens(false);
    pn->setUsed(false);
    pn->setDefn(false);
    pn->pn_left = lhs;
    pn->pn_right = rhs;
    return lhs;
}

static ParseNode *
MakeDefIntoUse(Definition *dn, ParseNode *pn, JSAtom *atom, TreeContext *tc)
{
    




    if (dn->isBindingForm()) {
        ParseNode *rhs = dn->expr();
        if (rhs) {
            ParseNode *lhs = MakeAssignment(dn, rhs, tc);
            if (!lhs)
                return NULL;
            
            dn = (Definition *) lhs;
        }

        dn->setOp((js_CodeSpec[dn->getOp()].format & JOF_SET) ? JSOP_SETNAME : JSOP_NAME);
    } else if (dn->kind() == Definition::FUNCTION) {
        JS_ASSERT(dn->isOp(JSOP_NOP));
        tc->parser->prepareNodeForMutation(dn);
        dn->setKind(TOK_NAME);
        dn->setArity(PN_NAME);
        dn->pn_atom = atom;
    }

    
    JS_ASSERT(dn->isKind(TOK_NAME));
    JS_ASSERT(dn->isArity(PN_NAME));
    JS_ASSERT(dn->pn_atom == atom);

    for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
        JS_ASSERT(pnu->isUsed());
        JS_ASSERT(!pnu->isDefn());
        pnu->pn_lexdef = (Definition *) pn;
        pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
    }
    pn->pn_dflags |= dn->pn_dflags & PND_USE2DEF_FLAGS;
    pn->dn_uses = dn;

    dn->setDefn(false);
    dn->setUsed(true);
    dn->pn_lexdef = (Definition *) pn;
    dn->pn_cookie.makeFree();
    dn->pn_dflags &= ~PND_BOUND;
    return dn;
}

namespace js {

bool
DefineArg(ParseNode *pn, JSAtom *atom, uintN i, TreeContext *tc)
{
    ParseNode *argpn, *argsbody;

    
    if (atom == tc->parser->context->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_PARAM_ARGUMENTS;

    




    argpn = NameNode::create(atom, tc);
    if (!argpn)
        return false;
    JS_ASSERT(argpn->isKind(TOK_NAME) && argpn->isOp(JSOP_NOP));

    
    argpn->pn_dflags |= PND_INITIALIZED;
    if (!Define(argpn, atom, tc))
        return false;

    argsbody = pn->pn_body;
    if (!argsbody) {
        argsbody = ListNode::create(tc);
        if (!argsbody)
            return false;
        argsbody->setKind(TOK_ARGSBODY);
        argsbody->setOp(JSOP_NOP);
        argsbody->makeEmpty();
        pn->pn_body = argsbody;
    }
    argsbody->append(argpn);

    argpn->setOp(JSOP_GETARG);
    argpn->pn_cookie.set(tc->staticLevel, i);
    argpn->pn_dflags |= PND_BOUND;
    return true;
}

} 








typedef JSBool
(*Binder)(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc);

struct BindData {
    BindData() : fresh(true) {}

    ParseNode       *pn;        

    JSOp            op;         
    Binder          binder;     
    union {
        struct {
            uintN   overflow;
        } let;
    };
    bool fresh;
};

static bool
BindLocalVariable(JSContext *cx, TreeContext *tc, ParseNode *pn, BindingKind kind)
{
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);

    
    JS_ASSERT_IF(pn->pn_atom == cx->runtime->atomState.argumentsAtom, kind == VARIABLE);

    uintN index = tc->bindings.countVars();
    if (!tc->bindings.add(cx, pn->pn_atom, kind))
        return false;

    pn->pn_cookie.set(tc->staticLevel, index);
    pn->pn_dflags |= PND_BOUND;
    return true;
}

#if JS_HAS_DESTRUCTURING
static JSBool
BindDestructuringArg(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc)
{
    
    if (atom == tc->parser->context->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_PARAM_ARGUMENTS;

    JS_ASSERT(tc->inFunction());

    




    if (tc->decls.lookupFirst(atom)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), NULL, JSREPORT_ERROR,
                                 JSMSG_DESTRUCT_DUP_ARG);
        return JS_FALSE;
    }

    ParseNode *pn = data->pn;

    


















    pn->setOp(JSOP_SETLOCAL);
    pn->pn_dflags |= PND_BOUND;

    return Define(pn, atom, tc);
}
#endif 

JSFunction *
Parser::newFunction(TreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, atom != NULL);

    





    while (tc->parent)
        tc = tc->parent;
    JSObject *parent = tc->inFunction() ? NULL : tc->scopeChain();

    JSFunction *fun =
        js_NewFunction(context, NULL, NULL, 0,
                       JSFUN_INTERPRETED | (kind == Expression ? JSFUN_LAMBDA : 0),
                       parent, atom);
    if (fun && !tc->compileAndGo()) {
        fun->clearParent();
        fun->clearType();
    }
    return fun;
}

static JSBool
MatchOrInsertSemicolon(JSContext *cx, TokenStream *ts)
{
    TokenKind tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return JS_FALSE;
    if (tt != TOK_EOF && tt != TOK_EOL && tt != TOK_SEMI && tt != TOK_RC) {
        
        ts->getToken(TSF_OPERAND);
        ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR, JSMSG_SEMI_BEFORE_STMNT);
        return JS_FALSE;
    }
    (void) ts->matchToken(TOK_SEMI);
    return JS_TRUE;
}

bool
Parser::analyzeFunctions(TreeContext *tc)
{
    cleanFunctionList(&tc->functionList);
    if (!tc->functionList)
        return true;
    if (!markFunArgs(tc->functionList))
        return false;
    markExtensibleScopeDescendants(tc->functionList, false);
    setFunctionKinds(tc->functionList, &tc->flags);
    return true;
}































static uintN
FindFunArgs(FunctionBox *funbox, int level, FunctionBoxQueue *queue)
{
    uintN allskipmin = UpvarCookie::FREE_LEVEL;

    do {
        ParseNode *fn = funbox->node;
        JS_ASSERT(fn->isArity(PN_FUNC));
        JSFunction *fun = funbox->function();
        int fnlevel = level;

        













        if (funbox->tcflags & (TCF_FUN_HEAVYWEIGHT | TCF_FUN_IS_GENERATOR)) {
            fn->setFunArg();
            for (FunctionBox *kid = funbox->kids; kid; kid = kid->siblings)
                kid->node->setFunArg();
        }

        




        uintN skipmin = UpvarCookie::FREE_LEVEL;
        ParseNode *pn = fn->pn_body;

        if (pn->isKind(TOK_UPVARS)) {
            AtomDefnMapPtr &upvars = pn->pn_names;
            JS_ASSERT(upvars->count() != 0);

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar()) {
                    uintN upvarLevel = lexdep->frameLevel();

                    if (int(upvarLevel) <= fnlevel)
                        fn->setFunArg();

                    uintN skip = (funbox->level + 1) - upvarLevel;
                    if (skip < skipmin)
                        skipmin = skip;
                }
            }
        }

        






        if (fn->isFunArg()) {
            queue->push(funbox);
            fnlevel = int(funbox->level);
        }

        



        if (funbox->kids) {
            uintN kidskipmin = FindFunArgs(funbox->kids, fnlevel, queue);

            JS_ASSERT(kidskipmin != 0);
            if (kidskipmin != UpvarCookie::FREE_LEVEL) {
                --kidskipmin;
                if (kidskipmin != 0 && kidskipmin < skipmin)
                    skipmin = kidskipmin;
            }
        }

        





        if (skipmin != UpvarCookie::FREE_LEVEL) {
            fun->u.i.skipmin = skipmin;
            if (skipmin < allskipmin)
                allskipmin = skipmin;
        }
    } while ((funbox = funbox->siblings) != NULL);

    return allskipmin;
}

bool
Parser::markFunArgs(FunctionBox *funbox)
{
    FunctionBoxQueue queue;
    if (!queue.init(functionCount)) {
        js_ReportOutOfMemory(context);
        return false;
    }

    FindFunArgs(funbox, -1, &queue);
    while ((funbox = queue.pull()) != NULL) {
        ParseNode *fn = funbox->node;
        JS_ASSERT(fn->isFunArg());

        ParseNode *pn = fn->pn_body;
        if (pn->isKind(TOK_UPVARS)) {
            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar() &&
                    !lexdep->isFunArg() &&
                    (lexdep->kind() == Definition::FUNCTION ||
                     lexdep->isOp(JSOP_CALLEE))) {
                    








                    lexdep->setFunArg();

                    FunctionBox *afunbox;
                    if (lexdep->isOp(JSOP_CALLEE)) {
                        






                        afunbox = funbox;
                        uintN calleeLevel = lexdep->pn_cookie.level();
                        uintN staticLevel = afunbox->level + 1U;
                        while (staticLevel != calleeLevel) {
                            afunbox = afunbox->parent;
                            --staticLevel;
                        }
                        JS_ASSERT(afunbox->level + 1U == calleeLevel);
                        afunbox->node->setFunArg();
                    } else {
                       afunbox = lexdep->pn_funbox;
                    }
                    queue.push(afunbox);

                    




                    if (afunbox->kids)
                        FindFunArgs(afunbox->kids, afunbox->level, &queue);
                }
            }
        }
    }
    return true;
}

static uint32
MinBlockId(ParseNode *fn, uint32 id)
{
    if (fn->pn_blockid < id)
        return false;
    if (fn->isDefn()) {
        for (ParseNode *pn = fn->dn_uses; pn; pn = pn->pn_link) {
            if (pn->pn_blockid < id)
                return false;
        }
    }
    return true;
}

static inline bool
CanFlattenUpvar(Definition *dn, FunctionBox *funbox, uint32 tcflags)
{
    

















    FunctionBox *afunbox = funbox;
    uintN dnLevel = dn->frameLevel();

    JS_ASSERT(dnLevel <= funbox->level);
    while (afunbox->level != dnLevel) {
        afunbox = afunbox->parent;

        







        JS_ASSERT(afunbox);

        





        if (!afunbox || afunbox->node->isFunArg())
            return false;

        




        if (afunbox->tcflags & TCF_FUN_IS_GENERATOR)
            return false;
    }

    





    if (afunbox->inLoop)
        return false;

    





    if ((afunbox->parent ? afunbox->parent->tcflags : tcflags) & TCF_FUN_HEAVYWEIGHT)
        return false;

    






    JSFunction *afun = afunbox->function();
    if (!(afun->flags & JSFUN_LAMBDA)) {
        if (dn->isBindingForm() || dn->pn_pos >= afunbox->node->pn_pos)
            return false;
    }

    if (!dn->isInitialized())
        return false;

    Definition::Kind dnKind = dn->kind();
    if (dnKind != Definition::CONST) {
        if (dn->isAssigned())
            return false;

        










        if (dnKind == Definition::ARG &&
            ((afunbox->parent ? afunbox->parent->tcflags : tcflags) & TCF_FUN_USES_ARGUMENTS)) {
            return false;
        }
    }

    




    if (dnKind != Definition::FUNCTION) {
        














        if (dn->pn_pos.end >= afunbox->node->pn_pos.end)
            return false;
        if (!MinBlockId(afunbox->node, dn->pn_blockid))
            return false;
    }
    return true;
}

static void
FlagHeavyweights(Definition *dn, FunctionBox *funbox, uint32 *tcflags)
{
    uintN dnLevel = dn->frameLevel();

    while ((funbox = funbox->parent) != NULL) {
        





        if (funbox->level + 1U == dnLevel || (dnLevel == 0 && dn->isLet())) {
            funbox->tcflags |= TCF_FUN_HEAVYWEIGHT;
            break;
        }
        funbox->tcflags |= TCF_FUN_ENTRAINS_SCOPES;
    }

    if (!funbox && (*tcflags & TCF_IN_FUNCTION))
        *tcflags |= TCF_FUN_HEAVYWEIGHT;
}

static bool
DeoptimizeUsesWithin(Definition *dn, const TokenPos &pos)
{
    uintN ndeoptimized = 0;

    for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
        JS_ASSERT(pnu->isUsed());
        JS_ASSERT(!pnu->isDefn());
        if (pnu->pn_pos.begin >= pos.begin && pnu->pn_pos.end <= pos.end) {
            pnu->pn_dflags |= PND_DEOPTIMIZED;
            ++ndeoptimized;
        }
    }

    return ndeoptimized != 0;
}

static void
ConsiderUnbranding(FunctionBox *funbox)
{
    









    bool returnsExpr = !!(funbox->tcflags & TCF_RETURN_EXPR);
#if JS_HAS_EXPR_CLOSURES
    {
        ParseNode *pn2 = funbox->node->pn_body;
        if (pn2->isKind(TOK_UPVARS))
            pn2 = pn2->pn_tree;
        if (pn2->isKind(TOK_ARGSBODY))
            pn2 = pn2->last();
        if (!pn2->isKind(TOK_LC))
            returnsExpr = true;
    }
#endif
    if (!returnsExpr) {
        uintN methodSets = 0, slowMethodSets = 0;

        for (ParseNode *method = funbox->methods; method; method = method->pn_link) {
            JS_ASSERT(method->isOp(JSOP_LAMBDA) || method->isOp(JSOP_LAMBDA_FC));
            ++methodSets;
            if (!method->pn_funbox->joinable())
                ++slowMethodSets;
        }

        if (funbox->shouldUnbrand(methodSets, slowMethodSets))
            funbox->tcflags |= TCF_FUN_UNBRAND_THIS;
    }
}

void
Parser::setFunctionKinds(FunctionBox *funbox, uint32 *tcflags)
{
    for (; funbox; funbox = funbox->siblings) {
        ParseNode *fn = funbox->node;
        ParseNode *pn = fn->pn_body;

        if (funbox->kids) {
            setFunctionKinds(funbox->kids, tcflags);
            ConsiderUnbranding(funbox);
        }

        JSFunction *fun = funbox->function();

        JS_ASSERT(fun->kind() == JSFUN_INTERPRETED);

        if (funbox->tcflags & TCF_FUN_HEAVYWEIGHT) {
            
        } else if (funbox->inAnyDynamicScope()) {
            JS_ASSERT(!fun->isNullClosure());
        } else {
            bool hasUpvars = false;
            bool canFlatten = true;

            if (pn->isKind(TOK_UPVARS)) {
                AtomDefnMapPtr upvars = pn->pn_names;
                JS_ASSERT(!upvars->empty());

                




                for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                    Definition *defn = r.front().value();
                    Definition *lexdep = defn->resolve();

                    if (!lexdep->isFreeVar()) {
                        hasUpvars = true;
                        if (!CanFlattenUpvar(lexdep, funbox, *tcflags)) {
                            





                            canFlatten = false;
                            break;
                        }
                    }
                }
            }

            if (!hasUpvars) {
                
                fun->setKind(JSFUN_NULL_CLOSURE);
            } else if (canFlatten) {
                fun->setKind(JSFUN_FLAT_CLOSURE);
                switch (fn->getOp()) {
                  case JSOP_DEFFUN:
                    fn->setOp(JSOP_DEFFUN_FC);
                    break;
                  case JSOP_DEFLOCALFUN:
                    fn->setOp(JSOP_DEFLOCALFUN_FC);
                    break;
                  case JSOP_LAMBDA:
                    fn->setOp(JSOP_LAMBDA_FC);
                    break;
                  default:
                    
                    JS_ASSERT(fn->isOp(JSOP_NOP));
                }
            }
        }

        if (fun->kind() == JSFUN_INTERPRETED && pn->isKind(TOK_UPVARS)) {
            








            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();
                if (!lexdep->isFreeVar())
                    FlagHeavyweights(lexdep, funbox, tcflags);
            }
        }

        if (funbox->joinable())
            fun->setJoinable();
    }
}












void
Parser::markExtensibleScopeDescendants(FunctionBox *funbox, bool hasExtensibleParent) 
{
    for (; funbox; funbox = funbox->siblings) {
        





        JS_ASSERT(!funbox->bindings.extensibleParents());
        if (hasExtensibleParent)
            funbox->bindings.setExtensibleParents();

        if (funbox->kids) {
            markExtensibleScopeDescendants(funbox->kids,
                                           hasExtensibleParent || funbox->scopeIsExtensible());
        }
    }
}

static FunctionBox *
EnterFunction(ParseNode *fn, TreeContext *funtc, JSAtom *funAtom = NULL,
              FunctionSyntaxKind kind = Expression)
{
    TreeContext *tc = funtc->parent;
    JSFunction *fun = tc->parser->newFunction(tc, funAtom, kind);
    if (!fun)
        return NULL;

    
    FunctionBox *funbox = tc->parser->newFunctionBox(fun, fn, tc);
    if (!funbox)
        return NULL;

    
    funtc->flags |= funbox->tcflags;
    funtc->blockidGen = tc->blockidGen;
    if (!GenerateBlockId(funtc, funtc->bodyid))
        return NULL;
    funtc->setFunction(fun);
    funtc->funbox = funbox;
    if (!SetStaticLevel(funtc, tc->staticLevel + 1))
        return NULL;

    return funbox;
}

static bool
LeaveFunction(ParseNode *fn, TreeContext *funtc, PropertyName *funName = NULL,
              FunctionSyntaxKind kind = Expression)
{
    TreeContext *tc = funtc->parent;
    tc->blockidGen = funtc->blockidGen;

    FunctionBox *funbox = fn->pn_funbox;
    funbox->tcflags |= funtc->flags & (TCF_FUN_FLAGS | TCF_COMPILE_N_GO | TCF_RETURN_EXPR);

    fn->pn_dflags |= PND_INITIALIZED;
    if (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
        fn->pn_dflags |= PND_BLOCKCHILD;

    






    if (funtc->lexdeps->count()) {
        int foundCallee = 0;

        for (AtomDefnRange r = funtc->lexdeps->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key();
            Definition *dn = r.front().value();
            JS_ASSERT(dn->isPlaceholder());

            if (atom == funName && kind == Expression) {
                dn->setOp(JSOP_CALLEE);
                dn->pn_cookie.set(funtc->staticLevel, UpvarCookie::CALLEE_SLOT);
                dn->pn_dflags |= PND_BOUND;

                



                if (dn->isFunArg())
                    funbox->tcflags |= TCF_FUN_USES_OWN_NAME;
                foundCallee = 1;
                continue;
            }

            if (!(funbox->tcflags & TCF_FUN_SETS_OUTER_NAME) &&
                dn->isAssigned()) {
                





                for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
                    if (pnu->isAssigned() && pnu->pn_blockid >= funtc->bodyid) {
                        funbox->tcflags |= TCF_FUN_SETS_OUTER_NAME;
                        break;
                    }
                }
            }

            Definition *outer_dn = tc->decls.lookupFirst(atom);

            



            if (funtc->callsEval() ||
                (outer_dn && tc->innermostWith &&
                 outer_dn->pn_pos < tc->innermostWith->pn_pos)) {
                DeoptimizeUsesWithin(dn, fn->pn_pos);
            }

            if (!outer_dn) {
                AtomDefnAddPtr p = tc->lexdeps->lookupForAdd(atom);
                if (p) {
                    outer_dn = p.value();
                } else {
                    




















                    outer_dn = MakePlaceholder(dn, tc);
                    if (!outer_dn || !tc->lexdeps->add(p, atom, outer_dn))
                        return false;
                }
            }

            












            if (dn != outer_dn) {
                ParseNode **pnup = &dn->dn_uses;
                ParseNode *pnu;

                while ((pnu = *pnup) != NULL) {
                    pnu->pn_lexdef = outer_dn;
                    pnup = &pnu->pn_link;
                }

                





                *pnup = outer_dn->dn_uses;
                outer_dn->dn_uses = dn;
                outer_dn->pn_dflags |= dn->pn_dflags & ~PND_PLACEHOLDER;
                dn->setDefn(false);
                dn->setUsed(true);
                dn->pn_lexdef = outer_dn;
            }

            
            outer_dn->pn_dflags |= PND_CLOSED;
        }

        if (funtc->lexdeps->count() - foundCallee != 0) {
            ParseNode *body = fn->pn_body;

            fn->pn_body = NameSetNode::create(tc);
            if (!fn->pn_body)
                return false;

            fn->pn_body->setKind(TOK_UPVARS);
            fn->pn_body->pn_pos = body->pn_pos;
            if (foundCallee)
                funtc->lexdeps->remove(funName);
            
            fn->pn_body->pn_names = funtc->lexdeps;
            funtc->lexdeps.clearMap();
            fn->pn_body->pn_tree = body;
        } else {
            funtc->lexdeps.releaseMap(funtc->parser->context);
        }

    }

    






    if (funtc->inStrictMode() && funbox->object->getFunctionPrivate()->nargs > 0) {
        AtomDeclsIter iter(&funtc->decls);
        Definition *dn;

        while ((dn = iter()) != NULL) {
            if (dn->kind() == Definition::ARG && dn->isAssigned()) {
                funbox->tcflags |= TCF_FUN_MUTATES_PARAMETER;
                break;
            }
        }
    }

    funbox->bindings.transfer(funtc->parser->context, &funtc->bindings);

    return true;
}

static bool
DefineGlobal(ParseNode *pn, CodeGenerator *cg, PropertyName *name);







bool
Parser::functionArguments(TreeContext &funtc, FunctionBox *funbox, ParseNode **listp)
{
    if (tokenStream.getToken() != TOK_LP) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_PAREN_BEFORE_FORMAL);
        return false;
    }

    if (!tokenStream.matchToken(TOK_RP)) {
#if JS_HAS_DESTRUCTURING
        JSAtom *duplicatedArg = NULL;
        bool destructuringArg = false;
        ParseNode *list = NULL;
#endif

        do {
            switch (TokenKind tt = tokenStream.getToken()) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
              {
                
                if (duplicatedArg)
                    goto report_dup_and_destructuring;
                destructuringArg = true;

                





                BindData data;
                data.pn = NULL;
                data.op = JSOP_DEFVAR;
                data.binder = BindDestructuringArg;
                ParseNode *lhs = destructuringExpr(&data, tt);
                if (!lhs)
                    return false;

                



                uint16 slot;
                if (!funtc.bindings.addDestructuring(context, &slot))
                    return false;

                




                ParseNode *rhs = NameNode::create(context->runtime->atomState.emptyAtom, &funtc);
                if (!rhs)
                    return false;
                rhs->setKind(TOK_NAME);
                rhs->setOp(JSOP_GETARG);
                rhs->pn_cookie.set(funtc.staticLevel, slot);
                rhs->pn_dflags |= PND_BOUND;

                ParseNode *item =
                    ParseNode::newBinaryOrAppend(TOK_ASSIGN, JSOP_NOP, lhs, rhs, &funtc);
                if (!item)
                    return false;
                if (!list) {
                    list = ListNode::create(&funtc);
                    if (!list)
                        return false;
                    list->setKind(TOK_VAR);
                    list->makeEmpty();
                    *listp = list;
                }
                list->append(item);
                break;
              }
#endif 

              case TOK_NAME:
              {
                PropertyName *name = tokenStream.currentToken().name();

#ifdef JS_HAS_DESTRUCTURING
                















                if (funtc.decls.lookupFirst(name)) {
                    duplicatedArg = name;
                    if (destructuringArg)
                        goto report_dup_and_destructuring;
                }
#endif

                uint16 slot;
                if (!funtc.bindings.addArgument(context, name, &slot))
                    return false;
                if (!DefineArg(funbox->node, name, slot, &funtc))
                    return false;
                break;
              }

              default:
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_MISSING_FORMAL);
                
              case TOK_ERROR:
                return false;

#if JS_HAS_DESTRUCTURING
              report_dup_and_destructuring:
                Definition *dn = funtc.decls.lookupFirst(duplicatedArg);
                reportErrorNumber(dn, JSREPORT_ERROR, JSMSG_DESTRUCT_DUP_ARG);
                return false;
#endif
            }
        } while (tokenStream.matchToken(TOK_COMMA));

        if (tokenStream.getToken() != TOK_RP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_PAREN_AFTER_FORMAL);
            return false;
        }
    }

    return true;
}

ParseNode *
Parser::functionDef(PropertyName *funName, FunctionType type, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, funName);

    
    tokenStream.mungeCurrentToken(TOK_FUNCTION, JSOP_NOP);
    ParseNode *pn = FunctionNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_body = NULL;
    pn->pn_cookie.makeFree();

    








    bool bodyLevel = tc->atBodyLevel();
    pn->pn_dflags = (kind == Expression || !bodyLevel) ? PND_FUNARG : 0;

    



    if (kind == Statement) {
        if (Definition *dn = tc->decls.lookupFirst(funName)) {
            Definition::Kind dn_kind = dn->kind();

            JS_ASSERT(!dn->isUsed());
            JS_ASSERT(dn->isDefn());

            if (context->hasStrictOption() || dn_kind == Definition::CONST) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(context, funName, &name) ||
                    !reportErrorNumber(NULL,
                                       (dn_kind != Definition::CONST)
                                       ? JSREPORT_WARNING | JSREPORT_STRICT
                                       : JSREPORT_ERROR,
                                       JSMSG_REDECLARED_VAR,
                                       Definition::kindString(dn_kind),
                                       name.ptr())) {
                    return NULL;
                }
            }

            if (bodyLevel) {
                tc->decls.updateFirst(funName, (Definition *) pn);
                pn->setDefn(true);
                pn->dn_uses = dn; 

                if (!MakeDefIntoUse(dn, pn, funName, tc))
                    return NULL;
            }
        } else if (bodyLevel) {
            





            if (Definition *fn = tc->lexdeps.lookupDefn(funName)) {
                JS_ASSERT(fn->isDefn());
                fn->setKind(TOK_FUNCTION);
                fn->setArity(PN_FUNC);
                fn->pn_pos.begin = pn->pn_pos.begin;

                



                fn->pn_pos.end = pn->pn_pos.end;

                fn->pn_body = NULL;
                fn->pn_cookie.makeFree();

                tc->lexdeps->remove(funName);
                freeTree(pn);
                pn = fn;
            }

            if (!Define(pn, funName, tc))
                return NULL;
        }

        







        if (bodyLevel && tc->inFunction()) {
            






            uintN index;
            switch (tc->bindings.lookup(context, funName, &index)) {
              case NONE:
              case ARGUMENT:
                index = tc->bindings.countVars();
                if (!tc->bindings.addVariable(context, funName))
                    return NULL;
                

              case VARIABLE:
                pn->pn_cookie.set(tc->staticLevel, index);
                pn->pn_dflags |= PND_BOUND;
                break;

              default:;
            }
        }
    }

    TreeContext *outertc = tc;

    
    TreeContext funtc(tc->parser);
    if (!funtc.init(context))
        return NULL;

    FunctionBox *funbox = EnterFunction(pn, &funtc, funName, kind);
    if (!funbox)
        return NULL;

    JSFunction *fun = funbox->function();

    
    ParseNode *prelude = NULL;
    if (!functionArguments(funtc, funbox, &prelude))
        return NULL;

    fun->setArgCount(funtc.bindings.countArgs());

#if JS_HAS_DESTRUCTURING
    







    if (prelude) {
        AtomDeclsIter iter(&funtc.decls);

        while (Definition *apn = iter()) {
            
            if (!apn->isOp(JSOP_SETLOCAL))
                continue;

            if (!BindLocalVariable(context, &funtc, apn, VARIABLE))
                return NULL;
        }
    }
#endif

    if (type == Getter && fun->nargs > 0) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_ACCESSOR_WRONG_ARGS,
                          "getter", "no", "s");
        return NULL;
    }
    if (type == Setter && fun->nargs != 1) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_ACCESSOR_WRONG_ARGS,
                          "setter", "one", "");
        return NULL;
    }

#if JS_HAS_EXPR_CLOSURES
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    if (tt != TOK_LC) {
        tokenStream.ungetToken();
        fun->flags |= JSFUN_EXPR_CLOSURE;
    }
#else
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_BODY);
#endif

    ParseNode *body = functionBody();
    if (!body)
        return NULL;

    if (funName && !CheckStrictBinding(context, &funtc, funName, pn))
        return NULL;

    if (!CheckStrictParameters(context, &funtc))
        return NULL;

#if JS_HAS_EXPR_CLOSURES
    if (tt == TOK_LC)
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
    else if (kind == Statement && !MatchOrInsertSemicolon(context, &tokenStream))
        return NULL;
#else
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
#endif
    pn->pn_pos.end = tokenStream.currentToken().pos.end;

    












    if (funtc.callsEval())
        outertc->noteCallsEval();

#if JS_HAS_DESTRUCTURING
    





    if (prelude) {
        if (!body->isArity(PN_LIST)) {
            ParseNode *block;

            block = ListNode::create(outertc);
            if (!block)
                return NULL;
            block->setKind(TOK_SEQ);
            block->pn_pos = body->pn_pos;
            block->initList(body);

            body = block;
        }

        ParseNode *item = UnaryNode::create(outertc);
        if (!item)
            return NULL;

        item->setKind(TOK_SEMI);
        item->pn_pos.begin = item->pn_pos.end = body->pn_pos.begin;
        item->pn_kid = prelude;
        item->pn_next = body->pn_head;
        body->pn_head = item;
        if (body->pn_tail == &body->pn_head)
            body->pn_tail = &item->pn_next;
        ++body->pn_count;
        body->pn_xflags |= PNX_DESTRUCT;
    }
#endif

    





    if (funtc.flags & TCF_FUN_HEAVYWEIGHT) {
        fun->flags |= JSFUN_HEAVYWEIGHT;
        outertc->flags |= TCF_FUN_HEAVYWEIGHT;
    } else {
        





        if (!bodyLevel && kind == Statement)
            outertc->flags |= TCF_FUN_HEAVYWEIGHT;
    }

    JSOp op = JSOP_NOP;
    if (kind == Expression) {
        op = JSOP_LAMBDA;
    } else {
        if (!bodyLevel) {
            





            JS_ASSERT(!outertc->inStrictMode());
            op = JSOP_DEFFUN;
            outertc->noteMightAliasLocals();
        }
    }

    funbox->kids = funtc.functionList;

    pn->pn_funbox = funbox;
    pn->setOp(op);
    if (pn->pn_body) {
        pn->pn_body->append(body);
        pn->pn_body->pn_pos = body->pn_pos;
    } else {
        pn->pn_body = body;
    }

    if (!outertc->inFunction() && bodyLevel && kind == Statement && outertc->compiling()) {
        JS_ASSERT(pn->pn_cookie.isFree());
        if (!DefineGlobal(pn, outertc->asCodeGenerator(), funName))
            return NULL;
    }

    pn->pn_blockid = outertc->blockid();

    if (!LeaveFunction(pn, &funtc, funName, kind))
        return NULL;

    
    if (!outertc->inStrictMode())
        tokenStream.setStrictMode(false);

    return pn;
}

ParseNode *
Parser::functionStmt()
{
    PropertyName *name = NULL;
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME) {
        name = tokenStream.currentToken().name();
    } else {
        
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_UNNAMED_FUNCTION_STMT);
        return NULL;
    }

    
    if (!tc->atBodyLevel() && tc->inStrictMode()) {
        reportErrorNumber(NULL, JSREPORT_STRICT_MODE_ERROR, JSMSG_STRICT_FUNCTION_STATEMENT);
        return NULL;
    }

    return functionDef(name, Normal, Statement);
}

ParseNode *
Parser::functionExpr()
{
    PropertyName *name = NULL;
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME)
        name = tokenStream.currentToken().name();
    else
        tokenStream.ungetToken();
    return functionDef(name, Normal, Expression);
}




















bool
Parser::recognizeDirectivePrologue(ParseNode *pn, bool *isDirectivePrologueMember)
{
    *isDirectivePrologueMember = pn->isStringExprStatement();
    if (!*isDirectivePrologueMember)
        return true;

    ParseNode *kid = pn->pn_kid;
    if (kid->isEscapeFreeStringLiteral()) {
        











        pn->pn_prologue = true;

        JSAtom *directive = kid->pn_atom;
        if (directive == context->runtime->atomState.useStrictAtom) {
            













            if (tokenStream.hasOctalCharacterEscape()) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_DEPRECATED_OCTAL);
                return false;
            }

            tc->flags |= TCF_STRICT_MODE_CODE;
            tokenStream.setStrictMode();
        }
    }
    return true;
}






ParseNode *
Parser::statements()
{
    ParseNode *pn, *pn2, *saveBlock;
    TokenKind tt;

    JS_CHECK_RECURSION(context, return NULL);

    pn = ListNode::create(tc);
    if (!pn)
        return NULL;
    pn->setKind(TOK_LC);
    pn->makeEmpty();
    pn->pn_blockid = tc->blockid();
    saveBlock = tc->blockNode;
    tc->blockNode = pn;

    bool inDirectivePrologue = tc->atBodyLevel();
    tokenStream.setOctalCharacterEscape(false);
    for (;;) {
        tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF || tt == TOK_RC) {
            if (tt == TOK_ERROR) {
                if (tokenStream.isEOF())
                    tokenStream.setUnexpectedEOF();
                return NULL;
            }
            break;
        }
        pn2 = statement();
        if (!pn2) {
            if (tokenStream.isEOF())
                tokenStream.setUnexpectedEOF();
            return NULL;
        }

        if (inDirectivePrologue && !recognizeDirectivePrologue(pn2, &inDirectivePrologue))
            return NULL;

        if (pn2->isKind(TOK_FUNCTION)) {
            








            if (tc->atBodyLevel()) {
                pn->pn_xflags |= PNX_FUNCDEFS;
            } else {
                tc->flags |= TCF_HAS_FUNCTION_STMT;
                
                tc->noteHasExtensibleScope();
            }
        }
        pn->append(pn2);
    }

    




    if (tc->blockNode != pn)
        pn = tc->blockNode;
    tc->blockNode = saveBlock;

    pn->pn_pos.end = tokenStream.currentToken().pos.end;
    return pn;
}

ParseNode *
Parser::condition()
{
    ParseNode *pn;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_COND);
    pn = parenExpr();
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    if (pn->isKind(TOK_ASSIGN) &&
        pn->isOp(JSOP_NOP) &&
        !pn->isInParens() &&
        !reportErrorNumber(NULL, JSREPORT_WARNING | JSREPORT_STRICT, JSMSG_EQUAL_AS_ASSIGN, "")) {
        return NULL;
    }
    return pn;
}

static bool
MatchLabel(JSContext *cx, TokenStream *ts, ParseNode *pn)
{
    TokenKind tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return false;
    PropertyName *label;
    if (tt == TOK_NAME) {
        (void) ts->getToken();
        label = ts->currentToken().name();
    } else {
        label = NULL;
    }
    pn->pn_atom = label;
    return true;
}










static JSBool
BindLet(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc)
{
    ParseNode *pn;
    JSObject *blockObj;
    jsint n;

    



    JS_ASSERT(!tc->atBodyLevel());

    pn = data->pn;
    if (!CheckStrictBinding(cx, tc, atom->asPropertyName(), pn))
        return false;

    blockObj = tc->blockChain();
    Definition *dn = tc->decls.lookupFirst(atom);
    if (dn && dn->pn_blockid == tc->blockid()) {
        JSAutoByteString name;
        if (js_AtomToPrintableString(cx, atom, &name)) {
            ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                     JSREPORT_ERROR, JSMSG_REDECLARED_VAR,
                                     dn->isConst() ? "const" : "variable",
                                     name.ptr());
        }
        return false;
    }

    n = OBJ_BLOCK_COUNT(cx, blockObj);
    if (n == JS_BIT(16)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, data->let.overflow);
        return false;
    }

    



    if (!Define(pn, atom, tc, true))
        return false;

    






    pn->setOp(JSOP_GETLOCAL);
    pn->pn_cookie.set(tc->staticLevel, uint16(n));
    pn->pn_dflags |= PND_LET | PND_BOUND;

    



    const Shape *shape = blockObj->defineBlockVariable(cx, ATOM_TO_JSID(atom), n);
    if (!shape)
        return false;

    






    blockObj->setSlot(shape->slot, PrivateValue(pn));
    return true;
}

static void
PopStatement(TreeContext *tc)
{
    StmtInfo *stmt = tc->topStmt;

    if (stmt->flags & SIF_SCOPE) {
        JSObject *obj = stmt->blockBox->object;
        JS_ASSERT(!obj->isClonedBlock());

        for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = JSID_TO_ATOM(r.front().propid);

            
            if (atom == tc->parser->context->runtime->atomState.emptyAtom)
                continue;
            tc->decls.remove(atom);
        }
    }
    PopStatementTC(tc);
}

static inline bool
OuterLet(TreeContext *tc, StmtInfo *stmt, JSAtom *atom)
{
    while (stmt->downScope) {
        stmt = LexicalLookup(tc, atom, NULL, stmt->downScope);
        if (!stmt)
            return false;
        if (stmt->type == STMT_BLOCK)
            return true;
    }
    return false;
}
















static bool
DefineGlobal(ParseNode *pn, CodeGenerator *cg, PropertyName *name)
{
    GlobalScope *globalScope = cg->compiler()->globalScope;
    JSObject *globalObj = globalScope->globalObj;

    if (!cg->compileAndGo() || !globalObj || cg->compilingForEval())
        return true;

    AtomIndexAddPtr p = globalScope->names.lookupForAdd(name);
    if (!p) {
        JSContext *cx = cg->parser->context;

        JSObject *holder;
        JSProperty *prop;
        if (!globalObj->lookupProperty(cx, name, &holder, &prop))
            return false;

        FunctionBox *funbox = pn->isKind(TOK_FUNCTION) ? pn->pn_funbox : NULL;

        GlobalScope::GlobalDef def;
        if (prop) {
            





            const Shape *shape = (const Shape *)prop;
            if (funbox ||
                globalObj != holder ||
                shape->configurable() ||
                !shape->hasSlot() ||
                !shape->hasDefaultGetterOrIsMethod() ||
                !shape->hasDefaultSetter()) {
                return true;
            }
            
            def = GlobalScope::GlobalDef(shape->slot);
        } else {
            def = GlobalScope::GlobalDef(name, funbox);
        }

        if (!globalScope->defs.append(def))
            return false;

        jsatomid index = globalScope->names.count();
        if (!globalScope->names.add(p, name, index))
            return false;

        JS_ASSERT(index == globalScope->defs.length() - 1);
    } else {
        












        if (pn->isKind(TOK_FUNCTION)) {
            JS_ASSERT(pn->isArity(PN_FUNC));
            jsatomid index = p.value();
            globalScope->defs[index].funbox = pn->pn_funbox;
        }
    }

    pn->pn_dflags |= PND_GVAR;

    return true;
}

static bool
BindTopLevelVar(JSContext *cx, BindData *data, ParseNode *pn, TreeContext *tc)
{
    JS_ASSERT(pn->isOp(JSOP_NAME));
    JS_ASSERT(!tc->inFunction());

    
    if (!tc->compiling())
        return true;

    



    if (tc->parser->callerFrame) {
        





        if (!tc->inStrictMode())
            return true;

        














        return true;
    }

    if (pn->pn_dflags & PND_CONST)
        return true;

    




    return DefineGlobal(pn, tc->asCodeGenerator(), pn->pn_atom->asPropertyName());
}

static bool
BindFunctionLocal(JSContext *cx, BindData *data, MultiDeclRange &mdl, TreeContext *tc)
{
    JS_ASSERT(tc->inFunction());

    ParseNode *pn = data->pn;
    JSAtom *name = pn->pn_atom;

    





    if (name == cx->runtime->atomState.argumentsAtom) {
        pn->setOp(JSOP_ARGUMENTS);
        pn->pn_dflags |= PND_BOUND;
        return true;
    }

    BindingKind kind = tc->bindings.lookup(cx, name, NULL);
    if (kind == NONE) {
        






        kind = (data->op == JSOP_DEFCONST) ? CONSTANT : VARIABLE;

        if (!BindLocalVariable(cx, tc, pn, kind))
            return false;
        pn->setOp(JSOP_GETLOCAL);
        return true;
    }

    if (kind == ARGUMENT) {
        JS_ASSERT(tc->inFunction());
        JS_ASSERT(!mdl.empty() && mdl.front()->kind() == Definition::ARG);
    } else {
        JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
    }

    return true;
}

static JSBool
BindVarOrConst(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc)
{
    ParseNode *pn = data->pn;

    
    pn->setOp(JSOP_NAME);

    if (!CheckStrictBinding(cx, tc, atom->asPropertyName(), pn))
        return false;

    StmtInfo *stmt = LexicalLookup(tc, atom, NULL);

    if (stmt && stmt->type == STMT_WITH) {
        data->fresh = false;
        pn->pn_dflags |= PND_DEOPTIMIZED;
        tc->noteMightAliasLocals();
        return true;
    }

    MultiDeclRange mdl = tc->decls.lookupMulti(atom);
    JSOp op = data->op;

    if (stmt || !mdl.empty()) {
        Definition *dn = mdl.empty() ? NULL : mdl.front();
        Definition::Kind dn_kind = dn ? dn->kind() : Definition::VAR;

        if (dn_kind == Definition::ARG) {
            JSAutoByteString name;
            if (!js_AtomToPrintableString(cx, atom, &name))
                return JS_FALSE;

            if (op == JSOP_DEFCONST) {
                ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                         JSREPORT_ERROR, JSMSG_REDECLARED_PARAM,
                                         name.ptr());
                return JS_FALSE;
            }
            if (!ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                          JSREPORT_WARNING | JSREPORT_STRICT,
                                          JSMSG_VAR_HIDES_ARG, name.ptr())) {
                return JS_FALSE;
            }
        } else {
            bool error = (op == JSOP_DEFCONST ||
                          dn_kind == Definition::CONST ||
                          (dn_kind == Definition::LET &&
                           (stmt->type != STMT_CATCH || OuterLet(tc, stmt, atom))));

            if (cx->hasStrictOption()
                ? op != JSOP_DEFVAR || dn_kind != Definition::VAR
                : error) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(cx, atom, &name) ||
                    !ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                              !error
                                              ? JSREPORT_WARNING | JSREPORT_STRICT
                                              : JSREPORT_ERROR,
                                              JSMSG_REDECLARED_VAR,
                                              Definition::kindString(dn_kind),
                                              name.ptr())) {
                    return JS_FALSE;
                }
            }
        }
    }

    if (mdl.empty()) {
        if (!Define(pn, atom, tc))
            return JS_FALSE;
    } else {
        










        Definition *dn = mdl.front();

        data->fresh = false;

        if (!pn->isUsed()) {
            
            ParseNode *pnu = pn;

            if (pn->isDefn()) {
                pnu = NameNode::create(atom, tc);
                if (!pnu)
                    return JS_FALSE;
            }

            LinkUseToDef(pnu, dn, tc);
            pnu->setOp(JSOP_NAME);
        }

        
        while (dn->kind() == Definition::LET) {
            mdl.popFront();
            if (mdl.empty())
                break;
            dn = mdl.front();
        }

        if (dn) {
            JS_ASSERT_IF(data->op == JSOP_DEFCONST,
                         dn->kind() == Definition::CONST);
            return JS_TRUE;
        }

        




        if (!pn->isDefn()) {
            if (tc->lexdeps->lookup(atom)) {
                tc->lexdeps->remove(atom);
            } else {
                ParseNode *pn2 = NameNode::create(atom, tc);
                if (!pn2)
                    return JS_FALSE;

                
                pn2->setKind(TOK_NAME);
                pn2->pn_pos = pn->pn_pos;
                pn = pn2;
            }
            pn->setOp(JSOP_NAME);
        }

        if (!tc->decls.addHoist(atom, (Definition *) pn))
            return JS_FALSE;
        pn->setDefn(true);
        pn->pn_dflags &= ~PND_PLACEHOLDER;
    }

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    if (tc->inFunction())
        return BindFunctionLocal(cx, data, mdl, tc);

    return BindTopLevelVar(cx, data, pn, tc);
}

static bool
MakeSetCall(JSContext *cx, ParseNode *pn, TreeContext *tc, uintN msg)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isOp(JSOP_CALL) || pn->isOp(JSOP_EVAL) ||
              pn->isOp(JSOP_FUNCALL) || pn->isOp(JSOP_FUNAPPLY));
    if (!ReportStrictModeError(cx, TS(tc->parser), tc, pn, msg))
        return false;

    ParseNode *pn2 = pn->pn_head;
    if (pn2->isKind(TOK_FUNCTION) && (pn2->pn_funbox->tcflags & TCF_GENEXP_LAMBDA)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR, msg);
        return false;
    }
    pn->pn_xflags |= PNX_SETCALL;
    return true;
}

static void
NoteLValue(JSContext *cx, ParseNode *pn, TreeContext *tc, uintN dflag = PND_ASSIGNED)
{
    if (pn->isUsed()) {
        Definition *dn = pn->pn_lexdef;

        



        if (!(dn->pn_dflags & (PND_INITIALIZED | PND_CONST | PND_PLACEHOLDER)) &&
            dn->isBlockChild() &&
            pn->isBlockChild() &&
            dn->pn_blockid == pn->pn_blockid &&
            dn->pn_pos.end <= pn->pn_pos.begin &&
            dn->dn_uses == pn) {
            dflag = PND_INITIALIZED;
        }

        dn->pn_dflags |= dflag;

        if (dn->pn_cookie.isFree() || dn->frameLevel() < tc->staticLevel)
            tc->flags |= TCF_FUN_SETS_OUTER_NAME;
    }

    pn->pn_dflags |= dflag;

    








    JSAtom *lname = pn->pn_atom;
    if (lname == cx->runtime->atomState.argumentsAtom) {
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        tc->countArgumentsUse(pn);
    } else if (tc->inFunction() && lname == tc->fun()->atom) {
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
    }
}

#if JS_HAS_DESTRUCTURING

static JSBool
BindDestructuringVar(JSContext *cx, BindData *data, ParseNode *pn, TreeContext *tc)
{
    JSAtom *atom;

    




    JS_ASSERT(pn->isKind(TOK_NAME));
    atom = pn->pn_atom;
    if (atom == cx->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_HEAVYWEIGHT;

    data->pn = pn;
    if (!data->binder(cx, data, atom, tc))
        return JS_FALSE;

    



    if (pn->pn_dflags & PND_BOUND) {
        JS_ASSERT(!(pn->pn_dflags & PND_GVAR));
        pn->setOp(pn->isOp(JSOP_ARGUMENTS) ? JSOP_SETNAME : JSOP_SETLOCAL);
    } else {
        pn->setOp((data->op == JSOP_DEFCONST) ? JSOP_SETCONST : JSOP_SETNAME);
    }

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    NoteLValue(cx, pn, tc, PND_INITIALIZED);
    return JS_TRUE;
}



















static JSBool
BindDestructuringLHS(JSContext *cx, ParseNode *pn, TreeContext *tc)
{
    switch (pn->getKind()) {
      case TOK_NAME:
        NoteLValue(cx, pn, tc);
        

      case TOK_DOT:
      case TOK_LB:
        




        if (!(js_CodeSpec[pn->getOp()].format & JOF_SET))
            pn->setOp(JSOP_SETNAME);
        break;

      case TOK_LP:
        if (!MakeSetCall(cx, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        if (pn->isOp(JSOP_XMLNAME)) {
            pn->setOp(JSOP_BINDXMLNAME);
            break;
        }
        
#endif

      default:
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return JS_FALSE;
    }

    return JS_TRUE;
}









































static bool
CheckDestructuring(JSContext *cx, BindData *data, ParseNode *left, TreeContext *tc)
{
    bool ok;

    if (left->isKind(TOK_ARRAYCOMP)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), left, JSREPORT_ERROR,
                                 JSMSG_ARRAY_COMP_LEFTSIDE);
        return false;
    }

    if (left->isKind(TOK_RB)) {
        for (ParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (!pn->isKind(TOK_COMMA) || !pn->isArity(PN_NULLARY)) {
                if (pn->isKind(TOK_RB) || pn->isKind(TOK_RC)) {
                    ok = CheckDestructuring(cx, data, pn, tc);
                } else {
                    if (data) {
                        if (!pn->isKind(TOK_NAME)) {
                            ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR,
                                                     JSMSG_NO_VARIABLE_NAME);
                            return false;
                        }
                        ok = BindDestructuringVar(cx, data, pn, tc);
                    } else {
                        ok = BindDestructuringLHS(cx, pn, tc);
                    }
                }
                if (!ok)
                    return false;
            }
        }
    } else {
        JS_ASSERT(left->isKind(TOK_RC));
        for (ParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->isKind(TOK_COLON));
            ParseNode *pn = pair->pn_right;

            if (pn->isKind(TOK_RB) || pn->isKind(TOK_RC)) {
                ok = CheckDestructuring(cx, data, pn, tc);
            } else if (data) {
                if (!pn->isKind(TOK_NAME)) {
                    ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR,
                                             JSMSG_NO_VARIABLE_NAME);
                    return false;
                }
                ok = BindDestructuringVar(cx, data, pn, tc);
            } else {
                ok = BindDestructuringLHS(cx, pn, tc);
            }
            if (!ok)
                return false;
        }
    }

    

















    if (data &&
        data->binder == BindLet &&
        OBJ_BLOCK_COUNT(cx, tc->blockChain()) == 0 &&
        !DefineNativeProperty(cx, tc->blockChain(),
                              ATOM_TO_JSID(cx->runtime->atomState.emptyAtom),
                              UndefinedValue(), NULL, NULL,
                              JSPROP_ENUMERATE | JSPROP_PERMANENT,
                              Shape::HAS_SHORTID, 0)) {
        return false;
    }

    return true;
}


















static void
UndominateInitializers(ParseNode *left, const TokenPtr &end, TreeContext *tc)
{
    if (left->isKind(TOK_RB)) {
        for (ParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (!pn->isKind(TOK_COMMA) || !pn->isArity(PN_NULLARY)) {
                if (pn->isKind(TOK_RB) || pn->isKind(TOK_RC))
                    UndominateInitializers(pn, end, tc);
                else
                    pn->pn_pos.end = end;
            }
        }
    } else {
        JS_ASSERT(left->isKind(TOK_RC));

        for (ParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->isKind(TOK_COLON));
            ParseNode *pn = pair->pn_right;
            if (pn->isKind(TOK_RB) || pn->isKind(TOK_RC))
                UndominateInitializers(pn, end, tc);
            else
                pn->pn_pos.end = end;
        }
    }
}

ParseNode *
Parser::destructuringExpr(BindData *data, TokenKind tt)
{
    tc->flags |= TCF_DECL_DESTRUCTURING;
    ParseNode *pn = primaryExpr(tt, JS_FALSE);
    tc->flags &= ~TCF_DECL_DESTRUCTURING;
    if (!pn)
        return NULL;
    if (!CheckDestructuring(context, data, pn, tc))
        return NULL;
    return pn;
}

#endif 

ParseNode *
Parser::returnOrYield(bool useAssignExpr)
{
    TokenKind tt, tt2;
    ParseNode *pn, *pn2;

    tt = tokenStream.currentToken().type;
    if (!tc->inFunction()) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_RETURN_OR_YIELD,
                          (tt == TOK_RETURN) ? js_return_str : js_yield_str);
        return NULL;
    }

    pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;

#if JS_HAS_GENERATORS
    if (tt == TOK_YIELD) {
        



        if (tc->parenDepth == 0) {
            tc->flags |= TCF_FUN_IS_GENERATOR;
        } else {
            tc->yieldCount++;
            tc->yieldNode = pn;
        }
    }
#endif

    
    tt2 = tokenStream.peekTokenSameLine(TSF_OPERAND);
    if (tt2 == TOK_ERROR)
        return NULL;

    if (tt2 != TOK_EOF && tt2 != TOK_EOL && tt2 != TOK_SEMI && tt2 != TOK_RC
#if JS_HAS_GENERATORS
        && (tt != TOK_YIELD ||
            (tt2 != tt && tt2 != TOK_RB && tt2 != TOK_RP &&
             tt2 != TOK_COLON && tt2 != TOK_COMMA))
#endif
        ) {
        pn2 = useAssignExpr ? assignExpr() : expr();
        if (!pn2)
            return NULL;
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            tc->flags |= TCF_RETURN_EXPR;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
    } else {
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            tc->flags |= TCF_RETURN_VOID;
    }

    if ((~tc->flags & (TCF_RETURN_EXPR | TCF_FUN_IS_GENERATOR)) == 0) {
        
        ReportBadReturn(context, tc, pn, JSREPORT_ERROR,
                        JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return NULL;
    }

    if (context->hasStrictOption() &&
        (~tc->flags & (TCF_RETURN_EXPR | TCF_RETURN_VOID)) == 0 &&
        !ReportBadReturn(context, tc, pn, JSREPORT_WARNING | JSREPORT_STRICT,
                         JSMSG_NO_RETURN_VALUE,
                         JSMSG_ANON_NO_RETURN_VALUE)) {
        return NULL;
    }

    return pn;
}

static ParseNode *
PushLexicalScope(JSContext *cx, TokenStream *ts, TreeContext *tc, StmtInfo *stmt)
{
    ParseNode *pn = LexicalScopeNode::create(tc);
    if (!pn)
        return NULL;

    JSObject *obj = js_NewBlockObject(cx);
    if (!obj)
        return NULL;

    ObjectBox *blockbox = tc->parser->newObjectBox(obj);
    if (!blockbox)
        return NULL;

    PushBlockScope(tc, stmt, blockbox, -1);
    pn->setKind(TOK_LEXICALSCOPE);
    pn->setOp(JSOP_LEAVEBLOCK);
    pn->pn_objbox = blockbox;
    pn->pn_cookie.makeFree();
    pn->pn_dflags = 0;
    if (!GenerateBlockId(tc, stmt->blockid))
        return NULL;
    pn->pn_blockid = stmt->blockid;
    return pn;
}

#if JS_HAS_BLOCK_SCOPE

ParseNode *
Parser::letBlock(JSBool statement)
{
    ParseNode *pn, *pnblock, *pnlet;
    StmtInfo stmtInfo;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LET);

    
    pnlet = BinaryNode::create(tc);
    if (!pnlet)
        return NULL;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    
    pnblock = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
    if (!pnblock)
        return NULL;
    pn = pnblock;
    pn->pn_expr = pnlet;

    pnlet->pn_left = variables(true);
    if (!pnlet->pn_left)
        return NULL;
    pnlet->pn_left->pn_xflags = PNX_POPVAR;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    if (statement && !tokenStream.matchToken(TOK_LC, TSF_OPERAND)) {
        







        if (!ReportStrictModeError(context, &tokenStream, tc, pnlet,
                                   JSMSG_STRICT_CODE_LET_EXPR_STMT)) {
            return NULL;
        }

        




        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->setKind(TOK_SEMI);
        pn->pn_num = -1;
        pn->pn_kid = pnblock;

        statement = JS_FALSE;
    }

    if (statement) {
        pnlet->pn_right = statements();
        if (!pnlet->pn_right)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_LET);
    } else {
        



        pnblock->setOp(JSOP_LEAVEBLOCKEXPR);
        pnlet->pn_right = assignExpr();
        if (!pnlet->pn_right)
            return NULL;
    }

    PopStatement(tc);
    return pn;
}

#endif 

static bool
PushBlocklikeStatement(StmtInfo *stmt, StmtType type, TreeContext *tc)
{
    PushStatement(tc, stmt, type, -1);
    return GenerateBlockId(tc, stmt->blockid);
}

static ParseNode *
NewBindingNode(JSAtom *atom, TreeContext *tc, bool let = false)
{
    ParseNode *pn;
    AtomDefnPtr removal;

    if ((pn = tc->decls.lookupFirst(atom))) {
        JS_ASSERT(!pn->isPlaceholder());
    } else {
        removal = tc->lexdeps->lookup(atom);
        pn = removal ? removal.value() : NULL;
        JS_ASSERT_IF(pn, pn->isPlaceholder());
    }

    if (pn) {
        JS_ASSERT(pn->isDefn());

        





        JS_ASSERT_IF(let && pn->pn_blockid == tc->blockid(),
                     pn->pn_blockid != tc->bodyid);

        if (pn->isPlaceholder() && pn->pn_blockid >= (let ? tc->blockid() : tc->bodyid)) {
            if (let)
                pn->pn_blockid = tc->blockid();

            tc->lexdeps->remove(removal);
            return pn;
        }
    }

    
    pn = NameNode::create(atom, tc);
    if (!pn)
        return NULL;

    if (atom == tc->parser->context->runtime->atomState.argumentsAtom)
        tc->countArgumentsUse(pn);

    return pn;
}

ParseNode *
Parser::switchStatement()
{
    ParseNode *pn5, *saveBlock;
    JSBool seenDefault = JS_FALSE;

    ParseNode *pn = BinaryNode::create(tc);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

    
    ParseNode *pn1 = parenExpr();
    if (!pn1)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

    



    StmtInfo stmtInfo;
    PushStatement(tc, &stmtInfo, STMT_SWITCH, -1);

    
    ParseNode *pn2 = ListNode::create(tc);
    if (!pn2)
        return NULL;
    pn2->makeEmpty();
    if (!GenerateBlockIdForStmtNode(pn2, tc))
        return NULL;
    saveBlock = tc->blockNode;
    tc->blockNode = pn2;

    TokenKind tt;
    while ((tt = tokenStream.getToken()) != TOK_RC) {
        ParseNode *pn3;
        switch (tt) {
          case TOK_DEFAULT:
            if (seenDefault) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_DEFAULTS);
                return NULL;
            }
            seenDefault = JS_TRUE;
            

          case TOK_CASE:
          {
            pn3 = BinaryNode::create(tc);
            if (!pn3)
                return NULL;
            if (tt == TOK_CASE) {
                pn3->pn_left = expr();
                if (!pn3->pn_left)
                    return NULL;
            }
            pn2->append(pn3);
            if (pn2->pn_count == JS_BIT(16)) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_CASES);
                return NULL;
            }
            break;
          }

          case TOK_ERROR:
            return NULL;

          default:
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_SWITCH);
            return NULL;
        }
        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        ParseNode *pn4 = ListNode::create(tc);
        if (!pn4)
            return NULL;
        pn4->setKind(TOK_LC);
        pn4->makeEmpty();
        while ((tt = tokenStream.peekToken(TSF_OPERAND)) != TOK_RC &&
               tt != TOK_CASE && tt != TOK_DEFAULT) {
            if (tt == TOK_ERROR)
                return NULL;
            pn5 = statement();
            if (!pn5)
                return NULL;
            pn4->pn_pos.end = pn5->pn_pos.end;
            pn4->append(pn5);
        }

        
        if (pn4->pn_head)
            pn4->pn_pos.begin = pn4->pn_head->pn_pos.begin;
        pn3->pn_pos.end = pn4->pn_pos.end;
        pn3->pn_right = pn4;
    }

    





    if (tc->blockNode != pn2)
        pn2 = tc->blockNode;
    tc->blockNode = saveBlock;
    PopStatement(tc);

    pn->pn_pos.end = pn2->pn_pos.end = tokenStream.currentToken().pos.end;
    pn->pn_left = pn1;
    pn->pn_right = pn2;
    return pn;
}

ParseNode *
Parser::forStatement()
{
    ParseNode *pnseq = NULL;
#if JS_HAS_BLOCK_SCOPE
    ParseNode *pnlet = NULL;
    StmtInfo blockInfo;
#endif

    
    ParseNode *pn = BinaryNode::create(tc);
    if (!pn)
        return NULL;
    StmtInfo stmtInfo;
    PushStatement(tc, &stmtInfo, STMT_FOR_LOOP, -1);

    pn->setOp(JSOP_ITER);
    pn->pn_iflags = 0;
    if (tokenStream.matchToken(TOK_NAME)) {
        if (tokenStream.currentToken().name() == context->runtime->atomState.eachAtom)
            pn->pn_iflags = JSITER_FOREACH;
        else
            tokenStream.ungetToken();
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);
    TokenKind tt = tokenStream.peekToken(TSF_OPERAND);

#if JS_HAS_BLOCK_SCOPE
    bool let = false;
#endif

    ParseNode *pn1;
    if (tt == TOK_SEMI) {
        if (pn->pn_iflags & JSITER_FOREACH) {
            reportErrorNumber(pn, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
            return NULL;
        }

        
        pn1 = NULL;
    } else {
        












        tc->flags |= TCF_IN_FOR_INIT;
        if (tt == TOK_VAR) {
            (void) tokenStream.getToken();
            pn1 = variables(false);
#if JS_HAS_BLOCK_SCOPE
        } else if (tt == TOK_LET) {
            let = true;
            (void) tokenStream.getToken();
            if (tokenStream.peekToken() == TOK_LP) {
                pn1 = letBlock(JS_FALSE);
                tt = TOK_LEXICALSCOPE;
            } else {
                pnlet = PushLexicalScope(context, &tokenStream, tc, &blockInfo);
                if (!pnlet)
                    return NULL;
                blockInfo.flags |= SIF_FOR_BLOCK;
                pn1 = variables(false);
            }
#endif
        } else {
            pn1 = expr();
        }
        tc->flags &= ~TCF_IN_FOR_INIT;
        if (!pn1)
            return NULL;
    }

    





    ParseNode *pn2, *pn3;
    ParseNode *pn4 = TernaryNode::create(tc);
    if (!pn4)
        return NULL;
    if (pn1 && tokenStream.matchToken(TOK_IN)) {
        






        pn->pn_iflags |= JSITER_ENUMERATE;
        stmtInfo.type = STMT_FOR_IN_LOOP;

        
        JS_ASSERT(!TokenKindIsDecl(tt) || pn1->isKind(tt));
        if (TokenKindIsDecl(tt)
            ? (pn1->pn_count > 1 || pn1->isOp(JSOP_DEFCONST)
#if JS_HAS_DESTRUCTURING
               || (versionNumber() == JSVERSION_1_7 &&
                   pn->isOp(JSOP_ITER) &&
                   !(pn->pn_iflags & JSITER_FOREACH) &&
                   (pn1->pn_head->isKind(TOK_RC) ||
                    (pn1->pn_head->isKind(TOK_RB) &&
                     pn1->pn_head->pn_count != 2) ||
                    (pn1->pn_head->isKind(TOK_ASSIGN) &&
                     (!pn1->pn_head->pn_left->isKind(TOK_RB) ||
                      pn1->pn_head->pn_left->pn_count != 2))))
#endif
              )
            : (!pn1->isKind(TOK_NAME) &&
               !pn1->isKind(TOK_DOT) &&
#if JS_HAS_DESTRUCTURING
               ((versionNumber() == JSVERSION_1_7 &&
                 pn->isOp(JSOP_ITER) &&
                 !(pn->pn_iflags & JSITER_FOREACH))
                ? (!pn1->isKind(TOK_RB) || pn1->pn_count != 2)
                : (!pn1->isKind(TOK_RB) && !pn1->isKind(TOK_RC))) &&
#endif
               !pn1->isKind(TOK_LP) &&
#if JS_HAS_XML_SUPPORT
               (!pn1->isKind(TOK_UNARYOP) ||
                !pn1->isOp(JSOP_XMLNAME)) &&
#endif
               !pn1->isKind(TOK_LB))) {
            reportErrorNumber(pn1, JSREPORT_ERROR, JSMSG_BAD_FOR_LEFTSIDE);
            return NULL;
        }

        





        pn2 = NULL;
        uintN dflag = PND_ASSIGNED;
        if (TokenKindIsDecl(tt)) {
            
            pn1->pn_xflags |= PNX_FORINVAR;

            pn2 = pn1->pn_head;
            if ((pn2->isKind(TOK_NAME) && pn2->maybeExpr())
#if JS_HAS_DESTRUCTURING
                || pn2->isKind(TOK_ASSIGN)
#endif
                ) {
                






#if JS_HAS_BLOCK_SCOPE
                if (tt == TOK_LET) {
                    reportErrorNumber(pn2, JSREPORT_ERROR, JSMSG_INVALID_FOR_IN_INIT);
                    return NULL;
                }
#endif 

                pnseq = ListNode::create(tc);
                if (!pnseq)
                    return NULL;
                pnseq->setKind(TOK_SEQ);
                pnseq->pn_pos.begin = pn->pn_pos.begin;

                dflag = PND_INITIALIZED;

                







                pn1->pn_xflags &= ~PNX_FORINVAR;
                pn1->pn_xflags |= PNX_POPVAR;
                pnseq->initList(pn1);

#if JS_HAS_DESTRUCTURING
                if (pn2->isKind(TOK_ASSIGN)) {
                    pn2 = pn2->pn_left;
                    JS_ASSERT(pn2->isKind(TOK_RB) || pn2->isKind(TOK_RC) ||
                              pn2->isKind(TOK_NAME));
                }
#endif
                pn1 = NULL;
            }

            



            pn2 = CloneLeftHandSide(pn2, tc);
            if (!pn2)
                return NULL;
        } else {
            
            pn2 = pn1;
            pn1 = NULL;

            if (!setAssignmentLhsOps(pn2, JSOP_NOP))
                return NULL;
        }

        switch (pn2->getKind()) {
          case TOK_NAME:
            
            NoteLValue(context, pn2, tc, dflag);
            break;

#if JS_HAS_DESTRUCTURING
          case TOK_ASSIGN:
            JS_NOT_REACHED("forStatement TOK_ASSIGN");
            break;

          case TOK_RB:
          case TOK_RC:
            if (versionNumber() == JSVERSION_1_7) {
                



                JS_ASSERT(pn->isOp(JSOP_ITER));
                if (!(pn->pn_iflags & JSITER_FOREACH))
                    pn->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          default:;
        }

        




#if JS_HAS_BLOCK_SCOPE
        StmtInfo *save = tc->topStmt;
        if (let)
            tc->topStmt = save->down;
#endif
        pn3 = expr();
        if (!pn3)
            return NULL;
#if JS_HAS_BLOCK_SCOPE
        if (let)
            tc->topStmt = save;
#endif

        pn4->setKind(TOK_IN);
    } else {
        if (pn->pn_iflags & JSITER_FOREACH) {
            reportErrorNumber(pn, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
            return NULL;
        }
        pn->setOp(JSOP_NOP);

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            pn2 = NULL;
        } else {
            pn2 = expr();
            if (!pn2)
                return NULL;
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_RP) {
            pn3 = NULL;
        } else {
            pn3 = expr();
            if (!pn3)
                return NULL;
        }

        pn4->setKind(TOK_FORHEAD);
    }
    pn4->setOp(JSOP_NOP);
    pn4->pn_kid1 = pn1;
    pn4->pn_kid2 = pn2;
    pn4->pn_kid3 = pn3;
    pn->pn_left = pn4;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    
    pn2 = statement();
    if (!pn2)
        return NULL;
    pn->pn_right = pn2;

    
    pn->pn_pos.end = pn2->pn_pos.end;

#if JS_HAS_BLOCK_SCOPE
    if (pnlet) {
        PopStatement(tc);
        pnlet->pn_expr = pn;
        pn = pnlet;
    }
#endif
    if (pnseq) {
        pnseq->pn_pos.end = pn->pn_pos.end;
        pnseq->append(pn);
        pn = pnseq;
    }
    PopStatement(tc);
    return pn;
}

ParseNode *
Parser::tryStatement()
{
    ParseNode *catchList, *lastCatch;

    
















    ParseNode *pn = TernaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_NOP);

    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    StmtInfo stmtInfo;
    if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, tc))
        return NULL;
    pn->pn_kid1 = statements();
    if (!pn->pn_kid1)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatement(tc);

    catchList = NULL;
    TokenKind tt = tokenStream.getToken();
    if (tt == TOK_CATCH) {
        catchList = ListNode::create(tc);
        if (!catchList)
            return NULL;
        catchList->setKind(TOK_RESERVED);
        catchList->makeEmpty();
        lastCatch = NULL;

        do {
            ParseNode *pnblock;
            BindData data;

            
            if (lastCatch && !lastCatch->pn_kid2) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_AFTER_GENERAL);
                return NULL;
            }

            



            pnblock = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
            if (!pnblock)
                return NULL;
            stmtInfo.type = STMT_CATCH;

            






            ParseNode *pn2 = TernaryNode::create(tc);
            if (!pn2)
                return NULL;
            pnblock->pn_expr = pn2;
            MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_CATCH);

            




            data.pn = NULL;
            data.op = JSOP_NOP;
            data.binder = BindLet;
            data.let.overflow = JSMSG_TOO_MANY_CATCH_VARS;

            tt = tokenStream.getToken();
            ParseNode *pn3;
            switch (tt) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
                pn3 = destructuringExpr(&data, tt);
                if (!pn3)
                    return NULL;
                break;
#endif

              case TOK_NAME:
              {
                JSAtom *label = tokenStream.currentToken().name();
                pn3 = NewBindingNode(label, tc, true);
                if (!pn3)
                    return NULL;
                data.pn = pn3;
                if (!data.binder(context, &data, label, tc))
                    return NULL;
                break;
              }

              default:
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_IDENTIFIER);
                return NULL;
            }

            pn2->pn_kid1 = pn3;
#if JS_HAS_CATCH_GUARD
            




            if (tokenStream.matchToken(TOK_IF)) {
                pn2->pn_kid2 = expr();
                if (!pn2->pn_kid2)
                    return NULL;
            }
#endif
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_CATCH);

            MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_CATCH);
            pn2->pn_kid3 = statements();
            if (!pn2->pn_kid3)
                return NULL;
            MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_CATCH);
            PopStatement(tc);

            catchList->append(pnblock);
            lastCatch = pn2;
            tt = tokenStream.getToken(TSF_OPERAND);
        } while (tt == TOK_CATCH);
    }
    pn->pn_kid2 = catchList;

    if (tt == TOK_FINALLY) {
        MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_FINALLY);
        if (!PushBlocklikeStatement(&stmtInfo, STMT_FINALLY, tc))
            return NULL;
        pn->pn_kid3 = statements();
        if (!pn->pn_kid3)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_FINALLY);
        PopStatement(tc);
    } else {
        tokenStream.ungetToken();
    }
    if (!catchList && !pn->pn_kid3) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_OR_FINALLY);
        return NULL;
    }
    return pn;
}

ParseNode *
Parser::withStatement()
{
    







    if (tc->flags & TCF_STRICT_MODE_CODE) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_STRICT_CODE_WITH);
        return NULL;
    }

    ParseNode *pn = BinaryNode::create(tc);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_WITH);
    ParseNode *pn2 = parenExpr();
    if (!pn2)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_WITH);
    pn->pn_left = pn2;

    ParseNode *oldWith = tc->innermostWith;
    tc->innermostWith = pn;

    StmtInfo stmtInfo;
    PushStatement(tc, &stmtInfo, STMT_WITH, -1);
    pn2 = statement();
    if (!pn2)
        return NULL;
    PopStatement(tc);

    pn->pn_pos.end = pn2->pn_pos.end;
    pn->pn_right = pn2;
    tc->flags |= TCF_FUN_HEAVYWEIGHT;
    tc->innermostWith = oldWith;

    



    for (AtomDefnRange r = tc->lexdeps->all(); !r.empty(); r.popFront()) {
        Definition *defn = r.front().value();
        Definition *lexdep = defn->resolve();
        DeoptimizeUsesWithin(lexdep, pn->pn_pos);
    }

    return pn;
}

#if JS_HAS_BLOCK_SCOPE
ParseNode *
Parser::letStatement()
{
    ParseNode *pn;
    do {
        
        if (tokenStream.peekToken() == TOK_LP) {
            pn = letBlock(JS_TRUE);
            if (!pn || pn->isOp(JSOP_LEAVEBLOCK))
                return pn;

            
            JS_ASSERT(pn->isKind(TOK_SEMI) || pn->isOp(JSOP_LEAVEBLOCKEXPR));
            break;
        }

        










        StmtInfo *stmt = tc->topStmt;
        if (stmt &&
            (!STMT_MAYBE_SCOPE(stmt) || (stmt->flags & SIF_FOR_BLOCK))) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LET_DECL_NOT_IN_BLOCK);
            return NULL;
        }

        if (stmt && (stmt->flags & SIF_SCOPE)) {
            JS_ASSERT(tc->blockChainBox == stmt->blockBox);
        } else {
            if (!stmt || (stmt->flags & SIF_BODY_BLOCK)) {
                



                tokenStream.mungeCurrentToken(TOK_VAR, JSOP_DEFVAR);

                pn = variables(false);
                if (!pn)
                    return NULL;
                pn->pn_xflags |= PNX_POPVAR;
                break;
            }

            




            JS_ASSERT(!(stmt->flags & SIF_SCOPE));
            JS_ASSERT(stmt != tc->topScopeStmt);
            JS_ASSERT(stmt->type == STMT_BLOCK ||
                      stmt->type == STMT_SWITCH ||
                      stmt->type == STMT_TRY ||
                      stmt->type == STMT_FINALLY);
            JS_ASSERT(!stmt->downScope);

            
            JSObject *obj = js_NewBlockObject(tc->parser->context);
            if (!obj)
                return NULL;

            ObjectBox *blockbox = tc->parser->newObjectBox(obj);
            if (!blockbox)
                return NULL;

            





            stmt->flags |= SIF_SCOPE;
            stmt->downScope = tc->topScopeStmt;
            tc->topScopeStmt = stmt;

            obj->setParent(tc->blockChain());
            blockbox->parent = tc->blockChainBox;
            tc->blockChainBox = blockbox;
            stmt->blockBox = blockbox;

#ifdef DEBUG
            ParseNode *tmp = tc->blockNode;
            JS_ASSERT(!tmp || !tmp->isKind(TOK_LEXICALSCOPE));
#endif

            
            ParseNode *pn1 = LexicalScopeNode::create(tc);
            if (!pn1)
                return NULL;

            pn1->setKind(TOK_LEXICALSCOPE);
            pn1->setOp(JSOP_LEAVEBLOCK);
            pn1->pn_pos = tc->blockNode->pn_pos;
            pn1->pn_objbox = blockbox;
            pn1->pn_expr = tc->blockNode;
            pn1->pn_blockid = tc->blockNode->pn_blockid;
            tc->blockNode = pn1;
        }

        pn = variables(false);
        if (!pn)
            return NULL;
        pn->pn_xflags = PNX_POPVAR;
    } while (0);

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}
#endif

ParseNode *
Parser::expressionStatement()
{
    tokenStream.ungetToken();
    ParseNode *pn2 = expr();
    if (!pn2)
        return NULL;

    if (tokenStream.peekToken() == TOK_COLON) {
        if (!pn2->isKind(TOK_NAME)) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_LABEL);
            return NULL;
        }
        JSAtom *label = pn2->pn_atom;
        for (StmtInfo *stmt = tc->topStmt; stmt; stmt = stmt->down) {
            if (stmt->type == STMT_LABEL && stmt->label == label) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_DUPLICATE_LABEL);
                return NULL;
            }
        }
        ForgetUse(pn2);

        (void) tokenStream.getToken();

        
        StmtInfo stmtInfo;
        PushStatement(tc, &stmtInfo, STMT_LABEL, -1);
        stmtInfo.label = label;
        ParseNode *pn = statement();
        if (!pn)
            return NULL;

        
        if (pn->isKind(TOK_SEMI) && !pn->pn_kid) {
            pn->setKind(TOK_LC);
            pn->setArity(PN_LIST);
            pn->makeEmpty();
        }

        
        PopStatement(tc);
        pn2->setKind(TOK_COLON);
        pn2->pn_pos.end = pn->pn_pos.end;
        pn2->pn_expr = pn;
        return pn2;
    }

    ParseNode *pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->setKind(TOK_SEMI);
    pn->pn_pos = pn2->pn_pos;
    pn->pn_kid = pn2;

    switch (pn2->getKind()) {
      case TOK_LP:
        



        if (pn2->pn_head->isKind(TOK_FUNCTION) &&
            !pn2->pn_head->pn_funbox->node->isFunArg()) {
            pn2->pn_head->pn_funbox->tcflags |= TCF_FUN_MODULE_PATTERN;
        }
        break;
      case TOK_ASSIGN:
        




        if (tc->funbox &&
            pn2->isOp(JSOP_NOP) &&
            pn2->pn_left->isOp(JSOP_SETPROP) &&
            pn2->pn_left->pn_expr->isOp(JSOP_THIS) &&
            pn2->pn_right->isOp(JSOP_LAMBDA)) {
            JS_ASSERT(!pn2->isDefn());
            JS_ASSERT(!pn2->isUsed());
            pn2->pn_right->pn_link = tc->funbox->methods;
            tc->funbox->methods = pn2->pn_right;
        }
        break;
      default:;
    }

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}

ParseNode *
Parser::statement()
{
    ParseNode *pn;

    JS_CHECK_RECURSION(context, return NULL);

    switch (tokenStream.getToken(TSF_OPERAND)) {
      case TOK_FUNCTION:
      {
#if JS_HAS_XML_SUPPORT
        TokenKind tt = tokenStream.peekToken(TSF_KEYWORD_IS_NAME);
        if (tt == TOK_DBLCOLON)
            goto expression;
#endif
        return functionStmt();
      }

      case TOK_IF:
      {
        
        pn = TernaryNode::create(tc);
        if (!pn)
            return NULL;
        ParseNode *pn1 = condition();
        if (!pn1)
            return NULL;
        StmtInfo stmtInfo;
        PushStatement(tc, &stmtInfo, STMT_IF, -1);
        ParseNode *pn2 = statement();
        if (!pn2)
            return NULL;
        ParseNode *pn3;
        if (tokenStream.matchToken(TOK_ELSE, TSF_OPERAND)) {
            stmtInfo.type = STMT_ELSE;
            pn3 = statement();
            if (!pn3)
                return NULL;
            pn->pn_pos.end = pn3->pn_pos.end;
        } else {
            pn3 = NULL;
            pn->pn_pos.end = pn2->pn_pos.end;
        }
        PopStatement(tc);
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
        return pn;
      }

      case TOK_SWITCH:
        return switchStatement();

      case TOK_WHILE:
      {
        pn = BinaryNode::create(tc);
        if (!pn)
            return NULL;
        StmtInfo stmtInfo;
        PushStatement(tc, &stmtInfo, STMT_WHILE_LOOP, -1);
        ParseNode *pn2 = condition();
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        ParseNode *pn3 = statement();
        if (!pn3)
            return NULL;
        PopStatement(tc);
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_right = pn3;
        return pn;
      }

      case TOK_DO:
      {
        pn = BinaryNode::create(tc);
        if (!pn)
            return NULL;
        StmtInfo stmtInfo;
        PushStatement(tc, &stmtInfo, STMT_DO_LOOP, -1);
        ParseNode *pn2 = statement();
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        MUST_MATCH_TOKEN(TOK_WHILE, JSMSG_WHILE_AFTER_DO);
        ParseNode *pn3 = condition();
        if (!pn3)
            return NULL;
        PopStatement(tc);
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_right = pn3;
        if (versionNumber() != JSVERSION_ECMA_3) {
            




            (void) tokenStream.matchToken(TOK_SEMI);
            return pn;
        }
        break;
      }

      case TOK_FOR:
        return forStatement();

      case TOK_TRY:
        return tryStatement();

      case TOK_THROW:
      {
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;

        
        TokenKind tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
        if (tt == TOK_ERROR)
            return NULL;
        if (tt == TOK_EOF || tt == TOK_EOL || tt == TOK_SEMI || tt == TOK_RC) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            return NULL;
        }

        ParseNode *pn2 = expr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->setOp(JSOP_THROW);
        pn->pn_kid = pn2;
        break;
      }

      
      case TOK_CATCH:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_WITHOUT_TRY);
        return NULL;

      case TOK_FINALLY:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_FINALLY_WITHOUT_TRY);
        return NULL;

      case TOK_BREAK:
      {
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!MatchLabel(context, &tokenStream, pn))
            return NULL;
        StmtInfo *stmt = tc->topStmt;
        JSAtom *label = pn->pn_atom;
        if (label) {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL && stmt->label == label)
                    break;
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOUGH_BREAK);
                    return NULL;
                }
                if (STMT_IS_LOOP(stmt) || stmt->type == STMT_SWITCH)
                    break;
            }
        }
        if (label)
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
        break;
      }

      case TOK_CONTINUE:
      {
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!MatchLabel(context, &tokenStream, pn))
            return NULL;
        StmtInfo *stmt = tc->topStmt;
        JSAtom *label = pn->pn_atom;
        if (label) {
            for (StmtInfo *stmt2 = NULL; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL) {
                    if (stmt->label == label) {
                        if (!stmt2 || !STMT_IS_LOOP(stmt2)) {
                            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_CONTINUE);
                            return NULL;
                        }
                        break;
                    }
                } else {
                    stmt2 = stmt;
                }
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_CONTINUE);
                    return NULL;
                }
                if (STMT_IS_LOOP(stmt))
                    break;
            }
        }
        if (label)
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
        break;
      }

      case TOK_WITH:
        return withStatement();

      case TOK_VAR:
        pn = variables(false);
        if (!pn)
            return NULL;

        
        pn->pn_xflags |= PNX_POPVAR;
        break;

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        return letStatement();
#endif 

      case TOK_RETURN:
        pn = returnOrYield(false);
        if (!pn)
            return NULL;
        break;

      case TOK_LC:
      {
        uintN oldflags;

        oldflags = tc->flags;
        tc->flags = oldflags & ~TCF_HAS_FUNCTION_STMT;
        StmtInfo stmtInfo;
        if (!PushBlocklikeStatement(&stmtInfo, STMT_BLOCK, tc))
            return NULL;
        pn = statements();
        if (!pn)
            return NULL;

        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_COMPOUND);
        PopStatement(tc);

        



        if ((tc->flags & TCF_HAS_FUNCTION_STMT) &&
            (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)) {
            pn->pn_xflags |= PNX_NEEDBRACES;
        }
        tc->flags = oldflags | (tc->flags & (TCF_FUN_FLAGS | TCF_RETURN_FLAGS));
        return pn;
      }

      case TOK_SEMI:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->setKind(TOK_SEMI);
        return pn;

      case TOK_DEBUGGER:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->setKind(TOK_DEBUGGER);
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_DEFAULT:
      {
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().name() != context->runtime->atomState.xmlAtom ||
            !tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().name() != context->runtime->atomState.namespaceAtom ||
            !tokenStream.matchToken(TOK_ASSIGN) ||
            tokenStream.currentToken().t_op != JSOP_NOP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_DEFAULT_XML_NAMESPACE);
            return NULL;
        }

        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        ParseNode *pn2 = expr();
        if (!pn2)
            return NULL;
        pn->setOp(JSOP_DEFXMLNS);
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
        break;
      }
#endif

      case TOK_ERROR:
        return NULL;

      default:
#if JS_HAS_XML_SUPPORT
      expression:
#endif
        return expressionStatement();
    }

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}

ParseNode *
Parser::variables(bool inLetHead)
{
    TokenKind tt;
    bool let;
    StmtInfo *scopeStmt;
    BindData data;
    ParseNode *pn, *pn2;

    





    tt = tokenStream.currentToken().type;
    let = (tt == TOK_LET || tt == TOK_LP);
    JS_ASSERT(let || tt == TOK_VAR);

#if JS_HAS_BLOCK_SCOPE
    bool popScope = (inLetHead || (let && (tc->flags & TCF_IN_FOR_INIT)));
    StmtInfo *save = tc->topStmt, *saveScope = tc->topScopeStmt;
#endif

    
    scopeStmt = tc->topScopeStmt;
    if (let) {
        while (scopeStmt && !(scopeStmt->flags & SIF_SCOPE)) {
            JS_ASSERT(!STMT_MAYBE_SCOPE(scopeStmt));
            scopeStmt = scopeStmt->downScope;
        }
        JS_ASSERT(scopeStmt);
    }

    data.op = let ? JSOP_NOP : tokenStream.currentToken().t_op;
    pn = ListNode::create(tc);
    if (!pn)
        return NULL;
    pn->setOp(data.op);
    pn->makeEmpty();

    




    if (let) {
        JS_ASSERT(tc->blockChainBox == scopeStmt->blockBox);
        data.binder = BindLet;
        data.let.overflow = JSMSG_TOO_MANY_LOCALS;
    } else {
        data.binder = BindVarOrConst;
    }

    do {
        tt = tokenStream.getToken();
#if JS_HAS_DESTRUCTURING
        if (tt == TOK_LB || tt == TOK_LC) {
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn2 = primaryExpr(tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn2)
                return NULL;

            if (!CheckDestructuring(context, &data, pn2, tc))
                return NULL;
            if ((tc->flags & TCF_IN_FOR_INIT) && tokenStream.peekToken() == TOK_IN) {
                pn->append(pn2);
                continue;
            }

            MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);
            if (tokenStream.currentToken().t_op != JSOP_NOP)
                goto bad_var_init;

#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save->down;
                tc->topScopeStmt = saveScope->downScope;
            }
#endif
            ParseNode *init = assignExpr();
#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save;
                tc->topScopeStmt = saveScope;
            }
#endif

            if (!init)
                return NULL;
            UndominateInitializers(pn2, init->pn_pos.end, tc);

            pn2 = ParseNode::newBinaryOrAppend(TOK_ASSIGN, JSOP_NOP, pn2, init, tc);
            if (!pn2)
                return NULL;
            pn->append(pn2);
            continue;
        }
#endif 

        if (tt != TOK_NAME) {
            if (tt != TOK_ERROR)
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NO_VARIABLE_NAME);
            return NULL;
        }

        PropertyName *name = tokenStream.currentToken().name();
        pn2 = NewBindingNode(name, tc, let);
        if (!pn2)
            return NULL;
        if (data.op == JSOP_DEFCONST)
            pn2->pn_dflags |= PND_CONST;
        data.pn = pn2;
        if (!data.binder(context, &data, name, tc))
            return NULL;
        pn->append(pn2);

        if (tokenStream.matchToken(TOK_ASSIGN)) {
            if (tokenStream.currentToken().t_op != JSOP_NOP)
                goto bad_var_init;

#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save->down;
                tc->topScopeStmt = saveScope->downScope;
            }
#endif
            ParseNode *init = assignExpr();
#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save;
                tc->topScopeStmt = saveScope;
            }
#endif
            if (!init)
                return NULL;

            if (pn2->isUsed()) {
                pn2 = MakeAssignment(pn2, init, tc);
                if (!pn2)
                    return NULL;
            } else {
                pn2->pn_expr = init;
            }

            JS_ASSERT_IF(pn2->pn_dflags & PND_GVAR, !(pn2->pn_dflags & PND_BOUND));

            pn2->setOp(pn2->isOp(JSOP_ARGUMENTS)
                       ? JSOP_SETNAME
                       : (pn2->pn_dflags & PND_BOUND)
                       ? JSOP_SETLOCAL
                       : (data.op == JSOP_DEFCONST)
                       ? JSOP_SETCONST
                       : JSOP_SETNAME);

            NoteLValue(context, pn2, tc, data.fresh ? PND_INITIALIZED : PND_ASSIGNED);

            
            pn2->pn_pos.end = init->pn_pos.end;

            if (tc->inFunction() && name == context->runtime->atomState.argumentsAtom) {
                tc->noteArgumentsUse(pn2);
                if (!let)
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
            }
        }
    } while (tokenStream.matchToken(TOK_COMMA));

    pn->pn_pos.end = pn->last()->pn_pos.end;
    return pn;

bad_var_init:
    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_VAR_INIT);
    return NULL;
}

ParseNode *
Parser::expr()
{
    ParseNode *pn = assignExpr();
    if (pn && tokenStream.matchToken(TOK_COMMA)) {
        ParseNode *pn2 = ListNode::create(tc);
        if (!pn2)
            return NULL;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->initList(pn);
        pn = pn2;
        do {
#if JS_HAS_GENERATORS
            pn2 = pn->last();
            if (pn2->isKind(TOK_YIELD) && !pn2->isInParens()) {
                reportErrorNumber(pn2, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
                return NULL;
            }
#endif
            pn2 = assignExpr();
            if (!pn2)
                return NULL;
            pn->append(pn2);
        } while (tokenStream.matchToken(TOK_COMMA));
        pn->pn_pos.end = pn->last()->pn_pos.end;
    }
    return pn;
}







#define BEGIN_EXPR_PARSER(name)                                               \
    JS_ALWAYS_INLINE ParseNode *                                              \
    Parser::name##i()

#define END_EXPR_PARSER(name)                                                 \
    JS_NEVER_INLINE ParseNode *                                               \
    Parser::name##n() {                                                       \
        return name##i();                                                     \
    }

BEGIN_EXPR_PARSER(mulExpr1)
{
    TokenKind tt;
    ParseNode *pn = unaryExpr();

    




    while (pn && ((tt = tokenStream.getToken()) == TOK_STAR || tt == TOK_DIVOP)) {
        tt = tokenStream.currentToken().type;
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(tt, op, pn, unaryExpr(), tc);
    }
    return pn;
}
END_EXPR_PARSER(mulExpr1)

BEGIN_EXPR_PARSER(addExpr1)
{
    ParseNode *pn = mulExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_PLUS, TOK_MINUS)) {
        TokenKind tt = tokenStream.currentToken().type;
        JSOp op = (tt == TOK_PLUS) ? JSOP_ADD : JSOP_SUB;
        pn = ParseNode::newBinaryOrAppend(tt, op, pn, mulExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(addExpr1)

BEGIN_EXPR_PARSER(shiftExpr1)
{
    ParseNode *pn = addExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_SHOP)) {
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(TOK_SHOP, op, pn, addExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(shiftExpr1)

BEGIN_EXPR_PARSER(relExpr1)
{
    uintN inForInitFlag = tc->flags & TCF_IN_FOR_INIT;

    



    tc->flags &= ~TCF_IN_FOR_INIT;

    ParseNode *pn = shiftExpr1i();
    while (pn &&
           (tokenStream.isCurrentTokenType(TOK_RELOP) ||
            



            (inForInitFlag == 0 && tokenStream.isCurrentTokenType(TOK_IN)) ||
            tokenStream.isCurrentTokenType(TOK_INSTANCEOF))) {
        TokenKind tt = tokenStream.currentToken().type;
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(tt, op, pn, shiftExpr1n(), tc);
    }
    
    tc->flags |= inForInitFlag;

    return pn;
}
END_EXPR_PARSER(relExpr1)

BEGIN_EXPR_PARSER(eqExpr1)
{
    ParseNode *pn = relExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_EQOP)) {
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(TOK_EQOP, op, pn, relExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(eqExpr1)

BEGIN_EXPR_PARSER(bitAndExpr1)
{
    ParseNode *pn = eqExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITAND))
        pn = ParseNode::newBinaryOrAppend(TOK_BITAND, JSOP_BITAND, pn, eqExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitAndExpr1)

BEGIN_EXPR_PARSER(bitXorExpr1)
{
    ParseNode *pn = bitAndExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITXOR))
        pn = ParseNode::newBinaryOrAppend(TOK_BITXOR, JSOP_BITXOR, pn, bitAndExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitXorExpr1)

BEGIN_EXPR_PARSER(bitOrExpr1)
{
    ParseNode *pn = bitXorExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITOR))
        pn = ParseNode::newBinaryOrAppend(TOK_BITOR, JSOP_BITOR, pn, bitXorExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitOrExpr1)

BEGIN_EXPR_PARSER(andExpr1)
{
    ParseNode *pn = bitOrExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_AND))
        pn = ParseNode::newBinaryOrAppend(TOK_AND, JSOP_AND, pn, bitOrExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(andExpr1)

JS_ALWAYS_INLINE ParseNode *
Parser::orExpr1()
{
    ParseNode *pn = andExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_OR))
        pn = ParseNode::newBinaryOrAppend(TOK_OR, JSOP_OR, pn, andExpr1n(), tc);
    return pn;
}

JS_ALWAYS_INLINE ParseNode *
Parser::condExpr1()
{
    ParseNode *pn = orExpr1();
    if (pn && tokenStream.isCurrentTokenType(TOK_HOOK)) {
        ParseNode *pn1 = pn;
        pn = TernaryNode::create(tc);
        if (!pn)
            return NULL;

        




        uintN oldflags = tc->flags;
        tc->flags &= ~TCF_IN_FOR_INIT;
        ParseNode *pn2 = assignExpr();
        tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);

        if (!pn2)
            return NULL;
        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_IN_COND);
        ParseNode *pn3 = assignExpr();
        if (!pn3)
            return NULL;
        pn->pn_pos.begin = pn1->pn_pos.begin;
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
        tokenStream.getToken();     
    }
    return pn;
}

bool
Parser::setAssignmentLhsOps(ParseNode *pn, JSOp op)
{
    switch (pn->getKind()) {
      case TOK_NAME:
        if (!CheckStrictAssignment(context, tc, pn))
            return false;
        pn->setOp(pn->isOp(JSOP_GETLOCAL) ? JSOP_SETLOCAL : JSOP_SETNAME);
        NoteLValue(context, pn, tc);
        break;
      case TOK_DOT:
        pn->setOp(JSOP_SETPROP);
        break;
      case TOK_LB:
        pn->setOp(JSOP_SETELEM);
        break;
#if JS_HAS_DESTRUCTURING
      case TOK_RB:
      case TOK_RC:
        if (op != JSOP_NOP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_DESTRUCT_ASS);
            return false;
        }
        if (!CheckDestructuring(context, NULL, pn, tc))
            return false;
        break;
#endif
      case TOK_LP:
        if (!MakeSetCall(context, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;
#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        if (pn->isOp(JSOP_XMLNAME)) {
            pn->setOp(JSOP_SETXMLNAME);
            break;
        }
        
#endif
      default:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return false;
    }
    return true;
}

ParseNode *
Parser::assignExpr()
{
    JS_CHECK_RECURSION(context, return NULL);

#if JS_HAS_GENERATORS
    if (tokenStream.matchToken(TOK_YIELD, TSF_OPERAND))
        return returnOrYield(true);
#endif

    ParseNode *pn = condExpr1();
    if (!pn)
        return NULL;

    if (!tokenStream.isCurrentTokenType(TOK_ASSIGN)) {
        tokenStream.ungetToken();
        return pn;
    }

    JSOp op = tokenStream.currentToken().t_op;
    if (!setAssignmentLhsOps(pn, op))
        return NULL;

    ParseNode *rhs = assignExpr();
    if (!rhs)
        return NULL;
    if (pn->isKind(TOK_NAME) && pn->isUsed()) {
        Definition *dn = pn->pn_lexdef;

        






        if (!dn->isAssigned()) {
            JS_ASSERT(dn->isInitialized());
            dn->pn_pos.end = rhs->pn_pos.end;
        }
    }

    return ParseNode::newBinaryOrAppend(TOK_ASSIGN, op, pn, rhs, tc);
}

static ParseNode *
SetLvalKid(JSContext *cx, TokenStream *ts, TreeContext *tc, ParseNode *pn, ParseNode *kid,
           const char *name)
{
    if (!kid->isKind(TOK_NAME) &&
        !kid->isKind(TOK_DOT) &&
        (!kid->isKind(TOK_LP) ||
         (!kid->isOp(JSOP_CALL) && !kid->isOp(JSOP_EVAL) &&
          !kid->isOp(JSOP_FUNCALL) && !kid->isOp(JSOP_FUNAPPLY))) &&
#if JS_HAS_XML_SUPPORT
        (!kid->isKind(TOK_UNARYOP) || !kid->isOp(JSOP_XMLNAME)) &&
#endif
        !kid->isKind(TOK_LB)) {
        ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR, JSMSG_BAD_OPERAND, name);
        return NULL;
    }
    if (!CheckStrictAssignment(cx, tc, kid))
        return NULL;
    pn->pn_kid = kid;
    return kid;
}

static const char incop_name_str[][10] = {"increment", "decrement"};

static JSBool
SetIncOpKid(JSContext *cx, TokenStream *ts, TreeContext *tc, ParseNode *pn, ParseNode *kid,
            TokenKind tt, JSBool preorder)
{
    JSOp op;

    kid = SetLvalKid(cx, ts, tc, pn, kid, incop_name_str[tt == TOK_DEC]);
    if (!kid)
        return JS_FALSE;
    switch (kid->getKind()) {
      case TOK_NAME:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCNAME : JSOP_NAMEINC)
             : (preorder ? JSOP_DECNAME : JSOP_NAMEDEC);
        NoteLValue(cx, kid, tc);
        break;

      case TOK_DOT:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCPROP : JSOP_PROPINC)
             : (preorder ? JSOP_DECPROP : JSOP_PROPDEC);
        break;

      case TOK_LP:
        if (!MakeSetCall(cx, kid, tc, JSMSG_BAD_INCOP_OPERAND))
            return JS_FALSE;
        
#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        if (kid->isOp(JSOP_XMLNAME))
            kid->setOp(JSOP_SETXMLNAME);
        
#endif
      case TOK_LB:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCELEM : JSOP_ELEMINC)
             : (preorder ? JSOP_DECELEM : JSOP_ELEMDEC);
        break;

      default:
        JS_ASSERT(0);
        op = JSOP_NOP;
    }
    pn->setOp(op);
    return JS_TRUE;
}

ParseNode *
Parser::unaryExpr()
{
    ParseNode *pn, *pn2;

    JS_CHECK_RECURSION(context, return NULL);

    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    switch (tt) {
      case TOK_UNARYOP:
      case TOK_PLUS:
      case TOK_MINUS:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->setKind(TOK_UNARYOP);      
        pn->setOp(tokenStream.currentToken().t_op);
        pn2 = unaryExpr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
        break;

      case TOK_INC:
      case TOK_DEC:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn2 = memberExpr(JS_TRUE);
        if (!pn2)
            return NULL;
        if (!SetIncOpKid(context, &tokenStream, tc, pn, pn2, tt, JS_TRUE))
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        break;

      case TOK_DELETE:
      {
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn2 = unaryExpr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;

        




        if (foldConstants && !FoldConstants(context, pn2, tc))
            return NULL;
        switch (pn2->getKind()) {
          case TOK_LP:
            if (!(pn2->pn_xflags & PNX_SETCALL)) {
                



                if (!MakeSetCall(context, pn2, tc, JSMSG_BAD_DELETE_OPERAND))
                    return NULL;
                pn2->pn_xflags &= ~PNX_SETCALL;
            }
            break;
          case TOK_NAME:
            if (!ReportStrictModeError(context, &tokenStream, tc, pn,
                                       JSMSG_DEPRECATED_DELETE_OPERAND)) {
                return NULL;
            }
            pn2->setOp(JSOP_DELNAME);
            if (pn2->pn_atom == context->runtime->atomState.argumentsAtom) {
                tc->flags |= TCF_FUN_HEAVYWEIGHT;
                tc->countArgumentsUse(pn2);
            }
            break;
          default:;
        }
        pn->pn_kid = pn2;
        break;
      }
      case TOK_ERROR:
        return NULL;

      default:
        tokenStream.ungetToken();
        pn = memberExpr(JS_TRUE);
        if (!pn)
            return NULL;

        
        if (tokenStream.onCurrentLine(pn->pn_pos)) {
            tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
            if (tt == TOK_INC || tt == TOK_DEC) {
                (void) tokenStream.getToken();
                pn2 = UnaryNode::create(tc);
                if (!pn2)
                    return NULL;
                if (!SetIncOpKid(context, &tokenStream, tc, pn2, pn, tt, JS_FALSE))
                    return NULL;
                pn2->pn_pos.begin = pn->pn_pos.begin;
                pn = pn2;
            }
        }
        break;
    }
    return pn;
}

#if JS_HAS_GENERATORS






















class CompExprTransplanter {
    ParseNode       *root;
    TreeContext     *tc;
    bool            genexp;
    uintN           adjust;
    uintN           funcLevel;

  public:
    CompExprTransplanter(ParseNode *pn, TreeContext *tc, bool ge, uintN adj)
      : root(pn), tc(tc), genexp(ge), adjust(adj), funcLevel(0)
    {
    }

    bool transplant(ParseNode *pn);
};


















class GenexpGuard {
    TreeContext     *tc;
    uint32          startYieldCount;
    uint32          startArgumentsCount;

  public:
    explicit GenexpGuard(TreeContext *tc)
      : tc(tc)
    {
        if (tc->parenDepth == 0) {
            tc->yieldCount = tc->argumentsCount = 0;
            tc->yieldNode = tc->argumentsNode = NULL;
        }
        startYieldCount = tc->yieldCount;
        startArgumentsCount = tc->argumentsCount;
        tc->parenDepth++;
    }

    void endBody();
    bool checkValidBody(ParseNode *pn);
    bool maybeNoteGenerator(ParseNode *pn);
};

void
GenexpGuard::endBody()
{
    tc->parenDepth--;
}








bool
GenexpGuard::checkValidBody(ParseNode *pn)
{
    if (tc->yieldCount > startYieldCount) {
        ParseNode *errorNode = tc->yieldNode;
        if (!errorNode)
            errorNode = pn;
        tc->parser->reportErrorNumber(errorNode, JSREPORT_ERROR, JSMSG_BAD_GENEXP_BODY, js_yield_str);
        return false;
    }

    if (tc->argumentsCount > startArgumentsCount) {
        ParseNode *errorNode = tc->argumentsNode;
        if (!errorNode)
            errorNode = pn;
        tc->parser->reportErrorNumber(errorNode, JSREPORT_ERROR, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
        return false;
    }

    return true;
}








bool
GenexpGuard::maybeNoteGenerator(ParseNode *pn)
{
    if (tc->yieldCount > 0) {
        tc->flags |= TCF_FUN_IS_GENERATOR;
        if (!tc->inFunction()) {
            tc->parser->reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_RETURN_OR_YIELD,
                                          js_yield_str);
            return false;
        }
        if (tc->flags & TCF_RETURN_EXPR) {
            
            ReportBadReturn(tc->parser->context, tc, pn, JSREPORT_ERROR,
                            JSMSG_BAD_GENERATOR_RETURN,
                            JSMSG_BAD_ANON_GENERATOR_RETURN);
            return false;
        }
    }
    return true;
}






static bool
BumpStaticLevel(ParseNode *pn, TreeContext *tc)
{
    if (!pn->pn_cookie.isFree()) {
        uintN level = pn->pn_cookie.level() + 1;

        JS_ASSERT(level >= tc->staticLevel);
        if (level >= UpvarCookie::FREE_LEVEL) {
            JS_ReportErrorNumber(tc->parser->context, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_DEEP, js_function_str);
            return false;
        }

        pn->pn_cookie.set(level, pn->pn_cookie.slot());
    }
    return true;
}

static void
AdjustBlockId(ParseNode *pn, uintN adjust, TreeContext *tc)
{
    JS_ASSERT(pn->isArity(PN_LIST) || pn->isArity(PN_FUNC) || pn->isArity(PN_NAME));
    pn->pn_blockid += adjust;
    if (pn->pn_blockid >= tc->blockidGen)
        tc->blockidGen = pn->pn_blockid + 1;
}

bool
CompExprTransplanter::transplant(ParseNode *pn)
{
    if (!pn)
        return true;

    switch (pn->getArity()) {
      case PN_LIST:
        for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!transplant(pn2))
                return false;
        }
        if (pn->pn_pos >= root->pn_pos)
            AdjustBlockId(pn, adjust, tc);
        break;

      case PN_TERNARY:
        if (!transplant(pn->pn_kid1) ||
            !transplant(pn->pn_kid2) ||
            !transplant(pn->pn_kid3))
            return false;
        break;

      case PN_BINARY:
        if (!transplant(pn->pn_left))
            return false;

        
        if (pn->pn_right != pn->pn_left) {
            if (!transplant(pn->pn_right))
                return false;
        }
        break;

      case PN_UNARY:
        if (!transplant(pn->pn_kid))
            return false;
        break;

      case PN_FUNC:
      {
        









        FunctionBox *funbox = pn->pn_funbox;

        funbox->level = tc->staticLevel + funcLevel;
        if (++funcLevel == 1 && genexp) {
            FunctionBox *parent = tc->funbox;

            FunctionBox **funboxp = &tc->parent->functionList;
            while (*funboxp != funbox)
                funboxp = &(*funboxp)->siblings;
            *funboxp = funbox->siblings;

            funbox->parent = parent;
            funbox->siblings = parent->kids;
            parent->kids = funbox;
            funbox->level = tc->staticLevel;
        }
        
      }

      case PN_NAME:
        if (!transplant(pn->maybeExpr()))
            return false;
        if (pn->isArity(PN_FUNC))
            --funcLevel;

        if (pn->isDefn()) {
            if (genexp && !BumpStaticLevel(pn, tc))
                return false;
        } else if (pn->isUsed()) {
            JS_ASSERT(!pn->isOp(JSOP_NOP));
            JS_ASSERT(pn->pn_cookie.isFree());

            Definition *dn = pn->pn_lexdef;
            JS_ASSERT(dn->isDefn());

            








            if (dn->isPlaceholder() && dn->pn_pos >= root->pn_pos && dn->dn_uses == pn) {
                if (genexp && !BumpStaticLevel(dn, tc))
                    return false;
                AdjustBlockId(dn, adjust, tc);
            }

            JSAtom *atom = pn->pn_atom;
#ifdef DEBUG
            StmtInfo *stmt = LexicalLookup(tc, atom, NULL);
            JS_ASSERT(!stmt || stmt != tc->topStmt);
#endif
            if (genexp && !dn->isOp(JSOP_CALLEE)) {
                JS_ASSERT(!tc->decls.lookupFirst(atom));

                if (dn->pn_pos < root->pn_pos) {
                    







                    Definition *dn2 = MakePlaceholder(pn, tc);
                    if (!dn2)
                        return false;
                    dn2->pn_pos = root->pn_pos;

                    



                    ParseNode **pnup = &dn->dn_uses;
                    ParseNode *pnu;
                    while ((pnu = *pnup) != NULL && pnu->pn_pos >= root->pn_pos) {
                        pnu->pn_lexdef = dn2;
                        dn2->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
                        pnup = &pnu->pn_link;
                    }
                    dn2->dn_uses = dn->dn_uses;
                    dn->dn_uses = *pnup;
                    *pnup = NULL;
                    if (!tc->lexdeps->put(atom, dn2))
                        return false;
                } else if (dn->isPlaceholder()) {
                    




                    tc->parent->lexdeps->remove(atom);
                    if (!tc->lexdeps->put(atom, dn))
                        return false;
                }
            }
        }

        if (pn->pn_pos >= root->pn_pos)
            AdjustBlockId(pn, adjust, tc);
        break;

      case PN_NAMESET:
        if (!transplant(pn->pn_tree))
            return false;
        break;

      case PN_NULLARY:
        
        break;
    }
    return true;
}










ParseNode *
Parser::comprehensionTail(ParseNode *kid, uintN blockid, bool isGenexp,
                          TokenKind type, JSOp op)
{
    uintN adjust;
    ParseNode *pn, *pn2, *pn3, **pnp;
    StmtInfo stmtInfo;
    BindData data;
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_FOR);

    if (type == TOK_SEMI) {
        




        pn = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
        if (!pn)
            return NULL;
        adjust = pn->pn_blockid - blockid;
    } else {
        JS_ASSERT(type == TOK_ARRAYPUSH);

        











        adjust = tc->blockid();
        pn = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
        if (!pn)
            return NULL;

        JS_ASSERT(blockid <= pn->pn_blockid);
        JS_ASSERT(blockid < tc->blockidGen);
        JS_ASSERT(tc->bodyid < blockid);
        pn->pn_blockid = stmtInfo.blockid = blockid;
        JS_ASSERT(adjust < blockid);
        adjust = blockid - adjust;
    }

    pnp = &pn->pn_expr;

    CompExprTransplanter transplanter(kid, tc, type == TOK_SEMI, adjust);
    transplanter.transplant(kid);

    data.pn = NULL;
    data.op = JSOP_NOP;
    data.binder = BindLet;
    data.let.overflow = JSMSG_ARRAY_INIT_TOO_BIG;

    do {
        




        pn2 = BinaryNode::create(tc);
        if (!pn2)
            return NULL;

        pn2->setOp(JSOP_ITER);
        pn2->pn_iflags = JSITER_ENUMERATE;
        if (tokenStream.matchToken(TOK_NAME)) {
            if (tokenStream.currentToken().name() == context->runtime->atomState.eachAtom)
                pn2->pn_iflags |= JSITER_FOREACH;
            else
                tokenStream.ungetToken();
        }
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

        GenexpGuard guard(tc);

        PropertyName *name = NULL;
        tt = tokenStream.getToken();
        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn3 = primaryExpr(tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn3)
                return NULL;
            break;
#endif

          case TOK_NAME:
            name = tokenStream.currentToken().name();

            






            pn3 = NewBindingNode(name, tc, true);
            if (!pn3)
                return NULL;
            break;

          default:
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return NULL;
        }

        MUST_MATCH_TOKEN(TOK_IN, JSMSG_IN_AFTER_FOR_NAME);
        ParseNode *pn4 = expr();
        if (!pn4)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

        guard.endBody();

        if (isGenexp) {
            if (!guard.checkValidBody(pn2))
                return NULL;
        } else {
            if (!guard.maybeNoteGenerator(pn2))
                return NULL;
        }

        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            if (!CheckDestructuring(context, &data, pn3, tc))
                return NULL;

            if (versionNumber() == JSVERSION_1_7) {
                
                if (!pn3->isKind(TOK_RB) || pn3->pn_count != 2) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_FOR_LEFTSIDE);
                    return NULL;
                }

                JS_ASSERT(pn2->isOp(JSOP_ITER));
                JS_ASSERT(pn2->pn_iflags & JSITER_ENUMERATE);
                if (!(pn2->pn_iflags & JSITER_FOREACH))
                    pn2->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          case TOK_NAME:
            data.pn = pn3;
            if (!data.binder(context, &data, name, tc))
                return NULL;
            break;

          default:;
        }

        



        ParseNode *vars = ListNode::create(tc);
        if (!vars)
            return NULL;
        vars->setOp(JSOP_NOP);
        vars->setKind(TOK_VAR);
        vars->pn_pos = pn3->pn_pos;
        vars->makeEmpty();
        vars->append(pn3);
        vars->pn_xflags |= PNX_FORINVAR;

        
        pn3 = CloneLeftHandSide(pn3, tc);
        if (!pn3)
            return NULL;

        pn2->pn_left = new_<TernaryNode>(TOK_IN, JSOP_NOP, vars, pn3, pn4);
        if (!pn2->pn_left)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_right;
    } while (tokenStream.matchToken(TOK_FOR));

    if (tokenStream.matchToken(TOK_IF)) {
        pn2 = TernaryNode::create(tc);
        if (!pn2)
            return NULL;
        pn2->pn_kid1 = condition();
        if (!pn2->pn_kid1)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_kid2;
    }

    pn2 = UnaryNode::create(tc);
    if (!pn2)
        return NULL;
    pn2->setKind(type);
    pn2->setOp(op);
    pn2->pn_kid = kid;
    *pnp = pn2;

    PopStatement(tc);
    return pn;
}

#if JS_HAS_GENERATOR_EXPRS
















ParseNode *
Parser::generatorExpr(ParseNode *kid)
{
    
    ParseNode *pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->setKind(TOK_YIELD);
    pn->setOp(JSOP_YIELD);
    pn->setInParens(true);
    pn->pn_pos = kid->pn_pos;
    pn->pn_kid = kid;
    pn->pn_hidden = true;

    
    ParseNode *genfn = FunctionNode::create(tc);
    if (!genfn)
        return NULL;
    genfn->setKind(TOK_FUNCTION);
    genfn->setOp(JSOP_LAMBDA);
    JS_ASSERT(!genfn->pn_body);
    genfn->pn_dflags = PND_FUNARG;

    {
        TreeContext *outertc = tc;
        TreeContext gentc(tc->parser);
        if (!gentc.init(context))
            return NULL;

        FunctionBox *funbox = EnterFunction(genfn, &gentc);
        if (!funbox)
            return NULL;

        






        if (outertc->flags & TCF_HAS_SHARPS) {
            gentc.flags |= TCF_IN_FUNCTION;
            if (!gentc.ensureSharpSlots())
                return NULL;
        }

        






        gentc.flags |= TCF_FUN_IS_GENERATOR | TCF_GENEXP_LAMBDA |
                       (outertc->flags & (TCF_FUN_FLAGS & ~TCF_FUN_PARAM_ARGUMENTS));
        funbox->tcflags |= gentc.flags;
        genfn->pn_funbox = funbox;
        genfn->pn_blockid = gentc.bodyid;

        ParseNode *body = comprehensionTail(pn, outertc->blockid(), true);
        if (!body)
            return NULL;
        JS_ASSERT(!genfn->pn_body);
        genfn->pn_body = body;
        genfn->pn_pos.begin = body->pn_pos.begin = kid->pn_pos.begin;
        genfn->pn_pos.end = body->pn_pos.end = tokenStream.currentToken().pos.end;

        if (!LeaveFunction(genfn, &gentc))
            return NULL;
    }

    



    ParseNode *result = ListNode::create(tc);
    if (!result)
        return NULL;
    result->setKind(TOK_LP);
    result->setOp(JSOP_CALL);
    result->pn_pos.begin = genfn->pn_pos.begin;
    result->initList(genfn);
    return result;
}

static const char js_generator_str[] = "generator";

#endif 
#endif 

JSBool
Parser::argumentList(ParseNode *listNode)
{
    if (tokenStream.matchToken(TOK_RP, TSF_OPERAND))
        return JS_TRUE;

    GenexpGuard guard(tc);
    bool arg0 = true;

    do {
        ParseNode *argNode = assignExpr();
        if (!argNode)
            return JS_FALSE;
        if (arg0)
            guard.endBody();

#if JS_HAS_GENERATORS
        if (argNode->isKind(TOK_YIELD) &&
            !argNode->isInParens() &&
            tokenStream.peekToken() == TOK_COMMA) {
            reportErrorNumber(argNode, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
            return JS_FALSE;
        }
#endif
#if JS_HAS_GENERATOR_EXPRS
        if (tokenStream.matchToken(TOK_FOR)) {
            if (!guard.checkValidBody(argNode))
                return JS_FALSE;
            argNode = generatorExpr(argNode);
            if (!argNode)
                return JS_FALSE;
            if (listNode->pn_count > 1 ||
                tokenStream.peekToken() == TOK_COMMA) {
                reportErrorNumber(argNode, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX,
                                  js_generator_str);
                return JS_FALSE;
            }
        } else
#endif
        if (arg0 && !guard.maybeNoteGenerator(argNode))
            return JS_FALSE;

        arg0 = false;

        listNode->append(argNode);
    } while (tokenStream.matchToken(TOK_COMMA));

    if (tokenStream.getToken() != TOK_RP) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_PAREN_AFTER_ARGS);
        return JS_FALSE;
    }
    return JS_TRUE;
}


static ParseNode *
CheckForImmediatelyAppliedLambda(ParseNode *pn)
{
    if (pn->isKind(TOK_FUNCTION)) {
        JS_ASSERT(pn->isArity(PN_FUNC));

        FunctionBox *funbox = pn->pn_funbox;
        JS_ASSERT((funbox->function())->flags & JSFUN_LAMBDA);
        if (!(funbox->tcflags & (TCF_FUN_USES_ARGUMENTS | TCF_FUN_USES_OWN_NAME)))
            pn->pn_dflags &= ~PND_FUNARG;
    }
    return pn;
}

ParseNode *
Parser::memberExpr(JSBool allowCallSyntax)
{
    ParseNode *pn, *pn2, *pn3;

    JS_CHECK_RECURSION(context, return NULL);

    
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    if (tt == TOK_NEW) {
        pn = ListNode::create(tc);
        if (!pn)
            return NULL;
        pn2 = memberExpr(JS_FALSE);
        if (!pn2)
            return NULL;
        pn2 = CheckForImmediatelyAppliedLambda(pn2);
        pn->setOp(JSOP_NEW);
        pn->initList(pn2);
        pn->pn_pos.begin = pn2->pn_pos.begin;

        if (tokenStream.matchToken(TOK_LP) && !argumentList(pn))
            return NULL;
        if (pn->pn_count > ARGC_LIMIT) {
            JS_ReportErrorNumber(context, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_MANY_CON_ARGS);
            return NULL;
        }
        pn->pn_pos.end = pn->last()->pn_pos.end;
    } else {
        pn = primaryExpr(tt, JS_FALSE);
        if (!pn)
            return NULL;

        if (pn->isKind(TOK_ANYNAME) || pn->isKind(TOK_AT) || pn->isKind(TOK_DBLCOLON)) {
            pn = new_<UnaryNode>(TOK_UNARYOP, JSOP_XMLNAME, pn->pn_pos, pn);
            if (!pn)
                return NULL;
        }
    }

    while ((tt = tokenStream.getToken()) > TOK_EOF) {
        if (tt == TOK_DOT) {
            pn2 = NameNode::create(NULL, tc);
            if (!pn2)
                return NULL;
#if JS_HAS_XML_SUPPORT
            tt = tokenStream.getToken(TSF_OPERAND | TSF_KEYWORD_IS_NAME);

            
            ParseNode *oldWith = tc->innermostWith;
            StmtInfo stmtInfo;
            if (tt == TOK_LP) {
                tc->innermostWith = pn;
                PushStatement(tc, &stmtInfo, STMT_WITH, -1);
            }

            pn3 = primaryExpr(tt, JS_TRUE);
            if (!pn3)
                return NULL;

            if (tt == TOK_LP) {
                tc->innermostWith = oldWith;
                PopStatement(tc);
            }

            
            if (tt == TOK_NAME && pn3->isKind(TOK_NAME)) {
                pn2->setOp(JSOP_GETPROP);
                pn2->pn_expr = pn;
                pn2->pn_atom = pn3->pn_atom;
                freeTree(pn3);
            } else {
                if (tt == TOK_LP) {
                    pn2->setKind(TOK_FILTER);
                    pn2->setOp(JSOP_FILTER);

                    
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                } else if (TokenKindIsXML(pn3->getKind())) {
                    pn2->setKind(TOK_LB);
                    pn2->setOp(JSOP_GETELEM);
                } else {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                    return NULL;
                }
                pn2->setArity(PN_BINARY);
                pn2->pn_left = pn;
                pn2->pn_right = pn3;
            }
#else
            MUST_MATCH_TOKEN_WITH_FLAGS(TOK_NAME, JSMSG_NAME_AFTER_DOT, TSF_KEYWORD_IS_NAME);
            pn2->setOp(JSOP_GETPROP);
            pn2->pn_expr = pn;
            pn2->pn_atom = tokenStream.currentToken().name();
#endif
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;
#if JS_HAS_XML_SUPPORT
        } else if (tt == TOK_DBLDOT) {
            pn2 = BinaryNode::create(tc);
            if (!pn2)
                return NULL;
            tt = tokenStream.getToken(TSF_OPERAND | TSF_KEYWORD_IS_NAME);
            pn3 = primaryExpr(tt, JS_TRUE);
            if (!pn3)
                return NULL;
            tt = pn3->getKind();
            if (tt == TOK_NAME && !pn3->isInParens()) {
                pn3->setKind(TOK_STRING);
                pn3->setArity(PN_NULLARY);
                pn3->setOp(JSOP_QNAMEPART);
            } else if (!TokenKindIsXML(tt)) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }
            pn2->setOp(JSOP_DESCENDANTS);
            pn2->pn_left = pn;
            pn2->pn_right = pn3;
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;
#endif
        } else if (tt == TOK_LB) {
            pn2 = BinaryNode::create(tc);
            if (!pn2)
                return NULL;
            pn3 = expr();
            if (!pn3)
                return NULL;

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_IN_INDEX);
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;

            






            do {
                if (pn3->isKind(TOK_STRING)) {
                    jsuint index;

                    if (!js_IdIsIndex(ATOM_TO_JSID(pn3->pn_atom), &index)) {
                        pn2->setKind(TOK_DOT);
                        pn2->setOp(JSOP_GETPROP);
                        pn2->setArity(PN_NAME);
                        pn2->pn_expr = pn;
                        pn2->pn_atom = pn3->pn_atom;
                        break;
                    }
                    pn3->setKind(TOK_NUMBER);
                    pn3->setOp(JSOP_DOUBLE);
                    pn3->pn_dval = index;
                }
                pn2->setOp(JSOP_GETELEM);
                pn2->pn_left = pn;
                pn2->pn_right = pn3;
            } while (0);
        } else if (allowCallSyntax && tt == TOK_LP) {
            pn2 = ListNode::create(tc);
            if (!pn2)
                return NULL;
            pn2->setOp(JSOP_CALL);

            pn = CheckForImmediatelyAppliedLambda(pn);
            if (pn->isOp(JSOP_NAME)) {
                if (pn->pn_atom == context->runtime->atomState.evalAtom) {
                    
                    pn2->setOp(JSOP_EVAL);
                    tc->noteCallsEval();
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                    



                    if (!tc->inStrictMode())
                        tc->noteHasExtensibleScope();
                }
            } else if (pn->isOp(JSOP_GETPROP)) {
                
                if (pn->pn_atom == context->runtime->atomState.applyAtom)
                    pn2->setOp(JSOP_FUNAPPLY);
                else if (pn->pn_atom == context->runtime->atomState.callAtom)
                    pn2->setOp(JSOP_FUNCALL);
            }

            pn2->initList(pn);
            pn2->pn_pos.begin = pn->pn_pos.begin;

            if (!argumentList(pn2))
                return NULL;
            if (pn2->pn_count > ARGC_LIMIT) {
                JS_ReportErrorNumber(context, js_GetErrorMessage, NULL,
                                     JSMSG_TOO_MANY_FUN_ARGS);
                return NULL;
            }
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;
        } else {
            tokenStream.ungetToken();
            return pn;
        }

        pn = pn2;
    }
    if (tt == TOK_ERROR)
        return NULL;
    return pn;
}

ParseNode *
Parser::bracketedExpr()
{
    uintN oldflags;
    ParseNode *pn;

    




    oldflags = tc->flags;
    tc->flags &= ~TCF_IN_FOR_INIT;
    pn = expr();
    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}

#if JS_HAS_XML_SUPPORT

ParseNode *
Parser::endBracketedExpr()
{
    ParseNode *pn = bracketedExpr();
    if (!pn)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ATTR_EXPR);
    return pn;
}




















































ParseNode *
Parser::propertySelector()
{
    DebugOnly<const Token *> tp = &tokenStream.currentToken();
    JS_ASSERT(tp->type == TOK_STAR || tp->type == TOK_NAME);

    ParseNode *pn = NullaryNode::create(tc);
    if (!pn)
        return NULL;
    if (pn->isKind(TOK_STAR)) {
        pn->setKind(TOK_ANYNAME);
        pn->setOp(JSOP_ANYNAME);
        pn->pn_atom = context->runtime->atomState.starAtom;
    } else {
        JS_ASSERT(pn->isKind(TOK_NAME));
        pn->setOp(JSOP_QNAMEPART);
        pn->setArity(PN_NAME);
        pn->pn_atom = tokenStream.currentToken().name();
        pn->pn_cookie.makeFree();
    }
    return pn;
}

ParseNode *
Parser::qualifiedSuffix(ParseNode *pn)
{
    ParseNode *pn2, *pn3;
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_DBLCOLON);
    pn2 = NameNode::create(NULL, tc);
    if (!pn2)
        return NULL;

    
    if (pn->isOp(JSOP_QNAMEPART))
        pn->setOp(JSOP_NAME);

    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
    if (tt == TOK_STAR || tt == TOK_NAME) {
        
        pn2->setOp(JSOP_QNAMECONST);
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->pn_atom = (tt == TOK_STAR)
                       ? context->runtime->atomState.starAtom
                       : tokenStream.currentToken().name();
        pn2->pn_expr = pn;
        pn2->pn_cookie.makeFree();
        return pn2;
    }

    if (tt != TOK_LB) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    pn3 = endBracketedExpr();
    if (!pn3)
        return NULL;

    pn2->setOp(JSOP_QNAME);
    pn2->setArity(PN_BINARY);
    pn2->pn_pos.begin = pn->pn_pos.begin;
    pn2->pn_pos.end = pn3->pn_pos.end;
    pn2->pn_left = pn;
    pn2->pn_right = pn3;
    return pn2;
}

ParseNode *
Parser::qualifiedIdentifier()
{
    DebugOnly<const Token *> tp = &tokenStream.currentToken();
    JS_ASSERT(tp->type == TOK_STAR || tp->type == TOK_NAME);

    ParseNode *pn = propertySelector();
    if (!pn)
        return NULL;
    if (tokenStream.matchToken(TOK_DBLCOLON)) {
        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        pn = qualifiedSuffix(pn);
    }
    return pn;
}

ParseNode *
Parser::attributeIdentifier()
{
    ParseNode *pn, *pn2;
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_AT);
    pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_TOATTRNAME);
    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
    if (tt == TOK_STAR || tt == TOK_NAME) {
        pn2 = qualifiedIdentifier();
    } else if (tt == TOK_LB) {
        pn2 = endBracketedExpr();
    } else {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    if (!pn2)
        return NULL;
    pn->pn_kid = pn2;
    return pn;
}




ParseNode *
Parser::xmlExpr(JSBool inTag)
{
    ParseNode *pn, *pn2;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LC);
    pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;

    





    bool oldflag = tokenStream.isXMLTagMode();
    tokenStream.setXMLTagMode(false);
    pn2 = expr();
    if (!pn2)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_XML_EXPR);
    tokenStream.setXMLTagMode(oldflag);
    pn->pn_kid = pn2;
    pn->setOp(inTag ? JSOP_XMLTAGEXPR : JSOP_XMLELTEXPR);
    return pn;
}







ParseNode *
Parser::xmlAtomNode()
{
    ParseNode *pn = NullaryNode::create(tc);
    if (!pn)
        return NULL;
    const Token &tok = tokenStream.currentToken();
    pn->setOp(tok.t_op);
    if (tok.type == TOK_XMLPI) {
        pn->pn_pitarget = tok.xmlPITarget();
        pn->pn_pidata = tok.xmlPIData();
    } else {
        pn->pn_atom = tok.atom();
    }
    return pn;
}













ParseNode *
Parser::xmlNameExpr()
{
    ParseNode *pn, *pn2, *list;
    TokenKind tt;

    pn = list = NULL;
    do {
        tt = tokenStream.currentToken().type;
        if (tt == TOK_LC) {
            pn2 = xmlExpr(JS_TRUE);
            if (!pn2)
                return NULL;
        } else {
            JS_ASSERT(tt == TOK_XMLNAME);
            pn2 = xmlAtomNode();
            if (!pn2)
                return NULL;
        }

        if (!pn) {
            pn = pn2;
        } else {
            if (!list) {
                list = ListNode::create(tc);
                if (!list)
                    return NULL;
                list->setKind(TOK_XMLNAME);
                list->pn_pos.begin = pn->pn_pos.begin;
                list->initList(pn);
                list->pn_xflags = PNX_CANTFOLD;
                pn = list;
            }
            pn->pn_pos.end = pn2->pn_pos.end;
            pn->append(pn2);
        }
    } while ((tt = tokenStream.getToken()) == TOK_XMLNAME || tt == TOK_LC);

    tokenStream.ungetToken();
    return pn;
}





#define XML_FOLDABLE(pn)        ((pn)->isArity(PN_LIST)                     \
                                 ? ((pn)->pn_xflags & PNX_CANTFOLD) == 0    \
                                 : !(pn)->isKind(TOK_LC))


















ParseNode *
Parser::xmlTagContent(TokenKind tagtype, JSAtom **namep)
{
    ParseNode *pn, *pn2, *list;
    TokenKind tt;

    pn = xmlNameExpr();
    if (!pn)
        return NULL;
    *namep = (pn->isArity(PN_NULLARY)) ? pn->pn_atom : NULL;
    list = NULL;

    while (tokenStream.matchToken(TOK_XMLSPACE)) {
        tt = tokenStream.getToken();
        if (tt != TOK_XMLNAME && tt != TOK_LC) {
            tokenStream.ungetToken();
            break;
        }

        pn2 = xmlNameExpr();
        if (!pn2)
            return NULL;
        if (!list) {
            list = ListNode::create(tc);
            if (!list)
                return NULL;
            list->setKind(tagtype);
            list->pn_pos.begin = pn->pn_pos.begin;
            list->initList(pn);
            pn = list;
        }
        pn->append(pn2);
        if (!XML_FOLDABLE(pn2))
            pn->pn_xflags |= PNX_CANTFOLD;

        tokenStream.matchToken(TOK_XMLSPACE);
        MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_NO_ASSIGN_IN_XML_ATTR);
        tokenStream.matchToken(TOK_XMLSPACE);

        tt = tokenStream.getToken();
        if (tt == TOK_XMLATTR) {
            pn2 = xmlAtomNode();
        } else if (tt == TOK_LC) {
            pn2 = xmlExpr(JS_TRUE);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_ATTR_VALUE);
            return NULL;
        }
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->append(pn2);
    }

    return pn;
}

#define XML_CHECK_FOR_ERROR_AND_EOF(tt,result)                                              \
    JS_BEGIN_MACRO                                                                          \
        if ((tt) <= TOK_EOF) {                                                              \
            if ((tt) == TOK_EOF) {                                                          \
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_END_OF_XML_SOURCE);           \
            }                                                                               \
            return result;                                                                  \
        }                                                                                   \
    JS_END_MACRO





JSBool
Parser::xmlElementContent(ParseNode *pn)
{
    tokenStream.setXMLTagMode(false);
    for (;;) {
        TokenKind tt = tokenStream.getToken(TSF_XMLTEXTMODE);
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);

        JS_ASSERT(tt == TOK_XMLSPACE || tt == TOK_XMLTEXT);
        JSAtom *textAtom = tokenStream.currentToken().atom();
        if (textAtom) {
            
            ParseNode *pn2 = xmlAtomNode();
            if (!pn2)
                return JS_FALSE;
            pn->pn_pos.end = pn2->pn_pos.end;
            pn->append(pn2);
        }

        tt = tokenStream.getToken(TSF_OPERAND);
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);
        if (tt == TOK_XMLETAGO)
            break;

        ParseNode *pn2;
        if (tt == TOK_LC) {
            pn2 = xmlExpr(JS_FALSE);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else if (tt == TOK_XMLSTAGO) {
            pn2 = xmlElementOrList(JS_FALSE);
            if (pn2) {
                pn2->pn_xflags &= ~PNX_XMLROOT;
                pn->pn_xflags |= pn2->pn_xflags;
            }
        } else {
            JS_ASSERT(tt == TOK_XMLCDATA || tt == TOK_XMLCOMMENT ||
                      tt == TOK_XMLPI);
            pn2 = xmlAtomNode();
        }
        if (!pn2)
            return JS_FALSE;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->append(pn2);
    }
    tokenStream.setXMLTagMode(true);

    JS_ASSERT(tokenStream.currentToken().type == TOK_XMLETAGO);
    return JS_TRUE;
}




ParseNode *
Parser::xmlElementOrList(JSBool allowList)
{
    ParseNode *pn, *pn2, *list;
    TokenKind tt;
    JSAtom *startAtom, *endAtom;

    JS_CHECK_RECURSION(context, return NULL);

    JS_ASSERT(tokenStream.currentToken().type == TOK_XMLSTAGO);
    pn = ListNode::create(tc);
    if (!pn)
        return NULL;

    tokenStream.setXMLTagMode(true);
    tt = tokenStream.getToken();
    if (tt == TOK_ERROR)
        return NULL;

    if (tt == TOK_XMLNAME || tt == TOK_LC) {
        


        pn2 = xmlTagContent(TOK_XMLSTAGO, &startAtom);
        if (!pn2)
            return NULL;
        tokenStream.matchToken(TOK_XMLSPACE);

        tt = tokenStream.getToken();
        if (tt == TOK_XMLPTAGC) {
            
            if (pn2->isKind(TOK_XMLSTAGO)) {
                pn->makeEmpty();
                freeTree(pn);
                pn = pn2;
            } else {
                JS_ASSERT(pn2->isKind(TOK_XMLNAME) ||
                          pn2->isKind(TOK_LC));
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
            }
            pn->setKind(TOK_XMLPTAGC);
            pn->pn_xflags |= PNX_XMLROOT;
        } else {
            
            if (tt != TOK_XMLTAGC) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;

            
            if (!pn2->isKind(TOK_XMLSTAGO)) {
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
                pn2 = pn;
                pn = ListNode::create(tc);
                if (!pn)
                    return NULL;
            }

            
            pn->setKind(TOK_XMLELEM);
            pn->pn_pos.begin = pn2->pn_pos.begin;
            pn->initList(pn2);
            if (!XML_FOLDABLE(pn2))
                pn->pn_xflags |= PNX_CANTFOLD;
            pn->pn_xflags |= PNX_XMLROOT;

            
            if (!xmlElementContent(pn))
                return NULL;

            tt = tokenStream.getToken();
            XML_CHECK_FOR_ERROR_AND_EOF(tt, NULL);
            if (tt != TOK_XMLNAME && tt != TOK_LC) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }

            
            pn2 = xmlTagContent(TOK_XMLETAGO, &endAtom);
            if (!pn2)
                return NULL;
            if (pn2->isKind(TOK_XMLETAGO)) {
                
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            if (endAtom && startAtom && endAtom != startAtom) {
                
                reportErrorNumber(pn2, JSREPORT_UC | JSREPORT_ERROR, JSMSG_XML_TAG_NAME_MISMATCH,
                                  startAtom->chars());
                return NULL;
            }

            
            JS_ASSERT(pn2->isKind(TOK_XMLNAME) || pn2->isKind(TOK_LC));
            list = ListNode::create(tc);
            if (!list)
                return NULL;
            list->setKind(TOK_XMLETAGO);
            list->initList(pn2);
            pn->append(list);
            if (!XML_FOLDABLE(pn2)) {
                list->pn_xflags |= PNX_CANTFOLD;
                pn->pn_xflags |= PNX_CANTFOLD;
            }

            tokenStream.matchToken(TOK_XMLSPACE);
            MUST_MATCH_TOKEN(TOK_XMLTAGC, JSMSG_BAD_XML_TAG_SYNTAX);
        }

        
        pn->setOp(JSOP_TOXML);
    } else if (allowList && tt == TOK_XMLTAGC) {
        
        pn->setKind(TOK_XMLLIST);
        pn->setOp(JSOP_TOXMLLIST);
        pn->makeEmpty();
        pn->pn_xflags |= PNX_XMLROOT;
        if (!xmlElementContent(pn))
            return NULL;

        MUST_MATCH_TOKEN(TOK_XMLTAGC, JSMSG_BAD_XML_LIST_SYNTAX);
    } else {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_NAME_SYNTAX);
        return NULL;
    }
    tokenStream.setXMLTagMode(false);

    pn->pn_pos.end = tokenStream.currentToken().pos.end;
    return pn;
}

ParseNode *
Parser::xmlElementOrListRoot(JSBool allowList)
{
    





    bool hadXML = tokenStream.hasXML();
    tokenStream.setXML(true);
    ParseNode *pn = xmlElementOrList(allowList);
    tokenStream.setXML(hadXML);
    return pn;
}

ParseNode *
Parser::parseXMLText(JSObject *chain, bool allowList)
{
    




    TreeContext xmltc(this);
    if (!xmltc.init(context))
        return NULL;
    xmltc.setScopeChain(chain);

    
    tokenStream.setXMLOnlyMode();
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);

    ParseNode *pn;
    if (tt != TOK_XMLSTAGO) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_MARKUP);
        pn = NULL;
    } else {
        pn = xmlElementOrListRoot(allowList);
    }
    tokenStream.setXMLOnlyMode(false);

    return pn;
}

#endif 

#if JS_HAS_BLOCK_SCOPE




















static inline bool
BlockIdInScope(uintN blockid, TreeContext *tc)
{
    if (blockid > tc->blockid())
        return false;
    for (StmtInfo *stmt = tc->topScopeStmt; stmt; stmt = stmt->downScope) {
        if (stmt->blockid == blockid)
            return true;
    }
    return false;
}
#endif

ParseNode *
Parser::primaryExpr(TokenKind tt, JSBool afterDot)
{
    ParseNode *pn, *pn2, *pn3;
    JSOp op;

    JS_CHECK_RECURSION(context, return NULL);

    switch (tt) {
      case TOK_FUNCTION:
#if JS_HAS_XML_SUPPORT
        if (tokenStream.matchToken(TOK_DBLCOLON, TSF_KEYWORD_IS_NAME)) {
            pn2 = NullaryNode::create(tc);
            if (!pn2)
                return NULL;
            pn2->setKind(TOK_FUNCTION);
            pn = qualifiedSuffix(pn2);
            if (!pn)
                return NULL;
            break;
        }
#endif
        pn = functionExpr();
        if (!pn)
            return NULL;
        break;

      case TOK_LB:
      {
        JSBool matched;
        jsuint index;

        pn = ListNode::create(tc);
        if (!pn)
            return NULL;
        pn->setKind(TOK_RB);
        pn->setOp(JSOP_NEWINIT);
        pn->makeEmpty();

#if JS_HAS_GENERATORS
        pn->pn_blockid = tc->blockidGen;
#endif

        matched = tokenStream.matchToken(TOK_RB, TSF_OPERAND);
        if (!matched) {
            for (index = 0; ; index++) {
                if (index == StackSpace::ARGS_LENGTH_MAX) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_ARRAY_INIT_TOO_BIG);
                    return NULL;
                }

                tt = tokenStream.peekToken(TSF_OPERAND);
                if (tt == TOK_RB) {
                    pn->pn_xflags |= PNX_ENDCOMMA;
                    break;
                }

                if (tt == TOK_COMMA) {
                    
                    tokenStream.matchToken(TOK_COMMA);
                    pn2 = NullaryNode::create(tc);
                    pn->pn_xflags |= PNX_HOLEY | PNX_NONCONST;
                } else {
                    pn2 = assignExpr();
                    if (pn2 && !pn2->isConstant())
                        pn->pn_xflags |= PNX_NONCONST;
                }
                if (!pn2)
                    return NULL;
                pn->append(pn2);

                if (tt != TOK_COMMA) {
                    
                    if (!tokenStream.matchToken(TOK_COMMA))
                        break;
                }
            }

#if JS_HAS_GENERATORS
            








































            if (index == 0 && pn->pn_count != 0 && tokenStream.matchToken(TOK_FOR)) {
                ParseNode *pnexp, *pntop;

                
                pn->setKind(TOK_ARRAYCOMP);

                




                pnexp = pn->last();
                JS_ASSERT(pn->pn_count == 1);
                pn->pn_count = 0;
                pn->pn_tail = &pn->pn_head;
                *pn->pn_tail = NULL;

                pntop = comprehensionTail(pnexp, pn->pn_blockid, false,
                                          TOK_ARRAYPUSH, JSOP_ARRAYPUSH);
                if (!pntop)
                    return NULL;
                pn->append(pntop);
            }
#endif 

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_LIST);
        }
        pn->pn_pos.end = tokenStream.currentToken().pos.end;
        return pn;
      }

      case TOK_LC:
      {
        ParseNode *pnval;

        



        AtomIndexMap seen(context);

        enum AssignmentType {
            GET     = 0x1,
            SET     = 0x2,
            VALUE   = 0x4 | GET | SET
        };

        pn = ListNode::create(tc);
        if (!pn)
            return NULL;
        pn->setKind(TOK_RC);
        pn->setOp(JSOP_NEWINIT);
        pn->makeEmpty();

        for (;;) {
            JSAtom *atom;
            tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            switch (tt) {
              case TOK_NUMBER:
                pn3 = NullaryNode::create(tc);
                if (!pn3)
                    return NULL;
                pn3->pn_dval = tokenStream.currentToken().t_dval;
                if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                    return NULL;
                break;
              case TOK_NAME:
                {
                    atom = tokenStream.currentToken().name();
                    if (atom == context->runtime->atomState.getAtom)
                        op = JSOP_GETTER;
                    else if (atom == context->runtime->atomState.setAtom)
                        op = JSOP_SETTER;
                    else
                        goto property_name;

                    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
                    if (tt == TOK_NAME || tt == TOK_STRING) {
                        atom = (tt == TOK_NAME)
                               ? tokenStream.currentToken().name()
                               : tokenStream.currentToken().atom();
                        pn3 = NameNode::create(atom, tc);
                        if (!pn3)
                            return NULL;
                    } else if (tt == TOK_NUMBER) {
                        pn3 = NullaryNode::create(tc);
                        if (!pn3)
                            return NULL;
                        pn3->pn_dval = tokenStream.currentToken().t_dval;
                        if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                            return NULL;
                    } else {
                        tokenStream.ungetToken();
                        goto property_name;
                    }

                    pn->pn_xflags |= PNX_NONCONST;

                    
                    pn2 = functionDef(NULL, op == JSOP_GETTER ? Getter : Setter, Expression);
                    pn2 = ParseNode::newBinaryOrAppend(TOK_COLON, op, pn3, pn2, tc);
                    goto skip;
                }
              case TOK_STRING:
                atom = tokenStream.currentToken().atom();
              property_name:
                pn3 = NullaryNode::create(tc);
                if (!pn3)
                    return NULL;
                pn3->pn_atom = atom;
                break;
              case TOK_RC:
                goto end_obj_init;
              default:
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_PROP_ID);
                return NULL;
            }

            op = JSOP_INITPROP;
            tt = tokenStream.getToken();
            if (tt == TOK_COLON) {
                pnval = assignExpr();

                




                if ((pnval && !pnval->isConstant()) ||
                    atom == context->runtime->atomState.protoAtom) {
                    pn->pn_xflags |= PNX_NONCONST;
                }
            } else {
#if JS_HAS_DESTRUCTURING_SHORTHAND
                if (tt != TOK_COMMA && tt != TOK_RC) {
#endif
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_COLON_AFTER_ID);
                    return NULL;
#if JS_HAS_DESTRUCTURING_SHORTHAND
                }

                



                tokenStream.ungetToken();
                pn->pn_xflags |= PNX_DESTRUCT | PNX_NONCONST;
                pnval = pn3;
                if (pnval->isKind(TOK_NAME)) {
                    pnval->setArity(PN_NAME);
                    ((NameNode *)pnval)->initCommon(tc);
                }
#endif
            }

            pn2 = ParseNode::newBinaryOrAppend(TOK_COLON, op, pn3, pnval, tc);
          skip:
            if (!pn2)
                return NULL;
            pn->append(pn2);

            





            AssignmentType assignType;
            if (op == JSOP_INITPROP) {
                assignType = VALUE;
            } else if (op == JSOP_GETTER) {
                assignType = GET;
            } else if (op == JSOP_SETTER) {
                assignType = SET;
            } else {
                JS_NOT_REACHED("bad opcode in object initializer");
                assignType = VALUE; 
            }

            AtomIndexAddPtr p = seen.lookupForAdd(atom);
            if (p) {
                jsatomid index = p.value();
                AssignmentType oldAssignType = AssignmentType(index);
                if ((oldAssignType & assignType) &&
                    (oldAssignType != VALUE || assignType != VALUE || tc->needStrictChecks()))
                {
                    JSAutoByteString name;
                    if (!js_AtomToPrintableString(context, atom, &name))
                        return NULL;

                    uintN flags = (oldAssignType == VALUE &&
                                   assignType == VALUE &&
                                   !tc->inStrictMode())
                                  ? JSREPORT_WARNING
                                  : JSREPORT_ERROR;
                    if (!ReportCompileErrorNumber(context, &tokenStream, NULL, flags,
                                                  JSMSG_DUPLICATE_PROPERTY, name.ptr()))
                    {
                        return NULL;
                    }
                }
                p.value() = assignType | oldAssignType;
            } else {
                if (!seen.add(p, atom, assignType))
                    return NULL;
            }

            tt = tokenStream.getToken();
            if (tt == TOK_RC)
                goto end_obj_init;
            if (tt != TOK_COMMA) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CURLY_AFTER_LIST);
                return NULL;
            }
        }

      end_obj_init:
        pn->pn_pos.end = tokenStream.currentToken().pos.end;
        return pn;
      }

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        pn = letBlock(JS_FALSE);
        if (!pn)
            return NULL;
        break;
#endif

#if JS_HAS_SHARP_VARS
      case TOK_DEFSHARP:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_num = (jsint) tokenStream.currentToken().t_dval;
        tt = tokenStream.getToken(TSF_OPERAND);
        pn->pn_kid = primaryExpr(tt, JS_FALSE);
        if (!pn->pn_kid)
            return NULL;
        if (pn->pn_kid->isKind(TOK_USESHARP) ||
            pn->pn_kid->isKind(TOK_DEFSHARP) ||
            pn->pn_kid->isKind(TOK_STRING) ||
            pn->pn_kid->isKind(TOK_NUMBER) ||
            pn->pn_kid->isKind(TOK_PRIMARY)) {
            reportErrorNumber(pn->pn_kid, JSREPORT_ERROR, JSMSG_BAD_SHARP_VAR_DEF);
            return NULL;
        }
        if (!tc->ensureSharpSlots())
            return NULL;
        break;

      case TOK_USESHARP:
        
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!tc->ensureSharpSlots())
            return NULL;
        pn->pn_num = (jsint) tokenStream.currentToken().t_dval;
        break;
#endif 

      case TOK_LP:
      {
        JSBool genexp;

        pn = parenExpr(&genexp);
        if (!pn)
            return NULL;
        pn->setInParens(true);
        if (!genexp)
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);
        break;
      }

#if JS_HAS_XML_SUPPORT
      case TOK_STAR:
        pn = qualifiedIdentifier();
        if (!pn)
            return NULL;
        break;

      case TOK_AT:
        pn = attributeIdentifier();
        if (!pn)
            return NULL;
        break;

      case TOK_XMLSTAGO:
        pn = xmlElementOrListRoot(JS_TRUE);
        if (!pn)
            return NULL;
        break;
#endif 

      case TOK_STRING:
#if JS_HAS_XML_SUPPORT
      case TOK_XMLCDATA:
      case TOK_XMLCOMMENT:
#endif
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_atom = tokenStream.currentToken().atom();
        pn->setOp(tokenStream.currentToken().t_op);
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_XMLPI:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_pitarget = tokenStream.currentToken().xmlPITarget();
        pn->pn_pidata = tokenStream.currentToken().xmlPIData();
        break;
#endif

      case TOK_NAME:
        pn = NameNode::create(tokenStream.currentToken().name(), tc);
        if (!pn)
            return NULL;
        JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
        pn->setOp(JSOP_NAME);

        if ((tc->flags & (TCF_IN_FUNCTION | TCF_FUN_PARAM_ARGUMENTS)) == TCF_IN_FUNCTION &&
            pn->pn_atom == context->runtime->atomState.argumentsAtom) {
            






            tc->noteArgumentsUse(pn);

            



            if (!afterDot && !(tc->flags & TCF_DECL_DESTRUCTURING)
                && !tc->inStatement(STMT_WITH)) {
                pn->setOp(JSOP_ARGUMENTS);
                pn->pn_dflags |= PND_BOUND;
            }
        } else if ((!afterDot
#if JS_HAS_XML_SUPPORT
                    || tokenStream.peekToken() == TOK_DBLCOLON
#endif
                   ) && !(tc->flags & TCF_DECL_DESTRUCTURING)) {
            
            if (!tc->inFunction() &&
                pn->pn_atom == context->runtime->atomState.argumentsAtom) {
                tc->countArgumentsUse(pn);
            }

            StmtInfo *stmt = LexicalLookup(tc, pn->pn_atom, NULL);

            MultiDeclRange mdl = tc->decls.lookupMulti(pn->pn_atom);
            Definition *dn;

            if (!mdl.empty()) {
                dn = mdl.front();
#if JS_HAS_BLOCK_SCOPE
                





                while (dn->isLet() && !BlockIdInScope(dn->pn_blockid, tc)) {
                    mdl.popFront();
                    if (mdl.empty())
                        break;
                    dn = mdl.front();
                }
#endif
            }

            if (!mdl.empty()) {
                dn = mdl.front();
            } else {
                AtomDefnAddPtr p = tc->lexdeps->lookupForAdd(pn->pn_atom);
                if (p) {
                    dn = p.value();
                } else {
                    







                    dn = MakePlaceholder(pn, tc);
                    if (!dn || !tc->lexdeps->add(p, dn->pn_atom, dn))
                        return NULL;

                    










                    if (tokenStream.peekToken() != TOK_LP)
                        dn->pn_dflags |= PND_FUNARG;
                }
            }

            JS_ASSERT(dn->isDefn());
            LinkUseToDef(pn, dn, tc);

            
            if (tokenStream.peekToken() != TOK_LP)
                dn->pn_dflags |= PND_FUNARG;

            pn->pn_dflags |= (dn->pn_dflags & PND_FUNARG);
            if (stmt && stmt->type == STMT_WITH)
                pn->pn_dflags |= PND_DEOPTIMIZED;
        }

#if JS_HAS_XML_SUPPORT
        if (tokenStream.matchToken(TOK_DBLCOLON)) {
            if (afterDot) {
                




                const KeywordInfo *ki = FindKeyword(pn->pn_atom->charsZ(), pn->pn_atom->length());
                if (ki) {
                    if (ki->tokentype != TOK_FUNCTION) {
                        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_KEYWORD_NOT_NS);
                        return NULL;
                    }

                    pn->setArity(PN_NULLARY);
                    pn->setKind(TOK_FUNCTION);
                }
            }
            pn = qualifiedSuffix(pn);
            if (!pn)
                return NULL;
        }
#endif
        break;

      case TOK_REGEXP:
      {
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;

        const jschar *chars = tokenStream.getTokenbuf().begin();
        size_t length = tokenStream.getTokenbuf().length();
        RegExpFlag flags = RegExpFlag(tokenStream.currentToken().t_reflags);
        RegExpStatics *res = context->regExpStatics();

        RegExpObject *reobj;
        if (context->hasfp())
            reobj = RegExpObject::create(context, res, chars, length, flags, &tokenStream);
        else
            reobj = RegExpObject::createNoStatics(context, chars, length, flags, &tokenStream);

        if (!reobj)
            return NULL;

        if (!tc->compileAndGo()) {
            reobj->clearParent();
            reobj->clearType();
        }

        pn->pn_objbox = tc->parser->newObjectBox(reobj);
        if (!pn->pn_objbox)
            return NULL;

        pn->setOp(JSOP_REGEXP);
        break;
      }

      case TOK_NUMBER:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->setOp(JSOP_DOUBLE);
        pn->pn_dval = tokenStream.currentToken().t_dval;
        break;

      case TOK_PRIMARY:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->setOp(tokenStream.currentToken().t_op);
        break;

      case TOK_ERROR:
        
        return NULL;

      default:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    return pn;
}

ParseNode *
Parser::parenExpr(JSBool *genexp)
{
    TokenPtr begin;
    ParseNode *pn;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LP);
    begin = tokenStream.currentToken().pos.begin;

    if (genexp)
        *genexp = JS_FALSE;

    GenexpGuard guard(tc);

    pn = bracketedExpr();
    if (!pn)
        return NULL;
    guard.endBody();

#if JS_HAS_GENERATOR_EXPRS
    if (tokenStream.matchToken(TOK_FOR)) {
        if (!guard.checkValidBody(pn))
            return NULL;
        JS_ASSERT(!pn->isKind(TOK_YIELD));
        if (pn->isKind(TOK_COMMA) && !pn->isInParens()) {
            reportErrorNumber(pn->last(), JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX,
                              js_generator_str);
            return NULL;
        }
        pn = generatorExpr(pn);
        if (!pn)
            return NULL;
        pn->pn_pos.begin = begin;
        if (genexp) {
            if (tokenStream.getToken() != TOK_RP) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX,
                                  js_generator_str);
                return NULL;
            }
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
            *genexp = JS_TRUE;
        }
    } else
#endif 

    if (!guard.maybeNoteGenerator(pn))
        return NULL;

    return pn;
}
