





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
#include "vm/ScopeObject-inl.h"

using namespace js;
using namespace js::frontend;
using mozilla::Maybe;

class MOZ_STACK_CLASS AutoCompilationTraceLogger
{
  public:
    AutoCompilationTraceLogger(ExclusiveContext* cx, const TraceLoggerTextId id);

  private:
    TraceLoggerThread* logger;
    TraceLoggerEvent event;
    AutoTraceLog scriptLogger;
    AutoTraceLog typeLogger;
};



class MOZ_STACK_CLASS BytecodeCompiler
{
  public:
    
    BytecodeCompiler(ExclusiveContext* cx,
                     LifoAlloc* alloc,
                     const ReadOnlyCompileOptions& options,
                     SourceBufferHolder& sourceBuffer,
                     TraceLoggerTextId logId);

    
    void maybeSetSourceCompressor(SourceCompressionTask* sourceCompressor);
    void setEnclosingStaticScope(Handle<ScopeObject*> scope);
    void setSourceArgumentsNotIncluded();

    JSScript* compileScript(HandleObject scopeChain, HandleScript evalCaller,
                            unsigned staticLevel);
    bool compileFunctionBody(MutableHandleFunction fun, const AutoNameVector& formals,
                             GeneratorKind generatorKind);

  private:
    bool checkLength();
    bool createScriptSource();
    bool maybeCompressSource();
    bool canLazilyParse();
    bool createParser();
    bool createSourceAndParser();
    bool createScript(bool savedCallerFun = false, unsigned staticLevel = 0);
    bool createEmitter(SharedContext* sharedContext, HandleScript evalCaller = nullptr,
                       bool insideNonGlobalEval = false);
    bool isInsideNonGlobalEval();
    bool createParseContext(Maybe<ParseContext<FullParseHandler>>& parseContext,
                            GlobalSharedContext& globalsc, unsigned staticLevel = 0,
                            uint32_t blockScopeDepth = 0);
    bool saveCallerFun(HandleScript evalCaller, ParseContext<FullParseHandler>& parseContext);
    bool handleStatementParseFailure(HandleObject scopeChain, HandleScript evalCaller,
                                     unsigned staticLevel,
                                     Maybe<ParseContext<FullParseHandler>>& parseContext,
                                     GlobalSharedContext& globalsc);
    bool handleParseFailure(const Directives& newDirectives);
    bool prepareAndEmitTree(ParseNode** pn);
    bool checkArgumentsWithinEval(JSContext* cx, HandleFunction fun);
    bool maybeCheckEvalFreeVariables(HandleScript evalCaller, HandleObject scopeChain,
                                     ParseContext<FullParseHandler>& pc);
    bool maybeSetDisplayURL(TokenStream& tokenStream);
    bool maybeSetSourceMap(TokenStream& tokenStream);
    bool maybeSetSourceMapFromOptions();
    bool emitFinalReturn();
    bool initGlobalBindings(ParseContext<FullParseHandler>& pc);
    void markFunctionsWithinEvalScript();
    bool maybeCompleteCompressSource();

    AutoCompilationTraceLogger traceLogger;
    AutoKeepAtoms keepAtoms;

    ExclusiveContext* cx;
    LifoAlloc* alloc;
    const ReadOnlyCompileOptions& options;
    SourceBufferHolder& sourceBuffer;

    Rooted<ScopeObject*> enclosingStaticScope;
    bool sourceArgumentsNotIncluded;

    RootedScriptSource sourceObject;
    ScriptSource* scriptSource;

    Maybe<SourceCompressionTask> maybeSourceCompressor;
    SourceCompressionTask* sourceCompressor;

    Maybe<Parser<SyntaxParseHandler>> syntaxParser;
    Maybe<Parser<FullParseHandler>> parser;

    Directives directives;
    TokenStream::Position startPosition;

    RootedScript script;
    Maybe<BytecodeEmitter> emitter;
    };

AutoCompilationTraceLogger::AutoCompilationTraceLogger(ExclusiveContext* cx, const TraceLoggerTextId id)
  : logger(cx->isJSContext() ? TraceLoggerForMainThread(cx->asJSContext()->runtime())
                             : TraceLoggerForCurrentThread()),
    event(logger, TraceLogger_AnnotateScripts),
    scriptLogger(logger, event),
    typeLogger(logger, id)
{}

