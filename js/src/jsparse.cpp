




















































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




#define pn_offsetof(m)  offsetof(JSParseNode, m)

JS_STATIC_ASSERT(pn_offsetof(pn_link) == pn_offsetof(dn_uses));
JS_STATIC_ASSERT(pn_offsetof(pn_u.name.atom) == pn_offsetof(pn_u.apair.atom));

#undef pn_offsetof








typedef JSParseNode *
JSParser(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc);

typedef JSParseNode *
JSVariablesParser(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
                  bool inLetHead);

typedef JSParseNode *
JSMemberParser(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
               JSBool allowCallSyntax);

typedef JSParseNode *
JSPrimaryParser(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
                JSTokenType tt, JSBool afterDot);

typedef JSParseNode *
JSParenParser(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
              JSParseNode *pn1, JSBool *genexp);

static JSParser FunctionStmt;
static JSParser FunctionExpr;
static JSParser Statements;
static JSParser Statement;
static JSVariablesParser Variables;
static JSParser Expr;
static JSParser AssignExpr;
static JSParser CondExpr;
static JSParser OrExpr;
static JSParser AndExpr;
static JSParser BitOrExpr;
static JSParser BitXorExpr;
static JSParser BitAndExpr;
static JSParser EqExpr;
static JSParser RelExpr;
static JSParser ShiftExpr;
static JSParser AddExpr;
static JSParser MulExpr;
static JSParser UnaryExpr;
static JSMemberParser  MemberExpr;
static JSPrimaryParser PrimaryExpr;
static JSParenParser   ParenExpr;

static bool RecognizeDirectivePrologue(JSContext *cx, JSTokenStream *ts,
                                       JSTreeContext *tc, JSParseNode *pn);





#define MUST_MATCH_TOKEN(tt, errno)                                           \
    JS_BEGIN_MACRO                                                            \
        if (js_GetToken(cx, ts) != tt) {                                      \
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR, errno); \
            return NULL;                                                      \
        }                                                                     \
    JS_END_MACRO

#ifdef METER_PARSENODES
static uint32 parsenodes = 0;
static uint32 maxparsenodes = 0;
static uint32 recyclednodes = 0;
#endif

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

    
    if (PN_TYPE(pn2) == TOK_FUNCTION && pn2->pn_arity == PN_FUNC)
        pn2->pn_funbox->node = this;

    pn_type = pn2->pn_type;
    pn_op = pn2->pn_op;
    pn_arity = pn2->pn_arity;
    pn_parens = pn2->pn_parens;
    pn_u = pn2->pn_u;
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

bool
JSCompiler::init(const jschar *base, size_t length,
                 FILE *fp, const char *filename, uintN lineno)
{
    JSContext *cx = context;

    tempPoolMark = JS_ARENA_MARK(&cx->tempPool);
    if (!tokenStream.init(cx, base, length, fp, filename, lineno)) {
        JS_ARENA_RELEASE(&cx->tempPool, tempPoolMark);
        return false;
    }

    
    JS_KEEP_ATOMS(cx->runtime);
    JS_PUSH_TEMP_ROOT_COMPILER(cx, this, &tempRoot);
    return true;
}

JSCompiler::~JSCompiler()
{
    JSContext *cx = context;

    if (principals)
        JSPRINCIPALS_DROP(cx, principals);
    JS_ASSERT(tempRoot.u.compiler == this);
    JS_POP_TEMP_ROOT(cx, &tempRoot);
    JS_UNKEEP_ATOMS(cx->runtime);
    tokenStream.close(cx);
    JS_ARENA_RELEASE(&cx->tempPool, tempPoolMark);
}

void
JSCompiler::setPrincipals(JSPrincipals *prin)
{
    JS_ASSERT(!principals);
    if (prin)
        JSPRINCIPALS_HOLD(context, prin);
    principals = prin;
}

JSObjectBox *
JSCompiler::newObjectBox(JSObject *obj)
{
    JS_ASSERT(obj);

    





    JSObjectBox *objbox;
    JS_ARENA_ALLOCATE_TYPE(objbox, JSObjectBox, &context->tempPool);
    if (!objbox) {
        js_ReportOutOfScriptQuota(context);
        return NULL;
    }
    objbox->traceLink = traceListHead;
    traceListHead = objbox;
    objbox->emitLink = NULL;
    objbox->object = obj;
    return objbox;
}

JSFunctionBox *
JSCompiler::newFunctionBox(JSObject *obj, JSParseNode *fn, JSTreeContext *tc)
{
    JS_ASSERT(obj);
    JS_ASSERT(HAS_FUNCTION_CLASS(obj));

    





    JSFunctionBox *funbox;
    JS_ARENA_ALLOCATE_TYPE(funbox, JSFunctionBox, &context->tempPool);
    if (!funbox) {
        js_ReportOutOfScriptQuota(context);
        return NULL;
    }
    funbox->traceLink = traceListHead;
    traceListHead = funbox;
    funbox->emitLink = NULL;
    funbox->object = obj;
    funbox->node = fn;
    funbox->siblings = tc->functionList;
    tc->functionList = funbox;
    ++tc->compiler->functionCount;
    funbox->kids = NULL;
    funbox->parent = tc->funbox;
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
    return funbox;
}

void
JSCompiler::trace(JSTracer *trc)
{
    JSObjectBox *objbox;

    JS_ASSERT(tempRoot.u.compiler == this);
    objbox = traceListHead;
    while (objbox) {
        JS_CALL_OBJECT_TRACER(trc, objbox->object, "parser.object");
        objbox = objbox->traceLink;
    }
}

static void
UnlinkFunctionBoxes(JSParseNode *pn, JSTreeContext *tc);

static void
UnlinkFunctionBox(JSParseNode *pn, JSTreeContext *tc)
{
    JSFunctionBox *funbox = pn->pn_funbox;
    if (funbox) {
        JS_ASSERT(funbox->node == pn);
        funbox->node = NULL;

        JSFunctionBox **funboxp = &tc->functionList;
        while (*funboxp) {
            if (*funboxp == funbox) {
                *funboxp = funbox->siblings;
                break;
            }
            funboxp = &(*funboxp)->siblings;
        }

        uint32 oldflags = tc->flags;
        JSFunctionBox *oldlist = tc->functionList;

        tc->flags = funbox->tcflags;
        tc->functionList = funbox->kids;
        UnlinkFunctionBoxes(pn->pn_body, tc);
        funbox->kids = tc->functionList;
        tc->flags = oldflags;
        tc->functionList = oldlist;

        
        pn->pn_funbox = NULL;
    }
}

static void
UnlinkFunctionBoxes(JSParseNode *pn, JSTreeContext *tc)
{
    if (pn) {
        switch (pn->pn_arity) {
          case PN_NULLARY:
            return;
          case PN_UNARY:
            UnlinkFunctionBoxes(pn->pn_kid, tc);
            return;
          case PN_BINARY:
            UnlinkFunctionBoxes(pn->pn_left, tc);
            UnlinkFunctionBoxes(pn->pn_right, tc);
            return;
          case PN_TERNARY:
            UnlinkFunctionBoxes(pn->pn_kid1, tc);
            UnlinkFunctionBoxes(pn->pn_kid2, tc);
            UnlinkFunctionBoxes(pn->pn_kid3, tc);
            return;
          case PN_LIST:
            for (JSParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next)
                UnlinkFunctionBoxes(pn2, tc);
            return;
          case PN_FUNC:
            UnlinkFunctionBox(pn, tc);
            return;
          case PN_NAME:
            UnlinkFunctionBoxes(pn->maybeExpr(), tc);
            return;
          case PN_NAMESET:
            UnlinkFunctionBoxes(pn->pn_tree, tc);
        }
    }
}

static void
RecycleFuncNameKids(JSParseNode *pn, JSTreeContext *tc);

static JSParseNode *
RecycleTree(JSParseNode *pn, JSTreeContext *tc)
{
    JSParseNode *next, **head;

    if (!pn)
        return NULL;

    
    JS_ASSERT(pn != tc->compiler->nodeList);
    next = pn->pn_next;
    if (pn->pn_used || pn->pn_defn) {
        







        pn->pn_next = NULL;
        RecycleFuncNameKids(pn, tc);
    } else {
        UnlinkFunctionBoxes(pn, tc);
        head = &tc->compiler->nodeList;
        pn->pn_next = *head;
        *head = pn;
#ifdef METER_PARSENODES
        recyclednodes++;
#endif
    }
    return next;
}

static void
RecycleFuncNameKids(JSParseNode *pn, JSTreeContext *tc)
{
    switch (pn->pn_arity) {
      case PN_FUNC:
        UnlinkFunctionBox(pn, tc);
        

      case PN_NAME:
        





        if (!pn->pn_used && pn->pn_expr) {
            RecycleTree(pn->pn_expr, tc);
            pn->pn_expr = NULL;
        }
        break;

      default:
        JS_ASSERT(PN_TYPE(pn) == TOK_FUNCTION);
    }
}

static JSParseNode *
NewOrRecycledNode(JSTreeContext *tc)
{
    JSParseNode *pn, *pn2;

    pn = tc->compiler->nodeList;
    if (!pn) {
        JSContext *cx = tc->compiler->context;

        JS_ARENA_ALLOCATE_TYPE(pn, JSParseNode, &cx->tempPool);
        if (!pn)
            js_ReportOutOfScriptQuota(cx);
    } else {
        tc->compiler->nodeList = pn->pn_next;

        
        switch (pn->pn_arity) {
          case PN_FUNC:
            RecycleTree(pn->pn_body, tc);
            break;
          case PN_LIST:
            pn2 = pn->pn_head;
            if (pn2) {
                while (pn2 && !pn2->pn_used && !pn2->pn_defn)
                    pn2 = pn2->pn_next;
                if (pn2) {
                    pn2 = pn->pn_head;
                    do {
                        pn2 = RecycleTree(pn2, tc);
                    } while (pn2);
                } else {
                    *pn->pn_tail = tc->compiler->nodeList;
                    tc->compiler->nodeList = pn->pn_head;
#ifdef METER_PARSENODES
                    recyclednodes += pn->pn_count;
#endif
                    break;
                }
            }
            break;
          case PN_TERNARY:
            RecycleTree(pn->pn_kid1, tc);
            RecycleTree(pn->pn_kid2, tc);
            RecycleTree(pn->pn_kid3, tc);
            break;
          case PN_BINARY:
            if (pn->pn_left != pn->pn_right)
                RecycleTree(pn->pn_left, tc);
            RecycleTree(pn->pn_right, tc);
            break;
          case PN_UNARY:
            RecycleTree(pn->pn_kid, tc);
            break;
          case PN_NAME:
            if (!pn->pn_used)
                RecycleTree(pn->pn_expr, tc);
            break;
          case PN_NULLARY:
            break;
        }
    }
    if (pn) {
#ifdef METER_PARSENODES
        parsenodes++;
        if (parsenodes - recyclednodes > maxparsenodes)
            maxparsenodes = parsenodes - recyclednodes;
#endif
        pn->pn_used = pn->pn_defn = false;
        memset(&pn->pn_u, 0, sizeof pn->pn_u);
        pn->pn_next = NULL;
    }
    return pn;
}

static inline void
InitParseNode(JSParseNode *pn, JSTokenType type, JSOp op, JSParseNodeArity arity)
{
    pn->pn_type = type;
    pn->pn_op = op;
    pn->pn_arity = arity;
    pn->pn_parens = false;
    JS_ASSERT(!pn->pn_used);
    JS_ASSERT(!pn->pn_defn);
    pn->pn_next = pn->pn_link = NULL;
}





static JSParseNode *
NewParseNode(JSParseNodeArity arity, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSToken *tp;

    pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    tp = &CURRENT_TOKEN(&tc->compiler->tokenStream);
    InitParseNode(pn, tp->type, JSOP_NOP, arity);
    pn->pn_pos = tp->pos;
    return pn;
}

static inline void
InitNameNodeCommon(JSParseNode *pn, JSTreeContext *tc)
{
    pn->pn_expr = NULL;
    pn->pn_cookie = FREE_UPVAR_COOKIE;
    pn->pn_dflags = tc->atTopLevel() ? PND_TOPLEVEL : 0;
    if (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
        pn->pn_dflags |= PND_BLOCKCHILD;
    pn->pn_blockid = tc->blockid();
}

static JSParseNode *
NewNameNode(JSContext *cx, JSAtom *atom, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = NewParseNode(PN_NAME, tc);
    if (pn) {
        pn->pn_atom = atom;
        InitNameNodeCommon(pn, tc);
    }
    return pn;
}

static JSParseNode *
NewBinary(JSTokenType tt, JSOp op, JSParseNode *left, JSParseNode *right,
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
        right->pn_type == TOK_NUMBER) {
        left->pn_dval += right->pn_dval;
        left->pn_pos.end = right->pn_pos.end;
        RecycleTree(right, tc);
        return left;
    }

    pn = NewOrRecycledNode(tc);
    if (!pn)
        return NULL;
    InitParseNode(pn, tt, op, PN_BINARY);
    pn->pn_pos.begin = left->pn_pos.begin;
    pn->pn_pos.end = right->pn_pos.end;
    pn->pn_left = left;
    pn->pn_right = right;
    return pn;
}

#if JS_HAS_GETTER_SETTER
static JSTokenType
CheckGetterOrSetter(JSContext *cx, JSTokenStream *ts, JSTokenType tt)
{
    JSAtom *atom;
    JSRuntime *rt;
    JSOp op;
    const char *name;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_NAME);
    atom = CURRENT_TOKEN(ts).t_atom;
    rt = cx->runtime;
    if (atom == rt->atomState.getterAtom)
        op = JSOP_GETTER;
    else if (atom == rt->atomState.setterAtom)
        op = JSOP_SETTER;
    else
        return TOK_NAME;
    if (js_PeekTokenSameLine(cx, ts) != tt)
        return TOK_NAME;
    (void) js_GetToken(cx, ts);
    if (CURRENT_TOKEN(ts).t_op != JSOP_NOP) {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_BAD_GETTER_OR_SETTER,
                                    (op == JSOP_GETTER)
                                    ? js_getter_str
                                    : js_setter_str);
        return TOK_ERROR;
    }
    CURRENT_TOKEN(ts).t_op = op;
    if (JS_HAS_STRICT_OPTION(cx)) {
        name = js_AtomToPrintableString(cx, atom);
        if (!name ||
            !js_ReportCompileErrorNumber(cx, ts, NULL,
                                         JSREPORT_WARNING | JSREPORT_STRICT,
                                         JSMSG_DEPRECATED_USAGE,
                                         name)) {
            return TOK_ERROR;
        }
    }
    return tt;
}
#endif

