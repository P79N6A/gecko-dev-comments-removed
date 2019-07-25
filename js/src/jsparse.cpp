




















































#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsstaticcheck.h"
#include "jslibmath.h"
#include "jsvector.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

#if JS_HAS_DESTRUCTURING
#include "jsdhash.h"
#endif

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsregexpinlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"


#ifdef CONST
#undef CONST
#endif

using namespace js;
using namespace js::gc;




#define pn_offsetof(m)  offsetof(JSParseNode, m)

JS_STATIC_ASSERT(pn_offsetof(pn_link) == pn_offsetof(dn_uses));
JS_STATIC_ASSERT(pn_offsetof(pn_u.name.atom) == pn_offsetof(pn_u.apair.atom));

#undef pn_offsetof





#define MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, __flags)                                     \
    JS_BEGIN_MACRO                                                                          \
        if (tokenStream.getToken((__flags)) != tt) {                                        \
            reportErrorNumber(NULL, JSREPORT_ERROR, errno);                                 \
            return NULL;                                                                    \
        }                                                                                   \
    JS_END_MACRO
#define MUST_MATCH_TOKEN(tt, errno) MUST_MATCH_TOKEN_WITH_FLAGS(tt, errno, 0)

void
JSParseNode::become(JSParseNode *pn2)
{
    JS_ASSERT(!pn_defn);
    JS_ASSERT(!pn2->pn_defn);

    JS_ASSERT(!pn_used);
    if (pn2->pn_used) {
        JSParseNode **pnup = &pn2->pn_lexdef->dn_uses;
        while (*pnup != pn2)
            pnup = &(*pnup)->pn_link;
        *pnup = this;
        pn_link = pn2->pn_link;
        pn_used = true;
        pn2->pn_link = NULL;
        pn2->pn_used = false;
    }

    pn_type = pn2->pn_type;
    pn_op = pn2->pn_op;
    pn_arity = pn2->pn_arity;
    pn_parens = pn2->pn_parens;
    pn_u = pn2->pn_u;

    



    if (PN_TYPE(this) == TOK_FUNCTION && pn_arity == PN_FUNC) {
        
        JS_ASSERT(pn_funbox->node == pn2);
        pn_funbox->node = this;
    } else if (pn_arity == PN_LIST && !pn_head) {
        
        JS_ASSERT(pn_count == 0);
        JS_ASSERT(pn_tail == &pn2->pn_head);
        pn_tail = &pn_head;
    }

    pn2->clear();
}

void
JSParseNode::clear()
{
    pn_type = TOK_EOF;
    pn_op = JSOP_NOP;
    pn_used = pn_defn = false;
    pn_arity = PN_NULLARY;
    pn_parens = false;
}

Parser::Parser(JSContext *cx, JSPrincipals *prin, StackFrame *cfp, bool foldConstants)
  : js::AutoGCRooter(cx, PARSER),
    context(cx),
    tokenStream(cx),
    principals(NULL),
    callerFrame(cfp),
    callerVarObj(cfp ? &cfp->varObj() : NULL),
    nodeList(NULL),
    functionCount(0),
    traceListHead(NULL),
    tc(NULL),
    emptyCallShape(NULL),
    keepAtoms(cx->runtime),
    foldConstants(foldConstants)
{
    cx->activeCompilations++;
    js::PodArrayZero(tempFreeList);
    setPrincipals(prin);
    JS_ASSERT_IF(cfp, cfp->isScriptFrame());
}

bool
Parser::init(const jschar *base, size_t length, const char *filename, uintN lineno,
             JSVersion version)
{
    JSContext *cx = context;
    if (!cx->ensureParseMapPool())
        return false;
    emptyCallShape = EmptyShape::getEmptyCallShape(cx);
    if (!emptyCallShape)
        return false;
    tempPoolMark = JS_ARENA_MARK(&cx->tempPool);
    if (!tokenStream.init(base, length, filename, lineno, version)) {
        JS_ARENA_RELEASE(&cx->tempPool, tempPoolMark);
        return false;
    }
    return true;
}

Parser::~Parser()
{
    JSContext *cx = context;

    if (principals)
        JSPRINCIPALS_DROP(cx, principals);
    JS_ARENA_RELEASE(&cx->tempPool, tempPoolMark);
    cx->activeCompilations--;
}

void
Parser::setPrincipals(JSPrincipals *prin)
{
    JS_ASSERT(!principals);
    if (prin)
        JSPRINCIPALS_HOLD(context, prin);
    principals = prin;
}

JSObjectBox *
Parser::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj);

    





    JSObjectBox *objbox;
    JS_ARENA_ALLOCATE_TYPE(objbox, JSObjectBox, &context->tempPool);
    if (!objbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }
    objbox->traceLink = traceListHead;
    traceListHead = objbox;
    objbox->emitLink = NULL;
    objbox->object = obj;
    objbox->isFunctionBox = false;
    return objbox;
}

JSFunctionBox *
Parser::newFunctionBox(JSObject *obj, JSParseNode *fn, JSTreeContext *tc)
{
    JS_ASSERT(obj);
    JS_ASSERT(obj->isFunction());

    





    JSFunctionBox *funbox;
    JS_ARENA_ALLOCATE_TYPE(funbox, JSFunctionBox, &context->tempPool);
    if (!funbox) {
        js_ReportOutOfMemory(context);
        return NULL;
    }
    funbox->traceLink = traceListHead;
    traceListHead = funbox;
    funbox->emitLink = NULL;
    funbox->object = obj;
    funbox->isFunctionBox = true;
    funbox->node = fn;
    funbox->siblings = tc->functionList;
    tc->functionList = funbox;
    ++tc->parser->functionCount;
    funbox->kids = NULL;
    funbox->parent = tc->funbox;
    funbox->methods = NULL;
    new (&funbox->bindings) Bindings(context, emptyCallShape);
    funbox->queued = false;
    funbox->inLoop = false;
    for (JSStmtInfo *stmt = tc->topStmt; stmt; stmt = stmt->down) {
        if (STMT_IS_LOOP(stmt)) {
            funbox->inLoop = true;
            break;
        }
    }
    funbox->level = tc->staticLevel;
    funbox->tcflags = (TCF_IN_FUNCTION | (tc->flags & (TCF_COMPILE_N_GO | TCF_STRICT_MODE_CODE)));
    if (tc->innermostWith)
        funbox->tcflags |= TCF_IN_WITH;
    return funbox;
}

bool
JSFunctionBox::joinable() const
{
    return FUN_NULL_CLOSURE(function()) &&
           !(tcflags & (TCF_FUN_USES_ARGUMENTS | TCF_FUN_USES_OWN_NAME));
}

bool
JSFunctionBox::inAnyDynamicScope() const
{
    for (const JSFunctionBox *funbox = this; funbox; funbox = funbox->parent) {
        if (funbox->tcflags & (TCF_IN_WITH | TCF_FUN_CALLS_EVAL))
            return true;
    }
    return false;
}

bool
JSFunctionBox::scopeIsExtensible() const
{
    return tcflags & TCF_FUN_EXTENSIBLE_SCOPE;
}

bool
JSFunctionBox::shouldUnbrand(uintN methods, uintN slowMethods) const
{
    if (slowMethods != 0) {
        for (const JSFunctionBox *funbox = this; funbox; funbox = funbox->parent) {
            if (!(funbox->tcflags & TCF_FUN_MODULE_PATTERN))
                return true;
            if (funbox->inLoop)
                return true;
        }
    }
    return false;
}

void
Parser::trace(JSTracer *trc)
{
    JSObjectBox *objbox = traceListHead;
    while (objbox) {
        MarkObject(trc, *objbox->object, "parser.object");
        if (objbox->isFunctionBox)
            static_cast<JSFunctionBox *>(objbox)->bindings.trace(trc);
        objbox = objbox->traceLink;
    }

    if (emptyCallShape)
        MarkShape(trc, emptyCallShape, "emptyCallShape");

    for (JSTreeContext *tc = this->tc; tc; tc = tc->parent)
        tc->trace(trc);
}


static inline void
AddNodeToFreeList(JSParseNode *pn, js::Parser *parser)
{
    
    JS_ASSERT(pn != parser->nodeList);

    





    JS_ASSERT(!pn->pn_used);
    JS_ASSERT(!pn->pn_defn);

    if (pn->pn_arity == PN_NAMESET && pn->pn_names.hasMap())
        pn->pn_names.releaseMap(parser->context);

#ifdef DEBUG
    
    memset(pn, 0xab, sizeof(*pn));
#endif

    pn->pn_next = parser->nodeList;
    parser->nodeList = pn;
}


static inline void
AddNodeToFreeList(JSParseNode *pn, JSTreeContext *tc)
{
    AddNodeToFreeList(pn, tc->parser);
}






























void
Parser::cleanFunctionList(JSFunctionBox **funboxHead)
{
    JSFunctionBox **link = funboxHead;
    while (JSFunctionBox *box = *link) {
        if (!box->node) {
            



            *link = box->siblings;
        } else if (!box->node->pn_funbox) {
            



            *link = box->siblings;
            AddNodeToFreeList(box->node, this);
        } else {
            

            
            {
                JSParseNode **methodLink = &box->methods;
                while (JSParseNode *method = *methodLink) {
                    
                    JS_ASSERT(method->pn_arity == PN_FUNC);
                    if (!method->pn_funbox) {
                        
                        *methodLink = method->pn_link;
                    } else {
                        
                        methodLink = &method->pn_link;
                    }
                }
            }

            
            cleanFunctionList(&box->kids);

            
            link = &box->siblings;
        }
    }
}

namespace js {










class NodeStack {
  public:
    NodeStack() : top(NULL) { }
    bool empty() { return top == NULL; }
    void push(JSParseNode *pn) {
        pn->pn_next = top;
        top = pn;
    }
    void pushUnlessNull(JSParseNode *pn) { if (pn) push(pn); }
    
    void pushList(JSParseNode *pn) {
        
        *pn->pn_tail = top;
        top = pn->pn_head;
    }
    JSParseNode *pop() {
        JS_ASSERT(!empty());
        JSParseNode *hold = top; 
        top = top->pn_next;
        return hold;
    }
  private:
    JSParseNode *top;
};

} 









static bool
PushNodeChildren(JSParseNode *pn, NodeStack *stack)
{
    switch (pn->pn_arity) {
      case PN_FUNC:
        

















        pn->pn_funbox = NULL;
        stack->pushUnlessNull(pn->pn_body);
        pn->pn_body = NULL;
        return false;

      case PN_NAME:
        











        if (!pn->pn_used) {
            stack->pushUnlessNull(pn->pn_expr);
            pn->pn_expr = NULL;
        }
        return !pn->pn_used && !pn->pn_defn;

      case PN_LIST:
        stack->pushList(pn);
        break;
      case PN_TERNARY:
        stack->pushUnlessNull(pn->pn_kid1);
        stack->pushUnlessNull(pn->pn_kid2);
        stack->pushUnlessNull(pn->pn_kid3);
        break;
      case PN_BINARY:
        if (pn->pn_left != pn->pn_right)
            stack->pushUnlessNull(pn->pn_left);
        stack->pushUnlessNull(pn->pn_right);
        break;
      case PN_UNARY:
        stack->pushUnlessNull(pn->pn_kid);
        break;
      case PN_NULLARY:
        



        return !pn->pn_used && !pn->pn_defn;
    }

    return true;
}






static void
PrepareNodeForMutation(JSParseNode *pn, JSTreeContext *tc)
{
    if (pn->pn_arity != PN_NULLARY) {
        if (pn->pn_arity == PN_FUNC) {
            












            if (pn->pn_funbox)
                pn->pn_funbox->node = NULL;
        }

        
        NodeStack stack;
        PushNodeChildren(pn, &stack);
        



        while (!stack.empty()) {
            pn = stack.pop();
            if (PushNodeChildren(pn, &stack))
                AddNodeToFreeList(pn, tc);
        }
    }
}











static JSParseNode *
RecycleTree(JSParseNode *pn, JSTreeContext *tc)
{
    if (!pn)
        return NULL;

    JSParseNode *savedNext = pn->pn_next;

    NodeStack stack;
    for (;;) {
        if (PushNodeChildren(pn, &stack))
            AddNodeToFreeList(pn, tc);
        if (stack.empty())
            break;
        pn = stack.pop();
    }

    return savedNext;
}





static JSParseNode *
NewOrRecycledNode(JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = tc->parser->nodeList;
    if (!pn) {
        JSContext *cx = tc->parser->context;

        JS_ARENA_ALLOCATE_TYPE(pn, JSParseNode, &cx->tempPool);
        if (!pn)
            js_ReportOutOfMemory(cx);
    } else {
        tc->parser->nodeList = pn->pn_next;
    }

    if (pn) {
        pn->pn_used = pn->pn_defn = false;
        memset(&pn->pn_u, 0, sizeof pn->pn_u);
        pn->pn_next = NULL;
    }
    return pn;
}



JSParseNode *
JSParseNode::create(JSParseNodeArity arity, JSTreeContext *tc)
{
    JSParseNode *pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    const Token &tok = tc->parser->tokenStream.currentToken();
    pn->init(tok.type, JSOP_NOP, arity);
    pn->pn_pos = tok.pos;
    return pn;
}

JSParseNode *
JSParseNode::newBinaryOrAppend(TokenKind tt, JSOp op, JSParseNode *left, JSParseNode *right,
                               JSTreeContext *tc)
{
    JSParseNode *pn, *pn1, *pn2;

    if (!left || !right)
        return NULL;

    



    if (PN_TYPE(left) == tt &&
        PN_OP(left) == op &&
        (js_CodeSpec[op].format & JOF_LEFTASSOC)) {
        if (left->pn_arity != PN_LIST) {
            pn1 = left->pn_left, pn2 = left->pn_right;
            left->pn_arity = PN_LIST;
            left->pn_parens = false;
            left->initList(pn1);
            left->append(pn2);
            if (tt == TOK_PLUS) {
                if (pn1->pn_type == TOK_STRING)
                    left->pn_xflags |= PNX_STRCAT;
                else if (pn1->pn_type != TOK_NUMBER)
                    left->pn_xflags |= PNX_CANTFOLD;
                if (pn2->pn_type == TOK_STRING)
                    left->pn_xflags |= PNX_STRCAT;
                else if (pn2->pn_type != TOK_NUMBER)
                    left->pn_xflags |= PNX_CANTFOLD;
            }
        }
        left->append(right);
        left->pn_pos.end = right->pn_pos.end;
        if (tt == TOK_PLUS) {
            if (right->pn_type == TOK_STRING)
                left->pn_xflags |= PNX_STRCAT;
            else if (right->pn_type != TOK_NUMBER)
                left->pn_xflags |= PNX_CANTFOLD;
        }
        return left;
    }

    






    if (tt == TOK_PLUS &&
        left->pn_type == TOK_NUMBER &&
        right->pn_type == TOK_NUMBER &&
        tc->parser->foldConstants) {
        left->pn_dval += right->pn_dval;
        left->pn_pos.end = right->pn_pos.end;
        RecycleTree(right, tc);
        return left;
    }

    pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    pn->init(tt, op, PN_BINARY);
    pn->pn_pos.begin = left->pn_pos.begin;
    pn->pn_pos.end = right->pn_pos.end;
    pn->pn_left = left;
    pn->pn_right = right;
    return (BinaryNode *)pn;
}

namespace js {

inline void
NameNode::initCommon(JSTreeContext *tc)
{
    pn_expr = NULL;
    pn_cookie.makeFree();
    pn_dflags = (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
                ? PND_BLOCKCHILD
                : 0;
    pn_blockid = tc->blockid();
}

NameNode *
NameNode::create(JSAtom *atom, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = JSParseNode::create(PN_NAME, tc);
    if (pn) {
        pn->pn_atom = atom;
        ((NameNode *)pn)->initCommon(tc);
    }
    return (NameNode *)pn;
}

} 

static bool
GenerateBlockId(JSTreeContext *tc, uint32& blockid)
{
    if (tc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(tc->parser->context, js_GetErrorMessage, NULL,
                             JSMSG_NEED_DIET, "program");
        return false;
    }
    blockid = tc->blockidGen++;
    return true;
}

static bool
GenerateBlockIdForStmtNode(JSParseNode *pn, JSTreeContext *tc)
{
    JS_ASSERT(tc->topStmt);
    JS_ASSERT(STMT_MAYBE_SCOPE(tc->topStmt));
    JS_ASSERT(pn->pn_type == TOK_LC || pn->pn_type == TOK_LEXICALSCOPE);
    if (!GenerateBlockId(tc, tc->topStmt->blockid))
        return false;
    pn->pn_blockid = tc->topStmt->blockid;
    return true;
}




JSParseNode *
Parser::parse(JSObject *chain)
{
    







    JSTreeContext globaltc(this);
    if (!globaltc.init(context))
        return NULL;
    globaltc.setScopeChain(chain);
    if (!GenerateBlockId(&globaltc, globaltc.bodyid))
        return NULL;

    JSParseNode *pn = statements();
    if (pn) {
        if (!tokenStream.matchToken(TOK_EOF)) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (foldConstants) {
            if (!js_FoldConstants(context, pn, &globaltc))
                pn = NULL;
        }
    }
    return pn;
}

JS_STATIC_ASSERT(UpvarCookie::FREE_LEVEL == JS_BITMASK(JSFB_LEVEL_BITS));

static inline bool
SetStaticLevel(JSTreeContext *tc, uintN staticLevel)
{
    



    if (UpvarCookie::isLevelReserved(staticLevel)) {
        JS_ReportErrorNumber(tc->parser->context, js_GetErrorMessage, NULL,
                             JSMSG_TOO_DEEP, js_function_str);
        return false;
    }
    tc->staticLevel = staticLevel;
    return true;
}




Compiler::Compiler(JSContext *cx, JSPrincipals *prin, StackFrame *cfp)
  : parser(cx, prin, cfp), globalScope(NULL)
{}

JSScript *
Compiler::compileScript(JSContext *cx, JSObject *scopeChain, StackFrame *callerFrame,
                        JSPrincipals *principals, uint32 tcflags,
                        const jschar *chars, size_t length,
                        const char *filename, uintN lineno, JSVersion version,
                        JSString *source ,
                        uintN staticLevel )
{
    JSArenaPool codePool, notePool;
    TokenKind tt;
    JSParseNode *pn;
    JSScript *script;
    bool inDirectivePrologue;

    JS_ASSERT(!(tcflags & ~(TCF_COMPILE_N_GO | TCF_NO_SCRIPT_RVAL | TCF_NEED_MUTABLE_SCRIPT |
                            TCF_COMPILE_FOR_EVAL)));

    



    JS_ASSERT_IF(callerFrame, tcflags & TCF_COMPILE_N_GO);
    JS_ASSERT_IF(staticLevel != 0, callerFrame);

    Compiler compiler(cx, principals, callerFrame);
    if (!compiler.init(chars, length, filename, lineno, version))
        return NULL;

    JS_InitArenaPool(&codePool, "code", 1024, sizeof(jsbytecode));
    JS_InitArenaPool(&notePool, "note", 1024, sizeof(jssrcnote));

    Parser &parser = compiler.parser;
    TokenStream &tokenStream = parser.tokenStream;

    JSCodeGenerator cg(&parser, &codePool, &notePool, tokenStream.getLineno());
    if (!cg.init(cx, JSTreeContext::USED_AS_TREE_CONTEXT))
        return NULL;

    MUST_FLOW_THROUGH("out");

    
    JSObject *globalObj = scopeChain && scopeChain == scopeChain->getGlobal()
                        ? scopeChain->getGlobal()
                        : NULL;

    JS_ASSERT_IF(globalObj, globalObj->isNative());
    JS_ASSERT_IF(globalObj, (globalObj->getClass()->flags & JSCLASS_GLOBAL_FLAGS) ==
                            JSCLASS_GLOBAL_FLAGS);

    
    script = NULL;

    GlobalScope globalScope(cx, globalObj, &cg);
    cg.flags |= tcflags;
    cg.setScopeChain(scopeChain);
    compiler.globalScope = &globalScope;
    if (!SetStaticLevel(&cg, staticLevel))
        goto out;

    
    if (callerFrame &&
        callerFrame->isScriptFrame() &&
        callerFrame->script()->strictModeCode) {
        cg.flags |= TCF_STRICT_MODE_CODE;
        tokenStream.setStrictMode();
    }

    



    JSObjectBox *funbox;
    funbox = NULL;

    if (tcflags & TCF_COMPILE_N_GO) {
        if (source) {
            



            JSAtom *atom = js_AtomizeString(cx, source);
            jsatomid _;
            if (!atom || !cg.makeAtomIndex(atom, &_))
                goto out;
        }

        if (callerFrame && callerFrame->isFunctionFrame()) {
            




            funbox = parser.newObjectBox(FUN_OBJECT(callerFrame->fun()));
            if (!funbox)
                goto out;
            funbox->emitLink = cg.objectList.lastbox;
            cg.objectList.lastbox = funbox;
            cg.objectList.length++;
        }
    }

    



    uint32 bodyid;
    if (!GenerateBlockId(&cg, bodyid))
        goto out;
    cg.bodyid = bodyid;

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
        JS_ASSERT(!cg.blockNode);

        if (inDirectivePrologue && !parser.recognizeDirectivePrologue(pn, &inDirectivePrologue))
            goto out;

        if (!js_FoldConstants(cx, pn, &cg))
            goto out;

        if (!parser.analyzeFunctions(&cg))
            goto out;
        cg.functionList = NULL;

        if (!js_EmitTree(cx, &cg, pn))
            goto out;

#if JS_HAS_XML_SUPPORT
        if (PN_TYPE(pn) != TOK_SEMI ||
            !pn->pn_kid ||
            !TreeTypeIsXML(PN_TYPE(pn->pn_kid))) {
            onlyXML = false;
        }
#endif
        RecycleTree(pn, &cg);
    }

#if JS_HAS_XML_SUPPORT
    





    if (pn && onlyXML && !callerFrame) {
        parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_XML_WHOLE_PROGRAM);
        goto out;
    }
#endif

    




    if (cg.hasSharps()) {
        jsbytecode *code, *end;
        JSOp op;
        const JSCodeSpec *cs;
        uintN len, slot;

        code = CG_BASE(&cg);
        for (end = code + CG_OFFSET(&cg); code != end; code += len) {
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
                             (JOF_TYPE(cs->format) == JOF_SLOTATOM) ==
                             (op == JSOP_GETLOCALPROP));
                slot = GET_SLOTNO(code);
                if (!(cs->format & JOF_SHARPSLOT))
                    slot += cg.sharpSlots();
                if (slot >= SLOTNO_LIMIT)
                    goto too_many_slots;
                SET_SLOTNO(code, slot);
            }
        }
    }

    



    if (js_Emit1(cx, &cg, JSOP_STOP) < 0)
        goto out;

    JS_ASSERT(cg.version() == version);

    script = JSScript::NewScriptFromCG(cx, &cg);
    if (!script)
        goto out;

    if (funbox)
        script->savedCallerFun = true;

    {
        AutoShapeRooter shapeRoot(cx, script->bindings.lastShape());
        if (!defineGlobals(cx, globalScope, script))
            goto late_error;
    }

  out:
    JS_FinishArenaPool(&codePool);
    JS_FinishArenaPool(&notePool);
    return script;

  too_many_slots:
    parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_LOCALS);
    

  late_error:
    if (script) {
        js_DestroyScript(cx, script);
        script = NULL;
    }
    goto out;
}

bool
Compiler::defineGlobals(JSContext *cx, GlobalScope &globalScope, JSScript *script)
{
    if (!globalScope.defs.length())
        return true;

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
        } else {
            rval.setUndefined();
        }

        const Shape *shape =
            DefineNativeProperty(cx, globalObj, id, rval, PropertyStub, StrictPropertyStub,
                                 JSPROP_ENUMERATE | JSPROP_PERMANENT, 0, 0);
        if (!shape)
            return false;
        def.knownSlot = shape->slot;
    }

    js::Vector<JSScript *, 16> worklist(cx);
    if (!worklist.append(script))
        return false;

    





    while (worklist.length()) {
        JSScript *inner = worklist.back();
        worklist.popBack();

        if (JSScript::isValidOffset(inner->objectsOffset)) {
            JSObjectArray *arr = inner->objects();
            for (size_t i = 0; i < arr->length; i++) {
                JSObject *obj = arr->vector[i];
                if (!obj->isFunction())
                    continue;
                JSFunction *fun = obj->getFunctionPrivate();
                JS_ASSERT(fun->isInterpreted());
                JSScript *inner = fun->script();
                if (!JSScript::isValidOffset(inner->globalsOffset) &&
                    !JSScript::isValidOffset(inner->objectsOffset)) {
                    continue;
                }
                if (!worklist.append(inner))
                    return false;
            }
        }

        if (!JSScript::isValidOffset(inner->globalsOffset))
            continue;

        GlobalSlotArray *globalUses = inner->globals();
        uint32 nGlobalUses = globalUses->length;
        for (uint32 i = 0; i < nGlobalUses; i++) {
            uint32 index = globalUses->vector[i].slot;
            JS_ASSERT(index < globalScope.defs.length());
            globalUses->vector[i].slot = globalScope.defs[index].knownSlot;
        }
    }

    return true;
}







#define ENDS_IN_OTHER   0
#define ENDS_IN_RETURN  1
#define ENDS_IN_BREAK   2

static int
HasFinalReturn(JSParseNode *pn)
{
    JSParseNode *pn2, *pn3;
    uintN rv, rv2, hasDefault;

    switch (pn->pn_type) {
      case TOK_LC:
        if (!pn->pn_head)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->last());

      case TOK_IF:
        if (!pn->pn_kid3)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->pn_kid2) & HasFinalReturn(pn->pn_kid3);

      case TOK_WHILE:
        pn2 = pn->pn_left;
        if (pn2->pn_type == TOK_PRIMARY && pn2->pn_op == JSOP_TRUE)
            return ENDS_IN_RETURN;
        if (pn2->pn_type == TOK_NUMBER && pn2->pn_dval)
            return ENDS_IN_RETURN;
        return ENDS_IN_OTHER;

      case TOK_DO:
        pn2 = pn->pn_right;
        if (pn2->pn_type == TOK_PRIMARY) {
            if (pn2->pn_op == JSOP_FALSE)
                return HasFinalReturn(pn->pn_left);
            if (pn2->pn_op == JSOP_TRUE)
                return ENDS_IN_RETURN;
        }
        if (pn2->pn_type == TOK_NUMBER) {
            if (pn2->pn_dval == 0)
                return HasFinalReturn(pn->pn_left);
            return ENDS_IN_RETURN;
        }
        return ENDS_IN_OTHER;

      case TOK_FOR:
        pn2 = pn->pn_left;
        if (pn2->pn_arity == PN_TERNARY && !pn2->pn_kid2)
            return ENDS_IN_RETURN;
        return ENDS_IN_OTHER;

      case TOK_SWITCH:
        rv = ENDS_IN_RETURN;
        hasDefault = ENDS_IN_OTHER;
        pn2 = pn->pn_right;
        if (pn2->pn_type == TOK_LEXICALSCOPE)
            pn2 = pn2->expr();
        for (pn2 = pn2->pn_head; rv && pn2; pn2 = pn2->pn_next) {
            if (pn2->pn_type == TOK_DEFAULT)
                hasDefault = ENDS_IN_RETURN;
            pn3 = pn2->pn_right;
            JS_ASSERT(pn3->pn_type == TOK_LC);
            if (pn3->pn_head) {
                rv2 = HasFinalReturn(pn3->last());
                if (rv2 == ENDS_IN_OTHER && pn2->pn_next)
                    ;
                else
                    rv &= rv2;
            }
        }
        
        rv &= hasDefault;
        return rv;

      case TOK_BREAK:
        return ENDS_IN_BREAK;

      case TOK_WITH:
        return HasFinalReturn(pn->pn_right);

      case TOK_RETURN:
        return ENDS_IN_RETURN;

      case TOK_COLON:
      case TOK_LEXICALSCOPE:
        return HasFinalReturn(pn->expr());

      case TOK_THROW:
        return ENDS_IN_RETURN;

      case TOK_TRY:
        
        if (pn->pn_kid3) {
            rv = HasFinalReturn(pn->pn_kid3);
            if (rv == ENDS_IN_RETURN)
                return rv;
        }

        
        rv = HasFinalReturn(pn->pn_kid1);
        if (pn->pn_kid2) {
            JS_ASSERT(pn->pn_kid2->pn_arity == PN_LIST);
            for (pn2 = pn->pn_kid2->pn_head; pn2; pn2 = pn2->pn_next)
                rv &= HasFinalReturn(pn2);
        }
        return rv;

      case TOK_CATCH:
        
        return HasFinalReturn(pn->pn_kid3);

      case TOK_LET:
        
        if (pn->pn_arity != PN_BINARY)
            return ENDS_IN_OTHER;
        return HasFinalReturn(pn->pn_right);

      default:
        return ENDS_IN_OTHER;
    }
}

