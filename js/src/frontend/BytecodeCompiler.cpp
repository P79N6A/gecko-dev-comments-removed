







































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
    Root<JSScript*> root(cx, &script);

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
frontend::CompileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
                        JSPrincipals *principals, JSPrincipals *originPrincipals,
                        uint32_t tcflags,
                        const jschar *chars, size_t length,
                        const char *filename, unsigned lineno, JSVersion version,
                        JSString *source ,
                        unsigned staticLevel )
{
    TokenKind tt;
    ParseNode *pn;
    bool inDirectivePrologue;

    JS_ASSERT(!(tcflags & ~(TCF_COMPILE_N_GO | TCF_NO_SCRIPT_RVAL | TCF_COMPILE_FOR_EVAL
                            | TCF_NEED_SCRIPT_GLOBAL)));

    



    JS_ASSERT_IF(callerFrame, tcflags & TCF_COMPILE_N_GO);
    JS_ASSERT_IF(staticLevel != 0, callerFrame);

    Parser parser(cx, principals, originPrincipals, callerFrame);
    if (!parser.init(chars, length, filename, lineno, version))
        return NULL;

    TokenStream &tokenStream = parser.tokenStream;

    BytecodeEmitter bce(&parser, tokenStream.getLineno());
    if (!bce.init(cx, TreeContext::USED_AS_TREE_CONTEXT))
        return NULL;

    Probes::compileScriptBegin(cx, filename, lineno);
    MUST_FLOW_THROUGH("out");

    
    JSObject *globalObj = scopeChain && scopeChain == &scopeChain->global()
                          ? &scopeChain->global()
                          : NULL;

    JS_ASSERT_IF(globalObj, globalObj->isNative());
    JS_ASSERT_IF(globalObj, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalObj->getClass()));

    RootedVar<JSScript*> script(cx);

    GlobalScope globalScope(cx, globalObj);
    bce.flags |= tcflags;
    bce.setScopeChain(scopeChain);
    bce.globalScope = &globalScope;
    if (!SetStaticLevel(&bce, staticLevel))
        goto out;

    
    if (callerFrame &&
        callerFrame->isScriptFrame() &&
        callerFrame->script()->strictModeCode) {
        bce.flags |= TCF_STRICT_MODE_CODE;
        tokenStream.setStrictMode();
    }

#ifdef DEBUG
    bool savedCallerFun;
    savedCallerFun = false;
#endif
    if (tcflags & TCF_COMPILE_N_GO) {
        if (source) {
            



            JSAtom *atom = js_AtomizeString(cx, source);
            jsatomid _;
            if (!atom || !bce.makeAtomIndex(atom, &_))
                goto out;
        }

        if (callerFrame && callerFrame->isFunctionFrame()) {
            




            ObjectBox *funbox = parser.newObjectBox(callerFrame->fun());
            if (!funbox)
                goto out;
            funbox->emitLink = bce.objectList.lastbox;
            bce.objectList.lastbox = funbox;
            bce.objectList.length++;
#ifdef DEBUG
            savedCallerFun = true;
#endif
        }
    }

    



    uint32_t bodyid;
    if (!GenerateBlockId(&bce, bodyid))
        goto out;
    bce.bodyid = bodyid;

#if JS_HAS_XML_SUPPORT
    pn = NULL;
    bool onlyXML;
    onlyXML = true;
#endif

    inDirectivePrologue = true;
    tokenStream.setOctalCharacterEscape(false);
    for (;;) {
        tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            goto out;
        }

        pn = parser.statement();
        if (!pn)
            goto out;
        JS_ASSERT(!bce.blockNode);

        if (inDirectivePrologue && !parser.recognizeDirectivePrologue(pn, &inDirectivePrologue))
            goto out;

        if (!FoldConstants(cx, pn, bce.parser))
            goto out;

        if (!AnalyzeFunctions(bce.parser))
            goto out;
        bce.functionList = NULL;

        if (!EmitTree(cx, &bce, pn))
            goto out;

#if JS_HAS_XML_SUPPORT
        if (!pn->isKind(PNK_SEMI) || !pn->pn_kid || !pn->pn_kid->isXMLItem())
            onlyXML = false;
#endif
        bce.parser->freeTree(pn);
    }

#if JS_HAS_XML_SUPPORT
    





    if (pn && onlyXML && !callerFrame) {
        parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_XML_WHOLE_PROGRAM);
        goto out;
    }
#endif

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        goto out;

    JS_ASSERT(bce.version() == version);

    script = JSScript::NewScriptFromEmitter(cx, &bce);
    if (!script)
        goto out;

    JS_ASSERT(script->savedCallerFun == savedCallerFun);

    if (!MarkInnerAndOuterFunctions(cx, script))
        script = NULL;

  out:
    Probes::compileScriptEnd(cx, script, filename, lineno);
    return script;
}





bool
frontend::CompileFunctionBody(JSContext *cx, JSFunction *fun,
                              JSPrincipals *principals, JSPrincipals *originPrincipals,
                              Bindings *bindings, const jschar *chars, size_t length,
                              const char *filename, unsigned lineno, JSVersion version)
{
    Parser parser(cx, principals, originPrincipals);
    if (!parser.init(chars, length, filename, lineno, version))
        return false;

    TokenStream &tokenStream = parser.tokenStream;

    BytecodeEmitter funbce(&parser, tokenStream.getLineno());
    if (!funbce.init(cx, TreeContext::USED_AS_TREE_CONTEXT))
        return false;

    funbce.flags |= TCF_IN_FUNCTION;
    funbce.setFunction(fun);
    funbce.bindings.transfer(cx, bindings);
    fun->setArgCount(funbce.bindings.numArgs());
    if (!GenerateBlockId(&funbce, funbce.bodyid))
        return false;

    
    ParseNode *fn = FunctionNode::create(PNK_NAME, funbce.parser);
    if (fn) {
        fn->pn_body = NULL;
        fn->pn_cookie.makeFree();

        unsigned nargs = fun->nargs;
        if (nargs) {
            



            BindingNames names(cx);
            if (!funbce.bindings.getLocalNameArray(cx, &names)) {
                fn = NULL;
            } else {
                for (unsigned i = 0; i < nargs; i++) {
                    if (!DefineArg(fn, names[i].maybeAtom, i, funbce.parser)) {
                        fn = NULL;
                        break;
                    }
                }
            }
        }
    }

    




    ParseNode *pn = fn ? parser.functionBody(Parser::StatementListBody) : NULL;
    if (pn) {
        if (!tokenStream.matchToken(TOK_EOF)) {
            parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (!FoldConstants(cx, pn, funbce.parser)) {
            
            pn = NULL;
        } else if (!AnalyzeFunctions(funbce.parser)) {
            pn = NULL;
        } else {
            if (fn->pn_body) {
                JS_ASSERT(fn->pn_body->isKind(PNK_ARGSBODY));
                fn->pn_body->append(pn);
                fn->pn_body->pn_pos = pn->pn_pos;
                pn = fn->pn_body;
            }

            if (!EmitFunctionScript(cx, &funbce, pn))
                pn = NULL;
        }
    }

    return pn != NULL;
}
