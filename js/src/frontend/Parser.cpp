



















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

#include "frontend/FoldConstants.h"
#include "frontend/ParseMaps.h"
#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#include "jsatominlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/Parser-inl.h"
#include "frontend/TreeContext-inl.h"

#include "vm/NumericConversions.h"
#include "vm/RegExpObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::frontend;





#define MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, __flags)                                     \
    JS_BEGIN_MACRO                                                                          \
        if (tokenStream.getToken((__flags)) != tt) {                                        \
            reportError(NULL, errno);                                                       \
            return NULL;                                                                    \
        }                                                                                   \
    JS_END_MACRO
#define MUST_MATCH_TOKEN(tt, errno) MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, 0)

StrictMode::StrictModeState
StrictModeGetter::get() const
{
    return parser->tc->sc->strictModeState;
}

CompileError *
StrictModeGetter::queuedStrictModeError() const
{
    return parser->tc->queuedStrictModeError;
}

void
StrictModeGetter::setQueuedStrictModeError(CompileError *e)
{
    parser->tc->setQueuedStrictModeError(e);
}

bool
frontend::GenerateBlockId(TreeContext *tc, uint32_t &blockid)
{
    if (tc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(tc->sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    JS_ASSERT(tc->blockidGen < JS_BIT(20));
    blockid = tc->blockidGen++;
    return true;
}

static void
PushStatementTC(TreeContext *tc, StmtInfoTC *stmt, StmtType type)
{
    stmt->blockid = tc->blockid();
    PushStatement(tc, stmt, type);
    stmt->isFunctionBodyBlock = false;
}


bool
TreeContext::define(JSContext *cx, PropertyName *name, ParseNode *pn, Definition::Kind kind)
{
    JS_ASSERT(!pn->isUsed());
    JS_ASSERT_IF(pn->isDefn(), pn->isPlaceholder());

    Definition *prevDef = NULL;
    if (kind == Definition::LET)
        prevDef = decls_.lookupFirst(name);
    else
        JS_ASSERT(!decls_.lookupFirst(name));

    if (!prevDef)
        prevDef = lexdeps.lookupDefn(name);

    if (prevDef) {
        ParseNode **pnup = &prevDef->dn_uses;
        ParseNode *pnu;
        unsigned start = (kind == Definition::LET) ? pn->pn_blockid : bodyid;

        while ((pnu = *pnup) != NULL && pnu->pn_blockid >= start) {
            JS_ASSERT(pnu->pn_blockid >= bodyid);
            JS_ASSERT(pnu->isUsed());
            pnu->pn_lexdef = (Definition *) pn;
            pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
            pnup = &pnu->pn_link;
        }

        if (!pnu || pnu != prevDef->dn_uses) {
            *pnup = pn->dn_uses;
            pn->dn_uses = prevDef->dn_uses;
            prevDef->dn_uses = pnu;

            if (!pnu && prevDef->isPlaceholder())
                lexdeps->remove(name);
        }

        pn->pn_dflags |= prevDef->pn_dflags & PND_CLOSED;
    }

    JS_ASSERT_IF(kind != Definition::LET, !lexdeps->lookup(name));
    pn->setDefn(true);
    pn->pn_dflags &= ~PND_PLACEHOLDER;
    if (kind == Definition::CONST)
        pn->pn_dflags |= PND_CONST;

    Definition *dn = (Definition *)pn;
    switch (kind) {
      case Definition::ARG:
        JS_ASSERT(sc->inFunction());
        dn->setOp(JSOP_GETARG);
        dn->pn_dflags |= PND_BOUND;
        if (!dn->pn_cookie.set(cx, staticLevel, args_.length()))
            return false;
        if (!args_.append(dn))
            return false;
        if (name == cx->runtime->atomState.emptyAtom)
            break;
        if (!decls_.addUnique(name, dn))
            return false;
        break;

      case Definition::CONST:
      case Definition::VAR:
        if (sc->inFunction()) {
            dn->setOp(JSOP_GETLOCAL);
            dn->pn_dflags |= PND_BOUND;
            if (!dn->pn_cookie.set(cx, staticLevel, vars_.length()))
                return false;
            if (!vars_.append(dn))
                return false;
        }
        if (!decls_.addUnique(name, dn))
            return false;
        break;

      case Definition::LET:
        dn->setOp(JSOP_GETLOCAL);
        dn->pn_dflags |= (PND_LET | PND_BOUND);
        JS_ASSERT(dn->pn_cookie.level() == staticLevel); 
        if (!decls_.addShadow(name, dn))
            return false;
        break;

      case Definition::PLACEHOLDER:
      case Definition::NAMED_LAMBDA:
        JS_NOT_REACHED("unexpected kind");
        break;
    }

    return true;
}

void
TreeContext::prepareToAddDuplicateArg(Definition *prevDecl)
{
    JS_ASSERT(prevDecl->kind() == Definition::ARG);
    JS_ASSERT(decls_.lookupFirst(prevDecl->name()) == prevDecl);
    JS_ASSERT(!prevDecl->isClosed());
    decls_.remove(prevDecl->name());
}

void
TreeContext::updateDecl(JSAtom *atom, ParseNode *pn)
{
    Definition *oldDecl = decls_.lookupFirst(atom);

    pn->setDefn(true);
    Definition *newDecl = (Definition *)pn;
    decls_.updateFirst(atom, newDecl);

    if (!sc->inFunction()) {
        JS_ASSERT(newDecl->isFreeVar());
        return;
    }

    JS_ASSERT(oldDecl->isBound());
    JS_ASSERT(!oldDecl->pn_cookie.isFree());
    newDecl->pn_cookie = oldDecl->pn_cookie;
    newDecl->pn_dflags |= PND_BOUND;
    if (JOF_OPTYPE(oldDecl->getOp()) == JOF_QARG) {
        newDecl->setOp(JSOP_GETARG);
        JS_ASSERT(args_[oldDecl->pn_cookie.slot()] == oldDecl);
        args_[oldDecl->pn_cookie.slot()] = newDecl;
    } else {
        JS_ASSERT(JOF_OPTYPE(oldDecl->getOp()) == JOF_LOCAL);
        newDecl->setOp(JSOP_GETLOCAL);
        JS_ASSERT(vars_[oldDecl->pn_cookie.slot()] == oldDecl);
        vars_[oldDecl->pn_cookie.slot()] = newDecl;
    }
}

void
TreeContext::popLetDecl(JSAtom *atom)
{
    JS_ASSERT(decls_.lookupFirst(atom)->isLet());
    decls_.remove(atom);
}

static void
AppendPackedBindings(const TreeContext *tc, const DeclVector &vec, Binding *dst)
{
    for (unsigned i = 0; i < vec.length(); ++i, ++dst) {
        Definition *dn = vec[i];
        PropertyName *name = dn->name();

        BindingKind kind;
        switch (dn->kind()) {
          case Definition::VAR:
            kind = VARIABLE;
            break;
          case Definition::CONST:
            kind = CONSTANT;
            break;
          case Definition::ARG:
            kind = ARGUMENT;
            break;
          case Definition::LET:
          case Definition::NAMED_LAMBDA:
          case Definition::PLACEHOLDER:
            JS_NOT_REACHED("unexpected dn->kind");
        }

        




        JS_ASSERT_IF(dn->isClosed(), tc->decls().lookupFirst(name) == dn);
        bool aliased = dn->isClosed() ||
                       (tc->sc->bindingsAccessedDynamically() &&
                        tc->decls().lookupFirst(name) == dn);

        *dst = Binding(name, kind, aliased);
    }
}

bool
TreeContext::generateFunctionBindings(JSContext *cx, Bindings *bindings) const
{
    JS_ASSERT(sc->inFunction());

    unsigned count = args_.length() + vars_.length();
    Binding *packedBindings = cx->tempLifoAlloc().newArrayUninitialized<Binding>(count);
    if (!packedBindings) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    AppendPackedBindings(this, args_, packedBindings);
    AppendPackedBindings(this, vars_, packedBindings + args_.length());

    if (!bindings->initWithTemporaryStorage(cx, args_.length(), vars_.length(), packedBindings))
        return false;

    if (bindings->hasAnyAliasedBindings() || sc->funHasExtensibleScope())
        sc->fun()->flags |= JSFUN_HEAVYWEIGHT;

    return true;
}

Parser::Parser(JSContext *cx, const CompileOptions &options,
               const jschar *chars, size_t length, bool foldConstants)
  : AutoGCRooter(cx, PARSER),
    context(cx),
    strictModeGetter(thisForCtor()),
    tokenStream(cx, options, chars, length, &strictModeGetter),
    tempPoolMark(NULL),
    allocator(cx),
    traceListHead(NULL),
    tc(NULL),
    sct(NULL),
    keepAtoms(cx->runtime),
    foldConstants(foldConstants),
    compileAndGo(options.compileAndGo),
    selfHostingMode(options.selfHostingMode)
{
    cx->activeCompilations++;
}

bool
Parser::init()
{
    if (!context->ensureParseMapPool())
        return false;

    tempPoolMark = context->tempLifoAlloc().mark();
    return true;
}

Parser::~Parser()
{
    JSContext *cx = context;
    cx->tempLifoAlloc().release(tempPoolMark);
    cx->activeCompilations--;
}

ObjectBox::ObjectBox(ObjectBox* traceLink, JSObject *obj)
  : traceLink(traceLink),
    emitLink(NULL),
    object(obj),
    isFunctionBox(false)
{
}

ObjectBox *
Parser::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj && !IsPoisonedPtr(obj));

    







    ObjectBox *objbox = context->tempLifoAlloc().new_<ObjectBox>(traceListHead, obj);
    if (!objbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = objbox;

    return objbox;
}

FunctionBox::FunctionBox(ObjectBox* traceListHead, JSObject *obj, ParseNode *fn, TreeContext *tc,
                         StrictMode::StrictModeState sms)
  : ObjectBox(traceListHead, obj),
    node(fn),
    siblings(tc->functionList),
    kids(NULL),
    parent(tc->sc->inFunction() ? tc->sc->funbox() : NULL),
    bindings(),
    bufStart(0),
    bufEnd(0),
    level(tc->staticLevel),
    ndefaults(0),
    strictModeState(sms),
    inLoop(false),
    inWith(!!tc->innermostWith),
    inGenexpLambda(false),
    cxFlags(tc->sc->context)     
{
    isFunctionBox = true;
    for (StmtInfoTC *stmt = tc->topStmt; stmt; stmt = stmt->down) {
        if (stmt->isLoop()) {
            inLoop = true;
            break;
        }
    }
    if (!tc->sc->inFunction()) {
        JSObject *scope = tc->sc->scopeChain();
        while (scope) {
            if (scope->isWith())
                inWith = true;
            scope = scope->enclosingScope();
        }
    }
}

FunctionBox *
Parser::newFunctionBox(JSObject *obj, ParseNode *fn, TreeContext *tc,
                       StrictMode::StrictModeState sms)
{
    JS_ASSERT(obj && !IsPoisonedPtr(obj));
    JS_ASSERT(obj->isFunction());

    






    FunctionBox *funbox = context->tempLifoAlloc().new_<FunctionBox>(traceListHead, obj, fn, tc, sms);
    if (!funbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = tc->functionList = funbox;

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
}

static bool
GenerateBlockIdForStmtNode(ParseNode *pn, TreeContext *tc)
{
    JS_ASSERT(tc->topStmt);
    JS_ASSERT(tc->topStmt->maybeScope());
    JS_ASSERT(pn->isKind(PNK_STATEMENTLIST) || pn->isKind(PNK_LEXICALSCOPE));
    if (!GenerateBlockId(tc, tc->topStmt->blockid))
        return false;
    pn->pn_blockid = tc->topStmt->blockid;
    return true;
}




ParseNode *
Parser::parse(JSObject *chain)
{
    







    SharedContext globalsc(context, chain,  NULL,  NULL,
                           StrictModeFromContext(context));
    TreeContext globaltc(this, &globalsc,  0,  0);
    if (!globaltc.init())
        return NULL;

    ParseNode *pn = statements();
    if (pn) {
        if (!tokenStream.matchToken(TOK_EOF)) {
            reportError(NULL, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (foldConstants) {
            if (!FoldConstants(context, pn, this))
                pn = NULL;
        }
    }
    return pn;
}







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

static bool
ReportBadReturn(JSContext *cx, Parser *parser, ParseNode *pn, Parser::Reporter reporter,
                unsigned errnum, unsigned anonerrnum)
{
    JSAutoByteString name;
    if (parser->tc->sc->fun()->atom) {
        if (!js_AtomToPrintableString(cx, parser->tc->sc->fun()->atom, &name))
            return false;
    } else {
        errnum = anonerrnum;
    }
    return (parser->*reporter)(pn, errnum, name.ptr());
}

static bool
CheckFinalReturn(JSContext *cx, Parser *parser, ParseNode *pn)
{
    JS_ASSERT(parser->tc->sc->inFunction());
    return HasFinalReturn(pn) == ENDS_IN_RETURN ||
           ReportBadReturn(cx, parser, pn, &Parser::reportStrictWarning,
                           JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE);
}





static bool
CheckStrictAssignment(JSContext *cx, Parser *parser, ParseNode *lhs)
{
    if (parser->tc->sc->needStrictChecks() && lhs->isKind(PNK_NAME)) {
        JSAtom *atom = lhs->pn_atom;
        JSAtomState *atomState = &cx->runtime->atomState;
        if (atom == atomState->evalAtom || atom == atomState->argumentsAtom) {
            JSAutoByteString name;
            if (!js_AtomToPrintableString(cx, atom, &name) ||
                !parser->reportStrictModeError(lhs, JSMSG_DEPRECATED_ASSIGN, name.ptr()))
            {
                return false;
            }
        }
    }
    return true;
}







bool
CheckStrictBinding(JSContext *cx, Parser *parser, HandlePropertyName name, ParseNode *pn)
{
    if (!parser->tc->sc->needStrictChecks())
        return true;

    JSAtomState *atomState = &cx->runtime->atomState;
    if (name == atomState->evalAtom ||
        name == atomState->argumentsAtom ||
        FindKeyword(name->charsZ(), name->length()))
    {
        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, name, &bytes))
            return false;
        return parser->reportStrictModeError(pn, JSMSG_BAD_BINDING, bytes.ptr());
    }

    return true;
}

ParseNode *
Parser::functionBody(FunctionBodyType type)
{
    JS_ASSERT(tc->sc->inFunction());

    StmtInfoTC stmtInfo(context);
    PushStatementTC(tc, &stmtInfo, STMT_BLOCK);
    stmtInfo.isFunctionBodyBlock = true;

    JS_ASSERT(!tc->hasReturnExpr && !tc->hasReturnVoid);

    ParseNode *pn;
    if (type == StatementListBody) {
        pn = statements();
    } else {
        JS_ASSERT(type == ExpressionBody);
        JS_ASSERT(JS_HAS_EXPR_CLOSURES);

        
        
        if (!setStrictMode(false))
            return NULL;
        pn = UnaryNode::create(PNK_RETURN, this);
        if (pn) {
            pn->pn_kid = assignExpr();
            if (!pn->pn_kid) {
                pn = NULL;
            } else {
                if (tc->sc->funIsGenerator()) {
                    ReportBadReturn(context, this, pn, &Parser::reportError,
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

    if (!pn)
        return NULL;

    JS_ASSERT(!tc->topStmt->isBlockScope);
    FinishPopStatement(tc);

    
    if (context->hasStrictOption() && tc->hasReturnExpr &&
        !CheckFinalReturn(context, this, pn))
    {
        pn = NULL;
    }

    
    Rooted<PropertyName*> arguments(context, context->runtime->atomState.argumentsAtom);

    







    if (FuncStmtSet *set = tc->funcStmts) {
        for (FuncStmtSet::Range r = set->all(); !r.empty(); r.popFront()) {
            PropertyName *name = r.front()->asPropertyName();
            if (name == arguments)
                tc->sc->setBindingsAccessedDynamically();
            else if (Definition *dn = tc->decls().lookupFirst(name))
                dn->pn_dflags |= PND_CLOSED;
        }
    }

    




    for (AtomDefnRange r = tc->lexdeps->all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            Definition *dn = r.front().value();
            tc->lexdeps->remove(arguments);
            dn->pn_dflags |= PND_IMPLICITARGUMENTS;
            if (!tc->define(context, arguments, dn, Definition::VAR))
                return NULL;
            break;
        }
    }

    



    Definition *maybeArgDef = tc->decls().lookupFirst(arguments);
    bool argumentsHasBinding = !!maybeArgDef;
    bool argumentsHasLocalBinding = maybeArgDef && maybeArgDef->kind() != Definition::ARG;
    bool hasRest = tc->sc->fun()->hasRest();
    if (hasRest && argumentsHasLocalBinding) {
        reportError(NULL, JSMSG_ARGUMENTS_AND_REST);
        return NULL;
    }

    




    if (!argumentsHasBinding && tc->sc->bindingsAccessedDynamically() && !hasRest) {
        ParseNode *pn = NameNode::create(PNK_NAME, arguments, this, tc);
        if (!pn)
            return NULL;
        if (!tc->define(context, arguments, pn, Definition::VAR))
            return NULL;
        argumentsHasBinding = true;
        argumentsHasLocalBinding = true;
    }

    




    if (argumentsHasLocalBinding) {
        tc->sc->setFunArgumentsHasLocalBinding();

        
        if (tc->sc->bindingsAccessedDynamically())
            tc->sc->setFunDefinitelyNeedsArgsObj();

        






        if (tc->sc->needStrictChecks()) {
            for (AtomDefnListMap::Range r = tc->decls().all(); !r.empty(); r.popFront()) {
                DefinitionList &dlist = r.front().value();
                for (DefinitionList::Range dr = dlist.all(); !dr.empty(); dr.popFront()) {
                    Definition *dn = dr.front();
                    if (dn->kind() == Definition::ARG && dn->isAssigned()) {
                        tc->sc->setFunDefinitelyNeedsArgsObj();
                        goto exitLoop;
                    }
                }
            }
          exitLoop: ;
        }
    }

    return pn;
}





static Definition *
MakePlaceholder(ParseNode *pn, Parser *parser, TreeContext *tc)
{
    Definition *dn = (Definition *) NameNode::create(PNK_NAME, pn->pn_atom, parser, tc);
    if (!dn)
        return NULL;

    dn->setOp(JSOP_NOP);
    dn->setDefn(true);
    dn->pn_dflags |= PND_PLACEHOLDER;
    return dn;
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
MakeAssignment(ParseNode *pn, ParseNode *rhs, Parser *parser)
{
    ParseNode *lhs = parser->cloneNode(*pn);
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


static bool
MakeDefIntoUse(Definition *dn, ParseNode *pn, JSAtom *atom, Parser *parser)
{
    
    parser->tc->updateDecl(atom, pn);

    
    for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
        JS_ASSERT(pnu->isUsed());
        JS_ASSERT(!pnu->isDefn());
        pnu->pn_lexdef = (Definition *) pn;
        pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
    }
    pn->pn_dflags |= dn->pn_dflags & PND_USE2DEF_FLAGS;
    pn->dn_uses = dn;

    














    if (dn->getKind() == PNK_FUNCTION) {
        JS_ASSERT(dn->functionIsHoisted());
        pn->dn_uses = dn->pn_link;
        parser->prepareNodeForMutation(dn);
        dn->setKind(PNK_NOP);
        dn->setArity(PN_NULLARY);
        return true;
    }

    




    if (dn->canHaveInitializer()) {
        if (ParseNode *rhs = dn->expr()) {
            ParseNode *lhs = MakeAssignment(dn, rhs, parser);
            if (!lhs)
                return false;
            pn->dn_uses = lhs;
            dn->pn_link = NULL;
            dn = (Definition *) lhs;
        }
    }

    
    JS_ASSERT(dn->isKind(PNK_NAME));
    JS_ASSERT(dn->isArity(PN_NAME));
    JS_ASSERT(dn->pn_atom == atom);
    dn->setOp((js_CodeSpec[dn->getOp()].format & JOF_SET) ? JSOP_SETNAME : JSOP_NAME);
    dn->setDefn(false);
    dn->setUsed(true);
    dn->pn_lexdef = (Definition *) pn;
    dn->pn_cookie.makeFree();
    dn->pn_dflags &= ~PND_BOUND;
    return true;
}








typedef bool
(*Binder)(JSContext *cx, BindData *data, HandlePropertyName name, Parser *parser);

static bool
BindLet(JSContext *cx, BindData *data, HandlePropertyName name, Parser *parser);

static bool
BindVarOrConst(JSContext *cx, BindData *data, HandlePropertyName name, Parser *parser);

struct frontend::BindData {
    BindData(JSContext *cx) : let(cx) {}

    ParseNode       *pn;        

    JSOp            op;         
    Binder          binder;     

    struct LetData {
        LetData(JSContext *cx) : blockObj(cx) {}
        VarContext varContext;
        Rooted<StaticBlockObject*> blockObj;
        unsigned   overflow;
    } let;

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

JSFunction *
Parser::newFunction(TreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, atom != NULL);

    





    while (tc->parent)
        tc = tc->parent;

    RootedObject parent(context);
    parent = tc->sc->inFunction() ? NULL : tc->sc->scopeChain();

    RootedFunction fun(context);
    uint32_t flags = JSFUN_INTERPRETED | (kind == Expression ? JSFUN_LAMBDA : 0);
    if (selfHostingMode)
        flags |= JSFUN_SELF_HOSTED;
    fun = js_NewFunction(context, NULL, NULL, 0, flags, parent, atom);
    if (fun && !compileAndGo) {
        if (!JSObject::clearParent(context, fun))
            return NULL;
        if (!JSObject::clearType(context, fun))
            return NULL;
        fun->setEnvironment(NULL);
    }
    return fun;
}

static bool
MatchOrInsertSemicolon(JSContext *cx, TokenStream *ts)
{
    TokenKind tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return false;
    if (tt != TOK_EOF && tt != TOK_EOL && tt != TOK_SEMI && tt != TOK_RC) {
        
        ts->getToken(TSF_OPERAND);
        ts->reportError(JSMSG_SEMI_BEFORE_STMNT);
        return false;
    }
    (void) ts->matchToken(TOK_SEMI);
    return true;
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
LeaveFunction(ParseNode *fn, Parser *parser, PropertyName *funName = NULL,
              FunctionSyntaxKind kind = Expression)
{
    JSContext *cx = parser->context;
    TreeContext *funtc = parser->tc;
    TreeContext *tc = funtc->parent;
    tc->blockidGen = funtc->blockidGen;

    FunctionBox *funbox = fn->pn_funbox;
    funbox->cxFlags = funtc->sc->cxFlags;   
    funbox->kids = funtc->functionList;

    if (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
        fn->pn_dflags |= PND_BLOCKCHILD;

    
    if (funtc->lexdeps->count()) {
        for (AtomDefnRange r = funtc->lexdeps->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key();
            Definition *dn = r.front().value();
            JS_ASSERT(dn->isPlaceholder());

            if (atom == funName && kind == Expression) {
                dn->setOp(JSOP_CALLEE);
                if (!dn->pn_cookie.set(cx, funtc->staticLevel,
                                       UpvarCookie::CALLEE_SLOT))
                    return false;
                dn->pn_dflags |= PND_BOUND;
                JS_ASSERT(dn->kind() == Definition::NAMED_LAMBDA);
                if (dn->isClosed())
                    funtc->sc->fun()->flags |= JSFUN_HEAVYWEIGHT;
                continue;
            }

            Definition *outer_dn = tc->decls().lookupFirst(atom);

            




            if (funtc->sc->funHasExtensibleScope() || tc->innermostWith)
                DeoptimizeUsesWithin(dn, fn->pn_pos);

            if (!outer_dn) {
                AtomDefnAddPtr p = tc->lexdeps->lookupForAdd(atom);
                if (p) {
                    outer_dn = p.value();
                } else {
                    




















                    outer_dn = MakePlaceholder(dn, parser, tc);
                    if (!outer_dn || !tc->lexdeps->add(p, atom, outer_dn))
                        return false;
                }
            }

            












            if (dn != outer_dn) {
                if (ParseNode *pnu = dn->dn_uses) {
                    while (true) {
                        pnu->pn_lexdef = outer_dn;
                        if (!pnu->pn_link)
                            break;
                        pnu = pnu->pn_link;
                    }
                    pnu->pn_link = outer_dn->dn_uses;
                    outer_dn->dn_uses = dn->dn_uses;
                    dn->dn_uses = NULL;
                }

                outer_dn->pn_dflags |= dn->pn_dflags & ~PND_PLACEHOLDER;
            }

            
            outer_dn->pn_dflags |= PND_CLOSED;
        }
    }

    if (!funtc->generateFunctionBindings(cx, &funbox->bindings))
        return false;

    funtc->lexdeps.releaseMap(cx);
    return true;
}











bool
frontend::DefineArg(Parser *parser, ParseNode *funcpn, HandlePropertyName name, bool destructuringArg,
                    Definition **duplicatedArg)
{
    JSContext *cx = parser->context;
    TreeContext *tc = parser->tc;
    SharedContext *sc = tc->sc;

    
    if (Definition *prevDecl = tc->decls().lookupFirst(name)) {
        





        if (sc->needStrictChecks()) {
            JSAutoByteString bytes;
            if (!js_AtomToPrintableString(cx, name, &bytes))
                return false;
            if (!parser->reportStrictModeError(prevDecl, JSMSG_DUPLICATE_FORMAL, bytes.ptr()))
                return false;
        }

        





        if (destructuringArg) {
            parser->reportError(prevDecl, JSMSG_DESTRUCT_DUP_ARG);
            return false;
        }

        



        if (duplicatedArg)
            *duplicatedArg = prevDecl;

        
        tc->prepareToAddDuplicateArg(prevDecl);
    }

    ParseNode *argpn = NameNode::create(PNK_NAME, name, parser, parser->tc);
    if (!argpn)
        return false;

    if (!CheckStrictBinding(parser->context, parser, name, argpn))
        return false;

    funcpn->pn_body->append(argpn);
    return parser->tc->define(parser->context, name, argpn, Definition::ARG);
}

#if JS_HAS_DESTRUCTURING
static bool
BindDestructuringArg(JSContext *cx, BindData *data, HandlePropertyName name, Parser *parser)
{
    TreeContext *tc = parser->tc;
    JS_ASSERT(tc->sc->inFunction());

    if (tc->decls().lookupFirst(name)) {
        parser->reportError(NULL, JSMSG_DESTRUCT_DUP_ARG);
        return false;
    }

    if (!CheckStrictBinding(cx, parser, name, data->pn))
        return false;

    return tc->define(cx, name, data->pn, Definition::VAR);
}
#endif 

bool
Parser::functionArguments(ParseNode **listp, bool &hasRest)
{
    if (tokenStream.getToken() != TOK_LP) {
        reportError(NULL, JSMSG_PAREN_BEFORE_FORMAL);
        return false;
    }

    FunctionBox *funbox = tc->sc->funbox();
    funbox->bufStart = tokenStream.offsetOfToken(tokenStream.currentToken());

    hasRest = false;

    ParseNode *argsbody = ListNode::create(PNK_ARGSBODY, this);
    if (!argsbody)
        return false;
    argsbody->setOp(JSOP_NOP);
    argsbody->makeEmpty();

    ParseNode *funcpn = funbox->node;
    funcpn->pn_body = argsbody;

    if (!tokenStream.matchToken(TOK_RP)) {
        bool hasDefaults = false;
        Definition *duplicatedArg = NULL;
        bool destructuringArg = false;
#if JS_HAS_DESTRUCTURING
        ParseNode *list = NULL;
#endif

        do {
            if (hasRest) {
                reportError(NULL, JSMSG_PARAMETER_AFTER_REST);
                return false;
            }
            switch (TokenKind tt = tokenStream.getToken()) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
              {
                
                if (duplicatedArg) {
                    reportError(duplicatedArg, JSMSG_DESTRUCT_DUP_ARG);
                    return false;
                }

                if (hasDefaults) {
                    reportError(NULL, JSMSG_NONDEFAULT_FORMAL_AFTER_DEFAULT);
                    return false;
                }

                destructuringArg = true;

                





                BindData data(context);
                data.pn = NULL;
                data.op = JSOP_DEFVAR;
                data.binder = BindDestructuringArg;
                ParseNode *lhs = destructuringExpr(&data, tt);
                if (!lhs)
                    return false;

                




                PropertyName *name = context->runtime->atomState.emptyAtom;
                ParseNode *rhs = NameNode::create(PNK_NAME, name, this, this->tc);
                if (!rhs)
                    return false;

                if (!tc->define(context, name, rhs, Definition::ARG))
                    return false;

                ParseNode *item = new_<BinaryNode>(PNK_ASSIGN, JSOP_NOP, lhs->pn_pos, lhs, rhs);
                if (!item)
                    return false;
                if (!list) {
                    list = ListNode::create(PNK_VAR, this);
                    if (!list)
                        return false;
                    list->makeEmpty();
                    *listp = list;
                }
                list->append(item);
                break;
              }
#endif 

              case TOK_TRIPLEDOT:
              {
                hasRest = true;
                tt = tokenStream.getToken();
                if (tt != TOK_NAME) {
                    if (tt != TOK_ERROR)
                        reportError(NULL, JSMSG_NO_REST_NAME);
                    return false;
                }
                
              }

              case TOK_NAME:
              {
                RootedPropertyName name(context, tokenStream.currentToken().name());
                if (!DefineArg(this, funcpn, name, destructuringArg, &duplicatedArg))
                    return false;

                if (tokenStream.matchToken(TOK_ASSIGN)) {
                    if (hasRest) {
                        reportError(NULL, JSMSG_REST_WITH_DEFAULT);
                        return false;
                    }
                    hasDefaults = true;
                    ParseNode *def_expr = assignExprWithoutYield(JSMSG_YIELD_IN_DEFAULT);
                    if (!def_expr)
                        return false;
                    ParseNode *arg = funcpn->pn_body->last();
                    arg->pn_dflags |= PND_DEFAULT;
                    arg->pn_expr = def_expr;
                    funbox->ndefaults++;
                } else if (!hasRest && hasDefaults) {
                    reportError(NULL, JSMSG_NONDEFAULT_FORMAL_AFTER_DEFAULT);
                    return false;
                }

                break;
              }

              default:
                reportError(NULL, JSMSG_MISSING_FORMAL);
                
              case TOK_ERROR:
                return false;
            }
        } while (tokenStream.matchToken(TOK_COMMA));

        if (tokenStream.getToken() != TOK_RP) {
            reportError(NULL, JSMSG_PAREN_AFTER_FORMAL);
            return false;
        }
    }

    return true;
}

ParseNode *
Parser::functionDef(HandlePropertyName funName, FunctionType type, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, funName);

    
    ParseNode *pn = FunctionNode::create(PNK_FUNCTION, this);
    if (!pn)
        return NULL;
    pn->pn_body = NULL;
    pn->pn_cookie.makeFree();
    pn->pn_dflags = 0;

    
    bool bodyLevel = tc->atBodyLevel();
    if (kind == Statement) {
        



        if (Definition *dn = tc->decls().lookupFirst(funName)) {
            JS_ASSERT(!dn->isUsed());
            JS_ASSERT(dn->isDefn());

            if (context->hasStrictOption() || dn->kind() == Definition::CONST) {
                JSAutoByteString name;
                Reporter reporter = (dn->kind() != Definition::CONST)
                                    ? &Parser::reportStrictWarning
                                    : &Parser::reportError;
                if (!js_AtomToPrintableString(context, funName, &name) ||
                    !(this->*reporter)(NULL, JSMSG_REDECLARED_VAR, Definition::kindString(dn->kind()),
                                       name.ptr()))
                {
                    return NULL;
                }
            }

            







            if (bodyLevel && !MakeDefIntoUse(dn, pn, funName, this))
                return NULL;
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

            if (!tc->define(context, funName, pn, Definition::VAR))
                return NULL;
        }

        






        if (bodyLevel) {
            JS_ASSERT(pn->functionIsHoisted());
            JS_ASSERT_IF(tc->sc->inFunction(), !pn->pn_cookie.isFree());
            JS_ASSERT_IF(!tc->sc->inFunction(), pn->pn_cookie.isFree());
        } else {
            JS_ASSERT(tc->sc->strictModeState != StrictMode::STRICT);
            JS_ASSERT(pn->pn_cookie.isFree());
            tc->sc->setFunMightAliasLocals();
            tc->sc->setFunHasExtensibleScope();
            pn->setOp(JSOP_DEFFUN);

            





            if (!tc->funcStmts) {
                tc->funcStmts = context->new_<FuncStmtSet>(context);
                if (!tc->funcStmts || !tc->funcStmts->init())
                    return NULL;
            }
            if (!tc->funcStmts->put(funName))
                return NULL;
        }

        
        pn->pn_dflags |= PND_BOUND;
    } else {
        
        pn->setOp(JSOP_LAMBDA);
    }

    TreeContext *outertc = tc;

    RootedFunction fun(context, newFunction(outertc, funName, kind));
    if (!fun)
        return NULL;

    
    StrictMode::StrictModeState sms = (outertc->sc->strictModeState == StrictMode::STRICT) ?
        StrictMode::STRICT : StrictMode::UNKNOWN;

    
    FunctionBox *funbox = newFunctionBox(fun, pn, outertc, sms);
    if (!funbox)
        return NULL;

    
    SharedContext funsc(context,  NULL, fun, funbox, sms);
    TreeContext funtc(this, &funsc, outertc->staticLevel + 1, outertc->blockidGen);
    if (!funtc.init())
        return NULL;

    
    ParseNode *prelude = NULL;
    bool hasRest;
    if (!functionArguments(&prelude, hasRest))
        return NULL;

    fun->setArgCount(funtc.numArgs());
    if (funbox->ndefaults)
        fun->setHasDefaults();
    if (hasRest)
        fun->setHasRest();

    if (type == Getter && fun->nargs > 0) {
        reportError(NULL, JSMSG_ACCESSOR_WRONG_ARGS, "getter", "no", "s");
        return NULL;
    }
    if (type == Setter && fun->nargs != 1) {
        reportError(NULL, JSMSG_ACCESSOR_WRONG_ARGS, "setter", "one", "");
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

    if (funName && !CheckStrictBinding(context, this, funName, pn))
        return NULL;

#if JS_HAS_EXPR_CLOSURES
    if (bodyType == StatementListBody) {
#endif
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
        funbox->bufEnd = tokenStream.offsetOfToken(tokenStream.currentToken()) + 1;
#if JS_HAS_EXPR_CLOSURES
    } else {
        
        if (tokenStream.hadError())
            return NULL;
        funbox->bufEnd = tokenStream.endOffset(tokenStream.currentToken());
        if (kind == Statement && !MatchOrInsertSemicolon(context, &tokenStream))
            return NULL;
    }
#endif
    pn->pn_pos.end = tokenStream.currentToken().pos.end;

    





    if (funsc.bindingsAccessedDynamically())
        outertc->sc->setBindingsAccessedDynamically();

#if JS_HAS_DESTRUCTURING
    





    if (prelude) {
        if (!body->isArity(PN_LIST)) {
            ParseNode *block;

            block = ListNode::create(PNK_SEQ, this);
            if (!block)
                return NULL;
            block->pn_pos = body->pn_pos;
            block->initList(body);

            body = block;
        }

        ParseNode *item = UnaryNode::create(PNK_SEMI, this);
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

    



    if (funsc.bindingsAccessedDynamically())
        outertc->sc->setBindingsAccessedDynamically();


    pn->pn_funbox = funbox;
    pn->pn_body->append(body);
    pn->pn_body->pn_pos = body->pn_pos;
    pn->pn_blockid = outertc->blockid();

    if (!LeaveFunction(pn, this, funName, kind))
        return NULL;

    return pn;
}

ParseNode *
Parser::functionStmt()
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_FUNCTION);
    RootedPropertyName name(context);
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME) {
        name = tokenStream.currentToken().name();
    } else {
        
        reportError(NULL, JSMSG_UNNAMED_FUNCTION_STMT);
        return NULL;
    }

    
    if (!tc->atBodyLevel() && tc->sc->needStrictChecks() &&
        !reportStrictModeError(NULL, JSMSG_STRICT_FUNCTION_STATEMENT))
        return NULL;

    return functionDef(name, Normal, Statement);
}

ParseNode *
Parser::functionExpr()
{
    RootedPropertyName name(context);
    JS_ASSERT(tokenStream.currentToken().type == TOK_FUNCTION);
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME)
        name = tokenStream.currentToken().name();
    else
        tokenStream.ungetToken();
    return functionDef(name, Normal, Expression);
}

void
FunctionBox::recursivelySetStrictMode(StrictMode::StrictModeState strictness)
{
    if (strictModeState == StrictMode::UNKNOWN) {
        strictModeState = strictness;
        for (FunctionBox *kid = kids; kid; kid = kid->siblings)
            kid->recursivelySetStrictMode(strictness);
    }
}





bool
Parser::setStrictMode(bool strictMode)
{
    if (tc->sc->strictModeState != StrictMode::UNKNOWN) {
        
        JS_ASSERT(tc->sc->strictModeState == StrictMode::STRICT);
        if (tc->sc->inFunction() && tc->sc->funbox()) {
            JS_ASSERT(tc->sc->funbox()->strictModeState == tc->sc->strictModeState);
            JS_ASSERT(tc->parent->sc->strictModeState == StrictMode::STRICT);
        } else {
            JS_ASSERT(StrictModeFromContext(context) == StrictMode::STRICT || tc->staticLevel);
        }
        return true;
    }
    if (strictMode) {
        if (tc->queuedStrictModeError) {
            
            
            JS_ASSERT(!(tc->queuedStrictModeError->report.flags & JSREPORT_WARNING));
            tc->queuedStrictModeError->throwError();
            return false;
        }
        tc->sc->strictModeState = StrictMode::STRICT;
    } else if (!tc->parent || tc->parent->sc->strictModeState == StrictMode::NOTSTRICT) {
        
        tc->sc->strictModeState = StrictMode::NOTSTRICT;
        if (tc->queuedStrictModeError && context->hasStrictOption() &&
            tc->queuedStrictModeError->report.errorNumber != JSMSG_STRICT_CODE_WITH) {
            
            tc->queuedStrictModeError->report.flags |= JSREPORT_WARNING;
            tc->queuedStrictModeError->throwError();
        }
    }
    JS_ASSERT_IF(!tc->sc->inFunction(), !tc->functionList);
    if (tc->sc->strictModeState != StrictMode::UNKNOWN && tc->sc->inFunction()) {
        
        
        
        if (tc->sc->funbox())
            tc->sc->funbox()->strictModeState = tc->sc->strictModeState;
        for (FunctionBox *kid = tc->functionList; kid; kid = kid->siblings)
            kid->recursivelySetStrictMode(tc->sc->strictModeState);
    }
    return true;
}






static bool
IsEscapeFreeStringLiteral(const Token &tok)
{
    




    return (tok.pos.begin.lineno == tok.pos.end.lineno &&
            tok.pos.begin.index + tok.atom()->length() + 2 == tok.pos.end.index);
}




















bool
Parser::processDirectives(ParseNode *stmts)
{
    bool gotStrictMode = false;
    for (TokenKind tt = tokenStream.getToken(TSF_OPERAND); tt == TOK_STRING; tt = tokenStream.getToken(TSF_OPERAND)) {
        ParseNode *stringNode = atomNode(PNK_STRING, JSOP_STRING);
        if (!stringNode)
            return false;
        const Token directive = tokenStream.currentToken();
        bool isDirective = IsEscapeFreeStringLiteral(directive);
        JSAtom *atom = directive.atom();
        TokenKind next = tokenStream.peekTokenSameLine();
        if (next != TOK_EOF && next != TOK_EOL && next != TOK_SEMI && next != TOK_RC) {
            freeTree(stringNode);
            if (next == TOK_ERROR)
                return false;
            break;
        }
        tokenStream.matchToken(TOK_SEMI);
        if (isDirective) {
            
            if (atom == context->runtime->atomState.useStrictAtom && !gotStrictMode) {
                tc->sc->setExplicitUseStrict();
                if (!setStrictMode(true))
                    return false;
                gotStrictMode = true;
            }
        }
        ParseNode *stmt = UnaryNode::create(PNK_SEMI, this);
        if (!stmt) {
            freeTree(stringNode);
            return false;
        }
        stmt->pn_pos = stringNode->pn_pos;
        stmt->pn_kid = stringNode;
        stmt->pn_prologue = isDirective;
        stmts->append(stmt);
    }
    tokenStream.ungetToken();
    if (!gotStrictMode && !setStrictMode(false))
        return false;
    return true;
}






ParseNode *
Parser::statements(bool *hasFunctionStmt)
{
    JS_CHECK_RECURSION(context, return NULL);
    if (hasFunctionStmt)
        *hasFunctionStmt = false;

    ParseNode *pn = ListNode::create(PNK_STATEMENTLIST, this);
    if (!pn)
        return NULL;
    pn->makeEmpty();
    pn->pn_blockid = tc->blockid();
    ParseNode *saveBlock = tc->blockNode;
    tc->blockNode = pn;

    if (tc->atBodyLevel() && !processDirectives(pn))
        return NULL;
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

        if (next->isKind(PNK_FUNCTION)) {
            








            if (tc->atBodyLevel()) {
                pn->pn_xflags |= PNX_FUNCDEFS;
            } else {
                



                JS_ASSERT(tc->sc->funHasExtensibleScope());
                if (hasFunctionStmt)
                    *hasFunctionStmt = true;
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
        !reportStrictWarning(NULL, JSMSG_EQUAL_AS_ASSIGN))
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
ReportRedeclaration(JSContext *cx, Parser *parser, ParseNode *pn, bool isConst, JSAtom *atom)
{
    JSAutoByteString name;
    if (js_AtomToPrintableString(cx, atom, &name))
        parser->reportError(pn, JSMSG_REDECLARED_VAR, isConst ? "const" : "variable", name.ptr());
    return false;
}










static bool
BindLet(JSContext *cx, BindData *data, HandlePropertyName name, Parser *parser)
{
    TreeContext *tc = parser->tc;
    ParseNode *pn = data->pn;
    if (!CheckStrictBinding(cx, parser, name, pn))
        return false;

    Rooted<StaticBlockObject *> blockObj(cx, data->let.blockObj);
    unsigned blockCount = blockObj->slotCount();
    if (blockCount == JS_BIT(16)) {
        parser->reportError(pn, data->let.overflow);
        return false;
    }

    






    if (!pn->pn_cookie.set(parser->context, tc->staticLevel, uint16_t(blockCount)))
        return false;

    



    if (data->let.varContext == HoistVars) {
        JS_ASSERT(!tc->atBodyLevel());
        Definition *dn = tc->decls().lookupFirst(name);
        if (dn && dn->pn_blockid == tc->blockid())
            return ReportRedeclaration(cx, parser, pn, dn->isConst(), name);
        if (!tc->define(cx, name, pn, Definition::LET))
            return false;
    }

    



    bool redeclared;
    RootedId id(cx, NameToId(name));
    Shape *shape = StaticBlockObject::addVar(cx, blockObj, id, blockCount, &redeclared);
    if (!shape) {
        if (redeclared)
            ReportRedeclaration(cx, parser, pn, false, name);
        return false;
    }

    
    blockObj->setDefinitionParseNode(blockCount, reinterpret_cast<Definition *>(pn));
    return true;
}

template <class Op>
static inline bool
ForEachLetDef(JSContext *cx, TreeContext *tc, StaticBlockObject &blockObj, Op op)
{
    for (Shape::Range r = blockObj.lastProperty()->all(); !r.empty(); r.popFront()) {
        Shape &shape = r.front();

        
        if (JSID_IS_INT(shape.propid()))
            continue;

        if (!op(cx, tc, blockObj, shape, JSID_TO_ATOM(shape.propid())))
            return false;
    }
    return true;
}

struct PopLetDecl {
    bool operator()(JSContext *, TreeContext *tc, StaticBlockObject &, const Shape &, JSAtom *atom) {
        tc->popLetDecl(atom);
        return true;
    }
};

static void
PopStatementTC(JSContext *cx, TreeContext *tc)
{
    StaticBlockObject *blockObj = tc->topStmt->blockObj;
    JS_ASSERT(!!blockObj == (tc->topStmt->isBlockScope));

    FinishPopStatement(tc);

    if (blockObj) {
        JS_ASSERT(!blockObj->inDictionaryMode());
        ForEachLetDef(cx, tc, *blockObj, PopLetDecl());
        blockObj->resetPrevBlockChainFromParser();
    }
}

static inline bool
OuterLet(TreeContext *tc, StmtInfoTC *stmt, HandleAtom atom)
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
BindVarOrConst(JSContext *cx, BindData *data, HandlePropertyName name, Parser *parser)
{
    TreeContext *tc = parser->tc;
    ParseNode *pn = data->pn;
    bool isConstDecl = data->op == JSOP_DEFCONST;

    
    pn->setOp(JSOP_NAME);

    if (!CheckStrictBinding(cx, parser, name, pn))
        return false;

    StmtInfoTC *stmt = LexicalLookup(tc, name, NULL, (StmtInfoTC *)NULL);

    if (stmt && stmt->type == STMT_WITH) {
        pn->pn_dflags |= PND_DEOPTIMIZED;
        tc->sc->setFunMightAliasLocals();
        return true;
    }

    DefinitionList::Range defs = tc->decls().lookupMulti(name);
    JS_ASSERT_IF(stmt, !defs.empty());

    if (defs.empty())
        return tc->define(cx, name, pn, isConstDecl ? Definition::CONST : Definition::VAR);

    






    Definition *dn = defs.front();
    Definition::Kind dn_kind = dn->kind();
    if (dn_kind == Definition::ARG) {
        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, name, &bytes))
            return false;

        if (isConstDecl) {
            parser->reportError(pn, JSMSG_REDECLARED_PARAM, bytes.ptr());
            return false;
        }
        if (!parser->reportStrictWarning(pn, JSMSG_VAR_HIDES_ARG, bytes.ptr()))
            return false;
    } else {
        bool error = (isConstDecl ||
                      dn_kind == Definition::CONST ||
                      (dn_kind == Definition::LET &&
                       (stmt->type != STMT_CATCH || OuterLet(tc, stmt, name))));

        if (cx->hasStrictOption()
            ? data->op != JSOP_DEFVAR || dn_kind != Definition::VAR
            : error)
        {
            JSAutoByteString bytes;
            Parser::Reporter reporter =
                error ? &Parser::reportError : &Parser::reportStrictWarning;
            if (!js_AtomToPrintableString(cx, name, &bytes) ||
                !(parser->*reporter)(pn, JSMSG_REDECLARED_VAR,
                                     Definition::kindString(dn_kind), bytes.ptr()))
            {
                return false;
            }
        }
    }

    LinkUseToDef(pn, dn);
    return true;
}

static bool
MakeSetCall(JSContext *cx, ParseNode *pn, Parser *parser, unsigned msg)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isOp(JSOP_CALL) || pn->isOp(JSOP_EVAL) ||
              pn->isOp(JSOP_FUNCALL) || pn->isOp(JSOP_FUNAPPLY));
    if (!parser->reportStrictModeError(pn, msg))
        return false;

    ParseNode *pn2 = pn->pn_head;
    if (pn2->isKind(PNK_FUNCTION) && (pn2->pn_funbox->inGenexpLambda)) {
        parser->reportError(pn, msg);
        return false;
    }
    pn->pn_xflags |= PNX_SETCALL;
    return true;
}

static void
NoteLValue(JSContext *cx, ParseNode *pn, SharedContext *sc)
{
    if (pn->isUsed())
        pn->pn_lexdef->pn_dflags |= PND_ASSIGNED;

    pn->pn_dflags |= PND_ASSIGNED;

    







    if (sc->inFunction() && pn->pn_atom == sc->fun()->atom)
        sc->setBindingsAccessedDynamically();
}

static bool
NoteNameUse(ParseNode *pn, Parser *parser)
{
    RootedPropertyName name(parser->context, pn->pn_atom->asPropertyName());
    StmtInfoTC *stmt = LexicalLookup(parser->tc, name, NULL, (StmtInfoTC *)NULL);

    DefinitionList::Range defs = parser->tc->decls().lookupMulti(name);

    Definition *dn;
    if (!defs.empty()) {
        dn = defs.front();
    } else {
        if (AtomDefnAddPtr p = parser->tc->lexdeps->lookupForAdd(name)) {
            dn = p.value();
        } else {
            







            dn = MakePlaceholder(pn, parser, parser->tc);
            if (!dn || !parser->tc->lexdeps->add(p, name, dn))
                return false;
        }
    }

    JS_ASSERT(dn->isDefn());
    LinkUseToDef(pn, dn);

    if (stmt && stmt->type == STMT_WITH)
        pn->pn_dflags |= PND_DEOPTIMIZED;

    return true;
}

#if JS_HAS_DESTRUCTURING

static bool
BindDestructuringVar(JSContext *cx, BindData *data, ParseNode *pn, Parser *parser)
{
    JS_ASSERT(pn->isKind(PNK_NAME));

    RootedPropertyName name(cx, pn->pn_atom->asPropertyName());

    data->pn = pn;
    if (!data->binder(cx, data, name, parser))
        return false;

    



    if (pn->pn_dflags & PND_BOUND)
        pn->setOp(JSOP_SETLOCAL);
    else if (data->op == JSOP_DEFCONST)
        pn->setOp(JSOP_SETCONST);
    else
        pn->setOp(JSOP_SETNAME);

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    NoteLValue(cx, pn, parser->tc->sc);
    return true;
}



















static bool
BindDestructuringLHS(JSContext *cx, ParseNode *pn, Parser *parser)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        NoteLValue(cx, pn, parser->tc->sc);
        

      case PNK_DOT:
      case PNK_LB:
        




        if (!(js_CodeSpec[pn->getOp()].format & JOF_SET))
            pn->setOp(JSOP_SETNAME);
        break;

      case PNK_LP:
        if (!MakeSetCall(cx, pn, parser, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(pn->isOp(JSOP_XMLNAME));
        pn->setOp(JSOP_BINDXMLNAME);
        break;
#endif

      default:
        parser->reportError(pn, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return false;
    }

    return true;
}








































static bool
CheckDestructuring(JSContext *cx, BindData *data, ParseNode *left, Parser *parser,
                   bool toplevel = true)
{
    bool ok;

    if (left->isKind(PNK_ARRAYCOMP)) {
        parser->reportError(left, JSMSG_ARRAY_COMP_LEFTSIDE);
        return false;
    }

    Rooted<StaticBlockObject *> blockObj(cx);
    blockObj = data && data->binder == BindLet ? data->let.blockObj.get() : NULL;
    uint32_t blockCountBefore = blockObj ? blockObj->slotCount() : 0;

    if (left->isKind(PNK_RB)) {
        for (ParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (!pn->isArrayHole()) {
                if (pn->isKind(PNK_RB) || pn->isKind(PNK_RC)) {
                    ok = CheckDestructuring(cx, data, pn, parser, false);
                } else {
                    if (data) {
                        if (!pn->isKind(PNK_NAME)) {
                            parser->reportError(pn, JSMSG_NO_VARIABLE_NAME);
                            return false;
                        }
                        ok = BindDestructuringVar(cx, data, pn, parser);
                    } else {
                        ok = BindDestructuringLHS(cx, pn, parser);
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
                ok = CheckDestructuring(cx, data, pn, parser, false);
            } else if (data) {
                if (!pn->isKind(PNK_NAME)) {
                    parser->reportError(pn, JSMSG_NO_VARIABLE_NAME);
                    return false;
                }
                ok = BindDestructuringVar(cx, data, pn, parser);
            } else {
                






                if (pair->pn_right == pair->pn_left && !NoteNameUse(pn, parser))
                    return false;
                ok = BindDestructuringLHS(cx, pn, parser);
            }
            if (!ok)
                return false;
        }
    }

    






















    if (toplevel && blockObj && blockCountBefore == blockObj->slotCount()) {
        bool redeclared;
        RootedId id(cx, INT_TO_JSID(blockCountBefore));
        if (!StaticBlockObject::addVar(cx, blockObj, id, blockCountBefore, &redeclared))
            return false;
        JS_ASSERT(!redeclared);
        JS_ASSERT(blockObj->slotCount() == blockCountBefore + 1);
    }

    return true;
}

ParseNode *
Parser::destructuringExpr(BindData *data, TokenKind tt)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(tt));

    tc->inDeclDestructuring = true;
    ParseNode *pn = primaryExpr(tt, false);
    tc->inDeclDestructuring = false;
    if (!pn)
        return NULL;
    if (!CheckDestructuring(context, data, pn, this))
        return NULL;
    return pn;
}

#endif 

ParseNode *
Parser::returnOrYield(bool useAssignExpr)
{
    TokenKind tt = tokenStream.currentToken().type;
    if (!tc->sc->inFunction()) {
        reportError(NULL, JSMSG_BAD_RETURN_OR_YIELD,
                    (tt == TOK_RETURN) ? js_return_str : js_yield_str);
        return NULL;
    }

    ParseNode *pn = UnaryNode::create((tt == TOK_RETURN) ? PNK_RETURN : PNK_YIELD, this);
    if (!pn)
        return NULL;

#if JS_HAS_GENERATORS
    if (tt == TOK_YIELD) {
        



        if (tc->parenDepth == 0) {
            tc->sc->setFunIsGenerator();
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
            tc->hasReturnExpr = true;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
    } else {
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            tc->hasReturnVoid = true;
    }

    if (tc->hasReturnExpr && tc->sc->funIsGenerator()) {
        
        ReportBadReturn(context, this, pn, &Parser::reportError, JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return NULL;
    }

    if (context->hasStrictOption() && tc->hasReturnExpr && tc->hasReturnVoid &&
        !ReportBadReturn(context, this, pn, &Parser::reportStrictWarning,
                         JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE))
    {
        return NULL;
    }

    return pn;
}

static ParseNode *
PushLexicalScope(JSContext *cx, Parser *parser, StaticBlockObject &blockObj, StmtInfoTC *stmt)
{
    ParseNode *pn = LexicalScopeNode::create(PNK_LEXICALSCOPE, parser);
    if (!pn)
        return NULL;

    ObjectBox *blockbox = parser->newObjectBox(&blockObj);
    if (!blockbox)
        return NULL;

    TreeContext *tc = parser->tc;

    PushStatementTC(tc, stmt, STMT_BLOCK);
    blockObj.initPrevBlockChainFromParser(tc->blockChain);
    FinishPushBlockScope(tc, stmt, blockObj);

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
PushLexicalScope(JSContext *cx, Parser *parser, StmtInfoTC *stmt)
{
    StaticBlockObject *blockObj = StaticBlockObject::create(cx);
    if (!blockObj)
        return NULL;

    return PushLexicalScope(cx, parser, *blockObj, stmt);
}

#if JS_HAS_BLOCK_SCOPE

struct AddLetDecl
{
    uint32_t blockid;

    AddLetDecl(uint32_t blockid) : blockid(blockid) {}

    bool operator()(JSContext *cx, TreeContext *tc, StaticBlockObject &blockObj, const Shape &shape, JSAtom *)
    {
        ParseNode *def = (ParseNode *) blockObj.getSlot(shape.slot()).toPrivate();
        def->pn_blockid = blockid;
        return tc->define(cx, def->name(), def, Definition::LET);
    }
};

static ParseNode *
PushLetScope(JSContext *cx, Parser *parser, StaticBlockObject &blockObj, StmtInfoTC *stmt)
{
    ParseNode *pn = PushLexicalScope(cx, parser, blockObj, stmt);
    if (!pn)
        return NULL;

    
    pn->pn_dflags |= PND_LET;

    
    if (!ForEachLetDef(cx, parser->tc, blockObj, AddLetDecl(stmt->blockid)))
        return NULL;

    return pn;
}






ParseNode *
Parser::letBlock(LetContext letContext)
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_LET);

    ParseNode *pnlet = BinaryNode::create(PNK_LET, this);
    if (!pnlet)
        return NULL;

    Rooted<StaticBlockObject*> blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return NULL;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    ParseNode *vars = variables(PNK_LET, blockObj, DontHoistVars);
    if (!vars)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    StmtInfoTC stmtInfo(context);
    ParseNode *block = PushLetScope(context, this, *blockObj, &stmtInfo);
    if (!block)
        return NULL;

    pnlet->pn_left = vars;
    pnlet->pn_right = block;

    ParseNode *ret;
    if (letContext == LetStatement && !tokenStream.matchToken(TOK_LC, TSF_OPERAND)) {
        







        if (!reportStrictModeError(pnlet, JSMSG_STRICT_CODE_LET_EXPR_STMT))
            return NULL;

        




        ParseNode *semi = UnaryNode::create(PNK_SEMI, this);
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

    PopStatementTC(context, tc);
    return ret;
}

#endif 

static bool
PushBlocklikeStatement(StmtInfoTC *stmt, StmtType type, TreeContext *tc)
{
    PushStatementTC(tc, stmt, type);
    return GenerateBlockId(tc, stmt->blockid);
}

static ParseNode *
NewBindingNode(JSAtom *atom, Parser *parser, VarContext varContext = HoistVars)
{
    TreeContext *tc = parser->tc;

    






    if (varContext == HoistVars) {
        if (AtomDefnPtr p = tc->lexdeps->lookup(atom)) {
            ParseNode *lexdep = p.value();
            JS_ASSERT(lexdep->isPlaceholder());
            if (lexdep->pn_blockid >= tc->blockid()) {
                lexdep->pn_blockid = tc->blockid();
                tc->lexdeps->remove(p);
                return lexdep;
            }
        }
    }

    
    JS_ASSERT(parser->tokenStream.currentToken().type == TOK_NAME);
    return NameNode::create(PNK_NAME, atom, parser, parser->tc);
}

ParseNode *
Parser::switchStatement()
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_SWITCH);
    ParseNode *pn = BinaryNode::create(PNK_SWITCH, this);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

    
    ParseNode *pn1 = parenExpr();
    if (!pn1)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

    



    StmtInfoTC stmtInfo(context);
    PushStatementTC(tc, &stmtInfo, STMT_SWITCH);

    
    ParseNode *pn2 = ListNode::create(PNK_STATEMENTLIST, this);
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
                reportError(NULL, JSMSG_TOO_MANY_DEFAULTS);
                return NULL;
            }
            seenDefault = true;
            pn3 = BinaryNode::create(PNK_DEFAULT, this);
            if (!pn3)
                return NULL;
            break;

          case TOK_CASE:
          {
            pn3 = BinaryNode::create(PNK_CASE, this);
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
            reportError(NULL, JSMSG_BAD_SWITCH);
            return NULL;
        }

        pn2->append(pn3);
        if (pn2->pn_count == JS_BIT(16)) {
            reportError(NULL, JSMSG_TOO_MANY_CASES);
            return NULL;
        }

        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        ParseNode *pn4 = ListNode::create(PNK_STATEMENTLIST, this);
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
    PopStatementTC(context, tc);

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

    
    ParseNode *pn = BinaryNode::create(PNK_FOR, this);
    if (!pn)
        return NULL;

    StmtInfoTC forStmt(context);
    PushStatementTC(tc, &forStmt, STMT_FOR_LOOP);

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

    
    Rooted<StaticBlockObject*> blockObj(context);

    
    ParseNode *pn1;

    {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            if (pn->pn_iflags & JSITER_FOREACH) {
                reportError(pn, JSMSG_BAD_FOR_EACH_LOOP);
                return NULL;
            }

            pn1 = NULL;
        } else {
            












            tc->inForInit = true;
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
            tc->inForInit = false;
            if (!pn1)
                return NULL;
        }
    }

    JS_ASSERT_IF(forDecl, pn1->isArity(PN_LIST));
    JS_ASSERT(!!blockObj == (forDecl && pn1->isOp(JSOP_NOP)));

    const TokenPos pos = tokenStream.currentToken().pos;

    
    ParseNode *forParent = NULL;

    





    ParseNode *forHead;        
    StmtInfoTC letStmt(context); 
    ParseNode *pn2, *pn3;      
    bool forOf;
    if (pn1 && matchInOrOf(&forOf)) {
        







        forStmt.type = STMT_FOR_IN_LOOP;

        
        if (forOf && pn->pn_iflags != 0) {
            JS_ASSERT(pn->pn_iflags == JSITER_FOREACH);
            reportError(NULL, JSMSG_BAD_FOR_EACH_LOOP);
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
            reportError(pn1, JSMSG_BAD_FOR_LEFTSIDE);
            return NULL;
        }

        





        pn2 = NULL;
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
                    reportError(pn2, JSMSG_INVALID_FOR_IN_INIT);
                    return NULL;
                }
#endif 

                ParseNode *pnseq = ListNode::create(PNK_SEQ, this);
                if (!pnseq)
                    return NULL;

                







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
            





            ParseNode *block = PushLetScope(context, this, *blockObj, &letStmt);
            if (!block)
                return NULL;
            letStmt.isForLetBlock = true;
            block->pn_expr = pn1;
            pn1 = block;
        }

        if (forDecl) {
            



            pn2 = CloneLeftHandSide(pn2, this);
            if (!pn2)
                return NULL;
        }

        switch (pn2->getKind()) {
          case PNK_NAME:
            
            NoteLValue(context, pn2, tc->sc);
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

        forHead = TernaryNode::create(PNK_FORIN, this);
        if (!forHead)
            return NULL;
    } else {
        if (blockObj) {
            



            ParseNode *block = PushLetScope(context, this, *blockObj, &letStmt);
            if (!block)
                return NULL;
            letStmt.isForLetBlock = true;

            ParseNode *let = new_<BinaryNode>(PNK_LET, JSOP_NOP, pos, pn1, block);
            if (!let)
                return NULL;

            pn1 = NULL;
            block->pn_expr = pn;
            forParent = let;
        }

        if (pn->pn_iflags & JSITER_FOREACH) {
            reportError(pn, JSMSG_BAD_FOR_EACH_LOOP);
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

        forHead = TernaryNode::create(PNK_FORHEAD, this);
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
        PopStatementTC(context, tc);
#endif
    PopStatementTC(context, tc);
    return forParent ? forParent : pn;
}

ParseNode *
Parser::tryStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_TRY));

    
















    ParseNode *pn = TernaryNode::create(PNK_TRY, this);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_NOP);

    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    StmtInfoTC stmtInfo(context);
    if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, tc))
        return NULL;
    pn->pn_kid1 = statements();
    if (!pn->pn_kid1)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatementTC(context, tc);

    ParseNode *lastCatch;
    ParseNode *catchList = NULL;
    TokenKind tt = tokenStream.getToken();
    if (tt == TOK_CATCH) {
        catchList = ListNode::create(PNK_CATCHLIST, this);
        if (!catchList)
            return NULL;
        catchList->makeEmpty();
        lastCatch = NULL;

        do {
            ParseNode *pnblock;
            BindData data(context);

            
            if (lastCatch && !lastCatch->pn_kid2) {
                reportError(NULL, JSMSG_CATCH_AFTER_GENERAL);
                return NULL;
            }

            



            pnblock = PushLexicalScope(context, this, &stmtInfo);
            if (!pnblock)
                return NULL;
            stmtInfo.type = STMT_CATCH;

            






            ParseNode *pn2 = TernaryNode::create(PNK_CATCH, this);
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
                RootedPropertyName label(context, tokenStream.currentToken().name());
                pn3 = NewBindingNode(label, this);
                if (!pn3)
                    return NULL;
                data.pn = pn3;
                if (!data.binder(context, &data, label, this))
                    return NULL;
                break;
              }

              default:
                reportError(NULL, JSMSG_CATCH_IDENTIFIER);
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
            PopStatementTC(context, tc);

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
        PopStatementTC(context, tc);
    } else {
        tokenStream.ungetToken();
    }
    if (!catchList && !pn->pn_kid3) {
        reportError(NULL, JSMSG_CATCH_OR_FINALLY);
        return NULL;
    }
    return pn;
}

