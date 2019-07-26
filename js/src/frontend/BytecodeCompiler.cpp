





#include "frontend/BytecodeCompiler.h"

#include "jsprobes.h"
#include "jsscript.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "frontend/NameFunctions.h"
#include "ion/AsmJS.h"
#include "vm/GlobalObject.h"

#include "jsinferinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/Parser-inl.h"
#include "frontend/SharedContext-inl.h"

using namespace js;
using namespace js::frontend;
using mozilla::Maybe;

static bool
CheckLength(JSContext *cx, size_t length)
{
    
    
    
    if (length > UINT32_MAX) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_SOURCE_TOO_LONG);
        return false;
    }
    return true;
}

static bool
SetSourceMap(JSContext *cx, TokenStream &tokenStream, ScriptSource *ss, JSScript *script)
{
    if (tokenStream.hasSourceMap()) {
        if (!ss->setSourceMap(cx, tokenStream.releaseSourceMap(), script->filename()))
            return false;
    }
    return true;
}

static bool
CheckArgumentsWithinEval(JSContext *cx, Parser<FullParseHandler> &parser, HandleFunction fun)
{
    if (fun->hasRest()) {
        
        
        parser.report(ParseError, false, NULL, JSMSG_ARGUMENTS_AND_REST);
        return false;
    }

    
    
    RootedScript script(cx, fun->nonLazyScript());
    if (script->argumentsHasVarBinding()) {
        if (!JSScript::argumentsOptimizationFailed(cx, script))
            return false;
    }

    
    if (script->isGeneratorExp) {
        parser.report(ParseError, false, NULL, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
        return false;
    }

    return true;
}

JSScript *
frontend::CompileScript(JSContext *cx, HandleObject scopeChain,
                        HandleScript evalCaller,
                        const CompileOptions &options,
                        const jschar *chars, size_t length,
                        JSString *source_ ,
                        unsigned staticLevel ,
                        SourceCompressionToken *extraSct )
{
    RootedString source(cx, source_);
    SkipRoot skip(cx, &chars);

    



    JS_ASSERT_IF(evalCaller, options.compileAndGo);
    JS_ASSERT_IF(evalCaller, options.forEval);
    JS_ASSERT_IF(staticLevel != 0, evalCaller);

    if (!CheckLength(cx, length))
        return NULL;
    JS_ASSERT_IF(staticLevel != 0, options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return NULL;
    if (options.filename && !ss->setFilename(cx, options.filename))
        return NULL;
    
    JS::RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss));
    if (!sourceObject)
        return NULL;
    SourceCompressionToken mysct(cx);
    SourceCompressionToken *sct = (extraSct) ? extraSct : &mysct;
    switch (options.sourcePolicy) {
      case CompileOptions::SAVE_SOURCE:
        if (!ss->setSourceCopy(cx, chars, length, false, sct))
            return NULL;
        break;
      case CompileOptions::LAZY_SOURCE:
        ss->setSourceRetrievable();
        break;
      case CompileOptions::NO_SOURCE:
        break;
    }

    Maybe<Parser<SyntaxParseHandler> > syntaxParser;
    if (options.canLazilyParse) {
        syntaxParser.construct(cx, options, chars, length,  false,
                               (Parser<SyntaxParseHandler> *) NULL,
                               (LazyScript *) NULL);
    }

    Parser<FullParseHandler> parser(cx, options, chars, length,  true,
                                    options.canLazilyParse ? &syntaxParser.ref() : NULL, NULL);
    parser.sct = sct;

    GlobalSharedContext globalsc(cx, scopeChain, StrictModeFromContext(cx));

    
    
    
    Maybe<ParseContext<FullParseHandler> > pc;

    pc.construct(&parser, (GenericParseContext *) NULL, &globalsc, staticLevel,  0);
    if (!pc.ref().init())
        return NULL;

    bool savedCallerFun =
        options.compileAndGo &&
        evalCaller &&
        (evalCaller->function() || evalCaller->savedCallerFun);
    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), savedCallerFun,
                                                  options, staticLevel, sourceObject, 0, length));
    if (!script)
        return NULL;

    
    
    InternalHandle<Bindings*> bindings(script, &script->bindings);
    if (!Bindings::initWithTemporaryStorage(cx, bindings, 0, 0, NULL))
        return NULL;

    
    JSObject *globalScope = scopeChain && scopeChain == &scopeChain->global() ? (JSObject*) scopeChain : NULL;
    JS_ASSERT_IF(globalScope, globalScope->isNative());
    JS_ASSERT_IF(globalScope, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalScope->getClass()));

    BytecodeEmitter::EmitterMode emitterMode =
        options.selfHostingMode ? BytecodeEmitter::SelfHosting : BytecodeEmitter::Normal;
    BytecodeEmitter bce( NULL, &parser, &globalsc, script, options.forEval, evalCaller,
                        !!globalScope, options.lineno, emitterMode);
    if (!bce.init())
        return NULL;

    
    if (evalCaller && evalCaller->strict)
        globalsc.strict = true;

    if (options.compileAndGo) {
        if (source) {
            



            JSAtom *atom = AtomizeString<CanGC>(cx, source);
            jsatomid _;
            if (!atom || !bce.makeAtomIndex(atom, &_))
                return NULL;
        }

        if (evalCaller && evalCaller->functionOrCallerFunction()) {
            




            JSFunction *fun = evalCaller->functionOrCallerFunction();
            ObjectBox *funbox = parser.newFunctionBox(fun, pc.addr(), fun->strict());
            if (!funbox)
                return NULL;
            bce.objectList.add(funbox);
        }
    }

    bool canHaveDirectives = true;
    for (;;) {
        TokenKind tt = parser.tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            return NULL;
        }

        TokenStream::Position pos(parser.keepAtoms);
        parser.tokenStream.tell(&pos);

        ParseNode *pn = parser.statement();
        if (!pn) {
            if (parser.hadAbortedSyntaxParse()) {
                
                
                
                
                
                parser.clearAbortedSyntaxParse();
                parser.tokenStream.seek(pos);
                pc.destroy();
                pc.construct(&parser, (GenericParseContext *) NULL, &globalsc,
                             staticLevel,  0);
                if (!pc.ref().init())
                    return NULL;
                JS_ASSERT(parser.pc == pc.addr());
                pn = parser.statement();
            }
            if (!pn) {
                JS_ASSERT(!parser.hadAbortedSyntaxParse());
                return NULL;
            }
        }

        if (canHaveDirectives) {
            if (!parser.maybeParseDirective(pn, &canHaveDirectives))
                return NULL;
        }

        if (!FoldConstants(cx, &pn, &parser))
            return NULL;
        if (!NameFunctions(cx, pn))
            return NULL;

        if (!EmitTree(cx, &bce, pn))
            return NULL;

        parser.handler.freeTree(pn);
    }

    if (!SetSourceMap(cx, parser.tokenStream, ss, script))
        return NULL;

    if (evalCaller && evalCaller->functionOrCallerFunction()) {
        
        
        RootedFunction fun(cx, evalCaller->functionOrCallerFunction());
        HandlePropertyName arguments = cx->names().arguments;
        for (AtomDefnRange r = pc.ref().lexdeps->all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                if (!CheckArgumentsWithinEval(cx, parser, fun))
                    return NULL;
            }
        }
        for (AtomDefnListMap::Range r = pc.ref().decls().all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                if (!CheckArgumentsWithinEval(cx, parser, fun))
                    return NULL;
            }
        }

        
        
        
        if (pc.ref().sc->hasDebuggerStatement()) {
            RootedObject scope(cx, scopeChain);
            while (scope->isScope() || scope->isDebugScope()) {
                if (scope->isCall() && !scope->asCall().isForEval()) {
                    RootedScript script(cx, scope->asCall().callee().nonLazyScript());
                    if (script->argumentsHasVarBinding()) {
                        if (!JSScript::argumentsOptimizationFailed(cx, script))
                            return NULL;
                    }
                }
                scope = scope->enclosingScope();
            }
        }
    }

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        return NULL;

    if (!JSScript::fullyInitFromEmitter(cx, script, &bce))
        return NULL;

    bce.tellDebuggerAboutCompiledScript(cx);

    if (sct == &mysct && !sct->complete())
        return NULL;

    return script;
}