static bool
GenerateBlockId(JSTreeContext *tc, uint32& blockid)
{
    if (tc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(tc->compiler->context, js_GetErrorMessage, NULL,
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
JSCompiler::parse(JSObject *chain)
{
    







    JSTreeContext tc(this);
    tc.scopeChain = chain;
    if (!GenerateBlockId(&tc, tc.bodyid))
        return NULL;

    JSParseNode *pn = Statements(context, TS(this), &tc);
    if (pn) {
        if (!js_MatchToken(context, TS(this), TOK_EOF)) {
            js_ReportCompileErrorNumber(context, TS(this), NULL, JSREPORT_ERROR,
                                        JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else {
            if (!js_FoldConstants(context, pn, &tc))
                pn = NULL;
        }
    }
    return pn;
}

JS_STATIC_ASSERT(FREE_STATIC_LEVEL == JS_BITMASK(JSFB_LEVEL_BITS));

static inline bool
SetStaticLevel(JSTreeContext *tc, uintN staticLevel)
{
    








    if (staticLevel >= FREE_STATIC_LEVEL) {
        JS_ReportErrorNumber(tc->compiler->context, js_GetErrorMessage, NULL,
                             JSMSG_TOO_DEEP, js_function_str);
        return false;
    }
    tc->staticLevel = staticLevel;
    return true;
}




JSScript *
JSCompiler::compileScript(JSContext *cx, JSObject *scopeChain, JSStackFrame *callerFrame,
                          JSPrincipals *principals, uint32 tcflags,
                          const jschar *chars, size_t length,
                          FILE *file, const char *filename, uintN lineno,
                          JSString *source ,
                          unsigned staticLevel )
{
    JSCompiler jsc(cx, principals, callerFrame);
    JSArenaPool codePool, notePool;
    JSTokenType tt;
    JSParseNode *pn;
    uint32 scriptGlobals;
    JSScript *script;
    bool inDirectivePrologue;
#ifdef METER_PARSENODES
    void *sbrk(ptrdiff_t), *before = sbrk(0);
#endif

    JS_ASSERT(!(tcflags & ~(TCF_COMPILE_N_GO | TCF_NO_SCRIPT_RVAL | TCF_NEED_MUTABLE_SCRIPT)));

    



    JS_ASSERT_IF(callerFrame, tcflags & TCF_COMPILE_N_GO);
    JS_ASSERT_IF(staticLevel != 0, callerFrame);

    if (!jsc.init(chars, length, file, filename, lineno))
        return NULL;

    JS_InitArenaPool(&codePool, "code", 1024, sizeof(jsbytecode),
                     &cx->scriptStackQuota);
    JS_InitArenaPool(&notePool, "note", 1024, sizeof(jssrcnote),
                     &cx->scriptStackQuota);

    JSCodeGenerator cg(&jsc, &codePool, &notePool, jsc.tokenStream.lineno);

    MUST_FLOW_THROUGH("out");

    
    script = NULL;

    cg.flags |= tcflags;
    cg.scopeChain = scopeChain;
    if (!SetStaticLevel(&cg, staticLevel))
        goto out;

    
    if (callerFrame &&
        callerFrame->script &&
        callerFrame->script->strictModeCode) {
        cg.flags |= TCF_STRICT_MODE_CODE;
        jsc.tokenStream.flags |= TSF_STRICT_MODE_CODE;
    }

    



    JSObjectBox *funbox;
    funbox = NULL;

    if (tcflags & TCF_COMPILE_N_GO) {
        if (source) {
            



            JSAtom *atom = js_AtomizeString(cx, source, 0);
            if (!atom || !cg.atomList.add(&jsc, atom))
                goto out;
        }

        if (callerFrame && callerFrame->fun) {
            




            funbox = jsc.newObjectBox(FUN_OBJECT(callerFrame->fun));
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

    CG_SWITCH_TO_PROLOG(&cg);
    if (js_Emit1(cx, &cg, JSOP_TRACE) < 0)
        goto out;
    CG_SWITCH_TO_MAIN(&cg);

    inDirectivePrologue = true;
    for (;;) {
        jsc.tokenStream.flags |= TSF_OPERAND;
        tt = js_PeekToken(cx, &jsc.tokenStream);
        jsc.tokenStream.flags &= ~TSF_OPERAND;
        if (tt <= TOK_EOF) {
            if (tt == TOK_EOF)
                break;
            JS_ASSERT(tt == TOK_ERROR);
            goto out;
        }

        pn = Statement(cx, &jsc.tokenStream, &cg);
        if (!pn)
            goto out;
        JS_ASSERT(!cg.blockNode);

        if (inDirectivePrologue)
            inDirectivePrologue = RecognizeDirectivePrologue(cx, &jsc.tokenStream, &cg, pn);

        if (!js_FoldConstants(cx, pn, &cg))
            goto out;

        if (cg.functionList) {
            if (!jsc.analyzeFunctions(cg.functionList, cg.flags))
                goto out;
            cg.functionList = NULL;
        }

        if (!js_EmitTree(cx, &cg, pn))
            goto out;
#if JS_HAS_XML_SUPPORT
        if (PN_TYPE(pn) != TOK_SEMI ||
            !pn->pn_kid ||
            !TREE_TYPE_IS_XML(PN_TYPE(pn->pn_kid))) {
            onlyXML = false;
        }
#endif
        RecycleTree(pn, &cg);
    }

#if JS_HAS_XML_SUPPORT
    





    if (pn && onlyXML && (tcflags & TCF_NO_SCRIPT_RVAL)) {
        js_ReportCompileErrorNumber(cx, &jsc.tokenStream, NULL, JSREPORT_ERROR,
                                    JSMSG_XML_WHOLE_PROGRAM);
        goto out;
    }
#endif

    




    scriptGlobals = cg.ngvars + cg.regexpList.length;
    if (scriptGlobals != 0 || cg.hasSharps()) {
        jsbytecode *code, *end;
        JSOp op;
        const JSCodeSpec *cs;
        uintN len, slot;

        if (scriptGlobals >= SLOTNO_LIMIT)
            goto too_many_slots;
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
                slot += scriptGlobals;
                if (!(cs->format & JOF_SHARPSLOT))
                    slot += cg.sharpSlots();
                if (slot >= SLOTNO_LIMIT)
                    goto too_many_slots;
                SET_SLOTNO(code, slot);
            }
        }
    }

#ifdef METER_PARSENODES
    printf("Parser growth: %d (%u nodes, %u max, %u unrecycled)\n",
           (char *)sbrk(0) - (char *)before,
           parsenodes,
           maxparsenodes,
           parsenodes - recyclednodes);
    before = sbrk(0);
#endif

    



    if (js_Emit1(cx, &cg, JSOP_STOP) < 0)
        goto out;
#ifdef METER_PARSENODES
    printf("Code-gen growth: %d (%u bytecodes, %u srcnotes)\n",
           (char *)sbrk(0) - (char *)before, CG_OFFSET(&cg), cg.noteCount);
#endif
#ifdef JS_ARENAMETER
    JS_DumpArenaStats(stdout);
#endif
    script = js_NewScriptFromCG(cx, &cg);
    if (script && funbox)
        script->savedCallerFun = true;

#ifdef JS_SCOPE_DEPTH_METER
    if (script) {
        JSObject *obj = scopeChain;
        uintN depth = 1;
        while ((obj = OBJ_GET_PARENT(cx, obj)) != NULL)
            ++depth;
        JS_BASIC_STATS_ACCUM(&cx->runtime->hostenvScopeDepthStats, depth);
    }
#endif

  out:
    JS_FinishArenaPool(&codePool);
    JS_FinishArenaPool(&notePool);
    return script;

  too_many_slots:
    js_ReportCompileErrorNumber(cx, &jsc.tokenStream, NULL,
                                JSREPORT_ERROR, JSMSG_TOO_MANY_LOCALS);
    script = NULL;
    goto out;
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
ReportBadReturn(JSContext *cx, JSTreeContext *tc, uintN flags, uintN errnum,
                uintN anonerrnum)
{
    const char *name;

    JS_ASSERT(tc->flags & TCF_IN_FUNCTION);
    if (tc->fun->atom) {
        name = js_AtomToPrintableString(cx, tc->fun->atom);
    } else {
        errnum = anonerrnum;
        name = NULL;
    }
    return js_ReportCompileErrorNumber(cx, TS(tc->compiler), NULL, flags,
                                       errnum, name);
}

static JSBool
CheckFinalReturn(JSContext *cx, JSTreeContext *tc, JSParseNode *pn)
{
    JS_ASSERT(tc->flags & TCF_IN_FUNCTION);
    return HasFinalReturn(pn) == ENDS_IN_RETURN ||
           ReportBadReturn(cx, tc, JSREPORT_WARNING | JSREPORT_STRICT,
                           JSMSG_NO_RETURN_VALUE, JSMSG_ANON_NO_RETURN_VALUE);
}





bool
CheckStrictAssignment(JSContext *cx, JSTreeContext *tc, JSParseNode *lhs)
{
    if (tc->needStrictChecks() &&
        lhs->pn_type == TOK_NAME) {
        JSAtom *atom = lhs->pn_atom;
        JSAtomState *atomState = &cx->runtime->atomState;
        if (atom == atomState->evalAtom || atom == atomState->argumentsAtom) {
            const char *name = js_AtomToPrintableString(cx, atom);
            if (!name ||
                !js_ReportStrictModeError(cx, TS(tc->compiler), tc, lhs,
                                          JSMSG_DEPRECATED_ASSIGN, name)) {
                return false;
            }
        }
    }
    return true;
}







bool
CheckStrictBinding(JSContext *cx, JSTreeContext *tc, JSAtom *atom, 
                   JSParseNode *pn)
{
    if (!tc->needStrictChecks())
        return true;

    JSAtomState *atomState = &cx->runtime->atomState;
    if (atom == atomState->evalAtom || atom == atomState->argumentsAtom) {
        const char *name = js_AtomToPrintableString(cx, atom);
        if (name)
            js_ReportStrictModeError(cx, TS(tc->compiler), tc, pn,
                                     JSMSG_BAD_BINDING, name);
        return false;
    }
    return true;
}














static bool
CheckStrictFormals(JSContext *cx, JSTreeContext *tc, JSFunction *fun,
                   JSParseNode *pn)
{
    JSAtom *atom;

    if (!tc->needStrictChecks())
        return true;

    atom = js_FindDuplicateFormal(fun);
    if (atom) {
        




        JSDefinition *dn = ALE_DEFN(tc->decls.lookup(atom));
        if (dn->pn_op == JSOP_GETARG)
            pn = dn;
        const char *name = js_AtomToPrintableString(cx, atom);
        if (!name ||
            !js_ReportStrictModeError(cx, TS(tc->compiler), tc, pn,
                                      JSMSG_DUPLICATE_FORMAL, name)) {
            return false;
        }
    }

    if (tc->flags & (TCF_FUN_PARAM_ARGUMENTS | TCF_FUN_PARAM_EVAL)) {
        JSAtomState *atoms = &cx->runtime->atomState;
        atom = (tc->flags & TCF_FUN_PARAM_ARGUMENTS
                ? atoms->argumentsAtom : atoms->evalAtom);
        
        JSDefinition *dn = ALE_DEFN(tc->decls.lookup(atom));
        JS_ASSERT(dn->pn_atom == atom);
        const char *name = js_AtomToPrintableString(cx, atom);
        if (!name ||
            !js_ReportStrictModeError(cx, TS(tc->compiler), tc, dn,
                                      JSMSG_BAD_BINDING, name)) {
            return false;
        }
    }

    return true;
}

static JSParseNode *
FunctionBody(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSStmtInfo stmtInfo;
    uintN oldflags, firstLine;
    JSParseNode *pn;

    JS_ASSERT(tc->flags & TCF_IN_FUNCTION);
    js_PushStatement(tc, &stmtInfo, STMT_BLOCK, -1);
    stmtInfo.flags = SIF_BODY_BLOCK;

    oldflags = tc->flags;
    tc->flags &= ~(TCF_RETURN_EXPR | TCF_RETURN_VOID);

    




    firstLine = ts->lineno;
#if JS_HAS_EXPR_CLOSURES
    if (CURRENT_TOKEN(ts).type == TOK_LC) {
        pn = Statements(cx, ts, tc);
    } else {
        pn = NewParseNode(PN_UNARY, tc);
        if (pn) {
            pn->pn_kid = AssignExpr(cx, ts, tc);
            if (!pn->pn_kid) {
                pn = NULL;
            } else {
                if (tc->flags & TCF_FUN_IS_GENERATOR) {
                    ReportBadReturn(cx, tc, JSREPORT_ERROR,
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
    pn = Statements(cx, ts, tc);
#endif

    if (pn) {
        JS_ASSERT(!(tc->topStmt->flags & SIF_SCOPE));
        js_PopStatement(tc);
        pn->pn_pos.begin.lineno = firstLine;

        
        if (JS_HAS_STRICT_OPTION(cx) && (tc->flags & TCF_RETURN_EXPR) &&
            !CheckFinalReturn(cx, tc, pn)) {
            pn = NULL;
        }
    }

    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}

static JSAtomListElement *
MakePlaceholder(JSParseNode *pn, JSTreeContext *tc)
{
    JSAtomListElement *ale = tc->lexdeps.add(tc->compiler, pn->pn_atom);
    if (!ale)
        return NULL;

    JSDefinition *dn = (JSDefinition *)
        NewNameNode(tc->compiler->context, pn->pn_atom, tc);
    if (!dn)
        return NULL;

    ALE_SET_DEFN(ale, dn);
    dn->pn_defn = true;
    dn->pn_dflags |= PND_PLACEHOLDER;
    return ale;
}

static bool
Define(JSParseNode *pn, JSAtom *atom, JSTreeContext *tc, bool let = false)
{
    JS_ASSERT(!pn->pn_used);
    JS_ASSERT_IF(pn->pn_defn, pn->isPlaceholder());

    JSHashEntry **hep;
    JSAtomListElement *ale = NULL;
    JSAtomList *list = NULL;

    if (let)
        ale = (list = &tc->decls)->rawLookup(atom, hep);
    if (!ale)
        ale = (list = &tc->lexdeps)->rawLookup(atom, hep);

    if (ale) {
        JSDefinition *dn = ALE_DEFN(ale);
        if (dn != pn) {
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

                if ((!pnu || pnu->pn_blockid < tc->bodyid) && list != &tc->decls)
                    list->rawRemove(tc->compiler, ale, hep);
            }
        }
    }

    ale = tc->decls.add(tc->compiler, atom, let ? JSAtomList::SHADOW : JSAtomList::UNIQUE);
    if (!ale)
        return false;
    ALE_SET_DEFN(ale, pn);
    pn->pn_defn = true;
    pn->pn_dflags &= ~PND_PLACEHOLDER;
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
        JS_ASSERT(dn->isTopLevel());
        JS_ASSERT(dn->pn_op == JSOP_NOP);
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
    dn->pn_cookie = FREE_UPVAR_COOKIE;
    dn->pn_dflags &= ~PND_BOUND;
    return dn;
}

static bool
DefineArg(JSParseNode *pn, JSAtom *atom, uintN i, JSTreeContext *tc)
{
    JSParseNode *argpn, *argsbody;

    
    if (atom == tc->compiler->context->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_PARAM_ARGUMENTS;
    if (atom == tc->compiler->context->runtime->atomState.evalAtom)
        tc->flags |= TCF_FUN_PARAM_EVAL;

    




    argpn = NewNameNode(tc->compiler->context, atom, tc);
    if (!argpn)
        return false;
    JS_ASSERT(PN_TYPE(argpn) == TOK_NAME && PN_OP(argpn) == JSOP_NOP);

    
    argpn->pn_dflags |= PND_INITIALIZED;
    if (!Define(argpn, atom, tc))
        return false;

    argsbody = pn->pn_body;
    if (!argsbody) {
        argsbody = NewParseNode(PN_LIST, tc);
        if (!argsbody)
            return false;
        argsbody->pn_type = TOK_ARGSBODY;
        argsbody->pn_op = JSOP_NOP;
        argsbody->makeEmpty();
        pn->pn_body = argsbody;
    }
    argsbody->append(argpn);

    argpn->pn_op = JSOP_GETARG;
    argpn->pn_cookie = MAKE_UPVAR_COOKIE(tc->staticLevel, i);
    argpn->pn_dflags |= PND_BOUND;
    return true;
}





bool
JSCompiler::compileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                                const jschar *chars, size_t length,
                                const char *filename, uintN lineno)
{
    JSCompiler jsc(cx, principals);

    if (!jsc.init(chars, length, NULL, filename, lineno))
        return false;

    
    JSArenaPool codePool, notePool;
    JS_InitArenaPool(&codePool, "code", 1024, sizeof(jsbytecode),
                     &cx->scriptStackQuota);
    JS_InitArenaPool(&notePool, "note", 1024, sizeof(jssrcnote),
                     &cx->scriptStackQuota);

    JSCodeGenerator funcg(&jsc, &codePool, &notePool, jsc.tokenStream.lineno);
    funcg.flags |= TCF_IN_FUNCTION;
    funcg.fun = fun;
    if (!GenerateBlockId(&funcg, funcg.bodyid))
        return NULL;

    
    jsc.tokenStream.tokens[0].type = TOK_NAME;
    JSParseNode *fn = NewParseNode(PN_FUNC, &funcg);
    if (fn) {
        fn->pn_body = NULL;
        fn->pn_cookie = FREE_UPVAR_COOKIE;

        uintN nargs = fun->nargs;
        if (nargs) {
            jsuword *names = js_GetLocalNameArray(cx, fun, &cx->tempPool);
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

    





    CURRENT_TOKEN(&jsc.tokenStream).type = TOK_LC;
    JSParseNode *pn = fn ? FunctionBody(cx, &jsc.tokenStream, &funcg) : NULL;
    if (pn) {
        if (!CheckStrictFormals(cx, &funcg, fun, pn)) {
            pn = NULL;
        } else if (!js_MatchToken(cx, &jsc.tokenStream, TOK_EOF)) {
            js_ReportCompileErrorNumber(cx, &jsc.tokenStream, NULL,
                                        JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
            pn = NULL;
        } else if (!js_FoldConstants(cx, pn, &funcg)) {
            
            pn = NULL;
        } else if (funcg.functionList &&
                   !jsc.analyzeFunctions(funcg.functionList, funcg.flags)) {
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








typedef struct BindData BindData;

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

static JSBool
BindLocalVariable(JSContext *cx, JSFunction *fun, JSAtom *atom,
                  JSLocalKind localKind, bool isArg)
{
    JS_ASSERT(localKind == JSLOCAL_VAR || localKind == JSLOCAL_CONST);

    








    if (atom == cx->runtime->atomState.argumentsAtom && !isArg)
        return JS_TRUE;

    return js_AddLocal(cx, fun, atom, localKind);
}

#if JS_HAS_DESTRUCTURING



static JSParseNode *
DestructuringExpr(JSContext *cx, BindData *data, JSTreeContext *tc,
                  JSTokenType tt);

static JSBool
BindDestructuringArg(JSContext *cx, BindData *data, JSAtom *atom,
                     JSTreeContext *tc)
{
    JSParseNode *pn;

    
    if (atom == tc->compiler->context->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_PARAM_ARGUMENTS;
    if (atom == tc->compiler->context->runtime->atomState.evalAtom)
        tc->flags |= TCF_FUN_PARAM_EVAL;

    JS_ASSERT(tc->flags & TCF_IN_FUNCTION);

    JSLocalKind localKind = js_LookupLocal(cx, tc->fun, atom, NULL);
    if (localKind != JSLOCAL_NONE) {
        js_ReportCompileErrorNumber(cx, TS(tc->compiler), NULL,
                                    JSREPORT_ERROR, JSMSG_DESTRUCT_DUP_ARG);
        return JS_FALSE;
    }
    JS_ASSERT(!tc->decls.lookup(atom));

    pn = data->pn;
    if (!Define(pn, atom, tc))
        return JS_FALSE;

    uintN index = tc->fun->u.i.nvars;
    if (!BindLocalVariable(cx, tc->fun, atom, JSLOCAL_VAR, true))
        return JS_FALSE;
    pn->pn_op = JSOP_SETLOCAL;
    pn->pn_cookie = MAKE_UPVAR_COOKIE(tc->staticLevel, index);
    pn->pn_dflags |= PND_BOUND;
    return JS_TRUE;
}
#endif 

JSFunction *
JSCompiler::newFunction(JSTreeContext *tc, JSAtom *atom, uintN lambda)
{
    JSObject *parent;
    JSFunction *fun;

    JS_ASSERT((lambda & ~JSFUN_LAMBDA) == 0);

    





    while (tc->parent)
        tc = tc->parent;
    parent = (tc->flags & TCF_IN_FUNCTION) ? NULL : tc->scopeChain;

    fun = js_NewFunction(context, NULL, NULL, 0, JSFUN_INTERPRETED | lambda,
                         parent, atom);

    if (fun && !(tc->flags & TCF_COMPILE_N_GO)) {
        STOBJ_CLEAR_PARENT(FUN_OBJECT(fun));
        STOBJ_CLEAR_PROTO(FUN_OBJECT(fun));
    }
    return fun;
}

static JSBool
MatchOrInsertSemicolon(JSContext *cx, JSTokenStream *ts)
{
    JSTokenType tt;

    ts->flags |= TSF_OPERAND;
    tt = js_PeekTokenSameLine(cx, ts);
    ts->flags &= ~TSF_OPERAND;
    if (tt == TOK_ERROR)
        return JS_FALSE;
    if (tt != TOK_EOF && tt != TOK_EOL && tt != TOK_SEMI && tt != TOK_RC) {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_SEMI_BEFORE_STMNT);
        return JS_FALSE;
    }
    (void) js_MatchToken(cx, ts, TOK_SEMI);
    return JS_TRUE;
}

bool
JSCompiler::analyzeFunctions(JSFunctionBox *funbox, uint32& tcflags)
{
    if (!markFunArgs(funbox, tcflags))
        return false;
    setFunctionKinds(funbox, tcflags);
    return true;
}































static uintN
FindFunArgs(JSFunctionBox *funbox, int level, JSFunctionBoxQueue *queue)
{
    uintN allskipmin = FREE_STATIC_LEVEL;

    do {
        JSParseNode *fn = funbox->node;
        JSFunction *fun = (JSFunction *) funbox->object;
        int fnlevel = level;

        






        if (funbox->tcflags & TCF_FUN_HEAVYWEIGHT) {
            fn->setFunArg();
            for (JSFunctionBox *kid = funbox->kids; kid; kid = kid->siblings)
                kid->node->setFunArg();
        }

        




        uintN skipmin = FREE_STATIC_LEVEL;
        JSParseNode *pn = fn->pn_body;

        if (pn->pn_type == TOK_UPVARS) {
            JSAtomList upvars(pn->pn_names);
            JS_ASSERT(upvars.count != 0);

            JSAtomListIterator iter(&upvars);
            JSAtomListElement *ale;

            while ((ale = iter()) != NULL) {
                JSDefinition *lexdep = ALE_DEFN(ale)->resolve();

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
            if (kidskipmin != FREE_STATIC_LEVEL) {
                --kidskipmin;
                if (kidskipmin != 0 && kidskipmin < skipmin)
                    skipmin = kidskipmin;
            }
        }

        





        if (skipmin != FREE_STATIC_LEVEL) {
            fun->u.i.skipmin = skipmin;
            if (skipmin < allskipmin)
                allskipmin = skipmin;
        }
    } while ((funbox = funbox->siblings) != NULL);

    return allskipmin;
}

bool
JSCompiler::markFunArgs(JSFunctionBox *funbox, uintN tcflags)
{
    JSFunctionBoxQueue queue;
    if (!queue.init(functionCount))
        return false;

    FindFunArgs(funbox, -1, &queue);
    while ((funbox = queue.pull()) != NULL) {
        JSParseNode *fn = funbox->node;
        JS_ASSERT(fn->isFunArg());

        JSParseNode *pn = fn->pn_body;
        if (pn->pn_type == TOK_UPVARS) {
            JSAtomList upvars(pn->pn_names);
            JS_ASSERT(upvars.count != 0);

            JSAtomListIterator iter(&upvars);
            JSAtomListElement *ale;

            while ((ale = iter()) != NULL) {
                JSDefinition *lexdep = ALE_DEFN(ale)->resolve();

                if (!lexdep->isFreeVar() &&
                    !lexdep->isFunArg() &&
                    lexdep->kind() == JSDefinition::FUNCTION) {
                    









                    lexdep->setFunArg();

                    JSFunctionBox *afunbox = lexdep->pn_funbox;
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

static bool
OneBlockId(JSParseNode *fn, uint32 id)
{
    if (fn->pn_blockid != id)
        return false;
    if (fn->pn_defn) {
        for (JSParseNode *pn = fn->dn_uses; pn; pn = pn->pn_link) {
            if (pn->pn_blockid != id)
                return false;
        }
    }
    return true;
}

void
JSCompiler::setFunctionKinds(JSFunctionBox *funbox, uint32& tcflags)
{
#ifdef JS_FUNCTION_METERING
# define FUN_METER(x)   JS_RUNTIME_METER(context->runtime, functionMeter.x)
#else
# define FUN_METER(x)   ((void)0)
#endif
    JSFunctionBox *parent = funbox->parent;

    for (;;) {
        JSParseNode *fn = funbox->node;

        if (funbox->kids)
            setFunctionKinds(funbox->kids, tcflags);

        JSParseNode *pn = fn->pn_body;
        JSFunction *fun = (JSFunction *) funbox->object;

        FUN_METER(allfun);
        if (funbox->tcflags & TCF_FUN_HEAVYWEIGHT) {
            FUN_METER(heavy);
            JS_ASSERT(FUN_KIND(fun) == JSFUN_INTERPRETED);
        } else if (pn->pn_type != TOK_UPVARS) {
            















            FUN_METER(nofreeupvar);
            FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
        } else {
            JSAtomList upvars(pn->pn_names);
            JS_ASSERT(upvars.count != 0);

            JSAtomListIterator iter(&upvars);
            JSAtomListElement *ale;

            if (!fn->isFunArg()) {
                
















                bool mutation = !!(funbox->tcflags & TCF_FUN_SETS_OUTER_NAME);
                uintN nupvars = 0;

                







                while ((ale = iter()) != NULL) {
                    JSDefinition *lexdep = ALE_DEFN(ale)->resolve();

                    if (!lexdep->isFreeVar()) {
                        JS_ASSERT(lexdep->frameLevel() <= funbox->level);
                        ++nupvars;
                        if (lexdep->isAssigned())
                            break;
                    }
                }
                if (!ale)
                    mutation = false;

                if (nupvars == 0) {
                    FUN_METER(onlyfreevar);
                    FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
                } else if (!mutation && !(funbox->tcflags & TCF_FUN_IS_GENERATOR)) {
                    




                    FUN_METER(display);
                    FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
                } else {
                    if (!(funbox->tcflags & TCF_FUN_IS_GENERATOR))
                        FUN_METER(setupvar);
                }
            } else {
                uintN nupvars = 0;

                




                while ((ale = iter()) != NULL) {
                    JSDefinition *lexdep = ALE_DEFN(ale)->resolve();

                    if (!lexdep->isFreeVar()) {
                        ++nupvars;

                        



















                        JSFunctionBox *afunbox = funbox;
                        uintN lexdepLevel = lexdep->frameLevel();

                        JS_ASSERT(lexdepLevel <= funbox->level);
                        while (afunbox->level != lexdepLevel) {
                            afunbox = afunbox->parent;

                            








                            JS_ASSERT(afunbox);

                            





                            if (!afunbox || afunbox->node->isFunArg())
                                goto break2;
                        }

                        







                        if (afunbox->inLoop)
                            break;

                        





                        if ((afunbox->parent ? afunbox->parent->tcflags : tcflags)
                            & TCF_FUN_HEAVYWEIGHT) {
                            break;
                        }

                        








                        JSFunction *afun = (JSFunction *) afunbox->object;
                        if (!(afun->flags & JSFUN_LAMBDA)) {
                            if (lexdep->isBindingForm())
                                break;
                            if (lexdep->pn_pos >= afunbox->node->pn_pos)
                                break;
                        }

                        if (!lexdep->isInitialized())
                            break;

                        JSDefinition::Kind lexdepKind = lexdep->kind();
                        if (lexdepKind != JSDefinition::CONST) {
                            if (lexdep->isAssigned())
                                break;

                            













                            if (lexdepKind == JSDefinition::ARG &&
                                ((afunbox->parent ? afunbox->parent->tcflags : tcflags) &
                                 TCF_FUN_USES_ARGUMENTS)) {
                                break;
                            }
                        }

                        





                        if (lexdepKind != JSDefinition::FUNCTION) {
                            















                            if (lexdep->pn_pos.end >= afunbox->node->pn_pos.end)
                                break;

                            if (lexdep->isTopLevel()
                                ? !MinBlockId(afunbox->node, lexdep->pn_blockid)
                                : !lexdep->isBlockChild() ||
                                  !afunbox->node->isBlockChild() ||
                                  !OneBlockId(afunbox->node, lexdep->pn_blockid)) {
                                break;
                            }
                        }
                    }
                }

              break2:
                if (nupvars == 0) {
                    FUN_METER(onlyfreevar);
                    FUN_SET_KIND(fun, JSFUN_NULL_CLOSURE);
                } else if (!ale) {
                    



                    FUN_METER(flat);
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
                } else {
                    FUN_METER(badfunarg);
                }
            }
        }

        if (FUN_KIND(fun) == JSFUN_INTERPRETED) {
            if (pn->pn_type != TOK_UPVARS) {
                if (parent)
                    parent->tcflags |= TCF_FUN_HEAVYWEIGHT;
            } else {
                JSAtomList upvars(pn->pn_names);
                JS_ASSERT(upvars.count != 0);

                JSAtomListIterator iter(&upvars);
                JSAtomListElement *ale;

                








                while ((ale = iter()) != NULL) {
                    JSDefinition *lexdep = ALE_DEFN(ale)->resolve();

                    if (!lexdep->isFreeVar()) {
                        JSFunctionBox *afunbox = funbox->parent;
                        uintN lexdepLevel = lexdep->frameLevel();

                        while (afunbox) {
                            







                            if (afunbox->level + 1U == lexdepLevel ||
                                (lexdepLevel == 0 && lexdep->isLet())) {
                                afunbox->tcflags |= TCF_FUN_HEAVYWEIGHT;
                                break;
                            }
                            afunbox = afunbox->parent;
                        }
                        if (!afunbox && (tcflags & TCF_IN_FUNCTION))
                            tcflags |= TCF_FUN_HEAVYWEIGHT;
                    }
                }
            }
        }

        funbox = funbox->siblings;
        if (!funbox)
            break;
        JS_ASSERT(funbox->parent == parent);
    }
#undef FUN_METER
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
EnterFunction(JSParseNode *fn, JSTreeContext *tc, JSTreeContext *funtc,
              JSAtom *funAtom = NULL, uintN lambda = JSFUN_LAMBDA)
{
    JSFunction *fun = tc->compiler->newFunction(tc, funAtom, lambda);
    if (!fun)
        return NULL;

    
    JSFunctionBox *funbox = tc->compiler->newFunctionBox(FUN_OBJECT(fun), fn, tc);
    if (!funbox)
        return NULL;

    
    funtc->flags |= funbox->tcflags;
    funtc->blockidGen = tc->blockidGen;
    if (!GenerateBlockId(funtc, funtc->bodyid))
        return NULL;
    funtc->fun = fun;
    funtc->funbox = funbox;
    funtc->parent = tc;
    if (!SetStaticLevel(funtc, tc->staticLevel + 1))
        return NULL;

    return funbox;
}

static bool
LeaveFunction(JSParseNode *fn, JSTreeContext *funtc, JSTreeContext *tc,
              JSAtom *funAtom = NULL, uintN lambda = JSFUN_LAMBDA)
{
    tc->blockidGen = funtc->blockidGen;

    fn->pn_funbox->tcflags |= funtc->flags & (TCF_FUN_FLAGS | TCF_COMPILE_N_GO);

    fn->pn_dflags |= PND_INITIALIZED;
    JS_ASSERT_IF(tc->atTopLevel() && lambda == 0 && funAtom,
                 fn->pn_dflags & PND_TOPLEVEL);
    if (!tc->topStmt || tc->topStmt->type == STMT_BLOCK)
        fn->pn_dflags |= PND_BLOCKCHILD;

    






    if (funtc->lexdeps.count != 0) {
        JSAtomListIterator iter(&funtc->lexdeps);
        JSAtomListElement *ale;
        int foundCallee = 0;

        while ((ale = iter()) != NULL) {
            JSAtom *atom = ALE_ATOM(ale);
            JSDefinition *dn = ALE_DEFN(ale);
            JS_ASSERT(dn->isPlaceholder());

            if (atom == funAtom && lambda != 0) {
                dn->pn_op = JSOP_CALLEE;
                dn->pn_cookie = MAKE_UPVAR_COOKIE(funtc->staticLevel, CALLEE_UPVAR_SLOT);
                dn->pn_dflags |= PND_BOUND;

                



                if (dn->isFunArg())
                    fn->pn_funbox->tcflags |= TCF_FUN_USES_OWN_NAME;
                foundCallee = 1;
                continue;
            }

            if (!(fn->pn_funbox->tcflags & TCF_FUN_SETS_OUTER_NAME) &&
                dn->isAssigned()) {
                





                for (JSParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
                    if (pnu->isAssigned() && pnu->pn_blockid >= funtc->bodyid) {
                        fn->pn_funbox->tcflags |= TCF_FUN_SETS_OUTER_NAME;
                        break;
                    }
                }
            }

            JSAtomListElement *outer_ale = tc->decls.lookup(atom);
            if (!outer_ale)
                outer_ale = tc->lexdeps.lookup(atom);
            if (outer_ale) {
                












                JSDefinition *outer_dn = ALE_DEFN(outer_ale);

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
            } else {
                
                outer_ale = tc->lexdeps.add(tc->compiler, atom);
                if (!outer_ale)
                    return false;
                ALE_SET_DEFN(outer_ale, ALE_DEFN(ale));
            }
        }

        if (funtc->lexdeps.count - foundCallee != 0) {
            JSParseNode *body = fn->pn_body;

            fn->pn_body = NewParseNode(PN_NAMESET, tc);
            if (!fn->pn_body)
                return false;

            fn->pn_body->pn_type = TOK_UPVARS;
            fn->pn_body->pn_pos = body->pn_pos;
            if (foundCallee)
                funtc->lexdeps.remove(tc->compiler, funAtom);
            fn->pn_body->pn_names = funtc->lexdeps;
            fn->pn_body->pn_tree = body;
        }

        funtc->lexdeps.clear();
    }

    return true;
}

static JSParseNode *
FunctionDef(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
            uintN lambda)
{
    JSOp op;
    JSParseNode *pn, *body, *result;
    JSTokenType tt;
    JSAtom *funAtom;
    JSAtomListElement *ale;
#if JS_HAS_DESTRUCTURING
    JSParseNode *item, *list = NULL;
    bool destructuringArg = false;
    JSAtom *duplicatedArg = NULL;
#endif

    
#if JS_HAS_GETTER_SETTER
    op = CURRENT_TOKEN(ts).t_op;
#endif
    pn = NewParseNode(PN_FUNC, tc);
    if (!pn)
        return NULL;
    pn->pn_body = NULL;
    pn->pn_cookie = FREE_UPVAR_COOKIE;

    







    bool topLevel = tc->atTopLevel();
    pn->pn_dflags = (lambda || !topLevel) ? PND_FUNARG : 0;

    
    ts->flags |= TSF_KEYWORD_IS_NAME;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_KEYWORD_IS_NAME;
    if (tt == TOK_NAME) {
        funAtom = CURRENT_TOKEN(ts).t_atom;
    } else {
        if (lambda == 0 && (cx->options & JSOPTION_ANONFUNFIX)) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_SYNTAX_ERROR);
            return NULL;
        }
        funAtom = NULL;
        js_UngetToken(ts);
    }

    



    if (lambda == 0 && funAtom) {
        ale = tc->decls.lookup(funAtom);
        if (ale) {
            JSDefinition *dn = ALE_DEFN(ale);
            JSDefinition::Kind dn_kind = dn->kind();

            JS_ASSERT(!dn->pn_used);
            JS_ASSERT(dn->pn_defn);

            if (JS_HAS_STRICT_OPTION(cx) || dn_kind == JSDefinition::CONST) {
                const char *name = js_AtomToPrintableString(cx, funAtom);
                if (!name ||
                    !js_ReportCompileErrorNumber(cx, ts, NULL,
                                                 (dn_kind != JSDefinition::CONST)
                                                 ? JSREPORT_WARNING | JSREPORT_STRICT
                                                 : JSREPORT_ERROR,
                                                 JSMSG_REDECLARED_VAR,
                                                 JSDefinition::kindString(dn_kind),
                                                 name)) {
                    return NULL;
                }
            }

            if (topLevel) {
                ALE_SET_DEFN(ale, pn);
                pn->pn_defn = true;
                pn->dn_uses = dn;               

                if (!MakeDefIntoUse(dn, pn, funAtom, tc))
                    return NULL;
            }
        } else if (topLevel) {
            




            JSHashEntry **hep;

            ale = tc->lexdeps.rawLookup(funAtom, hep);
            if (ale) {
                JSDefinition *fn = ALE_DEFN(ale);

                JS_ASSERT(fn->pn_defn);
                fn->pn_type = TOK_FUNCTION;
                fn->pn_arity = PN_FUNC;
                fn->pn_pos.begin = pn->pn_pos.begin;
                fn->pn_body = NULL;
                fn->pn_cookie = FREE_UPVAR_COOKIE;

                tc->lexdeps.rawRemove(tc->compiler, ale, hep);
                RecycleTree(pn, tc);
                pn = fn;
            }

            if (!Define(pn, funAtom, tc))
                return NULL;
        }

        







        if (topLevel) {
            pn->pn_dflags |= PND_TOPLEVEL;

            if (tc->flags & TCF_IN_FUNCTION) {
                JSLocalKind localKind;
                uintN index;

                






                localKind = js_LookupLocal(cx, tc->fun, funAtom, &index);
                switch (localKind) {
                  case JSLOCAL_NONE:
                  case JSLOCAL_ARG:
                    index = tc->fun->u.i.nvars;
                    if (!js_AddLocal(cx, tc->fun, funAtom, JSLOCAL_VAR))
                        return NULL;
                    

                  case JSLOCAL_VAR:
                    pn->pn_cookie = MAKE_UPVAR_COOKIE(tc->staticLevel, index);
                    pn->pn_dflags |= PND_BOUND;
                    break;

                  default:;
                }
            }
        }
    }

    
    JSTreeContext funtc(tc->compiler);

    JSFunctionBox *funbox = EnterFunction(pn, tc, &funtc, funAtom, lambda);
    if (!funbox)
        return NULL;

    JSFunction *fun = (JSFunction *) funbox->object;

#if JS_HAS_GETTER_SETTER
    if (op != JSOP_NOP)
        fun->flags |= (op == JSOP_GETTER) ? JSPROP_GETTER : JSPROP_SETTER;
#endif

    
    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_FORMAL);
    if (!js_MatchToken(cx, ts, TOK_RP)) {
        do {
            tt = js_GetToken(cx, ts);
            switch (tt) {
#if JS_HAS_DESTRUCTURING
              case TOK_LB:
              case TOK_LC:
              {
                BindData data;
                JSParseNode *lhs, *rhs;
                jsint slot;

                
                if (duplicatedArg)
                    goto report_dup_and_destructuring;
                destructuringArg = true;

                





                data.pn = NULL;
                data.op = JSOP_DEFVAR;
                data.binder = BindDestructuringArg;
                lhs = DestructuringExpr(cx, &data, &funtc, tt);
                if (!lhs)
                    return NULL;

                



                slot = fun->nargs;
                if (!js_AddLocal(cx, fun, NULL, JSLOCAL_ARG))
                    return NULL;

                




                rhs = NewNameNode(cx, cx->runtime->atomState.emptyAtom, &funtc);
                if (!rhs)
                    return NULL;
                rhs->pn_type = TOK_NAME;
                rhs->pn_op = JSOP_GETARG;
                rhs->pn_cookie = MAKE_UPVAR_COOKIE(funtc.staticLevel, slot);
                rhs->pn_dflags |= PND_BOUND;

                item = NewBinary(TOK_ASSIGN, JSOP_NOP, lhs, rhs, &funtc);
                if (!item)
                    return NULL;
                if (!list) {
                    list = NewParseNode(PN_LIST, &funtc);
                    if (!list)
                        return NULL;
                    list->pn_type = TOK_COMMA;
                    list->makeEmpty();
                }
                list->append(item);
                break;
              }
#endif 

              case TOK_NAME:
              {
                JSAtom *atom = CURRENT_TOKEN(ts).t_atom;
                if (!DefineArg(pn, atom, fun->nargs, &funtc))
                    return NULL;
#ifdef JS_HAS_DESTRUCTURING
                









                if (js_LookupLocal(cx, fun, atom, NULL) != JSLOCAL_NONE) {
                    duplicatedArg = atom;
                    if (destructuringArg)
                        goto report_dup_and_destructuring;
                }
#endif
                if (!js_AddLocal(cx, fun, atom, JSLOCAL_ARG))
                    return NULL;
                break;
              }

              default:
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_MISSING_FORMAL);
                
              case TOK_ERROR:
                return NULL;

#if JS_HAS_DESTRUCTURING
              report_dup_and_destructuring:
                JSDefinition *dn = ALE_DEFN(funtc.decls.lookup(duplicatedArg));
                js_ReportCompileErrorNumber(cx, TS(tc->compiler), dn,
                                            JSREPORT_ERROR,
                                            JSMSG_DESTRUCT_DUP_ARG);
                return NULL;
#endif
            }
        } while (js_MatchToken(cx, ts, TOK_COMMA));

        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FORMAL);
    }

#if JS_HAS_EXPR_CLOSURES
    ts->flags |= TSF_OPERAND;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_OPERAND;
    if (tt != TOK_LC) {
        js_UngetToken(ts);
        fun->flags |= JSFUN_EXPR_CLOSURE;
    }
#else
    MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_BODY);
#endif

    body = FunctionBody(cx, ts, &funtc);
    if (!body)
        return NULL;

    if (!CheckStrictBinding(cx, &funtc, funAtom, pn))
        return NULL;

    if (!CheckStrictFormals(cx, &funtc, fun, pn))
        return NULL;

#if JS_HAS_EXPR_CLOSURES
    if (tt == TOK_LC)
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
    else if (lambda == 0 && !MatchOrInsertSemicolon(cx, ts))
        return NULL;
#else
    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_BODY);
#endif
    pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;

#if JS_HAS_DESTRUCTURING
    






    if (list) {
        if (body->pn_arity != PN_LIST) {
            JSParseNode *block;

            block = NewParseNode(PN_LIST, tc);
            if (!block)
                return NULL;
            block->pn_type = TOK_SEQ;
            block->pn_pos = body->pn_pos;
            block->initList(body);

            body = block;
        }

        item = NewParseNode(PN_UNARY, tc);
        if (!item)
            return NULL;

        item->pn_type = TOK_SEMI;
        item->pn_pos.begin = item->pn_pos.end = body->pn_pos.begin;
        item->pn_kid = list;
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
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
    } else {
        




        if (!topLevel && lambda == 0 && funAtom)
            tc->flags |= TCF_FUN_HEAVYWEIGHT;
    }

    result = pn;
    if (lambda != 0) {
        


        op = JSOP_LAMBDA;
    } else if (!funAtom) {
        






        result = NewParseNode(PN_UNARY, tc);
        if (!result)
            return NULL;
        result->pn_type = TOK_SEMI;
        result->pn_pos = pn->pn_pos;
        result->pn_kid = pn;
        op = JSOP_LAMBDA;
    } else if (!topLevel) {
        





        op = JSOP_DEFFUN;
    } else {
        op = JSOP_NOP;
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

    pn->pn_blockid = tc->blockid();

    if (!LeaveFunction(pn, &funtc, tc, funAtom, lambda))
        return NULL;

    
    if (!(tc->flags & TCF_STRICT_MODE_CODE))
        ts->flags &= ~TSF_STRICT_MODE_CODE;

    return result;
}

static JSParseNode *
FunctionStmt(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    return FunctionDef(cx, ts, tc, 0);
}

static JSParseNode *
FunctionExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    return FunctionDef(cx, ts, tc, JSFUN_LAMBDA);
}



















static bool
RecognizeDirectivePrologue(JSContext *cx, JSTokenStream *ts,
                           JSTreeContext *tc, JSParseNode *pn)
{
    if (!pn->isDirectivePrologueMember())
        return false;
    if (pn->isDirective()) {
        JSAtom *directive = pn->pn_kid->pn_atom;
        if (directive == cx->runtime->atomState.useStrictAtom) {
            tc->flags |= TCF_STRICT_MODE_CODE;
            ts->flags |= TSF_STRICT_MODE_CODE;
        }
    }
    return true;
}






static JSParseNode *
Statements(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn, *pn2, *saveBlock;
    JSTokenType tt;
    bool inDirectivePrologue = tc->atTopLevel();

    JS_CHECK_RECURSION(cx, return NULL);

    pn = NewParseNode(PN_LIST, tc);
    if (!pn)
        return NULL;
    pn->pn_type = TOK_LC;
    pn->makeEmpty();
    pn->pn_blockid = tc->blockid();
    saveBlock = tc->blockNode;
    tc->blockNode = pn;

    for (;;) {
        ts->flags |= TSF_OPERAND;
        tt = js_PeekToken(cx, ts);
        ts->flags &= ~TSF_OPERAND;
        if (tt <= TOK_EOF || tt == TOK_RC) {
            if (tt == TOK_ERROR) {
                if (ts->flags & TSF_EOF)
                    ts->flags |= TSF_UNEXPECTED_EOF;
                return NULL;
            }
            break;
        }
        pn2 = Statement(cx, ts, tc);
        if (!pn2) {
            if (ts->flags & TSF_EOF)
                ts->flags |= TSF_UNEXPECTED_EOF;
            return NULL;
        }

        if (inDirectivePrologue) {
            if (RecognizeDirectivePrologue(cx, ts, tc, pn2)) {
                
                RecycleTree(pn2, tc);
                continue;
            } else {
                inDirectivePrologue = false;
            }
        }

        if (pn2->pn_type == TOK_FUNCTION) {
            








            if (tc->atTopLevel())
                pn->pn_xflags |= PNX_FUNCDEFS;
            else
                tc->flags |= TCF_HAS_FUNCTION_STMT;
        }
        pn->append(pn2);
    }

    




    if (tc->blockNode != pn)
        pn = tc->blockNode;
    tc->blockNode = saveBlock;

    pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
    return pn;
}

static JSParseNode *
Condition(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_COND);
    pn = ParenExpr(cx, ts, tc, NULL, NULL);
    if (!pn)
        return NULL;
    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_COND);

    
    if (pn->pn_type == TOK_ASSIGN &&
        pn->pn_op == JSOP_NOP &&
        !pn->pn_parens &&
        !js_ReportCompileErrorNumber(cx, ts, NULL,
                                     JSREPORT_WARNING | JSREPORT_STRICT,
                                     JSMSG_EQUAL_AS_ASSIGN,
                                     "")) {
        return NULL;
    }
    return pn;
}

static JSBool
MatchLabel(JSContext *cx, JSTokenStream *ts, JSParseNode *pn)
{
    JSAtom *label;
    JSTokenType tt;

    tt = js_PeekTokenSameLine(cx, ts);
    if (tt == TOK_ERROR)
        return JS_FALSE;
    if (tt == TOK_NAME) {
        (void) js_GetToken(cx, ts);
        label = CURRENT_TOKEN(ts).t_atom;
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
    JSAtomListElement *ale;
    jsint n;

    



    JS_ASSERT(!tc->atTopLevel());

    pn = data->pn;
    if (!CheckStrictBinding(cx, tc, atom, pn))
        return false;

    blockObj = tc->blockChain;
    ale = tc->decls.lookup(atom);
    if (ale && ALE_DEFN(ale)->pn_blockid == tc->blockid()) {
        const char *name = js_AtomToPrintableString(cx, atom);
        if (name) {
            js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn,
                                        JSREPORT_ERROR, JSMSG_REDECLARED_VAR,
                                        (ale && ALE_DEFN(ale)->isConst())
                                        ? js_const_str
                                        : js_variable_str,
                                        name);
        }
        return JS_FALSE;
    }

    n = OBJ_BLOCK_COUNT(cx, blockObj);
    if (n == JS_BIT(16)) {
        js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn,
                                    JSREPORT_ERROR, data->let.overflow);
        return JS_FALSE;
    }

    



    if (!Define(pn, atom, tc, true))
        return JS_FALSE;

    






    pn->pn_op = JSOP_GETLOCAL;
    pn->pn_cookie = MAKE_UPVAR_COOKIE(tc->staticLevel, n);
    pn->pn_dflags |= PND_LET | PND_BOUND;

    



    if (!js_DefineBlockVariable(cx, blockObj, ATOM_TO_JSID(atom), n))
        return JS_FALSE;

    





    uintN slot = JSSLOT_FREE(&js_BlockClass) + n;
    if (slot >= STOBJ_NSLOTS(blockObj) &&
        !js_GrowSlots(cx, blockObj, slot + 1)) {
        return JS_FALSE;
    }
    OBJ_SCOPE(blockObj)->freeslot = slot + 1;
    STOBJ_SET_SLOT(blockObj, slot, PRIVATE_TO_JSVAL(pn));
    return JS_TRUE;
}

static void
PopStatement(JSTreeContext *tc)
{
    JSStmtInfo *stmt = tc->topStmt;

    if (stmt->flags & SIF_SCOPE) {
        JSObject *obj = stmt->blockObj;
        JSScope *scope = OBJ_SCOPE(obj);
        JS_ASSERT(!OBJ_IS_CLONED_BLOCK(obj));

        for (JSScopeProperty *sprop = scope->lastProperty(); sprop; sprop = sprop->parent) {
            JSAtom *atom = JSID_TO_ATOM(sprop->id);

            
            if (atom == tc->compiler->context->runtime->atomState.emptyAtom)
                continue;
            tc->decls.remove(tc->compiler, atom);
        }

        



        scope->object = NULL;
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

static JSBool
BindVarOrConst(JSContext *cx, BindData *data, JSAtom *atom, JSTreeContext *tc)
{
    JSParseNode *pn = data->pn;

    if (!CheckStrictBinding(cx, tc, atom, pn))
        return false;

    JSStmtInfo *stmt = js_LexicalLookup(tc, atom, NULL);

    if (stmt && stmt->type == STMT_WITH) {
        pn->pn_op = JSOP_NAME;
        data->fresh = false;
        return JS_TRUE;
    }

    JSAtomListElement *ale = tc->decls.lookup(atom);
    JSOp op = data->op;

    if (stmt || ale) {
        JSDefinition *dn = ale ? ALE_DEFN(ale) : NULL;
        JSDefinition::Kind dn_kind = dn ? dn->kind() : JSDefinition::VAR;
        const char *name;

        if (dn_kind == JSDefinition::ARG) {
            name = js_AtomToPrintableString(cx, atom);
            if (!name)
                return JS_FALSE;

            if (op == JSOP_DEFCONST) {
                js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn,
                                            JSREPORT_ERROR, JSMSG_REDECLARED_PARAM,
                                            name);
                return JS_FALSE;
            }
            if (!js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn,
                                             JSREPORT_WARNING | JSREPORT_STRICT,
                                             JSMSG_VAR_HIDES_ARG, name)) {
                return JS_FALSE;
            }
        } else {
            bool error = (op == JSOP_DEFCONST ||
                          dn_kind == JSDefinition::CONST ||
                          (dn_kind == JSDefinition::LET &&
                           (stmt->type != STMT_CATCH || OuterLet(tc, stmt, atom))));

            if (JS_HAS_STRICT_OPTION(cx)
                ? op != JSOP_DEFVAR || dn_kind != JSDefinition::VAR
                : error) {
                name = js_AtomToPrintableString(cx, atom);
                if (!name ||
                    !js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn,
                                                 !error
                                                 ? JSREPORT_WARNING | JSREPORT_STRICT
                                                 : JSREPORT_ERROR,
                                                 JSMSG_REDECLARED_VAR,
                                                 JSDefinition::kindString(dn_kind),
                                                 name)) {
                    return JS_FALSE;
                }
            }
        }
    }

    if (!ale) {
        if (!Define(pn, atom, tc))
            return JS_FALSE;
    } else {
        










        JSDefinition *dn = ALE_DEFN(ale);

        data->fresh = false;

        if (!pn->pn_used) {
            
            JSParseNode *pnu = pn;

            if (pn->pn_defn) {
                pnu = NewNameNode(cx, atom, tc);
                if (!pnu)
                    return JS_FALSE;
            }

            LinkUseToDef(pnu, dn, tc);
            pnu->pn_op = JSOP_NAME;
        }

        while (dn->kind() == JSDefinition::LET) {
            do {
                ale = ALE_NEXT(ale);
            } while (ale && ALE_ATOM(ale) != atom);
            if (!ale)
                break;
            dn = ALE_DEFN(ale);
        }

        if (ale) {
            JS_ASSERT_IF(data->op == JSOP_DEFCONST,
                         dn->kind() == JSDefinition::CONST);
            return JS_TRUE;
        }

        




        if (!pn->pn_defn) {
            JSHashEntry **hep;

            ale = tc->lexdeps.rawLookup(atom, hep);
            if (ale) {
                pn = ALE_DEFN(ale);
                tc->lexdeps.rawRemove(tc->compiler, ale, hep);
            } else {
                JSParseNode *pn2 = NewNameNode(cx, atom, tc);
                if (!pn2)
                    return JS_FALSE;

                
                pn2->pn_type = TOK_NAME;
                pn2->pn_pos = pn->pn_pos;
                pn = pn2;
            }
            pn->pn_op = JSOP_NAME;
        }

        ale = tc->decls.add(tc->compiler, atom, JSAtomList::HOIST);
        if (!ale)
            return JS_FALSE;
        ALE_SET_DEFN(ale, pn);
        pn->pn_defn = true;
        pn->pn_dflags &= ~PND_PLACEHOLDER;
    }

    if (data->op == JSOP_DEFCONST)
        pn->pn_dflags |= PND_CONST;

    if (!(tc->flags & TCF_IN_FUNCTION)) {
        












        pn->pn_op = JSOP_NAME;
        if ((tc->flags & TCF_COMPILING) && !tc->compiler->callerFrame) {
            JSCodeGenerator *cg = (JSCodeGenerator *) tc;

            
            ale = cg->atomList.add(tc->compiler, atom);
            if (!ale)
                return JS_FALSE;

            
            uintN slot = ALE_INDEX(ale);
            if ((slot + 1) >> 16)
                return JS_TRUE;

            if ((uint16)(slot + 1) > cg->ngvars)
                cg->ngvars = (uint16)(slot + 1);

            pn->pn_op = JSOP_GETGVAR;
            pn->pn_cookie = MAKE_UPVAR_COOKIE(tc->staticLevel, slot);
            pn->pn_dflags |= PND_BOUND | PND_GVAR;
        }
        return JS_TRUE;
    }

    if (atom == cx->runtime->atomState.argumentsAtom) {
        pn->pn_op = JSOP_ARGUMENTS;
        pn->pn_dflags |= PND_BOUND;
        return JS_TRUE;
    }

    JSLocalKind localKind = js_LookupLocal(cx, tc->fun, atom, NULL);
    if (localKind == JSLOCAL_NONE) {
        







        localKind = (data->op == JSOP_DEFCONST) ? JSLOCAL_CONST : JSLOCAL_VAR;

        uintN index = tc->fun->u.i.nvars;
        if (!BindLocalVariable(cx, tc->fun, atom, localKind, false))
            return JS_FALSE;
        pn->pn_op = JSOP_GETLOCAL;
        pn->pn_cookie = MAKE_UPVAR_COOKIE(tc->staticLevel, index);
        pn->pn_dflags |= PND_BOUND;
        return JS_TRUE;
    }

    if (localKind == JSLOCAL_ARG) {
        
        JS_ASSERT(ale && ALE_DEFN(ale)->kind() == JSDefinition::ARG);
    } else {
        
        JS_ASSERT(localKind == JSLOCAL_VAR || localKind == JSLOCAL_CONST);
    }
    pn->pn_op = JSOP_NAME;
    return JS_TRUE;
}

static JSBool
MakeSetCall(JSContext *cx, JSParseNode *pn, JSTreeContext *tc, uintN msg)
{
    JSParseNode *pn2;

    JS_ASSERT(pn->pn_arity == PN_LIST);
    JS_ASSERT(pn->pn_op == JSOP_CALL || pn->pn_op == JSOP_EVAL || pn->pn_op == JSOP_APPLY);
    pn2 = pn->pn_head;
    if (pn2->pn_type == TOK_FUNCTION && (pn2->pn_funbox->tcflags & TCF_GENEXP_LAMBDA)) {
        js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn, JSREPORT_ERROR, msg);
        return JS_FALSE;
    }
    pn->pn_op = JSOP_SETCALL;
    return JS_TRUE;
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

        if (dn->frameLevel() != tc->staticLevel) {
            




            JS_ASSERT_IF(dn->pn_cookie != FREE_UPVAR_COOKIE,
                         dn->frameLevel() < tc->staticLevel);
            tc->flags |= TCF_FUN_SETS_OUTER_NAME;
        }
    }

    pn->pn_dflags |= dflag;

    if (pn->pn_atom == cx->runtime->atomState.argumentsAtom)
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
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
        pn->pn_op = (pn->pn_op == JSOP_ARGUMENTS)
                    ? JSOP_SETNAME
                    : (pn->pn_dflags & PND_GVAR)
                    ? JSOP_SETGVAR
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
        js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn,
                                    JSREPORT_ERROR, JSMSG_BAD_LEFTSIDE_OF_ASS);
        return JS_FALSE;
    }

    return JS_TRUE;
}

typedef struct FindPropValData {
    uint32          numvars;    
    uint32          maxstep;    
    JSDHashTable    table;      
} FindPropValData;

typedef struct FindPropValEntry {
    JSDHashEntryHdr hdr;
    JSParseNode     *pnkey;
    JSParseNode     *pnval;
} FindPropValEntry;

#define ASSERT_VALID_PROPERTY_KEY(pnkey)                                      \
    JS_ASSERT(((pnkey)->pn_arity == PN_NULLARY &&                             \
               ((pnkey)->pn_type == TOK_NUMBER ||                             \
                (pnkey)->pn_type == TOK_STRING ||                             \
                (pnkey)->pn_type == TOK_NAME)) ||                             \
               ((pnkey)->pn_arity == PN_NAME && (pnkey)->pn_type == TOK_NAME))

static JSDHashNumber
HashFindPropValKey(JSDHashTable *table, const void *key)
{
    const JSParseNode *pnkey = (const JSParseNode *)key;

    ASSERT_VALID_PROPERTY_KEY(pnkey);
    return (pnkey->pn_type == TOK_NUMBER)
           ? (JSDHashNumber) JS_HASH_DOUBLE(pnkey->pn_dval)
           : ATOM_HASH(pnkey->pn_atom);
}

static JSBool
MatchFindPropValEntry(JSDHashTable *table,
                      const JSDHashEntryHdr *entry,
                      const void *key)
{
    const FindPropValEntry *fpve = (const FindPropValEntry *)entry;
    const JSParseNode *pnkey = (const JSParseNode *)key;

    ASSERT_VALID_PROPERTY_KEY(pnkey);
    return pnkey->pn_type == fpve->pnkey->pn_type &&
           ((pnkey->pn_type == TOK_NUMBER)
            ? pnkey->pn_dval == fpve->pnkey->pn_dval
            : pnkey->pn_atom == fpve->pnkey->pn_atom);
}

static const JSDHashTableOps FindPropValOps = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashFindPropValKey,
    MatchFindPropValEntry,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    NULL
};

#define STEP_HASH_THRESHOLD     10
#define BIG_DESTRUCTURING        5
#define BIG_OBJECT_INIT         20

static JSParseNode *
FindPropertyValue(JSParseNode *pn, JSParseNode *pnid, FindPropValData *data)
{
    FindPropValEntry *entry;
    JSParseNode *pnhit, *pnhead, *pnprop, *pnkey;
    uint32 step;

    
    if (data->table.ops) {
        entry = (FindPropValEntry *)
                JS_DHashTableOperate(&data->table, pnid, JS_DHASH_LOOKUP);
        return JS_DHASH_ENTRY_IS_BUSY(&entry->hdr) ? entry->pnval : NULL;
    }

    
    if (pn->pn_type != TOK_RC)
        return NULL;

    



    pnhit = NULL;
    step = 0;
    ASSERT_VALID_PROPERTY_KEY(pnid);
    pnhead = pn->pn_head;
    if (pnid->pn_type == TOK_NUMBER) {
        for (pnprop = pnhead; pnprop; pnprop = pnprop->pn_next) {
            JS_ASSERT(pnprop->pn_type == TOK_COLON);
            if (pnprop->pn_op == JSOP_NOP) {
                pnkey = pnprop->pn_left;
                ASSERT_VALID_PROPERTY_KEY(pnkey);
                if (pnkey->pn_type == TOK_NUMBER &&
                    pnkey->pn_dval == pnid->pn_dval) {
                    pnhit = pnprop;
                }
                ++step;
            }
        }
    } else {
        for (pnprop = pnhead; pnprop; pnprop = pnprop->pn_next) {
            JS_ASSERT(pnprop->pn_type == TOK_COLON);
            if (pnprop->pn_op == JSOP_NOP) {
                pnkey = pnprop->pn_left;
                ASSERT_VALID_PROPERTY_KEY(pnkey);
                if (pnkey->pn_type == pnid->pn_type &&
                    pnkey->pn_atom == pnid->pn_atom) {
                    pnhit = pnprop;
                }
                ++step;
            }
        }
    }
    if (!pnhit)
        return NULL;

    
    JS_ASSERT(!data->table.ops);
    if (step > data->maxstep) {
        data->maxstep = step;
        if (step >= STEP_HASH_THRESHOLD &&
            data->numvars >= BIG_DESTRUCTURING &&
            pn->pn_count >= BIG_OBJECT_INIT &&
            JS_DHashTableInit(&data->table, &FindPropValOps, pn,
                              sizeof(FindPropValEntry),
                              JS_DHASH_DEFAULT_CAPACITY(pn->pn_count)))
        {
            for (pn = pnhead; pn; pn = pn->pn_next) {
                JS_ASSERT(pnprop->pn_type == TOK_COLON);
                ASSERT_VALID_PROPERTY_KEY(pn->pn_left);
                entry = (FindPropValEntry *)
                        JS_DHashTableOperate(&data->table, pn->pn_left,
                                             JS_DHASH_ADD);
                entry->pnval = pn->pn_right;
            }
        }
    }
    return pnhit->pn_right;
}









































static JSBool
CheckDestructuring(JSContext *cx, BindData *data,
                   JSParseNode *left, JSParseNode *right,
                   JSTreeContext *tc)
{
    JSBool ok;
    FindPropValData fpvd;
    JSParseNode *lhs, *rhs, *pn, *pn2;

    if (left->pn_type == TOK_ARRAYCOMP) {
        js_ReportCompileErrorNumber(cx, TS(tc->compiler), left,
                                    JSREPORT_ERROR, JSMSG_ARRAY_COMP_LEFTSIDE);
        return JS_FALSE;
    }

#if JS_HAS_DESTRUCTURING_SHORTHAND
    if (right && right->pn_arity == PN_LIST && (right->pn_xflags & PNX_DESTRUCT)) {
        js_ReportCompileErrorNumber(cx, TS(tc->compiler), right,
                                    JSREPORT_ERROR, JSMSG_BAD_OBJECT_INIT);
        return JS_FALSE;
    }
#endif

    fpvd.table.ops = NULL;
    lhs = left->pn_head;
    if (left->pn_type == TOK_RB) {
        rhs = (right && right->pn_type == left->pn_type)
              ? right->pn_head
              : NULL;

        while (lhs) {
            pn = lhs, pn2 = rhs;

            
            if (pn->pn_type != TOK_COMMA || pn->pn_arity != PN_NULLARY) {
                if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC) {
                    ok = CheckDestructuring(cx, data, pn, pn2, tc);
                } else {
                    if (data) {
                        if (pn->pn_type != TOK_NAME)
                            goto no_var_name;

                        ok = BindDestructuringVar(cx, data, pn, tc);
                    } else {
                        ok = BindDestructuringLHS(cx, pn, tc);
                    }
                }
                if (!ok)
                    goto out;
            }

            lhs = lhs->pn_next;
            if (rhs)
                rhs = rhs->pn_next;
        }
    } else {
        JS_ASSERT(left->pn_type == TOK_RC);
        fpvd.numvars = left->pn_count;
        fpvd.maxstep = 0;
        rhs = NULL;

        while (lhs) {
            JS_ASSERT(lhs->pn_type == TOK_COLON);
            pn = lhs->pn_right;

            if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC) {
                if (right)
                    rhs = FindPropertyValue(right, lhs->pn_left, &fpvd);
                ok = CheckDestructuring(cx, data, pn, rhs, tc);
            } else if (data) {
                if (pn->pn_type != TOK_NAME)
                    goto no_var_name;

                ok = BindDestructuringVar(cx, data, pn, tc);
            } else {
                ok = BindDestructuringLHS(cx, pn, tc);
            }
            if (!ok)
                goto out;

            lhs = lhs->pn_next;
        }
    }

    

















    if (data &&
        data->binder == BindLet &&
        OBJ_BLOCK_COUNT(cx, tc->blockChain) == 0) {
        ok = !!js_DefineNativeProperty(cx, tc->blockChain,
                                       ATOM_TO_JSID(cx->runtime->
                                                    atomState.emptyAtom),
                                       JSVAL_VOID, NULL, NULL,
                                       JSPROP_ENUMERATE |
                                       JSPROP_PERMANENT |
                                       JSPROP_SHARED,
                                       SPROP_HAS_SHORTID, 0, NULL);
        if (!ok)
            goto out;
    }

    ok = JS_TRUE;

  out:
    if (fpvd.table.ops)
        JS_DHashTableFinish(&fpvd.table);
    return ok;

  no_var_name:
    js_ReportCompileErrorNumber(cx, TS(tc->compiler), pn, JSREPORT_ERROR,
                                JSMSG_NO_VARIABLE_NAME);
    ok = JS_FALSE;
    goto out;
}


















static JSBool
UndominateInitializers(JSParseNode *left, JSParseNode *right, JSTreeContext *tc)
{
    FindPropValData fpvd;
    JSParseNode *lhs, *rhs;

    JS_ASSERT(left->pn_type != TOK_ARRAYCOMP);
    JS_ASSERT(right);

#if JS_HAS_DESTRUCTURING_SHORTHAND
    if (right->pn_arity == PN_LIST && (right->pn_xflags & PNX_DESTRUCT)) {
        js_ReportCompileErrorNumber(tc->compiler->context, TS(tc->compiler), right,
                                    JSREPORT_ERROR, JSMSG_BAD_OBJECT_INIT);
        return JS_FALSE;
    }
#endif

    if (right->pn_type != left->pn_type)
        return JS_TRUE;

    fpvd.table.ops = NULL;
    lhs = left->pn_head;
    if (left->pn_type == TOK_RB) {
        rhs = right->pn_head;

        while (lhs && rhs) {
            
            if (lhs->pn_type != TOK_COMMA || lhs->pn_arity != PN_NULLARY) {
                if (lhs->pn_type == TOK_RB || lhs->pn_type == TOK_RC) {
                    if (!UndominateInitializers(lhs, rhs, tc))
                        return JS_FALSE;
                } else {
                    lhs->pn_pos.end = rhs->pn_pos.end;
                }
            }

            lhs = lhs->pn_next;
            rhs = rhs->pn_next;
        }
    } else {
        JS_ASSERT(left->pn_type == TOK_RC);
        fpvd.numvars = left->pn_count;
        fpvd.maxstep = 0;

        while (lhs) {
            JS_ASSERT(lhs->pn_type == TOK_COLON);
            JSParseNode *pn = lhs->pn_right;

            rhs = FindPropertyValue(right, lhs->pn_left, &fpvd);
            if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC) {
                if (rhs && !UndominateInitializers(pn, rhs, tc))
                    return JS_FALSE;
            } else {
                if (rhs)
                    pn->pn_pos.end = rhs->pn_pos.end;
            }

            lhs = lhs->pn_next;
        }
    }
    return JS_TRUE;
}

static JSParseNode *
DestructuringExpr(JSContext *cx, BindData *data, JSTreeContext *tc,
                  JSTokenType tt)
{
    JSTokenStream *ts;
    JSParseNode *pn;

    ts = TS(tc->compiler);
    tc->flags |= TCF_DECL_DESTRUCTURING;
    pn = PrimaryExpr(cx, ts, tc, tt, JS_FALSE);
    tc->flags &= ~TCF_DECL_DESTRUCTURING;
    if (!pn)
        return NULL;
    if (!CheckDestructuring(cx, data, pn, NULL, tc))
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
                  tc->compiler->newFunctionBox(opn->pn_funbox->object, pn, tc));
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
        pn->pn_val = opn->pn_val;
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
ContainsStmt(JSParseNode *pn, JSTokenType tt)
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

static JSParseNode *
ReturnOrYield(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
              JSParser operandParser)
{
    JSTokenType tt, tt2;
    JSParseNode *pn, *pn2;

    tt = CURRENT_TOKEN(ts).type;
    if (tt == TOK_RETURN && !(tc->flags & TCF_IN_FUNCTION)) {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_BAD_RETURN_OR_YIELD, js_return_str);
        return NULL;
    }

    pn = NewParseNode(PN_UNARY, tc);
    if (!pn)
        return NULL;

#if JS_HAS_GENERATORS
    if (tt == TOK_YIELD)
        tc->flags |= TCF_FUN_IS_GENERATOR;
#endif

    
    ts->flags |= TSF_OPERAND;
    tt2 = js_PeekTokenSameLine(cx, ts);
    ts->flags &= ~TSF_OPERAND;
    if (tt2 == TOK_ERROR)
        return NULL;

    if (tt2 != TOK_EOF && tt2 != TOK_EOL && tt2 != TOK_SEMI && tt2 != TOK_RC
#if JS_HAS_GENERATORS
        && (tt != TOK_YIELD ||
            (tt2 != tt && tt2 != TOK_RB && tt2 != TOK_RP &&
             tt2 != TOK_COLON && tt2 != TOK_COMMA))
#endif
        ) {
        pn2 = operandParser(cx, ts, tc);
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
        
        ReportBadReturn(cx, tc, JSREPORT_ERROR,
                        JSMSG_BAD_GENERATOR_RETURN,
                        JSMSG_BAD_ANON_GENERATOR_RETURN);
        return NULL;
    }

    if (JS_HAS_STRICT_OPTION(cx) &&
        (~tc->flags & (TCF_RETURN_EXPR | TCF_RETURN_VOID)) == 0 &&
        !ReportBadReturn(cx, tc, JSREPORT_WARNING | JSREPORT_STRICT,
                         JSMSG_NO_RETURN_VALUE,
                         JSMSG_ANON_NO_RETURN_VALUE)) {
        return NULL;
    }

    return pn;
}

static JSParseNode *
PushLexicalScope(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
                 JSStmtInfo *stmt)
{
    JSParseNode *pn;
    JSObject *obj;
    JSObjectBox *blockbox;

    pn = NewParseNode(PN_NAME, tc);
    if (!pn)
        return NULL;

    obj = js_NewBlockObject(cx);
    if (!obj)
        return NULL;

    blockbox = tc->compiler->newObjectBox(obj);
    if (!blockbox)
        return NULL;

    js_PushBlockScope(tc, stmt, obj, -1);
    pn->pn_type = TOK_LEXICALSCOPE;
    pn->pn_op = JSOP_LEAVEBLOCK;
    pn->pn_objbox = blockbox;
    pn->pn_cookie = FREE_UPVAR_COOKIE;
    pn->pn_dflags = 0;
    if (!GenerateBlockId(tc, stmt->blockid))
        return NULL;
    pn->pn_blockid = stmt->blockid;
    return pn;
}

#if JS_HAS_BLOCK_SCOPE

static JSParseNode *
LetBlock(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc, JSBool statement)
{
    JSParseNode *pn, *pnblock, *pnlet;
    JSStmtInfo stmtInfo;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_LET);

    
    pnlet = NewParseNode(PN_BINARY, tc);
    if (!pnlet)
        return NULL;

    MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_LET);

    
    pnblock = PushLexicalScope(cx, ts, tc, &stmtInfo);
    if (!pnblock)
        return NULL;
    pn = pnblock;
    pn->pn_expr = pnlet;

    pnlet->pn_left = Variables(cx, ts, tc, true);
    if (!pnlet->pn_left)
        return NULL;
    pnlet->pn_left->pn_xflags = PNX_POPVAR;

    MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_LET);

    ts->flags |= TSF_OPERAND;
    if (statement && !js_MatchToken(cx, ts, TOK_LC)) {
        




        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_SEMI;
        pn->pn_num = -1;
        pn->pn_kid = pnblock;

        statement = JS_FALSE;
    }
    ts->flags &= ~TSF_OPERAND;

    if (statement) {
        pnlet->pn_right = Statements(cx, ts, tc);
        if (!pnlet->pn_right)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_LET);
    } else {
        



        pnblock->pn_op = JSOP_LEAVEBLOCKEXPR;
        pnlet->pn_right = AssignExpr(cx, ts, tc);
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
    JSParseNode *pn = NULL;

    JSAtomListElement *ale = tc->decls.lookup(atom);
    if (ale) {
        pn = ALE_DEFN(ale);
        JS_ASSERT(!pn->isPlaceholder());
    } else {
        ale = tc->lexdeps.lookup(atom);
        if (ale) {
            pn = ALE_DEFN(ale);
            JS_ASSERT(pn->isPlaceholder());
        }
    }

    if (pn) {
        JS_ASSERT(pn->pn_defn);

        





        JS_ASSERT_IF(let && pn->pn_blockid == tc->blockid(),
                     pn->pn_blockid != tc->bodyid);

        if (pn->isPlaceholder() && pn->pn_blockid >= (let ? tc->blockid() : tc->bodyid)) {
            if (let)
                pn->pn_blockid = tc->blockid();

            tc->lexdeps.remove(tc->compiler, atom);
            return pn;
        }
    }

    
    pn = NewNameNode(tc->compiler->context, atom, tc);
    if (!pn)
        return NULL;
    return pn;
}