BytecodeCompiler::BytecodeCompiler(ExclusiveContext* cx,
                                   LifoAlloc* alloc,
                                   const ReadOnlyCompileOptions& options,
                                   SourceBufferHolder& sourceBuffer,
                                   TraceLoggerTextId logId)
  : traceLogger(cx, logId),
    keepAtoms(cx->perThreadData),
    cx(cx),
    alloc(alloc),
    options(options),
    sourceBuffer(sourceBuffer),
    enclosingStaticScope(cx),
    sourceArgumentsNotIncluded(false),
    sourceObject(cx),
    scriptSource(nullptr),
    sourceCompressor(nullptr),
    directives(options.strictOption),
    startPosition(keepAtoms),
    script(cx)
{
}

void
BytecodeCompiler::maybeSetSourceCompressor(SourceCompressionTask* sourceCompressor)
{
    this->sourceCompressor = sourceCompressor;
}

void
BytecodeCompiler::setEnclosingStaticScope(Handle<ScopeObject*> scope)
{
    enclosingStaticScope = scope;
}

void
BytecodeCompiler::setSourceArgumentsNotIncluded()
{
    sourceArgumentsNotIncluded = true;
}

bool
BytecodeCompiler::checkLength()
{
    
    
    
    if (sourceBuffer.length() > UINT32_MAX) {
        if (cx->isJSContext())
            JS_ReportErrorNumber(cx->asJSContext(), GetErrorMessage, nullptr,
                                 JSMSG_SOURCE_TOO_LONG);
        return false;
    }
    return true;
}

bool
BytecodeCompiler::createScriptSource()
{
    if (!checkLength())
        return false;

    sourceObject = CreateScriptSourceObject(cx, options);
    if (!sourceObject)
        return false;

    scriptSource = sourceObject->source();
    return true;
}

bool
BytecodeCompiler::maybeCompressSource()
{
    if (!sourceCompressor) {
        maybeSourceCompressor.emplace(cx);
        sourceCompressor = maybeSourceCompressor.ptr();
    }

    if (!cx->compartment()->options().discardSource()) {
        if (options.sourceIsLazy) {
            scriptSource->setSourceRetrievable();
        } else if (!scriptSource->setSourceCopy(cx, sourceBuffer, sourceArgumentsNotIncluded,
                                                sourceCompressor))
        {
            return nullptr;
        }
    }

    return true;
}

bool
BytecodeCompiler::canLazilyParse()
{
    return options.canLazilyParse &&
           !HasNonSyntacticStaticScopeChain(enclosingStaticScope) &&
           !cx->compartment()->options().disableLazyParsing() &&
           !cx->compartment()->options().discardSource() &&
           !options.sourceIsLazy;
}

bool
BytecodeCompiler::createParser()
{
    if (canLazilyParse()) {
        syntaxParser.emplace(cx, alloc, options, sourceBuffer.get(), sourceBuffer.length(),
                              false, (Parser<SyntaxParseHandler>*) nullptr,
                             (LazyScript*) nullptr);

        if (!syntaxParser->checkOptions())
            return false;
    }

    parser.emplace(cx, alloc, options, sourceBuffer.get(), sourceBuffer.length(),
                    true, syntaxParser.ptrOr(nullptr), nullptr);
    parser->sct = sourceCompressor;
    parser->ss = scriptSource;
    if (!parser->checkOptions())
        return false;

    parser->tokenStream.tell(&startPosition);
    return true;
}

bool
BytecodeCompiler::createSourceAndParser()
{
    return createScriptSource() &&
           maybeCompressSource() &&
           createParser();
}

bool
BytecodeCompiler::createScript(bool savedCallerFun, unsigned staticLevel)
{
    script = JSScript::Create(cx, enclosingStaticScope, savedCallerFun,
                              options, staticLevel,
                              sourceObject,  0,
                              sourceBuffer.length());

    return script != nullptr;
}