bool
frontend::CompileLazyFunction(JSContext *cx, HandleFunction fun, LazyScript *lazy,
                              const jschar *chars, size_t length)
{
    CompileOptions options(cx);
    options.setPrincipals(cx->compartment->principals)
           .setOriginPrincipals(lazy->parent()->originPrincipals)
           .setVersion(lazy->parent()->getVersion())
           .setFileAndLine(lazy->parent()->filename(), lazy->lineno())
           .setColumn(lazy->column())
           .setCompileAndGo(lazy->parent()->compileAndGo)
           .setNoScriptRval(false)
           .setSelfHostingMode(false);

    Parser<FullParseHandler> parser(cx, options, chars, length,
                                     true, NULL, lazy);

    RootedObject enclosingScope(cx, lazy->parent()->function());

    ParseNode *pn = parser.standaloneLazyFunction(fun, lazy->parent()->staticLevel + 1,
                                                  lazy->strict());
    if (!pn)
        return false;

    JS::RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, lazy->source()));
    if (!sourceObject)
        return false;

    Rooted<JSScript*> script(cx, JSScript::Create(cx, enclosingScope, false,
                                                  options, lazy->parent()->staticLevel + 1,
                                                  sourceObject, lazy->begin(), lazy->end()));
    if (!script)
        return false;

    script->bindings = pn->pn_funbox->bindings;

    if (lazy->directlyInsideEval())
        script->directlyInsideEval = true;

    bool hasGlobalScope = lazy->parent()->compileAndGo;

    BytecodeEmitter bce( NULL, &parser, pn->pn_funbox, script, options.forEval,
                         NullPtr(), hasGlobalScope,
                        options.lineno, BytecodeEmitter::LazyFunction);
    if (!bce.init())
        return false;

    return EmitFunctionScript(cx, &bce, pn->pn_body);
}