#if JS_HAS_BLOCK_SCOPE
static bool
RebindLets(JSParseNode *pn, JSTreeContext *tc)
{
    if (!pn)
        return true;

    switch (pn->pn_arity) {
      case PN_LIST:
        for (JSParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next)
            RebindLets(pn2, tc);
        break;

      case PN_TERNARY:
        RebindLets(pn->pn_kid1, tc);
        RebindLets(pn->pn_kid2, tc);
        RebindLets(pn->pn_kid3, tc);
        break;

      case PN_BINARY:
        RebindLets(pn->pn_left, tc);
        RebindLets(pn->pn_right, tc);
        break;

      case PN_UNARY:
        RebindLets(pn->pn_kid, tc);
        break;

      case PN_FUNC:
        RebindLets(pn->pn_body, tc);
        break;

      case PN_NAME:
        RebindLets(pn->maybeExpr(), tc);

        if (pn->pn_defn) {
            JS_ASSERT(pn->pn_blockid > tc->topStmt->blockid);
        } else if (pn->pn_used) {
            if (pn->pn_lexdef->pn_blockid == tc->topStmt->blockid) {
                ForgetUse(pn);

                JSAtomListElement *ale = tc->decls.lookup(pn->pn_atom);
                if (ale) {
                    while ((ale = ALE_NEXT(ale)) != NULL) {
                        if (ALE_ATOM(ale) == pn->pn_atom) {
                            LinkUseToDef(pn, ALE_DEFN(ale), tc);
                            return true;
                        }
                    }
                }

                ale = tc->lexdeps.lookup(pn->pn_atom);
                if (!ale) {
                    ale = MakePlaceholder(pn, tc);
                    if (!ale)
                        return NULL;

                    JSDefinition *dn = ALE_DEFN(ale);
                    dn->pn_type = TOK_NAME;
                    dn->pn_op = JSOP_NOP;
                }
                LinkUseToDef(pn, ALE_DEFN(ale), tc);
            }
        }
        break;

      case PN_NAMESET:
        RebindLets(pn->pn_tree, tc);
        break;
    }

    return true;
}
#endif 

