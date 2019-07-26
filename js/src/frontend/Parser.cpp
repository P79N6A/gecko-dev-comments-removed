





















































#include "frontend/Parser.h"

#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "frontend/ParseMaps.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsatominlines.h"
#include "jsscriptinlines.h"

#include "frontend/BytecodeEmitter-inl.h"
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

Parser::Parser(JSContext *cx, JSPrincipals *prin, JSPrincipals *originPrin,
               StackFrame *cfp, bool foldConstants)
  : AutoGCRooter(cx, PARSER),
    context(cx),
    tokenStream(cx, prin, originPrin),
    principals(NULL),
    originPrincipals(NULL),
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
    setPrincipals(prin, originPrin);
    JS_ASSERT_IF(cfp, cfp->isScriptFrame());
}

bool
Parser::init(const jschar *base, size_t length, const char *filename, unsigned lineno,
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
        JS_DropPrincipals(cx->runtime, principals);
    if (originPrincipals)
        JS_DropPrincipals(cx->runtime, originPrincipals);
    cx->tempLifoAlloc().release(tempPoolMark);
    cx->activeCompilations--;
}

void
Parser::setPrincipals(JSPrincipals *prin, JSPrincipals *originPrin)
{
    JS_ASSERT(!principals && !originPrincipals);
    principals = prin;
    if (principals)
        JS_HoldPrincipals(principals);
    originPrincipals = originPrin;
    if (originPrincipals)
        JS_HoldPrincipals(originPrincipals);
}

ObjectBox *
Parser::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj && !IsPoisonedPtr(obj));

    






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
    JS_ASSERT(obj && !IsPoisonedPtr(obj));
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
    if (!tc->inFunction()) {
        JSObject *scope = tc->scopeChain();
        while (scope) {
            if (scope->isWith())
                funbox->tcflags |= TCF_IN_WITH;
            scope = scope->enclosingScope();
        }
    }
    return funbox;
}

void
Parser::trace(JSTracer *trc)
{
    ObjectBox *objbox = traceListHead;
    while (objbox) {
        MarkObjectRoot(trc, &objbox->object, "parser.object");
        if (objbox->isFunctionBox)
            static_cast<FunctionBox *>(objbox)->bindings.trace(trc);
        objbox = objbox->traceLink;
    }

    for (TreeContext *tc = this->tc; tc; tc = tc->parent)
        tc->trace(trc);
}

static bool
GenerateBlockIdForStmtNode(ParseNode *pn, TreeContext *tc)
{
    JS_ASSERT(tc->topStmt);
    JS_ASSERT(STMT_MAYBE_SCOPE(tc->topStmt));
    JS_ASSERT(pn->isKind(PNK_STATEMENTLIST) || pn->isKind(PNK_LEXICALSCOPE));
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
    unsigned rv, rv2, hasDefault;

    switch (pn->getKind()) {
      case PNK_STATEMENTLIST:
        if (!pn->pn_head)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->last());

      case PNK_IF:
        if (!pn->pn_kid3)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->pn_kid2) & HasFinalReturn(pn->pn_kid3);

      case PNK_WHILE:
        pn2 = pn->pn_left;
        if (pn2->isKind(PNK_TRUE))
            return ENDS_IN_RETURN;
        if (pn2->isKind(PNK_NUMBER) && pn2->pn_dval)
            return ENDS_IN_RETURN;
        return ENDS_IN_OTHER;

      case PNK_DOWHILE:
        pn2 = pn->pn_right;
        if (pn2->isKind(PNK_FALSE))
            return HasFinalReturn(pn->pn_left);
        if (pn2->isKind(PNK_TRUE))
            return ENDS_IN_RETURN;
        if (pn2->isKind(PNK_NUMBER)) {
            if (pn2->pn_dval == 0)
                return HasFinalReturn(pn->pn_left);
            return ENDS_IN_RETURN;
        }
        return ENDS_IN_OTHER;

      case PNK_FOR:
        pn2 = pn->pn_left;
        if (pn2->isArity(PN_TERNARY) && !pn2->pn_kid2)
            return ENDS_IN_RETURN;
        return ENDS_IN_OTHER;

      case PNK_SWITCH:
        rv = ENDS_IN_RETURN;
        hasDefault = ENDS_IN_OTHER;
        pn2 = pn->pn_right;
        if (pn2->isKind(PNK_LEXICALSCOPE))
            pn2 = pn2->expr();
        for (pn2 = pn2->pn_head; rv && pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(PNK_DEFAULT))
                hasDefault = ENDS_IN_RETURN;
            pn3 = pn2->pn_right;
            JS_ASSERT(pn3->isKind(PNK_STATEMENTLIST));
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

      case PNK_BREAK:
        return ENDS_IN_BREAK;

      case PNK_WITH:
        return HasFinalReturn(pn->pn_right);

      case PNK_RETURN:
        return ENDS_IN_RETURN;

      case PNK_COLON:
      case PNK_LEXICALSCOPE:
        return HasFinalReturn(pn->expr());

      case PNK_THROW:
        return ENDS_IN_RETURN;

      case PNK_TRY:
        
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

      case PNK_CATCH:
        
        return HasFinalReturn(pn->pn_kid3);

      case PNK_LET:
        
        if (!pn->isArity(PN_BINARY))
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->pn_right);

      default:
        return ENDS_IN_OTHER;
    }
}

static JSBool
ReportBadReturn(JSContext *cx, TreeContext *tc, ParseNode *pn, unsigned flags, unsigned errnum,
                unsigned anonerrnum)
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
    if (tc->needStrictChecks() && lhs->isKind(PNK_NAME)) {
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
ReportBadParameter(JSContext *cx, TreeContext *tc, JSAtom *name, unsigned errorNumber)
{
    Definition *dn = tc->decls.lookupFirst(name);
    JSAutoByteString bytes;
    return js_AtomToPrintableString(cx, name, &bytes) &&
           ReportStrictModeError(cx, TS(tc->parser), tc, dn, errorNumber, bytes.ptr());
}







static bool
CheckStrictParameters(JSContext *cx, TreeContext *tc)
{
    JS_ASSERT(tc->inFunction());

    if (!tc->needStrictChecks() || tc->bindings.numArgs() == 0)
        return true;

    JSAtom *argumentsAtom = cx->runtime->atomState.argumentsAtom;
    JSAtom *evalAtom = cx->runtime->atomState.evalAtom;

    
    HashMap<JSAtom *, bool> parameters(cx);
    if (!parameters.init(tc->bindings.numArgs()))
        return false;

    
    for (Shape::Range r = tc->bindings.lastVariable(); !r.empty(); r.popFront()) {
        jsid id = r.front().propid();
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

static bool
BindLocalVariable(JSContext *cx, TreeContext *tc, ParseNode *pn, BindingKind kind)
{
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);

    unsigned index = tc->bindings.numVars();
    if (!tc->bindings.add(cx, RootedVarAtom(cx, pn->pn_atom), kind))
        return false;

    pn->pn_cookie.set(tc->staticLevel, index);
    pn->pn_dflags |= PND_BOUND;
    return true;
}

ParseNode *
Parser::functionBody(FunctionBodyType type)
{
    JS_ASSERT(tc->inFunction());

    StmtInfo stmtInfo(context);
    PushStatement(tc, &stmtInfo, STMT_BLOCK, -1);
    stmtInfo.flags = SIF_BODY_BLOCK;

    unsigned oldflags = tc->flags;
    tc->flags &= ~(TCF_RETURN_EXPR | TCF_RETURN_VOID);

    ParseNode *pn;
    if (type == StatementListBody) {
        pn = statements();
    } else {
        JS_ASSERT(type == ExpressionBody);
        JS_ASSERT(JS_HAS_EXPR_CLOSURES);
        pn = UnaryNode::create(PNK_RETURN, tc);
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
                    pn->setOp(JSOP_RETURN);
                    pn->pn_pos.end = pn->pn_kid->pn_pos.end;
                }
            }
        }
    }

    if (pn) {
        JS_ASSERT(!(tc->topStmt->flags & SIF_SCOPE));
        PopStatementTC(tc);

        
        if (context->hasStrictOption() && (tc->flags & TCF_RETURN_EXPR) &&
            !CheckFinalReturn(context, tc, pn)) {
            pn = NULL;
        }
    }

    



    if (!CheckStrictParameters(context, tc))
        return NULL;

    RootedVar<PropertyName*> const arguments(context, context->runtime->atomState.argumentsAtom);

    







    if (FuncStmtSet *set = tc->funcStmts) {
        for (FuncStmtSet::Range r = set->all(); !r.empty(); r.popFront()) {
            PropertyName *name = r.front()->asPropertyName();
            if (name == arguments)
                tc->noteBindingsAccessedDynamically();
            else if (Definition *dn = tc->decls.lookupFirst(name))
                dn->pn_dflags |= PND_CLOSED;
        }
    }

    




    for (AtomDefnRange r = tc->lexdeps->all(); !r.empty(); r.popFront()) {
        JSAtom *atom = r.front().key();
        Definition *dn = r.front().value();
        JS_ASSERT(dn->isPlaceholder());
        if (atom == arguments) {
            



            if (!BindLocalVariable(context, tc, dn, VARIABLE))
                return NULL;
            dn->setOp(JSOP_GETLOCAL);
            dn->pn_dflags &= ~PND_PLACEHOLDER;

            
            tc->lexdeps->remove(arguments);
            break;
        }
    }

    



    if (tc->bindingsAccessedDynamically() && !tc->bindings.hasBinding(context, arguments)) {
        if (!tc->bindings.addVariable(context, arguments))
            return NULL;
    }

    




    BindingKind bindKind = tc->bindings.lookup(context, arguments, NULL);
    if (bindKind == VARIABLE || bindKind == CONSTANT) {
        tc->noteArgumentsHasLocalBinding();

        
        if (tc->bindingsAccessedDynamically())
            tc->noteDefinitelyNeedsArgsObj();

        






        if (tc->inStrictMode()) {
            AtomDeclsIter iter(&tc->decls);
            while (Definition *dn = iter.next()) {
                if (dn->kind() == Definition::ARG && dn->isAssigned()) {
                    tc->noteDefinitelyNeedsArgsObj();
                    break;
                }
             }
        }
    }

    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}


static Definition *
MakePlaceholder(ParseNode *pn, TreeContext *tc)
{
    Definition *dn = (Definition *) NameNode::create(PNK_NAME, pn->pn_atom, tc);
    if (!dn)
        return NULL;

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
        unsigned start = let ? pn->pn_blockid : tc->bodyid;

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

        pn->pn_dflags |= dn->pn_dflags & PND_CLOSED;
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
    ParseNode *lhs = tc->parser->cloneNode(*pn);
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

    pn->setKind(PNK_ASSIGN);
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
        dn->setKind(PNK_NAME);
        dn->setArity(PN_NAME);
        dn->pn_atom = atom;
    }

    
    JS_ASSERT(dn->isKind(PNK_NAME));
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