bool
BytecodeCompiler::createEmitter(SharedContext* sharedContext, HandleScript evalCaller,
                                bool insideNonGlobalEval)
{
    BytecodeEmitter::EmitterMode emitterMode =
        options.selfHostingMode ? BytecodeEmitter::SelfHosting : BytecodeEmitter::Normal;
    emitter.emplace( nullptr, parser.ptr(), sharedContext, script,
                     nullptr, options.forEval, evalCaller,
                    insideNonGlobalEval, options.lineno, emitterMode);
    return emitter->init();
}

bool BytecodeCompiler::isInsideNonGlobalEval()
{
    return enclosingStaticScope && enclosingStaticScope->is<StaticEvalObject>() &&
        enclosingStaticScope->as<StaticEvalObject>().enclosingScopeForStaticScopeIter();
}

bool
BytecodeCompiler::createParseContext(Maybe<ParseContext<FullParseHandler>>& parseContext,
                                     GlobalSharedContext& globalsc, unsigned staticLevel,
                                     uint32_t blockScopeDepth)
{
    parseContext.emplace(parser.ptr(), (GenericParseContext*) nullptr, (ParseNode*) nullptr,
                         &globalsc, (Directives*) nullptr, staticLevel,  0,
                         blockScopeDepth);
    return parseContext->init(parser->tokenStream);
}

bool
BytecodeCompiler::saveCallerFun(HandleScript evalCaller,
                                ParseContext<FullParseHandler>& parseContext)
{
    






    JSFunction* fun = evalCaller->functionOrCallerFunction();
    MOZ_ASSERT_IF(fun->strict(), options.strictOption);
    Directives directives( options.strictOption);
    ObjectBox* funbox = parser->newFunctionBox( nullptr, fun, &parseContext,
                                              directives, fun->generatorKind());
    if (!funbox)
        return false;

    emitter->objectList.add(funbox);
    return true;
}

bool
BytecodeCompiler::handleStatementParseFailure(HandleObject scopeChain, HandleScript evalCaller,
                                              unsigned staticLevel,
                                              Maybe<ParseContext<FullParseHandler>>& parseContext,
                                              GlobalSharedContext& globalsc)
{
    if (!parser->hadAbortedSyntaxParse())
        return false;

    
    
    
    
    
    parser->clearAbortedSyntaxParse();
    parser->tokenStream.seek(startPosition);

    
    
    if (!maybeCheckEvalFreeVariables(evalCaller, scopeChain, parseContext.ref()))
        return nullptr;

    parseContext.reset();
    if (!createParseContext(parseContext, globalsc, staticLevel, script->bindings.numBlockScoped()))
        return false;

    MOZ_ASSERT(parser->pc == parseContext.ptr());
    return true;
}

bool
BytecodeCompiler::handleParseFailure(const Directives& newDirectives)
{
    if (parser->hadAbortedSyntaxParse()) {
        
        
        
        parser->clearAbortedSyntaxParse();
    } else if (parser->tokenStream.hadError() || directives == newDirectives) {
        return false;
    }

    parser->tokenStream.seek(startPosition);

    
    MOZ_ASSERT_IF(directives.strict(), newDirectives.strict());
    MOZ_ASSERT_IF(directives.asmJS(), newDirectives.asmJS());
    directives = newDirectives;
    return true;
}

bool
BytecodeCompiler::prepareAndEmitTree(ParseNode** ppn)
{
    if (!FoldConstants(cx, ppn, parser.ptr()) ||
        !NameFunctions(cx, *ppn) ||
        !emitter->updateLocalsToFrameSlots() ||
        !emitter->emitTree(*ppn))
    {
        return false;
    }

    return true;
}

bool
BytecodeCompiler::maybeSetDisplayURL(TokenStream& tokenStream)
{
    if (tokenStream.hasDisplayURL()) {
        if (!scriptSource->setDisplayURL(cx, tokenStream.displayURL()))
            return false;
    }
    return true;
}

bool
BytecodeCompiler::maybeSetSourceMap(TokenStream& tokenStream)
{
    if (tokenStream.hasSourceMapURL()) {
        MOZ_ASSERT(!scriptSource->hasSourceMapURL());
        if (!scriptSource->setSourceMapURL(cx, tokenStream.sourceMapURL()))
            return false;
    }
    return true;
}

