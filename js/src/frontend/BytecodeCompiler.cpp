





#include "frontend/BytecodeCompiler.h"

#include "jscntxt.h"
#include "jsscript.h"

#include "asmjs/AsmJSLink.h"
#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "frontend/NameFunctions.h"
#include "frontend/Parser.h"
#include "vm/GlobalObject.h"
#include "vm/TraceLogging.h"

#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "frontend/Parser-inl.h"

using namespace js;
using namespace js::frontend;
using mozilla::Maybe;

static bool
CheckLength(ExclusiveContext *cx, SourceBufferHolder &srcBuf)
{
    
    
    
    if (srcBuf.length() > UINT32_MAX) {
        if (cx->isJSContext())
            JS_ReportErrorNumber(cx->asJSContext(), js_GetErrorMessage, nullptr,
                                 JSMSG_SOURCE_TOO_LONG);
        return false;
    }
    return true;
}

static bool
SetDisplayURL(ExclusiveContext *cx, TokenStream &tokenStream, ScriptSource *ss)
{
    if (tokenStream.hasDisplayURL()) {
        if (!ss->setDisplayURL(cx, tokenStream.displayURL()))
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

    
    
    RootedScript script(cx, fun->getOrCreateScript(cx));
    if (!script)
        return false;
    if (script->argumentsHasVarBinding()) {
        if (!JSScript::argumentsOptimizationFailed(cx, script))
            return false;
    }

    
    if (script->isGeneratorExp() && script->isLegacyGenerator()) {
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
                RootedScript script(cx, scope->as<CallObject>().callee().getOrCreateScript(cx));
                if (!script)
                    return false;
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

static inline bool
CanLazilyParse(ExclusiveContext *cx, const ReadOnlyCompileOptions &options)
{
    return options.canLazilyParse &&
        options.compileAndGo &&
        !cx->compartment()->options().discardSource() &&
        !options.sourceIsLazy;
}

static void
MarkFunctionsWithinEvalScript(JSScript *script)
{
    
    

    if (!script->hasObjects())
        return;

    ObjectArray *objects = script->objects();
    size_t start = script->innerObjectsStart();

    for (size_t i = start; i < objects->length; i++) {
        JSObject *obj = objects->vector[i];
        if (obj->is<JSFunction>()) {
            JSFunction *fun = &obj->as<JSFunction>();
            if (fun->hasScript())
                fun->nonLazyScript()->setDirectlyInsideEval();
            else if (fun->isInterpretedLazy())
                fun->lazyScript()->setDirectlyInsideEval();
        }
    }
}

ScriptSourceObject *
frontend::CreateScriptSourceObject(ExclusiveContext *cx, const ReadOnlyCompileOptions &options)
{
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return nullptr;
    ScriptSourceHolder ssHolder(ss);

    if (!ss->initFromOptions(cx, options))
        return nullptr;

    RootedScriptSource sso(cx, ScriptSourceObject::create(cx, ss));
    if (!sso)
        return nullptr;

    
    
    
    
    
    
    
    
    
    if (cx->isJSContext()) {
        if (!ScriptSourceObject::initFromOptions(cx->asJSContext(), sso, options))
            return nullptr;
    }

    return sso;
}

JSScript *
frontend::CompileScript(ExclusiveContext *cx, LifoAlloc *alloc, HandleObject scopeChain,
                        HandleScript evalCaller,
                        const ReadOnlyCompileOptions &options,
                        SourceBufferHolder &srcBuf,
                        JSString *source_ ,
                        unsigned staticLevel ,
                        SourceCompressionTask *extraSct )
{
    JS_ASSERT(srcBuf.get());

    RootedString source(cx, source_);

    js::TraceLogger *logger = nullptr;
    if (cx->isJSContext())
        logger = TraceLoggerForMainThread(cx->asJSContext()->runtime());
    else
        logger = TraceLoggerForCurrentThread();
    uint32_t logId = js::TraceLogCreateTextId(logger, options);
    js::AutoTraceLog scriptLogger(logger, logId);
    js::AutoTraceLog typeLogger(logger, TraceLogger::ParserCompileScript);

    



    JS_ASSERT_IF(evalCaller, options.compileAndGo);
    JS_ASSERT_IF(evalCaller, options.forEval);
    JS_ASSERT_IF(staticLevel != 0, evalCaller);

    if (!CheckLength(cx, srcBuf))
        return nullptr;
    JS_ASSERT_IF(staticLevel != 0, !options.sourceIsLazy);

    RootedScriptSource sourceObject(cx, CreateScriptSourceObject(cx, options));
    if (!sourceObject)
        return nullptr;

    ScriptSource *ss = sourceObject->source();

    SourceCompressionTask mysct(cx);
    SourceCompressionTask *sct = extraSct ? extraSct : &mysct;

    if (!cx->compartment()->options().discardSource()) {
        if (options.sourceIsLazy)
            ss->setSourceRetrievable();
        else if (!ss->setSourceCopy(cx, srcBuf, false, sct))
            return nullptr;
    }

    bool canLazilyParse = CanLazilyParse(cx, options);

    Maybe<Parser<SyntaxParseHandler> > syntaxParser;
    if (canLazilyParse) {
        syntaxParser.emplace(cx, alloc, options, srcBuf.get(), srcBuf.length(),
                              false,
                             (Parser<SyntaxParseHandler> *) nullptr,
                             (LazyScript *) nullptr);
    }

    Parser<FullParseHandler> parser(cx, alloc, options, srcBuf.get(), srcBuf.length(),
                                     true,
                                    canLazilyParse ? syntaxParser.ptr() : nullptr, nullptr);
    parser.sct = sct;
    parser.ss = ss;

    Directives directives(options.strictOption);
    GlobalSharedContext globalsc(cx, scopeChain, directives, options.extraWarningsOption);

    bool savedCallerFun = options.compileAndGo &&
                          evalCaller && evalCaller->functionOrCallerFunction();
    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), savedCallerFun,
                                                  options, staticLevel, sourceObject, 0,
                                                  srcBuf.length()));
    if (!script)
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

    pc.emplace(&parser, (GenericParseContext *) nullptr, (ParseNode *) nullptr, &globalsc,
               (Directives *) nullptr, staticLevel,  0,
                0);
    if (!pc->init(parser.tokenStream))
        return nullptr;

    
    if (evalCaller && evalCaller->strict())
        globalsc.strict = true;

    if (options.compileAndGo) {
        if (source) {
            



            JSAtom *atom = AtomizeString(cx, source);
            jsatomid _;
            if (!atom || !bce.makeAtomIndex(atom, &_))
                return nullptr;
        }

        if (evalCaller && evalCaller->functionOrCallerFunction()) {
            




            JSFunction *fun = evalCaller->functionOrCallerFunction();
            Directives directives( fun->strict());
            ObjectBox *funbox = parser.newFunctionBox( nullptr, fun, pc.ptr(),
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

                
                
                if (!MaybeCheckEvalFreeVariables(cx, evalCaller, scopeChain, parser, *pc))
                    return nullptr;

                pc.reset();
                pc.emplace(&parser, (GenericParseContext *) nullptr, (ParseNode *) nullptr,
                           &globalsc, (Directives *) nullptr, staticLevel,  0,
                           script->bindings.numBlockScoped());
                if (!pc->init(parser.tokenStream))
                    return nullptr;
                JS_ASSERT(parser.pc == pc.ptr());

                pn = parser.statement();
            }
            if (!pn) {
                JS_ASSERT(!parser.hadAbortedSyntaxParse());
                return nullptr;
            }
        }

        
        
        
        script->bindings.updateNumBlockScoped(pc->blockScopeDepth);

        if (canHaveDirectives) {
            if (!parser.maybeParseDirective( nullptr, pn, &canHaveDirectives))
                return nullptr;
        }

        if (!FoldConstants(cx, &pn, &parser))
            return nullptr;

        if (!NameFunctions(cx, pn))
            return nullptr;

        if (!EmitTree(cx, &bce, pn))
            return nullptr;

        parser.handler.freeTree(pn);
    }

    if (!MaybeCheckEvalFreeVariables(cx, evalCaller, scopeChain, parser, *pc))
        return nullptr;

    if (!SetDisplayURL(cx, parser.tokenStream, ss))
        return nullptr;

    if (!SetSourceMap(cx, parser.tokenStream, ss))
        return nullptr;

    



    if (options.sourceMapURL()) {
        if (!ss->setSourceMapURL(cx, options.sourceMapURL()))
            return nullptr;
    }

    



    if (Emit1(cx, &bce, JSOP_RETRVAL) < 0)
        return nullptr;

    
    
    
    
    InternalHandle<Bindings*> bindings(script, &script->bindings);
    if (!Bindings::initWithTemporaryStorage(cx, bindings, 0, 0, nullptr,
                                            pc->blockScopeDepth))
        return nullptr;

    if (!JSScript::fullyInitFromEmitter(cx, script, &bce))
        return nullptr;

    
    
    
    if (options.forEval)
        MarkFunctionsWithinEvalScript(script);

    bce.tellDebuggerAboutCompiledScript(cx);

    if (sct && !extraSct && !sct->complete())
        return nullptr;

    return script;
}