static JSBool
ReportBadReturn(JSContext *cx, JSTreeContext *tc, JSParseNode *pn, uintN flags, uintN errnum,
                uintN anonerrnum)
{
    JSAutoByteString name;
    if (tc->fun()->atom) {
        if (!js_AtomToPrintableString(cx, tc->fun()->atom, &name))
            return false;
    } else {
        errnum = anonerrnum;
    }
    return ReportCompileErrorNumber(cx, TS(tc->parser), pn, flags, errnum, name.ptr());
}

static JSBool
CheckFinalReturn(JSContext *cx, JSTreeContext *tc, JSParseNode *pn)
{
    JS_ASSERT(tc->inFunction());
    return HasFinalReturn(pn) == ENDS_IN_RETURN ||
           ReportBadReturn(cx, tc, pn, JSREPORT_WARNING | JSREPORT_STRICT,
                           JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE);
}





bool
CheckStrictAssignment(JSContext *cx, JSTreeContext *tc, JSParseNode *lhs)
{
    if (tc->needStrictChecks() && lhs->pn_type == TOK_NAME) {
        JSAtom *atom = lhs->pn_atom;
        JSAtomState *atomState = &cx->runtime->atomState;
        if (atom == atomState->evalAtom || atom == atomState->argumentsAtom) {
            JSAutoByteString name;
            if (!js_AtomToPrintableString(cx, atom, &name) ||
                !ReportStrictModeError(cx, TS(tc->parser), tc, lhs, JSMSG_DEPRECATED_ASSIGN,
                                       name.ptr())) {
                return false;
            }
        }
    }
    return true;
}







bool
CheckStrictBinding(JSContext *cx, JSTreeContext *tc, JSAtom *atom, JSParseNode *pn)
{
    if (!tc->needStrictChecks())
        return true;

    JSAtomState *atomState = &cx->runtime->atomState;
    if (atom == atomState->evalAtom ||
        atom == atomState->argumentsAtom ||
        FindKeyword(atom->charsZ(), atom->length()))
    {
        JSAutoByteString name;
        if (!js_AtomToPrintableString(cx, atom, &name))
            return false;
        return ReportStrictModeError(cx, TS(tc->parser), tc, pn, JSMSG_BAD_BINDING, name.ptr());
    }

    return true;
}

static bool
ReportBadParameter(JSContext *cx, JSTreeContext *tc, JSAtom *name, uintN errorNumber)
{
    JSDefinition *dn = tc->decls.lookupFirst(name);
    JSAutoByteString bytes;
    return js_AtomToPrintableString(cx, name, &bytes) &&
           ReportStrictModeError(cx, TS(tc->parser), tc, dn, errorNumber, bytes.ptr());
}







static bool
CheckStrictParameters(JSContext *cx, JSTreeContext *tc)
{
    JS_ASSERT(tc->inFunction());

    if (!tc->needStrictChecks() || tc->bindings.countArgs() == 0)
        return true;

    JSAtom *argumentsAtom = cx->runtime->atomState.argumentsAtom;
    JSAtom *evalAtom = cx->runtime->atomState.evalAtom;

    
    HashMap<JSAtom *, bool> parameters(cx);
    if (!parameters.init(tc->bindings.countArgs()))
        return false;

    
    for (Shape::Range r = tc->bindings.lastVariable(); !r.empty(); r.popFront()) {
        jsid id = r.front().propid;
        if (!JSID_IS_ATOM(id))
            continue;

        JSAtom *name = JSID_TO_ATOM(id);

        if (name == argumentsAtom || name == evalAtom) {
            if (!ReportBadParameter(cx, tc, name, JSMSG_BAD_BINDING))
                return false;
        }

        if (tc->inStrictMode() && FindKeyword(name->charsZ(), name->length())) {
            



            JS_ALWAYS_TRUE(!ReportBadParameter(cx, tc, name, JSMSG_RESERVED_ID));
            return false;
        }

        



        if (HashMap<JSAtom *, bool>::AddPtr p = parameters.lookupForAdd(name)) {
            if (!p->value && !ReportBadParameter(cx, tc, name, JSMSG_DUPLICATE_FORMAL))
                return false;
            p->value = true;
        } else {
            if (!parameters.add(p, name, false))
                return false;
        }
    }

    return true;
}

JSParseNode *
Parser::functionBody()
{
    JSStmtInfo stmtInfo;
    uintN oldflags, firstLine;
    JSParseNode *pn;

    JS_ASSERT(tc->inFunction());
    js_PushStatement(tc, &stmtInfo, STMT_BLOCK, -1);
    stmtInfo.flags = SIF_BODY_BLOCK;

    oldflags = tc->flags;
    tc->flags &= ~(TCF_RETURN_EXPR | TCF_RETURN_VOID);

    




    firstLine = tokenStream.getLineno();
#if JS_HAS_EXPR_CLOSURES
    if (tokenStream.currentToken().type == TOK_LC) {
        pn = statements();
    } else {
        pn = UnaryNode::create(tc);
        if (pn) {
            pn->pn_kid = assignExpr();
            if (!pn->pn_kid) {
                pn = NULL;
            } else {
                if (tc->flags & TCF_FUN_IS_GENERATOR) {
                    ReportBadReturn(context, tc, pn, JSREPORT_ERROR,
                                    JSMSG_BAD_GENERATOR_RETURN,
                                    JSMSG_BAD_ANON_GENERATOR_RETURN);
                    pn = NULL;
                } else {
                    pn->pn_type = TOK_RETURN;
                    pn->pn_op = JSOP_RETURN;
                    pn->pn_pos.end = pn->pn_kid->pn_pos.end;
                }
            }
        }
    }
#else
    pn = statements();
#endif

    if (pn) {
        JS_ASSERT(!(tc->topStmt->flags & SIF_SCOPE));
        js_PopStatement(tc);
        pn->pn_pos.begin.lineno = firstLine;

        
        if (context->hasStrictOption() && (tc->flags & TCF_RETURN_EXPR) &&
            !CheckFinalReturn(context, tc, pn)) {
            pn = NULL;
        }
    }

    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}





static JSDefinition *
MakePlaceholder(AtomDefnAddPtr &p, JSParseNode *pn, JSTreeContext *tc)
{
    JSAtom *atom = pn->pn_atom;
    JSDefinition *dn = (JSDefinition *) NameNode::create(atom, tc);
    if (!dn)
        return NULL;

    if (!tc->lexdeps->add(p, atom, dn))
        return NULL;

    dn->pn_type = TOK_NAME;
    dn->pn_op = JSOP_NOP;
    dn->pn_defn = true;
    dn->pn_dflags |= PND_PLACEHOLDER;
    return dn;
}

static bool
Define(JSParseNode *pn, JSAtom *atom, JSTreeContext *tc, bool let = false)
{
    JS_ASSERT(!pn->pn_used);
    JS_ASSERT_IF(pn->pn_defn, pn->isPlaceholder());

    bool foundLexdep = false;
    JSDefinition *dn = NULL;

    if (let)
        dn = tc->decls.lookupFirst(atom);

    if (!dn) {
        dn = tc->lexdeps.lookupDefn(atom);
        foundLexdep = !!dn;
    }

    if (dn && dn != pn) {
        JSParseNode **pnup = &dn->dn_uses;
        JSParseNode *pnu;
        uintN start = let ? pn->pn_blockid : tc->bodyid;

        while ((pnu = *pnup) != NULL && pnu->pn_blockid >= start) {
            JS_ASSERT(pnu->pn_used);
            pnu->pn_lexdef = (JSDefinition *) pn;
            pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
            pnup = &pnu->pn_link;
        }

        if (pnu != dn->dn_uses) {
            *pnup = pn->dn_uses;
            pn->dn_uses = dn->dn_uses;
            dn->dn_uses = pnu;

            if ((!pnu || pnu->pn_blockid < tc->bodyid) && foundLexdep)
                tc->lexdeps->remove(atom);
        }
    }

    JSDefinition *toAdd = (JSDefinition *) pn;
    bool ok = let ? tc->decls.addShadow(atom, toAdd) : tc->decls.addUnique(atom, toAdd);
    if (!ok)
        return false;
    pn->pn_defn = true;
    pn->pn_dflags &= ~PND_PLACEHOLDER;
    if (!tc->parent)
        pn->pn_dflags |= PND_TOPLEVEL;
    return true;
}

static void
LinkUseToDef(JSParseNode *pn, JSDefinition *dn, JSTreeContext *tc)
{
    JS_ASSERT(!pn->pn_used);
    JS_ASSERT(!pn->pn_defn);
    JS_ASSERT(pn != dn->dn_uses);
    pn->pn_link = dn->dn_uses;
    dn->dn_uses = pn;
    dn->pn_dflags |= pn->pn_dflags & PND_USE2DEF_FLAGS;
    pn->pn_used = true;
    pn->pn_lexdef = dn;
}

static void
ForgetUse(JSParseNode *pn)
{
    if (!pn->pn_used) {
        JS_ASSERT(!pn->pn_defn);
        return;
    }

    JSParseNode **pnup = &pn->lexdef()->dn_uses;
    JSParseNode *pnu;
    while ((pnu = *pnup) != pn)
        pnup = &pnu->pn_link;
    *pnup = pn->pn_link;
    pn->pn_used = false;
}

static JSParseNode *
MakeAssignment(JSParseNode *pn, JSParseNode *rhs, JSTreeContext *tc)
{
    JSParseNode *lhs = NewOrRecycledNode(tc);
    if (!lhs)
        return NULL;
    *lhs = *pn;

    if (pn->pn_used) {
        JSDefinition *dn = pn->pn_lexdef;
        JSParseNode **pnup = &dn->dn_uses;

        while (*pnup != pn)
            pnup = &(*pnup)->pn_link;
        *pnup = lhs;
        lhs->pn_link = pn->pn_link;
        pn->pn_link = NULL;
    }

    pn->pn_type = TOK_ASSIGN;
    pn->pn_op = JSOP_NOP;
    pn->pn_arity = PN_BINARY;
    pn->pn_parens = false;
    pn->pn_used = pn->pn_defn = false;
    pn->pn_left = lhs;
    pn->pn_right = rhs;
    return lhs;
}

static JSParseNode *
MakeDefIntoUse(JSDefinition *dn, JSParseNode *pn, JSAtom *atom, JSTreeContext *tc)
{
    




    if (dn->isBindingForm()) {
        JSParseNode *rhs = dn->expr();
        if (rhs) {
            JSParseNode *lhs = MakeAssignment(dn, rhs, tc);
            if (!lhs)
                return NULL;
            
            dn = (JSDefinition *) lhs;
        }

        dn->pn_op = (js_CodeSpec[dn->pn_op].format & JOF_SET) ? JSOP_SETNAME : JSOP_NAME;
    } else if (dn->kind() == JSDefinition::FUNCTION) {
        JS_ASSERT(dn->pn_op == JSOP_NOP);
        PrepareNodeForMutation(dn, tc);
        dn->pn_type = TOK_NAME;
        dn->pn_arity = PN_NAME;
        dn->pn_atom = atom;
    }

    
    JS_ASSERT(dn->pn_type == TOK_NAME);
    JS_ASSERT(dn->pn_arity == PN_NAME);
    JS_ASSERT(dn->pn_atom == atom);

    for (JSParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
        JS_ASSERT(pnu->pn_used);
        JS_ASSERT(!pnu->pn_defn);
        pnu->pn_lexdef = (JSDefinition *) pn;
        pn->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
    }
    pn->pn_dflags |= dn->pn_dflags & PND_USE2DEF_FLAGS;
    pn->dn_uses = dn;

    dn->pn_defn = false;
    dn->pn_used = true;
    dn->pn_lexdef = (JSDefinition *) pn;
    dn->pn_cookie.makeFree();
    dn->pn_dflags &= ~PND_BOUND;
    return dn;
}

static bool
DefineArg(JSParseNode *pn, JSAtom *atom, uintN i, JSTreeContext *tc)
{
    JSParseNode *argpn, *argsbody;

    
    if (atom == tc->parser->context->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_PARAM_ARGUMENTS;

    




    argpn = NameNode::create(atom, tc);
    if (!argpn)
        return false;
    JS_ASSERT(PN_TYPE(argpn) == TOK_NAME && PN_OP(argpn) == JSOP_NOP);

    
    argpn->pn_dflags |= PND_INITIALIZED;
    if (!Define(argpn, atom, tc))
        return false;

    argsbody = pn->pn_body;
    if (!argsbody) {
        argsbody = ListNode::create(tc);
        if (!argsbody)
            return false;
        argsbody->pn_type = TOK_ARGSBODY;
        argsbody->pn_op = JSOP_NOP;
        argsbody->makeEmpty();
        pn->pn_body = argsbody;
    }
    argsbody->append(argpn);

    argpn->pn_op = JSOP_GETARG;
    argpn->pn_cookie.set(tc->staticLevel, i);
    argpn->pn_dflags |= PND_BOUND;
    return true;
}





bool
Compiler::compileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                              Bindings *bindings, const jschar *chars, size_t length,
                              const char *filename, uintN lineno, JSVersion version)
{
    Compiler compiler(cx, principals);

    if (!compiler.init(chars, length, filename, lineno, version))
        return false;

    
    JSArenaPool codePool, notePool;
    JS_InitArenaPool(&codePool, "code", 1024, sizeof(jsbytecode));
    JS_InitArenaPool(&notePool, "note", 1024, sizeof(jssrcnote));

    Parser &parser = compiler.parser;
    TokenStream &tokenStream = parser.tokenStream;

    JSCodeGenerator funcg(&parser, &codePool, &notePool, tokenStream.getLineno());
    if (!funcg.init(cx, JSTreeContext::USED_AS_TREE_CONTEXT))
        return false;

    funcg.flags |= TCF_IN_FUNCTION;
    funcg.setFunction(fun);
    funcg.bindings.transfer(cx, bindings);
    fun->setArgCount(funcg.bindings.countArgs());
    if (!GenerateBlockId(&funcg, funcg.bodyid))
        return false;

    
    tokenStream.mungeCurrentToken(TOK_NAME);
    JSParseNode *fn = FunctionNode::create(&funcg);
    if (fn) {
        fn->pn_body = NULL;
        fn->pn_cookie.makeFree();

        uintN nargs = fun->nargs;
        if (nargs) {
            



            jsuword *names = funcg.bindings.getLocalNameArray(cx, &cx->tempPool);
            if (!names) {
                fn = NULL;
            } else {
                for (uintN i = 0; i < nargs; i++) {
                    JSAtom *name = JS_LOCAL_NAME_TO_ATOM(names[i]);
                    if (!DefineArg(fn, name, i, &funcg)) {
                        fn = NULL;
                        break;
                    }
                }
            }
        }
    }

    





    tokenStream.mungeCurrentToken(TOK_LC);
    JSParseNode *pn = fn ? parser.functionBody() : NULL;
    if (pn) {
        if (!CheckStrictParameters(cx, &funcg)) {
            pn = NULL;
        } else if (!tokenStream.matchToken(TOK_EOF)) {
            parser.reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (!js_FoldConstants(cx, pn, &funcg)) {
            
            pn = NULL;
        } else if (!parser.analyzeFunctions(&funcg)) {
            pn = NULL;
        } else {
            if (fn->pn_body) {
                JS_ASSERT(PN_TYPE(fn->pn_body) == TOK_ARGSBODY);
                fn->pn_body->append(pn);
                fn->pn_body->pn_pos = pn->pn_pos;
                pn = fn->pn_body;
            }

            if (!js_EmitFunctionScript(cx, &funcg, pn))
                pn = NULL;
        }
    }

    
    JS_FinishArenaPool(&codePool);
    JS_FinishArenaPool(&notePool);
    return pn != NULL;
}








typedef JSBool
(*Binder)(JSContext *cx, BindData *data, JSAtom *atom, JSTreeContext *tc);

struct BindData {
    BindData() : fresh(true) {}

    JSParseNode     *pn;        

    JSOp            op;         
    Binder          binder;     
    union {
        struct {
            uintN   overflow;
        } let;
    };
    bool fresh;
};

static bool
BindLocalVariable(JSContext *cx, JSTreeContext *tc, JSAtom *atom, BindingKind kind, bool isArg)
{
    JS_ASSERT(kind == VARIABLE || kind == CONSTANT);

    








    if (atom == cx->runtime->atomState.argumentsAtom && !isArg)
        return true;

    return tc->bindings.add(cx, atom, kind);
}

#if JS_HAS_DESTRUCTURING
static JSBool
BindDestructuringArg(JSContext *cx, BindData *data, JSAtom *atom, JSTreeContext *tc)
{
    
    if (atom == tc->parser->context->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_PARAM_ARGUMENTS;

    JS_ASSERT(tc->inFunction());

    




    if (tc->decls.lookupFirst(atom)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), NULL, JSREPORT_ERROR,
                                 JSMSG_DESTRUCT_DUP_ARG);
        return JS_FALSE;
    }

    JSParseNode *pn = data->pn;

    


















    pn->pn_op = JSOP_SETLOCAL;
    pn->pn_dflags |= PND_BOUND;

    return Define(pn, atom, tc);
}
#endif 

JSFunction *
Parser::newFunction(JSTreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, atom != NULL);

    





    while (tc->parent)
        tc = tc->parent;
    JSObject *parent = tc->inFunction() ? NULL : tc->scopeChain();

    JSFunction *fun =
        js_NewFunction(context, NULL, NULL, 0,
                       JSFUN_INTERPRETED | (kind == Expression ? JSFUN_LAMBDA : 0),
                       parent, atom);
    if (fun && !tc->compileAndGo()) {
        FUN_OBJECT(fun)->clearParent();
        FUN_OBJECT(fun)->clearProto();
    }
    return fun;
}

static JSBool
MatchOrInsertSemicolon(JSContext *cx, TokenStream *ts)
{
    TokenKind tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return JS_FALSE;
    if (tt != TOK_EOF && tt != TOK_EOL && tt != TOK_SEMI && tt != TOK_RC) {
        
        ts->getToken(TSF_OPERAND);
        ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR, JSMSG_SEMI_BEFORE_STMNT);
        return JS_FALSE;
    }
    (void) ts->matchToken(TOK_SEMI);
    return JS_TRUE;
}

bool
Parser::analyzeFunctions(JSTreeContext *tc)
{
    cleanFunctionList(&tc->functionList);
    if (!tc->functionList)
        return true;
    if (!markFunArgs(tc->functionList))
        return false;
    markExtensibleScopeDescendants(tc->functionList, false);
    setFunctionKinds(tc->functionList, &tc->flags);
    return true;
}































static uintN
FindFunArgs(JSFunctionBox *funbox, int level, JSFunctionBoxQueue *queue)
{
    uintN allskipmin = UpvarCookie::FREE_LEVEL;

    do {
        JSParseNode *fn = funbox->node;
        JS_ASSERT(fn->pn_arity == PN_FUNC);
        JSFunction *fun = funbox->function();
        int fnlevel = level;

        













        if (funbox->tcflags & (TCF_FUN_HEAVYWEIGHT | TCF_FUN_IS_GENERATOR)) {
            fn->setFunArg();
            for (JSFunctionBox *kid = funbox->kids; kid; kid = kid->siblings)
                kid->node->setFunArg();
        }

        




        uintN skipmin = UpvarCookie::FREE_LEVEL;
        JSParseNode *pn = fn->pn_body;

        if (pn->pn_type == TOK_UPVARS) {
            AtomDefnMapPtr &upvars = pn->pn_names;
            JS_ASSERT(upvars->count() != 0);

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                JSDefinition *defn = r.front().value();
                JSDefinition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar()) {
                    uintN upvarLevel = lexdep->frameLevel();

                    if (int(upvarLevel) <= fnlevel)
                        fn->setFunArg();

                    uintN skip = (funbox->level + 1) - upvarLevel;
                    if (skip < skipmin)
                        skipmin = skip;
                }
            }
        }

        






        if (fn->isFunArg()) {
            queue->push(funbox);
            fnlevel = int(funbox->level);
        }

        



        if (funbox->kids) {
            uintN kidskipmin = FindFunArgs(funbox->kids, fnlevel, queue);

            JS_ASSERT(kidskipmin != 0);
            if (kidskipmin != UpvarCookie::FREE_LEVEL) {
                --kidskipmin;
                if (kidskipmin != 0 && kidskipmin < skipmin)
                    skipmin = kidskipmin;
            }
        }

        





        if (skipmin != UpvarCookie::FREE_LEVEL) {
            fun->u.i.skipmin = skipmin;
            if (skipmin < allskipmin)
                allskipmin = skipmin;
        }
    } while ((funbox = funbox->siblings) != NULL);

    return allskipmin;
}

bool
Parser::markFunArgs(JSFunctionBox *funbox)
{
    JSFunctionBoxQueue queue;
    if (!queue.init(functionCount)) {
        js_ReportOutOfMemory(context);
        return false;
    }

    FindFunArgs(funbox, -1, &queue);
    while ((funbox = queue.pull()) != NULL) {
        JSParseNode *fn = funbox->node;
        JS_ASSERT(fn->isFunArg());

        JSParseNode *pn = fn->pn_body;
        if (pn->pn_type == TOK_UPVARS) {
            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                JSDefinition *defn = r.front().value();
                JSDefinition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar() &&
                    !lexdep->isFunArg() &&
                    (lexdep->kind() == JSDefinition::FUNCTION ||
                     PN_OP(lexdep) == JSOP_CALLEE)) {
                    








                    lexdep->setFunArg();

                    JSFunctionBox *afunbox;
                    if (PN_OP(lexdep) == JSOP_CALLEE) {
                        






                        afunbox = funbox;
                        uintN calleeLevel = lexdep->pn_cookie.level();
                        uintN staticLevel = afunbox->level + 1U;
                        while (staticLevel != calleeLevel) {
                            afunbox = afunbox->parent;
                            --staticLevel;
                        }
                        JS_ASSERT(afunbox->level + 1U == calleeLevel);
                        afunbox->node->setFunArg();
                    } else {
                       afunbox = lexdep->pn_funbox;
                    }
                    queue.push(afunbox);

                    




                    if (afunbox->kids)
                        FindFunArgs(afunbox->kids, afunbox->level, &queue);
                }
            }
        }
    }
    return true;
}

static uint32
MinBlockId(JSParseNode *fn, uint32 id)
{
    if (fn->pn_blockid < id)
        return false;
    if (fn->pn_defn) {
        for (JSParseNode *pn = fn->dn_uses; pn; pn = pn->pn_link) {
            if (pn->pn_blockid < id)
                return false;
        }
    }
    return true;
}

static inline bool
CanFlattenUpvar(JSDefinition *dn, JSFunctionBox *funbox, uint32 tcflags)
{
    

















    JSFunctionBox *afunbox = funbox;
    uintN dnLevel = dn->frameLevel();

    JS_ASSERT(dnLevel <= funbox->level);
    while (afunbox->level != dnLevel) {
        afunbox = afunbox->parent;

        







        JS_ASSERT(afunbox);

        




        if (!afunbox || afunbox->node->isFunArg())
            return false;

        




        if (afunbox->tcflags & TCF_FUN_IS_GENERATOR)
            return false;
    }

    





    if (afunbox->inLoop)
        return false;

    





    if ((afunbox->parent ? afunbox->parent->tcflags : tcflags) & TCF_FUN_HEAVYWEIGHT)
        return false;

    






    JSFunction *afun = afunbox->function();
    if (!(afun->flags & JSFUN_LAMBDA)) {
        if (dn->isBindingForm() || dn->pn_pos >= afunbox->node->pn_pos)
            return false;
    }

    if (!dn->isInitialized())
        return false;

    JSDefinition::Kind dnKind = dn->kind();
    if (dnKind != JSDefinition::CONST) {
        if (dn->isAssigned())
            return false;

        










        if (dnKind == JSDefinition::ARG &&
            ((afunbox->parent ? afunbox->parent->tcflags : tcflags) & TCF_FUN_USES_ARGUMENTS)) {
            return false;
        }
    }

    




    if (dnKind != JSDefinition::FUNCTION) {
        














        if (dn->pn_pos.end >= afunbox->node->pn_pos.end)
            return false;
        if (!MinBlockId(afunbox->node, dn->pn_blockid))
            return false;
    }
    return true;
}

static void
FlagHeavyweights(JSDefinition *dn, JSFunctionBox *funbox, uint32 *tcflags)
{
    uintN dnLevel = dn->frameLevel();

    while ((funbox = funbox->parent) != NULL) {
        





        if (funbox->level + 1U == dnLevel || (dnLevel == 0 && dn->isLet())) {
            funbox->tcflags |= TCF_FUN_HEAVYWEIGHT;
            break;
        }
        funbox->tcflags |= TCF_FUN_ENTRAINS_SCOPES;
    }

    if (!funbox && (*tcflags & TCF_IN_FUNCTION))
        *tcflags |= TCF_FUN_HEAVYWEIGHT;
}

static bool
DeoptimizeUsesWithin(JSDefinition *dn, const TokenPos &pos)
{
    uintN ndeoptimized = 0;

    for (JSParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
        JS_ASSERT(pnu->pn_used);
        JS_ASSERT(!pnu->pn_defn);
        if (pnu->pn_pos.begin >= pos.begin && pnu->pn_pos.end <= pos.end) {
            pnu->pn_dflags |= PND_DEOPTIMIZED;
            ++ndeoptimized;
        }
    }

    return ndeoptimized != 0;
}

void
Parser::setFunctionKinds(JSFunctionBox *funbox, uint32 *tcflags)
{
    for (;;) {
        JSParseNode *fn = funbox->node;
        JSParseNode *pn = fn->pn_body;

        if (funbox->kids) {
            setFunctionKinds(funbox->kids, tcflags);

            









            JSParseNode *pn2 = pn;
            if (PN_TYPE(pn2) == TOK_UPVARS)
                pn2 = pn2->pn_tree;
            if (PN_TYPE(pn2) == TOK_ARGSBODY)
                pn2 = pn2->last();

#if JS_HAS_EXPR_CLOSURES
            if (PN_TYPE(pn2) == TOK_LC)
#endif
            if (!(funbox->tcflags & TCF_RETURN_EXPR)) {
                uintN methodSets = 0, slowMethodSets = 0;

                for (JSParseNode *method = funbox->methods; method; method = method->pn_link) {
                    JS_ASSERT(PN_OP(method) == JSOP_LAMBDA || PN_OP(method) == JSOP_LAMBDA_FC);
                    ++methodSets;
                    if (!method->pn_funbox->joinable())
                        ++slowMethodSets;
                }

                if (funbox->shouldUnbrand(methodSets, slowMethodSets))
                    funbox->tcflags |= TCF_FUN_UNBRAND_THIS;
            }
        }

        JSFunction *fun = funbox->function();

        JS_ASSERT(FUN_KIND(fun) == JSFUN_INTERPRETED);

        if (funbox->tcflags & TCF_FUN_HEAVYWEIGHT) {
            
        } else if (funbox->inAnyDynamicScope()) {
            JS_ASSERT(!FUN_NULL_CLOSURE(fun));
        } else if (pn->pn_type != TOK_UPVARS) {
            















            FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
        } else {
            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            if (!fn->isFunArg()) {
                









                AtomDefnRange r = upvars->all();
                for (; !r.empty(); r.popFront()) {
                    JSDefinition *defn = r.front().value();
                    JSDefinition *lexdep = defn->resolve();

                    if (!lexdep->isFreeVar()) {
                        JS_ASSERT(lexdep->frameLevel() <= funbox->level);
                        break;
                    }
                }

                if (r.empty())
                    FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
            } else {
                uintN nupvars = 0, nflattened = 0;

                




                for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                    JSDefinition *defn = r.front().value();
                    JSDefinition *lexdep = defn->resolve();

                    if (!lexdep->isFreeVar()) {
                        ++nupvars;
                        if (CanFlattenUpvar(lexdep, funbox, *tcflags)) {
                            ++nflattened;
                            continue;
                        }

                        













                    }
                }

                if (nupvars == 0) {
                    FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
                } else if (nflattened == nupvars) {
                    



                    FUN_SET_KIND(fun, JSFUN_FLAT_CLOSURE);
                    switch (PN_OP(fn)) {
                      case JSOP_DEFFUN:
                        fn->pn_op = JSOP_DEFFUN_FC;
                        break;
                      case JSOP_DEFLOCALFUN:
                        fn->pn_op = JSOP_DEFLOCALFUN_FC;
                        break;
                      case JSOP_LAMBDA:
                        fn->pn_op = JSOP_LAMBDA_FC;
                        break;
                      default:
                        
                        JS_ASSERT(PN_OP(fn) == JSOP_NOP);
                    }
                }
            }
        }

        if (FUN_KIND(fun) == JSFUN_INTERPRETED && pn->pn_type == TOK_UPVARS) {
            








            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                JSDefinition *defn = r.front().value();
                JSDefinition *lexdep = defn->resolve();
                if (!lexdep->isFreeVar())
                    FlagHeavyweights(lexdep, funbox, tcflags);
            }
        }

        if (funbox->joinable())
            fun->setJoinable();

        funbox = funbox->siblings;
        if (!funbox)
            break;
    }
}