ParseNode *
Parser::withStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_WITH));

    
    
    
    
    
    
    
    
    if (!reportStrictModeError(NULL, JSMSG_STRICT_CODE_WITH))
        return NULL;

    ParseNode *pn = BinaryNode::create(PNK_WITH, this);
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

    StmtInfoTC stmtInfo(context);
    PushStatementTC(tc, &stmtInfo, STMT_WITH);
    pn2 = statement();
    if (!pn2)
        return NULL;
    PopStatementTC(context, tc);

    pn->pn_pos.end = pn2->pn_pos.end;
    pn->pn_right = pn2;

    tc->sc->setBindingsAccessedDynamically();
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

        










        StmtInfoTC *stmt = tc->topStmt;
        if (stmt && (!stmt->maybeScope() || stmt->isForLetBlock)) {
            reportError(NULL, JSMSG_LET_DECL_NOT_IN_BLOCK);
            return NULL;
        }

        if (stmt && stmt->isBlockScope) {
            JS_ASSERT(tc->blockChain == stmt->blockObj);
        } else {
            if (!stmt || stmt->isFunctionBodyBlock) {
                



                pn = variables(PNK_VAR);
                if (!pn)
                    return NULL;
                pn->pn_xflags |= PNX_POPVAR;
                break;
            }

            




            JS_ASSERT(!stmt->isBlockScope);
            JS_ASSERT(stmt != tc->topScopeStmt);
            JS_ASSERT(stmt->type == STMT_BLOCK ||
                      stmt->type == STMT_SWITCH ||
                      stmt->type == STMT_TRY ||
                      stmt->type == STMT_FINALLY);
            JS_ASSERT(!stmt->downScope);

            
            StaticBlockObject *blockObj = StaticBlockObject::create(context);
            if (!blockObj)
                return NULL;

            ObjectBox *blockbox = newObjectBox(blockObj);
            if (!blockbox)
                return NULL;

            





            stmt->isBlockScope = true;
            stmt->downScope = tc->topScopeStmt;
            tc->topScopeStmt = stmt;

            blockObj->initPrevBlockChainFromParser(tc->blockChain);
            tc->blockChain = blockObj;
            stmt->blockObj = blockObj;

#ifdef DEBUG
            ParseNode *tmp = tc->blockNode;
            JS_ASSERT(!tmp || !tmp->isKind(PNK_LEXICALSCOPE));
#endif

            
            ParseNode *pn1 = LexicalScopeNode::create(PNK_LEXICALSCOPE, this);
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
            reportError(NULL, JSMSG_BAD_LABEL);
            return NULL;
        }
        JSAtom *label = pn2->pn_atom;
        for (StmtInfoTC *stmt = tc->topStmt; stmt; stmt = stmt->down) {
            if (stmt->type == STMT_LABEL && stmt->label == label) {
                reportError(NULL, JSMSG_DUPLICATE_LABEL);
                return NULL;
            }
        }
        ForgetUse(pn2);

        (void) tokenStream.getToken();

        
        StmtInfoTC stmtInfo(context);
        PushStatementTC(tc, &stmtInfo, STMT_LABEL);
        stmtInfo.label = label;
        ParseNode *pn = statement();
        if (!pn)
            return NULL;

        
        if (pn->isKind(PNK_SEMI) && !pn->pn_kid) {
            pn->setKind(PNK_STATEMENTLIST);
            pn->setArity(PN_LIST);
            pn->makeEmpty();
        }

        
        PopStatementTC(context, tc);
        pn2->setKind(PNK_COLON);
        pn2->pn_pos.end = pn->pn_pos.end;
        pn2->pn_expr = pn;
        return pn2;
    }

    ParseNode *pn = UnaryNode::create(PNK_SEMI, this);
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
        if (allowsXML()) {
            TokenKind tt = tokenStream.peekToken(TSF_KEYWORD_IS_NAME);
            if (tt == TOK_DBLCOLON)
                return expressionStatement();
        }