bool
js::DefineArg(ParseNode *pn, JSAtom *atom, unsigned i, TreeContext *tc)
{
    




    ParseNode *argpn = NameNode::create(PNK_NAME, atom, tc);
    if (!argpn)
        return false;
    JS_ASSERT(argpn->isKind(PNK_NAME) && argpn->isOp(JSOP_NOP));

    
    argpn->pn_dflags |= PND_INITIALIZED;
    if (!Define(argpn, atom, tc))
        return false;

    ParseNode *argsbody = pn->pn_body;
    if (!argsbody) {
        argsbody = ListNode::create(PNK_ARGSBODY, tc);
        if (!argsbody)
            return false;
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








typedef JSBool
(*Binder)(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc);

static JSBool
BindLet(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc);

static JSBool
BindVarOrConst(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc);

struct BindData {
    BindData(JSContext *cx) : let(cx), fresh(true) {}

    ParseNode       *pn;        

    JSOp            op;         
    Binder          binder;     

    struct LetData {
        LetData(JSContext *cx) : blockObj(cx) {}
        VarContext varContext;
        RootedVar<StaticBlockObject*> blockObj;
        unsigned   overflow;
    } let;

    bool fresh;

    void initLet(VarContext varContext, StaticBlockObject &blockObj, unsigned overflow) {
        this->pn = NULL;
        this->op = JSOP_NOP;
        this->binder = BindLet;
        this->let.varContext = varContext;
        this->let.blockObj = &blockObj;
        this->let.overflow = overflow;
    }

    void initVarOrConst(JSOp op) {
        this->op = op;
        this->binder = BindVarOrConst;
    }
};

#if JS_HAS_DESTRUCTURING
static JSBool
BindDestructuringArg(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc)
{
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

    RootedVarObject parent(context);
    parent = tc->inFunction() ? NULL : tc->scopeChain();

    RootedVarFunction fun(context);
    fun = js_NewFunction(context, NULL, NULL, 0,
                         JSFUN_INTERPRETED | (kind == Expression ? JSFUN_LAMBDA : 0),
                         parent, atom);
    if (fun && !tc->compileAndGo()) {
        if (!fun->clearParent(context))
            return NULL;
        if (!fun->clearType(context))
            return NULL;
        fun->setEnvironment(NULL);
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
DeoptimizeUsesWithin(Definition *dn, const TokenPos &pos)
{
    unsigned ndeoptimized = 0;

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
                foundCallee = 1;
                continue;
            }

            Definition *outer_dn = tc->decls.lookupFirst(atom);

            




            if (funtc->bindingsAccessedDynamically() ||
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

            fn->pn_body = NameSetNode::create(PNK_UPVARS, tc);
            if (!fn->pn_body)
                return false;

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

    funbox->bindings.transfer(funtc->parser->context, &funtc->bindings);

    return true;
}

static bool
DefineGlobal(ParseNode *pn, BytecodeEmitter *bce, Handle<PropertyName*> name);







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

                





                BindData data(context);
                data.pn = NULL;
                data.op = JSOP_DEFVAR;
                data.binder = BindDestructuringArg;
                ParseNode *lhs = destructuringExpr(&data, tt);
                if (!lhs)
                    return false;

                



                uint16_t slot;
                if (!funtc.bindings.addDestructuring(context, &slot))
                    return false;

                




                ParseNode *rhs =
                    NameNode::create(PNK_NAME, context->runtime->atomState.emptyAtom, &funtc);
                if (!rhs)
                    return false;
                rhs->setOp(JSOP_GETARG);
                rhs->pn_cookie.set(funtc.staticLevel, slot);
                rhs->pn_dflags |= PND_BOUND;
                rhs->setDefn(true);

                ParseNode *item = new_<BinaryNode>(PNK_ASSIGN, JSOP_NOP, lhs->pn_pos, lhs, rhs);
                if (!item)
                    return false;
                if (!list) {
                    list = ListNode::create(PNK_VAR, &funtc);
                    if (!list)
                        return false;
                    list->makeEmpty();
                    *listp = list;
                }
                list->append(item);
                break;
              }
#endif 

              case TOK_NAME:
              {
                RootedVar<PropertyName*> name(context, tokenStream.currentToken().name());

#ifdef JS_HAS_DESTRUCTURING
                















                if (funtc.decls.lookupFirst(name)) {
                    funtc.bindings.noteDup();
                    duplicatedArg = name;
                    if (destructuringArg)
                        goto report_dup_and_destructuring;
                }
#endif

                uint16_t slot;
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
Parser::functionDef(HandlePropertyName funName, FunctionType type, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, funName);

    
    ParseNode *pn = FunctionNode::create(PNK_FUNCTION, tc);
    if (!pn)
        return NULL;
    pn->pn_body = NULL;
    pn->pn_cookie.makeFree();
    pn->pn_dflags = 0;

    



    bool bodyLevel = tc->atBodyLevel();
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
                fn->setKind(PNK_FUNCTION);
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
            






            unsigned index;
            switch (tc->bindings.lookup(context, funName, &index)) {
              case NONE:
              case ARGUMENT:
                index = tc->bindings.numVars();
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

    RootedVarFunction fun(context, funbox->function());

    
    ParseNode *prelude = NULL;
    if (!functionArguments(funtc, funbox, &prelude))
        return NULL;

    fun->setArgCount(funtc.bindings.numArgs());

#if JS_HAS_DESTRUCTURING
    







    if (prelude) {
        AtomDeclsIter iter(&funtc.decls);
        while (Definition *apn = iter.next()) {
            
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

    FunctionBodyType bodyType = StatementListBody;
#if JS_HAS_EXPR_CLOSURES
    if (tokenStream.getToken(TSF_OPERAND) != TOK_LC) {
        tokenStream.ungetToken();
        fun->flags |= JSFUN_EXPR_CLOSURE;
        bodyType = ExpressionBody;
    }
#else
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_BODY);
#endif

    ParseNode *body = functionBody(bodyType);
    if (!body)
        return NULL;

    if (funName && !CheckStrictBinding(context, &funtc, funName, pn))
        return NULL;

#if JS_HAS_EXPR_CLOSURES
    if (bodyType == StatementListBody)
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
    else if (kind == Statement && !MatchOrInsertSemicolon(context, &tokenStream))
        return NULL;
#else
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
#endif
    pn->pn_pos.end = tokenStream.currentToken().pos.end;

    





    if (funtc.bindingsAccessedDynamically())
        outertc->noteBindingsAccessedDynamically();

#if JS_HAS_DESTRUCTURING
    





    if (prelude) {
        if (!body->isArity(PN_LIST)) {
            ParseNode *block;

            block = ListNode::create(PNK_SEQ, outertc);
            if (!block)
                return NULL;
            block->pn_pos = body->pn_pos;
            block->initList(body);

            body = block;
        }

        ParseNode *item = UnaryNode::create(PNK_SEMI, outertc);
        if (!item)
            return NULL;

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
    }

    JSOp op = JSOP_NOP;
    if (kind == Expression) {
        op = JSOP_LAMBDA;
    } else {
        if (!bodyLevel) {
            





            JS_ASSERT(!outertc->inStrictMode());
            op = JSOP_DEFFUN;
            outertc->noteMightAliasLocals();
            outertc->noteHasExtensibleScope();
            outertc->flags |= TCF_FUN_HEAVYWEIGHT;

            





            if (!outertc->funcStmts) {
                outertc->funcStmts = context->new_<FuncStmtSet>(context);
                if (!outertc->funcStmts || !outertc->funcStmts->init())
                    return NULL;
            }
            if (!outertc->funcStmts->put(funName))
                return NULL;
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
        if (!DefineGlobal(pn, outertc->asBytecodeEmitter(), funName))
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
    RootedVarPropertyName name(context);
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
    RootedVarPropertyName name(context);
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
    JS_CHECK_RECURSION(context, return NULL);

    ParseNode *pn = ListNode::create(PNK_STATEMENTLIST, tc);
    if (!pn)
        return NULL;
    pn->makeEmpty();
    pn->pn_blockid = tc->blockid();
    ParseNode *saveBlock = tc->blockNode;
    tc->blockNode = pn;

    bool inDirectivePrologue = tc->atBodyLevel();
    tokenStream.setOctalCharacterEscape(false);
    for (;;) {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF || tt == TOK_RC) {
            if (tt == TOK_ERROR) {
                if (tokenStream.isEOF())
                    tokenStream.setUnexpectedEOF();
                return NULL;
            }
            break;
        }
        ParseNode *next = statement();
        if (!next) {
            if (tokenStream.isEOF())
                tokenStream.setUnexpectedEOF();
            return NULL;
        }

        if (inDirectivePrologue && !recognizeDirectivePrologue(next, &inDirectivePrologue))
            return NULL;

        if (next->isKind(PNK_FUNCTION)) {
            








            if (tc->atBodyLevel()) {
                pn->pn_xflags |= PNX_FUNCDEFS;
            } else {
                



                JS_ASSERT(tc->hasExtensibleScope());
                tc->flags |= TCF_HAS_FUNCTION_STMT;
            }
        }
        pn->append(next);
    }

    




    if (tc->blockNode != pn)
        pn = tc->blockNode;
    tc->blockNode = saveBlock;

    pn->pn_pos.end = tokenStream.currentToken().pos.end;
    JS_ASSERT(pn->pn_pos.begin <= pn->pn_pos.end);
    return pn;
}

ParseNode *
Parser::condition()
{
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_COND);
    ParseNode *pn = parenExpr();
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    JS_ASSERT_IF(pn->isKind(PNK_ASSIGN), pn->isOp(JSOP_NOP));
    if (pn->isKind(PNK_ASSIGN) &&
        !pn->isInParens() &&
        !reportErrorNumber(NULL, JSREPORT_WARNING | JSREPORT_STRICT, JSMSG_EQUAL_AS_ASSIGN))
    {
        return NULL;
    }
    return pn;
}

static bool
MatchLabel(JSContext *cx, TokenStream *ts, PropertyName **label)
{
    TokenKind tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return false;
    if (tt == TOK_NAME) {
        (void) ts->getToken();
        *label = ts->currentToken().name();
    } else {
        *label = NULL;
    }
    return true;
}

static bool
ReportRedeclaration(JSContext *cx, TreeContext *tc, ParseNode *pn, bool isConst, JSAtom *atom)
{
    JSAutoByteString name;
    if (js_AtomToPrintableString(cx, atom, &name)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, JSMSG_REDECLARED_VAR,
                                 isConst ? "const" : "variable",
                                 name.ptr());
    }
    return false;
}










static JSBool
BindLet(JSContext *cx, BindData *data, JSAtom *atom, TreeContext *tc)
{
    ParseNode *pn = data->pn;
    if (!CheckStrictBinding(cx, tc, atom->asPropertyName(), pn))
        return false;

    RootedVar<StaticBlockObject *> blockObj(cx, data->let.blockObj);
    unsigned blockCount = blockObj->slotCount();
    if (blockCount == JS_BIT(16)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, data->let.overflow);
        return false;
    }

    



    if (data->let.varContext == HoistVars) {
        JS_ASSERT(!tc->atBodyLevel());
        Definition *dn = tc->decls.lookupFirst(atom);
        if (dn && dn->pn_blockid == tc->blockid())
            return ReportRedeclaration(cx, tc, pn, dn->isConst(), atom);
        if (!Define(pn, atom, tc, true))
            return false;
    }

    






    pn->setOp(JSOP_GETLOCAL);
    pn->pn_cookie.set(tc->staticLevel, uint16_t(blockCount));
    pn->pn_dflags |= PND_LET | PND_BOUND;

    



    bool redeclared;
    jsid id = ATOM_TO_JSID(atom);
    const Shape *shape = blockObj->addVar(cx, id, blockCount, &redeclared);
    if (!shape) {
        if (redeclared)
            ReportRedeclaration(cx, tc, pn, false, atom);
        return false;
    }

    
    blockObj->setDefinitionParseNode(blockCount, reinterpret_cast<Definition *>(pn));
    return true;
}

template <class Op>
static inline bool
ForEachLetDef(TreeContext *tc, StaticBlockObject &blockObj, Op op)
{
    for (Shape::Range r = blockObj.lastProperty()->all(); !r.empty(); r.popFront()) {
        const Shape &shape = r.front();

        
        if (JSID_IS_INT(shape.propid()))
            continue;

        if (!op(tc, blockObj, shape, JSID_TO_ATOM(shape.propid())))
            return false;
    }
    return true;
}

struct RemoveDecl {
    bool operator()(TreeContext *tc, StaticBlockObject &, const Shape &, JSAtom *atom) {
        tc->decls.remove(atom);
        return true;
    }
};

static void
PopStatement(TreeContext *tc)
{
    if (tc->topStmt->flags & SIF_SCOPE) {
        StaticBlockObject &blockObj = *tc->topStmt->blockObj;
        JS_ASSERT(!blockObj.inDictionaryMode());
        ForEachLetDef(tc, blockObj, RemoveDecl());
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
DefineGlobal(ParseNode *pn, BytecodeEmitter *bce, Handle<PropertyName*> name)
{
    GlobalScope *globalScope = bce->globalScope;
    HandleObject globalObj = globalScope->globalObj;

    if (!bce->compileAndGo() || !globalObj || bce->compilingForEval())
        return true;

    AtomIndexAddPtr p = globalScope->names.lookupForAdd(name);
    if (!p) {
        JSContext *cx = bce->parser->context;

        JSObject *holder;
        JSProperty *prop;
        if (!globalObj->lookupProperty(cx, name, &holder, &prop))
            return false;

        FunctionBox *funbox = pn->isKind(PNK_FUNCTION) ? pn->pn_funbox : NULL;

        GlobalScope::GlobalDef def;
        if (prop) {
            





            const Shape *shape = (const Shape *)prop;
            if (funbox ||
                globalObj != holder ||
                shape->configurable() ||
                !shape->hasSlot() ||
                !shape->hasDefaultGetter() ||
                !shape->hasDefaultSetter()) {
                return true;
            }

            def = GlobalScope::GlobalDef(shape->slot());
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
        












        if (pn->isKind(PNK_FUNCTION)) {
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

    




    return DefineGlobal(pn, tc->asBytecodeEmitter(),
                        RootedVarPropertyName(cx, pn->pn_atom->asPropertyName()));
}

static bool
BindFunctionLocal(JSContext *cx, BindData *data, MultiDeclRange &mdl, TreeContext *tc)
{
    JS_ASSERT(tc->inFunction());

    ParseNode *pn = data->pn;
    JSAtom *name = pn->pn_atom;

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
                pnu = NameNode::create(PNK_NAME, atom, tc);
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
                ParseNode *pn2 = NameNode::create(PNK_NAME, atom, tc);
                if (!pn2)
                    return JS_FALSE;

                
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
MakeSetCall(JSContext *cx, ParseNode *pn, TreeContext *tc, unsigned msg)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isOp(JSOP_CALL) || pn->isOp(JSOP_EVAL) ||
              pn->isOp(JSOP_FUNCALL) || pn->isOp(JSOP_FUNAPPLY));
    if (!ReportStrictModeError(cx, TS(tc->parser), tc, pn, msg))
        return false;

    ParseNode *pn2 = pn->pn_head;
    if (pn2->isKind(PNK_FUNCTION) && (pn2->pn_funbox->tcflags & TCF_GENEXP_LAMBDA)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR, msg);
        return false;
    }
    pn->pn_xflags |= PNX_SETCALL;
    return true;
}

static void
NoteLValue(JSContext *cx, ParseNode *pn, TreeContext *tc, unsigned dflag = PND_ASSIGNED)
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
    }

    pn->pn_dflags |= dflag;

    







    if (tc->inFunction() && pn->pn_atom == tc->fun()->atom)
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
}

static bool
NoteNameUse(ParseNode *pn, TreeContext *tc)
{
    PropertyName *name = pn->pn_atom->asPropertyName();
    StmtInfo *stmt = LexicalLookup(tc, name, NULL);

    MultiDeclRange mdl = tc->decls.lookupMulti(name);

    Definition *dn;
    if (!mdl.empty()) {
        dn = mdl.front();
    } else {
        if (AtomDefnAddPtr p = tc->lexdeps->lookupForAdd(name)) {
            dn = p.value();
        } else {
            







            dn = MakePlaceholder(pn, tc);
            if (!dn || !tc->lexdeps->add(p, name, dn))
                return false;
        }
    }

    JS_ASSERT(dn->isDefn());
    LinkUseToDef(pn, dn, tc);

    if (stmt && stmt->type == STMT_WITH)
        pn->pn_dflags |= PND_DEOPTIMIZED;

    return true;
}

#if JS_HAS_DESTRUCTURING

static JSBool
BindDestructuringVar(JSContext *cx, BindData *data, ParseNode *pn, TreeContext *tc)
{
    JS_ASSERT(pn->isKind(PNK_NAME));

    data->pn = pn;
    if (!data->binder(cx, data, pn->pn_atom, tc))
        return JS_FALSE;

    



    if (pn->pn_dflags & PND_BOUND) {
        JS_ASSERT(!(pn->pn_dflags & PND_GVAR));
        pn->setOp(JSOP_SETLOCAL);
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
      case PNK_NAME:
        NoteLValue(cx, pn, tc);
        

      case PNK_DOT:
      case PNK_LB:
        




        if (!(js_CodeSpec[pn->getOp()].format & JOF_SET))
            pn->setOp(JSOP_SETNAME);
        break;

      case PNK_LP:
        if (!MakeSetCall(cx, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(pn->isOp(JSOP_XMLNAME));
        pn->setOp(JSOP_BINDXMLNAME);
        break;
#endif

      default:
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return JS_FALSE;
    }

    return JS_TRUE;
}












































static bool
CheckDestructuring(JSContext *cx, BindData *data, ParseNode *left, TreeContext *tc,
                   bool toplevel = true)
{
    bool ok;

    if (left->isKind(PNK_ARRAYCOMP)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), left, JSREPORT_ERROR,
                                 JSMSG_ARRAY_COMP_LEFTSIDE);
        return false;
    }

    RootedVar<StaticBlockObject *> blockObj(cx);
    blockObj = data && data->binder == BindLet ? data->let.blockObj.reference() : NULL;
    uint32_t blockCountBefore = blockObj ? blockObj->slotCount() : 0;

    if (left->isKind(PNK_RB)) {
        for (ParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (!pn->isArrayHole()) {
                if (pn->isKind(PNK_RB) || pn->isKind(PNK_RC)) {
                    ok = CheckDestructuring(cx, data, pn, tc, false);
                } else {
                    if (data) {
                        if (!pn->isKind(PNK_NAME)) {
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
        JS_ASSERT(left->isKind(PNK_RC));
        for (ParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->isKind(PNK_COLON));
            ParseNode *pn = pair->pn_right;

            if (pn->isKind(PNK_RB) || pn->isKind(PNK_RC)) {
                ok = CheckDestructuring(cx, data, pn, tc, false);
            } else if (data) {
                if (!pn->isKind(PNK_NAME)) {
                    ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR,
                                             JSMSG_NO_VARIABLE_NAME);
                    return false;
                }
                ok = BindDestructuringVar(cx, data, pn, tc);
            } else {
                






                if (pair->pn_right == pair->pn_left && !NoteNameUse(pn, tc))
                    return false;
                ok = BindDestructuringLHS(cx, pn, tc);
            }
            if (!ok)
                return false;
        }
    }

    






















    if (toplevel && blockObj && blockCountBefore == blockObj->slotCount()) {
        bool redeclared;
        if (!blockObj->addVar(cx, INT_TO_JSID(blockCountBefore), blockCountBefore, &redeclared))
            return false;
        JS_ASSERT(!redeclared);
        JS_ASSERT(blockObj->slotCount() == blockCountBefore + 1);
    }

    return true;
}


















static void
UndominateInitializers(ParseNode *left, const TokenPtr &end, TreeContext *tc)
{
    if (left->isKind(PNK_RB)) {
        for (ParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (!pn->isKind(PNK_COMMA) || !pn->isArity(PN_NULLARY)) {
                if (pn->isKind(PNK_RB) || pn->isKind(PNK_RC))
                    UndominateInitializers(pn, end, tc);
                else
                    pn->pn_pos.end = end;
            }
        }
    } else {
        JS_ASSERT(left->isKind(PNK_RC));

        for (ParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->isKind(PNK_COLON));
            ParseNode *pn = pair->pn_right;
            if (pn->isKind(PNK_RB) || pn->isKind(PNK_RC))
                UndominateInitializers(pn, end, tc);
            else
                pn->pn_pos.end = end;
        }
    }
}

ParseNode *
Parser::destructuringExpr(BindData *data, TokenKind tt)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(tt));

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
    TokenKind tt = tokenStream.currentToken().type;
    if (!tc->inFunction()) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_RETURN_OR_YIELD,
                          (tt == TOK_RETURN) ? js_return_str : js_yield_str);
        return NULL;
    }

    ParseNode *pn = UnaryNode::create((tt == TOK_RETURN) ? PNK_RETURN : PNK_YIELD, tc);
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

    
    TokenKind tt2 = tokenStream.peekTokenSameLine(TSF_OPERAND);
    if (tt2 == TOK_ERROR)
        return NULL;

    if (tt2 != TOK_EOF && tt2 != TOK_EOL && tt2 != TOK_SEMI && tt2 != TOK_RC
#if JS_HAS_GENERATORS
        && (tt != TOK_YIELD ||
            (tt2 != tt && tt2 != TOK_RB && tt2 != TOK_RP &&
             tt2 != TOK_COLON && tt2 != TOK_COMMA))
#endif
        )
    {
        ParseNode *pn2 = useAssignExpr ? assignExpr() : expr();
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
PushLexicalScope(JSContext *cx, TreeContext *tc, StaticBlockObject &obj, StmtInfo *stmt)
{
    ParseNode *pn = LexicalScopeNode::create(PNK_LEXICALSCOPE, tc);
    if (!pn)
        return NULL;

    ObjectBox *blockbox = tc->parser->newObjectBox(&obj);
    if (!blockbox)
        return NULL;

    PushBlockScope(tc, stmt, obj, -1);
    pn->setOp(JSOP_LEAVEBLOCK);
    pn->pn_objbox = blockbox;
    pn->pn_cookie.makeFree();
    pn->pn_dflags = 0;
    if (!GenerateBlockId(tc, stmt->blockid))
        return NULL;
    pn->pn_blockid = stmt->blockid;
    return pn;
}

static ParseNode *
PushLexicalScope(JSContext *cx, TreeContext *tc, StmtInfo *stmt)
{
    StaticBlockObject *blockObj = StaticBlockObject::create(cx);
    if (!blockObj)
        return NULL;

    return PushLexicalScope(cx, tc, *blockObj, stmt);
}

#if JS_HAS_BLOCK_SCOPE

struct AddDecl
{
    uint32_t blockid;

    AddDecl(uint32_t blockid) : blockid(blockid) {}

    bool operator()(TreeContext *tc, StaticBlockObject &blockObj, const Shape &shape, JSAtom *atom)
    {
        ParseNode *def = (ParseNode *) blockObj.getSlot(shape.slot()).toPrivate();
        def->pn_blockid = blockid;
        return Define(def, atom, tc, true);
    }
};

static ParseNode *
PushLetScope(JSContext *cx, TreeContext *tc, StaticBlockObject &blockObj, StmtInfo *stmt)
{
    ParseNode *pn = PushLexicalScope(cx, tc, blockObj, stmt);
    if (!pn)
        return NULL;

    
    pn->pn_dflags |= PND_LET;

    
    if (!ForEachLetDef(tc, blockObj, AddDecl(stmt->blockid)))
        return NULL;

    return pn;
}






ParseNode *
Parser::letBlock(LetContext letContext)
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_LET);

    ParseNode *pnlet = BinaryNode::create(PNK_LET, tc);
    if (!pnlet)
        return NULL;

    RootedVar<StaticBlockObject*> blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return NULL;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    ParseNode *vars = variables(PNK_LET, blockObj, DontHoistVars);
    if (!vars)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    StmtInfo stmtInfo(context);
    ParseNode *block = PushLetScope(context, tc, *blockObj, &stmtInfo);
    if (!block)
        return NULL;

    pnlet->pn_left = vars;
    pnlet->pn_right = block;

    ParseNode *ret;
    if (letContext == LetStatement && !tokenStream.matchToken(TOK_LC, TSF_OPERAND)) {
        







        if (!ReportStrictModeError(context, &tokenStream, tc, pnlet,
                                   JSMSG_STRICT_CODE_LET_EXPR_STMT)) {
            return NULL;
        }

        




        ParseNode *semi = UnaryNode::create(PNK_SEMI, tc);
        if (!semi)
            return NULL;

        semi->pn_kid = pnlet;

        letContext = LetExpresion;
        ret = semi;
    } else {
        ret = pnlet;
    }

    if (letContext == LetStatement) {
        JS_ASSERT(block->getOp() == JSOP_LEAVEBLOCK);
        block->pn_expr = statements();
        if (!block->pn_expr)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_LET);
    } else {
        JS_ASSERT(letContext == LetExpresion);
        block->setOp(JSOP_LEAVEBLOCKEXPR);
        block->pn_expr = assignExpr();
        if (!block->pn_expr)
            return NULL;
    }

    PopStatement(tc);
    return ret;
}

#endif 

static bool
PushBlocklikeStatement(StmtInfo *stmt, StmtType type, TreeContext *tc)
{
    PushStatement(tc, stmt, type, -1);
    return GenerateBlockId(tc, stmt->blockid);
}

static ParseNode *
NewBindingNode(JSAtom *atom, TreeContext *tc, StaticBlockObject *blockObj = NULL,
               VarContext varContext = HoistVars)
{
    






    if (!blockObj || varContext == HoistVars) {
        ParseNode *pn = tc->decls.lookupFirst(atom);
        AtomDefnPtr removal;
        if (pn) {
            JS_ASSERT(!pn->isPlaceholder());
        } else {
            removal = tc->lexdeps->lookup(atom);
            pn = removal ? removal.value() : NULL;
            JS_ASSERT_IF(pn, pn->isPlaceholder());
        }

        if (pn) {
            JS_ASSERT(pn->isDefn());

            





            JS_ASSERT_IF(blockObj && pn->pn_blockid == tc->blockid(),
                         pn->pn_blockid != tc->bodyid);

            if (pn->isPlaceholder() && pn->pn_blockid >= tc->blockid()) {
                pn->pn_blockid = tc->blockid();
                tc->lexdeps->remove(removal);
                return pn;
            }
        }
    }

    
    JS_ASSERT(tc->parser->tokenStream.currentToken().type == TOK_NAME);
    return NameNode::create(PNK_NAME, atom, tc);
}

ParseNode *
Parser::switchStatement()
{
    JS_ASSERT(tc->parser->tokenStream.currentToken().type == TOK_SWITCH);
    ParseNode *pn = BinaryNode::create(PNK_SWITCH, tc);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

    
    ParseNode *pn1 = parenExpr();
    if (!pn1)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

    



    StmtInfo stmtInfo(context);
    PushStatement(tc, &stmtInfo, STMT_SWITCH, -1);

    
    ParseNode *pn2 = ListNode::create(PNK_STATEMENTLIST, tc);
    if (!pn2)
        return NULL;
    pn2->makeEmpty();
    if (!GenerateBlockIdForStmtNode(pn2, tc))
        return NULL;
    ParseNode *saveBlock = tc->blockNode;
    tc->blockNode = pn2;

    bool seenDefault = false;
    TokenKind tt;
    while ((tt = tokenStream.getToken()) != TOK_RC) {
        ParseNode *pn3;
        switch (tt) {
          case TOK_DEFAULT:
            if (seenDefault) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_DEFAULTS);
                return NULL;
            }
            seenDefault = true;
            pn3 = BinaryNode::create(PNK_DEFAULT, tc);
            if (!pn3)
                return NULL;
            break;

          case TOK_CASE:
          {
            pn3 = BinaryNode::create(PNK_CASE, tc);
            if (!pn3)
                return NULL;
            pn3->pn_left = expr();
            if (!pn3->pn_left)
                return NULL;
            break;
          }

          case TOK_ERROR:
            return NULL;

          default:
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_SWITCH);
            return NULL;
        }

        pn2->append(pn3);
        if (pn2->pn_count == JS_BIT(16)) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_CASES);
            return NULL;
        }

        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        ParseNode *pn4 = ListNode::create(PNK_STATEMENTLIST, tc);
        if (!pn4)
            return NULL;
        pn4->makeEmpty();
        while ((tt = tokenStream.peekToken(TSF_OPERAND)) != TOK_RC &&
               tt != TOK_CASE && tt != TOK_DEFAULT) {
            if (tt == TOK_ERROR)
                return NULL;
            ParseNode *pn5 = statement();
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

bool
Parser::matchInOrOf(bool *isForOfp)
{
    if (tokenStream.matchToken(TOK_IN)) {
        *isForOfp = false;
        return true;
    }
    if (tokenStream.matchToken(TOK_NAME)) {
        if (tokenStream.currentToken().name() == context->runtime->atomState.ofAtom) {
            *isForOfp = true;
            return true;
        }
        tokenStream.ungetToken();
    }
    return false;
}

ParseNode *
Parser::forStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    
    ParseNode *pn = BinaryNode::create(PNK_FOR, tc);
    if (!pn)
        return NULL;

    StmtInfo forStmt(context);
    PushStatement(tc, &forStmt, STMT_FOR_LOOP, -1);

    pn->setOp(JSOP_ITER);
    pn->pn_iflags = 0;
    if (tokenStream.matchToken(TOK_NAME)) {
        if (tokenStream.currentToken().name() == context->runtime->atomState.eachAtom)
            pn->pn_iflags = JSITER_FOREACH;
        else
            tokenStream.ungetToken();
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    



    bool forDecl = false;

    
    RootedVar<StaticBlockObject*> blockObj(context);

    
    ParseNode *pn1;

    {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            if (pn->pn_iflags & JSITER_FOREACH) {
                reportErrorNumber(pn, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
                return NULL;
            }

            pn1 = NULL;
        } else {
            












            tc->flags |= TCF_IN_FOR_INIT;
            if (tt == TOK_VAR || tt == TOK_CONST) {
                forDecl = true;
                tokenStream.consumeKnownToken(tt);
                pn1 = variables(tt == TOK_VAR ? PNK_VAR : PNK_CONST);
            }
#if JS_HAS_BLOCK_SCOPE
            else if (tt == TOK_LET) {
                (void) tokenStream.getToken();
                if (tokenStream.peekToken() == TOK_LP) {
                    pn1 = letBlock(LetExpresion);
                } else {
                    forDecl = true;
                    blockObj = StaticBlockObject::create(context);
                    if (!blockObj)
                        return NULL;
                    pn1 = variables(PNK_LET, blockObj, DontHoistVars);
                }
            }
#endif
            else {
                pn1 = expr();
            }
            tc->flags &= ~TCF_IN_FOR_INIT;
            if (!pn1)
                return NULL;
        }
    }

    JS_ASSERT_IF(forDecl, pn1->isArity(PN_LIST));
    JS_ASSERT(!!blockObj == (forDecl && pn1->isOp(JSOP_NOP)));

    const TokenPos pos = tokenStream.currentToken().pos;

    
    ParseNode *forParent = NULL;

    





    ParseNode *forHead;        
    StmtInfo letStmt(context); 
    ParseNode *pn2, *pn3;      
    bool forOf;
    if (pn1 && matchInOrOf(&forOf)) {
        







        forStmt.type = STMT_FOR_IN_LOOP;

        
        if (forOf && pn->pn_iflags != 0) {
            JS_ASSERT(pn->pn_iflags == JSITER_FOREACH);
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
            return NULL;
        }
        pn->pn_iflags |= (forOf ? JSITER_FOR_OF : JSITER_ENUMERATE);

        
        if (forDecl
            ? (pn1->pn_count > 1 || pn1->isOp(JSOP_DEFCONST)
#if JS_HAS_DESTRUCTURING
               || (versionNumber() == JSVERSION_1_7 &&
                   pn->isOp(JSOP_ITER) &&
                   !(pn->pn_iflags & JSITER_FOREACH) &&
                   (pn1->pn_head->isKind(PNK_RC) ||
                    (pn1->pn_head->isKind(PNK_RB) &&
                     pn1->pn_head->pn_count != 2) ||
                    (pn1->pn_head->isKind(PNK_ASSIGN) &&
                     (!pn1->pn_head->pn_left->isKind(PNK_RB) ||
                      pn1->pn_head->pn_left->pn_count != 2))))
#endif
              )
            : (!pn1->isKind(PNK_NAME) &&
               !pn1->isKind(PNK_DOT) &&
#if JS_HAS_DESTRUCTURING
               ((versionNumber() == JSVERSION_1_7 &&
                 pn->isOp(JSOP_ITER) &&
                 !(pn->pn_iflags & JSITER_FOREACH))
                ? (!pn1->isKind(PNK_RB) || pn1->pn_count != 2)
                : (!pn1->isKind(PNK_RB) && !pn1->isKind(PNK_RC))) &&
#endif
               !pn1->isKind(PNK_LP) &&
#if JS_HAS_XML_SUPPORT
               !pn1->isKind(PNK_XMLUNARY) &&
#endif
               !pn1->isKind(PNK_LB)))
        {
            reportErrorNumber(pn1, JSREPORT_ERROR, JSMSG_BAD_FOR_LEFTSIDE);
            return NULL;
        }

        





        pn2 = NULL;
        unsigned dflag = PND_ASSIGNED;
        if (forDecl) {
            
            pn1->pn_xflags |= PNX_FORINVAR;

            pn2 = pn1->pn_head;
            if ((pn2->isKind(PNK_NAME) && pn2->maybeExpr())
#if JS_HAS_DESTRUCTURING
                || pn2->isKind(PNK_ASSIGN)
#endif
                )
            {
                






#if JS_HAS_BLOCK_SCOPE
                if (blockObj) {
                    reportErrorNumber(pn2, JSREPORT_ERROR, JSMSG_INVALID_FOR_IN_INIT);
                    return NULL;
                }
#endif 

                ParseNode *pnseq = ListNode::create(PNK_SEQ, tc);
                if (!pnseq)
                    return NULL;

                dflag = PND_INITIALIZED;

                







                pn1->pn_xflags &= ~PNX_FORINVAR;
                pn1->pn_xflags |= PNX_POPVAR;
                pnseq->initList(pn1);
                pn1 = NULL;

#if JS_HAS_DESTRUCTURING
                if (pn2->isKind(PNK_ASSIGN)) {
                    pn2 = pn2->pn_left;
                    JS_ASSERT(pn2->isKind(PNK_RB) || pn2->isKind(PNK_RC) ||
                              pn2->isKind(PNK_NAME));
                }
#endif
                pnseq->append(pn);
                forParent = pnseq;
            }
        } else {
            
            JS_ASSERT(!blockObj);
            pn2 = pn1;
            pn1 = NULL;

            if (!setAssignmentLhsOps(pn2, JSOP_NOP))
                return NULL;
        }

        pn3 = expr();
        if (!pn3)
            return NULL;

        if (blockObj) {
            





            ParseNode *block = PushLetScope(context, tc, *blockObj, &letStmt);
            if (!block)
                return NULL;
            letStmt.flags |= SIF_FOR_BLOCK;
            block->pn_expr = pn1;
            pn1 = block;
        }

        if (forDecl) {
            





            pn2 = CloneLeftHandSide(pn2, tc);
            if (!pn2)
                return NULL;
        }

        switch (pn2->getKind()) {
          case PNK_NAME:
            
            NoteLValue(context, pn2, tc, dflag);
            break;

#if JS_HAS_DESTRUCTURING
          case PNK_ASSIGN:
            JS_NOT_REACHED("forStatement TOK_ASSIGN");
            break;

          case PNK_RB:
          case PNK_RC:
            if (versionNumber() == JSVERSION_1_7) {
                



                JS_ASSERT(pn->isOp(JSOP_ITER));
                if (!(pn->pn_iflags & JSITER_FOREACH))
                    pn->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          default:;
        }

        forHead = TernaryNode::create(PNK_FORIN, tc);
        if (!forHead)
            return NULL;
    } else {
        if (blockObj) {
            



            ParseNode *block = PushLetScope(context, tc, *blockObj, &letStmt);
            if (!block)
                return NULL;
            letStmt.flags |= SIF_FOR_BLOCK;

            ParseNode *let = new_<BinaryNode>(PNK_LET, JSOP_NOP, pos, pn1, block);
            if (!let)
                return NULL;

            pn1 = NULL;
            block->pn_expr = pn;
            forParent = let;
        }

        if (pn->pn_iflags & JSITER_FOREACH) {
            reportErrorNumber(pn, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
            return NULL;
        }
        pn->setOp(JSOP_NOP);

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        if (tokenStream.peekToken(TSF_OPERAND) == TOK_SEMI) {
            pn2 = NULL;
        } else {
            pn2 = expr();
            if (!pn2)
                return NULL;
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        if (tokenStream.peekToken(TSF_OPERAND) == TOK_RP) {
            pn3 = NULL;
        } else {
            pn3 = expr();
            if (!pn3)
                return NULL;
        }

        forHead = TernaryNode::create(PNK_FORHEAD, tc);
        if (!forHead)
            return NULL;
    }

    forHead->pn_pos = pos;
    forHead->setOp(JSOP_NOP);
    forHead->pn_kid1 = pn1;
    forHead->pn_kid2 = pn2;
    forHead->pn_kid3 = pn3;
    pn->pn_left = forHead;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    
    ParseNode *body = statement();
    if (!body)
        return NULL;

    
    pn->pn_pos.end = body->pn_pos.end;
    pn->pn_right = body;

    if (forParent) {
        forParent->pn_pos.begin = pn->pn_pos.begin;
        forParent->pn_pos.end = pn->pn_pos.end;
    }

#if JS_HAS_BLOCK_SCOPE
    if (blockObj)
        PopStatement(tc);
#endif
    PopStatement(tc);
    return forParent ? forParent : pn;
}

ParseNode *
Parser::tryStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_TRY));

    
















    ParseNode *pn = TernaryNode::create(PNK_TRY, tc);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_NOP);

    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    StmtInfo stmtInfo(context);
    if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, tc))
        return NULL;
    pn->pn_kid1 = statements();
    if (!pn->pn_kid1)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatement(tc);

    ParseNode *lastCatch;
    ParseNode *catchList = NULL;
    TokenKind tt = tokenStream.getToken();
    if (tt == TOK_CATCH) {
        catchList = ListNode::create(PNK_CATCHLIST, tc);
        if (!catchList)
            return NULL;
        catchList->makeEmpty();
        lastCatch = NULL;

        do {
            ParseNode *pnblock;
            BindData data(context);

            
            if (lastCatch && !lastCatch->pn_kid2) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_AFTER_GENERAL);
                return NULL;
            }

            



            pnblock = PushLexicalScope(context, tc, &stmtInfo);
            if (!pnblock)
                return NULL;
            stmtInfo.type = STMT_CATCH;

            






            ParseNode *pn2 = TernaryNode::create(PNK_CATCH, tc);
            if (!pn2)
                return NULL;
            pnblock->pn_expr = pn2;
            MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_CATCH);

            




            data.initLet(HoistVars, *tc->blockChain, JSMSG_TOO_MANY_CATCH_VARS);
            JS_ASSERT(data.let.blockObj && data.let.blockObj == pnblock->pn_objbox->object);

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
                pn3 = NewBindingNode(label, tc);
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
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_WITH));

    







    if (tc->flags & TCF_STRICT_MODE_CODE) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_STRICT_CODE_WITH);
        return NULL;
    }

    ParseNode *pn = BinaryNode::create(PNK_WITH, tc);
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

    StmtInfo stmtInfo(context);
    PushStatement(tc, &stmtInfo, STMT_WITH, -1);
    pn2 = statement();
    if (!pn2)
        return NULL;
    PopStatement(tc);

    pn->pn_pos.end = pn2->pn_pos.end;
    pn->pn_right = pn2;

    tc->noteBindingsAccessedDynamically();
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
            pn = letBlock(LetStatement);
            if (!pn)
                return NULL;

            JS_ASSERT(pn->isKind(PNK_LET) || pn->isKind(PNK_SEMI));
            if (pn->isKind(PNK_LET) && pn->pn_expr->getOp() == JSOP_LEAVEBLOCK)
                return pn;

            
            JS_ASSERT(pn->isKind(PNK_SEMI) || pn->isOp(JSOP_NOP));
            break;
        }

        










        StmtInfo *stmt = tc->topStmt;
        if (stmt &&
            (!STMT_MAYBE_SCOPE(stmt) || (stmt->flags & SIF_FOR_BLOCK))) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LET_DECL_NOT_IN_BLOCK);
            return NULL;
        }

        if (stmt && (stmt->flags & SIF_SCOPE)) {
            JS_ASSERT(tc->blockChain == stmt->blockObj);
        } else {
            if (!stmt || (stmt->flags & SIF_BODY_BLOCK)) {
                



                pn = variables(PNK_VAR);
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

            
            StaticBlockObject *blockObj = StaticBlockObject::create(tc->parser->context);
            if (!blockObj)
                return NULL;

            ObjectBox *blockbox = tc->parser->newObjectBox(blockObj);
            if (!blockbox)
                return NULL;

            





            stmt->flags |= SIF_SCOPE;
            stmt->downScope = tc->topScopeStmt;
            tc->topScopeStmt = stmt;

            blockObj->setEnclosingBlock(tc->blockChain);
            tc->blockChain = blockObj;
            stmt->blockObj = blockObj;

#ifdef DEBUG
            ParseNode *tmp = tc->blockNode;
            JS_ASSERT(!tmp || !tmp->isKind(PNK_LEXICALSCOPE));
#endif

            
            ParseNode *pn1 = LexicalScopeNode::create(PNK_LEXICALSCOPE, tc);
            if (!pn1)
                return NULL;

            pn1->setOp(JSOP_LEAVEBLOCK);
            pn1->pn_pos = tc->blockNode->pn_pos;
            pn1->pn_objbox = blockbox;
            pn1->pn_expr = tc->blockNode;
            pn1->pn_blockid = tc->blockNode->pn_blockid;
            tc->blockNode = pn1;
        }

        pn = variables(PNK_LET, tc->blockChain, HoistVars);
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
        if (!pn2->isKind(PNK_NAME)) {
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

        
        StmtInfo stmtInfo(context);
        PushStatement(tc, &stmtInfo, STMT_LABEL, -1);
        stmtInfo.label = label;
        ParseNode *pn = statement();
        if (!pn)
            return NULL;

        
        if (pn->isKind(PNK_SEMI) && !pn->pn_kid) {
            pn->setKind(PNK_STATEMENTLIST);
            pn->setArity(PN_LIST);
            pn->makeEmpty();
        }

        
        PopStatement(tc);
        pn2->setKind(PNK_COLON);
        pn2->pn_pos.end = pn->pn_pos.end;
        pn2->pn_expr = pn;
        return pn2;
    }

    ParseNode *pn = UnaryNode::create(PNK_SEMI, tc);
    if (!pn)
        return NULL;
    pn->pn_pos = pn2->pn_pos;
    pn->pn_kid = pn2;

    
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
        if (!tc->inStrictMode()) {
            TokenKind tt = tokenStream.peekToken(TSF_KEYWORD_IS_NAME);
            if (tt == TOK_DBLCOLON)
                return expressionStatement();
        }
#endif
        return functionStmt();
      }

      case TOK_IF:
      {
        
        pn = TernaryNode::create(PNK_IF, tc);
        if (!pn)
            return NULL;
        ParseNode *pn1 = condition();
        if (!pn1)
            return NULL;

        StmtInfo stmtInfo(context);
        PushStatement(tc, &stmtInfo, STMT_IF, -1);
        ParseNode *pn2 = statement();
        if (!pn2)
            return NULL;

        if (pn2->isKind(PNK_SEMI) &&
            !pn2->pn_kid &&
            !reportErrorNumber(NULL, JSREPORT_WARNING | JSREPORT_STRICT, JSMSG_EMPTY_CONSEQUENT))
        {
            return NULL;
        }

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
        pn = BinaryNode::create(PNK_WHILE, tc);
        if (!pn)
            return NULL;
        StmtInfo stmtInfo(context);
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
        pn = BinaryNode::create(PNK_DOWHILE, tc);
        if (!pn)
            return NULL;
        StmtInfo stmtInfo(context);
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
        pn = UnaryNode::create(PNK_THROW, tc);
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
        TokenPtr begin = tokenStream.currentToken().pos.begin;
        PropertyName *label;
        if (!MatchLabel(context, &tokenStream, &label))
            return NULL;
        TokenPtr end = tokenStream.currentToken().pos.end;
        pn = new_<BreakStatement>(label, begin, end);
        if (!pn)
            return NULL;
        StmtInfo *stmt = tc->topStmt;
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
        break;
      }

      case TOK_CONTINUE:
      {
        TokenPtr begin = tokenStream.currentToken().pos.begin;
        PropertyName *label;
        if (!MatchLabel(context, &tokenStream, &label))
            return NULL;
        TokenPtr end = tokenStream.currentToken().pos.begin;
        pn = new_<ContinueStatement>(label, begin, end);
        if (!pn)
            return NULL;
        StmtInfo *stmt = tc->topStmt;
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
        break;
      }

      case TOK_WITH:
        return withStatement();

      case TOK_VAR:
        pn = variables(PNK_VAR);
        if (!pn)
            return NULL;

        
        pn->pn_xflags |= PNX_POPVAR;
        break;

      case TOK_CONST:
        pn = variables(PNK_CONST);
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
        unsigned oldflags;

        oldflags = tc->flags;
        tc->flags = oldflags & ~TCF_HAS_FUNCTION_STMT;
        StmtInfo stmtInfo(context);
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
        pn = UnaryNode::create(PNK_SEMI, tc);
        if (!pn)
            return NULL;
        return pn;

      case TOK_DEBUGGER:
        pn = tc->parser->new_<DebuggerStatement>(tokenStream.currentToken().pos);
        if (!pn)
            return NULL;
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_DEFAULT:
      {
        if (tc->inStrictMode())
            return expressionStatement();

        pn = UnaryNode::create(PNK_DEFXMLNS, tc);
        if (!pn)
            return NULL;
        if (!tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().name() != context->runtime->atomState.xmlAtom ||
            !tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().name() != context->runtime->atomState.namespaceAtom ||
            !tokenStream.matchToken(TOK_ASSIGN))
        {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_DEFAULT_XML_NAMESPACE);
            return NULL;
        }

        JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

        
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
        return expressionStatement();
    }

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}