void
Parser::markExtensibleScopeDescendants(JSFunctionBox *funbox, bool hasExtensibleParent) 
{
    for (; funbox; funbox = funbox->siblings) {
        





        JS_ASSERT(!funbox->bindings.extensibleParents());
        if (hasExtensibleParent)
            funbox->bindings.setExtensibleParents();

        if (funbox->kids) {
            markExtensibleScopeDescendants(funbox->kids,
                                           hasExtensibleParent || funbox->scopeIsExtensible());
        }
    }
}

const char js_argument_str[] = "argument";
const char js_variable_str[] = "variable";
const char js_unknown_str[]  = "unknown";

const char *
JSDefinition::kindString(Kind kind)
{
    static const char *table[] = {
        js_var_str, js_const_str, js_let_str,
        js_function_str, js_argument_str, js_unknown_str
    };

    JS_ASSERT(unsigned(kind) <= unsigned(ARG));
    return table[kind];
}

static JSFunctionBox *
EnterFunction(JSParseNode *fn, JSTreeContext *funtc, JSAtom *funAtom = NULL,
              FunctionSyntaxKind kind = Expression)
{
    JSTreeContext *tc = funtc->parent;
    JSFunction *fun = tc->parser->newFunction(tc, funAtom, kind);
    if (!fun)
        return NULL;

    
    JSFunctionBox *funbox = tc->parser->newFunctionBox(FUN_OBJECT(fun), fn, tc);
    if (!funbox)
        return NULL;

    
    funtc->flags |= funbox->tcflags;
    funtc->blockidGen = tc->blockidGen;
    if (!GenerateBlockId(funtc, funtc->bodyid))
        return NULL;
    funtc->setFunction(fun);
    funtc->funbox = funbox;
    if (!SetStaticLevel(funtc, tc->staticLevel + 1))
        return NULL;

    return funbox;
}

static bool
LeaveFunction(JSParseNode *fn, JSTreeContext *funtc, JSAtom *funAtom = NULL,
              FunctionSyntaxKind kind = Expression)
{
    JSTreeContext *tc = funtc->parent;
    tc->blockidGen = funtc->blockidGen;

    JSFunctionBox *funbox = fn->pn_funbox;
    funbox->tcflags |= funtc->flags & (TCF_FUN_FLAGS | TCF_COMPILE_N_GO | TCF_RETURN_EXPR);

    fn->pn_dflags |= PND_INITIALIZED;
    if (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
        fn->pn_dflags |= PND_BLOCKCHILD;

    






    if (funtc->lexdeps->count()) {
        int foundCallee = 0;

        for (AtomDefnRange r = funtc->lexdeps->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key();
            JSDefinition *dn = r.front().value();
            JS_ASSERT(dn->isPlaceholder());

            if (atom == funAtom && kind == Expression) {
                dn->pn_op = JSOP_CALLEE;
                dn->pn_cookie.set(funtc->staticLevel, UpvarCookie::CALLEE_SLOT);
                dn->pn_dflags |= PND_BOUND;

                



                if (dn->isFunArg())
                    funbox->tcflags |= TCF_FUN_USES_OWN_NAME;
                foundCallee = 1;
                continue;
            }

            if (!(funbox->tcflags & TCF_FUN_SETS_OUTER_NAME) &&
                dn->isAssigned()) {
                





                for (JSParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
                    if (pnu->isAssigned() && pnu->pn_blockid >= funtc->bodyid) {
                        funbox->tcflags |= TCF_FUN_SETS_OUTER_NAME;
                        break;
                    }
                }
            }

            JSDefinition *outer_dn = tc->decls.lookupFirst(atom);

            



            if (funtc->callsEval() ||
                (outer_dn && tc->innermostWith &&
                 outer_dn->pn_pos < tc->innermostWith->pn_pos)) {
                DeoptimizeUsesWithin(dn, fn->pn_pos);
            }

            if (!outer_dn) {
                AtomDefnAddPtr p = tc->lexdeps->lookupForAdd(atom);
                if (p) {
                    outer_dn = p.value();
                } else {
                    




















                    outer_dn = MakePlaceholder(p, dn, tc);
                    if (!outer_dn)
                        return false;
                }
            }

            












            if (dn != outer_dn) {
                JSParseNode **pnup = &dn->dn_uses;
                JSParseNode *pnu;

                while ((pnu = *pnup) != NULL) {
                    pnu->pn_lexdef = outer_dn;
                    pnup = &pnu->pn_link;
                }

                





                *pnup = outer_dn->dn_uses;
                outer_dn->dn_uses = dn;
                outer_dn->pn_dflags |= dn->pn_dflags & ~PND_PLACEHOLDER;
                dn->pn_defn = false;
                dn->pn_used = true;
                dn->pn_lexdef = outer_dn;
            }

            
            outer_dn->pn_dflags |= PND_CLOSED;
        }

        if (funtc->lexdeps->count() - foundCallee != 0) {
            JSParseNode *body = fn->pn_body;

            fn->pn_body = NameSetNode::create(tc);
            if (!fn->pn_body)
                return false;

            fn->pn_body->pn_type = TOK_UPVARS;
            fn->pn_body->pn_pos = body->pn_pos;
            if (foundCallee)
                funtc->lexdeps->remove(funAtom);
            
            fn->pn_body->pn_names = funtc->lexdeps;
            funtc->lexdeps.clearMap();
            fn->pn_body->pn_tree = body;
        } else {
            funtc->lexdeps.releaseMap(funtc->parser->context);
        }

    }

    






    if (funtc->inStrictMode() && funbox->object->getFunctionPrivate()->nargs > 0) {
        AtomDeclsIter iter(&funtc->decls);
        JSDefinition *dn;

        while ((dn = iter()) != NULL) {
            if (dn->kind() == JSDefinition::ARG && dn->isAssigned()) {
                funbox->tcflags |= TCF_FUN_MUTATES_PARAMETER;
                break;
            }
        }
    }

    funbox->bindings.transfer(funtc->parser->context, &funtc->bindings);

    return true;
}

static bool
DefineGlobal(JSParseNode *pn, JSCodeGenerator *cg, JSAtom *atom);







bool
Parser::functionArguments(JSTreeContext &funtc, JSFunctionBox *funbox, JSParseNode **listp)
{
    if (tokenStream.getToken() != TOK_LP) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_PAREN_BEFORE_FORMAL);
        return false;
    }

    if (!tokenStream.matchToken(TOK_RP)) {
#if JS_HAS_DESTRUCTURING
        JSAtom *duplicatedArg = NULL;
        bool destructuringArg = false;
        JSParseNode *list = NULL;
#endif

        do {
            switch (TokenKind tt = tokenStream.getToken()) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
              {
                
                if (duplicatedArg)
                    goto report_dup_and_destructuring;
                destructuringArg = true;

                





                BindData data;
                data.pn = NULL;
                data.op = JSOP_DEFVAR;
                data.binder = BindDestructuringArg;
                JSParseNode *lhs = destructuringExpr(&data, tt);
                if (!lhs)
                    return false;

                



                uint16 slot;
                if (!funtc.bindings.addDestructuring(context, &slot))
                    return false;

                




                JSParseNode *rhs = NameNode::create(context->runtime->atomState.emptyAtom, &funtc);
                if (!rhs)
                    return false;
                rhs->pn_type = TOK_NAME;
                rhs->pn_op = JSOP_GETARG;
                rhs->pn_cookie.set(funtc.staticLevel, slot);
                rhs->pn_dflags |= PND_BOUND;

                JSParseNode *item =
                    JSParseNode::newBinaryOrAppend(TOK_ASSIGN, JSOP_NOP, lhs, rhs, &funtc);
                if (!item)
                    return false;
                if (!list) {
                    list = ListNode::create(&funtc);
                    if (!list)
                        return false;
                    list->pn_type = TOK_VAR;
                    list->makeEmpty();
                    *listp = list;
                }
                list->append(item);
                break;
              }
#endif 

              case TOK_NAME:
              {
                JSAtom *atom = tokenStream.currentToken().t_atom;

#ifdef JS_HAS_DESTRUCTURING
                















                if (funtc.decls.lookupFirst(atom)) {
                    duplicatedArg = atom;
                    if (destructuringArg)
                        goto report_dup_and_destructuring;
                }
#endif

                uint16 slot;
                if (!funtc.bindings.addArgument(context, atom, &slot))
                    return false;
                if (!DefineArg(funbox->node, atom, slot, &funtc))
                    return false;
                break;
              }

              default:
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_MISSING_FORMAL);
                
              case TOK_ERROR:
                return false;

#if JS_HAS_DESTRUCTURING
              report_dup_and_destructuring:
                JSDefinition *dn = funtc.decls.lookupFirst(duplicatedArg);
                reportErrorNumber(dn, JSREPORT_ERROR, JSMSG_DESTRUCT_DUP_ARG);
                return false;
#endif
            }
        } while (tokenStream.matchToken(TOK_COMMA));

        if (tokenStream.getToken() != TOK_RP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_PAREN_AFTER_FORMAL);
            return false;
        }
    }

    return true;
}

JSParseNode *
Parser::functionDef(JSAtom *funAtom, FunctionType type, FunctionSyntaxKind kind)
{
    JS_ASSERT_IF(kind == Statement, funAtom);

    
    tokenStream.mungeCurrentToken(TOK_FUNCTION, JSOP_NOP);
    JSParseNode *pn = FunctionNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_body = NULL;
    pn->pn_cookie.makeFree();

    








    bool bodyLevel = tc->atBodyLevel();
    pn->pn_dflags = (kind == Expression || !bodyLevel) ? PND_FUNARG : 0;

    



    if (kind == Statement) {
        if (JSDefinition *dn = tc->decls.lookupFirst(funAtom)) {
            JSDefinition::Kind dn_kind = dn->kind();

            JS_ASSERT(!dn->pn_used);
            JS_ASSERT(dn->pn_defn);

            if (context->hasStrictOption() || dn_kind == JSDefinition::CONST) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(context, funAtom, &name) ||
                    !reportErrorNumber(NULL,
                                       (dn_kind != JSDefinition::CONST)
                                       ? JSREPORT_WARNING | JSREPORT_STRICT
                                       : JSREPORT_ERROR,
                                       JSMSG_REDECLARED_VAR,
                                       JSDefinition::kindString(dn_kind),
                                       name.ptr())) {
                    return NULL;
                }
            }

            if (bodyLevel) {
                tc->decls.update(funAtom, (JSDefinition *) pn);
                pn->pn_defn = true;
                pn->dn_uses = dn;               

                if (!MakeDefIntoUse(dn, pn, funAtom, tc))
                    return NULL;
            }
        } else if (bodyLevel) {
            





            if (JSDefinition *fn = tc->lexdeps.lookupDefn(funAtom)) {
                JS_ASSERT(fn->pn_defn);
                fn->pn_type = TOK_FUNCTION;
                fn->pn_arity = PN_FUNC;
                fn->pn_pos.begin = pn->pn_pos.begin;

                



                fn->pn_pos.end = pn->pn_pos.end;

                fn->pn_body = NULL;
                fn->pn_cookie.makeFree();

                tc->lexdeps->remove(funAtom);
                RecycleTree(pn, tc);
                pn = fn;
            }

            if (!Define(pn, funAtom, tc))
                return NULL;
        }

        







        if (bodyLevel && tc->inFunction()) {
            






            uintN index;
            switch (tc->bindings.lookup(context, funAtom, &index)) {
              case NONE:
              case ARGUMENT:
                index = tc->bindings.countVars();
                if (!tc->bindings.addVariable(context, funAtom))
                    return NULL;
                

              case VARIABLE:
                pn->pn_cookie.set(tc->staticLevel, index);
                pn->pn_dflags |= PND_BOUND;
                break;

              default:;
            }
        }
    }

    JSTreeContext *outertc = tc;

    
    JSTreeContext funtc(tc->parser);
    if (!funtc.init(context))
        return NULL;

    JSFunctionBox *funbox = EnterFunction(pn, &funtc, funAtom, kind);
    if (!funbox)
        return NULL;

    JSFunction *fun = funbox->function();

    
    JSParseNode *prelude = NULL;
    if (!functionArguments(funtc, funbox, &prelude))
        return NULL;

    fun->setArgCount(funtc.bindings.countArgs());

#if JS_HAS_DESTRUCTURING
    







    if (prelude) {
        AtomDeclsIter iter(&funtc.decls);

        while (JSDefinition *apn = iter()) {
            
            if (apn->pn_op != JSOP_SETLOCAL)
                continue;

            uint16 index = funtc.bindings.countVars();
            if (!BindLocalVariable(context, &funtc, apn->pn_atom, VARIABLE, true))
                return NULL;
            apn->pn_cookie.set(funtc.staticLevel, index);
        }
    }
#endif

    if (type == Getter && fun->nargs > 0) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_ACCESSOR_WRONG_ARGS,
                          "getter", "no", "s");
        return NULL;
    }
    if (type == Setter && fun->nargs != 1) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_ACCESSOR_WRONG_ARGS,
                          "setter", "one", "");
        return NULL;
    }

#if JS_HAS_EXPR_CLOSURES
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    if (tt != TOK_LC) {
        tokenStream.ungetToken();
        fun->flags |= JSFUN_EXPR_CLOSURE;
    }
#else
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_BODY);
#endif

    JSParseNode *body = functionBody();
    if (!body)
        return NULL;

    if (funAtom && !CheckStrictBinding(context, &funtc, funAtom, pn))
        return NULL;

    if (!CheckStrictParameters(context, &funtc))
        return NULL;

#if JS_HAS_EXPR_CLOSURES
    if (tt == TOK_LC)
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
    else if (kind == Statement && !MatchOrInsertSemicolon(context, &tokenStream))
        return NULL;
#else
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
#endif
    pn->pn_pos.end = tokenStream.currentToken().pos.end;

    












    if (funtc.callsEval())
        outertc->noteCallsEval();

#if JS_HAS_DESTRUCTURING
    





    if (prelude) {
        if (body->pn_arity != PN_LIST) {
            JSParseNode *block;

            block = ListNode::create(outertc);
            if (!block)
                return NULL;
            block->pn_type = TOK_SEQ;
            block->pn_pos = body->pn_pos;
            block->initList(body);

            body = block;
        }

        JSParseNode *item = UnaryNode::create(outertc);
        if (!item)
            return NULL;

        item->pn_type = TOK_SEMI;
        item->pn_pos.begin = item->pn_pos.end = body->pn_pos.begin;
        item->pn_kid = prelude;
        item->pn_next = body->pn_head;
        body->pn_head = item;
        if (body->pn_tail == &body->pn_head)
            body->pn_tail = &item->pn_next;
        ++body->pn_count;
        body->pn_xflags |= PNX_DESTRUCT;
    }
#endif

    





    if (funtc.flags & TCF_FUN_HEAVYWEIGHT) {
        fun->flags |= JSFUN_HEAVYWEIGHT;
        outertc->flags |= TCF_FUN_HEAVYWEIGHT;
    } else {
        





        if (!bodyLevel && kind == Statement)
            outertc->flags |= TCF_FUN_HEAVYWEIGHT;
    }

    JSOp op = JSOP_NOP;
    if (kind == Expression) {
        op = JSOP_LAMBDA;
    } else {
        if (!bodyLevel) {
            





            JS_ASSERT(!outertc->inStrictMode());
            op = JSOP_DEFFUN;
            outertc->noteMightAliasLocals();
        }
    }

    funbox->kids = funtc.functionList;

    pn->pn_funbox = funbox;
    pn->pn_op = op;
    if (pn->pn_body) {
        pn->pn_body->append(body);
        pn->pn_body->pn_pos = body->pn_pos;
    } else {
        pn->pn_body = body;
    }

    if (!outertc->inFunction() && bodyLevel && kind == Statement && outertc->compiling()) {
        JS_ASSERT(pn->pn_cookie.isFree());
        if (!DefineGlobal(pn, outertc->asCodeGenerator(), funAtom))
            return NULL;
    }

    pn->pn_blockid = outertc->blockid();

    if (!LeaveFunction(pn, &funtc, funAtom, kind))
        return NULL;

    
    if (!outertc->inStrictMode())
        tokenStream.setStrictMode(false);

    return pn;
}

JSParseNode *
Parser::functionStmt()
{
    JSAtom *name = NULL;
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME) {
        name = tokenStream.currentToken().t_atom;
    } else {
        
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_UNNAMED_FUNCTION_STMT);
        return NULL;
    }

    
    if (!tc->atBodyLevel() && tc->inStrictMode()) {
        reportErrorNumber(NULL, JSREPORT_STRICT_MODE_ERROR, JSMSG_STRICT_FUNCTION_STATEMENT);
        return NULL;
    }

    return functionDef(name, Normal, Statement);
}

JSParseNode *
Parser::functionExpr()
{
    JSAtom *name = NULL;
    if (tokenStream.getToken(TSF_KEYWORD_IS_NAME) == TOK_NAME)
        name = tokenStream.currentToken().t_atom;
    else
        tokenStream.ungetToken();
    return functionDef(name, Normal, Expression);
}




















bool
Parser::recognizeDirectivePrologue(JSParseNode *pn, bool *isDirectivePrologueMember)
{
    *isDirectivePrologueMember = pn->isStringExprStatement();
    if (!*isDirectivePrologueMember)
        return true;

    JSParseNode *kid = pn->pn_kid;
    if (kid->isEscapeFreeStringLiteral()) {
        











        pn->pn_prologue = true;

        JSAtom *directive = kid->pn_atom;
        if (directive == context->runtime->atomState.useStrictAtom) {
            













            if (tokenStream.hasOctalCharacterEscape()) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_DEPRECATED_OCTAL);
                return false;
            }

            tc->flags |= TCF_STRICT_MODE_CODE;
            tokenStream.setStrictMode();
        }
    }
    return true;
}






JSParseNode *
Parser::statements()
{
    JSParseNode *pn, *pn2, *saveBlock;
    TokenKind tt;

    JS_CHECK_RECURSION(context, return NULL);

    pn = ListNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_type = TOK_LC;
    pn->makeEmpty();
    pn->pn_blockid = tc->blockid();
    saveBlock = tc->blockNode;
    tc->blockNode = pn;

    bool inDirectivePrologue = tc->atBodyLevel();
    tokenStream.setOctalCharacterEscape(false);
    for (;;) {
        tt = tokenStream.peekToken(TSF_OPERAND);
        if (tt <= TOK_EOF || tt == TOK_RC) {
            if (tt == TOK_ERROR) {
                if (tokenStream.isEOF())
                    tokenStream.setUnexpectedEOF();
                return NULL;
            }
            break;
        }
        pn2 = statement();
        if (!pn2) {
            if (tokenStream.isEOF())
                tokenStream.setUnexpectedEOF();
            return NULL;
        }

        if (inDirectivePrologue && !recognizeDirectivePrologue(pn2, &inDirectivePrologue))
            return NULL;

        if (pn2->pn_type == TOK_FUNCTION) {
            








            if (tc->atBodyLevel())
                pn->pn_xflags |= PNX_FUNCDEFS;
            else {
                tc->flags |= TCF_HAS_FUNCTION_STMT;
                
                tc->noteHasExtensibleScope();
            }
        }
        pn->append(pn2);
    }

    




    if (tc->blockNode != pn)
        pn = tc->blockNode;
    tc->blockNode = saveBlock;

    pn->pn_pos.end = tokenStream.currentToken().pos.end;
    return pn;
}

JSParseNode *
Parser::condition()
{
    JSParseNode *pn;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_COND);
    pn = parenExpr();
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    if (pn->pn_type == TOK_ASSIGN &&
        pn->pn_op == JSOP_NOP &&
        !pn->pn_parens &&
        !reportErrorNumber(NULL, JSREPORT_WARNING | JSREPORT_STRICT, JSMSG_EQUAL_AS_ASSIGN, "")) {
        return NULL;
    }
    return pn;
}

static JSBool
MatchLabel(JSContext *cx, TokenStream *ts, JSParseNode *pn)
{
    JSAtom *label;
    TokenKind tt;

    tt = ts->peekTokenSameLine(TSF_OPERAND);
    if (tt == TOK_ERROR)
        return JS_FALSE;
    if (tt == TOK_NAME) {
        (void) ts->getToken();
        label = ts->currentToken().t_atom;
    } else {
        label = NULL;
    }
    pn->pn_atom = label;
    return JS_TRUE;
}

static JSBool
BindLet(JSContext *cx, BindData *data, JSAtom *atom, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSObject *blockObj;
    jsint n;

    



    JS_ASSERT(!tc->atBodyLevel());

    pn = data->pn;
    if (!CheckStrictBinding(cx, tc, atom, pn))
        return false;

    blockObj = tc->blockChain();
    JSDefinition *dn = tc->decls.lookupFirst(atom);
    if (dn && dn->pn_blockid == tc->blockid()) {
        JSAutoByteString name;
        if (js_AtomToPrintableString(cx, atom, &name)) {
            ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                     JSREPORT_ERROR, JSMSG_REDECLARED_VAR,
                                     dn->isConst() ? js_const_str : js_variable_str,
                                     name.ptr());
        }
        return false;
    }

    n = OBJ_BLOCK_COUNT(cx, blockObj);
    if (n == JS_BIT(16)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, data->let.overflow);
        return false;
    }

    



    if (!Define(pn, atom, tc, true))
        return false;

    






    pn->pn_op = JSOP_GETLOCAL;
    pn->pn_cookie.set(tc->staticLevel, uint16(n));
    pn->pn_dflags |= PND_LET | PND_BOUND;

    



    const Shape *shape = blockObj->defineBlockVariable(cx, ATOM_TO_JSID(atom), n);
    if (!shape)
        return false;

    






    blockObj->setSlot(shape->slot, PrivateValue(pn));
    return true;
}

static void
PopStatement(JSTreeContext *tc)
{
    JSStmtInfo *stmt = tc->topStmt;

    if (stmt->flags & SIF_SCOPE) {
        JSObject *obj = stmt->blockBox->object;
        JS_ASSERT(!obj->isClonedBlock());

        for (Shape::Range r = obj->lastProperty()->all(); !r.empty(); r.popFront()) {
            JSAtom *atom = JSID_TO_ATOM(r.front().propid);

            
            if (atom == tc->parser->context->runtime->atomState.emptyAtom)
                continue;
            tc->decls.remove(atom);
        }
    }
    js_PopStatement(tc);
}

static inline bool
OuterLet(JSTreeContext *tc, JSStmtInfo *stmt, JSAtom *atom)
{
    while (stmt->downScope) {
        stmt = js_LexicalLookup(tc, atom, NULL, stmt->downScope);
        if (!stmt)
            return false;
        if (stmt->type == STMT_BLOCK)
            return true;
    }
    return false;
}
















static bool
DefineGlobal(JSParseNode *pn, JSCodeGenerator *cg, JSAtom *atom)
{
    GlobalScope *globalScope = cg->compiler()->globalScope;
    JSObject *globalObj = globalScope->globalObj;

    if (!cg->compileAndGo() || !globalObj || cg->compilingForEval())
        return true;

    AtomIndexAddPtr p = globalScope->names.lookupForAdd(atom);
    if (!p) {
        JSContext *cx = cg->parser->context;

        JSObject *holder;
        JSProperty *prop;
        if (!globalObj->lookupProperty(cx, ATOM_TO_JSID(atom), &holder, &prop))
            return false;

        JSFunctionBox *funbox = (pn->pn_type == TOK_FUNCTION) ? pn->pn_funbox : NULL;

        GlobalScope::GlobalDef def;
        if (prop) {
            





            const Shape *shape = (const Shape *)prop;
            if (funbox ||
                globalObj != holder ||
                shape->configurable() ||
                !shape->hasSlot() ||
                !shape->hasDefaultGetterOrIsMethod() ||
                !shape->hasDefaultSetter()) {
                return true;
            }
            
            def = GlobalScope::GlobalDef(shape->slot);
        } else {
            def = GlobalScope::GlobalDef(atom, funbox);
        }

        if (!globalScope->defs.append(def))
            return false;

        jsatomid index = globalScope->names.count();
        if (!globalScope->names.add(p, atom, index))
            return false;

        JS_ASSERT(index == globalScope->defs.length() - 1);
    } else {
        












        if (pn->pn_type == TOK_FUNCTION) {
            JS_ASSERT(pn->pn_arity = PN_FUNC);
            jsatomid index = p.value();
            globalScope->defs[index].funbox = pn->pn_funbox;
        }
    }

    pn->pn_dflags |= PND_GVAR;

    return true;
}

static bool
BindTopLevelVar(JSContext *cx, BindData *data, JSParseNode *pn, JSAtom *varname, JSTreeContext *tc)
{
    JS_ASSERT(pn->pn_op == JSOP_NAME);
    JS_ASSERT(!tc->inFunction());

    
    if (!tc->compiling())
        return true;

    



    if (tc->parser->callerFrame) {
        





        if (!tc->inStrictMode())
            return true;

        














        return true;
    }

    if (pn->pn_dflags & PND_CONST)
        return true;

    




    return DefineGlobal(pn, tc->asCodeGenerator(), pn->pn_atom);
}

static bool
BindFunctionLocal(JSContext *cx, BindData *data, MultiDeclRange &mdl, JSParseNode *pn,
                  JSAtom *name, JSTreeContext *tc)
{
    JS_ASSERT(tc->inFunction());

    if (name == cx->runtime->atomState.argumentsAtom) {
        pn->pn_op = JSOP_ARGUMENTS;
        pn->pn_dflags |= PND_BOUND;
        return true;
    }

    BindingKind kind = tc->bindings.lookup(cx, name, NULL);
    if (kind == NONE) {
        






        kind = (data->op == JSOP_DEFCONST) ? CONSTANT : VARIABLE;

        uintN index = tc->bindings.countVars();
        if (!BindLocalVariable(cx, tc, name, kind, false))
            return false;
        pn->pn_op = JSOP_GETLOCAL;
        pn->pn_cookie.set(tc->staticLevel, index);
        pn->pn_dflags |= PND_BOUND;
        return true;
    }

    if (kind == ARGUMENT) {
        JS_ASSERT(tc->inFunction());
        JS_ASSERT(!mdl.empty() && mdl.front()->kind() == JSDefinition::ARG);
    } else {
        JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
    }

    return true;
}

