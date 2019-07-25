






#include "frontend/BytecodeCompiler.h"

#include "jsprobes.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "frontend/SemanticAnalysis.h"
#include "vm/GlobalObject.h"

#include "jsinferinlines.h"

#include "frontend/TreeContext-inl.h"

using namespace js;
using namespace js::frontend;

bool
MarkInnerAndOuterFunctions(JSContext *cx, JSScript* script)
{
    AssertRootingUnnecessary safe(cx);

    Vector<JSScript *, 16> worklist(cx);
    if (!worklist.append(script))
        return false;

    while (worklist.length()) {
        JSScript *outer = worklist.back();
        worklist.popBack();

        if (outer->hasObjects()) {
            ObjectArray *arr = outer->objects();

            



            size_t start = outer->savedCallerFun ? 1 : 0;

            for (size_t i = start; i < arr->length; i++) {
                JSObject *obj = arr->vector[i];
                if (!obj->isFunction())
                    continue;
                JSFunction *fun = obj->toFunction();
                JS_ASSERT(fun->isInterpreted());
                JSScript *inner = fun->script();
                if (outer->function() && outer->function()->isHeavyweight()) {
                    outer->isOuterFunction = true;
                    inner->isInnerFunction = true;
                }
                if (!inner->hasObjects())
                    continue;
                if (!worklist.append(inner))
                    return false;
            }
        }
    }

    return true;
}

JSScript *
frontend::CompileScript(JSContext *cx, HandleObject scopeChain, StackFrame *callerFrame,
                        JSPrincipals *principals, JSPrincipals *originPrincipals,
                        bool compileAndGo, bool noScriptRval, bool needScriptGlobal,
                        const jschar *chars, size_t length,
                        const char *filename, unsigned lineno, JSVersion version,
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
    ProbesManager probesManager(filename, lineno);

    



    JS_ASSERT_IF(callerFrame, compileAndGo);
    JS_ASSERT_IF(staticLevel != 0, callerFrame);

    Parser parser(cx, principals, originPrincipals, chars, length, filename, lineno, version,
                   true, compileAndGo);
    if (!parser.init())
        return NULL;

    SharedContext sc(cx, scopeChain,  NULL,  NULL, StrictModeFromContext(cx));

    TreeContext tc(&parser, &sc, staticLevel,  0);
    if (!tc.init())
        return NULL;

    bool savedCallerFun = compileAndGo && callerFrame && callerFrame->isFunctionFrame();
    GlobalObject *globalObject = needScriptGlobal ? GetCurrentGlobal(cx) : NULL;
    Rooted<JSScript*> script(cx, JSScript::Create(cx,
                                                  savedCallerFun,
                                                  principals,
                                                  originPrincipals,
                                                  compileAndGo,
                                                  noScriptRval,
                                                  globalObject,
                                                  version,
                                                  staticLevel));
    if (!script)
        return NULL;

    
    JSObject *globalScope = scopeChain && scopeChain == &scopeChain->global() ? (JSObject*) scopeChain : NULL;
    JS_ASSERT_IF(globalScope, globalScope->isNative());
    JS_ASSERT_IF(globalScope, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalScope->getClass()));

    BytecodeEmitter bce( NULL, &parser, &sc, script, callerFrame, !!globalScope,
                        lineno);
    if (!bce.init())
        return NULL;

    
    if (callerFrame && callerFrame->isScriptFrame() && callerFrame->script()->strictModeCode)
        sc.strictModeState = StrictMode::STRICT;

    if (compileAndGo) {
        if (source) {
            



            JSAtom *atom = js_AtomizeString(cx, source);
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

        if (!AnalyzeFunctions(&parser, callerFrame))
            return NULL;
        tc.functionList = NULL;

        if (!EmitTree(cx, &bce, pn))
            return NULL;

#if JS_HAS_XML_SUPPORT
        if (!pn->isKind(PNK_SEMI) || !pn->pn_kid || !pn->pn_kid->isXMLItem())
            onlyXML = false;
#endif
        parser.freeTree(pn);
    }

#if JS_HAS_XML_SUPPORT
    





    if (pn && onlyXML && !callerFrame) {
        parser.reportError(NULL, JSMSG_XML_WHOLE_PROGRAM);
        return NULL;
    }
#endif

    
    if (callerFrame && callerFrame->isFunctionFrame() && callerFrame->fun()->hasRest()) {
        PropertyName *arguments = cx->runtime->atomState.argumentsAtom;
        for (AtomDefnRange r = tc.lexdeps->all(); !r.empty(); r.popFront()) {
            if (r.front().key() == arguments) {
                parser.reportError(NULL, JSMSG_ARGUMENTS_AND_REST);
                return NULL;
            }
        }
        
        JS_ASSERT(sc.bindings.lookup(cx, arguments, NULL) == NONE);
    }

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        return NULL;

    if (!JSScript::fullyInitFromEmitter(cx, script, &bce))
        return NULL;

    bce.tellDebuggerAboutCompiledScript(cx);

    if (!MarkInnerAndOuterFunctions(cx, script))
        return NULL;

    return script;
}



bool
frontend::CompileFunctionBody(JSContext *cx, HandleFunction fun,
                              JSPrincipals *principals, JSPrincipals *originPrincipals,
                              Bindings *bindings, const jschar *chars, size_t length,
                              const char *filename, unsigned lineno, JSVersion version)
{
    Parser parser(cx, principals, originPrincipals, chars, length, filename, lineno, version,
                   true,  false);
    if (!parser.init())
        return false;

    JS_ASSERT(fun);
    SharedContext funsc(cx,  NULL, fun,  NULL,
                        StrictModeFromContext(cx));
    funsc.bindings.transfer(bindings);
    fun->setArgCount(funsc.bindings.numArgs());

    unsigned staticLevel = 0;
    TreeContext funtc(&parser, &funsc, staticLevel,  0);
    if (!funtc.init())
        return false;

    GlobalObject *globalObject = fun->getParent() ? &fun->getParent()->global() : NULL;
    Rooted<JSScript*> script(cx, JSScript::Create(cx,
                                                   false,
                                                  principals,
                                                  originPrincipals,
                                                   false,
                                                   false,
                                                  globalObject,
                                                  version,
                                                  staticLevel));
    if (!script)
        return false;

    StackFrame *nullCallerFrame = NULL;
    BytecodeEmitter funbce( NULL, &parser, &funsc, script, nullCallerFrame,
                            false, lineno);
    if (!funbce.init())
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

    unsigned nargs = fun->nargs;
    if (nargs) {
        



        BindingNames names(cx);
        if (!funsc.bindings.getLocalNameArray(cx, &names))
            return false;

        for (unsigned i = 0; i < nargs; i++) {
            if (!DefineArg(fn, names[i].maybeAtom, i, &parser))
                return false;
        }
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

    if (!AnalyzeFunctions(&parser, nullCallerFrame))
        return false;

    if (fn->pn_body) {
        JS_ASSERT(fn->pn_body->isKind(PNK_ARGSBODY));
        fn->pn_body->append(pn);
        fn->pn_body->pn_pos = pn->pn_pos;
        pn = fn->pn_body;
    }

    if (!EmitFunctionScript(cx, &funbce, pn))
        return false;

    return true;
}