ParseNode *
Parser::variables(ParseNodeKind kind, StaticBlockObject *blockObj, VarContext varContext)
{
    






    JS_ASSERT(kind == PNK_VAR || kind == PNK_CONST || kind == PNK_LET || kind == PNK_LP);

    ParseNode *pn = ListNode::create(kind, tc);
    if (!pn)
        return NULL;

    pn->setOp(blockObj ? JSOP_NOP : kind == PNK_VAR ? JSOP_DEFVAR : JSOP_DEFCONST);
    pn->makeEmpty();

    




    BindData data(context);
    if (blockObj)
        data.initLet(varContext, *blockObj, JSMSG_TOO_MANY_LOCALS);
    else
        data.initVarOrConst(pn->getOp());

    ParseNode *pn2;
    do {
        TokenKind tt = tokenStream.getToken();
#if JS_HAS_DESTRUCTURING
        if (tt == TOK_LB || tt == TOK_LC) {
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn2 = primaryExpr(tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn2)
                return NULL;

            if (!CheckDestructuring(context, &data, pn2, tc))
                return NULL;
            bool ignored;
            if ((tc->flags & TCF_IN_FOR_INIT) && matchInOrOf(&ignored)) {
                tokenStream.ungetToken();
                pn->append(pn2);
                continue;
            }

            MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

            ParseNode *init = assignExpr();
            if (!init)
                return NULL;
            UndominateInitializers(pn2, init->pn_pos.end, tc);

            pn2 = ParseNode::newBinaryOrAppend(PNK_ASSIGN, JSOP_NOP, pn2, init, tc);
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
        pn2 = NewBindingNode(name, tc, blockObj, varContext);
        if (!pn2)
            return NULL;
        if (data.op == JSOP_DEFCONST)
            pn2->pn_dflags |= PND_CONST;
        data.pn = pn2;
        if (!data.binder(context, &data, name, tc))
            return NULL;
        pn->append(pn2);

        if (tokenStream.matchToken(TOK_ASSIGN)) {
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

            ParseNode *init = assignExpr();
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

            pn2->setOp((pn2->pn_dflags & PND_BOUND)
                       ? JSOP_SETLOCAL
                       : (data.op == JSOP_DEFCONST)
                       ? JSOP_SETCONST
                       : JSOP_SETNAME);

            NoteLValue(context, pn2, tc, data.fresh ? PND_INITIALIZED : PND_ASSIGNED);

            
            pn2->pn_pos.end = init->pn_pos.end;
        }
    } while (tokenStream.matchToken(TOK_COMMA));

    pn->pn_pos.end = pn->last()->pn_pos.end;
    return pn;
}

ParseNode *
Parser::expr()
{
    ParseNode *pn = assignExpr();
    if (pn && tokenStream.matchToken(TOK_COMMA)) {
        ParseNode *pn2 = ListNode::create(PNK_COMMA, tc);
        if (!pn2)
            return NULL;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->initList(pn);
        pn = pn2;
        do {
#if JS_HAS_GENERATORS
            pn2 = pn->last();
            if (pn2->isKind(PNK_YIELD) && !pn2->isInParens()) {
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
    ParseNode *pn = unaryExpr();

    




    TokenKind tt;
    while (pn && ((tt = tokenStream.getToken()) == TOK_STAR || tt == TOK_DIV || tt == TOK_MOD)) {
        ParseNodeKind kind = (tt == TOK_STAR)
                             ? PNK_STAR
                             : (tt == TOK_DIV)
                             ? PNK_DIV
                             : PNK_MOD;
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(kind, op, pn, unaryExpr(), tc);
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
        ParseNodeKind kind = (tt == TOK_PLUS) ? PNK_ADD : PNK_SUB;
        pn = ParseNode::newBinaryOrAppend(kind, op, pn, mulExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(addExpr1)

inline ParseNodeKind
ShiftTokenToParseNodeKind(const Token &token)
{
    switch (token.type) {
      case TOK_LSH:
        return PNK_LSH;
      case TOK_RSH:
        return PNK_RSH;
      default:
        JS_ASSERT(token.type == TOK_URSH);
        return PNK_URSH;
    }
}

BEGIN_EXPR_PARSER(shiftExpr1)
{
    ParseNode *left = addExpr1i();
    while (left && tokenStream.isCurrentTokenShift()) {
        ParseNodeKind kind = ShiftTokenToParseNodeKind(tokenStream.currentToken());
        JSOp op = tokenStream.currentToken().t_op;
        ParseNode *right = addExpr1n();
        if (!right)
            return NULL;
        left = tc->parser->new_<BinaryNode>(kind, op, left, right);
    }
    return left;
}
END_EXPR_PARSER(shiftExpr1)

inline ParseNodeKind
RelationalTokenToParseNodeKind(const Token &token)
{
    switch (token.type) {
      case TOK_IN:
        return PNK_IN;
      case TOK_INSTANCEOF:
        return PNK_INSTANCEOF;
      case TOK_LT:
        return PNK_LT;
      case TOK_LE:
        return PNK_LE;
      case TOK_GT:
        return PNK_GT;
      default:
        JS_ASSERT(token.type == TOK_GE);
        return PNK_GE;
    }
}

BEGIN_EXPR_PARSER(relExpr1)
{
    unsigned inForInitFlag = tc->flags & TCF_IN_FOR_INIT;

    



    tc->flags &= ~TCF_IN_FOR_INIT;

    ParseNode *pn = shiftExpr1i();
    while (pn &&
           (tokenStream.isCurrentTokenRelational() ||
            



            (inForInitFlag == 0 && tokenStream.isCurrentTokenType(TOK_IN)) ||
            tokenStream.isCurrentTokenType(TOK_INSTANCEOF))) {
        ParseNodeKind kind = RelationalTokenToParseNodeKind(tokenStream.currentToken());
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(kind, op, pn, shiftExpr1n(), tc);
    }
    
    tc->flags |= inForInitFlag;

    return pn;
}
END_EXPR_PARSER(relExpr1)

inline ParseNodeKind
EqualityTokenToParseNodeKind(const Token &token)
{
    switch (token.type) {
      case TOK_STRICTEQ:
        return PNK_STRICTEQ;
      case TOK_EQ:
        return PNK_EQ;
      case TOK_STRICTNE:
        return PNK_STRICTNE;
      default:
        JS_ASSERT(token.type == TOK_NE);
        return PNK_NE;
    }
}

BEGIN_EXPR_PARSER(eqExpr1)
{
    ParseNode *left = relExpr1i();
    while (left && tokenStream.isCurrentTokenEquality()) {
        ParseNodeKind kind = EqualityTokenToParseNodeKind(tokenStream.currentToken());
        JSOp op = tokenStream.currentToken().t_op;
        ParseNode *right = relExpr1n();
        if (!right)
            return NULL;
        left = tc->parser->new_<BinaryNode>(kind, op, left, right);
    }
    return left;
}
END_EXPR_PARSER(eqExpr1)

BEGIN_EXPR_PARSER(bitAndExpr1)
{
    ParseNode *pn = eqExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITAND))
        pn = ParseNode::newBinaryOrAppend(PNK_BITAND, JSOP_BITAND, pn, eqExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitAndExpr1)

BEGIN_EXPR_PARSER(bitXorExpr1)
{
    ParseNode *pn = bitAndExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITXOR))
        pn = ParseNode::newBinaryOrAppend(PNK_BITXOR, JSOP_BITXOR, pn, bitAndExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitXorExpr1)

BEGIN_EXPR_PARSER(bitOrExpr1)
{
    ParseNode *pn = bitXorExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITOR))
        pn = ParseNode::newBinaryOrAppend(PNK_BITOR, JSOP_BITOR, pn, bitXorExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitOrExpr1)

BEGIN_EXPR_PARSER(andExpr1)
{
    ParseNode *pn = bitOrExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_AND))
        pn = ParseNode::newBinaryOrAppend(PNK_AND, JSOP_AND, pn, bitOrExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(andExpr1)

JS_ALWAYS_INLINE ParseNode *
Parser::orExpr1()
{
    ParseNode *pn = andExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_OR))
        pn = ParseNode::newBinaryOrAppend(PNK_OR, JSOP_OR, pn, andExpr1n(), tc);
    return pn;
}

JS_ALWAYS_INLINE ParseNode *
Parser::condExpr1()
{
    ParseNode *condition = orExpr1();
    if (!condition || !tokenStream.isCurrentTokenType(TOK_HOOK))
        return condition;

    




    unsigned oldflags = tc->flags;
    tc->flags &= ~TCF_IN_FOR_INIT;
    ParseNode *thenExpr = assignExpr();
    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    if (!thenExpr)
        return NULL;

    MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_IN_COND);

    ParseNode *elseExpr = assignExpr();
    if (!elseExpr)
        return NULL;

    tokenStream.getToken(); 
    return new_<ConditionalExpression>(condition, thenExpr, elseExpr);
}

bool
Parser::setAssignmentLhsOps(ParseNode *pn, JSOp op)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        if (!CheckStrictAssignment(context, tc, pn))
            return false;
        pn->setOp(pn->isOp(JSOP_GETLOCAL) ? JSOP_SETLOCAL : JSOP_SETNAME);
        NoteLValue(context, pn, tc);
        break;
      case PNK_DOT:
        pn->setOp(JSOP_SETPROP);
        break;
      case PNK_LB:
        pn->setOp(JSOP_SETELEM);
        break;
#if JS_HAS_DESTRUCTURING
      case PNK_RB:
      case PNK_RC:
        if (op != JSOP_NOP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_DESTRUCT_ASS);
            return false;
        }
        if (!CheckDestructuring(context, NULL, pn, tc))
            return false;
        break;