static JSParseNode *
Statement(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSTokenType tt;
    JSParseNode *pn, *pn1, *pn2, *pn3, *pn4;
    JSStmtInfo stmtInfo, *stmt, *stmt2;
    JSAtom *label;

    JS_CHECK_RECURSION(cx, return NULL);

    ts->flags |= TSF_OPERAND;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_OPERAND;

#if JS_HAS_GETTER_SETTER
    if (tt == TOK_NAME) {
        tt = CheckGetterOrSetter(cx, ts, TOK_FUNCTION);
        if (tt == TOK_ERROR)
            return NULL;
    }
#endif

    switch (tt) {
      case TOK_FUNCTION:
#if JS_HAS_XML_SUPPORT
        ts->flags |= TSF_KEYWORD_IS_NAME;
        tt = js_PeekToken(cx, ts);
        ts->flags &= ~TSF_KEYWORD_IS_NAME;
        if (tt == TOK_DBLCOLON)
            goto expression;
#endif
        return FunctionStmt(cx, ts, tc);

      case TOK_IF:
        
        pn = NewParseNode(PN_TERNARY, tc);
        if (!pn)
            return NULL;
        pn1 = Condition(cx, ts, tc);
        if (!pn1)
            return NULL;
        js_PushStatement(tc, &stmtInfo, STMT_IF, -1);
        pn2 = Statement(cx, ts, tc);
        if (!pn2)
            return NULL;
        ts->flags |= TSF_OPERAND;
        if (js_MatchToken(cx, ts, TOK_ELSE)) {
            ts->flags &= ~TSF_OPERAND;
            stmtInfo.type = STMT_ELSE;
            pn3 = Statement(cx, ts, tc);
            if (!pn3)
                return NULL;
            pn->pn_pos.end = pn3->pn_pos.end;
        } else {
            ts->flags &= ~TSF_OPERAND;
            pn3 = NULL;
            pn->pn_pos.end = pn2->pn_pos.end;
        }
        PopStatement(tc);
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
        return pn;

      case TOK_SWITCH:
      {
        JSParseNode *pn5, *saveBlock;
        JSBool seenDefault = JS_FALSE;

        pn = NewParseNode(PN_BINARY, tc);
        if (!pn)
            return NULL;
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_SWITCH);

        
        pn1 = ParenExpr(cx, ts, tc, NULL, NULL);
        if (!pn1)
            return NULL;

        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_SWITCH);
        MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_SWITCH);

        



        js_PushStatement(tc, &stmtInfo, STMT_SWITCH, -1);

        
        pn2 = NewParseNode(PN_LIST, tc);
        if (!pn2)
            return NULL;
        pn2->makeEmpty();
        if (!GenerateBlockIdForStmtNode(pn2, tc))
            return NULL;
        saveBlock = tc->blockNode;
        tc->blockNode = pn2;

        while ((tt = js_GetToken(cx, ts)) != TOK_RC) {
            switch (tt) {
              case TOK_DEFAULT:
                if (seenDefault) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_TOO_MANY_DEFAULTS);
                    return NULL;
                }
                seenDefault = JS_TRUE;
                

              case TOK_CASE:
                pn3 = NewParseNode(PN_BINARY, tc);
                if (!pn3)
                    return NULL;
                if (tt == TOK_CASE) {
                    pn3->pn_left = Expr(cx, ts, tc);
                    if (!pn3->pn_left)
                        return NULL;
                }
                pn2->append(pn3);
                if (pn2->pn_count == JS_BIT(16)) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_TOO_MANY_CASES);
                    return NULL;
                }
                break;

              case TOK_ERROR:
                return NULL;

              default:
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_SWITCH);
                return NULL;
            }
            MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_AFTER_CASE);

            pn4 = NewParseNode(PN_LIST, tc);
            if (!pn4)
                return NULL;
            pn4->pn_type = TOK_LC;
            pn4->makeEmpty();
            ts->flags |= TSF_OPERAND;
            while ((tt = js_PeekToken(cx, ts)) != TOK_RC &&
                   tt != TOK_CASE && tt != TOK_DEFAULT) {
                ts->flags &= ~TSF_OPERAND;
                if (tt == TOK_ERROR)
                    return NULL;
                pn5 = Statement(cx, ts, tc);
                if (!pn5)
                    return NULL;
                pn4->pn_pos.end = pn5->pn_pos.end;
                pn4->append(pn5);
                ts->flags |= TSF_OPERAND;
            }
            ts->flags &= ~TSF_OPERAND;

            
            if (pn4->pn_head)
                pn4->pn_pos.begin = pn4->pn_head->pn_pos.begin;
            pn3->pn_pos.end = pn4->pn_pos.end;
            pn3->pn_right = pn4;
        }

        





        if (tc->blockNode != pn2)
            pn2 = tc->blockNode;
        tc->blockNode = saveBlock;
        PopStatement(tc);

        pn->pn_pos.end = pn2->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
        pn->pn_left = pn1;
        pn->pn_right = pn2;
        return pn;
      }

      case TOK_WHILE:
        pn = NewParseNode(PN_BINARY, tc);
        if (!pn)
            return NULL;
        js_PushStatement(tc, &stmtInfo, STMT_WHILE_LOOP, -1);
        pn2 = Condition(cx, ts, tc);
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        pn2 = Statement(cx, ts, tc);
        if (!pn2)
            return NULL;
        PopStatement(tc);
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_right = pn2;
        return pn;

      case TOK_DO:
        pn = NewParseNode(PN_BINARY, tc);
        if (!pn)
            return NULL;
        js_PushStatement(tc, &stmtInfo, STMT_DO_LOOP, -1);
        pn2 = Statement(cx, ts, tc);
        if (!pn2)
            return NULL;
        pn->pn_left = pn2;
        MUST_MATCH_TOKEN(TOK_WHILE, JSMSG_WHILE_AFTER_DO);
        pn2 = Condition(cx, ts, tc);
        if (!pn2)
            return NULL;
        PopStatement(tc);
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_right = pn2;
        if (JSVERSION_NUMBER(cx) != JSVERSION_ECMA_3) {
            




            (void) js_MatchToken(cx, ts, TOK_SEMI);
            return pn;
        }
        break;

      case TOK_FOR:
      {
        JSParseNode *pnseq = NULL;
#if JS_HAS_BLOCK_SCOPE
        JSParseNode *pnlet = NULL;
        JSStmtInfo blockInfo;
#endif

        
        pn = NewParseNode(PN_BINARY, tc);
        if (!pn)
            return NULL;
        js_PushStatement(tc, &stmtInfo, STMT_FOR_LOOP, -1);

        pn->pn_op = JSOP_ITER;
        pn->pn_iflags = 0;
        if (js_MatchToken(cx, ts, TOK_NAME)) {
            if (CURRENT_TOKEN(ts).t_atom == cx->runtime->atomState.eachAtom)
                pn->pn_iflags = JSITER_FOREACH;
            else
                js_UngetToken(ts);
        }

        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);
        ts->flags |= TSF_OPERAND;
        tt = js_PeekToken(cx, ts);
        ts->flags &= ~TSF_OPERAND;