static JSBool
BindVarOrConst(JSContext *cx, BindData *data, JSAtom *atom, JSTreeContext *tc)
{
    JSParseNode *pn = data->pn;

    
    pn->pn_op = JSOP_NAME;

    if (!CheckStrictBinding(cx, tc, atom, pn))
        return false;

    JSStmtInfo *stmt = js_LexicalLookup(tc, atom, NULL);

    if (stmt && stmt->type == STMT_WITH) {
        data->fresh = false;
        pn->pn_dflags |= PND_DEOPTIMIZED;
        tc->noteMightAliasLocals();
        return true;
    }

    MultiDeclRange mdl = tc->decls.lookupMulti(atom);
    JSOp op = data->op;

    if (stmt || !mdl.empty()) {
        JSDefinition *dn = mdl.empty() ? NULL : mdl.front();
        JSDefinition::Kind dn_kind = dn ? dn->kind() : JSDefinition::VAR;

        if (dn_kind == JSDefinition::ARG) {
            JSAutoByteString name;
            if (!js_AtomToPrintableString(cx, atom, &name))
                return JS_FALSE;

            if (op == JSOP_DEFCONST) {
                ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                         JSREPORT_ERROR, JSMSG_REDECLARED_PARAM,
                                         name.ptr());
                return JS_FALSE;
            }
            if (!ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                          JSREPORT_WARNING | JSREPORT_STRICT,
                                          JSMSG_VAR_HIDES_ARG, name.ptr())) {
                return JS_FALSE;
            }
        } else {
            bool error = (op == JSOP_DEFCONST ||
                          dn_kind == JSDefinition::CONST ||
                          (dn_kind == JSDefinition::LET &&
                           (stmt->type != STMT_CATCH || OuterLet(tc, stmt, atom))));

            if (cx->hasStrictOption()
                ? op != JSOP_DEFVAR || dn_kind != JSDefinition::VAR
                : error) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(cx, atom, &name) ||
                    !ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                              !error
                                              ? JSREPORT_WARNING | JSREPORT_STRICT
                                              : JSREPORT_ERROR,
                                              JSMSG_REDECLARED_VAR,
                                              JSDefinition::kindString(dn_kind),
                                              name.ptr())) {
                    return JS_FALSE;
                }
            }
        }
    }

    if (mdl.empty()) {
        if (!Define(pn, atom, tc))
            return JS_FALSE;
    } else {
        










        JSDefinition *dn = mdl.front();

        data->fresh = false;

        if (!pn->pn_used) {
            
            JSParseNode *pnu = pn;

            if (pn->pn_defn) {
                pnu = NameNode::create(atom, tc);
                if (!pnu)
                    return JS_FALSE;
            }

            LinkUseToDef(pnu, dn, tc);
            pnu->pn_op = JSOP_NAME;
        }

        
        while (dn->kind() == JSDefinition::LET) {
            mdl.popFront();
            if (mdl.empty())
                break;
            dn = mdl.front();
        }

        if (dn) {
            JS_ASSERT_IF(data->op == JSOP_DEFCONST,
                         dn->kind() == JSDefinition::CONST);
            return JS_TRUE;
        }

        




        if (!pn->pn_defn) {
            if (tc->lexdeps->lookup(atom)) {
                tc->lexdeps->remove(atom);
            } else {
                JSParseNode *pn2 = NameNode::create(atom, tc);
                if (!pn2)
                    return JS_FALSE;

                
                pn2->pn_type = TOK_NAME;
                pn2->pn_pos = pn->pn_pos;
                pn = pn2;
            }
            pn->pn_op = JSOP_NAME;
        }

        if (!tc->decls.addHoist(atom, (JSDefinition *) pn))
            return JS_FALSE;
        pn->pn_defn = true;
        pn->pn_dflags &= ~PND_PLACEHOLDER;
    }

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    if (tc->inFunction())
        return BindFunctionLocal(cx, data, mdl, pn, atom, tc);

    return BindTopLevelVar(cx, data, pn, atom, tc);
}

static bool
MakeSetCall(JSContext *cx, JSParseNode *pn, JSTreeContext *tc, uintN msg)
{
    JS_ASSERT(pn->pn_arity == PN_LIST);
    JS_ASSERT(pn->pn_op == JSOP_CALL || pn->pn_op == JSOP_EVAL ||
              pn->pn_op == JSOP_FUNCALL || pn->pn_op == JSOP_FUNAPPLY);
    if (!ReportStrictModeError(cx, TS(tc->parser), tc, pn, msg))
        return false;

    JSParseNode *pn2 = pn->pn_head;
    if (pn2->pn_type == TOK_FUNCTION && (pn2->pn_funbox->tcflags & TCF_GENEXP_LAMBDA)) {
        ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR, msg);
        return false;
    }
    pn->pn_xflags |= PNX_SETCALL;
    return true;
}

static void
NoteLValue(JSContext *cx, JSParseNode *pn, JSTreeContext *tc, uintN dflag = PND_ASSIGNED)
{
    if (pn->pn_used) {
        JSDefinition *dn = pn->pn_lexdef;

        



        if (!(dn->pn_dflags & (PND_INITIALIZED | PND_CONST | PND_PLACEHOLDER)) &&
            dn->isBlockChild() &&
            pn->isBlockChild() &&
            dn->pn_blockid == pn->pn_blockid &&
            dn->pn_pos.end <= pn->pn_pos.begin &&
            dn->dn_uses == pn) {
            dflag = PND_INITIALIZED;
        }

        dn->pn_dflags |= dflag;

        if (dn->pn_cookie.isFree() || dn->frameLevel() < tc->staticLevel)
            tc->flags |= TCF_FUN_SETS_OUTER_NAME;
    }

    pn->pn_dflags |= dflag;

    








    JSAtom *lname = pn->pn_atom;
    if (lname == cx->runtime->atomState.argumentsAtom) {
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        tc->countArgumentsUse(pn);
    } else if (tc->inFunction() && lname == tc->fun()->atom) {
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
    }
}

#if JS_HAS_DESTRUCTURING

static JSBool
BindDestructuringVar(JSContext *cx, BindData *data, JSParseNode *pn,
                     JSTreeContext *tc)
{
    JSAtom *atom;

    




    JS_ASSERT(pn->pn_type == TOK_NAME);
    atom = pn->pn_atom;
    if (atom == cx->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_HEAVYWEIGHT;

    data->pn = pn;
    if (!data->binder(cx, data, atom, tc))
        return JS_FALSE;

    



    if (pn->pn_dflags & PND_BOUND) {
        JS_ASSERT(!(pn->pn_dflags & PND_GVAR));
        pn->pn_op = (pn->pn_op == JSOP_ARGUMENTS)
                    ? JSOP_SETNAME
                    : JSOP_SETLOCAL;
    } else {
        pn->pn_op = (data->op == JSOP_DEFCONST)
                    ? JSOP_SETCONST
                    : JSOP_SETNAME;
    }

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    NoteLValue(cx, pn, tc, PND_INITIALIZED);
    return JS_TRUE;
}



















static JSBool
BindDestructuringLHS(JSContext *cx, JSParseNode *pn, JSTreeContext *tc)
{
    switch (pn->pn_type) {
      case TOK_NAME:
        NoteLValue(cx, pn, tc);
        

      case TOK_DOT:
      case TOK_LB:
        




        if (!(js_CodeSpec[pn->pn_op].format & JOF_SET))
            pn->pn_op = JSOP_SETNAME;
        break;

      case TOK_LP:
        if (!MakeSetCall(cx, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        if (pn->pn_op == JSOP_XMLNAME) {
            pn->pn_op = JSOP_BINDXMLNAME;
            break;
        }
        
#endif

      default:
        ReportCompileErrorNumber(cx, TS(tc->parser), pn,
                                 JSREPORT_ERROR, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return JS_FALSE;
    }

    return JS_TRUE;
}









































static bool
CheckDestructuring(JSContext *cx, BindData *data, JSParseNode *left, JSTreeContext *tc)
{
    bool ok;

    if (left->pn_type == TOK_ARRAYCOMP) {
        ReportCompileErrorNumber(cx, TS(tc->parser), left, JSREPORT_ERROR,
                                 JSMSG_ARRAY_COMP_LEFTSIDE);
        return false;
    }

    if (left->pn_type == TOK_RB) {
        for (JSParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (pn->pn_type != TOK_COMMA || pn->pn_arity != PN_NULLARY) {
                if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC) {
                    ok = CheckDestructuring(cx, data, pn, tc);
                } else {
                    if (data) {
                        if (pn->pn_type != TOK_NAME) {
                            ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR,
                                                     JSMSG_NO_VARIABLE_NAME);
                            return false;
                        }
                        ok = BindDestructuringVar(cx, data, pn, tc);
                    } else {
                        ok = BindDestructuringLHS(cx, pn, tc);
                    }
                }
                if (!ok)
                    return false;
            }
        }
    } else {
        JS_ASSERT(left->pn_type == TOK_RC);
        for (JSParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->pn_type == TOK_COLON);
            JSParseNode *pn = pair->pn_right;

            if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC) {
                ok = CheckDestructuring(cx, data, pn, tc);
            } else if (data) {
                if (pn->pn_type != TOK_NAME) {
                    ReportCompileErrorNumber(cx, TS(tc->parser), pn, JSREPORT_ERROR,
                                             JSMSG_NO_VARIABLE_NAME);
                    return false;
                }
                ok = BindDestructuringVar(cx, data, pn, tc);
            } else {
                ok = BindDestructuringLHS(cx, pn, tc);
            }
            if (!ok)
                return false;
        }
    }

    

















    if (data &&
        data->binder == BindLet &&
        OBJ_BLOCK_COUNT(cx, tc->blockChain()) == 0 &&
        !DefineNativeProperty(cx, tc->blockChain(),
                              ATOM_TO_JSID(cx->runtime->atomState.emptyAtom),
                              UndefinedValue(), NULL, NULL,
                              JSPROP_ENUMERATE | JSPROP_PERMANENT,
                              Shape::HAS_SHORTID, 0)) {
        return false;
    }

    return true;
}


















static void
UndominateInitializers(JSParseNode *left, const TokenPtr &end, JSTreeContext *tc)
{
    if (left->pn_type == TOK_RB) {
        for (JSParseNode *pn = left->pn_head; pn; pn = pn->pn_next) {
            
            if (pn->pn_type != TOK_COMMA || pn->pn_arity != PN_NULLARY) {
                if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC)
                    UndominateInitializers(pn, end, tc);
                else
                    pn->pn_pos.end = end;
            }
        }
    } else {
        JS_ASSERT(left->pn_type == TOK_RC);

        for (JSParseNode *pair = left->pn_head; pair; pair = pair->pn_next) {
            JS_ASSERT(pair->pn_type == TOK_COLON);
            JSParseNode *pn = pair->pn_right;
            if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC)
                UndominateInitializers(pn, end, tc);
            else
                pn->pn_pos.end = end;
        }
    }
}

JSParseNode *
Parser::destructuringExpr(BindData *data, TokenKind tt)
{
    JSParseNode *pn;

    tc->flags |= TCF_DECL_DESTRUCTURING;
    pn = primaryExpr(tt, JS_FALSE);
    tc->flags &= ~TCF_DECL_DESTRUCTURING;
    if (!pn)
        return NULL;
    if (!CheckDestructuring(context, data, pn, tc))
        return NULL;
    return pn;
}






static JSParseNode *
CloneParseTree(JSParseNode *opn, JSTreeContext *tc)
{
    JSParseNode *pn, *pn2, *opn2;

    pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    pn->pn_type = opn->pn_type;
    pn->pn_pos = opn->pn_pos;
    pn->pn_op = opn->pn_op;
    pn->pn_used = opn->pn_used;
    pn->pn_defn = opn->pn_defn;
    pn->pn_arity = opn->pn_arity;
    pn->pn_parens = opn->pn_parens;

    switch (pn->pn_arity) {
#define NULLCHECK(e)    JS_BEGIN_MACRO if (!(e)) return NULL; JS_END_MACRO

      case PN_FUNC:
        NULLCHECK(pn->pn_funbox =
                  tc->parser->newFunctionBox(opn->pn_funbox->object, pn, tc));
        NULLCHECK(pn->pn_body = CloneParseTree(opn->pn_body, tc));
        pn->pn_cookie = opn->pn_cookie;
        pn->pn_dflags = opn->pn_dflags;
        pn->pn_blockid = opn->pn_blockid;
        break;

      case PN_LIST:
        pn->makeEmpty();
        for (opn2 = opn->pn_head; opn2; opn2 = opn2->pn_next) {
            NULLCHECK(pn2 = CloneParseTree(opn2, tc));
            pn->append(pn2);
        }
        pn->pn_xflags = opn->pn_xflags;
        break;

      case PN_TERNARY:
        NULLCHECK(pn->pn_kid1 = CloneParseTree(opn->pn_kid1, tc));
        NULLCHECK(pn->pn_kid2 = CloneParseTree(opn->pn_kid2, tc));
        NULLCHECK(pn->pn_kid3 = CloneParseTree(opn->pn_kid3, tc));
        break;

      case PN_BINARY:
        NULLCHECK(pn->pn_left = CloneParseTree(opn->pn_left, tc));
        if (opn->pn_right != opn->pn_left)
            NULLCHECK(pn->pn_right = CloneParseTree(opn->pn_right, tc));
        else
            pn->pn_right = pn->pn_left;
        pn->pn_pval = opn->pn_pval;
        pn->pn_iflags = opn->pn_iflags;
        break;

      case PN_UNARY:
        NULLCHECK(pn->pn_kid = CloneParseTree(opn->pn_kid, tc));
        pn->pn_num = opn->pn_num;
        pn->pn_hidden = opn->pn_hidden;
        break;

      case PN_NAME:
        
        pn->pn_u = opn->pn_u;
        if (opn->pn_used) {
            



            JSDefinition *dn = pn->pn_lexdef;

            pn->pn_link = dn->dn_uses;
            dn->dn_uses = pn;
        } else if (opn->pn_expr) {
            NULLCHECK(pn->pn_expr = CloneParseTree(opn->pn_expr, tc));

            



            if (opn->pn_defn) {
                opn->pn_defn = false;
                LinkUseToDef(opn, (JSDefinition *) pn, tc);
            }
        }
        break;

      case PN_NAMESET:
        pn->pn_names = opn->pn_names;
        NULLCHECK(pn->pn_tree = CloneParseTree(opn->pn_tree, tc));
        break;

      case PN_NULLARY:
        
        pn->pn_u = opn->pn_u;
        break;

#undef NULLCHECK
    }
    return pn;
}

#endif 

extern const char js_with_statement_str[];

static JSParseNode *
ContainsStmt(JSParseNode *pn, TokenKind tt)
{
    JSParseNode *pn2, *pnt;

    if (!pn)
        return NULL;
    if (PN_TYPE(pn) == tt)
        return pn;
    switch (pn->pn_arity) {
      case PN_LIST:
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            pnt = ContainsStmt(pn2, tt);
            if (pnt)
                return pnt;
        }
        break;
      case PN_TERNARY:
        pnt = ContainsStmt(pn->pn_kid1, tt);
        if (pnt)
            return pnt;
        pnt = ContainsStmt(pn->pn_kid2, tt);
        if (pnt)
            return pnt;
        return ContainsStmt(pn->pn_kid3, tt);
      case PN_BINARY:
        



        if (pn->pn_op != JSOP_NOP)
            return NULL;
        pnt = ContainsStmt(pn->pn_left, tt);
        if (pnt)
            return pnt;
        return ContainsStmt(pn->pn_right, tt);
      case PN_UNARY:
        if (pn->pn_op != JSOP_NOP)
            return NULL;
        return ContainsStmt(pn->pn_kid, tt);
      case PN_NAME:
        return ContainsStmt(pn->maybeExpr(), tt);
      case PN_NAMESET:
        return ContainsStmt(pn->pn_tree, tt);
      default:;
    }
    return NULL;
}

JSParseNode *
Parser::returnOrYield(bool useAssignExpr)
{
    TokenKind tt, tt2;
    JSParseNode *pn, *pn2;

    tt = tokenStream.currentToken().type;
    if (!tc->inFunction()) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_RETURN_OR_YIELD,
                          (tt == TOK_RETURN) ? js_return_str : js_yield_str);
        return NULL;
    }

    pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;

#if JS_HAS_GENERATORS
    if (tt == TOK_YIELD) {
        



        if (tc->parenDepth == 0) {
            tc->flags |= TCF_FUN_IS_GENERATOR;
        } else {
            tc->yieldCount++;
            tc->yieldNode = pn;
        }
    }
#endif

    
    tt2 = tokenStream.peekTokenSameLine(TSF_OPERAND);
    if (tt2 == TOK_ERROR)
        return NULL;

    if (tt2 != TOK_EOF && tt2 != TOK_EOL && tt2 != TOK_SEMI && tt2 != TOK_RC
#if JS_HAS_GENERATORS
        && (tt != TOK_YIELD ||
            (tt2 != tt && tt2 != TOK_RB && tt2 != TOK_RP &&
             tt2 != TOK_COLON && tt2 != TOK_COMMA))
#endif
        ) {
        pn2 = useAssignExpr ? assignExpr() : expr();
        if (!pn2)
            return NULL;
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            tc->flags |= TCF_RETURN_EXPR;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
    } else {
#if JS_HAS_GENERATORS
        if (tt == TOK_RETURN)
#endif
            tc->flags |= TCF_RETURN_VOID;
    }

    if ((~tc->flags & (TCF_RETURN_EXPR | TCF_FUN_IS_GENERATOR)) == 0) {
        
        ReportBadReturn(context, tc, pn, JSREPORT_ERROR,
                        JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return NULL;
    }

    if (context->hasStrictOption() &&
        (~tc->flags & (TCF_RETURN_EXPR | TCF_RETURN_VOID)) == 0 &&
        !ReportBadReturn(context, tc, pn, JSREPORT_WARNING | JSREPORT_STRICT,
                         JSMSG_NO_RETURN_VALUE,
                         JSMSG_ANON_NO_RETURN_VALUE)) {
        return NULL;
    }

    return pn;
}

static JSParseNode *
PushLexicalScope(JSContext *cx, TokenStream *ts, JSTreeContext *tc,
                 JSStmtInfo *stmt)
{
    JSParseNode *pn;
    JSObject *obj;
    JSObjectBox *blockbox;

    pn = LexicalScopeNode::create(tc);
    if (!pn)
        return NULL;

    obj = js_NewBlockObject(cx);
    if (!obj)
        return NULL;

    blockbox = tc->parser->newObjectBox(obj);
    if (!blockbox)
        return NULL;

    js_PushBlockScope(tc, stmt, blockbox, -1);
    pn->pn_type = TOK_LEXICALSCOPE;
    pn->pn_op = JSOP_LEAVEBLOCK;
    pn->pn_objbox = blockbox;
    pn->pn_cookie.makeFree();
    pn->pn_dflags = 0;
    if (!GenerateBlockId(tc, stmt->blockid))
        return NULL;
    pn->pn_blockid = stmt->blockid;
    return pn;
}

#if JS_HAS_BLOCK_SCOPE

JSParseNode *
Parser::letBlock(JSBool statement)
{
    JSParseNode *pn, *pnblock, *pnlet;
    JSStmtInfo stmtInfo;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LET);

    
    pnlet = BinaryNode::create(tc);
    if (!pnlet)
        return NULL;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    
    pnblock = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
    if (!pnblock)
        return NULL;
    pn = pnblock;
    pn->pn_expr = pnlet;

    pnlet->pn_left = variables(true);
    if (!pnlet->pn_left)
        return NULL;
    pnlet->pn_left->pn_xflags = PNX_POPVAR;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    if (statement && !tokenStream.matchToken(TOK_LC, TSF_OPERAND)) {
        







        if (!ReportStrictModeError(context, &tokenStream, tc, pnlet,
                                   JSMSG_STRICT_CODE_LET_EXPR_STMT)) {
            return NULL;
        }

        




        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_SEMI;
        pn->pn_num = -1;
        pn->pn_kid = pnblock;

        statement = JS_FALSE;
    }

    if (statement) {
        pnlet->pn_right = statements();
        if (!pnlet->pn_right)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_LET);
    } else {
        



        pnblock->pn_op = JSOP_LEAVEBLOCKEXPR;
        pnlet->pn_right = assignExpr();
        if (!pnlet->pn_right)
            return NULL;
    }

    PopStatement(tc);
    return pn;
}

#endif 

static bool
PushBlocklikeStatement(JSStmtInfo *stmt, JSStmtType type, JSTreeContext *tc)
{
    js_PushStatement(tc, stmt, type, -1);
    return GenerateBlockId(tc, stmt->blockid);
}

static JSParseNode *
NewBindingNode(JSAtom *atom, JSTreeContext *tc, bool let = false)
{
    JSParseNode *pn;
    AtomDefnPtr removal;

    if ((pn = tc->decls.lookupFirst(atom))) {
        JS_ASSERT(!pn->isPlaceholder());
    } else {
        removal = tc->lexdeps->lookup(atom);
        pn = removal ? removal.value() : NULL;
        JS_ASSERT_IF(pn, pn->isPlaceholder());
    }

    if (pn) {
        JS_ASSERT(pn->pn_defn);

        





        JS_ASSERT_IF(let && pn->pn_blockid == tc->blockid(),
                     pn->pn_blockid != tc->bodyid);

        if (pn->isPlaceholder() && pn->pn_blockid >= (let ? tc->blockid() : tc->bodyid)) {
            if (let)
                pn->pn_blockid = tc->blockid();

            tc->lexdeps->remove(removal);
            return pn;
        }
    }

    
    pn = NameNode::create(atom, tc);
    if (!pn)
        return NULL;

    if (atom == tc->parser->context->runtime->atomState.argumentsAtom)
        tc->countArgumentsUse(pn);

    return pn;
}

JSParseNode *
Parser::switchStatement()
{
    JSParseNode *pn5, *saveBlock;
    JSBool seenDefault = JS_FALSE;

    JSParseNode *pn = BinaryNode::create(tc);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

    
    JSParseNode *pn1 = parenExpr();
    if (!pn1)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

    



    JSStmtInfo stmtInfo;
    js_PushStatement(tc, &stmtInfo, STMT_SWITCH, -1);

    
    JSParseNode *pn2 = ListNode::create(tc);
    if (!pn2)
        return NULL;
    pn2->makeEmpty();
    if (!GenerateBlockIdForStmtNode(pn2, tc))
        return NULL;
    saveBlock = tc->blockNode;
    tc->blockNode = pn2;

    TokenKind tt;
    while ((tt = tokenStream.getToken()) != TOK_RC) {
        JSParseNode *pn3;
        switch (tt) {
          case TOK_DEFAULT:
            if (seenDefault) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_DEFAULTS);
                return NULL;
            }
            seenDefault = JS_TRUE;
            

          case TOK_CASE:
          {
            pn3 = BinaryNode::create(tc);
            if (!pn3)
                return NULL;
            if (tt == TOK_CASE) {
                pn3->pn_left = expr();
                if (!pn3->pn_left)
                    return NULL;
            }
            pn2->append(pn3);
            if (pn2->pn_count == JS_BIT(16)) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_CASES);
                return NULL;
            }
            break;
          }

          case TOK_ERROR:
            return NULL;

          default:
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_SWITCH);
            return NULL;
        }
        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

        JSParseNode *pn4 = ListNode::create(tc);
        if (!pn4)
            return NULL;
        pn4->pn_type = TOK_LC;
        pn4->makeEmpty();
        while ((tt = tokenStream.peekToken(TSF_OPERAND)) != TOK_RC &&
               tt != TOK_CASE && tt != TOK_DEFAULT) {
            if (tt == TOK_ERROR)
                return NULL;
            pn5 = statement();
            if (!pn5)
                return NULL;
            pn4->pn_pos.end = pn5->pn_pos.end;
            pn4->append(pn5);
        }

        
        if (pn4->pn_head)
            pn4->pn_pos.begin = pn4->pn_head->pn_pos.begin;
        pn3->pn_pos.end = pn4->pn_pos.end;
        pn3->pn_right = pn4;
    }

    





    if (tc->blockNode != pn2)
        pn2 = tc->blockNode;
    tc->blockNode = saveBlock;
    PopStatement(tc);

    pn->pn_pos.end = pn2->pn_pos.end = tokenStream.currentToken().pos.end;
    pn->pn_left = pn1;
    pn->pn_right = pn2;
    return pn;
}

JSParseNode *
Parser::forStatement()
{
    JSParseNode *pnseq = NULL;
#if JS_HAS_BLOCK_SCOPE
    JSParseNode *pnlet = NULL;
    JSStmtInfo blockInfo;
#endif

    
    JSParseNode *pn = BinaryNode::create(tc);
    if (!pn)
        return NULL;
    JSStmtInfo stmtInfo;
    js_PushStatement(tc, &stmtInfo, STMT_FOR_LOOP, -1);

    pn->pn_op = JSOP_ITER;
    pn->pn_iflags = 0;
    if (tokenStream.matchToken(TOK_NAME)) {
        if (tokenStream.currentToken().t_atom == context->runtime->atomState.eachAtom)
            pn->pn_iflags = JSITER_FOREACH;
        else
            tokenStream.ungetToken();
    }

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);
    TokenKind tt = tokenStream.peekToken(TSF_OPERAND);

#if JS_HAS_BLOCK_SCOPE
    bool let = false;
#endif

    JSParseNode *pn1;
    if (tt == TOK_SEMI) {
        if (pn->pn_iflags & JSITER_FOREACH)
            goto bad_for_each;

        
        pn1 = NULL;
    } else {
        












        tc->flags |= TCF_IN_FOR_INIT;
        if (tt == TOK_VAR) {
            (void) tokenStream.getToken();
            pn1 = variables(false);
#if JS_HAS_BLOCK_SCOPE
        } else if (tt == TOK_LET) {
            let = true;
            (void) tokenStream.getToken();
            if (tokenStream.peekToken() == TOK_LP) {
                pn1 = letBlock(JS_FALSE);
                tt = TOK_LEXICALSCOPE;
            } else {
                pnlet = PushLexicalScope(context, &tokenStream, tc, &blockInfo);
                if (!pnlet)
                    return NULL;
                blockInfo.flags |= SIF_FOR_BLOCK;
                pn1 = variables(false);
            }
#endif
        } else {
            pn1 = expr();
        }
        tc->flags &= ~TCF_IN_FOR_INIT;
        if (!pn1)
            return NULL;
    }

    





    if (pn1 && tokenStream.matchToken(TOK_IN)) {
        pn->pn_iflags |= JSITER_ENUMERATE;
        stmtInfo.type = STMT_FOR_IN_LOOP;

        
        JS_ASSERT(!TokenKindIsDecl(tt) || PN_TYPE(pn1) == tt);
        if (TokenKindIsDecl(tt)
            ? (pn1->pn_count > 1 || pn1->pn_op == JSOP_DEFCONST
#if JS_HAS_DESTRUCTURING
               || (versionNumber() == JSVERSION_1_7 &&
                   pn->pn_op == JSOP_ITER &&
                   !(pn->pn_iflags & JSITER_FOREACH) &&
                   (pn1->pn_head->pn_type == TOK_RC ||
                    (pn1->pn_head->pn_type == TOK_RB &&
                     pn1->pn_head->pn_count != 2) ||
                    (pn1->pn_head->pn_type == TOK_ASSIGN &&
                     (pn1->pn_head->pn_left->pn_type != TOK_RB ||
                      pn1->pn_head->pn_left->pn_count != 2))))
#endif
              )
            : (pn1->pn_type != TOK_NAME &&
               pn1->pn_type != TOK_DOT &&
#if JS_HAS_DESTRUCTURING
               ((versionNumber() == JSVERSION_1_7 &&
                 pn->pn_op == JSOP_ITER &&
                 !(pn->pn_iflags & JSITER_FOREACH))
                ? (pn1->pn_type != TOK_RB || pn1->pn_count != 2)
                : (pn1->pn_type != TOK_RB && pn1->pn_type != TOK_RC)) &&
#endif
               pn1->pn_type != TOK_LP &&
#if JS_HAS_XML_SUPPORT
               (pn1->pn_type != TOK_UNARYOP ||
                pn1->pn_op != JSOP_XMLNAME) &&
#endif
               pn1->pn_type != TOK_LB)) {
            reportErrorNumber(pn1, JSREPORT_ERROR, JSMSG_BAD_FOR_LEFTSIDE);
            return NULL;
        }

        
        JSParseNode *pn2 = NULL;
        uintN dflag = PND_ASSIGNED;

        if (TokenKindIsDecl(tt)) {
            
            pn1->pn_xflags |= PNX_FORINVAR;

            




            pn2 = pn1->pn_head;
            if ((pn2->pn_type == TOK_NAME && pn2->maybeExpr())
#if JS_HAS_DESTRUCTURING
                || pn2->pn_type == TOK_ASSIGN
#endif
                ) {
#if JS_HAS_BLOCK_SCOPE
                if (tt == TOK_LET) {
                    reportErrorNumber(pn2, JSREPORT_ERROR, JSMSG_INVALID_FOR_IN_INIT);
                    return NULL;
                }
#endif 

                pnseq = ListNode::create(tc);
                if (!pnseq)
                    return NULL;
                pnseq->pn_type = TOK_SEQ;
                pnseq->pn_pos.begin = pn->pn_pos.begin;

                dflag = PND_INITIALIZED;

                







                pn1->pn_xflags &= ~PNX_FORINVAR;
                pn1->pn_xflags |= PNX_POPVAR;
                pnseq->initList(pn1);

#if JS_HAS_DESTRUCTURING
                if (pn2->pn_type == TOK_ASSIGN) {
                    pn1 = CloneParseTree(pn2->pn_left, tc);
                    if (!pn1)
                        return NULL;
                } else
#endif
                {
                    JS_ASSERT(pn2->pn_type == TOK_NAME);
                    pn1 = NameNode::create(pn2->pn_atom, tc);
                    if (!pn1)
                        return NULL;
                    pn1->pn_type = TOK_NAME;
                    pn1->pn_op = JSOP_NAME;
                    pn1->pn_pos = pn2->pn_pos;
                    if (pn2->pn_defn)
                        LinkUseToDef(pn1, (JSDefinition *) pn2, tc);
                }
                pn2 = pn1;
            }
        }

        if (!pn2) {
            pn2 = pn1;
            if (pn2->pn_type == TOK_LP &&
                !MakeSetCall(context, pn2, tc, JSMSG_BAD_LEFTSIDE_OF_ASS)) {
                return NULL;
            }
#if JS_HAS_XML_SUPPORT
            if (pn2->pn_type == TOK_UNARYOP)
                pn2->pn_op = JSOP_BINDXMLNAME;
#endif
        }

        switch (pn2->pn_type) {
          case TOK_NAME:
            
            NoteLValue(context, pn2, tc, dflag);
            break;

#if JS_HAS_DESTRUCTURING
          case TOK_ASSIGN:
            pn2 = pn2->pn_left;
            JS_ASSERT(pn2->pn_type == TOK_RB || pn2->pn_type == TOK_RC);
            
          case TOK_RB:
          case TOK_RC:
            
            if (pn1 == pn2 && !CheckDestructuring(context, NULL, pn2, tc))
                return NULL;

            if (versionNumber() == JSVERSION_1_7) {
                



                JS_ASSERT(pn->pn_op == JSOP_ITER);
                if (!(pn->pn_iflags & JSITER_FOREACH))
                    pn->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          default:;
        }

        




#if JS_HAS_BLOCK_SCOPE
        JSStmtInfo *save = tc->topStmt;
        if (let)
            tc->topStmt = save->down;
#endif
        pn2 = expr();
#if JS_HAS_BLOCK_SCOPE
        if (let)
            tc->topStmt = save;
#endif

        pn2 = JSParseNode::newBinaryOrAppend(TOK_IN, JSOP_NOP, pn1, pn2, tc);
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
    } else {
        if (pn->pn_iflags & JSITER_FOREACH)
            goto bad_for_each;
        pn->pn_op = JSOP_NOP;

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
        tt = tokenStream.peekToken(TSF_OPERAND);
        JSParseNode *pn2;
        if (tt == TOK_SEMI) {
            pn2 = NULL;
        } else {
            pn2 = expr();
            if (!pn2)
                return NULL;
        }

        
        MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
        tt = tokenStream.peekToken(TSF_OPERAND);
        JSParseNode *pn3;
        if (tt == TOK_RP) {
            pn3 = NULL;
        } else {
            pn3 = expr();
            if (!pn3)
                return NULL;
        }

        
        JSParseNode *pn4 = TernaryNode::create(tc);
        if (!pn4)
            return NULL;
        pn4->pn_type = TOK_FORHEAD;
        pn4->pn_op = JSOP_NOP;
        pn4->pn_kid1 = pn1;
        pn4->pn_kid2 = pn2;
        pn4->pn_kid3 = pn3;
        pn->pn_left = pn4;
    }

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

    
    JSParseNode *pn2;
    pn2 = statement();
    if (!pn2)
        return NULL;
    pn->pn_right = pn2;

    
    pn->pn_pos.end = pn2->pn_pos.end;

#if JS_HAS_BLOCK_SCOPE
    if (pnlet) {
        PopStatement(tc);
        pnlet->pn_expr = pn;
        pn = pnlet;
    }
#endif
    if (pnseq) {
        pnseq->pn_pos.end = pn->pn_pos.end;
        pnseq->append(pn);
        pn = pnseq;
    }
    PopStatement(tc);
    return pn;

  bad_for_each:
    reportErrorNumber(pn, JSREPORT_ERROR, JSMSG_BAD_FOR_EACH_LOOP);
    return NULL;
}