#endif
        return functionStmt();
      }

      case TOK_IF:
      {
        
        pn = TernaryNode::create(PNK_IF, this);
        if (!pn)
            return NULL;
        ParseNode *pn1 = condition();
        if (!pn1)
            return NULL;

        StmtInfoTC stmtInfo(context);
        PushStatementTC(tc, &stmtInfo, STMT_IF);
        ParseNode *pn2 = statement();
        if (!pn2)
            return NULL;

        if (pn2->isKind(PNK_SEMI) &&
            !pn2->pn_kid &&
            !reportStrictWarning(NULL, JSMSG_EMPTY_CONSEQUENT))
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
        PopStatementTC(context, tc);
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
        return pn;
      }

      case TOK_SWITCH:
        return switchStatement();

      case TOK_WHILE:
      {
        pn = BinaryNode::create(PNK_WHILE, this);
        if (!pn)
            return NULL;
        StmtInfoTC stmtInfo(context);
        PushStatementTC(tc, &stmtInfo, STMT_WHILE_LOOP);
        ParseNode *pn2 = condition();
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        ParseNode *pn3 = statement();
        if (!pn3)
            return NULL;
        PopStatementTC(context, tc);
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_right = pn3;
        return pn;
      }

      case TOK_DO:
      {
        pn = BinaryNode::create(PNK_DOWHILE, this);
        if (!pn)
            return NULL;
        StmtInfoTC stmtInfo(context);
        PushStatementTC(tc, &stmtInfo, STMT_DO_LOOP);
        ParseNode *pn2 = statement();
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        MUST_MATCH_TOKEN(TOK_WHILE, JSMSG_WHILE_AFTER_DO);
        ParseNode *pn3 = condition();
        if (!pn3)
            return NULL;
        PopStatementTC(context, tc);
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
        pn = UnaryNode::create(PNK_THROW, this);
        if (!pn)
            return NULL;

        
        TokenKind tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
        if (tt == TOK_ERROR)
            return NULL;
        if (tt == TOK_EOF || tt == TOK_EOL || tt == TOK_SEMI || tt == TOK_RC) {
            reportError(NULL, JSMSG_SYNTAX_ERROR);
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
        reportError(NULL, JSMSG_CATCH_WITHOUT_TRY);
        return NULL;

      case TOK_FINALLY:
        reportError(NULL, JSMSG_FINALLY_WITHOUT_TRY);
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
        StmtInfoTC *stmt = tc->topStmt;
        if (label) {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportError(NULL, JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL && stmt->label == label)
                    break;
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportError(NULL, JSMSG_TOUGH_BREAK);
                    return NULL;
                }
                if (stmt->isLoop() || stmt->type == STMT_SWITCH)
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
        StmtInfoTC *stmt = tc->topStmt;
        if (label) {
            for (StmtInfoTC *stmt2 = NULL; ; stmt = stmt->down) {
                if (!stmt) {
                    reportError(NULL, JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL) {
                    if (stmt->label == label) {
                        if (!stmt2 || !stmt2->isLoop()) {
                            reportError(NULL, JSMSG_BAD_CONTINUE);
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
                    reportError(NULL, JSMSG_BAD_CONTINUE);
                    return NULL;
                }
                if (stmt->isLoop())
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
        StmtInfoTC stmtInfo(context);
        if (!PushBlocklikeStatement(&stmtInfo, STMT_BLOCK, tc))
            return NULL;
        bool hasFunctionStmt;
        pn = statements(&hasFunctionStmt);
        if (!pn)
            return NULL;

        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_COMPOUND);
        PopStatementTC(context, tc);

        



        if (hasFunctionStmt && (!tc->topStmt || tc->topStmt->type == STMT_BLOCK))
            pn->pn_xflags |= PNX_NEEDBRACES;

        return pn;
      }

      case TOK_SEMI:
        pn = UnaryNode::create(PNK_SEMI, this);
        if (!pn)
            return NULL;
        return pn;

      case TOK_DEBUGGER:
        pn = new_<DebuggerStatement>(tokenStream.currentToken().pos);
        if (!pn)
            return NULL;
        tc->sc->setBindingsAccessedDynamically();
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_DEFAULT:
      {
        if (!allowsXML())
            return expressionStatement();

        pn = UnaryNode::create(PNK_DEFXMLNS, this);
        if (!pn)
            return NULL;
        if (!tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().name() != context->runtime->atomState.xmlAtom ||
            !tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().name() != context->runtime->atomState.namespaceAtom ||
            !tokenStream.matchToken(TOK_ASSIGN))
        {
            reportError(NULL, JSMSG_BAD_DEFAULT_XML_NAMESPACE);
            return NULL;
        }

        JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

        
        tc->sc->setBindingsAccessedDynamically();
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

    ParseNode *pn = ListNode::create(kind, this);
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
            tc->inDeclDestructuring = true;
            pn2 = primaryExpr(tt, false);
            tc->inDeclDestructuring = false;
            if (!pn2)
                return NULL;

            if (!CheckDestructuring(context, &data, pn2, this))
                return NULL;
            bool ignored;
            if (tc->inForInit && matchInOrOf(&ignored)) {
                tokenStream.ungetToken();
                pn->append(pn2);
                continue;
            }

            MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

            ParseNode *init = assignExpr();
            if (!init)
                return NULL;

            pn2 = ParseNode::newBinaryOrAppend(PNK_ASSIGN, JSOP_NOP, pn2, init, this);
            if (!pn2)
                return NULL;
            pn->append(pn2);
            continue;
        }
#endif 

        if (tt != TOK_NAME) {
            if (tt != TOK_ERROR)
                reportError(NULL, JSMSG_NO_VARIABLE_NAME);
            return NULL;
        }

        RootedPropertyName name(context, tokenStream.currentToken().name());
        pn2 = NewBindingNode(name, this, varContext);
        if (!pn2)
            return NULL;
        if (data.op == JSOP_DEFCONST)
            pn2->pn_dflags |= PND_CONST;
        data.pn = pn2;
        if (!data.binder(context, &data, name, this))
            return NULL;
        pn->append(pn2);

        if (tokenStream.matchToken(TOK_ASSIGN)) {
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

            ParseNode *init = assignExpr();
            if (!init)
                return NULL;

            if (pn2->isUsed()) {
                pn2 = MakeAssignment(pn2, init, this);
                if (!pn2)
                    return NULL;
            } else {
                pn2->pn_expr = init;
            }

            pn2->setOp((pn2->pn_dflags & PND_BOUND)
                       ? JSOP_SETLOCAL
                       : (data.op == JSOP_DEFCONST)
                       ? JSOP_SETCONST
                       : JSOP_SETNAME);

            NoteLValue(context, pn2, tc->sc);

            
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
        ParseNode *pn2 = ListNode::create(PNK_COMMA, this);
        if (!pn2)
            return NULL;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->initList(pn);
        pn = pn2;
        do {
#if JS_HAS_GENERATORS
            pn2 = pn->last();
            if (pn2->isKind(PNK_YIELD) && !pn2->isInParens()) {
                reportError(pn2, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
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
        pn = ParseNode::newBinaryOrAppend(kind, op, pn, unaryExpr(), this);
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
        pn = ParseNode::newBinaryOrAppend(kind, op, pn, mulExpr1n(), this);
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
        left = new_<BinaryNode>(kind, op, left, right);
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
    



    bool oldInForInit = tc->inForInit;
    tc->inForInit = false;

    ParseNode *pn = shiftExpr1i();
    while (pn &&
           (tokenStream.isCurrentTokenRelational() ||
            



            (oldInForInit == 0 && tokenStream.isCurrentTokenType(TOK_IN)) ||
            tokenStream.isCurrentTokenType(TOK_INSTANCEOF))) {
        ParseNodeKind kind = RelationalTokenToParseNodeKind(tokenStream.currentToken());
        JSOp op = tokenStream.currentToken().t_op;
        pn = ParseNode::newBinaryOrAppend(kind, op, pn, shiftExpr1n(), this);
    }
    
    tc->inForInit |= oldInForInit;

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
        left = new_<BinaryNode>(kind, op, left, right);
    }
    return left;
}
END_EXPR_PARSER(eqExpr1)

BEGIN_EXPR_PARSER(bitAndExpr1)
{
    ParseNode *pn = eqExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITAND))
        pn = ParseNode::newBinaryOrAppend(PNK_BITAND, JSOP_BITAND, pn, eqExpr1n(), this);
    return pn;
}
END_EXPR_PARSER(bitAndExpr1)

BEGIN_EXPR_PARSER(bitXorExpr1)
{
    ParseNode *pn = bitAndExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITXOR))
        pn = ParseNode::newBinaryOrAppend(PNK_BITXOR, JSOP_BITXOR, pn, bitAndExpr1n(), this);
    return pn;
}
END_EXPR_PARSER(bitXorExpr1)

BEGIN_EXPR_PARSER(bitOrExpr1)
{
    ParseNode *pn = bitXorExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITOR))
        pn = ParseNode::newBinaryOrAppend(PNK_BITOR, JSOP_BITOR, pn, bitXorExpr1n(), this);
    return pn;
}
END_EXPR_PARSER(bitOrExpr1)

BEGIN_EXPR_PARSER(andExpr1)
{
    ParseNode *pn = bitOrExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_AND))
        pn = ParseNode::newBinaryOrAppend(PNK_AND, JSOP_AND, pn, bitOrExpr1n(), this);
    return pn;
}
END_EXPR_PARSER(andExpr1)