#if JS_HAS_BLOCK_SCOPE
        bool let = false;
#endif

        if (tt == TOK_SEMI) {
            if (pn->pn_iflags & JSITER_FOREACH)
                goto bad_for_each;

            
            pn1 = NULL;
        } else {
            












            tc->flags |= TCF_IN_FOR_INIT;
            if (tt == TOK_VAR) {
                (void) js_GetToken(cx, ts);
                pn1 = Variables(cx, ts, tc, false);
#if JS_HAS_BLOCK_SCOPE
            } else if (tt == TOK_LET) {
                let = true;
                (void) js_GetToken(cx, ts);
                if (js_PeekToken(cx, ts) == TOK_LP) {
                    pn1 = LetBlock(cx, ts, tc, JS_FALSE);
                    tt = TOK_LEXICALSCOPE;
                } else {
                    pnlet = PushLexicalScope(cx, ts, tc, &blockInfo);
                    if (!pnlet)
                        return NULL;
                    blockInfo.flags |= SIF_FOR_BLOCK;
                    pn1 = Variables(cx, ts, tc, false);
                }
#endif
            } else {
                pn1 = Expr(cx, ts, tc);
            }
            tc->flags &= ~TCF_IN_FOR_INIT;
            if (!pn1)
                return NULL;
        }

        





        if (pn1 && js_MatchToken(cx, ts, TOK_IN)) {
            pn->pn_iflags |= JSITER_ENUMERATE;
            stmtInfo.type = STMT_FOR_IN_LOOP;

            
            JS_ASSERT(!TOKEN_TYPE_IS_DECL(tt) || PN_TYPE(pn1) == tt);
            if (TOKEN_TYPE_IS_DECL(tt)
                ? (pn1->pn_count > 1 || pn1->pn_op == JSOP_DEFCONST
#if JS_HAS_DESTRUCTURING
                   || (JSVERSION_NUMBER(cx) == JSVERSION_1_7 &&
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
                   ((JSVERSION_NUMBER(cx) == JSVERSION_1_7 &&
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
                js_ReportCompileErrorNumber(cx, ts, pn1, JSREPORT_ERROR,
                                            JSMSG_BAD_FOR_LEFTSIDE);
                return NULL;
            }

            
            pn2 = NULL;
            uintN dflag = PND_ASSIGNED;

            if (TOKEN_TYPE_IS_DECL(tt)) {
                
                pn1->pn_xflags |= PNX_FORINVAR;

                





                pn2 = pn1->pn_head;
                if ((pn2->pn_type == TOK_NAME && pn2->maybeExpr())
#if JS_HAS_DESTRUCTURING
                    || pn2->pn_type == TOK_ASSIGN
#endif
                    ) {
                    pnseq = NewParseNode(PN_LIST, tc);
                    if (!pnseq)
                        return NULL;
                    pnseq->pn_type = TOK_SEQ;
                    pnseq->pn_pos.begin = pn->pn_pos.begin;

#if JS_HAS_BLOCK_SCOPE
                    if (tt == TOK_LET) {
                        



                        pn3 = NewParseNode(PN_UNARY, tc);
                        if (!pn3)
                            return NULL;
                        pn3->pn_type = TOK_SEMI;
                        pn3->pn_op = JSOP_NOP;
#if JS_HAS_DESTRUCTURING
                        if (pn2->pn_type == TOK_ASSIGN) {
                            pn4 = pn2->pn_right;
                            pn2 = pn1->pn_head = pn2->pn_left;
                        } else
#endif
                        {
                            pn4 = pn2->pn_expr;
                            pn2->pn_expr = NULL;
                        }
                        if (!RebindLets(pn4, tc))
                            return NULL;
                        pn3->pn_pos = pn4->pn_pos;
                        pn3->pn_kid = pn4;
                        pnseq->initList(pn3);
                    } else
#endif 
                    {
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
                            pn1 = NewNameNode(cx, pn2->pn_atom, tc);
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
            }

            if (!pn2) {
                pn2 = pn1;
                if (pn2->pn_type == TOK_LP &&
                    !MakeSetCall(cx, pn2, tc, JSMSG_BAD_LEFTSIDE_OF_ASS)) {
                    return NULL;
                }
#if JS_HAS_XML_SUPPORT
                if (pn2->pn_type == TOK_UNARYOP)
                    pn2->pn_op = JSOP_BINDXMLNAME;
#endif
            }

            switch (pn2->pn_type) {
              case TOK_NAME:
                
                NoteLValue(cx, pn2, tc, dflag);
                break;

#if JS_HAS_DESTRUCTURING
              case TOK_ASSIGN:
                pn2 = pn2->pn_left;
                JS_ASSERT(pn2->pn_type == TOK_RB || pn2->pn_type == TOK_RC);
                
              case TOK_RB:
              case TOK_RC:
                
                if (pn1 == pn2 && !CheckDestructuring(cx, NULL, pn2, NULL, tc))
                    return NULL;

                if (JSVERSION_NUMBER(cx) == JSVERSION_1_7) {
                    



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
            pn2 = Expr(cx, ts, tc);
#if JS_HAS_BLOCK_SCOPE
            if (let)
                tc->topStmt = save;
#endif

            pn2 = NewBinary(TOK_IN, JSOP_NOP, pn1, pn2, tc);
            if (!pn2)
                return NULL;
            pn->pn_left = pn2;
        } else {
            if (pn->pn_iflags & JSITER_FOREACH)
                goto bad_for_each;
            pn->pn_op = JSOP_NOP;

            
            MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_INIT);
            ts->flags |= TSF_OPERAND;
            tt = js_PeekToken(cx, ts);
            ts->flags &= ~TSF_OPERAND;
            if (tt == TOK_SEMI) {
                pn2 = NULL;
            } else {
                pn2 = Expr(cx, ts, tc);
                if (!pn2)
                    return NULL;
            }

            
            MUST_MATCH_TOKEN(TOK_SEMI, JSMSG_SEMI_AFTER_FOR_COND);
            ts->flags |= TSF_OPERAND;
            tt = js_PeekToken(cx, ts);
            ts->flags &= ~TSF_OPERAND;
            if (tt == TOK_RP) {
                pn3 = NULL;
            } else {
                pn3 = Expr(cx, ts, tc);
                if (!pn3)
                    return NULL;
            }

            
            pn4 = NewParseNode(PN_TERNARY, tc);
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

        
        pn2 = Statement(cx, ts, tc);
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
        js_ReportCompileErrorNumber(cx, ts, pn, JSREPORT_ERROR,
                                    JSMSG_BAD_FOR_EACH_LOOP);
        return NULL;
      }

      case TOK_TRY: {
        JSParseNode *catchList, *lastCatch;

        
















        pn = NewParseNode(PN_TERNARY, tc);
        if (!pn)
            return NULL;
        pn->pn_op = JSOP_NOP;

        MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_TRY);
        if (!PushBlocklikeStatement(&stmtInfo, STMT_TRY, tc))
            return NULL;
        pn->pn_kid1 = Statements(cx, ts, tc);
        if (!pn->pn_kid1)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_TRY);
        PopStatement(tc);

        catchList = NULL;
        tt = js_GetToken(cx, ts);
        if (tt == TOK_CATCH) {
            catchList = NewParseNode(PN_LIST, tc);
            if (!catchList)
                return NULL;
            catchList->pn_type = TOK_RESERVED;
            catchList->makeEmpty();
            lastCatch = NULL;

            do {
                JSParseNode *pnblock;
                BindData data;

                
                if (lastCatch && !lastCatch->pn_kid2) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_CATCH_AFTER_GENERAL);
                    return NULL;
                }

                



                pnblock = PushLexicalScope(cx, ts, tc, &stmtInfo);
                if (!pnblock)
                    return NULL;
                stmtInfo.type = STMT_CATCH;

                






                pn2 = NewParseNode(PN_TERNARY, tc);
                if (!pn2)
                    return NULL;
                pnblock->pn_expr = pn2;
                MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_CATCH);

                




                data.pn = NULL;
                data.op = JSOP_NOP;
                data.binder = BindLet;
                data.let.overflow = JSMSG_TOO_MANY_CATCH_VARS;

                tt = js_GetToken(cx, ts);
                switch (tt) {
#if JS_HAS_DESTRUCTURING
                  case TOK_LB:
                  case TOK_LC:
                    pn3 = DestructuringExpr(cx, &data, tc, tt);
                    if (!pn3)
                        return NULL;
                    break;
#endif

                  case TOK_NAME:
                    label = CURRENT_TOKEN(ts).t_atom;
                    pn3 = NewBindingNode(label, tc, true);
                    if (!pn3)
                        return NULL;
                    data.pn = pn3;
                    if (!data.binder(cx, &data, label, tc))
                        return NULL;
                    break;

                  default:
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_CATCH_IDENTIFIER);
                    return NULL;
                }

                pn2->pn_kid1 = pn3;
#if JS_HAS_CATCH_GUARD
                




                if (js_MatchToken(cx, ts, TOK_IF)) {
                    pn2->pn_kid2 = Expr(cx, ts, tc);
                    if (!pn2->pn_kid2)
                        return NULL;
                }
#endif
                MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_CATCH);

                MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_CATCH);
                pn2->pn_kid3 = Statements(cx, ts, tc);
                if (!pn2->pn_kid3)
                    return NULL;
                MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_CATCH);
                PopStatement(tc);

                catchList->append(pnblock);
                lastCatch = pn2;
                ts->flags |= TSF_OPERAND;
                tt = js_GetToken(cx, ts);
                ts->flags &= ~TSF_OPERAND;
            } while (tt == TOK_CATCH);
        }
        pn->pn_kid2 = catchList;

        if (tt == TOK_FINALLY) {
            MUST_MATCH_TOKEN(TOK_LC, JSMSG_CURLY_BEFORE_FINALLY);
            if (!PushBlocklikeStatement(&stmtInfo, STMT_FINALLY, tc))
                return NULL;
            pn->pn_kid3 = Statements(cx, ts, tc);
            if (!pn->pn_kid3)
                return NULL;
            MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_AFTER_FINALLY);
            PopStatement(tc);
        } else {
            js_UngetToken(ts);
        }
        if (!catchList && !pn->pn_kid3) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_CATCH_OR_FINALLY);
            return NULL;
        }
        return pn;
      }

      case TOK_THROW:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;

        
        ts->flags |= TSF_OPERAND;
        tt = js_PeekTokenSameLine(cx, ts);
        ts->flags &= ~TSF_OPERAND;
        if (tt == TOK_ERROR)
            return NULL;
        if (tt == TOK_EOF || tt == TOK_EOL || tt == TOK_SEMI || tt == TOK_RC) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_SYNTAX_ERROR);
            return NULL;
        }

        pn2 = Expr(cx, ts, tc);
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_op = JSOP_THROW;
        pn->pn_kid = pn2;
        break;

      
      case TOK_CATCH:
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_CATCH_WITHOUT_TRY);
        return NULL;

      case TOK_FINALLY:
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_FINALLY_WITHOUT_TRY);
        return NULL;

      case TOK_BREAK:
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        if (!MatchLabel(cx, ts, pn))
            return NULL;
        stmt = tc->topStmt;
        label = pn->pn_atom;
        if (label) {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL && stmt->label == label)
                    break;
            }
        } else {
            for (; ; stmt = stmt->down) {
                if (!stmt) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_TOUGH_BREAK);
                    return NULL;
                }
                if (STMT_IS_LOOP(stmt) || stmt->type == STMT_SWITCH)
                    break;
            }
        }
        if (label)
            pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
        break;

      case TOK_CONTINUE:
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        if (!MatchLabel(cx, ts, pn))
            return NULL;
        stmt = tc->topStmt;
        label = pn->pn_atom;
        if (label) {
            for (stmt2 = NULL; ; stmt = stmt->down) {
                if (!stmt) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_LABEL_NOT_FOUND);
                    return NULL;
                }
                if (stmt->type == STMT_LABEL) {
                    if (stmt->label == label) {
                        if (!stmt2 || !STMT_IS_LOOP(stmt2)) {
                            js_ReportCompileErrorNumber(cx, ts, NULL,
                                                        JSREPORT_ERROR,
                                                        JSMSG_BAD_CONTINUE);
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
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_BAD_CONTINUE);
                    return NULL;
                }
                if (STMT_IS_LOOP(stmt))
                    break;
            }
        }
        if (label)
            pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
        break;

      case TOK_WITH:
        







        if (tc->flags & TCF_STRICT_MODE_CODE) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_STRICT_CODE_WITH);
            return NULL;
        }

        pn = NewParseNode(PN_BINARY, tc);
        if (!pn)
            return NULL;
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_BEFORE_WITH);
        pn2 = ParenExpr(cx, ts, tc, NULL, NULL);
        if (!pn2)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_WITH);
        pn->pn_left = pn2;

        js_PushStatement(tc, &stmtInfo, STMT_WITH, -1);
        pn2 = Statement(cx, ts, tc);
        if (!pn2)
            return NULL;
        PopStatement(tc);

        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_right = pn2;
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        return pn;

      case TOK_VAR:
        pn = Variables(cx, ts, tc, false);
        if (!pn)
            return NULL;

        
        pn->pn_xflags |= PNX_POPVAR;
        break;

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
      {
        JSObject *obj;
        JSObjectBox *blockbox;

        
        if (js_PeekToken(cx, ts) == TOK_LP) {
            pn = LetBlock(cx, ts, tc, JS_TRUE);
            if (!pn || pn->pn_op == JSOP_LEAVEBLOCK)
                return pn;

            
            JS_ASSERT(pn->pn_type == TOK_SEMI ||
                      pn->pn_op == JSOP_LEAVEBLOCKEXPR);
            break;
        }

        










        stmt = tc->topStmt;
        if (stmt &&
            (!STMT_MAYBE_SCOPE(stmt) || (stmt->flags & SIF_FOR_BLOCK))) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_LET_DECL_NOT_IN_BLOCK);
            return NULL;
        }

        if (stmt && (stmt->flags & SIF_SCOPE)) {
            JS_ASSERT(tc->blockChain == stmt->blockObj);
            obj = tc->blockChain;
        } else {
            if (!stmt || (stmt->flags & SIF_BODY_BLOCK)) {
                



                CURRENT_TOKEN(ts).type = TOK_VAR;
                CURRENT_TOKEN(ts).t_op = JSOP_DEFVAR;

                pn = Variables(cx, ts, tc, false);
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

            
            JSObject *obj = js_NewBlockObject(tc->compiler->context);
            if (!obj)
                return NULL;

            blockbox = tc->compiler->newObjectBox(obj);
            if (!blockbox)
                return NULL;

            





            stmt->flags |= SIF_SCOPE;
            stmt->downScope = tc->topScopeStmt;
            tc->topScopeStmt = stmt;
            JS_SCOPE_DEPTH_METERING(++tc->scopeDepth > tc->maxScopeDepth &&
                                    (tc->maxScopeDepth = tc->scopeDepth));

            STOBJ_SET_PARENT(obj, tc->blockChain);
            tc->blockChain = obj;
            stmt->blockObj = obj;

#ifdef DEBUG
            pn1 = tc->blockNode;
            JS_ASSERT(!pn1 || pn1->pn_type != TOK_LEXICALSCOPE);
#endif

            
            pn1 = NewParseNode(PN_NAME, tc);
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

        pn = Variables(cx, ts, tc, false);
        if (!pn)
            return NULL;
        pn->pn_xflags = PNX_POPVAR;
        break;
      }
#endif 

      case TOK_RETURN:
        pn = ReturnOrYield(cx, ts, tc, Expr);
        if (!pn)
            return NULL;
        break;

      case TOK_LC:
      {
        uintN oldflags;

        oldflags = tc->flags;
        tc->flags = oldflags & ~TCF_HAS_FUNCTION_STMT;
        if (!PushBlocklikeStatement(&stmtInfo, STMT_BLOCK, tc))
            return NULL;
        pn = Statements(cx, ts, tc);
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

      case TOK_EOL:
      case TOK_SEMI:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_SEMI;
        return pn;

#if JS_HAS_DEBUGGER_KEYWORD
      case TOK_DEBUGGER:
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_DEBUGGER;
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        break;
#endif 

#if JS_HAS_XML_SUPPORT
      case TOK_DEFAULT:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        if (!js_MatchToken(cx, ts, TOK_NAME) ||
            CURRENT_TOKEN(ts).t_atom != cx->runtime->atomState.xmlAtom ||
            !js_MatchToken(cx, ts, TOK_NAME) ||
            CURRENT_TOKEN(ts).t_atom != cx->runtime->atomState.namespaceAtom ||
            !js_MatchToken(cx, ts, TOK_ASSIGN) ||
            CURRENT_TOKEN(ts).t_op != JSOP_NOP) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_BAD_DEFAULT_XML_NAMESPACE);
            return NULL;
        }

        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        pn2 = Expr(cx, ts, tc);
        if (!pn2)
            return NULL;
        pn->pn_op = JSOP_DEFXMLNS;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
        break;
#endif

      case TOK_ERROR:
        return NULL;

      default:
#if JS_HAS_XML_SUPPORT
      expression:
#endif
        js_UngetToken(ts);
        pn2 = Expr(cx, ts, tc);
        if (!pn2)
            return NULL;

        if (js_PeekToken(cx, ts) == TOK_COLON) {
            if (pn2->pn_type != TOK_NAME) {
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_LABEL);
                return NULL;
            }
            label = pn2->pn_atom;
            for (stmt = tc->topStmt; stmt; stmt = stmt->down) {
                if (stmt->type == STMT_LABEL && stmt->label == label) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_DUPLICATE_LABEL);
                    return NULL;
                }
            }
            ForgetUse(pn2);

            (void) js_GetToken(cx, ts);

            
            js_PushStatement(tc, &stmtInfo, STMT_LABEL, -1);
            stmtInfo.label = label;
            pn = Statement(cx, ts, tc);
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

        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_SEMI;
        pn->pn_pos = pn2->pn_pos;
        pn->pn_kid = pn2;

        





        if (PN_TYPE(pn2) == TOK_ASSIGN && PN_OP(pn2) == JSOP_NOP &&
            PN_OP(pn2->pn_left) == JSOP_SETPROP &&
            PN_OP(pn2->pn_right) == JSOP_LAMBDA &&
            !(pn2->pn_right->pn_funbox->tcflags
              & (TCF_FUN_USES_ARGUMENTS | TCF_FUN_USES_OWN_NAME))) {
            pn2->pn_left->pn_op = JSOP_SETMETHOD;
        }
        break;
    }

    
    return MatchOrInsertSemicolon(cx, ts) ? pn : NULL;
}

static void
NoteArgumentsUse(JSTreeContext *tc)
{
    JS_ASSERT(tc->flags & TCF_IN_FUNCTION);
    tc->flags |= TCF_FUN_USES_ARGUMENTS;
    if (tc->funbox)
        tc->funbox->node->pn_dflags |= PND_FUNARG;
}