#endif
      case PNK_LP:
        if (!MakeSetCall(context, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(pn->isOp(JSOP_XMLNAME));
        pn->setOp(JSOP_SETXMLNAME);
        break;
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

    ParseNode *lhs = condExpr1();
    if (!lhs)
        return NULL;

    ParseNodeKind kind;
    switch (tokenStream.currentToken().type) {
      case TOK_ASSIGN:       kind = PNK_ASSIGN;       break;
      case TOK_ADDASSIGN:    kind = PNK_ADDASSIGN;    break;
      case TOK_SUBASSIGN:    kind = PNK_SUBASSIGN;    break;
      case TOK_BITORASSIGN:  kind = PNK_BITORASSIGN;  break;
      case TOK_BITXORASSIGN: kind = PNK_BITXORASSIGN; break;
      case TOK_BITANDASSIGN: kind = PNK_BITANDASSIGN; break;
      case TOK_LSHASSIGN:    kind = PNK_LSHASSIGN;    break;
      case TOK_RSHASSIGN:    kind = PNK_RSHASSIGN;    break;
      case TOK_URSHASSIGN:   kind = PNK_URSHASSIGN;   break;
      case TOK_MULASSIGN:    kind = PNK_MULASSIGN;    break;
      case TOK_DIVASSIGN:    kind = PNK_DIVASSIGN;    break;
      case TOK_MODASSIGN:    kind = PNK_MODASSIGN;    break;
      default:
        JS_ASSERT(!tokenStream.isCurrentTokenAssignment());
        tokenStream.ungetToken();
        return lhs;
    }

    JSOp op = tokenStream.currentToken().t_op;
    if (!setAssignmentLhsOps(lhs, op))
        return NULL;

    ParseNode *rhs = assignExpr();
    if (!rhs)
        return NULL;
    if (lhs->isKind(PNK_NAME) && lhs->isUsed()) {
        Definition *dn = lhs->pn_lexdef;

        






        if (!dn->isAssigned()) {
            JS_ASSERT(dn->isInitialized());
            dn->pn_pos.end = rhs->pn_pos.end;
        }
    }

    return ParseNode::newBinaryOrAppend(kind, op, lhs, rhs, tc);
}

static bool
SetLvalKid(JSContext *cx, TokenStream *ts, TreeContext *tc, ParseNode *pn, ParseNode *kid,
           const char *name)
{
    if (!kid->isKind(PNK_NAME) &&
        !kid->isKind(PNK_DOT) &&
        (!kid->isKind(PNK_LP) ||
         (!kid->isOp(JSOP_CALL) && !kid->isOp(JSOP_EVAL) &&
          !kid->isOp(JSOP_FUNCALL) && !kid->isOp(JSOP_FUNAPPLY))) &&
#if JS_HAS_XML_SUPPORT
        !kid->isKind(PNK_XMLUNARY) &&
#endif
        !kid->isKind(PNK_LB))
    {
        ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR, JSMSG_BAD_OPERAND, name);
        return false;
    }
    if (!CheckStrictAssignment(cx, tc, kid))
        return false;
    pn->pn_kid = kid;
    return true;
}