bool
frontend::CompileFunctionBody(JSContext *cx, MutableHandleFunction fun, CompileOptions options,
                              const AutoNameVector &formals, const jschar *chars, size_t length,
                              bool isAsmJSRecompile)
{
    SkipRoot skip(cx, &chars);

    if (!CheckLength(cx, length))
        return false;
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return false;
    if (options.filename && !ss->setFilename(cx, options.filename))
        return false;
    JS::RootedScriptSource sourceObject(cx, ScriptSourceObject::create(cx, ss));
    if (!sourceObject)
        return false;
    SourceCompressionToken sct(cx);
    JS_ASSERT(options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    if (options.sourcePolicy == CompileOptions::SAVE_SOURCE) {
        if (!ss->setSourceCopy(cx, chars, length, true, &sct))
            return false;
    }

    Maybe<Parser<SyntaxParseHandler> > syntaxParser;
    if (options.canLazilyParse) {
        syntaxParser.construct(cx, options, chars, length,  false,
                               (Parser<SyntaxParseHandler> *) NULL,
                               (LazyScript *) NULL);
    }

    JS_ASSERT(!options.forEval);

    Parser<FullParseHandler> parser(cx, options, chars, length,  true,
                                    options.canLazilyParse ? &syntaxParser.ref() : NULL, NULL);
    parser.sct = &sct;

    JS_ASSERT(fun);

    fun->setArgCount(formals.length());

    
    ParseNode *fn = CodeNode::create(PNK_FUNCTION, &parser.handler);
    if (!fn)
        return false;

    fn->pn_body = NULL;
    fn->pn_funbox = NULL;
    fn->pn_cookie.makeFree();

    ParseNode *argsbody = ListNode::create(PNK_ARGSBODY, &parser.handler);
    if (!argsbody)
        return false;
    argsbody->setOp(JSOP_NOP);
    argsbody->makeEmpty();
    fn->pn_body = argsbody;

    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), false, options,
                                                   0, sourceObject,
                                                   0, length));
    if (!script)
        return false;

    
    
    
    TokenStream::Position start(parser.keepAtoms);
    parser.tokenStream.tell(&start);
    bool strict = StrictModeFromContext(cx);
    bool becameStrict;
    FunctionBox *funbox;
    ParseNode *pn;
    while (true) {
        pn = parser.standaloneFunctionBody(fun, formals, script, fn, &funbox,
                                           strict, &becameStrict);
        if (pn)
            break;

        if (parser.hadAbortedSyntaxParse()) {
            
            
            
            parser.clearAbortedSyntaxParse();
        } else {
            
            if (strict || !becameStrict || parser.tokenStream.hadError())
                return false;
            strict = true;
        }

        parser.tokenStream.seek(start);
    }

    if (!NameFunctions(cx, pn))
        return false;

    if (fn->pn_body) {
        JS_ASSERT(fn->pn_body->isKind(PNK_ARGSBODY));
        fn->pn_body->append(pn);
        fn->pn_body->pn_pos = pn->pn_pos;
        pn = fn->pn_body;
    }

    bool generateBytecode = true;
#ifdef JS_ION
    JS_ASSERT_IF(isAsmJSRecompile, fn->pn_funbox->useAsm);
    if (fn->pn_funbox->useAsm && !isAsmJSRecompile) {
        RootedFunction moduleFun(cx);
        if (!CompileAsmJS(cx, parser.tokenStream, fn, options,
                          ss,  0,  length,
                          &moduleFun))
            return false;

        if (moduleFun) {
            funbox->object = moduleFun;
            fun.set(moduleFun); 
            generateBytecode = false;
        }
    }
#endif

    if (generateBytecode) {
        






        BytecodeEmitter funbce( NULL, &parser, funbox, script,
                                false,  NullPtr(),
                               fun->environment() && fun->environment()->isGlobal(),
                               options.lineno);
        if (!funbce.init())
            return false;

        if (!EmitFunctionScript(cx, &funbce, pn))
            return false;
    }

    if (!SetSourceMap(cx, parser.tokenStream, ss, script))
        return false;

    if (!sct.complete())
        return false;

    return true;
}
