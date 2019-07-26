


















#include "frontend/Parser.h"

#include "jstypes.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/FoldConstants.h"
#include "frontend/ParseMaps.h"
#include "frontend/TokenStream.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsfuninlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/SharedContext-inl.h"

#include "vm/NumericConversions.h"
#include "vm/RegExpStatics-inl.h"

using namespace js;
using namespace js::gc;
using mozilla::Maybe;

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
        if (!pc->sc->context->isJSContext())
            return false;
        JS_ReportErrorNumber(pc->sc->context->asJSContext(),
                             js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
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
ParseContext<FullParseHandler>::define(TokenStream &ts,
                                       PropertyName *name, ParseNode *pn, Definition::Kind kind)
{
    JS_ASSERT(!pn->isUsed());
    JS_ASSERT_IF(pn->isDefn(), pn->isPlaceholder());

    Definition *prevDef = NULL;
    if (kind == Definition::LET)
        prevDef = decls_.lookupFirst(name);
    else
        JS_ASSERT(!decls_.lookupFirst(name));

    if (!prevDef)
        prevDef = lexdeps.lookupDefn<FullParseHandler>(name);

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
        if (!dn->pn_cookie.set(ts, staticLevel, args_.length()))
            return false;
        if (!args_.append(dn))
            return false;
        if (name == ts.names().empty)
            break;
        if (!decls_.addUnique(name, dn))
            return false;
        break;

      case Definition::CONST:
      case Definition::VAR:
        if (sc->isFunctionBox()) {
            dn->setOp(JSOP_GETLOCAL);
            dn->pn_dflags |= PND_BOUND;
            if (!dn->pn_cookie.set(ts, staticLevel, vars_.length()))
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

      default:
        MOZ_ASSUME_UNREACHABLE("unexpected kind");
    }

    return true;
}

template <>
bool
ParseContext<SyntaxParseHandler>::define(TokenStream &ts, PropertyName *name, Node pn,
                                         Definition::Kind kind)
{
    JS_ASSERT(!decls_.lookupFirst(name));

    if (lexdeps.lookupDefn<SyntaxParseHandler>(name))
        lexdeps->remove(name);

    
    if (kind == Definition::ARG && !args_.append((Definition *) NULL))
        return false;

    return decls_.addUnique(name, kind);
}

template <typename ParseHandler>
void
ParseContext<ParseHandler>::prepareToAddDuplicateArg(HandlePropertyName name, DefinitionNode prevDecl)
{
    JS_ASSERT(decls_.lookupFirst(name) == prevDecl);
    decls_.remove(name);
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
    JS_ASSERT(ParseHandler::getDefinitionKind(decls_.lookupFirst(atom)) == Definition::LET);
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
          default:
            MOZ_ASSUME_UNREACHABLE("unexpected dn->kind");
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
ParseContext<ParseHandler>::generateFunctionBindings(ExclusiveContext *cx, LifoAlloc &alloc,
                                                     InternalHandle<Bindings*> bindings) const
{
    JS_ASSERT(sc->isFunctionBox());

    unsigned count = args_.length() + vars_.length();
    Binding *packedBindings = alloc.newArrayUninitialized<Binding>(count);
    if (!packedBindings) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    AppendPackedBindings(this, args_, packedBindings);
    AppendPackedBindings(this, vars_, packedBindings + args_.length());

    return Bindings::initWithTemporaryStorage(cx, bindings, args_.length(), vars_.length(),
                                              packedBindings);
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::reportHelper(ParseReportKind kind, bool strict, uint32_t offset,
                             unsigned errorNumber, va_list args)
{
    bool result = false;
    switch (kind) {
      case ParseError:
        result = tokenStream.reportCompileErrorNumberVA(offset, JSREPORT_ERROR, errorNumber, args);
        break;
      case ParseWarning:
        result =
            tokenStream.reportCompileErrorNumberVA(offset, JSREPORT_WARNING, errorNumber, args);
        break;
      case ParseExtraWarning:
        result = tokenStream.reportStrictWarningErrorNumberVA(offset, errorNumber, args);
        break;
      case ParseStrictError:
        result = tokenStream.reportStrictModeErrorNumberVA(offset, strict, errorNumber, args);
        break;
    }
    return result;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::report(ParseReportKind kind, bool strict, Node pn, unsigned errorNumber, ...)
{
    uint32_t offset = (pn ? handler.getPosition(pn) : pos()).begin;

    va_list args;
    va_start(args, errorNumber);
    bool result = reportHelper(kind, strict, offset, errorNumber, args);
    va_end(args);
    return result;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::reportWithOffset(ParseReportKind kind, bool strict, uint32_t offset,
                                       unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = reportHelper(kind, strict, offset, errorNumber, args);
    va_end(args);
    return result;
}

template <>
bool
Parser<FullParseHandler>::abortIfSyntaxParser()
{
    handler.disableSyntaxParser();
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::abortIfSyntaxParser()
{
    abortedSyntaxParse = true;
    return false;
}

template <typename ParseHandler>
Parser<ParseHandler>::Parser(ExclusiveContext *cx, LifoAlloc *alloc,
                             const CompileOptions &options,
                             const jschar *chars, size_t length, bool foldConstants,
                             Parser<SyntaxParseHandler> *syntaxParser,
                             LazyScript *lazyOuterFunction)
  : AutoGCRooter(cx, PARSER),
    context(cx),
    alloc(*alloc),
    tokenStream(cx, options, chars, length, thisForCtor(), keepAtoms),
    traceListHead(NULL),
    pc(NULL),
    sct(NULL),
    keepAtoms(cx->asJSContext()->runtime()),
    foldConstants(foldConstants),
    abortedSyntaxParse(false),
    handler(cx, *alloc, tokenStream, foldConstants, syntaxParser, lazyOuterFunction)
{
    cx->asJSContext()->runtime()->activeCompilations++;

    
    
    
    if (options.extraWarningsOption)
        handler.disableSyntaxParser();

    tempPoolMark = alloc->mark();
}

template <typename ParseHandler>
Parser<ParseHandler>::~Parser()
{
    context->asJSContext()->runtime()->activeCompilations--;

    alloc.release(tempPoolMark);

    




    alloc.freeAllIfHugeAndUnused();
}

template <typename ParseHandler>
ObjectBox *
Parser<ParseHandler>::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj && !IsPoisonedPtr(obj));

    







    ObjectBox *objbox = alloc.new_<ObjectBox>(obj, traceListHead);
    if (!objbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = objbox;

    return objbox;
}

template <typename ParseHandler>
FunctionBox::FunctionBox(ExclusiveContext *cx, ObjectBox* traceListHead, JSFunction *fun,
                         ParseContext<ParseHandler> *outerpc, bool strict, bool extraWarnings)
  : ObjectBox(fun, traceListHead),
    SharedContext(cx, strict, extraWarnings),
    bindings(),
    bufStart(0),
    bufEnd(0),
    asmStart(0),
    ndefaults(0),
    inWith(false),                  
    inGenexpLambda(false),
    useAsm(false),
    insideUseAsm(outerpc && outerpc->useAsmOrInsideUseAsm()),
    usesArguments(false),
    usesApply(false),
    funCxFlags()
{
    JS_ASSERT(fun->isTenured());

    if (!outerpc) {
        inWith = false;

    } else if (outerpc->parsingWith) {
        
        
        
        
        
        
        inWith = true;

    } else if (outerpc->sc->isGlobalSharedContext()) {
        
        
        
        
        
        
        
        
        
        
        JSObject *scope = outerpc->sc->asGlobalSharedContext()->scopeChain();
        while (scope) {
            if (scope->is<WithObject>())
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
        alloc.new_<FunctionBox>(context, traceListHead, fun, outerpc,
                                strict, options().extraWarningsOption);
    if (!funbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }

    traceListHead = funbox;

    return funbox;
}

ModuleBox::ModuleBox(ExclusiveContext *cx, ObjectBox *traceListHead, Module *module,
                     ParseContext<FullParseHandler> *pc, bool extraWarnings)
  : ObjectBox(module, traceListHead),
    SharedContext(cx, true, extraWarnings)
{
}

template <>
ModuleBox *
Parser<FullParseHandler>::newModuleBox(Module *module, ParseContext<FullParseHandler> *outerpc)
{
    JS_ASSERT(module && !IsPoisonedPtr(module));

    






    ModuleBox *modulebox =
        alloc.new_<ModuleBox>(context, traceListHead, module, outerpc,
                              options().extraWarningsOption);
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

void
MarkParser(JSTracer *trc, AutoGCRooter *parser)
{
    static_cast<Parser<FullParseHandler> *>(parser)->trace(trc);
}




template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::parse(JSObject *chain)
{
    







    GlobalSharedContext globalsc(context, chain,
                                 options().strictOption, options().extraWarningsOption);
    ParseContext<ParseHandler> globalpc(this, NULL, &globalsc,  0,  0);
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
        if (!AtomToPrintableString(context, atom, &name))
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
           reportBadReturn(pn, ParseExtraWarning,
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
        if (!AtomToPrintableString(context, atom, &name) ||
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

    if (name == context->names().eval || name == context->names().arguments || IsKeyword(name)) {
        JSAutoByteString bytes;
        if (!AtomToPrintableString(context, name, &bytes))
            return false;
        return report(ParseStrictError, pc->sc->strict, pn,
                      JSMSG_BAD_BINDING, bytes.ptr());
    }

    return true;
}

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

    ParseContext<FullParseHandler> funpc(this, pc, *funbox,  0,  0);
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

    InternalHandle<Bindings*> scriptBindings(script, &script->bindings);
    if (!funpc.generateFunctionBindings(context, alloc, scriptBindings))
        return null();

    
    
    InternalHandle<Bindings*> funboxBindings =
        InternalHandle<Bindings*>::fromMarkedLocation(&(*funbox)->bindings);
    if (!funpc.generateFunctionBindings(context, alloc, funboxBindings))
        return null();

    return pn;
}

template <>
bool
Parser<FullParseHandler>::checkFunctionArguments()
{
    



    if (FuncStmtSet *set = pc->funcStmts) {
        for (FuncStmtSet::Range r = set->all(); !r.empty(); r.popFront()) {
            PropertyName *name = r.front()->asPropertyName();
            if (Definition *dn = pc->decls().lookupFirst(name))
                dn->pn_dflags |= PND_CLOSED;
        }
    }

    
    HandlePropertyName arguments = context->names().arguments;

    




    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            Definition *dn = r.front().value().get<FullParseHandler>();
            pc->lexdeps->remove(arguments);
            dn->pn_dflags |= PND_IMPLICITARGUMENTS;
            if (!pc->define(tokenStream, arguments, dn, Definition::VAR))
                return false;
            pc->sc->asFunctionBox()->usesArguments = true;
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
        ParseNode *pn = newName(arguments);
        if (!pn)
            return false;
        if (!pc->define(tokenStream, arguments, pn, Definition::VAR))
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
                    Definition *dn = dr.front<FullParseHandler>();
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
    bool hasRest = pc->sc->asFunctionBox()->function()->hasRest();

    if (pc->lexdeps->lookup(context->names().arguments)) {
        pc->sc->asFunctionBox()->usesArguments = true;
        if (hasRest) {
            report(ParseError, false, null(), JSMSG_ARGUMENTS_AND_REST);
            return false;
        }
    } else if (hasRest) {
        DefinitionNode maybeArgDef = pc->decls().lookupFirst(context->names().arguments);
        if (maybeArgDef && handler.getDefinitionKind(maybeArgDef) != Definition::ARG) {
            report(ParseError, false, null(), JSMSG_ARGUMENTS_AND_REST);
            return false;
        }
    }

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

        pn = handler.newReturnStatement(kid, handler.getPosition(kid));
        if (!pn)
            return null();

        if (pc->sc->asFunctionBox()->isGenerator()) {
            reportBadReturn(pn, ParseError,
                            JSMSG_BAD_GENERATOR_RETURN,
                            JSMSG_BAD_ANON_GENERATOR_RETURN);
            return null();
        }
    }

    
    if (options().extraWarningsOption && pc->funHasReturnExpr && !checkFinalReturn(pn))
        return null();

    
    if (!checkFunctionArguments())
        return null();

    return pn;
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
    BindData(ExclusiveContext *cx) : let(cx) {}

    typedef bool
    (*Binder)(BindData *data, HandlePropertyName name, Parser<ParseHandler> *parser);

    
    typename ParseHandler::Node pn;

    JSOp            op;         
    Binder          binder;     

    struct LetData {
        LetData(ExclusiveContext *cx) : blockObj(cx) {}
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
Parser<ParseHandler>::newFunction(GenericParseContext *pc, HandleAtom atom,
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
    if (options().selfHostingMode)
        fun->setIsSelfHostedBuiltin();
    if (fun && !options().compileAndGo) {
        if (!JSObject::clearParent(context->asJSContext(), fun))
            return NULL;
        if (!JSObject::clearType(context->asJSContext(), fun))
            return NULL;
        fun->setEnvironment(NULL);
    }
    return fun;
}

static bool
MatchOrInsertSemicolon(TokenStream &ts)
{
    TokenKind tt = ts.peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return false;
    if (tt != TOK_EOF && tt != TOK_EOL && tt != TOK_SEMI && tt != TOK_RC) {
        
        ts.getToken(TSF_OPERAND);
        ts.reportError(JSMSG_SEMI_BEFORE_STMNT);
        return false;
    }
    (void) ts.matchToken(TOK_SEMI);
    return true;
}

template <typename ParseHandler>
typename ParseHandler::DefinitionNode
Parser<ParseHandler>::getOrCreateLexicalDependency(ParseContext<ParseHandler> *pc, JSAtom *atom)
{
    AtomDefnAddPtr p = pc->lexdeps->lookupForAdd(atom);
    if (p)
        return p.value().get<ParseHandler>();

    DefinitionNode dn = handler.newPlaceholder(atom, pc->inBlock(), pc->blockid(), pos());
    if (!dn)
        return ParseHandler::nullDefinition();
    DefinitionSingle def = DefinitionSingle::new_<ParseHandler>(dn);
    if (!pc->lexdeps->add(p, atom, def))
        return ParseHandler::nullDefinition();
    return dn;
}

static bool
ConvertDefinitionToNamedLambdaUse(TokenStream &ts, ParseContext<FullParseHandler> *pc,
                                  FunctionBox *funbox, Definition *dn)
{
    dn->setOp(JSOP_CALLEE);
    if (!dn->pn_cookie.set(ts, pc->staticLevel, UpvarCookie::CALLEE_SLOT))
        return false;
    dn->pn_dflags |= PND_BOUND;
    JS_ASSERT(dn->kind() == Definition::NAMED_LAMBDA);

    










    if (dn->isClosed() || dn->isAssigned())
        funbox->setNeedsDeclEnvObject();
    return true;
}







template <>
bool
Parser<FullParseHandler>::leaveFunction(ParseNode *fn, HandlePropertyName funName,
                                        ParseContext<FullParseHandler> *outerpc,
                                        FunctionSyntaxKind kind)
{
    outerpc->blockidGen = pc->blockidGen;

    FunctionBox *funbox = fn->pn_funbox;
    JS_ASSERT(funbox == pc->sc->asFunctionBox());

    if (!outerpc->topStmt || outerpc->topStmt->type == STMT_BLOCK)
        fn->pn_dflags |= PND_BLOCKCHILD;

    
    if (pc->lexdeps->count()) {
        for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key();
            Definition *dn = r.front().value().get<FullParseHandler>();
            JS_ASSERT(dn->isPlaceholder());

            if (atom == funName && kind == Expression) {
                if (!ConvertDefinitionToNamedLambdaUse(tokenStream, pc, funbox, dn))
                    return false;
                continue;
            }

            Definition *outer_dn = outerpc->decls().lookupFirst(atom);

            




            if (funbox->hasExtensibleScope() || outerpc->parsingWith)
                handler.deoptimizeUsesWithin(dn, fn->pn_pos);

            if (!outer_dn) {
                




















                outer_dn = getOrCreateLexicalDependency(outerpc, atom);
                if (!outer_dn)
                    return false;
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
    return pc->generateFunctionBindings(context, alloc, bindings);
}

template <>
bool
Parser<SyntaxParseHandler>::leaveFunction(Node fn, HandlePropertyName funName,
                                          ParseContext<SyntaxParseHandler> *outerpc,
                                          FunctionSyntaxKind kind)
{
    outerpc->blockidGen = pc->blockidGen;

    FunctionBox *funbox = pc->sc->asFunctionBox();
    return addFreeVariablesFromLazyFunction(funbox->function(), outerpc);
}















template <typename ParseHandler>
bool
Parser<ParseHandler>::defineArg(Node funcpn, HandlePropertyName name,
                                bool disallowDuplicateArgs, Node *duplicatedArg)
{
    SharedContext *sc = pc->sc;

    
    if (DefinitionNode prevDecl = pc->decls().lookupFirst(name)) {
        Node pn = handler.getDefinitionNode(prevDecl);

        





        if (sc->needStrictChecks()) {
            JSAutoByteString bytes;
            if (!AtomToPrintableString(context, name, &bytes))
                return false;
            if (!report(ParseStrictError, pc->sc->strict, pn,
                        JSMSG_DUPLICATE_FORMAL, bytes.ptr()))
            {
                return false;
            }
        }

        if (disallowDuplicateArgs) {
            report(ParseError, false, pn, JSMSG_BAD_DUP_ARGS);
            return false;
        }

        if (duplicatedArg)
            *duplicatedArg = pn;

        
        JS_ASSERT(handler.getDefinitionKind(prevDecl) == Definition::ARG);
        pc->prepareToAddDuplicateArg(name, prevDecl);
    }

    Node argpn = newName(name);
    if (!argpn)
        return false;

    if (!checkStrictBinding(name, argpn))
        return false;

    handler.addFunctionArgument(funcpn, argpn);
    return pc->define(tokenStream, name, argpn, Definition::ARG);
}

#if JS_HAS_DESTRUCTURING
template <typename ParseHandler>
 bool
Parser<ParseHandler>::bindDestructuringArg(BindData<ParseHandler> *data,
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

    return pc->define(parser->tokenStream, name, data->pn, Definition::VAR);
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

        
        
        funbox->setStart(tokenStream);
    }

    hasRest = false;

    Node argsbody = handler.newList(PNK_ARGSBODY);
    if (!argsbody)
        return false;
    handler.setFunctionBody(funcpn, argsbody);

    if (parenFreeArrow || !tokenStream.matchToken(TOK_RP)) {
        bool hasDefaults = false;
        Node duplicatedArg = null();
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
                Node rhs = newName(name);
                if (!rhs)
                    return false;

                if (!pc->define(tokenStream, name, rhs, Definition::ARG))
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
                    funbox->setStart(tokenStream);

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
                                                  ParseNode **pn_, FunctionSyntaxKind kind,
                                                  bool *pbodyProcessed)
{
    ParseNode *&pn = *pn_;
    *pbodyProcessed = false;

    
    bool bodyLevel = pc->atBodyLevel();

    if (kind == Statement) {
        



        if (Definition *dn = pc->decls().lookupFirst(funName)) {
            JS_ASSERT(!dn->isUsed());
            JS_ASSERT(dn->isDefn());

            if (options().extraWarningsOption || dn->kind() == Definition::CONST) {
                JSAutoByteString name;
                ParseReportKind reporter = (dn->kind() != Definition::CONST)
                                           ? ParseExtraWarning
                                           : ParseError;
                if (!AtomToPrintableString(context, funName, &name) ||
                    !report(reporter, false, NULL, JSMSG_REDECLARED_VAR,
                            Definition::kindString(dn->kind()), name.ptr()))
                {
                    return false;
                }
            }

            







            if (bodyLevel && !makeDefIntoUse(dn, pn, funName))
                return false;
        } else if (bodyLevel) {
            




            if (Definition *fn = pc->lexdeps.lookupDefn<FullParseHandler>(funName)) {
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

            if (!pc->define(tokenStream, funName, pn, Definition::VAR))
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

            





            if (funName == context->names().arguments)
                pc->sc->setBindingsAccessedDynamically();
        }

        
        pn->pn_dflags |= PND_BOUND;
    } else {
        
        pn->setOp(JSOP_LAMBDA);
    }

    
    
    
    
    if (LazyScript *lazyOuter = handler.lazyOuterFunction()) {
        JSFunction *fun = handler.nextLazyInnerFunction();
        FunctionBox *funbox = newFunctionBox(fun, pc,  false);
        if (!funbox)
            return false;
        handler.setFunctionBox(pn, funbox);

        if (!addFreeVariablesFromLazyFunction(fun, pc))
            return false;

        
        
        
        
        
        uint32_t userbufBase = lazyOuter->begin() - lazyOuter->column();
        tokenStream.advance(fun->lazyScript()->end() - userbufBase);

        *pbodyProcessed = true;
        return true;
    }

    return true;
}

template <class T, class U>
static inline void
PropagateTransitiveParseFlags(const T *inner, U *outer)
{
   if (inner->bindingsAccessedDynamically())
     outer->setBindingsAccessedDynamically();
   if (inner->hasDebuggerStatement())
     outer->setHasDebuggerStatement();
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::addFreeVariablesFromLazyFunction(JSFunction *fun,
                                                       ParseContext<ParseHandler> *pc)
{
    
    

    LazyScript *lazy = fun->lazyScript();
    HeapPtrAtom *freeVariables = lazy->freeVariables();
    for (size_t i = 0; i < lazy->numFreeVariables(); i++) {
        JSAtom *atom = freeVariables[i];

        
        if (atom == context->names().arguments)
            continue;

        DefinitionNode dn = pc->decls().lookupFirst(atom);

        if (!dn) {
            dn = getOrCreateLexicalDependency(pc, atom);
            if (!dn)
                return false;
        }

        
        handler.setFlag(handler.getDefinitionNode(dn), PND_CLOSED);
    }

    PropagateTransitiveParseFlags(lazy, pc->sc);
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkFunctionDefinition(HandlePropertyName funName,
                                                    Node *pn, FunctionSyntaxKind kind,
                                                    bool *pbodyProcessed)
{
    *pbodyProcessed = false;

    
    bool bodyLevel = pc->atBodyLevel();

    if (kind == Statement) {
        



        if (DefinitionNode dn = pc->decls().lookupFirst(funName)) {
            if (dn == Definition::CONST) {
                JSAutoByteString name;
                if (!AtomToPrintableString(context, funName, &name) ||
                    !report(ParseError, false, null(), JSMSG_REDECLARED_VAR,
                            Definition::kindString(dn), name.ptr()))
                {
                    return false;
                }
            }
        } else if (bodyLevel) {
            if (pc->lexdeps.lookupDefn<SyntaxParseHandler>(funName))
                pc->lexdeps->remove(funName);

            if (!pc->define(tokenStream, funName, *pn, Definition::VAR))
                return false;
        }

        if (!bodyLevel && funName == context->names().arguments)
            pc->sc->setBindingsAccessedDynamically();
    }

    if (kind == Arrow) {
        
        return abortIfSyntaxParser();
    }

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

    bool bodyProcessed;
    if (!checkFunctionDefinition(funName, &pn, kind, &bodyProcessed))
        return null();

    if (bodyProcessed)
        return pn;

    RootedFunction fun(context, newFunction(pc, funName, kind));
    if (!fun)
        return null();

    
    
    
    handler.setFunctionBody(pn, null());
    bool initiallyStrict = pc->sc->strict;
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
                                                   ParseNode *prelude, ParseNode *body)
{
    pn->pn_pos.end = pos().end;

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

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::finishFunctionDefinition(Node pn, FunctionBox *funbox,
                                                     Node prelude, Node body)
{
    
    
    

    if (funbox->inWith)
        return abortIfSyntaxParser();

    size_t numFreeVariables = pc->lexdeps->count();
    size_t numInnerFunctions = pc->innerFunctions.length();

    LazyScript *lazy = LazyScript::Create(context, numFreeVariables, numInnerFunctions, versionNumber(),
                                          funbox->bufStart, funbox->bufEnd,
                                          funbox->startLine, funbox->startColumn);
    if (!lazy)
        return false;

    HeapPtrAtom *freeVariables = lazy->freeVariables();
    size_t i = 0;
    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront())
        freeVariables[i++].init(r.front().key());
    JS_ASSERT(i == numFreeVariables);

    HeapPtrFunction *innerFunctions = lazy->innerFunctions();
    for (size_t i = 0; i < numInnerFunctions; i++)
        innerFunctions[i].init(pc->innerFunctions[i]);

    if (pc->sc->strict)
        lazy->setStrict();
    if (funbox->usesArguments && funbox->usesApply)
        lazy->setUsesArgumentsAndApply();
    PropagateTransitiveParseFlags(funbox, lazy);

    funbox->object->as<JSFunction>().initLazyScript(lazy);
    return true;
}

template <>
bool
Parser<FullParseHandler>::functionArgsAndBody(ParseNode *pn, HandleFunction fun,
                                              HandlePropertyName funName,
                                              size_t startOffset, FunctionType type,
                                              FunctionSyntaxKind kind,
                                              bool strict, bool *becameStrict)
{
    if (becameStrict)
        *becameStrict = false;
    ParseContext<FullParseHandler> *outerpc = pc;

    
    FunctionBox *funbox = newFunctionBox(fun, pc, strict);
    if (!funbox)
        return false;

    
    do {
        Parser<SyntaxParseHandler> *parser = handler.syntaxParser;
        if (!parser)
            break;

        {
            
            TokenStream::Position position(keepAtoms);
            tokenStream.tell(&position);
            parser->tokenStream.seek(position, tokenStream);

            ParseContext<SyntaxParseHandler> funpc(parser, outerpc, funbox,
                                                   outerpc->staticLevel + 1, outerpc->blockidGen);
            if (!funpc.init())
                return false;

            if (!parser->functionArgsAndBodyGeneric(SyntaxParseHandler::NodeGeneric,
                                                    fun, funName, type, kind, strict, becameStrict))
            {
                if (parser->hadAbortedSyntaxParse()) {
                    
                    parser->clearAbortedSyntaxParse();
                    break;
                }
                return false;
            }

            outerpc->blockidGen = funpc.blockidGen;

            
            parser->tokenStream.tell(&position);
            tokenStream.seek(position, parser->tokenStream);
        }

        pn->pn_funbox = funbox;

        if (!addFreeVariablesFromLazyFunction(fun, pc))
            return false;

        pn->pn_blockid = outerpc->blockid();
        PropagateTransitiveParseFlags(funbox, outerpc->sc);
        return true;
    } while (false);

    
    ParseContext<FullParseHandler> funpc(this, pc, funbox,
                                         outerpc->staticLevel + 1, outerpc->blockidGen);
    if (!funpc.init())
        return false;

    if (!functionArgsAndBodyGeneric(pn, fun, funName, type, kind, strict, becameStrict))
        return false;

    if (!leaveFunction(pn, funName, outerpc, kind))
        return false;

    pn->pn_blockid = outerpc->blockid();

    





    PropagateTransitiveParseFlags(funbox, outerpc->sc);
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::functionArgsAndBody(Node pn, HandleFunction fun,
                                                HandlePropertyName funName,
                                                size_t startOffset, FunctionType type,
                                                FunctionSyntaxKind kind,
                                                bool strict, bool *becameStrict)
{
    if (becameStrict)
        *becameStrict = false;
    ParseContext<SyntaxParseHandler> *outerpc = pc;

    
    FunctionBox *funbox = newFunctionBox(fun, pc, strict);
    if (!funbox)
        return false;

    
    ParseContext<SyntaxParseHandler> funpc(this, pc, funbox,
                                           outerpc->staticLevel + 1, outerpc->blockidGen);
    if (!funpc.init())
        return false;

    if (!functionArgsAndBodyGeneric(pn, fun, funName, type, kind, strict, becameStrict))
        return false;

    if (!leaveFunction(pn, funName, outerpc, kind))
        return false;

    
    
    
    JS_ASSERT(fun->lazyScript());
    return outerpc->innerFunctions.append(fun);
}

template <>
ParseNode *
Parser<FullParseHandler>::standaloneLazyFunction(HandleFunction fun, unsigned staticLevel,
                                                 bool strict)
{
    Node pn = handler.newFunctionDefinition();
    if (!pn)
        return null();

    FunctionBox *funbox = newFunctionBox(fun,  NULL, strict);
    if (!funbox)
        return null();
    handler.setFunctionBox(pn, funbox);

    ParseContext<FullParseHandler> funpc(this, NULL, funbox, staticLevel, 0);
    if (!funpc.init())
        return null();

    RootedPropertyName funName(context, fun->atom() ? fun->atom()->asPropertyName() : NULL);

    if (!functionArgsAndBodyGeneric(pn, fun, funName, Normal, Statement, strict, NULL))
        return null();

    if (fun->isNamedLambda()) {
        if (AtomDefnPtr p = pc->lexdeps->lookup(funName)) {
            Definition *dn = p.value().get<FullParseHandler>();
            if (!ConvertDefinitionToNamedLambdaUse(tokenStream, pc, funbox, dn))
                return NULL;
        }
    }

    InternalHandle<Bindings*> bindings =
        InternalHandle<Bindings*>::fromMarkedLocation(&funbox->bindings);
    if (!pc->generateFunctionBindings(context, alloc, bindings))
        return null();

    return pn;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::functionArgsAndBodyGeneric(Node pn, HandleFunction fun,
                                                 HandlePropertyName funName, FunctionType type,
                                                 FunctionSyntaxKind kind,
                                                 bool strict, bool *becameStrict)
{
    
    
    

    Node prelude = null();
    bool hasRest;
    if (!functionArguments(kind, &prelude, pn, hasRest))
        return false;

    FunctionBox *funbox = pc->sc->asFunctionBox();

    fun->setArgCount(pc->numArgs());
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

    
    Maybe<GenexpGuard<ParseHandler> > yieldGuard;
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
        funbox->bufEnd = pos().begin + 1;
#if JS_HAS_EXPR_CLOSURES
    } else {
        if (tokenStream.hadError())
            return false;
        funbox->bufEnd = pos().end;
        if (kind == Statement && !MatchOrInsertSemicolon(tokenStream))
            return false;
    }
#endif

    return finishFunctionDefinition(pn, funbox, prelude, body);
}

template <>
ParseNode *
Parser<FullParseHandler>::moduleDecl()
{
    JS_ASSERT(tokenStream.currentToken().name() == context->names().module);
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
    Module *module = Module::create(context, atom);
    if (!module)
        return NULL;
    ModuleBox *modulebox = newModuleBox(module, pc);
    if (!modulebox)
        return NULL;
    pn->pn_modulebox = modulebox;

    ParseContext<FullParseHandler> modulepc(this, pc, modulebox, pc->staticLevel + 1, pc->blockidGen);
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
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionStmt()
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_FUNCTION);

    TokenStream::Position start(keepAtoms);
    tokenStream.tell(&start);

    RootedPropertyName name(context);
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME) {
        name = tokenStream.currentToken().name();
    } else {
        
        report(ParseError, false, null(), JSMSG_UNNAMED_FUNCTION_STMT);
        return null();
    }

    
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
    TokenStream::Position start(keepAtoms);
    tokenStream.tell(&start);
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

        if (directive == context->names().useStrict) {
            
            
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
                if (!abortIfSyntaxParser())
                    return false;
            } else {
                if (!report(ParseWarning, false, pn, JSMSG_USE_ASM_DIRECTIVE_FAIL))
                    return false;
            }
        }
    }
    return true;
}






template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::statements()
{
    JS_CHECK_RECURSION(context, return null());

    Node pn = handler.newStatementList(pc->blockid(), pos());
    if (!pn)
        return null();

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
        Node next = statement(canHaveDirectives);
        if (!next) {
            if (tokenStream.isEOF())
                tokenStream.setUnexpectedEOF();
            return null();
        }

        if (canHaveDirectives) {
            if (!maybeParseDirective(next, &canHaveDirectives))
                return null();
        }

        handler.addStatementToList(pn, next, pc);
    }

    




    if (pc->blockNode != pn)
        pn = pc->blockNode;
    pc->blockNode = saveBlock;
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
        !report(ParseExtraWarning, false, null(), JSMSG_EQUAL_AS_ASSIGN))
    {
        return null();
    }
    return pn;
}

static bool
MatchLabel(TokenStream &ts, MutableHandlePropertyName label)
{
    TokenKind tt = ts.peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return false;
    if (tt == TOK_NAME) {
        (void) ts.getToken();
        label.set(ts.currentToken().name());
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
    if (AtomToPrintableString(context, atom, &name))
        report(ParseError, false, pn, JSMSG_REDECLARED_VAR, isConst ? "const" : "variable", name.ptr());
    return false;
}










template <>
 bool
Parser<FullParseHandler>::bindLet(BindData<FullParseHandler> *data,
                                  HandlePropertyName name, Parser<FullParseHandler> *parser)
{
    ParseContext<FullParseHandler> *pc = parser->pc;
    ParseNode *pn = data->pn;
    if (!parser->checkStrictBinding(name, pn))
        return false;

    ExclusiveContext *cx = parser->context;

    Rooted<StaticBlockObject *> blockObj(cx, data->let.blockObj);
    unsigned blockCount = blockObj->slotCount();
    if (blockCount == JS_BIT(16)) {
        parser->report(ParseError, false, pn, data->let.overflow);
        return false;
    }

    






    if (!pn->pn_cookie.set(parser->tokenStream, pc->staticLevel, uint16_t(blockCount)))
        return false;

    



    if (data->let.varContext == HoistVars) {
        JS_ASSERT(!pc->atBodyLevel());
        Definition *dn = pc->decls().lookupFirst(name);
        if (dn && dn->pn_blockid == pc->blockid())
            return parser->reportRedeclaration(pn, dn->isConst(), name);
        if (!pc->define(parser->tokenStream, name, pn, Definition::LET))
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
Parser<SyntaxParseHandler>::bindLet(BindData<SyntaxParseHandler> *data,
                                    HandlePropertyName name, Parser<SyntaxParseHandler> *parser)
{
    return true;
}

template <typename ParseHandler, class Op>
static inline bool
ForEachLetDef(TokenStream &ts, ParseContext<ParseHandler> *pc,
              HandleStaticBlockObject blockObj, Op op)
{
    for (Shape::Range<CanGC> r(ts.context(), blockObj->lastProperty()); !r.empty(); r.popFront()) {
        Shape &shape = r.front();

        
        if (JSID_IS_INT(shape.propid()))
            continue;

        if (!op(ts, pc, blockObj, shape, JSID_TO_ATOM(shape.propid())))
            return false;
    }
    return true;
}

template <typename ParseHandler>
struct PopLetDecl {
    bool operator()(TokenStream &, ParseContext<ParseHandler> *pc, HandleStaticBlockObject,
                    const Shape &, JSAtom *atom)
    {
        pc->popLetDecl(atom);
        return true;
    }
};

template <typename ParseHandler>
static void
PopStatementPC(TokenStream &ts, ParseContext<ParseHandler> *pc)
{
    RootedStaticBlockObject blockObj(ts.context(), pc->topStmt->blockObj);
    JS_ASSERT(!!blockObj == (pc->topStmt->isBlockScope));

    FinishPopStatement(pc);

    if (blockObj) {
        JS_ASSERT(!blockObj->inDictionaryMode());
        ForEachLetDef(ts, pc, blockObj, PopLetDecl<ParseHandler>());
        blockObj->resetPrevBlockChainFromParser();
    }
}









template <class ContextT>
typename ContextT::StmtInfo *
LexicalLookup(ContextT *ct, HandleAtom atom, int *slotp, typename ContextT::StmtInfo *stmt)
{
    RootedId id(ct->sc->context, AtomToId(atom));

    if (!stmt)
        stmt = ct->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        




        if (stmt->type == STMT_WITH)
            break;

        
        if (!stmt->isBlockScope)
            continue;

        StaticBlockObject &blockObj = *stmt->blockObj;
        Shape *shape = blockObj.nativeLookup(ct->sc->context, id);
        if (shape) {
            JS_ASSERT(shape->hasShortID());

            if (slotp)
                *slotp = blockObj.stackDepth() + shape->shortid();
            return stmt;
        }
    }

    if (slotp)
        *slotp = -1;
    return stmt;
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

template <typename ParseHandler>
 bool
Parser<ParseHandler>::bindVarOrConst(BindData<ParseHandler> *data,
                                     HandlePropertyName name, Parser<ParseHandler> *parser)
{
    ExclusiveContext *cx = parser->context;
    ParseContext<ParseHandler> *pc = parser->pc;
    Node pn = data->pn;
    bool isConstDecl = data->op == JSOP_DEFCONST;

    
    parser->handler.setOp(pn, JSOP_NAME);

    if (!parser->checkStrictBinding(name, pn))
        return false;

    StmtInfoPC *stmt = LexicalLookup(pc, name, NULL, (StmtInfoPC *)NULL);

    if (stmt && stmt->type == STMT_WITH) {
        parser->handler.setFlag(pn, PND_DEOPTIMIZED);
        if (pc->sc->isFunctionBox()) {
            FunctionBox *funbox = pc->sc->asFunctionBox();
            funbox->setMightAliasLocals();

            






            if (name == cx->names().arguments)
                funbox->setHasDebuggerStatement();
        }
        return true;
    }

    DefinitionList::Range defs = pc->decls().lookupMulti(name);
    JS_ASSERT_IF(stmt, !defs.empty());

    if (defs.empty()) {
        return pc->define(parser->tokenStream, name, pn,
                          isConstDecl ? Definition::CONST : Definition::VAR);
    }

    






    DefinitionNode dn = defs.front<ParseHandler>();
    Definition::Kind dn_kind = parser->handler.getDefinitionKind(dn);
    if (dn_kind == Definition::ARG) {
        JSAutoByteString bytes;
        if (!AtomToPrintableString(cx, name, &bytes))
            return false;

        if (isConstDecl) {
            parser->report(ParseError, false, pn, JSMSG_REDECLARED_PARAM, bytes.ptr());
            return false;
        }
        if (!parser->report(ParseExtraWarning, false, pn, JSMSG_VAR_HIDES_ARG, bytes.ptr()))
            return false;
    } else {
        bool error = (isConstDecl ||
                      dn_kind == Definition::CONST ||
                      (dn_kind == Definition::LET &&
                       (stmt->type != STMT_CATCH || OuterLet(pc, stmt, name))));

        if (parser->options().extraWarningsOption
            ? data->op != JSOP_DEFVAR || dn_kind != Definition::VAR
            : error)
        {
            JSAutoByteString bytes;
            ParseReportKind reporter = error ? ParseError : ParseExtraWarning;
            if (!AtomToPrintableString(cx, name, &bytes) ||
                !parser->report(reporter, false, pn, JSMSG_REDECLARED_VAR,
                                Definition::kindString(dn_kind), bytes.ptr()))
            {
                return false;
            }
        }
    }

    parser->handler.linkUseToDef(pn, dn);
    return true;
}

template <>
bool
Parser<FullParseHandler>::makeSetCall(ParseNode *pn, unsigned msg)
{
    JS_ASSERT(pn->isKind(PNK_CALL));
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isOp(JSOP_CALL) || pn->isOp(JSOP_EVAL) ||
              pn->isOp(JSOP_FUNCALL) || pn->isOp(JSOP_FUNAPPLY));

    if (!report(ParseStrictError, pc->sc->strict, pn, msg))
        return false;
    handler.markAsSetCall(pn);
    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::noteNameUse(HandlePropertyName name, Node pn)
{
    StmtInfoPC *stmt = LexicalLookup(pc, name, NULL, (StmtInfoPC *)NULL);

    DefinitionList::Range defs = pc->decls().lookupMulti(name);

    DefinitionNode dn;
    if (!defs.empty()) {
        dn = defs.front<ParseHandler>();
    } else {
        







        dn = getOrCreateLexicalDependency(pc, name);
        if (!dn)
            return false;
    }

    handler.linkUseToDef(pn, dn);

    if (stmt && stmt->type == STMT_WITH)
        handler.setFlag(pn, PND_DEOPTIMIZED);

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
    if (!data->binder(data, name, this))
        return false;

    



    if (pn->pn_dflags & PND_BOUND)
        pn->setOp(JSOP_SETLOCAL);
    else if (data->op == JSOP_DEFCONST)
        pn->setOp(JSOP_SETCONST);
    else
        pn->setOp(JSOP_SETNAME);

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    pn->markAsAssigned();
    return true;
}



















template <>
bool
Parser<FullParseHandler>::bindDestructuringLHS(ParseNode *pn)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        pn->markAsAssigned();
        

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
            if (!pn->isKind(PNK_ELISION)) {
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
                






                if (pair->pn_right == pair->pn_left) {
                    RootedPropertyName name(context, pn->pn_atom->asPropertyName());
                    if (!noteNameUse(name, pn))
                        return false;
                }
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
    return abortIfSyntaxParser();
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

    bool operator()(TokenStream &ts, ParseContext<FullParseHandler> *pc,
                    HandleStaticBlockObject blockObj, const Shape &shape, JSAtom *)
    {
        ParseNode *def = (ParseNode *) blockObj->getSlot(shape.slot()).toPrivate();
        def->pn_blockid = blockid;
        RootedPropertyName name(ts.context(), def->name());
        return pc->define(ts, name, def, Definition::LET);
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

    
    if (!ForEachLetDef(tokenStream, pc, blockObj, AddLetDecl(stmt->blockid)))
        return null();

    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::pushLetScope(HandleStaticBlockObject blockObj, StmtInfoPC *stmt)
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
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

    uint32_t begin = pos().begin;

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

    bool needExprStmt = false;
    if (letContext == LetStatement && !tokenStream.matchToken(TOK_LC, TSF_OPERAND)) {
        







        if (!report(ParseStrictError, pc->sc->strict, pnlet,
                    JSMSG_STRICT_CODE_LET_EXPR_STMT))
        {
            return null();
        }

        




        needExprStmt = true;
        letContext = LetExpresion;
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
    PopStatementPC(tokenStream, pc);

    handler.setEndPosition(pnlet, pos().end);

    if (needExprStmt) {
        if (!MatchOrInsertSemicolon(tokenStream))
            return null();
        return handler.newExprStatement(pnlet, pos().end);
    }
    return pnlet;
}

#endif 

template <typename ParseHandler>
static bool
PushBlocklikeStatement(StmtInfoPC *stmt, StmtType type, ParseContext<ParseHandler> *pc)
{
    PushStatementPC(pc, stmt, type);
    return GenerateBlockId(pc, stmt->blockid);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::blockStatement()
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_LC);

    StmtInfoPC stmtInfo(context);
    if (!PushBlocklikeStatement(&stmtInfo, STMT_BLOCK, pc))
        return null();

    Node list = statements();
    if (!list)
        return null();

    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_COMPOUND);
    PopStatementPC(tokenStream, pc);
    return list;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::newBindingNode(PropertyName *name, bool functionScope, VarContext varContext)
{
    






    if (varContext == HoistVars) {
        if (AtomDefnPtr p = pc->lexdeps->lookup(name)) {
            DefinitionNode lexdep = p.value().get<ParseHandler>();
            JS_ASSERT(handler.getDefinitionKind(lexdep) == Definition::PLACEHOLDER);

            Node pn = handler.getDefinitionNode(lexdep);
            if (handler.dependencyCovered(pn, pc->blockid(), functionScope)) {
                handler.setBlockId(pn, pc->blockid());
                pc->lexdeps->remove(p);
                handler.setPosition(pn, pos());
                return pn;
            }
        }
    }

    
    JS_ASSERT(tokenStream.currentToken().type == TOK_NAME);
    return newName(name);
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
        pn2 = newBindingNode(name, kind == PNK_VAR || kind == PNK_CONST, varContext);
        if (!pn2)
            return null();
        if (data.op == JSOP_DEFCONST)
            handler.setFlag(pn2, PND_CONST);
        data.pn = pn2;
        if (!data.binder(&data, name, this))
            return null();
        handler.addList(pn, pn2);

        if (tokenStream.matchToken(TOK_ASSIGN)) {
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

#if JS_HAS_BLOCK_SCOPE
template <>
ParseNode *
Parser<FullParseHandler>::letStatement()
{
    handler.disableSyntaxParser();

    ParseNode *pn;
    do {
        
        if (tokenStream.peekToken() == TOK_LP) {
            pn = letBlock(LetStatement);
            JS_ASSERT_IF(pn, pn->isKind(PNK_LET) || pn->isKind(PNK_SEMI));
            JS_ASSERT_IF(pn && pn->isKind(PNK_LET) && pn->pn_expr->getOp() != JSOP_LEAVEBLOCK,
                         pn->isOp(JSOP_NOP));
            return pn;
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

    
    return MatchOrInsertSemicolon(tokenStream) ? pn : NULL;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::letStatement()
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

#endif 

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::expressionStatement()
{
    tokenStream.ungetToken();
    Node pnexpr = expr();
    if (!pnexpr)
        return null();
    if (!MatchOrInsertSemicolon(tokenStream))
        return null();
    return handler.newExprStatement(pnexpr, pos().end);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::ifStatement()
{
    uint32_t begin = pos().begin;

    
    Node cond = condition();
    if (!cond)
        return null();

    if (tokenStream.peekToken(TSF_OPERAND) == TOK_SEMI &&
        !report(ParseExtraWarning, false, null(), JSMSG_EMPTY_CONSEQUENT))
    {
        return null();
    }

    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_IF);
    Node thenBranch = statement();
    if (!thenBranch)
        return null();

    Node elseBranch;
    if (tokenStream.matchToken(TOK_ELSE, TSF_OPERAND)) {
        stmtInfo.type = STMT_ELSE;
        elseBranch = statement();
        if (!elseBranch)
            return null();
    } else {
        elseBranch = null();
    }

    PopStatementPC(tokenStream, pc);
    return handler.newIfStatement(begin, cond, thenBranch, elseBranch);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::doWhileStatement()
{
    uint32_t begin = pos().begin;
    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_DO_LOOP);
    Node body = statement();
    if (!body)
        return null();
    MUST_MATCH_TOKEN(TOK_WHILE, JSMSG_WHILE_AFTER_DO);
    Node cond = condition();
    if (!cond)
        return null();
    PopStatementPC(tokenStream, pc);

    if (versionNumber() == JSVERSION_ECMA_3) {
        
        
        if (!MatchOrInsertSemicolon(tokenStream))
            return null();
    } else {
        
        
        
        
        
        (void) tokenStream.matchToken(TOK_SEMI);
    }

    return handler.newDoWhileStatement(body, cond, TokenPos(begin, pos().end));
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::whileStatement()
{
    uint32_t begin = pos().begin;
    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_WHILE_LOOP);
    Node cond = condition();
    if (!cond)
        return null();
    Node body = statement();
    if (!body)
        return null();
    PopStatementPC(tokenStream, pc);
    return handler.newWhileStatement(begin, cond, body);
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::matchInOrOf(bool *isForOfp)
{
    if (tokenStream.matchToken(TOK_IN)) {
        *isForOfp = false;
        return true;
    }
    if (tokenStream.matchContextualKeyword(context->names().of)) {
        *isForOfp = true;
        return true;
    }
    return false;
}

template <>
bool
Parser<FullParseHandler>::isValidForStatementLHS(ParseNode *pn1, JSVersion version,
                                                 bool isForDecl, bool isForEach, bool isForOf)
{
    if (isForDecl) {
        if (pn1->pn_count > 1)
            return false;
        if (pn1->isOp(JSOP_DEFCONST))
            return false;
#if JS_HAS_DESTRUCTURING
        
        
        if (version == JSVERSION_1_7 && !isForEach && !isForOf) {
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
        
        
        if (version == JSVERSION_1_7 && !isForEach && !isForOf)
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
    uint32_t begin = pos().begin;

    StmtInfoPC forStmt(context);
    PushStatementPC(pc, &forStmt, STMT_FOR_LOOP);

    bool isForEach = false;
    unsigned iflags = 0;

    if (allowsForEachIn() && tokenStream.matchContextualKeyword(context->names().each)) {
        iflags = JSITER_FOREACH;
        isForEach = true;
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    



    bool isForDecl = false;

    
    RootedStaticBlockObject blockObj(context);

    
    ParseNode *pn1;

    {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            pn1 = NULL;
        } else {
            












            pc->parsingForInit = true;
            if (tt == TOK_VAR || tt == TOK_CONST) {
                isForDecl = true;
                tokenStream.consumeKnownToken(tt);
                pn1 = variables(tt == TOK_VAR ? PNK_VAR : PNK_CONST);
            }
#if JS_HAS_BLOCK_SCOPE
            else if (tt == TOK_LET) {
                handler.disableSyntaxParser();
                (void) tokenStream.getToken();
                if (tokenStream.peekToken() == TOK_LP) {
                    pn1 = letBlock(LetExpresion);
                } else {
                    isForDecl = true;
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

    JS_ASSERT_IF(isForDecl, pn1->isArity(PN_LIST));
    JS_ASSERT(!!blockObj == (isForDecl && pn1->isOp(JSOP_NOP)));

    
    
    
    
    
    
    ParseNode *forLetImpliedBlock = NULL;
    ParseNode *forLetDecl = NULL;

    
    
    ParseNode *hoistedVar = NULL;

    





    StmtInfoPC letStmt(context); 
    ParseNode *pn2, *pn3;      
    bool isForOf;
    bool isForInOrOf = pn1 && matchInOrOf(&isForOf);
    if (isForInOrOf) {
        







        forStmt.type = STMT_FOR_IN_LOOP;

        
        if (isForOf && isForEach) {
            report(ParseError, false, null(), JSMSG_BAD_FOR_EACH_LOOP);
            return null();
        }
        iflags |= (isForOf ? JSITER_FOR_OF : JSITER_ENUMERATE);

        
        if (!isValidForStatementLHS(pn1, versionNumber(), isForDecl, isForEach, isForOf)) {
            report(ParseError, false, pn1, JSMSG_BAD_FOR_LEFTSIDE);
            return null();
        }

        





        if (isForDecl) {
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

                hoistedVar = pn1;

                






                pn1->pn_xflags |= PNX_POPVAR;
                pn1 = NULL;

#if JS_HAS_DESTRUCTURING
                if (pn2->isKind(PNK_ASSIGN)) {
                    pn2 = pn2->pn_left;
                    JS_ASSERT(pn2->isKind(PNK_ARRAY) || pn2->isKind(PNK_OBJECT) ||
                              pn2->isKind(PNK_NAME));
                }
#endif
            }
        } else {
            
            JS_ASSERT(!blockObj);
            pn2 = pn1;
            pn1 = NULL;

            if (!setAssignmentLhsOps(pn2, true))
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

        if (isForDecl) {
            



            pn2 = cloneLeftHandSide(pn2);
            if (!pn2)
                return null();
        }

        switch (pn2->getKind()) {
          case PNK_NAME:
            
            pn2->markAsAssigned();
            break;

#if JS_HAS_DESTRUCTURING
          case PNK_ASSIGN:
            MOZ_ASSUME_UNREACHABLE("forStatement TOK_ASSIGN");

          case PNK_ARRAY:
          case PNK_OBJECT:
            if (versionNumber() == JSVERSION_1_7) {
                



                if (!isForEach && !isForOf)
                    iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          default:;
        }
    } else {
        if (isForEach) {
            reportWithOffset(ParseError, false, begin, JSMSG_BAD_FOR_EACH_LOOP);
            return null();
        }

        if (blockObj) {
            



            forLetImpliedBlock = pushLetScope(blockObj, &letStmt);
            if (!forLetImpliedBlock)
                return null();
            letStmt.isForLetBlock = true;

            forLetDecl = pn1;
            pn1 = NULL;
        }

        
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
    }

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    TokenPos headPos(begin, pos().end);
    ParseNode *forHead = handler.newForHead(isForInOrOf, pn1, pn2, pn3, headPos);
    if (!forHead)
        return null();

    
    ParseNode *body = statement();
    if (!body)
        return null();

#if JS_HAS_BLOCK_SCOPE
    if (blockObj)
        PopStatementPC(tokenStream, pc);
#endif
    PopStatementPC(tokenStream, pc);

    ParseNode *forLoop = handler.newForStatement(begin, forHead, body, iflags);
    if (!forLoop)
        return null();

    if (hoistedVar) {
        ParseNode *pnseq = handler.newList(PNK_SEQ, hoistedVar);
        if (!pnseq)
            return null();
        pnseq->pn_pos = forLoop->pn_pos;
        pnseq->append(forLoop);
        return pnseq;
    }
    if (forLetImpliedBlock) {
        forLetImpliedBlock->pn_expr = forLoop;
        forLetImpliedBlock->pn_pos = forLoop->pn_pos;
        ParseNode *let = handler.newBinary(PNK_LET, forLetDecl, forLetImpliedBlock);
        if (!let)
            return null();
        let->pn_pos = forLoop->pn_pos;
        return let;
    }
    return forLoop;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::forStatement()
{
    





    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    StmtInfoPC forStmt(context);
    PushStatementPC(pc, &forStmt, STMT_FOR_LOOP);

    
    if (allowsForEachIn() && tokenStream.peekToken() == TOK_NAME) {
        JS_ALWAYS_FALSE(abortIfSyntaxParser());
        return null();
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    
    bool isForDecl = false;
    bool simpleForDecl = true;

    
    Node lhsNode;

    {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt == TOK_SEMI) {
            lhsNode = null();
        } else {
            
            pc->parsingForInit = true;
            if (tt == TOK_VAR) {
                isForDecl = true;
                tokenStream.consumeKnownToken(tt);
                lhsNode = variables(tt == TOK_VAR ? PNK_VAR : PNK_CONST, &simpleForDecl);
            }
#if JS_HAS_BLOCK_SCOPE
            else if (tt == TOK_CONST || tt == TOK_LET) {
                JS_ALWAYS_FALSE(abortIfSyntaxParser());
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

    





    bool isForOf;
    if (lhsNode && matchInOrOf(&isForOf)) {
        
        forStmt.type = STMT_FOR_IN_LOOP;

        
        if (!isForDecl &&
            lhsNode != SyntaxParseHandler::NodeName &&
            lhsNode != SyntaxParseHandler::NodeGetProp &&
            lhsNode != SyntaxParseHandler::NodeLValue)
        {
            JS_ALWAYS_FALSE(abortIfSyntaxParser());
            return null();
        }

        if (!simpleForDecl) {
            JS_ALWAYS_FALSE(abortIfSyntaxParser());
            return null();
        }

        if (!isForDecl && !setAssignmentLhsOps(lhsNode, true))
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

    PopStatementPC(tokenStream, pc);
    return SyntaxParseHandler::NodeGeneric;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::switchStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_SWITCH));
    uint32_t begin = pos().begin;

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

    Node caseList = handler.newStatementList(pc->blockid(), pos());
    if (!caseList)
        return null();

    Node saveBlock = pc->blockNode;
    pc->blockNode = caseList;

    bool seenDefault = false;
    TokenKind tt;
    while ((tt = tokenStream.getToken()) != TOK_RC) {
        uint32_t caseBegin = pos().begin;

        Node caseExpr;
        switch (tt) {
          case TOK_DEFAULT:
            if (seenDefault) {
                report(ParseError, false, null(), JSMSG_TOO_MANY_DEFAULTS);
                return null();
            }
            seenDefault = true;
            caseExpr = null();  
            break;

          case TOK_CASE:
            caseExpr = expr();
            if (!caseExpr)
                return null();
            break;

          case TOK_ERROR:
            return null();

          default:
            report(ParseError, false, null(), JSMSG_BAD_SWITCH);
            return null();
        }

        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        Node body = handler.newStatementList(pc->blockid(), pos());
        if (!body)
            return null();

        while ((tt = tokenStream.peekToken(TSF_OPERAND)) != TOK_RC &&
               tt != TOK_CASE && tt != TOK_DEFAULT) {
            if (tt == TOK_ERROR)
                return null();
            Node stmt = statement();
            if (!stmt)
                return null();
            handler.addList(body, stmt);
        }

        Node casepn = handler.newCaseOrDefault(caseBegin, caseExpr, body);
        if (!casepn)
            return null();
        handler.addList(caseList, casepn);
    }

    





    if (pc->blockNode != caseList)
        caseList = pc->blockNode;
    pc->blockNode = saveBlock;

    PopStatementPC(tokenStream, pc);

    handler.setEndPosition(caseList, pos().end);

    return handler.newSwitchStatement(begin, discriminant, caseList);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::continueStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_CONTINUE));
    uint32_t begin = pos().begin;

    RootedPropertyName label(context);
    if (!MatchLabel(tokenStream, &label))
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

    if (!MatchOrInsertSemicolon(tokenStream))
        return null();

    return handler.newContinueStatement(label, TokenPos(begin, pos().end));
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::breakStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_BREAK));
    uint32_t begin = pos().begin;

    RootedPropertyName label(context);
    if (!MatchLabel(tokenStream, &label))
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

    if (!MatchOrInsertSemicolon(tokenStream))
        return null();

    return handler.newBreakStatement(label, TokenPos(begin, pos().end));
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::returnStatementOrYieldExpression()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_RETURN) ||
              tokenStream.isCurrentTokenType(TOK_YIELD));
    bool isYield = tokenStream.isCurrentTokenType(TOK_YIELD);
    uint32_t begin = pos().begin;

    if (!pc->sc->isFunctionBox()) {
        report(ParseError, false, null(), JSMSG_BAD_RETURN_OR_YIELD,
               isYield ? js_yield_str : js_return_str);
        return null();
    }

    if (isYield) {
        if (!abortIfSyntaxParser())
            return null();

        
        
        
        if (pc->parenDepth == 0) {
            pc->sc->asFunctionBox()->setIsGenerator();
        } else {
            pc->yieldCount++;
            pc->yieldOffset = begin;
        }
    }

    
    
    
    
    
    
    
    
    Node exprNode;
    TokenKind next = tokenStream.peekTokenSameLine(TSF_OPERAND);
    if (next == TOK_ERROR)
        return null();
    if (next == TOK_EOF || next == TOK_EOL || next == TOK_SEMI || next == TOK_RC ||
        (isYield && (next == TOK_RB || next == TOK_RP || next == TOK_COLON || next == TOK_COMMA)))
    {
        if (isYield) {
            if (!reportWithOffset(ParseWarning, false, pos().begin, JSMSG_YIELD_WITHOUT_OPERAND))
                return null();
        }

        exprNode = null();
        if (!isYield)
            pc->funHasReturnVoid = true;
    } else {
        exprNode = isYield ? assignExpr() : expr();
        if (!exprNode)
            return null();
        if (!isYield)
            pc->funHasReturnExpr = true;
    }

    if (!isYield) {
        if (!MatchOrInsertSemicolon(tokenStream))
            return null();
    }

    Node pn = isYield
              ? handler.newUnary(PNK_YIELD, JSOP_YIELD, begin, exprNode)
              : handler.newReturnStatement(exprNode, TokenPos(begin, pos().end));
    if (!pn)
        return null();

    if (pc->funHasReturnExpr && pc->sc->asFunctionBox()->isGenerator()) {
        
        reportBadReturn(pn, ParseError, JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return null();
    }

    if (options().extraWarningsOption && pc->funHasReturnExpr && pc->funHasReturnVoid &&
        !reportBadReturn(pn, ParseExtraWarning,
                         JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE))
    {
        return null();
    }

    return pn;
}

template <>
ParseNode *
Parser<FullParseHandler>::withStatement()
{
    if (handler.syntaxParser) {
        handler.disableSyntaxParser();
        abortedSyntaxParse = true;
        return null();
    }

    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_WITH));
    uint32_t begin = pos().begin;

    
    
    
    
    
    
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
    PopStatementPC(tokenStream, pc);

    pc->sc->setBindingsAccessedDynamically();
    pc->parsingWith = oldParsingWith;

    



    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
        DefinitionNode defn = r.front().value().get<FullParseHandler>();
        DefinitionNode lexdep = handler.resolve(defn);
        handler.deoptimizeUsesWithin(lexdep, TokenPos(begin, pos().begin));
    }

    return handler.newWithStatement(begin, objectExpr, innerBlock);
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::withStatement()
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return null();
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::labeledStatement()
{
    uint32_t begin = pos().begin;
    RootedPropertyName label(context, tokenStream.currentToken().name());
    for (StmtInfoPC *stmt = pc->topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == STMT_LABEL && stmt->label == label) {
            report(ParseError, false, null(), JSMSG_DUPLICATE_LABEL);
            return null();
        }
    }

    tokenStream.consumeKnownToken(TOK_COLON);

    
    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_LABEL);
    stmtInfo.label = label;
    Node pn = statement();
    if (!pn)
        return null();

    
    PopStatementPC(tokenStream, pc);

    return handler.newLabeledStatement(label, pn, begin);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::throwStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_THROW));
    uint32_t begin = pos().begin;

    
    TokenKind tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return null();
    if (tt == TOK_EOF || tt == TOK_EOL || tt == TOK_SEMI || tt == TOK_RC) {
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();
    }

    Node throwExpr = expr();
    if (!throwExpr)
        return null();

    if (!MatchOrInsertSemicolon(tokenStream))
        return null();

    return handler.newThrowStatement(throwExpr, TokenPos(begin, pos().end));
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::tryStatement()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_TRY));
    uint32_t begin = pos().begin;

    

















    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    StmtInfoPC stmtInfo(context);
    if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, pc))
        return null();
    Node innerBlock = statements();
    if (!innerBlock)
        return null();
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatementPC(tokenStream, pc);

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
                catchName = newBindingNode(label, false);
                if (!catchName)
                    return null();
                data.pn = catchName;
                if (!data.binder(&data, label, this))
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
            PopStatementPC(tokenStream, pc);

            if (!catchGuard)
                hasUnconditionalCatch = true;

            if (!handler.addCatchBlock(catchList, pnblock, catchName, catchGuard, catchBody))
                return null();
            handler.setEndPosition(catchList, pos().end);
            handler.setEndPosition(pnblock, pos().end);

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
        PopStatementPC(tokenStream, pc);
    } else {
        tokenStream.ungetToken();
    }
    if (!catchList && !finallyBlock) {
        report(ParseError, false, null(), JSMSG_CATCH_OR_FINALLY);
        return null();
    }

    return handler.newTryStatement(begin, innerBlock, catchList, finallyBlock);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::debuggerStatement()
{
    TokenPos p;
    p.begin = pos().begin;
    if (!MatchOrInsertSemicolon(tokenStream))
        return null();
    p.end = pos().end;

    pc->sc->setBindingsAccessedDynamically();
    pc->sc->setHasDebuggerStatement();

    return handler.newDebuggerStatement(p);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::statement(bool canHaveDirectives)
{
    Node pn;

    JS_CHECK_RECURSION(context, return null());

    switch (tokenStream.getToken(TSF_OPERAND)) {
      case TOK_LC:
        return blockStatement();

      case TOK_VAR:
        pn = variables(PNK_VAR);
        if (!pn)
            return null();

        
        handler.setListFlag(pn, PNX_POPVAR);
        break;

      case TOK_CONST:
        if (!abortIfSyntaxParser())
            return null();

        pn = variables(PNK_CONST);
        if (!pn)
            return null();

        
        handler.setListFlag(pn, PNX_POPVAR);
        break;

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        return letStatement();
#endif

      case TOK_SEMI:
        return handler.newEmptyStatement(pos());
      case TOK_IF:
        return ifStatement();
      case TOK_DO:
        return doWhileStatement();
      case TOK_WHILE:
        return whileStatement();
      case TOK_FOR:
        return forStatement();
      case TOK_SWITCH:
        return switchStatement();
      case TOK_CONTINUE:
        return continueStatement();
      case TOK_BREAK:
        return breakStatement();
      case TOK_RETURN:
        return returnStatementOrYieldExpression();
      case TOK_WITH:
        return withStatement();
      case TOK_THROW:
        return throwStatement();
      case TOK_TRY:
        return tryStatement();
      case TOK_FUNCTION:
        return functionStmt();
      case TOK_DEBUGGER:
        return debuggerStatement();

      
      case TOK_CATCH:
        report(ParseError, false, null(), JSMSG_CATCH_WITHOUT_TRY);
        return null();

      case TOK_FINALLY:
        report(ParseError, false, null(), JSMSG_FINALLY_WITHOUT_TRY);
        return null();

      case TOK_ERROR:
        return null();

      case TOK_STRING:
        if (!canHaveDirectives && tokenStream.currentToken().atom() == context->names().useAsm) {
            if (!report(ParseWarning, false, null(), JSMSG_USE_ASM_DIRECTIVE_FAIL))
                return null();
        }
        return expressionStatement();

      case TOK_NAME:
        if (tokenStream.peekToken() == TOK_COLON)
            return labeledStatement();
        if (tokenStream.currentToken().name() == context->names().module
            && tokenStream.peekTokenSameLine() == TOK_STRING)
        {
            return moduleDecl();
        }
        return expressionStatement();

      default:
        return expressionStatement();
    }

    
    return MatchOrInsertSemicolon(tokenStream) ? pn : null();
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
            pn2 = pn->last();
            if (pn2->isKind(PNK_YIELD) && !pn2->isInParens()) {
                report(ParseError, false, pn2, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
                return null();
            }
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
Parser<FullParseHandler>::setAssignmentLhsOps(ParseNode *pn, bool isPlainAssignment)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        if (!checkStrictAssignment(pn))
            return false;
        pn->setOp(pn->isOp(JSOP_GETLOCAL) ? JSOP_SETLOCAL : JSOP_SETNAME);
        pn->markAsAssigned();
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
        if (!isPlainAssignment) {
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
Parser<SyntaxParseHandler>::setAssignmentLhsOps(Node pn, bool isPlainAssignment)
{
    
    if (pn != SyntaxParseHandler::NodeName &&
        pn != SyntaxParseHandler::NodeGetProp &&
        pn != SyntaxParseHandler::NodeLValue)
    {
        return abortIfSyntaxParser();
    }
    return checkStrictAssignment(pn);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::assignExpr()
{
    JS_CHECK_RECURSION(context, return null());

    
    
    
    
    
    
    
    
    
    

    TokenKind tt = tokenStream.getToken(TSF_OPERAND);

    if (tt == TOK_NAME && tokenStream.nextTokenEndsExpr())
        return identifierName();

    if (tt == TOK_NUMBER && tokenStream.nextTokenEndsExpr())
        return newNumber(tokenStream.currentToken());

    if (tt == TOK_STRING && tokenStream.nextTokenEndsExpr())
        return stringLiteral();

    if (tt == TOK_YIELD)
        return returnStatementOrYieldExpression();

    tokenStream.ungetToken();

    
    
    TokenStream::Position start(keepAtoms);
    tokenStream.tell(&start);

    Node lhs = condExpr1();
    if (!lhs)
        return null();

    ParseNodeKind kind;
    JSOp op;
    switch (tokenStream.currentToken().type) {
      case TOK_ASSIGN:       kind = PNK_ASSIGN;       op = JSOP_NOP;    break;
      case TOK_ADDASSIGN:    kind = PNK_ADDASSIGN;    op = JSOP_ADD;    break;
      case TOK_SUBASSIGN:    kind = PNK_SUBASSIGN;    op = JSOP_SUB;    break;
      case TOK_BITORASSIGN:  kind = PNK_BITORASSIGN;  op = JSOP_BITOR;  break;
      case TOK_BITXORASSIGN: kind = PNK_BITXORASSIGN; op = JSOP_BITXOR; break;
      case TOK_BITANDASSIGN: kind = PNK_BITANDASSIGN; op = JSOP_BITAND; break;
      case TOK_LSHASSIGN:    kind = PNK_LSHASSIGN;    op = JSOP_LSH;    break;
      case TOK_RSHASSIGN:    kind = PNK_RSHASSIGN;    op = JSOP_RSH;    break;
      case TOK_URSHASSIGN:   kind = PNK_URSHASSIGN;   op = JSOP_URSH;   break;
      case TOK_MULASSIGN:    kind = PNK_MULASSIGN;    op = JSOP_MUL;    break;
      case TOK_DIVASSIGN:    kind = PNK_DIVASSIGN;    op = JSOP_DIV;    break;
      case TOK_MODASSIGN:    kind = PNK_MODASSIGN;    op = JSOP_MOD;    break;

      case TOK_ARROW: {
        tokenStream.seek(start);
        if (!abortIfSyntaxParser())
            return null();

        if (tokenStream.getToken() == TOK_ERROR)
            return null();
        size_t offset = pos().begin;
        tokenStream.ungetToken();

        return functionDef(NullPtr(), start, offset, Normal, Arrow);
      }

      default:
        JS_ASSERT(!tokenStream.isCurrentTokenAssignment());
        tokenStream.ungetToken();
        return lhs;
    }

    if (!setAssignmentLhsOps(lhs, kind == PNK_ASSIGN))
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
    if (!setLvalKid(pn, kid, incop_name_str[tt == TOK_DEC]))
        return false;

    switch (kid->getKind()) {
      case PNK_NAME:
        kid->markAsAssigned();
        break;

      case PNK_CALL:
        if (!makeSetCall(kid, JSMSG_BAD_INCOP_OPERAND))
            return false;
        break;

      case PNK_DOT:
      case PNK_ELEM:
        break;

      default:
        JS_ASSERT(0);
    }
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::setIncOpKid(Node pn, Node kid, TokenKind tt, bool preorder)
{
    return setAssignmentLhsOps(kid, false);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::unaryOpExpr(ParseNodeKind kind, JSOp op, uint32_t begin)
{
    Node kid = unaryExpr();
    if (!kid)
        return null();
    return handler.newUnary(kind, op, begin, kid);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::unaryExpr()
{
    Node pn, pn2;

    JS_CHECK_RECURSION(context, return null());

    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    uint32_t begin = pos().begin;
    switch (tt) {
      case TOK_TYPEOF:
        return unaryOpExpr(PNK_TYPEOF, JSOP_TYPEOF, begin);
      case TOK_VOID:
        return unaryOpExpr(PNK_VOID, JSOP_VOID, begin);
      case TOK_NOT:
        return unaryOpExpr(PNK_NOT, JSOP_NOT, begin);
      case TOK_BITNOT:
        return unaryOpExpr(PNK_BITNOT, JSOP_BITNOT, begin);
      case TOK_PLUS:
        return unaryOpExpr(PNK_POS, JSOP_POS, begin);
      case TOK_MINUS:
        return unaryOpExpr(PNK_NEG, JSOP_NEG, begin);

      case TOK_INC:
      case TOK_DEC:
      {
        TokenKind tt2 = tokenStream.getToken(TSF_OPERAND);
        pn2 = memberExpr(tt2, true);
        if (!pn2)
            return null();
        pn = handler.newUnary((tt == TOK_INC) ? PNK_PREINCREMENT : PNK_PREDECREMENT,
                              JSOP_NOP,
                              begin,
                              pn2);
        if (!pn)
            return null();
        if (!setIncOpKid(pn, pn2, tt, true))
            return null();
        break;
      }

      case TOK_DELETE: {
        Node expr = unaryExpr();
        if (!expr)
            return null();

        
        
        if (handler.isName(expr)) {
            if (!report(ParseStrictError, pc->sc->strict, expr, JSMSG_DEPRECATED_DELETE_OPERAND))
                return null();
            pc->sc->setBindingsAccessedDynamically();
        }

        return handler.newDelete(begin, expr);
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
            pn2 = handler.newUnary((tt == TOK_INC) ? PNK_POSTINCREMENT : PNK_POSTDECREMENT,
                                   JSOP_NOP,
                                   begin,
                                   pn);
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






















class CompExprTransplanter
{
    ParseNode       *root;
    Parser<FullParseHandler> *parser;
    ParseContext<FullParseHandler> *outerpc;
    bool            genexp;
    unsigned        adjust;
    HashSet<Definition *> visitedImplicitArguments;

  public:
    CompExprTransplanter(ParseNode *pn, Parser<FullParseHandler> *parser,
                         ParseContext<FullParseHandler> *outerpc,
                         bool ge, unsigned adj)
      : root(pn), parser(parser), outerpc(outerpc), genexp(ge), adjust(adj),
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
            pc->yieldOffset = 0;
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
        uint32_t offset = pc->yieldOffset
                          ? pc->yieldOffset
                          : (pn ? parser->handler.getPosition(pn)
                                : parser->pos()).begin;

        parser->reportWithOffset(ParseError, false, offset, err, js_yield_str);
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
BumpStaticLevel(TokenStream &ts, ParseNode *pn, ParseContext<ParseHandler> *pc)
{
    if (pn->pn_cookie.isFree())
        return true;

    unsigned level = unsigned(pn->pn_cookie.level()) + 1;
    JS_ASSERT(level >= pc->staticLevel);
    return pn->pn_cookie.set(ts, level, pn->pn_cookie.slot());
}

template <typename ParseHandler>
static bool
AdjustBlockId(TokenStream &ts, ParseNode *pn, unsigned adjust, ParseContext<ParseHandler> *pc)
{
    JS_ASSERT(pn->isArity(PN_LIST) || pn->isArity(PN_CODE) || pn->isArity(PN_NAME));
    if (JS_BIT(20) - pn->pn_blockid <= adjust + 1) {
        ts.reportError(JSMSG_NEED_DIET, "program");
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
            if (!AdjustBlockId(parser->tokenStream, pn, adjust, pc))
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
            if (genexp && !BumpStaticLevel(parser->tokenStream, pn, pc))
                return false;
        } else if (pn->isUsed()) {
            JS_ASSERT(pn->pn_cookie.isFree());

            Definition *dn = pn->pn_lexdef;
            JS_ASSERT(dn->isDefn());

            








            if (dn->isPlaceholder() && dn->pn_pos >= root->pn_pos && dn->dn_uses == pn) {
                if (genexp && !BumpStaticLevel(parser->tokenStream, dn, pc))
                    return false;
                if (!AdjustBlockId(parser->tokenStream, dn, adjust, pc))
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
                    







                    Definition *dn2 = parser->handler.newPlaceholder(
                        atom, parser->pc->inBlock(), parser->pc->blockid(), parser->pos());
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
                    DefinitionSingle def = DefinitionSingle::new_<FullParseHandler>(dn2);
                    if (!pc->lexdeps->put(atom, def))
                        return false;
                    if (dn->isClosed())
                        dn2->pn_dflags |= PND_CLOSED;
                } else if (dn->isPlaceholder()) {
                    




                    outerpc->lexdeps->remove(atom);
                    DefinitionSingle def = DefinitionSingle::new_<FullParseHandler>(dn);
                    if (!pc->lexdeps->put(atom, def))
                        return false;
                } else if (dn->isImplicitArguments()) {
                    






                    if (genexp && !visitedImplicitArguments.has(dn)) {
                        if (!BumpStaticLevel(parser->tokenStream, dn, pc))
                            return false;
                        if (!AdjustBlockId(parser->tokenStream, dn, adjust, pc))
                            return false;
                        if (!visitedImplicitArguments.put(dn))
                            return false;
                    }
                }
            }
        }

        if (pn->pn_pos >= root->pn_pos) {
            if (!AdjustBlockId(parser->tokenStream, pn, adjust, pc))
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
                                            ParseContext<FullParseHandler> *outerpc,
                                            ParseNodeKind kind, JSOp op)
{
    





    if (handler.syntaxParser) {
        handler.disableSyntaxParser();
        abortedSyntaxParse = true;
        return NULL;
    }

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

    CompExprTransplanter transplanter(kid, this, outerpc, kind == PNK_SEMI, adjust);
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
        if (allowsForEachIn() && tokenStream.matchContextualKeyword(context->names().each))
            pn2->pn_iflags |= JSITER_FOREACH;
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

            






            pn3 = newBindingNode(name, false);
            if (!pn3)
                return null();
            break;

          default:
            report(ParseError, false, null(), JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return null();
        }

        bool isForOf;
        if (!matchInOrOf(&isForOf)) {
            report(ParseError, false, null(), JSMSG_IN_AFTER_FOR_NAME);
            return null();
        }
        if (isForOf) {
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
                !isForOf)
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
            if (!data.binder(&data, name, this))
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

    PopStatementPC(tokenStream, pc);
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

    ParseNode *pntop = comprehensionTail(pnexp, pn->pn_blockid, false, NULL,
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
    return abortIfSyntaxParser();
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

        ParseContext<FullParseHandler> genpc(this, outerpc, genFunbox,
                                             outerpc->staticLevel + 1, outerpc->blockidGen);
        if (!genpc.init())
            return null();

        





        genFunbox->anyCxFlags = outerpc->sc->anyCxFlags;
        if (outerpc->sc->isFunctionBox())
            genFunbox->funCxFlags = outerpc->sc->asFunctionBox()->funCxFlags;

        genFunbox->setIsGenerator();
        genFunbox->inGenexpLambda = true;
        genfn->pn_funbox = genFunbox;
        genfn->pn_blockid = genpc.bodyid;

        ParseNode *body = comprehensionTail(pn, outerpc->blockid(), true, outerpc);
        if (!body)
            return null();
        JS_ASSERT(!genfn->pn_body);
        genfn->pn_body = body;
        genfn->pn_pos.begin = body->pn_pos.begin = kid->pn_pos.begin;
        genfn->pn_pos.end = body->pn_pos.end = pos().end;

        RootedPropertyName funName(context);
        if (!leaveFunction(genfn, funName, outerpc))
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
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

static const char js_generator_str[] = "generator";

#endif 

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::assignExprWithoutYield(unsigned msg)
{
    GenexpGuard<ParseHandler> yieldGuard(this);
    Node res = assignExpr();
    yieldGuard.endBody();
    if (res) {
        if (!yieldGuard.checkValidBody(res, msg))
            return null();
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

        if (handler.isOperationWithoutParens(argNode, PNK_YIELD) &&
            tokenStream.peekToken() == TOK_COMMA) {
            report(ParseError, false, argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
            return false;
        }
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
                nextMember = handler.newPropertyAccess(lhs, field, pos().end);
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

            nextMember = handler.newPropertyByValue(lhs, propExpr, pos().end);
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
                
                if (atom == context->names().apply) {
                    handler.setOp(nextMember, JSOP_FUNAPPLY);
                    if (pc->sc->isFunctionBox())
                        pc->sc->asFunctionBox()->usesApply = true;
                } else if (atom == context->names().call) {
                    handler.setOp(nextMember, JSOP_FUNCALL);
                }
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
Parser<ParseHandler>::newName(PropertyName *name)
{
    return handler.newName(name, pc->inBlock(), pc->blockid(), pos());
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::identifierName()
{
    JS_ASSERT(tokenStream.isCurrentTokenType(TOK_NAME));

    RootedPropertyName name(context, tokenStream.currentToken().name());
    Node pn = newName(name);
    if (!pn)
        return null();

    if (!pc->inDeclDestructuring && !noteNameUse(name, pn))
        return null();

    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::stringLiteral()
{
    JSAtom *atom = tokenStream.currentToken().atom();

    
    
    
    const size_t HUGE_STRING = 50000;
    if (sct && sct->active() && atom->length() >= HUGE_STRING)
        sct->abort();

    return handler.newStringLiteral(atom, pos());
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::newRegExp()
{
    
    size_t length = tokenStream.getTokenbuf().length();
    const StableCharPtr chars(tokenStream.getTokenbuf().begin(), length);
    RegExpFlag flags = tokenStream.currentToken().regExpFlags();

    Rooted<RegExpObject*> reobj(context);
    if (RegExpStatics *res = context->regExpStatics())
        reobj = RegExpObject::create(context, res, chars.get(), length, flags, &tokenStream);
    else
        reobj = RegExpObject::createNoStatics(context, chars.get(), length, flags, &tokenStream);

    if (!reobj)
        return null();

    return handler.newRegExp(reobj, pos(), *this);
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
        return functionExpr();

      case TOK_LB:
      {
        pn = handler.newList(PNK_ARRAY, null(), JSOP_NEWINIT);
        if (!pn)
            return null();
        handler.setBlockId(pn, pc->blockidGen);

        if (tokenStream.matchToken(TOK_RB, TSF_OPERAND)) {
            



            handler.setListFlag(pn, PNX_NONCONST);
        } else {
            bool spread = false, missingTrailingComma = false;
            unsigned index = 0;
            for (; ; index++) {
                if (index == JSObject::NELEMENTS_LIMIT) {
                    report(ParseError, false, null(), JSMSG_ARRAY_INIT_TOO_BIG);
                    return null();
                }

                tt = tokenStream.peekToken(TSF_OPERAND);
                if (tt == TOK_RB)
                    break;

                if (tt == TOK_COMMA) {
                    tokenStream.consumeKnownToken(TOK_COMMA);
                    pn2 = handler.newElision();
                    if (!pn2)
                        return null();
                    handler.setListFlag(pn, PNX_SPECIALARRAYINIT | PNX_NONCONST);
                } else if (tt == TOK_TRIPLEDOT) {
                    spread = true;
                    handler.setListFlag(pn, PNX_SPECIALARRAYINIT | PNX_NONCONST);

                    tokenStream.consumeKnownToken(TOK_TRIPLEDOT);
                    uint32_t begin = pos().begin;
                    Node inner = assignExpr();
                    if (!inner)
                        return null();

                    pn2 = handler.newUnary(PNK_SPREAD, JSOP_NOP, begin, inner);
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

            








































            if (index == 0 && !spread && tokenStream.matchToken(TOK_FOR) && missingTrailingComma) {
                if (!arrayInitializerComprehensionTail(pn))
                    return null();
            }

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_LIST);
        }
        handler.setEndPosition(pn, pos().end);
        return pn;
      }

      case TOK_LC:
      {
        Node pnval;

        



        AtomIndexMap seen;

        enum AssignmentType {
            GET     = 0x1,
            SET     = 0x2,
            VALUE   = 0x4 | GET | SET
        };

        pn = handler.newList(PNK_OBJECT, null(), JSOP_NEWINIT);
        if (!pn)
            return null();

        RootedAtom atom(context);
        Value tmp;
        for (;;) {
            TokenKind ltok = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            switch (ltok) {
              case TOK_NUMBER:
                tmp = DoubleValue(tokenStream.currentToken().number());
                atom = ToAtom<CanGC>(context, HandleValue::fromMarkedLocation(&tmp));
                if (!atom)
                    return null();
                pn3 = newNumber(tokenStream.currentToken());
                break;
              case TOK_NAME:
                {
                    atom = tokenStream.currentToken().name();
                    if (atom == context->names().get) {
                        op = JSOP_INITPROP_GETTER;
                    } else if (atom == context->names().set) {
                        op = JSOP_INITPROP_SETTER;
                    } else {
                        pn3 = handler.newIdentifier(atom, pos());
                        if (!pn3)
                            return null();
                        break;
                    }

                    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
                    if (tt == TOK_NAME) {
                        atom = tokenStream.currentToken().name();
                        pn3 = newName(atom->asPropertyName());
                        if (!pn3)
                            return null();
                    } else if (tt == TOK_STRING) {
                        atom = tokenStream.currentToken().atom();

                        uint32_t index;
                        if (atom->isIndex(&index)) {
                            pn3 = handler.newNumber(index, NoDecimal, pos());
                            if (!pn3)
                                return null();
                            tmp = DoubleValue(index);
                            atom = ToAtom<CanGC>(context, HandleValue::fromMarkedLocation(&tmp));
                            if (!atom)
                                return null();
                        } else {
                            pn3 = stringLiteral();
                            if (!pn3)
                                return null();
                        }
                    } else if (tt == TOK_NUMBER) {
                        double number = tokenStream.currentToken().number();
                        tmp = DoubleValue(number);
                        atom = ToAtom<CanGC>(context, HandleValue::fromMarkedLocation(&tmp));
                        if (!atom)
                            return null();
                        pn3 = newNumber(tokenStream.currentToken());
                        if (!pn3)
                            return null();
                    } else {
                        tokenStream.ungetToken();
                        pn3 = handler.newIdentifier(atom, pos());
                        if (!pn3)
                            return null();
                        break;
                    }

                    JS_ASSERT(op == JSOP_INITPROP_GETTER || op == JSOP_INITPROP_SETTER);

                    handler.setListFlag(pn, PNX_NONCONST);

                    
                    Rooted<PropertyName*> funName(context, NULL);
                    TokenStream::Position start(keepAtoms);
                    tokenStream.tell(&start);
                    pn2 = functionDef(funName, start, tokenStream.positionToOffset(start),
                                      op == JSOP_INITPROP_GETTER ? Getter : Setter,
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
                    pn3 = handler.newNumber(index, NoDecimal, pos());
                    if (!pn3)
                        return null();
                } else {
                    pn3 = stringLiteral();
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
                



                if (!abortIfSyntaxParser())
                    return null();
                tokenStream.ungetToken();
                if (!tokenStream.checkForKeyword(atom->charsZ(), atom->length(), NULL))
                    return null();
                handler.setListFlag(pn, PNX_DESTRUCT | PNX_NONCONST);
                PropertyName *name = handler.isName(pn3);
                JS_ASSERT(atom);
                pn3 = newName(name);
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
            } else if (op == JSOP_INITPROP_GETTER) {
                assignType = GET;
            } else if (op == JSOP_INITPROP_SETTER) {
                assignType = SET;
            } else {
                MOZ_ASSUME_UNREACHABLE("bad opcode in object initializer");
            }

            AtomIndexAddPtr p = seen.lookupForAdd(atom);
            if (p) {
                jsatomid index = p.value();
                AssignmentType oldAssignType = AssignmentType(index);
                if ((oldAssignType & assignType) &&
                    (oldAssignType != VALUE || assignType != VALUE || pc->sc->needStrictChecks()))
                {
                    JSAutoByteString name;
                    if (!AtomToPrintableString(context, atom, &name))
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
        handler.setEndPosition(pn, pos().end);
        return pn;
      }

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        return letBlock(LetExpresion);
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
        return pn;
      }

      case TOK_STRING:
        return stringLiteral();

      case TOK_NAME:
        return identifierName();

      case TOK_REGEXP:
        return newRegExp();

      case TOK_NUMBER:
        return newNumber(tokenStream.currentToken());

      case TOK_TRUE:
        return handler.newBooleanLiteral(true, pos());
      case TOK_FALSE:
        return handler.newBooleanLiteral(false, pos());
      case TOK_THIS:
        return handler.newThisLiteral(pos());
      case TOK_NULL:
        return handler.newNullLiteral(pos());

      case TOK_RP:
        
        
        if (tokenStream.peekToken() == TOK_ARROW) {
            tokenStream.ungetToken();  

            
            
            
            return handler.newNullLiteral(pos());
        }
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();

      case TOK_TRIPLEDOT:
        
        
        if (tokenStream.matchToken(TOK_NAME) &&
            tokenStream.matchToken(TOK_RP) &&
            tokenStream.peekToken() == TOK_ARROW)
        {
            tokenStream.ungetToken();  

            
            return handler.newNullLiteral(pos());
        }
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();

      case TOK_ERROR:
        
        return null();

      default:
        report(ParseError, false, null(), JSMSG_SYNTAX_ERROR);
        return null();
    }
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::parenExpr(bool *genexp)
{
    JS_ASSERT(tokenStream.currentToken().type == TOK_LP);
    uint32_t begin = pos().begin;

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
            handler.setEndPosition(pn, pos().end);
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