static const char incop_name_str[][10] = {"increment", "decrement"};

static JSBool
SetIncOpKid(JSContext *cx, TokenStream *ts, TreeContext *tc, ParseNode *pn, ParseNode *kid,
            TokenKind tt, bool preorder)
{
    JSOp op;

    if (!SetLvalKid(cx, ts, tc, pn, kid, incop_name_str[tt == TOK_DEC]))
        return false;
    switch (kid->getKind()) {
      case PNK_NAME:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCNAME : JSOP_NAMEINC)
             : (preorder ? JSOP_DECNAME : JSOP_NAMEDEC);
        NoteLValue(cx, kid, tc);
        break;

      case PNK_DOT:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCPROP : JSOP_PROPINC)
             : (preorder ? JSOP_DECPROP : JSOP_PROPDEC);
        break;

      case PNK_LP:
        if (!MakeSetCall(cx, kid, tc, JSMSG_BAD_INCOP_OPERAND))
            return JS_FALSE;
        
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        if (kid->isOp(JSOP_XMLNAME))
            kid->setOp(JSOP_SETXMLNAME);
        
#endif
      case PNK_LB:
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
Parser::unaryOpExpr(ParseNodeKind kind, JSOp op)
{
    TokenPtr begin = tokenStream.currentToken().pos.begin;
    ParseNode *kid = unaryExpr();
    if (!kid)
        return NULL;
    return new_<UnaryNode>(kind, op, TokenPos::make(begin, kid->pn_pos.end), kid);
}

