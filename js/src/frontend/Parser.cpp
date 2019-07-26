



















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
#include "jsscript.h"
#include "jsstr.h"

#include "frontend/FoldConstants.h"
#include "frontend/ParseMaps.h"
#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "gc/Marking.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/Parser-inl.h"
#include "frontend/SharedContext-inl.h"

#include "vm/NumericConversions.h"
#include "vm/RegExpObject-inl.h"

using namespace js;
using namespace js::gc;

namespace js {
namespace frontend {

typedef Rooted<StaticBlockObject*> RootedStaticBlockObject;
typedef Handle<StaticBlockObject*> HandleStaticBlockObject;

typedef MutableHandle<PropertyName*> MutableHandlePropertyName;





#define MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, __flags)                                     \
    JS_BEGIN_MACRO                                                                          \
        if (tokenStream.getToken((__flags)) != tt) {                                        \
            report(ParseError, false, null(), errno);                                       \
            return null();                                                                  \
        }                                                                                   \
    JS_END_MACRO
#define MUST_MATCH_TOKEN(tt, errno) MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, 0)

template <typename ParseHandler>
bool
GenerateBlockId(ParseContext<ParseHandler> *pc, uint32_t &blockid)
{
    if (pc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(pc->sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    JS_ASSERT(pc->blockidGen < JS_BIT(20));
    blockid = pc->blockidGen++;
    return true;
}

template bool
GenerateBlockId(ParseContext<SyntaxParseHandler> *pc, uint32_t &blockid);

template bool
GenerateBlockId(ParseContext<FullParseHandler> *pc, uint32_t &blockid);

template <typename ParseHandler>
static void
PushStatementPC(ParseContext<ParseHandler> *pc, StmtInfoPC *stmt, StmtType type)
{
    stmt->blockid = pc->blockid();
    PushStatement(pc, stmt, type);
}


template <>
bool
ParseContext<FullParseHandler>::define(JSContext *cx, HandlePropertyName name,
                                       ParseNode *pn, Definition::Kind kind)
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
        JS_ASSERT(sc->isFunctionBox());
        dn->setOp(JSOP_GETARG);
        dn->pn_dflags |= PND_BOUND;
        if (!dn->pn_cookie.set(cx, staticLevel, args_.length()))
            return false;
        if (!args_.append(dn))
            return false;
        if (name == cx->names().empty)
            break;
        if (!decls_.addUnique(name, dn))
            return false;
        break;

      case Definition::CONST:
      case Definition::VAR:
        if (sc->isFunctionBox()) {
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

template <>
bool
ParseContext<SyntaxParseHandler>::define(JSContext *cx, HandlePropertyName name, Node pn,
                                         Definition::Kind kind)
{
    return true;
}

template <typename ParseHandler>
void
ParseContext<ParseHandler>::prepareToAddDuplicateArg(Definition *prevDecl)
{
    JS_ASSERT(prevDecl->kind() == Definition::ARG);
    JS_ASSERT(decls_.lookupFirst(prevDecl->name()) == prevDecl);
    JS_ASSERT(!prevDecl->isClosed());
    decls_.remove(prevDecl->name());
}

template <typename ParseHandler>
void
ParseContext<ParseHandler>::updateDecl(JSAtom *atom, Node pn)
{
    Definition *oldDecl = decls_.lookupFirst(atom);

    pn->setDefn(true);
    Definition *newDecl = (Definition *)pn;
    decls_.updateFirst(atom, newDecl);

    if (!sc->isFunctionBox()) {
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

template <typename ParseHandler>
void
ParseContext<ParseHandler>::popLetDecl(JSAtom *atom)
{
    JS_ASSERT(decls_.lookupFirst(atom)->isLet());
    decls_.remove(atom);
}

template <typename ParseHandler>
static void
AppendPackedBindings(const ParseContext<ParseHandler> *pc, const DeclVector &vec, Binding *dst)
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

        




        JS_ASSERT_IF(dn->isClosed(), pc->decls().lookupFirst(name) == dn);
        bool aliased = dn->isClosed() ||
                       (pc->sc->bindingsAccessedDynamically() &&
                        pc->decls().lookupFirst(name) == dn);

        *dst = Binding(name, kind, aliased);
    }
}

template <typename ParseHandler>
bool
ParseContext<ParseHandler>::generateFunctionBindings(JSContext *cx, InternalHandle<Bindings*> bindings) const
{
    JS_ASSERT(sc->isFunctionBox());

    unsigned count = args_.length() + vars_.length();
    Binding *packedBindings = cx->tempLifoAlloc().newArrayUninitialized<Binding>(count);
    if (!packedBindings) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    AppendPackedBindings(this, args_, packedBindings);
    AppendPackedBindings(this, vars_, packedBindings + args_.length());

    if (!Bindings::initWithTemporaryStorage(cx, bindings, args_.length(), vars_.length(),
                                            packedBindings))
    {
        return false;
    }

    FunctionBox *funbox = sc->asFunctionBox();
    if (bindings->hasAnyAliasedBindings() || funbox->hasExtensibleScope())
        funbox->function()->setIsHeavyweight();

    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::report(ParseReportKind kind, bool strict, Node pn, unsigned errorNumber, ...)
{
    uint32_t offset = (pn ? handler.getPosition(pn) : tokenStream.currentToken().pos).begin;

    va_list args;
    va_start(args, errorNumber);
    bool result = false;
    switch (kind) {
      case ParseError:
        result = tokenStream.reportCompileErrorNumberVA(offset, JSREPORT_ERROR, errorNumber, args);
        break;
      case ParseWarning:
        result =
            tokenStream.reportCompileErrorNumberVA(offset, JSREPORT_WARNING, errorNumber, args);
        break;
      case ParseStrictWarning:
        result = tokenStream.reportStrictWarningErrorNumberVA(offset, errorNumber, args);
        break;
      case ParseStrictError:
        result = tokenStream.reportStrictModeErrorNumberVA(offset, strict, errorNumber, args);
        break;
    }
    va_end(args);
    return result;
}

template <typename ParseHandler>
Parser<ParseHandler>::Parser(JSContext *cx, const CompileOptions &options,
                             const jschar *chars, size_t length, bool foldConstants)
  : AutoGCRooter(cx, PARSER),
    context(cx),
    tokenStream(cx, options, chars, length, thisForCtor()),
    tempPoolMark(NULL),
    traceListHead(NULL),
    pc(NULL),
    sct(NULL),
    keepAtoms(cx->runtime),
    foldConstants(foldConstants),
    compileAndGo(options.compileAndGo),
    selfHostingMode(options.selfHostingMode),
    unknownResult(false),
    handler(cx, tokenStream, foldConstants)
{
    cx->activeCompilations++;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::init()
{
    if (!context->ensureParseMapPool())
        return false;

    tempPoolMark = context->tempLifoAlloc().mark();
    return true;
}

template <typename ParseHandler>
Parser<ParseHandler>::~Parser()
{
    JSContext *cx = context;
    cx->tempLifoAlloc().release(tempPoolMark);
    cx->activeCompilations--;

    




    cx->tempLifoAlloc().freeAllIfHugeAndUnused();
}

template <typename ParseHandler>
ObjectBox *
Parser<ParseHandler>::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj && !IsPoisonedPtr(obj));

    







    ObjectBox *objbox = context->tempLifoAlloc().new_<ObjectBox>(obj, traceListHead);
    if (!objbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = objbox;

    return objbox;
}

template <typename ParseHandler>
FunctionBox::FunctionBox(JSContext *cx, ObjectBox* traceListHead, JSFunction *fun,
                         ParseContext<ParseHandler> *outerpc, bool strict)
  : ObjectBox(fun, traceListHead),
    SharedContext(cx, strict),
    bindings(),
    bufStart(0),
    bufEnd(0),
    asmStart(0),
    ndefaults(0),
    inWith(false),                  
    inGenexpLambda(false),
    useAsm(false),
    insideUseAsm(outerpc && outerpc->useAsmOrInsideUseAsm()),
    funCxFlags()
{
    if (!outerpc) {
        inWith = false;

    } else if (outerpc->parsingWith) {
        
        
        
        
        
        
        inWith = true;

    } else if (outerpc->sc->isGlobalSharedContext()) {
        
        
        
        
        
        
        
        
        
        
        JSObject *scope = outerpc->sc->asGlobalSharedContext()->scopeChain();
        while (scope) {
            if (scope->isWith())
                inWith = true;
            scope = scope->enclosingScope();
        }
    } else if (outerpc->sc->isFunctionBox()) {
        
        
        
        
        
        
        
        FunctionBox *parent = outerpc->sc->asFunctionBox();
        if (parent && parent->inWith)
            inWith = true;
    }
}

template <typename ParseHandler>
FunctionBox *
Parser<ParseHandler>::newFunctionBox(JSFunction *fun,
                                     ParseContext<ParseHandler> *outerpc, bool strict)
{
    JS_ASSERT(fun && !IsPoisonedPtr(fun));

    






    FunctionBox *funbox =
        context->tempLifoAlloc().new_<FunctionBox>(context, traceListHead, fun, outerpc, strict);
    if (!funbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = funbox;

    return funbox;
}

ModuleBox::ModuleBox(JSContext *cx, ObjectBox *traceListHead, Module *module,
                     ParseContext<FullParseHandler> *pc)
    : ObjectBox(module, traceListHead),
      SharedContext(cx, true)
{
}

template <>
ModuleBox *
Parser<FullParseHandler>::newModuleBox(Module *module, ParseContext<FullParseHandler> *outerpc)
{
    JS_ASSERT(module && !IsPoisonedPtr(module));

    






    ModuleBox *modulebox =
        context->tempLifoAlloc().new_<ModuleBox>(context, traceListHead, module, outerpc);
    if (!modulebox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = modulebox;

    return modulebox;
}

template <typename ParseHandler>
void
Parser<ParseHandler>::trace(JSTracer *trc)
{
    traceListHead->trace(trc);
}




template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::parse(JSObject *chain)
{
    







    GlobalSharedContext globalsc(context, chain, StrictModeFromContext(context));
    ParseContext<ParseHandler> globalpc(this, &globalsc,  0,  0);
    if (!globalpc.init())
        return null();

    Node pn = statements();
    if (pn) {
        if (!tokenStream.matchToken(TOK_EOF)) {
            report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
            return null();
        }
        if (foldConstants) {
            if (!FoldConstants(context, &pn, this))
                return null();
        }
    }
    return pn;
}







enum {
    ENDS_IN_OTHER = 0,
    ENDS_IN_RETURN = 1,
    ENDS_IN_BREAK = 2
};

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

static int
HasFinalReturn(SyntaxParseHandler::Node pn)
{
    return ENDS_IN_RETURN;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::reportBadReturn(Node pn, ParseReportKind kind,
                                      unsigned errnum, unsigned anonerrnum)
{
    JSAutoByteString name;
    JSAtom *atom = pc->sc->asFunctionBox()->function()->atom();
    if (atom) {
        if (!js_AtomToPrintableString(context, atom, &name))
            return false;
    } else {
        errnum = anonerrnum;
    }
    return report(kind, pc->sc->strict, pn, errnum, name.ptr());
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::checkFinalReturn(Node pn)
{
    JS_ASSERT(pc->sc->isFunctionBox());
    return HasFinalReturn(pn) == ENDS_IN_RETURN ||
           reportBadReturn(pn, ParseStrictWarning,
                           JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE);
}





template <typename ParseHandler>
bool
Parser<ParseHandler>::checkStrictAssignment(Node lhs)
{
    if (!pc->sc->needStrictChecks())
        return true;

    JSAtom *atom = handler.isName(lhs);
    if (!atom)
        return true;

    if (atom == context->names().eval || atom == context->names().arguments) {
        JSAutoByteString name;
        if (!js_AtomToPrintableString(context, atom, &name) ||
            !report(ParseStrictError, pc->sc->strict, lhs,
                    JSMSG_DEPRECATED_ASSIGN, name.ptr()))
        {
            return false;
        }
    }
    return true;
}







template <typename ParseHandler>
bool
Parser<ParseHandler>::checkStrictBinding(HandlePropertyName name, Node pn)
{
    if (!pc->sc->needStrictChecks())
        return true;

    if (name == context->names().eval ||
        name == context->names().arguments ||
        FindKeyword(name->charsZ(), name->length()))
    {
        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(context, name, &bytes))
            return false;
        return report(ParseStrictError, pc->sc->strict, pn,
                      JSMSG_BAD_BINDING, bytes.ptr());
    }

    return true;
}

template <>
bool
Parser<FullParseHandler>::defineArg(ParseNode *funcpn, HandlePropertyName name,
                                    bool disallowDuplicateArgs, Definition **duplicatedArg);

template <>
ParseNode *
Parser<FullParseHandler>::standaloneFunctionBody(HandleFunction fun, const AutoNameVector &formals,
                                                 HandleScript script, Node fn, FunctionBox **funbox,
                                                 bool strict, bool *becameStrict)
{
    if (becameStrict)
        *becameStrict = false;

    *funbox = newFunctionBox(fun,  NULL, strict);
    if (!funbox)
        return null();
    handler.setFunctionBox(fn, *funbox);

    ParseContext<FullParseHandler> funpc(this, *funbox,  0,  0);
    if (!funpc.init())
        return null();

    for (unsigned i = 0; i < formals.length(); i++) {
        if (!defineArg(fn, formals[i]))
            return null();
    }

    ParseNode *pn = functionBody(Statement, StatementListBody);
    if (!pn) {
        if (becameStrict && pc->funBecameStrict)
            *becameStrict = true;
        return null();
    }

    if (!tokenStream.matchToken(TOK_EOF)) {
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();
    }

    if (!FoldConstants(context, &pn, this))
        return null();

    InternalHandle<Bindings*> bindings(script, &script->bindings);
    if (!funpc.generateFunctionBindings(context, bindings))
        return null();

    return pn;
}

template <>
bool
Parser<FullParseHandler>::checkFunctionArguments()
{
    
    HandlePropertyName arguments = context->names().arguments;

    







    if (FuncStmtSet *set = pc->funcStmts) {
        for (FuncStmtSet::Range r = set->all(); !r.empty(); r.popFront()) {
            PropertyName *name = r.front()->asPropertyName();
            if (name == arguments)
                pc->sc->setBindingsAccessedDynamically();
            else if (Definition *dn = pc->decls().lookupFirst(name))
                dn->pn_dflags |= PND_CLOSED;
        }
    }

    




    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            Definition *dn = r.front().value();
            pc->lexdeps->remove(arguments);
            dn->pn_dflags |= PND_IMPLICITARGUMENTS;
            if (!pc->define(context, arguments, dn, Definition::VAR))
                return false;
            break;
        }
    }

    



    Definition *maybeArgDef = pc->decls().lookupFirst(arguments);
    bool argumentsHasBinding = !!maybeArgDef;
    bool argumentsHasLocalBinding = maybeArgDef && maybeArgDef->kind() != Definition::ARG;
    bool hasRest = pc->sc->asFunctionBox()->function()->hasRest();
    if (hasRest && argumentsHasLocalBinding) {
        report(ParseError, false, NULL, JSMSG_ARGUMENTS_AND_REST);
        return false;
    }

    




    if (!argumentsHasBinding && pc->sc->bindingsAccessedDynamically() && !hasRest) {
        ParseNode *pn = NameNode::create(PNK_NAME, arguments, &handler, pc);
        if (!pn)
            return false;
        if (!pc->define(context, arguments, pn, Definition::VAR))
            return false;
        argumentsHasBinding = true;
        argumentsHasLocalBinding = true;
    }

    




    if (argumentsHasLocalBinding) {
        FunctionBox *funbox = pc->sc->asFunctionBox();
        funbox->setArgumentsHasLocalBinding();

        






        if (pc->sc->bindingsAccessedDynamically() && maybeArgDef)
            funbox->setDefinitelyNeedsArgsObj();

        





        if (pc->sc->hasDebuggerStatement())
            funbox->setDefinitelyNeedsArgsObj();

        






        if (pc->sc->needStrictChecks()) {
            for (AtomDefnListMap::Range r = pc->decls().all(); !r.empty(); r.popFront()) {
                DefinitionList &dlist = r.front().value();
                for (DefinitionList::Range dr = dlist.all(); !dr.empty(); dr.popFront()) {
                    Definition *dn = dr.front();
                    if (dn->kind() == Definition::ARG && dn->isAssigned())
                        funbox->setDefinitelyNeedsArgsObj();
                }
            }
            
            if (pc->sc->bindingsAccessedDynamically())
                funbox->setDefinitelyNeedsArgsObj();
        }
    }

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkFunctionArguments()
{
    return true;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionBody(FunctionSyntaxKind kind, FunctionBodyType type)
{
    JS_ASSERT(pc->sc->isFunctionBox());
    JS_ASSERT(!pc->funHasReturnExpr && !pc->funHasReturnVoid);

    Node pn;
    if (type == StatementListBody) {
        pn = statements();
        if (!pn)
            return null();
    } else {
        JS_ASSERT(type == ExpressionBody);
        JS_ASSERT(JS_HAS_EXPR_CLOSURES);

        Node kid = assignExpr();
        if (!kid)
            return null();

        pn = handler.newUnary(PNK_RETURN, kid, JSOP_RETURN);
        if (!pn)
            return null();

        if (pc->sc->asFunctionBox()->isGenerator()) {
            reportBadReturn(pn, ParseError,
                            JSMSG_BAD_GENERATOR_RETURN,
                            JSMSG_BAD_ANON_GENERATOR_RETURN);
            return null();
        }
    }

    
    if (context->hasStrictOption() && pc->funHasReturnExpr && !checkFinalReturn(pn))
        return null();

    if (kind != Arrow) {
        
        if (!checkFunctionArguments())
            return null();
    }

    return pn;
}





static Definition *
MakePlaceholder(ParseNode *pn, FullParseHandler *handler, ParseContext<FullParseHandler> *pc)
{
    Definition *dn = (Definition *) NameNode::create(PNK_NAME, pn->pn_atom, handler, pc);
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

static void
ForgetUse(SyntaxParseHandler::Node pn)
{
}


template <>
bool
Parser<FullParseHandler>::makeDefIntoUse(Definition *dn, ParseNode *pn, JSAtom *atom)
{
    
    pc->updateDecl(atom, pn);

    
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
        handler.prepareNodeForMutation(dn);
        dn->setKind(PNK_NOP);
        dn->setArity(PN_NULLARY);
        return true;
    }

    




    if (dn->canHaveInitializer()) {
        if (ParseNode *rhs = dn->expr()) {
            ParseNode *lhs = handler.makeAssignment(dn, rhs);
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









template <typename ParseHandler>
struct BindData
{
    BindData(JSContext *cx) : let(cx) {}

    typedef bool
    (*Binder)(JSContext *cx, BindData *data, HandlePropertyName name, Parser<ParseHandler> *parser);

    
    typename ParseHandler::Node pn;

    JSOp            op;         
    Binder          binder;     

    struct LetData {
        LetData(JSContext *cx) : blockObj(cx) {}
        VarContext varContext;
        RootedStaticBlockObject blockObj;
        unsigned   overflow;
    } let;

    void initLet(VarContext varContext, StaticBlockObject &blockObj, unsigned overflow) {
        this->pn = ParseHandler::null();
        this->op = JSOP_NOP;
        this->binder = Parser<ParseHandler>::bindLet;
        this->let.varContext = varContext;
        this->let.blockObj = &blockObj;
        this->let.overflow = overflow;
    }

    void initVarOrConst(JSOp op) {
        this->op = op;
        this->binder = Parser<ParseHandler>::bindVarOrConst;
    }
};

template <typename ParseHandler>
JSFunction *
Parser<ParseHandler>::newFunction(ParseContext<ParseHandler> *pc, HandleAtom atom,
                                  FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, atom != NULL);

    





    while (pc->parent)
        pc = pc->parent;

    RootedObject parent(context);
    parent = pc->sc->isFunctionBox() ? NULL : pc->sc->asGlobalSharedContext()->scopeChain();

    RootedFunction fun(context);
    JSFunction::Flags flags = (kind == Expression)
                              ? JSFunction::INTERPRETED_LAMBDA
                              : (kind == Arrow)
                                ? JSFunction::INTERPRETED_LAMBDA_ARROW
                                : JSFunction::INTERPRETED;
    fun = NewFunction(context, NullPtr(), NULL, 0, flags, parent, atom,
                      JSFunction::FinalizeKind, MaybeSingletonObject);
    if (selfHostingMode)
        fun->setIsSelfHostedBuiltin();
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







template <>
bool
Parser<FullParseHandler>::leaveFunction(ParseNode *fn, HandlePropertyName funName,
                                        FunctionSyntaxKind kind)
{
    ParseContext<FullParseHandler> *funpc = pc;
    ParseContext<FullParseHandler> *pc = funpc->parent;
    pc->blockidGen = funpc->blockidGen;

    FunctionBox *funbox = fn->pn_funbox;
    JS_ASSERT(funbox == funpc->sc->asFunctionBox());

    if (!pc->topStmt || pc->topStmt->type == STMT_BLOCK)
        fn->pn_dflags |= PND_BLOCKCHILD;

    
    if (funpc->lexdeps->count()) {
        for (AtomDefnRange r = funpc->lexdeps->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key();
            Definition *dn = r.front().value();
            JS_ASSERT(dn->isPlaceholder());

            if (atom == funName && kind == Expression) {
                dn->setOp(JSOP_CALLEE);
                if (!dn->pn_cookie.set(context, funpc->staticLevel,
                                       UpvarCookie::CALLEE_SLOT))
                    return false;
                dn->pn_dflags |= PND_BOUND;
                JS_ASSERT(dn->kind() == Definition::NAMED_LAMBDA);

                










                if (dn->isClosed() || dn->isAssigned())
                    funbox->function()->setIsHeavyweight();
                continue;
            }

            




            if (!dn->dn_uses)
                continue;

            Definition *outer_dn = pc->decls().lookupFirst(atom);

            




            if (funbox->hasExtensibleScope() || pc->parsingWith)
                DeoptimizeUsesWithin(dn, fn->pn_pos);

            if (!outer_dn) {
                AtomDefnAddPtr p = pc->lexdeps->lookupForAdd(atom);
                if (p) {
                    outer_dn = p.value();
                } else {
                    




















                    outer_dn = MakePlaceholder(dn, &handler, pc);
                    if (!outer_dn || !pc->lexdeps->add(p, atom, outer_dn))
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

    InternalHandle<Bindings*> bindings =
        InternalHandle<Bindings*>::fromMarkedLocation(&funbox->bindings);
    if (!funpc->generateFunctionBindings(context, bindings))
        return false;

    funpc->lexdeps.releaseMap(context);
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::leaveFunction(Node fn, HandlePropertyName funName,
                                          FunctionSyntaxKind kind)
{
    pc->lexdeps.releaseMap(context);
    return true;
}















template <>
bool
Parser<FullParseHandler>::defineArg(ParseNode *funcpn, HandlePropertyName name,
                                    bool disallowDuplicateArgs, Definition **duplicatedArg)
{
    SharedContext *sc = pc->sc;

    
    if (Definition *prevDecl = pc->decls().lookupFirst(name)) {
        





        if (sc->needStrictChecks()) {
            JSAutoByteString bytes;
            if (!js_AtomToPrintableString(context, name, &bytes))
                return false;
            if (!report(ParseStrictError, pc->sc->strict, prevDecl,
                        JSMSG_DUPLICATE_FORMAL, bytes.ptr()))
            {
                return false;
            }
        }

        if (disallowDuplicateArgs) {
            report(ParseError, false, prevDecl, JSMSG_BAD_DUP_ARGS);
            return false;
        }

        if (duplicatedArg)
            *duplicatedArg = prevDecl;

        
        pc->prepareToAddDuplicateArg(prevDecl);
    }

    ParseNode *argpn = handler.newName(name, pc);
    if (!argpn)
        return false;

    if (!checkStrictBinding(name, argpn))
        return false;

    funcpn->pn_body->append(argpn);
    return pc->define(context, name, argpn, Definition::ARG);
}

template <>
bool
Parser<SyntaxParseHandler>::defineArg(Node funcpn, HandlePropertyName name,
                                      bool disallowDuplicateArgs, DefinitionNode *duplicatedArg)
{
    return true;
}

#if JS_HAS_DESTRUCTURING
template <typename ParseHandler>
 bool
Parser<ParseHandler>::bindDestructuringArg(JSContext *cx, BindData<ParseHandler> *data,
                                           HandlePropertyName name, Parser<ParseHandler> *parser)
{
    ParseContext<ParseHandler> *pc = parser->pc;
    JS_ASSERT(pc->sc->isFunctionBox());

    if (pc->decls().lookupFirst(name)) {
        parser->report(ParseError, false, null(), JSMSG_BAD_DUP_ARGS);
        return false;
    }

    if (!parser->checkStrictBinding(name, data->pn))
        return false;

    return pc->define(cx, name, data->pn, Definition::VAR);
}
#endif 

template <typename ParseHandler>
bool
Parser<ParseHandler>::functionArguments(FunctionSyntaxKind kind, Node *listp, Node funcpn,
                                        bool &hasRest)
{
    FunctionBox *funbox = pc->sc->asFunctionBox();

    bool parenFreeArrow = false;
    if (kind == Arrow && tokenStream.peekToken() == TOK_NAME) {
        parenFreeArrow = true;
    } else {
        if (tokenStream.getToken() != TOK_LP) {
            report(ParseError, false, null(),
                   kind == Arrow ? JSMSG_BAD_ARROW_ARGS : JSMSG_PAREN_BEFORE_FORMAL);
            return false;
        }

        
        
        funbox->bufStart = tokenStream.currentToken().pos.begin;
    }

    hasRest = false;

    Node argsbody = handler.newList(PNK_ARGSBODY);
    if (!argsbody)
        return false;
    handler.setFunctionBody(funcpn, argsbody);

    if (parenFreeArrow || !tokenStream.matchToken(TOK_RP)) {
        bool hasDefaults = false;
        DefinitionNode duplicatedArg = null();
        bool destructuringArg = false;
#if JS_HAS_DESTRUCTURING
        Node list = null();
#endif

        do {
            if (hasRest) {
                report(ParseError, false, null(), JSMSG_PARAMETER_AFTER_REST);
                return false;
            }

            TokenKind tt = tokenStream.getToken();
            JS_ASSERT_IF(parenFreeArrow, tt == TOK_NAME);
            switch (tt) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
              {
                
                if (duplicatedArg) {
                    report(ParseError, false, duplicatedArg, JSMSG_BAD_DUP_ARGS);
                    return false;
                }

                if (hasDefaults) {
                    report(ParseError, false, null(), JSMSG_NONDEFAULT_FORMAL_AFTER_DEFAULT);
                    return false;
                }

                destructuringArg = true;

                





                BindData<ParseHandler> data(context);
                data.pn = ParseHandler::null();
                data.op = JSOP_DEFVAR;
                data.binder = bindDestructuringArg;
                Node lhs = destructuringExpr(&data, tt);
                if (!lhs)
                    return false;

                




                HandlePropertyName name = context->names().empty;
                Node rhs = handler.newName(name, pc);
                if (!rhs)
                    return false;

                if (!pc->define(context, name, rhs, Definition::ARG))
                    return false;

                Node item = handler.newBinary(PNK_ASSIGN, lhs, rhs);
                if (!item)
                    return false;
                if (list) {
                    handler.addList(list, item);
                } else {
                    list = handler.newList(PNK_VAR, item);
                    if (!list)
                        return false;
                    *listp = list;
                }
                break;
              }
#endif 

              case TOK_TRIPLEDOT:
              {
                hasRest = true;
                tt = tokenStream.getToken();
                if (tt != TOK_NAME) {
                    if (tt != TOK_ERROR)
                        report(ParseError, false, null(), JSMSG_NO_REST_NAME);
                    return false;
                }
                
              }

              case TOK_NAME:
              {
                if (parenFreeArrow)
                    funbox->bufStart = tokenStream.currentToken().pos.begin;

                RootedPropertyName name(context, tokenStream.currentToken().name());
                bool disallowDuplicateArgs = destructuringArg || hasDefaults;
                if (!defineArg(funcpn, name, disallowDuplicateArgs, &duplicatedArg))
                    return false;

                if (tokenStream.matchToken(TOK_ASSIGN)) {
                    
                    
                    
                    
                    JS_ASSERT(!parenFreeArrow);

                    if (hasRest) {
                        report(ParseError, false, null(), JSMSG_REST_WITH_DEFAULT);
                        return false;
                    }
                    if (duplicatedArg) {
                        report(ParseError, false, duplicatedArg, JSMSG_BAD_DUP_ARGS);
                        return false;
                    }
                    hasDefaults = true;
                    Node def_expr = assignExprWithoutYield(JSMSG_YIELD_IN_DEFAULT);
                    if (!def_expr)
                        return false;
                    handler.setLastFunctionArgumentDefault(funcpn, def_expr);
                    funbox->ndefaults++;
                } else if (!hasRest && hasDefaults) {
                    report(ParseError, false, null(), JSMSG_NONDEFAULT_FORMAL_AFTER_DEFAULT);
                    return false;
                }

                break;
              }

              default:
                report(ParseError, false, null(), JSMSG_MISSING_FORMAL);
                
              case TOK_ERROR:
                return false;
            }
        } while (!parenFreeArrow && tokenStream.matchToken(TOK_COMMA));

        if (!parenFreeArrow && tokenStream.getToken() != TOK_RP) {
            report(ParseError, false, null(), JSMSG_PAREN_AFTER_FORMAL);
            return false;
        }
    }

    return true;
}

template <>
bool
Parser<FullParseHandler>::checkFunctionDefinition(HandlePropertyName funName,
                                                  ParseNode **pn_, FunctionSyntaxKind kind)
{
    ParseNode *&pn = *pn_;

    
    bool bodyLevel = pc->atBodyLevel();

    if (kind == Statement) {
        



        if (Definition *dn = pc->decls().lookupFirst(funName)) {
            JS_ASSERT(!dn->isUsed());
            JS_ASSERT(dn->isDefn());

            if (context->hasStrictOption() || dn->kind() == Definition::CONST) {
                JSAutoByteString name;
                ParseReportKind reporter = (dn->kind() != Definition::CONST)
                                           ? ParseStrictWarning
                                           : ParseError;
                if (!js_AtomToPrintableString(context, funName, &name) ||
                    !report(reporter, false, NULL, JSMSG_REDECLARED_VAR,
                            Definition::kindString(dn->kind()), name.ptr()))
                {
                    return false;
                }
            }

            







            if (bodyLevel && !makeDefIntoUse(dn, pn, funName))
                return false;
        } else if (bodyLevel) {
            




            if (Definition *fn = pc->lexdeps.lookupDefn(funName)) {
                JS_ASSERT(fn->isDefn());
                fn->setKind(PNK_FUNCTION);
                fn->setArity(PN_CODE);
                fn->pn_pos.begin = pn->pn_pos.begin;
                fn->pn_pos.end = pn->pn_pos.end;

                fn->pn_body = NULL;
                fn->pn_cookie.makeFree();

                pc->lexdeps->remove(funName);
                handler.freeTree(pn);
                pn = fn;
            }

            if (!pc->define(context, funName, pn, Definition::VAR))
                return false;
        }

        






        if (bodyLevel) {
            JS_ASSERT(pn->functionIsHoisted());
            JS_ASSERT_IF(pc->sc->isFunctionBox(), !pn->pn_cookie.isFree());
            JS_ASSERT_IF(!pc->sc->isFunctionBox(), pn->pn_cookie.isFree());
        } else {
            JS_ASSERT(!pc->sc->strict);
            JS_ASSERT(pn->pn_cookie.isFree());
            if (pc->sc->isFunctionBox()) {
                FunctionBox *funbox = pc->sc->asFunctionBox();
                funbox->setMightAliasLocals();
                funbox->setHasExtensibleScope();
            }
            pn->setOp(JSOP_DEFFUN);

            





            if (!pc->funcStmts) {
                pc->funcStmts = context->new_<FuncStmtSet>(context);
                if (!pc->funcStmts || !pc->funcStmts->init())
                    return false;
            }
            if (!pc->funcStmts->put(funName))
                return false;
        }

        
        pn->pn_dflags |= PND_BOUND;
    } else {
        
        pn->setOp(JSOP_LAMBDA);
    }

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkFunctionDefinition(HandlePropertyName funName,
                                                    Node *pn, FunctionSyntaxKind kind)
{
    return true;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionDef(HandlePropertyName funName, const TokenStream::Position &start,
                                  size_t startOffset, FunctionType type, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, funName);

    
    Node pn = handler.newFunctionDefinition();
    if (!pn)
        return null();

    if (!checkFunctionDefinition(funName, &pn, kind))
        return null();

    RootedFunction fun(context, newFunction(pc, funName, kind));
    if (!fun)
        return null();

    
    
    
    handler.setFunctionBody(pn, null());
    bool initiallyStrict = kind == Arrow || pc->sc->strict;
    bool becameStrict;
    if (!functionArgsAndBody(pn, fun, funName, startOffset, type, kind, initiallyStrict,
                             &becameStrict))
    {
        if (initiallyStrict || !becameStrict || tokenStream.hadError())
            return null();

        
        tokenStream.seek(start);
        if (funName && tokenStream.getToken() == TOK_ERROR)
            return null();
        handler.setFunctionBody(pn, null());
        if (!functionArgsAndBody(pn, fun, funName, startOffset, type, kind, true))
            return null();
    }

    return pn;
}

template <>
bool
Parser<FullParseHandler>::finishFunctionDefinition(ParseNode *pn, FunctionBox *funbox,
                                                   ParseNode *prelude, ParseNode *body,
                                                   ParseContext<FullParseHandler> *outerpc)
{
    pn->pn_pos.end = tokenStream.currentToken().pos.end;

    





    if (funbox->bindingsAccessedDynamically())
        outerpc->sc->setBindingsAccessedDynamically();
    if (funbox->hasDebuggerStatement())
        outerpc->sc->setHasDebuggerStatement();

#if JS_HAS_DESTRUCTURING
    





    if (prelude) {
        if (!body->isArity(PN_LIST)) {
            ParseNode *block;

            block = ListNode::create(PNK_SEQ, &handler);
            if (!block)
                return false;
            block->pn_pos = body->pn_pos;
            block->initList(body);

            body = block;
        }

        ParseNode *item = UnaryNode::create(PNK_SEMI, &handler);
        if (!item)
            return false;

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

    pn->pn_funbox = funbox;
    pn->pn_body->append(body);
    pn->pn_body->pn_pos = body->pn_pos;
    pn->pn_blockid = outerpc->blockid();

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::finishFunctionDefinition(Node pn, FunctionBox *funbox,
                                                     Node prelude, Node body,
                                                     ParseContext<SyntaxParseHandler> *outerpc)
{
    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::functionArgsAndBody(Node pn, HandleFunction fun, HandlePropertyName funName,
                                          size_t startOffset, FunctionType type,
                                          FunctionSyntaxKind kind, bool strict, bool *becameStrict)
{
    if (becameStrict)
        *becameStrict = false;
    ParseContext<ParseHandler> *outerpc = pc;

    
    FunctionBox *funbox = newFunctionBox(fun, pc, strict);
    if (!funbox)
        return false;

    
    ParseContext<ParseHandler> funpc(this, funbox, outerpc->staticLevel + 1, outerpc->blockidGen);
    if (!funpc.init())
        return false;

    
    Node prelude = null();
    bool hasRest;
    if (!functionArguments(kind, &prelude, pn, hasRest))
        return false;

    fun->setArgCount(funpc.numArgs());
    if (funbox->ndefaults)
        fun->setHasDefaults();
    if (hasRest)
        fun->setHasRest();

    if (type == Getter && fun->nargs > 0) {
        report(ParseError, false, null(), JSMSG_ACCESSOR_WRONG_ARGS, "getter", "no", "s");
        return false;
    }
    if (type == Setter && fun->nargs != 1) {
        report(ParseError, false, null(), JSMSG_ACCESSOR_WRONG_ARGS, "setter", "one", "");
        return false;
    }

    if (kind == Arrow && !tokenStream.matchToken(TOK_ARROW)) {
        report(ParseError, false, null(), JSMSG_BAD_ARROW_ARGS);
        return false;
    }

    
    mozilla::Maybe<GenexpGuard<ParseHandler> > yieldGuard;
    if (kind == Arrow)
        yieldGuard.construct(this);

    FunctionBodyType bodyType = StatementListBody;
    if (tokenStream.getToken(TSF_OPERAND) != TOK_LC) {
        tokenStream.ungetToken();
        bodyType = ExpressionBody;
        fun->setIsExprClosure();
    }

    Node body = functionBody(kind, bodyType);
    if (!body) {
        
        if (becameStrict && pc->funBecameStrict)
            *becameStrict = true;
        return false;
    }

    if (!yieldGuard.empty() && !yieldGuard.ref().checkValidBody(body, JSMSG_YIELD_IN_ARROW))
        return false;

    if (funName && !checkStrictBinding(funName, pn))
        return false;

#if JS_HAS_EXPR_CLOSURES
    if (bodyType == StatementListBody) {
#endif
        if (!tokenStream.matchToken(TOK_RC)) {
            report(ParseError, false, null(), JSMSG_CURLY_AFTER_BODY);
            return false;
        }
        funbox->bufEnd = tokenStream.currentToken().pos.begin + 1;
#if JS_HAS_EXPR_CLOSURES
    } else {
        if (tokenStream.hadError())
            return false;
        funbox->bufEnd = tokenStream.currentToken().pos.end;
        if (kind == Statement && !MatchOrInsertSemicolon(context, &tokenStream))
            return false;
    }
#endif

    if (!finishFunctionDefinition(pn, funbox, prelude, body, outerpc))
        return false;

    return leaveFunction(pn, funName, kind);
}

template <>
ParseNode *
Parser<FullParseHandler>::moduleDecl()
{
    JS_ASSERT(tokenStream.currentToken().name() == context->runtime->atomState.module);
    if (!((pc->sc->isGlobalSharedContext() || pc->sc->isModuleBox()) && pc->atBodyLevel()))
    {
        report(ParseError, false, NULL, JSMSG_MODULE_STATEMENT);
        return NULL;
    }

    ParseNode *pn = CodeNode::create(PNK_MODULE, &handler);
    if (!pn)
        return NULL;
    JS_ALWAYS_TRUE(tokenStream.matchToken(TOK_STRING));
    RootedAtom atom(context, tokenStream.currentToken().atom());
    Module *module = js_NewModule(context, atom);
    if (!module)
        return NULL;
    ModuleBox *modulebox = newModuleBox(module, pc);
    if (!modulebox)
        return NULL;
    pn->pn_modulebox = modulebox;

    ParseContext<FullParseHandler> modulepc(this, modulebox, pc->staticLevel + 1, pc->blockidGen);
    if (!modulepc.init())
        return NULL;
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_MODULE);
    pn->pn_body = statements();
    if (!pn->pn_body)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_MODULE);

    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::moduleDecl()
{
    setUnknownResult();
    return SyntaxParseHandler::NodeFailure;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionStmt()
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_FUNCTION);
    RootedPropertyName name(context);
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME) {
        name = tokenStream.currentToken().name();
    } else {
        
        report(ParseError, false, null(), JSMSG_UNNAMED_FUNCTION_STMT);
        return null();
    }

    TokenStream::Position start;
    tokenStream.positionAfterLastFunctionKeyword(start);

    
    if (!pc->atBodyLevel() && pc->sc->needStrictChecks() &&
        !report(ParseStrictError, pc->sc->strict, null(), JSMSG_STRICT_FUNCTION_STATEMENT))
        return null();

    return functionDef(name, start, tokenStream.positionToOffset(start), Normal, Statement);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionExpr()
{
    RootedPropertyName name(context);
    JS_ASSERT(tokenStream.currentToken().type == TOK_FUNCTION);
    TokenStream::Position start;
    tokenStream.positionAfterLastFunctionKeyword(start);
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME)
        name = tokenStream.currentToken().name();
    else
        tokenStream.ungetToken();
    return functionDef(name, start, tokenStream.positionToOffset(start), Normal, Expression);
}








static inline bool
IsEscapeFreeStringLiteral(const TokenPos &pos, JSAtom *str)
{
    




    return pos.begin + str->length() + 2 == pos.end;
}




















template <typename ParseHandler>
bool
Parser<ParseHandler>::maybeParseDirective(Node pn, bool *cont)
{
    TokenPos directivePos;
    JSAtom *directive = handler.isStringExprStatement(pn, &directivePos);

    *cont = !!directive;
    if (!*cont)
        return true;

    if (IsEscapeFreeStringLiteral(directivePos, directive)) {
        
        
        
        
        
        
        
        
        
        
        handler.setPrologue(pn);

        if (directive == context->runtime->atomState.useStrict) {
            
            
            pc->sc->setExplicitUseStrict();
            if (!pc->sc->strict) {
                if (pc->sc->isFunctionBox()) {
                    
                    pc->funBecameStrict = true;
                    return false;
                } else {
                    
                    
                    
                    if (tokenStream.sawOctalEscape()) {
                        report(ParseError, false, null(), JSMSG_DEPRECATED_OCTAL);
                        return false;
                    }
                    pc->sc->strict = true;
                }
            }
        } else if (directive == context->names().useAsm) {
            if (pc->sc->isFunctionBox()) {
                pc->sc->asFunctionBox()->useAsm = true;
                pc->sc->asFunctionBox()->asmStart = handler.getPosition(pn).begin;
            } else {
                if (!report(ParseWarning, false, pn, JSMSG_USE_ASM_DIRECTIVE_FAIL))
                    return false;
            }
        }
    }
    return true;
}

template <>
void
Parser<FullParseHandler>::addStatementToList(ParseNode *pn, ParseNode *kid, bool *hasFunctionStmt)
{
    JS_ASSERT(pn->isKind(PNK_STATEMENTLIST));

    if (kid->isKind(PNK_FUNCTION)) {
        








        if (pc->atBodyLevel()) {
            pn->pn_xflags |= PNX_FUNCDEFS;
        } else {
            



            JS_ASSERT_IF(pc->sc->isFunctionBox(), pc->sc->asFunctionBox()->hasExtensibleScope());
            if (hasFunctionStmt)
                *hasFunctionStmt = true;
        }
    }

    pn->append(kid);
    pn->pn_pos.end = kid->pn_pos.end;
}

template <>
void
Parser<SyntaxParseHandler>::addStatementToList(Node pn, Node kid, bool *hasFunctionStmt)
{
}






template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::statements(bool *hasFunctionStmt)
{
    JS_CHECK_RECURSION(context, return null());
    if (hasFunctionStmt)
        *hasFunctionStmt = false;

    Node pn = handler.newList(PNK_STATEMENTLIST);
    if (!pn)
        return null();
    handler.setBlockId(pn, pc->blockid());

    Node saveBlock = pc->blockNode;
    pc->blockNode = pn;

    bool canHaveDirectives = pc->atBodyLevel();
    for (;;) {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF || tt == TOK_RC) {
            if (tt == TOK_ERROR) {
                if (tokenStream.isEOF())
                    tokenStream.setUnexpectedEOF();
                return null();
            }
            break;
        }
        Node next = statement();
        if (!next) {
            if (tokenStream.isEOF())
                tokenStream.setUnexpectedEOF();
            return null();
        }

        if (canHaveDirectives) {
            if (!maybeParseDirective(next, &canHaveDirectives))
                return null();
        }

        addStatementToList(pn, next, hasFunctionStmt);
    }

    




    if (pc->blockNode != pn)
        pn = pc->blockNode;
    pc->blockNode = saveBlock;

    handler.setEndPosition(pn, tokenStream.currentToken().pos.end);
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::condition()
{
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_COND);
    Node pn = parenExpr();
    if (!pn)
        return null();
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    if (handler.isOperationWithoutParens(pn, PNK_ASSIGN) &&
        !report(ParseStrictWarning, false, null(), JSMSG_EQUAL_AS_ASSIGN))
    {
        return null();
    }
    return pn;
}

static bool
MatchLabel(JSContext *cx, TokenStream *ts, MutableHandlePropertyName label)
{
    TokenKind tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return false;
    if (tt == TOK_NAME) {
        (void) ts->getToken();
        label.set(ts->currentToken().name());
    } else {
        label.set(NULL);
    }
    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::reportRedeclaration(Node pn, bool isConst, JSAtom *atom)
{
    JSAutoByteString name;
    if (js_AtomToPrintableString(context, atom, &name))
        report(ParseError, false, pn, JSMSG_REDECLARED_VAR, isConst ? "const" : "variable", name.ptr());
    return false;
}










template <>
 bool
Parser<FullParseHandler>::bindLet(JSContext *cx, BindData<FullParseHandler> *data,
                                  HandlePropertyName name, Parser<FullParseHandler> *parser)
{
    ParseContext<FullParseHandler> *pc = parser->pc;
    ParseNode *pn = data->pn;
    if (!parser->checkStrictBinding(name, pn))
        return false;

    Rooted<StaticBlockObject *> blockObj(cx, data->let.blockObj);
    unsigned blockCount = blockObj->slotCount();
    if (blockCount == JS_BIT(16)) {
        parser->report(ParseError, false, pn, data->let.overflow);
        return false;
    }

    






    if (!pn->pn_cookie.set(parser->context, pc->staticLevel, uint16_t(blockCount)))
        return false;

    



    if (data->let.varContext == HoistVars) {
        JS_ASSERT(!pc->atBodyLevel());
        Definition *dn = pc->decls().lookupFirst(name);
        if (dn && dn->pn_blockid == pc->blockid())
            return parser->reportRedeclaration(pn, dn->isConst(), name);
        if (!pc->define(cx, name, pn, Definition::LET))
            return false;
    }

    



    bool redeclared;
    RootedId id(cx, NameToId(name));
    RootedShape shape(cx, StaticBlockObject::addVar(cx, blockObj, id, blockCount, &redeclared));
    if (!shape) {
        if (redeclared)
            parser->reportRedeclaration(pn, false, name);
        return false;
    }

    
    blockObj->setDefinitionParseNode(blockCount, reinterpret_cast<Definition *>(pn));
    return true;
}

template <>
 bool
Parser<SyntaxParseHandler>::bindLet(JSContext *cx, BindData<SyntaxParseHandler> *data,
                                    HandlePropertyName name, Parser<SyntaxParseHandler> *parser)
{
    return true;
}

template <typename ParseHandler, class Op>
static inline bool
ForEachLetDef(JSContext *cx, ParseContext<ParseHandler> *pc,
              HandleStaticBlockObject blockObj, Op op)
{
    for (Shape::Range<CanGC> r(cx, blockObj->lastProperty()); !r.empty(); r.popFront()) {
        Shape &shape = r.front();

        
        if (JSID_IS_INT(shape.propid()))
            continue;

        if (!op(cx, pc, blockObj, shape, JSID_TO_ATOM(shape.propid())))
            return false;
    }
    return true;
}

template <typename ParseHandler>
struct PopLetDecl {
    bool operator()(JSContext *, ParseContext<ParseHandler> *pc, HandleStaticBlockObject,
                    const Shape &, JSAtom *atom)
    {
        pc->popLetDecl(atom);
        return true;
    }
};

template <typename ParseHandler>
static void
PopStatementPC(JSContext *cx, ParseContext<ParseHandler> *pc)
{
    RootedStaticBlockObject blockObj(cx, pc->topStmt->blockObj);
    JS_ASSERT(!!blockObj == (pc->topStmt->isBlockScope));

    FinishPopStatement(pc);

    if (blockObj) {
        JS_ASSERT(!blockObj->inDictionaryMode());
        ForEachLetDef(cx, pc, blockObj, PopLetDecl<ParseHandler>());
        blockObj->resetPrevBlockChainFromParser();
    }
}

template <typename ParseHandler>
static inline bool
OuterLet(ParseContext<ParseHandler> *pc, StmtInfoPC *stmt, HandleAtom atom)
{
    while (stmt->downScope) {
        stmt = LexicalLookup(pc, atom, NULL, stmt->downScope);
        if (!stmt)
            return false;
        if (stmt->type == STMT_BLOCK)
            return true;
    }
    return false;
}

template <>
 bool
Parser<FullParseHandler>::bindVarOrConst(JSContext *cx, BindData<FullParseHandler> *data,
                                         HandlePropertyName name, Parser<FullParseHandler> *parser)
{
    ParseContext<FullParseHandler> *pc = parser->pc;
    ParseNode *pn = data->pn;
    bool isConstDecl = data->op == JSOP_DEFCONST;

    
    pn->setOp(JSOP_NAME);

    if (!parser->checkStrictBinding(name, pn))
        return false;

    StmtInfoPC *stmt = LexicalLookup(pc, name, NULL, (StmtInfoPC *)NULL);

    if (stmt && stmt->type == STMT_WITH) {
        pn->pn_dflags |= PND_DEOPTIMIZED;
        if (pc->sc->isFunctionBox())
            pc->sc->asFunctionBox()->setMightAliasLocals();
        return true;
    }

    DefinitionList::Range defs = pc->decls().lookupMulti(name);
    JS_ASSERT_IF(stmt, !defs.empty());

    if (defs.empty())
        return pc->define(cx, name, pn, isConstDecl ? Definition::CONST : Definition::VAR);

    






    Definition *dn = defs.front();
    Definition::Kind dn_kind = dn->kind();
    if (dn_kind == Definition::ARG) {
        JSAutoByteString bytes;
        if (!js_AtomToPrintableString(cx, name, &bytes))
            return false;

        if (isConstDecl) {
            parser->report(ParseError, false, pn, JSMSG_REDECLARED_PARAM, bytes.ptr());
            return false;
        }
        if (!parser->report(ParseStrictWarning, false, pn, JSMSG_VAR_HIDES_ARG, bytes.ptr()))
            return false;
    } else {
        bool error = (isConstDecl ||
                      dn_kind == Definition::CONST ||
                      (dn_kind == Definition::LET &&
                       (stmt->type != STMT_CATCH || OuterLet(pc, stmt, name))));

        if (cx->hasStrictOption()
            ? data->op != JSOP_DEFVAR || dn_kind != Definition::VAR
            : error)
        {
            JSAutoByteString bytes;
            ParseReportKind reporter = error ? ParseError : ParseStrictWarning;
            if (!js_AtomToPrintableString(cx, name, &bytes) ||
                !parser->report(reporter, false, pn, JSMSG_REDECLARED_VAR,
                                Definition::kindString(dn_kind), bytes.ptr()))
            {
                return false;
            }
        }
    }

    LinkUseToDef(pn, dn);
    return true;
}

template <>
 bool
Parser<SyntaxParseHandler>::bindVarOrConst(JSContext *cx, BindData<SyntaxParseHandler> *data,
                                           HandlePropertyName name,
                                           Parser<SyntaxParseHandler> *parser)
{
    return true;
}

template <>
bool
Parser<FullParseHandler>::makeSetCall(ParseNode *pn, unsigned msg)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isOp(JSOP_CALL) || pn->isOp(JSOP_EVAL) ||
              pn->isOp(JSOP_FUNCALL) || pn->isOp(JSOP_FUNAPPLY));
    if (!report(ParseStrictError, pc->sc->strict, pn, msg))
        return false;

    ParseNode *pn2 = pn->pn_head;
    if (pn2->isKind(PNK_FUNCTION) && (pn2->pn_funbox->inGenexpLambda)) {
        report(ParseError, false, pn, msg);
        return false;
    }
    pn->pn_xflags |= PNX_SETCALL;
    return true;
}

template <>
bool
Parser<FullParseHandler>::noteNameUse(ParseNode *pn)
{
    RootedPropertyName name(context, pn->pn_atom->asPropertyName());
    StmtInfoPC *stmt = LexicalLookup(pc, name, NULL, (StmtInfoPC *)NULL);

    DefinitionList::Range defs = pc->decls().lookupMulti(name);

    Definition *dn;
    if (!defs.empty()) {
        dn = defs.front();
    } else {
        if (AtomDefnAddPtr p = pc->lexdeps->lookupForAdd(name)) {
            dn = p.value();
        } else {
            







            dn = MakePlaceholder(pn, &handler, pc);
            if (!dn || !pc->lexdeps->add(p, name, dn))
                return false;
        }
    }

    JS_ASSERT(dn->isDefn());
    LinkUseToDef(pn, dn);

    if (stmt && stmt->type == STMT_WITH)
        pn->pn_dflags |= PND_DEOPTIMIZED;

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::noteNameUse(Node pn)
{
    return true;
}

#if JS_HAS_DESTRUCTURING

template <>
bool
Parser<FullParseHandler>::bindDestructuringVar(BindData<FullParseHandler> *data, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_NAME));

    RootedPropertyName name(context, pn->pn_atom->asPropertyName());

    data->pn = pn;
    if (!data->binder(context, data, name, this))
        return false;

    



    if (pn->pn_dflags & PND_BOUND)
        pn->setOp(JSOP_SETLOCAL);
    else if (data->op == JSOP_DEFCONST)
        pn->setOp(JSOP_SETCONST);
    else
        pn->setOp(JSOP_SETNAME);

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    handler.noteLValue(pn);
    return true;
}



















template <>
bool
Parser<FullParseHandler>::bindDestructuringLHS(ParseNode *pn)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        handler.noteLValue(pn);
        

      case PNK_DOT:
      case PNK_ELEM:
        




        if (!(js_CodeSpec[pn->getOp()].format & JOF_SET))
            pn->setOp(JSOP_SETNAME);
        break;

      case PNK_CALL:
        if (!makeSetCall(pn, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;

      default:
        report(ParseError, false, pn, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return false;
    }

    return true;
}








































template <>
bool
Parser<FullParseHandler>::checkDestructuring(BindData<FullParseHandler> *data,
                                             ParseNode *left, bool toplevel)
{
    bool ok;

    if (left->isKind(PNK_ARRAYCOMP)) {
        report(ParseError, false, left, JSMSG_ARRAY_COMP_LEFTSIDE);
        return false;
    }

    Rooted<StaticBlockObject *> blockObj(context);
    blockObj = data && data->binder == bindLet ? data->let.blockObj.get() : NULL;
    uint32_t blockCountBefore = blockObj ? blockObj->slotCount() : 0;

    if (left->isKind(PNK_ARRAY)) {
        for (ParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (!pn->isArrayHole()) {
                if (pn->isKind(PNK_ARRAY) || pn->isKind(PNK_OBJECT)) {
                    ok = checkDestructuring(data, pn, false);
                } else {
                    if (data) {
                        if (!pn->isKind(PNK_NAME)) {
                            report(ParseError, false, pn, JSMSG_NO_VARIABLE_NAME);
                            return false;
                        }
                        ok = bindDestructuringVar(data, pn);
                    } else {
                        ok = bindDestructuringLHS(pn);
                    }
                }
                if (!ok)
                    return false;
            }
        }
    } else {
        JS_ASSERT(left->isKind(PNK_OBJECT));
        for (ParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->isKind(PNK_COLON));
            ParseNode *pn = pair->pn_right;

            if (pn->isKind(PNK_ARRAY) || pn->isKind(PNK_OBJECT)) {
                ok = checkDestructuring(data, pn, false);
            } else if (data) {
                if (!pn->isKind(PNK_NAME)) {
                    report(ParseError, false, pn, JSMSG_NO_VARIABLE_NAME);
                    return false;
                }
                ok = bindDestructuringVar(data, pn);
            } else {
                






                if (pair->pn_right == pair->pn_left && !noteNameUse(pn))
                    return false;
                ok = bindDestructuringLHS(pn);
            }
            if (!ok)
                return false;
        }
    }

    






















    if (toplevel && blockObj && blockCountBefore == blockObj->slotCount()) {
        bool redeclared;
        RootedId id(context, INT_TO_JSID(blockCountBefore));
        if (!StaticBlockObject::addVar(context, blockObj, id, blockCountBefore, &redeclared))
            return false;
        JS_ASSERT(!redeclared);
        JS_ASSERT(blockObj->slotCount() == blockCountBefore + 1);
    }

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkDestructuring(BindData<SyntaxParseHandler> *data,
                                               Node left, bool toplevel)
{
    setUnknownResult();
    return false;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::destructuringExpr(BindData<ParseHandler> *data, TokenKind tt)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(tt));

    pc->inDeclDestructuring = true;
    Node pn = primaryExpr(tt);
    pc->inDeclDestructuring = false;
    if (!pn)
        return null();
    if (!checkDestructuring(data, pn))
        return null();
    return pn;
}

#endif 

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::returnOrYield(bool useAssignExpr)
{
    TokenKind tt = tokenStream.currentToken().type;
    if (!pc->sc->isFunctionBox()) {
        report(ParseError, false, null(), JSMSG_BAD_RETURN_OR_YIELD,
               (tt == TOK_RETURN) ? js_return_str : js_yield_str);
        return null();
    }

    ParseNodeKind kind = (tt == TOK_RETURN) ? PNK_RETURN : PNK_YIELD;
    JSOp op = (tt == TOK_RETURN) ? JSOP_RETURN : JSOP_YIELD;

    Node pn = handler.newUnary(kind, op);
    if (!pn)
        return null();

#if JS_HAS_GENERATORS
    if (tt == TOK_YIELD) {
        



        if (pc->parenDepth == 0) {
            pc->sc->asFunctionBox()->setIsGenerator();
        } else {
            pc->yieldCount++;
            pc->yieldNode = pn;
        }
    }
#endif

    
    TokenKind tt2 = tokenStream.peekTokenSameLine(TSF_OPERAND);
    if (tt2 == TOK_ERROR)
        return null();

    if (tt2 != TOK_EOF && tt2 != TOK_EOL && tt2 != TOK_SEMI && tt2 != TOK_RC
#if JS_HAS_GENERATORS
        && (tt != TOK_YIELD ||
            (tt2 != tt && tt2 != TOK_RB && tt2 != TOK_RP &&
             tt2 != TOK_COLON && tt2 != TOK_COMMA))
#endif
        )
    {
        Node pn2 = useAssignExpr ? assignExpr() : expr();
        if (!pn2)
            return null();
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            pc->funHasReturnExpr = true;
        handler.setUnaryKid(pn, pn2);
    } else {
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            pc->funHasReturnVoid = true;
    }

    if (pc->funHasReturnExpr && pc->sc->asFunctionBox()->isGenerator()) {
        
        reportBadReturn(pn, ParseError, JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return null();
    }

    if (context->hasStrictOption() && pc->funHasReturnExpr && pc->funHasReturnVoid &&
        !reportBadReturn(pn, ParseStrictWarning,
                         JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE))
    {
        return null();
    }

    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::pushLexicalScope(HandleStaticBlockObject blockObj, StmtInfoPC *stmt)
{
    JS_ASSERT(blockObj);

    ObjectBox *blockbox = newObjectBox(blockObj);
    if (!blockbox)
        return null();

    PushStatementPC(pc, stmt, STMT_BLOCK);
    blockObj->initPrevBlockChainFromParser(pc->blockChain);
    FinishPushBlockScope(pc, stmt, *blockObj.get());

    Node pn = handler.newLexicalScope(blockbox);
    if (!pn)
        return null();

    if (!GenerateBlockId(pc, stmt->blockid))
        return null();
    handler.setBlockId(pn, stmt->blockid);
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::pushLexicalScope(StmtInfoPC *stmt)
{
    RootedStaticBlockObject blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return null();

    return pushLexicalScope(blockObj, stmt);
}

#if JS_HAS_BLOCK_SCOPE

struct AddLetDecl
{
    uint32_t blockid;

    AddLetDecl(uint32_t blockid) : blockid(blockid) {}

    bool operator()(JSContext *cx, ParseContext<FullParseHandler> *pc,
                    HandleStaticBlockObject blockObj, const Shape &shape, JSAtom *)
    {
        ParseNode *def = (ParseNode *) blockObj->getSlot(shape.slot()).toPrivate();
        def->pn_blockid = blockid;
        RootedPropertyName name(cx, def->name());
        return pc->define(cx, name, def, Definition::LET);
    }
};

template <>
ParseNode *
Parser<FullParseHandler>::pushLetScope(HandleStaticBlockObject blockObj, StmtInfoPC *stmt)
{
    JS_ASSERT(blockObj);
    ParseNode *pn = pushLexicalScope(blockObj, stmt);
    if (!pn)
        return null();

    
    pn->pn_dflags |= PND_LET;

    
    if (!ForEachLetDef(context, pc, blockObj, AddLetDecl(stmt->blockid)))
        return null();

    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::pushLetScope(HandleStaticBlockObject blockObj, StmtInfoPC *stmt)
{
    setUnknownResult();
    return SyntaxParseHandler::NodeFailure;
}






template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::letBlock(LetContext letContext)
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_LET);

    RootedStaticBlockObject blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return null();

    uint32_t begin = tokenStream.currentToken().pos.begin;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    Node vars = variables(PNK_LET, NULL, blockObj, DontHoistVars);
    if (!vars)
        return null();

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    StmtInfoPC stmtInfo(context);
    Node block = pushLetScope(blockObj, &stmtInfo);
    if (!block)
        return null();

    Node pnlet = handler.newBinary(PNK_LET, vars, block);
    if (!pnlet)
        return null();
    handler.setBeginPosition(pnlet, begin);

    Node ret;
    if (letContext == LetStatement && !tokenStream.matchToken(TOK_LC, TSF_OPERAND)) {
        







        if (!report(ParseStrictError, pc->sc->strict, pnlet,
                    JSMSG_STRICT_CODE_LET_EXPR_STMT))
        {
            return null();
        }

        




        Node semi = handler.newUnary(PNK_SEMI, pnlet);

        letContext = LetExpresion;
        ret = semi;
    } else {
        ret = pnlet;
    }

    Node expr;
    if (letContext == LetStatement) {
        expr = statements();
        if (!expr)
            return null();
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_LET);
    } else {
        JS_ASSERT(letContext == LetExpresion);
        expr = assignExpr();
        if (!expr)
            return null();
    }
    handler.setLeaveBlockResult(block, expr, letContext != LetStatement);

    handler.setBeginPosition(ret, vars);
    handler.setEndPosition(ret, expr);

    handler.setBeginPosition(pnlet, vars);
    handler.setEndPosition(pnlet, expr);

    PopStatementPC(context, pc);
    return ret;
}

#endif 

template <typename ParseHandler>
static bool
PushBlocklikeStatement(StmtInfoPC *stmt, StmtType type, ParseContext<ParseHandler> *pc)
{
    PushStatementPC(pc, stmt, type);
    return GenerateBlockId(pc, stmt->blockid);
}

template <>
ParseNode *
Parser<FullParseHandler>::newBindingNode(PropertyName *name, VarContext varContext)
{
    






    if (varContext == HoistVars) {
        if (AtomDefnPtr p = pc->lexdeps->lookup(name)) {
            ParseNode *lexdep = p.value();
            JS_ASSERT(lexdep->isPlaceholder());
            if (lexdep->pn_blockid >= pc->blockid()) {
                lexdep->pn_blockid = pc->blockid();
                pc->lexdeps->remove(p);
                lexdep->pn_pos = tokenStream.currentToken().pos;
                return lexdep;
            }
        }
    }

    
    JS_ASSERT(tokenStream.currentToken().type == TOK_NAME);
    return handler.newName(name, pc);
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::newBindingNode(PropertyName *name, VarContext varContext)
{
    return SyntaxParseHandler::NodeGeneric;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::switchStatement()
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_SWITCH);

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

    Node discriminant = parenExpr();
    if (!discriminant)
        return null();

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_SWITCH);

    if (!GenerateBlockId(pc, pc->topStmt->blockid))
        return null();

    
    Node caseList = handler.newList(PNK_STATEMENTLIST);
    if (!caseList)
        return null();
    handler.setBlockId(caseList, pc->blockid());

    Node saveBlock = pc->blockNode;
    pc->blockNode = caseList;

    bool seenDefault = false;
    TokenKind tt;
    while ((tt = tokenStream.getToken()) != TOK_RC) {
        Node casepn;
        switch (tt) {
          case TOK_DEFAULT:
            if (seenDefault) {
                report(ParseError, false, null(), JSMSG_TOO_MANY_DEFAULTS);
                return null();
            }
            seenDefault = true;
            casepn = handler.newBinary(PNK_DEFAULT);
            if (!casepn)
                return null();
            break;

          case TOK_CASE:
          {
            Node left = expr();
            if (!left)
                return null();
            casepn = handler.newBinary(PNK_CASE, left);
            if (!casepn)
                return null();
            break;
          }

          case TOK_ERROR:
            return null();

          default:
            report(ParseError, false, null(), JSMSG_BAD_SWITCH);
            return null();
        }

        handler.addList(caseList, casepn);

        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        Node body = handler.newList(PNK_STATEMENTLIST);
        if (!body)
            return null();
        handler.setBlockId(body, pc->blockid());

        while ((tt = tokenStream.peekToken(TSF_OPERAND)) != TOK_RC &&
               tt != TOK_CASE && tt != TOK_DEFAULT) {
            if (tt == TOK_ERROR)
                return null();
            Node stmt = statement();
            if (!stmt)
                return null();
            handler.addList(body, stmt);
        }

        handler.setBinaryRHS(casepn, body);
    }

    





    if (pc->blockNode != caseList)
        caseList = pc->blockNode;
    pc->blockNode = saveBlock;

    PopStatementPC(context, pc);

    Node pn = handler.newBinary(PNK_SWITCH, discriminant, caseList);
    if (!pn)
        return null();

    handler.setEndPosition(pn, tokenStream.currentToken().pos.end);
    handler.setEndPosition(caseList, tokenStream.currentToken().pos.end);
    return pn;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::matchInOrOf(bool *isForOfp)
{
    if (tokenStream.matchToken(TOK_IN)) {
        *isForOfp = false;
        return true;
    }
    if (tokenStream.matchToken(TOK_NAME)) {
        if (tokenStream.currentToken().name() == context->names().of) {
            *isForOfp = true;
            return true;
        }
        tokenStream.ungetToken();
    }
    return false;
}

template <>
bool
Parser<FullParseHandler>::isValidForStatementLHS(ParseNode *pn1, JSVersion version,
                                                 bool forDecl, bool forEach, bool forOf)
{
    if (forDecl) {
        if (pn1->pn_count > 1)
            return false;
        if (pn1->isOp(JSOP_DEFCONST))
            return false;
#if JS_HAS_DESTRUCTURING
        
        
        if (version == JSVERSION_1_7 && !forEach && !forOf) {
            ParseNode *lhs = pn1->pn_head;
            if (lhs->isKind(PNK_ASSIGN))
                lhs = lhs->pn_left;

            if (lhs->isKind(PNK_OBJECT))
                return false;
            if (lhs->isKind(PNK_ARRAY) && lhs->pn_count != 2)
                return false;
        }
#endif
        return true;
    }

    switch (pn1->getKind()) {
      case PNK_NAME:
      case PNK_DOT:
      case PNK_CALL:
      case PNK_ELEM:
        return true;

#if JS_HAS_DESTRUCTURING
      case PNK_ARRAY:
      case PNK_OBJECT:
        
        
        if (version == JSVERSION_1_7 && !forEach && !forOf)
            return pn1->isKind(PNK_ARRAY) && pn1->pn_count == 2;
        return true;
#endif

      default:
        return false;
    }
}

template <>
ParseNode *
Parser<FullParseHandler>::forStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    StmtInfoPC forStmt(context);
    PushStatementPC(pc, &forStmt, STMT_FOR_LOOP);

    
    ParseNode *pn = BinaryNode::create(PNK_FOR, &handler);
    if (!pn)
        return null();

    pn->setOp(JSOP_ITER);
    pn->pn_iflags = 0;

    if (allowsForEachIn() && tokenStream.matchToken(TOK_NAME)) {
        if (tokenStream.currentToken().name() == context->names().each)
            pn->pn_iflags = JSITER_FOREACH;
        else
            tokenStream.ungetToken();
    }

    TokenPos lp_pos = tokenStream.currentToken().pos;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    



    bool forDecl = false;

    
    RootedStaticBlockObject blockObj(context);

    
    ParseNode *pn1;

    {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            if (pn->pn_iflags & JSITER_FOREACH) {
                report(ParseError, false, null(), JSMSG_BAD_FOR_EACH_LOOP);
                return null();
            }

            pn1 = NULL;
        } else {
            












            pc->parsingForInit = true;
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
                        return null();
                    pn1 = variables(PNK_LET, NULL, blockObj, DontHoistVars);
                }
            }
#endif
            else {
                pn1 = expr();
            }
            pc->parsingForInit = false;
            if (!pn1)
                return null();
        }
    }

    JS_ASSERT_IF(forDecl, pn1->isArity(PN_LIST));
    JS_ASSERT(!!blockObj == (forDecl && pn1->isOp(JSOP_NOP)));

    const TokenPos pos = tokenStream.currentToken().pos;

    
    ParseNode *forParent = NULL;

    





    ParseNode *forHead;        
    StmtInfoPC letStmt(context); 
    ParseNode *pn2, *pn3;      
    bool forOf;
    if (pn1 && matchInOrOf(&forOf)) {
        







        forStmt.type = STMT_FOR_IN_LOOP;

        
        if (forOf && pn->pn_iflags != 0) {
            JS_ASSERT(pn->pn_iflags == JSITER_FOREACH);
            report(ParseError, false, null(), JSMSG_BAD_FOR_EACH_LOOP);
            return null();
        }
        pn->pn_iflags |= (forOf ? JSITER_FOR_OF : JSITER_ENUMERATE);

        
        bool forEach = bool(pn->pn_iflags & JSITER_FOREACH);
        if (!isValidForStatementLHS(pn1, versionNumber(), forDecl, forEach, forOf)) {
            report(ParseError, false, pn1, JSMSG_BAD_FOR_LEFTSIDE);
            return null();
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
                    report(ParseError, false, pn2, JSMSG_INVALID_FOR_IN_INIT);
                    return null();
                }
#endif 

                ParseNode *pnseq = handler.newList(PNK_SEQ, pn1);
                if (!pnseq)
                    return null();

                







                pn1->pn_xflags &= ~PNX_FORINVAR;
                pn1->pn_xflags |= PNX_POPVAR;
                pn1 = NULL;

#if JS_HAS_DESTRUCTURING
                if (pn2->isKind(PNK_ASSIGN)) {
                    pn2 = pn2->pn_left;
                    JS_ASSERT(pn2->isKind(PNK_ARRAY) || pn2->isKind(PNK_OBJECT) ||
                              pn2->isKind(PNK_NAME));
                }
#endif
                pnseq->pn_pos.begin = pn->pn_pos.begin;
                pnseq->append(pn);
                forParent = pnseq;
            }
        } else {
            
            JS_ASSERT(!blockObj);
            pn2 = pn1;
            pn1 = NULL;

            if (!setAssignmentLhsOps(pn2, JSOP_NOP))
                return null();
        }

        pn3 = expr();
        if (!pn3)
            return null();

        if (blockObj) {
            





            ParseNode *block = pushLetScope(blockObj, &letStmt);
            if (!block)
                return null();
            letStmt.isForLetBlock = true;
            block->pn_expr = pn1;
            block->pn_pos = pn1->pn_pos;
            pn1 = block;
        }

        if (forDecl) {
            



            pn2 = cloneLeftHandSide(pn2);
            if (!pn2)
                return null();
        }

        switch (pn2->getKind()) {
          case PNK_NAME:
            
            handler.noteLValue(pn2);
            break;

#if JS_HAS_DESTRUCTURING
          case PNK_ASSIGN:
            JS_NOT_REACHED("forStatement TOK_ASSIGN");
            break;

          case PNK_ARRAY:
          case PNK_OBJECT:
            if (versionNumber() == JSVERSION_1_7) {
                



                JS_ASSERT(pn->isOp(JSOP_ITER));
                if (!(pn->pn_iflags & JSITER_FOREACH) && !forOf)
                    pn->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          default:;
        }

        forHead = TernaryNode::create(PNK_FORIN, &handler);
        if (!forHead)
            return null();
    } else {
        if (blockObj) {
            



            ParseNode *block = pushLetScope(blockObj, &letStmt);
            if (!block)
                return null();
            letStmt.isForLetBlock = true;

            ParseNode *let = handler.newBinary(PNK_LET, pn1, block);
            if (!let)
                return null();

            pn1 = NULL;
            block->pn_expr = pn;
            forParent = let;
        }

        if (pn->pn_iflags & JSITER_FOREACH) {
            report(ParseError, false, pn, JSMSG_BAD_FOR_EACH_LOOP);
            return null();
        }
        pn->setOp(JSOP_NOP);

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        if (tokenStream.peekToken(TSF_OPERAND) == TOK_SEMI) {
            pn2 = NULL;
        } else {
            pn2 = expr();
            if (!pn2)
                return null();
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        if (tokenStream.peekToken(TSF_OPERAND) == TOK_RP) {
            pn3 = NULL;
        } else {
            pn3 = expr();
            if (!pn3)
                return null();
        }

        forHead = TernaryNode::create(PNK_FORHEAD, &handler);
        if (!forHead)
            return null();
    }

    forHead->pn_pos = pos;
    forHead->setOp(JSOP_NOP);
    forHead->pn_kid1 = pn1;
    forHead->pn_kid2 = pn2;
    forHead->pn_kid3 = pn3;
    forHead->pn_pos.begin = lp_pos.begin;
    forHead->pn_pos.end   = tokenStream.currentToken().pos.end;
    pn->pn_left = forHead;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    
    ParseNode *body = statement();
    if (!body)
        return null();

    
    pn->pn_pos.end = body->pn_pos.end;
    pn->pn_right = body;

    if (forParent) {
        forParent->pn_pos.begin = pn->pn_pos.begin;
        forParent->pn_pos.end = pn->pn_pos.end;
    }

#if JS_HAS_BLOCK_SCOPE
    if (blockObj)
        PopStatementPC(context, pc);
#endif
    PopStatementPC(context, pc);
    return forParent ? forParent : pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::forStatement()
{
    





    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    StmtInfoPC forStmt(context);
    PushStatementPC(pc, &forStmt, STMT_FOR_LOOP);

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    
    bool forDecl = false;
    bool simpleForDecl = true;

    
    Node lhsNode;

    {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            lhsNode = null();
        } else {
            
            pc->parsingForInit = true;
            if (tt == TOK_VAR) {
                forDecl = true;
                tokenStream.consumeKnownToken(tt);
                lhsNode = variables(tt == TOK_VAR ? PNK_VAR : PNK_CONST, &simpleForDecl);
            }
#if JS_HAS_BLOCK_SCOPE
            else if (tt == TOK_CONST || tt == TOK_LET) {
                setUnknownResult();
                return null();
            }
#endif
            else {
                lhsNode = expr();
            }
            if (!lhsNode)
                return null();
            pc->parsingForInit = false;
        }
    }

    





    bool forOf;
    if (lhsNode && matchInOrOf(&forOf)) {
        
        forStmt.type = STMT_FOR_IN_LOOP;

        
        if (!forDecl &&
            lhsNode != SyntaxParseHandler::NodeName &&
            lhsNode != SyntaxParseHandler::NodeLValue)
        {
            setUnknownResult();
            return null();
        }

        if (!simpleForDecl) {
            setUnknownResult();
            return null();
        }

        if (!forDecl && !setAssignmentLhsOps(lhsNode, JSOP_NOP))
            return null();

        if (!expr())
            return null();
    } else {
        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        if (tokenStream.peekToken(TSF_OPERAND) != TOK_SEMI) {
            if (!expr())
                return null();
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        if (tokenStream.peekToken(TSF_OPERAND) != TOK_RP) {
            if (!expr())
                return null();
        }
    }

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    
    if (!statement())
        return null();

    PopStatementPC(context, pc);
    return SyntaxParseHandler::NodeGeneric;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::tryStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_TRY));
    uint32_t begin = tokenStream.currentToken().pos.begin;

    

















    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    StmtInfoPC stmtInfo(context);
    if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, pc))
        return null();
    Node innerBlock = statements();
    if (!innerBlock)
        return null();
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatementPC(context, pc);

    bool hasUnconditionalCatch = false;
    Node catchList = null();
    TokenKind tt = tokenStream.getToken();
    if (tt == TOK_CATCH) {
        catchList = handler.newList(PNK_CATCH);
        if (!catchList)
            return null();

        do {
            Node pnblock;
            BindData<ParseHandler> data(context);

            
            if (hasUnconditionalCatch) {
                report(ParseError, false, null(), JSMSG_CATCH_AFTER_GENERAL);
                return null();
            }

            



            pnblock = pushLexicalScope(&stmtInfo);
            if (!pnblock)
                return null();
            stmtInfo.type = STMT_CATCH;

            






            MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_CATCH);

            




            data.initLet(HoistVars, *pc->blockChain, JSMSG_TOO_MANY_CATCH_VARS);
            JS_ASSERT(data.let.blockObj);

            tt = tokenStream.getToken();
            Node catchName;
            switch (tt) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
                catchName = destructuringExpr(&data, tt);
                if (!catchName)
                    return null();
                break;
#endif

              case TOK_NAME:
              {
                RootedPropertyName label(context, tokenStream.currentToken().name());
                catchName = newBindingNode(label);
                if (!catchName)
                    return null();
                data.pn = catchName;
                if (!data.binder(context, &data, label, this))
                    return null();
                break;
              }

              default:
                report(ParseError, false, null(), JSMSG_CATCH_IDENTIFIER);
                return null();
            }

            Node catchGuard = null();
#if JS_HAS_CATCH_GUARD
            




            if (tokenStream.matchToken(TOK_IF)) {
                catchGuard = expr();
                if (!catchGuard)
                    return null();
            }
#endif
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_CATCH);

            MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_CATCH);
            Node catchBody = statements();
            if (!catchBody)
                return null();
            MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_CATCH);
            PopStatementPC(context, pc);

            if (!catchGuard)
                hasUnconditionalCatch = true;

            if (!handler.addCatchBlock(catchList, pnblock, catchName, catchGuard, catchBody))
                return null();
            handler.setEndPosition(catchList, tokenStream.currentToken().pos.end);
            handler.setEndPosition(pnblock, tokenStream.currentToken().pos.end);

            tt = tokenStream.getToken(TSF_OPERAND);
        } while (tt == TOK_CATCH);
    }

    Node finallyBlock = null();

    if (tt == TOK_FINALLY) {
        MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_FINALLY);
        if (!PushBlocklikeStatement(&stmtInfo, STMT_FINALLY, pc))
            return null();
        finallyBlock = statements();
        if (!finallyBlock)
            return null();
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_FINALLY);
        PopStatementPC(context, pc);
    } else {
        tokenStream.ungetToken();
    }
    if (!catchList && !finallyBlock) {
        report(ParseError, false, null(), JSMSG_CATCH_OR_FINALLY);
        return null();
    }

