





#include "frontend/BytecodeCompiler.h"

#include "jscntxt.h"
#include "jsscript.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "frontend/NameFunctions.h"
#include "frontend/Parser.h"
#include "jit/AsmJSLink.h"
#include "vm/GlobalObject.h"

#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "frontend/Parser-inl.h"

using namespace js;
using namespace js::frontend;
using mozilla::Maybe;

static bool
CheckLength(ExclusiveContext *cx, size_t length)
{
    
    
    
    if (length > UINT32_MAX) {
        if (cx->isJSContext())
            JS_ReportErrorNumber(cx->asJSContext(), js_GetErrorMessage, nullptr,
                                 JSMSG_SOURCE_TOO_LONG);
        return false;
    }
    return true;
}

static bool
SetSourceURL(ExclusiveContext *cx, TokenStream &tokenStream, ScriptSource *ss)
{
    if (tokenStream.hasSourceURL()) {
        if (!ss->setSourceURL(cx, tokenStream.sourceURL()))
            return false;
    }
    return true;
}

static bool
SetSourceMap(ExclusiveContext *cx, TokenStream &tokenStream, ScriptSource *ss)
{
    if (tokenStream.hasSourceMapURL()) {
        if (!ss->setSourceMapURL(cx, tokenStream.sourceMapURL()))
            return false;
    }
    return true;
}

static bool
CheckArgumentsWithinEval(JSContext *cx, Parser<FullParseHandler> &parser, HandleFunction fun)
{
    if (fun->hasRest()) {
        
        
        parser.report(ParseError, false, nullptr, JSMSG_ARGUMENTS_AND_REST);
        return false;
    }

    
    
    RootedScript script(cx, fun->nonLazyScript());
    if (script->argumentsHasVarBinding()) {
        if (!JSScript::argumentsOptimizationFailed(cx, script))
            return false;
    }

    
    if (script->isGeneratorExp && script->isLegacyGenerator()) {
        parser.report(ParseError, false, nullptr, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
        return false;
    }

    return true;
}

static bool
MaybeCheckEvalFreeVariables(ExclusiveContext *cxArg, HandleScript evalCaller, HandleObject scopeChain,
                            Parser<FullParseHandler> &parser,
                            ParseContext<FullParseHandler> &pc)
{
    if (!evalCaller || !evalCaller->functionOrCallerFunction())
        return true;

    
    JSContext *cx = cxArg->asJSContext();

    
    
    RootedFunction fun(cx, evalCaller->functionOrCallerFunction());
    HandlePropertyName arguments = cx->names().arguments;
    for (AtomDefnRange r = pc.lexdeps->all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            if (!CheckArgumentsWithinEval(cx, parser, fun))
                return false;
        }
    }
    for (AtomDefnListMap::Range r = pc.decls().all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            if (!CheckArgumentsWithinEval(cx, parser, fun))
                return false;
        }
    }

    
    
    
    
    if (pc.sc->hasDebuggerStatement()) {
        RootedObject scope(cx, scopeChain);
        while (scope->is<ScopeObject>() || scope->is<DebugScopeObject>()) {
            if (scope->is<CallObject>() && !scope->as<CallObject>().isForEval()) {
                RootedScript script(cx, scope->as<CallObject>().callee().nonLazyScript());
                if (script->argumentsHasVarBinding()) {
                    if (!JSScript::argumentsOptimizationFailed(cx, script))
                        return false;
                }
            }
            scope = scope->enclosingScope();
        }
    }

    return true;
}

inline bool
CanLazilyParse(ExclusiveContext *cx, const CompileOptions &options)
{
    return options.canLazilyParse &&
        options.compileAndGo &&
        options.sourcePolicy == CompileOptions::SAVE_SOURCE &&
        !cx->compartment()->debugMode();
}

void
frontend::MaybeCallSourceHandler(JSContext *cx, const CompileOptions &options,
                                 const jschar *chars, size_t length)
{
    JSSourceHandler listener = cx->runtime()->debugHooks.sourceHandler;
    void *listenerData = cx->runtime()->debugHooks.sourceHandlerData;

    if (listener) {
        void *listenerTSData;
        listener(options.filename, options.lineno, chars, length,
                 &listenerTSData, listenerData);
    }
}