JSParseNode *
Parser::tryStatement()
{
    JSParseNode *catchList, *lastCatch;

    
















    JSParseNode *pn = TernaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_op = JSOP_NOP;

    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
    JSStmtInfo stmtInfo;
    if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, tc))
        return NULL;
    pn->pn_kid1 = statements();
    if (!pn->pn_kid1)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
    PopStatement(tc);

    catchList = NULL;
    TokenKind tt = tokenStream.getToken();
    if (tt == TOK_CATCH) {
        catchList = ListNode::create(tc);
        if (!catchList)
            return NULL;
        catchList->pn_type = TOK_RESERVED;
        catchList->makeEmpty();
        lastCatch = NULL;

        do {
            JSParseNode *pnblock;
            BindData data;

            
            if (lastCatch && !lastCatch->pn_kid2) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_AFTER_GENERAL);
                return NULL;
            }

            



            pnblock = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
            if (!pnblock)
                return NULL;
            stmtInfo.type = STMT_CATCH;

            






            JSParseNode *pn2 = TernaryNode::create(tc);
            if (!pn2)
                return NULL;
            pnblock->pn_expr = pn2;
            MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_CATCH);

            




            data.pn = NULL;
            data.op = JSOP_NOP;
            data.binder = BindLet;
            data.let.overflow = JSMSG_TOO_MANY_CATCH_VARS;

            tt = tokenStream.getToken();
            JSParseNode *pn3;
            switch (tt) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
                pn3 = destructuringExpr(&data, tt);
                if (!pn3)
                    return NULL;
                break;
#endif

              case TOK_NAME:
              {
                JSAtom *label = tokenStream.currentToken().t_atom;
                pn3 = NewBindingNode(label, tc, true);
                if (!pn3)
                    return NULL;
                data.pn = pn3;
                if (!data.binder(context, &data, label, tc))
                    return NULL;
                break;
              }

              default:
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_IDENTIFIER);
                return NULL;
            }

            pn2->pn_kid1 = pn3;
#if JS_HAS_CATCH_GUARD
            




            if (tokenStream.matchToken(TOK_IF)) {
                pn2->pn_kid2 = expr();
                if (!pn2->pn_kid2)
                    return NULL;
            }
#endif
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_CATCH);

            MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_CATCH);
            pn2->pn_kid3 = statements();
            if (!pn2->pn_kid3)
                return NULL;
            MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_CATCH);
            PopStatement(tc);

            catchList->append(pnblock);
            lastCatch = pn2;
            tt = tokenStream.getToken(TSF_OPERAND);
        } while (tt == TOK_CATCH);
    }
    pn->pn_kid2 = catchList;

    if (tt == TOK_FINALLY) {
        MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_FINALLY);
        if (!PushBlocklikeStatement(&stmtInfo, STMT_FINALLY, tc))
            return NULL;
        pn->pn_kid3 = statements();
        if (!pn->pn_kid3)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_FINALLY);
        PopStatement(tc);
    } else {
        tokenStream.ungetToken();
    }
    if (!catchList && !pn->pn_kid3) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_OR_FINALLY);
        return NULL;
    }
    return pn;
}

JSParseNode *
Parser::withStatement()
{
    







    if (tc->flags & TCF_STRICT_MODE_CODE) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_STRICT_CODE_WITH);
        return NULL;
    }

    JSParseNode *pn = BinaryNode::create(tc);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_WITH);
    JSParseNode *pn2 = parenExpr();
    if (!pn2)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_WITH);
    pn->pn_left = pn2;

    JSParseNode *oldWith = tc->innermostWith;
    tc->innermostWith = pn;

    JSStmtInfo stmtInfo;
    js_PushStatement(tc, &stmtInfo, STMT_WITH, -1);
    pn2 = statement();
    if (!pn2)
        return NULL;
    PopStatement(tc);

    pn->pn_pos.end = pn2->pn_pos.end;
    pn->pn_right = pn2;
    tc->flags |= TCF_FUN_HEAVYWEIGHT;
    tc->innermostWith = oldWith;

    



    for (AtomDefnRange r = tc->lexdeps->all(); !r.empty(); r.popFront()) {
        JSDefinition *defn = r.front().value();
        JSDefinition *lexdep = defn->resolve();
        DeoptimizeUsesWithin(lexdep, pn->pn_pos);
    }

    return pn;
}

#if JS_HAS_BLOCK_SCOPE
JSParseNode *
Parser::letStatement()
{
    JSObjectBox *blockbox;

    JSParseNode *pn;
    do {
        
        if (tokenStream.peekToken() == TOK_LP) {
            pn = letBlock(JS_TRUE);
            if (!pn || pn->pn_op == JSOP_LEAVEBLOCK)
                return pn;

            
            JS_ASSERT(pn->pn_type == TOK_SEMI ||
                      pn->pn_op == JSOP_LEAVEBLOCKEXPR);
            break;
        }

        










        JSStmtInfo *stmt = tc->topStmt;
        if (stmt &&
            (!STMT_MAYBE_SCOPE(stmt) || (stmt->flags & SIF_FOR_BLOCK))) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LET_DECL_NOT_IN_BLOCK);
            return NULL;
        }

        if (stmt && (stmt->flags & SIF_SCOPE)) {
            JS_ASSERT(tc->blockChainBox == stmt->blockBox);
        } else {
            if (!stmt || (stmt->flags & SIF_BODY_BLOCK)) {
                



                tokenStream.mungeCurrentToken(TOK_VAR, JSOP_DEFVAR);

                pn = variables(false);
                if (!pn)
                    return NULL;
                pn->pn_xflags |= PNX_POPVAR;
                break;
            }

            




            JS_ASSERT(!(stmt->flags & SIF_SCOPE));
            JS_ASSERT(stmt != tc->topScopeStmt);
            JS_ASSERT(stmt->type == STMT_BLOCK ||
                      stmt->type == STMT_SWITCH ||
                      stmt->type == STMT_TRY ||
                      stmt->type == STMT_FINALLY);
            JS_ASSERT(!stmt->downScope);

            
            JSObject *obj = js_NewBlockObject(tc->parser->context);
            if (!obj)
                return NULL;

            blockbox = tc->parser->newObjectBox(obj);
            if (!blockbox)
                return NULL;

            





            stmt->flags |= SIF_SCOPE;
            stmt->downScope = tc->topScopeStmt;
            tc->topScopeStmt = stmt;

            obj->setParent(tc->blockChain());
            blockbox->parent = tc->blockChainBox;
            tc->blockChainBox = blockbox;
            stmt->blockBox = blockbox;

#ifdef DEBUG
            JSParseNode *tmp = tc->blockNode;
            JS_ASSERT(!tmp || tmp->pn_type != TOK_LEXICALSCOPE);
#endif

            
            JSParseNode *pn1 = LexicalScopeNode::create(tc);
            if (!pn1)
                return NULL;

            pn1->pn_type = TOK_LEXICALSCOPE;
            pn1->pn_op = JSOP_LEAVEBLOCK;
            pn1->pn_pos = tc->blockNode->pn_pos;
            pn1->pn_objbox = blockbox;
            pn1->pn_expr = tc->blockNode;
            pn1->pn_blockid = tc->blockNode->pn_blockid;
            tc->blockNode = pn1;
        }

        pn = variables(false);
        if (!pn)
            return NULL;
        pn->pn_xflags = PNX_POPVAR;
    } while (0);

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}
#endif

JSParseNode *
Parser::expressionStatement()
{
    tokenStream.ungetToken();
    JSParseNode *pn2 = expr();
    if (!pn2)
        return NULL;

    if (tokenStream.peekToken() == TOK_COLON) {
        if (pn2->pn_type != TOK_NAME) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_LABEL);
            return NULL;
        }
        JSAtom *label = pn2->pn_atom;
        for (JSStmtInfo *stmt = tc->topStmt; stmt; stmt = stmt->down) {
            if (stmt->type == STMT_LABEL && stmt->label == label) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_DUPLICATE_LABEL);
                return NULL;
            }
        }
        ForgetUse(pn2);

        (void) tokenStream.getToken();

        
        JSStmtInfo stmtInfo;
        js_PushStatement(tc, &stmtInfo, STMT_LABEL, -1);
        stmtInfo.label = label;
        JSParseNode *pn = statement();
        if (!pn)
            return NULL;

        
        if (pn->pn_type == TOK_SEMI && !pn->pn_kid) {
            pn->pn_type = TOK_LC;
            pn->pn_arity = PN_LIST;
            pn->makeEmpty();
        }

        
        PopStatement(tc);
        pn2->pn_type = TOK_COLON;
        pn2->pn_pos.end = pn->pn_pos.end;
        pn2->pn_expr = pn;
        return pn2;
    }

    JSParseNode *pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_type = TOK_SEMI;
    pn->pn_pos = pn2->pn_pos;
    pn->pn_kid = pn2;

    switch (PN_TYPE(pn2)) {
      case TOK_LP:
        



        if (PN_TYPE(pn2->pn_head) == TOK_FUNCTION &&
            !pn2->pn_head->pn_funbox->node->isFunArg()) {
            pn2->pn_head->pn_funbox->tcflags |= TCF_FUN_MODULE_PATTERN;
        }
        break;
      case TOK_ASSIGN:
        




        if (tc->funbox &&
            PN_OP(pn2) == JSOP_NOP &&
            PN_OP(pn2->pn_left) == JSOP_SETPROP &&
            PN_OP(pn2->pn_left->pn_expr) == JSOP_THIS &&
            PN_OP(pn2->pn_right) == JSOP_LAMBDA) {
            JS_ASSERT(!pn2->pn_defn);
            JS_ASSERT(!pn2->pn_used);
            pn2->pn_right->pn_link = tc->funbox->methods;
            tc->funbox->methods = pn2->pn_right;
        }
        break;
      default:;
    }

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}

JSParseNode *
Parser::statement()
{
    JSParseNode *pn;

    JS_CHECK_RECURSION(context, return NULL);

    switch (tokenStream.getToken(TSF_OPERAND)) {
      case TOK_FUNCTION:
      {
#if JS_HAS_XML_SUPPORT
        TokenKind tt = tokenStream.peekToken(TSF_KEYWORD_IS_NAME);
        if (tt == TOK_DBLCOLON)
            goto expression;
#endif
        return functionStmt();
      }

      case TOK_IF:
      {
        
        pn = TernaryNode::create(tc);
        if (!pn)
            return NULL;
        JSParseNode *pn1 = condition();
        if (!pn1)
            return NULL;
        JSStmtInfo stmtInfo;
        js_PushStatement(tc, &stmtInfo, STMT_IF, -1);
        JSParseNode *pn2 = statement();
        if (!pn2)
            return NULL;
        JSParseNode *pn3;
        if (tokenStream.matchToken(TOK_ELSE, TSF_OPERAND)) {
            stmtInfo.type = STMT_ELSE;
            pn3 = statement();
            if (!pn3)
                return NULL;
            pn->pn_pos.end = pn3->pn_pos.end;
        } else {
            pn3 = NULL;
            pn->pn_pos.end = pn2->pn_pos.end;
        }
        PopStatement(tc);
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
        return pn;
      }

      case TOK_SWITCH:
        return switchStatement();

      case TOK_WHILE:
      {
        pn = BinaryNode::create(tc);
        if (!pn)
            return NULL;
        JSStmtInfo stmtInfo;
        js_PushStatement(tc, &stmtInfo, STMT_WHILE_LOOP, -1);
        JSParseNode *pn2 = condition();
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        JSParseNode *pn3 = statement();
        if (!pn3)
            return NULL;
        PopStatement(tc);
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_right = pn3;
        return pn;
      }

      case TOK_DO:
      {
        pn = BinaryNode::create(tc);
        if (!pn)
            return NULL;
        JSStmtInfo stmtInfo;
        js_PushStatement(tc, &stmtInfo, STMT_DO_LOOP, -1);
        JSParseNode *pn2 = statement();
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        MUST_MATCH_TOKEN(TOK_WHILE, JSMSG_WHILE_AFTER_DO);
        JSParseNode *pn3 = condition();
        if (!pn3)
            return NULL;
        PopStatement(tc);
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_right = pn3;
        if (versionNumber() != JSVERSION_ECMA_3) {
            




            (void) tokenStream.matchToken(TOK_SEMI);
            return pn;
        }
        break;
      }

      case TOK_FOR:
        return forStatement();

      case TOK_TRY:
        return tryStatement();

      case TOK_THROW:
      {
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;

        
        TokenKind tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
        if (tt == TOK_ERROR)
            return NULL;
        if (tt == TOK_EOF || tt == TOK_EOL || tt == TOK_SEMI || tt == TOK_RC) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            return NULL;
        }

        JSParseNode *pn2 = expr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_op = JSOP_THROW;
        pn->pn_kid = pn2;
        break;
      }

      
      case TOK_CATCH:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CATCH_WITHOUT_TRY);
        return NULL;

      case TOK_FINALLY:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_FINALLY_WITHOUT_TRY);
        return NULL;

      case TOK_BREAK:
      {
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!MatchLabel(context, &tokenStream, pn))
            return NULL;
        JSStmtInfo *stmt = tc->topStmt;
        JSAtom *label = pn->pn_atom;
        if (label) {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL && stmt->label == label)
                    break;
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_TOUGH_BREAK);
                    return NULL;
                }
                if (STMT_IS_LOOP(stmt) || stmt->type == STMT_SWITCH)
                    break;
            }
        }
        if (label)
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
        break;
      }

      case TOK_CONTINUE:
      {
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!MatchLabel(context, &tokenStream, pn))
            return NULL;
        JSStmtInfo *stmt = tc->topStmt;
        JSAtom *label = pn->pn_atom;
        if (label) {
            for (JSStmtInfo *stmt2 = NULL; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL) {
                    if (stmt->label == label) {
                        if (!stmt2 || !STMT_IS_LOOP(stmt2)) {
                            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_CONTINUE);
                            return NULL;
                        }
                        break;
                    }
                } else {
                    stmt2 = stmt;
                }
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_CONTINUE);
                    return NULL;
                }
                if (STMT_IS_LOOP(stmt))
                    break;
            }
        }
        if (label)
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
        break;
      }

      case TOK_WITH:
        return withStatement();

      case TOK_VAR:
        pn = variables(false);
        if (!pn)
            return NULL;

        
        pn->pn_xflags |= PNX_POPVAR;
        break;

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        return letStatement();
#endif 

      case TOK_RETURN:
        pn = returnOrYield(false);
        if (!pn)
            return NULL;
        break;

      case TOK_LC:
      {
        uintN oldflags;

        oldflags = tc->flags;
        tc->flags = oldflags & ~TCF_HAS_FUNCTION_STMT;
        JSStmtInfo stmtInfo;
        if (!PushBlocklikeStatement(&stmtInfo, STMT_BLOCK, tc))
            return NULL;
        pn = statements();
        if (!pn)
            return NULL;

        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_COMPOUND);
        PopStatement(tc);

        



        if ((tc->flags & TCF_HAS_FUNCTION_STMT) &&
            (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)) {
            pn->pn_xflags |= PNX_NEEDBRACES;
        }
        tc->flags = oldflags | (tc->flags & (TCF_FUN_FLAGS | TCF_RETURN_FLAGS));
        return pn;
      }

      case TOK_SEMI:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_SEMI;
        return pn;

      case TOK_DEBUGGER:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_DEBUGGER;
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_DEFAULT:
      {
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().t_atom != context->runtime->atomState.xmlAtom ||
            !tokenStream.matchToken(TOK_NAME) ||
            tokenStream.currentToken().t_atom != context->runtime->atomState.namespaceAtom ||
            !tokenStream.matchToken(TOK_ASSIGN) ||
            tokenStream.currentToken().t_op != JSOP_NOP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_DEFAULT_XML_NAMESPACE);
            return NULL;
        }

        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        JSParseNode *pn2 = expr();
        if (!pn2)
            return NULL;
        pn->pn_op = JSOP_DEFXMLNS;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
        break;
      }
#endif

      case TOK_ERROR:
        return NULL;

      default:
#if JS_HAS_XML_SUPPORT
      expression:
#endif
        return expressionStatement();
    }

    
    return MatchOrInsertSemicolon(context, &tokenStream) ? pn : NULL;
}

JSParseNode *
Parser::variables(bool inLetHead)
{
    TokenKind tt;
    bool let;
    JSStmtInfo *scopeStmt;
    BindData data;
    JSParseNode *pn, *pn2;
    JSAtom *atom;

    





    tt = tokenStream.currentToken().type;
    let = (tt == TOK_LET || tt == TOK_LP);
    JS_ASSERT(let || tt == TOK_VAR);

#if JS_HAS_BLOCK_SCOPE
    bool popScope = (inLetHead || (let && (tc->flags & TCF_IN_FOR_INIT)));
    JSStmtInfo *save = tc->topStmt, *saveScope = tc->topScopeStmt;
#endif

    
    scopeStmt = tc->topScopeStmt;
    if (let) {
        while (scopeStmt && !(scopeStmt->flags & SIF_SCOPE)) {
            JS_ASSERT(!STMT_MAYBE_SCOPE(scopeStmt));
            scopeStmt = scopeStmt->downScope;
        }
        JS_ASSERT(scopeStmt);
    }

    data.op = let ? JSOP_NOP : tokenStream.currentToken().t_op;
    pn = ListNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_op = data.op;
    pn->makeEmpty();

    




    if (let) {
        JS_ASSERT(tc->blockChainBox == scopeStmt->blockBox);
        data.binder = BindLet;
        data.let.overflow = JSMSG_TOO_MANY_LOCALS;
    } else {
        data.binder = BindVarOrConst;
    }

    do {
        tt = tokenStream.getToken();
#if JS_HAS_DESTRUCTURING
        if (tt == TOK_LB || tt == TOK_LC) {
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn2 = primaryExpr(tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn2)
                return NULL;

            if (!CheckDestructuring(context, &data, pn2, tc))
                return NULL;
            if ((tc->flags & TCF_IN_FOR_INIT) &&
                tokenStream.peekToken() == TOK_IN) {
                pn->append(pn2);
                continue;
            }

            MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);
            if (tokenStream.currentToken().t_op != JSOP_NOP)
                goto bad_var_init;

#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save->down;
                tc->topScopeStmt = saveScope->downScope;
            }
#endif
            JSParseNode *init = assignExpr();
#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save;
                tc->topScopeStmt = saveScope;
            }
#endif

            if (!init)
                return NULL;
            UndominateInitializers(pn2, init->pn_pos.end, tc);

            pn2 = JSParseNode::newBinaryOrAppend(TOK_ASSIGN, JSOP_NOP, pn2, init, tc);
            if (!pn2)
                return NULL;
            pn->append(pn2);
            continue;
        }
#endif 

        if (tt != TOK_NAME) {
            if (tt != TOK_ERROR) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NO_VARIABLE_NAME);
            }
            return NULL;
        }

        atom = tokenStream.currentToken().t_atom;
        pn2 = NewBindingNode(atom, tc, let);
        if (!pn2)
            return NULL;
        if (data.op == JSOP_DEFCONST)
            pn2->pn_dflags |= PND_CONST;
        data.pn = pn2;
        if (!data.binder(context, &data, atom, tc))
            return NULL;
        pn->append(pn2);

        if (tokenStream.matchToken(TOK_ASSIGN)) {
            if (tokenStream.currentToken().t_op != JSOP_NOP)
                goto bad_var_init;

#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save->down;
                tc->topScopeStmt = saveScope->downScope;
            }
#endif
            JSParseNode *init = assignExpr();
#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save;
                tc->topScopeStmt = saveScope;
            }
#endif
            if (!init)
                return NULL;

            if (pn2->pn_used) {
                pn2 = MakeAssignment(pn2, init, tc);
                if (!pn2)
                    return NULL;
            } else {
                pn2->pn_expr = init;
            }

            JS_ASSERT_IF(pn2->pn_dflags & PND_GVAR, !(pn2->pn_dflags & PND_BOUND));

            pn2->pn_op = (PN_OP(pn2) == JSOP_ARGUMENTS)
                         ? JSOP_SETNAME
                         : (pn2->pn_dflags & PND_BOUND)
                         ? JSOP_SETLOCAL
                         : (data.op == JSOP_DEFCONST)
                         ? JSOP_SETCONST
                         : JSOP_SETNAME;

            NoteLValue(context, pn2, tc, data.fresh ? PND_INITIALIZED : PND_ASSIGNED);

            
            pn2->pn_pos.end = init->pn_pos.end;

            if (tc->inFunction() &&
                atom == context->runtime->atomState.argumentsAtom) {
                tc->noteArgumentsUse(pn2);
                if (!let)
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
            }
        }
    } while (tokenStream.matchToken(TOK_COMMA));

    pn->pn_pos.end = pn->last()->pn_pos.end;
    return pn;

bad_var_init:
    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_VAR_INIT);
    return NULL;
}

JSParseNode *
Parser::expr()
{
    JSParseNode *pn = assignExpr();
    if (pn && tokenStream.matchToken(TOK_COMMA)) {
        JSParseNode *pn2 = ListNode::create(tc);
        if (!pn2)
            return NULL;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->initList(pn);
        pn = pn2;
        do {
#if JS_HAS_GENERATORS
            pn2 = pn->last();
            if (pn2->pn_type == TOK_YIELD && !pn2->pn_parens) {
                reportErrorNumber(pn2, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
                return NULL;
            }
#endif
            pn2 = assignExpr();
            if (!pn2)
                return NULL;
            pn->append(pn2);
        } while (tokenStream.matchToken(TOK_COMMA));
        pn->pn_pos.end = pn->last()->pn_pos.end;
    }
    return pn;
}







#define BEGIN_EXPR_PARSER(name)                                               \
    JS_ALWAYS_INLINE JSParseNode *                                            \
    Parser::name##i()

#define END_EXPR_PARSER(name)                                                 \
    JS_NEVER_INLINE JSParseNode *                                             \
    Parser::name##n() {                                                       \
        return name##i();                                                     \
    }

BEGIN_EXPR_PARSER(mulExpr1)
{
    TokenKind tt;
    JSParseNode *pn = unaryExpr();

    




    while (pn && ((tt = tokenStream.getToken()) == TOK_STAR || tt == TOK_DIVOP)) {
        tt = tokenStream.currentToken().type;
        JSOp op = tokenStream.currentToken().t_op;
        pn = JSParseNode::newBinaryOrAppend(tt, op, pn, unaryExpr(), tc);
    }
    return pn;
}
END_EXPR_PARSER(mulExpr1)

BEGIN_EXPR_PARSER(addExpr1)
{
    JSParseNode *pn = mulExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_PLUS, TOK_MINUS)) {
        TokenKind tt = tokenStream.currentToken().type;
        JSOp op = (tt == TOK_PLUS) ? JSOP_ADD : JSOP_SUB;
        pn = JSParseNode::newBinaryOrAppend(tt, op, pn, mulExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(addExpr1)

BEGIN_EXPR_PARSER(shiftExpr1)
{
    JSParseNode *pn = addExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_SHOP)) {
        JSOp op = tokenStream.currentToken().t_op;
        pn = JSParseNode::newBinaryOrAppend(TOK_SHOP, op, pn, addExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(shiftExpr1)

BEGIN_EXPR_PARSER(relExpr1)
{
    uintN inForInitFlag = tc->flags & TCF_IN_FOR_INIT;

    



    tc->flags &= ~TCF_IN_FOR_INIT;

    JSParseNode *pn = shiftExpr1i();
    while (pn &&
           (tokenStream.isCurrentTokenType(TOK_RELOP) ||
            



            (inForInitFlag == 0 && tokenStream.isCurrentTokenType(TOK_IN)) ||
            tokenStream.isCurrentTokenType(TOK_INSTANCEOF))) {
        TokenKind tt = tokenStream.currentToken().type;
        JSOp op = tokenStream.currentToken().t_op;
        pn = JSParseNode::newBinaryOrAppend(tt, op, pn, shiftExpr1n(), tc);
    }
    
    tc->flags |= inForInitFlag;

    return pn;
}
END_EXPR_PARSER(relExpr1)

BEGIN_EXPR_PARSER(eqExpr1)
{
    JSParseNode *pn = relExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_EQOP)) {
        JSOp op = tokenStream.currentToken().t_op;
        pn = JSParseNode::newBinaryOrAppend(TOK_EQOP, op, pn, relExpr1n(), tc);
    }
    return pn;
}
END_EXPR_PARSER(eqExpr1)

BEGIN_EXPR_PARSER(bitAndExpr1)
{
    JSParseNode *pn = eqExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITAND))
        pn = JSParseNode::newBinaryOrAppend(TOK_BITAND, JSOP_BITAND, pn, eqExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitAndExpr1)

BEGIN_EXPR_PARSER(bitXorExpr1)
{
    JSParseNode *pn = bitAndExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITXOR))
        pn = JSParseNode::newBinaryOrAppend(TOK_BITXOR, JSOP_BITXOR, pn, bitAndExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitXorExpr1)