bool
frontend::CompileLazyFunction(JSContext *cx, Handle<LazyScript*> lazy, const char16_t *chars, size_t length)
{
    JS_ASSERT(cx->compartment() == lazy->functionNonDelazifying()->compartment());

    CompileOptions options(cx, lazy->version());
    options.setOriginPrincipals(lazy->originPrincipals())
           .setFileAndLine(lazy->source()->filename(), lazy->lineno())
           .setColumn(lazy->column())
           .setCompileAndGo(true)
           .setNoScriptRval(false)
           .setSelfHostingMode(false);

    js::TraceLogger *logger = js::TraceLoggerForMainThread(cx->runtime());
    uint32_t logId = js::TraceLogCreateTextId(logger, options);
    js::AutoTraceLog scriptLogger(logger, logId);
    js::AutoTraceLog typeLogger(logger, TraceLogger::ParserCompileLazy);

    Parser<FullParseHandler> parser(cx, &cx->tempLifoAlloc(), options, chars, length,
                                     true, nullptr, lazy);

    uint32_t staticLevel = lazy->staticLevel(cx);

    Rooted<JSFunction*> fun(cx, lazy->functionNonDelazifying());
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
        script->setDirectlyInsideEval();
    if (lazy->usesArgumentsAndApply())
        script->setUsesArgumentsAndApply();
    if (lazy->hasBeenCloned())
        script->setHasBeenCloned();

    BytecodeEmitter bce( nullptr, &parser, pn->pn_funbox, script, options.forEval,
                         NullPtr(),  true,
                        options.lineno, BytecodeEmitter::LazyFunction);
    if (!bce.init())
        return false;

    if (lazy->treatAsRunOnce())
        bce.lazyRunOnceLambda = true;

    return EmitFunctionScript(cx, &bce, pn->pn_body);
}