JSScript *
frontend::CompileScript(ExclusiveContext *cx, LifoAlloc *alloc, HandleObject scopeChain,
                        HandleScript evalCaller,
                        const CompileOptions &options,
                        const jschar *chars, size_t length,
                        JSString *source_ ,
                        unsigned staticLevel ,
                        SourceCompressionTask *extraSct )
{
    RootedString source(cx, source_);
    SkipRoot skip(cx, &chars);

#if JS_TRACE_LOGGING
        js::AutoTraceLog logger(js::TraceLogging::defaultLogger(),
                                js::TraceLogging::PARSER_COMPILE_SCRIPT_START,
                                js::TraceLogging::PARSER_COMPILE_SCRIPT_STOP,
                                options);
#endif

    if (cx->isJSContext())
        MaybeCallSourceHandler(cx->asJSContext(), options, chars, length);

    



    JS_ASSERT_IF(evalCaller, options.compileAndGo);
    JS_ASSERT_IF(evalCaller, options.forEval);
    JS_ASSERT_IF(staticLevel != 0, evalCaller);

    if (!CheckLength(cx, length))
        return nullptr;
    JS_ASSERT_IF(staticLevel != 0, options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    ScriptSource *ss = cx->new_<ScriptSource>(options.originPrincipals());
    if (!ss)
        return nullptr;
    if (options.filename && !ss->setFilename(cx, options.filename))
        return nullptr;

    RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss));
    if (!sourceObject)
        return nullptr;

    SourceCompressionTask mysct(cx);
    SourceCompressionTask *sct = extraSct ? extraSct : &mysct;

    switch (options.sourcePolicy) {
      case CompileOptions::SAVE_SOURCE:
        if (!ss->setSourceCopy(cx, chars, length, false, sct))
            return nullptr;
        break;
      case CompileOptions::LAZY_SOURCE:
        ss->setSourceRetrievable();
        break;
      case CompileOptions::NO_SOURCE:
        break;
    }

    bool canLazilyParse = CanLazilyParse(cx, options);

    Maybe<Parser<SyntaxParseHandler> > syntaxParser;
    if (canLazilyParse) {
        syntaxParser.construct(cx, alloc, options, chars, length,  false,
                               (Parser<SyntaxParseHandler> *) nullptr,
                               (LazyScript *) nullptr);
    }

    Parser<FullParseHandler> parser(cx, alloc, options, chars, length,  true,
                                    canLazilyParse ? &syntaxParser.ref() : nullptr, nullptr);
    parser.sct = sct;
    parser.ss = ss;

    Directives directives(options.strictOption);
    GlobalSharedContext globalsc(cx, scopeChain, directives, options.extraWarningsOption);

    bool savedCallerFun =
        options.compileAndGo &&
        evalCaller &&
        (evalCaller->function() || evalCaller->savedCallerFun);
    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), savedCallerFun,
                                                  options, staticLevel, sourceObject, 0, length));
    if (!script)
        return nullptr;

    
    
    InternalHandle<Bindings*> bindings(script, &script->bindings);
    if (!Bindings::initWithTemporaryStorage(cx, bindings, 0, 0, nullptr))
        return nullptr;

    
    JSObject *globalScope =
        scopeChain && scopeChain == &scopeChain->global() ? (JSObject*) scopeChain : nullptr;
    JS_ASSERT_IF(globalScope, globalScope->isNative());
    JS_ASSERT_IF(globalScope, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalScope->getClass()));

    BytecodeEmitter::EmitterMode emitterMode =
        options.selfHostingMode ? BytecodeEmitter::SelfHosting : BytecodeEmitter::Normal;
    BytecodeEmitter bce( nullptr, &parser, &globalsc, script, options.forEval,
                        evalCaller, !!globalScope, options.lineno, emitterMode);
    if (!bce.init())
        return nullptr;

    
    
    
    Maybe<ParseContext<FullParseHandler> > pc;

    pc.construct(&parser, (GenericParseContext *) nullptr, (ParseNode *) nullptr, &globalsc,
                 (Directives *) nullptr, staticLevel,  0);
    if (!pc.ref().init(parser.tokenStream))
        return nullptr;

    
    if (evalCaller && evalCaller->strict)
        globalsc.strict = true;

    if (options.compileAndGo) {
        if (source) {
            



            JSAtom *atom = AtomizeString<CanGC>(cx, source);
            jsatomid _;
            if (!atom || !bce.makeAtomIndex(atom, &_))
                return nullptr;
        }

        if (evalCaller && evalCaller->functionOrCallerFunction()) {
            




            JSFunction *fun = evalCaller->functionOrCallerFunction();
            Directives directives( fun->strict());
            ObjectBox *funbox = parser.newFunctionBox( nullptr, fun, pc.addr(),
                                                      directives, fun->generatorKind());
            if (!funbox)
                return nullptr;
            bce.objectList.add(funbox);
        }
    }

    bool canHaveDirectives = true;
    for (;;) {
        TokenKind tt = parser.tokenStream.peekToken(TokenStream::Operand);
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            return nullptr;
        }

        TokenStream::Position pos(parser.keepAtoms);
        parser.tokenStream.tell(&pos);

        ParseNode *pn = parser.statement(canHaveDirectives);
        if (!pn) {
            if (parser.hadAbortedSyntaxParse()) {
                
                
                
                
                
                parser.clearAbortedSyntaxParse();
                parser.tokenStream.seek(pos);

                
                
                if (!MaybeCheckEvalFreeVariables(cx, evalCaller, scopeChain, parser, pc.ref()))
                    return nullptr;

                pc.destroy();
                pc.construct(&parser, (GenericParseContext *) nullptr, (ParseNode *) nullptr,
                             &globalsc, (Directives *) nullptr, staticLevel,  0);
                if (!pc.ref().init(parser.tokenStream))
                    return nullptr;
                JS_ASSERT(parser.pc == pc.addr());
                pn = parser.statement();
            }
            if (!pn) {
                JS_ASSERT(!parser.hadAbortedSyntaxParse());
                return nullptr;
            }
        }

        if (canHaveDirectives) {
            if (!parser.maybeParseDirective( nullptr, pn, &canHaveDirectives))
                return nullptr;
        }

        if (!FoldConstants(cx, &pn, &parser))
            return nullptr;

        
        
        if (cx->isJSContext() && !NameFunctions(cx->asJSContext(), pn))
            return nullptr;

        if (!EmitTree(cx, &bce, pn))
            return nullptr;

        parser.handler.freeTree(pn);
    }

    if (!MaybeCheckEvalFreeVariables(cx, evalCaller, scopeChain, parser, pc.ref()))
        return nullptr;

    if (!SetSourceURL(cx, parser.tokenStream, ss))
        return nullptr;

    if (!SetSourceMap(cx, parser.tokenStream, ss))
        return nullptr;

    



    if (options.sourceMapURL) {
        if (!ss->setSourceMapURL(cx, options.sourceMapURL))
            return nullptr;
    }

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        return nullptr;

    if (!JSScript::fullyInitFromEmitter(cx, script, &bce))
        return nullptr;

    bce.tellDebuggerAboutCompiledScript(cx);

    if (sct && !extraSct && !sct->complete())
        return nullptr;

    return script;
}