static JSParseNode *
Variables(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc, bool inLetHead)
{
    JSTokenType tt;
    bool let;
    JSStmtInfo *scopeStmt;
    BindData data;
    JSParseNode *pn, *pn2;
    JSAtom *atom;

    





    tt = CURRENT_TOKEN(ts).type;
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

    data.op = let ? JSOP_NOP : CURRENT_TOKEN(ts).t_op;
    pn = NewParseNode(PN_LIST, tc);
    if (!pn)
        return NULL;
    pn->pn_op = data.op;
    pn->makeEmpty();

    




    if (let) {
        JS_ASSERT(tc->blockChain == scopeStmt->blockObj);
        data.binder = BindLet;
        data.let.overflow = JSMSG_TOO_MANY_LOCALS;
    } else {
        data.binder = BindVarOrConst;
    }

    do {
        tt = js_GetToken(cx, ts);
#if JS_HAS_DESTRUCTURING
        if (tt == TOK_LB || tt == TOK_LC) {
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn2 = PrimaryExpr(cx, ts, tc, tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn2)
                return NULL;

            if (!CheckDestructuring(cx, &data, pn2, NULL, tc))
                return NULL;
            if ((tc->flags & TCF_IN_FOR_INIT) &&
                js_PeekToken(cx, ts) == TOK_IN) {
                pn->append(pn2);
                continue;
            }

            MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_BAD_DESTRUCT_DECL);
            if (CURRENT_TOKEN(ts).t_op != JSOP_NOP)
                goto bad_var_init;

#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save->down;
                tc->topScopeStmt = saveScope->downScope;
            }
#endif
            JSParseNode *init = AssignExpr(cx, ts, tc);
#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save;
                tc->topScopeStmt = saveScope;
            }
#endif

            if (!init || !UndominateInitializers(pn2, init, tc))
                return NULL;

            pn2 = NewBinary(TOK_ASSIGN, JSOP_NOP, pn2, init, tc);
            if (!pn2)
                return NULL;
            pn->append(pn2);
            continue;
        }
#endif 

        if (tt != TOK_NAME) {
            if (tt != TOK_ERROR) {
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_NO_VARIABLE_NAME);
            }
            return NULL;
        }

        atom = CURRENT_TOKEN(ts).t_atom;
        pn2 = NewBindingNode(atom, tc, let);
        if (!pn2)
            return NULL;
        if (data.op == JSOP_DEFCONST)
            pn2->pn_dflags |= PND_CONST;
        data.pn = pn2;
        if (!data.binder(cx, &data, atom, tc))
            return NULL;
        pn->append(pn2);

        if (js_MatchToken(cx, ts, TOK_ASSIGN)) {
            if (CURRENT_TOKEN(ts).t_op != JSOP_NOP)
                goto bad_var_init;

#if JS_HAS_BLOCK_SCOPE
            if (popScope) {
                tc->topStmt = save->down;
                tc->topScopeStmt = saveScope->downScope;
            }
#endif
            JSParseNode *init = AssignExpr(cx, ts, tc);
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

            pn2->pn_op = (PN_OP(pn2) == JSOP_ARGUMENTS)
                         ? JSOP_SETNAME
                         : (pn2->pn_dflags & PND_GVAR)
                         ? JSOP_SETGVAR
                         : (pn2->pn_dflags & PND_BOUND)
                         ? JSOP_SETLOCAL
                         : (data.op == JSOP_DEFCONST)
                         ? JSOP_SETCONST
                         : JSOP_SETNAME;

            NoteLValue(cx, pn2, tc, data.fresh ? PND_INITIALIZED : PND_ASSIGNED);

            
            pn2->pn_pos.end = init->pn_pos.end;

            if ((tc->flags & TCF_IN_FUNCTION) &&
                atom == cx->runtime->atomState.argumentsAtom) {
                NoteArgumentsUse(tc);
                if (!let)
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
            }
        }
    } while (js_MatchToken(cx, ts, TOK_COMMA));

    pn->pn_pos.end = pn->last()->pn_pos.end;
    return pn;

bad_var_init:
    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                JSMSG_BAD_VAR_INIT);
    return NULL;
}

static JSParseNode *
Expr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn, *pn2;

    pn = AssignExpr(cx, ts, tc);
    if (pn && js_MatchToken(cx, ts, TOK_COMMA)) {
        pn2 = NewParseNode(PN_LIST, tc);
        if (!pn2)
            return NULL;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->initList(pn);
        pn = pn2;
        do {
#if JS_HAS_GENERATORS
            pn2 = pn->last();
            if (pn2->pn_type == TOK_YIELD && !pn2->pn_parens) {
                js_ReportCompileErrorNumber(cx, ts, pn2, JSREPORT_ERROR,
                                            JSMSG_BAD_GENERATOR_SYNTAX,
                                            js_yield_str);
                return NULL;
            }
#endif
            pn2 = AssignExpr(cx, ts, tc);
            if (!pn2)
                return NULL;
            pn->append(pn2);
        } while (js_MatchToken(cx, ts, TOK_COMMA));
        pn->pn_pos.end = pn->last()->pn_pos.end;
    }
    return pn;
}

static JSParseNode *
AssignExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn, *rhs;
    JSTokenType tt;
    JSOp op;

    JS_CHECK_RECURSION(cx, return NULL);

#if JS_HAS_GENERATORS
    ts->flags |= TSF_OPERAND;
    if (js_MatchToken(cx, ts, TOK_YIELD)) {
        ts->flags &= ~TSF_OPERAND;
        return ReturnOrYield(cx, ts, tc, AssignExpr);
    }
    ts->flags &= ~TSF_OPERAND;
#endif

    pn = CondExpr(cx, ts, tc);
    if (!pn)
        return NULL;

    tt = js_GetToken(cx, ts);
#if JS_HAS_GETTER_SETTER
    if (tt == TOK_NAME) {
        tt = CheckGetterOrSetter(cx, ts, TOK_ASSIGN);
        if (tt == TOK_ERROR)
            return NULL;
    }
#endif
    if (tt != TOK_ASSIGN) {
        js_UngetToken(ts);
        return pn;
    }

    op = CURRENT_TOKEN(ts).t_op;
    switch (pn->pn_type) {
      case TOK_NAME:
        if (!CheckStrictAssignment(cx, tc, pn))
            return NULL;
        pn->pn_op = JSOP_SETNAME;
        NoteLValue(cx, pn, tc);
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
        if (op != JSOP_NOP) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_BAD_DESTRUCT_ASS);
            return NULL;
        }
        rhs = AssignExpr(cx, ts, tc);
        if (!rhs || !CheckDestructuring(cx, NULL, pn, rhs, tc))
            return NULL;
        return NewBinary(TOK_ASSIGN, op, pn, rhs, tc);
#endif
      case TOK_LP:
        if (!MakeSetCall(cx, pn, tc, JSMSG_BAD_LEFTSIDE_OF_ASS))
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
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_BAD_LEFTSIDE_OF_ASS);
        return NULL;
    }

    rhs = AssignExpr(cx, ts, tc);
    if (rhs && PN_TYPE(pn) == TOK_NAME && pn->pn_used) {
        JSDefinition *dn = pn->pn_lexdef;

        






        if (!dn->isAssigned()) {
            JS_ASSERT(dn->isInitialized());
            dn->pn_pos.end = rhs->pn_pos.end;
        }
    }

    return NewBinary(TOK_ASSIGN, op, pn, rhs, tc);
}

static JSParseNode *
CondExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn, *pn1, *pn2, *pn3;
    uintN oldflags;

    pn = OrExpr(cx, ts, tc);
    if (pn && js_MatchToken(cx, ts, TOK_HOOK)) {
        pn1 = pn;
        pn = NewParseNode(PN_TERNARY, tc);
        if (!pn)
            return NULL;
        




        oldflags = tc->flags;
        tc->flags &= ~TCF_IN_FOR_INIT;
        pn2 = AssignExpr(cx, ts, tc);
        tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);

        if (!pn2)
            return NULL;
        MUST_MATCH_TOKEN(TOK_COLON, JSMSG_COLON_IN_COND);
        pn3 = AssignExpr(cx, ts, tc);
        if (!pn3)
            return NULL;
        pn->pn_pos.begin = pn1->pn_pos.begin;
        pn->pn_pos.end = pn3->pn_pos.end;
        pn->pn_kid1 = pn1;
        pn->pn_kid2 = pn2;
        pn->pn_kid3 = pn3;
    }
    return pn;
}

static JSParseNode *
OrExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = AndExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_OR))
        pn = NewBinary(TOK_OR, JSOP_OR, pn, AndExpr(cx, ts, tc), tc);
    return pn;
}

static JSParseNode *
AndExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = BitOrExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_AND))
        pn = NewBinary(TOK_AND, JSOP_AND, pn, BitOrExpr(cx, ts, tc), tc);
    return pn;
}

static JSParseNode *
BitOrExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = BitXorExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_BITOR)) {
        pn = NewBinary(TOK_BITOR, JSOP_BITOR, pn, BitXorExpr(cx, ts, tc),
                       tc);
    }
    return pn;
}

static JSParseNode *
BitXorExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = BitAndExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_BITXOR)) {
        pn = NewBinary(TOK_BITXOR, JSOP_BITXOR, pn, BitAndExpr(cx, ts, tc),
                       tc);
    }
    return pn;
}

static JSParseNode *
BitAndExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = EqExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_BITAND))
        pn = NewBinary(TOK_BITAND, JSOP_BITAND, pn, EqExpr(cx, ts, tc), tc);
    return pn;
}

static JSParseNode *
EqExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSOp op;

    pn = RelExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_EQOP)) {
        op = CURRENT_TOKEN(ts).t_op;
        pn = NewBinary(TOK_EQOP, op, pn, RelExpr(cx, ts, tc), tc);
    }
    return pn;
}

static JSParseNode *
RelExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSTokenType tt;
    JSOp op;
    uintN inForInitFlag = tc->flags & TCF_IN_FOR_INIT;

    



    tc->flags &= ~TCF_IN_FOR_INIT;

    pn = ShiftExpr(cx, ts, tc);
    while (pn &&
           (js_MatchToken(cx, ts, TOK_RELOP) ||
            



            (inForInitFlag == 0 && js_MatchToken(cx, ts, TOK_IN)) ||
            js_MatchToken(cx, ts, TOK_INSTANCEOF))) {
        tt = CURRENT_TOKEN(ts).type;
        op = CURRENT_TOKEN(ts).t_op;
        pn = NewBinary(tt, op, pn, ShiftExpr(cx, ts, tc), tc);
    }
    
    tc->flags |= inForInitFlag;

    return pn;
}

static JSParseNode *
ShiftExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSOp op;

    pn = AddExpr(cx, ts, tc);
    while (pn && js_MatchToken(cx, ts, TOK_SHOP)) {
        op = CURRENT_TOKEN(ts).t_op;
        pn = NewBinary(TOK_SHOP, op, pn, AddExpr(cx, ts, tc), tc);
    }
    return pn;
}

static JSParseNode *
AddExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSTokenType tt;
    JSOp op;

    pn = MulExpr(cx, ts, tc);
    while (pn &&
           (js_MatchToken(cx, ts, TOK_PLUS) ||
            js_MatchToken(cx, ts, TOK_MINUS))) {
        tt = CURRENT_TOKEN(ts).type;
        op = (tt == TOK_PLUS) ? JSOP_ADD : JSOP_SUB;
        pn = NewBinary(tt, op, pn, MulExpr(cx, ts, tc), tc);
    }
    return pn;
}

static JSParseNode *
MulExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSTokenType tt;
    JSOp op;

    pn = UnaryExpr(cx, ts, tc);
    while (pn &&
           (js_MatchToken(cx, ts, TOK_STAR) ||
            js_MatchToken(cx, ts, TOK_DIVOP))) {
        tt = CURRENT_TOKEN(ts).type;
        op = CURRENT_TOKEN(ts).t_op;
        pn = NewBinary(tt, op, pn, UnaryExpr(cx, ts, tc), tc);
    }
    return pn;
}

static JSParseNode *
SetLvalKid(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
           JSParseNode *pn, JSParseNode *kid, const char *name)
{
    if (kid->pn_type != TOK_NAME &&
        kid->pn_type != TOK_DOT &&
        (kid->pn_type != TOK_LP ||
         (kid->pn_op != JSOP_CALL && kid->pn_op != JSOP_EVAL && kid->pn_op != JSOP_APPLY)) &&
#if JS_HAS_XML_SUPPORT
        (kid->pn_type != TOK_UNARYOP || kid->pn_op != JSOP_XMLNAME) &&
#endif
        kid->pn_type != TOK_LB) {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_BAD_OPERAND, name);
        return NULL;
    }
    if (!CheckStrictAssignment(cx, tc, kid))
        return NULL;
    pn->pn_kid = kid;
    return kid;
}

static const char incop_name_str[][10] = {"increment", "decrement"};

static JSBool
SetIncOpKid(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
            JSParseNode *pn, JSParseNode *kid,
            JSTokenType tt, JSBool preorder)
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

static JSParseNode *
UnaryExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSTokenType tt;
    JSParseNode *pn, *pn2;

    JS_CHECK_RECURSION(cx, return NULL);

    ts->flags |= TSF_OPERAND;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_OPERAND;

    switch (tt) {
      case TOK_UNARYOP:
      case TOK_PLUS:
      case TOK_MINUS:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_UNARYOP;      
        pn->pn_op = CURRENT_TOKEN(ts).t_op;
        pn2 = UnaryExpr(cx, ts, tc);
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->pn_kid = pn2;
        break;

      case TOK_INC:
      case TOK_DEC:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn2 = MemberExpr(cx, ts, tc, JS_TRUE);
        if (!pn2)
            return NULL;
        if (!SetIncOpKid(cx, ts, tc, pn, pn2, tt, JS_TRUE))
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        break;

      case TOK_DELETE:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn2 = UnaryExpr(cx, ts, tc);
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;

        




        if (!js_FoldConstants(cx, pn2, tc))
            return NULL;
        switch (pn2->pn_type) {
          case TOK_LP:
            if (pn2->pn_op != JSOP_SETCALL &&
                !MakeSetCall(cx, pn2, tc, JSMSG_BAD_DELETE_OPERAND)) {
                return NULL;
            }
            break;
          case TOK_NAME:
            if (!js_ReportStrictModeError(cx, ts, tc, pn, JSMSG_DEPRECATED_DELETE_OPERAND))
                return NULL;
            pn2->pn_op = JSOP_DELNAME;
            break;
          default:;
        }
        pn->pn_kid = pn2;
        break;

      case TOK_ERROR:
        return NULL;

      default:
        js_UngetToken(ts);
        pn = MemberExpr(cx, ts, tc, JS_TRUE);
        if (!pn)
            return NULL;

        
        if (ON_CURRENT_LINE(ts, pn->pn_pos)) {
            ts->flags |= TSF_OPERAND;
            tt = js_PeekTokenSameLine(cx, ts);
            ts->flags &= ~TSF_OPERAND;
            if (tt == TOK_INC || tt == TOK_DEC) {
                (void) js_GetToken(cx, ts);
                pn2 = NewParseNode(PN_UNARY, tc);
                if (!pn2)
                    return NULL;
                if (!SetIncOpKid(cx, ts, tc, pn2, pn, tt, JS_FALSE))
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






static bool
BumpStaticLevel(JSParseNode *pn, JSTreeContext *tc)
{
    if (pn->pn_cookie != FREE_UPVAR_COOKIE) {
        uintN level = UPVAR_FRAME_SKIP(pn->pn_cookie) + 1;

        JS_ASSERT(level >= tc->staticLevel);
        if (level >= FREE_STATIC_LEVEL) {
            JS_ReportErrorNumber(tc->compiler->context, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_DEEP, js_function_str);
            return false;
        }

        pn->pn_cookie = MAKE_UPVAR_COOKIE(level, UPVAR_FRAME_SLOT(pn->pn_cookie));
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
        for (JSParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next)
            transplant(pn2);
        if (pn->pn_pos >= root->pn_pos)
            AdjustBlockId(pn, adjust, tc);
        break;

      case PN_TERNARY:
        transplant(pn->pn_kid1);
        transplant(pn->pn_kid2);
        transplant(pn->pn_kid3);
        break;

      case PN_BINARY:
        transplant(pn->pn_left);

        
        if (pn->pn_right != pn->pn_left)
            transplant(pn->pn_right);
        break;

      case PN_UNARY:
        transplant(pn->pn_kid);
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
        transplant(pn->maybeExpr());
        if (pn->pn_arity == PN_FUNC)
            --funcLevel;

        if (pn->pn_defn) {
            if (genexp && !BumpStaticLevel(pn, tc))
                return false;
        } else if (pn->pn_used) {
            JS_ASSERT(pn->pn_op != JSOP_NOP);
            JS_ASSERT(pn->pn_cookie == FREE_UPVAR_COOKIE);

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
                JS_ASSERT(!tc->decls.lookup(atom));

                if (dn->pn_pos < root->pn_pos || dn->isPlaceholder()) {
                    JSAtomListElement *ale = tc->lexdeps.add(tc->compiler, dn->pn_atom);
                    if (!ale)
                        return false;

                    if (dn->pn_pos >= root->pn_pos) {
                        tc->parent->lexdeps.remove(tc->compiler, atom);
                    } else {
                        JSDefinition *dn2 = (JSDefinition *)
                            NewNameNode(tc->compiler->context, dn->pn_atom, tc);
                        if (!dn2)
                            return false;

                        dn2->pn_type = dn->pn_type;
                        dn2->pn_pos = root->pn_pos;
                        dn2->pn_defn = true;
                        dn2->pn_dflags |= PND_PLACEHOLDER;

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

                    ALE_SET_DEFN(ale, dn);
                }
            }
        }

        if (pn->pn_pos >= root->pn_pos)
            AdjustBlockId(pn, adjust, tc);
        break;

      case PN_NAMESET:
        transplant(pn->pn_tree);
        break;
    }
    return true;
}










static JSParseNode *
ComprehensionTail(JSParseNode *kid, uintN blockid, JSTreeContext *tc,
                  JSTokenType type = TOK_SEMI, JSOp op = JSOP_NOP)
{
    JSContext *cx = tc->compiler->context;
    JSTokenStream *ts = TS(tc->compiler);

    uintN adjust;
    JSParseNode *pn, *pn2, *pn3, **pnp;
    JSStmtInfo stmtInfo;
    BindData data;
    JSTokenType tt;
    JSAtom *atom;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_FOR);

    if (type == TOK_SEMI) {
        




        pn = PushLexicalScope(cx, ts, tc, &stmtInfo);
        if (!pn)
            return NULL;
        adjust = pn->pn_blockid - blockid;
    } else {
        JS_ASSERT(type == TOK_ARRAYPUSH);

        











        adjust = tc->blockid();
        pn = PushLexicalScope(cx, ts, tc, &stmtInfo);
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
        




        pn2 = NewParseNode(PN_BINARY, tc);
        if (!pn2)
            return NULL;

        pn2->pn_op = JSOP_ITER;
        pn2->pn_iflags = JSITER_ENUMERATE;
        if (js_MatchToken(cx, ts, TOK_NAME)) {
            if (CURRENT_TOKEN(ts).t_atom == cx->runtime->atomState.eachAtom)
                pn2->pn_iflags |= JSITER_FOREACH;
            else
                js_UngetToken(ts);
        }
        MUST_MATCH_TOKEN(TOK_LP, JSMSG_PAREN_AFTER_FOR);

        atom = NULL;
        tt = js_GetToken(cx, ts);
        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            tc->flags |= TCF_DECL_DESTRUCTURING;
            pn3 = PrimaryExpr(cx, ts, tc, tt, JS_FALSE);
            tc->flags &= ~TCF_DECL_DESTRUCTURING;
            if (!pn3)
                return NULL;
            break;
#endif

          case TOK_NAME:
            atom = CURRENT_TOKEN(ts).t_atom;

            






            pn3 = NewBindingNode(atom, tc, true);
            if (!pn3)
                return NULL;
            break;

          default:
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_NO_VARIABLE_NAME);

          case TOK_ERROR:
            return NULL;
        }

        MUST_MATCH_TOKEN(TOK_IN, JSMSG_IN_AFTER_FOR_NAME);
        JSParseNode *pn4 = Expr(cx, ts, tc);
        if (!pn4)
            return NULL;
        MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_AFTER_FOR_CTRL);

        switch (tt) {
#if JS_HAS_DESTRUCTURING
          case TOK_LB:
          case TOK_LC:
            if (!CheckDestructuring(cx, &data, pn3, NULL, tc))
                return NULL;

            if (JSVERSION_NUMBER(cx) == JSVERSION_1_7) {
                
                if (pn3->pn_type != TOK_RB || pn3->pn_count != 2) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_BAD_FOR_LEFTSIDE);
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
            if (!data.binder(cx, &data, atom, tc))
                return NULL;
            break;

          default:;
        }

        pn2->pn_left = NewBinary(TOK_IN, JSOP_NOP, pn3, pn4, tc);
        if (!pn2->pn_left)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_right;
    } while (js_MatchToken(cx, ts, TOK_FOR));

    if (js_MatchToken(cx, ts, TOK_IF)) {
        pn2 = NewParseNode(PN_TERNARY, tc);
        if (!pn2)
            return NULL;
        pn2->pn_kid1 = Condition(cx, ts, tc);
        if (!pn2->pn_kid1)
            return NULL;
        *pnp = pn2;
        pnp = &pn2->pn_kid2;
    }

    pn2 = NewParseNode(PN_UNARY, tc);
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
