static bool
CompileFunctionBody(JSContext *cx, MutableHandleFunction fun, const ReadOnlyCompileOptions &options,
                    const AutoNameVector &formals, SourceBufferHolder &srcBuf,
                    GeneratorKind generatorKind)
{
    js::TraceLogger *logger = js::TraceLoggerForMainThread(cx->runtime());
    uint32_t logId = js::TraceLogCreateTextId(logger, options);
    js::AutoTraceLog scriptLogger(logger, logId);
    js::AutoTraceLog typeLogger(logger, TraceLogger::ParserCompileFunction);

    
    

    if (!CheckLength(cx, srcBuf))
        return false;

    RootedScriptSource sourceObject(cx, CreateScriptSourceObject(cx, options));
    if (!sourceObject)
        return false;
    ScriptSource *ss = sourceObject->source();

    SourceCompressionTask sct(cx);
    JS_ASSERT(!options.sourceIsLazy);
    if (!cx->compartment()->options().discardSource()) {
        if (!ss->setSourceCopy(cx, srcBuf, true, &sct))
            return false;
    }

    bool canLazilyParse = CanLazilyParse(cx, options);

    Maybe<Parser<SyntaxParseHandler> > syntaxParser;
    if (canLazilyParse) {
        syntaxParser.emplace(cx, &cx->tempLifoAlloc(),
                             options, srcBuf.get(), srcBuf.length(),
                              false,
                             (Parser<SyntaxParseHandler> *) nullptr,
                             (LazyScript *) nullptr);
    }

    JS_ASSERT(!options.forEval);

    Parser<FullParseHandler> parser(cx, &cx->tempLifoAlloc(),
                                    options, srcBuf.get(), srcBuf.length(),
                                     true,
                                    canLazilyParse ? syntaxParser.ptr() : nullptr, nullptr);
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

        Rooted<JSScript*> script(cx, JSScript::Create(cx, js::NullPtr(), false, options,
                                                       0, sourceObject,
                                                       0, srcBuf.length()));
        if (!script)
            return false;

        script->bindings = fn->pn_funbox->bindings;

        






        BytecodeEmitter funbce( nullptr, &parser, fn->pn_funbox, script,
                                false,  js::NullPtr(),
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

    if (!SetDisplayURL(cx, parser.tokenStream, ss))
        return false;

    if (!SetSourceMap(cx, parser.tokenStream, ss))
        return false;

    if (!sct.complete())
        return false;

    return true;
}

bool
frontend::CompileFunctionBody(JSContext *cx, MutableHandleFunction fun,
                              const ReadOnlyCompileOptions &options,
                              const AutoNameVector &formals, JS::SourceBufferHolder &srcBuf)
{
    return CompileFunctionBody(cx, fun, options, formals, srcBuf, NotGenerator);
}

bool
frontend::CompileStarGeneratorBody(JSContext *cx, MutableHandleFunction fun,
                                   const ReadOnlyCompileOptions &options, const AutoNameVector &formals,
                                   JS::SourceBufferHolder &srcBuf)
{
    return CompileFunctionBody(cx, fun, options, formals, srcBuf, StarGenerator);
}
