






#include "frontend/BytecodeCompiler.h"

#include "jsprobes.h"

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
SetSourceMap(JSContext *cx, TokenStream &tokenStream, ScriptSource *ss, RawScript script)
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

RawScript
frontend::CompileScript(JSContext *cx, HandleObject scopeChain,
                        HandleScript evalCaller,
                        const CompileOptions &options,
                        const jschar *chars, size_t length,
                        JSString *source_ ,
                        unsigned staticLevel ,
                        SourceCompressionToken *extraSct )
{
    RootedString source(cx, source_);

    class ProbesManager
    {
        const char* filename;
        unsigned lineno;

      public:
        ProbesManager(const char *f, unsigned l) : filename(f), lineno(l) {
            Probes::compileScriptBegin(filename, lineno);
        }
        ~ProbesManager() { Probes::compileScriptEnd(filename, lineno); }
    };
    ProbesManager probesManager(options.filename, options.lineno);

    



    JS_ASSERT_IF(evalCaller, options.compileAndGo);
    JS_ASSERT_IF(staticLevel != 0, evalCaller);

    if (!CheckLength(cx, length))
        return NULL;
    JS_ASSERT_IF(staticLevel != 0, options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return NULL;
    if (options.filename && !ss->setFilename(cx, options.filename))
        return NULL;
    ScriptSourceHolder ssh(ss);
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

    Parser<FullParseHandler> parser(cx, options, chars, length,  true);
    if (!parser.init())
        return NULL;
    parser.sct = sct;

    GlobalSharedContext globalsc(cx, scopeChain, StrictModeFromContext(cx));

    ParseContext<FullParseHandler> pc(&parser, &globalsc, staticLevel,  0);
    if (!pc.init())
        return NULL;

    bool savedCallerFun =
        options.compileAndGo &&
        evalCaller &&
        (evalCaller->function() || evalCaller->savedCallerFun);
    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), savedCallerFun,
                                                  options, staticLevel, ss, 0, length));
    if (!script)
        return NULL;

    
    
    InternalHandle<Bindings*> bindings(script, &script->bindings);
    if (!Bindings::initWithTemporaryStorage(cx, bindings, 0, 0, NULL))
        return NULL;

    
    JSObject *globalScope = scopeChain && scopeChain == &scopeChain->global() ? (JSObject*) scopeChain : NULL;
    JS_ASSERT_IF(globalScope, globalScope->isNative());
    JS_ASSERT_IF(globalScope, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalScope->getClass()));

    BytecodeEmitter bce( NULL, &parser, &globalsc, script, evalCaller, !!globalScope,
                        options.lineno, options.selfHostingMode);
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
            ObjectBox *funbox = parser.newFunctionBox(fun, &pc, fun->strict());
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

        ParseNode *pn = parser.statement();
        if (!pn)
            return NULL;

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
        for (AtomDefnRange r = pc.lexdeps->all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                if (!CheckArgumentsWithinEval(cx, parser, fun))
                    return NULL;
            }
        }
        for (AtomDefnListMap::Range r = pc.decls().all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                if (!CheckArgumentsWithinEval(cx, parser, fun))
                    return NULL;
            }
        }

        
        
        
        if (pc.sc->hasDebuggerStatement()) {
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
frontend::ParseScript(JSContext *cx, HandleObject scopeChain,
                      const CompileOptions &options, StableCharPtr chars, size_t length)
{
    if (!CheckLength(cx, length))
        return false;

    Parser<SyntaxParseHandler> parser(cx, options, chars.get(), length,  false);
    if (!parser.init()) {
        cx->clearPendingException();
        return false;
    }

    GlobalSharedContext globalsc(cx, scopeChain, StrictModeFromContext(cx));

    ParseContext<SyntaxParseHandler> pc(&parser, &globalsc, 0,  0);
    if (!pc.init()) {
        cx->clearPendingException();
        return false;
    }

    for (;;) {
        TokenKind tt = parser.tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            cx->clearPendingException();
            return false;
        }

        if (!parser.statement()) {
            cx->clearPendingException();
            return false;
        }
    }

    return true;
}



bool
frontend::CompileFunctionBody(JSContext *cx, HandleFunction fun, CompileOptions options,
                              const AutoNameVector &formals, const jschar *chars, size_t length)
{
    if (!CheckLength(cx, length))
        return false;
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return false;
    if (options.filename && !ss->setFilename(cx, options.filename))
        return false;
    ScriptSourceHolder ssh(ss);
    SourceCompressionToken sct(cx);
    JS_ASSERT(options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    if (options.sourcePolicy == CompileOptions::SAVE_SOURCE) {
        if (!ss->setSourceCopy(cx, chars, length, true, &sct))
            return false;
    }

    options.setCompileAndGo(false);
    Parser<FullParseHandler> parser(cx, options, chars, length,  true);
    if (!parser.init())
        return false;
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
                                                   0, ss,
                                                   0, length));
    if (!script)
        return false;

    
    
    
    TokenStream::Position start;
    parser.tokenStream.tell(&start);
    bool initiallyStrict = StrictModeFromContext(cx);
    bool becameStrict;
    FunctionBox *funbox;
    ParseNode *pn = parser.standaloneFunctionBody(fun, formals, script, fn, &funbox,
                                                  initiallyStrict, &becameStrict);
    if (!pn) {
        if (initiallyStrict || !becameStrict || parser.tokenStream.hadError())
            return false;

        
        parser.tokenStream.seek(start);
        pn = parser.standaloneFunctionBody(fun, formals, script, fn, &funbox,
                                            true);
        if (!pn)
            return false;
    }

    BytecodeEmitter funbce( NULL, &parser, funbox, script,
                            NullPtr(),
                            false, options.lineno);
    if (!funbce.init())
        return false;

    if (!NameFunctions(cx, pn))
        return false;

    if (fn->pn_body) {
        JS_ASSERT(fn->pn_body->isKind(PNK_ARGSBODY));
        fn->pn_body->append(pn);
        fn->pn_body->pn_pos = pn->pn_pos;
        pn = fn->pn_body;
    }

#ifdef JS_ION
    



    if (fn->pn_funbox->useAsm && !CompileAsmJS(cx, parser.tokenStream, fn, script))
        return false;
#endif

    if (!SetSourceMap(cx, parser.tokenStream, ss, script))
        return false;

    if (!EmitFunctionScript(cx, &funbce, pn))
        return false;

    if (!sct.complete())
        return false;

    return true;
}