static JSParseNode *
GeneratorExpr(JSParseNode *pn, JSParseNode *kid, JSTreeContext *tc)
{
    
    JS_ASSERT(pn->pn_arity == PN_UNARY);
    pn->pn_type = TOK_YIELD;
    pn->pn_op = JSOP_YIELD;
    pn->pn_parens = true;
    pn->pn_pos = kid->pn_pos;
    pn->pn_kid = kid;
    pn->pn_hidden = true;

    
    JSParseNode *genfn = NewParseNode(PN_FUNC, tc);
    if (!genfn)
        return NULL;
    genfn->pn_type = TOK_FUNCTION;
    genfn->pn_op = JSOP_LAMBDA;
    JS_ASSERT(!genfn->pn_body);
    genfn->pn_dflags = PND_FUNARG;

    {
        JSTreeContext gentc(tc->compiler);

        JSFunctionBox *funbox = EnterFunction(genfn, tc, &gentc);
        if (!funbox)
            return NULL;

        





        if (tc->flags & TCF_HAS_SHARPS) {
            gentc.flags |= TCF_IN_FUNCTION;
            if (!gentc.ensureSharpSlots())
                return NULL;
        }

        






        gentc.flags |= TCF_FUN_IS_GENERATOR | TCF_GENEXP_LAMBDA |
                       (tc->flags & (TCF_FUN_FLAGS & ~TCF_FUN_PARAM_ARGUMENTS));
        funbox->tcflags |= gentc.flags;
        genfn->pn_funbox = funbox;
        genfn->pn_blockid = gentc.bodyid;

        JSParseNode *body = ComprehensionTail(pn, tc->blockid(), &gentc);
        if (!body)
            return NULL;
        JS_ASSERT(!genfn->pn_body);
        genfn->pn_body = body;
        genfn->pn_pos.begin = body->pn_pos.begin = kid->pn_pos.begin;
        genfn->pn_pos.end = body->pn_pos.end = CURRENT_TOKEN(TS(tc->compiler)).pos.end;

        if (!LeaveFunction(genfn, &gentc, tc))
            return NULL;
    }

    



    JSParseNode *result = NewParseNode(PN_LIST, tc);
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

static JSBool
ArgumentList(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
             JSParseNode *listNode)
{
    JSBool matched;

    ts->flags |= TSF_OPERAND;
    matched = js_MatchToken(cx, ts, TOK_RP);
    ts->flags &= ~TSF_OPERAND;
    if (!matched) {
        do {
            JSParseNode *argNode = AssignExpr(cx, ts, tc);
            if (!argNode)
                return JS_FALSE;
#if JS_HAS_GENERATORS
            if (argNode->pn_type == TOK_YIELD &&
                !argNode->pn_parens &&
                js_PeekToken(cx, ts) == TOK_COMMA) {
                js_ReportCompileErrorNumber(cx, ts, argNode, JSREPORT_ERROR,
                                            JSMSG_BAD_GENERATOR_SYNTAX,
                                            js_yield_str);
                return JS_FALSE;
            }
#endif
#if JS_HAS_GENERATOR_EXPRS
            if (js_MatchToken(cx, ts, TOK_FOR)) {
                JSParseNode *pn = NewParseNode(PN_UNARY, tc);
                if (!pn)
                    return JS_FALSE;
                argNode = GeneratorExpr(pn, argNode, tc);
                if (!argNode)
                    return JS_FALSE;
                if (listNode->pn_count > 1 ||
                    js_PeekToken(cx, ts) == TOK_COMMA) {
                    js_ReportCompileErrorNumber(cx, ts, argNode, JSREPORT_ERROR,
                                                JSMSG_BAD_GENERATOR_SYNTAX,
                                                js_generator_str);
                    return JS_FALSE;
                }
            }
#endif
            listNode->append(argNode);
        } while (js_MatchToken(cx, ts, TOK_COMMA));

        if (js_GetToken(cx, ts) != TOK_RP) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_PAREN_AFTER_ARGS);
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}


static JSParseNode *
CheckForImmediatelyAppliedLambda(JSParseNode *pn)
{
    while (pn->pn_type == TOK_RP)
        pn = pn->pn_kid;
    if (pn->pn_type == TOK_FUNCTION) {
        JS_ASSERT(pn->pn_arity == PN_FUNC);

        JSFunctionBox *funbox = pn->pn_funbox;
        JS_ASSERT(((JSFunction *) funbox->object)->flags & JSFUN_LAMBDA);
        if (!(funbox->tcflags & (TCF_FUN_USES_ARGUMENTS | TCF_FUN_USES_OWN_NAME)))
            pn->pn_dflags &= ~PND_FUNARG;
    }
    return pn;
}

static JSParseNode *
MemberExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
           JSBool allowCallSyntax)
{
    JSParseNode *pn, *pn2, *pn3;
    JSTokenType tt;

    JS_CHECK_RECURSION(cx, return NULL);

    
    ts->flags |= TSF_OPERAND;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_OPERAND;
    if (tt == TOK_NEW) {
        pn = NewParseNode(PN_LIST, tc);
        if (!pn)
            return NULL;
        pn2 = MemberExpr(cx, ts, tc, JS_FALSE);
        if (!pn2)
            return NULL;
        pn2 = CheckForImmediatelyAppliedLambda(pn2);
        pn->pn_op = JSOP_NEW;
        pn->initList(pn2);
        pn->pn_pos.begin = pn2->pn_pos.begin;

        if (js_MatchToken(cx, ts, TOK_LP) && !ArgumentList(cx, ts, tc, pn))
            return NULL;
        if (pn->pn_count > ARGC_LIMIT) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TOO_MANY_CON_ARGS);
            return NULL;
        }
        pn->pn_pos.end = pn->last()->pn_pos.end;
    } else {
        pn = PrimaryExpr(cx, ts, tc, tt, JS_FALSE);
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

    while ((tt = js_GetToken(cx, ts)) > TOK_EOF) {
        if (tt == TOK_DOT) {
            pn2 = NewNameNode(cx, NULL, tc);
            if (!pn2)
                return NULL;
#if JS_HAS_XML_SUPPORT
            ts->flags |= TSF_OPERAND | TSF_KEYWORD_IS_NAME;
            tt = js_GetToken(cx, ts);
            ts->flags &= ~(TSF_OPERAND | TSF_KEYWORD_IS_NAME);
            pn3 = PrimaryExpr(cx, ts, tc, tt, JS_TRUE);
            if (!pn3)
                return NULL;

            
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
                } else if (TOKEN_TYPE_IS_XML(PN_TYPE(pn3))) {
                    pn2->pn_type = TOK_LB;
                    pn2->pn_op = JSOP_GETELEM;
                } else {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_NAME_AFTER_DOT);
                    return NULL;
                }
                pn2->pn_arity = PN_BINARY;
                pn2->pn_left = pn;
                pn2->pn_right = pn3;
            }
#else
            ts->flags |= TSF_KEYWORD_IS_NAME;
            MUST_MATCH_TOKEN(TOK_NAME, JSMSG_NAME_AFTER_DOT);
            ts->flags &= ~TSF_KEYWORD_IS_NAME;
            pn2->pn_op = JSOP_GETPROP;
            pn2->pn_expr = pn;
            pn2->pn_atom = CURRENT_TOKEN(ts).t_atom;
#endif
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
#if JS_HAS_XML_SUPPORT
        } else if (tt == TOK_DBLDOT) {
            pn2 = NewParseNode(PN_BINARY, tc);
            if (!pn2)
                return NULL;
            ts->flags |= TSF_OPERAND | TSF_KEYWORD_IS_NAME;
            tt = js_GetToken(cx, ts);
            ts->flags &= ~(TSF_OPERAND | TSF_KEYWORD_IS_NAME);
            pn3 = PrimaryExpr(cx, ts, tc, tt, JS_TRUE);
            if (!pn3)
                return NULL;
            tt = PN_TYPE(pn3);
            if (tt == TOK_NAME && !pn3->pn_parens) {
                pn3->pn_type = TOK_STRING;
                pn3->pn_arity = PN_NULLARY;
                pn3->pn_op = JSOP_QNAMEPART;
            } else if (!TOKEN_TYPE_IS_XML(tt)) {
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_NAME_AFTER_DOT);
                return NULL;
            }
            pn2->pn_op = JSOP_DESCENDANTS;
            pn2->pn_left = pn;
            pn2->pn_right = pn3;
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
#endif
        } else if (tt == TOK_LB) {
            pn2 = NewParseNode(PN_BINARY, tc);
            if (!pn2)
                return NULL;
            pn3 = Expr(cx, ts, tc);
            if (!pn3)
                return NULL;

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_IN_INDEX);
            pn2->pn_pos.begin = pn->pn_pos.begin;
            pn2->pn_pos.end = CURRENT_TOKEN(ts).pos.end;

            






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
            pn2 = NewParseNode(PN_LIST, tc);
            if (!pn2)
                return NULL;
            pn2->pn_op = JSOP_CALL;

            
            pn = CheckForImmediatelyAppliedLambda(pn);
            if (pn->pn_op == JSOP_NAME) {
                if (pn->pn_atom == cx->runtime->atomState.evalAtom) {
                    
                    pn2->pn_op = JSOP_EVAL;
                    tc->flags |= TCF_FUN_HEAVYWEIGHT;
                }
            } else if (pn->pn_op == JSOP_GETPROP) {
                if (pn->pn_atom == cx->runtime->atomState.applyAtom ||
                    pn->pn_atom == cx->runtime->atomState.callAtom) {
                    
                    pn2->pn_op = JSOP_APPLY;
                }
            }

            pn2->initList(pn);
            pn2->pn_pos.begin = pn->pn_pos.begin;

            if (!ArgumentList(cx, ts, tc, pn2))
                return NULL;
            if (pn2->pn_count > ARGC_LIMIT) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TOO_MANY_FUN_ARGS);
                return NULL;
            }
            pn2->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
        } else {
            js_UngetToken(ts);
            return pn;
        }

        pn = pn2;
    }
    if (tt == TOK_ERROR)
        return NULL;
    return pn;
}

static JSParseNode *
BracketedExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    uintN oldflags;
    JSParseNode *pn;

    




    oldflags = tc->flags;
    tc->flags &= ~TCF_IN_FOR_INIT;
    pn = Expr(cx, ts, tc);
    tc->flags = oldflags | (tc->flags & TCF_FUN_FLAGS);
    return pn;
}

#if JS_HAS_XML_SUPPORT

static JSParseNode *
EndBracketedExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = BracketedExpr(cx, ts, tc);
    if (!pn)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_ATTR_EXPR);
    return pn;
}




















































static JSParseNode *
PropertySelector(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = NewParseNode(PN_NULLARY, tc);
    if (!pn)
        return NULL;
    if (pn->pn_type == TOK_STAR) {
        pn->pn_type = TOK_ANYNAME;
        pn->pn_op = JSOP_ANYNAME;
        pn->pn_atom = cx->runtime->atomState.starAtom;
    } else {
        JS_ASSERT(pn->pn_type == TOK_NAME);
        pn->pn_op = JSOP_QNAMEPART;
        pn->pn_arity = PN_NAME;
        pn->pn_atom = CURRENT_TOKEN(ts).t_atom;
        pn->pn_cookie = FREE_UPVAR_COOKIE;
    }
    return pn;
}

static JSParseNode *
QualifiedSuffix(JSContext *cx, JSTokenStream *ts, JSParseNode *pn,
                JSTreeContext *tc)
{
    JSParseNode *pn2, *pn3;
    JSTokenType tt;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_DBLCOLON);
    pn2 = NewNameNode(cx, NULL, tc);
    if (!pn2)
        return NULL;

    
    if (pn->pn_op == JSOP_QNAMEPART)
        pn->pn_op = JSOP_NAME;

    ts->flags |= TSF_KEYWORD_IS_NAME;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_KEYWORD_IS_NAME;
    if (tt == TOK_STAR || tt == TOK_NAME) {
        
        pn2->pn_op = JSOP_QNAMECONST;
        pn2->pn_pos.begin = pn->pn_pos.begin;
        pn2->pn_atom = (tt == TOK_STAR)
                       ? cx->runtime->atomState.starAtom
                       : CURRENT_TOKEN(ts).t_atom;
        pn2->pn_expr = pn;
        pn2->pn_cookie = FREE_UPVAR_COOKIE;
        return pn2;
    }

    if (tt != TOK_LB) {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    pn3 = EndBracketedExpr(cx, ts, tc);
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

static JSParseNode *
QualifiedIdentifier(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;

    pn = PropertySelector(cx, ts, tc);
    if (!pn)
        return NULL;
    if (js_MatchToken(cx, ts, TOK_DBLCOLON)) {
        
        tc->flags |= TCF_FUN_HEAVYWEIGHT;
        pn = QualifiedSuffix(cx, ts, pn, tc);
    }
    return pn;
}

static JSParseNode *
AttributeIdentifier(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn, *pn2;
    JSTokenType tt;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_AT);
    pn = NewParseNode(PN_UNARY, tc);
    if (!pn)
        return NULL;
    pn->pn_op = JSOP_TOATTRNAME;
    ts->flags |= TSF_KEYWORD_IS_NAME;
    tt = js_GetToken(cx, ts);
    ts->flags &= ~TSF_KEYWORD_IS_NAME;
    if (tt == TOK_STAR || tt == TOK_NAME) {
        pn2 = QualifiedIdentifier(cx, ts, tc);
    } else if (tt == TOK_LB) {
        pn2 = EndBracketedExpr(cx, ts, tc);
    } else {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    if (!pn2)
        return NULL;
    pn->pn_kid = pn2;
    return pn;
}




static JSParseNode *
XMLExpr(JSContext *cx, JSTokenStream *ts, JSBool inTag, JSTreeContext *tc)
{
    JSParseNode *pn, *pn2;
    uintN oldflags;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_LC);
    pn = NewParseNode(PN_UNARY, tc);
    if (!pn)
        return NULL;

    





    oldflags = ts->flags;
    ts->flags = oldflags & ~TSF_XMLTAGMODE;
    pn2 = Expr(cx, ts, tc);
    if (!pn2)
        return NULL;

    MUST_MATCH_TOKEN(TOK_RC, JSMSG_CURLY_IN_XML_EXPR);
    ts->flags = oldflags;
    pn->pn_kid = pn2;
    pn->pn_op = inTag ? JSOP_XMLTAGEXPR : JSOP_XMLELTEXPR;
    return pn;
}







static JSParseNode *
XMLAtomNode(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn;
    JSToken *tp;

    pn = NewParseNode(PN_NULLARY, tc);
    if (!pn)
        return NULL;
    tp = &CURRENT_TOKEN(ts);
    pn->pn_op = tp->t_op;
    pn->pn_atom = tp->t_atom;
    if (tp->type == TOK_XMLPI)
        pn->pn_atom2 = tp->t_atom2;
    return pn;
}













static JSParseNode *
XMLNameExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc)
{
    JSParseNode *pn, *pn2, *list;
    JSTokenType tt;

    pn = list = NULL;
    do {
        tt = CURRENT_TOKEN(ts).type;
        if (tt == TOK_LC) {
            pn2 = XMLExpr(cx, ts, JS_TRUE, tc);
            if (!pn2)
                return NULL;
        } else {
            JS_ASSERT(tt == TOK_XMLNAME);
            pn2 = XMLAtomNode(cx, ts, tc);
            if (!pn2)
                return NULL;
        }

        if (!pn) {
            pn = pn2;
        } else {
            if (!list) {
                list = NewParseNode(PN_LIST, tc);
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
    } while ((tt = js_GetToken(cx, ts)) == TOK_XMLNAME || tt == TOK_LC);

    js_UngetToken(ts);
    return pn;
}





#define XML_FOLDABLE(pn)        ((pn)->pn_arity == PN_LIST                    \
                                 ? ((pn)->pn_xflags & PNX_CANTFOLD) == 0      \
                                 : (pn)->pn_type != TOK_LC)


















static JSParseNode *
XMLTagContent(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
              JSTokenType tagtype, JSAtom **namep)
{
    JSParseNode *pn, *pn2, *list;
    JSTokenType tt;

    pn = XMLNameExpr(cx, ts, tc);
    if (!pn)
        return NULL;
    *namep = (pn->pn_arity == PN_NULLARY) ? pn->pn_atom : NULL;
    list = NULL;

    while (js_MatchToken(cx, ts, TOK_XMLSPACE)) {
        tt = js_GetToken(cx, ts);
        if (tt != TOK_XMLNAME && tt != TOK_LC) {
            js_UngetToken(ts);
            break;
        }

        pn2 = XMLNameExpr(cx, ts, tc);
        if (!pn2)
            return NULL;
        if (!list) {
            list = NewParseNode(PN_LIST, tc);
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

        js_MatchToken(cx, ts, TOK_XMLSPACE);
        MUST_MATCH_TOKEN(TOK_ASSIGN, JSMSG_NO_ASSIGN_IN_XML_ATTR);
        js_MatchToken(cx, ts, TOK_XMLSPACE);

        tt = js_GetToken(cx, ts);
        if (tt == TOK_XMLATTR) {
            pn2 = XMLAtomNode(cx, ts, tc);
        } else if (tt == TOK_LC) {
            pn2 = XMLExpr(cx, ts, JS_TRUE, tc);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_BAD_XML_ATTR_VALUE);
            return NULL;
        }
        if (!pn2)
            return NULL;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->append(pn2);
    }

    return pn;
}

#define XML_CHECK_FOR_ERROR_AND_EOF(tt,result)                                \
    JS_BEGIN_MACRO                                                            \
        if ((tt) <= TOK_EOF) {                                                \
            if ((tt) == TOK_EOF) {                                            \
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,     \
                                            JSMSG_END_OF_XML_SOURCE);         \
            }                                                                 \
            return result;                                                    \
        }                                                                     \
    JS_END_MACRO

static JSParseNode *
XMLElementOrList(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
                 JSBool allowList);





static JSBool
XMLElementContent(JSContext *cx, JSTokenStream *ts, JSParseNode *pn,
                  JSTreeContext *tc)
{
    JSTokenType tt;
    JSParseNode *pn2;
    JSAtom *textAtom;

    ts->flags &= ~TSF_XMLTAGMODE;
    for (;;) {
        ts->flags |= TSF_XMLTEXTMODE;
        tt = js_GetToken(cx, ts);
        ts->flags &= ~TSF_XMLTEXTMODE;
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);

        JS_ASSERT(tt == TOK_XMLSPACE || tt == TOK_XMLTEXT);
        textAtom = CURRENT_TOKEN(ts).t_atom;
        if (textAtom) {
            
            pn2 = XMLAtomNode(cx, ts, tc);
            if (!pn2)
                return JS_FALSE;
            pn->pn_pos.end = pn2->pn_pos.end;
            pn->append(pn2);
        }

        ts->flags |= TSF_OPERAND;
        tt = js_GetToken(cx, ts);
        ts->flags &= ~TSF_OPERAND;
        XML_CHECK_FOR_ERROR_AND_EOF(tt, JS_FALSE);
        if (tt == TOK_XMLETAGO)
            break;

        if (tt == TOK_LC) {
            pn2 = XMLExpr(cx, ts, JS_FALSE, tc);
            pn->pn_xflags |= PNX_CANTFOLD;
        } else if (tt == TOK_XMLSTAGO) {
            pn2 = XMLElementOrList(cx, ts, tc, JS_FALSE);
            if (pn2) {
                pn2->pn_xflags &= ~PNX_XMLROOT;
                pn->pn_xflags |= pn2->pn_xflags;
            }
        } else {
            JS_ASSERT(tt == TOK_XMLCDATA || tt == TOK_XMLCOMMENT ||
                      tt == TOK_XMLPI);
            pn2 = XMLAtomNode(cx, ts, tc);
        }
        if (!pn2)
            return JS_FALSE;
        pn->pn_pos.end = pn2->pn_pos.end;
        pn->append(pn2);
    }

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_XMLETAGO);
    ts->flags |= TSF_XMLTAGMODE;
    return JS_TRUE;
}




static JSParseNode *
XMLElementOrList(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
                 JSBool allowList)
{
    JSParseNode *pn, *pn2, *list;
    JSTokenType tt;
    JSAtom *startAtom, *endAtom;

    JS_CHECK_RECURSION(cx, return NULL);

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_XMLSTAGO);
    pn = NewParseNode(PN_LIST, tc);
    if (!pn)
        return NULL;

    ts->flags |= TSF_XMLTAGMODE;
    tt = js_GetToken(cx, ts);
    if (tt == TOK_ERROR)
        return NULL;

    if (tt == TOK_XMLNAME || tt == TOK_LC) {
        


        pn2 = XMLTagContent(cx, ts, tc, TOK_XMLSTAGO, &startAtom);
        if (!pn2)
            return NULL;
        js_MatchToken(cx, ts, TOK_XMLSPACE);

        tt = js_GetToken(cx, ts);
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
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            pn2->pn_pos.end = CURRENT_TOKEN(ts).pos.end;

            
            if (pn2->pn_type != TOK_XMLSTAGO) {
                pn->initList(pn2);
                if (!XML_FOLDABLE(pn2))
                    pn->pn_xflags |= PNX_CANTFOLD;
                pn2 = pn;
                pn = NewParseNode(PN_LIST, tc);
                if (!pn)
                    return NULL;
            }

            
            pn->pn_type = TOK_XMLELEM;
            pn->pn_pos.begin = pn2->pn_pos.begin;
            pn->initList(pn2);
            if (!XML_FOLDABLE(pn2))
                pn->pn_xflags |= PNX_CANTFOLD;
            pn->pn_xflags |= PNX_XMLROOT;

            
            if (!XMLElementContent(cx, ts, pn, tc))
                return NULL;

            tt = js_GetToken(cx, ts);
            XML_CHECK_FOR_ERROR_AND_EOF(tt, NULL);
            if (tt != TOK_XMLNAME && tt != TOK_LC) {
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }

            
            pn2 = XMLTagContent(cx, ts, tc, TOK_XMLETAGO, &endAtom);
            if (!pn2)
                return NULL;
            if (pn2->pn_type == TOK_XMLETAGO) {
                
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_XML_TAG_SYNTAX);
                return NULL;
            }
            if (endAtom && startAtom && endAtom != startAtom) {
                JSString *str = ATOM_TO_STRING(startAtom);

                
                js_ReportCompileErrorNumber(cx, ts, pn2,
                                            JSREPORT_UC | JSREPORT_ERROR,
                                            JSMSG_XML_TAG_NAME_MISMATCH,
                                            str->chars());
                return NULL;
            }

            
            JS_ASSERT(pn2->pn_type == TOK_XMLNAME || pn2->pn_type == TOK_LC);
            list = NewParseNode(PN_LIST, tc);
            if (!list)
                return NULL;
            list->pn_type = TOK_XMLETAGO;
            list->initList(pn2);
            pn->append(list);
            if (!XML_FOLDABLE(pn2)) {
                list->pn_xflags |= PNX_CANTFOLD;
                pn->pn_xflags |= PNX_CANTFOLD;
            }

            js_MatchToken(cx, ts, TOK_XMLSPACE);
            MUST_MATCH_TOKEN(TOK_XMLTAGC, JSMSG_BAD_XML_TAG_SYNTAX);
        }

        
        pn->pn_op = JSOP_TOXML;
    } else if (allowList && tt == TOK_XMLTAGC) {
        
        pn->pn_type = TOK_XMLLIST;
        pn->pn_op = JSOP_TOXMLLIST;
        pn->makeEmpty();
        pn->pn_xflags |= PNX_XMLROOT;
        if (!XMLElementContent(cx, ts, pn, tc))
            return NULL;

        MUST_MATCH_TOKEN(TOK_XMLTAGC, JSMSG_BAD_XML_LIST_SYNTAX);
    } else {
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_BAD_XML_NAME_SYNTAX);
        return NULL;
    }

    pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
    ts->flags &= ~TSF_XMLTAGMODE;
    return pn;
}

static JSParseNode *
XMLElementOrListRoot(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
                     JSBool allowList)
{
    uint32 oldopts;
    JSParseNode *pn;

    





    oldopts = JS_SetOptions(cx, cx->options | JSOPTION_XML);
    pn = XMLElementOrList(cx, ts, tc, allowList);
    JS_SetOptions(cx, oldopts);
    return pn;
}