bool
frontend::CompileLazyFunction(JSContext *cx, LazyScript *lazy, const jschar *chars, size_t length)
{
    JS_ASSERT(cx->compartment() == lazy->function()->compartment());

    CompileOptions options(cx, lazy->version());
    options.setPrincipals(cx->compartment()->principals)
           .setOriginPrincipals(lazy->originPrincipals())
           .setFileAndLine(lazy->source()->filename(), lazy->lineno())
           .setColumn(lazy->column())
           .setCompileAndGo(true)
           .setNoScriptRval(false)
           .setSelfHostingMode(false);

#if JS_TRACE_LOGGING
        js::AutoTraceLog logger(js::TraceLogging::defaultLogger(),
                                js::TraceLogging::PARSER_COMPILE_LAZY_START,
                                js::TraceLogging::PARSER_COMPILE_LAZY_STOP,
                                options);
#endif

    Parser<FullParseHandler> parser(cx, &cx->tempLifoAlloc(), options, chars, length,
                                     true, nullptr, lazy);

    uint32_t staticLevel = lazy->staticLevel(cx);

    Rooted<JSFunction*> fun(cx, lazy->function());
    JS_ASSERT(!lazy->isLegacyGenerator());
    ParseNode *pn = parser.standaloneLazyFunction(fun, staticLevel, lazy->strict(),
                                                  lazy->generatorKind());
    if (!pn)
        return false;

    if (!NameFunctions(cx, pn))
        return false;

    RootedObject enclosingScope(cx, lazy->enclosingScope());
    RootedScriptSource sourceObject(cx, lazy->sourceObject());
    JS_ASSERT(sourceObject);

    Rooted<JSScript*> script(cx, JSScript::Create(cx, enclosingScope, false,
                                                  options, staticLevel,
                                                  sourceObject, lazy->begin(), lazy->end()));
    if (!script)
        return false;

    script->bindings = pn->pn_funbox->bindings;

    if (lazy->directlyInsideEval())
        script->directlyInsideEval = true;
    if (lazy->usesArgumentsAndApply())
        script->usesArgumentsAndApply = true;

    BytecodeEmitter bce( nullptr, &parser, pn->pn_funbox, script, options.forEval,
                         NullPtr(),  true,
                        options.lineno, BytecodeEmitter::LazyFunction);
    if (!bce.init())
        return false;

    return EmitFunctionScript(cx, &bce, pn->pn_body);
}