ParseNode *
Parser::unaryExpr()
{
    ParseNode *pn, *pn2;

    JS_CHECK_RECURSION(context, return NULL);

    switch (TokenKind tt = tokenStream.getToken(TSF_OPERAND)) {
      case TOK_TYPEOF:
        return unaryOpExpr(PNK_TYPEOF, JSOP_TYPEOF);
      case TOK_VOID:
        return unaryOpExpr(PNK_VOID, JSOP_VOID);
      case TOK_NOT:
        return unaryOpExpr(PNK_NOT, JSOP_NOT);
      case TOK_BITNOT:
        return unaryOpExpr(PNK_BITNOT, JSOP_BITNOT);
      case TOK_PLUS:
        return unaryOpExpr(PNK_POS, JSOP_POS);
      case TOK_MINUS:
        return unaryOpExpr(PNK_NEG, JSOP_NEG);

      case TOK_INC:
      case TOK_DEC:
        pn = UnaryNode::create((tt == TOK_INC) ? PNK_PREINCREMENT : PNK_PREDECREMENT, tc);
        if (!pn)
            return NULL;
        pn2 = memberExpr(JS_TRUE);
        if (!pn2)
            return NULL;
        if (!SetIncOpKid(context, &tokenStream, tc, pn, pn2, tt, true))
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        break;

      case TOK_DELETE:
      {
        pn = UnaryNode::create(PNK_DELETE, tc);
        if (!pn)
            return NULL;
        pn2 = unaryExpr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;

        




        if (foldConstants && !FoldConstants(context, pn2, tc))
            return NULL;
        switch (pn2->getKind()) {
          case PNK_LP:
            if (!(pn2->pn_xflags & PNX_SETCALL)) {
                



                if (!MakeSetCall(context, pn2, tc, JSMSG_BAD_DELETE_OPERAND))
                    return NULL;
                pn2->pn_xflags &= ~PNX_SETCALL;
            }
            break;
          case PNK_NAME:
            if (!ReportStrictModeError(context, &tokenStream, tc, pn,
                                       JSMSG_DEPRECATED_DELETE_OPERAND)) {
                return NULL;
            }
            pn2->setOp(JSOP_DELNAME);
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
                tokenStream.consumeKnownToken(tt);
                pn2 = UnaryNode::create((tt == TOK_INC) ? PNK_POSTINCREMENT : PNK_POSTDECREMENT, tc);
                if (!pn2)
                    return NULL;
                if (!SetIncOpKid(context, &tokenStream, tc, pn2, pn, tt, false))
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
    unsigned           adjust;
    unsigned           funcLevel;

  public:
    CompExprTransplanter(ParseNode *pn, TreeContext *tc, bool ge, unsigned adj)
      : root(pn), tc(tc), genexp(ge), adjust(adj), funcLevel(0)
    {
    }

    bool transplant(ParseNode *pn);
};


















class GenexpGuard {
    TreeContext     *tc;
    uint32_t        startYieldCount;

  public:
    explicit GenexpGuard(TreeContext *tc)
      : tc(tc)
    {
        if (tc->parenDepth == 0) {
            tc->yieldCount = 0;
            tc->yieldNode = tc->argumentsNode = NULL;
        }
        startYieldCount = tc->yieldCount;
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
        unsigned level = pn->pn_cookie.level() + 1;

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
AdjustBlockId(ParseNode *pn, unsigned adjust, TreeContext *tc)
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
Parser::comprehensionTail(ParseNode *kid, unsigned blockid, bool isGenexp,
                          ParseNodeKind kind, JSOp op)
{
    unsigned adjust;
    ParseNode *pn, *pn2, *pn3, **pnp;
    StmtInfo stmtInfo(context);
    BindData data(context);
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_FOR);

    if (kind == PNK_SEMI) {
        




        pn = PushLexicalScope(context, tc, &stmtInfo);
        if (!pn)
            return NULL;
        adjust = pn->pn_blockid - blockid;
    } else {
        JS_ASSERT(kind == PNK_ARRAYPUSH);

        











        adjust = tc->blockid();
        pn = PushLexicalScope(context, tc, &stmtInfo);
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

    CompExprTransplanter transplanter(kid, tc, kind == PNK_SEMI, adjust);
    transplanter.transplant(kid);

    JS_ASSERT(tc->blockChain && tc->blockChain == pn->pn_objbox->object);
    data.initLet(HoistVars, *tc->blockChain, JSMSG_ARRAY_INIT_TOO_BIG);

    do {
        




        pn2 = BinaryNode::create(PNK_FOR, tc);
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

        RootedVarPropertyName name(context);
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

            






            pn3 = NewBindingNode(name, tc);
            if (!pn3)
                return NULL;
            break;

          default:
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return NULL;
        }

        bool forOf;
        if (!matchInOrOf(&forOf)) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_IN_AFTER_FOR_NAME);
            return NULL;
        }
        if (forOf) {
            if (pn2->pn_iflags != JSITER_ENUMERATE) {
                JS_ASSERT(pn2->pn_iflags == (JSITER_FOREACH | JSITER_ENUMERATE));
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
                return NULL;
            }
            pn2->pn_iflags = JSITER_FOR_OF;
        }

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
                
                if (!pn3->isKind(PNK_RB) || pn3->pn_count != 2) {
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

        



        ParseNode *vars = ListNode::create(PNK_VAR, tc);
        if (!vars)
            return NULL;
        vars->setOp(JSOP_NOP);
        vars->pn_pos = pn3->pn_pos;
        vars->makeEmpty();
        vars->append(pn3);
        vars->pn_xflags |= PNX_FORINVAR;

        
        pn3 = CloneLeftHandSide(pn3, tc);
        if (!pn3)
            return NULL;

        pn2->pn_left = new_<TernaryNode>(PNK_FORIN, JSOP_NOP, vars, pn3, pn4);
        if (!pn2->pn_left)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_right;
    } while (tokenStream.matchToken(TOK_FOR));

    if (tokenStream.matchToken(TOK_IF)) {
        pn2 = TernaryNode::create(PNK_IF, tc);
        if (!pn2)
            return NULL;
        pn2->pn_kid1 = condition();
        if (!pn2->pn_kid1)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_kid2;
    }

    pn2 = UnaryNode::create(kind, tc);
    if (!pn2)
        return NULL;
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
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    
    ParseNode *pn = UnaryNode::create(PNK_YIELD, tc);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_YIELD);
    pn->setInParens(true);
    pn->pn_pos = kid->pn_pos;
    pn->pn_kid = kid;
    pn->pn_hidden = true;

    
    ParseNode *genfn = FunctionNode::create(PNK_FUNCTION, tc);
    if (!genfn)
        return NULL;
    genfn->setOp(JSOP_LAMBDA);
    JS_ASSERT(!genfn->pn_body);
    genfn->pn_dflags = 0;

    {
        TreeContext *outertc = tc;
        TreeContext gentc(tc->parser);
        if (!gentc.init(context))
            return NULL;

        FunctionBox *funbox = EnterFunction(genfn, &gentc);
        if (!funbox)
            return NULL;

        





        gentc.flags |= TCF_FUN_IS_GENERATOR | TCF_GENEXP_LAMBDA |
                       (outertc->flags & TCF_FUN_FLAGS);
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

        JSAtom *arguments = gentc.parser->context->runtime->atomState.argumentsAtom;
        if (AtomDefnPtr p = gentc.lexdeps->lookup(arguments)) {
            Definition *dn = p.value();
            ParseNode *errorNode = dn->dn_uses ? dn->dn_uses : body;
            gentc.parser->reportErrorNumber(errorNode, JSREPORT_ERROR, JSMSG_BAD_GENEXP_BODY,
                                             js_arguments_str);
            return NULL;
        }

        if (!LeaveFunction(genfn, &gentc))
            return NULL;
    }

    



    ParseNode *result = ListNode::create(PNK_LP, tc);
    if (!result)
        return NULL;
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
        if (argNode->isKind(PNK_YIELD) &&
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

ParseNode *
Parser::memberExpr(JSBool allowCallSyntax)
{
    ParseNode *lhs;

    JS_CHECK_RECURSION(context, return NULL);

    
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    if (tt == TOK_NEW) {
        lhs = ListNode::create(PNK_NEW, tc);
        if (!lhs)
            return NULL;
        ParseNode *ctorExpr = memberExpr(JS_FALSE);
        if (!ctorExpr)
            return NULL;
        lhs->setOp(JSOP_NEW);
        lhs->initList(ctorExpr);
        lhs->pn_pos.begin = ctorExpr->pn_pos.begin;

        if (tokenStream.matchToken(TOK_LP) && !argumentList(lhs))
            return NULL;
        if (lhs->pn_count > ARGC_LIMIT) {
            JS_ReportErrorNumber(context, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_MANY_CON_ARGS);
            return NULL;
        }
        lhs->pn_pos.end = lhs->last()->pn_pos.end;
    } else {
        lhs = primaryExpr(tt, JS_FALSE);
        if (!lhs)
            return NULL;

        if (lhs->isXMLNameOp()) {
            lhs = new_<UnaryNode>(PNK_XMLUNARY, JSOP_XMLNAME, lhs->pn_pos, lhs);
            if (!lhs)
                return NULL;
        }
    }

    while ((tt = tokenStream.getToken()) > TOK_EOF) {
        ParseNode *nextMember;
        if (tt == TOK_DOT) {
            tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            if (tt == TOK_ERROR)
                return NULL;
            if (tt == TOK_NAME) {
#if JS_HAS_XML_SUPPORT
                if (!tc->inStrictMode() && tokenStream.peekToken() == TOK_DBLCOLON) {
                    ParseNode *propertyId = propertyQualifiedIdentifier();
                    if (!propertyId)
                        return NULL;

                    nextMember = new_<XMLDoubleColonProperty>(lhs, propertyId,
                                                              lhs->pn_pos.begin,
                                                              tokenStream.currentToken().pos.end);
                    if (!nextMember)
                        return NULL;
                } else
#endif
                {
                    PropertyName *field = tokenStream.currentToken().name();
                    nextMember = new_<PropertyAccess>(lhs, field,
                                                      lhs->pn_pos.begin,
                                                      tokenStream.currentToken().pos.end);
                    if (!nextMember)
                        return NULL;
                }
            }
#if JS_HAS_XML_SUPPORT
            else if (!tc->inStrictMode()) {
                TokenPtr begin = lhs->pn_pos.begin;
                if (tt == TOK_LP) {
                    
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                    tc->noteBindingsAccessedDynamically();

                    StmtInfo stmtInfo(context);
                    ParseNode *oldWith = tc->innermostWith;
                    tc->innermostWith = lhs;
                    PushStatement(tc, &stmtInfo, STMT_WITH, -1);

                    ParseNode *filter = bracketedExpr();
                    if (!filter)
                        return NULL;
                    filter->setInParens(true);
                    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);

                    tc->innermostWith = oldWith;
                    PopStatement(tc);

                    nextMember =
                        new_<XMLFilterExpression>(lhs, filter,
                                                  begin, tokenStream.currentToken().pos.end);
                    if (!nextMember)
                        return NULL;
                } else if (tt == TOK_AT || tt == TOK_STAR) {
                    ParseNode *propertyId = starOrAtPropertyIdentifier(tt);
                    if (!propertyId)
                        return NULL;
                    nextMember = new_<XMLProperty>(lhs, propertyId,
                                                   begin, tokenStream.currentToken().pos.end);
                    if (!nextMember)
                        return NULL;
                } else {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                    return NULL;
                }
            }
#endif
            else {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }
        }
#if JS_HAS_XML_SUPPORT
        else if (tt == TOK_DBLDOT) {
            if (tc->inStrictMode()) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }

            nextMember = BinaryNode::create(PNK_DBLDOT, tc);
            if (!nextMember)
                return NULL;
            tt = tokenStream.getToken(TSF_OPERAND | TSF_KEYWORD_IS_NAME);
            ParseNode *pn3 = primaryExpr(tt, JS_TRUE);
            if (!pn3)
                return NULL;
            if (pn3->isKind(PNK_NAME) && !pn3->isInParens()) {
                pn3->setKind(PNK_STRING);
                pn3->setArity(PN_NULLARY);
                pn3->setOp(JSOP_QNAMEPART);
            } else if (!pn3->isXMLPropertyIdentifier()) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }
            nextMember->setOp(JSOP_DESCENDANTS);
            nextMember->pn_left = lhs;
            nextMember->pn_right = pn3;
            nextMember->pn_pos.begin = lhs->pn_pos.begin;
            nextMember->pn_pos.end = tokenStream.currentToken().pos.end;
        }
#endif
        else if (tt == TOK_LB) {
            ParseNode *propExpr = expr();
            if (!propExpr)
                return NULL;

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_IN_INDEX);
            TokenPtr begin = lhs->pn_pos.begin, end = tokenStream.currentToken().pos.end;

            






            uint32_t index;
            PropertyName *name = NULL;
            if (propExpr->isKind(PNK_STRING)) {
                JSAtom *atom = propExpr->pn_atom;
                if (atom->isIndex(&index)) {
                    propExpr->setKind(PNK_NUMBER);
                    propExpr->setOp(JSOP_DOUBLE);
                    propExpr->pn_dval = index;
                } else {
                    name = atom->asPropertyName();
                }
            } else if (propExpr->isKind(PNK_NUMBER)) {
                JSAtom *atom;
                if (!js_ValueToAtom(context, NumberValue(propExpr->pn_dval), &atom))
                    return NULL;
                if (!atom->isIndex(&index))
                    name = atom->asPropertyName();
            }

            if (name)
                nextMember = new_<PropertyAccess>(lhs, name, begin, end);
            else
                nextMember = new_<PropertyByValue>(lhs, propExpr, begin, end);
            if (!nextMember)
                return NULL;
        } else if (allowCallSyntax && tt == TOK_LP) {
            nextMember = ListNode::create(PNK_LP, tc);
            if (!nextMember)
                return NULL;
            nextMember->setOp(JSOP_CALL);

            if (lhs->isOp(JSOP_NAME)) {
                if (lhs->pn_atom == context->runtime->atomState.evalAtom) {
                    
                    nextMember->setOp(JSOP_EVAL);
                    tc->noteBindingsAccessedDynamically();
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                    



                    if (!tc->inStrictMode())
                        tc->noteHasExtensibleScope();
                }
            } else if (lhs->isOp(JSOP_GETPROP)) {
                
                if (lhs->pn_atom == context->runtime->atomState.applyAtom)
                    nextMember->setOp(JSOP_FUNAPPLY);
                else if (lhs->pn_atom == context->runtime->atomState.callAtom)
                    nextMember->setOp(JSOP_FUNCALL);
            }

            nextMember->initList(lhs);
            nextMember->pn_pos.begin = lhs->pn_pos.begin;

            if (!argumentList(nextMember))
                return NULL;
            if (nextMember->pn_count > ARGC_LIMIT) {
                JS_ReportErrorNumber(context, js_GetErrorMessage, NULL,
                                     JSMSG_TOO_MANY_FUN_ARGS);
                return NULL;
            }
            nextMember->pn_pos.end = tokenStream.currentToken().pos.end;
        } else {
            tokenStream.ungetToken();
            return lhs;
        }

        lhs = nextMember;
    }
    if (tt == TOK_ERROR)
        return NULL;
    return lhs;
}

