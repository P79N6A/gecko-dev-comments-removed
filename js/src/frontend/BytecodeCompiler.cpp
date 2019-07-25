







































#include "frontend/BytecodeCompiler.h"

#include "jsprobes.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/FoldConstants.h"
#include "vm/GlobalObject.h"

#include "jsinferinlines.h"

using namespace js;
using namespace js::frontend;




BytecodeCompiler::BytecodeCompiler(JSContext *cx, JSPrincipals *prin, StackFrame *cfp)
  : parser(cx, prin, cfp)
{}

JSScript *
BytecodeCompiler::compileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
                                JSPrincipals *principals, uint32 tcflags,
                                const jschar *chars, size_t length,
                                const char *filename, uintN lineno, JSVersion version,
                                JSString *source ,
                                uintN staticLevel )
{
    TokenKind tt;
    ParseNode *pn;
    JSScript *script;
    bool inDirectivePrologue;

    JS_ASSERT(!(tcflags & ~(TCF_COMPILE_N_GO | TCF_NO_SCRIPT_RVAL | TCF_NEED_MUTABLE_SCRIPT |
                            TCF_COMPILE_FOR_EVAL | TCF_NEED_SCRIPT_GLOBAL)));

    



    JS_ASSERT_IF(callerFrame, tcflags & TCF_COMPILE_N_GO);
    JS_ASSERT_IF(staticLevel != 0, callerFrame);

    BytecodeCompiler compiler(cx, principals, callerFrame);
    if (!compiler.init(chars, length, filename, lineno, version))
        return NULL;

    Parser &parser = compiler.parser;
    TokenStream &tokenStream = parser.tokenStream;

    BytecodeEmitter bce(&parser, tokenStream.getLineno());
    if (!bce.init(cx, TreeContext::USED_AS_TREE_CONTEXT))
        return NULL;

    Probes::compileScriptBegin(cx, filename, lineno);

    MUST_FLOW_THROUGH("out");

    
    JSObject *globalObj = scopeChain && scopeChain == scopeChain->getGlobal()
                        ? scopeChain->getGlobal()
                        : NULL;

    JS_ASSERT_IF(globalObj, globalObj->isNative());
    JS_ASSERT_IF(globalObj, JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(globalObj->getClass()));

    
    script = NULL;

    GlobalScope globalScope(cx, globalObj, &bce);
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

    



    uint32 bodyid;
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

        if (!FoldConstants(cx, pn, &bce))
            goto out;

        if (!parser.analyzeFunctions(&bce))
            goto out;
        bce.functionList = NULL;

        if (!EmitTree(cx, &bce, pn))
            goto out;

#if JS_HAS_XML_SUPPORT
        if (!pn->isKind(TOK_SEMI) || !pn->pn_kid || !TreeTypeIsXML(pn->pn_kid->getKind()))
            onlyXML = false;
#endif
        bce.freeTree(pn);
    }

#if JS_HAS_XML_SUPPORT
    





    if (pn && onlyXML && !callerFrame) {
        parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_XML_WHOLE_PROGRAM);
        goto out;
    }
#endif

    




    if (bce.hasSharps()) {
        jsbytecode *code, *end;
        JSOp op;
        const JSCodeSpec *cs;
        uintN len, slot;

        code = bce.base();
        for (end = code + bce.offset(); code != end; code += len) {
            JS_ASSERT(code < end);
            op = (JSOp) *code;
            cs = &js_CodeSpec[op];
            len = (cs->length > 0)
                  ? (uintN) cs->length
                  : js_GetVariableBytecodeLength(code);
            if ((cs->format & JOF_SHARPSLOT) ||
                JOF_TYPE(cs->format) == JOF_LOCAL ||
                (JOF_TYPE(cs->format) == JOF_SLOTATOM)) {
                JS_ASSERT_IF(!(cs->format & JOF_SHARPSLOT),
                             JOF_TYPE(cs->format) != JOF_SLOTATOM);
                slot = GET_SLOTNO(code);
                if (!(cs->format & JOF_SHARPSLOT))
                    slot += bce.sharpSlots();
                if (slot >= SLOTNO_LIMIT)
                    goto too_many_slots;
                SET_SLOTNO(code, slot);
            }
        }
    }

    



    if (Emit1(cx, &bce, JSOP_STOP) < 0)
        goto out;

    JS_ASSERT(bce.version() == version);

    script = JSScript::NewScriptFromEmitter(cx, &bce);
    if (!script)
        goto out;

    JS_ASSERT(script->savedCallerFun == savedCallerFun);

    if (!defineGlobals(cx, globalScope, script))
        script = NULL;

  out:
    Probes::compileScriptEnd(cx, script, filename, lineno);
    return script;

  too_many_slots:
    parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_LOCALS);
    script = NULL;
    goto out;
}