BEGIN_EXPR_PARSER(bitOrExpr1)
{
    JSParseNode *pn = bitXorExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_BITOR))
        pn = JSParseNode::newBinaryOrAppend(TOK_BITOR, JSOP_BITOR, pn, bitXorExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(bitOrExpr1)

BEGIN_EXPR_PARSER(andExpr1)
{
    JSParseNode *pn = bitOrExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_AND))
        pn = JSParseNode::newBinaryOrAppend(TOK_AND, JSOP_AND, pn, bitOrExpr1n(), tc);
    return pn;
}
END_EXPR_PARSER(andExpr1)

JS_ALWAYS_INLINE JSParseNode *
Parser::orExpr1()
{
    JSParseNode *pn = andExpr1i();
    while (pn && tokenStream.isCurrentTokenType(TOK_OR))
        pn = JSParseNode::newBinaryOrAppend(TOK_OR, JSOP_OR, pn, andExpr1n(), tc);
    return pn;
}

JS_ALWAYS_INLINE JSParseNode *
Parser::condExpr1()
{
    JSParseNode *pn = orExpr1();
    if (pn && tokenStream.isCurrentTokenType(TOK_HOOK)) {
        JSParseNode *pn1 = pn;
        pn = TernaryNode::create(tc);
        if (!pn)
            return NULL;

        




        uintN oldflags = tc->flags;
        tc->flags &= ~TCF_IN_FOR_INIT;
        JSParseNode *pn2 = assignExpr();
        tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);

        if (!pn2)
            return NULL;
        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_IN_COND);
        JSParseNode *pn3 = assignExpr();
        if (!pn3)
            return NULL;
        pn->pn_pos.begin = pn1->pn_pos.begin;
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
        tokenStream.getToken();     
    }
    return pn;
}

JSParseNode *
Parser::assignExpr()
{
    JS_CHECK_RECURSION(context, return NULL);

#if JS_HAS_GENERATORS
    if (tokenStream.matchToken(TOK_YIELD, TSF_OPERAND))
        return returnOrYield(true);
#endif

    JSParseNode *pn = condExpr1();
    if (!pn)
        return NULL;

    if (!tokenStream.isCurrentTokenType(TOK_ASSIGN)) {
        tokenStream.ungetToken();
        return pn;
    }

    JSOp op = tokenStream.currentToken().t_op;
    switch (pn->pn_type) {
      case TOK_NAME:
        if (!CheckStrictAssignment(context, tc, pn))
            return NULL;
        pn->pn_op = JSOP_SETNAME;
        NoteLValue(context, pn, tc);
        break;
      case TOK_DOT:
        pn->pn_op = JSOP_SETPROP;
        break;
      case TOK_LB:
        pn->pn_op = JSOP_SETELEM;
        break;
#if JS_HAS_DESTRUCTURING
      case TOK_RB:
      case TOK_RC:
      {
        if (op != JSOP_NOP) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_DESTRUCT_ASS);
            return NULL;
        }
        JSParseNode *rhs = assignExpr();
        if (!rhs || !CheckDestructuring(context, NULL, pn, tc))
            return NULL;
        return JSParseNode::newBinaryOrAppend(TOK_ASSIGN, op, pn, rhs, tc);
      }
#endif
      case TOK_LP:
        if (!MakeSetCall(context, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
            return NULL;
        break;
#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        if (pn->pn_op == JSOP_XMLNAME) {
            pn->pn_op = JSOP_SETXMLNAME;
            break;
        }
        
#endif
      default:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return NULL;
    }

    JSParseNode *rhs = assignExpr();
    if (rhs && PN_TYPE(pn) == TOK_NAME && pn->pn_used) {
        JSDefinition *dn = pn->pn_lexdef;

        






        if (!dn->isAssigned()) {
            JS_ASSERT(dn->isInitialized());
            dn->pn_pos.end = rhs->pn_pos.end;
        }
    }

    return JSParseNode::newBinaryOrAppend(TOK_ASSIGN, op, pn, rhs, tc);
}

static JSParseNode *
SetLvalKid(JSContext *cx, TokenStream *ts, JSTreeContext *tc,
           JSParseNode *pn, JSParseNode *kid, const char *name)
{
    if (kid->pn_type != TOK_NAME &&
        kid->pn_type != TOK_DOT &&
        (kid->pn_type != TOK_LP ||
         (kid->pn_op != JSOP_CALL && kid->pn_op != JSOP_EVAL &&
          kid->pn_op != JSOP_FUNCALL && kid->pn_op != JSOP_FUNAPPLY)) &&
#if JS_HAS_XML_SUPPORT
        (kid->pn_type != TOK_UNARYOP || kid->pn_op != JSOP_XMLNAME) &&
#endif
        kid->pn_type != TOK_LB) {
        ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR, JSMSG_BAD_OPERAND, name);
        return NULL;
    }
    if (!CheckStrictAssignment(cx, tc, kid))
        return NULL;
    pn->pn_kid = kid;
    return kid;
}

static const char incop_name_str[][10] = {"increment", "decrement"};

static JSBool
SetIncOpKid(JSContext *cx, TokenStream *ts, JSTreeContext *tc,
            JSParseNode *pn, JSParseNode *kid,
            TokenKind tt, JSBool preorder)
{
    JSOp op;

    kid = SetLvalKid(cx, ts, tc, pn, kid, incop_name_str[tt == TOK_DEC]);
    if (!kid)
        return JS_FALSE;
    switch (kid->pn_type) {
      case TOK_NAME:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCNAME : JSOP_NAMEINC)
             : (preorder ? JSOP_DECNAME : JSOP_NAMEDEC);
        NoteLValue(cx, kid, tc);
        break;

      case TOK_DOT:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCPROP : JSOP_PROPINC)
             : (preorder ? JSOP_DECPROP : JSOP_PROPDEC);
        break;

      case TOK_LP:
        if (!MakeSetCall(cx, kid, tc, JSMSG_BAD_INCOP_OPERAND))
            return JS_FALSE;
        
#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        if (kid->pn_op == JSOP_XMLNAME)
            kid->pn_op = JSOP_SETXMLNAME;
        
#endif
      case TOK_LB:
        op = (tt == TOK_INC)
             ? (preorder ? JSOP_INCELEM : JSOP_ELEMINC)
             : (preorder ? JSOP_DECELEM : JSOP_ELEMDEC);
        break;

      default:
        JS_ASSERT(0);
        op = JSOP_NOP;
    }
    pn->pn_op = op;
    return JS_TRUE;
}

JSParseNode *
Parser::unaryExpr()
{
    JSParseNode *pn, *pn2;

    JS_CHECK_RECURSION(context, return NULL);

    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    switch (tt) {
      case TOK_UNARYOP:
      case TOK_PLUS:
      case TOK_MINUS:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_UNARYOP;      
        pn->pn_op = tokenStream.currentToken().t_op;
        pn2 = unaryExpr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
        break;

      case TOK_INC:
      case TOK_DEC:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn2 = memberExpr(JS_TRUE);
        if (!pn2)
            return NULL;
        if (!SetIncOpKid(context, &tokenStream, tc, pn, pn2, tt, JS_TRUE))
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        break;

      case TOK_DELETE:
      {
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn2 = unaryExpr();
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;

        




        if (foldConstants && !js_FoldConstants(context, pn2, tc))
            return NULL;
        switch (pn2->pn_type) {
          case TOK_LP:
            if (!(pn2->pn_xflags & PNX_SETCALL)) {
                



                if (!MakeSetCall(context, pn2, tc, JSMSG_BAD_DELETE_OPERAND))
                    return NULL;
                pn2->pn_xflags &= ~PNX_SETCALL;
            }
            break;
          case TOK_NAME:
            if (!ReportStrictModeError(context, &tokenStream, tc, pn,
                                       JSMSG_DEPRECATED_DELETE_OPERAND)) {
                return NULL;
            }
            pn2->pn_op = JSOP_DELNAME;
            if (pn2->pn_atom == context->runtime->atomState.argumentsAtom) {
                tc->flags |= TCF_FUN_HEAVYWEIGHT;
                tc->countArgumentsUse(pn2);
            }
            break;
          default:;
        }
        pn->pn_kid = pn2;
        break;
      }
      case TOK_ERROR:
        return NULL;

      default:
        tokenStream.ungetToken();
        pn = memberExpr(JS_TRUE);
        if (!pn)
            return NULL;

        
        if (tokenStream.onCurrentLine(pn->pn_pos)) {
            tt = tokenStream.peekTokenSameLine(TSF_OPERAND);
            if (tt == TOK_INC || tt == TOK_DEC) {
                (void) tokenStream.getToken();
                pn2 = UnaryNode::create(tc);
                if (!pn2)
                    return NULL;
                if (!SetIncOpKid(context, &tokenStream, tc, pn2, pn, tt, JS_FALSE))
                    return NULL;
                pn2->pn_pos.begin = pn->pn_pos.begin;
                pn = pn2;
            }
        }
        break;
    }
    return pn;
}

#if JS_HAS_GENERATORS






















class CompExprTransplanter {
    JSParseNode     *root;
    JSTreeContext   *tc;
    bool            genexp;
    uintN           adjust;
    uintN           funcLevel;

  public:
    CompExprTransplanter(JSParseNode *pn, JSTreeContext *tc, bool ge, uintN adj)
      : root(pn), tc(tc), genexp(ge), adjust(adj), funcLevel(0)
    {
    }

    bool transplant(JSParseNode *pn);
};










class GenexpGuard {
    JSTreeContext   *tc;
    uint32          startYieldCount;
    uint32          startArgumentsCount;

  public:
    explicit GenexpGuard(JSTreeContext *tc)
      : tc(tc)
    {
        if (tc->parenDepth == 0) {
            tc->yieldCount = tc->argumentsCount = 0;
            tc->yieldNode = tc->argumentsNode = NULL;
        }
        startYieldCount = tc->yieldCount;
        startArgumentsCount = tc->argumentsCount;
        tc->parenDepth++;
    }

    void endBody();
    bool checkValidBody(JSParseNode *pn);
};

void
GenexpGuard::endBody()
{
    tc->parenDepth--;
}

bool
GenexpGuard::checkValidBody(JSParseNode *pn)
{
    if (tc->yieldCount > startYieldCount) {
        JSParseNode *errorNode = tc->yieldNode;
        if (!errorNode)
            errorNode = pn;
        tc->parser->reportErrorNumber(errorNode, JSREPORT_ERROR, JSMSG_BAD_GENEXP_BODY, js_yield_str);
        return false;
    }

    if (tc->argumentsCount > startArgumentsCount) {
        JSParseNode *errorNode = tc->argumentsNode;
        if (!errorNode)
            errorNode = pn;
        tc->parser->reportErrorNumber(errorNode, JSREPORT_ERROR, JSMSG_BAD_GENEXP_BODY, js_arguments_str);
        return false;
    }

    return true;
}






static bool
BumpStaticLevel(JSParseNode *pn, JSTreeContext *tc)
{
    if (!pn->pn_cookie.isFree()) {
        uintN level = pn->pn_cookie.level() + 1;

        JS_ASSERT(level >= tc->staticLevel);
        if (level >= UpvarCookie::FREE_LEVEL) {
            JS_ReportErrorNumber(tc->parser->context, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_DEEP, js_function_str);
            return false;
        }

        pn->pn_cookie.set(level, pn->pn_cookie.slot());
    }
    return true;
}

static void
AdjustBlockId(JSParseNode *pn, uintN adjust, JSTreeContext *tc)
{
    JS_ASSERT(pn->pn_arity == PN_LIST || pn->pn_arity == PN_FUNC || pn->pn_arity == PN_NAME);
    pn->pn_blockid += adjust;
    if (pn->pn_blockid >= tc->blockidGen)
        tc->blockidGen = pn->pn_blockid + 1;
}

bool
CompExprTransplanter::transplant(JSParseNode *pn)
{
    if (!pn)
        return true;

    switch (pn->pn_arity) {
      case PN_LIST:
        for (JSParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!transplant(pn2))
                return false;
        }
        if (pn->pn_pos >= root->pn_pos)
            AdjustBlockId(pn, adjust, tc);
        break;

      case PN_TERNARY:
        if (!transplant(pn->pn_kid1) ||
            !transplant(pn->pn_kid2) ||
            !transplant(pn->pn_kid3))
            return false;
        break;

      case PN_BINARY:
        if (!transplant(pn->pn_left))
            return false;

        
        if (pn->pn_right != pn->pn_left) {
            if (!transplant(pn->pn_right))
                return false;
        }
        break;

      case PN_UNARY:
        if (!transplant(pn->pn_kid))
            return false;
        break;

      case PN_FUNC:
      {
        









        JSFunctionBox *funbox = pn->pn_funbox;

        funbox->level = tc->staticLevel + funcLevel;
        if (++funcLevel == 1 && genexp) {
            JSFunctionBox *parent = tc->funbox;

            JSFunctionBox **funboxp = &tc->parent->functionList;
            while (*funboxp != funbox)
                funboxp = &(*funboxp)->siblings;
            *funboxp = funbox->siblings;

            funbox->parent = parent;
            funbox->siblings = parent->kids;
            parent->kids = funbox;
            funbox->level = tc->staticLevel;
        }
        
      }

      case PN_NAME:
        if (!transplant(pn->maybeExpr()))
            return false;
        if (pn->pn_arity == PN_FUNC)
            --funcLevel;

        if (pn->pn_defn) {
            if (genexp && !BumpStaticLevel(pn, tc))
                return false;
        } else if (pn->pn_used) {
            JS_ASSERT(pn->pn_op != JSOP_NOP);
            JS_ASSERT(pn->pn_cookie.isFree());

            JSDefinition *dn = pn->pn_lexdef;
            JS_ASSERT(dn->pn_defn);

            








            if (dn->isPlaceholder() && dn->pn_pos >= root->pn_pos && dn->dn_uses == pn) {
                if (genexp && !BumpStaticLevel(dn, tc))
                    return false;
                AdjustBlockId(dn, adjust, tc);
            }

            JSAtom *atom = pn->pn_atom;
#ifdef DEBUG
            JSStmtInfo *stmt = js_LexicalLookup(tc, atom, NULL);
            JS_ASSERT(!stmt || stmt != tc->topStmt);
#endif
            if (genexp && PN_OP(dn) != JSOP_CALLEE) {
                JS_ASSERT(!tc->decls.lookupFirst(atom));

                if (dn->pn_pos < root->pn_pos || dn->isPlaceholder()) {
                    if (dn->pn_pos >= root->pn_pos) {
                        tc->parent->lexdeps->remove(atom);
                    } else {
                        JSDefinition *dn2 = (JSDefinition *)NameNode::create(atom, tc);
                        if (!dn2)
                            return false;

                        dn2->pn_type = TOK_NAME;
                        dn2->pn_op = JSOP_NOP;
                        dn2->pn_defn = true;
                        dn2->pn_dflags |= PND_PLACEHOLDER;
                        dn2->pn_pos = root->pn_pos;

                        JSParseNode **pnup = &dn->dn_uses;
                        JSParseNode *pnu;
                        while ((pnu = *pnup) != NULL && pnu->pn_pos >= root->pn_pos) {
                            pnu->pn_lexdef = dn2;
                            dn2->pn_dflags |= pnu->pn_dflags & PND_USE2DEF_FLAGS;
                            pnup = &pnu->pn_link;
                        }
                        dn2->dn_uses = dn->dn_uses;
                        dn->dn_uses = *pnup;
                        *pnup = NULL;

                        dn = dn2;
                    }
                    if (!tc->lexdeps->put(atom, dn))
                        return false;
                }
            }
        }

        if (pn->pn_pos >= root->pn_pos)
            AdjustBlockId(pn, adjust, tc);
        break;

      case PN_NAMESET:
        if (!transplant(pn->pn_tree))
            return false;
        break;
    }
    return true;
}










JSParseNode *
Parser::comprehensionTail(JSParseNode *kid, uintN blockid, bool isGenexp,
                          TokenKind type, JSOp op)
{
    uintN adjust;
    JSParseNode *pn, *pn2, *pn3, **pnp;
    JSStmtInfo stmtInfo;
    BindData data;
    TokenKind tt;
    JSAtom *atom;

    JS_ASSERT(tokenStream.currentToken().type == TOK_FOR);

    if (type == TOK_SEMI) {
        




        pn = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
        if (!pn)
            return NULL;
        adjust = pn->pn_blockid - blockid;
    } else {
        JS_ASSERT(type == TOK_ARRAYPUSH);

        











        adjust = tc->blockid();
        pn = PushLexicalScope(context, &tokenStream, tc, &stmtInfo);
        if (!pn)
            return NULL;

        JS_ASSERT(blockid <= pn->pn_blockid);
        JS_ASSERT(blockid < tc->blockidGen);
        JS_ASSERT(tc->bodyid < blockid);
        pn->pn_blockid = stmtInfo.blockid = blockid;
        JS_ASSERT(adjust < blockid);
        adjust = blockid - adjust;
    }

    pnp = &pn->pn_expr;

    CompExprTransplanter transplanter(kid, tc, type == TOK_SEMI, adjust);
    transplanter.transplant(kid);

    data.pn = NULL;
    data.op = JSOP_NOP;
    data.binder = BindLet;
    data.let.overflow = JSMSG_ARRAY_INIT_TOO_BIG;

    do {
        




        pn2 = BinaryNode::create(tc);
        if (!pn2)
            return NULL;

        pn2->pn_op = JSOP_ITER;
        pn2->pn_iflags = JSITER_ENUMERATE;
        if (tokenStream.matchToken(TOK_NAME)) {
            if (tokenStream.currentToken().t_atom == context->runtime->atomState.eachAtom)
                pn2->pn_iflags |= JSITER_FOREACH;
            else
                tokenStream.ungetToken();
        }
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

        GenexpGuard guard(tc);

        atom = NULL;
        tt = tokenStream.getToken();
        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn3 = primaryExpr(tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn3)
                return NULL;
            break;
#endif

          case TOK_NAME:
            atom = tokenStream.currentToken().t_atom;

            






            pn3 = NewBindingNode(atom, tc, true);
            if (!pn3)
                return NULL;
            break;

          default:
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return NULL;
        }

        MUST_MATCH_TOKEN(TOK_IN, JSMSG_IN_AFTER_FOR_NAME);
        JSParseNode *pn4 = expr();
        if (!pn4)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

        guard.endBody();

        if (isGenexp && !guard.checkValidBody(pn2))
            return NULL;

        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            if (!CheckDestructuring(context, &data, pn3, tc))
                return NULL;

            if (versionNumber() == JSVERSION_1_7) {
                
                if (pn3->pn_type != TOK_RB || pn3->pn_count != 2) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_FOR_LEFTSIDE);
                    return NULL;
                }

                JS_ASSERT(pn2->pn_op == JSOP_ITER);
                JS_ASSERT(pn2->pn_iflags & JSITER_ENUMERATE);
                if (!(pn2->pn_iflags & JSITER_FOREACH))
                    pn2->pn_iflags |= JSITER_FOREACH | JSITER_KEYVALUE;
            }
            break;
#endif

          case TOK_NAME:
            data.pn = pn3;
            if (!data.binder(context, &data, atom, tc))
                return NULL;
            break;

          default:;
        }

        pn2->pn_left = JSParseNode::newBinaryOrAppend(TOK_IN, JSOP_NOP, pn3, pn4, tc);
        if (!pn2->pn_left)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_right;
    } while (tokenStream.matchToken(TOK_FOR));

    if (tokenStream.matchToken(TOK_IF)) {
        pn2 = TernaryNode::create(tc);
        if (!pn2)
            return NULL;
        pn2->pn_kid1 = condition();
        if (!pn2->pn_kid1)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_kid2;
    }

    if (!maybeNoteGenerator())
        return NULL;

    pn2 = UnaryNode::create(tc);
    if (!pn2)
        return NULL;
    pn2->pn_type = type;
    pn2->pn_op = op;
    pn2->pn_kid = kid;
    *pnp = pn2;

    PopStatement(tc);
    return pn;
}

#if JS_HAS_GENERATOR_EXPRS
















JSParseNode *
Parser::generatorExpr(JSParseNode *kid)
{
    
    JSParseNode *pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_type = TOK_YIELD;
    pn->pn_op = JSOP_YIELD;
    pn->pn_parens = true;
    pn->pn_pos = kid->pn_pos;
    pn->pn_kid = kid;
    pn->pn_hidden = true;

    
    JSParseNode *genfn = FunctionNode::create(tc);
    if (!genfn)
        return NULL;
    genfn->pn_type = TOK_FUNCTION;
    genfn->pn_op = JSOP_LAMBDA;
    JS_ASSERT(!genfn->pn_body);
    genfn->pn_dflags = PND_FUNARG;

    {
        JSTreeContext *outertc = tc;
        JSTreeContext gentc(tc->parser);
        if (!gentc.init(context))
            return NULL;

        JSFunctionBox *funbox = EnterFunction(genfn, &gentc);
        if (!funbox)
            return NULL;

        






        if (outertc->flags & TCF_HAS_SHARPS) {
            gentc.flags |= TCF_IN_FUNCTION;
            if (!gentc.ensureSharpSlots())
                return NULL;
        }

        






        gentc.flags |= TCF_FUN_IS_GENERATOR | TCF_GENEXP_LAMBDA |
                       (tc->flags & (TCF_FUN_FLAGS & ~TCF_FUN_PARAM_ARGUMENTS));
        funbox->tcflags |= gentc.flags;
        genfn->pn_funbox = funbox;
        genfn->pn_blockid = gentc.bodyid;

        JSParseNode *body = comprehensionTail(pn, outertc->blockid(), true);
        if (!body)
            return NULL;
        JS_ASSERT(!genfn->pn_body);
        genfn->pn_body = body;
        genfn->pn_pos.begin = body->pn_pos.begin = kid->pn_pos.begin;
        genfn->pn_pos.end = body->pn_pos.end = tokenStream.currentToken().pos.end;

        if (!LeaveFunction(genfn, &gentc))
            return NULL;
    }

    



    JSParseNode *result = ListNode::create(tc);
    if (!result)
        return NULL;
    result->pn_type = TOK_LP;
    result->pn_op = JSOP_CALL;
    result->pn_pos.begin = genfn->pn_pos.begin;
    result->initList(genfn);
    return result;
}

static const char js_generator_str[] = "generator";

#endif 
#endif 









bool
Parser::maybeNoteGenerator()
{
    if (tc->yieldCount > 0) {
        tc->flags |= TCF_FUN_IS_GENERATOR;
        if (!tc->inFunction()) {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_RETURN_OR_YIELD, js_yield_str);
            return false;
        }
    }
    return true;
}

JSBool
Parser::argumentList(JSParseNode *listNode)
{
    if (tokenStream.matchToken(TOK_RP, TSF_OPERAND))
        return JS_TRUE;

    GenexpGuard guard(tc);
    bool arg0 = true;

    do {
        JSParseNode *argNode = assignExpr();
        if (!argNode)
            return JS_FALSE;
        if (arg0) {
            guard.endBody();
            arg0 = false;
        }

#if JS_HAS_GENERATORS
        if (argNode->pn_type == TOK_YIELD &&
            !argNode->pn_parens &&
            tokenStream.peekToken() == TOK_COMMA) {
            reportErrorNumber(argNode, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX, js_yield_str);
            return JS_FALSE;
        }
#endif
#if JS_HAS_GENERATOR_EXPRS
        if (tokenStream.matchToken(TOK_FOR)) {
            if (!guard.checkValidBody(argNode))
                return JS_FALSE;
            argNode = generatorExpr(argNode);
            if (!argNode)
                return JS_FALSE;
            if (listNode->pn_count > 1 ||
                tokenStream.peekToken() == TOK_COMMA) {
                reportErrorNumber(argNode, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX,
                                  js_generator_str);
                return JS_FALSE;
            }
        }
#endif
        listNode->append(argNode);
    } while (tokenStream.matchToken(TOK_COMMA));

    if (!maybeNoteGenerator())
        return JS_FALSE;

    if (tokenStream.getToken() != TOK_RP) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_PAREN_AFTER_ARGS);
        return JS_FALSE;
    }
    return JS_TRUE;
}


static JSParseNode *
CheckForImmediatelyAppliedLambda(JSParseNode *pn)
{
    if (pn->pn_type == TOK_FUNCTION) {
        JS_ASSERT(pn->pn_arity == PN_FUNC);

        JSFunctionBox *funbox = pn->pn_funbox;
        JS_ASSERT((funbox->function())->flags & JSFUN_LAMBDA);
        if (!(funbox->tcflags & (TCF_FUN_USES_ARGUMENTS | TCF_FUN_USES_OWN_NAME)))
            pn->pn_dflags &= ~PND_FUNARG;
    }
    return pn;
}

JSParseNode *
Parser::memberExpr(JSBool allowCallSyntax)
{
    JSParseNode *pn, *pn2, *pn3;

    JS_CHECK_RECURSION(context, return NULL);

    
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);
    if (tt == TOK_NEW) {
        pn = ListNode::create(tc);
        if (!pn)
            return NULL;
        pn2 = memberExpr(JS_FALSE);
        if (!pn2)
            return NULL;
        pn2 = CheckForImmediatelyAppliedLambda(pn2);
        pn->pn_op = JSOP_NEW;
        pn->initList(pn2);
        pn->pn_pos.begin = pn2->pn_pos.begin;

        if (tokenStream.matchToken(TOK_LP) && !argumentList(pn))
            return NULL;
        if (pn->pn_count > ARGC_LIMIT) {
            JS_ReportErrorNumber(context, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_MANY_CON_ARGS);
            return NULL;
        }
        pn->pn_pos.end = pn->last()->pn_pos.end;
    } else {
        pn = primaryExpr(tt, JS_FALSE);
        if (!pn)
            return NULL;

        if (pn->pn_type == TOK_ANYNAME ||
            pn->pn_type == TOK_AT ||
            pn->pn_type == TOK_DBLCOLON) {
            pn2 = NewOrRecycledNode(tc);
            if (!pn2)
                return NULL;
            pn2->pn_type = TOK_UNARYOP;
            pn2->pn_pos = pn->pn_pos;
            pn2->pn_op = JSOP_XMLNAME;
            pn2->pn_arity = PN_UNARY;
            pn2->pn_parens = false;
            pn2->pn_kid = pn;
            pn = pn2;
        }
    }

    while ((tt = tokenStream.getToken()) > TOK_EOF) {
        if (tt == TOK_DOT) {
            pn2 = NameNode::create(NULL, tc);
            if (!pn2)
                return NULL;
#if JS_HAS_XML_SUPPORT
            tt = tokenStream.getToken(TSF_OPERAND | TSF_KEYWORD_IS_NAME);

            
            JSParseNode *oldWith = tc->innermostWith;
            JSStmtInfo stmtInfo;
            if (tt == TOK_LP) {
                tc->innermostWith = pn;
                js_PushStatement(tc, &stmtInfo, STMT_WITH, -1);
            }

            pn3 = primaryExpr(tt, JS_TRUE);
            if (!pn3)
                return NULL;

            if (tt == TOK_LP) {
                tc->innermostWith = oldWith;
                PopStatement(tc);
            }

            
            if (tt == TOK_NAME && pn3->pn_type == TOK_NAME) {
                pn2->pn_op = JSOP_GETPROP;
                pn2->pn_expr = pn;
                pn2->pn_atom = pn3->pn_atom;
                RecycleTree(pn3, tc);
            } else {
                if (tt == TOK_LP) {
                    pn2->pn_type = TOK_FILTER;
                    pn2->pn_op = JSOP_FILTER;

                    
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                } else if (TokenKindIsXML(PN_TYPE(pn3))) {
                    pn2->pn_type = TOK_LB;
                    pn2->pn_op = JSOP_GETELEM;
                } else {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                    return NULL;
                }
                pn2->pn_arity = PN_BINARY;
                pn2->pn_left = pn;
                pn2->pn_right = pn3;
            }
#else
            MUST_MATCH_TOKEN_WITH_FLAGS(TOK_NAME, JSMSG_NAME_AFTER_DOT, TSF_KEYWORD_IS_NAME);
            pn2->pn_op = JSOP_GETPROP;
            pn2->pn_expr = pn;
            pn2->pn_atom = tokenStream.currentToken().t_atom;
#endif
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;
#if JS_HAS_XML_SUPPORT
        } else if (tt == TOK_DBLDOT) {
            pn2 = BinaryNode::create(tc);
            if (!pn2)
                return NULL;
            tt = tokenStream.getToken(TSF_OPERAND | TSF_KEYWORD_IS_NAME);
            pn3 = primaryExpr(tt, JS_TRUE);
            if (!pn3)
                return NULL;
            tt = PN_TYPE(pn3);
            if (tt == TOK_NAME && !pn3->pn_parens) {
                pn3->pn_type = TOK_STRING;
                pn3->pn_arity = PN_NULLARY;
                pn3->pn_op = JSOP_QNAMEPART;
            } else if (!TokenKindIsXML(tt)) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_NAME_AFTER_DOT);
                return NULL;
            }
            pn2->pn_op = JSOP_DESCENDANTS;
            pn2->pn_left = pn;
            pn2->pn_right = pn3;
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;
#endif
        } else if (tt == TOK_LB) {
            pn2 = BinaryNode::create(tc);
            if (!pn2)
                return NULL;
            pn3 = expr();
            if (!pn3)
                return NULL;

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_IN_INDEX);
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;

            






            do {
                if (pn3->pn_type == TOK_STRING) {
                    jsuint index;

                    if (!js_IdIsIndex(ATOM_TO_JSID(pn3->pn_atom), &index)) {
                        pn2->pn_type = TOK_DOT;
                        pn2->pn_op = JSOP_GETPROP;
                        pn2->pn_arity = PN_NAME;
                        pn2->pn_expr = pn;
                        pn2->pn_atom = pn3->pn_atom;
                        break;
                    }
                    pn3->pn_type = TOK_NUMBER;
                    pn3->pn_op = JSOP_DOUBLE;
                    pn3->pn_dval = index;
                }
                pn2->pn_op = JSOP_GETELEM;
                pn2->pn_left = pn;
                pn2->pn_right = pn3;
            } while (0);
        } else if (allowCallSyntax && tt == TOK_LP) {
            pn2 = ListNode::create(tc);
            if (!pn2)
                return NULL;
            pn2->pn_op = JSOP_CALL;

            pn = CheckForImmediatelyAppliedLambda(pn);
            if (pn->pn_op == JSOP_NAME) {
                if (pn->pn_atom == context->runtime->atomState.evalAtom) {
                    
                    pn2->pn_op = JSOP_EVAL;
                    tc->noteCallsEval();
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                    



                    if (!tc->inStrictMode())
                        tc->noteHasExtensibleScope();
                }
            } else if (pn->pn_op == JSOP_GETPROP) {
                
                if (pn->pn_atom == context->runtime->atomState.applyAtom)
                    pn2->pn_op = JSOP_FUNAPPLY;
                else if (pn->pn_atom == context->runtime->atomState.callAtom)
                    pn2->pn_op = JSOP_FUNCALL;
            }

            pn2->initList(pn);
            pn2->pn_pos.begin = pn->pn_pos.begin;

            if (!argumentList(pn2))
                return NULL;
            if (pn2->pn_count > ARGC_LIMIT) {
                JS_ReportErrorNumber(context, js_GetErrorMessage, NULL,
                                     JSMSG_TOO_MANY_FUN_ARGS);
                return NULL;
            }
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;
        } else {
            tokenStream.ungetToken();
            return pn;
        }

        pn = pn2;
    }
    if (tt == TOK_ERROR)
        return NULL;
    return pn;
}