ParseNode *
Parser::bracketedExpr()
{
    unsigned oldflags;
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
    JS_ASSERT(!tc->inStrictMode());

    ParseNode *pn = bracketedExpr();
    if (!pn)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ATTR_EXPR);
    return pn;
}




















































ParseNode *
Parser::propertySelector()
{
    JS_ASSERT(!tc->inStrictMode());

    ParseNode *selector;
    if (tokenStream.isCurrentTokenType(TOK_STAR)) {
        selector = NullaryNode::create(PNK_ANYNAME, tc);
        if (!selector)
            return NULL;
        selector->setOp(JSOP_ANYNAME);
        selector->pn_atom = context->runtime->atomState.starAtom;
    } else {
        JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));
        selector = NullaryNode::create(PNK_NAME, tc);
        if (!selector)
            return NULL;
        selector->setOp(JSOP_QNAMEPART);
        selector->setArity(PN_NAME);
        selector->pn_atom = tokenStream.currentToken().name();
        selector->pn_cookie.makeFree();
    }
    return selector;
}

ParseNode *
Parser::qualifiedSuffix(ParseNode *pn)
{
    JS_ASSERT(!tc->inStrictMode());

    JS_ASSERT(tokenStream.currentToken().type == TOK_DBLCOLON);
    ParseNode *pn2 = NameNode::create(PNK_DBLCOLON, NULL, tc);
    if (!pn2)
        return NULL;

    tc->flags |= TCF_FUN_HEAVYWEIGHT;
    tc->noteBindingsAccessedDynamically();

    
    if (pn->isOp(JSOP_QNAMEPART))
        pn->setOp(JSOP_NAME);

    TokenKind tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
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
    ParseNode *pn3 = endBracketedExpr();
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
    JS_ASSERT(!tc->inStrictMode());

    ParseNode *pn = propertySelector();
    if (!pn)
        return NULL;
    if (tokenStream.matchToken(TOK_DBLCOLON)) {
        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        tc->noteBindingsAccessedDynamically();
        pn = qualifiedSuffix(pn);
    }
    return pn;
}

ParseNode *
Parser::attributeIdentifier()
{
    JS_ASSERT(!tc->inStrictMode());

    JS_ASSERT(tokenStream.currentToken().type == TOK_AT);
    ParseNode *pn = UnaryNode::create(PNK_AT, tc);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_TOATTRNAME);

    ParseNode *pn2;
    TokenKind tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
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
    JS_ASSERT(!tc->inStrictMode());

    JS_ASSERT(tokenStream.currentToken().type == TOK_LC);
    ParseNode *pn = UnaryNode::create(PNK_XMLCURLYEXPR, tc);
    if (!pn)
        return NULL;

    





    bool oldflag = tokenStream.isXMLTagMode();
    tokenStream.setXMLTagMode(false);
    ParseNode *pn2 = expr();
    if (!pn2)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_XML_EXPR);
    tokenStream.setXMLTagMode(oldflag);
    pn->pn_kid = pn2;
    pn->setOp(inTag ? JSOP_XMLTAGEXPR : JSOP_XMLELTEXPR);
    return pn;
}

ParseNode *
Parser::atomNode(ParseNodeKind kind, JSOp op)
{
    ParseNode *node = NullaryNode::create(kind, tc);
    if (!node)
        return NULL;
    node->setOp(op);
    const Token &tok = tokenStream.currentToken();
    node->pn_atom = tok.atom();
    return node;
}













ParseNode *
Parser::xmlNameExpr()
{
    JS_ASSERT(!tc->inStrictMode());

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
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_STRING);
            pn2 = atomNode(PNK_XMLNAME, JSOP_STRING);
            if (!pn2)
                return NULL;
        }

        if (!pn) {
            pn = pn2;
        } else {
            if (!list) {
                list = ListNode::create(PNK_XMLNAME, tc);
                if (!list)
                    return NULL;
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
                                 : !(pn)->isKind(PNK_XMLCURLYEXPR))


















ParseNode *
Parser::xmlTagContent(ParseNodeKind tagkind, JSAtom **namep)
{
    JS_ASSERT(!tc->inStrictMode());

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
            list = ListNode::create(tagkind, tc);
            if (!list)
                return NULL;
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
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_STRING);
            pn2 = atomNode(PNK_XMLATTR, JSOP_STRING);
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
    JS_ASSERT(!tc->inStrictMode());

    tokenStream.setXMLTagMode(false);
    for (;;) {
        TokenKind tt = tokenStream.getToken(TSF_XMLTEXTMODE);
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);

        JS_ASSERT(tt == TOK_XMLSPACE || tt == TOK_XMLTEXT);
        JSAtom *textAtom = tokenStream.currentToken().atom();
        if (textAtom) {
            
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_STRING);
            ParseNode *pn2 = atomNode(tt == TOK_XMLSPACE ? PNK_XMLSPACE : PNK_XMLTEXT,
                                      JSOP_STRING);
            if (!pn2)
                return false;
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
            if (!pn2)
                return false;
            pn->pn_xflags |= PNX_CANTFOLD;
        } else if (tt == TOK_XMLSTAGO) {
            pn2 = xmlElementOrList(JS_FALSE);
            if (!pn2)
                return false;
            pn2->pn_xflags &= ~PNX_XMLROOT;
            pn->pn_xflags |= pn2->pn_xflags;
        } else if (tt == TOK_XMLPI) {
            const Token &tok = tokenStream.currentToken();
            pn2 = new_<XMLProcessingInstruction>(tok.xmlPITarget(), tok.xmlPIData(), tok.pos);
            if (!pn2)
                return false;
        } else {
            JS_ASSERT(tt == TOK_XMLCDATA || tt == TOK_XMLCOMMENT);
            pn2 = atomNode(tt == TOK_XMLCDATA ? PNK_XMLCDATA : PNK_XMLCOMMENT,
                           tokenStream.currentToken().t_op);
            if (!pn2)
                return false;
        }
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
    JS_ASSERT(!tc->inStrictMode());

    ParseNode *pn, *pn2, *list;
    TokenKind tt;
    RootedVarAtom startAtom(context), endAtom(context);

    JS_CHECK_RECURSION(context, return NULL);

    JS_ASSERT(tokenStream.currentToken().type == TOK_XMLSTAGO);
    pn = ListNode::create(PNK_XMLSTAGO, tc);
    if (!pn)
        return NULL;

    tokenStream.setXMLTagMode(true);
    tt = tokenStream.getToken();
    if (tt == TOK_ERROR)
        return NULL;

    if (tt == TOK_XMLNAME || tt == TOK_LC) {
        


        pn2 = xmlTagContent(PNK_XMLSTAGO, startAtom.address());
        if (!pn2)
            return NULL;
        tokenStream.matchToken(TOK_XMLSPACE);

        tt = tokenStream.getToken();
        if (tt == TOK_XMLPTAGC) {
            
            if (pn2->isKind(PNK_XMLSTAGO)) {
                pn->makeEmpty();
                freeTree(pn);
                pn = pn2;
            } else {
                JS_ASSERT(pn2->isKind(PNK_XMLNAME) || pn2->isKind(PNK_XMLCURLYEXPR));
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
            }
            pn->setKind(PNK_XMLPTAGC);
            pn->pn_xflags |= PNX_XMLROOT;
        } else {
            
            if (tt != TOK_XMLTAGC) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;

            
            if (!pn2->isKind(PNK_XMLSTAGO)) {
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
                pn2 = pn;
                pn = ListNode::create(PNK_XMLTAGC, tc);
                if (!pn)
                    return NULL;
            }

            
            pn->setKind(PNK_XMLELEM);
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

            
            pn2 = xmlTagContent(PNK_XMLETAGO, endAtom.address());
            if (!pn2)
                return NULL;
            if (pn2->isKind(PNK_XMLETAGO)) {
                
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            if (endAtom && startAtom && endAtom != startAtom) {
                
                reportErrorNumber(pn2, JSREPORT_UC | JSREPORT_ERROR, JSMSG_XML_TAG_NAME_MISMATCH,
                                  startAtom->chars());
                return NULL;
            }

            
            JS_ASSERT(pn2->isKind(PNK_XMLNAME) || pn2->isKind(PNK_XMLCURLYEXPR));
            list = ListNode::create(PNK_XMLETAGO, tc);
            if (!list)
                return NULL;
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
        
        pn->setKind(PNK_XMLLIST);
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
    JS_ASSERT(!tc->inStrictMode());

    





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
    JS_ASSERT(!xmltc.inStrictMode());
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

bool
Parser::checkForFunctionNode(PropertyName *name, ParseNode *node)
{
    





    if (const KeywordInfo *ki = FindKeyword(name->charsZ(), name->length())) {
        if (ki->tokentype != TOK_FUNCTION) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_KEYWORD_NOT_NS);
            return false;
        }

        node->setArity(PN_NULLARY);
        node->setKind(PNK_FUNCTION);
    }

    return true;
}

#if JS_HAS_XML_SUPPORT
ParseNode *
Parser::propertyQualifiedIdentifier()
{
    JS_ASSERT(!tc->inStrictMode());
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));
    JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
    JS_ASSERT(tokenStream.peekToken() == TOK_DBLCOLON);

    
    tc->flags |= TCF_FUN_HEAVYWEIGHT;
    tc->noteBindingsAccessedDynamically();

    PropertyName *name = tokenStream.currentToken().name();
    ParseNode *node = NameNode::create(PNK_NAME, name, tc);
    if (!node)
        return NULL;
    node->setOp(JSOP_NAME);
    node->pn_dflags |= PND_DEOPTIMIZED;

    if (!checkForFunctionNode(name, node))
        return NULL;

    tokenStream.consumeKnownToken(TOK_DBLCOLON);
    return qualifiedSuffix(node);
}
#endif