bool
BytecodeCompiler::defineGlobals(JSContext *cx, GlobalScope &globalScope, JSScript *script)
{
    JSObject *globalObj = globalScope.globalObj;

    
    for (size_t i = 0; i < globalScope.defs.length(); i++) {
        GlobalScope::GlobalDef &def = globalScope.defs[i];

        
        if (!def.atom)
            continue;

        jsid id = ATOM_TO_JSID(def.atom);
        Value rval;

        if (def.funbox) {
            JSFunction *fun = def.funbox->function();

            



            rval.setObject(*fun);
            types::AddTypePropertyId(cx, globalObj, id, rval);
        } else {
            rval.setUndefined();
        }

        






        const Shape *shape =
            DefineNativeProperty(cx, globalObj, id, rval, JS_PropertyStub, JS_StrictPropertyStub,
                                 JSPROP_ENUMERATE | JSPROP_PERMANENT, 0, 0, DNP_SKIP_TYPE);
        if (!shape)
            return false;
        def.knownSlot = shape->slot;
    }

    Vector<JSScript *, 16> worklist(cx);
    if (!worklist.append(script))
        return false;

    





    while (worklist.length()) {
        JSScript *outer = worklist.back();
        worklist.popBack();

        if (JSScript::isValidOffset(outer->objectsOffset)) {
            JSObjectArray *arr = outer->objects();

            



            size_t start = outer->savedCallerFun ? 1 : 0;

            for (size_t i = start; i < arr->length; i++) {
                JSObject *obj = arr->vector[i];
                if (!obj->isFunction())
                    continue;
                JSFunction *fun = obj->getFunctionPrivate();
                JS_ASSERT(fun->isInterpreted());
                JSScript *inner = fun->script();
                if (outer->isHeavyweightFunction) {
                    outer->isOuterFunction = true;
                    inner->isInnerFunction = true;
                }
                if (!JSScript::isValidOffset(inner->globalsOffset) &&
                    !JSScript::isValidOffset(inner->objectsOffset)) {
                    continue;
                }
                if (!worklist.append(inner))
                    return false;
            }
        }

        if (!JSScript::isValidOffset(outer->globalsOffset))
            continue;

        GlobalSlotArray *globalUses = outer->globals();
        uint32 nGlobalUses = globalUses->length;
        for (uint32 i = 0; i < nGlobalUses; i++) {
            uint32 index = globalUses->vector[i].slot;
            JS_ASSERT(index < globalScope.defs.length());
            globalUses->vector[i].slot = globalScope.defs[index].knownSlot;
        }
    }

    return true;
}





bool
BytecodeCompiler::compileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                                      Bindings *bindings, const jschar *chars, size_t length,
                                      const char *filename, uintN lineno, JSVersion version)
{
    BytecodeCompiler compiler(cx, principals);

    if (!compiler.init(chars, length, filename, lineno, version))
        return false;

    Parser &parser = compiler.parser;
    TokenStream &tokenStream = parser.tokenStream;

    BytecodeEmitter funbce(&parser, tokenStream.getLineno());
    if (!funbce.init(cx, TreeContext::USED_AS_TREE_CONTEXT))
        return false;

    funbce.flags |= TCF_IN_FUNCTION;
    funbce.setFunction(fun);
    funbce.bindings.transfer(cx, bindings);
    fun->setArgCount(funbce.bindings.countArgs());
    if (!GenerateBlockId(&funbce, funbce.bodyid))
        return false;

    
    tokenStream.mungeCurrentToken(TOK_NAME);
    ParseNode *fn = FunctionNode::create(&funbce);
    if (fn) {
        fn->pn_body = NULL;
        fn->pn_cookie.makeFree();

        uintN nargs = fun->nargs;
        if (nargs) {
            



            Vector<JSAtom *> names(cx);
            if (!funbce.bindings.getLocalNameArray(cx, &names)) {
                fn = NULL;
            } else {
                for (uintN i = 0; i < nargs; i++) {
                    if (!DefineArg(fn, names[i], i, &funbce)) {
                        fn = NULL;
                        break;
                    }
                }
            }
        }
    }

    






    tokenStream.mungeCurrentToken(TOK_LC);
    ParseNode *pn = fn ? parser.functionBody() : NULL;
    if (pn) {
        if (!CheckStrictParameters(cx, &funbce)) {
            pn = NULL;
        } else if (!tokenStream.matchToken(TOK_EOF)) {
            parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (!FoldConstants(cx, pn, &funbce)) {
            
            pn = NULL;
        } else if (!parser.analyzeFunctions(&funbce)) {
            pn = NULL;
        } else {
            if (fn->pn_body) {
                JS_ASSERT(fn->pn_body->isKind(TOK_ARGSBODY));
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