static bool
CompileFunctionBody(JSContext *cx, MutableHandleFunction fun, CompileOptions options,
                    const AutoNameVector &formals, const jschar *chars, size_t length,
                    GeneratorKind generatorKind)
{
#if JS_TRACE_LOGGING
        js::AutoTraceLog logger(js::TraceLogging::defaultLogger(),
                                js::TraceLogging::PARSER_COMPILE_FUNCTION_START,
                                js::TraceLogging::PARSER_COMPILE_FUNCTION_STOP,
                                options);
#endif

    
    
    SkipRoot skip(cx, &chars);

    MaybeCallSourceHandler(cx, options, chars, length);

    if (!CheckLength(cx, length))
        return false;
    ScriptSource *ss = cx->new_<ScriptSource>(options.originPrincipals());
    if (!ss)
        return false;
    if (options.filename && !ss->setFilename(cx, options.filename))
        return false;
    RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss));
    if (!sourceObject)
        return false;
    SourceCompressionTask sct(cx);
    JS_ASSERT(options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    if (options.sourcePolicy == CompileOptions::SAVE_SOURCE) {
        if (!ss->setSourceCopy(cx, chars, length, true, &sct))
            return false;
    }

    bool canLazilyParse = CanLazilyParse(cx, options);

    Maybe<Parser<SyntaxParseHandler> > syntaxParser;
    if (canLazilyParse) {
        syntaxParser.construct(cx, &cx->tempLifoAlloc(),
                               options, chars, length,  false,
                               (Parser<SyntaxParseHandler> *) nullptr,
                               (LazyScript *) nullptr);
    }

    JS_ASSERT(!options.forEval);

    Parser<FullParseHandler> parser(cx, &cx->tempLifoAlloc(),
                                    options, chars, length,  true,
                                    canLazilyParse ? &syntaxParser.ref() : nullptr, nullptr);
    parser.sct = &sct;
    parser.ss = ss;

    JS_ASSERT(fun);
    JS_ASSERT(fun->isTenured());

    fun->setArgCount(formals.length());

    
    
    
    
    Directives directives(options.strictOption);

    TokenStream::Position start(parser.keepAtoms);
    parser.tokenStream.tell(&start);

    ParseNode *fn;
    while (true) {
        Directives newDirectives = directives;
        fn = parser.standaloneFunctionBody(fun, formals, generatorKind, directives, &newDirectives);
        if (fn)
            break;

        if (parser.hadAbortedSyntaxParse()) {
            
            
            
            parser.clearAbortedSyntaxParse();
        } else {
            if (parser.tokenStream.hadError() || directives == newDirectives)
                return false;

            
            JS_ASSERT_IF(directives.strict(), newDirectives.strict());
            JS_ASSERT_IF(directives.asmJS(), newDirectives.asmJS());
            directives = newDirectives;
        }

        parser.tokenStream.seek(start);
    }

    if (!NameFunctions(cx, fn))
        return false;

    if (fn->pn_funbox->function()->isInterpreted()) {
        JS_ASSERT(fun == fn->pn_funbox->function());

        Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), false, options,
                                                       0, sourceObject,
                                                       0, length));
        if (!script)
            return false;

        script->bindings = fn->pn_funbox->bindings;

        






        BytecodeEmitter funbce( nullptr, &parser, fn->pn_funbox, script,
                                false,  NullPtr(),
                               fun->environment() && fun->environment()->is<GlobalObject>(),
                               options.lineno);
        if (!funbce.init())
            return false;

        if (!EmitFunctionScript(cx, &funbce, fn->pn_body))
            return false;
    } else {
        fun.set(fn->pn_funbox->function());
        JS_ASSERT(IsAsmJSModuleNative(fun->native()));
    }

    if (!SetSourceURL(cx, parser.tokenStream, ss))
        return false;

    if (!SetSourceMap(cx, parser.tokenStream, ss))
        return false;

    if (!sct.complete())
        return false;

    return true;
}

bool
frontend::CompileFunctionBody(JSContext *cx, MutableHandleFunction fun, CompileOptions options,
                              const AutoNameVector &formals, const jschar *chars, size_t length)
{
    return CompileFunctionBody(cx, fun, options, formals, chars, length, NotGenerator);
}

bool
frontend::CompileStarGeneratorBody(JSContext *cx, MutableHandleFunction fun,
                                   CompileOptions options, const AutoNameVector &formals,
                                   const jschar *chars, size_t length)
{
    return CompileFunctionBody(cx, fun, options, formals, chars, length, StarGenerator);
}
