






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
SetSourceMap(JSContext *cx, TokenStream &tokenStream, ScriptSource *ss, JSScript *script)
{
    if (tokenStream.hasSourceMap()) {
        if (!ss->setSourceMap(cx, tokenStream.releaseSourceMap(), script->filename))
            return false;
    }
    return true;
}

JSScript *
frontend::CompileScript(JSContext *cx, HandleObject scopeChain, StackFrame *callerFrame,
                        const CompileOptions &options,
                        const jschar *chars, size_t length,
                        JSString *source_ ,
                        unsigned staticLevel )
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
        return NULL;
    JS_ASSERT_IF(staticLevel != 0, options.sourcePolicy != CompileOptions::LAZY_SOURCE);
    ScriptSource *ss = cx->new_<ScriptSource>();
    if (!ss)
        return NULL;
    ScriptSourceHolder ssh(cx->runtime, ss);
    SourceCompressionToken sct(cx);
    switch (options.sourcePolicy) {
      case CompileOptions::SAVE_SOURCE:
        if (!ss->setSourceCopy(cx, chars, length, false, &sct))
            return NULL;
        break;
      case CompileOptions::LAZY_SOURCE:
        ss->setSourceRetrievable();
        break;
      case CompileOptions::NO_SOURCE:
        break;
    }

    Parser parser(cx, options, chars, length,  true);
    if (!parser.init())
        return NULL;
    parser.sct = &sct;

    SharedContext sc(cx, scopeChain,  NULL,  NULL, StrictModeFromContext(cx));

    ParseContext pc(&parser, &sc, staticLevel,  0);
    if (!pc.init())
        return NULL;

    bool savedCallerFun = options.compileAndGo && callerFrame && callerFrame->isFunctionFrame();
    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), savedCallerFun,
                                                  options, staticLevel, ss, 0, length));
    if (!script)
        return NULL;

    
    
    if (!script->bindings.initWithTemporaryStorage(cx, 0, 0, NULL))
        return NULL;

    
    JSObject *globalScope = scopeChain && scopeChain == &scopeChain->global() ? (JSObject*) scopeChain : NULL;
    JS_ASSERT_IF(globalScope, globalScope->isNative());
    JS_ASSERT_IF(globalScope, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalScope->getClass()));

    BytecodeEmitter bce( NULL, &parser, &sc, script, callerFrame, !!globalScope,
                        options.lineno, options.selfHostingMode);
    if (!bce.init())
        return NULL;

    
    if (callerFrame && callerFrame->isScriptFrame() && callerFrame->script()->strictModeCode)
        sc.strictModeState = StrictMode::STRICT;

    if (options.compileAndGo) {
        if (source) {
            



            JSAtom *atom = AtomizeString(cx, source);
            jsatomid _;
            if (!atom || !bce.makeAtomIndex(atom, &_))
                return NULL;
        }

        if (callerFrame && callerFrame->isFunctionFrame()) {
            




            ObjectBox *funbox = parser.newObjectBox(callerFrame->fun());
            if (!funbox)
                return NULL;
            funbox->emitLink = bce.objectList.lastbox;
            bce.objectList.lastbox = funbox;
            bce.objectList.length++;
        }
    }

    ParseNode *pn;
#if JS_HAS_XML_SUPPORT
    pn = NULL;
    bool onlyXML;
    onlyXML = true;
#endif

    TokenStream &tokenStream = parser.tokenStream;
    {
        ParseNode *stringsAtStart = ListNode::create(PNK_STATEMENTLIST, &parser);
        if (!stringsAtStart)
            return NULL;
        stringsAtStart->makeEmpty();
        bool ok = parser.processDirectives(stringsAtStart) && EmitTree(cx, &bce, stringsAtStart);
        parser.freeTree(stringsAtStart);
        if (!ok)
            return NULL;
    }
    JS_ASSERT(sc.strictModeState != StrictMode::UNKNOWN);
    for (;;) {
        TokenKind tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            return NULL;
        }

        pn = parser.statement();
        if (!pn)
            return NULL;

        if (!FoldConstants(cx, pn, &parser))
            return NULL;
        if (!NameFunctions(cx, pn))
            return NULL;

        pc.functionList = NULL;

        if (!EmitTree(cx, &bce, pn))
            return NULL;

#if JS_HAS_XML_SUPPORT
        if (!pn->isKind(PNK_SEMI) || !pn->pn_kid || !pn->pn_kid->isXMLItem())
            onlyXML = false;
#endif
        parser.freeTree(pn);
    }

    if (!SetSourceMap(cx, tokenStream, ss, script))
        return NULL;

#if JS_HAS_XML_SUPPORT
    





    if (pn && onlyXML && !callerFrame) {
        parser.reportError(NULL, JSMSG_XML_WHOLE_PROGRAM);
        return NULL;
    }
#endif

    
    if (callerFrame && callerFrame->isFunctionFrame() && callerFrame->fun()->hasRest()) {
        PropertyName *arguments = cx->runtime->atomState.argumentsAtom;
        for (AtomDefnRange r = pc.lexdeps->all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                parser.reportError(NULL, JSMSG_ARGUMENTS_AND_REST);
                return NULL;
            }
        }
    }

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        return NULL;

    if (!JSScript::fullyInitFromEmitter(cx, script, &bce))
        return NULL;

    bce.tellDebuggerAboutCompiledScript(cx);

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
    ScriptSourceHolder ssh(cx->runtime, ss);
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
    SharedContext funsc(cx,  NULL, fun,  NULL,
                        StrictModeFromContext(cx));
    fun->setArgCount(formals.length());

    unsigned staticLevel = 0;
    ParseContext funpc(&parser, &funsc, staticLevel,  0);
    if (!funpc.init())
        return false;

    
    ParseNode *fn = FunctionNode::create(PNK_NAME, &parser);
    if (!fn)
        return false;

    fn->pn_body = NULL;
    fn->pn_cookie.makeFree();

    ParseNode *argsbody = ListNode::create(PNK_ARGSBODY, &parser);
    if (!argsbody)
        return false;
    argsbody->setOp(JSOP_NOP);
    argsbody->makeEmpty();
    fn->pn_body = argsbody;

    for (unsigned i = 0; i < formals.length(); i++) {
        if (!DefineArg(&parser, fn, formals[i]))
            return false;
    }

    




    ParseNode *pn = parser.functionBody(Parser::StatementListBody);
    if (!pn) 
        return false;

    if (!parser.tokenStream.matchToken(TOK_EOF)) {
        parser.reportError(NULL, JSMSG_SYNTAX_ERROR);
        return false;
    }

    if (!FoldConstants(cx, pn, &parser))
        return false;

    Rooted<JSScript*> script(cx, JSScript::Create(cx, NullPtr(), false, options,
                                                  staticLevel, ss, 0, length));
    if (!script)
        return false;

    if (!funpc.generateFunctionBindings(cx, &script->bindings))
        return false;

    BytecodeEmitter funbce( NULL, &parser, &funsc, script,  NULL,
                            false, options.lineno);
    if (!funbce.init())
        return false;

    if (!NameFunctions(cx, pn))
        return NULL;

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

    return true;
}
