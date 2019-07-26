






#include "frontend/BytecodeCompiler.h"

#include "jsprobes.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "frontend/NameFunctions.h"
#include "vm/GlobalObject.h"

#include "jsinferinlines.h"

#include "frontend/ParseMaps-inl.h"
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
SetSourceMap(JSContext *cx, TokenStream &tokenStream, ScriptSource *ss, UnrootedScript script)
{
    if (tokenStream.hasSourceMap()) {
        if (!ss->setSourceMap(cx, tokenStream.releaseSourceMap(), script->filename))
            return false;
    }
    return true;
}

UnrootedScript
frontend::CompileScript(JSContext *cx, HandleObject scopeChain, AbstractFramePtr callerFrame,
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

    



    JS_ASSERT_IF(callerFrame, options.compileAndGo);
    JS_ASSERT_IF(staticLevel != 0, callerFrame);

    if (!CheckLength(cx, length))
        return UnrootedScript(NULL);
    JS_ASSERT_IF(staticLevel != 0, options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return UnrootedScript(NULL);
    ScriptSourceHolder ssh(ss);
    SourceCompressionToken mysct(cx);
    SourceCompressionToken *sct = (extraSct) ? extraSct : &mysct;
    switch (options.sourcePolicy) {
      case CompileOptions::SAVE_SOURCE:
        if (!ss->setSourceCopy(cx, chars, length, false, sct))
            return UnrootedScript(NULL);
        break;
      case CompileOptions::LAZY_SOURCE:
        ss->setSourceRetrievable();
        break;
      case CompileOptions::NO_SOURCE:
        break;
    }

    Parser parser(cx, options, chars, length,  true);
    if (!parser.init())
        return UnrootedScript(NULL);
    parser.sct = sct;

    GlobalSharedContext globalsc(cx, scopeChain, StrictModeFromContext(cx));

    ParseContext pc(&parser, &globalsc, staticLevel,  0);
    if (!pc.init())
        return UnrootedScript(NULL);

    bool savedCallerFun = options.compileAndGo && callerFrame && callerFrame.isFunctionFrame();
    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), savedCallerFun,
                                                  options, staticLevel, ss, 0, length));
    if (!script)
        return UnrootedScript(NULL);

    
    
    InternalHandle<Bindings*> bindings(script, &script->bindings);
    if (!Bindings::initWithTemporaryStorage(cx, bindings, 0, 0, NULL))
        return UnrootedScript(NULL);

    
    JSObject *globalScope = scopeChain && scopeChain == &scopeChain->global() ? (JSObject*) scopeChain : NULL;
    JS_ASSERT_IF(globalScope, globalScope->isNative());
    JS_ASSERT_IF(globalScope, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalScope->getClass()));

    BytecodeEmitter bce( NULL, &parser, &globalsc, script, callerFrame, !!globalScope,
                        options.lineno, options.selfHostingMode);
    if (!bce.init())
        return UnrootedScript(NULL);

    
    if (callerFrame && callerFrame.script()->strict)
        globalsc.strict = true;

    if (options.compileAndGo) {
        if (source) {
            



            JSAtom *atom = AtomizeString<CanGC>(cx, source);
            jsatomid _;
            if (!atom || !bce.makeAtomIndex(atom, &_))
                return UnrootedScript(NULL);
        }

        if (callerFrame && callerFrame.isFunctionFrame()) {
            




            JSFunction *fun = callerFrame.fun();
            ObjectBox *funbox = parser.newFunctionBox(fun, &pc, fun->strict());
            if (!funbox)
                return UnrootedScript(NULL);
            bce.objectList.add(funbox);
        }
    }

    TokenStream &tokenStream = parser.tokenStream;
    bool canHaveDirectives = true;
    for (;;) {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            return UnrootedScript(NULL);
        }

        ParseNode *pn = parser.statement();
        if (!pn)
            return UnrootedScript(NULL);

        if (canHaveDirectives) {
            if (!parser.maybeParseDirective(pn, &canHaveDirectives))
                return UnrootedScript(NULL);
        }

        if (!FoldConstants(cx, &pn, &parser))
            return UnrootedScript(NULL);
        if (!NameFunctions(cx, pn))
            return UnrootedScript(NULL);

        if (!EmitTree(cx, &bce, pn))
            return UnrootedScript(NULL);

        parser.freeTree(pn);
    }

    if (!SetSourceMap(cx, tokenStream, ss, script))
        return UnrootedScript(NULL);

    
    if (callerFrame && callerFrame.isFunctionFrame() && callerFrame.fun()->hasRest()) {
        HandlePropertyName arguments = cx->names().arguments;
        for (AtomDefnRange r = pc.lexdeps->all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                parser.reportError(NULL, JSMSG_ARGUMENTS_AND_REST);
                return UnrootedScript(NULL);
            }
        }
    }

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        return UnrootedScript(NULL);

    if (!JSScript::fullyInitFromEmitter(cx, script, &bce))
        return UnrootedScript(NULL);

    bce.tellDebuggerAboutCompiledScript(cx);

    if (sct == &mysct && !sct->complete())
        return UnrootedScript(NULL);

    return script;
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
    ScriptSourceHolder ssh(ss);
    SourceCompressionToken sct(cx);
    JS_ASSERT(options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    if (options.sourcePolicy == CompileOptions::SAVE_SOURCE) {
        if (!ss->setSourceCopy(cx, chars, length, true, &sct))
            return false;
    }

    options.setCompileAndGo(false);
    Parser parser(cx, options, chars, length,  true);
    if (!parser.init())
        return false;
    parser.sct = &sct;

    JS_ASSERT(fun);

    fun->setArgCount(formals.length());

    
    ParseNode *fn = FunctionNode::create(PNK_FUNCTION, &parser);
    if (!fn)
        return false;

    fn->pn_body = NULL;
    fn->pn_funbox = NULL;
    fn->pn_cookie.makeFree();

    ParseNode *argsbody = ListNode::create(PNK_ARGSBODY, &parser);
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
                            NullFramePtr(),
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

    if (!SetSourceMap(cx, parser.tokenStream, ss, script))
        return false;

    if (!EmitFunctionScript(cx, &funbce, pn))
        return false;

    if (!sct.complete())
        return false;

    return true;
}