    Node pn = handler.newTernary(PNK_TRY, innerBlock, catchList, finallyBlock);
    if (!pn)
        return null();

    handler.setBeginPosition(pn, begin);
    handler.setEndPosition(pn, finallyBlock ? finallyBlock : catchList);
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::withStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_WITH));
    uint32_t begin = tokenStream.currentToken().pos.begin;

    
    
    
    
    
    
    if (pc->sc->strict && !report(ParseStrictError, true, null(), JSMSG_STRICT_CODE_WITH))
        return null();

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_WITH);
    Node objectExpr = parenExpr();
    if (!objectExpr)
        return null();
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_WITH);

    bool oldParsingWith = pc->parsingWith;
    pc->parsingWith = true;

    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_WITH);
    Node innerBlock = statement();
    if (!innerBlock)
        return null();
    PopStatementPC(context, pc);

    pc->sc->setBindingsAccessedDynamically();
    pc->parsingWith = oldParsingWith;

    



    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
        Definition *defn = r.front().value();
        Definition *lexdep = defn->resolve();
        DeoptimizeUsesWithin(lexdep, TokenPos::make(begin, tokenStream.currentToken().pos.begin));
    }

    Node pn = handler.newBinary(PNK_WITH, objectExpr, innerBlock);
    if (!pn)
        return null();

    handler.setBeginPosition(pn, begin);
    handler.setEndPosition(pn, innerBlock);
    return pn;
}