JS_ALWAYS_INLINE ParseNode *
Parser::orExpr1()
{
    ParseNode *pn = andExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_OR))
        pn = ParseNode::newBinaryOrAppend(PNK_OR, JSOP_OR, pn, andExpr1n(), this);
    return pn;
}

JS_ALWAYS_INLINE ParseNode *
Parser::condExpr1()
{
    ParseNode *condition = orExpr1();
    if (!condition || !tokenStream.isCurrentTokenType(TOK_HOOK))
        return condition;

    




    bool oldInForInit = tc->inForInit;
    tc->inForInit = false;
    ParseNode *thenExpr = assignExpr();
    tc->inForInit = oldInForInit;
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
        if (!CheckStrictAssignment(context, this, pn))
            return false;
        pn->setOp(pn->isOp(JSOP_GETLOCAL) ? JSOP_SETLOCAL : JSOP_SETNAME);
        NoteLValue(context, pn, tc->sc);
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
            reportError(NULL, JSMSG_BAD_DESTRUCT_ASS);
            return false;
        }
        if (!CheckDestructuring(context, NULL, pn, this))
            return false;
        break;
#endif
      case PNK_LP:
        if (!MakeSetCall(context, pn, this, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(pn->isOp(JSOP_XMLNAME));
        pn->setOp(JSOP_SETXMLNAME);
        break;
#endif
      default:
        reportError(NULL, JSMSG_BAD_LEFTSIDE_OF_ASS);
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

    return ParseNode::newBinaryOrAppend(kind, op, lhs, rhs, this);
}

static bool
SetLvalKid(JSContext *cx, Parser *parser, ParseNode *pn, ParseNode *kid,
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
        parser->reportError(NULL, JSMSG_BAD_OPERAND, name);
        return false;
    }
    if (!CheckStrictAssignment(cx, parser, kid))
        return false;
    pn->pn_kid = kid;
    return true;
}