JSParseNode *
Parser::bracketedExpr()
{
    uintN oldflags;
    JSParseNode *pn;

    




    oldflags = tc->flags;
    tc->flags &= ~TCF_IN_FOR_INIT;
    pn = expr();
    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}

#if JS_HAS_XML_SUPPORT

JSParseNode *
Parser::endBracketedExpr()
{
    JSParseNode *pn;

    pn = bracketedExpr();
    if (!pn)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ATTR_EXPR);
    return pn;
}




















































JSParseNode *
Parser::propertySelector()
{
    JSParseNode *pn;

    pn = NullaryNode::create(tc);
    if (!pn)
        return NULL;
    if (pn->pn_type == TOK_STAR) {
        pn->pn_type = TOK_ANYNAME;
        pn->pn_op = JSOP_ANYNAME;
        pn->pn_atom = context->runtime->atomState.starAtom;
    } else {
        JS_ASSERT(pn->pn_type == TOK_NAME);
        pn->pn_op = JSOP_QNAMEPART;
        pn->pn_arity = PN_NAME;
        pn->pn_atom = tokenStream.currentToken().t_atom;
        pn->pn_cookie.makeFree();
    }
    return pn;
}

JSParseNode *
Parser::qualifiedSuffix(JSParseNode *pn)
{
    JSParseNode *pn2, *pn3;
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_DBLCOLON);
    pn2 = NameNode::create(NULL, tc);
    if (!pn2)
        return NULL;

    
    if (pn->pn_op == JSOP_QNAMEPART)
        pn->pn_op = JSOP_NAME;

    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
    if (tt == TOK_STAR || tt == TOK_NAME) {
        
        pn2->pn_op = JSOP_QNAMECONST;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->pn_atom = (tt == TOK_STAR)
                       ? context->runtime->atomState.starAtom
                       : tokenStream.currentToken().t_atom;
        pn2->pn_expr = pn;
        pn2->pn_cookie.makeFree();
        return pn2;
    }

    if (tt != TOK_LB) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    pn3 = endBracketedExpr();
    if (!pn3)
        return NULL;

    pn2->pn_op = JSOP_QNAME;
    pn2->pn_arity = PN_BINARY;
    pn2->pn_pos.begin = pn->pn_pos.begin;
    pn2->pn_pos.end = pn3->pn_pos.end;
    pn2->pn_left = pn;
    pn2->pn_right = pn3;
    return pn2;
}

JSParseNode *
Parser::qualifiedIdentifier()
{
    JSParseNode *pn;

    pn = propertySelector();
    if (!pn)
        return NULL;
    if (tokenStream.matchToken(TOK_DBLCOLON)) {
        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        pn = qualifiedSuffix(pn);
    }
    return pn;
}

JSParseNode *
Parser::attributeIdentifier()
{
    JSParseNode *pn, *pn2;
    TokenKind tt;

    JS_ASSERT(tokenStream.currentToken().type == TOK_AT);
    pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;
    pn->pn_op = JSOP_TOATTRNAME;
    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
    if (tt == TOK_STAR || tt == TOK_NAME) {
        pn2 = qualifiedIdentifier();
    } else if (tt == TOK_LB) {
        pn2 = endBracketedExpr();
    } else {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    if (!pn2)
        return NULL;
    pn->pn_kid = pn2;
    return pn;
}




JSParseNode *
Parser::xmlExpr(JSBool inTag)
{
    JSParseNode *pn, *pn2;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LC);
    pn = UnaryNode::create(tc);
    if (!pn)
        return NULL;

    





    bool oldflag = tokenStream.isXMLTagMode();
    tokenStream.setXMLTagMode(false);
    pn2 = expr();
    if (!pn2)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_XML_EXPR);
    tokenStream.setXMLTagMode(oldflag);
    pn->pn_kid = pn2;
    pn->pn_op = inTag ? JSOP_XMLTAGEXPR : JSOP_XMLELTEXPR;
    return pn;
}







JSParseNode *
Parser::xmlAtomNode()
{
    JSParseNode *pn = NullaryNode::create(tc);
    if (!pn)
        return NULL;
    const Token &tok = tokenStream.currentToken();
    pn->pn_op = tok.t_op;
    pn->pn_atom = tok.t_atom;
    if (tok.type == TOK_XMLPI)
        pn->pn_atom2 = tok.t_atom2;
    return pn;
}













JSParseNode *
Parser::xmlNameExpr()
{
    JSParseNode *pn, *pn2, *list;
    TokenKind tt;

    pn = list = NULL;
    do {
        tt = tokenStream.currentToken().type;
        if (tt == TOK_LC) {
            pn2 = xmlExpr(JS_TRUE);
            if (!pn2)
                return NULL;
        } else {
            JS_ASSERT(tt == TOK_XMLNAME);
            pn2 = xmlAtomNode();
            if (!pn2)
                return NULL;
        }

        if (!pn) {
            pn = pn2;
        } else {
            if (!list) {
                list = ListNode::create(tc);
                if (!list)
                    return NULL;
                list->pn_type = TOK_XMLNAME;
                list->pn_pos.begin = pn->pn_pos.begin;
                list->initList(pn);
                list->pn_xflags = PNX_CANTFOLD;
                pn = list;
            }
            pn->pn_pos.end = pn2->pn_pos.end;
            pn->append(pn2);
        }
    } while ((tt = tokenStream.getToken()) == TOK_XMLNAME || tt == TOK_LC);

    tokenStream.ungetToken();
    return pn;
}





#define XML_FOLDABLE(pn)        ((pn)->pn_arity == PN_LIST                    \
                                 ? ((pn)->pn_xflags & PNX_CANTFOLD) == 0      \
                                 : (pn)->pn_type != TOK_LC)


















JSParseNode *
Parser::xmlTagContent(TokenKind tagtype, JSAtom **namep)
{
    JSParseNode *pn, *pn2, *list;
    TokenKind tt;

    pn = xmlNameExpr();
    if (!pn)
        return NULL;
    *namep = (pn->pn_arity == PN_NULLARY) ? pn->pn_atom : NULL;
    list = NULL;

    while (tokenStream.matchToken(TOK_XMLSPACE)) {
        tt = tokenStream.getToken();
        if (tt != TOK_XMLNAME && tt != TOK_LC) {
            tokenStream.ungetToken();
            break;
        }

        pn2 = xmlNameExpr();
        if (!pn2)
            return NULL;
        if (!list) {
            list = ListNode::create(tc);
            if (!list)
                return NULL;
            list->pn_type = tagtype;
            list->pn_pos.begin = pn->pn_pos.begin;
            list->initList(pn);
            pn = list;
        }
        pn->append(pn2);
        if (!XML_FOLDABLE(pn2))
            pn->pn_xflags |= PNX_CANTFOLD;

        tokenStream.matchToken(TOK_XMLSPACE);
        MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_NO_ASSIGN_IN_XML_ATTR);
        tokenStream.matchToken(TOK_XMLSPACE);

        tt = tokenStream.getToken();
        if (tt == TOK_XMLATTR) {
            pn2 = xmlAtomNode();
        } else if (tt == TOK_LC) {
            pn2 = xmlExpr(JS_TRUE);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else {
            reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_ATTR_VALUE);
            return NULL;
        }
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->append(pn2);
    }

    return pn;
}

#define XML_CHECK_FOR_ERROR_AND_EOF(tt,result)                                              \
    JS_BEGIN_MACRO                                                                          \
        if ((tt) <= TOK_EOF) {                                                              \
            if ((tt) == TOK_EOF) {                                                          \
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_END_OF_XML_SOURCE);           \
            }                                                                               \
            return result;                                                                  \
        }                                                                                   \
    JS_END_MACRO





JSBool
Parser::xmlElementContent(JSParseNode *pn)
{
    tokenStream.setXMLTagMode(false);
    for (;;) {
        TokenKind tt = tokenStream.getToken(TSF_XMLTEXTMODE);
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);

        JS_ASSERT(tt == TOK_XMLSPACE || tt == TOK_XMLTEXT);
        JSAtom *textAtom = tokenStream.currentToken().t_atom;
        if (textAtom) {
            
            JSParseNode *pn2 = xmlAtomNode();
            if (!pn2)
                return JS_FALSE;
            pn->pn_pos.end = pn2->pn_pos.end;
            pn->append(pn2);
        }

        tt = tokenStream.getToken(TSF_OPERAND);
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);
        if (tt == TOK_XMLETAGO)
            break;

        JSParseNode *pn2;
        if (tt == TOK_LC) {
            pn2 = xmlExpr(JS_FALSE);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else if (tt == TOK_XMLSTAGO) {
            pn2 = xmlElementOrList(JS_FALSE);
            if (pn2) {
                pn2->pn_xflags &= ~PNX_XMLROOT;
                pn->pn_xflags |= pn2->pn_xflags;
            }
        } else {
            JS_ASSERT(tt == TOK_XMLCDATA || tt == TOK_XMLCOMMENT ||
                      tt == TOK_XMLPI);
            pn2 = xmlAtomNode();
        }
        if (!pn2)
            return JS_FALSE;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->append(pn2);
    }
    tokenStream.setXMLTagMode(true);

    JS_ASSERT(tokenStream.currentToken().type == TOK_XMLETAGO);
    return JS_TRUE;
}




JSParseNode *
Parser::xmlElementOrList(JSBool allowList)
{
    JSParseNode *pn, *pn2, *list;
    TokenKind tt;
    JSAtom *startAtom, *endAtom;

    JS_CHECK_RECURSION(context, return NULL);

    JS_ASSERT(tokenStream.currentToken().type == TOK_XMLSTAGO);
    pn = ListNode::create(tc);
    if (!pn)
        return NULL;

    tokenStream.setXMLTagMode(true);
    tt = tokenStream.getToken();
    if (tt == TOK_ERROR)
        return NULL;

    if (tt == TOK_XMLNAME || tt == TOK_LC) {
        


        pn2 = xmlTagContent(TOK_XMLSTAGO, &startAtom);
        if (!pn2)
            return NULL;
        tokenStream.matchToken(TOK_XMLSPACE);

        tt = tokenStream.getToken();
        if (tt == TOK_XMLPTAGC) {
            
            if (pn2->pn_type == TOK_XMLSTAGO) {
                pn->makeEmpty();
                RecycleTree(pn, tc);
                pn = pn2;
            } else {
                JS_ASSERT(pn2->pn_type == TOK_XMLNAME ||
                          pn2->pn_type == TOK_LC);
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
            }
            pn->pn_type = TOK_XMLPTAGC;
            pn->pn_xflags |= PNX_XMLROOT;
        } else {
            
            if (tt != TOK_XMLTAGC) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            pn2->pn_pos.end = tokenStream.currentToken().pos.end;

            
            if (pn2->pn_type != TOK_XMLSTAGO) {
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
                pn2 = pn;
                pn = ListNode::create(tc);
                if (!pn)
                    return NULL;
            }

            
            pn->pn_type = TOK_XMLELEM;
            pn->pn_pos.begin = pn2->pn_pos.begin;
            pn->initList(pn2);
            if (!XML_FOLDABLE(pn2))
                pn->pn_xflags |= PNX_CANTFOLD;
            pn->pn_xflags |= PNX_XMLROOT;

            
            if (!xmlElementContent(pn))
                return NULL;

            tt = tokenStream.getToken();
            XML_CHECK_FOR_ERROR_AND_EOF(tt, NULL);
            if (tt != TOK_XMLNAME && tt != TOK_LC) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }

            
            pn2 = xmlTagContent(TOK_XMLETAGO, &endAtom);
            if (!pn2)
                return NULL;
            if (pn2->pn_type == TOK_XMLETAGO) {
                
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            if (endAtom && startAtom && endAtom != startAtom) {
                
                reportErrorNumber(pn2, JSREPORT_UC | JSREPORT_ERROR, JSMSG_XML_TAG_NAME_MISMATCH,
                                  startAtom->chars());
                return NULL;
            }

            
            JS_ASSERT(pn2->pn_type == TOK_XMLNAME || pn2->pn_type == TOK_LC);
            list = ListNode::create(tc);
            if (!list)
                return NULL;
            list->pn_type = TOK_XMLETAGO;
            list->initList(pn2);
            pn->append(list);
            if (!XML_FOLDABLE(pn2)) {
                list->pn_xflags |= PNX_CANTFOLD;
                pn->pn_xflags |= PNX_CANTFOLD;
            }

            tokenStream.matchToken(TOK_XMLSPACE);
            MUST_MATCH_TOKEN(TOK_XMLTAGC, JSMSG_BAD_XML_TAG_SYNTAX);
        }

        
        pn->pn_op = JSOP_TOXML;
    } else if (allowList && tt == TOK_XMLTAGC) {
        
        pn->pn_type = TOK_XMLLIST;
        pn->pn_op = JSOP_TOXMLLIST;
        pn->makeEmpty();
        pn->pn_xflags |= PNX_XMLROOT;
        if (!xmlElementContent(pn))
            return NULL;

        MUST_MATCH_TOKEN(TOK_XMLTAGC, JSMSG_BAD_XML_LIST_SYNTAX);
    } else {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_NAME_SYNTAX);
        return NULL;
    }
    tokenStream.setXMLTagMode(false);

    pn->pn_pos.end = tokenStream.currentToken().pos.end;
    return pn;
}

JSParseNode *
Parser::xmlElementOrListRoot(JSBool allowList)
{
    





    bool hadXML = tokenStream.hasXML();
    tokenStream.setXML(true);
    JSParseNode *pn = xmlElementOrList(allowList);
    tokenStream.setXML(hadXML);
    return pn;
}

JSParseNode *
Parser::parseXMLText(JSObject *chain, bool allowList)
{
    




    JSTreeContext xmltc(this);
    if (!xmltc.init(context))
        return NULL;
    xmltc.setScopeChain(chain);

    
    tokenStream.setXMLOnlyMode();
    TokenKind tt = tokenStream.getToken(TSF_OPERAND);

    JSParseNode *pn;
    if (tt != TOK_XMLSTAGO) {
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_XML_MARKUP);
        pn = NULL;
    } else {
        pn = xmlElementOrListRoot(allowList);
    }
    tokenStream.setXMLOnlyMode(false);

    return pn;
}

#endif 

#if JS_HAS_BLOCK_SCOPE




















static inline bool
BlockIdInScope(uintN blockid, JSTreeContext *tc)
{
    if (blockid > tc->blockid())
        return false;
    for (JSStmtInfo *stmt = tc->topScopeStmt; stmt; stmt = stmt->downScope) {
        if (stmt->blockid == blockid)
            return true;
    }
    return false;
}
#endif

bool
JSParseNode::isConstant()
{
    switch (pn_type) {
      case TOK_NUMBER:
      case TOK_STRING:
        return true;
      case TOK_PRIMARY:
        switch (pn_op) {
          case JSOP_NULL:
          case JSOP_FALSE:
          case JSOP_TRUE:
            return true;
          default:
            return false;
        }
      case TOK_RB:
      case TOK_RC:
        return (pn_op == JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST);
      default:
        return false;
    }
}

JSParseNode *
Parser::primaryExpr(TokenKind tt, JSBool afterDot)
{
    JSParseNode *pn, *pn2, *pn3;
    JSOp op;

    JS_CHECK_RECURSION(context, return NULL);

    switch (tt) {
      case TOK_FUNCTION:
#if JS_HAS_XML_SUPPORT
        if (tokenStream.matchToken(TOK_DBLCOLON, TSF_KEYWORD_IS_NAME)) {
            pn2 = NullaryNode::create(tc);
            if (!pn2)
                return NULL;
            pn2->pn_type = TOK_FUNCTION;
            pn = qualifiedSuffix(pn2);
            if (!pn)
                return NULL;
            break;
        }
#endif
        pn = functionExpr();
        if (!pn)
            return NULL;
        break;

      case TOK_LB:
      {
        JSBool matched;
        jsuint index;

        pn = ListNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_RB;
        pn->pn_op = JSOP_NEWINIT;
        pn->makeEmpty();

#if JS_HAS_GENERATORS
        pn->pn_blockid = tc->blockidGen;
#endif

        matched = tokenStream.matchToken(TOK_RB, TSF_OPERAND);
        if (!matched) {
            for (index = 0; ; index++) {
                if (index == JS_ARGS_LENGTH_MAX) {
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_ARRAY_INIT_TOO_BIG);
                    return NULL;
                }

                tt = tokenStream.peekToken(TSF_OPERAND);
                if (tt == TOK_RB) {
                    pn->pn_xflags |= PNX_ENDCOMMA;
                    break;
                }

                if (tt == TOK_COMMA) {
                    
                    tokenStream.matchToken(TOK_COMMA);
                    pn2 = NullaryNode::create(tc);
                    pn->pn_xflags |= PNX_HOLEY | PNX_NONCONST;
                } else {
                    pn2 = assignExpr();
                    if (pn2 && !pn2->isConstant())
                        pn->pn_xflags |= PNX_NONCONST;
                }
                if (!pn2)
                    return NULL;
                pn->append(pn2);

                if (tt != TOK_COMMA) {
                    
                    if (!tokenStream.matchToken(TOK_COMMA))
                        break;
                }
            }

#if JS_HAS_GENERATORS
            









































            if (index == 0 && pn->pn_count != 0 && tokenStream.matchToken(TOK_FOR)) {
                JSParseNode *pnexp, *pntop;

                
                pn->pn_type = TOK_ARRAYCOMP;

                




                pnexp = pn->last();
                JS_ASSERT(pn->pn_count == 1);
                pn->pn_count = 0;
                pn->pn_tail = &pn->pn_head;
                *pn->pn_tail = NULL;

                pntop = comprehensionTail(pnexp, pn->pn_blockid, false,
                                          TOK_ARRAYPUSH, JSOP_ARRAYPUSH);
                if (!pntop)
                    return NULL;
                pn->append(pntop);
            }
#endif 

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_LIST);
        }
        pn->pn_pos.end = tokenStream.currentToken().pos.end;
        return pn;
      }

      case TOK_LC:
      {
        JSBool afterComma;
        JSParseNode *pnval;

        



        AtomIndexMap seen(context);

        enum AssignmentType {
            GET     = 0x1,
            SET     = 0x2,
            VALUE   = 0x4 | GET | SET
        };

        pn = ListNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_RC;
        pn->pn_op = JSOP_NEWINIT;
        pn->makeEmpty();

        afterComma = JS_FALSE;
        for (;;) {
            JSAtom *atom;
            tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
            switch (tt) {
              case TOK_NUMBER:
                pn3 = NullaryNode::create(tc);
                if (!pn3)
                    return NULL;
                pn3->pn_dval = tokenStream.currentToken().t_dval;
                if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                    return NULL;
                break;
              case TOK_NAME:
                {
                    atom = tokenStream.currentToken().t_atom;
                    if (atom == context->runtime->atomState.getAtom)
                        op = JSOP_GETTER;
                    else if (atom == context->runtime->atomState.setAtom)
                        op = JSOP_SETTER;
                    else
                        goto property_name;

                    tt = tokenStream.getToken(TSF_KEYWORD_IS_NAME);
                    if (tt == TOK_NAME || tt == TOK_STRING) {
                        atom = tokenStream.currentToken().t_atom;
                        pn3 = NameNode::create(atom, tc);
                        if (!pn3)
                            return NULL;
                    } else if (tt == TOK_NUMBER) {
                        pn3 = NullaryNode::create(tc);
                        if (!pn3)
                            return NULL;
                        pn3->pn_dval = tokenStream.currentToken().t_dval;
                        if (!js_ValueToAtom(context, DoubleValue(pn3->pn_dval), &atom))
                            return NULL;
                    } else {
                        tokenStream.ungetToken();
                        goto property_name;
                    }

                    pn->pn_xflags |= PNX_NONCONST;

                    
                    pn2 = functionDef(NULL, op == JSOP_GETTER ? Getter : Setter, Expression);
                    pn2 = JSParseNode::newBinaryOrAppend(TOK_COLON, op, pn3, pn2, tc);
                    goto skip;
                }
              property_name:
              case TOK_STRING:
                atom = tokenStream.currentToken().t_atom;
                pn3 = NullaryNode::create(tc);
                if (!pn3)
                    return NULL;
                pn3->pn_atom = atom;
                break;
              case TOK_RC:
                goto end_obj_init;
              default:
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_PROP_ID);
                return NULL;
            }

            op = JSOP_INITPROP;
            tt = tokenStream.getToken();
            if (tt == TOK_COLON) {
                pnval = assignExpr();
                if (pnval && !pnval->isConstant())
                    pn->pn_xflags |= PNX_NONCONST;
            } else {
#if JS_HAS_DESTRUCTURING_SHORTHAND
                if (tt != TOK_COMMA && tt != TOK_RC) {
#endif
                    reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_COLON_AFTER_ID);
                    return NULL;
#if JS_HAS_DESTRUCTURING_SHORTHAND
                }

                



                tokenStream.ungetToken();
                pn->pn_xflags |= PNX_DESTRUCT | PNX_NONCONST;
                pnval = pn3;
                if (pnval->pn_type == TOK_NAME) {
                    pnval->pn_arity = PN_NAME;
                    ((NameNode *)pnval)->initCommon(tc);
                }
#endif
            }

            pn2 = JSParseNode::newBinaryOrAppend(TOK_COLON, op, pn3, pnval, tc);
          skip:
            if (!pn2)
                return NULL;
            pn->append(pn2);

            





            AssignmentType assignType;
            if (op == JSOP_INITPROP) {
                assignType = VALUE;
            } else if (op == JSOP_GETTER) {
                assignType = GET;
            } else if (op == JSOP_SETTER) {
                assignType = SET;
            } else {
                JS_NOT_REACHED("bad opcode in object initializer");
                assignType = VALUE; 
            }

            AtomIndexAddPtr p = seen.lookupForAdd(atom);
            if (p) {
                jsatomid index = p.value();
                AssignmentType oldAssignType = AssignmentType(index);
                if ((oldAssignType & assignType) &&
                    (oldAssignType != VALUE || assignType != VALUE || tc->needStrictChecks()))
                {
                    JSAutoByteString name;
                    if (!js_AtomToPrintableString(context, atom, &name))
                        return NULL;

                    uintN flags = (oldAssignType == VALUE &&
                                   assignType == VALUE &&
                                   !tc->inStrictMode())
                                  ? JSREPORT_WARNING
                                  : JSREPORT_ERROR;
                    if (!ReportCompileErrorNumber(context, &tokenStream, NULL, flags,
                                                  JSMSG_DUPLICATE_PROPERTY, name.ptr()))
                    {
                        return NULL;
                    }
                }
                p.value() = assignType | oldAssignType;
            } else {
                if (!seen.add(p, atom, assignType))
                    return NULL;
            }

            tt = tokenStream.getToken();
            if (tt == TOK_RC)
                goto end_obj_init;
            if (tt != TOK_COMMA) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_CURLY_AFTER_LIST);
                return NULL;
            }
            afterComma = JS_TRUE;
        }

      end_obj_init:
        pn->pn_pos.end = tokenStream.currentToken().pos.end;
        return pn;
      }

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        pn = letBlock(JS_FALSE);
        if (!pn)
            return NULL;
        break;
#endif

#if JS_HAS_SHARP_VARS
      case TOK_DEFSHARP:
        pn = UnaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_num = (jsint) tokenStream.currentToken().t_dval;
        tt = tokenStream.getToken(TSF_OPERAND);
        pn->pn_kid = primaryExpr(tt, JS_FALSE);
        if (!pn->pn_kid)
            return NULL;
        if (PN_TYPE(pn->pn_kid) == TOK_USESHARP ||
            PN_TYPE(pn->pn_kid) == TOK_DEFSHARP ||
            PN_TYPE(pn->pn_kid) == TOK_STRING ||
            PN_TYPE(pn->pn_kid) == TOK_NUMBER ||
            PN_TYPE(pn->pn_kid) == TOK_PRIMARY) {
            reportErrorNumber(pn->pn_kid, JSREPORT_ERROR, JSMSG_BAD_SHARP_VAR_DEF);
            return NULL;
        }
        if (!tc->ensureSharpSlots())
            return NULL;
        break;

      case TOK_USESHARP:
        
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        if (!tc->ensureSharpSlots())
            return NULL;
        pn->pn_num = (jsint) tokenStream.currentToken().t_dval;
        break;
#endif 

      case TOK_LP:
      {
        JSBool genexp;

        pn = parenExpr(&genexp);
        if (!pn)
            return NULL;
        pn->pn_parens = true;
        if (!genexp)
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);
        break;
      }

#if JS_HAS_XML_SUPPORT
      case TOK_STAR:
        pn = qualifiedIdentifier();
        if (!pn)
            return NULL;
        break;

      case TOK_AT:
        pn = attributeIdentifier();
        if (!pn)
            return NULL;
        break;

      case TOK_XMLSTAGO:
        pn = xmlElementOrListRoot(JS_TRUE);
        if (!pn)
            return NULL;
        break;
#endif 

      case TOK_STRING:
#if JS_HAS_SHARP_VARS
        
#endif

#if JS_HAS_XML_SUPPORT
      case TOK_XMLCDATA:
      case TOK_XMLCOMMENT:
      case TOK_XMLPI:
#endif
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_atom = tokenStream.currentToken().t_atom;
#if JS_HAS_XML_SUPPORT
        if (tt == TOK_XMLPI)
            pn->pn_atom2 = tokenStream.currentToken().t_atom2;
        else