#if JS_HAS_BLOCK_SCOPE
template <>
ParseNode *
Parser<FullParseHandler>::letStatement()
{
    ParseNode *pn;
    do {
        
        if (tokenStream.peekToken() == TOK_LP) {
            pn = letBlock(LetStatement);
            if (!pn)
                return null();

            JS_ASSERT(pn->isKind(PNK_LET) || pn->isKind(PNK_SEMI));
            if (pn->isKind(PNK_LET) && pn->pn_expr->getOp() == JSOP_LEAVEBLOCK)
                return pn;

            
            JS_ASSERT(pn->isKind(PNK_SEMI) || pn->isOp(JSOP_NOP));
            break;
        }

        










        StmtInfoPC *stmt = pc->topStmt;
        if (stmt && (!stmt->maybeScope() || stmt->isForLetBlock)) {
            report(ParseError, false, null(), JSMSG_LET_DECL_NOT_IN_BLOCK);
            return null();
        }

        if (stmt && stmt->isBlockScope) {
            JS_ASSERT(pc->blockChain == stmt->blockObj);
        } else {
            if (pc->atBodyLevel()) {
                



                pn = variables(PNK_VAR);
                if (!pn)
                    return null();
                pn->pn_xflags |= PNX_POPVAR;
                break;
            }

            




            JS_ASSERT(!stmt->isBlockScope);
            JS_ASSERT(stmt != pc->topScopeStmt);
            JS_ASSERT(stmt->type == STMT_BLOCK ||
                      stmt->type == STMT_SWITCH ||
                      stmt->type == STMT_TRY ||
                      stmt->type == STMT_FINALLY);
            JS_ASSERT(!stmt->downScope);

            
            StaticBlockObject *blockObj = StaticBlockObject::create(context);
            if (!blockObj)
                return null();

            ObjectBox *blockbox = newObjectBox(blockObj);
            if (!blockbox)
                return null();

            





            stmt->isBlockScope = true;
            stmt->downScope = pc->topScopeStmt;
            pc->topScopeStmt = stmt;

            blockObj->initPrevBlockChainFromParser(pc->blockChain);
            pc->blockChain = blockObj;
            stmt->blockObj = blockObj;

#ifdef DEBUG
            ParseNode *tmp = pc->blockNode;
            JS_ASSERT(!tmp || !tmp->isKind(PNK_LEXICALSCOPE));
#endif

            
            ParseNode *pn1 = LexicalScopeNode::create(PNK_LEXICALSCOPE, &handler);
            if (!pn1)
                return null();

            pn1->setOp(JSOP_LEAVEBLOCK);
            pn1->pn_pos = pc->blockNode->pn_pos;
            pn1->pn_objbox = blockbox;
            pn1->pn_expr = pc->blockNode;
            pn1->pn_blockid = pc->blockNode->pn_blockid;
            pc->blockNode = pn1;
        }

        pn = variables(PNK_LET, NULL, pc->blockChain, HoistVars);
        if (!pn)
            return null();
        pn->pn_xflags = PNX_POPVAR;
    } while (0);

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::letStatement()
{
    setUnknownResult();
    return SyntaxParseHandler::NodeFailure;
}

#endif 

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::expressionStatement()
{
    tokenStream.ungetToken();
    Node pn2 = expr();
    if (!pn2)
        return null();

    if (tokenStream.peekToken() == TOK_COLON) {
        RootedAtom label(context, handler.isName(pn2));
        if (!label) {
            report(ParseError, false, null(), JSMSG_BAD_LABEL);
            return null();
        }
        for (StmtInfoPC *stmt = pc->topStmt; stmt; stmt = stmt->down) {
            if (stmt->type == STMT_LABEL && stmt->label == label) {
                report(ParseError, false, null(), JSMSG_DUPLICATE_LABEL);
                return null();
            }
        }
        ForgetUse(pn2);

        (void) tokenStream.getToken();

        
        StmtInfoPC stmtInfo(context);
        PushStatementPC(pc, &stmtInfo, STMT_LABEL);
        stmtInfo.label = label;
        Node pn = statement();
        if (!pn)
            return null();

        
        PopStatementPC(context, pc);

        handler.morphNameIntoLabel(pn2, pn);
        return pn2;
    }

    Node pn = handler.newUnary(PNK_SEMI, pn2);

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : null();
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::statement()
{
    Node pn;

    JS_CHECK_RECURSION(context, return null());

    switch (tokenStream.getToken(TSF_OPERAND)) {
      case TOK_FUNCTION:
        return functionStmt();

      case TOK_IF:
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;

        
        Node cond = condition();
        if (!cond)
            return null();

        StmtInfoPC stmtInfo(context);
        PushStatementPC(pc, &stmtInfo, STMT_IF);
        Node thenBranch = statement();
        if (!thenBranch)
            return null();

        if (handler.isEmptySemicolon(thenBranch) &&
            !report(ParseStrictWarning, false, null(), JSMSG_EMPTY_CONSEQUENT))
        {
            return null();
        }

        Node elseBranch;
        if (tokenStream.matchToken(TOK_ELSE, TSF_OPERAND)) {
            stmtInfo.type = STMT_ELSE;
            elseBranch = statement();
            if (!elseBranch)
                return null();
        } else {
            elseBranch = null();
        }

        PopStatementPC(context, pc);
        pn = handler.newTernary(PNK_IF, cond, thenBranch, elseBranch);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        return pn;
      }

      case TOK_SWITCH:
        return switchStatement();

      case TOK_WHILE:
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;
        StmtInfoPC stmtInfo(context);
        PushStatementPC(pc, &stmtInfo, STMT_WHILE_LOOP);
        Node cond = condition();
        if (!cond)
            return null();
        Node body = statement();
        if (!body)
            return null();
        PopStatementPC(context, pc);
        pn = handler.newBinary(PNK_WHILE, cond, body);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        return pn;
      }

      case TOK_DO:
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;
        StmtInfoPC stmtInfo(context);
        PushStatementPC(pc, &stmtInfo, STMT_DO_LOOP);
        Node body = statement();
        if (!body)
            return null();
        MUST_MATCH_TOKEN(TOK_WHILE, JSMSG_WHILE_AFTER_DO);
        Node cond = condition();
        if (!cond)
            return null();
        PopStatementPC(context, pc);

        pn = handler.newBinary(PNK_DOWHILE, body, cond);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);

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
        uint32_t begin = tokenStream.currentToken().pos.begin;

        
        TokenKind tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
        if (tt == TOK_ERROR)
            return null();
        if (tt == TOK_EOF || tt == TOK_EOL || tt == TOK_SEMI || tt == TOK_RC) {
            report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
            return null();
        }

        Node pnexp = expr();
        if (!pnexp)
            return null();

        pn = handler.newUnary(PNK_THROW, pnexp, JSOP_THROW);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        break;
      }

      
      case TOK_CATCH:
        report(ParseError, false, null(), JSMSG_CATCH_WITHOUT_TRY);
        return null();

      case TOK_FINALLY:
        report(ParseError, false, null(), JSMSG_FINALLY_WITHOUT_TRY);
        return null();

      case TOK_BREAK:
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;
        RootedPropertyName label(context);
        if (!MatchLabel(context, &tokenStream, &label))
            return null();
        uint32_t end = tokenStream.currentToken().pos.end;
        pn = handler.newBreak(label, begin, end);
        if (!pn)
            return null();
        StmtInfoPC *stmt = pc->topStmt;
        if (label) {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    report(ParseError, false, null(), JSMSG_LABEL_NOT_FOUND);
                    return null();
                }
                if (stmt->type == STMT_LABEL && stmt->label == label)
                    break;
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    report(ParseError, false, null(), JSMSG_TOUGH_BREAK);
                    return null();
                }
                if (stmt->isLoop() || stmt->type == STMT_SWITCH)
                    break;
            }
        }
        break;
      }

      case TOK_CONTINUE:
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;
        RootedPropertyName label(context);
        if (!MatchLabel(context, &tokenStream, &label))
            return null();
        uint32_t end = tokenStream.currentToken().pos.end;
        pn = handler.newContinue(label, begin, end);
        if (!pn)
            return null();
        StmtInfoPC *stmt = pc->topStmt;
        if (label) {
            for (StmtInfoPC *stmt2 = NULL; ; stmt = stmt->down) {
                if (!stmt) {
                    report(ParseError, false, null(), JSMSG_LABEL_NOT_FOUND);
                    return null();
                }
                if (stmt->type == STMT_LABEL) {
                    if (stmt->label == label) {
                        if (!stmt2 || !stmt2->isLoop()) {
                            report(ParseError, false, null(), JSMSG_BAD_CONTINUE);
                            return null();
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
                    report(ParseError, false, null(), JSMSG_BAD_CONTINUE);
                    return null();
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
            return null();

        
        handler.setListFlag(pn, PNX_POPVAR);
        break;

      case TOK_CONST:
        pn = variables(PNK_CONST);
        if (!pn)
            return null();

        
        handler.setListFlag(pn, PNX_POPVAR);
        break;

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        return letStatement();
#endif 

      case TOK_RETURN:
        pn = returnOrYield(false);
        if (!pn)
            return null();
        break;

      case TOK_LC:
      {
        StmtInfoPC stmtInfo(context);
        if (!PushBlocklikeStatement(&stmtInfo, STMT_BLOCK, pc))
            return null();
        bool hasFunctionStmt;
        pn = statements(&hasFunctionStmt);
        if (!pn)
            return null();

        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_COMPOUND);
        PopStatementPC(context, pc);
        return pn;
      }

      case TOK_SEMI:
        return handler.newUnary(PNK_SEMI);

      case TOK_DEBUGGER:
        pn = handler.newDebuggerStatement(tokenStream.currentToken().pos);
        if (!pn)
            return null();
        pc->sc->setBindingsAccessedDynamically();
        pc->sc->setHasDebuggerStatement();
        break;

      case TOK_ERROR:
        return null();

      case TOK_NAME:
        if (tokenStream.currentToken().name() == context->names().module &&
            tokenStream.peekTokenSameLine(TSF_OPERAND) == TOK_STRING)
        {
            return moduleDecl();
        }

      default:
        return expressionStatement();
    }

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : null();
}






template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::variables(ParseNodeKind kind, bool *psimple,
                                StaticBlockObject *blockObj, VarContext varContext)
{
    






    JS_ASSERT(kind == PNK_VAR || kind == PNK_CONST || kind == PNK_LET || kind == PNK_CALL);

    



    JS_ASSERT_IF(psimple, *psimple);

    JSOp op = blockObj ? JSOP_NOP : kind == PNK_VAR ? JSOP_DEFVAR : JSOP_DEFCONST;

    Node pn = handler.newList(kind, null(), op);
    if (!pn)
        return null();

    




    BindData<ParseHandler> data(context);
    if (blockObj)
        data.initLet(varContext, *blockObj, JSMSG_TOO_MANY_LOCALS);
    else
        data.initVarOrConst(op);

    bool first = true;
    Node pn2;
    do {
        if (psimple && !first)
            *psimple = false;
        first = false;

        TokenKind tt = tokenStream.getToken();
#if JS_HAS_DESTRUCTURING
        if (tt == TOK_LB || tt == TOK_LC) {
            if (psimple)
                *psimple = false;

            pc->inDeclDestructuring = true;
            pn2 = primaryExpr(tt);
            pc->inDeclDestructuring = false;
            if (!pn2)
                return null();

            if (!checkDestructuring(&data, pn2))
                return null();
            bool ignored;
            if (pc->parsingForInit && matchInOrOf(&ignored)) {
                tokenStream.ungetToken();
                handler.addList(pn, pn2);
                continue;
            }

            MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

            Node init = assignExpr();
            if (!init)
                return null();

            pn2 = handler.newBinaryOrAppend(PNK_ASSIGN, pn2, init, pc);
            if (!pn2)
                return null();
            handler.addList(pn, pn2);
            continue;
        }
#endif 

        if (tt != TOK_NAME) {
            if (tt != TOK_ERROR)
                report(ParseError, false, null(), JSMSG_NO_VARIABLE_NAME);
            return null();
        }

        RootedPropertyName name(context, tokenStream.currentToken().name());
        pn2 = newBindingNode(name, varContext);
        if (!pn2)
            return null();
        if (data.op == JSOP_DEFCONST)
            handler.setFlag(pn2, PND_CONST);
        data.pn = pn2;
        if (!data.binder(context, &data, name, this))
            return null();
        handler.addList(pn, pn2);

        if (tokenStream.matchToken(TOK_ASSIGN)) {
            JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NOP);

            if (psimple)
                *psimple = false;

            Node init = assignExpr();
            if (!init)
                return null();

            if (!handler.finishInitializerAssignment(pn2, init, data.op))
                return null();
        }
    } while (tokenStream.matchToken(TOK_COMMA));

    return pn;
}

template <>
ParseNode *
Parser<FullParseHandler>::expr()
{
    ParseNode *pn = assignExpr();
    if (pn && tokenStream.matchToken(TOK_COMMA)) {
        ParseNode *pn2 = ListNode::create(PNK_COMMA, &handler);
        if (!pn2)
            return null();
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->initList(pn);
        pn = pn2;
        do {
#if JS_HAS_GENERATORS
            pn2 = pn->last();
            if (pn2->isKind(PNK_YIELD) && !pn2->isInParens()) {
                report(ParseError, false, pn2, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
                return null();
            }
#endif
            pn2 = assignExpr();
            if (!pn2)
                return null();
            pn->append(pn2);
        } while (tokenStream.matchToken(TOK_COMMA));
        pn->pn_pos.end = pn->last()->pn_pos.end;
    }
    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::expr()
{
    Node pn = assignExpr();
    if (pn && tokenStream.matchToken(TOK_COMMA)) {
        do {
            if (!assignExpr())
                return null();
        } while (tokenStream.matchToken(TOK_COMMA));
        return SyntaxParseHandler::NodeGeneric;
    }
    return pn;
}

static const JSOp ParseNodeKindToJSOp[] = {
    JSOP_OR,
    JSOP_AND,
    JSOP_BITOR,
    JSOP_BITXOR,
    JSOP_BITAND,
    JSOP_STRICTEQ,
    JSOP_EQ,
    JSOP_STRICTNE,
    JSOP_NE,
    JSOP_LT,
    JSOP_LE,
    JSOP_GT,
    JSOP_GE,
    JSOP_INSTANCEOF,
    JSOP_IN,
    JSOP_LSH,
    JSOP_RSH,
    JSOP_URSH,
    JSOP_ADD,
    JSOP_SUB,
    JSOP_MUL,
    JSOP_DIV,
    JSOP_MOD
};

static inline JSOp
BinaryOpParseNodeKindToJSOp(ParseNodeKind pnk)
{
    JS_ASSERT(pnk >= PNK_BINOP_FIRST);
    JS_ASSERT(pnk <= PNK_BINOP_LAST);
    return ParseNodeKindToJSOp[pnk - PNK_BINOP_FIRST];
}

static bool
IsBinaryOpToken(TokenKind tok, bool parsingForInit)
{
    return tok == TOK_IN ? !parsingForInit : TokenKindIsBinaryOp(tok);
}

static ParseNodeKind
BinaryOpTokenKindToParseNodeKind(TokenKind tok)
{
    JS_ASSERT(TokenKindIsBinaryOp(tok));
    return ParseNodeKind(PNK_BINOP_FIRST + (tok - TOK_BINOP_FIRST));
}

static const int PrecedenceTable[] = {
    1, 
    2, 
    3, 
    4, 
    5, 
    6, 
    6, 
    6, 
    6, 
    7, 
    7, 
    7, 
    7, 
    7, 
    7, 
    8, 
    8, 
    8, 
    9, 
    9, 
    10, 
    10, 
    10  
};

static const int PRECEDENCE_CLASSES = 10;

static int
Precedence(ParseNodeKind pnk) {
    
    
    
    if (pnk == PNK_LIMIT)
        return 0;

    JS_ASSERT(pnk >= PNK_BINOP_FIRST);
    JS_ASSERT(pnk <= PNK_BINOP_LAST);
    return PrecedenceTable[pnk - PNK_BINOP_FIRST];
}

template <typename ParseHandler>
JS_ALWAYS_INLINE typename ParseHandler::Node
Parser<ParseHandler>::orExpr1()
{
    
    

    
    
    Node nodeStack[PRECEDENCE_CLASSES];
    ParseNodeKind kindStack[PRECEDENCE_CLASSES];
    int depth = 0;

    bool oldParsingForInit = pc->parsingForInit;
    pc->parsingForInit = false;

    Node pn;
    for (;;) {
        pn = unaryExpr();
        if (!pn)
            return pn;

        
        
        TokenKind tok = tokenStream.getToken();
        if (tok == TOK_ERROR)
            return null();
        ParseNodeKind pnk;
        if (IsBinaryOpToken(tok, oldParsingForInit)) {
            pnk = BinaryOpTokenKindToParseNodeKind(tok);
        } else {
            tok = TOK_EOF;
            pnk = PNK_LIMIT;
        }

        
        
        
        
        
        
        
        
        while (depth > 0 && Precedence(kindStack[depth - 1]) >= Precedence(pnk)) {
            depth--;
            ParseNodeKind combiningPnk = kindStack[depth];
            JSOp combiningOp = BinaryOpParseNodeKindToJSOp(combiningPnk);
            pn = handler.newBinaryOrAppend(combiningPnk, nodeStack[depth], pn, pc, combiningOp);
            if (!pn)
                return pn;
        }

        if (pnk == PNK_LIMIT)
            break;

        nodeStack[depth] = pn;
        kindStack[depth] = pnk;
        depth++;
        JS_ASSERT(depth <= PRECEDENCE_CLASSES);
    }

    JS_ASSERT(depth == 0);
    pc->parsingForInit = oldParsingForInit;
    return pn;
}

template <typename ParseHandler>
JS_ALWAYS_INLINE typename ParseHandler::Node
Parser<ParseHandler>::condExpr1()
{
    Node condition = orExpr1();
    if (!condition || !tokenStream.isCurrentTokenType(TOK_HOOK))
        return condition;

    




    bool oldParsingForInit = pc->parsingForInit;
    pc->parsingForInit = false;
    Node thenExpr = assignExpr();
    pc->parsingForInit = oldParsingForInit;
    if (!thenExpr)
        return null();

    MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_IN_COND);

    Node elseExpr = assignExpr();
    if (!elseExpr)
        return null();

    tokenStream.getToken(); 
    return handler.newConditional(condition, thenExpr, elseExpr);
}

template <>
bool
Parser<FullParseHandler>::setAssignmentLhsOps(ParseNode *pn, JSOp op)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        if (!checkStrictAssignment(pn))
            return false;
        pn->setOp(pn->isOp(JSOP_GETLOCAL) ? JSOP_SETLOCAL : JSOP_SETNAME);
        handler.noteLValue(pn);
        break;
      case PNK_DOT:
        pn->setOp(JSOP_SETPROP);
        break;
      case PNK_ELEM:
        pn->setOp(JSOP_SETELEM);
        break;
#if JS_HAS_DESTRUCTURING
      case PNK_ARRAY:
      case PNK_OBJECT:
        if (op != JSOP_NOP) {
            report(ParseError, false, null(), JSMSG_BAD_DESTRUCT_ASS);
            return false;
        }
        if (!checkDestructuring(NULL, pn))
            return false;
        break;
#endif
      case PNK_CALL:
        if (!makeSetCall(pn, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;
      default:
        report(ParseError, false, null(), JSMSG_BAD_LEFTSIDE_OF_ASS);
        return false;
    }
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::setAssignmentLhsOps(Node pn, JSOp op)
{
    
    if (pn != SyntaxParseHandler::NodeName && pn != SyntaxParseHandler::NodeLValue) {
        setUnknownResult();
        return false;
    }
    return checkStrictAssignment(pn);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::assignExpr()
{
    JS_CHECK_RECURSION(context, return null());

#if JS_HAS_GENERATORS
    if (tokenStream.matchToken(TOK_YIELD, TSF_OPERAND))
        return returnOrYield(true);
    if (tokenStream.hadError())
        return null();
#endif

    
    
    TokenStream::Position start;
    tokenStream.tell(&start);

    Node lhs = condExpr1();
    if (!lhs)
        return null();

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

      case TOK_ARROW: {
        tokenStream.seek(start);

        if (tokenStream.getToken() == TOK_ERROR)
            return null();
        size_t offset = tokenStream.currentToken().pos.begin;
        tokenStream.ungetToken();

        return functionDef(NullPtr(), start, offset, Normal, Arrow);
      }

      default:
        JS_ASSERT(!tokenStream.isCurrentTokenAssignment());
        tokenStream.ungetToken();
        return lhs;
    }

    JSOp op = tokenStream.currentToken().t_op;
    if (!setAssignmentLhsOps(lhs, op))
        return null();

    Node rhs = assignExpr();
    if (!rhs)
        return null();

    return handler.newBinaryOrAppend(kind, lhs, rhs, pc, op);
}

template <> bool
Parser<FullParseHandler>::setLvalKid(ParseNode *pn, ParseNode *kid, const char *name)
{
    if (!kid->isKind(PNK_NAME) &&
        !kid->isKind(PNK_DOT) &&
        (!kid->isKind(PNK_CALL) ||
         (!kid->isOp(JSOP_CALL) && !kid->isOp(JSOP_EVAL) &&
          !kid->isOp(JSOP_FUNCALL) && !kid->isOp(JSOP_FUNAPPLY))) &&
        !kid->isKind(PNK_ELEM))
    {
        report(ParseError, false, null(), JSMSG_BAD_OPERAND, name);
        return false;
    }
    if (!checkStrictAssignment(kid))
        return false;
    pn->pn_kid = kid;
    return true;
}

static const char incop_name_str[][10] = {"increment", "decrement"};

template <>
bool
Parser<FullParseHandler>::setIncOpKid(ParseNode *pn, ParseNode *kid, TokenKind tt, bool preorder)
{
    JSOp op;

    if (!setLvalKid(pn, kid, incop_name_str[tt == TOK_DEC]))
        return false;

    switch (kid->getKind()) {
      case PNK_NAME:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCNAME : JSOP_NAMEINC)
             : (preorder ? JSOP_DECNAME : JSOP_NAMEDEC);
        handler.noteLValue(kid);
        break;

      case PNK_DOT:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCPROP : JSOP_PROPINC)
             : (preorder ? JSOP_DECPROP : JSOP_PROPDEC);
        break;

      case PNK_CALL:
        if (!makeSetCall(kid, JSMSG_BAD_INCOP_OPERAND))
            return false;
        
      case PNK_ELEM:
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

template <>
bool
Parser<SyntaxParseHandler>::setIncOpKid(Node pn, Node kid, TokenKind tt, bool preorder)
{
    return setAssignmentLhsOps(kid, JSOP_NOP);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::unaryOpExpr(ParseNodeKind kind, JSOp op)
{
    Node kid = unaryExpr();
    if (!kid)
        return null();
    return handler.newUnary(kind, kid, op);
}

template <>
bool
Parser<FullParseHandler>::checkDeleteExpression(ParseNode **pn_)
{
    ParseNode *&pn = *pn_;

    




    if (foldConstants && !FoldConstants(context, &pn, this))
        return false;
    switch (pn->getKind()) {
      case PNK_CALL:
        if (!(pn->pn_xflags & PNX_SETCALL)) {
            



            if (!makeSetCall(pn, JSMSG_BAD_DELETE_OPERAND))
                return false;
            pn->pn_xflags &= ~PNX_SETCALL;
        }
        break;
      case PNK_NAME:
        if (!report(ParseStrictError, pc->sc->strict, pn, JSMSG_DEPRECATED_DELETE_OPERAND))
            return null();
        pc->sc->setBindingsAccessedDynamically();
        pn->pn_dflags |= PND_DEOPTIMIZED;
        pn->setOp(JSOP_DELNAME);
        break;
      default:;
    }
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkDeleteExpression(Node *pn)
{
    PropertyName *name = handler.isName(*pn);
    if (name)
        return report(ParseStrictError, pc->sc->strict, *pn, JSMSG_DEPRECATED_DELETE_OPERAND);
    return true;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::unaryExpr()
{
    Node pn, pn2;

    JS_CHECK_RECURSION(context, return null());

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
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;
        TokenKind tt2 = tokenStream.getToken(TSF_OPERAND);
        pn2 = memberExpr(tt2, true);
        if (!pn2)
            return null();
        pn = handler.newUnary((tt == TOK_INC) ? PNK_PREINCREMENT : PNK_PREDECREMENT, pn2);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        if (!setIncOpKid(pn, pn2, tt, true))
            return null();
        break;
      }

      case TOK_DELETE:
      {
        uint32_t begin = tokenStream.currentToken().pos.begin;
        pn2 = unaryExpr();
        if (!pn2)
            return null();

        if (!checkDeleteExpression(&pn2))
            return null();

        pn = handler.newUnary(PNK_DELETE, pn2);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        break;
      }
      case TOK_ERROR:
        return null();

      default:
        pn = memberExpr(tt, true);
        if (!pn)
            return null();

        
        tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
        if (tt == TOK_INC || tt == TOK_DEC) {
            tokenStream.consumeKnownToken(tt);
            pn2 = handler.newUnary((tt == TOK_INC) ? PNK_POSTINCREMENT : PNK_POSTDECREMENT, pn);
            if (!pn2)
                return null();
            if (!setIncOpKid(pn2, pn, tt, false))
                return null();
            pn = pn2;
        }
        break;
    }
    return pn;
}

#if JS_HAS_GENERATORS






















class CompExprTransplanter
{
    ParseNode       *root;
    Parser<FullParseHandler> *parser;
    bool            genexp;
    unsigned        adjust;
    HashSet<Definition *> visitedImplicitArguments;

  public:
    CompExprTransplanter(ParseNode *pn, Parser<FullParseHandler> *parser, bool ge, unsigned adj)
      : root(pn), parser(parser), genexp(ge), adjust(adj),
        visitedImplicitArguments(parser->context)
    {}

    bool init() {
        return visitedImplicitArguments.init();
    }

    bool transplant(ParseNode *pn);
};


















template <typename ParseHandler>
class GenexpGuard
{
    Parser<ParseHandler> *parser;
    uint32_t startYieldCount;

    typedef typename ParseHandler::Node Node;

  public:
    explicit GenexpGuard(Parser<ParseHandler> *parser)
      : parser(parser)
    {
        ParseContext<ParseHandler> *pc = parser->pc;
        if (pc->parenDepth == 0) {
            pc->yieldCount = 0;
            pc->yieldNode = ParseHandler::null();
        }
        startYieldCount = pc->yieldCount;
        pc->parenDepth++;
    }

    void endBody();
    bool checkValidBody(Node pn, unsigned err = JSMSG_BAD_GENEXP_BODY);
    bool maybeNoteGenerator(Node pn);
};

template <typename ParseHandler>
void
GenexpGuard<ParseHandler>::endBody()
{
    parser->pc->parenDepth--;
}








template <typename ParseHandler>
bool
GenexpGuard<ParseHandler>::checkValidBody(Node pn, unsigned err)
{
    ParseContext<ParseHandler> *pc = parser->pc;
    if (pc->yieldCount > startYieldCount) {
        Node errorNode = pc->yieldNode;
        if (!errorNode)
            errorNode = pn;
        parser->report(ParseError, false, errorNode, err, js_yield_str);
        return false;
    }

    return true;
}








template <typename ParseHandler>
bool
GenexpGuard<ParseHandler>::maybeNoteGenerator(Node pn)
{
    ParseContext<ParseHandler> *pc = parser->pc;
    if (pc->yieldCount > 0) {
        if (!pc->sc->isFunctionBox()) {
            parser->report(ParseError, false, ParseHandler::null(),
                           JSMSG_BAD_RETURN_OR_YIELD, js_yield_str);
            return false;
        }
        pc->sc->asFunctionBox()->setIsGenerator();
        if (pc->funHasReturnExpr) {
            
            parser->reportBadReturn(pn, ParseError,
                                    JSMSG_BAD_GENERATOR_RETURN, JSMSG_BAD_ANON_GENERATOR_RETURN);
            return false;
        }
    }
    return true;
}






template <typename ParseHandler>
static bool
BumpStaticLevel(ParseNode *pn, ParseContext<ParseHandler> *pc)
{
    if (pn->pn_cookie.isFree())
        return true;

    unsigned level = unsigned(pn->pn_cookie.level()) + 1;
    JS_ASSERT(level >= pc->staticLevel);
    return pn->pn_cookie.set(pc->sc->context, level, pn->pn_cookie.slot());
}

template <typename ParseHandler>
static bool
AdjustBlockId(ParseNode *pn, unsigned adjust, ParseContext<ParseHandler> *pc)
{
    JS_ASSERT(pn->isArity(PN_LIST) || pn->isArity(PN_CODE) || pn->isArity(PN_NAME));
    if (JS_BIT(20) - pn->pn_blockid <= adjust + 1) {
        JS_ReportErrorNumber(pc->sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    pn->pn_blockid += adjust;
    if (pn->pn_blockid >= pc->blockidGen)
        pc->blockidGen = pn->pn_blockid + 1;
    return true;
}

bool
CompExprTransplanter::transplant(ParseNode *pn)
{
    ParseContext<FullParseHandler> *pc = parser->pc;

    if (!pn)
        return true;

    switch (pn->getArity()) {
      case PN_LIST:
        for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!transplant(pn2))
                return false;
        }
        if (pn->pn_pos >= root->pn_pos) {
            if (!AdjustBlockId(pn, adjust, pc))
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

      case PN_CODE:
      case PN_NAME:
        if (!transplant(pn->maybeExpr()))
            return false;

        if (pn->isDefn()) {
            if (genexp && !BumpStaticLevel(pn, pc))
                return false;
        } else if (pn->isUsed()) {
            JS_ASSERT(pn->pn_cookie.isFree());

            Definition *dn = pn->pn_lexdef;
            JS_ASSERT(dn->isDefn());

            








            if (dn->isPlaceholder() && dn->pn_pos >= root->pn_pos && dn->dn_uses == pn) {
                if (genexp && !BumpStaticLevel(dn, pc))
                    return false;
                if (!AdjustBlockId(dn, adjust, pc))
                    return false;
            }

            RootedAtom atom(parser->context, pn->pn_atom);
#ifdef DEBUG
            StmtInfoPC *stmt = LexicalLookup(pc, atom, NULL, (StmtInfoPC *)NULL);
            JS_ASSERT(!stmt || stmt != pc->topStmt);
#endif
            if (genexp && !dn->isOp(JSOP_CALLEE)) {
                JS_ASSERT(!pc->decls().lookupFirst(atom));

                if (dn->pn_pos < root->pn_pos) {
                    







                    Definition *dn2 = MakePlaceholder(pn, &parser->handler, parser->pc);
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
                    if (!pc->lexdeps->put(atom, dn2))
                        return false;
                    if (dn->isClosed())
                        dn2->pn_dflags |= PND_CLOSED;
                } else if (dn->isPlaceholder()) {
                    




                    pc->parent->lexdeps->remove(atom);
                    if (!pc->lexdeps->put(atom, dn))
                        return false;
                } else if (dn->isImplicitArguments()) {
                    






                    if (genexp && !visitedImplicitArguments.has(dn)) {
                        if (!BumpStaticLevel(dn, pc))
                            return false;
                        if (!AdjustBlockId(dn, adjust, pc))
                            return false;
                        if (!visitedImplicitArguments.put(dn))
                            return false;
                    }
                }
            }
        }

        if (pn->pn_pos >= root->pn_pos) {
            if (!AdjustBlockId(pn, adjust, pc))
                return false;
        }
        break;

      case PN_NULLARY:
        
        break;
    }
    return true;
}










template <>
ParseNode *
Parser<FullParseHandler>::comprehensionTail(ParseNode *kid, unsigned blockid, bool isGenexp,
                                            ParseNodeKind kind, JSOp op)
{
    unsigned adjust;
    ParseNode *pn, *pn2, *pn3, **pnp;
    StmtInfoPC stmtInfo(context);
    BindData<FullParseHandler> data(context);
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_FOR);

    if (kind == PNK_SEMI) {
        




        pn = pushLexicalScope(&stmtInfo);
        if (!pn)
            return null();
        adjust = pn->pn_blockid - blockid;
    } else {
        JS_ASSERT(kind == PNK_ARRAYPUSH);

        











        adjust = pc->blockid();
        pn = pushLexicalScope(&stmtInfo);
        if (!pn)
            return null();

        JS_ASSERT(blockid <= pn->pn_blockid);
        JS_ASSERT(blockid < pc->blockidGen);
        JS_ASSERT(pc->bodyid < blockid);
        pn->pn_blockid = stmtInfo.blockid = blockid;
        JS_ASSERT(adjust < blockid);
        adjust = blockid - adjust;
    }

    pnp = &pn->pn_expr;

    CompExprTransplanter transplanter(kid, this, kind == PNK_SEMI, adjust);
    if (!transplanter.init())
        return null();

    if (!transplanter.transplant(kid))
        return null();

    JS_ASSERT(pc->blockChain && pc->blockChain == pn->pn_objbox->object);
    data.initLet(HoistVars, *pc->blockChain, JSMSG_ARRAY_INIT_TOO_BIG);

    do {
        




        pn2 = BinaryNode::create(PNK_FOR, &handler);
        if (!pn2)
            return null();

        pn2->setOp(JSOP_ITER);
        pn2->pn_iflags = JSITER_ENUMERATE;
        if (allowsForEachIn() && tokenStream.matchToken(TOK_NAME)) {
            if (tokenStream.currentToken().name() == context->names().each)
                pn2->pn_iflags |= JSITER_FOREACH;
            else
                tokenStream.ungetToken();
        }
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

        GenexpGuard<FullParseHandler> guard(this);

        RootedPropertyName name(context);
        tt = tokenStream.getToken();
        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            pc->inDeclDestructuring = true;
            pn3 = primaryExpr(tt);
            pc->inDeclDestructuring = false;
            if (!pn3)
                return null();
            break;
#endif

          case TOK_NAME:
            name = tokenStream.currentToken().name();

            






            pn3 = newBindingNode(name);
            if (!pn3)
                return null();
            break;

          default:
            report(ParseError, false, null(), JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return null();
        }

        bool forOf;
        if (!matchInOrOf(&forOf)) {
            report(ParseError, false, null(), JSMSG_IN_AFTER_FOR_NAME);
            return null();
        }
        if (forOf) {
            if (pn2->pn_iflags != JSITER_ENUMERATE) {
                JS_ASSERT(pn2->pn_iflags == (JSITER_FOREACH | JSITER_ENUMERATE));
                report(ParseError, false, null(), JSMSG_BAD_FOR_EACH_LOOP);
                return null();
            }
            pn2->pn_iflags = JSITER_FOR_OF;
        }

        ParseNode *pn4 = expr();
        if (!pn4)
            return null();
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

        guard.endBody();

        if (isGenexp) {
            if (!guard.checkValidBody(pn2))
                return null();
        } else {
            if (!guard.maybeNoteGenerator(pn2))
                return null();
        }

        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            if (!checkDestructuring(&data, pn3))
                return null();

            if (versionNumber() == JSVERSION_1_7 &&
                !(pn2->pn_iflags & JSITER_FOREACH) &&
                !forOf)
            {
                
                if (!pn3->isKind(PNK_ARRAY) || pn3->pn_count != 2) {
                    report(ParseError, false, null(), JSMSG_BAD_FOR_LEFTSIDE);
                    return null();
                }

                JS_ASSERT(pn2->isOp(JSOP_ITER));
                JS_ASSERT(pn2->pn_iflags & JSITER_ENUMERATE);
                pn2->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          case TOK_NAME:
            data.pn = pn3;
            if (!data.binder(context, &data, name, this))
                return null();
            break;

          default:;
        }

        



        ParseNode *vars = ListNode::create(PNK_VAR, &handler);
        if (!vars)
            return null();
        vars->setOp(JSOP_NOP);
        vars->pn_pos = pn3->pn_pos;
        vars->makeEmpty();
        vars->append(pn3);
        vars->pn_xflags |= PNX_FORINVAR;

        
        pn3 = cloneLeftHandSide(pn3);
        if (!pn3)
            return null();

        pn2->pn_left = handler.newTernary(PNK_FORIN, vars, pn3, pn4);
        if (!pn2->pn_left)
            return null();
        *pnp = pn2;
        pnp = &pn2->pn_right;
    } while (tokenStream.matchToken(TOK_FOR));

    if (tokenStream.matchToken(TOK_IF)) {
        pn2 = TernaryNode::create(PNK_IF, &handler);
        if (!pn2)
            return null();
        pn2->pn_kid1 = condition();
        if (!pn2->pn_kid1)
            return null();
        *pnp = pn2;
        pnp = &pn2->pn_kid2;
    }

    pn2 = UnaryNode::create(kind, &handler);
    if (!pn2)
        return null();
    pn2->setOp(op);
    pn2->pn_kid = kid;
    *pnp = pn2;

    PopStatementPC(context, pc);
    return pn;
}

template <>
bool
Parser<FullParseHandler>::arrayInitializerComprehensionTail(ParseNode *pn)
{
    
    pn->setKind(PNK_ARRAYCOMP);

    




    ParseNode *pnexp = pn->last();
    JS_ASSERT(pn->pn_count == 1);
    pn->pn_count = 0;
    pn->pn_tail = &pn->pn_head;
    *pn->pn_tail = NULL;

    ParseNode *pntop = comprehensionTail(pnexp, pn->pn_blockid, false,
                                         PNK_ARRAYPUSH, JSOP_ARRAYPUSH);
    if (!pntop)
        return false;
    pn->append(pntop);
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::arrayInitializerComprehensionTail(Node pn)
{
    setUnknownResult();
    return false;
}

#if JS_HAS_GENERATOR_EXPRS
















template <>
ParseNode *
Parser<FullParseHandler>::generatorExpr(ParseNode *kid)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    
    ParseNode *pn = UnaryNode::create(PNK_YIELD, &handler);
    if (!pn)
        return null();
    pn->setOp(JSOP_YIELD);
    pn->setInParens(true);
    pn->pn_pos = kid->pn_pos;
    pn->pn_kid = kid;
    pn->pn_hidden = true;

    
    ParseNode *genfn = CodeNode::create(PNK_FUNCTION, &handler);
    if (!genfn)
        return null();
    genfn->setOp(JSOP_LAMBDA);
    JS_ASSERT(!genfn->pn_body);
    genfn->pn_dflags = 0;

    {
        ParseContext<FullParseHandler> *outerpc = pc;

        RootedFunction fun(context, newFunction(outerpc,  NullPtr(), Expression));
        if (!fun)
            return null();

        
        FunctionBox *genFunbox = newFunctionBox(fun, outerpc, outerpc->sc->strict);
        if (!genFunbox)
            return null();

        ParseContext<FullParseHandler> genpc(this, genFunbox, outerpc->staticLevel + 1, outerpc->blockidGen);
        if (!genpc.init())
            return null();

        





        genFunbox->anyCxFlags = outerpc->sc->anyCxFlags;
        if (outerpc->sc->isFunctionBox())
            genFunbox->funCxFlags = outerpc->sc->asFunctionBox()->funCxFlags;

        genFunbox->setIsGenerator();
        genFunbox->inGenexpLambda = true;
        genfn->pn_funbox = genFunbox;
        genfn->pn_blockid = genpc.bodyid;

        ParseNode *body = comprehensionTail(pn, outerpc->blockid(), true);
        if (!body)
            return null();
        JS_ASSERT(!genfn->pn_body);
        genfn->pn_body = body;
        genfn->pn_pos.begin = body->pn_pos.begin = kid->pn_pos.begin;
        genfn->pn_pos.end = body->pn_pos.end = tokenStream.currentToken().pos.end;

        if (AtomDefnPtr p = genpc.lexdeps->lookup(context->names().arguments)) {
            Definition *dn = p.value();
            ParseNode *errorNode = dn->dn_uses ? dn->dn_uses : body;
            report(ParseError, false, errorNode, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
            return null();
        }

        RootedPropertyName funName(context);
        if (!leaveFunction(genfn, funName))
            return null();
    }

    



    ParseNode *result = ListNode::create(PNK_GENEXP, &handler);
    if (!result)
        return null();
    result->setOp(JSOP_CALL);
    result->pn_pos.begin = genfn->pn_pos.begin;
    result->initList(genfn);
    return result;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::generatorExpr(Node kid)
{
    setUnknownResult();
    return SyntaxParseHandler::NodeFailure;
}

static const char js_generator_str[] = "generator";

#endif 
#endif 

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::assignExprWithoutYield(unsigned msg)
{
#ifdef JS_HAS_GENERATORS
    GenexpGuard<ParseHandler> yieldGuard(this);
#endif
    Node res = assignExpr();
    yieldGuard.endBody();
    if (res) {
#ifdef JS_HAS_GENERATORS
        if (!yieldGuard.checkValidBody(res, msg))
            return null();
#endif
    }
    return res;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::argumentList(Node listNode)
{
    if (tokenStream.matchToken(TOK_RP, TSF_OPERAND))
        return true;

    GenexpGuard<ParseHandler> guard(this);
    bool arg0 = true;

    do {
        Node argNode = assignExpr();
        if (!argNode)
            return false;
        if (arg0)
            guard.endBody();

#if JS_HAS_GENERATORS
        if (handler.isOperationWithoutParens(argNode, PNK_YIELD) &&
            tokenStream.peekToken() == TOK_COMMA) {
            report(ParseError, false, argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
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
            if (!arg0 || tokenStream.peekToken() == TOK_COMMA) {
                report(ParseError, false, argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
                return false;
            }
        } else
#endif
        if (arg0 && !guard.maybeNoteGenerator(argNode))
            return false;

        arg0 = false;

        handler.addList(listNode, argNode);
    } while (tokenStream.matchToken(TOK_COMMA));

    if (tokenStream.getToken() != TOK_RP) {
        report(ParseError, false, null(), JSMSG_PAREN_AFTER_ARGS);
        return false;
    }
    return true;
}

template <>
PropertyName *
Parser<FullParseHandler>::foldPropertyByValue(ParseNode *pn)
{
    







    uint32_t index;
    if (foldConstants) {
        if (pn->isKind(PNK_STRING)) {
            JSAtom *atom = pn->pn_atom;
            if (atom->isIndex(&index)) {
                pn->setKind(PNK_NUMBER);
                pn->setOp(JSOP_DOUBLE);
                pn->pn_dval = index;
            } else {
                return atom->asPropertyName();
            }
        } else if (pn->isKind(PNK_NUMBER)) {
            double number = pn->pn_dval;
            if (number != ToUint32(number)) {
                JSAtom *atom = ToAtom<NoGC>(context, DoubleValue(number));
                if (!atom)
                    return NULL;
                return atom->asPropertyName();
            }
        }
    }

    return NULL;
}

template <>
PropertyName *
Parser<SyntaxParseHandler>::foldPropertyByValue(Node pn)
{
    return NULL;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::memberExpr(TokenKind tt, bool allowCallSyntax)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(tt));

    Node lhs;

    JS_CHECK_RECURSION(context, return null());

    
    if (tt == TOK_NEW) {
        lhs = handler.newList(PNK_NEW, null(), JSOP_NEW);
        if (!lhs)
            return null();

        tt = tokenStream.getToken(TSF_OPERAND);
        Node ctorExpr = memberExpr(tt, false);
        if (!ctorExpr)
            return null();

        handler.addList(lhs, ctorExpr);

        if (tokenStream.matchToken(TOK_LP) && !argumentList(lhs))
            return null();
    } else {
        lhs = primaryExpr(tt);
        if (!lhs)
            return null();
    }

    while ((tt = tokenStream.getToken()) > TOK_EOF) {
        Node nextMember;
        if (tt == TOK_DOT) {
            tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            if (tt == TOK_ERROR)
                return null();
            if (tt == TOK_NAME) {
                PropertyName *field = tokenStream.currentToken().name();
                uint32_t end = tokenStream.currentToken().pos.end;
                nextMember = handler.newPropertyAccess(lhs, field, end);
                if (!nextMember)
                    return null();
            } else {
                report(ParseError, false, null(), JSMSG_NAME_AFTER_DOT);
                return null();
            }
        } else if (tt == TOK_LB) {
            Node propExpr = expr();
            if (!propExpr)
                return null();

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_IN_INDEX);

            



            if (foldConstants && !FoldConstants(context, &propExpr, this))
                return null();

            PropertyName *name = foldPropertyByValue(propExpr);

            uint32_t end = tokenStream.currentToken().pos.end;
            if (name)
                nextMember = handler.newPropertyAccess(lhs, name, end);
            else
                nextMember = handler.newPropertyByValue(lhs, propExpr, end);
            if (!nextMember)
                return null();
        } else if (allowCallSyntax && tt == TOK_LP) {
            nextMember = handler.newList(PNK_CALL, null(), JSOP_CALL);
            if (!nextMember)
                return null();

            if (JSAtom *atom = handler.isName(lhs)) {
                if (atom == context->names().eval) {
                    
                    handler.setOp(nextMember, JSOP_EVAL);
                    pc->sc->setBindingsAccessedDynamically();

                    



                    if (pc->sc->isFunctionBox() && !pc->sc->strict)
                        pc->sc->asFunctionBox()->setHasExtensibleScope();
                }
            } else if (JSAtom *atom = handler.isGetProp(lhs)) {
                
                if (atom == context->names().apply)
                    handler.setOp(nextMember, JSOP_FUNAPPLY);
                else if (atom == context->names().call)
                    handler.setOp(nextMember, JSOP_FUNCALL);
            }

            handler.setBeginPosition(nextMember, lhs);
            handler.addList(nextMember, lhs);

            if (!argumentList(nextMember))
                return null();
        } else {
            tokenStream.ungetToken();
            return lhs;
        }

        lhs = nextMember;
    }
    if (tt == TOK_ERROR)
        return null();
    return lhs;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::bracketedExpr()
{
    




    bool oldParsingForInit = pc->parsingForInit;
    pc->parsingForInit = false;
    Node pn = expr();
    pc->parsingForInit = oldParsingForInit;
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::identifierName()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));

    PropertyName *name = tokenStream.currentToken().name();
    Node pn = handler.newName(name, pc);
    if (!pn)
        return null();

    if (!pc->inDeclDestructuring && !noteNameUse(pn))
        return null();

    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::atomNode(ParseNodeKind kind, JSOp op)
{
    JSAtom *atom = tokenStream.currentToken().atom();
    Node pn = handler.newAtom(kind, atom, op);
    if (!pn)
        return null();

    
    
    
    const size_t HUGE_STRING = 50000;
    if (sct && sct->active() && kind == PNK_STRING && atom->length() >= HUGE_STRING)
        sct->abort();

    return pn;
}

template <>
ParseNode *
Parser<FullParseHandler>::newRegExp(const jschar *buf, size_t length, RegExpFlag flags)
{
    ParseNode *pn = NullaryNode::create(PNK_REGEXP, &handler);
    if (!pn)
        return NULL;

    const StableCharPtr chars(buf, length);
    RegExpStatics *res = context->regExpStatics();

    Rooted<RegExpObject*> reobj(context);
    if (context->hasfp())
        reobj = RegExpObject::create(context, res, chars.get(), length, flags, &tokenStream);
    else
        reobj = RegExpObject::createNoStatics(context, chars.get(), length, flags, &tokenStream);

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
    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::newRegExp(const jschar *buf, size_t length, RegExpFlag flags)
{
    return SyntaxParseHandler::NodeGeneric;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::primaryExpr(TokenKind tt)
{
    JS_ASSERT(tokenStream.isCurrentTokenType(tt));

    Node pn, pn2, pn3;
    JSOp op;

    JS_CHECK_RECURSION(context, return null());

    switch (tt) {
      case TOK_FUNCTION:
        pn = functionExpr();
        if (!pn)
            return null();
        break;

      case TOK_LB:
      {
        pn = handler.newList(PNK_ARRAY, null(), JSOP_NEWINIT);
        if (!pn)
            return null();
#if JS_HAS_GENERATORS
        handler.setBlockId(pn, pc->blockidGen);
#endif

        if (tokenStream.matchToken(TOK_RB, TSF_OPERAND)) {
            



            handler.setListFlag(pn, PNX_NONCONST);
        } else {
            bool spread = false, missingTrailingComma = false;
            unsigned index = 0;
            for (; ; index++) {
                if (index == StackSpace::ARGS_LENGTH_MAX) {
                    report(ParseError, false, null(), JSMSG_ARRAY_INIT_TOO_BIG);
                    return null();
                }

                tt = tokenStream.peekToken(TSF_OPERAND);
                if (tt == TOK_RB)
                    break;

                if (tt == TOK_COMMA) {
                    
                    tokenStream.matchToken(TOK_COMMA);
                    pn2 = handler.newNullary(PNK_COMMA);
                    if (!pn2)
                        return null();
                    handler.setListFlag(pn, PNX_SPECIALARRAYINIT | PNX_NONCONST);
                } else if (tt == TOK_TRIPLEDOT) {
                    spread = true;
                    handler.setListFlag(pn, PNX_SPECIALARRAYINIT | PNX_NONCONST);

                    tokenStream.getToken();

                    Node inner = assignExpr();
                    if (!inner)
                        return null();

                    pn2 = handler.newUnary(PNK_SPREAD, inner);
                    if (!pn2)
                        return null();
                } else {
                    pn2 = assignExpr();
                    if (!pn2)
                        return null();
                    if (foldConstants && !FoldConstants(context, &pn2, this))
                        return null();
                    if (!handler.isConstant(pn2))
                        handler.setListFlag(pn, PNX_NONCONST);
                }
                handler.addList(pn, pn2);

                if (tt != TOK_COMMA) {
                    
                    if (!tokenStream.matchToken(TOK_COMMA)) {
                        missingTrailingComma = true;
                        break;
                    }
                }
            }

#if JS_HAS_GENERATORS
            








































            if (index == 0 && !spread && tokenStream.matchToken(TOK_FOR) && missingTrailingComma) {
                if (!arrayInitializerComprehensionTail(pn))
                    return null();
            }
#endif 

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_LIST);
        }
        handler.setEndPosition(pn, tokenStream.currentToken().pos.end);
        return pn;
      }

      case TOK_LC:
      {
        Node pnval;

        



        AtomIndexMap seen(context);

        enum AssignmentType {
            GET     = 0x1,
            SET     = 0x2,
            VALUE   = 0x4 | GET | SET
        };

        pn = handler.newList(PNK_OBJECT, null(), JSOP_NEWINIT);
        if (!pn)
            return null();

        RootedAtom atom(context);
        for (;;) {
            TokenKind ltok = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            switch (ltok) {
              case TOK_NUMBER:
                atom = ToAtom<CanGC>(context, DoubleValue(tokenStream.currentToken().number()));
                if (!atom)
                    return null();
                pn3 = handler.newNumber(tokenStream.currentToken());
                break;
              case TOK_NAME:
                {
                    atom = tokenStream.currentToken().name();
                    if (atom == context->names().get) {
                        op = JSOP_GETTER;
                    } else if (atom == context->names().set) {
                        op = JSOP_SETTER;
                    } else {
                        pn3 = handler.newAtom(PNK_NAME, atom);
                        if (!pn3)
                            return null();
                        break;
                    }

                    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
                    if (tt == TOK_NAME) {
                        atom = tokenStream.currentToken().name();
                        pn3 = handler.newName(atom->asPropertyName(), pc);
                        if (!pn3)
                            return null();
                    } else if (tt == TOK_STRING) {
                        atom = tokenStream.currentToken().atom();

                        uint32_t index;
                        if (atom->isIndex(&index)) {
                            pn3 = handler.newNumber(index);
                            if (!pn3)
                                return null();
                            atom = ToAtom<CanGC>(context, DoubleValue(index));
                            if (!atom)
                                return null();
                        } else {
                            pn3 = handler.newName(atom->asPropertyName(), pc, PNK_STRING);
                            if (!pn3)
                                return null();
                        }
                    } else if (tt == TOK_NUMBER) {
                        double number = tokenStream.currentToken().number();
                        atom = ToAtom<CanGC>(context, DoubleValue(number));
                        if (!atom)
                            return null();
                        pn3 = handler.newNumber(tokenStream.currentToken());
                        if (!pn3)
                            return null();
                    } else {
                        tokenStream.ungetToken();
                        pn3 = handler.newAtom(PNK_NAME, atom);
                        if (!pn3)
                            return null();
                        break;
                    }

                    handler.setListFlag(pn, PNX_NONCONST);

                    
                    Rooted<PropertyName*> funName(context, NULL);
                    TokenStream::Position start;
                    tokenStream.tell(&start);
                    pn2 = functionDef(funName, start, tokenStream.positionToOffset(start),
                                      op == JSOP_GETTER ? Getter : Setter,
                                      Expression);
                    if (!pn2)
                        return null();
                    pn2 = handler.newBinary(PNK_COLON, pn3, pn2, op);
                    goto skip;
                }
              case TOK_STRING: {
                atom = tokenStream.currentToken().atom();
                uint32_t index;
                if (atom->isIndex(&index)) {
                    pn3 = handler.newNumber(index);
                    if (!pn3)
                        return null();
                } else {
                    pn3 = handler.newAtom(PNK_STRING, atom);
                    if (!pn3)
                        return null();
                }
                break;
              }
              case TOK_RC:
                goto end_obj_init;
              default:
                report(ParseError, false, null(), JSMSG_BAD_PROP_ID);
                return null();
            }

            op = JSOP_INITPROP;
            tt = tokenStream.getToken();
            if (tt == TOK_COLON) {
                pnval = assignExpr();
                if (!pnval)
                    return null();

                if (foldConstants && !FoldConstants(context, &pnval, this))
                    return null();

                




                if (!handler.isConstant(pnval) || atom == context->names().proto)
                    handler.setListFlag(pn, PNX_NONCONST);
            }
#if JS_HAS_DESTRUCTURING_SHORTHAND
            else if (ltok == TOK_NAME && (tt == TOK_COMMA || tt == TOK_RC)) {
                



                tokenStream.ungetToken();
                if (!tokenStream.checkForKeyword(atom->charsZ(), atom->length(), NULL, NULL))
                    return null();
                handler.setListFlag(pn, PNX_DESTRUCT | PNX_NONCONST);
                PropertyName *name = handler.isName(pn3);
                JS_ASSERT(atom);
                pn3 = handler.newName(name, pc);
                if (!pn3)
                    return null();
                pnval = pn3;
            }
#endif
            else {
                report(ParseError, false, null(), JSMSG_COLON_AFTER_ID);
                return null();
            }

            pn2 = handler.newBinary(PNK_COLON, pn3, pnval, op);
          skip:
            if (!pn2)
                return null();
            handler.addList(pn, pn2);

            





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
                    (oldAssignType != VALUE || assignType != VALUE || pc->sc->needStrictChecks()))
                {
                    JSAutoByteString name;
                    if (!js_AtomToPrintableString(context, atom, &name))
                        return null();

                    ParseReportKind reportKind =
                        (oldAssignType == VALUE && assignType == VALUE && !pc->sc->needStrictChecks())
                        ? ParseWarning
                        : (pc->sc->needStrictChecks() ? ParseStrictError : ParseError);
                    if (!report(reportKind, pc->sc->strict, null(),
                                JSMSG_DUPLICATE_PROPERTY, name.ptr()))
                    {
                        return null();
                    }
                }
                p.value() = assignType | oldAssignType;
            } else {
                if (!seen.add(p, atom, assignType))
                    return null();
            }

            tt = tokenStream.getToken();
            if (tt == TOK_RC)
                goto end_obj_init;
            if (tt != TOK_COMMA) {
                report(ParseError, false, null(), JSMSG_CURLY_AFTER_LIST);
                return null();
            }
        }

      end_obj_init:
        handler.setEndPosition(pn, tokenStream.currentToken().pos.end);
        return pn;
      }

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        pn = letBlock(LetExpresion);
        if (!pn)
            return null();
        break;
#endif

      case TOK_LP:
      {
        bool genexp;

        pn = parenExpr(&genexp);
        if (!pn)
            return null();
        pn = handler.setInParens(pn);

        if (!genexp)
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);
        break;
      }

      case TOK_STRING:
        pn = atomNode(PNK_STRING, JSOP_STRING);
        if (!pn)
            return null();
        break;

      case TOK_NAME:
        pn = identifierName();
        break;

      case TOK_REGEXP:
        pn = newRegExp(tokenStream.getTokenbuf().begin(),
                       tokenStream.getTokenbuf().length(),
                       tokenStream.currentToken().regExpFlags());
        break;

      case TOK_NUMBER:
        pn = handler.newNumber(tokenStream.currentToken());
        break;

      case TOK_TRUE:
        return handler.newBooleanLiteral(true, tokenStream.currentToken().pos);
      case TOK_FALSE:
        return handler.newBooleanLiteral(false, tokenStream.currentToken().pos);
      case TOK_THIS:
        return handler.newThisLiteral(tokenStream.currentToken().pos);
      case TOK_NULL:
        return handler.newNullLiteral(tokenStream.currentToken().pos);

      case TOK_RP:
        
        
        if (tokenStream.peekToken() == TOK_ARROW) {
            tokenStream.ungetToken();  

            
            
            
            return handler.newNullLiteral(tokenStream.currentToken().pos);
        }
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();

      case TOK_TRIPLEDOT:
        
        
        if (tokenStream.matchToken(TOK_NAME) &&
            tokenStream.matchToken(TOK_RP) &&
            tokenStream.peekToken() == TOK_ARROW)
        {
            tokenStream.ungetToken();  

            
            return handler.newNullLiteral(tokenStream.currentToken().pos);
        }
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();

      case TOK_ERROR:
        
        return null();

      default:
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();
    }
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::parenExpr(bool *genexp)
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_LP);
    uint32_t begin = tokenStream.currentToken().pos.begin;

    if (genexp)
        *genexp = false;

    GenexpGuard<ParseHandler> guard(this);

    Node pn = bracketedExpr();
    if (!pn)
        return null();
    guard.endBody();

#if JS_HAS_GENERATOR_EXPRS
    if (tokenStream.matchToken(TOK_FOR)) {
        if (!guard.checkValidBody(pn))
            return null();
        if (handler.isOperationWithoutParens(pn, PNK_COMMA)) {
            report(ParseError, false, null(),
                   JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
            return null();
        }
        pn = generatorExpr(pn);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        if (genexp) {
            if (tokenStream.getToken() != TOK_RP) {
                report(ParseError, false, null(),
                       JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
                return null();
            }
            handler.setEndPosition(pn, tokenStream.currentToken().pos.end);
            *genexp = true;
        }
    } else
#endif 

    if (!guard.maybeNoteGenerator(pn))
        return null();

    return pn;
}

template class Parser<FullParseHandler>;
template class Parser<SyntaxParseHandler>;

} 
} 