ParseNode *
Parser::identifierName(bool afterDoubleDot)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));

    PropertyName *name = tokenStream.currentToken().name();
    ParseNode *node = NameNode::create(PNK_NAME, name, tc);
    if (!node)
        return NULL;
    JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
    node->setOp(JSOP_NAME);

    if ((!afterDoubleDot
#if JS_HAS_XML_SUPPORT
                || (!tc->inStrictMode() && tokenStream.peekToken() == TOK_DBLCOLON)
#endif
               ) && !(tc->flags & TCF_DECL_DESTRUCTURING))
    {
        if (!NoteNameUse(node, tc))
            return NULL;
    }

#if JS_HAS_XML_SUPPORT
    if (!tc->inStrictMode() && tokenStream.matchToken(TOK_DBLCOLON)) {
        if (afterDoubleDot) {
            if (!checkForFunctionNode(name, node))
                return NULL;
        }
        node = qualifiedSuffix(node);
        if (!node)
            return NULL;
    }
#endif

    return node;
}

#if JS_HAS_XML_SUPPORT
ParseNode *
Parser::starOrAtPropertyIdentifier(TokenKind tt)
{
    JS_ASSERT(tt == TOK_AT || tt == TOK_STAR);
    if (tc->inStrictMode()) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    return (tt == TOK_AT) ? attributeIdentifier() : qualifiedIdentifier();
}
#endif

ParseNode *
Parser::primaryExpr(TokenKind tt, bool afterDoubleDot)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(tt));

    ParseNode *pn, *pn2, *pn3;
    JSOp op;

    JS_CHECK_RECURSION(context, return NULL);

    switch (tt) {
      case TOK_FUNCTION:
#if JS_HAS_XML_SUPPORT
        if (!tc->inStrictMode() && tokenStream.matchToken(TOK_DBLCOLON, TSF_KEYWORD_IS_NAME)) {
            pn2 = NullaryNode::create(PNK_FUNCTION, tc);
            if (!pn2)
                return NULL;
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
        unsigned index;

        pn = ListNode::create(PNK_RB, tc);
        if (!pn)
            return NULL;
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
                    pn2 = NullaryNode::create(PNK_COMMA, tc);
                    pn->pn_xflags |= PNX_HOLEY | PNX_NONCONST;
                } else {
                    pn2 = assignExpr();
                    if (pn2) {
                        if (foldConstants && !FoldConstants(context, pn2, tc))
                            return NULL;
                        if (!pn2->isConstant())
                            pn->pn_xflags |= PNX_NONCONST;
                    }
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

                
                pn->setKind(PNK_ARRAYCOMP);

                




                pnexp = pn->last();
                JS_ASSERT(pn->pn_count == 1);
                pn->pn_count = 0;
                pn->pn_tail = &pn->pn_head;
                *pn->pn_tail = NULL;

                pntop = comprehensionTail(pnexp, pn->pn_blockid, false,
                                          PNK_ARRAYPUSH, JSOP_ARRAYPUSH);
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

        pn = ListNode::create(PNK_RC, tc);
        if (!pn)
            return NULL;
        pn->setOp(JSOP_NEWINIT);
        pn->makeEmpty();

        for (;;) {
            JSAtom *atom;
            TokenKind ltok = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            TokenPtr begin = tokenStream.currentToken().pos.begin;
            switch (ltok) {
              case TOK_NUMBER:
                pn3 = NullaryNode::create(PNK_NUMBER, tc);
                if (!pn3)
                    return NULL;
                pn3->pn_dval = tokenStream.currentToken().number();
                if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                    return NULL;
                break;
              case TOK_NAME:
                {
                    atom = tokenStream.currentToken().name();
                    if (atom == context->runtime->atomState.getAtom) {
                        op = JSOP_GETTER;
                    } else if (atom == context->runtime->atomState.setAtom) {
                        op = JSOP_SETTER;
                    } else {
                        pn3 = NullaryNode::create(PNK_NAME, tc);
                        if (!pn3)
                            return NULL;
                        pn3->pn_atom = atom;
                        break;
                    }

                    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
                    if (tt == TOK_NAME) {
                        atom = tokenStream.currentToken().name();
                        pn3 = NameNode::create(PNK_NAME, atom, tc);
                        if (!pn3)
                            return NULL;
                    } else if (tt == TOK_STRING) {
                        atom = tokenStream.currentToken().atom();

                        uint32_t index;
                        if (atom->isIndex(&index)) {
                            pn3 = NullaryNode::create(PNK_NUMBER, tc);
                            if (!pn3)
                                return NULL;
                            pn3->pn_dval = index;
                            if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                                return NULL;
                        } else {
                            pn3 = NameNode::create(PNK_STRING, atom, tc);
                            if (!pn3)
                                return NULL;
                        }
                    } else if (tt == TOK_NUMBER) {
                        pn3 = NullaryNode::create(PNK_NUMBER, tc);
                        if (!pn3)
                            return NULL;
                        pn3->pn_dval = tokenStream.currentToken().number();
                        if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                            return NULL;
                    } else {
                        tokenStream.ungetToken();
                        pn3 = NullaryNode::create(PNK_NAME, tc);
                        if (!pn3)
                            return NULL;
                        pn3->pn_atom = atom;
                        break;
                    }

                    pn->pn_xflags |= PNX_NONCONST;

                    
                    pn2 = functionDef(RootedVarPropertyName(context, NULL),
                                      op == JSOP_GETTER ? Getter : Setter, Expression);
                    if (!pn2)
                        return NULL;
                    TokenPos pos = {begin, pn2->pn_pos.end};
                    pn2 = new_<BinaryNode>(PNK_COLON, op, pos, pn3, pn2);
                    goto skip;
                }
              case TOK_STRING: {
                atom = tokenStream.currentToken().atom();
                uint32_t index;
                if (atom->isIndex(&index)) {
                    pn3 = NullaryNode::create(PNK_NUMBER, tc);
                    if (!pn3)
                        return NULL;
                    pn3->pn_dval = index;
                } else {
                    pn3 = NullaryNode::create(PNK_STRING, tc);
                    if (!pn3)
                        return NULL;
                    pn3->pn_atom = atom;
                }
                break;
              }
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
                if (!pnval)
                    return NULL;

                if (foldConstants && !FoldConstants(context, pnval, tc))
                    return NULL;

                




                if (!pnval->isConstant() || atom == context->runtime->atomState.protoAtom)
                    pn->pn_xflags |= PNX_NONCONST;
            }
#if JS_HAS_DESTRUCTURING_SHORTHAND
            else if (ltok == TOK_NAME && (tt == TOK_COMMA || tt == TOK_RC)) {
                



                tokenStream.ungetToken();
                if (!tokenStream.checkForKeyword(atom->charsZ(), atom->length(), NULL, NULL))
                    return NULL;
                pn->pn_xflags |= PNX_DESTRUCT | PNX_NONCONST;
                pnval = pn3;
                JS_ASSERT(pnval->isKind(PNK_NAME));
                pnval->setArity(PN_NAME);
                ((NameNode *)pnval)->initCommon(tc);
            }
#endif
            else {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_COLON_AFTER_ID);
                return NULL;
            }

            {
                TokenPos pos = {begin, pnval->pn_pos.end};
                pn2 = new_<BinaryNode>(PNK_COLON, op, pos, pn3, pnval);
            }
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

                    unsigned flags = (oldAssignType == VALUE &&
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
        pn = letBlock(LetExpresion);
        if (!pn)
            return NULL;
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

      case TOK_STRING:
        pn = atomNode(PNK_STRING, JSOP_STRING);
        if (!pn)
            return NULL;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_AT:
      case TOK_STAR:
        pn = starOrAtPropertyIdentifier(tt);
        break;

      case TOK_XMLSTAGO:
        pn = xmlElementOrListRoot(JS_TRUE);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLCDATA:
        JS_ASSERT(!tc->inStrictMode());
        pn = atomNode(PNK_XMLCDATA, JSOP_XMLCDATA);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLCOMMENT:
        JS_ASSERT(!tc->inStrictMode());
        pn = atomNode(PNK_XMLCOMMENT, JSOP_XMLCOMMENT);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLPI: {
        JS_ASSERT(!tc->inStrictMode());
        const Token &tok = tokenStream.currentToken();
        pn = new_<XMLProcessingInstruction>(tok.xmlPITarget(), tok.xmlPIData(), tok.pos);
        if (!pn)
            return NULL;
        break;
      }
#endif

      case TOK_NAME:
        pn = identifierName(afterDoubleDot);
        break;

      case TOK_REGEXP:
      {
        pn = NullaryNode::create(PNK_REGEXP, tc);
        if (!pn)
            return NULL;

        const jschar *chars = tokenStream.getTokenbuf().begin();
        size_t length = tokenStream.getTokenbuf().length();
        RegExpFlag flags = tokenStream.currentToken().regExpFlags();
        RegExpStatics *res = context->regExpStatics();

        RootedVar<RegExpObject*> reobj(context);
        if (context->hasfp())
            reobj = RegExpObject::create(context, res, chars, length, flags, &tokenStream);
        else
            reobj = RegExpObject::createNoStatics(context, chars, length, flags, &tokenStream);

        if (!reobj)
            return NULL;

        if (!tc->compileAndGo()) {
            if (!reobj->clearParent(context))
                return NULL;
            if (!reobj->clearType(context))
                return NULL;
        }

        pn->pn_objbox = tc->parser->newObjectBox(reobj);
        if (!pn->pn_objbox)
            return NULL;

        pn->setOp(JSOP_REGEXP);
        break;
      }

      case TOK_NUMBER:
        pn = NullaryNode::create(PNK_NUMBER, tc);
        if (!pn)
            return NULL;
        pn->setOp(JSOP_DOUBLE);
        pn->pn_dval = tokenStream.currentToken().number();
        break;

      case TOK_TRUE:
        return new_<BooleanLiteral>(true, tokenStream.currentToken().pos);
      case TOK_FALSE:
        return new_<BooleanLiteral>(false, tokenStream.currentToken().pos);
      case TOK_THIS:
        return new_<ThisLiteral>(tokenStream.currentToken().pos);
      case TOK_NULL:
        return new_<NullLiteral>(tokenStream.currentToken().pos);

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
        JS_ASSERT(!pn->isKind(PNK_YIELD));
        if (pn->isKind(PNK_COMMA) && !pn->isInParens()) {
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