#endif
            pn->pn_op = tokenStream.currentToken().t_op;
        break;

      case TOK_NAME:
        pn = NameNode::create(tokenStream.currentToken().t_atom, tc);
        if (!pn)
            return NULL;
        JS_ASSERT(tokenStream.currentToken().t_op == JSOP_NAME);
        pn->pn_op = JSOP_NAME;

        if ((tc->flags & (TCF_IN_FUNCTION | TCF_FUN_PARAM_ARGUMENTS)) == TCF_IN_FUNCTION &&
            pn->pn_atom == context->runtime->atomState.argumentsAtom) {
            






            tc->noteArgumentsUse(pn);

            



            if (!afterDot && !(tc->flags & TCF_DECL_DESTRUCTURING)
                && !tc->inStatement(STMT_WITH)) {
                pn->pn_op = JSOP_ARGUMENTS;
                pn->pn_dflags |= PND_BOUND;
            }
        } else if ((!afterDot
#if JS_HAS_XML_SUPPORT
                    || tokenStream.peekToken() == TOK_DBLCOLON
#endif
                   ) && !(tc->flags & TCF_DECL_DESTRUCTURING)) {
            
            if (!tc->inFunction() &&
                pn->pn_atom == context->runtime->atomState.argumentsAtom) {
                tc->countArgumentsUse(pn);
            }

            JSStmtInfo *stmt = js_LexicalLookup(tc, pn->pn_atom, NULL);

            MultiDeclRange mdl = tc->decls.lookupMulti(pn->pn_atom);
            JSDefinition *dn;

            if (!mdl.empty()) {
                dn = mdl.front();
#if JS_HAS_BLOCK_SCOPE
                





                while (dn->isLet() && !BlockIdInScope(dn->pn_blockid, tc)) {
                    mdl.popFront();
                    if (mdl.empty())
                        break;
                    dn = mdl.front();
                }
#endif
            }

            if (!mdl.empty()) {
                dn = mdl.front();
            } else {
                AtomDefnAddPtr p = tc->lexdeps->lookupForAdd(pn->pn_atom);
                if (p) {
                    dn = p.value();
                } else {
                    







                    dn = MakePlaceholder(p, pn, tc);
                    if (!dn)
                        return NULL;

                    










                    if (tokenStream.peekToken() != TOK_LP)
                        dn->pn_dflags |= PND_FUNARG;
                }
            }

            JS_ASSERT(dn->pn_defn);
            LinkUseToDef(pn, dn, tc);

            
            if (tokenStream.peekToken() != TOK_LP)
                dn->pn_dflags |= PND_FUNARG;

            pn->pn_dflags |= (dn->pn_dflags & PND_FUNARG);
            if (stmt && stmt->type == STMT_WITH)
                pn->pn_dflags |= PND_DEOPTIMIZED;
        }

#if JS_HAS_XML_SUPPORT
        if (tokenStream.matchToken(TOK_DBLCOLON)) {
            if (afterDot) {
                




                const KeywordInfo *ki = FindKeyword(pn->pn_atom->charsZ(), pn->pn_atom->length());
                if (ki) {
                    if (ki->tokentype != TOK_FUNCTION) {
                        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_KEYWORD_NOT_NS);
                        return NULL;
                    }

                    pn->pn_arity = PN_NULLARY;
                    pn->pn_type = TOK_FUNCTION;
                }
            }
            pn = qualifiedSuffix(pn);
            if (!pn)
                return NULL;
        }
#endif
        break;

      case TOK_REGEXP:
      {
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;

        JSObject *obj;
        if (context->hasfp()) {
            obj = RegExp::createObject(context, context->regExpStatics(),
                                       tokenStream.getTokenbuf().begin(),
                                       tokenStream.getTokenbuf().length(),
                                       tokenStream.currentToken().t_reflags);
        } else {
            obj = RegExp::createObjectNoStatics(context,
                                                tokenStream.getTokenbuf().begin(),
                                                tokenStream.getTokenbuf().length(),
                                                tokenStream.currentToken().t_reflags);
        }

        if (!obj)
            return NULL;
        if (!tc->compileAndGo()) {
            obj->clearParent();
            obj->clearProto();
        }

        pn->pn_objbox = tc->parser->newObjectBox(obj);
        if (!pn->pn_objbox)
            return NULL;

        pn->pn_op = JSOP_REGEXP;
        break;
      }

      case TOK_NUMBER:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_op = JSOP_DOUBLE;
        pn->pn_dval = tokenStream.currentToken().t_dval;
        break;

      case TOK_PRIMARY:
        pn = NullaryNode::create(tc);
        if (!pn)
            return NULL;
        pn->pn_op = tokenStream.currentToken().t_op;
        break;

      case TOK_ERROR:
        
        return NULL;

      default:
        reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    return pn;
}

JSParseNode *
Parser::parenExpr(JSBool *genexp)
{
    TokenPtr begin;
    JSParseNode *pn;

    JS_ASSERT(tokenStream.currentToken().type == TOK_LP);
    begin = tokenStream.currentToken().pos.begin;

    if (genexp)
        *genexp = JS_FALSE;

    GenexpGuard guard(tc);

    pn = bracketedExpr();
    if (!pn)
        return NULL;
    guard.endBody();

#if JS_HAS_GENERATOR_EXPRS
    if (tokenStream.matchToken(TOK_FOR)) {
        if (!guard.checkValidBody(pn))
            return NULL;
        JS_ASSERT(pn->pn_type != TOK_YIELD);
        if (pn->pn_type == TOK_COMMA && !pn->pn_parens) {
            reportErrorNumber(pn->last(), JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX,
                              js_generator_str);
            return NULL;
        }
        pn = generatorExpr(pn);
        if (!pn)
            return NULL;
        pn->pn_pos.begin = begin;
        if (genexp) {
            if (tokenStream.getToken() != TOK_RP) {
                reportErrorNumber(NULL, JSREPORT_ERROR, JSMSG_BAD_GENERATOR_SYNTAX,
                                  js_generator_str);
                return NULL;
            }
            pn->pn_pos.end = tokenStream.currentToken().pos.end;
            *genexp = JS_TRUE;
        }
    }
#endif 

    if (!maybeNoteGenerator())
        return NULL;

    return pn;
}





static JSBool
FoldType(JSContext *cx, JSParseNode *pn, TokenKind type)
{
    if (PN_TYPE(pn) != type) {
        switch (type) {
          case TOK_NUMBER:
            if (pn->pn_type == TOK_STRING) {
                jsdouble d;
                if (!ValueToNumber(cx, StringValue(pn->pn_atom), &d))
                    return JS_FALSE;
                pn->pn_dval = d;
                pn->pn_type = TOK_NUMBER;
                pn->pn_op = JSOP_DOUBLE;
            }
            break;

          case TOK_STRING:
            if (pn->pn_type == TOK_NUMBER) {
                JSString *str = js_NumberToString(cx, pn->pn_dval);
                if (!str)
                    return JS_FALSE;
                pn->pn_atom = js_AtomizeString(cx, str);
                if (!pn->pn_atom)
                    return JS_FALSE;
                pn->pn_type = TOK_STRING;
                pn->pn_op = JSOP_STRING;
            }
            break;

          default:;
        }
    }
    return JS_TRUE;
}






static JSBool
FoldBinaryNumeric(JSContext *cx, JSOp op, JSParseNode *pn1, JSParseNode *pn2,
                  JSParseNode *pn, JSTreeContext *tc)
{
    jsdouble d, d2;
    int32 i, j;

    JS_ASSERT(pn1->pn_type == TOK_NUMBER && pn2->pn_type == TOK_NUMBER);
    d = pn1->pn_dval;
    d2 = pn2->pn_dval;
    switch (op) {
      case JSOP_LSH:
      case JSOP_RSH:
        i = js_DoubleToECMAInt32(d);
        j = js_DoubleToECMAInt32(d2);
        j &= 31;
        d = (op == JSOP_LSH) ? i << j : i >> j;
        break;

      case JSOP_URSH:
        j = js_DoubleToECMAInt32(d2);
        j &= 31;
        d = js_DoubleToECMAUint32(d) >> j;
        break;

      case JSOP_ADD:
        d += d2;
        break;

      case JSOP_SUB:
        d -= d2;
        break;

      case JSOP_MUL:
        d *= d2;
        break;

      case JSOP_DIV:
        if (d2 == 0) {
#if defined(XP_WIN)
            
            if (JSDOUBLE_IS_NaN(d2))
                d = js_NaN;
            else
#endif
            if (d == 0 || JSDOUBLE_IS_NaN(d))
                d = js_NaN;
            else if (JSDOUBLE_IS_NEG(d) != JSDOUBLE_IS_NEG(d2))
                d = js_NegativeInfinity;
            else
                d = js_PositiveInfinity;
        } else {
            d /= d2;
        }
        break;

      case JSOP_MOD:
        if (d2 == 0) {
            d = js_NaN;
        } else {
            d = js_fmod(d, d2);
        }
        break;

      default:;
    }

    
    if (pn1 != pn)
        RecycleTree(pn1, tc);
    if (pn2 != pn)
        RecycleTree(pn2, tc);
    pn->pn_type = TOK_NUMBER;
    pn->pn_op = JSOP_DOUBLE;
    pn->pn_arity = PN_NULLARY;
    pn->pn_dval = d;
    return JS_TRUE;
}

#if JS_HAS_XML_SUPPORT

static JSBool
FoldXMLConstants(JSContext *cx, JSParseNode *pn, JSTreeContext *tc)
{
    TokenKind tt;
    JSParseNode **pnp, *pn1, *pn2;
    JSString *accum, *str;
    uint32 i, j;

    JS_ASSERT(pn->pn_arity == PN_LIST);
    tt = PN_TYPE(pn);
    pnp = &pn->pn_head;
    pn1 = *pnp;
    accum = NULL;
    str = NULL;
    if ((pn->pn_xflags & PNX_CANTFOLD) == 0) {
        if (tt == TOK_XMLETAGO)
            accum = cx->runtime->atomState.etagoAtom;
        else if (tt == TOK_XMLSTAGO || tt == TOK_XMLPTAGC)
            accum = cx->runtime->atomState.stagoAtom;
    }

    






    for (pn2 = pn1, i = j = 0; pn2; pn2 = pn2->pn_next, i++) {
        
        JS_ASSERT(tt != TOK_XMLETAGO || i == 0);
        switch (pn2->pn_type) {
          case TOK_XMLATTR:
            if (!accum)
                goto cantfold;
            
          case TOK_XMLNAME:
          case TOK_XMLSPACE:
          case TOK_XMLTEXT:
          case TOK_STRING:
            if (pn2->pn_arity == PN_LIST)
                goto cantfold;
            str = pn2->pn_atom;
            break;

          case TOK_XMLCDATA:
            str = js_MakeXMLCDATAString(cx, pn2->pn_atom);
            if (!str)
                return JS_FALSE;
            break;

          case TOK_XMLCOMMENT:
            str = js_MakeXMLCommentString(cx, pn2->pn_atom);
            if (!str)
                return JS_FALSE;
            break;

          case TOK_XMLPI:
            str = js_MakeXMLPIString(cx, pn2->pn_atom, pn2->pn_atom2);
            if (!str)
                return JS_FALSE;
            break;

          cantfold:
          default:
            JS_ASSERT(*pnp == pn1);
            if ((tt == TOK_XMLSTAGO || tt == TOK_XMLPTAGC) &&
                (i & 1) ^ (j & 1)) {
#ifdef DEBUG_brendanXXX
                printf("1: %d, %d => ", i, j);
                if (accum)
                    FileEscapedString(stdout, accum, 0);
                else
                    fputs("NULL", stdout);
                fputc('\n', stdout);
#endif
            } else if (accum && pn1 != pn2) {
                while (pn1->pn_next != pn2) {
                    pn1 = RecycleTree(pn1, tc);
                    --pn->pn_count;
                }
                pn1->pn_type = TOK_XMLTEXT;
                pn1->pn_op = JSOP_STRING;
                pn1->pn_arity = PN_NULLARY;
                pn1->pn_atom = js_AtomizeString(cx, accum);
                if (!pn1->pn_atom)
                    return JS_FALSE;
                JS_ASSERT(pnp != &pn1->pn_next);
                *pnp = pn1;
            }
            pnp = &pn2->pn_next;
            pn1 = *pnp;
            accum = NULL;
            continue;
        }

        if (accum) {
            {
                AutoStringRooter tvr(cx, accum);
                str = ((tt == TOK_XMLSTAGO || tt == TOK_XMLPTAGC) && i != 0)
                      ? js_AddAttributePart(cx, i & 1, accum, str)
                      : js_ConcatStrings(cx, accum, str);
            }
            if (!str)
                return JS_FALSE;
#ifdef DEBUG_brendanXXX
            printf("2: %d, %d => ", i, j);
            FileEscapedString(stdout, str, 0);
            printf(" (%u)\n", str->length());
#endif
            ++j;
        }
        accum = str;
    }

    if (accum) {
        str = NULL;
        if ((pn->pn_xflags & PNX_CANTFOLD) == 0) {
            if (tt == TOK_XMLPTAGC)
                str = cx->runtime->atomState.ptagcAtom;
            else if (tt == TOK_XMLSTAGO || tt == TOK_XMLETAGO)
                str = cx->runtime->atomState.tagcAtom;
        }
        if (str) {
            accum = js_ConcatStrings(cx, accum, str);
            if (!accum)
                return JS_FALSE;
        }

        JS_ASSERT(*pnp == pn1);
        while (pn1->pn_next) {
            pn1 = RecycleTree(pn1, tc);
            --pn->pn_count;
        }
        pn1->pn_type = TOK_XMLTEXT;
        pn1->pn_op = JSOP_STRING;
        pn1->pn_arity = PN_NULLARY;
        pn1->pn_atom = js_AtomizeString(cx, accum);
        if (!pn1->pn_atom)
            return JS_FALSE;
        JS_ASSERT(pnp != &pn1->pn_next);
        *pnp = pn1;
    }

    if (pn1 && pn->pn_count == 1) {
        






        if (!(pn->pn_xflags & PNX_XMLROOT)) {
            pn->become(pn1);
        } else if (tt == TOK_XMLPTAGC) {
            pn->pn_type = TOK_XMLELEM;
            pn->pn_op = JSOP_TOXML;
        }
    }
    return JS_TRUE;
}

#endif 

static int
Boolish(JSParseNode *pn)
{
    switch (pn->pn_op) {
      case JSOP_DOUBLE:
        return pn->pn_dval != 0 && !JSDOUBLE_IS_NaN(pn->pn_dval);

      case JSOP_STRING:
        return pn->pn_atom->length() != 0;

#if JS_HAS_GENERATOR_EXPRS
      case JSOP_CALL:
      {
        




        if (pn->pn_count != 1)
            break;
        JSParseNode *pn2 = pn->pn_head;
        if (pn2->pn_type != TOK_FUNCTION)
            break;
        if (!(pn2->pn_funbox->tcflags & TCF_GENEXP_LAMBDA))
            break;
        
      }
#endif

      case JSOP_DEFFUN:
      case JSOP_LAMBDA:
      case JSOP_TRUE:
        return 1;

      case JSOP_NULL:
      case JSOP_FALSE:
        return 0;

      default:;
    }
    return -1;
}

JSBool
js_FoldConstants(JSContext *cx, JSParseNode *pn, JSTreeContext *tc, bool inCond)
{
    JSParseNode *pn1 = NULL, *pn2 = NULL, *pn3 = NULL;

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    switch (pn->pn_arity) {
      case PN_FUNC:
      {
        uint32 oldflags = tc->flags;
        JSFunctionBox *oldlist = tc->functionList;

        tc->flags = pn->pn_funbox->tcflags;
        tc->functionList = pn->pn_funbox->kids;
        if (!js_FoldConstants(cx, pn->pn_body, tc))
            return JS_FALSE;
        pn->pn_funbox->kids = tc->functionList;
        tc->flags = oldflags;
        tc->functionList = oldlist;
        break;
      }

      case PN_LIST:
      {
        
        bool cond = inCond && (pn->pn_type == TOK_OR || pn->pn_type == TOK_AND);

        
        pn1 = pn2 = pn->pn_head;
        if ((pn->pn_type == TOK_LP || pn->pn_type == TOK_NEW) && pn2->pn_parens)
            pn2 = pn2->pn_next;

        
        for (; pn2; pn2 = pn2->pn_next) {
            if (!js_FoldConstants(cx, pn2, tc, cond))
                return JS_FALSE;
        }
        break;
      }

      case PN_TERNARY:
        
        pn1 = pn->pn_kid1;
        pn2 = pn->pn_kid2;
        pn3 = pn->pn_kid3;
        if (pn1 && !js_FoldConstants(cx, pn1, tc, pn->pn_type == TOK_IF))
            return JS_FALSE;
        if (pn2) {
            if (!js_FoldConstants(cx, pn2, tc, pn->pn_type == TOK_FORHEAD))
                return JS_FALSE;
            if (pn->pn_type == TOK_FORHEAD && pn2->pn_op == JSOP_TRUE) {
                RecycleTree(pn2, tc);
                pn->pn_kid2 = NULL;
            }
        }
        if (pn3 && !js_FoldConstants(cx, pn3, tc))
            return JS_FALSE;
        break;

      case PN_BINARY:
        pn1 = pn->pn_left;
        pn2 = pn->pn_right;

        
        if (pn->pn_type == TOK_OR || pn->pn_type == TOK_AND) {
            if (!js_FoldConstants(cx, pn1, tc, inCond))
                return JS_FALSE;
            if (!js_FoldConstants(cx, pn2, tc, inCond))
                return JS_FALSE;
            break;
        }

        
        if (pn1 && !js_FoldConstants(cx, pn1, tc, pn->pn_type == TOK_WHILE))
            return JS_FALSE;
        if (!js_FoldConstants(cx, pn2, tc, pn->pn_type == TOK_DO))
            return JS_FALSE;
        break;

      case PN_UNARY:
        pn1 = pn->pn_kid;

        








        if (pn->pn_op == JSOP_TYPEOF && pn1->pn_type != TOK_NAME)
            pn->pn_op = JSOP_TYPEOFEXPR;

        if (pn1 && !js_FoldConstants(cx, pn1, tc, pn->pn_op == JSOP_NOT))
            return JS_FALSE;
        break;

      case PN_NAME:
        





        if (!pn->pn_used) {
            pn1 = pn->pn_expr;
            while (pn1 && pn1->pn_arity == PN_NAME && !pn1->pn_used)
                pn1 = pn1->pn_expr;
            if (pn1 && !js_FoldConstants(cx, pn1, tc))
                return JS_FALSE;
        }
        break;

      case PN_NAMESET:
        pn1 = pn->pn_tree;
        if (!js_FoldConstants(cx, pn1, tc))
            return JS_FALSE;
        break;

      case PN_NULLARY:
        break;
    }

    switch (pn->pn_type) {
      case TOK_IF:
        if (ContainsStmt(pn2, TOK_VAR) || ContainsStmt(pn3, TOK_VAR))
            break;
        

      case TOK_HOOK:
        
        switch (pn1->pn_type) {
          case TOK_NUMBER:
            if (pn1->pn_dval == 0 || JSDOUBLE_IS_NaN(pn1->pn_dval))
                pn2 = pn3;
            break;
          case TOK_STRING:
            if (pn1->pn_atom->length() == 0)
                pn2 = pn3;
            break;
          case TOK_PRIMARY:
            if (pn1->pn_op == JSOP_TRUE)
                break;
            if (pn1->pn_op == JSOP_FALSE || pn1->pn_op == JSOP_NULL) {
                pn2 = pn3;
                break;
            }
            
          default:
            
            return JS_TRUE;
        }

#if JS_HAS_GENERATOR_EXPRS
        
        if (!pn2 && (tc->flags & TCF_GENEXP_LAMBDA))
            break;
#endif

        if (pn2 && !pn2->pn_defn)
            pn->become(pn2);
        if (!pn2 || (pn->pn_type == TOK_SEMI && !pn->pn_kid)) {
            






            pn->pn_type = TOK_LC;
            pn->pn_arity = PN_LIST;
            pn->makeEmpty();
        }
        RecycleTree(pn2, tc);
        if (pn3 && pn3 != pn2)
            RecycleTree(pn3, tc);
        break;

      case TOK_OR:
      case TOK_AND:
        if (inCond) {
            if (pn->pn_arity == PN_LIST) {
                JSParseNode **pnp = &pn->pn_head;
                JS_ASSERT(*pnp == pn1);
                do {
                    int cond = Boolish(pn1);
                    if (cond == (pn->pn_type == TOK_OR)) {
                        for (pn2 = pn1->pn_next; pn2; pn2 = pn3) {
                            pn3 = pn2->pn_next;
                            RecycleTree(pn2, tc);
                            --pn->pn_count;
                        }
                        pn1->pn_next = NULL;
                        break;
                    }
                    if (cond != -1) {
                        JS_ASSERT(cond == (pn->pn_type == TOK_AND));
                        if (pn->pn_count == 1)
                            break;
                        *pnp = pn1->pn_next;
                        RecycleTree(pn1, tc);
                        --pn->pn_count;
                    } else {
                        pnp = &pn1->pn_next;
                    }
                } while ((pn1 = *pnp) != NULL);

                
                pn1 = pn->pn_head;
                if (pn->pn_count == 2) {
                    pn2 = pn1->pn_next;
                    pn1->pn_next = NULL;
                    JS_ASSERT(!pn2->pn_next);
                    pn->pn_arity = PN_BINARY;
                    pn->pn_left = pn1;
                    pn->pn_right = pn2;
                } else if (pn->pn_count == 1) {
                    pn->become(pn1);
                    RecycleTree(pn1, tc);
                }
            } else {
                int cond = Boolish(pn1);
                if (cond == (pn->pn_type == TOK_OR)) {
                    RecycleTree(pn2, tc);
                    pn->become(pn1);
                } else if (cond != -1) {
                    JS_ASSERT(cond == (pn->pn_type == TOK_AND));
                    RecycleTree(pn1, tc);
                    pn->become(pn2);
                }
            }
        }
        break;

      case TOK_ASSIGN:
        





        if (pn->pn_op == JSOP_NOP)
            break;
        if (pn->pn_op != JSOP_ADD)
            goto do_binary_op;
        

      case TOK_PLUS:
        if (pn->pn_arity == PN_LIST) {
            




            JS_ASSERT(pn->pn_count > 2);
            if (pn->pn_xflags & PNX_CANTFOLD)
                return JS_TRUE;
            if (pn->pn_xflags != PNX_STRCAT)
                goto do_binary_op;

            
            size_t length = 0;
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                if (!FoldType(cx, pn2, TOK_STRING))
                    return JS_FALSE;
                
                if (pn2->pn_type != TOK_STRING)
                    return JS_TRUE;
                length += pn2->pn_atom->length();
            }

            
            jschar *chars = (jschar *) cx->malloc_((length + 1) * sizeof(jschar));
            if (!chars)
                return JS_FALSE;
            chars[length] = 0;
            JSString *str = js_NewString(cx, chars, length);
            if (!str) {
                cx->free_(chars);
                return JS_FALSE;
            }

            
            for (pn2 = pn1; pn2; pn2 = RecycleTree(pn2, tc)) {
                JSAtom *atom = pn2->pn_atom;
                size_t length2 = atom->length();
                js_strncpy(chars, atom->chars(), length2);
                chars += length2;
            }
            JS_ASSERT(*chars == 0);

            
            pn->pn_atom = js_AtomizeString(cx, str);
            if (!pn->pn_atom)
                return JS_FALSE;
            pn->pn_type = TOK_STRING;
            pn->pn_op = JSOP_STRING;
            pn->pn_arity = PN_NULLARY;
            break;
        }

        
        JS_ASSERT(pn->pn_arity == PN_BINARY);
        if (pn1->pn_type == TOK_STRING || pn2->pn_type == TOK_STRING) {
            JSString *left, *right, *str;

            if (!FoldType(cx, (pn1->pn_type != TOK_STRING) ? pn1 : pn2,
                          TOK_STRING)) {
                return JS_FALSE;
            }
            if (pn1->pn_type != TOK_STRING || pn2->pn_type != TOK_STRING)
                return JS_TRUE;
            left = pn1->pn_atom;
            right = pn2->pn_atom;
            str = js_ConcatStrings(cx, left, right);
            if (!str)
                return JS_FALSE;
            pn->pn_atom = js_AtomizeString(cx, str);
            if (!pn->pn_atom)
                return JS_FALSE;
            pn->pn_type = TOK_STRING;
            pn->pn_op = JSOP_STRING;
            pn->pn_arity = PN_NULLARY;
            RecycleTree(pn1, tc);
            RecycleTree(pn2, tc);
            break;
        }

        
        goto do_binary_op;

      case TOK_STAR:
      case TOK_SHOP:
      case TOK_MINUS:
      case TOK_DIVOP:
      do_binary_op:
        if (pn->pn_arity == PN_LIST) {
            JS_ASSERT(pn->pn_count > 2);
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                if (!FoldType(cx, pn2, TOK_NUMBER))
                    return JS_FALSE;
            }
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                
                if (pn2->pn_type != TOK_NUMBER)
                    break;
            }
            if (!pn2) {
                JSOp op = PN_OP(pn);

                pn2 = pn1->pn_next;
                pn3 = pn2->pn_next;
                if (!FoldBinaryNumeric(cx, op, pn1, pn2, pn, tc))
                    return JS_FALSE;
                while ((pn2 = pn3) != NULL) {
                    pn3 = pn2->pn_next;
                    if (!FoldBinaryNumeric(cx, op, pn, pn2, pn, tc))
                        return JS_FALSE;
                }
            }
        } else {
            JS_ASSERT(pn->pn_arity == PN_BINARY);
            if (!FoldType(cx, pn1, TOK_NUMBER) ||
                !FoldType(cx, pn2, TOK_NUMBER)) {
                return JS_FALSE;
            }
            if (pn1->pn_type == TOK_NUMBER && pn2->pn_type == TOK_NUMBER) {
                if (!FoldBinaryNumeric(cx, PN_OP(pn), pn1, pn2, pn, tc))
                    return JS_FALSE;
            }
        }
        break;

      case TOK_UNARYOP:
        if (pn1->pn_type == TOK_NUMBER) {
            jsdouble d;

            
            d = pn1->pn_dval;
            switch (pn->pn_op) {
              case JSOP_BITNOT:
                d = ~js_DoubleToECMAInt32(d);
                break;

              case JSOP_NEG:
                d = -d;
                break;

              case JSOP_POS:
                break;

              case JSOP_NOT:
                pn->pn_type = TOK_PRIMARY;
                pn->pn_op = (d == 0 || JSDOUBLE_IS_NaN(d)) ? JSOP_TRUE : JSOP_FALSE;
                pn->pn_arity = PN_NULLARY;
                

              default:
                
                return JS_TRUE;
            }
            pn->pn_type = TOK_NUMBER;
            pn->pn_op = JSOP_DOUBLE;
            pn->pn_arity = PN_NULLARY;
            pn->pn_dval = d;
            RecycleTree(pn1, tc);
        } else if (pn1->pn_type == TOK_PRIMARY) {
            if (pn->pn_op == JSOP_NOT &&
                (pn1->pn_op == JSOP_TRUE ||
                 pn1->pn_op == JSOP_FALSE)) {
                pn->become(pn1);
                pn->pn_op = (pn->pn_op == JSOP_TRUE) ? JSOP_FALSE : JSOP_TRUE;
                RecycleTree(pn1, tc);
            }
        }
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_XMLELEM:
      case TOK_XMLLIST:
      case TOK_XMLPTAGC:
      case TOK_XMLSTAGO:
      case TOK_XMLETAGO:
      case TOK_XMLNAME:
        if (pn->pn_arity == PN_LIST) {
            JS_ASSERT(pn->pn_type == TOK_XMLLIST || pn->pn_count != 0);
            if (!FoldXMLConstants(cx, pn, tc))
                return JS_FALSE;
        }
        break;

      case TOK_AT:
        if (pn1->pn_type == TOK_XMLNAME) {
            JSObjectBox *xmlbox;

            Value v = StringValue(pn1->pn_atom);
            if (!js_ToAttributeName(cx, &v))
                return JS_FALSE;
            JS_ASSERT(v.isObject());

            xmlbox = tc->parser->newObjectBox(&v.toObject());
            if (!xmlbox)
                return JS_FALSE;

            pn->pn_type = TOK_XMLNAME;
            pn->pn_op = JSOP_OBJECT;
            pn->pn_arity = PN_NULLARY;
            pn->pn_objbox = xmlbox;
            RecycleTree(pn1, tc);
        }
        break;
#endif 

      default:;
    }

    if (inCond) {
        int cond = Boolish(pn);
        if (cond >= 0) {
            





            PrepareNodeForMutation(pn, tc);
            pn->pn_type = TOK_PRIMARY;
            pn->pn_op = cond ? JSOP_TRUE : JSOP_FALSE;
            pn->pn_arity = PN_NULLARY;
        }
    }

    return JS_TRUE;
}