JSParseNode *
JSCompiler::parseXMLText(JSObject *chain, bool allowList)
{
    




    JSTreeContext tc(this);
    tc.scopeChain = chain;

    
    TS(this)->flags |= TSF_OPERAND | TSF_XMLONLYMODE;
    JSTokenType tt = js_GetToken(context, TS(this));
    TS(this)->flags &= ~TSF_OPERAND;

    JSParseNode *pn;
    if (tt != TOK_XMLSTAGO) {
        js_ReportCompileErrorNumber(context, TS(this), NULL, JSREPORT_ERROR,
                                    JSMSG_BAD_XML_MARKUP);
        pn = NULL;
    } else {
        pn = XMLElementOrListRoot(context, TS(this), &tc, allowList);
    }

    TS(this)->flags &= ~TSF_XMLONLYMODE;
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

static JSParseNode *
PrimaryExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
            JSTokenType tt, JSBool afterDot)
{
    JSParseNode *pn, *pn2, *pn3;
    JSOp op;

    JS_CHECK_RECURSION(cx, return NULL);

#if JS_HAS_GETTER_SETTER
    if (tt == TOK_NAME) {
        tt = CheckGetterOrSetter(cx, ts, TOK_FUNCTION);
        if (tt == TOK_ERROR)
            return NULL;
    }
#endif

    switch (tt) {
      case TOK_FUNCTION:
#if JS_HAS_XML_SUPPORT
        ts->flags |= TSF_KEYWORD_IS_NAME;
        if (js_MatchToken(cx, ts, TOK_DBLCOLON)) {
            ts->flags &= ~TSF_KEYWORD_IS_NAME;
            pn2 = NewParseNode(PN_NULLARY, tc);
            if (!pn2)
                return NULL;
            pn2->pn_type = TOK_FUNCTION;
            pn = QualifiedSuffix(cx, ts, pn2, tc);
            if (!pn)
                return NULL;
            break;
        }
        ts->flags &= ~TSF_KEYWORD_IS_NAME;
#endif
        pn = FunctionExpr(cx, ts, tc);
        if (!pn)
            return NULL;
        break;

      case TOK_LB:
      {
        JSBool matched;
        jsuint index;

        pn = NewParseNode(PN_LIST, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_RB;
        pn->pn_op = JSOP_NEWINIT;
        pn->makeEmpty();

#if JS_HAS_GENERATORS
        pn->pn_blockid = tc->blockidGen;
#endif

        ts->flags |= TSF_OPERAND;
        matched = js_MatchToken(cx, ts, TOK_RB);
        ts->flags &= ~TSF_OPERAND;
        if (!matched) {
            for (index = 0; ; index++) {
                if (index == JS_ARGS_LENGTH_MAX) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_ARRAY_INIT_TOO_BIG);
                    return NULL;
                }

                ts->flags |= TSF_OPERAND;
                tt = js_PeekToken(cx, ts);
                ts->flags &= ~TSF_OPERAND;
                if (tt == TOK_RB) {
                    pn->pn_xflags |= PNX_ENDCOMMA;
                    break;
                }

                if (tt == TOK_COMMA) {
                    
                    js_MatchToken(cx, ts, TOK_COMMA);
                    pn2 = NewParseNode(PN_NULLARY, tc);
                    pn->pn_xflags |= PNX_HOLEY;
                } else {
                    pn2 = AssignExpr(cx, ts, tc);
                }
                if (!pn2)
                    return NULL;
                pn->append(pn2);

                if (tt != TOK_COMMA) {
                    
                    if (!js_MatchToken(cx, ts, TOK_COMMA))
                        break;
                }
            }

#if JS_HAS_GENERATORS
            









































            if (index == 0 &&
                pn->pn_count != 0 &&
                js_MatchToken(cx, ts, TOK_FOR)) {
                JSParseNode *pnexp, *pntop;

                
                pn->pn_type = TOK_ARRAYCOMP;

                




                pnexp = pn->last();
                JS_ASSERT(pn->pn_count == 1 || pn->pn_count == 2);
                pn->pn_tail = (--pn->pn_count == 1)
                              ? &pn->pn_head->pn_next
                              : &pn->pn_head;
                *pn->pn_tail = NULL;

                pntop = ComprehensionTail(pnexp, pn->pn_blockid, tc,
                                          TOK_ARRAYPUSH, JSOP_ARRAYPUSH);
                if (!pntop)
                    return NULL;
                pn->append(pntop);
            }
#endif 

            MUST_MATCH_TOKEN(TOK_RB, JSMSG_BRACKET_AFTER_LIST);
        }
        pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
        return pn;
      }

      case TOK_LC:
      {
        JSBool afterComma;
        JSParseNode *pnval;

        






        JSAutoAtomList seen(tc->compiler);

        pn = NewParseNode(PN_LIST, tc);
        if (!pn)
            return NULL;
        pn->pn_type = TOK_RC;
        pn->pn_op = JSOP_NEWINIT;
        pn->makeEmpty();

        afterComma = JS_FALSE;
        for (;;) {
            JSAtom *atom;
            ts->flags |= TSF_KEYWORD_IS_NAME;
            tt = js_GetToken(cx, ts);
            ts->flags &= ~TSF_KEYWORD_IS_NAME;
            switch (tt) {
              case TOK_NUMBER:
                pn3 = NewParseNode(PN_NULLARY, tc);
                if (!pn3)
                    return NULL;
                pn3->pn_dval = CURRENT_TOKEN(ts).t_dval;
                if (tc->needStrictChecks())
                    atom = js_AtomizeDouble(cx, pn3->pn_dval);
                else
                    atom = NULL; 
                break;
              case TOK_NAME:
#if JS_HAS_GETTER_SETTER
                {
                    atom = CURRENT_TOKEN(ts).t_atom;
                    if (atom == cx->runtime->atomState.getAtom)
                        op = JSOP_GETTER;
                    else if (atom == cx->runtime->atomState.setAtom)
                        op = JSOP_SETTER;
                    else
                        goto property_name;

                    ts->flags |= TSF_KEYWORD_IS_NAME;
                    tt = js_GetToken(cx, ts);
                    ts->flags &= ~TSF_KEYWORD_IS_NAME;
                    if (tt != TOK_NAME) {
                        js_UngetToken(ts);
                        goto property_name;
                    }
                    atom = CURRENT_TOKEN(ts).t_atom;
                    pn3 = NewNameNode(cx, atom, tc);
                    if (!pn3)
                        return NULL;

                    
                    CURRENT_TOKEN(ts).t_op = JSOP_NOP;
                    CURRENT_TOKEN(ts).type = TOK_FUNCTION;
                    pn2 = FunctionExpr(cx, ts, tc);
                    pn2 = NewBinary(TOK_COLON, op, pn3, pn2, tc);
                    goto skip;
                }
              property_name:
#endif
              case TOK_STRING:
                atom = CURRENT_TOKEN(ts).t_atom;
                pn3 = NewParseNode(PN_NULLARY, tc);
                if (!pn3)
                    return NULL;
                pn3->pn_atom = atom;
                break;
              case TOK_RC:
                goto end_obj_init;
              default:
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_PROP_ID);
                return NULL;
            }

            tt = js_GetToken(cx, ts);
#if JS_HAS_GETTER_SETTER
            if (tt == TOK_NAME) {
                tt = CheckGetterOrSetter(cx, ts, TOK_COLON);
                if (tt == TOK_ERROR)
                    return NULL;
            }
#endif

            if (tt != TOK_COLON) {
#if JS_HAS_DESTRUCTURING_SHORTHAND
                if (tt != TOK_COMMA && tt != TOK_RC) {
#endif
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_COLON_AFTER_ID);
                    return NULL;
#if JS_HAS_DESTRUCTURING_SHORTHAND
                }

                



                js_UngetToken(ts);
                pn->pn_xflags |= PNX_DESTRUCT;
                pnval = pn3;
                if (pnval->pn_type == TOK_NAME) {
                    pnval->pn_arity = PN_NAME;
                    InitNameNodeCommon(pnval, tc);
                }
                op = JSOP_NOP;
#endif
            } else {
                op = CURRENT_TOKEN(ts).t_op;
                pnval = AssignExpr(cx, ts, tc);
            }

            pn2 = NewBinary(TOK_COLON, op, pn3, pnval, tc);
#if JS_HAS_GETTER_SETTER
          skip:
#endif
            if (!pn2)
                return NULL;
            pn->append(pn2);

            



 
            if (tc->needStrictChecks()) {
                unsigned attributesMask;
                if (op == JSOP_NOP)
                    attributesMask = JSPROP_GETTER | JSPROP_SETTER;
                else if (op == JSOP_GETTER)
                    attributesMask = JSPROP_GETTER;
                else if (op == JSOP_SETTER)
                    attributesMask = JSPROP_SETTER;
                else {
                    JS_NOT_REACHED("bad opcode in object initializer");
                    attributesMask = 0;
                }

                JSAtomListElement *ale = seen.lookup(atom);
                if (ale) {
                    if (ALE_INDEX(ale) & attributesMask) {
                        const char *name = js_AtomToPrintableString(cx, atom);
                        if (!name ||
                            !js_ReportStrictModeError(cx, ts, tc, NULL,
                                                      JSMSG_DUPLICATE_PROPERTY, name)) {
                            return NULL;
                        }
                    }
                    ALE_SET_INDEX(ale, attributesMask | ALE_INDEX(ale));
                } else {
                    ale = seen.add(tc->compiler, atom);
                    if (!ale)
                        return NULL;
                    ALE_SET_INDEX(ale, attributesMask);
                }
            }                    

            tt = js_GetToken(cx, ts);
            if (tt == TOK_RC)
                goto end_obj_init;
            if (tt != TOK_COMMA) {
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_CURLY_AFTER_LIST);
                return NULL;
            }
            afterComma = JS_TRUE;
        }

      end_obj_init:
        pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
        return pn;
      }

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        pn = LetBlock(cx, ts, tc, JS_FALSE);
        if (!pn)
            return NULL;
        break;
#endif

#if JS_HAS_SHARP_VARS
      case TOK_DEFSHARP:
        pn = NewParseNode(PN_UNARY, tc);
        if (!pn)
            return NULL;
        pn->pn_num = (jsint) CURRENT_TOKEN(ts).t_dval;
        ts->flags |= TSF_OPERAND;
        tt = js_GetToken(cx, ts);
        ts->flags &= ~TSF_OPERAND;
        if (tt == TOK_USESHARP || tt == TOK_DEFSHARP ||
#if JS_HAS_XML_SUPPORT
            tt == TOK_STAR || tt == TOK_AT ||
            tt == TOK_XMLSTAGO  ||
#endif
            tt == TOK_STRING || tt == TOK_NUMBER || tt == TOK_PRIMARY) {
            js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                        JSMSG_BAD_SHARP_VAR_DEF);
            return NULL;
        }
        pn->pn_kid = PrimaryExpr(cx, ts, tc, tt, JS_FALSE);
        if (!pn->pn_kid)
            return NULL;
        if (!tc->ensureSharpSlots())
            return NULL;
        break;

      case TOK_USESHARP:
        
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        if (!tc->ensureSharpSlots())
            return NULL;
        pn->pn_num = (jsint) CURRENT_TOKEN(ts).t_dval;
        break;
#endif 

      case TOK_LP:
      {
        JSBool genexp;

        pn = ParenExpr(cx, ts, tc, NULL, &genexp);
        if (!pn)
            return NULL;
        pn->pn_parens = true;
        if (!genexp)
            MUST_MATCH_TOKEN(TOK_RP, JSMSG_PAREN_IN_PAREN);
        break;
      }

#if JS_HAS_XML_SUPPORT
      case TOK_STAR:
        pn = QualifiedIdentifier(cx, ts, tc);
        if (!pn)
            return NULL;
        break;

      case TOK_AT:
        pn = AttributeIdentifier(cx, ts, tc);
        if (!pn)
            return NULL;
        break;

      case TOK_XMLSTAGO:
        pn = XMLElementOrListRoot(cx, ts, tc, JS_TRUE);
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
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        pn->pn_atom = CURRENT_TOKEN(ts).t_atom;
#if JS_HAS_XML_SUPPORT
        if (tt == TOK_XMLPI)
            pn->pn_atom2 = CURRENT_TOKEN(ts).t_atom2;
        else
#endif
            pn->pn_op = CURRENT_TOKEN(ts).t_op;
        break;

      case TOK_NAME:
        pn = NewNameNode(cx, CURRENT_TOKEN(ts).t_atom, tc);
        if (!pn)
            return NULL;
        JS_ASSERT(CURRENT_TOKEN(ts).t_op == JSOP_NAME);
        pn->pn_op = JSOP_NAME;

        if ((tc->flags & (TCF_IN_FUNCTION | TCF_FUN_PARAM_ARGUMENTS)) == TCF_IN_FUNCTION &&
            pn->pn_atom == cx->runtime->atomState.argumentsAtom) {
            






            NoteArgumentsUse(tc);

            



            if (!afterDot && !(tc->flags & TCF_DECL_DESTRUCTURING) && !tc->inStatement(STMT_WITH)) {
                pn->pn_op = JSOP_ARGUMENTS;
                pn->pn_dflags |= PND_BOUND;
            }
        } else if ((!afterDot
#if JS_HAS_XML_SUPPORT
                    || js_PeekToken(cx, ts) == TOK_DBLCOLON
#endif
                   ) && !(tc->flags & TCF_DECL_DESTRUCTURING)) {
            JSStmtInfo *stmt = js_LexicalLookup(tc, pn->pn_atom, NULL);
            if (!stmt || stmt->type != STMT_WITH) {
                JSDefinition *dn;

                JSAtomListElement *ale = tc->decls.lookup(pn->pn_atom);
                if (ale) {
                    dn = ALE_DEFN(ale);
#if JS_HAS_BLOCK_SCOPE
                    





                    while (dn->isLet() && !BlockIdInScope(dn->pn_blockid, tc)) {
                        do {
                            ale = ALE_NEXT(ale);
                        } while (ale && ALE_ATOM(ale) != pn->pn_atom);
                        if (!ale)
                            break;
                        dn = ALE_DEFN(ale);
                    }
#endif
                }

                if (ale) {
                    dn = ALE_DEFN(ale);
                } else {
                    ale = tc->lexdeps.lookup(pn->pn_atom);
                    if (ale) {
                        dn = ALE_DEFN(ale);
                    } else {
                        








                        ale = MakePlaceholder(pn, tc);
                        if (!ale)
                            return NULL;
                        dn = ALE_DEFN(ale);

                        










                        JS_ASSERT(PN_TYPE(dn) == TOK_NAME);
                        JS_ASSERT(dn->pn_op == JSOP_NOP);
                        if (js_PeekToken(cx, ts) != TOK_LP)
                            dn->pn_dflags |= PND_FUNARG;
                    }
                }

                JS_ASSERT(dn->pn_defn);
                LinkUseToDef(pn, dn, tc);

                
                if (js_PeekToken(cx, ts) != TOK_LP)
                    dn->pn_dflags |= PND_FUNARG;

                pn->pn_dflags |= (dn->pn_dflags & PND_FUNARG);
            }
        }

#if JS_HAS_XML_SUPPORT
        if (js_MatchToken(cx, ts, TOK_DBLCOLON)) {
            if (afterDot) {
                JSString *str;

                




                str = ATOM_TO_STRING(pn->pn_atom);
                tt = js_CheckKeyword(str->chars(), str->length());
                if (tt == TOK_FUNCTION) {
                    pn->pn_arity = PN_NULLARY;
                    pn->pn_type = TOK_FUNCTION;
                } else if (tt != TOK_EOF) {
                    js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                                JSMSG_KEYWORD_NOT_NS);
                    return NULL;
                }
            }
            pn = QualifiedSuffix(cx, ts, pn, tc);
            if (!pn)
                return NULL;
        }
#endif
        break;

      case TOK_REGEXP:
      {
        JSObject *obj;

        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;

        obj = js_NewRegExpObject(cx, ts,
                                 ts->tokenbuf.begin(),
                                 ts->tokenbuf.length(),
                                 CURRENT_TOKEN(ts).t_reflags);
        if (!obj)
            return NULL;
        if (!(tc->flags & TCF_COMPILE_N_GO)) {
            STOBJ_CLEAR_PARENT(obj);
            STOBJ_CLEAR_PROTO(obj);
        }

        pn->pn_objbox = tc->compiler->newObjectBox(obj);
        if (!pn->pn_objbox)
            return NULL;

        pn->pn_op = JSOP_REGEXP;
        break;
      }

      case TOK_NUMBER:
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        pn->pn_op = JSOP_DOUBLE;
        pn->pn_dval = CURRENT_TOKEN(ts).t_dval;
        break;

      case TOK_PRIMARY:
        pn = NewParseNode(PN_NULLARY, tc);
        if (!pn)
            return NULL;
        pn->pn_op = CURRENT_TOKEN(ts).t_op;
        break;

      case TOK_ERROR:
        
        return NULL;

      default:
        js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                    JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    return pn;
}

static JSParseNode *
ParenExpr(JSContext *cx, JSTokenStream *ts, JSTreeContext *tc,
          JSParseNode *pn1, JSBool *genexp)
{
    JSTokenPtr begin;
    JSParseNode *pn;

    JS_ASSERT(CURRENT_TOKEN(ts).type == TOK_LP);
    begin = CURRENT_TOKEN(ts).pos.begin;

    if (genexp)
        *genexp = JS_FALSE;
    pn = BracketedExpr(cx, ts, tc);
    if (!pn)
        return NULL;

#if JS_HAS_GENERATOR_EXPRS
    if (js_MatchToken(cx, ts, TOK_FOR)) {
        if (pn->pn_type == TOK_YIELD && !pn->pn_parens) {
            js_ReportCompileErrorNumber(cx, ts, pn, JSREPORT_ERROR,
                                        JSMSG_BAD_GENERATOR_SYNTAX,
                                        js_yield_str);
            return NULL;
        }
        if (pn->pn_type == TOK_COMMA && !pn->pn_parens) {
            js_ReportCompileErrorNumber(cx, ts, pn->last(), JSREPORT_ERROR,
                                        JSMSG_BAD_GENERATOR_SYNTAX,
                                        js_generator_str);
            return NULL;
        }
        if (!pn1) {
            pn1 = NewParseNode(PN_UNARY, tc);
            if (!pn1)
                return NULL;
        }
        pn = GeneratorExpr(pn1, pn, tc);
        if (!pn)
            return NULL;
        pn->pn_pos.begin = begin;
        if (genexp) {
            if (js_GetToken(cx, ts) != TOK_RP) {
                js_ReportCompileErrorNumber(cx, ts, NULL, JSREPORT_ERROR,
                                            JSMSG_BAD_GENERATOR_SYNTAX,
                                            js_generator_str);
                return NULL;
            }
            pn->pn_pos.end = CURRENT_TOKEN(ts).pos.end;
            *genexp = JS_TRUE;
        }
    }
#endif 

    return pn;
}





static JSBool
FoldType(JSContext *cx, JSParseNode *pn, JSTokenType type)
{
    if (PN_TYPE(pn) != type) {
        switch (type) {
          case TOK_NUMBER:
            if (pn->pn_type == TOK_STRING) {
                jsdouble d;
                if (!JS_ValueToNumber(cx, ATOM_KEY(pn->pn_atom), &d))
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
                pn->pn_atom = js_AtomizeString(cx, str, 0);
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
    JSTokenType tt;
    JSParseNode **pnp, *pn1, *pn2;
    JSString *accum, *str;
    uint32 i, j;
    JSTempValueRooter tvr;

    JS_ASSERT(pn->pn_arity == PN_LIST);
    tt = PN_TYPE(pn);
    pnp = &pn->pn_head;
    pn1 = *pnp;
    accum = NULL;
    if ((pn->pn_xflags & PNX_CANTFOLD) == 0) {
        if (tt == TOK_XMLETAGO)
            accum = ATOM_TO_STRING(cx->runtime->atomState.etagoAtom);
        else if (tt == TOK_XMLSTAGO || tt == TOK_XMLPTAGC)
            accum = ATOM_TO_STRING(cx->runtime->atomState.stagoAtom);
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
            str = ATOM_TO_STRING(pn2->pn_atom);
            break;

          case TOK_XMLCDATA:
            str = js_MakeXMLCDATAString(cx, ATOM_TO_STRING(pn2->pn_atom));
            if (!str)
                return JS_FALSE;
            break;

          case TOK_XMLCOMMENT:
            str = js_MakeXMLCommentString(cx, ATOM_TO_STRING(pn2->pn_atom));
            if (!str)
                return JS_FALSE;
            break;

          case TOK_XMLPI:
            str = js_MakeXMLPIString(cx, ATOM_TO_STRING(pn2->pn_atom),
                                         ATOM_TO_STRING(pn2->pn_atom2));
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
                    js_FileEscapedString(stdout, accum, 0);
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
                pn1->pn_atom = js_AtomizeString(cx, accum, 0);
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
            JS_PUSH_TEMP_ROOT_STRING(cx, accum, &tvr);
            str = ((tt == TOK_XMLSTAGO || tt == TOK_XMLPTAGC) && i != 0)
                  ? js_AddAttributePart(cx, i & 1, accum, str)
                  : js_ConcatStrings(cx, accum, str);
            JS_POP_TEMP_ROOT(cx, &tvr);
            if (!str)
                return JS_FALSE;
#ifdef DEBUG_brendanXXX
            printf("2: %d, %d => ", i, j);
            js_FileEscapedString(stdout, str, 0);
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
                str = ATOM_TO_STRING(cx->runtime->atomState.ptagcAtom);
            else if (tt == TOK_XMLSTAGO || tt == TOK_XMLETAGO)
                str = ATOM_TO_STRING(cx->runtime->atomState.tagcAtom);
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
        pn1->pn_atom = js_AtomizeString(cx, accum, 0);
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
        return ATOM_TO_STRING(pn->pn_atom)->length() != 0;

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
      case JSOP_THIS:
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

        
        for (pn1 = pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
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
            if (ATOM_TO_STRING(pn1->pn_atom)->length() == 0)
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
            size_t length, length2;
            jschar *chars;
            JSString *str, *str2;

            




            JS_ASSERT(pn->pn_count > 2);
            if (pn->pn_xflags & PNX_CANTFOLD)
                return JS_TRUE;
            if (pn->pn_xflags != PNX_STRCAT)
                goto do_binary_op;

            
            length = 0;
            for (pn2 = pn1; pn2; pn2 = pn2->pn_next) {
                if (!FoldType(cx, pn2, TOK_STRING))
                    return JS_FALSE;
                
                if (pn2->pn_type != TOK_STRING)
                    return JS_TRUE;
                length += ATOM_TO_STRING(pn2->pn_atom)->flatLength();
            }

            
            chars = (jschar *) cx->malloc((length + 1) * sizeof(jschar));
            if (!chars)
                return JS_FALSE;
            str = js_NewString(cx, chars, length);
            if (!str) {
                cx->free(chars);
                return JS_FALSE;
            }

            
            for (pn2 = pn1; pn2; pn2 = RecycleTree(pn2, tc)) {
                str2 = ATOM_TO_STRING(pn2->pn_atom);
                length2 = str2->flatLength();
                js_strncpy(chars, str2->flatChars(), length2);
                chars += length2;
            }
            *chars = 0;

            
            pn->pn_atom = js_AtomizeString(cx, str, 0);
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
            left = ATOM_TO_STRING(pn1->pn_atom);
            right = ATOM_TO_STRING(pn2->pn_atom);
            str = js_ConcatStrings(cx, left, right);
            if (!str)
                return JS_FALSE;
            pn->pn_atom = js_AtomizeString(cx, str, 0);
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
            jsval v;
            JSObjectBox *xmlbox;

            v = ATOM_KEY(pn1->pn_atom);
            if (!js_ToAttributeName(cx, &v))
                return JS_FALSE;
            JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));

            xmlbox = tc->compiler->newObjectBox(JSVAL_TO_OBJECT(v));
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
            switch (pn->pn_arity) {
              case PN_LIST:
                pn2 = pn->pn_head;
                do {
                    pn3 = pn2->pn_next;
                    RecycleTree(pn2, tc);
                } while ((pn2 = pn3) != NULL);
                break;
              case PN_FUNC:
                RecycleFuncNameKids(pn, tc);
                break;
              case PN_NULLARY:
                break;
              default:
                JS_NOT_REACHED("unhandled arity");
            }
            pn->pn_type = TOK_PRIMARY;
            pn->pn_op = cond ? JSOP_TRUE : JSOP_FALSE;
            pn->pn_arity = PN_NULLARY;
        }
    }

    return JS_TRUE;
}