bool
BytecodeCompiler::maybeSetSourceMapFromOptions()
{
    



    if (options.sourceMapURL()) {
        
        if (scriptSource->hasSourceMapURL()) {
            if(!parser->report(ParseWarning, false, nullptr, JSMSG_ALREADY_HAS_PRAGMA,
                              scriptSource->filename(), "//# sourceMappingURL"))
                return false;
        }

        if (!scriptSource->setSourceMapURL(cx, options.sourceMapURL()))
            return false;
    }

    return true;
}

bool
BytecodeCompiler::checkArgumentsWithinEval(JSContext* cx, HandleFunction fun)
{
    if (fun->hasRest()) {
        
        
        parser->report(ParseError, false, nullptr, JSMSG_ARGUMENTS_AND_REST);
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
        parser->report(ParseError, false, nullptr, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
        return false;
    }

    return true;
}

bool
BytecodeCompiler::maybeCheckEvalFreeVariables(HandleScript evalCaller, HandleObject scopeChain,
                                              ParseContext<FullParseHandler>& pc)
{
    if (!evalCaller || !evalCaller->functionOrCallerFunction())
        return true;

    
    JSContext* cx = this->cx->asJSContext();

    
    
    RootedFunction fun(cx, evalCaller->functionOrCallerFunction());
    HandlePropertyName arguments = cx->names().arguments;
    for (AtomDefnRange r = pc.lexdeps->all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            if (!checkArgumentsWithinEval(cx, fun))
                return false;
        }
    }
    for (AtomDefnListMap::Range r = pc.decls().all(); !r.empty(); r.popFront()) {
        if (r.front().key() == arguments) {
            if (!checkArgumentsWithinEval(cx, fun))
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

bool
BytecodeCompiler::emitFinalReturn()
{
    



    return emitter->emit1(JSOP_RETRVAL);
}

bool
BytecodeCompiler::initGlobalBindings(ParseContext<FullParseHandler>& pc)
{
    
    
    
    
    InternalHandle<Bindings*> bindings(script, &script->bindings);
    return Bindings::initWithTemporaryStorage(cx, bindings, 0, 0, 0,
                                              pc.blockScopeDepth, 0, 0, nullptr);
}

void
BytecodeCompiler::markFunctionsWithinEvalScript()
{
    

    if (!script->hasObjects())
        return;

    ObjectArray* objects = script->objects();
    size_t start = script->innerObjectsStart();

    for (size_t i = start; i < objects->length; i++) {
        JSObject* obj = objects->vector[i];
        if (obj->is<JSFunction>()) {
            JSFunction* fun = &obj->as<JSFunction>();
            if (fun->hasScript())
                fun->nonLazyScript()->setDirectlyInsideEval();
            else if (fun->isInterpretedLazy())
                fun->lazyScript()->setDirectlyInsideEval();
        }
    }
}

bool
BytecodeCompiler::maybeCompleteCompressSource()
{
    return !maybeSourceCompressor || maybeSourceCompressor->complete();
}

JSScript*
BytecodeCompiler::compileScript(HandleObject scopeChain, HandleScript evalCaller,
                                          unsigned staticLevel)
{
    if (!createSourceAndParser())
        return nullptr;

    bool savedCallerFun = evalCaller && evalCaller->functionOrCallerFunction();
    if (!createScript(savedCallerFun, staticLevel))
        return nullptr;

    GlobalSharedContext globalsc(cx, directives, enclosingStaticScope, options.extraWarningsOption);
    if (!createEmitter(&globalsc, evalCaller, isInsideNonGlobalEval()))
        return nullptr;

    
    
    
    Maybe<ParseContext<FullParseHandler>> pc;
    if (!createParseContext(pc, globalsc, staticLevel))
        return nullptr;

    if (savedCallerFun && !saveCallerFun(evalCaller, pc.ref()))
        return nullptr;

    bool canHaveDirectives = true;
    for (;;) {
        TokenKind tt;
        if (!parser->tokenStream.peekToken(&tt, TokenStream::Operand))
            return nullptr;
        if (tt == TOK_EOF)
            break;

        parser->tokenStream.tell(&startPosition);

        ParseNode* pn = parser->statement(YieldIsName, canHaveDirectives);
        if (!pn) {
            if (!handleStatementParseFailure(scopeChain, evalCaller, staticLevel, pc, globalsc))
                return nullptr;

            pn = parser->statement(YieldIsName);
            if (!pn) {
                MOZ_ASSERT(!parser->hadAbortedSyntaxParse());
                return nullptr;
            }
        }

        
        
        
        script->bindings.updateNumBlockScoped(pc->blockScopeDepth);

        if (canHaveDirectives) {
            if (!parser->maybeParseDirective( nullptr, pn, &canHaveDirectives))
                return nullptr;
        }

        if (!prepareAndEmitTree(&pn))
            return nullptr;

        parser->handler.freeTree(pn);
    }

    if (!maybeCheckEvalFreeVariables(evalCaller, scopeChain, *pc) ||
        !maybeSetDisplayURL(parser->tokenStream) ||
        !maybeSetSourceMap(parser->tokenStream) ||
        !maybeSetSourceMapFromOptions() ||
        !emitFinalReturn() ||
        !initGlobalBindings(pc.ref()) ||
        !JSScript::fullyInitFromEmitter(cx, script, emitter.ptr()))
    {
        return nullptr;
    }

    
    
    
    if (options.forEval)
        markFunctionsWithinEvalScript();

    emitter->tellDebuggerAboutCompiledScript(cx);

    if (!maybeCompleteCompressSource())
        return nullptr;

    MOZ_ASSERT_IF(cx->isJSContext(), !cx->asJSContext()->isExceptionPending());
    return script;
}

bool
BytecodeCompiler::compileFunctionBody(MutableHandleFunction fun, const AutoNameVector& formals,
                                      GeneratorKind generatorKind)
{
    MOZ_ASSERT(fun);
    MOZ_ASSERT(fun->isTenured());

    fun->setArgCount(formals.length());

    if (!createSourceAndParser())
        return false;

    
    
    
    

    ParseNode* fn;
    do {
        Directives newDirectives = directives;
        fn = parser->standaloneFunctionBody(fun, formals, generatorKind, directives,
                                            &newDirectives);
        if (!fn && !handleParseFailure(newDirectives))
            return false;
    } while (!fn);

    if (!NameFunctions(cx, fn) ||
        !maybeSetDisplayURL(parser->tokenStream) ||
        !maybeSetSourceMap(parser->tokenStream))
    {
        return false;
    }

    if (fn->pn_funbox->function()->isInterpreted()) {
        MOZ_ASSERT(fun == fn->pn_funbox->function());

        if (!createScript())
            return false;

        script->bindings = fn->pn_funbox->bindings;

        if (!createEmitter(fn->pn_funbox) ||
            !emitter->emitFunctionScript(fn->pn_body))
        {
            return false;
        }
    } else {
        fun.set(fn->pn_funbox->function());
        MOZ_ASSERT(IsAsmJSModuleNative(fun->native()));
    }

    if (!maybeCompleteCompressSource())
        return false;

    return true;
}

ScriptSourceObject*
frontend::CreateScriptSourceObject(ExclusiveContext* cx, const ReadOnlyCompileOptions& options)
{
    ScriptSource* ss = cx->new_<ScriptSource>();
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

JSScript*
frontend::CompileScript(ExclusiveContext* cx, LifoAlloc* alloc, HandleObject scopeChain,
                        Handle<ScopeObject*> enclosingStaticScope,
                        HandleScript evalCaller,
                        const ReadOnlyCompileOptions& options,
                        SourceBufferHolder& srcBuf,
                        JSString* source_ ,
                        unsigned staticLevel ,
                        SourceCompressionTask* extraSct )
{
    MOZ_ASSERT(srcBuf.get());

    



    MOZ_ASSERT_IF(evalCaller, options.isRunOnce);
    MOZ_ASSERT_IF(evalCaller, options.forEval);
    MOZ_ASSERT_IF(evalCaller && evalCaller->strict(), options.strictOption);
    MOZ_ASSERT_IF(staticLevel != 0, evalCaller);
    MOZ_ASSERT_IF(staticLevel != 0, !options.sourceIsLazy);

    BytecodeCompiler compiler(cx, alloc, options, srcBuf, TraceLogger_ParserCompileScript);
    compiler.maybeSetSourceCompressor(extraSct);
    compiler.setEnclosingStaticScope(enclosingStaticScope);
    return compiler.compileScript(scopeChain, evalCaller, staticLevel);
}

bool
frontend::CompileLazyFunction(JSContext* cx, Handle<LazyScript*> lazy, const char16_t* chars, size_t length)
{
    MOZ_ASSERT(cx->compartment() == lazy->functionNonDelazifying()->compartment());

    CompileOptions options(cx, lazy->version());
    options.setMutedErrors(lazy->mutedErrors())
           .setFileAndLine(lazy->filename(), lazy->lineno())
           .setColumn(lazy->column())
           .setNoScriptRval(false)
           .setSelfHostingMode(false);

    AutoCompilationTraceLogger traceLogger(cx, TraceLogger_ParserCompileLazy);

    Parser<FullParseHandler> parser(cx, &cx->tempLifoAlloc(), options, chars, length,
                                     true, nullptr, lazy);
    if (!parser.checkOptions())
        return false;

    uint32_t staticLevel = lazy->staticLevel(cx);

    Rooted<JSFunction*> fun(cx, lazy->functionNonDelazifying());
    MOZ_ASSERT(!lazy->isLegacyGenerator());
    ParseNode* pn = parser.standaloneLazyFunction(fun, staticLevel, lazy->strict(),
                                                  lazy->generatorKind());
    if (!pn)
        return false;

    if (!NameFunctions(cx, pn))
        return false;

    RootedObject enclosingScope(cx, lazy->enclosingScope());
    RootedScriptSource sourceObject(cx, lazy->sourceObject());
    MOZ_ASSERT(sourceObject);

    Rooted<JSScript*> script(cx, JSScript::Create(cx, enclosingScope, false,
                                                  options, staticLevel,
                                                  sourceObject, lazy->begin(), lazy->end()));
    if (!script)
        return false;

    script->bindings = pn->pn_funbox->bindings;

    if (lazy->directlyInsideEval())
        script->setDirectlyInsideEval();
    if (lazy->usesArgumentsApplyAndThis())
        script->setUsesArgumentsApplyAndThis();
    if (lazy->hasBeenCloned())
        script->setHasBeenCloned();

    





    MOZ_ASSERT(!options.forEval);
    BytecodeEmitter bce( nullptr, &parser, pn->pn_funbox, script, lazy,
                         false,  nullptr,
                         false, options.lineno,
                        BytecodeEmitter::LazyFunction);
    if (!bce.init())
        return false;

    return bce.emitFunctionScript(pn->pn_body);
}



static bool
CompileFunctionBody(JSContext* cx, MutableHandleFunction fun, const ReadOnlyCompileOptions& options,
                    const AutoNameVector& formals, SourceBufferHolder& srcBuf,
                    Handle<ScopeObject*> enclosingStaticScope, GeneratorKind generatorKind)
{
    MOZ_ASSERT(!options.isRunOnce);

    
    

    BytecodeCompiler compiler(cx, &cx->tempLifoAlloc(), options, srcBuf,
                              TraceLogger_ParserCompileFunction);
    compiler.setEnclosingStaticScope(enclosingStaticScope);
    compiler.setSourceArgumentsNotIncluded();
    return compiler.compileFunctionBody(fun, formals, generatorKind);
}

bool
frontend::CompileFunctionBody(JSContext* cx, MutableHandleFunction fun,
                              const ReadOnlyCompileOptions& options,
                              const AutoNameVector& formals, JS::SourceBufferHolder& srcBuf,
                              Handle<ScopeObject*> enclosingStaticScope)
{
    return CompileFunctionBody(cx, fun, options, formals, srcBuf,
                               enclosingStaticScope, NotGenerator);
}

bool
frontend::CompileStarGeneratorBody(JSContext* cx, MutableHandleFunction fun,
                                   const ReadOnlyCompileOptions& options, const AutoNameVector& formals,
                                   JS::SourceBufferHolder& srcBuf)
{
    return CompileFunctionBody(cx, fun, options, formals, srcBuf, nullptr, StarGenerator);
}
