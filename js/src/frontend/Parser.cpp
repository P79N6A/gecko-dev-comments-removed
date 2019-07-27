


















#include "frontend/Parser-inl.h"

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jstypes.h"

#include "asmjs/AsmJSValidate.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/FoldConstants.h"
#include "frontend/ParseMaps.h"
#include "frontend/TokenStream.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseNode-inl.h"
#include "vm/ScopeObject-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::Maybe;

using JS::AutoGCRooter;

namespace js {
namespace frontend {

typedef Rooted<StaticBlockObject*> RootedStaticBlockObject;
typedef Handle<StaticBlockObject*> HandleStaticBlockObject;
typedef Rooted<NestedScopeObject*> RootedNestedScopeObject;
typedef Handle<NestedScopeObject*> HandleNestedScopeObject;


#define MUST_MATCH_TOKEN(tt, errno)                                                         \
    JS_BEGIN_MACRO                                                                          \
        TokenKind token;                                                                    \
        if (!tokenStream.getToken(&token))                                                  \
            return null();                                                                  \
        if (token != tt) {                                                                  \
            report(ParseError, false, null(), errno);                                       \
            return null();                                                                  \
        }                                                                                   \
    JS_END_MACRO

static const unsigned BlockIdLimit = 1 << ParseNode::NumBlockIdBits;

template <typename ParseHandler>
bool
GenerateBlockId(TokenStream& ts, ParseContext<ParseHandler>* pc, uint32_t& blockid)
{
    if (pc->blockidGen == BlockIdLimit) {
        ts.reportError(JSMSG_NEED_DIET, "program");
        return false;
    }
    MOZ_ASSERT(pc->blockidGen < BlockIdLimit);
    blockid = pc->blockidGen++;
    return true;
}

template bool
GenerateBlockId(TokenStream& ts, ParseContext<SyntaxParseHandler>* pc, uint32_t& blockid);

template bool
GenerateBlockId(TokenStream& ts, ParseContext<FullParseHandler>* pc, uint32_t& blockid);

template <typename ParseHandler>
static void
PushStatementPC(ParseContext<ParseHandler>* pc, StmtInfoPC* stmt, StmtType type)
{
    stmt->blockid = pc->blockid();
    PushStatement(pc, stmt, type);
}

template <>
bool
ParseContext<FullParseHandler>::checkLocalsOverflow(TokenStream& ts)
{
    if (vars_.length() + bodyLevelLexicals_.length() >= LOCALNO_LIMIT) {
        ts.reportError(JSMSG_TOO_MANY_LOCALS);
        return false;
    }
    return true;
}

static void
MarkUsesAsHoistedLexical(ParseNode* pn)
{
    MOZ_ASSERT(pn->isDefn());

    Definition* dn = (Definition*)pn;
    ParseNode** pnup = &dn->dn_uses;
    ParseNode* pnu;
    unsigned start = pn->pn_blockid;

    
    
    while ((pnu = *pnup) != nullptr && pnu->pn_blockid >= start) {
        MOZ_ASSERT(pnu->isUsed());
        pnu->pn_dflags |= PND_LEXICAL;
        pnup = &pnu->pn_link;
    }
}


template <>
bool
ParseContext<FullParseHandler>::define(TokenStream& ts,
                                       HandlePropertyName name, ParseNode* pn, Definition::Kind kind)
{
    MOZ_ASSERT(!pn->isUsed());
    MOZ_ASSERT_IF(pn->isDefn(), pn->isPlaceholder());

    Definition* prevDef = nullptr;
    if (kind == Definition::LET || kind == Definition::CONST)
        prevDef = decls_.lookupFirst(name);
    else
        MOZ_ASSERT(!decls_.lookupFirst(name));

    if (!prevDef)
        prevDef = lexdeps.lookupDefn<FullParseHandler>(name);

    if (prevDef) {
        ParseNode** pnup = &prevDef->dn_uses;
        ParseNode* pnu;
        unsigned start = (kind == Definition::LET || kind == Definition::CONST) ? pn->pn_blockid
                                                                                : bodyid;

        while ((pnu = *pnup) != nullptr && pnu->pn_blockid >= start) {
            MOZ_ASSERT(pnu->pn_blockid >= bodyid);
            MOZ_ASSERT(pnu->isUsed());
            pnu->pn_lexdef = (Definition*) pn;
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

    MOZ_ASSERT_IF(kind != Definition::LET && kind != Definition::CONST, !lexdeps->lookup(name));
    pn->setDefn(true);
    pn->pn_dflags &= ~PND_PLACEHOLDER;
    if (kind == Definition::CONST)
        pn->pn_dflags |= PND_CONST;

    Definition* dn = (Definition*)pn;
    switch (kind) {
      case Definition::ARG:
        MOZ_ASSERT(sc->isFunctionBox());
        dn->setOp((js_CodeSpec[dn->getOp()].format & JOF_SET) ? JSOP_SETARG : JSOP_GETARG);
        dn->pn_blockid = bodyid;
        dn->pn_dflags |= PND_BOUND;
        if (!dn->pn_cookie.set(ts, staticLevel, args_.length()))
            return false;
        if (!args_.append(dn))
            return false;
        if (args_.length() >= ARGNO_LIMIT) {
            ts.reportError(JSMSG_TOO_MANY_FUN_ARGS);
            return false;
        }
        if (name == ts.names().empty)
            break;
        if (!decls_.addUnique(name, dn))
            return false;
        break;

      case Definition::GLOBALCONST:
      case Definition::VAR:
        if (sc->isFunctionBox()) {
            dn->setOp((js_CodeSpec[dn->getOp()].format & JOF_SET) ? JSOP_SETLOCAL : JSOP_GETLOCAL);
            dn->pn_blockid = bodyid;
            dn->pn_dflags |= PND_BOUND;
            if (!dn->pn_cookie.set(ts, staticLevel, vars_.length()))
                return false;
            if (!vars_.append(dn))
                return false;
            if (!checkLocalsOverflow(ts))
                return false;
        }
        if (!decls_.addUnique(name, dn))
            return false;
        break;

      case Definition::LET:
      case Definition::CONST:
        dn->setOp(JSOP_INITLEXICAL);
        dn->pn_dflags |= (PND_LEXICAL | PND_BOUND);
        MOZ_ASSERT(dn->pn_cookie.level() == staticLevel); 
        if (atBodyLevel()) {
            if (!bodyLevelLexicals_.append(dn))
                return false;
            if (!checkLocalsOverflow(ts))
                return false;
        }

        
        
        
        MarkUsesAsHoistedLexical(pn);

        if (!decls_.addShadow(name, dn))
            return false;
        break;

      default:
        MOZ_CRASH("unexpected kind");
    }

    return true;
}

template <>
bool
ParseContext<SyntaxParseHandler>::checkLocalsOverflow(TokenStream& ts)
{
    return true;
}

template <>
bool
ParseContext<SyntaxParseHandler>::define(TokenStream& ts, HandlePropertyName name, Node pn,
                                         Definition::Kind kind)
{
    MOZ_ASSERT(!decls_.lookupFirst(name));

    if (lexdeps.lookupDefn<SyntaxParseHandler>(name))
        lexdeps->remove(name);

    
    if (kind == Definition::ARG) {
        if (!args_.append((Definition*) nullptr))
            return false;
        if (args_.length() >= ARGNO_LIMIT) {
            ts.reportError(JSMSG_TOO_MANY_FUN_ARGS);
            return false;
        }
    }

    return decls_.addUnique(name, kind);
}

template <typename ParseHandler>
void
ParseContext<ParseHandler>::prepareToAddDuplicateArg(HandlePropertyName name, DefinitionNode prevDecl)
{
    MOZ_ASSERT(decls_.lookupFirst(name) == prevDecl);
    decls_.remove(name);
}

template <typename ParseHandler>
void
ParseContext<ParseHandler>::updateDecl(JSAtom* atom, Node pn)
{
    Definition* oldDecl = decls_.lookupFirst(atom);

    pn->setDefn(true);
    Definition* newDecl = (Definition*)pn;
    decls_.updateFirst(atom, newDecl);

    if (!sc->isFunctionBox()) {
        MOZ_ASSERT(newDecl->isFreeVar());
        return;
    }

    MOZ_ASSERT(oldDecl->isBound());
    MOZ_ASSERT(!oldDecl->pn_cookie.isFree());
    newDecl->pn_cookie = oldDecl->pn_cookie;
    newDecl->pn_dflags |= PND_BOUND;
    if (IsArgOp(oldDecl->getOp())) {
        newDecl->setOp(JSOP_GETARG);
        MOZ_ASSERT(args_[oldDecl->pn_cookie.slot()] == oldDecl);
        args_[oldDecl->pn_cookie.slot()] = newDecl;
    } else {
        MOZ_ASSERT(IsLocalOp(oldDecl->getOp()));
        newDecl->setOp(JSOP_GETLOCAL);
        MOZ_ASSERT(vars_[oldDecl->pn_cookie.slot()] == oldDecl);
        vars_[oldDecl->pn_cookie.slot()] = newDecl;
    }
}

template <typename ParseHandler>
void
ParseContext<ParseHandler>::popLetDecl(JSAtom* atom)
{
    MOZ_ASSERT(ParseHandler::getDefinitionKind(decls_.lookupFirst(atom)) == Definition::LET ||
               ParseHandler::getDefinitionKind(decls_.lookupFirst(atom)) == Definition::CONST);
    decls_.remove(atom);
}

template <typename ParseHandler>
static void
AppendPackedBindings(const ParseContext<ParseHandler>* pc, const DeclVector& vec, Binding* dst,
                     uint32_t* numUnaliased = nullptr)
{
    for (size_t i = 0; i < vec.length(); ++i, ++dst) {
        Definition* dn = vec[i];
        PropertyName* name = dn->name();

        Binding::Kind kind;
        switch (dn->kind()) {
          case Definition::LET:
            
            
            
            
          case Definition::VAR:
            kind = Binding::VARIABLE;
            break;
          case Definition::CONST:
          case Definition::GLOBALCONST:
            kind = Binding::CONSTANT;
            break;
          case Definition::ARG:
            kind = Binding::ARGUMENT;
            break;
          default:
            MOZ_CRASH("unexpected dn->kind");
        }

        




        MOZ_ASSERT_IF(dn->isClosed(), pc->decls().lookupFirst(name) == dn);
        bool aliased = dn->isClosed() ||
                       (pc->sc->allLocalsAliased() &&
                        pc->decls().lookupFirst(name) == dn);

        *dst = Binding(name, kind, aliased);
        if (!aliased && numUnaliased)
            ++*numUnaliased;
    }
}

template <typename ParseHandler>
bool
ParseContext<ParseHandler>::generateFunctionBindings(ExclusiveContext* cx, TokenStream& ts,
                                                     LifoAlloc& alloc,
                                                     InternalHandle<Bindings*> bindings) const
{
    MOZ_ASSERT(sc->isFunctionBox());
    MOZ_ASSERT(args_.length() < ARGNO_LIMIT);
    MOZ_ASSERT(vars_.length() + bodyLevelLexicals_.length() < LOCALNO_LIMIT);

    



    if (UINT32_MAX - args_.length() <= vars_.length() + bodyLevelLexicals_.length())
        return ts.reportError(JSMSG_TOO_MANY_LOCALS);

    
    
    for (size_t i = 0; i < bodyLevelLexicals_.length(); i++) {
        Definition* dn = bodyLevelLexicals_[i];
        if (!dn->pn_cookie.set(ts, dn->pn_cookie.level(), vars_.length() + i))
            return false;
    }

    uint32_t count = args_.length() + vars_.length() + bodyLevelLexicals_.length();
    Binding* packedBindings = alloc.newArrayUninitialized<Binding>(count);
    if (!packedBindings) {
        ReportOutOfMemory(cx);
        return false;
    }

    uint32_t numUnaliasedVars = 0;
    uint32_t numUnaliasedBodyLevelLexicals = 0;

    AppendPackedBindings(this, args_, packedBindings);
    AppendPackedBindings(this, vars_, packedBindings + args_.length(), &numUnaliasedVars);
    AppendPackedBindings(this, bodyLevelLexicals_,
                         packedBindings + args_.length() + vars_.length(), &numUnaliasedBodyLevelLexicals);

    return Bindings::initWithTemporaryStorage(cx, bindings, args_.length(), vars_.length(),
                                              bodyLevelLexicals_.length(), blockScopeDepth,
                                              numUnaliasedVars, numUnaliasedBodyLevelLexicals,
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
Parser<ParseHandler>::reportNoOffset(ParseReportKind kind, bool strict, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = reportHelper(kind, strict, TokenStream::NoOffset, errorNumber, args);
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
Parser<ParseHandler>::Parser(ExclusiveContext* cx, LifoAlloc* alloc,
                             const ReadOnlyCompileOptions& options,
                             const char16_t* chars, size_t length, bool foldConstants,
                             Parser<SyntaxParseHandler>* syntaxParser,
                             LazyScript* lazyOuterFunction)
  : AutoGCRooter(cx, PARSER),
    context(cx),
    alloc(*alloc),
    tokenStream(cx, options, chars, length, thisForCtor()),
    traceListHead(nullptr),
    pc(nullptr),
    sct(nullptr),
    ss(nullptr),
    keepAtoms(cx->perThreadData),
    foldConstants(foldConstants),
#ifdef DEBUG
    checkOptionsCalled(false),
#endif
    abortedSyntaxParse(false),
    isUnexpectedEOF_(false),
    handler(cx, *alloc, tokenStream, syntaxParser, lazyOuterFunction)
{
    {
        AutoLockForExclusiveAccess lock(cx);
        cx->perThreadData->addActiveCompilation();
    }

    
    
    
    if (options.extraWarningsOption)
        handler.disableSyntaxParser();

    tempPoolMark = alloc->mark();
}

template<typename ParseHandler>
bool
Parser<ParseHandler>::checkOptions()
{
#ifdef DEBUG
    checkOptionsCalled = true;
#endif

    if (!tokenStream.checkOptions())
        return false;

    return true;
}

template <typename ParseHandler>
Parser<ParseHandler>::~Parser()
{
    MOZ_ASSERT(checkOptionsCalled);

    alloc.release(tempPoolMark);

    




    alloc.freeAllIfHugeAndUnused();

    {
        AutoLockForExclusiveAccess lock(context);
        context->perThreadData->removeActiveCompilation();
    }
}

template <typename ParseHandler>
ObjectBox*
Parser<ParseHandler>::newObjectBox(JSObject* obj)
{
    MOZ_ASSERT(obj);

    







    ObjectBox* objbox = alloc.new_<ObjectBox>(obj, traceListHead);
    if (!objbox) {
        ReportOutOfMemory(context);
        return nullptr;
    }

    traceListHead = objbox;

    return objbox;
}

template <typename ParseHandler>
FunctionBox::FunctionBox(ExclusiveContext* cx, ObjectBox* traceListHead, JSFunction* fun,
                         ParseContext<ParseHandler>* outerpc, Directives directives,
                         bool extraWarnings, GeneratorKind generatorKind)
  : ObjectBox(fun, traceListHead),
    SharedContext(cx, directives, extraWarnings),
    bindings(),
    bufStart(0),
    bufEnd(0),
    length(0),
    generatorKindBits_(GeneratorKindAsBits(generatorKind)),
    inWith(false),                  
    inGenexpLambda(false),
    hasDestructuringArgs(false),
    useAsm(false),
    insideUseAsm(outerpc && outerpc->useAsmOrInsideUseAsm()),
    usesArguments(false),
    usesApply(false),
    usesThis(false),
    funCxFlags()
{
    
    
    
    MOZ_ASSERT(fun->isTenured());

    if (!outerpc) {
        inWith = false;

    } else if (outerpc->parsingWith) {
        
        
        
        
        
        
        inWith = true;

    } else if (outerpc->sc->isFunctionBox()) {
        
        
        
        
        
        
        
        FunctionBox* parent = outerpc->sc->asFunctionBox();
        if (parent && parent->inWith)
            inWith = true;
    }
}

template <typename ParseHandler>
FunctionBox*
Parser<ParseHandler>::newFunctionBox(Node fn, JSFunction* fun, ParseContext<ParseHandler>* outerpc,
                                     Directives inheritedDirectives, GeneratorKind generatorKind)
{
    MOZ_ASSERT(fun);

    






    FunctionBox* funbox =
        alloc.new_<FunctionBox>(context, traceListHead, fun, outerpc,
                                inheritedDirectives, options().extraWarningsOption,
                                generatorKind);
    if (!funbox) {
        ReportOutOfMemory(context);
        return nullptr;
    }

    traceListHead = funbox;
    if (fn)
        handler.setFunctionBox(fn, funbox);

    return funbox;
}

template <typename ParseHandler>
void
Parser<ParseHandler>::trace(JSTracer* trc)
{
    traceListHead->trace(trc);
}

void
MarkParser(JSTracer* trc, AutoGCRooter* parser)
{
    static_cast<Parser<FullParseHandler>*>(parser)->trace(trc);
}




template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::parse(JSObject* chain)
{
    MOZ_ASSERT(checkOptionsCalled);

    







    Directives directives(options().strictOption);
    GlobalSharedContext globalsc(context, directives, options().extraWarningsOption,
                                  false);
    ParseContext<ParseHandler> globalpc(this,  nullptr, ParseHandler::null(),
                                        &globalsc,  nullptr,
                                         0,  0,
                                         0);
    if (!globalpc.init(tokenStream))
        return null();

    Node pn = statements();
    if (pn) {
        TokenKind tt;
        if (!tokenStream.getToken(&tt))
            return null();
        if (tt != TOK_EOF) {
            report(ParseError, false, null(), JSMSG_GARBAGE_AFTER_INPUT,
                   "script", TokenKindToDesc(tt));
            return null();
        }
        if (foldConstants) {
            if (!FoldConstants(context, &pn, this))
                return null();
        }
    }
    return pn;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::reportBadReturn(Node pn, ParseReportKind kind,
                                      unsigned errnum, unsigned anonerrnum)
{
    JSAutoByteString name;
    JSAtom* atom = pc->sc->asFunctionBox()->function()->atom();
    if (atom) {
        if (!AtomToPrintableString(context, atom, &name))
            return false;
    } else {
        errnum = anonerrnum;
    }
    return report(kind, pc->sc->strict(), pn, errnum, name.ptr());
}





template <typename ParseHandler>
bool
Parser<ParseHandler>::checkStrictAssignment(Node lhs)
{
    if (!pc->sc->needStrictChecks())
        return true;

    JSAtom* atom = handler.isName(lhs);
    if (!atom)
        return true;

    if (atom == context->names().eval || atom == context->names().arguments) {
        JSAutoByteString name;
        if (!AtomToPrintableString(context, atom, &name))
            return false;

        if (!report(ParseStrictError, pc->sc->strict(), lhs, JSMSG_BAD_STRICT_ASSIGN, name.ptr()))
            return false;
    }
    return true;
}







template <typename ParseHandler>
bool
Parser<ParseHandler>::checkStrictBinding(PropertyName* name, Node pn)
{
    if (!pc->sc->needStrictChecks())
        return true;

    if (name == context->names().eval || name == context->names().arguments || IsKeyword(name)) {
        JSAutoByteString bytes;
        if (!AtomToPrintableString(context, name, &bytes))
            return false;
        return report(ParseStrictError, pc->sc->strict(), pn,
                      JSMSG_BAD_BINDING, bytes.ptr());
    }

    return true;
}

template <>
ParseNode*
Parser<FullParseHandler>::standaloneFunctionBody(HandleFunction fun, const AutoNameVector& formals,
                                                 GeneratorKind generatorKind,
                                                 Directives inheritedDirectives,
                                                 Directives* newDirectives)
{
    MOZ_ASSERT(checkOptionsCalled);

    Node fn = handler.newFunctionDefinition();
    if (!fn)
        return null();

    ParseNode* argsbody = handler.newList(PNK_ARGSBODY);
    if (!argsbody)
        return null();
    fn->pn_body = argsbody;

    FunctionBox* funbox = newFunctionBox(fn, fun,  nullptr, inheritedDirectives,
                                         generatorKind);
    if (!funbox)
        return null();
    funbox->length = fun->nargs() - fun->hasRest();
    handler.setFunctionBox(fn, funbox);

    ParseContext<FullParseHandler> funpc(this, pc, fn, funbox, newDirectives,
                                          0,  0,
                                          0);
    if (!funpc.init(tokenStream))
        return null();

    for (unsigned i = 0; i < formals.length(); i++) {
        if (!defineArg(fn, formals[i]))
            return null();
    }

    ParseNode* pn = functionBody(Statement, StatementListBody);
    if (!pn)
        return null();

    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();
    if (tt != TOK_EOF) {
        report(ParseError, false, null(), JSMSG_GARBAGE_AFTER_INPUT,
               "function body", TokenKindToDesc(tt));
        return null();
    }

    if (!FoldConstants(context, &pn, this))
        return null();

    InternalHandle<Bindings*> funboxBindings =
        InternalHandle<Bindings*>::fromMarkedLocation(&funbox->bindings);
    if (!funpc.generateFunctionBindings(context, tokenStream, alloc, funboxBindings))
        return null();

    MOZ_ASSERT(fn->pn_body->isKind(PNK_ARGSBODY));
    fn->pn_body->append(pn);
    fn->pn_body->pn_pos = pn->pn_pos;
    return fn;
}

template <>
bool
Parser<FullParseHandler>::checkFunctionArguments()
{
    



    if (FuncStmtSet* set = pc->funcStmts) {
        for (FuncStmtSet::Range r = set->all(); !r.empty(); r.popFront()) {
            PropertyName* name = r.front()->asPropertyName();
            if (Definition* dn = pc->decls().lookupFirst(name))
                dn->pn_dflags |= PND_CLOSED;
        }
    }

    
    HandlePropertyName arguments = context->names().arguments;

    




    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            Definition* dn = r.front().value().get<FullParseHandler>();
            pc->lexdeps->remove(arguments);
            dn->pn_dflags |= PND_IMPLICITARGUMENTS;
            if (!pc->define(tokenStream, arguments, dn, Definition::VAR))
                return false;
            pc->sc->asFunctionBox()->usesArguments = true;
            break;
        }
    }

    



    Definition* maybeArgDef = pc->decls().lookupFirst(arguments);
    bool argumentsHasBinding = !!maybeArgDef;
    
    
    bool argumentsHasLocalBinding = maybeArgDef && (maybeArgDef->kind() != Definition::ARG &&
                                                    maybeArgDef->kind() != Definition::LET &&
                                                    maybeArgDef->kind() != Definition::CONST);
    bool hasRest = pc->sc->asFunctionBox()->function()->hasRest();
    if (hasRest && argumentsHasLocalBinding) {
        report(ParseError, false, nullptr, JSMSG_ARGUMENTS_AND_REST);
        return false;
    }

    




    if (!argumentsHasBinding && pc->sc->bindingsAccessedDynamically() && !hasRest) {
        ParseNode* pn = newName(arguments);
        if (!pn)
            return false;
        if (!pc->define(tokenStream, arguments, pn, Definition::VAR))
            return false;
        argumentsHasBinding = true;
        argumentsHasLocalBinding = true;
    }

    




    if (argumentsHasLocalBinding) {
        FunctionBox* funbox = pc->sc->asFunctionBox();
        funbox->setArgumentsHasLocalBinding();

        






        if (pc->sc->bindingsAccessedDynamically() && maybeArgDef)
            funbox->setDefinitelyNeedsArgsObj();

        





        if (pc->sc->hasDebuggerStatement())
            funbox->setDefinitelyNeedsArgsObj();

        






        if (pc->sc->needStrictChecks()) {
            for (AtomDefnListMap::Range r = pc->decls().all(); !r.empty(); r.popFront()) {
                DefinitionList& dlist = r.front().value();
                for (DefinitionList::Range dr = dlist.all(); !dr.empty(); dr.popFront()) {
                    Definition* dn = dr.front<FullParseHandler>();
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
    MOZ_ASSERT(pc->sc->isFunctionBox());
    MOZ_ASSERT(!pc->funHasReturnExpr && !pc->funHasReturnVoid);

#ifdef DEBUG
    uint32_t startYieldOffset = pc->lastYieldOffset;
#endif

    Node pn;
    if (type == StatementListBody) {
        pn = statements();
        if (!pn)
            return null();
    } else {
        MOZ_ASSERT(type == ExpressionBody);

        Node kid = assignExpr();
        if (!kid)
            return null();

        pn = handler.newReturnStatement(kid, null(), handler.getPosition(kid));
        if (!pn)
            return null();
    }

    switch (pc->generatorKind()) {
      case NotGenerator:
        MOZ_ASSERT(pc->lastYieldOffset == startYieldOffset);
        break;

      case LegacyGenerator:
        
        MOZ_ASSERT(pc->lastYieldOffset != startYieldOffset);
        if (kind == Arrow) {
            reportWithOffset(ParseError, false, pc->lastYieldOffset,
                             JSMSG_YIELD_IN_ARROW, js_yield_str);
            return null();
        }
        if (type == ExpressionBody) {
            reportBadReturn(pn, ParseError,
                            JSMSG_BAD_GENERATOR_RETURN,
                            JSMSG_BAD_ANON_GENERATOR_RETURN);
            return null();
        }
        break;

      case StarGenerator:
        MOZ_ASSERT(kind != Arrow);
        MOZ_ASSERT(type == StatementListBody);
        break;
    }

    if (pc->isGenerator()) {
        MOZ_ASSERT(type == StatementListBody);
        Node generator = newName(context->names().dotGenerator);
        if (!generator)
            return null();
        if (!pc->define(tokenStream, context->names().dotGenerator, generator, Definition::VAR))
            return null();

        if (pc->isStarGenerator()) {
            Node genrval = newName(context->names().dotGenRVal);
            if (!genrval)
                return null();
            if (!pc->define(tokenStream, context->names().dotGenRVal, genrval, Definition::VAR))
                return null();
        }

        generator = newName(context->names().dotGenerator);
        if (!generator)
            return null();
        if (!noteNameUse(context->names().dotGenerator, generator))
            return null();
        if (!handler.prependInitialYield(pn, generator))
            return null();
    }

    
    if (!checkFunctionArguments())
        return null();

    return pn;
}


template <>
bool
Parser<FullParseHandler>::makeDefIntoUse(Definition* dn, ParseNode* pn, JSAtom* atom)
{
    
    pc->updateDecl(atom, pn);

    
    for (ParseNode* pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
        MOZ_ASSERT(pnu->isUsed());
        MOZ_ASSERT(!pnu->isDefn());
        pnu->pn_lexdef = (Definition*) pn;
        pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
    }
    pn->pn_dflags |= dn->pn_dflags & PND_USE2DEF_FLAGS;
    pn->dn_uses = dn;

    














    if (dn->getKind() == PNK_FUNCTION) {
        MOZ_ASSERT(dn->functionIsHoisted());
        pn->dn_uses = dn->pn_link;
        handler.prepareNodeForMutation(dn);
        dn->setKind(PNK_NOP);
        dn->setArity(PN_NULLARY);
        dn->setDefn(false);
        return true;
    }

    




    if (dn->canHaveInitializer()) {
        if (ParseNode* rhs = dn->expr()) {
            ParseNode* lhs = handler.makeAssignment(dn, rhs);
            if (!lhs)
                return false;
            pn->dn_uses = lhs;
            dn->pn_link = nullptr;
            dn = (Definition*) lhs;
        }
    }

    
    MOZ_ASSERT(dn->isKind(PNK_NAME));
    MOZ_ASSERT(dn->isArity(PN_NAME));
    MOZ_ASSERT(dn->pn_atom == atom);
    dn->setOp((js_CodeSpec[dn->getOp()].format & JOF_SET) ? JSOP_SETNAME : JSOP_GETNAME);
    dn->setDefn(false);
    dn->setUsed(true);
    dn->pn_lexdef = (Definition*) pn;
    dn->pn_cookie.makeFree();
    dn->pn_dflags &= ~PND_BOUND;
    return true;
}









template <typename ParseHandler>
struct BindData
{
    explicit BindData(ExclusiveContext* cx) : let(cx) {}

    typedef bool
    (*Binder)(BindData* data, HandlePropertyName name, Parser<ParseHandler>* parser);

    
    typename ParseHandler::Node pn;

    JSOp            op;         
    Binder          binder;     
    bool            isConst;    

    struct LetData {
        explicit LetData(ExclusiveContext* cx) : blockObj(cx) {}
        VarContext varContext;
        RootedStaticBlockObject blockObj;
        unsigned   overflow;
    } let;

    void initLexical(VarContext varContext, StaticBlockObject* blockObj, unsigned overflow,
                     bool isConst = false) {
        this->pn = ParseHandler::null();
        this->op = JSOP_INITLEXICAL;
        this->isConst = isConst;
        this->binder = Parser<ParseHandler>::bindLexical;
        this->let.varContext = varContext;
        this->let.blockObj = blockObj;
        this->let.overflow = overflow;
    }

    void initVarOrGlobalConst(JSOp op) {
        this->op = op;
        this->isConst = op == JSOP_DEFCONST;
        this->binder = Parser<ParseHandler>::bindVarOrGlobalConst;
    }
};

template <typename ParseHandler>
JSFunction*
Parser<ParseHandler>::newFunction(HandleAtom atom, FunctionSyntaxKind kind, HandleObject proto)
{
    MOZ_ASSERT_IF(kind == Statement, atom != nullptr);

    RootedFunction fun(context);
    
    JSFunction::Flags flags;
    switch(kind) {
      case Expression:
        flags = JSFunction::INTERPRETED_LAMBDA;
        break;
      case Arrow:
        flags = JSFunction::INTERPRETED_LAMBDA_ARROW;
        break;
      case Method:
        flags = JSFunction::INTERPRETED_METHOD;
        break;
      default:
        flags = JSFunction::INTERPRETED;
        break;
    }
    
    gc::AllocKind allocKind = JSFunction::FinalizeKind;
    if (kind == Arrow || kind == Method)
        allocKind = JSFunction::ExtendedFinalizeKind;
    fun = NewFunctionWithProto(context, nullptr, 0, flags, NullPtr(), atom, proto,
                               allocKind, TenuredObject);
    if (!fun)
        return nullptr;
    if (options().selfHostingMode)
        fun->setIsSelfHostedBuiltin();
    return fun;
}

static bool
MatchOrInsertSemicolon(TokenStream& ts)
{
    TokenKind tt;
    if (!ts.peekTokenSameLine(&tt, TokenStream::Operand))
        return false;
    if (tt != TOK_EOF && tt != TOK_EOL && tt != TOK_SEMI && tt != TOK_RC) {
        
        ts.consumeKnownToken(tt);
        ts.reportError(JSMSG_SEMI_BEFORE_STMNT);
        return false;
    }
    bool ignored;
    return ts.matchToken(&ignored, TOK_SEMI);
}

template <typename ParseHandler>
typename ParseHandler::DefinitionNode
Parser<ParseHandler>::getOrCreateLexicalDependency(ParseContext<ParseHandler>* pc, JSAtom* atom)
{
    AtomDefnAddPtr p = pc->lexdeps->lookupForAdd(atom);
    if (p)
        return p.value().get<ParseHandler>();

    DefinitionNode dn = handler.newPlaceholder(atom, pc->blockid(), pos());
    if (!dn)
        return ParseHandler::nullDefinition();
    DefinitionSingle def = DefinitionSingle::new_<ParseHandler>(dn);
    if (!pc->lexdeps->add(p, atom, def))
        return ParseHandler::nullDefinition();
    return dn;
}

static bool
ConvertDefinitionToNamedLambdaUse(TokenStream& ts, ParseContext<FullParseHandler>* pc,
                                  FunctionBox* funbox, Definition* dn)
{
    dn->setOp(JSOP_CALLEE);
    if (!dn->pn_cookie.set(ts, pc->staticLevel, 0))
        return false;
    dn->pn_dflags |= PND_BOUND;
    MOZ_ASSERT(dn->kind() == Definition::NAMED_LAMBDA);

    










    if (dn->isClosed() || dn->isAssigned())
        funbox->setNeedsDeclEnvObject();
    return true;
}

static bool
IsNonDominatingInScopedSwitch(ParseContext<FullParseHandler>* pc, HandleAtom name,
                              Definition* dn)
{
    MOZ_ASSERT(dn->isLexical());
    StmtInfoPC* stmt = LexicalLookup(pc, name, nullptr, (StmtInfoPC*)nullptr);
    if (stmt && stmt->type == STMT_SWITCH)
        return dn->pn_cookie.slot() < stmt->firstDominatingLexicalInCase;
    return false;
}

static void
AssociateUsesWithOuterDefinition(ParseNode* pnu, Definition* dn, Definition* outer_dn,
                                 bool markUsesAsLexical)
{
    uint32_t dflags = markUsesAsLexical ? PND_LEXICAL : 0;
    while (true) {
        pnu->pn_lexdef = outer_dn;
        pnu->pn_dflags |= dflags;
        if (!pnu->pn_link)
            break;
        pnu = pnu->pn_link;
    }
    pnu->pn_link = outer_dn->dn_uses;
    outer_dn->dn_uses = dn->dn_uses;
    dn->dn_uses = nullptr;
}







template <>
bool
Parser<FullParseHandler>::leaveFunction(ParseNode* fn, ParseContext<FullParseHandler>* outerpc,
                                        FunctionSyntaxKind kind)
{
    outerpc->blockidGen = pc->blockidGen;

    bool bodyLevel = outerpc->atBodyLevel();
    FunctionBox* funbox = fn->pn_funbox;
    MOZ_ASSERT(funbox == pc->sc->asFunctionBox());

    
    if (pc->lexdeps->count()) {
        for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
            JSAtom* atom = r.front().key();
            Definition* dn = r.front().value().get<FullParseHandler>();
            MOZ_ASSERT(dn->isPlaceholder());

            if (atom == funbox->function()->name() && kind == Expression) {
                if (!ConvertDefinitionToNamedLambdaUse(tokenStream, pc, funbox, dn))
                    return false;
                continue;
            }

            Definition* outer_dn = outerpc->decls().lookupFirst(atom);

            




            if (funbox->hasExtensibleScope() || outerpc->parsingWith)
                handler.deoptimizeUsesWithin(dn, fn->pn_pos);

            if (!outer_dn) {
                




















                outer_dn = getOrCreateLexicalDependency(outerpc, atom);
                if (!outer_dn)
                    return false;
            }

            












            if (dn != outer_dn) {
                if (ParseNode* pnu = dn->dn_uses) {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    RootedAtom name(context, atom);
                    bool markUsesAsLexical = outer_dn->isLexical() &&
                                             (bodyLevel ||
                                              IsNonDominatingInScopedSwitch(outerpc, name, outer_dn));
                    AssociateUsesWithOuterDefinition(pnu, dn, outer_dn, markUsesAsLexical);
                }

                outer_dn->pn_dflags |= dn->pn_dflags & ~PND_PLACEHOLDER;
            }

            
            outer_dn->pn_dflags |= PND_CLOSED;
        }
    }

    InternalHandle<Bindings*> bindings =
        InternalHandle<Bindings*>::fromMarkedLocation(&funbox->bindings);
    return pc->generateFunctionBindings(context, tokenStream, alloc, bindings);
}

template <>
bool
Parser<SyntaxParseHandler>::leaveFunction(Node fn, ParseContext<SyntaxParseHandler>* outerpc,
                                          FunctionSyntaxKind kind)
{
    outerpc->blockidGen = pc->blockidGen;

    FunctionBox* funbox = pc->sc->asFunctionBox();
    return addFreeVariablesFromLazyFunction(funbox->function(), outerpc);
}















template <typename ParseHandler>
bool
Parser<ParseHandler>::defineArg(Node funcpn, HandlePropertyName name,
                                bool disallowDuplicateArgs, Node* duplicatedArg)
{
    SharedContext* sc = pc->sc;

    
    if (DefinitionNode prevDecl = pc->decls().lookupFirst(name)) {
        Node pn = handler.getDefinitionNode(prevDecl);

        





        if (sc->needStrictChecks()) {
            JSAutoByteString bytes;
            if (!AtomToPrintableString(context, name, &bytes))
                return false;
            if (!report(ParseStrictError, pc->sc->strict(), pn,
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

        
        MOZ_ASSERT(handler.getDefinitionKind(prevDecl) == Definition::ARG);
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

template <typename ParseHandler>
 bool
Parser<ParseHandler>::bindDestructuringArg(BindData<ParseHandler>* data,
                                           HandlePropertyName name, Parser<ParseHandler>* parser)
{
    ParseContext<ParseHandler>* pc = parser->pc;
    MOZ_ASSERT(pc->sc->isFunctionBox());

    if (pc->decls().lookupFirst(name)) {
        parser->report(ParseError, false, null(), JSMSG_BAD_DUP_ARGS);
        return false;
    }

    if (!parser->checkStrictBinding(name, data->pn))
        return false;

    return pc->define(parser->tokenStream, name, data->pn, Definition::VAR);
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::functionArguments(FunctionSyntaxKind kind, FunctionType type, Node* listp,
                                        Node funcpn, bool* hasRest)
{
    FunctionBox* funbox = pc->sc->asFunctionBox();

    *hasRest = false;

    bool parenFreeArrow = false;
    if (kind == Arrow) {
        TokenKind tt;
        if (!tokenStream.peekToken(&tt))
            return false;
        if (tt == TOK_NAME)
            parenFreeArrow = true;
    }
    if (!parenFreeArrow) {
        TokenKind tt;
        if (!tokenStream.getToken(&tt))
            return false;
        if (tt != TOK_LP) {
            report(ParseError, false, null(),
                   kind == Arrow ? JSMSG_BAD_ARROW_ARGS : JSMSG_PAREN_BEFORE_FORMAL);
            return false;
        }

        
        
        funbox->setStart(tokenStream);
    }

    Node argsbody = handler.newList(PNK_ARGSBODY);
    if (!argsbody)
        return false;
    handler.setFunctionBody(funcpn, argsbody);

    bool hasArguments = false;
    if (parenFreeArrow) {
        hasArguments = true;
    } else {
        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_RP))
            return false;
        if (!matched)
            hasArguments = true;
    }
    if (hasArguments) {
        bool hasDefaults = false;
        Node duplicatedArg = null();
        Node list = null();
        bool disallowDuplicateArgs = kind == Arrow || kind == Method;

        if (type == Getter) {
            report(ParseError, false, null(), JSMSG_ACCESSOR_WRONG_ARGS, "getter", "no", "s");
            return false;
        }

        while (true) {
            if (*hasRest) {
                report(ParseError, false, null(), JSMSG_PARAMETER_AFTER_REST);
                return false;
            }

            TokenKind tt;
            if (!tokenStream.getToken(&tt))
                return false;
            MOZ_ASSERT_IF(parenFreeArrow, tt == TOK_NAME);
            switch (tt) {
              case TOK_LB:
              case TOK_LC:
              {
                
                disallowDuplicateArgs = true;
                if (duplicatedArg) {
                    report(ParseError, false, duplicatedArg, JSMSG_BAD_DUP_ARGS);
                    return false;
                }

                if (hasDefaults) {
                    report(ParseError, false, null(), JSMSG_NONDEFAULT_FORMAL_AFTER_DEFAULT);
                    return false;
                }

                funbox->hasDestructuringArgs = true;

                





                BindData<ParseHandler> data(context);
                data.pn = ParseHandler::null();
                data.op = JSOP_DEFVAR;
                data.binder = bindDestructuringArg;
                Node lhs = destructuringExprWithoutYield(&data, tt, JSMSG_YIELD_IN_DEFAULT);
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
                    list = handler.newDeclarationList(PNK_VAR, item);
                    if (!list)
                        return false;
                    *listp = list;
                }
                break;
              }

              case TOK_YIELD:
                if (!checkYieldNameValidity())
                    return false;
                goto TOK_NAME;

              case TOK_TRIPLEDOT:
              {
                if (type == Setter) {
                    report(ParseError, false, null(),
                           JSMSG_ACCESSOR_WRONG_ARGS, "setter", "one", "");
                    return false;
                }
                *hasRest = true;
                if (!tokenStream.getToken(&tt))
                    return false;
                if (tt != TOK_NAME) {
                    report(ParseError, false, null(), JSMSG_NO_REST_NAME);
                    return false;
                }
                disallowDuplicateArgs = true;
                if (duplicatedArg) {
                    
                    report(ParseError, false, duplicatedArg, JSMSG_BAD_DUP_ARGS);
                    return false;
                }
                goto TOK_NAME;
              }

              TOK_NAME:
              case TOK_NAME:
              {
                if (parenFreeArrow)
                    funbox->setStart(tokenStream);

                RootedPropertyName name(context, tokenStream.currentName());
                if (!defineArg(funcpn, name, disallowDuplicateArgs, &duplicatedArg))
                    return false;

                bool matched;
                if (!tokenStream.matchToken(&matched, TOK_ASSIGN))
                    return false;
                if (matched) {
                    
                    
                    
                    
                    MOZ_ASSERT(!parenFreeArrow);

                    if (*hasRest) {
                        report(ParseError, false, null(), JSMSG_REST_WITH_DEFAULT);
                        return false;
                    }
                    disallowDuplicateArgs = true;
                    if (duplicatedArg) {
                        report(ParseError, false, duplicatedArg, JSMSG_BAD_DUP_ARGS);
                        return false;
                    }
                    if (!hasDefaults) {
                        hasDefaults = true;

                        
                        
                        funbox->length = pc->numArgs() - 1;
                    }
                    Node def_expr = assignExprWithoutYield(JSMSG_YIELD_IN_DEFAULT);
                    if (!def_expr)
                        return false;
                    handler.setLastFunctionArgumentDefault(funcpn, def_expr);
                }

                break;
              }

              default:
                report(ParseError, false, null(), JSMSG_MISSING_FORMAL);
                return false;
            }

            if (parenFreeArrow || type == Setter)
                break;

            bool matched;
            if (!tokenStream.matchToken(&matched, TOK_COMMA))
                return false;
            if (!matched)
                break;
        }

        if (!parenFreeArrow) {
            TokenKind tt;
            if (!tokenStream.getToken(&tt))
                return false;
            if (tt != TOK_RP) {
                if (type == Setter) {
                    report(ParseError, false, null(),
                           JSMSG_ACCESSOR_WRONG_ARGS, "setter", "one", "");
                    return false;
                }

                report(ParseError, false, null(), JSMSG_PAREN_AFTER_FORMAL);
                return false;
            }
        }

        if (!hasDefaults)
            funbox->length = pc->numArgs() - *hasRest;
    } else if (type == Setter) {
        report(ParseError, false, null(), JSMSG_ACCESSOR_WRONG_ARGS, "setter", "one", "");
        return false;
    }

    return true;
}

template <>
bool
Parser<FullParseHandler>::checkFunctionDefinition(HandlePropertyName funName,
                                                  ParseNode** pn_, FunctionSyntaxKind kind,
                                                  bool* pbodyProcessed)
{
    ParseNode*& pn = *pn_;
    *pbodyProcessed = false;

    
    bool bodyLevel = pc->atBodyLevel();

    if (kind == Statement) {
        



        if (Definition* dn = pc->decls().lookupFirst(funName)) {
            MOZ_ASSERT(!dn->isUsed());
            MOZ_ASSERT(dn->isDefn());

            bool throwRedeclarationError = dn->kind() == Definition::GLOBALCONST ||
                                           dn->kind() == Definition::CONST ||
                                           dn->kind() == Definition::LET;
            if (options().extraWarningsOption || throwRedeclarationError) {
                JSAutoByteString name;
                ParseReportKind reporter = throwRedeclarationError
                                           ? ParseError
                                           : ParseExtraWarning;
                if (!AtomToPrintableString(context, funName, &name) ||
                    !report(reporter, false, nullptr, JSMSG_REDECLARED_VAR,
                            Definition::kindString(dn->kind()), name.ptr()))
                {
                    return false;
                }
            }

            







            if (bodyLevel) {
                if (dn->kind() == Definition::ARG) {
                    
                    
                    
                    
                    pn->setOp(JSOP_GETARG);
                    pn->setDefn(true);
                    pn->pn_cookie = dn->pn_cookie;
                    pn->pn_dflags |= PND_BOUND;
                    dn->markAsAssigned();
                } else {
                    if (!makeDefIntoUse(dn, pn, funName))
                        return false;
                }
            }
        } else if (bodyLevel) {
            




            if (Definition* fn = pc->lexdeps.lookupDefn<FullParseHandler>(funName)) {
                MOZ_ASSERT(fn->isDefn());
                fn->setKind(PNK_FUNCTION);
                fn->setArity(PN_CODE);
                fn->pn_pos.begin = pn->pn_pos.begin;
                fn->pn_pos.end = pn->pn_pos.end;

                fn->pn_body = nullptr;
                fn->pn_cookie.makeFree();

                pc->lexdeps->remove(funName);
                handler.freeTree(pn);
                pn = fn;
            }

            if (!pc->define(tokenStream, funName, pn, Definition::VAR))
                return false;
        }

        if (bodyLevel) {
            MOZ_ASSERT(pn->functionIsHoisted());
            MOZ_ASSERT_IF(pc->sc->isFunctionBox(), !pn->pn_cookie.isFree());
            MOZ_ASSERT_IF(!pc->sc->isFunctionBox(), pn->pn_cookie.isFree());
        } else {
            




            MOZ_ASSERT(!pc->sc->strict());
            MOZ_ASSERT(pn->pn_cookie.isFree());
            if (pc->sc->isFunctionBox()) {
                FunctionBox* funbox = pc->sc->asFunctionBox();
                funbox->setMightAliasLocals();
                funbox->setHasExtensibleScope();
            }
            pn->setOp(JSOP_DEFFUN);

            





            if (!pc->funcStmts) {
                pc->funcStmts = alloc.new_<FuncStmtSet>(alloc);
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
        
        pn->setOp(kind == Arrow ? JSOP_LAMBDA_ARROW : JSOP_LAMBDA);
    }

    
    
    
    
    if (LazyScript* lazyOuter = handler.lazyOuterFunction()) {
        JSFunction* fun = handler.nextLazyInnerFunction();
        MOZ_ASSERT(!fun->isLegacyGenerator());
        FunctionBox* funbox = newFunctionBox(pn, fun, pc, Directives( false),
                                             fun->generatorKind());
        if (!funbox)
            return false;

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
PropagateTransitiveParseFlags(const T* inner, U* outer)
{
    if (inner->bindingsAccessedDynamically())
        outer->setBindingsAccessedDynamically();
    if (inner->hasDebuggerStatement())
        outer->setHasDebuggerStatement();
    if (inner->hasDirectEval())
        outer->setHasDirectEval();
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::addFreeVariablesFromLazyFunction(JSFunction* fun,
                                                       ParseContext<ParseHandler>* pc)
{
    
    

    bool bodyLevel = pc->atBodyLevel();
    LazyScript* lazy = fun->lazyScript();
    LazyScript::FreeVariable* freeVariables = lazy->freeVariables();
    for (size_t i = 0; i < lazy->numFreeVariables(); i++) {
        JSAtom* atom = freeVariables[i].atom();

        
        if (atom == context->names().arguments)
            continue;

        DefinitionNode dn = pc->decls().lookupFirst(atom);

        if (!dn) {
            dn = getOrCreateLexicalDependency(pc, atom);
            if (!dn)
                return false;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (handler.isPlaceholderDefinition(dn) || bodyLevel)
            freeVariables[i].setIsHoistedUse();

        
        handler.setFlag(handler.getDefinitionNode(dn), PND_CLOSED);
    }

    PropagateTransitiveParseFlags(lazy, pc->sc);
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkFunctionDefinition(HandlePropertyName funName,
                                                    Node* pn, FunctionSyntaxKind kind,
                                                    bool* pbodyProcessed)
{
    *pbodyProcessed = false;

    
    bool bodyLevel = pc->atBodyLevel();

    if (kind == Statement) {
        



        if (DefinitionNode dn = pc->decls().lookupFirst(funName)) {
            if (dn == Definition::GLOBALCONST ||
                dn == Definition::CONST       ||
                dn == Definition::LET)
            {
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
bool
Parser<ParseHandler>::addExprAndGetNextTemplStrToken(Node nodeList, TokenKind* ttp)
{
    Node pn = expr();
    if (!pn)
        return false;
    handler.addList(nodeList, pn);

    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return false;
    if (tt != TOK_RC) {
        report(ParseError, false, null(), JSMSG_TEMPLSTR_UNTERM_EXPR);
        return false;
    }

    return tokenStream.getToken(ttp, TokenStream::TemplateTail);
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::taggedTemplate(Node nodeList, TokenKind tt)
{
    Node callSiteObjNode = handler.newCallSiteObject(pos().begin, pc->blockidGen);
    if (!callSiteObjNode)
        return false;
    handler.addList(nodeList, callSiteObjNode);

    while (true) {
        if (!appendToCallSiteObj(callSiteObjNode))
            return false;
        if (tt != TOK_TEMPLATE_HEAD)
            break;

        if (!addExprAndGetNextTemplStrToken(nodeList, &tt))
            return false;
    }
    handler.setEndPosition(nodeList, callSiteObjNode);
    return true;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::templateLiteral()
{
    Node pn = noSubstitutionTemplate();
    if (!pn)
        return null();
    Node nodeList = handler.newList(PNK_TEMPLATE_STRING_LIST, pn);

    TokenKind tt;
    do {
        if (!addExprAndGetNextTemplStrToken(nodeList, &tt))
            return null();

        pn = noSubstitutionTemplate();
        if (!pn)
            return null();

        handler.addList(nodeList, pn);
    } while (tt == TOK_TEMPLATE_HEAD);
    return nodeList;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionDef(HandlePropertyName funName,
                                  FunctionType type, FunctionSyntaxKind kind,
                                  GeneratorKind generatorKind, InvokedPrediction invoked)
{
    MOZ_ASSERT_IF(kind == Statement, funName);

    
    Node pn = handler.newFunctionDefinition();
    if (!pn)
        return null();

    if (invoked)
        pn = handler.setLikelyIIFE(pn);

    bool bodyProcessed;
    if (!checkFunctionDefinition(funName, &pn, kind, &bodyProcessed))
        return null();

    if (bodyProcessed)
        return pn;

    RootedObject proto(context);
    if (generatorKind == StarGenerator) {
        
        
        
        JSContext* cx = context->maybeJSContext();
        proto = GlobalObject::getOrCreateStarGeneratorFunctionPrototype(cx, context->global());
        if (!proto)
            return null();
    }
    RootedFunction fun(context, newFunction(funName, kind, proto));
    if (!fun)
        return null();

    
    
    
    
    Directives directives(pc);
    Directives newDirectives = directives;

    TokenStream::Position start(keepAtoms);
    tokenStream.tell(&start);

    while (true) {
        if (functionArgsAndBody(pn, fun, type, kind, generatorKind, directives, &newDirectives))
            break;
        if (tokenStream.hadError() || directives == newDirectives)
            return null();

        
        MOZ_ASSERT_IF(directives.strict(), newDirectives.strict());
        MOZ_ASSERT_IF(directives.asmJS(), newDirectives.asmJS());
        directives = newDirectives;

        tokenStream.seek(start);

        
        handler.setFunctionBody(pn, null());
    }

    return pn;
}

template <>
bool
Parser<FullParseHandler>::finishFunctionDefinition(ParseNode* pn, FunctionBox* funbox,
                                                   ParseNode* prelude, ParseNode* body)
{
    pn->pn_pos.end = pos().end;

    





    if (prelude) {
        if (!body->isArity(PN_LIST)) {
            ParseNode* block;

            block = handler.newList(PNK_SEQ, body);
            if (!block)
                return false;
            body = block;
        }

        ParseNode* item = handler.new_<UnaryNode>(PNK_SEMI, JSOP_NOP,
                                                  TokenPos(body->pn_pos.begin, body->pn_pos.begin),
                                                  prelude);
        if (!item)
            return false;

        body->prepend(item);
        body->pn_xflags |= PNX_DESTRUCT;
    }

    MOZ_ASSERT(pn->pn_funbox == funbox);
    MOZ_ASSERT(pn->pn_body->isKind(PNK_ARGSBODY));
    pn->pn_body->append(body);

    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::finishFunctionDefinition(Node pn, FunctionBox* funbox,
                                                     Node prelude, Node body)
{
    
    
    

    if (funbox->inWith)
        return abortIfSyntaxParser();

    size_t numFreeVariables = pc->lexdeps->count();
    size_t numInnerFunctions = pc->innerFunctions.length();

    RootedFunction fun(context, funbox->function());
    LazyScript* lazy = LazyScript::CreateRaw(context, fun, numFreeVariables, numInnerFunctions,
                                             versionNumber(), funbox->bufStart, funbox->bufEnd,
                                             funbox->startLine, funbox->startColumn);
    if (!lazy)
        return false;

    LazyScript::FreeVariable* freeVariables = lazy->freeVariables();
    size_t i = 0;
    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront())
        freeVariables[i++] = LazyScript::FreeVariable(r.front().key());
    MOZ_ASSERT(i == numFreeVariables);

    HeapPtrFunction* innerFunctions = lazy->innerFunctions();
    for (size_t i = 0; i < numInnerFunctions; i++)
        innerFunctions[i].init(pc->innerFunctions[i]);

    if (pc->sc->strict())
        lazy->setStrict();
    lazy->setGeneratorKind(funbox->generatorKind());
    if (funbox->usesArguments && funbox->usesApply && funbox->usesThis)
        lazy->setUsesArgumentsApplyAndThis();
    PropagateTransitiveParseFlags(funbox, lazy);

    fun->initLazyScript(lazy);
    return true;
}

template <>
bool
Parser<FullParseHandler>::functionArgsAndBody(ParseNode* pn, HandleFunction fun,
                                              FunctionType type, FunctionSyntaxKind kind,
                                              GeneratorKind generatorKind,
                                              Directives inheritedDirectives,
                                              Directives* newDirectives)
{
    ParseContext<FullParseHandler>* outerpc = pc;

    
    FunctionBox* funbox = newFunctionBox(pn, fun, pc, inheritedDirectives, generatorKind);
    if (!funbox)
        return false;

    
    do {
        
        
        
        
        if (pn->isLikelyIIFE() && !funbox->isGenerator())
            break;

        Parser<SyntaxParseHandler>* parser = handler.syntaxParser;
        if (!parser)
            break;

        {
            
            TokenStream::Position position(keepAtoms);
            tokenStream.tell(&position);
            if (!parser->tokenStream.seek(position, tokenStream))
                return false;

            ParseContext<SyntaxParseHandler> funpc(parser, outerpc, SyntaxParseHandler::null(), funbox,
                                                   newDirectives, outerpc->staticLevel + 1,
                                                   outerpc->blockidGen,  0);
            if (!funpc.init(tokenStream))
                return false;

            if (!parser->functionArgsAndBodyGeneric(SyntaxParseHandler::NodeGeneric,
                                                    fun, type, kind))
            {
                if (parser->hadAbortedSyntaxParse()) {
                    
                    parser->clearAbortedSyntaxParse();
                    MOZ_ASSERT_IF(parser->context->isJSContext(),
                                  !parser->context->asJSContext()->isExceptionPending());
                    break;
                }
                return false;
            }

            outerpc->blockidGen = funpc.blockidGen;

            
            parser->tokenStream.tell(&position);
            if (!tokenStream.seek(position, parser->tokenStream))
                return false;

            
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
        }

        if (!addFreeVariablesFromLazyFunction(fun, pc))
            return false;

        pn->pn_blockid = outerpc->blockid();
        PropagateTransitiveParseFlags(funbox, outerpc->sc);
        return true;
    } while (false);

    
    ParseContext<FullParseHandler> funpc(this, pc, pn, funbox, newDirectives,
                                         outerpc->staticLevel + 1, outerpc->blockidGen,
                                          0);
    if (!funpc.init(tokenStream))
        return false;

    if (!functionArgsAndBodyGeneric(pn, fun, type, kind))
        return false;

    if (!leaveFunction(pn, outerpc, kind))
        return false;

    pn->pn_blockid = outerpc->blockid();

    





    PropagateTransitiveParseFlags(funbox, outerpc->sc);
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::functionArgsAndBody(Node pn, HandleFunction fun,
                                                FunctionType type, FunctionSyntaxKind kind,
                                                GeneratorKind generatorKind,
                                                Directives inheritedDirectives,
                                                Directives* newDirectives)
{
    ParseContext<SyntaxParseHandler>* outerpc = pc;

    
    FunctionBox* funbox = newFunctionBox(pn, fun, pc, inheritedDirectives, generatorKind);
    if (!funbox)
        return false;

    
    ParseContext<SyntaxParseHandler> funpc(this, pc, handler.null(), funbox, newDirectives,
                                           outerpc->staticLevel + 1, outerpc->blockidGen,
                                            0);
    if (!funpc.init(tokenStream))
        return false;

    if (!functionArgsAndBodyGeneric(pn, fun, type, kind))
        return false;

    if (!leaveFunction(pn, outerpc, kind))
        return false;

    
    
    
    MOZ_ASSERT(fun->lazyScript());
    return outerpc->innerFunctions.append(fun);
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::appendToCallSiteObj(Node callSiteObj)
{
    Node cookedNode = noSubstitutionTemplate();
    if (!cookedNode)
        return false;

    JSAtom* atom = tokenStream.getRawTemplateStringAtom();
    if (!atom)
        return false;
    Node rawNode = handler.newTemplateStringLiteral(atom, pos());
    if (!rawNode)
        return false;

    return handler.addToCallSiteObject(callSiteObj, rawNode, cookedNode);
}

template <>
ParseNode*
Parser<FullParseHandler>::standaloneLazyFunction(HandleFunction fun, unsigned staticLevel,
                                                 bool strict, GeneratorKind generatorKind)
{
    MOZ_ASSERT(checkOptionsCalled);

    Node pn = handler.newFunctionDefinition();
    if (!pn)
        return null();

    
    
    if (!tokenStream.peekTokenPos(&pn->pn_pos))
        return null();

    Directives directives( strict);
    FunctionBox* funbox = newFunctionBox(pn, fun,  nullptr, directives,
                                         generatorKind);
    if (!funbox)
        return null();
    funbox->length = fun->nargs() - fun->hasRest();

    Directives newDirectives = directives;
    ParseContext<FullParseHandler> funpc(this,  nullptr, pn, funbox,
                                         &newDirectives, staticLevel,  0,
                                          0);
    if (!funpc.init(tokenStream))
        return null();

    FunctionSyntaxKind syntaxKind = fun->isMethod() ? Method : Statement;
    if (!functionArgsAndBodyGeneric(pn, fun, Normal, syntaxKind)) {
        MOZ_ASSERT(directives == newDirectives);
        return null();
    }

    if (fun->isNamedLambda()) {
        if (AtomDefnPtr p = pc->lexdeps->lookup(fun->name())) {
            Definition* dn = p.value().get<FullParseHandler>();
            if (!ConvertDefinitionToNamedLambdaUse(tokenStream, pc, funbox, dn))
                return nullptr;
        }
    }

    InternalHandle<Bindings*> bindings =
        InternalHandle<Bindings*>::fromMarkedLocation(&funbox->bindings);
    if (!pc->generateFunctionBindings(context, tokenStream, alloc, bindings))
        return null();

    if (!FoldConstants(context, &pn, this))
        return null();

    return pn;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::functionArgsAndBodyGeneric(Node pn, HandleFunction fun, FunctionType type,
                                                 FunctionSyntaxKind kind)
{
    
    
    

    Node prelude = null();
    bool hasRest;
    if (!functionArguments(kind, type, &prelude, pn, &hasRest))
        return false;

    FunctionBox* funbox = pc->sc->asFunctionBox();

    fun->setArgCount(pc->numArgs());
    if (hasRest)
        fun->setHasRest();

    if (kind == Arrow) {
        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_ARROW))
            return false;
        if (!matched) {
            report(ParseError, false, null(), JSMSG_BAD_ARROW_ARGS);
            return false;
        }
    }

    
    FunctionBodyType bodyType = StatementListBody;
    TokenKind tt;
    if (!tokenStream.getToken(&tt, TokenStream::Operand))
        return false;
    if (tt != TOK_LC) {
        if (funbox->isStarGenerator()) {
            report(ParseError, false, null(), JSMSG_CURLY_BEFORE_BODY);
            return false;
        }

        if (kind != Arrow) {
#if JS_HAS_EXPR_CLOSURES
            addTelemetry(JSCompartment::DeprecatedExpressionClosure);
#else
            report(ParseError, false, null(), JSMSG_CURLY_BEFORE_BODY);
            return false;
#endif
        }

        tokenStream.ungetToken();
        bodyType = ExpressionBody;
#if JS_HAS_EXPR_CLOSURES
        fun->setIsExprBody();
#endif
    }

    Node body = functionBody(kind, bodyType);
    if (!body)
        return false;

    if (kind != Method && fun->name() && !checkStrictBinding(fun->name(), pn))
        return false;

    if (bodyType == StatementListBody) {
        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_RC))
            return false;
        if (!matched) {
            report(ParseError, false, null(), JSMSG_CURLY_AFTER_BODY);
            return false;
        }
        funbox->bufEnd = pos().begin + 1;
    } else {
#if !JS_HAS_EXPR_CLOSURES
        MOZ_ASSERT(kind == Arrow);
#endif
        if (tokenStream.hadError())
            return false;
        funbox->bufEnd = pos().end;
        if (kind == Statement && !MatchOrInsertSemicolon(tokenStream))
            return false;
    }

    return finishFunctionDefinition(pn, funbox, prelude, body);
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::checkYieldNameValidity()
{
    
    
    if (pc->isStarGenerator() || versionNumber() >= JSVERSION_1_7 || pc->sc->strict()) {
        report(ParseError, false, null(), JSMSG_RESERVED_ID, "yield");
        return false;
    }
    return true;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionStmt()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FUNCTION));

    RootedPropertyName name(context);
    GeneratorKind generatorKind = NotGenerator;
    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();

    if (tt == TOK_MUL) {
        generatorKind = StarGenerator;
        if (!tokenStream.getToken(&tt))
            return null();
    }

    if (tt == TOK_NAME) {
        name = tokenStream.currentName();
    } else if (tt == TOK_YIELD) {
        if (!checkYieldNameValidity())
            return null();
        name = tokenStream.currentName();
    } else {
        
        report(ParseError, false, null(), JSMSG_UNNAMED_FUNCTION_STMT);
        return null();
    }

    
    if (!pc->atBodyLevel() && pc->sc->needStrictChecks() &&
        !report(ParseStrictError, pc->sc->strict(), null(), JSMSG_STRICT_FUNCTION_STATEMENT))
        return null();

    return functionDef(name, Normal, Statement, generatorKind);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::functionExpr(InvokedPrediction invoked)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FUNCTION));

    GeneratorKind generatorKind = NotGenerator;
    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();

    if (tt == TOK_MUL) {
        generatorKind = StarGenerator;
        if (!tokenStream.getToken(&tt))
            return null();
    }

    RootedPropertyName name(context);
    if (tt == TOK_NAME) {
        name = tokenStream.currentName();
    } else if (tt == TOK_YIELD) {
        if (!checkYieldNameValidity())
            return null();
        name = tokenStream.currentName();
    } else {
        tokenStream.ungetToken();
    }

    return functionDef(name, Normal, Expression, generatorKind, invoked);
}








static inline bool
IsEscapeFreeStringLiteral(const TokenPos& pos, JSAtom* str)
{
    




    return pos.begin + str->length() + 2 == pos.end;
}

template <>
bool
Parser<SyntaxParseHandler>::asmJS(Node list)
{
    
    
    
    
    
    
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return false;
}

template <>
bool
Parser<FullParseHandler>::asmJS(Node list)
{
    
    handler.disableSyntaxParser();

    
    
    
    
    if (!pc->newDirectives || pc->newDirectives->asmJS())
        return true;

    
    
    if (ss == nullptr)
        return true;

    pc->sc->asFunctionBox()->useAsm = true;

    
    
    
    
    
    bool validated;
    if (!ValidateAsmJS(context, *this, list, &validated))
        return false;
    if (!validated) {
        pc->newDirectives->setAsmJS();
        return false;
    }

    return true;
}




















template <typename ParseHandler>
bool
Parser<ParseHandler>::maybeParseDirective(Node list, Node pn, bool* cont)
{
    TokenPos directivePos;
    JSAtom* directive = handler.isStringExprStatement(pn, &directivePos);

    *cont = !!directive;
    if (!*cont)
        return true;

    if (IsEscapeFreeStringLiteral(directivePos, directive)) {
        
        
        
        
        
        
        
        
        
        
        handler.setPrologue(pn);

        if (directive == context->names().useStrict) {
            
            
            pc->sc->setExplicitUseStrict();
            if (!pc->sc->strict()) {
                if (pc->sc->isFunctionBox()) {
                    
                    pc->newDirectives->setStrict();
                    return false;
                } else {
                    
                    
                    
                    if (tokenStream.sawOctalEscape()) {
                        report(ParseError, false, null(), JSMSG_DEPRECATED_OCTAL);
                        return false;
                    }
                    pc->sc->strictScript = true;
                }
            }
        } else if (directive == context->names().useAsm) {
            if (pc->sc->isFunctionBox())
                return asmJS(list);
            return report(ParseWarning, false, pn, JSMSG_USE_ASM_DIRECTIVE_FAIL);
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
    bool afterReturn = false;
    bool warnedAboutStatementsAfterReturn = false;
    uint32_t statementBegin;
    for (;;) {
        TokenKind tt;
        if (!tokenStream.peekToken(&tt, TokenStream::Operand)) {
            if (tokenStream.isEOF())
                isUnexpectedEOF_ = true;
            return null();
        }
        if (tt == TOK_EOF || tt == TOK_RC)
            break;
        if (afterReturn) {
            TokenPos pos(0, 0);
            if (!tokenStream.peekTokenPos(&pos, TokenStream::Operand))
                return null();
            statementBegin = pos.begin;
        }
        Node next = statement(canHaveDirectives);
        if (!next) {
            if (tokenStream.isEOF())
                isUnexpectedEOF_ = true;
            return null();
        }
        if (!warnedAboutStatementsAfterReturn) {
            if (afterReturn) {
                if (!handler.isStatementPermittedAfterReturnStatement(next)) {
                    if (!reportWithOffset(ParseWarning, false, statementBegin,
                                          JSMSG_STMT_AFTER_RETURN))
                    {
                        return null();
                    }
                    warnedAboutStatementsAfterReturn = true;
                }
            } else if (handler.isReturnStatement(next)) {
                afterReturn = true;
            }
        }

        if (canHaveDirectives) {
            if (!maybeParseDirective(pn, next, &canHaveDirectives))
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
    Node pn = exprInParens();
    if (!pn)
        return null();
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    if (handler.isUnparenthesizedAssignment(pn)) {
        if (!report(ParseExtraWarning, false, null(), JSMSG_EQUAL_AS_ASSIGN))
            return null();
    }
    return pn;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::matchLabel(MutableHandle<PropertyName*> label)
{
    TokenKind tt;
    if (!tokenStream.peekTokenSameLine(&tt, TokenStream::Operand))
        return false;
    if (tt == TOK_NAME) {
        tokenStream.consumeKnownToken(TOK_NAME);
        label.set(tokenStream.currentName());
    } else if (tt == TOK_YIELD) {
        tokenStream.consumeKnownToken(TOK_YIELD);
        if (!checkYieldNameValidity())
            return false;
        label.set(tokenStream.currentName());
    } else {
        label.set(nullptr);
    }
    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::reportRedeclaration(Node pn, Definition::Kind redeclKind, HandlePropertyName name)
{
    JSAutoByteString printable;
    if (!AtomToPrintableString(context, name, &printable))
        return false;

    StmtInfoPC* stmt = LexicalLookup(pc, name, nullptr, (StmtInfoPC*)nullptr);
    if (stmt && stmt->type == STMT_CATCH) {
        report(ParseError, false, pn, JSMSG_REDECLARED_CATCH_IDENTIFIER, printable.ptr());
    } else {
        if (redeclKind == Definition::ARG) {
            report(ParseError, false, pn, JSMSG_REDECLARED_PARAM, printable.ptr());
        } else {
            report(ParseError, false, pn, JSMSG_REDECLARED_VAR, Definition::kindString(redeclKind),
                   printable.ptr());
        }
    }
    return false;
}










template <>
 bool
Parser<FullParseHandler>::bindLexical(BindData<FullParseHandler>* data,
                                      HandlePropertyName name, Parser<FullParseHandler>* parser)
{
    ParseContext<FullParseHandler>* pc = parser->pc;
    ParseNode* pn = data->pn;
    if (!parser->checkStrictBinding(name, pn))
        return false;

    ExclusiveContext* cx = parser->context;
    Rooted<StaticBlockObject*> blockObj(cx, data->let.blockObj);

    unsigned index;
    if (blockObj) {
        index = blockObj->numVariables();
        if (index >= StaticBlockObject::LOCAL_INDEX_LIMIT) {
            parser->report(ParseError, false, pn, data->let.overflow);
            return false;
        }
    } else {
        
        
        
        index = 0;
    }

    
    
    
    
    
    
    
    
    
    
    if (!pn->pn_cookie.set(parser->tokenStream, pc->staticLevel, index))
        return false;

    Definition* dn = pc->decls().lookupFirst(name);
    Definition::Kind bindingKind = data->isConst ? Definition::CONST : Definition::LET;

    



    if (data->let.varContext == HoistVars) {
        if (dn && dn->pn_blockid == pc->blockid())
            return parser->reportRedeclaration(pn, dn->kind(), name);
        if (!pc->define(parser->tokenStream, name, pn, bindingKind))
            return false;
    }

    if (blockObj) {
        bool redeclared;
        RootedId id(cx, NameToId(name));
        RootedShape shape(cx, StaticBlockObject::addVar(cx, blockObj, id,
                                                        data->isConst, index, &redeclared));
        if (!shape) {
            if (redeclared) {
                
                
                
                
                Definition::Kind dnKind = dn ? dn->kind() : bindingKind;
                parser->reportRedeclaration(pn, dnKind, name);
            }
            return false;
        }

        
        blockObj->setDefinitionParseNode(index, reinterpret_cast<Definition*>(pn));
    } else {
        
        
        MOZ_ASSERT(data->let.varContext == HoistVars);
        MOZ_ASSERT(pc->decls().lookupFirst(name));
    }

    return true;
}

template <>
 bool
Parser<SyntaxParseHandler>::bindLexical(BindData<SyntaxParseHandler>* data,
                                        HandlePropertyName name, Parser<SyntaxParseHandler>* parser)
{
    if (!parser->checkStrictBinding(name, data->pn))
        return false;

    return true;
}

template <typename ParseHandler, class Op>
static inline bool
ForEachLetDef(TokenStream& ts, ParseContext<ParseHandler>* pc,
              HandleStaticBlockObject blockObj, Op op)
{
    for (Shape::Range<CanGC> r(ts.context(), blockObj->lastProperty()); !r.empty(); r.popFront()) {
        Shape& shape = r.front();

        
        if (JSID_IS_INT(shape.propid()))
            continue;

        if (!op(ts, pc, blockObj, shape, JSID_TO_ATOM(shape.propid())))
            return false;
    }
    return true;
}

template <typename ParseHandler>
struct PopLetDecl {
    bool operator()(TokenStream&, ParseContext<ParseHandler>* pc, HandleStaticBlockObject,
                    const Shape&, JSAtom* atom)
    {
        pc->popLetDecl(atom);
        return true;
    }
};









template <typename ParseHandler>
static void
AccumulateBlockScopeDepth(ParseContext<ParseHandler>* pc)
{
    uint32_t innerDepth = pc->topStmt->innerBlockScopeDepth;
    StmtInfoPC* outer = pc->topStmt->down;

    if (pc->topStmt->isBlockScope)
        innerDepth += pc->topStmt->staticScope->template as<StaticBlockObject>().numVariables();

    if (outer) {
        if (outer->innerBlockScopeDepth < innerDepth)
            outer->innerBlockScopeDepth = innerDepth;
    } else {
        if (pc->blockScopeDepth < innerDepth)
            pc->blockScopeDepth = innerDepth;
    }
}

template <typename ParseHandler>
static void
PopStatementPC(TokenStream& ts, ParseContext<ParseHandler>* pc)
{
    RootedNestedScopeObject scopeObj(ts.context(), pc->topStmt->staticScope);
    MOZ_ASSERT(!!scopeObj == pc->topStmt->isNestedScope);

    AccumulateBlockScopeDepth(pc);
    FinishPopStatement(pc);

    if (scopeObj) {
        if (scopeObj->is<StaticBlockObject>()) {
            RootedStaticBlockObject blockObj(ts.context(), &scopeObj->as<StaticBlockObject>());
            MOZ_ASSERT(!blockObj->inDictionaryMode());
            ForEachLetDef(ts, pc, blockObj, PopLetDecl<ParseHandler>());
        }
        scopeObj->resetEnclosingNestedScopeFromParser();
    }
}









template <class ContextT>
typename ContextT::StmtInfo*
LexicalLookup(ContextT* ct, HandleAtom atom, int* slotp, typename ContextT::StmtInfo* stmt)
{
    RootedId id(ct->sc->context, AtomToId(atom));

    if (!stmt)
        stmt = ct->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        




        if (stmt->type == STMT_WITH && !ct->sc->isDotVariable(atom))
            break;

        
        if (!stmt->isBlockScope)
            continue;

        StaticBlockObject& blockObj = stmt->staticBlock();
        Shape* shape = blockObj.lookup(ct->sc->context, id);
        if (shape) {
            if (slotp)
                *slotp = blockObj.shapeToIndex(*shape);
            return stmt;
        }
    }

    if (slotp)
        *slotp = -1;
    return stmt;
}

template <typename ParseHandler>
static inline bool
OuterLet(ParseContext<ParseHandler>* pc, StmtInfoPC* stmt, HandleAtom atom)
{
    while (stmt->downScope) {
        stmt = LexicalLookup(pc, atom, nullptr, stmt->downScope);
        if (!stmt)
            return false;
        if (stmt->type == STMT_BLOCK)
            return true;
    }
    return false;
}

template <typename ParseHandler>
 bool
Parser<ParseHandler>::bindVarOrGlobalConst(BindData<ParseHandler>* data,
                                           HandlePropertyName name, Parser<ParseHandler>* parser)
{
    ExclusiveContext* cx = parser->context;
    ParseContext<ParseHandler>* pc = parser->pc;
    Node pn = data->pn;
    bool isConstDecl = data->op == JSOP_DEFCONST;

    
    parser->handler.setOp(pn, JSOP_GETNAME);

    if (!parser->checkStrictBinding(name, pn))
        return false;

    StmtInfoPC* stmt = LexicalLookup(pc, name, nullptr, (StmtInfoPC*)nullptr);

    if (stmt && stmt->type == STMT_WITH) {
        parser->handler.setFlag(pn, PND_DEOPTIMIZED);
        if (pc->sc->isFunctionBox()) {
            FunctionBox* funbox = pc->sc->asFunctionBox();
            funbox->setMightAliasLocals();
        }

        






        if (name == cx->names().arguments)
            pc->sc->setHasDebuggerStatement();

        return true;
    }

    DefinitionList::Range defs = pc->decls().lookupMulti(name);
    MOZ_ASSERT_IF(stmt, !defs.empty());

    if (defs.empty()) {
        return pc->define(parser->tokenStream, name, pn,
                          isConstDecl ? Definition::GLOBALCONST : Definition::VAR);
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
        bool inCatchBody = (stmt && stmt->type == STMT_CATCH);
        bool error = (isConstDecl ||
                      dn_kind == Definition::CONST ||
                      dn_kind == Definition::GLOBALCONST ||
                      (dn_kind == Definition::LET &&
                       (!inCatchBody || OuterLet(pc, stmt, name))));

        if (parser->options().extraWarningsOption
            ? data->op != JSOP_DEFVAR || dn_kind != Definition::VAR
            : error)
        {
            JSAutoByteString bytes;
            if (!AtomToPrintableString(cx, name, &bytes))
                return false;

            ParseReportKind reporter = error ? ParseError : ParseExtraWarning;
            if (!(inCatchBody
                  ? parser->report(reporter, false, pn,
                                   JSMSG_REDECLARED_CATCH_IDENTIFIER, bytes.ptr())
                  : parser->report(reporter, false, pn, JSMSG_REDECLARED_VAR,
                                   Definition::kindString(dn_kind), bytes.ptr())))
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
Parser<FullParseHandler>::makeSetCall(ParseNode* pn, unsigned msg)
{
    MOZ_ASSERT(pn->isKind(PNK_CALL));
    MOZ_ASSERT(pn->isArity(PN_LIST));
    MOZ_ASSERT(pn->isOp(JSOP_CALL) || pn->isOp(JSOP_SPREADCALL) ||
               pn->isOp(JSOP_EVAL) || pn->isOp(JSOP_STRICTEVAL) ||
               pn->isOp(JSOP_SPREADEVAL) || pn->isOp(JSOP_STRICTSPREADEVAL) ||
               pn->isOp(JSOP_FUNCALL) || pn->isOp(JSOP_FUNAPPLY));

    if (!report(ParseStrictError, pc->sc->strict(), pn, msg))
        return false;
    handler.markAsSetCall(pn);
    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::noteNameUse(HandlePropertyName name, Node pn)
{
    





    if (pc->useAsmOrInsideUseAsm())
        return true;

    StmtInfoPC* stmt = LexicalLookup(pc, name, nullptr, (StmtInfoPC*)nullptr);

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

    if (stmt) {
        if (stmt->type == STMT_WITH) {
            handler.setFlag(pn, PND_DEOPTIMIZED);
        } else if (stmt->type == STMT_SWITCH && stmt->isBlockScope) {
            
            
            MOZ_ASSERT(stmt->firstDominatingLexicalInCase <= stmt->staticBlock().numVariables());
            handler.markMaybeUninitializedLexicalUseInSwitch(pn, dn,
                                                             stmt->firstDominatingLexicalInCase);
        }
    }

    return true;
}

template <>
bool
Parser<FullParseHandler>::bindInitialized(BindData<FullParseHandler>* data, ParseNode* pn)
{
    MOZ_ASSERT(pn->isKind(PNK_NAME));

    RootedPropertyName name(context, pn->pn_atom->asPropertyName());

    data->pn = pn;
    if (!data->binder(data, name, this))
        return false;

    



    if (data->op == JSOP_INITLEXICAL)
        pn->setOp(JSOP_INITLEXICAL);
    else if (pn->pn_dflags & PND_BOUND)
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
Parser<FullParseHandler>::checkDestructuring(BindData<FullParseHandler>* data, ParseNode* left);

template <>
bool
Parser<FullParseHandler>::checkDestructuringObject(BindData<FullParseHandler>* data,
                                                   ParseNode* objectPattern)
{
    MOZ_ASSERT(objectPattern->isKind(PNK_OBJECT));

    for (ParseNode* member = objectPattern->pn_head; member; member = member->pn_next) {
        ParseNode* expr;
        if (member->isKind(PNK_MUTATEPROTO)) {
            expr = member->pn_kid;
        } else {
            MOZ_ASSERT(member->isKind(PNK_COLON) || member->isKind(PNK_SHORTHAND));
            expr = member->pn_right;
        }
        if (expr->isKind(PNK_ASSIGN))
            expr = expr->pn_left;

        bool ok;
        if (expr->isKind(PNK_ARRAY) || expr->isKind(PNK_OBJECT)) {
            ok = checkDestructuring(data, expr);
        } else if (data) {
            if (!expr->isKind(PNK_NAME)) {
                report(ParseError, false, expr, JSMSG_NO_VARIABLE_NAME);
                return false;
            }
            ok = bindInitialized(data, expr);
        } else {
            ok = checkAndMarkAsAssignmentLhs(expr, KeyedDestructuringAssignment);
        }
        if (!ok)
            return false;
    }

    return true;
}

template <>
bool
Parser<FullParseHandler>::checkDestructuringArray(BindData<FullParseHandler>* data,
                                                  ParseNode* arrayPattern)
{
    MOZ_ASSERT(arrayPattern->isKind(PNK_ARRAY));

    for (ParseNode* element = arrayPattern->pn_head; element; element = element->pn_next) {
        if (element->isKind(PNK_ELISION))
            continue;

        ParseNode* target = element;
        if (target->isKind(PNK_SPREAD)) {
            if (target->pn_next) {
                report(ParseError, false, target->pn_next, JSMSG_PARAMETER_AFTER_REST);
                return false;
            }
            target = target->pn_kid;

            
            if (target->isKind(PNK_ARRAY) || target->isKind(PNK_OBJECT)) {
                report(ParseError, false, target, JSMSG_BAD_DESTRUCT_TARGET);
                return false;
            }
        } else if (target->isKind(PNK_ASSIGN)) {
            target = target->pn_left;
        }

        bool ok;
        if (target->isKind(PNK_ARRAY) || target->isKind(PNK_OBJECT)) {
            ok = checkDestructuring(data, target);
        } else {
            if (data) {
                if (!target->isKind(PNK_NAME)) {
                    report(ParseError, false, target, JSMSG_NO_VARIABLE_NAME);
                    return false;
                }
                ok = bindInitialized(data, target);
            } else {
                ok = checkAndMarkAsAssignmentLhs(target, KeyedDestructuringAssignment);
            }
        }
        if (!ok)
            return false;
    }

    return true;
}





































template <>
bool
Parser<FullParseHandler>::checkDestructuring(BindData<FullParseHandler>* data, ParseNode* left)
{
    if (left->isKind(PNK_ARRAYCOMP)) {
        report(ParseError, false, left, JSMSG_ARRAY_COMP_LEFTSIDE);
        return false;
    }

    if (left->isKind(PNK_ARRAY))
        return checkDestructuringArray(data, left);
    return checkDestructuringObject(data, left);
}

template <>
bool
Parser<SyntaxParseHandler>::checkDestructuring(BindData<SyntaxParseHandler>* data, Node left)
{
    return abortIfSyntaxParser();
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::destructuringExpr(BindData<ParseHandler>* data, TokenKind tt)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(tt));

    pc->inDeclDestructuring = true;
    Node pn = primaryExpr(tt);
    pc->inDeclDestructuring = false;
    if (!pn)
        return null();
    if (!checkDestructuring(data, pn))
        return null();
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::destructuringExprWithoutYield(BindData<ParseHandler>* data, TokenKind tt,
                                                    unsigned msg)
{
    uint32_t startYieldOffset = pc->lastYieldOffset;
    Node res = destructuringExpr(data, tt);
    if (res && pc->lastYieldOffset != startYieldOffset) {
        reportWithOffset(ParseError, false, pc->lastYieldOffset,
                         msg, js_yield_str);
        return null();
    }
    return res;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::pushLexicalScope(HandleStaticBlockObject blockObj, StmtInfoPC* stmt)
{
    MOZ_ASSERT(blockObj);

    ObjectBox* blockbox = newObjectBox(blockObj);
    if (!blockbox)
        return null();

    PushStatementPC(pc, stmt, STMT_BLOCK);
    blockObj->initEnclosingNestedScopeFromParser(pc->staticScope);
    FinishPushNestedScope(pc, stmt, *blockObj.get());
    stmt->isBlockScope = true;

    Node pn = handler.newLexicalScope(blockbox);
    if (!pn)
        return null();

    if (!GenerateBlockId(tokenStream, pc, stmt->blockid))
        return null();
    handler.setBlockId(pn, stmt->blockid);
    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::pushLexicalScope(StmtInfoPC* stmt)
{
    RootedStaticBlockObject blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return null();

    return pushLexicalScope(blockObj, stmt);
}

struct AddLetDecl
{
    uint32_t blockid;

    explicit AddLetDecl(uint32_t blockid) : blockid(blockid) {}

    bool operator()(TokenStream& ts, ParseContext<FullParseHandler>* pc,
                    HandleStaticBlockObject blockObj, const Shape& shape, JSAtom*)
    {
        ParseNode* def = (ParseNode*) blockObj->getSlot(shape.slot()).toPrivate();
        def->pn_blockid = blockid;
        RootedPropertyName name(ts.context(), def->name());
        return pc->define(ts, name, def, Definition::LET);
    }
};

template <>
ParseNode*
Parser<FullParseHandler>::pushLetScope(HandleStaticBlockObject blockObj, StmtInfoPC* stmt)
{
    MOZ_ASSERT(blockObj);
    ParseNode* pn = pushLexicalScope(blockObj, stmt);
    if (!pn)
        return null();

    pn->pn_dflags |= PND_LEXICAL;

    
    if (!ForEachLetDef(tokenStream, pc, blockObj, AddLetDecl(stmt->blockid)))
        return null();

    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::pushLetScope(HandleStaticBlockObject blockObj, StmtInfoPC* stmt)
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}






template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::deprecatedLetBlockOrExpression(LetContext letContext)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_LET));

    RootedStaticBlockObject blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return null();

    uint32_t begin = pos().begin;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    Node vars = variables(PNK_LET, nullptr, blockObj, DontHoistVars);
    if (!vars)
        return null();

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    StmtInfoPC stmtInfo(context);
    Node block = pushLetScope(blockObj, &stmtInfo);
    if (!block)
        return null();

    bool needExprStmt = false;
    if (letContext == LetStatement) {
        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_LC, TokenStream::Operand))
            return null();
        if (!matched) {
            















            if (!reportWithOffset(ParseStrictError, pc->sc->strict(), begin,
                                  JSMSG_STRICT_CODE_LET_EXPR_STMT))
            {
                return null();
            }

            




            needExprStmt = true;
            letContext = LetExpression;
        }
    }

    Node expr;
    if (letContext == LetStatement) {
        expr = statements();
        if (!expr)
            return null();
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_LET);

        addTelemetry(JSCompartment::DeprecatedLetBlock);
        if (!report(ParseWarning, pc->sc->strict(), expr, JSMSG_DEPRECATED_LET_BLOCK))
            return null();
    } else {
        MOZ_ASSERT(letContext == LetExpression);
        expr = assignExpr();
        if (!expr)
            return null();

        addTelemetry(JSCompartment::DeprecatedLetExpression);
        if (!report(ParseWarning, pc->sc->strict(), expr, JSMSG_DEPRECATED_LET_EXPRESSION))
            return null();
    }
    handler.setLexicalScopeBody(block, expr);
    PopStatementPC(tokenStream, pc);

    TokenPos letPos(begin, pos().end);

    if (letContext == LetExpression) {
        if (needExprStmt) {
            if (!MatchOrInsertSemicolon(tokenStream))
                return null();
        }

        Node letExpr = handler.newLetExpression(vars, block, letPos);
        if (!letExpr)
            return null();

        return needExprStmt ? handler.newExprStatement(letExpr, pos().end) : letExpr;
    }

    return handler.newLetBlock(vars, block, letPos);
}

template <typename ParseHandler>
static bool
PushBlocklikeStatement(TokenStream& ts, StmtInfoPC* stmt, StmtType type,
                       ParseContext<ParseHandler>* pc)
{
    PushStatementPC(pc, stmt, type);
    return GenerateBlockId(ts, pc, stmt->blockid);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::blockStatement()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_LC));

    StmtInfoPC stmtInfo(context);
    if (!PushBlocklikeStatement(tokenStream, &stmtInfo, STMT_BLOCK, pc))
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
Parser<ParseHandler>::newBindingNode(PropertyName* name, bool functionScope, VarContext varContext)
{
    






    if (varContext == HoistVars) {
        if (AtomDefnPtr p = pc->lexdeps->lookup(name)) {
            DefinitionNode lexdep = p.value().get<ParseHandler>();
            MOZ_ASSERT(handler.getDefinitionKind(lexdep) == Definition::PLACEHOLDER);

            Node pn = handler.getDefinitionNode(lexdep);
            if (handler.dependencyCovered(pn, pc->blockid(), functionScope)) {
                handler.setBlockId(pn, pc->blockid());
                pc->lexdeps->remove(p);
                handler.setPosition(pn, pos());
                return pn;
            }
        }
    }

    
    return newName(name);
}






template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::variables(ParseNodeKind kind, bool* psimple,
                                StaticBlockObject* blockObj, VarContext varContext)
{
    






    MOZ_ASSERT(kind == PNK_VAR || kind == PNK_CONST || kind == PNK_LET || kind == PNK_GLOBALCONST);

    



    MOZ_ASSERT_IF(psimple, *psimple);

    JSOp op = JSOP_NOP;
    if (kind == PNK_VAR)
        op = JSOP_DEFVAR;
    else if (kind == PNK_GLOBALCONST)
        op = JSOP_DEFCONST;

    Node pn = handler.newDeclarationList(kind, op);
    if (!pn)
        return null();

    




    BindData<ParseHandler> data(context);
    if (kind == PNK_VAR || kind == PNK_GLOBALCONST) {
        data.initVarOrGlobalConst(op);
    } else {
        data.initLexical(varContext, blockObj, JSMSG_TOO_MANY_LOCALS,
                          kind == PNK_CONST);
    }

    bool first = true;
    Node pn2;
    while (true) {
        do {
            if (psimple && !first)
                *psimple = false;
            first = false;

            TokenKind tt;
            if (!tokenStream.getToken(&tt))
                return null();
            if (tt == TOK_LB || tt == TOK_LC) {
                if (psimple)
                    *psimple = false;

                pc->inDeclDestructuring = true;
                pn2 = primaryExpr(tt);
                pc->inDeclDestructuring = false;
                if (!pn2)
                    return null();

                bool parsingForInOrOfInit = false;
                if (pc->parsingForInit) {
                    bool isForIn, isForOf;
                    if (!matchInOrOf(&isForIn, &isForOf))
                        return null();
                    parsingForInOrOfInit = isForIn || isForOf;
                }

                
                
                bool bindBeforeInitializer = (kind != PNK_LET && kind != PNK_CONST) ||
                                             parsingForInOrOfInit;
                if (bindBeforeInitializer && !checkDestructuring(&data, pn2))
                    return null();

                if (parsingForInOrOfInit) {
                    tokenStream.ungetToken();
                    handler.addList(pn, pn2);
                    break;
                }

                MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);

                Node init = assignExpr();
                if (!init)
                    return null();

                if (!bindBeforeInitializer && !checkDestructuring(&data, pn2))
                    return null();

                pn2 = handler.newBinary(PNK_ASSIGN, pn2, init);
                if (!pn2)
                    return null();
                handler.addList(pn, pn2);
                break;
            }

            if (tt != TOK_NAME) {
                if (tt == TOK_YIELD) {
                    if (!checkYieldNameValidity())
                        return null();
                } else {
                    report(ParseError, false, null(), JSMSG_NO_VARIABLE_NAME);
                    return null();
                }
            }

            RootedPropertyName name(context, tokenStream.currentName());
            pn2 = newBindingNode(name, kind == PNK_VAR || kind == PNK_GLOBALCONST, varContext);
            if (!pn2)
                return null();
            if (data.isConst)
                handler.setFlag(pn2, PND_CONST);
            data.pn = pn2;

            handler.addList(pn, pn2);

            bool matched;
            if (!tokenStream.matchToken(&matched, TOK_ASSIGN))
                return null();
            if (matched) {
                if (psimple)
                    *psimple = false;

                
                
                
                
                
                
                
                
                bool bindBeforeInitializer = kind != PNK_LET && kind != PNK_CONST;
                if (bindBeforeInitializer && !data.binder(&data, name, this))
                    return null();

                Node init = assignExpr();
                if (!init)
                    return null();

                if (!bindBeforeInitializer && !data.binder(&data, name, this))
                    return null();

                if (!handler.finishInitializerAssignment(pn2, init, data.op))
                    return null();
            } else {
                if (data.isConst && !pc->parsingForInit) {
                    report(ParseError, false, null(), JSMSG_BAD_CONST_DECL);
                    return null();
                }

                if (!data.binder(&data, name, this))
                    return null();
            }

            handler.setEndPosition(pn, pn2);
        } while (false);

        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_COMMA))
            return null();
        if (!matched)
            break;
    }

    return pn;
}

template <>
bool
Parser<FullParseHandler>::checkAndPrepareLexical(bool isConst, const TokenPos& errorPos)
{
    










    StmtInfoPC* stmt = pc->topStmt;
    if (stmt && (!stmt->maybeScope() || stmt->isForLetBlock)) {
        reportWithOffset(ParseError, false, errorPos.begin, JSMSG_LEXICAL_DECL_NOT_IN_BLOCK,
                         isConst ? "const" : "lexical");
        return false;
    }

    if (stmt && stmt->isBlockScope) {
        MOZ_ASSERT(pc->staticScope == stmt->staticScope);
    } else {
        if (pc->atBodyLevel()) {
            










            bool isGlobal = !pc->sc->isFunctionBox() && stmt == pc->topScopeStmt;
            if (options().selfHostingMode && isGlobal) {
                report(ParseError, false, null(), JSMSG_SELFHOSTED_TOP_LEVEL_LEXICAL,
                        isConst ? "'const'" : "'let'");
                return false;
            }
            return true;
        }

        




        MOZ_ASSERT(!stmt->isBlockScope);
        MOZ_ASSERT(stmt != pc->topScopeStmt);
        MOZ_ASSERT(stmt->type == STMT_BLOCK ||
                    stmt->type == STMT_SWITCH ||
                    stmt->type == STMT_TRY ||
                    stmt->type == STMT_FINALLY);
        MOZ_ASSERT(!stmt->downScope);

        
        StaticBlockObject* blockObj = StaticBlockObject::create(context);
        if (!blockObj)
            return false;

        ObjectBox* blockbox = newObjectBox(blockObj);
        if (!blockbox)
            return false;

        





        stmt->isBlockScope = stmt->isNestedScope = true;
        stmt->downScope = pc->topScopeStmt;
        pc->topScopeStmt = stmt;

        blockObj->initEnclosingNestedScopeFromParser(pc->staticScope);
        pc->staticScope = blockObj;
        stmt->staticScope = blockObj;

#ifdef DEBUG
        ParseNode* tmp = pc->blockNode;
        MOZ_ASSERT(!tmp || !tmp->isKind(PNK_LEXICALSCOPE));
#endif

        
        ParseNode* pn1 = handler.new_<LexicalScopeNode>(blockbox, pc->blockNode);
        if (!pn1)
            return false;;
        pc->blockNode = pn1;
    }
    return true;
}

static StaticBlockObject*
CurrentLexicalStaticBlock(ParseContext<FullParseHandler>* pc)
{
    return pc->atBodyLevel() ? nullptr :
           &pc->staticScope->as<StaticBlockObject>();
}

template <>
ParseNode*
Parser<FullParseHandler>::makeInitializedLexicalBinding(HandlePropertyName name, bool isConst,
                                                        const TokenPos& pos)
{
    
    BindData<FullParseHandler> data(context);
    if (pc->atGlobalLevel()) {
        data.initVarOrGlobalConst(isConst ? JSOP_DEFCONST : JSOP_DEFVAR);
    } else {
        if (!checkAndPrepareLexical(isConst, pos))
            return null();
        data.initLexical(HoistVars, CurrentLexicalStaticBlock(pc), JSMSG_TOO_MANY_LOCALS, isConst);
    }
    ParseNode* dn = newBindingNode(name, pc->atGlobalLevel());
    if (!dn)
        return null();
    handler.setPosition(dn, pos);

    if (!bindInitialized(&data, dn))
        return null();

    return dn;
}

template <>
ParseNode*
Parser<FullParseHandler>::lexicalDeclaration(bool isConst)
{
    handler.disableSyntaxParser();

    if (!checkAndPrepareLexical(isConst, pos()))
        return null();

    













    ParseNodeKind kind = PNK_LET;
    if (pc->atGlobalLevel())
        kind = isConst ? PNK_GLOBALCONST : PNK_VAR;
    else if (isConst)
        kind = PNK_CONST;

    ParseNode* pn = variables(kind, nullptr,
                              CurrentLexicalStaticBlock(pc),
                              HoistVars);
    if (!pn)
        return null();
    pn->pn_xflags = PNX_POPVAR;
    return MatchOrInsertSemicolon(tokenStream) ? pn : nullptr;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::lexicalDeclaration(bool)
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

template <>
ParseNode*
Parser<FullParseHandler>::letDeclarationOrBlock()
{
    handler.disableSyntaxParser();

    
    TokenKind tt;
    if (!tokenStream.peekToken(&tt))
        return null();
    if (tt == TOK_LP) {
        ParseNode* node = deprecatedLetBlockOrExpression(LetStatement);
        if (!node)
            return nullptr;

        if (node->isKind(PNK_LETBLOCK)) {
            MOZ_ASSERT(node->isArity(PN_BINARY));
        } else {
            MOZ_ASSERT(node->isKind(PNK_SEMI));
            MOZ_ASSERT(node->pn_kid->isKind(PNK_LETEXPR));
            MOZ_ASSERT(node->pn_kid->isArity(PN_BINARY));
        }

        return node;
    }

    ParseNode* decl = lexicalDeclaration( false);
    if (!decl)
        return nullptr;

    
    
    MOZ_ASSERT(decl->isKind(PNK_LET) || decl->isKind(PNK_VAR));
    MOZ_ASSERT(decl->isArity(PN_LIST));
    return decl;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::letDeclarationOrBlock()
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

template<typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::importDeclaration()
{
    MOZ_ASSERT(tokenStream.currentToken().type == TOK_IMPORT);

    if (pc->sc->isFunctionBox() || !pc->atBodyLevel()) {
        report(ParseError, false, null(), JSMSG_IMPORT_DECL_AT_TOP_LEVEL);
        return null();
    }

    uint32_t begin = pos().begin;
    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();

    Node importSpecSet = handler.newList(PNK_IMPORT_SPEC_LIST);
    if (!importSpecSet)
        return null();

    if (tt == TOK_NAME || tt == TOK_LC) {
        if (tt == TOK_NAME) {
            
            
            
            
            Node importName = newName(context->names().default_);
            if (!importName)
                return null();

            Node bindingName = newName(tokenStream.currentName());
            if (!bindingName)
                return null();

            Node importSpec = handler.newBinary(PNK_IMPORT_SPEC, importName, bindingName);
            if (!importSpec)
                return null();

            handler.addList(importSpecSet, importSpec);
        } else {
            while (true) {
                
                
                
                if (!tokenStream.peekToken(&tt, TokenStream::KeywordIsName))
                    return null();
                if (tt == TOK_RC)
                    break;

                
                
                
                MUST_MATCH_TOKEN(TOK_NAME, JSMSG_NO_IMPORT_NAME);
                Node importName = newName(tokenStream.currentName());
                if (!importName)
                    return null();

                if (!tokenStream.getToken(&tt))
                    return null();
                if (tt == TOK_NAME && tokenStream.currentName() == context->names().as) {
                    if (!tokenStream.getToken(&tt))
                        return null();
                    if (tt != TOK_NAME) {
                        report(ParseError, false, null(), JSMSG_NO_BINDING_NAME);
                        return null();
                    }
                } else {
                    
                    
                    
                    if (IsKeyword(importName->name())) {
                        JSAutoByteString bytes;
                        if (!AtomToPrintableString(context, importName->name(), &bytes))
                            return null();
                        report(ParseError, false, null(), JSMSG_AS_AFTER_RESERVED_WORD, bytes.ptr());
                        return null();
                    }
                    tokenStream.ungetToken();
                }
                Node bindingName = newName(tokenStream.currentName());
                if (!bindingName)
                    return null();

                Node importSpec = handler.newBinary(PNK_IMPORT_SPEC, importName, bindingName);
                if (!importSpec)
                    return null();

                handler.addList(importSpecSet, importSpec);

                bool matched;
                if (!tokenStream.matchToken(&matched, TOK_COMMA))
                    return null();
                if (!matched)
                    break;
            }

            MUST_MATCH_TOKEN(TOK_RC, JSMSG_RC_AFTER_IMPORT_SPEC_LIST);
        }

        if (!tokenStream.getToken(&tt))
            return null();
        if (tt != TOK_NAME || tokenStream.currentName() != context->names().from) {
            report(ParseError, false, null(), JSMSG_FROM_AFTER_IMPORT_SPEC_SET);
            return null();
        }

        MUST_MATCH_TOKEN(TOK_STRING, JSMSG_MODULE_SPEC_AFTER_FROM);
    } else {
        if (tt != TOK_STRING) {
            report(ParseError, false, null(), JSMSG_DECLARATION_AFTER_IMPORT);
            return null();
        }

        
        
        importSpecSet->pn_pos.end = importSpecSet->pn_pos.begin;
    }

    Node moduleSpec = stringLiteral();
    if (!moduleSpec)
        return null();

    if (!MatchOrInsertSemicolon(tokenStream))
        return null();

    return handler.newImportDeclaration(importSpecSet, moduleSpec,
                                        TokenPos(begin, pos().end));
}

template<>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::importDeclaration()
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

template<typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::exportDeclaration()
{
    MOZ_ASSERT(tokenStream.currentToken().type == TOK_EXPORT);

    if (pc->sc->isFunctionBox() || !pc->atBodyLevel()) {
        report(ParseError, false, null(), JSMSG_EXPORT_DECL_AT_TOP_LEVEL);
        return null();
    }

    uint32_t begin = pos().begin;

    Node kid;
    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();
    switch (tt) {
      case TOK_LC:
      case TOK_MUL:
        kid = handler.newList(PNK_EXPORT_SPEC_LIST);
        if (!kid)
            return null();

        if (tt == TOK_LC) {
            while (true) {
                
                
                
                if (!tokenStream.peekToken(&tt))
                    return null();
                if (tt == TOK_RC)
                    break;

                MUST_MATCH_TOKEN(TOK_NAME, JSMSG_NO_BINDING_NAME);
                Node bindingName = newName(tokenStream.currentName());
                if (!bindingName)
                    return null();

                if (!tokenStream.getToken(&tt))
                    return null();
                if (tt == TOK_NAME && tokenStream.currentName() == context->names().as) {
                    if (!tokenStream.getToken(&tt, TokenStream::KeywordIsName))
                        return null();
                    if (tt != TOK_NAME) {
                        report(ParseError, false, null(), JSMSG_NO_EXPORT_NAME);
                        return null();
                    }
                } else {
                    tokenStream.ungetToken();
                }
                Node exportName = newName(tokenStream.currentName());
                if (!exportName)
                    return null();

                Node exportSpec = handler.newBinary(PNK_EXPORT_SPEC, bindingName, exportName);
                if (!exportSpec)
                    return null();

                handler.addList(kid, exportSpec);

                bool matched;
                if (!tokenStream.matchToken(&matched, TOK_COMMA))
                    return null();
                if (!matched)
                    break;
            }

            MUST_MATCH_TOKEN(TOK_RC, JSMSG_RC_AFTER_EXPORT_SPEC_LIST);
        } else {
            
            
            Node exportSpec = handler.newNullary(PNK_EXPORT_BATCH_SPEC, JSOP_NOP, pos());
            if (!kid)
                return null();

            handler.addList(kid, exportSpec);
        }
        if (!tokenStream.getToken(&tt))
            return null();
        if (tt == TOK_NAME && tokenStream.currentName() == context->names().from) {
            MUST_MATCH_TOKEN(TOK_STRING, JSMSG_MODULE_SPEC_AFTER_FROM);

            Node moduleSpec = stringLiteral();
            if (!moduleSpec)
                return null();

            if (!MatchOrInsertSemicolon(tokenStream))
                return null();

            return handler.newExportFromDeclaration(begin, kid, moduleSpec);
        } else {
            tokenStream.ungetToken();
        }

        kid = MatchOrInsertSemicolon(tokenStream) ? kid : nullptr;
        if (!kid)
            return null();
        break;

      case TOK_FUNCTION:
        kid = functionStmt();
        if (!kid)
            return null();
        break;

      case TOK_VAR:
        kid = variables(PNK_VAR);
        if (!kid)
            return null();
        kid->pn_xflags = PNX_POPVAR;

        kid = MatchOrInsertSemicolon(tokenStream) ? kid : nullptr;
        if (!kid)
            return null();
        break;

      case TOK_NAME:
        
        
        
        tokenStream.ungetToken();
      case TOK_LET:
      case TOK_CONST:
        kid = lexicalDeclaration(tt == TOK_CONST);
        if (!kid)
            return null();
        break;

      default:
        report(ParseError, false, null(), JSMSG_DECLARATION_AFTER_EXPORT);
        return null();
    }

    return handler.newExportDeclaration(kid, TokenPos(begin, pos().end));
}

template<>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::exportDeclaration()
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::expressionStatement(InvokedPrediction invoked)
{
    tokenStream.ungetToken();
    Node pnexpr = expr(invoked);
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

    TokenKind tt;
    if (!tokenStream.peekToken(&tt, TokenStream::Operand))
        return null();
    if (tt == TOK_SEMI) {
        if (!report(ParseExtraWarning, false, null(), JSMSG_EMPTY_CONSEQUENT))
            return null();
    }

    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_IF);
    Node thenBranch = statement();
    if (!thenBranch)
        return null();

    Node elseBranch;
    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_ELSE, TokenStream::Operand))
        return null();
    if (matched) {
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

    
    
    
    
    
    bool ignored;
    if (!tokenStream.matchToken(&ignored, TOK_SEMI))
        return null();
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
Parser<ParseHandler>::matchInOrOf(bool* isForInp, bool* isForOfp)
{
    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return false;
    *isForInp = tt == TOK_IN;
    *isForOfp = tt == TOK_NAME && tokenStream.currentToken().name() == context->names().of;
    if (!*isForInp && !*isForOfp)
        tokenStream.ungetToken();
    return true;
}

template <>
bool
Parser<FullParseHandler>::isValidForStatementLHS(ParseNode* pn1, JSVersion version,
                                                 bool isForDecl, bool isForEach,
                                                 ParseNodeKind headKind)
{
    if (isForDecl)
        return pn1->pn_count < 2 && !pn1->isKind(PNK_CONST);

    switch (pn1->getKind()) {
      case PNK_ARRAY:
      case PNK_CALL:
      case PNK_DOT:
      case PNK_SUPERPROP:
      case PNK_ELEM:
      case PNK_SUPERELEM:
      case PNK_NAME:
      case PNK_OBJECT:
        return true;

      default:
        return false;
    }
}

template <>
bool
Parser<FullParseHandler>::checkForHeadConstInitializers(ParseNode* pn1)
{
    if (!pn1->isKind(PNK_CONST))
        return true;

    for (ParseNode* assign = pn1->pn_head; assign; assign = assign->pn_next) {
        MOZ_ASSERT(assign->isKind(PNK_ASSIGN) || assign->isKind(PNK_NAME));
        if (assign->isKind(PNK_NAME) && !assign->isAssigned())
            return false;
        
    }
    return true;
}

template <>
ParseNode*
Parser<FullParseHandler>::forStatement()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));
    uint32_t begin = pos().begin;

    StmtInfoPC forStmt(context);
    PushStatementPC(pc, &forStmt, STMT_FOR_LOOP);

    bool isForEach = false;
    unsigned iflags = 0;

    if (allowsForEachIn()) {
        bool matched;
        if (!tokenStream.matchContextualKeyword(&matched, context->names().each))
            return null();
        if (matched) {
            iflags = JSITER_FOREACH;
            isForEach = true;
            addTelemetry(JSCompartment::DeprecatedForEach);
            if (versionNumber() < JSVERSION_LATEST) {
                if (!report(ParseWarning, pc->sc->strict(), null(), JSMSG_DEPRECATED_FOR_EACH))
                    return null();
            }
        }
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    



    bool isForDecl = false;

    
    RootedStaticBlockObject blockObj(context);

    
    ParseNode* pn1;

    {
        TokenKind tt;
        if (!tokenStream.peekToken(&tt, TokenStream::Operand))
            return null();
        if (tt == TOK_SEMI) {
            pn1 = nullptr;
        } else {
            












            pc->parsingForInit = true;
            if (tt == TOK_VAR) {
                isForDecl = true;
                tokenStream.consumeKnownToken(tt);
                pn1 = variables(PNK_VAR);
            } else if (tt == TOK_LET || tt == TOK_CONST) {
                handler.disableSyntaxParser();
                bool constDecl = tt == TOK_CONST;
                tokenStream.consumeKnownToken(tt);
                if (!tokenStream.peekToken(&tt))
                    return null();
                if (tt == TOK_LP) {
                    pn1 = deprecatedLetBlockOrExpression(LetExpression);
                } else {
                    isForDecl = true;
                    blockObj = StaticBlockObject::create(context);
                    if (!blockObj)
                        return null();
                    pn1 = variables(constDecl ? PNK_CONST : PNK_LET, nullptr, blockObj,
                                    DontHoistVars);
                }
            } else {
                pn1 = expr();
            }
            pc->parsingForInit = false;
            if (!pn1)
                return null();
        }
    }

    MOZ_ASSERT_IF(isForDecl, pn1->isArity(PN_LIST));
    MOZ_ASSERT(!!blockObj == (isForDecl && pn1->isOp(JSOP_NOP)));

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    ParseNode* forLetImpliedBlock = nullptr;
    ParseNode* forLetDecl = nullptr;

    
    
    ParseNode* hoistedVar = nullptr;

    





    StmtInfoPC letStmt(context); 
    ParseNode* pn2;      
    ParseNode* pn3;      
    ParseNodeKind headKind = PNK_FORHEAD;
    if (pn1) {
        bool isForIn, isForOf;
        if (!matchInOrOf(&isForIn, &isForOf))
            return null();
        if (isForIn)
            headKind = PNK_FORIN;
        else if (isForOf)
            headKind = PNK_FOROF;
    }

    if (headKind == PNK_FOROF || headKind == PNK_FORIN) {
        







        if (headKind == PNK_FOROF) {
            forStmt.type = (headKind == PNK_FOROF) ? STMT_FOR_OF_LOOP : STMT_FOR_IN_LOOP;
            if (isForEach) {
                report(ParseError, false, null(), JSMSG_BAD_FOR_EACH_LOOP);
                return null();
            }
        } else {
            forStmt.type = STMT_FOR_IN_LOOP;
            iflags |= JSITER_ENUMERATE;
        }

        
        if (!isValidForStatementLHS(pn1, versionNumber(), isForDecl, isForEach, headKind)) {
            report(ParseError, false, pn1, JSMSG_BAD_FOR_LEFTSIDE);
            return null();
        }

        





        if (isForDecl) {
            pn2 = pn1->pn_head;
            if ((pn2->isKind(PNK_NAME) && pn2->maybeExpr()) || pn2->isKind(PNK_ASSIGN)) {
                
                
                report(ParseError, false, pn2, JSMSG_INVALID_FOR_INOF_DECL_WITH_INIT,
                       headKind == PNK_FOROF ? "of" : "in");
                return null();
            }
        } else {
            
            MOZ_ASSERT(!blockObj);
            pn2 = pn1;
            pn1 = nullptr;

            if (!checkAndMarkAsAssignmentLhs(pn2, PlainAssignment))
                return null();
        }

        pn3 = (headKind == PNK_FOROF) ? assignExpr() : expr();
        if (!pn3)
            return null();

        if (blockObj) {
            





            ParseNode* block = pushLetScope(blockObj, &letStmt);
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

        ParseNodeKind kind2 = pn2->getKind();
        MOZ_ASSERT(kind2 != PNK_ASSIGN, "forStatement TOK_ASSIGN");

        if (kind2 == PNK_NAME) {
            
            pn2->markAsAssigned();
        }
    } else {
        if (isForEach) {
            reportWithOffset(ParseError, false, begin, JSMSG_BAD_FOR_EACH_LOOP);
            return null();
        }

        headKind = PNK_FORHEAD;

        if (blockObj) {
            
            
            if (!checkForHeadConstInitializers(pn1)) {
                report(ParseError, false, nullptr, JSMSG_BAD_CONST_DECL);
                return null();
            }

            
            
            
            
            
            
            
            
            
            forLetImpliedBlock = pushLetScope(blockObj, &letStmt);
            if (!forLetImpliedBlock)
                return null();
            letStmt.isForLetBlock = true;

            forLetDecl = pn1;

            
            
            
            
            
            
            
            
            
            if (pn1->isKind(PNK_CONST)) {
                pn1 = nullptr;
            } else {
                pn1 = handler.newFreshenBlock(pn1->pn_pos);
                if (!pn1)
                    return null();
            }
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        TokenKind tt;
        if (!tokenStream.peekToken(&tt, TokenStream::Operand))
            return null();
        if (tt == TOK_SEMI) {
            pn2 = nullptr;
        } else {
            pn2 = expr();
            if (!pn2)
                return null();
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        if (!tokenStream.peekToken(&tt, TokenStream::Operand))
            return null();
        if (tt == TOK_RP) {
            pn3 = nullptr;
        } else {
            pn3 = expr();
            if (!pn3)
                return null();
        }
    }

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    TokenPos headPos(begin, pos().end);
    ParseNode* forHead = handler.newForHead(headKind, pn1, pn2, pn3, headPos);
    if (!forHead)
        return null();

    
    ParseNode* body = statement();
    if (!body)
        return null();

    if (blockObj)
        PopStatementPC(tokenStream, pc);
    PopStatementPC(tokenStream, pc);

    ParseNode* forLoop = handler.newForStatement(begin, forHead, body, iflags);
    if (!forLoop)
        return null();

    if (hoistedVar) {
        ParseNode* pnseq = handler.newList(PNK_SEQ, hoistedVar);
        if (!pnseq)
            return null();
        pnseq->pn_pos = forLoop->pn_pos;
        pnseq->append(forLoop);
        return pnseq;
    }
    if (forLetImpliedBlock) {
        forLetImpliedBlock->pn_expr = forLoop;
        forLetImpliedBlock->pn_pos = forLoop->pn_pos;
        return handler.newLetBlock(forLetDecl, forLetImpliedBlock, forLoop->pn_pos);
    }
    return forLoop;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::forStatement()
{
    





    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    StmtInfoPC forStmt(context);
    PushStatementPC(pc, &forStmt, STMT_FOR_LOOP);

    
    if (allowsForEachIn()) {
        TokenKind tt;
        if (!tokenStream.peekToken(&tt))
            return null();
        
        
        if (tt == TOK_NAME || tt == TOK_YIELD) {
            JS_ALWAYS_FALSE(abortIfSyntaxParser());
            return null();
        }
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    
    bool isForDecl = false;
    bool simpleForDecl = true;

    
    Node lhsNode;

    {
        TokenKind tt;
        if (!tokenStream.peekToken(&tt, TokenStream::Operand))
            return null();
        if (tt == TOK_SEMI) {
            lhsNode = null();
        } else {
            
            pc->parsingForInit = true;
            if (tt == TOK_VAR) {
                isForDecl = true;
                tokenStream.consumeKnownToken(tt);
                lhsNode = variables(PNK_VAR, &simpleForDecl);
            }
            else if (tt == TOK_CONST || tt == TOK_LET) {
                JS_ALWAYS_FALSE(abortIfSyntaxParser());
                return null();
            }
            else {
                lhsNode = expr();
            }
            if (!lhsNode)
                return null();
            pc->parsingForInit = false;
        }
    }

    





    bool isForIn = false, isForOf = false;
    if (lhsNode) {
        if (!matchInOrOf(&isForIn, &isForOf))
            return null();
    }
    if (isForIn || isForOf) {
        
        forStmt.type = isForOf ? STMT_FOR_OF_LOOP : STMT_FOR_IN_LOOP;

        
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

        if (!isForDecl && !checkAndMarkAsAssignmentLhs(lhsNode, PlainAssignment))
            return null();

        if (!expr())
            return null();
    } else {
        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        TokenKind tt;
        if (!tokenStream.peekToken(&tt, TokenStream::Operand))
            return null();
        if (tt != TOK_SEMI) {
            if (!expr())
                return null();
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        if (!tokenStream.peekToken(&tt, TokenStream::Operand))
            return null();
        if (tt != TOK_RP) {
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
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_SWITCH));
    uint32_t begin = pos().begin;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

    Node discriminant = exprInParens();
    if (!discriminant)
        return null();

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_SWITCH);

    if (!GenerateBlockId(tokenStream, pc, pc->topStmt->blockid))
        return null();

    Node caseList = handler.newStatementList(pc->blockid(), pos());
    if (!caseList)
        return null();

    Node saveBlock = pc->blockNode;
    pc->blockNode = caseList;

    bool seenDefault = false;
    TokenKind tt;
    while (true) {
        if (!tokenStream.getToken(&tt))
            return null();
        if (tt == TOK_RC)
            break;
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

          default:
            report(ParseError, false, null(), JSMSG_BAD_SWITCH);
            return null();
        }

        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        Node body = handler.newStatementList(pc->blockid(), pos());
        if (!body)
            return null();

        bool afterReturn = false;
        bool warnedAboutStatementsAfterReturn = false;
        uint32_t statementBegin;
        while (true) {
            if (!tokenStream.peekToken(&tt, TokenStream::Operand))
                return null();
            if (tt == TOK_RC || tt == TOK_CASE || tt == TOK_DEFAULT)
                break;
            if (afterReturn) {
                TokenPos pos(0, 0);
                if (!tokenStream.peekTokenPos(&pos, TokenStream::Operand))
                    return null();
                statementBegin = pos.begin;
            }
            Node stmt = statement();
            if (!stmt)
                return null();
            if (!warnedAboutStatementsAfterReturn) {
                if (afterReturn) {
                    if (!handler.isStatementPermittedAfterReturnStatement(stmt)) {
                        if (!reportWithOffset(ParseWarning, false, statementBegin,
                                              JSMSG_STMT_AFTER_RETURN))
                        {
                            return null();
                        }
                        warnedAboutStatementsAfterReturn = true;
                    }
                } else if (handler.isReturnStatement(stmt)) {
                    afterReturn = true;
                }
            }
            handler.addList(body, stmt);
        }

        
        
        
        
        
        
        
        
        
        
        if (stmtInfo.isBlockScope)
            stmtInfo.firstDominatingLexicalInCase = stmtInfo.staticBlock().numVariables();

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
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_CONTINUE));
    uint32_t begin = pos().begin;

    RootedPropertyName label(context);
    if (!matchLabel(&label))
        return null();

    StmtInfoPC* stmt = pc->topStmt;
    if (label) {
        for (StmtInfoPC* stmt2 = nullptr; ; stmt = stmt->down) {
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
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_BREAK));
    uint32_t begin = pos().begin;

    RootedPropertyName label(context);
    if (!matchLabel(&label))
        return null();
    StmtInfoPC* stmt = pc->topStmt;
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
Parser<ParseHandler>::returnStatement()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_RETURN));
    uint32_t begin = pos().begin;

    if (!pc->sc->isFunctionBox()) {
        report(ParseError, false, null(), JSMSG_BAD_RETURN_OR_YIELD, js_return_str);
        return null();
    }

    
    
    
    Node exprNode;
    TokenKind tt;
    if (!tokenStream.peekTokenSameLine(&tt, TokenStream::Operand))
        return null();
    switch (tt) {
      case TOK_EOL:
      case TOK_EOF:
      case TOK_SEMI:
      case TOK_RC:
        exprNode = null();
        pc->funHasReturnVoid = true;
        break;
      default: {
        exprNode = expr();
        if (!exprNode)
            return null();
        pc->funHasReturnExpr = true;
      }
    }

    if (!MatchOrInsertSemicolon(tokenStream))
        return null();

    Node genrval = null();
    if (pc->isStarGenerator()) {
        genrval = newName(context->names().dotGenRVal);
        if (!genrval)
            return null();
        if (!noteNameUse(context->names().dotGenRVal, genrval))
            return null();
        if (!checkAndMarkAsAssignmentLhs(genrval, PlainAssignment))
            return null();
    }

    Node pn = handler.newReturnStatement(exprNode, genrval, TokenPos(begin, pos().end));
    if (!pn)
        return null();

    if (pc->isLegacyGenerator() && exprNode) {
        
        reportBadReturn(pn, ParseError, JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return null();
    }

    return pn;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::newYieldExpression(uint32_t begin, typename ParseHandler::Node expr,
                                         bool isYieldStar)
{
    Node generator = newName(context->names().dotGenerator);
    if (!generator)
        return null();
    if (!noteNameUse(context->names().dotGenerator, generator))
        return null();
    if (isYieldStar)
        return handler.newYieldStarExpression(begin, expr, generator);
    return handler.newYieldExpression(begin, expr, generator);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::yieldExpression()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_YIELD));
    uint32_t begin = pos().begin;

    switch (pc->generatorKind()) {
      case StarGenerator:
      {
        MOZ_ASSERT(pc->sc->isFunctionBox());

        pc->lastYieldOffset = begin;

        Node exprNode;
        ParseNodeKind kind = PNK_YIELD;
        TokenKind tt;
        if (!tokenStream.peekTokenSameLine(&tt, TokenStream::Operand))
            return null();
        switch (tt) {
          
          
          case TOK_EOL:
          
          
          
          
          case TOK_EOF:
          case TOK_SEMI:
          case TOK_RC:
          case TOK_RB:
          case TOK_RP:
          case TOK_COLON:
          case TOK_COMMA:
            
            exprNode = null();
            break;
          case TOK_MUL:
            kind = PNK_YIELD_STAR;
            tokenStream.consumeKnownToken(TOK_MUL);
            
          default:
            exprNode = assignExpr();
            if (!exprNode)
                return null();
        }
        return newYieldExpression(begin, exprNode, kind == PNK_YIELD_STAR);
      }

      case NotGenerator:
        
        
        MOZ_ASSERT(tokenStream.versionNumber() >= JSVERSION_1_7);
        MOZ_ASSERT(pc->lastYieldOffset == ParseContext<ParseHandler>::NoYieldOffset);

        if (!abortIfSyntaxParser())
            return null();

        if (!pc->sc->isFunctionBox()) {
            report(ParseError, false, null(), JSMSG_BAD_RETURN_OR_YIELD, js_yield_str);
            return null();
        }

        pc->sc->asFunctionBox()->setGeneratorKind(LegacyGenerator);
        addTelemetry(JSCompartment::DeprecatedLegacyGenerator);

        if (pc->funHasReturnExpr) {
            
            reportBadReturn(null(), ParseError, JSMSG_BAD_GENERATOR_RETURN,
                            JSMSG_BAD_ANON_GENERATOR_RETURN);
            return null();
        }
        

      case LegacyGenerator:
      {
        
        
        MOZ_ASSERT(pc->sc->isFunctionBox());

        pc->lastYieldOffset = begin;

        
        Node exprNode;
        TokenKind tt;
        if (!tokenStream.peekTokenSameLine(&tt, TokenStream::Operand))
            return null();
        switch (tt) {
          case TOK_EOF:
          case TOK_EOL:
          case TOK_SEMI:
          case TOK_RC:
          case TOK_RB:
          case TOK_RP:
          case TOK_COLON:
          case TOK_COMMA:
            
            exprNode = null();
            break;
          default:
            exprNode = assignExpr();
            if (!exprNode)
                return null();
        }

        return newYieldExpression(begin, exprNode);
      }
    }

    MOZ_CRASH("yieldExpr");
}

template <>
ParseNode*
Parser<FullParseHandler>::withStatement()
{
    
    
    if (handler.syntaxParser) {
        handler.disableSyntaxParser();
        abortedSyntaxParse = true;
        return null();
    }

    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_WITH));
    uint32_t begin = pos().begin;

    
    
    
    
    
    
    if (pc->sc->strict() && !report(ParseStrictError, true, null(), JSMSG_STRICT_CODE_WITH))
        return null();

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_WITH);
    Node objectExpr = exprInParens();
    if (!objectExpr)
        return null();
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_WITH);

    bool oldParsingWith = pc->parsingWith;
    pc->parsingWith = true;

    StmtInfoPC stmtInfo(context);
    PushStatementPC(pc, &stmtInfo, STMT_WITH);
    Rooted<StaticWithObject*> staticWith(context, StaticWithObject::create(context));
    if (!staticWith)
        return null();
    staticWith->initEnclosingNestedScopeFromParser(pc->staticScope);
    FinishPushNestedScope(pc, &stmtInfo, *staticWith);

    Node innerBlock = statement();
    if (!innerBlock)
        return null();

    PopStatementPC(tokenStream, pc);

    pc->sc->setBindingsAccessedDynamically();
    pc->parsingWith = oldParsingWith;

    



    for (AtomDefnRange r = pc->lexdeps->all(); !r.empty(); r.popFront()) {
        DefinitionNode defn = r.front().value().get<FullParseHandler>();
        DefinitionNode lexdep = handler.resolve(defn);
        if (!pc->sc->isDotVariable(lexdep->name()))
            handler.deoptimizeUsesWithin(lexdep, TokenPos(begin, pos().begin));
    }

    ObjectBox* staticWithBox = newObjectBox(staticWith);
    if (!staticWithBox)
        return null();
    return handler.newWithStatement(begin, objectExpr, innerBlock, staticWithBox);
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
    RootedPropertyName label(context, tokenStream.currentName());
    for (StmtInfoPC* stmt = pc->topStmt; stmt; stmt = stmt->down) {
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
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_THROW));
    uint32_t begin = pos().begin;

    
    TokenKind tt;
    if (!tokenStream.peekTokenSameLine(&tt, TokenStream::Operand))
        return null();
    if (tt == TOK_EOF || tt == TOK_SEMI || tt == TOK_RC) {
        report(ParseError, false, null(), JSMSG_MISSING_EXPR_AFTER_THROW);
        return null();
    }
    if (tt == TOK_EOL) {
        report(ParseError, false, null(), JSMSG_LINE_BREAK_AFTER_THROW);
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
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_TRY));
    uint32_t begin = pos().begin;

    

















    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    StmtInfoPC stmtInfo(context);
    if (!PushBlocklikeStatement(tokenStream, &stmtInfo, STMT_TRY, pc))
        return null();
    Node innerBlock = statements();
    if (!innerBlock)
        return null();
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatementPC(tokenStream, pc);

    bool hasUnconditionalCatch = false;
    Node catchList = null();
    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();
    if (tt == TOK_CATCH) {
        catchList = handler.newCatchList();
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

            




            data.initLexical(HoistVars, &pc->staticScope->template as<StaticBlockObject>(),
                             JSMSG_TOO_MANY_CATCH_VARS);
            MOZ_ASSERT(data.let.blockObj);

            if (!tokenStream.getToken(&tt))
                return null();
            Node catchName;
            switch (tt) {
              case TOK_LB:
              case TOK_LC:
                catchName = destructuringExpr(&data, tt);
                if (!catchName)
                    return null();
                break;

              case TOK_YIELD:
                if (!checkYieldNameValidity())
                    return null();
                
              case TOK_NAME:
              {
                RootedPropertyName label(context, tokenStream.currentName());
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
            




            bool matched;
            if (!tokenStream.matchToken(&matched, TOK_IF))
                return null();
            if (matched) {
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

            if (!tokenStream.getToken(&tt, TokenStream::Operand))
                return null();
        } while (tt == TOK_CATCH);
    }

    Node finallyBlock = null();

    if (tt == TOK_FINALLY) {
        MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_FINALLY);
        if (!PushBlocklikeStatement(tokenStream, &stmtInfo, STMT_FINALLY, pc))
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

template <>
ParseNode*
Parser<FullParseHandler>::classDefinition(ClassContext classContext)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_CLASS));

    bool savedStrictness = setLocalStrictMode(true);

    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return null();

    RootedPropertyName name(context);
    if (tt == TOK_NAME) {
        name = tokenStream.currentName();
    } else if (tt == TOK_YIELD) {
        if (!checkYieldNameValidity())
            return null();
        name = tokenStream.currentName();
    } else if (classContext == ClassStatement) {
        
        report(ParseError, false, null(), JSMSG_UNNAMED_CLASS_STMT);
        return null();
    } else {
        
        tokenStream.ungetToken();
    }

    if (name == context->names().let) {
        report(ParseError, false, null(), JSMSG_LET_CLASS_BINDING);
        return null();
    }

    ParseNode* classBlock = null();
    StmtInfoPC classStmt(context);
    if (name) {
        classBlock = pushLexicalScope(&classStmt);
        if (!classBlock)
            return null();
    }

    
    
    
    TokenPos namePos = pos();

    ParseNode* classHeritage = null();
    bool hasHeritage;
    if (!tokenStream.matchToken(&hasHeritage, TOK_EXTENDS))
        return null();
    if (hasHeritage) {
        if (!tokenStream.getToken(&tt))
            return null();
        classHeritage = memberExpr(tt, true);
        if (!classHeritage)
            return null();
    }

    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_CLASS);

    ParseNode* classMethods = propertyList(ClassBody);
    if (!classMethods)
        return null();

    ParseNode* nameNode = null();
    ParseNode* methodsOrBlock = classMethods;
    if (name) {
        ParseNode* innerBinding = makeInitializedLexicalBinding(name, true, namePos);
        if (!innerBinding)
            return null();

        MOZ_ASSERT(classBlock);
        handler.setLexicalScopeBody(classBlock, classMethods);
        methodsOrBlock = classBlock;

        PopStatementPC(tokenStream, pc);

        ParseNode* outerBinding = null();
        if (classContext == ClassStatement) {
            outerBinding = makeInitializedLexicalBinding(name, false, namePos);
            if (!outerBinding)
                return null();
        }

        nameNode = handler.newClassNames(outerBinding, innerBinding, namePos);
        if (!nameNode)
            return null();
    }

    MOZ_ALWAYS_TRUE(setLocalStrictMode(savedStrictness));

    return handler.newClass(nameNode, classHeritage, methodsOrBlock);
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::classDefinition(ClassContext classContext)
{
    MOZ_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::statement(bool canHaveDirectives)
{
    MOZ_ASSERT(checkOptionsCalled);

    JS_CHECK_RECURSION(context, return null());

    TokenKind tt;
    if (!tokenStream.getToken(&tt, TokenStream::Operand))
        return null();
    switch (tt) {
      case TOK_LC:
        return blockStatement();

      case TOK_CONST:
        if (!abortIfSyntaxParser())
            return null();
        return lexicalDeclaration( true);

      case TOK_VAR: {
        Node pn = variables(PNK_VAR);
        if (!pn)
            return null();

        
        handler.setListFlag(pn, PNX_POPVAR);

        if (!MatchOrInsertSemicolon(tokenStream))
            return null();
        return pn;
      }

      case TOK_LET:
        return letDeclarationOrBlock();
      case TOK_IMPORT:
        return importDeclaration();
      case TOK_EXPORT:
        return exportDeclaration();
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
        return returnStatement();
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
      case TOK_CLASS:
        if (!abortIfSyntaxParser())
            return null();
        return classDefinition(ClassStatement);

      
      case TOK_CATCH:
        report(ParseError, false, null(), JSMSG_CATCH_WITHOUT_TRY);
        return null();

      case TOK_FINALLY:
        report(ParseError, false, null(), JSMSG_FINALLY_WITHOUT_TRY);
        return null();

      case TOK_STRING:
        if (!canHaveDirectives && tokenStream.currentToken().atom() == context->names().useAsm) {
            if (!abortIfSyntaxParser())
                return null();
            if (!report(ParseWarning, false, null(), JSMSG_USE_ASM_DIRECTIVE_FAIL))
                return null();
        }
        return expressionStatement();

      case TOK_YIELD: {
        TokenKind next;
        TokenStream::Modifier modifier = yieldExpressionsSupported()
                                         ? TokenStream::Operand
                                         : TokenStream::None;
        if (!tokenStream.peekToken(&next, modifier))
            return null();
        if (next == TOK_COLON) {
            if (!checkYieldNameValidity())
                return null();
            return labeledStatement();
        }
        return expressionStatement();
      }

      case TOK_NAME: {
        TokenKind next;
        if (!tokenStream.peekToken(&next))
            return null();
        if (next == TOK_COLON)
            return labeledStatement();
        return expressionStatement();
      }

      case TOK_NEW:
        return expressionStatement(PredictInvoked);

      default:
        return expressionStatement();
    }
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::expr(InvokedPrediction invoked)
{
    Node pn = assignExpr(invoked);
    if (!pn)
        return null();

    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_COMMA))
        return null();
    if (matched) {
        Node seq = handler.newCommaExpressionList(pn);
        if (!seq)
            return null();
        while (true) {
            if (handler.isUnparenthesizedYieldExpression(pn)) {
                report(ParseError, false, pn, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
                return null();
            }

            pn = assignExpr();
            if (!pn)
                return null();
            handler.addList(seq, pn);

            if (!tokenStream.matchToken(&matched, TOK_COMMA))
                return null();
            if (!matched)
                break;
        }
        return seq;
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
    MOZ_ASSERT(pnk >= PNK_BINOP_FIRST);
    MOZ_ASSERT(pnk <= PNK_BINOP_LAST);
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
    MOZ_ASSERT(TokenKindIsBinaryOp(tok));
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

    MOZ_ASSERT(pnk >= PNK_BINOP_FIRST);
    MOZ_ASSERT(pnk <= PNK_BINOP_LAST);
    return PrecedenceTable[pnk - PNK_BINOP_FIRST];
}

template <typename ParseHandler>
MOZ_ALWAYS_INLINE typename ParseHandler::Node
Parser<ParseHandler>::orExpr1(InvokedPrediction invoked)
{
    
    

    
    
    Node nodeStack[PRECEDENCE_CLASSES];
    ParseNodeKind kindStack[PRECEDENCE_CLASSES];
    int depth = 0;

    bool oldParsingForInit = pc->parsingForInit;
    pc->parsingForInit = false;

    Node pn;
    for (;;) {
        pn = unaryExpr(invoked);
        if (!pn)
            return pn;

        
        
        TokenKind tok;
        if (!tokenStream.getToken(&tok))
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
            pn = handler.appendOrCreateList(combiningPnk, nodeStack[depth], pn, pc, combiningOp);
            if (!pn)
                return pn;
        }

        if (pnk == PNK_LIMIT)
            break;

        nodeStack[depth] = pn;
        kindStack[depth] = pnk;
        depth++;
        MOZ_ASSERT(depth <= PRECEDENCE_CLASSES);
    }

    MOZ_ASSERT(depth == 0);
    pc->parsingForInit = oldParsingForInit;
    return pn;
}

template <typename ParseHandler>
MOZ_ALWAYS_INLINE typename ParseHandler::Node
Parser<ParseHandler>::condExpr1(InvokedPrediction invoked)
{
    Node condition = orExpr1(invoked);
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

    
    TokenKind ignored;
    if (!tokenStream.getToken(&ignored))
        return null();
    return handler.newConditional(condition, thenExpr, elseExpr);
}

template <>
bool
Parser<FullParseHandler>::checkAndMarkAsAssignmentLhs(ParseNode* pn, AssignmentFlavor flavor)
{
    switch (pn->getKind()) {
      case PNK_NAME:
        if (!checkStrictAssignment(pn))
            return false;
        if (flavor == KeyedDestructuringAssignment) {
            




            if (!(js_CodeSpec[pn->getOp()].format & JOF_SET))
                pn->setOp(JSOP_SETNAME);
        } else {
            pn->setOp(pn->isOp(JSOP_GETLOCAL) ? JSOP_SETLOCAL : JSOP_SETNAME);
        }
        pn->markAsAssigned();
        break;

      case PNK_DOT:
      case PNK_ELEM:
      case PNK_SUPERPROP:
      case PNK_SUPERELEM:
        break;

      case PNK_ARRAY:
      case PNK_OBJECT:
        if (flavor == CompoundAssignment) {
            report(ParseError, false, null(), JSMSG_BAD_DESTRUCT_ASS);
            return false;
        }
        if (!checkDestructuring(nullptr, pn))
            return false;
        break;

      case PNK_CALL:
        if (flavor == KeyedDestructuringAssignment) {
            report(ParseError, false, pn, JSMSG_BAD_DESTRUCT_TARGET);
            return false;
        }
        if (!makeSetCall(pn, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return false;
        break;

      default:
        unsigned errnum = (flavor == KeyedDestructuringAssignment) ? JSMSG_BAD_DESTRUCT_TARGET :
            JSMSG_BAD_LEFTSIDE_OF_ASS;
        report(ParseError, false, pn, errnum);
        return false;
    }
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkAndMarkAsAssignmentLhs(Node pn, AssignmentFlavor flavor)
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
Parser<ParseHandler>::assignExpr(InvokedPrediction invoked)
{
    JS_CHECK_RECURSION(context, return null());

    
    
    
    
    
    
    
    
    
    

    TokenKind tt;
    if (!tokenStream.getToken(&tt, TokenStream::Operand))
        return null();

    bool endsExpr;

    if (tt == TOK_NAME) {
        if (!tokenStream.nextTokenEndsExpr(&endsExpr))
            return null();
        if (endsExpr)
            return identifierName();
    }

    if (tt == TOK_NUMBER) {
        if (!tokenStream.nextTokenEndsExpr(&endsExpr))
            return null();
        if (endsExpr)
            return newNumber(tokenStream.currentToken());
    }

    if (tt == TOK_STRING) {
        if (!tokenStream.nextTokenEndsExpr(&endsExpr))
            return null();
        if (endsExpr)
            return stringLiteral();
    }

    if (tt == TOK_YIELD && yieldExpressionsSupported())
        return yieldExpression();

    tokenStream.ungetToken();

    
    
    TokenStream::Position start(keepAtoms);
    tokenStream.tell(&start);

    Node lhs = condExpr1(invoked);
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
        
        tokenStream.ungetToken();
        TokenKind next;
        if (!tokenStream.peekTokenSameLine(&next) || next != TOK_ARROW) {
            report(ParseError, false, null(), JSMSG_UNEXPECTED_TOKEN,
                   "expression", TokenKindToDesc(TOK_ARROW));
            return null();
        }

        tokenStream.seek(start);
        if (!abortIfSyntaxParser())
            return null();

        TokenKind ignored;
        if (!tokenStream.peekToken(&ignored))
            return null();

        return functionDef(NullPtr(), Normal, Arrow, NotGenerator);
      }

      default:
        MOZ_ASSERT(!tokenStream.isCurrentTokenAssignment());
        tokenStream.ungetToken();
        return lhs;
    }

    AssignmentFlavor flavor = kind == PNK_ASSIGN ? PlainAssignment : CompoundAssignment;
    if (!checkAndMarkAsAssignmentLhs(lhs, flavor))
        return null();

    bool saved = pc->inDeclDestructuring;
    pc->inDeclDestructuring = false;
    Node rhs = assignExpr();
    pc->inDeclDestructuring = saved;
    if (!rhs)
        return null();

    return handler.newAssignment(kind, lhs, rhs, pc, op);
}

static const char incop_name_str[][10] = {"increment", "decrement"};

template <>
bool
Parser<FullParseHandler>::checkAndMarkAsIncOperand(ParseNode* kid, TokenKind tt, bool preorder)
{
    
    if (!kid->isKind(PNK_NAME) &&
        !kid->isKind(PNK_DOT) &&
        !kid->isKind(PNK_SUPERPROP) &&
        !kid->isKind(PNK_SUPERELEM) &&
        !kid->isKind(PNK_ELEM) &&
        !(kid->isKind(PNK_CALL) &&
          (kid->isOp(JSOP_CALL) || kid->isOp(JSOP_SPREADCALL) ||
           kid->isOp(JSOP_EVAL) || kid->isOp(JSOP_STRICTEVAL) ||
           kid->isOp(JSOP_SPREADEVAL) || kid->isOp(JSOP_STRICTSPREADEVAL) ||
           kid->isOp(JSOP_FUNCALL) ||
           kid->isOp(JSOP_FUNAPPLY))))
    {
        report(ParseError, false, null(), JSMSG_BAD_OPERAND, incop_name_str[tt == TOK_DEC]);
        return false;
    }

    if (!checkStrictAssignment(kid))
        return false;

    
    if (kid->isKind(PNK_NAME)) {
        kid->markAsAssigned();
    } else if (kid->isKind(PNK_CALL)) {
        if (!makeSetCall(kid, JSMSG_BAD_INCOP_OPERAND))
            return false;
    }
    return true;
}

template <>
bool
Parser<SyntaxParseHandler>::checkAndMarkAsIncOperand(Node kid, TokenKind tt, bool preorder)
{
    
    
    
    
    return checkAndMarkAsAssignmentLhs(kid, IncDecAssignment);
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
Parser<ParseHandler>::unaryExpr(InvokedPrediction invoked)
{
    Node pn, pn2;

    JS_CHECK_RECURSION(context, return null());

    TokenKind tt;
    if (!tokenStream.getToken(&tt, TokenStream::Operand))
        return null();
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
      case TOK_ADD:
        return unaryOpExpr(PNK_POS, JSOP_POS, begin);
      case TOK_SUB:
        return unaryOpExpr(PNK_NEG, JSOP_NEG, begin);

      case TOK_INC:
      case TOK_DEC:
      {
        TokenKind tt2;
        if (!tokenStream.getToken(&tt2, TokenStream::Operand))
            return null();
        pn2 = memberExpr(tt2, true);
        if (!pn2)
            return null();
        if (!checkAndMarkAsIncOperand(pn2, tt, true))
            return null();
        return handler.newUnary((tt == TOK_INC) ? PNK_PREINCREMENT : PNK_PREDECREMENT,
                                JSOP_NOP,
                                begin,
                                pn2);
      }

      case TOK_DELETE: {
        Node expr = unaryExpr();
        if (!expr)
            return null();

        
        
        if (handler.isName(expr)) {
            if (!report(ParseStrictError, pc->sc->strict(), expr, JSMSG_DEPRECATED_DELETE_OPERAND))
                return null();
            pc->sc->setBindingsAccessedDynamically();
        }

        return handler.newDelete(begin, expr);
      }

      default:
        pn = memberExpr(tt,  true, invoked);
        if (!pn)
            return null();

        
        if (!tokenStream.peekTokenSameLine(&tt, TokenStream::Operand))
            return null();
        if (tt == TOK_INC || tt == TOK_DEC) {
            tokenStream.consumeKnownToken(tt);
            if (!checkAndMarkAsIncOperand(pn, tt, false))
                return null();
            return handler.newUnary((tt == TOK_INC) ? PNK_POSTINCREMENT : PNK_POSTDECREMENT,
                                    JSOP_NOP,
                                    begin,
                                    pn);
        }
        return pn;
    }
}






















class LegacyCompExprTransplanter
{
    ParseNode*      root;
    Parser<FullParseHandler>* parser;
    ParseContext<FullParseHandler>* outerpc;
    GeneratorKind   comprehensionKind;
    unsigned        adjust;
    HashSet<Definition*> visitedImplicitArguments;

  public:
    LegacyCompExprTransplanter(ParseNode* pn, Parser<FullParseHandler>* parser,
                               ParseContext<FullParseHandler>* outerpc,
                               GeneratorKind kind, unsigned adj)
      : root(pn), parser(parser), outerpc(outerpc), comprehensionKind(kind), adjust(adj),
        visitedImplicitArguments(parser->context)
    {}

    bool init() {
        return visitedImplicitArguments.init();
    }

    bool transplant(ParseNode* pn);
};






template <typename ParseHandler>
static bool
BumpStaticLevel(TokenStream& ts, ParseNode* pn, ParseContext<ParseHandler>* pc)
{
    if (pn->pn_cookie.isFree())
        return true;

    unsigned level = unsigned(pn->pn_cookie.level()) + 1;
    MOZ_ASSERT(level >= pc->staticLevel);
    return pn->pn_cookie.set(ts, level, pn->pn_cookie.slot());
}

template <typename ParseHandler>
static bool
AdjustBlockId(TokenStream& ts, ParseNode* pn, unsigned adjust, ParseContext<ParseHandler>* pc)
{
    MOZ_ASSERT(pn->isArity(PN_LIST) || pn->isArity(PN_CODE) || pn->isArity(PN_NAME));
    if (BlockIdLimit - pn->pn_blockid <= adjust + 1) {
        ts.reportError(JSMSG_NEED_DIET, "program");
        return false;
    }
    pn->pn_blockid += adjust;
    if (pn->pn_blockid >= pc->blockidGen)
        pc->blockidGen = pn->pn_blockid + 1;
    return true;
}

bool
LegacyCompExprTransplanter::transplant(ParseNode* pn)
{
    ParseContext<FullParseHandler>* pc = parser->pc;

    bool isGenexp = comprehensionKind != NotGenerator;

    if (!pn)
        return true;

    switch (pn->getArity()) {
      case PN_LIST:
        for (ParseNode* pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
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
      case PN_BINARY_OBJ:
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
            if (isGenexp && !BumpStaticLevel(parser->tokenStream, pn, pc))
                return false;
        } else if (pn->isUsed()) {
            MOZ_ASSERT(pn->pn_cookie.isFree());

            Definition* dn = pn->pn_lexdef;
            MOZ_ASSERT(dn->isDefn());

            








            if (dn->isPlaceholder() && dn->pn_pos >= root->pn_pos && dn->dn_uses == pn) {
                if (isGenexp && !BumpStaticLevel(parser->tokenStream, dn, pc))
                    return false;
                if (!AdjustBlockId(parser->tokenStream, dn, adjust, pc))
                    return false;
            }

            RootedAtom atom(parser->context, pn->pn_atom);
#ifdef DEBUG
            StmtInfoPC* stmt = LexicalLookup(pc, atom, nullptr, (StmtInfoPC*)nullptr);
            MOZ_ASSERT(!stmt || stmt != pc->topStmt);
#endif
            if (isGenexp && !dn->isOp(JSOP_CALLEE)) {
                MOZ_ASSERT_IF(!pc->sc->isDotVariable(atom), !pc->decls().lookupFirst(atom));

                if (pc->sc->isDotVariable(atom)) {
                    if (dn->dn_uses == pn) {
                        if (!BumpStaticLevel(parser->tokenStream, dn, pc))
                            return false;
                        if (!AdjustBlockId(parser->tokenStream, dn, adjust, pc))
                            return false;
                    }
                } else if (dn->pn_pos < root->pn_pos) {
                    







                    Definition* dn2 = parser->handler.newPlaceholder(atom, parser->pc->blockid(),
                                                                     parser->pos());
                    if (!dn2)
                        return false;
                    dn2->pn_pos = root->pn_pos;

                    



                    ParseNode** pnup = &dn->dn_uses;
                    ParseNode* pnu;
                    while ((pnu = *pnup) != nullptr && pnu->pn_pos >= root->pn_pos) {
                        pnu->pn_lexdef = dn2;
                        dn2->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
                        pnup = &pnu->pn_link;
                    }
                    dn2->dn_uses = dn->dn_uses;
                    dn->dn_uses = *pnup;
                    *pnup = nullptr;
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
                    






                    if (isGenexp && !visitedImplicitArguments.has(dn)) {
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



















template <typename ParseHandler>
static unsigned
LegacyComprehensionHeadBlockScopeDepth(ParseContext<ParseHandler>* pc)
{
    return pc->topStmt ? pc->topStmt->innerBlockScopeDepth : pc->blockScopeDepth;
}










template <>
ParseNode*
Parser<FullParseHandler>::legacyComprehensionTail(ParseNode* bodyExpr, unsigned blockid,
                                                  GeneratorKind comprehensionKind,
                                                  ParseContext<FullParseHandler>* outerpc,
                                                  unsigned innerBlockScopeDepth)
{
    





    if (handler.syntaxParser) {
        handler.disableSyntaxParser();
        abortedSyntaxParse = true;
        return nullptr;
    }

    unsigned adjust;
    ParseNode* pn;
    ParseNode* pn3;
    ParseNode** pnp;
    StmtInfoPC stmtInfo(context);
    BindData<FullParseHandler> data(context);
    TokenKind tt;

    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    bool isGenexp = comprehensionKind != NotGenerator;

    if (isGenexp) {
        MOZ_ASSERT(comprehensionKind == LegacyGenerator);
        




        pn = pushLexicalScope(&stmtInfo);
        if (!pn)
            return null();
        adjust = pn->pn_blockid - blockid;
    } else {
        











        adjust = pc->blockid();
        pn = pushLexicalScope(&stmtInfo);
        if (!pn)
            return null();

        MOZ_ASSERT(blockid <= pn->pn_blockid);
        MOZ_ASSERT(blockid < pc->blockidGen);
        MOZ_ASSERT(pc->bodyid < blockid);
        pn->pn_blockid = stmtInfo.blockid = blockid;
        MOZ_ASSERT(adjust < blockid);
        adjust = blockid - adjust;
    }

    handler.setBeginPosition(pn, bodyExpr);

    pnp = &pn->pn_expr;

    LegacyCompExprTransplanter transplanter(bodyExpr, this, outerpc, comprehensionKind, adjust);
    if (!transplanter.init())
        return null();

    if (!transplanter.transplant(bodyExpr))
        return null();

    MOZ_ASSERT(pc->staticScope && pc->staticScope == pn->pn_objbox->object);
    data.initLexical(HoistVars, &pc->staticScope->as<StaticBlockObject>(),
                     JSMSG_ARRAY_INIT_TOO_BIG);

    while (true) {
        




        ParseNode* pn2 = handler.new_<BinaryNode>(PNK_FOR, JSOP_ITER, pos(),
                                                  nullptr, nullptr);
        if (!pn2)
            return null();

        pn2->pn_iflags = JSITER_ENUMERATE;
        if (allowsForEachIn()) {
            bool matched;
            if (!tokenStream.matchContextualKeyword(&matched, context->names().each))
                return null();
            if (matched) {
                pn2->pn_iflags |= JSITER_FOREACH;
                addTelemetry(JSCompartment::DeprecatedForEach);
                if (versionNumber() < JSVERSION_LATEST) {
                    if (!report(ParseWarning, pc->sc->strict(), pn2, JSMSG_DEPRECATED_FOR_EACH))
                        return null();
                }
            }
        }
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

        uint32_t startYieldOffset = pc->lastYieldOffset;

        RootedPropertyName name(context);
        if (!tokenStream.getToken(&tt))
            return null();
        switch (tt) {
          case TOK_LB:
          case TOK_LC:
            pc->inDeclDestructuring = true;
            pn3 = primaryExpr(tt);
            pc->inDeclDestructuring = false;
            if (!pn3)
                return null();
            break;

          case TOK_NAME:
            name = tokenStream.currentName();

            






            pn3 = newBindingNode(name, false);
            if (!pn3)
                return null();
            break;

          default:
            report(ParseError, false, null(), JSMSG_NO_VARIABLE_NAME);
            return null();
        }

        bool isForIn, isForOf;
        if (!matchInOrOf(&isForIn, &isForOf))
            return null();
        if (!isForIn && !isForOf) {
            report(ParseError, false, null(), JSMSG_IN_AFTER_FOR_NAME);
            return null();
        }
        ParseNodeKind headKind = PNK_FORIN;
        if (isForOf) {
            if (pn2->pn_iflags != JSITER_ENUMERATE) {
                MOZ_ASSERT(pn2->pn_iflags == (JSITER_FOREACH | JSITER_ENUMERATE));
                report(ParseError, false, null(), JSMSG_BAD_FOR_EACH_LOOP);
                return null();
            }
            pn2->pn_iflags = 0;
            headKind = PNK_FOROF;
        }

        ParseNode* pn4 = expr();
        if (!pn4)
            return null();
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

        if (isGenexp && pc->lastYieldOffset != startYieldOffset) {
            reportWithOffset(ParseError, false, pc->lastYieldOffset,
                             JSMSG_BAD_GENEXP_BODY, js_yield_str);
            return null();
        }

        switch (tt) {
          case TOK_LB:
          case TOK_LC:
            if (!checkDestructuring(&data, pn3))
                return null();
            break;

          case TOK_NAME:
            data.pn = pn3;
            if (!data.binder(&data, name, this))
                return null();
            break;

          default:;
        }

        






        ParseNode* lets = handler.newList(PNK_LET, pn3);
        if (!lets)
            return null();
        lets->pn_xflags |= PNX_POPVAR;

        
        pn3 = cloneLeftHandSide(pn3);
        if (!pn3)
            return null();

        pn2->pn_left = handler.newTernary(headKind, lets, pn3, pn4);
        if (!pn2->pn_left)
            return null();
        *pnp = pn2;
        pnp = &pn2->pn_right;

        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_FOR))
            return null();
        if (!matched)
            break;
    }

    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_IF))
        return null();
    if (matched) {
        ParseNode* cond = condition();
        if (!cond)
            return null();
        ParseNode* ifNode = handler.new_<TernaryNode>(PNK_IF, JSOP_NOP, cond, nullptr, nullptr,
                                                      cond->pn_pos);
        if (!ifNode)
            return null();
        *pnp = ifNode;
        pnp = &ifNode->pn_kid2;
    }

    ParseNode* bodyStmt;
    if (isGenexp) {
        ParseNode* yieldExpr = newYieldExpression(bodyExpr->pn_pos.begin, bodyExpr);
        if (!yieldExpr)
            return null();
        yieldExpr->setInParens(true);

        bodyStmt = handler.newExprStatement(yieldExpr, bodyExpr->pn_pos.end);
        if (!bodyStmt)
            return null();
    } else {
        bodyStmt = handler.newUnary(PNK_ARRAYPUSH, JSOP_ARRAYPUSH,
                                    bodyExpr->pn_pos.begin, bodyExpr);
        if (!bodyStmt)
            return null();
    }

    *pnp = bodyStmt;

    pc->topStmt->innerBlockScopeDepth += innerBlockScopeDepth;
    PopStatementPC(tokenStream, pc);

    handler.setEndPosition(pn, pos().end);

    return pn;
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::legacyComprehensionTail(SyntaxParseHandler::Node bodyStmt,
                                                    unsigned blockid,
                                                    GeneratorKind comprehensionKind,
                                                    ParseContext<SyntaxParseHandler>* outerpc,
                                                    unsigned innerBlockScopeDepth)
{
    abortIfSyntaxParser();
    return null();
}

template <>
ParseNode*
Parser<FullParseHandler>::legacyArrayComprehension(ParseNode* array)
{
    
    
    
    MOZ_ASSERT(array->isKind(PNK_ARRAY));
    MOZ_ASSERT(array->pn_count == 1);

    uint32_t arrayBegin = handler.getPosition(array).begin;
    uint32_t blockid = array->pn_blockid;

    ParseNode* bodyExpr = array->pn_head;
    array->pn_count = 0;
    array->pn_tail = &array->pn_head;
    *array->pn_tail = nullptr;

    handler.freeTree(array);

    ParseNode* comp = legacyComprehensionTail(bodyExpr, blockid, NotGenerator, nullptr,
                                              LegacyComprehensionHeadBlockScopeDepth(pc));
    if (!comp)
        return null();

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ARRAY_COMPREHENSION);

    return handler.newArrayComprehension(comp, blockid, TokenPos(arrayBegin, pos().end));
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::legacyArrayComprehension(Node array)
{
    abortIfSyntaxParser();
    return null();
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::generatorComprehensionLambda(GeneratorKind comprehensionKind,
                                                   unsigned begin, Node innerExpr)
{
    MOZ_ASSERT(comprehensionKind == LegacyGenerator || comprehensionKind == StarGenerator);
    MOZ_ASSERT(!!innerExpr == (comprehensionKind == LegacyGenerator));

    Node genfn = handler.newFunctionDefinition();
    if (!genfn)
        return null();
    handler.setOp(genfn, JSOP_LAMBDA);

    ParseContext<ParseHandler>* outerpc = pc;

    
    
    
    RootedObject proto(context);
    if (comprehensionKind == StarGenerator) {
        JSContext* cx = context->maybeJSContext();
        proto = GlobalObject::getOrCreateStarGeneratorFunctionPrototype(cx, context->global());
        if (!proto)
            return null();
    }

    RootedFunction fun(context, newFunction( NullPtr(), Expression, proto));
    if (!fun)
        return null();

    
    Directives directives( outerpc->sc->strict());
    FunctionBox* genFunbox = newFunctionBox(genfn, fun, outerpc, directives, comprehensionKind);
    if (!genFunbox)
        return null();

    ParseContext<ParseHandler> genpc(this, outerpc, genfn, genFunbox,
                                      nullptr,
                                     outerpc->staticLevel + 1, outerpc->blockidGen,
                                      0);
    if (!genpc.init(tokenStream))
        return null();

    





    genFunbox->anyCxFlags = outerpc->sc->anyCxFlags;
    if (outerpc->sc->isFunctionBox())
        genFunbox->funCxFlags = outerpc->sc->asFunctionBox()->funCxFlags;

    MOZ_ASSERT(genFunbox->generatorKind() == comprehensionKind);
    genFunbox->inGenexpLambda = true;
    handler.setBlockId(genfn, genpc.bodyid);

    Node generator = newName(context->names().dotGenerator);
    if (!generator)
        return null();
    if (!pc->define(tokenStream, context->names().dotGenerator, generator, Definition::VAR))
        return null();

    Node body = handler.newStatementList(pc->blockid(), TokenPos(begin, pos().end));
    if (!body)
        return null();

    Node comp;
    if (comprehensionKind == StarGenerator) {
        comp = comprehension(StarGenerator);
        if (!comp)
            return null();
    } else {
        MOZ_ASSERT(comprehensionKind == LegacyGenerator);
        comp = legacyComprehensionTail(innerExpr, outerpc->blockid(), LegacyGenerator,
                                       outerpc, LegacyComprehensionHeadBlockScopeDepth(outerpc));
        if (!comp)
            return null();
    }

    if (comprehensionKind == StarGenerator)
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);

    handler.setBeginPosition(comp, begin);
    handler.setEndPosition(comp, pos().end);
    handler.addStatementToList(body, comp, pc);
    handler.setEndPosition(body, pos().end);
    handler.setBeginPosition(genfn, begin);
    handler.setEndPosition(genfn, pos().end);

    generator = newName(context->names().dotGenerator);
    if (!generator)
        return null();
    if (!noteNameUse(context->names().dotGenerator, generator))
        return null();
    if (!handler.prependInitialYield(body, generator))
        return null();

    
    
    
    handler.setFunctionBody(genfn, body);

    PropagateTransitiveParseFlags(genFunbox, outerpc->sc);

    if (!leaveFunction(genfn, outerpc))
        return null();

    return genfn;
}

#if JS_HAS_GENERATOR_EXPRS
















template <>
ParseNode*
Parser<FullParseHandler>::legacyGeneratorExpr(ParseNode* expr)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    
    ParseNode* genfn = generatorComprehensionLambda(LegacyGenerator, expr->pn_pos.begin, expr);
    if (!genfn)
        return null();

    
    
    return handler.newList(PNK_GENEXP, genfn, JSOP_CALL);
}

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::legacyGeneratorExpr(Node kid)
{
    JS_ALWAYS_FALSE(abortIfSyntaxParser());
    return SyntaxParseHandler::NodeFailure;
}

static const char js_generator_str[] = "generator";

#endif 

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::comprehensionFor(GeneratorKind comprehensionKind)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    uint32_t begin = pos().begin;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

    

    MUST_MATCH_TOKEN(TOK_NAME, JSMSG_NO_VARIABLE_NAME);
    RootedPropertyName name(context, tokenStream.currentName());
    if (name == context->names().let) {
        report(ParseError, false, null(), JSMSG_LET_COMP_BINDING);
        return null();
    }
    Node assignLhs = newName(name);
    if (!assignLhs)
        return null();
    Node lhs = newName(name);
    if (!lhs)
        return null();
    bool matched;
    if (!tokenStream.matchContextualKeyword(&matched, context->names().of))
        return null();
    if (!matched) {
        report(ParseError, false, null(), JSMSG_OF_AFTER_FOR_NAME);
        return null();
    }

    Node rhs = assignExpr();
    if (!rhs)
        return null();

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_OF_ITERABLE);

    TokenPos headPos(begin, pos().end);

    StmtInfoPC stmtInfo(context);
    BindData<ParseHandler> data(context);
    RootedStaticBlockObject blockObj(context, StaticBlockObject::create(context));
    if (!blockObj)
        return null();
    data.initLexical(DontHoistVars, blockObj, JSMSG_TOO_MANY_LOCALS);
    Node decls = handler.newList(PNK_LET, lhs);
    if (!decls)
        return null();
    data.pn = lhs;
    if (!data.binder(&data, name, this))
        return null();
    Node letScope = pushLetScope(blockObj, &stmtInfo);
    if (!letScope)
        return null();
    handler.setLexicalScopeBody(letScope, decls);

    if (!noteNameUse(name, assignLhs))
        return null();
    handler.setOp(assignLhs, JSOP_SETNAME);

    Node head = handler.newForHead(PNK_FOROF, letScope, assignLhs, rhs, headPos);
    if (!head)
        return null();

    Node tail = comprehensionTail(comprehensionKind);
    if (!tail)
        return null();

    PopStatementPC(tokenStream, pc);

    return handler.newForStatement(begin, head, tail, JSOP_ITER);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::comprehensionIf(GeneratorKind comprehensionKind)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_IF));

    uint32_t begin = pos().begin;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_COND);
    Node cond = assignExpr();
    if (!cond)
        return null();
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    if (handler.isUnparenthesizedAssignment(cond)) {
        if (!report(ParseExtraWarning, false, null(), JSMSG_EQUAL_AS_ASSIGN))
            return null();
    }

    Node then = comprehensionTail(comprehensionKind);
    if (!then)
        return null();

    return handler.newIfStatement(begin, cond, then, null());
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::comprehensionTail(GeneratorKind comprehensionKind)
{
    JS_CHECK_RECURSION(context, return null());

    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_FOR, TokenStream::Operand))
        return null();
    if (matched)
        return comprehensionFor(comprehensionKind);

    if (!tokenStream.matchToken(&matched, TOK_IF, TokenStream::Operand))
        return null();
    if (matched)
        return comprehensionIf(comprehensionKind);

    uint32_t begin = pos().begin;

    Node bodyExpr = assignExpr();
    if (!bodyExpr)
        return null();

    if (comprehensionKind == NotGenerator)
        return handler.newUnary(PNK_ARRAYPUSH, JSOP_ARRAYPUSH, begin, bodyExpr);

    MOZ_ASSERT(comprehensionKind == StarGenerator);
    Node yieldExpr = newYieldExpression(begin, bodyExpr);
    if (!yieldExpr)
        return null();
    yieldExpr = handler.parenthesize(yieldExpr);

    return handler.newExprStatement(yieldExpr, pos().end);
}



template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::comprehension(GeneratorKind comprehensionKind)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    uint32_t startYieldOffset = pc->lastYieldOffset;

    Node body = comprehensionFor(comprehensionKind);
    if (!body)
        return null();

    if (comprehensionKind != NotGenerator && pc->lastYieldOffset != startYieldOffset) {
        reportWithOffset(ParseError, false, pc->lastYieldOffset,
                         JSMSG_BAD_GENEXP_BODY, js_yield_str);
        return null();
    }

    return body;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::arrayComprehension(uint32_t begin)
{
    Node inner = comprehension(NotGenerator);
    if (!inner)
        return null();

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ARRAY_COMPREHENSION);

    Node comp = handler.newList(PNK_ARRAYCOMP, inner);
    if (!comp)
        return null();

    handler.setBeginPosition(comp, begin);
    handler.setEndPosition(comp, pos().end);

    return comp;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::generatorComprehension(uint32_t begin)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_FOR));

    
    
    
    
    if (!abortIfSyntaxParser())
        return null();

    Node genfn = generatorComprehensionLambda(StarGenerator, begin, null());
    if (!genfn)
        return null();

    Node result = handler.newList(PNK_GENEXP, genfn, JSOP_CALL);
    if (!result)
        return null();
    handler.setBeginPosition(result, begin);
    handler.setEndPosition(result, pos().end);

    return result;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::assignExprWithoutYield(unsigned msg)
{
    uint32_t startYieldOffset = pc->lastYieldOffset;
    Node res = assignExpr();
    if (res && pc->lastYieldOffset != startYieldOffset) {
        reportWithOffset(ParseError, false, pc->lastYieldOffset,
                         msg, js_yield_str);
        return null();
    }
    return res;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::argumentList(Node listNode, bool* isSpread)
{
    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_RP, TokenStream::Operand))
        return false;
    if (matched) {
        handler.setEndPosition(listNode, pos().end);
        return true;
    }

    uint32_t startYieldOffset = pc->lastYieldOffset;
    bool arg0 = true;

    while (true) {
        bool spread = false;
        uint32_t begin = 0;
        if (!tokenStream.matchToken(&matched, TOK_TRIPLEDOT, TokenStream::Operand))
            return false;
        if (matched) {
            spread = true;
            begin = pos().begin;
            *isSpread = true;
        }

        Node argNode = assignExpr();
        if (!argNode)
            return false;
        if (spread) {
            argNode = handler.newUnary(PNK_SPREAD, JSOP_NOP, begin, argNode);
            if (!argNode)
                return false;
        }

        if (handler.isUnparenthesizedYieldExpression(argNode)) {
            TokenKind tt;
            if (!tokenStream.peekToken(&tt))
                return false;
            if (tt == TOK_COMMA) {
                report(ParseError, false, argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
                return false;
            }
        }
#if JS_HAS_GENERATOR_EXPRS
        if (!spread) {
            if (!tokenStream.matchToken(&matched, TOK_FOR))
                return false;
            if (matched) {
                if (pc->lastYieldOffset != startYieldOffset) {
                    reportWithOffset(ParseError, false, pc->lastYieldOffset,
                                     JSMSG_BAD_GENEXP_BODY, js_yield_str);
                    return false;
                }
                argNode = legacyGeneratorExpr(argNode);
                if (!argNode)
                    return false;
                if (!arg0) {
                    report(ParseError, false, argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
                    return false;
                }
                TokenKind tt;
                if (!tokenStream.peekToken(&tt))
                    return false;
                if (tt == TOK_COMMA) {
                    report(ParseError, false, argNode, JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
                    return false;
                }
            }
        }
#endif
        arg0 = false;

        handler.addList(listNode, argNode);

        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_COMMA))
            return false;
        if (!matched)
            break;
    }

    TokenKind tt;
    if (!tokenStream.getToken(&tt))
        return false;
    if (tt != TOK_RP) {
        report(ParseError, false, null(), JSMSG_PAREN_AFTER_ARGS);
        return false;
    }
    handler.setEndPosition(listNode, pos().end);
    return true;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::checkAndMarkSuperScope()
{
    for (GenericParseContext* gpc = pc; gpc; gpc = gpc->parent) {
        SharedContext* sc = gpc->sc;
        if (sc->allowSuperProperty()) {
            if (sc->isFunctionBox())
                sc->asFunctionBox()->setNeedsHomeObject();
            return true;
        } else if (sc->isFunctionBox() && !sc->asFunctionBox()->function()->isArrow()) {
            
            break;
        }
    }
    return false;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::memberExpr(TokenKind tt, bool allowCallSyntax, InvokedPrediction invoked)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(tt));

    Node lhs;

    JS_CHECK_RECURSION(context, return null());

    bool isSuper = false;
    uint32_t superBegin = pos().begin;

    
    if (tt == TOK_NEW) {
        lhs = handler.newList(PNK_NEW, JSOP_NEW);
        if (!lhs)
            return null();

        if (!tokenStream.getToken(&tt, TokenStream::Operand))
            return null();
        Node ctorExpr = memberExpr(tt, false, PredictInvoked);
        if (!ctorExpr)
            return null();

        handler.addList(lhs, ctorExpr);

        bool matched;
        if (!tokenStream.matchToken(&matched, TOK_LP))
            return null();
        if (matched) {
            bool isSpread = false;
            if (!argumentList(lhs, &isSpread))
                return null();
            if (isSpread)
                handler.setOp(lhs, JSOP_SPREADNEW);
        }
    } else if (tt == TOK_SUPER) {
        lhs = null();
        isSuper = true;
    } else {
        lhs = primaryExpr(tt, invoked);
        if (!lhs)
            return null();
    }

    while (true) {
        if (!tokenStream.getToken(&tt))
            return null();
        if (tt == TOK_EOF)
            break;

        Node nextMember;
        if (tt == TOK_DOT) {
            if (!tokenStream.getToken(&tt, TokenStream::KeywordIsName))
                return null();
            if (tt == TOK_NAME) {
                PropertyName* field = tokenStream.currentName();
                if (isSuper) {
                    isSuper = false;
                    if (!checkAndMarkSuperScope()) {
                        report(ParseError, false, null(), JSMSG_BAD_SUPERPROP, "property");
                        return null();
                    }
                    nextMember = handler.newSuperProperty(field, TokenPos(superBegin, pos().end));
                } else {
                    nextMember = handler.newPropertyAccess(lhs, field, pos().end);
                }
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

            if (isSuper) {
                isSuper = false;
                if (!checkAndMarkSuperScope()) {
                    report(ParseError, false, null(), JSMSG_BAD_SUPERPROP, "member");
                    return null();
                }
                nextMember = handler.newSuperElement(propExpr, TokenPos(superBegin, pos().end));
            } else {
                nextMember = handler.newPropertyByValue(lhs, propExpr, pos().end);
            }
            if (!nextMember)
                return null();
        } else if ((allowCallSyntax && tt == TOK_LP) ||
                   tt == TOK_TEMPLATE_HEAD ||
                   tt == TOK_NO_SUBS_TEMPLATE)
        {
            if (isSuper) {
                
                report(ParseError, false, null(), JSMSG_BAD_SUPER);
                return null();
            }

            JSOp op = JSOP_CALL;
            nextMember = handler.newList(tt == TOK_LP ? PNK_CALL : PNK_TAGGED_TEMPLATE, JSOP_CALL);
            if (!nextMember)
                return null();

            if (JSAtom* atom = handler.isName(lhs)) {
                if (tt == TOK_LP && atom == context->names().eval) {
                    
                    op = pc->sc->strict() ? JSOP_STRICTEVAL : JSOP_EVAL;
                    pc->sc->setBindingsAccessedDynamically();
                    pc->sc->setHasDirectEval();

                    



                    if (pc->sc->isFunctionBox() && !pc->sc->strict())
                        pc->sc->asFunctionBox()->setHasExtensibleScope();

                    
                    
                    
                    
                    checkAndMarkSuperScope();
                }
            } else if (JSAtom* atom = handler.isGetProp(lhs)) {
                
                if (atom == context->names().apply) {
                    op = JSOP_FUNAPPLY;
                    if (pc->sc->isFunctionBox())
                        pc->sc->asFunctionBox()->usesApply = true;
                } else if (atom == context->names().call) {
                    op = JSOP_FUNCALL;
                }
            }

            handler.setBeginPosition(nextMember, lhs);
            handler.addList(nextMember, lhs);

            if (tt == TOK_LP) {
                bool isSpread = false;
                if (!argumentList(nextMember, &isSpread))
                    return null();
                if (isSpread) {
                    if (op == JSOP_EVAL)
                        op = JSOP_SPREADEVAL;
                    else if (op == JSOP_STRICTEVAL)
                        op = JSOP_STRICTSPREADEVAL;
                    else
                        op = JSOP_SPREADCALL;
                }
            } else {
                if (!taggedTemplate(nextMember, tt))
                    return null();
            }
            handler.setOp(nextMember, op);
        } else {
            if (isSuper) {
                report(ParseError, false, null(), JSMSG_BAD_SUPER);
                return null();
            }
            tokenStream.ungetToken();
            return lhs;
        }

        lhs = nextMember;
    }

    if (isSuper) {
        report(ParseError, false, null(), JSMSG_BAD_SUPER);
        return null();
    }

    return lhs;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::newName(PropertyName* name)
{
    return handler.newName(name, pc->blockid(), pos());
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::identifierName()
{
    RootedPropertyName name(context, tokenStream.currentName());
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
    return handler.newStringLiteral(stopStringCompression(), pos());
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::noSubstitutionTemplate()
{
    return handler.newTemplateStringLiteral(stopStringCompression(), pos());
}

template <typename ParseHandler>
JSAtom * Parser<ParseHandler>::stopStringCompression() {
    JSAtom* atom = tokenStream.currentToken().atom();

    
    
    
    const size_t HUGE_STRING = 50000;
    if (sct && sct->active() && atom->length() >= HUGE_STRING)
        sct->abort();
    return atom;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::newRegExp()
{
    MOZ_ASSERT(!options().selfHostingMode);
    
    const char16_t* chars = tokenStream.getTokenbuf().begin();
    size_t length = tokenStream.getTokenbuf().length();
    RegExpFlag flags = tokenStream.currentToken().regExpFlags();

    Rooted<RegExpObject*> reobj(context);
    RegExpStatics* res = context->global()->getRegExpStatics(context);
    if (!res)
        return null();

    reobj = RegExpObject::create(context, res, chars, length, flags, &tokenStream, alloc);
    if (!reobj)
        return null();

    return handler.newRegExp(reobj, pos(), *this);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::arrayInitializer()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_LB));

    uint32_t begin = pos().begin;
    Node literal = handler.newArrayLiteral(begin, pc->blockidGen);
    if (!literal)
        return null();

    TokenKind tt;
    if (!tokenStream.getToken(&tt, TokenStream::Operand))
        return null();

    
    if (tt == TOK_FOR)
        return arrayComprehension(begin);

    if (tt == TOK_RB) {
        



        handler.setListFlag(literal, PNX_NONCONST);
    } else {
        tokenStream.ungetToken();

        bool spread = false, missingTrailingComma = false;
        uint32_t index = 0;
        for (; ; index++) {
            if (index == NativeObject::NELEMENTS_LIMIT) {
                report(ParseError, false, null(), JSMSG_ARRAY_INIT_TOO_BIG);
                return null();
            }

            TokenKind tt;
            if (!tokenStream.peekToken(&tt, TokenStream::Operand))
                return null();
            if (tt == TOK_RB)
                break;

            if (tt == TOK_COMMA) {
                tokenStream.consumeKnownToken(TOK_COMMA);
                if (!handler.addElision(literal, pos()))
                    return null();
            } else if (tt == TOK_TRIPLEDOT) {
                spread = true;
                tokenStream.consumeKnownToken(TOK_TRIPLEDOT);
                uint32_t begin = pos().begin;
                Node inner = assignExpr();
                if (!inner)
                    return null();
                if (!handler.addSpreadElement(literal, begin, inner))
                    return null();
            } else {
                Node element = assignExpr();
                if (!element)
                    return null();
                if (foldConstants && !FoldConstants(context, &element, this))
                    return null();
                handler.addArrayElement(literal, element);
            }

            if (tt != TOK_COMMA) {
                
                bool matched;
                if (!tokenStream.matchToken(&matched, TOK_COMMA))
                    return null();
                if (!matched) {
                    missingTrailingComma = true;
                    break;
                }
            }
        }

        













































        if (index == 0 && !spread) {
            bool matched;
            if (!tokenStream.matchToken(&matched, TOK_FOR))
                return null();
            if (matched && missingTrailingComma)
                return legacyArrayComprehension(literal);
        }

        MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_LIST);
    }
    handler.setEndPosition(literal, pos().end);
    return literal;
}

static JSAtom*
DoubleToAtom(ExclusiveContext* cx, double value)
{
    
    Value tmp = DoubleValue(value);
    return ToAtom<CanGC>(cx, HandleValue::fromMarkedLocation(&tmp));
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::computedPropertyName(Node literal)
{
    uint32_t begin = pos().begin;

    
    
    
    
    bool saved = pc->inDeclDestructuring;
    pc->inDeclDestructuring = false;
    Node assignNode = assignExpr();
    pc->inDeclDestructuring = saved;
    if (!assignNode)
        return null();

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_COMP_PROP_UNTERM_EXPR);
    Node propname = handler.newComputedName(assignNode, begin, pos().end);
    if (!propname)
        return null();
    handler.setListFlag(literal, PNX_NONCONST);
    return propname;
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::newPropertyListNode(PropListType type)
{
    if (type == ClassBody)
        return handler.newClassMethodList(pos().begin);

    MOZ_ASSERT(type == ObjectLiteral);
    return handler.newObjectLiteral(pos().begin);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::propertyList(PropListType type)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_LC));

    Node propList = newPropertyListNode(type);
    if (!propList)
        return null();

    bool seenPrototypeMutation = false;
    bool seenConstructor = false;
    RootedAtom atom(context);
    for (;;) {
        TokenKind ltok;
        if (!tokenStream.getToken(&ltok, TokenStream::KeywordIsName))
            return null();
        if (ltok == TOK_RC)
            break;

        bool isStatic = false;
        if (type == ClassBody) {
            if (ltok == TOK_SEMI)
                continue;

            if (ltok == TOK_NAME &&
                tokenStream.currentName() == context->names().static_)
            {
                isStatic = true;
                if (!tokenStream.getToken(&ltok, TokenStream::KeywordIsName))
                    return null();
            }
        }

        bool isGenerator = false;
        if (ltok == TOK_MUL) {
            isGenerator = true;
            if (!tokenStream.getToken(&ltok, TokenStream::KeywordIsName))
                return null();
        }

        atom = nullptr;

        JSOp op = JSOP_INITPROP;
        Node propname;
        switch (ltok) {
          case TOK_NUMBER:
            atom = DoubleToAtom(context, tokenStream.currentToken().number());
            if (!atom)
                return null();
            propname = newNumber(tokenStream.currentToken());
            if (!propname)
                return null();
            break;

          case TOK_LB: {
              propname = computedPropertyName(propList);
              if (!propname)
                  return null();
              break;
          }

          case TOK_NAME: {
            atom = tokenStream.currentName();
            
            if (!isGenerator &&
                (atom == context->names().get ||
                 atom == context->names().set))
            {
                op = atom == context->names().get ? JSOP_INITPROP_GETTER
                                                  : JSOP_INITPROP_SETTER;
            } else {
                propname = handler.newObjectLiteralPropertyName(atom, pos());
                if (!propname)
                    return null();
                break;
            }

            
            
            TokenKind tt;
            if (!tokenStream.getToken(&tt, TokenStream::KeywordIsName))
                return null();
            if (tt == TOK_NAME) {
                atom = tokenStream.currentName();
                propname = handler.newObjectLiteralPropertyName(atom, pos());
                if (!propname)
                    return null();
            } else if (tt == TOK_STRING) {
                atom = tokenStream.currentToken().atom();

                uint32_t index;
                if (atom->isIndex(&index)) {
                    propname = handler.newNumber(index, NoDecimal, pos());
                    if (!propname)
                        return null();
                    atom = DoubleToAtom(context, index);
                    if (!atom)
                        return null();
                } else {
                    propname = stringLiteral();
                    if (!propname)
                        return null();
                }
            } else if (tt == TOK_NUMBER) {
                atom = DoubleToAtom(context, tokenStream.currentToken().number());
                if (!atom)
                    return null();
                propname = newNumber(tokenStream.currentToken());
                if (!propname)
                    return null();
            } else if (tt == TOK_LB) {
                propname = computedPropertyName(propList);
                if (!propname)
                    return null();
            } else {
                
                tokenStream.ungetToken();
                propname = handler.newObjectLiteralPropertyName(atom, pos());
                if (!propname)
                    return null();
                op = JSOP_INITPROP;
                break;
            }

            MOZ_ASSERT(op == JSOP_INITPROP_GETTER || op == JSOP_INITPROP_SETTER);
            break;
          }

          case TOK_STRING: {
            atom = tokenStream.currentToken().atom();
            uint32_t index;
            if (atom->isIndex(&index)) {
                propname = handler.newNumber(index, NoDecimal, pos());
                if (!propname)
                    return null();
            } else {
                propname = stringLiteral();
                if (!propname)
                    return null();
            }
            break;
          }

          default:
            
            if (isStatic && !isGenerator) {
                
                isStatic = false;
                tokenStream.ungetToken();
                atom = tokenStream.currentName();
                propname = handler.newObjectLiteralPropertyName(atom->asPropertyName(), pos());
                if (!propname)
                    return null();
            } else {
                report(ParseError, false, null(), JSMSG_BAD_PROP_ID);
                return null();
            }
        }

        if (type == ClassBody) {
            if (!isStatic && atom == context->names().constructor) {
                if (isGenerator || op != JSOP_INITPROP) {
                    report(ParseError, false, propname, JSMSG_BAD_METHOD_DEF);
                    return null();
                }
                if (seenConstructor) {
                    report(ParseError, false, propname, JSMSG_DUPLICATE_PROPERTY, "constructor");
                    return null();
                }
                seenConstructor = true;
            } else if (isStatic && atom == context->names().prototype) {
                report(ParseError, false, propname, JSMSG_BAD_METHOD_DEF);
                return null();
            }
        }

        if (op == JSOP_INITPROP) {
            TokenKind tt;
            if (!tokenStream.getToken(&tt))
                return null();

            if (tt == TOK_COLON) {
                if (type == ClassBody) {
                    report(ParseError, false, null(), JSMSG_BAD_METHOD_DEF);
                    return null();
                }
                if (isGenerator) {
                    report(ParseError, false, null(), JSMSG_BAD_PROP_ID);
                    return null();
                }

                Node propexpr = assignExpr();
                if (!propexpr)
                    return null();

                if (foldConstants && !FoldConstants(context, &propexpr, this))
                    return null();

                if (atom == context->names().proto) {
                    if (seenPrototypeMutation) {
                        report(ParseError, false, propname, JSMSG_DUPLICATE_PROPERTY, "__proto__");
                        return null();
                    }
                    seenPrototypeMutation = true;

                    
                    
                    
                    
                    uint32_t begin = handler.getPosition(propname).begin;
                    if (!handler.addPrototypeMutation(propList, begin, propexpr))
                        return null();
                } else {
                    if (!handler.isConstant(propexpr))
                        handler.setListFlag(propList, PNX_NONCONST);

                    if (!handler.addPropertyDefinition(propList, propname, propexpr))
                        return null();
                }
            } else if (ltok == TOK_NAME && (tt == TOK_COMMA || tt == TOK_RC)) {
                



                if (type == ClassBody) {
                    report(ParseError, false, null(), JSMSG_BAD_METHOD_DEF);
                    return null();
                }
                if (isGenerator) {
                    report(ParseError, false, null(), JSMSG_BAD_PROP_ID);
                    return null();
                }

                tokenStream.ungetToken();
                if (!tokenStream.checkForKeyword(atom, nullptr))
                    return null();

                Node nameExpr = identifierName();
                if (!nameExpr)
                    return null();

                if (!handler.addShorthand(propList, propname, nameExpr))
                    return null();
            } else if (tt == TOK_LP) {
                tokenStream.ungetToken();
                if (!methodDefinition(type, propList, propname, Normal,
                                      isGenerator ? StarGenerator : NotGenerator, isStatic, op)) {
                    return null();
                }
            } else {
                report(ParseError, false, null(), JSMSG_COLON_AFTER_ID);
                return null();
            }
        } else {
            if (!methodDefinition(type, propList, propname, op == JSOP_INITPROP_GETTER ? Getter : Setter,
                                  NotGenerator, isStatic, op)) {
                return null();
            }
        }

        if (type == ObjectLiteral) {
            TokenKind tt;
            if (!tokenStream.getToken(&tt))
                return null();
            if (tt == TOK_RC)
                break;
            if (tt != TOK_COMMA) {
                report(ParseError, false, null(), JSMSG_CURLY_AFTER_LIST);
                return null();
            }
        }
    }

    
    if (type == ClassBody && !seenConstructor) {
        report(ParseError, false, null(), JSMSG_NO_CLASS_CONSTRUCTOR);
        return null();
    }

    handler.setEndPosition(propList, pos().end);
    return propList;
}

template <typename ParseHandler>
bool
Parser<ParseHandler>::methodDefinition(PropListType listType, Node propList, Node propname,
                                       FunctionType type, GeneratorKind generatorKind,
                                       bool isStatic, JSOp op)
{
    
    RootedPropertyName funName(context);
    if (type == Normal && tokenStream.isCurrentTokenType(TOK_NAME))
        funName = tokenStream.currentName();
    else
        funName = nullptr;

    Node fn = functionDef(funName, type, Method, generatorKind);
    if (!fn)
        return false;

    if (listType == ClassBody)
        return handler.addClassMethodDefinition(propList, propname, fn, op, isStatic);

    MOZ_ASSERT(listType == ObjectLiteral);
    return handler.addObjectMethodDefinition(propList, propname, fn, op);
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::primaryExpr(TokenKind tt, InvokedPrediction invoked)
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(tt));
    JS_CHECK_RECURSION(context, return null());

    switch (tt) {
      case TOK_FUNCTION:
        return functionExpr(invoked);

      case TOK_CLASS:
        return classDefinition(ClassExpression);

      case TOK_LB:
        return arrayInitializer();

      case TOK_LC:
        return propertyList(ObjectLiteral);

      case TOK_LET:
        return deprecatedLetBlockOrExpression(LetExpression);

      case TOK_LP: {
        TokenKind next;
        if (!tokenStream.peekToken(&next, TokenStream::Operand))
            return null();
        if (next != TOK_RP)
            return parenExprOrGeneratorComprehension();

        
        
        tokenStream.consumeKnownToken(next);

        if (!tokenStream.peekToken(&next))
            return null();
        if (next != TOK_ARROW) {
            report(ParseError, false, null(), JSMSG_UNEXPECTED_TOKEN,
                   "expression", TokenKindToDesc(TOK_RP));
            return null();
        }

        
        
        
        return handler.newNullLiteral(pos());
      }

      case TOK_TEMPLATE_HEAD:
        return templateLiteral();

      case TOK_NO_SUBS_TEMPLATE:
        return noSubstitutionTemplate();

      case TOK_STRING:
        return stringLiteral();

      case TOK_YIELD:
        if (!checkYieldNameValidity())
            return null();
        
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
        if (pc->sc->isFunctionBox())
            pc->sc->asFunctionBox()->usesThis = true;
        return handler.newThisLiteral(pos());
      case TOK_NULL:
        return handler.newNullLiteral(pos());

      case TOK_TRIPLEDOT: {
        TokenKind next;

        
        
        
        
        if (!tokenStream.getToken(&next))
            return null();
        if (next != TOK_NAME) {
            report(ParseError, false, null(), JSMSG_UNEXPECTED_TOKEN,
                   "rest argument name", TokenKindToDesc(next));
            return null();
        }

        if (!tokenStream.getToken(&next))
            return null();
        if (next != TOK_RP) {
            report(ParseError, false, null(), JSMSG_UNEXPECTED_TOKEN,
                   "closing parenthesis", TokenKindToDesc(next));
            return null();
        }

        if (!tokenStream.peekTokenSameLine(&next))
            return null();
        if (next != TOK_ARROW) {
            report(ParseError, false, null(), JSMSG_UNEXPECTED_TOKEN,
                   "'=>' after argument list", TokenKindToDesc(next));
            return null();
        }

        tokenStream.ungetToken();  

        
        return handler.newNullLiteral(pos());
      }

      default:
        report(ParseError, false, null(), JSMSG_UNEXPECTED_TOKEN,
               "expression", TokenKindToDesc(tt));
        return null();
    }
}

template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::parenExprOrGeneratorComprehension()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_LP));
    uint32_t begin = pos().begin;
    uint32_t startYieldOffset = pc->lastYieldOffset;

    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_FOR, TokenStream::Operand))
        return null();
    if (matched)
        return generatorComprehension(begin);

    




    bool oldParsingForInit = pc->parsingForInit;
    pc->parsingForInit = false;
    Node pn = expr(PredictInvoked);
    pc->parsingForInit = oldParsingForInit;

    if (!pn)
        return null();

#if JS_HAS_GENERATOR_EXPRS
    if (!tokenStream.matchToken(&matched, TOK_FOR))
        return null();
    if (matched) {
        if (pc->lastYieldOffset != startYieldOffset) {
            reportWithOffset(ParseError, false, pc->lastYieldOffset,
                             JSMSG_BAD_GENEXP_BODY, js_yield_str);
            return null();
        }
        if (handler.isUnparenthesizedCommaExpression(pn)) {
            report(ParseError, false, null(),
                   JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
            return null();
        }
        pn = legacyGeneratorExpr(pn);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
        TokenKind tt;
        if (!tokenStream.getToken(&tt))
            return null();
        if (tt != TOK_RP) {
            report(ParseError, false, null(),
                   JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
            return null();
        }
        handler.setEndPosition(pn, pos().end);
        return handler.parenthesize(pn);
    }
#endif 

    pn = handler.parenthesize(pn);

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);

    return pn;
}



















template <typename ParseHandler>
typename ParseHandler::Node
Parser<ParseHandler>::exprInParens()
{
    MOZ_ASSERT(tokenStream.isCurrentTokenType(TOK_LP));
    uint32_t begin = pos().begin;
    uint32_t startYieldOffset = pc->lastYieldOffset;

    




    bool oldParsingForInit = pc->parsingForInit;
    pc->parsingForInit = false;
    Node pn = expr(PredictInvoked);
    pc->parsingForInit = oldParsingForInit;

    if (!pn)
        return null();

#if JS_HAS_GENERATOR_EXPRS
    bool matched;
    if (!tokenStream.matchToken(&matched, TOK_FOR))
        return null();
    if (matched) {
        if (pc->lastYieldOffset != startYieldOffset) {
            reportWithOffset(ParseError, false, pc->lastYieldOffset,
                             JSMSG_BAD_GENEXP_BODY, js_yield_str);
            return null();
        }
        if (handler.isUnparenthesizedCommaExpression(pn)) {
            report(ParseError, false, null(),
                   JSMSG_BAD_GENERATOR_SYNTAX, js_generator_str);
            return null();
        }
        pn = legacyGeneratorExpr(pn);
        if (!pn)
            return null();
        handler.setBeginPosition(pn, begin);
    }
#endif 

    return pn;
}

template <typename ParseHandler>
void
Parser<ParseHandler>::addTelemetry(JSCompartment::DeprecatedLanguageExtension e)
{
    JSContext* cx = context->maybeJSContext();
    if (!cx)
        return;
    cx->compartment()->addTelemetry(getFilename(), e);
}

template class Parser<FullParseHandler>;
template class Parser<SyntaxParseHandler>;

} 
} 