static const char incop_name_str[][10] = {"increment", "decrement"};

static bool
SetIncOpKid(JSContext *cx, Parser *parser, ParseNode *pn, ParseNode *kid,
            TokenKind tt, bool preorder)
{
    JSOp op;

    if (!SetLvalKid(cx, parser, pn, kid, incop_name_str[tt == TOK_DEC]))
        return false;
    switch (kid->getKind()) {
      case PNK_NAME:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCNAME : JSOP_NAMEINC)
             : (preorder ? JSOP_DECNAME : JSOP_NAMEDEC);
        NoteLValue(cx, kid, parser->tc->sc);
        break;

      case PNK_DOT:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCPROP : JSOP_PROPINC)
             : (preorder ? JSOP_DECPROP : JSOP_PROPDEC);
        break;

      case PNK_LP:
        if (!MakeSetCall(cx, kid, parser, JSMSG_BAD_INCOP_OPERAND))
            return false;
        
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
    return true;
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
        pn = UnaryNode::create((tt == TOK_INC) ? PNK_PREINCREMENT : PNK_PREDECREMENT, this);
        if (!pn)
            return NULL;
        pn2 = memberExpr(true);
        if (!pn2)
            return NULL;
        if (!SetIncOpKid(context, this, pn, pn2, tt, true))
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        break;

      case TOK_DELETE:
      {
        pn = UnaryNode::create(PNK_DELETE, this);
        if (!pn)
            return NULL;
        pn2 = unaryExpr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;

        




        if (foldConstants && !FoldConstants(context, pn2, this))
            return NULL;
        switch (pn2->getKind()) {
          case PNK_LP:
            if (!(pn2->pn_xflags & PNX_SETCALL)) {
                



                if (!MakeSetCall(context, pn2, this, JSMSG_BAD_DELETE_OPERAND))
                    return NULL;
                pn2->pn_xflags &= ~PNX_SETCALL;
            }
            break;
          case PNK_NAME:
            if (!reportStrictModeError(pn, JSMSG_DEPRECATED_DELETE_OPERAND))
                return NULL;
            tc->sc->setBindingsAccessedDynamically();
            pn2->pn_dflags |= PND_DEOPTIMIZED;
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
        pn = memberExpr(true);
        if (!pn)
            return NULL;

        
        if (tokenStream.onCurrentLine(pn->pn_pos)) {
            tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
            if (tt == TOK_INC || tt == TOK_DEC) {
                tokenStream.consumeKnownToken(tt);
                pn2 = UnaryNode::create((tt == TOK_INC) ? PNK_POSTINCREMENT : PNK_POSTDECREMENT, this);
                if (!pn2)
                    return NULL;
                if (!SetIncOpKid(context, this, pn2, pn, tt, false))
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
    Parser          *parser;
    bool            genexp;
    unsigned        adjust;
    unsigned        funcLevel;
    HashSet<Definition *> visitedImplicitArguments;

  public:
    CompExprTransplanter(ParseNode *pn, Parser *parser, bool ge, unsigned adj)
      : root(pn), parser(parser), genexp(ge), adjust(adj), funcLevel(0),
        visitedImplicitArguments(parser->context)
    {}

    bool init() {
        return visitedImplicitArguments.init();
    }

    bool transplant(ParseNode *pn);
};


















class GenexpGuard {
    Parser          *parser;
    uint32_t        startYieldCount;

  public:
    explicit GenexpGuard(Parser *parser)
      : parser(parser)
    {
        TreeContext *tc = parser->tc;
        if (tc->parenDepth == 0) {
            tc->yieldCount = 0;
            tc->yieldNode = NULL;
        }
        startYieldCount = tc->yieldCount;
        tc->parenDepth++;
    }

    void endBody();
    bool checkValidBody(ParseNode *pn, unsigned err);
    bool maybeNoteGenerator(ParseNode *pn);
};

void
GenexpGuard::endBody()
{
    parser->tc->parenDepth--;
}








bool
GenexpGuard::checkValidBody(ParseNode *pn, unsigned err = JSMSG_BAD_GENEXP_BODY)
{
    TreeContext *tc = parser->tc;
    if (tc->yieldCount > startYieldCount) {
        ParseNode *errorNode = tc->yieldNode;
        if (!errorNode)
            errorNode = pn;
        parser->reportError(errorNode, err, js_yield_str);
        return false;
    }

    return true;
}








bool
GenexpGuard::maybeNoteGenerator(ParseNode *pn)
{
    TreeContext *tc = parser->tc;
    if (tc->yieldCount > 0) {
        tc->sc->setFunIsGenerator();
        if (!tc->sc->inFunction()) {
            parser->reportError(NULL, JSMSG_BAD_RETURN_OR_YIELD, js_yield_str);
            return false;
        }
        if (tc->hasReturnExpr) {
            
            ReportBadReturn(tc->sc->context, parser, pn, &Parser::reportError,
                            JSMSG_BAD_GENERATOR_RETURN, JSMSG_BAD_ANON_GENERATOR_RETURN);
            return false;
        }
    }
    return true;
}






static bool
BumpStaticLevel(ParseNode *pn, TreeContext *tc)
{
    if (pn->pn_cookie.isFree())
        return true;

    unsigned level = unsigned(pn->pn_cookie.level()) + 1;
    JS_ASSERT(level >= tc->staticLevel);
    return pn->pn_cookie.set(tc->sc->context, level, pn->pn_cookie.slot());
}

static bool
AdjustBlockId(ParseNode *pn, unsigned adjust, TreeContext *tc)
{
    JS_ASSERT(pn->isArity(PN_LIST) || pn->isArity(PN_FUNC) || pn->isArity(PN_NAME));
    if (JS_BIT(20) - pn->pn_blockid <= adjust + 1) {
        JS_ReportErrorNumber(tc->sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    pn->pn_blockid += adjust;
    if (pn->pn_blockid >= tc->blockidGen)
        tc->blockidGen = pn->pn_blockid + 1;
    return true;
}

bool
CompExprTransplanter::transplant(ParseNode *pn)
{
    TreeContext *tc = parser->tc;

    if (!pn)
        return true;

    switch (pn->getArity()) {
      case PN_LIST:
        for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!transplant(pn2))
                return false;
        }
        if (pn->pn_pos >= root->pn_pos) {
            if (!AdjustBlockId(pn, adjust, tc))
                return false;
        }
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
            FunctionBox *parent = tc->sc->funbox();

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
            JS_ASSERT(pn->pn_cookie.isFree());

            Definition *dn = pn->pn_lexdef;
            JS_ASSERT(dn->isDefn());

            








            if (dn->isPlaceholder() && dn->pn_pos >= root->pn_pos && dn->dn_uses == pn) {
                if (genexp && !BumpStaticLevel(dn, tc))
                    return false;
                if (!AdjustBlockId(dn, adjust, tc))
                    return false;
            }

            RootedAtom atom(parser->context, pn->pn_atom);
#ifdef DEBUG
            StmtInfoTC *stmt = LexicalLookup(tc, atom, NULL, (StmtInfoTC *)NULL);
            JS_ASSERT(!stmt || stmt != tc->topStmt);
#endif
            if (genexp && !dn->isOp(JSOP_CALLEE)) {
                JS_ASSERT(!tc->decls().lookupFirst(atom));

                if (dn->pn_pos < root->pn_pos) {
                    







                    Definition *dn2 = MakePlaceholder(pn, parser, parser->tc);
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
                    if (dn->isClosed())
                        dn2->pn_dflags |= PND_CLOSED;
                } else if (dn->isPlaceholder()) {
                    




                    tc->parent->lexdeps->remove(atom);
                    if (!tc->lexdeps->put(atom, dn))
                        return false;
                } else if (dn->isImplicitArguments()) {
                    






                    if (genexp && !visitedImplicitArguments.has(dn)) {
                        if (!BumpStaticLevel(dn, tc))
                            return false;
                        if (!AdjustBlockId(dn, adjust, tc))
                            return false;
                        if (!visitedImplicitArguments.put(dn))
                            return false;
                    }
                }
            }
        }

        if (pn->pn_pos >= root->pn_pos) {
            if (!AdjustBlockId(pn, adjust, tc))
                return false;
        }
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
    StmtInfoTC stmtInfo(context);
    BindData data(context);
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_FOR);

    if (kind == PNK_SEMI) {
        




        pn = PushLexicalScope(context, this, &stmtInfo);
        if (!pn)
            return NULL;
        adjust = pn->pn_blockid - blockid;
    } else {
        JS_ASSERT(kind == PNK_ARRAYPUSH);

        











        adjust = tc->blockid();
        pn = PushLexicalScope(context, this, &stmtInfo);
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

    CompExprTransplanter transplanter(kid, this, kind == PNK_SEMI, adjust);
    if (!transplanter.init())
        return NULL;

    if (!transplanter.transplant(kid))
        return NULL;

    JS_ASSERT(tc->blockChain && tc->blockChain == pn->pn_objbox->object);
    data.initLet(HoistVars, *tc->blockChain, JSMSG_ARRAY_INIT_TOO_BIG);

    do {
        




        pn2 = BinaryNode::create(PNK_FOR, this);
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

        GenexpGuard guard(this);

        RootedPropertyName name(context);
        tt = tokenStream.getToken();
        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            tc->inDeclDestructuring = true;
            pn3 = primaryExpr(tt, false);
            tc->inDeclDestructuring = false;
            if (!pn3)
                return NULL;
            break;
#endif

          case TOK_NAME:
            name = tokenStream.currentToken().name();

            






            pn3 = NewBindingNode(name, this);
            if (!pn3)
                return NULL;
            break;

          default:
            reportError(NULL, JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return NULL;
        }

        bool forOf;
        if (!matchInOrOf(&forOf)) {
            reportError(NULL, JSMSG_IN_AFTER_FOR_NAME);
            return NULL;
        }
        if (forOf) {
            if (pn2->pn_iflags != JSITER_ENUMERATE) {
                JS_ASSERT(pn2->pn_iflags == (JSITER_FOREACH | JSITER_ENUMERATE));
                reportError(NULL, JSMSG_BAD_FOR_EACH_LOOP);
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
            if (!CheckDestructuring(context, &data, pn3, this))
                return NULL;

            if (versionNumber() == JSVERSION_1_7) {
                
                if (!pn3->isKind(PNK_RB) || pn3->pn_count != 2) {
                    reportError(NULL, JSMSG_BAD_FOR_LEFTSIDE);
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
            if (!data.binder(context, &data, name, this))
                return NULL;
            break;

          default:;
        }

        



        ParseNode *vars = ListNode::create(PNK_VAR, this);
        if (!vars)
            return NULL;
        vars->setOp(JSOP_NOP);
        vars->pn_pos = pn3->pn_pos;
        vars->makeEmpty();
        vars->append(pn3);
        vars->pn_xflags |= PNX_FORINVAR;

        
        pn3 = CloneLeftHandSide(pn3, this);
        if (!pn3)
            return NULL;

        pn2->pn_left = new_<TernaryNode>(PNK_FORIN, JSOP_NOP, vars, pn3, pn4);
        if (!pn2->pn_left)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_right;
    } while (tokenStream.matchToken(TOK_FOR));

    if (tokenStream.matchToken(TOK_IF)) {
        pn2 = TernaryNode::create(PNK_IF, this);
        if (!pn2)
            return NULL;
        pn2->pn_kid1 = condition();
        if (!pn2->pn_kid1)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_kid2;
    }

    pn2 = UnaryNode::create(kind, this);
    if (!pn2)
        return NULL;
    pn2->setOp(op);
    pn2->pn_kid = kid;
    *pnp = pn2;

    PopStatementTC(context, tc);
    return pn;
}

#if JS_HAS_GENERATOR_EXPRS
















ParseNode *
Parser::generatorExpr(ParseNode *kid)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    
    ParseNode *pn = UnaryNode::create(PNK_YIELD, this);
    if (!pn)
        return NULL;
    pn->setOp(JSOP_YIELD);
    pn->setInParens(true);
    pn->pn_pos = kid->pn_pos;
    pn->pn_kid = kid;
    pn->pn_hidden = true;

    
    ParseNode *genfn = FunctionNode::create(PNK_FUNCTION, this);
    if (!genfn)
        return NULL;
    genfn->setOp(JSOP_LAMBDA);
    JS_ASSERT(!genfn->pn_body);
    genfn->pn_dflags = 0;

    {
        TreeContext *outertc = tc;

        RootedFunction fun(context, newFunction(outertc,  NULL, Expression));
        if (!fun)
            return NULL;

        
        FunctionBox *funbox = newFunctionBox(fun, genfn, outertc, outertc->sc->strictModeState);
        if (!funbox)
            return NULL;

        SharedContext gensc(context,  NULL, fun, funbox, outertc->sc->strictModeState);
        TreeContext gentc(this, &gensc, outertc->staticLevel + 1, outertc->blockidGen);
        if (!gentc.init())
            return NULL;

        





        gensc.cxFlags = outertc->sc->cxFlags;
        gensc.setFunIsGenerator();

        funbox->inGenexpLambda = true;
        genfn->pn_funbox = funbox;
        genfn->pn_blockid = gentc.bodyid;

        ParseNode *body = comprehensionTail(pn, outertc->blockid(), true);
        if (!body)
            return NULL;
        JS_ASSERT(!genfn->pn_body);
        genfn->pn_body = body;
        genfn->pn_pos.begin = body->pn_pos.begin = kid->pn_pos.begin;
        genfn->pn_pos.end = body->pn_pos.end = tokenStream.currentToken().pos.end;

        JSAtom *arguments = gensc.context->runtime->atomState.argumentsAtom;
        if (AtomDefnPtr p = gentc.lexdeps->lookup(arguments)) {
            Definition *dn = p.value();
            ParseNode *errorNode = dn->dn_uses ? dn->dn_uses : body;
            reportError(errorNode, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
            return NULL;
        }

        if (!LeaveFunction(genfn, this))
            return NULL;
    }

    



    ParseNode *result = ListNode::create(PNK_LP, this);
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

ParseNode *
Parser::assignExprWithoutYield(unsigned msg)
{
#ifdef JS_HAS_GENERATORS
    GenexpGuard yieldGuard(this);
#endif
    ParseNode *res = assignExpr();
    yieldGuard.endBody();
    if (res) {
#ifdef JS_HAS_GENERATORS
        if (!yieldGuard.checkValidBody(res, msg)) {
            freeTree(res);
            res = NULL;
        }
#endif
    }
    return res;
}

bool
Parser::argumentList(ParseNode *listNode)
{
    if (tokenStream.matchToken(TOK_RP, TSF_OPERAND))
        return true;

    GenexpGuard guard(this);
    bool arg0 = true;

    do {
        ParseNode *argNode = assignExpr();
        if (!argNode)
            return false;
        if (arg0)
            guard.endBody();

#if JS_HAS_GENERATORS
        if (argNode->isKind(PNK_YIELD) &&
            !argNode->isInParens() &&
            tokenStream.peekToken() == TOK_COMMA) {
            reportError(argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
            return false;
        }
#endif
#if JS_HAS_GENERATOR_EXPRS
        if (tokenStream.matchToken(TOK_FOR)) {
            if (!guard.checkValidBody(argNode))
                return false;
            argNode = generatorExpr(argNode);
            if (!argNode)
                return false;
            if (listNode->pn_count > 1 ||
                tokenStream.peekToken() == TOK_COMMA) {
                reportError(argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
                return false;
            }
        } else
#endif
        if (arg0 && !guard.maybeNoteGenerator(argNode))
            return false;

        arg0 = false;

        listNode->append(argNode);
    } while (tokenStream.matchToken(TOK_COMMA));

    if (tokenStream.getToken() != TOK_RP) {
        reportError(NULL, JSMSG_PAREN_AFTER_ARGS);
        return false;
    }
    return true;
}

ParseNode *
Parser::memberExpr(bool allowCallSyntax)
{
    ParseNode *lhs;

    JS_CHECK_RECURSION(context, return NULL);

    
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    if (tt == TOK_NEW) {
        lhs = ListNode::create(PNK_NEW, this);
        if (!lhs)
            return NULL;
        ParseNode *ctorExpr = memberExpr(false);
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
        lhs = primaryExpr(tt, false);
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
                if (allowsXML() && tokenStream.peekToken() == TOK_DBLCOLON) {
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
            else if (allowsXML()) {
                TokenPtr begin = lhs->pn_pos.begin;
                if (tt == TOK_LP) {
                    
                    tc->sc->setBindingsAccessedDynamically();

                    StmtInfoTC stmtInfo(context);
                    ParseNode *oldWith = tc->innermostWith;
                    tc->innermostWith = lhs;
                    PushStatementTC(tc, &stmtInfo, STMT_WITH);

                    ParseNode *filter = bracketedExpr();
                    if (!filter)
                        return NULL;
                    filter->setInParens(true);
                    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);

                    tc->innermostWith = oldWith;
                    PopStatementTC(context, tc);

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
                    reportError(NULL, JSMSG_NAME_AFTER_DOT);
                    return NULL;
                }
            }
#endif
            else {
                reportError(NULL, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }
        }
#if JS_HAS_XML_SUPPORT
        else if (tt == TOK_DBLDOT) {
            if (!allowsXML()) {
                reportError(NULL, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }

            nextMember = BinaryNode::create(PNK_DBLDOT, this);
            if (!nextMember)
                return NULL;
            tt = tokenStream.getToken(TSF_OPERAND | TSF_KEYWORD_IS_NAME);
            ParseNode *pn3 = primaryExpr(tt, true);
            if (!pn3)
                return NULL;
            if (pn3->isKind(PNK_NAME) && !pn3->isInParens()) {
                pn3->setKind(PNK_STRING);
                pn3->setArity(PN_NULLARY);
                pn3->setOp(JSOP_QNAMEPART);
            } else if (!pn3->isXMLPropertyIdentifier()) {
                reportError(NULL, JSMSG_NAME_AFTER_DOT);
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

            



            if (foldConstants && !FoldConstants(context, propExpr, this))
                return NULL;

            






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
                double number = propExpr->pn_dval;
                if (number != ToUint32(number)) {
                    JSAtom *atom = ToAtom(context, DoubleValue(number));
                    if (!atom)
                        return NULL;
                    name = atom->asPropertyName();
                }
            }

            if (name)
                nextMember = new_<PropertyAccess>(lhs, name, begin, end);
            else
                nextMember = new_<PropertyByValue>(lhs, propExpr, begin, end);
            if (!nextMember)
                return NULL;
        } else if (allowCallSyntax && tt == TOK_LP) {
            nextMember = ListNode::create(PNK_LP, this);
            if (!nextMember)
                return NULL;
            nextMember->setOp(JSOP_CALL);

            if (lhs->isOp(JSOP_NAME)) {
                if (lhs->pn_atom == context->runtime->atomState.evalAtom) {
                    
                    nextMember->setOp(JSOP_EVAL);
                    tc->sc->setBindingsAccessedDynamically();

                    



                    if (tc->sc->strictModeState != StrictMode::STRICT)
                        tc->sc->setFunHasExtensibleScope();
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
    




    bool oldInForInit = tc->inForInit;
    tc->inForInit = false;
    ParseNode *pn = expr();
    tc->inForInit = oldInForInit;
    return pn;
}

#if JS_HAS_XML_SUPPORT

ParseNode *
Parser::endBracketedExpr()
{
    JS_ASSERT(allowsXML());

    ParseNode *pn = bracketedExpr();
    if (!pn)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ATTR_EXPR);
    return pn;
}




















































ParseNode *
Parser::propertySelector()
{
    JS_ASSERT(allowsXML());

    ParseNode *selector;
    if (tokenStream.isCurrentTokenType(TOK_STAR)) {
        selector = NullaryNode::create(PNK_ANYNAME, this);
        if (!selector)
            return NULL;
        selector->setOp(JSOP_ANYNAME);
        selector->pn_atom = context->runtime->atomState.starAtom;
    } else {
        JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));
        selector = NullaryNode::create(PNK_NAME, this);
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
    JS_ASSERT(allowsXML());

    JS_ASSERT(tokenStream.currentToken().type == TOK_DBLCOLON);
    ParseNode *pn2 = NameNode::create(PNK_DBLCOLON, NULL, this, this->tc);
    if (!pn2)
        return NULL;

    tc->sc->setBindingsAccessedDynamically();

    
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
        reportError(NULL, JSMSG_SYNTAX_ERROR);
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
    JS_ASSERT(allowsXML());

    ParseNode *pn = propertySelector();
    if (!pn)
        return NULL;
    if (tokenStream.matchToken(TOK_DBLCOLON)) {
        
        tc->sc->setBindingsAccessedDynamically();
        pn = qualifiedSuffix(pn);
    }
    return pn;
}

ParseNode *
Parser::attributeIdentifier()
{
    JS_ASSERT(allowsXML());

    JS_ASSERT(tokenStream.currentToken().type == TOK_AT);
    ParseNode *pn = UnaryNode::create(PNK_AT, this);
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
        reportError(NULL, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    if (!pn2)
        return NULL;
    pn->pn_kid = pn2;
    return pn;
}




ParseNode *
Parser::xmlExpr(bool inTag)
{
    JS_ASSERT(allowsXML());

    JS_ASSERT(tokenStream.currentToken().type == TOK_LC);
    ParseNode *pn = UnaryNode::create(PNK_XMLCURLYEXPR, this);
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
Parser::xmlNameExpr()
{
    JS_ASSERT(allowsXML());

    ParseNode *pn, *pn2, *list;
    TokenKind tt;

    pn = list = NULL;
    do {
        tt = tokenStream.currentToken().type;
        if (tt == TOK_LC) {
            pn2 = xmlExpr(true);
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
                list = ListNode::create(PNK_XMLNAME, this);
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
    JS_ASSERT(allowsXML());

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
            list = ListNode::create(tagkind, this);
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
            pn2 = xmlExpr(true);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else {
            reportError(NULL, JSMSG_BAD_XML_ATTR_VALUE);
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
                reportError(NULL, JSMSG_END_OF_XML_SOURCE);                                 \
            }                                                                               \
            return result;                                                                  \
        }                                                                                   \
    JS_END_MACRO





bool
Parser::xmlElementContent(ParseNode *pn)
{
    JS_ASSERT(allowsXML());

    tokenStream.setXMLTagMode(false);
    for (;;) {
        TokenKind tt = tokenStream.getToken(TSF_XMLTEXTMODE);
        XML_CHECK_FOR_ERROR_AND_EOF(tt, false);

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
        XML_CHECK_FOR_ERROR_AND_EOF(tt, false);
        if (tt == TOK_XMLETAGO)
            break;

        ParseNode *pn2;
        if (tt == TOK_LC) {
            pn2 = xmlExpr(false);
            if (!pn2)
                return false;
            pn->pn_xflags |= PNX_CANTFOLD;
        } else if (tt == TOK_XMLSTAGO) {
            pn2 = xmlElementOrList(false);
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
    return true;
}




ParseNode *
Parser::xmlElementOrList(bool allowList)
{
    JS_ASSERT(allowsXML());

    ParseNode *pn, *pn2, *list;
    TokenKind tt;
    RootedAtom startAtom(context), endAtom(context);

    JS_CHECK_RECURSION(context, return NULL);

    JS_ASSERT(tokenStream.currentToken().type == TOK_XMLSTAGO);
    pn = ListNode::create(PNK_XMLSTAGO, this);
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
                reportError(NULL, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;

            
            if (!pn2->isKind(PNK_XMLSTAGO)) {
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
                pn2 = pn;
                pn = ListNode::create(PNK_XMLTAGC, this);
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
                reportError(NULL, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }

            
            pn2 = xmlTagContent(PNK_XMLETAGO, endAtom.address());
            if (!pn2)
                return NULL;
            if (pn2->isKind(PNK_XMLETAGO)) {
                
                reportError(NULL, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            if (endAtom && startAtom && endAtom != startAtom) {
                
                reportUcError(pn2, JSMSG_XML_TAG_NAME_MISMATCH, startAtom->chars());
                return NULL;
            }

            
            JS_ASSERT(pn2->isKind(PNK_XMLNAME) || pn2->isKind(PNK_XMLCURLYEXPR));
            list = ListNode::create(PNK_XMLETAGO, this);
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
        reportError(NULL, JSMSG_BAD_XML_NAME_SYNTAX);
        return NULL;
    }
    tokenStream.setXMLTagMode(false);

    pn->pn_pos.end = tokenStream.currentToken().pos.end;
    return pn;
}

ParseNode *
Parser::xmlElementOrListRoot(bool allowList)
{
    JS_ASSERT(allowsXML());

    





    bool hadMoarXML = tokenStream.hasMoarXML();
    tokenStream.setMoarXML(true);
    ParseNode *pn = xmlElementOrList(allowList);
    tokenStream.setMoarXML(hadMoarXML);
    return pn;
}

ParseNode *
Parser::parseXMLText(JSObject *chain, bool allowList)
{
    




    SharedContext xmlsc(context, chain,  NULL,  NULL, StrictMode::NOTSTRICT);
    TreeContext xmltc(this, &xmlsc,  0,  0);
    if (!xmltc.init())
        return NULL;

    
    tokenStream.setXMLOnlyMode();
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);

    ParseNode *pn;
    if (tt != TOK_XMLSTAGO) {
        reportError(NULL, JSMSG_BAD_XML_MARKUP);
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
            reportError(NULL, JSMSG_KEYWORD_NOT_NS);
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
    JS_ASSERT(allowsXML());
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));
    JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
    JS_ASSERT(tokenStream.peekToken() == TOK_DBLCOLON);

    
    tc->sc->setBindingsAccessedDynamically();

    PropertyName *name = tokenStream.currentToken().name();
    ParseNode *node = NameNode::create(PNK_NAME, name, this, this->tc);
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
    ParseNode *node = NameNode::create(PNK_NAME, name, this, this->tc);
    if (!node)
        return NULL;
    JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
    node->setOp(JSOP_NAME);

    if ((!afterDoubleDot
#if JS_HAS_XML_SUPPORT
                || (allowsXML() && tokenStream.peekToken() == TOK_DBLCOLON)
#endif
               ) && !tc->inDeclDestructuring)
    {
        if (!NoteNameUse(node, this))
            return NULL;
    }

#if JS_HAS_XML_SUPPORT
    if (allowsXML() && tokenStream.matchToken(TOK_DBLCOLON)) {
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

ParseNode *
Parser::intrinsicName()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_MOD));
    if (tokenStream.getToken() != TOK_NAME) {
        reportError(NULL, JSMSG_SYNTAX_ERROR);
        return NULL;
    }

    PropertyName *name = tokenStream.currentToken().name();
    if (!(name == context->runtime->atomState._CallFunctionAtom ||
          context->global()->hasIntrinsicFunction(context, name)))
    {
        reportError(NULL, JSMSG_INTRINSIC_NOT_DEFINED, JS_EncodeString(context, name));
        return NULL;
    }
    ParseNode *node = NameNode::create(PNK_INTRINSICNAME, name, this, this->tc);
    if (!node)
        return NULL;
    JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
    node->setOp(JSOP_INTRINSICNAME);
    return node;
}

#if JS_HAS_XML_SUPPORT
ParseNode *
Parser::starOrAtPropertyIdentifier(TokenKind tt)
{
    JS_ASSERT(tt == TOK_AT || tt == TOK_STAR);
    if (allowsXML())
        return (tt == TOK_AT) ? attributeIdentifier() : qualifiedIdentifier();
    reportError(NULL, JSMSG_SYNTAX_ERROR);
    return NULL;
}
#endif

ParseNode *
Parser::atomNode(ParseNodeKind kind, JSOp op)
{
    ParseNode *node = NullaryNode::create(kind, this);
    if (!node)
        return NULL;
    node->setOp(op);
    const Token &tok = tokenStream.currentToken();
    node->pn_atom = tok.atom();

    
    
    
    const size_t HUGE_STRING = 50000;
    if (sct && kind == PNK_STRING && node->pn_atom->length() >= HUGE_STRING)
        sct->abort();

    return node;
}

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
        if (allowsXML() && tokenStream.matchToken(TOK_DBLCOLON, TSF_KEYWORD_IS_NAME)) {
            pn2 = NullaryNode::create(PNK_FUNCTION, this);
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
        pn = ListNode::create(PNK_RB, this);
        if (!pn)
            return NULL;
        pn->setOp(JSOP_NEWINIT);
        pn->makeEmpty();

#if JS_HAS_GENERATORS
        pn->pn_blockid = tc->blockidGen;
#endif
        if (tokenStream.matchToken(TOK_RB, TSF_OPERAND)) {
            



            pn->pn_xflags |= PNX_NONCONST;
        } else {
            bool spread = false;
            unsigned index = 0;
            for (; ; index++) {
                if (index == StackSpace::ARGS_LENGTH_MAX) {
                    reportError(NULL, JSMSG_ARRAY_INIT_TOO_BIG);
                    return NULL;
                }

                tt = tokenStream.peekToken(TSF_OPERAND);
                if (tt == TOK_RB) {
                    pn->pn_xflags |= PNX_ENDCOMMA;
                    break;
                }

                if (tt == TOK_COMMA) {
                    
                    tokenStream.matchToken(TOK_COMMA);
                    pn2 = NullaryNode::create(PNK_COMMA, this);
                    pn->pn_xflags |= PNX_HOLEY | PNX_NONCONST;
                } else {
                    ParseNode *spreadNode = NULL;
                    if (tt == TOK_TRIPLEDOT) {
                        spread = true;
                        spreadNode = UnaryNode::create(PNK_SPREAD, this);
                        if (!spreadNode)
                            return NULL;
                        tokenStream.getToken();
                    }
                    pn2 = assignExpr();
                    if (pn2) {
                        if (foldConstants && !FoldConstants(context, pn2, this))
                            return NULL;
                        if (!pn2->isConstant() || spreadNode)
                            pn->pn_xflags |= PNX_NONCONST;
                        if (spreadNode) {
                            spreadNode->pn_kid = pn2;
                            pn2 = spreadNode;
                        }
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
            








































            if (index == 0 && !spread && pn->pn_count != 0 && tokenStream.matchToken(TOK_FOR)) {
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

        pn = ListNode::create(PNK_RC, this);
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
                pn3 = NullaryNode::create(PNK_NUMBER, this);
                if (!pn3)
                    return NULL;
                pn3->pn_dval = tokenStream.currentToken().number();
                atom = ToAtom(context, DoubleValue(pn3->pn_dval));
                if (!atom)
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
                        pn3 = NullaryNode::create(PNK_NAME, this);
                        if (!pn3)
                            return NULL;
                        pn3->pn_atom = atom;
                        break;
                    }

                    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
                    if (tt == TOK_NAME) {
                        atom = tokenStream.currentToken().name();
                        pn3 = NameNode::create(PNK_NAME, atom, this, this->tc);
                        if (!pn3)
                            return NULL;
                    } else if (tt == TOK_STRING) {
                        atom = tokenStream.currentToken().atom();

                        uint32_t index;
                        if (atom->isIndex(&index)) {
                            pn3 = NullaryNode::create(PNK_NUMBER, this);
                            if (!pn3)
                                return NULL;
                            pn3->pn_dval = index;
                            atom = ToAtom(context, DoubleValue(pn3->pn_dval));
                            if (!atom)
                                return NULL;
                        } else {
                            pn3 = NameNode::create(PNK_STRING, atom, this, this->tc);
                            if (!pn3)
                                return NULL;
                        }
                    } else if (tt == TOK_NUMBER) {
                        pn3 = NullaryNode::create(PNK_NUMBER, this);
                        if (!pn3)
                            return NULL;
                        pn3->pn_dval = tokenStream.currentToken().number();
                        atom = ToAtom(context, DoubleValue(pn3->pn_dval));
                        if (!atom)
                            return NULL;
                    } else {
                        tokenStream.ungetToken();
                        pn3 = NullaryNode::create(PNK_NAME, this);
                        if (!pn3)
                            return NULL;
                        pn3->pn_atom = atom;
                        break;
                    }

                    pn->pn_xflags |= PNX_NONCONST;

                    
                    Rooted<PropertyName*> funName(context, NULL);
                    pn2 = functionDef(funName, op == JSOP_GETTER ? Getter : Setter, Expression);
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
                    pn3 = NullaryNode::create(PNK_NUMBER, this);
                    if (!pn3)
                        return NULL;
                    pn3->pn_dval = index;
                } else {
                    pn3 = NullaryNode::create(PNK_STRING, this);
                    if (!pn3)
                        return NULL;
                    pn3->pn_atom = atom;
                }
                break;
              }
              case TOK_RC:
                goto end_obj_init;
              default:
                reportError(NULL, JSMSG_BAD_PROP_ID);
                return NULL;
            }

            op = JSOP_INITPROP;
            tt = tokenStream.getToken();
            if (tt == TOK_COLON) {
                pnval = assignExpr();
                if (!pnval)
                    return NULL;

                if (foldConstants && !FoldConstants(context, pnval, this))
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
                reportError(NULL, JSMSG_COLON_AFTER_ID);
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
                    (oldAssignType != VALUE || assignType != VALUE || tc->sc->needStrictChecks()))
                {
                    JSAutoByteString name;
                    if (!js_AtomToPrintableString(context, atom, &name))
                        return NULL;

                    Reporter reporter =
                        (oldAssignType == VALUE && assignType == VALUE && !tc->sc->needStrictChecks())
                        ? &Parser::reportWarning
                        : (tc->sc->needStrictChecks() ? &Parser::reportStrictModeError : &Parser::reportError);
                    if (!(this->*reporter)(NULL, JSMSG_DUPLICATE_PROPERTY, name.ptr()))
                        return NULL;
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
                reportError(NULL, JSMSG_CURLY_AFTER_LIST);
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
        bool genexp;

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
        if (!allowsXML())
            goto syntaxerror;
        pn = starOrAtPropertyIdentifier(tt);
        break;

      case TOK_XMLSTAGO:
        if (!allowsXML())
            goto syntaxerror;
        pn = xmlElementOrListRoot(true);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLCDATA:
        if (!allowsXML())
            goto syntaxerror;
        pn = atomNode(PNK_XMLCDATA, JSOP_XMLCDATA);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLCOMMENT:
        if (!allowsXML())
            goto syntaxerror;
        pn = atomNode(PNK_XMLCOMMENT, JSOP_XMLCOMMENT);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLPI: {
        if (!allowsXML())
            goto syntaxerror;
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
        pn = NullaryNode::create(PNK_REGEXP, this);
        if (!pn)
            return NULL;

        const jschar *chars = tokenStream.getTokenbuf().begin();
        size_t length = tokenStream.getTokenbuf().length();
        RegExpFlag flags = tokenStream.currentToken().regExpFlags();
        RegExpStatics *res = context->regExpStatics();

        Rooted<RegExpObject*> reobj(context);
        if (context->hasfp())
            reobj = RegExpObject::create(context, res, chars, length, flags, &tokenStream);
        else
            reobj = RegExpObject::createNoStatics(context, chars, length, flags, &tokenStream);

        if (!reobj)
            return NULL;

        if (!compileAndGo) {
            if (!JSObject::clearParent(context, reobj))
                return NULL;
            if (!JSObject::clearType(context, reobj))
                return NULL;
        }

        pn->pn_objbox = newObjectBox(reobj);
        if (!pn->pn_objbox)
            return NULL;

        pn->setOp(JSOP_REGEXP);
        break;
      }

      case TOK_NUMBER:
        pn = NullaryNode::create(PNK_NUMBER, this);
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

      case TOK_MOD:
        if (selfHostingMode)
            return intrinsicName();
        else
            goto syntaxerror;

      case TOK_ERROR:
        
        return NULL;

    syntaxerror:
      default:
        reportError(NULL, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    return pn;
}

ParseNode *
Parser::parenExpr(bool *genexp)
{
    TokenPtr begin;
    ParseNode *pn;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LP);
    begin = tokenStream.currentToken().pos.begin;

    if (genexp)
        *genexp = false;

    GenexpGuard guard(this);

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
            reportError(pn->last(), JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
            return NULL;
        }
        pn = generatorExpr(pn);
        if (!pn)
            return NULL;
        pn->pn_pos.begin = begin;
        if (genexp) {
            if (tokenStream.getToken() != TOK_RP) {
                reportError(NULL, JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
                return NULL;
            }
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
            *genexp = true;
        }
    } else
#endif 

    if (!guard.maybeNoteGenerator(pn))
        return NULL;

    return pn;
}

