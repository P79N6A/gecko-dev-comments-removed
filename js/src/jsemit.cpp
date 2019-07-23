










































#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <new>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsarena.h" 
#include "jsutil.h" 
#include "jsbit.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsemit.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsregexp.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsautooplen.h"
#include "jsstaticcheck.h"


#define BYTECODE_CHUNK  256     /* code allocation increment */
#define SRCNOTE_CHUNK   64      /* initial srcnote allocation increment */
#define TRYNOTE_CHUNK   64      /* trynote allocation increment */


#define BYTECODE_SIZE(n)        ((n) * sizeof(jsbytecode))
#define SRCNOTE_SIZE(n)         ((n) * sizeof(jssrcnote))
#define TRYNOTE_SIZE(n)         ((n) * sizeof(JSTryNote))

static JSBool
NewTryNote(JSContext *cx, JSCodeGenerator *cg, JSTryNoteKind kind,
           uintN stackDepth, size_t start, size_t end);

JSCodeGenerator::JSCodeGenerator(JSCompiler *jsc,
                                 JSArenaPool *cpool, JSArenaPool *npool,
                                 uintN lineno)
  : JSTreeContext(jsc),
    codePool(cpool), notePool(npool),
    codeMark(JS_ARENA_MARK(cpool)), noteMark(JS_ARENA_MARK(npool)),
    stackDepth(0), maxStackDepth(0),
    ntrynotes(0), lastTryNode(NULL),
    spanDeps(NULL), jumpTargets(NULL), jtFreeList(NULL),
    numSpanDeps(0), numJumpTargets(0), spanDepTodo(0),
    arrayCompDepth(0),
    emitLevel(0)
{
    flags = TCF_COMPILING;
    memset(&prolog, 0, sizeof prolog);
    memset(&main, 0, sizeof main);
    current = &main;
    firstLine = prolog.currentLine = main.currentLine = lineno;
    prolog.noteMask = main.noteMask = SRCNOTE_CHUNK - 1;
    memset(&upvarMap, 0, sizeof upvarMap);
}

JSCodeGenerator::~JSCodeGenerator()
{
    JS_ARENA_RELEASE(codePool, codeMark);
    JS_ARENA_RELEASE(notePool, noteMark);

    
    if (spanDeps)
        compiler->context->free(spanDeps);

    if (upvarMap.vector)
        compiler->context->free(upvarMap.vector);
}

static ptrdiff_t
EmitCheck(JSContext *cx, JSCodeGenerator *cg, JSOp op, ptrdiff_t delta)
{
    jsbytecode *base, *limit, *next;
    ptrdiff_t offset, length;
    size_t incr, size;

    base = CG_BASE(cg);
    next = CG_NEXT(cg);
    limit = CG_LIMIT(cg);
    offset = next - base;
    if (next + delta > limit) {
        length = offset + delta;
        length = (length <= BYTECODE_CHUNK)
                 ? BYTECODE_CHUNK
                 : JS_BIT(JS_CeilingLog2(length));
        incr = BYTECODE_SIZE(length);
        if (!base) {
            JS_ARENA_ALLOCATE_CAST(base, jsbytecode *, cg->codePool, incr);
        } else {
            size = BYTECODE_SIZE(limit - base);
            incr -= size;
            JS_ARENA_GROW_CAST(base, jsbytecode *, cg->codePool, size, incr);
        }
        if (!base) {
            js_ReportOutOfScriptQuota(cx);
            return -1;
        }
        CG_BASE(cg) = base;
        CG_LIMIT(cg) = base + length;
        CG_NEXT(cg) = base + offset;
    }
    return offset;
}

static void
UpdateDepth(JSContext *cx, JSCodeGenerator *cg, ptrdiff_t target)
{
    jsbytecode *pc;
    JSOp op;
    const JSCodeSpec *cs;
    uintN extra, depth, nuses;
    intN ndefs;

    pc = CG_CODE(cg, target);
    op = (JSOp) *pc;
    cs = &js_CodeSpec[op];
#ifdef JS_TRACER
    extern uint8 js_opcode2extra[];
    extra = js_opcode2extra[op];
#else
    extra = 0;
#endif
    if ((cs->format & JOF_TMPSLOT_MASK) || extra) {
        depth = (uintN) cg->stackDepth +
                ((cs->format & JOF_TMPSLOT_MASK) >> JOF_TMPSLOT_SHIFT) +
                extra;
        if (depth > cg->maxStackDepth)
            cg->maxStackDepth = depth;
    }

    nuses = js_GetStackUses(cs, op, pc);
    cg->stackDepth -= nuses;
    JS_ASSERT(cg->stackDepth >= 0);
    if (cg->stackDepth < 0) {
        char numBuf[12];
        JSTokenStream *ts;

        JS_snprintf(numBuf, sizeof numBuf, "%d", target);
        ts = &cg->compiler->tokenStream;
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING,
                                     js_GetErrorMessage, NULL,
                                     JSMSG_STACK_UNDERFLOW,
                                     ts->filename ? ts->filename : "stdin",
                                     numBuf);
    }
    ndefs = cs->ndefs;
    if (ndefs < 0) {
        JSObject *blockObj;

        
        JS_ASSERT(op == JSOP_ENTERBLOCK);
        JS_ASSERT(nuses == 0);
        blockObj = cg->objectList.lastbox->object;
        JS_ASSERT(STOBJ_GET_CLASS(blockObj) == &js_BlockClass);
        JS_ASSERT(JSVAL_IS_VOID(blockObj->fslots[JSSLOT_BLOCK_DEPTH]));

        OBJ_SET_BLOCK_DEPTH(cx, blockObj, cg->stackDepth);
        ndefs = OBJ_BLOCK_COUNT(cx, blockObj);
    }
    cg->stackDepth += ndefs;
    if ((uintN)cg->stackDepth > cg->maxStackDepth)
        cg->maxStackDepth = cg->stackDepth;
}

ptrdiff_t
js_Emit1(JSContext *cx, JSCodeGenerator *cg, JSOp op)
{
    ptrdiff_t offset = EmitCheck(cx, cg, op, 1);

    if (offset >= 0) {
        *CG_NEXT(cg)++ = (jsbytecode)op;
        UpdateDepth(cx, cg, offset);
    }
    return offset;
}

ptrdiff_t
js_Emit2(JSContext *cx, JSCodeGenerator *cg, JSOp op, jsbytecode op1)
{
    ptrdiff_t offset = EmitCheck(cx, cg, op, 2);

    if (offset >= 0) {
        jsbytecode *next = CG_NEXT(cg);
        next[0] = (jsbytecode)op;
        next[1] = op1;
        CG_NEXT(cg) = next + 2;
        UpdateDepth(cx, cg, offset);
    }
    return offset;
}

ptrdiff_t
js_Emit3(JSContext *cx, JSCodeGenerator *cg, JSOp op, jsbytecode op1,
         jsbytecode op2)
{
    ptrdiff_t offset = EmitCheck(cx, cg, op, 3);

    if (offset >= 0) {
        jsbytecode *next = CG_NEXT(cg);
        next[0] = (jsbytecode)op;
        next[1] = op1;
        next[2] = op2;
        CG_NEXT(cg) = next + 3;
        UpdateDepth(cx, cg, offset);
    }
    return offset;
}

ptrdiff_t
js_EmitN(JSContext *cx, JSCodeGenerator *cg, JSOp op, size_t extra)
{
    ptrdiff_t length = 1 + (ptrdiff_t)extra;
    ptrdiff_t offset = EmitCheck(cx, cg, op, length);

    if (offset >= 0) {
        jsbytecode *next = CG_NEXT(cg);
        *next = (jsbytecode)op;
        memset(next + 1, 0, BYTECODE_SIZE(extra));
        CG_NEXT(cg) = next + length;

        



        if (js_CodeSpec[op].nuses >= 0)
            UpdateDepth(cx, cg, offset);
    }
    return offset;
}


const char js_with_statement_str[] = "with statement";
const char js_finally_block_str[]  = "finally block";
const char js_script_str[]         = "script";

static const char *statementName[] = {
    "label statement",       
    "if statement",          
    "else statement",        
    "destructuring body",    
    "switch statement",      
    "block",                 
    js_with_statement_str,   
    "catch block",           
    "try block",             
    js_finally_block_str,    
    js_finally_block_str,    
    "do loop",               
    "for loop",              
    "for/in loop",           
    "while loop",            
};

JS_STATIC_ASSERT(JS_ARRAY_LENGTH(statementName) == STMT_LIMIT);

static const char *
StatementName(JSCodeGenerator *cg)
{
    if (!cg->topStmt)
        return js_script_str;
    return statementName[cg->topStmt->type];
}

static void
ReportStatementTooLarge(JSContext *cx, JSCodeGenerator *cg)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET,
                         StatementName(cg));
}



































































static int
BalanceJumpTargets(JSJumpTarget **jtp)
{
    JSJumpTarget *jt, *jt2, *root;
    int dir, otherDir, heightChanged;
    JSBool doubleRotate;

    jt = *jtp;
    JS_ASSERT(jt->balance != 0);

    if (jt->balance < -1) {
        dir = JT_RIGHT;
        doubleRotate = (jt->kids[JT_LEFT]->balance > 0);
    } else if (jt->balance > 1) {
        dir = JT_LEFT;
        doubleRotate = (jt->kids[JT_RIGHT]->balance < 0);
    } else {
        return 0;
    }

    otherDir = JT_OTHER_DIR(dir);
    if (doubleRotate) {
        jt2 = jt->kids[otherDir];
        *jtp = root = jt2->kids[dir];

        jt->kids[otherDir] = root->kids[dir];
        root->kids[dir] = jt;

        jt2->kids[dir] = root->kids[otherDir];
        root->kids[otherDir] = jt2;

        heightChanged = 1;
        root->kids[JT_LEFT]->balance = -JS_MAX(root->balance, 0);
        root->kids[JT_RIGHT]->balance = -JS_MIN(root->balance, 0);
        root->balance = 0;
    } else {
        *jtp = root = jt->kids[otherDir];
        jt->kids[otherDir] = root->kids[dir];
        root->kids[dir] = jt;

        heightChanged = (root->balance != 0);
        jt->balance = -((dir == JT_LEFT) ? --root->balance : ++root->balance);
    }

    return heightChanged;
}

typedef struct AddJumpTargetArgs {
    JSContext           *cx;
    JSCodeGenerator     *cg;
    ptrdiff_t           offset;
    JSJumpTarget        *node;
} AddJumpTargetArgs;

static int
AddJumpTarget(AddJumpTargetArgs *args, JSJumpTarget **jtp)
{
    JSJumpTarget *jt;
    int balanceDelta;

    jt = *jtp;
    if (!jt) {
        JSCodeGenerator *cg = args->cg;

        jt = cg->jtFreeList;
        if (jt) {
            cg->jtFreeList = jt->kids[JT_LEFT];
        } else {
            JS_ARENA_ALLOCATE_CAST(jt, JSJumpTarget *, &args->cx->tempPool,
                                   sizeof *jt);
            if (!jt) {
                js_ReportOutOfScriptQuota(args->cx);
                return 0;
            }
        }
        jt->offset = args->offset;
        jt->balance = 0;
        jt->kids[JT_LEFT] = jt->kids[JT_RIGHT] = NULL;
        cg->numJumpTargets++;
        args->node = jt;
        *jtp = jt;
        return 1;
    }

    if (jt->offset == args->offset) {
        args->node = jt;
        return 0;
    }

    if (args->offset < jt->offset)
        balanceDelta = -AddJumpTarget(args, &jt->kids[JT_LEFT]);
    else
        balanceDelta = AddJumpTarget(args, &jt->kids[JT_RIGHT]);
    if (!args->node)
        return 0;

    jt->balance += balanceDelta;
    return (balanceDelta && jt->balance)
           ? 1 - BalanceJumpTargets(jtp)
           : 0;
}

#ifdef DEBUG_brendan
static int AVLCheck(JSJumpTarget *jt)
{
    int lh, rh;

    if (!jt) return 0;
    JS_ASSERT(-1 <= jt->balance && jt->balance <= 1);
    lh = AVLCheck(jt->kids[JT_LEFT]);
    rh = AVLCheck(jt->kids[JT_RIGHT]);
    JS_ASSERT(jt->balance == rh - lh);
    return 1 + JS_MAX(lh, rh);
}
#endif

static JSBool
SetSpanDepTarget(JSContext *cx, JSCodeGenerator *cg, JSSpanDep *sd,
                 ptrdiff_t off)
{
    AddJumpTargetArgs args;

    if (off < JUMPX_OFFSET_MIN || JUMPX_OFFSET_MAX < off) {
        ReportStatementTooLarge(cx, cg);
        return JS_FALSE;
    }

    args.cx = cx;
    args.cg = cg;
    args.offset = sd->top + off;
    args.node = NULL;
    AddJumpTarget(&args, &cg->jumpTargets);
    if (!args.node)
        return JS_FALSE;

#ifdef DEBUG_brendan
    AVLCheck(cg->jumpTargets);
#endif

    SD_SET_TARGET(sd, args.node);
    return JS_TRUE;
}

#define SPANDEPS_MIN            256
#define SPANDEPS_SIZE(n)        ((n) * sizeof(JSSpanDep))
#define SPANDEPS_SIZE_MIN       SPANDEPS_SIZE(SPANDEPS_MIN)

static JSBool
AddSpanDep(JSContext *cx, JSCodeGenerator *cg, jsbytecode *pc, jsbytecode *pc2,
           ptrdiff_t off)
{
    uintN index;
    JSSpanDep *sdbase, *sd;
    size_t size;

    index = cg->numSpanDeps;
    if (index + 1 == 0) {
        ReportStatementTooLarge(cx, cg);
        return JS_FALSE;
    }

    if ((index & (index - 1)) == 0 &&
        (!(sdbase = cg->spanDeps) || index >= SPANDEPS_MIN)) {
        size = sdbase ? SPANDEPS_SIZE(index) : SPANDEPS_SIZE_MIN / 2;
        sdbase = (JSSpanDep *) cx->realloc(sdbase, size + size);
        if (!sdbase)
            return JS_FALSE;
        cg->spanDeps = sdbase;
    }

    cg->numSpanDeps = index + 1;
    sd = cg->spanDeps + index;
    sd->top = pc - CG_BASE(cg);
    sd->offset = sd->before = pc2 - CG_BASE(cg);

    if (js_CodeSpec[*pc].format & JOF_BACKPATCH) {
        
        if (off != 0) {
            JS_ASSERT(off >= 1 + JUMP_OFFSET_LEN);
            if (off > BPDELTA_MAX) {
                ReportStatementTooLarge(cx, cg);
                return JS_FALSE;
            }
        }
        SD_SET_BPDELTA(sd, off);
    } else if (off == 0) {
        
        SD_SET_TARGET(sd, 0);
    } else {
        
        if (!SetSpanDepTarget(cx, cg, sd, off))
            return JS_FALSE;
    }

    if (index > SPANDEP_INDEX_MAX)
        index = SPANDEP_INDEX_HUGE;
    SET_SPANDEP_INDEX(pc2, index);
    return JS_TRUE;
}

static jsbytecode *
AddSwitchSpanDeps(JSContext *cx, JSCodeGenerator *cg, jsbytecode *pc)
{
    JSOp op;
    jsbytecode *pc2;
    ptrdiff_t off;
    jsint low, high;
    uintN njumps, indexlen;

    op = (JSOp) *pc;
    JS_ASSERT(op == JSOP_TABLESWITCH || op == JSOP_LOOKUPSWITCH);
    pc2 = pc;
    off = GET_JUMP_OFFSET(pc2);
    if (!AddSpanDep(cx, cg, pc, pc2, off))
        return NULL;
    pc2 += JUMP_OFFSET_LEN;
    if (op == JSOP_TABLESWITCH) {
        low = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        high = GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        njumps = (uintN) (high - low + 1);
        indexlen = 0;
    } else {
        njumps = GET_UINT16(pc2);
        pc2 += UINT16_LEN;
        indexlen = INDEX_LEN;
    }
    while (njumps) {
        --njumps;
        pc2 += indexlen;
        off = GET_JUMP_OFFSET(pc2);
        if (!AddSpanDep(cx, cg, pc, pc2, off))
            return NULL;
        pc2 += JUMP_OFFSET_LEN;
    }
    return 1 + pc2;
}

static JSBool
BuildSpanDepTable(JSContext *cx, JSCodeGenerator *cg)
{
    jsbytecode *pc, *end;
    JSOp op;
    const JSCodeSpec *cs;
    ptrdiff_t off;

    pc = CG_BASE(cg) + cg->spanDepTodo;
    end = CG_NEXT(cg);
    while (pc != end) {
        JS_ASSERT(pc < end);
        op = (JSOp)*pc;
        cs = &js_CodeSpec[op];

        switch (JOF_TYPE(cs->format)) {
          case JOF_TABLESWITCH:
          case JOF_LOOKUPSWITCH:
            pc = AddSwitchSpanDeps(cx, cg, pc);
            if (!pc)
                return JS_FALSE;
            break;

          case JOF_JUMP:
            off = GET_JUMP_OFFSET(pc);
            if (!AddSpanDep(cx, cg, pc, pc, off))
                return JS_FALSE;
            
          default:
            pc += cs->length;
            break;
        }
    }

    return JS_TRUE;
}

static JSSpanDep *
GetSpanDep(JSCodeGenerator *cg, jsbytecode *pc)
{
    uintN index;
    ptrdiff_t offset;
    int lo, hi, mid;
    JSSpanDep *sd;

    index = GET_SPANDEP_INDEX(pc);
    if (index != SPANDEP_INDEX_HUGE)
        return cg->spanDeps + index;

    offset = pc - CG_BASE(cg);
    lo = 0;
    hi = cg->numSpanDeps - 1;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        sd = cg->spanDeps + mid;
        if (sd->before == offset)
            return sd;
        if (sd->before < offset)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    JS_ASSERT(0);
    return NULL;
}

static JSBool
SetBackPatchDelta(JSContext *cx, JSCodeGenerator *cg, jsbytecode *pc,
                  ptrdiff_t delta)
{
    JSSpanDep *sd;

    JS_ASSERT(delta >= 1 + JUMP_OFFSET_LEN);
    if (!cg->spanDeps && delta < JUMP_OFFSET_MAX) {
        SET_JUMP_OFFSET(pc, delta);
        return JS_TRUE;
    }

    if (delta > BPDELTA_MAX) {
        ReportStatementTooLarge(cx, cg);
        return JS_FALSE;
    }

    if (!cg->spanDeps && !BuildSpanDepTable(cx, cg))
        return JS_FALSE;

    sd = GetSpanDep(cg, pc);
    JS_ASSERT(SD_GET_BPDELTA(sd) == 0);
    SD_SET_BPDELTA(sd, delta);
    return JS_TRUE;
}

static void
UpdateJumpTargets(JSJumpTarget *jt, ptrdiff_t pivot, ptrdiff_t delta)
{
    if (jt->offset > pivot) {
        jt->offset += delta;
        if (jt->kids[JT_LEFT])
            UpdateJumpTargets(jt->kids[JT_LEFT], pivot, delta);
    }
    if (jt->kids[JT_RIGHT])
        UpdateJumpTargets(jt->kids[JT_RIGHT], pivot, delta);
}

static JSSpanDep *
FindNearestSpanDep(JSCodeGenerator *cg, ptrdiff_t offset, int lo,
                   JSSpanDep *guard)
{
    int num, hi, mid;
    JSSpanDep *sdbase, *sd;

    num = cg->numSpanDeps;
    JS_ASSERT(num > 0);
    hi = num - 1;
    sdbase = cg->spanDeps;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        sd = sdbase + mid;
        if (sd->before == offset)
            return sd;
        if (sd->before < offset)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    if (lo == num)
        return guard;
    sd = sdbase + lo;
    JS_ASSERT(sd->before >= offset && (lo == 0 || sd[-1].before < offset));
    return sd;
}

static void
FreeJumpTargets(JSCodeGenerator *cg, JSJumpTarget *jt)
{
    if (jt->kids[JT_LEFT])
        FreeJumpTargets(cg, jt->kids[JT_LEFT]);
    if (jt->kids[JT_RIGHT])
        FreeJumpTargets(cg, jt->kids[JT_RIGHT]);
    jt->kids[JT_LEFT] = cg->jtFreeList;
    cg->jtFreeList = jt;
}

static JSBool
OptimizeSpanDeps(JSContext *cx, JSCodeGenerator *cg)
{
    jsbytecode *pc, *oldpc, *base, *limit, *next;
    JSSpanDep *sd, *sd2, *sdbase, *sdlimit, *sdtop, guard;
    ptrdiff_t offset, growth, delta, top, pivot, span, length, target;
    JSBool done;
    JSOp op;
    uint32 type;
    size_t size, incr;
    jssrcnote *sn, *snlimit;
    JSSrcNoteSpec *spec;
    uintN i, n, noteIndex;
    JSTryNode *tryNode;
#ifdef DEBUG_brendan
    int passes = 0;
#endif

    base = CG_BASE(cg);
    sdbase = cg->spanDeps;
    sdlimit = sdbase + cg->numSpanDeps;
    offset = CG_OFFSET(cg);
    growth = 0;

    do {
        done = JS_TRUE;
        delta = 0;
        top = pivot = -1;
        sdtop = NULL;
        pc = NULL;
        op = JSOP_NOP;
        type = 0;
#ifdef DEBUG_brendan
        passes++;
#endif

        for (sd = sdbase; sd < sdlimit; sd++) {
            JS_ASSERT(JT_HAS_TAG(sd->target));
            sd->offset += delta;

            if (sd->top != top) {
                sdtop = sd;
                top = sd->top;
                JS_ASSERT(top == sd->before);
                pivot = sd->offset;
                pc = base + top;
                op = (JSOp) *pc;
                type = JOF_OPTYPE(op);
                if (JOF_TYPE_IS_EXTENDED_JUMP(type)) {
                    





                    continue;
                }

                JS_ASSERT(type == JOF_JUMP ||
                          type == JOF_TABLESWITCH ||
                          type == JOF_LOOKUPSWITCH);
            }

            if (!JOF_TYPE_IS_EXTENDED_JUMP(type)) {
                span = SD_SPAN(sd, pivot);
                if (span < JUMP_OFFSET_MIN || JUMP_OFFSET_MAX < span) {
                    ptrdiff_t deltaFromTop = 0;

                    done = JS_FALSE;

                    switch (op) {
                      case JSOP_GOTO:         op = JSOP_GOTOX; break;
                      case JSOP_IFEQ:         op = JSOP_IFEQX; break;
                      case JSOP_IFNE:         op = JSOP_IFNEX; break;
                      case JSOP_OR:           op = JSOP_ORX; break;
                      case JSOP_AND:          op = JSOP_ANDX; break;
                      case JSOP_GOSUB:        op = JSOP_GOSUBX; break;
                      case JSOP_CASE:         op = JSOP_CASEX; break;
                      case JSOP_DEFAULT:      op = JSOP_DEFAULTX; break;
                      case JSOP_TABLESWITCH:  op = JSOP_TABLESWITCHX; break;
                      case JSOP_LOOKUPSWITCH: op = JSOP_LOOKUPSWITCHX; break;
                      default:
                        ReportStatementTooLarge(cx, cg);
                        return JS_FALSE;
                    }
                    *pc = (jsbytecode) op;

                    for (sd2 = sdtop; sd2 < sdlimit && sd2->top == top; sd2++) {
                        if (sd2 <= sd) {
                            










                            sd2->offset += deltaFromTop;
                            deltaFromTop += JUMPX_OFFSET_LEN - JUMP_OFFSET_LEN;
                        } else {
                            




                            sd2->offset += delta;
                        }

                        delta += JUMPX_OFFSET_LEN - JUMP_OFFSET_LEN;
                        UpdateJumpTargets(cg->jumpTargets, sd2->offset,
                                          JUMPX_OFFSET_LEN - JUMP_OFFSET_LEN);
                    }
                    sd = sd2 - 1;
                }
            }
        }

        growth += delta;
    } while (!done);

    if (growth) {
#ifdef DEBUG_brendan
        JSTokenStream *ts = &cg->compiler->tokenStream;

        printf("%s:%u: %u/%u jumps extended in %d passes (%d=%d+%d)\n",
               ts->filename ? ts->filename : "stdin", cg->firstLine,
               growth / (JUMPX_OFFSET_LEN - JUMP_OFFSET_LEN), cg->numSpanDeps,
               passes, offset + growth, offset, growth);
#endif

        



        limit = CG_LIMIT(cg);
        length = offset + growth;
        next = base + length;
        if (next > limit) {
            JS_ASSERT(length > BYTECODE_CHUNK);
            size = BYTECODE_SIZE(limit - base);
            incr = BYTECODE_SIZE(length) - size;
            JS_ARENA_GROW_CAST(base, jsbytecode *, cg->codePool, size, incr);
            if (!base) {
                js_ReportOutOfScriptQuota(cx);
                return JS_FALSE;
            }
            CG_BASE(cg) = base;
            CG_LIMIT(cg) = next = base + length;
        }
        CG_NEXT(cg) = next;

        





        guard.top = -1;
        guard.offset = offset + growth;
        guard.before = offset;
        guard.target = NULL;
    }

    







    JS_ASSERT(sd == sdlimit);
    top = -1;
    while (--sd >= sdbase) {
        if (sd->top != top) {
            top = sd->top;
            op = (JSOp) base[top];
            type = JOF_OPTYPE(op);

            for (sd2 = sd - 1; sd2 >= sdbase && sd2->top == top; sd2--)
                continue;
            sd2++;
            pivot = sd2->offset;
            JS_ASSERT(top == sd2->before);
        }

        oldpc = base + sd->before;
        span = SD_SPAN(sd, pivot);

        









        if (!JOF_TYPE_IS_EXTENDED_JUMP(type)) {
            JS_ASSERT(JUMP_OFFSET_MIN <= span && span <= JUMP_OFFSET_MAX);
            SET_JUMP_OFFSET(oldpc, span);
            continue;
        }

        






        pc = base + sd->offset;
        delta = offset - sd->before;
        JS_ASSERT(delta >= 1 + JUMP_OFFSET_LEN);

        





        offset = sd->before + 1;
        size = BYTECODE_SIZE(delta - (1 + JUMP_OFFSET_LEN));
        if (size) {
            memmove(pc + 1 + JUMPX_OFFSET_LEN,
                    oldpc + 1 + JUMP_OFFSET_LEN,
                    size);
        }

        SET_JUMPX_OFFSET(pc, span);
    }

    if (growth) {
        







        offset = growth = 0;
        sd = sdbase;
        for (sn = cg->main.notes, snlimit = sn + cg->main.noteCount;
             sn < snlimit;
             sn = SN_NEXT(sn)) {
            




            offset += SN_DELTA(sn);
            while (sd < sdlimit && sd->before < offset) {
                




                sd2 = sd + 1;
                if (sd2 == sdlimit)
                    sd2 = &guard;
                delta = sd2->offset - (sd2->before + growth);
                if (delta > 0) {
                    JS_ASSERT(delta == JUMPX_OFFSET_LEN - JUMP_OFFSET_LEN);
                    sn = js_AddToSrcNoteDelta(cx, cg, sn, delta);
                    if (!sn)
                        return JS_FALSE;
                    snlimit = cg->main.notes + cg->main.noteCount;
                    growth += delta;
                }
                sd++;
            }

            







            spec = &js_SrcNoteSpec[SN_TYPE(sn)];
            if (spec->isSpanDep) {
                pivot = offset + spec->offsetBias;
                n = spec->arity;
                for (i = 0; i < n; i++) {
                    span = js_GetSrcNoteOffset(sn, i);
                    if (span == 0)
                        continue;
                    target = pivot + span * spec->isSpanDep;
                    sd2 = FindNearestSpanDep(cg, target,
                                             (target >= pivot)
                                             ? sd - sdbase
                                             : 0,
                                             &guard);

                    





                    target += sd2->offset - sd2->before;
                    span = target - (pivot + growth);
                    span *= spec->isSpanDep;
                    noteIndex = sn - cg->main.notes;
                    if (!js_SetSrcNoteOffset(cx, cg, noteIndex, i, span))
                        return JS_FALSE;
                    sn = cg->main.notes + noteIndex;
                    snlimit = cg->main.notes + cg->main.noteCount;
                }
            }
        }
        cg->main.lastNoteOffset += growth;

        



        for (tryNode = cg->lastTryNode; tryNode; tryNode = tryNode->prev) {
            




            offset = tryNode->note.start;
            sd = FindNearestSpanDep(cg, offset, 0, &guard);
            delta = sd->offset - sd->before;
            tryNode->note.start = offset + delta;

            



            length = tryNode->note.length;
            sd2 = FindNearestSpanDep(cg, offset + length, sd - sdbase, &guard);
            if (sd2 != sd) {
                tryNode->note.length =
                    length + sd2->offset - sd2->before - delta;
            }
        }
    }

#ifdef DEBUG_brendan
  {
    uintN bigspans = 0;
    top = -1;
    for (sd = sdbase; sd < sdlimit; sd++) {
        offset = sd->offset;

        
        if (sd->top != top) {
            JS_ASSERT(top == -1 ||
                      !JOF_TYPE_IS_EXTENDED_JUMP(type) ||
                      bigspans != 0);
            bigspans = 0;
            top = sd->top;
            JS_ASSERT(top == sd->before);
            op = (JSOp) base[offset];
            type = JOF_OPTYPE(op);
            JS_ASSERT(type == JOF_JUMP ||
                      type == JOF_JUMPX ||
                      type == JOF_TABLESWITCH ||
                      type == JOF_TABLESWITCHX ||
                      type == JOF_LOOKUPSWITCH ||
                      type == JOF_LOOKUPSWITCHX);
            pivot = offset;
        }

        pc = base + offset;
        if (JOF_TYPE_IS_EXTENDED_JUMP(type)) {
            span = GET_JUMPX_OFFSET(pc);
            if (span < JUMP_OFFSET_MIN || JUMP_OFFSET_MAX < span) {
                bigspans++;
            } else {
                JS_ASSERT(type == JOF_TABLESWITCHX ||
                          type == JOF_LOOKUPSWITCHX);
            }
        } else {
            span = GET_JUMP_OFFSET(pc);
        }
        JS_ASSERT(SD_SPAN(sd, pivot) == span);
    }
    JS_ASSERT(!JOF_TYPE_IS_EXTENDED_JUMP(type) || bigspans != 0);
  }
#endif

    




    size = SPANDEPS_SIZE(JS_BIT(JS_CeilingLog2(cg->numSpanDeps)));
    cx->free(cg->spanDeps);
    cg->spanDeps = NULL;
    FreeJumpTargets(cg, cg->jumpTargets);
    cg->jumpTargets = NULL;
    cg->numSpanDeps = cg->numJumpTargets = 0;
    cg->spanDepTodo = CG_OFFSET(cg);
    return JS_TRUE;
}

static ptrdiff_t
EmitJump(JSContext *cx, JSCodeGenerator *cg, JSOp op, ptrdiff_t off)
{
    JSBool extend;
    ptrdiff_t jmp;
    jsbytecode *pc;

    extend = off < JUMP_OFFSET_MIN || JUMP_OFFSET_MAX < off;
    if (extend && !cg->spanDeps && !BuildSpanDepTable(cx, cg))
        return -1;

    jmp = js_Emit3(cx, cg, op, JUMP_OFFSET_HI(off), JUMP_OFFSET_LO(off));
    if (jmp >= 0 && (extend || cg->spanDeps)) {
        pc = CG_CODE(cg, jmp);
        if (!AddSpanDep(cx, cg, pc, pc, off))
            return -1;
    }
    return jmp;
}

static ptrdiff_t
GetJumpOffset(JSCodeGenerator *cg, jsbytecode *pc)
{
    JSSpanDep *sd;
    JSJumpTarget *jt;
    ptrdiff_t top;

    if (!cg->spanDeps)
        return GET_JUMP_OFFSET(pc);

    sd = GetSpanDep(cg, pc);
    jt = sd->target;
    if (!JT_HAS_TAG(jt))
        return JT_TO_BPDELTA(jt);

    top = sd->top;
    while (--sd >= cg->spanDeps && sd->top == top)
        continue;
    sd++;
    return JT_CLR_TAG(jt)->offset - sd->offset;
}

JSBool
js_SetJumpOffset(JSContext *cx, JSCodeGenerator *cg, jsbytecode *pc,
                 ptrdiff_t off)
{
    if (!cg->spanDeps) {
        if (JUMP_OFFSET_MIN <= off && off <= JUMP_OFFSET_MAX) {
            SET_JUMP_OFFSET(pc, off);
            return JS_TRUE;
        }

        if (!BuildSpanDepTable(cx, cg))
            return JS_FALSE;
    }

    return SetSpanDepTarget(cx, cg, GetSpanDep(cg, pc), off);
}

bool
JSTreeContext::inStatement(JSStmtType type)
{
    for (JSStmtInfo *stmt = topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == type)
            return true;
    }
    return false;
}

bool
JSTreeContext::ensureSharpSlots()
{
#if JS_HAS_SHARP_VARS
    JS_STATIC_ASSERT(SHARP_NSLOTS == 2);

    if (sharpSlotBase >= 0) {
        JS_ASSERT(flags & TCF_HAS_SHARPS);
        return true;
    }

    JS_ASSERT(!(flags & TCF_HAS_SHARPS));
    if (flags & TCF_IN_FUNCTION) {
        JSContext *cx = compiler->context;
        JSAtom *sharpArrayAtom = js_Atomize(cx, "#array", 6, 0);
        JSAtom *sharpDepthAtom = js_Atomize(cx, "#depth", 6, 0);
        if (!sharpArrayAtom || !sharpDepthAtom)
            return false;

        sharpSlotBase = fun->u.i.nvars;
        if (!js_AddLocal(cx, fun, sharpArrayAtom, JSLOCAL_VAR))
            return false;
        if (!js_AddLocal(cx, fun, sharpDepthAtom, JSLOCAL_VAR))
            return false;
    } else {
        




        sharpSlotBase = 0;
    }
    flags |= TCF_HAS_SHARPS;
#endif
    return true;
}

void
js_PushStatement(JSTreeContext *tc, JSStmtInfo *stmt, JSStmtType type,
                 ptrdiff_t top)
{
    stmt->type = type;
    stmt->flags = 0;
    stmt->blockid = tc->blockid();
    SET_STATEMENT_TOP(stmt, top);
    stmt->label = NULL;
    JS_ASSERT(!stmt->blockObj);
    stmt->down = tc->topStmt;
    tc->topStmt = stmt;
    if (STMT_LINKS_SCOPE(stmt)) {
        stmt->downScope = tc->topScopeStmt;
        tc->topScopeStmt = stmt;
    } else {
        stmt->downScope = NULL;
    }
}

void
js_PushBlockScope(JSTreeContext *tc, JSStmtInfo *stmt, JSObject *blockObj,
                  ptrdiff_t top)
{
    js_PushStatement(tc, stmt, STMT_BLOCK, top);
    stmt->flags |= SIF_SCOPE;
    STOBJ_SET_PARENT(blockObj, tc->blockChain);
    stmt->downScope = tc->topScopeStmt;
    tc->topScopeStmt = stmt;
    tc->blockChain = blockObj;
    stmt->blockObj = blockObj;
}





static ptrdiff_t
EmitBackPatchOp(JSContext *cx, JSCodeGenerator *cg, JSOp op, ptrdiff_t *lastp)
{
    ptrdiff_t offset, delta;

    offset = CG_OFFSET(cg);
    delta = offset - *lastp;
    *lastp = offset;
    JS_ASSERT(delta > 0);
    return EmitJump(cx, cg, op, delta);
}







#define EMIT_UINT16_IMM_OP(op, i)                                             \
    JS_BEGIN_MACRO                                                            \
        if (js_Emit3(cx, cg, op, UINT16_HI(i), UINT16_LO(i)) < 0)             \
            return JS_FALSE;                                                  \
    JS_END_MACRO

#define EMIT_UINT16PAIR_IMM_OP(op, i, j)                                      \
    JS_BEGIN_MACRO                                                            \
        ptrdiff_t off_ = js_EmitN(cx, cg, op, 2 * UINT16_LEN);                \
        if (off_ < 0)                                                         \
            return JS_FALSE;                                                  \
        jsbytecode *pc_ = CG_CODE(cg, off_);                                  \
        SET_UINT16(pc_, i);                                                   \
        pc_ += UINT16_LEN;                                                    \
        SET_UINT16(pc_, j);                                                   \
    JS_END_MACRO

static JSBool
FlushPops(JSContext *cx, JSCodeGenerator *cg, intN *npops)
{
    JS_ASSERT(*npops != 0);
    if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
        return JS_FALSE;
    EMIT_UINT16_IMM_OP(JSOP_POPN, *npops);
    *npops = 0;
    return JS_TRUE;
}




static JSBool
EmitNonLocalJumpFixup(JSContext *cx, JSCodeGenerator *cg, JSStmtInfo *toStmt)
{
    intN depth, npops;
    JSStmtInfo *stmt;

    





    depth = cg->stackDepth;
    npops = 0;

#define FLUSH_POPS() if (npops && !FlushPops(cx, cg, &npops)) return JS_FALSE

    for (stmt = cg->topStmt; stmt != toStmt; stmt = stmt->down) {
        switch (stmt->type) {
          case STMT_FINALLY:
            FLUSH_POPS();
            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &GOSUBS(*stmt)) < 0)
                return JS_FALSE;
            break;

          case STMT_WITH:
            
            FLUSH_POPS();
            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_LEAVEWITH) < 0)
                return JS_FALSE;
            break;

          case STMT_FOR_IN_LOOP:
            


            FLUSH_POPS();
            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_ENDITER) < 0)
                return JS_FALSE;
            break;

          case STMT_SUBROUTINE:
            



            npops += 2;
            break;

          default:;
        }

        if (stmt->flags & SIF_SCOPE) {
            uintN i;

            
            FLUSH_POPS();
            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            i = OBJ_BLOCK_COUNT(cx, stmt->blockObj);
            EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, i);
        }
    }

    FLUSH_POPS();
    cg->stackDepth = depth;
    return JS_TRUE;

#undef FLUSH_POPS
}

static ptrdiff_t
EmitGoto(JSContext *cx, JSCodeGenerator *cg, JSStmtInfo *toStmt,
         ptrdiff_t *lastp, JSAtomListElement *label, JSSrcNoteType noteType)
{
    intN index;

    if (!EmitNonLocalJumpFixup(cx, cg, toStmt))
        return -1;

    if (label)
        index = js_NewSrcNote2(cx, cg, noteType, (ptrdiff_t) ALE_INDEX(label));
    else if (noteType != SRC_NULL)
        index = js_NewSrcNote(cx, cg, noteType);
    else
        index = 0;
    if (index < 0)
        return -1;

    return EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, lastp);
}

static JSBool
BackPatch(JSContext *cx, JSCodeGenerator *cg, ptrdiff_t last,
          jsbytecode *target, jsbytecode op)
{
    jsbytecode *pc, *stop;
    ptrdiff_t delta, span;

    pc = CG_CODE(cg, last);
    stop = CG_CODE(cg, -1);
    while (pc != stop) {
        delta = GetJumpOffset(cg, pc);
        span = target - pc;
        CHECK_AND_SET_JUMP_OFFSET(cx, cg, pc, span);

        




        *pc = op;
        pc -= delta;
    }
    return JS_TRUE;
}

void
js_PopStatement(JSTreeContext *tc)
{
    JSStmtInfo *stmt;

    stmt = tc->topStmt;
    tc->topStmt = stmt->down;
    if (STMT_LINKS_SCOPE(stmt)) {
        tc->topScopeStmt = stmt->downScope;
        if (stmt->flags & SIF_SCOPE) {
            tc->blockChain = STOBJ_GET_PARENT(stmt->blockObj);
            JS_SCOPE_DEPTH_METERING(--tc->scopeDepth);
        }
    }
}

JSBool
js_PopStatementCG(JSContext *cx, JSCodeGenerator *cg)
{
    JSStmtInfo *stmt;

    stmt = cg->topStmt;
    if (!STMT_IS_TRYING(stmt) &&
        (!BackPatch(cx, cg, stmt->breaks, CG_NEXT(cg), JSOP_GOTO) ||
         !BackPatch(cx, cg, stmt->continues, CG_CODE(cg, stmt->update),
                    JSOP_GOTO))) {
        return JS_FALSE;
    }
    js_PopStatement(cg);
    return JS_TRUE;
}

JSBool
js_DefineCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                             JSParseNode *pn)
{
    jsdouble dval;
    jsint ival;
    JSAtom *valueAtom;
    jsval v;
    JSAtomListElement *ale;

    
    if (pn->pn_type == TOK_NUMBER) {
        dval = pn->pn_dval;
        if (JSDOUBLE_IS_INT(dval, ival) && INT_FITS_IN_JSVAL(ival)) {
            v = INT_TO_JSVAL(ival);
        } else {
            




            valueAtom = js_AtomizeDouble(cx, dval);
            if (!valueAtom)
                return JS_FALSE;
            v = ATOM_KEY(valueAtom);
        }
        ale = cg->constList.add(cg->compiler, atom);
        if (!ale)
            return JS_FALSE;
        ALE_SET_VALUE(ale, v);
    }
    return JS_TRUE;
}

JSStmtInfo *
js_LexicalLookup(JSTreeContext *tc, JSAtom *atom, jsint *slotp, JSStmtInfo *stmt)
{
    JSObject *obj;
    JSScope *scope;
    JSScopeProperty *sprop;

    if (!stmt)
        stmt = tc->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        if (stmt->type == STMT_WITH)
            break;

        
        if (!(stmt->flags & SIF_SCOPE))
            continue;

        obj = stmt->blockObj;
        JS_ASSERT(obj->getClass() == &js_BlockClass);
        scope = OBJ_SCOPE(obj);
        sprop = scope->lookup(ATOM_TO_JSID(atom));
        if (sprop) {
            JS_ASSERT(sprop->flags & SPROP_HAS_SHORTID);

            if (slotp) {
                JS_ASSERT(JSVAL_IS_INT(obj->fslots[JSSLOT_BLOCK_DEPTH]));
                *slotp = JSVAL_TO_INT(obj->fslots[JSSLOT_BLOCK_DEPTH]) +
                         sprop->shortid;
            }
            return stmt;
        }
    }

    if (slotp)
        *slotp = -1;
    return stmt;
}





#define IS_CONSTANT_PROPERTY(attrs)                                           \
    (((attrs) & (JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_GETTER)) ==      \
     (JSPROP_READONLY | JSPROP_PERMANENT))





static JSBool
LookupCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                          jsval *vp)
{
    JSBool ok;
    JSStmtInfo *stmt;
    JSAtomListElement *ale;
    JSObject *obj, *objbox;
    JSProperty *prop;
    uintN attrs;

    




    *vp = JSVAL_HOLE;
    do {
        if (cg->flags & (TCF_IN_FUNCTION | TCF_COMPILE_N_GO)) {
            
            stmt = js_LexicalLookup(cg, atom, NULL);
            if (stmt)
                return JS_TRUE;

            ale = cg->constList.lookup(atom);
            if (ale) {
                JS_ASSERT(ALE_VALUE(ale) != JSVAL_HOLE);
                *vp = ALE_VALUE(ale);
                return JS_TRUE;
            }

            






            if (cg->flags & TCF_IN_FUNCTION) {
                if (js_LookupLocal(cx, cg->fun, atom, NULL) != JSLOCAL_NONE)
                    break;
            } else {
                JS_ASSERT(cg->flags & TCF_COMPILE_N_GO);
                obj = cg->scopeChain;
                ok = obj->lookupProperty(cx, ATOM_TO_JSID(atom), &objbox, &prop);
                if (!ok)
                    return JS_FALSE;
                if (objbox == obj) {
                    





                    ok = obj->getAttributes(cx, ATOM_TO_JSID(atom), prop, &attrs);
                    if (ok && IS_CONSTANT_PROPERTY(attrs)) {
                        ok = obj->getProperty(cx, ATOM_TO_JSID(atom), vp);
                        JS_ASSERT_IF(ok, *vp != JSVAL_HOLE);
                    }
                }
                if (prop)
                    objbox->dropProperty(cx, prop);
                if (!ok)
                    return JS_FALSE;
                if (prop)
                    break;
            }
        }
    } while ((cg = (JSCodeGenerator *) cg->parent) != NULL);
    return JS_TRUE;
}







static JSOp
EmitBigIndexPrefix(JSContext *cx, JSCodeGenerator *cg, uintN index)
{
    uintN indexBase;

    



    JS_STATIC_ASSERT(INDEX_LIMIT <= JS_BIT(24));
    JS_STATIC_ASSERT(INDEX_LIMIT >=
                     (JSOP_INDEXBASE3 - JSOP_INDEXBASE1 + 2) << 16);

    if (index < JS_BIT(16))
        return JSOP_NOP;
    indexBase = index >> 16;
    if (indexBase <= JSOP_INDEXBASE3 - JSOP_INDEXBASE1 + 1) {
        if (js_Emit1(cx, cg, (JSOp)(JSOP_INDEXBASE1 + indexBase - 1)) < 0)
            return JSOP_FALSE;
        return JSOP_RESETBASE0;
    }

    if (index >= INDEX_LIMIT) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_TOO_MANY_LITERALS);
        return JSOP_FALSE;
    }

    if (js_Emit2(cx, cg, JSOP_INDEXBASE, (JSOp)indexBase) < 0)
        return JSOP_FALSE;
    return JSOP_RESETBASE;
}











static JSBool
EmitIndexOp(JSContext *cx, JSOp op, uintN index, JSCodeGenerator *cg)
{
    JSOp bigSuffix;

    bigSuffix = EmitBigIndexPrefix(cx, cg, index);
    if (bigSuffix == JSOP_FALSE)
        return JS_FALSE;
    EMIT_UINT16_IMM_OP(op, index);
    return bigSuffix == JSOP_NOP || js_Emit1(cx, cg, bigSuffix) >= 0;
}





#define EMIT_INDEX_OP(op, index)                                              \
    JS_BEGIN_MACRO                                                            \
        if (!EmitIndexOp(cx, op, index, cg))                                  \
            return JS_FALSE;                                                  \
    JS_END_MACRO

static JSBool
EmitAtomOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    JSAtomListElement *ale;

    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
    if (op == JSOP_GETPROP &&
        pn->pn_atom == cx->runtime->atomState.lengthAtom) {
        return js_Emit1(cx, cg, JSOP_LENGTH) >= 0;
    }
    ale = cg->atomList.add(cg->compiler, pn->pn_atom);
    if (!ale)
        return JS_FALSE;
    return EmitIndexOp(cx, op, ALE_INDEX(ale), cg);
}

static JSBool
EmitObjectOp(JSContext *cx, JSObjectBox *objbox, JSOp op,
             JSCodeGenerator *cg)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_OBJECT);
    return EmitIndexOp(cx, op, cg->objectList.index(objbox), cg);
}








JS_STATIC_ASSERT(ARGNO_LEN == 2);
JS_STATIC_ASSERT(SLOTNO_LEN == 2);

static JSBool
EmitSlotIndexOp(JSContext *cx, JSOp op, uintN slot, uintN index,
                JSCodeGenerator *cg)
{
    JSOp bigSuffix;
    ptrdiff_t off;
    jsbytecode *pc;

    JS_ASSERT(JOF_OPTYPE(op) == JOF_SLOTATOM ||
              JOF_OPTYPE(op) == JOF_SLOTOBJECT);
    bigSuffix = EmitBigIndexPrefix(cx, cg, index);
    if (bigSuffix == JSOP_FALSE)
        return JS_FALSE;

    
    off = js_EmitN(cx, cg, op, 2 + INDEX_LEN);
    if (off < 0)
        return JS_FALSE;
    pc = CG_CODE(cg, off);
    SET_UINT16(pc, slot);
    pc += 2;
    SET_INDEX(pc, index);
    return bigSuffix == JSOP_NOP || js_Emit1(cx, cg, bigSuffix) >= 0;
}









static jsint
AdjustBlockSlot(JSContext *cx, JSCodeGenerator *cg, jsint slot)
{
    JS_ASSERT((jsuint) slot < cg->maxStackDepth);
    if (cg->flags & TCF_IN_FUNCTION) {
        slot += cg->fun->u.i.nvars;
        if ((uintN) slot >= SLOTNO_LIMIT) {
            js_ReportCompileErrorNumber(cx, CG_TS(cg), NULL,
                                        JSREPORT_ERROR,
                                        JSMSG_TOO_MANY_LOCALS);
            slot = -1;
        }
    }
    return slot;
}

static bool
EmitEnterBlock(JSContext *cx, JSParseNode *pn, JSCodeGenerator *cg)
{
    JS_ASSERT(PN_TYPE(pn) == TOK_LEXICALSCOPE);
    if (!EmitObjectOp(cx, pn->pn_objbox, JSOP_ENTERBLOCK, cg))
        return false;

    JSObject *blockObj = pn->pn_objbox->object;
    jsint depth = AdjustBlockSlot(cx, cg, OBJ_BLOCK_DEPTH(cx, blockObj));
    if (depth < 0)
        return false;

    for (uintN slot = JSSLOT_FREE(&js_BlockClass),
               limit = slot + OBJ_BLOCK_COUNT(cx, blockObj);
         slot < limit; slot++) {
        jsval v = STOBJ_GET_SLOT(blockObj, slot);

        
        if (JSVAL_IS_VOID(v)) {
            JS_ASSERT(slot + 1 <= limit);
            continue;
        }

        JSDefinition *dn = (JSDefinition *) JSVAL_TO_PRIVATE(v);
        JS_ASSERT(dn->pn_defn);
        JS_ASSERT(uintN(dn->frameSlot() + depth) < JS_BIT(16));
        dn->pn_cookie += depth;
#ifdef DEBUG
        for (JSParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
            JS_ASSERT(pnu->pn_lexdef == dn);
            JS_ASSERT(!(pnu->pn_dflags & PND_BOUND));
            JS_ASSERT(pnu->pn_cookie == FREE_UPVAR_COOKIE);
        }
#endif
    }

    OBJ_SCOPE(blockObj)->freeslot = JSSLOT_FREE(&js_BlockClass);
    return js_GrowSlots(cx, blockObj, JSSLOT_FREE(&js_BlockClass));
}















static bool
MakeUpvarForEval(JSParseNode *pn, JSCodeGenerator *cg)
{
    JSContext *cx = cg->compiler->context;
    JSFunction *fun = cg->compiler->callerFrame->fun;
    uintN upvarLevel = fun->u.i.script->staticLevel;

    JSFunctionBox *funbox = cg->funbox;
    if (funbox) {
        





        if (funbox->level == fun->u.i.script->staticLevel + 1U &&
            !(((JSFunction *) funbox->object)->flags & JSFUN_LAMBDA)) {
            JS_ASSERT_IF(cx->options & JSOPTION_ANONFUNFIX,
                         ((JSFunction *) funbox->object)->atom);
            return true;
        }

        while (funbox->level >= upvarLevel) {
            if (funbox->node->pn_dflags & PND_FUNARG)
                return true;
            funbox = funbox->parent;
            if (!funbox)
                break;
        }
    }

    JSAtom *atom = pn->pn_atom;

    uintN index;
    JSLocalKind localKind = js_LookupLocal(cx, fun, atom, &index);
    if (localKind == JSLOCAL_NONE)
        return true;

    JS_ASSERT(cg->staticLevel > upvarLevel);
    if (cg->staticLevel >= JS_DISPLAY_SIZE || upvarLevel >= JS_DISPLAY_SIZE)
        return true;

    JSAtomListElement *ale = cg->upvarList.lookup(atom);
    if (!ale) {
        if ((cg->flags & TCF_IN_FUNCTION) &&
            !js_AddLocal(cx, cg->fun, atom, JSLOCAL_UPVAR)) {
            return false;
        }

        ale = cg->upvarList.add(cg->compiler, atom);
        if (!ale)
            return false;
        JS_ASSERT(ALE_INDEX(ale) == cg->upvarList.count - 1);

        uint32 *vector = cg->upvarMap.vector;
        uint32 length = cg->upvarMap.length;

        JS_ASSERT(ALE_INDEX(ale) <= length);
        if (ALE_INDEX(ale) == length) {
            length = 2 * JS_MAX(2, length);
            vector = (uint32 *) cx->realloc(vector, length * sizeof *vector);
            if (!vector)
                return false;
            cg->upvarMap.vector = vector;
            cg->upvarMap.length = length;
        }

        if (localKind != JSLOCAL_ARG)
            index += fun->nargs;
        JS_ASSERT(index < JS_BIT(16));

        uintN skip = cg->staticLevel - upvarLevel;
        vector[ALE_INDEX(ale)] = MAKE_UPVAR_COOKIE(skip, index);
    }

    pn->pn_op = JSOP_GETUPVAR;
    pn->pn_cookie = MAKE_UPVAR_COOKIE(cg->staticLevel, ALE_INDEX(ale));
    pn->pn_dflags |= PND_BOUND;
    return true;
}


















static JSBool
BindNameToSlot(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn)
{
    JSDefinition *dn;
    JSOp op;
    JSAtom *atom;
    uint32 cookie;
    JSDefinition::Kind dn_kind;
    JSAtomListElement *ale;
    uintN index;

    JS_ASSERT(pn->pn_type == TOK_NAME);

    
    if (pn->pn_dflags & PND_BOUND)
        return JS_TRUE;

    
    JS_ASSERT(pn->pn_op != JSOP_ARGUMENTS && pn->pn_op != JSOP_CALLEE);

    



    if (pn->pn_used) {
        JS_ASSERT(pn->pn_cookie == FREE_UPVAR_COOKIE);
        dn = pn->pn_lexdef;
        JS_ASSERT(dn->pn_defn);
        pn->pn_dflags |= (dn->pn_dflags & PND_CONST);
    } else {
        if (!pn->pn_defn)
            return JS_TRUE;
        dn = (JSDefinition *) pn;
    }

    op = PN_OP(pn);
    if (op == JSOP_NOP)
        return JS_TRUE;

    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
    atom = pn->pn_atom;
    cookie = dn->pn_cookie;
    dn_kind = dn->kind();

    














    switch (op) {
      case JSOP_NAME:
      case JSOP_SETCONST:
        break;
      case JSOP_DELNAME:
        if (dn_kind != JSDefinition::UNKNOWN) {
            if (cg->compiler->callerFrame && !cg->funbox)
                JS_ASSERT(cg->flags & TCF_COMPILE_N_GO);
            else
                pn->pn_op = JSOP_FALSE;
            pn->pn_dflags |= PND_BOUND;
            return JS_TRUE;
        }
        break;
      default:
        if (pn->isConst())
            pn->pn_op = op = JSOP_NAME;
    }

    if (cookie == FREE_UPVAR_COOKIE) {
        JSStackFrame *caller = cg->compiler->callerFrame;
        if (caller) {
            JS_ASSERT(cg->flags & TCF_COMPILE_N_GO);

            



            if (cg->flags & TCF_IN_FOR_INIT)
                return JS_TRUE;

            JS_ASSERT(caller->script);
            if (!caller->fun)
                return JS_TRUE;

            





            JSObject *scopeobj = (cg->flags & TCF_IN_FUNCTION)
                                 ? STOBJ_GET_PARENT(FUN_OBJECT(cg->fun))
                                 : cg->scopeChain;
            if (scopeobj != caller->varobj)
                return JS_TRUE;

            





            if (op != JSOP_NAME)
                return JS_TRUE;

            return MakeUpvarForEval(pn, cg);
        }
        return JS_TRUE;
    }

    if (dn->pn_dflags & PND_GVAR) {
        





        if (cg->flags & TCF_IN_FUNCTION)
            return JS_TRUE;

        







        JS_ASSERT(dn_kind == JSDefinition::VAR || dn_kind == JSDefinition::CONST);

        switch (op) {
          case JSOP_NAME:     op = JSOP_GETGVAR; break;
          case JSOP_SETNAME:  op = JSOP_SETGVAR; break;
          case JSOP_SETCONST:  break;
          case JSOP_INCNAME:  op = JSOP_INCGVAR; break;
          case JSOP_NAMEINC:  op = JSOP_GVARINC; break;
          case JSOP_DECNAME:  op = JSOP_DECGVAR; break;
          case JSOP_NAMEDEC:  op = JSOP_GVARDEC; break;
          case JSOP_FORNAME:   break;
          case JSOP_DELNAME:   break;
          default: JS_NOT_REACHED("gvar");
        }
        pn->pn_op = op;
        pn->pn_cookie = cookie;
        pn->pn_dflags |= PND_BOUND;
        return JS_TRUE;
    }

    uintN level = UPVAR_FRAME_SKIP(cookie);
    JS_ASSERT(cg->staticLevel >= level);

    







    if (PN_OP(dn) == JSOP_GETUPVAR) {
        JS_ASSERT(cg->staticLevel >= level);
        if (op != JSOP_NAME)
            return JS_TRUE;

#ifdef DEBUG
        JSStackFrame *caller = cg->compiler->callerFrame;
#endif
        JS_ASSERT(caller);
        JS_ASSERT(caller->script);

        JSTreeContext *tc = cg;
        while (tc->staticLevel != level)
            tc = tc->parent;
        JS_ASSERT(tc->flags & TCF_COMPILING);

        JSCodeGenerator *evalcg = (JSCodeGenerator *) tc;
        JS_ASSERT(evalcg->flags & TCF_COMPILE_N_GO);
        JS_ASSERT(caller->fun && caller->varobj == evalcg->scopeChain);

        



        if (evalcg->flags & TCF_IN_FOR_INIT)
            return JS_TRUE;

        if (cg->staticLevel == level) {
            pn->pn_op = JSOP_GETUPVAR;
            pn->pn_cookie = cookie;
            pn->pn_dflags |= PND_BOUND;
            return JS_TRUE;
        }

        return MakeUpvarForEval(pn, cg);
    }

    uintN skip = cg->staticLevel - level;
    if (skip != 0) {
        JS_ASSERT(cg->flags & TCF_IN_FUNCTION);
        JS_ASSERT_IF(UPVAR_FRAME_SLOT(cookie) != CALLEE_UPVAR_SLOT,
                     cg->lexdeps.lookup(atom));
        JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
        JS_ASSERT(cg->fun->u.i.skipmin <= skip);

        




        if (op != JSOP_NAME)
            return JS_TRUE;
        if (level >= JS_DISPLAY_SIZE)
            return JS_TRUE;
        if (cg->flags & TCF_FUN_HEAVYWEIGHT)
            return JS_TRUE;

        if (FUN_FLAT_CLOSURE(cg->fun)) {
            op = JSOP_GETDSLOT;
        } else {
            






            if (cg->funbox->node->pn_dflags & PND_FUNARG)
                return JS_TRUE;

            




            if (cg->flags & TCF_FUN_IS_GENERATOR)
                return JS_TRUE;

            op = JSOP_GETUPVAR;
        }

        ale = cg->upvarList.lookup(atom);
        if (ale) {
            index = ALE_INDEX(ale);
        } else {
            if (!js_AddLocal(cx, cg->fun, atom, JSLOCAL_UPVAR))
                return JS_FALSE;

            ale = cg->upvarList.add(cg->compiler, atom);
            if (!ale)
                return JS_FALSE;
            index = ALE_INDEX(ale);
            JS_ASSERT(index == cg->upvarList.count - 1);

            uint32 *vector = cg->upvarMap.vector;
            if (!vector) {
                uint32 length = cg->lexdeps.count;

                vector = (uint32 *) js_calloc(length * sizeof *vector);
                if (!vector) {
                    JS_ReportOutOfMemory(cx);
                    return JS_FALSE;
                }
                cg->upvarMap.vector = vector;
                cg->upvarMap.length = length;
            }

            uintN slot = UPVAR_FRAME_SLOT(cookie);
            if (slot != CALLEE_UPVAR_SLOT && dn_kind != JSDefinition::ARG) {
                JSTreeContext *tc = cg;
                do {
                    tc = tc->parent;
                } while (tc->staticLevel != level);
                if (tc->flags & TCF_IN_FUNCTION)
                    slot += tc->fun->nargs;
            }

            vector[index] = MAKE_UPVAR_COOKIE(skip, slot);
        }

        pn->pn_op = op;
        pn->pn_cookie = index;
        pn->pn_dflags |= PND_BOUND;
        return JS_TRUE;
    }

    




    switch (dn_kind) {
      case JSDefinition::UNKNOWN:
        return JS_TRUE;

      case JSDefinition::LET:
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETLOCAL; break;
          case JSOP_SETNAME:  op = JSOP_SETLOCAL; break;
          case JSOP_INCNAME:  op = JSOP_INCLOCAL; break;
          case JSOP_NAMEINC:  op = JSOP_LOCALINC; break;
          case JSOP_DECNAME:  op = JSOP_DECLOCAL; break;
          case JSOP_NAMEDEC:  op = JSOP_LOCALDEC; break;
          case JSOP_FORNAME:  op = JSOP_FORLOCAL; break;
          default: JS_NOT_REACHED("let");
        }
        break;

      case JSDefinition::ARG:
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETARG; break;
          case JSOP_SETNAME:  op = JSOP_SETARG; break;
          case JSOP_INCNAME:  op = JSOP_INCARG; break;
          case JSOP_NAMEINC:  op = JSOP_ARGINC; break;
          case JSOP_DECNAME:  op = JSOP_DECARG; break;
          case JSOP_NAMEDEC:  op = JSOP_ARGDEC; break;
          case JSOP_FORNAME:  op = JSOP_FORARG; break;
          default: JS_NOT_REACHED("arg");
        }
        JS_ASSERT(!pn->isConst());
        break;

      case JSDefinition::VAR:
        if (PN_OP(dn) == JSOP_CALLEE) {
            JS_ASSERT(op != JSOP_CALLEE);
            JS_ASSERT((cg->fun->flags & JSFUN_LAMBDA) && atom == cg->fun->atom);

            










            JS_ASSERT(op != JSOP_DELNAME);
            if (!(cg->flags & TCF_FUN_HEAVYWEIGHT)) {
                op = JSOP_CALLEE;
                pn->pn_dflags |= PND_CONST;
            }

            pn->pn_op = op;
            pn->pn_dflags |= PND_BOUND;
            return JS_TRUE;
        }
        

      default:
        JS_ASSERT_IF(dn_kind != JSDefinition::FUNCTION,
                     dn_kind == JSDefinition::VAR ||
                     dn_kind == JSDefinition::CONST);
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETLOCAL; break;
          case JSOP_SETNAME:  op = JSOP_SETLOCAL; break;
          case JSOP_SETCONST: op = JSOP_SETLOCAL; break;
          case JSOP_INCNAME:  op = JSOP_INCLOCAL; break;
          case JSOP_NAMEINC:  op = JSOP_LOCALINC; break;
          case JSOP_DECNAME:  op = JSOP_DECLOCAL; break;
          case JSOP_NAMEDEC:  op = JSOP_LOCALDEC; break;
          case JSOP_FORNAME:  op = JSOP_FORLOCAL; break;
          default: JS_NOT_REACHED("local");
        }
        JS_ASSERT_IF(dn_kind == JSDefinition::CONST, pn->pn_dflags & PND_CONST);
        break;
    }

    JS_ASSERT(op != PN_OP(pn));
    pn->pn_op = op;
    pn->pn_cookie = UPVAR_FRAME_SLOT(cookie);
    pn->pn_dflags |= PND_BOUND;
    return JS_TRUE;
}













static JSBool
CheckSideEffects(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn,
                 JSBool *answer)
{
    JSBool ok;
    JSParseNode *pn2;

    ok = JS_TRUE;
    if (!pn || *answer)
        return ok;

    switch (pn->pn_arity) {
      case PN_FUNC:
        






        *answer = JS_FALSE;
        break;

      case PN_LIST:
        if (pn->pn_op == JSOP_NOP ||
            pn->pn_op == JSOP_OR || pn->pn_op == JSOP_AND ||
            pn->pn_op == JSOP_STRICTEQ || pn->pn_op == JSOP_STRICTNE) {
            



            for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next)
                ok &= CheckSideEffects(cx, cg, pn2, answer);
        } else {
            















            *answer = JS_TRUE;
        }
        break;

      case PN_TERNARY:
        ok = CheckSideEffects(cx, cg, pn->pn_kid1, answer) &&
             CheckSideEffects(cx, cg, pn->pn_kid2, answer) &&
             CheckSideEffects(cx, cg, pn->pn_kid3, answer);
        break;

      case PN_BINARY:
        if (pn->pn_type == TOK_ASSIGN) {
            








            pn2 = pn->pn_left;
            if (pn2->pn_type != TOK_NAME) {
                *answer = JS_TRUE;
            } else {
                if (!BindNameToSlot(cx, cg, pn2))
                    return JS_FALSE;
                if (!CheckSideEffects(cx, cg, pn->pn_right, answer))
                    return JS_FALSE;
                if (!*answer && (pn->pn_op != JSOP_NOP || !pn2->isConst()))
                    *answer = JS_TRUE;
            }
        } else {
            if (pn->pn_op == JSOP_OR || pn->pn_op == JSOP_AND ||
                pn->pn_op == JSOP_STRICTEQ || pn->pn_op == JSOP_STRICTNE) {
                



                ok = CheckSideEffects(cx, cg, pn->pn_left, answer) &&
                     CheckSideEffects(cx, cg, pn->pn_right, answer);
            } else {
                



                *answer = JS_TRUE;
            }
        }
        break;

      case PN_UNARY:
        switch (pn->pn_type) {
          case TOK_DELETE:
            pn2 = pn->pn_kid;
            switch (pn2->pn_type) {
              case TOK_NAME:
                if (!BindNameToSlot(cx, cg, pn2))
                    return JS_FALSE;
                if (pn2->isConst()) {
                    *answer = JS_FALSE;
                    break;
                }
                
              case TOK_DOT:
#if JS_HAS_XML_SUPPORT
              case TOK_DBLDOT:
#endif
              case TOK_LP:
              case TOK_LB:
                
                *answer = JS_TRUE;
                break;
              default:
                ok = CheckSideEffects(cx, cg, pn2, answer);
                break;
            }
            break;

          case TOK_UNARYOP:
            if (pn->pn_op == JSOP_NOT) {
                
                ok = CheckSideEffects(cx, cg, pn->pn_kid, answer);
                break;
            }
            

          default:
            





            *answer = JS_TRUE;
            break;
        }
        break;

      case PN_NAME:
        




        if (pn->pn_type == TOK_NAME && pn->pn_op != JSOP_NOP) {
            if (!BindNameToSlot(cx, cg, pn))
                return JS_FALSE;
            if (pn->pn_op != JSOP_ARGUMENTS && pn->pn_op != JSOP_CALLEE &&
                pn->pn_cookie == FREE_UPVAR_COOKIE) {
                




                *answer = JS_TRUE;
            }
        }
        pn2 = pn->maybeExpr();
        if (pn->pn_type == TOK_DOT) {
            if (pn2->pn_type == TOK_NAME && !BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            if (!(pn2->pn_op == JSOP_ARGUMENTS &&
                  pn->pn_atom == cx->runtime->atomState.lengthAtom)) {
                



                *answer = JS_TRUE;
            }
        }
        ok = CheckSideEffects(cx, cg, pn2, answer);
        break;

      case PN_NAMESET:
        ok = CheckSideEffects(cx, cg, pn->pn_tree, answer);
        break;

      case PN_NULLARY:
        if (pn->pn_type == TOK_DEBUGGER)
            *answer = JS_TRUE;
        break;
    }
    return ok;
}

static JSBool
EmitNameOp(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn,
           JSBool callContext)
{
    JSOp op;

    if (!BindNameToSlot(cx, cg, pn))
        return JS_FALSE;
    op = PN_OP(pn);

    if (callContext) {
        switch (op) {
          case JSOP_NAME:
            op = JSOP_CALLNAME;
            break;
          case JSOP_GETGVAR:
            op = JSOP_CALLGVAR;
            break;
          case JSOP_GETARG:
            op = JSOP_CALLARG;
            break;
          case JSOP_GETLOCAL:
            op = JSOP_CALLLOCAL;
            break;
          case JSOP_GETUPVAR:
            op = JSOP_CALLUPVAR;
            break;
          case JSOP_GETDSLOT:
            op = JSOP_CALLDSLOT;
            break;
          default:
            JS_ASSERT(op == JSOP_ARGUMENTS || op == JSOP_CALLEE);
            break;
        }
    }

    if (op == JSOP_ARGUMENTS || op == JSOP_CALLEE) {
        if (js_Emit1(cx, cg, op) < 0)
            return JS_FALSE;
        if (callContext && js_Emit1(cx, cg, JSOP_NULL) < 0)
            return JS_FALSE;
    } else {
        if (pn->pn_cookie != FREE_UPVAR_COOKIE) {
            EMIT_UINT16_IMM_OP(op, pn->pn_cookie);
        } else {
            if (!EmitAtomOp(cx, pn, op, cg))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
}

#if JS_HAS_XML_SUPPORT
static JSBool
EmitXMLName(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    JSParseNode *pn2;
    uintN oldflags;

    JS_ASSERT(pn->pn_type == TOK_UNARYOP);
    JS_ASSERT(pn->pn_op == JSOP_XMLNAME);
    JS_ASSERT(op == JSOP_XMLNAME || op == JSOP_CALLXMLNAME);

    pn2 = pn->pn_kid;
    oldflags = cg->flags;
    cg->flags &= ~TCF_IN_FOR_INIT;
    if (!js_EmitTree(cx, cg, pn2))
        return JS_FALSE;
    cg->flags |= oldflags & TCF_IN_FOR_INIT;
    if (js_NewSrcNote2(cx, cg, SRC_PCBASE,
                       CG_OFFSET(cg) - pn2->pn_offset) < 0) {
        return JS_FALSE;
    }

    return js_Emit1(cx, cg, op) >= 0;
}
#endif

static JSBool
EmitSpecialPropOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    





    JSAtomListElement *ale = cg->atomList.add(cg->compiler, pn->pn_atom);
    if (!ale)
        return JS_FALSE;
    if (!EmitIndexOp(cx, JSOP_QNAMEPART, ALE_INDEX(ale), cg))
        return JS_FALSE;
    if (js_Emit1(cx, cg, op) < 0)
        return JS_FALSE;
    return JS_TRUE;
}

static JSBool
EmitPropOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg,
           JSBool callContext)
{
    JSParseNode *pn2, *pndot, *pnup, *pndown;
    ptrdiff_t top;

    JS_ASSERT(pn->pn_arity == PN_NAME);
    pn2 = pn->maybeExpr();

    
    if ((op == JSOP_GETPROP || op == JSOP_CALLPROP) &&
        (pn->pn_atom == cx->runtime->atomState.protoAtom ||
         pn->pn_atom == cx->runtime->atomState.parentAtom ||
         pn->pn_atom == cx->runtime->atomState.countAtom)) {
        if (pn2 && !js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
        return EmitSpecialPropOp(cx, pn, callContext ? JSOP_CALLELEM : JSOP_GETELEM, cg);
    }

    if (callContext) {
        JS_ASSERT(pn->pn_type == TOK_DOT);
        JS_ASSERT(op == JSOP_GETPROP);
        op = JSOP_CALLPROP;
    } else if (op == JSOP_GETPROP && pn->pn_type == TOK_DOT) {
        if (pn2->pn_op == JSOP_THIS) {
            if (pn->pn_atom != cx->runtime->atomState.lengthAtom) {
                
                return EmitAtomOp(cx, pn, JSOP_GETTHISPROP, cg);
            }
        } else if (pn2->pn_type == TOK_NAME) {
            







            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            if (pn->pn_atom == cx->runtime->atomState.lengthAtom) {
                if (pn2->pn_op == JSOP_ARGUMENTS)
                    return js_Emit1(cx, cg, JSOP_ARGCNT) >= 0;
            } else {
                switch (pn2->pn_op) {
                  case JSOP_GETARG:
                    op = JSOP_GETARGPROP;
                    goto do_indexconst;
                  case JSOP_GETLOCAL:
                    op = JSOP_GETLOCALPROP;
                  do_indexconst: {
                        JSAtomListElement *ale;
                        jsatomid atomIndex;

                        ale = cg->atomList.add(cg->compiler, pn->pn_atom);
                        if (!ale)
                            return JS_FALSE;
                        atomIndex = ALE_INDEX(ale);
                        return EmitSlotIndexOp(cx, op, pn2->pn_cookie, atomIndex, cg);
                    }

                  default:;
                }
            }
        }
    }

    




    if (pn2->pn_type == TOK_DOT) {
        pndot = pn2;
        pnup = NULL;
        top = CG_OFFSET(cg);
        for (;;) {
            
            pndot->pn_offset = top;
            JS_ASSERT(!pndot->pn_used);
            pndown = pndot->pn_expr;
            pndot->pn_expr = pnup;
            if (pndown->pn_type != TOK_DOT)
                break;
            pnup = pndot;
            pndot = pndown;
        }

        
        if (!js_EmitTree(cx, cg, pndown))
            return JS_FALSE;

        do {
            
            if (js_NewSrcNote2(cx, cg, SRC_PCBASE,
                               CG_OFFSET(cg) - pndown->pn_offset) < 0) {
                return JS_FALSE;
            }

            



            if (pndot->pn_arity == PN_NAME &&
                (pndot->pn_atom == cx->runtime->atomState.protoAtom ||
                 pndot->pn_atom == cx->runtime->atomState.parentAtom ||
                 pndot->pn_atom == cx->runtime->atomState.countAtom)) {
                if (!EmitSpecialPropOp(cx, pndot, JSOP_GETELEM, cg))
                    return JS_FALSE;
            } else if (!EmitAtomOp(cx, pndot, PN_OP(pndot), cg)) {
                return JS_FALSE;
            }

            
            pnup = pndot->pn_expr;
            pndot->pn_expr = pndown;
            pndown = pndot;
        } while ((pndot = pnup) != NULL);
    } else {
        if (!js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
    }

    if (js_NewSrcNote2(cx, cg, SRC_PCBASE,
                       CG_OFFSET(cg) - pn2->pn_offset) < 0) {
        return JS_FALSE;
    }

    return EmitAtomOp(cx, pn, op, cg);
}

static JSBool
EmitElemOp(JSContext *cx, JSParseNode *pn, JSOp op, JSCodeGenerator *cg)
{
    ptrdiff_t top;
    JSParseNode *left, *right, *next, ltmp, rtmp;
    jsint slot;

    top = CG_OFFSET(cg);
    if (pn->pn_arity == PN_LIST) {
        
        JS_ASSERT(pn->pn_op == JSOP_GETELEM);
        JS_ASSERT(pn->pn_count >= 3);
        left = pn->pn_head;
        right = pn->last();
        next = left->pn_next;
        JS_ASSERT(next != right);

        



        if (left->pn_type == TOK_NAME && next->pn_type == TOK_NUMBER) {
            if (!BindNameToSlot(cx, cg, left))
                return JS_FALSE;
            if (left->pn_op == JSOP_ARGUMENTS &&
                JSDOUBLE_IS_INT(next->pn_dval, slot) &&
                (jsuint)slot < JS_BIT(16)) {
                



                JS_ASSERT(op != JSOP_CALLELEM || next->pn_next);
                left->pn_offset = next->pn_offset = top;
                EMIT_UINT16_IMM_OP(JSOP_ARGSUB, (jsatomid)slot);
                left = next;
                next = left->pn_next;
            }
        }

        






        JS_ASSERT(next != right || pn->pn_count == 3);
        if (left == pn->pn_head) {
            if (!js_EmitTree(cx, cg, left))
                return JS_FALSE;
        }
        while (next != right) {
            if (!js_EmitTree(cx, cg, next))
                return JS_FALSE;
            if (js_NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_GETELEM) < 0)
                return JS_FALSE;
            next = next->pn_next;
        }
    } else {
        if (pn->pn_arity == PN_NAME) {
            






            left = pn->maybeExpr();
            if (!left) {
                left = &ltmp;
                left->pn_type = TOK_STRING;
                left->pn_op = JSOP_BINDNAME;
                left->pn_arity = PN_NULLARY;
                left->pn_pos = pn->pn_pos;
                left->pn_atom = pn->pn_atom;
            }
            right = &rtmp;
            right->pn_type = TOK_STRING;
            JS_ASSERT(ATOM_IS_STRING(pn->pn_atom));
            right->pn_op = js_IsIdentifier(ATOM_TO_STRING(pn->pn_atom))
                           ? JSOP_QNAMEPART
                           : JSOP_STRING;
            right->pn_arity = PN_NULLARY;
            right->pn_pos = pn->pn_pos;
            right->pn_atom = pn->pn_atom;
        } else {
            JS_ASSERT(pn->pn_arity == PN_BINARY);
            left = pn->pn_left;
            right = pn->pn_right;
        }

        
        if (op == JSOP_GETELEM &&
            left->pn_type == TOK_NAME &&
            right->pn_type == TOK_NUMBER) {
            if (!BindNameToSlot(cx, cg, left))
                return JS_FALSE;
            if (left->pn_op == JSOP_ARGUMENTS &&
                JSDOUBLE_IS_INT(right->pn_dval, slot) &&
                (jsuint)slot < JS_BIT(16)) {
                left->pn_offset = right->pn_offset = top;
                EMIT_UINT16_IMM_OP(JSOP_ARGSUB, (jsatomid)slot);
                return JS_TRUE;
            }
        }

        if (!js_EmitTree(cx, cg, left))
            return JS_FALSE;
    }

    
    JS_ASSERT(op != JSOP_DESCENDANTS || right->pn_type != TOK_STRING ||
              right->pn_op == JSOP_QNAMEPART);
    if (!js_EmitTree(cx, cg, right))
        return JS_FALSE;
    if (js_NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
        return JS_FALSE;
    return js_Emit1(cx, cg, op) >= 0;
}

static JSBool
EmitNumberOp(JSContext *cx, jsdouble dval, JSCodeGenerator *cg)
{
    jsint ival;
    uint32 u;
    ptrdiff_t off;
    jsbytecode *pc;
    JSAtom *atom;
    JSAtomListElement *ale;

    if (JSDOUBLE_IS_INT(dval, ival) && INT_FITS_IN_JSVAL(ival)) {
        if (ival == 0)
            return js_Emit1(cx, cg, JSOP_ZERO) >= 0;
        if (ival == 1)
            return js_Emit1(cx, cg, JSOP_ONE) >= 0;
        if ((jsint)(int8)ival == ival)
            return js_Emit2(cx, cg, JSOP_INT8, (jsbytecode)(int8)ival) >= 0;

        u = (uint32)ival;
        if (u < JS_BIT(16)) {
            EMIT_UINT16_IMM_OP(JSOP_UINT16, u);
        } else if (u < JS_BIT(24)) {
            off = js_EmitN(cx, cg, JSOP_UINT24, 3);
            if (off < 0)
                return JS_FALSE;
            pc = CG_CODE(cg, off);
            SET_UINT24(pc, u);
        } else {
            off = js_EmitN(cx, cg, JSOP_INT32, 4);
            if (off < 0)
                return JS_FALSE;
            pc = CG_CODE(cg, off);
            SET_INT32(pc, ival);
        }
        return JS_TRUE;
    }

    atom = js_AtomizeDouble(cx, dval);
    if (!atom)
        return JS_FALSE;

    ale = cg->atomList.add(cg->compiler, atom);
    if (!ale)
        return JS_FALSE;
    return EmitIndexOp(cx, JSOP_DOUBLE, ALE_INDEX(ale), cg);
}

static JSBool
EmitSwitch(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn,
           JSStmtInfo *stmtInfo)
{
    JSOp switchOp;
    JSBool ok, hasDefault, constPropagated;
    ptrdiff_t top, off, defaultOffset;
    JSParseNode *pn2, *pn3, *pn4;
    uint32 caseCount, tableLength;
    JSParseNode **table;
    jsdouble d;
    jsint i, low, high;
    jsval v;
    JSAtom *atom;
    JSAtomListElement *ale;
    intN noteIndex;
    size_t switchSize, tableSize;
    jsbytecode *pc, *savepc;
#if JS_HAS_BLOCK_SCOPE
    jsint count;
#endif

    
    switchOp = JSOP_TABLESWITCH;
    ok = JS_TRUE;
    hasDefault = constPropagated = JS_FALSE;
    defaultOffset = -1;

    





    pn2 = pn->pn_right;
#if JS_HAS_BLOCK_SCOPE
    if (pn2->pn_type == TOK_LEXICALSCOPE) {
        






        count = OBJ_BLOCK_COUNT(cx, pn2->pn_objbox->object);
        js_PushBlockScope(cg, stmtInfo, pn2->pn_objbox->object, -1);
        stmtInfo->type = STMT_SWITCH;

        
        if (!EmitEnterBlock(cx, pn2, cg))
            return JS_FALSE;

        





        cg->topStmt = stmtInfo->down;
        cg->topScopeStmt = stmtInfo->downScope;
    }
#ifdef __GNUC__
    else {
        count = 0;
    }
#endif
#endif

    



    if (!js_EmitTree(cx, cg, pn->pn_left))
        return JS_FALSE;

    
    top = CG_OFFSET(cg);
#if !JS_HAS_BLOCK_SCOPE
    js_PushStatement(cg, stmtInfo, STMT_SWITCH, top);
#else
    if (pn2->pn_type == TOK_LC) {
        js_PushStatement(cg, stmtInfo, STMT_SWITCH, top);
    } else {
        
        cg->topStmt = cg->topScopeStmt = stmtInfo;

        
        stmtInfo->update = top;

        
        pn2 = pn2->expr();
    }
#endif

    caseCount = pn2->pn_count;
    tableLength = 0;
    table = NULL;

    if (caseCount == 0 ||
        (caseCount == 1 &&
         (hasDefault = (pn2->pn_head->pn_type == TOK_DEFAULT)))) {
        caseCount = 0;
        low = 0;
        high = -1;
    } else {
#define INTMAP_LENGTH   256
        jsbitmap intmap_space[INTMAP_LENGTH];
        jsbitmap *intmap = NULL;
        int32 intmap_bitlen = 0;

        low  = JSVAL_INT_MAX;
        high = JSVAL_INT_MIN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->pn_type == TOK_DEFAULT) {
                hasDefault = JS_TRUE;
                caseCount--;    
                continue;
            }

            JS_ASSERT(pn3->pn_type == TOK_CASE);
            if (switchOp == JSOP_CONDSWITCH)
                continue;

            pn4 = pn3->pn_left;
            while (pn4->pn_type == TOK_RP)
                pn4 = pn4->pn_kid;
            switch (pn4->pn_type) {
              case TOK_NUMBER:
                d = pn4->pn_dval;
                if (JSDOUBLE_IS_INT(d, i) && INT_FITS_IN_JSVAL(i)) {
                    pn3->pn_val = INT_TO_JSVAL(i);
                } else {
                    atom = js_AtomizeDouble(cx, d);
                    if (!atom) {
                        ok = JS_FALSE;
                        goto release;
                    }
                    pn3->pn_val = ATOM_KEY(atom);
                }
                break;
              case TOK_STRING:
                pn3->pn_val = ATOM_KEY(pn4->pn_atom);
                break;
              case TOK_NAME:
                if (!pn4->maybeExpr()) {
                    ok = LookupCompileTimeConstant(cx, cg, pn4->pn_atom, &v);
                    if (!ok)
                        goto release;
                    if (v != JSVAL_HOLE) {
                        if (!JSVAL_IS_PRIMITIVE(v)) {
                            



                            switchOp = JSOP_CONDSWITCH;
                            continue;
                        }
                        pn3->pn_val = v;
                        constPropagated = JS_TRUE;
                        break;
                    }
                }
                
              case TOK_PRIMARY:
                if (pn4->pn_op == JSOP_TRUE) {
                    pn3->pn_val = JSVAL_TRUE;
                    break;
                }
                if (pn4->pn_op == JSOP_FALSE) {
                    pn3->pn_val = JSVAL_FALSE;
                    break;
                }
                
              default:
                switchOp = JSOP_CONDSWITCH;
                continue;
            }

            JS_ASSERT(JSVAL_IS_PRIMITIVE(pn3->pn_val));

            if (switchOp != JSOP_TABLESWITCH)
                continue;
            if (!JSVAL_IS_INT(pn3->pn_val)) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            i = JSVAL_TO_INT(pn3->pn_val);
            if ((jsuint)(i + (jsint)JS_BIT(15)) >= (jsuint)JS_BIT(16)) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            if (i < low)
                low = i;
            if (high < i)
                high = i;

            




            if (i < 0)
                i += JS_BIT(16);
            if (i >= intmap_bitlen) {
                if (!intmap &&
                    i < (INTMAP_LENGTH << JS_BITS_PER_WORD_LOG2)) {
                    intmap = intmap_space;
                    intmap_bitlen = INTMAP_LENGTH << JS_BITS_PER_WORD_LOG2;
                } else {
                    
                    intmap_bitlen = JS_BIT(16);
                    intmap = (jsbitmap *)
                        cx->malloc((JS_BIT(16) >> JS_BITS_PER_WORD_LOG2)
                                   * sizeof(jsbitmap));
                    if (!intmap) {
                        JS_ReportOutOfMemory(cx);
                        return JS_FALSE;
                    }
                }
                memset(intmap, 0, intmap_bitlen >> JS_BITS_PER_BYTE_LOG2);
            }
            if (JS_TEST_BIT(intmap, i)) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            JS_SET_BIT(intmap, i);
        }

      release:
        if (intmap && intmap != intmap_space)
            cx->free(intmap);
        if (!ok)
            return JS_FALSE;

        



        if (switchOp == JSOP_TABLESWITCH) {
            tableLength = (uint32)(high - low + 1);
            if (tableLength >= JS_BIT(16) || tableLength > 2 * caseCount)
                switchOp = JSOP_LOOKUPSWITCH;
        } else if (switchOp == JSOP_LOOKUPSWITCH) {
            





            if (caseCount + cg->atomList.count > JS_BIT(16))
                switchOp = JSOP_CONDSWITCH;
        }
    }

    



    noteIndex = js_NewSrcNote3(cx, cg, SRC_SWITCH, 0, 0);
    if (noteIndex < 0)
        return JS_FALSE;

    if (switchOp == JSOP_CONDSWITCH) {
        


        switchSize = 0;
    } else if (switchOp == JSOP_TABLESWITCH) {
        


        switchSize = (size_t)(JUMP_OFFSET_LEN * (3 + tableLength));
    } else {
        




        switchSize = (size_t)(JUMP_OFFSET_LEN + INDEX_LEN +
                              (INDEX_LEN + JUMP_OFFSET_LEN) * caseCount);
    }

    










    if (js_EmitN(cx, cg, switchOp, switchSize) < 0)
        return JS_FALSE;

    off = -1;
    if (switchOp == JSOP_CONDSWITCH) {
        intN caseNoteIndex = -1;
        JSBool beforeCases = JS_TRUE;

        
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            pn4 = pn3->pn_left;
            if (pn4 && !js_EmitTree(cx, cg, pn4))
                return JS_FALSE;
            if (caseNoteIndex >= 0) {
                
                if (!js_SetSrcNoteOffset(cx, cg, (uintN)caseNoteIndex, 0,
                                         CG_OFFSET(cg) - off)) {
                    return JS_FALSE;
                }
            }
            if (!pn4) {
                JS_ASSERT(pn3->pn_type == TOK_DEFAULT);
                continue;
            }
            caseNoteIndex = js_NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
            if (caseNoteIndex < 0)
                return JS_FALSE;
            off = EmitJump(cx, cg, JSOP_CASE, 0);
            if (off < 0)
                return JS_FALSE;
            pn3->pn_offset = off;
            if (beforeCases) {
                uintN noteCount, noteCountDelta;

                
                noteCount = CG_NOTE_COUNT(cg);
                if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 1,
                                         off - top)) {
                    return JS_FALSE;
                }
                noteCountDelta = CG_NOTE_COUNT(cg) - noteCount;
                if (noteCountDelta != 0)
                    caseNoteIndex += noteCountDelta;
                beforeCases = JS_FALSE;
            }
        }

        





        if (!hasDefault &&
            caseNoteIndex >= 0 &&
            !js_SetSrcNoteOffset(cx, cg, (uintN)caseNoteIndex, 0,
                                 CG_OFFSET(cg) - off)) {
            return JS_FALSE;
        }

        
        defaultOffset = EmitJump(cx, cg, JSOP_DEFAULT, 0);
        if (defaultOffset < 0)
            return JS_FALSE;
    } else {
        pc = CG_CODE(cg, top + JUMP_OFFSET_LEN);

        if (switchOp == JSOP_TABLESWITCH) {
            
            SET_JUMP_OFFSET(pc, low);
            pc += JUMP_OFFSET_LEN;
            SET_JUMP_OFFSET(pc, high);
            pc += JUMP_OFFSET_LEN;

            




            if (tableLength != 0) {
                tableSize = (size_t)tableLength * sizeof *table;
                table = (JSParseNode **) cx->malloc(tableSize);
                if (!table)
                    return JS_FALSE;
                memset(table, 0, tableSize);
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    if (pn3->pn_type == TOK_DEFAULT)
                        continue;
                    i = JSVAL_TO_INT(pn3->pn_val);
                    i -= low;
                    JS_ASSERT((uint32)i < tableLength);
                    table[i] = pn3;
                }
            }
        } else {
            JS_ASSERT(switchOp == JSOP_LOOKUPSWITCH);

            
            SET_INDEX(pc, caseCount);
            pc += INDEX_LEN;
        }

        




        MUST_FLOW_THROUGH("out");
        if (cg->spanDeps) {
            





            if (!AddSwitchSpanDeps(cx, cg, CG_CODE(cg, top)))
                goto bad;
        }

        if (constPropagated) {
            




            savepc = CG_NEXT(cg);
            CG_NEXT(cg) = pc + 1;
            if (switchOp == JSOP_TABLESWITCH) {
                for (i = 0; i < (jsint)tableLength; i++) {
                    pn3 = table[i];
                    if (pn3 &&
                        (pn4 = pn3->pn_left) != NULL &&
                        pn4->pn_type == TOK_NAME) {
                        
                        JS_ASSERT(!pn4->maybeExpr());
                        ale = cg->atomList.add(cg->compiler, pn4->pn_atom);
                        if (!ale)
                            goto bad;
                        CG_NEXT(cg) = pc;
                        if (js_NewSrcNote2(cx, cg, SRC_LABEL, (ptrdiff_t)
                                           ALE_INDEX(ale)) < 0) {
                            goto bad;
                        }
                    }
                    pc += JUMP_OFFSET_LEN;
                }
            } else {
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    pn4 = pn3->pn_left;
                    if (pn4 && pn4->pn_type == TOK_NAME) {
                        
                        JS_ASSERT(!pn4->maybeExpr());
                        ale = cg->atomList.add(cg->compiler, pn4->pn_atom);
                        if (!ale)
                            goto bad;
                        CG_NEXT(cg) = pc;
                        if (js_NewSrcNote2(cx, cg, SRC_LABEL, (ptrdiff_t)
                                           ALE_INDEX(ale)) < 0) {
                            goto bad;
                        }
                    }
                    pc += INDEX_LEN + JUMP_OFFSET_LEN;
                }
            }
            CG_NEXT(cg) = savepc;
        }
    }

    
    for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
        if (switchOp == JSOP_CONDSWITCH && pn3->pn_type != TOK_DEFAULT)
            CHECK_AND_SET_JUMP_OFFSET_AT_CUSTOM(cx, cg, pn3->pn_offset, goto bad);
        pn4 = pn3->pn_right;
        ok = js_EmitTree(cx, cg, pn4);
        if (!ok)
            goto out;
        pn3->pn_offset = pn4->pn_offset;
        if (pn3->pn_type == TOK_DEFAULT)
            off = pn3->pn_offset - top;
    }

    if (!hasDefault) {
        
        off = CG_OFFSET(cg) - top;
    }

    
    JS_ASSERT(off != -1);

    
    if (switchOp == JSOP_CONDSWITCH) {
        pc = NULL;
        JS_ASSERT(defaultOffset != -1);
        ok = js_SetJumpOffset(cx, cg, CG_CODE(cg, defaultOffset),
                              off - (defaultOffset - top));
        if (!ok)
            goto out;
    } else {
        pc = CG_CODE(cg, top);
        ok = js_SetJumpOffset(cx, cg, pc, off);
        if (!ok)
            goto out;
        pc += JUMP_OFFSET_LEN;
    }

    
    off = CG_OFFSET(cg) - top;
    ok = js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, off);
    if (!ok)
        goto out;

    if (switchOp == JSOP_TABLESWITCH) {
        
        pc += 2 * JUMP_OFFSET_LEN;

        
        for (i = 0; i < (jsint)tableLength; i++) {
            pn3 = table[i];
            off = pn3 ? pn3->pn_offset - top : 0;
            ok = js_SetJumpOffset(cx, cg, pc, off);
            if (!ok)
                goto out;
            pc += JUMP_OFFSET_LEN;
        }
    } else if (switchOp == JSOP_LOOKUPSWITCH) {
        
        pc += INDEX_LEN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->pn_type == TOK_DEFAULT)
                continue;
            if (!js_AtomizePrimitiveValue(cx, pn3->pn_val, &atom))
                goto bad;
            ale = cg->atomList.add(cg->compiler, atom);
            if (!ale)
                goto bad;
            SET_INDEX(pc, ALE_INDEX(ale));
            pc += INDEX_LEN;

            off = pn3->pn_offset - top;
            ok = js_SetJumpOffset(cx, cg, pc, off);
            if (!ok)
                goto out;
            pc += JUMP_OFFSET_LEN;
        }
    }

out:
    if (table)
        cx->free(table);
    if (ok) {
        ok = js_PopStatementCG(cx, cg);

#if JS_HAS_BLOCK_SCOPE
        if (ok && pn->pn_right->pn_type == TOK_LEXICALSCOPE)
            EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, count);
#endif
    }
    return ok;

bad:
    ok = JS_FALSE;
    goto out;
}

JSBool
js_EmitFunctionScript(JSContext *cx, JSCodeGenerator *cg, JSParseNode *body)
{
    if (cg->flags & TCF_FUN_IS_GENERATOR) {
        
        CG_SWITCH_TO_PROLOG(cg);
        JS_ASSERT(CG_NEXT(cg) == CG_BASE(cg));
        if (js_Emit1(cx, cg, JSOP_GENERATOR) < 0)
            return JS_FALSE;
        CG_SWITCH_TO_MAIN(cg);
    } else {
        



        if (js_Emit1(cx, cg, JSOP_TRACE) < 0)
            return JS_FALSE;
    }

    return js_EmitTree(cx, cg, body) &&
           js_Emit1(cx, cg, JSOP_STOP) >= 0 &&
           js_NewScriptFromCG(cx, cg);
}


#define UPDATE_LINE_NUMBER_NOTES(cx, cg, line)                                \
    JS_BEGIN_MACRO                                                            \
        uintN line_ = (line);                                                 \
        uintN delta_ = line_ - CG_CURRENT_LINE(cg);                           \
        if (delta_ != 0) {                                                    \
            /*                                                                \
             * Encode any change in the current source line number by using   \
             * either several SRC_NEWLINE notes or just one SRC_SETLINE note, \
             * whichever consumes less space.                                 \
             *                                                                \
             * NB: We handle backward line number deltas (possible with for   \
             * loops where the update part is emitted after the body, but its \
             * line number is <= any line number in the body) here by letting \
             * unsigned delta_ wrap to a very large number, which triggers a  \
             * SRC_SETLINE.                                                   \
             */                                                               \
            CG_CURRENT_LINE(cg) = line_;                                      \
            if (delta_ >= (uintN)(2 + ((line_ > SN_3BYTE_OFFSET_MASK)<<1))) { \
                if (js_NewSrcNote2(cx, cg, SRC_SETLINE, (ptrdiff_t)line_) < 0)\
                    return JS_FALSE;                                          \
            } else {                                                          \
                do {                                                          \
                    if (js_NewSrcNote(cx, cg, SRC_NEWLINE) < 0)               \
                        return JS_FALSE;                                      \
                } while (--delta_ != 0);                                      \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO


static JSBool
UpdateLineNumberNotes(JSContext *cx, JSCodeGenerator *cg, uintN line)
{
    UPDATE_LINE_NUMBER_NOTES(cx, cg, line);
    return JS_TRUE;
}

static JSBool
MaybeEmitVarDecl(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                 JSParseNode *pn, jsatomid *result)
{
    jsatomid atomIndex;
    JSAtomListElement *ale;

    if (pn->pn_cookie != FREE_UPVAR_COOKIE) {
        atomIndex = (jsatomid) UPVAR_FRAME_SLOT(pn->pn_cookie);
    } else {
        ale = cg->atomList.add(cg->compiler, pn->pn_atom);
        if (!ale)
            return JS_FALSE;
        atomIndex = ALE_INDEX(ale);
    }

    if (JOF_OPTYPE(pn->pn_op) == JOF_ATOM &&
        (!(cg->flags & TCF_IN_FUNCTION) || (cg->flags & TCF_FUN_HEAVYWEIGHT))) {
        CG_SWITCH_TO_PROLOG(cg);
        if (!UpdateLineNumberNotes(cx, cg, pn->pn_pos.begin.lineno))
            return JS_FALSE;
        EMIT_INDEX_OP(prologOp, atomIndex);
        CG_SWITCH_TO_MAIN(cg);
    }

    if (result)
        *result = atomIndex;
    return JS_TRUE;
}

#if JS_HAS_DESTRUCTURING

typedef JSBool
(*DestructuringDeclEmitter)(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                            JSParseNode *pn);

static JSBool
EmitDestructuringDecl(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                      JSParseNode *pn)
{
    JS_ASSERT(pn->pn_type == TOK_NAME);
    if (!BindNameToSlot(cx, cg, pn))
        return JS_FALSE;

    JS_ASSERT(PN_OP(pn) != JSOP_ARGUMENTS && PN_OP(pn) != JSOP_CALLEE);
    return MaybeEmitVarDecl(cx, cg, prologOp, pn, NULL);
}

static JSBool
EmitDestructuringDecls(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                       JSParseNode *pn)
{
    JSParseNode *pn2, *pn3;
    DestructuringDeclEmitter emitter;

    if (pn->pn_type == TOK_RB) {
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->pn_type == TOK_COMMA)
                continue;
            emitter = (pn2->pn_type == TOK_NAME)
                      ? EmitDestructuringDecl
                      : EmitDestructuringDecls;
            if (!emitter(cx, cg, prologOp, pn2))
                return JS_FALSE;
        }
    } else {
        JS_ASSERT(pn->pn_type == TOK_RC);
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            pn3 = pn2->pn_right;
            emitter = (pn3->pn_type == TOK_NAME)
                      ? EmitDestructuringDecl
                      : EmitDestructuringDecls;
            if (!emitter(cx, cg, prologOp, pn3))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
EmitDestructuringOpsHelper(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn);

static JSBool
EmitDestructuringLHS(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn)
{
    jsuint slot;

    





    if (pn->pn_type == TOK_RB || pn->pn_type == TOK_RC) {
        if (!EmitDestructuringOpsHelper(cx, cg, pn))
            return JS_FALSE;
        if (js_Emit1(cx, cg, JSOP_POP) < 0)
            return JS_FALSE;
    } else {
        if (pn->pn_type == TOK_NAME) {
            if (!BindNameToSlot(cx, cg, pn))
                return JS_FALSE;
            if (pn->isConst() && !pn->isInitialized())
                return js_Emit1(cx, cg, JSOP_POP) >= 0;
        }

        switch (pn->pn_op) {
          case JSOP_SETNAME:
            




            if (!EmitElemOp(cx, pn, JSOP_ENUMELEM, cg))
                return JS_FALSE;
            break;

          case JSOP_SETCONST:
            if (!EmitElemOp(cx, pn, JSOP_ENUMCONSTELEM, cg))
                return JS_FALSE;
            break;

          case JSOP_SETLOCAL:
            slot = (jsuint) pn->pn_cookie;
            EMIT_UINT16_IMM_OP(JSOP_SETLOCALPOP, slot);
            break;

          case JSOP_SETARG:
          case JSOP_SETGVAR:
            slot = (jsuint) pn->pn_cookie;
            EMIT_UINT16_IMM_OP(PN_OP(pn), slot);
            if (js_Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
            break;

          default:
          {
            ptrdiff_t top;

            top = CG_OFFSET(cg);
            if (!js_EmitTree(cx, cg, pn))
                return JS_FALSE;
            if (js_NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_ENUMELEM) < 0)
                return JS_FALSE;
            break;
          }

          case JSOP_ENUMELEM:
            JS_ASSERT(0);
        }
    }

    return JS_TRUE;
}








static JSBool
EmitDestructuringOpsHelper(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn)
{
    jsuint index;
    JSParseNode *pn2, *pn3;
    JSBool doElemOp;

#ifdef DEBUG
    intN stackDepth = cg->stackDepth;
    JS_ASSERT(stackDepth != 0);
    JS_ASSERT(pn->pn_arity == PN_LIST);
    JS_ASSERT(pn->pn_type == TOK_RB || pn->pn_type == TOK_RC);
#endif

    if (pn->pn_count == 0) {
        
        return js_Emit1(cx, cg, JSOP_DUP) >= 0 &&
               js_Emit1(cx, cg, JSOP_POP) >= 0;
    }

    index = 0;
    for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
        



        if (pn2 != pn->pn_head && js_NewSrcNote(cx, cg, SRC_CONTINUE) < 0)
            return JS_FALSE;
        if (js_Emit1(cx, cg, JSOP_DUP) < 0)
            return JS_FALSE;

        





        doElemOp = JS_TRUE;
        if (pn->pn_type == TOK_RB) {
            if (!EmitNumberOp(cx, index, cg))
                return JS_FALSE;
            pn3 = pn2;
        } else {
            JS_ASSERT(pn->pn_type == TOK_RC);
            JS_ASSERT(pn2->pn_type == TOK_COLON);
            pn3 = pn2->pn_left;
            if (pn3->pn_type == TOK_NUMBER) {
                




                if (js_NewSrcNote(cx, cg, SRC_INITPROP) < 0)
                    return JS_FALSE;
                if (!EmitNumberOp(cx, pn3->pn_dval, cg))
                    return JS_FALSE;
            } else {
                JS_ASSERT(pn3->pn_type == TOK_STRING ||
                          pn3->pn_type == TOK_NAME);
                if (!EmitAtomOp(cx, pn3, JSOP_GETPROP, cg))
                    return JS_FALSE;
                doElemOp = JS_FALSE;
            }
            pn3 = pn2->pn_right;
        }

        if (doElemOp) {
            




            if (js_Emit1(cx, cg, JSOP_GETELEM) < 0)
                return JS_FALSE;
            JS_ASSERT(cg->stackDepth == stackDepth + 1);
        }

        
        if (pn3->pn_type == TOK_COMMA && pn3->pn_arity == PN_NULLARY) {
            JS_ASSERT(pn->pn_type == TOK_RB);
            JS_ASSERT(pn2 == pn3);
            if (js_Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
        } else {
            if (!EmitDestructuringLHS(cx, cg, pn3))
                return JS_FALSE;
        }

        JS_ASSERT(cg->stackDepth == stackDepth);
        ++index;
    }

    return JS_TRUE;
}

static ptrdiff_t
OpToDeclType(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return SRC_DECL_LET;
      case JSOP_DEFCONST:
        return SRC_DECL_CONST;
      case JSOP_DEFVAR:
        return SRC_DECL_VAR;
      default:
        return SRC_DECL_NONE;
    }
}

static JSBool
EmitDestructuringOps(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                     JSParseNode *pn)
{
    





    if (js_NewSrcNote2(cx, cg, SRC_DESTRUCT, OpToDeclType(prologOp)) < 0)
        return JS_FALSE;

    



    return EmitDestructuringOpsHelper(cx, cg, pn);
}

static JSBool
EmitGroupAssignment(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                    JSParseNode *lhs, JSParseNode *rhs)
{
    jsuint depth, limit, i, nslots;
    JSParseNode *pn;

    depth = limit = (uintN) cg->stackDepth;
    for (pn = rhs->pn_head; pn; pn = pn->pn_next) {
        if (limit == JS_BIT(16)) {
            js_ReportCompileErrorNumber(cx, CG_TS(cg), rhs, JSREPORT_ERROR,
                                        JSMSG_ARRAY_INIT_TOO_BIG);
            return JS_FALSE;
        }

        
        JS_ASSERT(!(pn->pn_type == TOK_COMMA && pn->pn_arity == PN_NULLARY));
        if (!js_EmitTree(cx, cg, pn))
            return JS_FALSE;
        ++limit;
    }

    if (js_NewSrcNote2(cx, cg, SRC_GROUPASSIGN, OpToDeclType(prologOp)) < 0)
        return JS_FALSE;

    i = depth;
    for (pn = lhs->pn_head; pn; pn = pn->pn_next, ++i) {
        
        JS_ASSERT(i < limit);
        jsint slot = AdjustBlockSlot(cx, cg, i);
        if (slot < 0)
            return JS_FALSE;
        EMIT_UINT16_IMM_OP(JSOP_GETLOCAL, slot);

        if (pn->pn_type == TOK_COMMA && pn->pn_arity == PN_NULLARY) {
            if (js_Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
        } else {
            if (!EmitDestructuringLHS(cx, cg, pn))
                return JS_FALSE;
        }
    }

    nslots = limit - depth;
    EMIT_UINT16_IMM_OP(JSOP_POPN, nslots);
    cg->stackDepth = (uintN) depth;
    return JS_TRUE;
}






static JSBool
MaybeEmitGroupAssignment(JSContext *cx, JSCodeGenerator *cg, JSOp prologOp,
                         JSParseNode *pn, JSOp *pop)
{
    JSParseNode *lhs, *rhs;

    JS_ASSERT(pn->pn_type == TOK_ASSIGN);
    JS_ASSERT(*pop == JSOP_POP || *pop == JSOP_POPV);
    lhs = pn->pn_left;
    rhs = pn->pn_right;
    if (lhs->pn_type == TOK_RB && rhs->pn_type == TOK_RB &&
        !(rhs->pn_xflags & PNX_HOLEY) &&
        lhs->pn_count <= rhs->pn_count) {
        if (!EmitGroupAssignment(cx, cg, prologOp, lhs, rhs))
            return JS_FALSE;
        *pop = JSOP_NOP;
    }
    return JS_TRUE;
}

#endif 

static JSBool
EmitVariables(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn,
              JSBool inLetHead, ptrdiff_t *headNoteIndex)
{
    bool let, forInVar, first;
#if JS_HAS_BLOCK_SCOPE
    bool forInLet, popScope;
    JSStmtInfo *stmt, *scopeStmt;
#endif
    ptrdiff_t off, noteIndex, tmp;
    JSParseNode *pn2, *pn3, *next;
    JSOp op;
    jsatomid atomIndex;
    uintN oldflags;

    
    *headNoteIndex = -1;

    











    let = (pn->pn_op == JSOP_NOP);
    forInVar = (pn->pn_xflags & PNX_FORINVAR) != 0;
#if JS_HAS_BLOCK_SCOPE
    forInLet = let && forInVar;
    popScope = (inLetHead || (let && (cg->flags & TCF_IN_FOR_INIT)));
    if (popScope) {
        stmt = cg->topStmt;
        scopeStmt = cg->topScopeStmt;
    }
# ifdef __GNUC__
    else stmt = scopeStmt = NULL;   
# endif
    JS_ASSERT(!popScope || let);
#endif

    off = noteIndex = -1;
    for (pn2 = pn->pn_head; ; pn2 = next) {
        first = pn2 == pn->pn_head;
        next = pn2->pn_next;

        if (pn2->pn_type != TOK_NAME) {
#if JS_HAS_DESTRUCTURING
            if (pn2->pn_type == TOK_RB || pn2->pn_type == TOK_RC) {
                







                JS_ASSERT(forInVar);
                JS_ASSERT(pn->pn_count == 1);
                if (!EmitDestructuringDecls(cx, cg, PN_OP(pn), pn2))
                    return JS_FALSE;
                break;
            }
#endif

            





            JS_ASSERT(pn2->pn_type == TOK_ASSIGN);
            JS_ASSERT(!forInVar);

            




#if !JS_HAS_DESTRUCTURING
            JS_ASSERT(pn2->pn_left->pn_type == TOK_NAME);
#else
            if (pn2->pn_left->pn_type == TOK_NAME)
#endif
            {
                pn3 = pn2->pn_right;
                pn2 = pn2->pn_left;
                goto do_name;
            }

#if JS_HAS_DESTRUCTURING
            if (pn->pn_count == 1) {
                





                JS_ASSERT(noteIndex < 0 && !pn2->pn_next);
                op = JSOP_POP;
                if (!MaybeEmitGroupAssignment(cx, cg,
                                              inLetHead ? JSOP_POP : PN_OP(pn),
                                              pn2, &op)) {
                    return JS_FALSE;
                }
                if (op == JSOP_NOP) {
                    pn->pn_xflags = (pn->pn_xflags & ~PNX_POPVAR) | PNX_GROUPINIT;
                    break;
                }
            }

            pn3 = pn2->pn_left;
            if (!EmitDestructuringDecls(cx, cg, PN_OP(pn), pn3))
                return JS_FALSE;

            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;

            




            if (!EmitDestructuringOps(cx, cg,
                                      inLetHead ? JSOP_POP : PN_OP(pn),
                                      pn3)) {
                return JS_FALSE;
            }
            goto emit_note_pop;
#endif
        }

        





        pn3 = pn2->maybeExpr();

     do_name:
        if (!BindNameToSlot(cx, cg, pn2))
            return JS_FALSE;

        op = PN_OP(pn2);
        if (op == JSOP_ARGUMENTS) {
            
            JS_ASSERT(!pn3 && !let);
            pn3 = NULL;
#ifdef __GNUC__
            atomIndex = 0;            
#endif
        } else {
            JS_ASSERT(op != JSOP_CALLEE);
            JS_ASSERT(pn2->pn_cookie != FREE_UPVAR_COOKIE || !let);
            if (!MaybeEmitVarDecl(cx, cg, PN_OP(pn), pn2, &atomIndex))
                return JS_FALSE;

            if (pn3) {
                JS_ASSERT(!forInVar);
                if (op == JSOP_SETNAME) {
                    JS_ASSERT(!let);
                    EMIT_INDEX_OP(JSOP_BINDNAME, atomIndex);
                }
                if (pn->pn_op == JSOP_DEFCONST &&
                    !js_DefineCompileTimeConstant(cx, cg, pn2->pn_atom, pn3)) {
                    return JS_FALSE;
                }

#if JS_HAS_BLOCK_SCOPE
                
                if (popScope) {
                    cg->topStmt = stmt->down;
                    cg->topScopeStmt = scopeStmt->downScope;
                }
#endif

                oldflags = cg->flags;
                cg->flags &= ~TCF_IN_FOR_INIT;
                if (!js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;
                cg->flags |= oldflags & TCF_IN_FOR_INIT;

#if JS_HAS_BLOCK_SCOPE
                if (popScope) {
                    cg->topStmt = stmt;
                    cg->topScopeStmt = scopeStmt;
                }
#endif
            }
        }

        







        JS_ASSERT_IF(pn2->pn_defn, pn3 == pn2->pn_expr);
        if (forInVar) {
            JS_ASSERT(pn->pn_count == 1);
            JS_ASSERT(!pn3);
            break;
        }

        if (first &&
            !inLetHead &&
            js_NewSrcNote2(cx, cg, SRC_DECL,
                           (pn->pn_op == JSOP_DEFCONST)
                           ? SRC_DECL_CONST
                           : (pn->pn_op == JSOP_DEFVAR)
                           ? SRC_DECL_VAR
                           : SRC_DECL_LET) < 0) {
            return JS_FALSE;
        }
        if (op == JSOP_ARGUMENTS) {
            if (js_Emit1(cx, cg, op) < 0)
                return JS_FALSE;
        } else if (pn2->pn_cookie != FREE_UPVAR_COOKIE) {
            EMIT_UINT16_IMM_OP(op, atomIndex);
        } else {
            EMIT_INDEX_OP(op, atomIndex);
        }

#if JS_HAS_DESTRUCTURING
    emit_note_pop:
#endif
        tmp = CG_OFFSET(cg);
        if (noteIndex >= 0) {
            if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp-off))
                return JS_FALSE;
        }
        if (!next)
            break;
        off = tmp;
        noteIndex = js_NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
        if (noteIndex < 0 || js_Emit1(cx, cg, JSOP_POP) < 0)
            return JS_FALSE;
    }

    
    if (inLetHead) {
        *headNoteIndex = js_NewSrcNote(cx, cg, SRC_DECL);
        if (*headNoteIndex < 0)
            return JS_FALSE;
        if (!(pn->pn_xflags & PNX_POPVAR))
            return js_Emit1(cx, cg, JSOP_NOP) >= 0;
    }

    return !(pn->pn_xflags & PNX_POPVAR) || js_Emit1(cx, cg, JSOP_POP) >= 0;
}

#if defined DEBUG_brendan || defined DEBUG_mrbkap
static JSBool
GettableNoteForNextOp(JSCodeGenerator *cg)
{
    ptrdiff_t offset, target;
    jssrcnote *sn, *end;

    offset = 0;
    target = CG_OFFSET(cg);
    for (sn = CG_NOTES(cg), end = sn + CG_NOTE_COUNT(cg); sn < end;
         sn = SN_NEXT(sn)) {
        if (offset == target && SN_IS_GETTABLE(sn))
            return JS_TRUE;
        offset += SN_DELTA(sn);
    }
    return JS_FALSE;
}
#endif


static JSBool
EmitFunctionDefNop(JSContext *cx, JSCodeGenerator *cg, uintN index)
{
    return js_NewSrcNote2(cx, cg, SRC_FUNCDEF, (ptrdiff_t)index) >= 0 &&
           js_Emit1(cx, cg, JSOP_NOP) >= 0;
}

static bool
EmitNewInit(JSContext *cx, JSCodeGenerator *cg, JSProtoKey key, JSParseNode *pn, int sharpnum)
{
    if (js_Emit2(cx, cg, JSOP_NEWINIT, (jsbytecode) key) < 0)
        return false;
#if JS_HAS_SHARP_VARS
    if (cg->hasSharps()) {
        if (pn->pn_count != 0)
            EMIT_UINT16_IMM_OP(JSOP_SHARPINIT, cg->sharpSlotBase);
        if (sharpnum >= 0)
            EMIT_UINT16PAIR_IMM_OP(JSOP_DEFSHARP, cg->sharpSlotBase, sharpnum);
    } else {
        JS_ASSERT(sharpnum < 0);
    }
#endif
    return true;
}

static bool
EmitEndInit(JSContext *cx, JSCodeGenerator *cg, uint32 count)
{
#if JS_HAS_SHARP_VARS
    
    if (cg->hasSharps() && count != 0)
        EMIT_UINT16_IMM_OP(JSOP_SHARPINIT, cg->sharpSlotBase);
#endif
    return js_Emit1(cx, cg, JSOP_ENDINIT) >= 0;
}


JS_STATIC_ASSERT(JSOP_NOP_LENGTH == 1);
JS_STATIC_ASSERT(JSOP_POP_LENGTH == 1);

JSBool
js_EmitTree(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn)
{
    JSBool ok, useful, wantval;
    JSStmtInfo *stmt, stmtInfo;
    ptrdiff_t top, off, tmp, beq, jmp;
    JSParseNode *pn2, *pn3;
    JSAtom *atom;
    JSAtomListElement *ale;
    jsatomid atomIndex;
    uintN index;
    ptrdiff_t noteIndex;
    JSSrcNoteType noteType;
    jsbytecode *pc;
    JSOp op;
    JSTokenType type;
    uint32 argc;
#if JS_HAS_SHARP_VARS
    jsint sharpnum;
#endif

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    ok = JS_TRUE;
    cg->emitLevel++;
    pn->pn_offset = top = CG_OFFSET(cg);

    
    UPDATE_LINE_NUMBER_NOTES(cx, cg, pn->pn_pos.begin.lineno);

    switch (pn->pn_type) {
      case TOK_FUNCTION:
      {
        JSFunction *fun;
        uintN slot;

#if JS_HAS_XML_SUPPORT
        if (pn->pn_arity == PN_NULLARY) {
            if (js_Emit1(cx, cg, JSOP_GETFUNNS) < 0)
                return JS_FALSE;
            break;
        }
#endif

        fun = (JSFunction *) pn->pn_funbox->object;
        JS_ASSERT(FUN_INTERPRETED(fun));
        if (fun->u.i.script) {
            




            JS_ASSERT(pn->pn_op == JSOP_NOP);
            JS_ASSERT(cg->flags & TCF_IN_FUNCTION);
            if (!EmitFunctionDefNop(cx, cg, pn->pn_index))
                return JS_FALSE;
            break;
        }

        JS_ASSERT_IF(cx->options & JSOPTION_ANONFUNFIX,
                     pn->pn_defn ||
                     (!pn->pn_used && !pn->isTopLevel()) ||
                     (fun->flags & JSFUN_LAMBDA));

        JS_ASSERT_IF(pn->pn_funbox->tcflags & TCF_FUN_HEAVYWEIGHT,
                     FUN_KIND(fun) == JSFUN_INTERPRETED);

        
        void *cg2mark = JS_ARENA_MARK(cg->codePool);
        void *cg2space;
        JS_ARENA_ALLOCATE_TYPE(cg2space, JSCodeGenerator, cg->codePool);
        if (!cg2space) {
            js_ReportOutOfScriptQuota(cx);
            return JS_FALSE;
        }
        JSCodeGenerator *cg2 =
            new (cg2space) JSCodeGenerator(cg->compiler,
                                           cg->codePool, cg->notePool,
                                           pn->pn_pos.begin.lineno);
        cg2->flags = pn->pn_funbox->tcflags | TCF_IN_FUNCTION;
#if JS_HAS_SHARP_VARS
        if (cg2->flags & TCF_HAS_SHARPS) {
            cg2->sharpSlotBase = fun->sharpSlotBase(cx);
            if (cg2->sharpSlotBase < 0)
                return JS_FALSE;
        }
#endif
        cg2->fun = fun;
        cg2->funbox = pn->pn_funbox;
        cg2->parent = cg;

        




        JS_ASSERT(cg->staticLevel < JS_BITMASK(16) - 1);
        cg2->staticLevel = cg->staticLevel + 1;

        
        JS_SCOPE_DEPTH_METERING(cg2->maxScopeDepth = (uintN) -1);
        if (!js_EmitFunctionScript(cx, cg2, pn->pn_body))
            pn = NULL;

        cg2->~JSCodeGenerator();
        JS_ARENA_RELEASE(cg->codePool, cg2mark);
        cg2 = NULL;
        if (!pn)
            return JS_FALSE;

        
        index = cg->objectList.index(pn->pn_funbox);

        
        op = PN_OP(pn);
        if (op != JSOP_NOP) {
            if ((pn->pn_funbox->tcflags & TCF_GENEXP_LAMBDA) &&
                js_NewSrcNote(cx, cg, SRC_GENEXP) < 0) {
                return JS_FALSE;
            }
            EMIT_INDEX_OP(op, index);
            break;
        }

        








        if (!(cg->flags & TCF_IN_FUNCTION)) {
            JS_ASSERT(!cg->topStmt);
            CG_SWITCH_TO_PROLOG(cg);
            op = FUN_FLAT_CLOSURE(fun) ? JSOP_DEFFUN_FC : JSOP_DEFFUN;
            EMIT_INDEX_OP(op, index);
            CG_SWITCH_TO_MAIN(cg);

            
            if (!EmitFunctionDefNop(cx, cg, index))
                return JS_FALSE;
        } else {
#ifdef DEBUG
            JSLocalKind localKind =
#endif
                js_LookupLocal(cx, cg->fun, fun->atom, &slot);
            JS_ASSERT(localKind == JSLOCAL_VAR || localKind == JSLOCAL_CONST);
            JS_ASSERT(index < JS_BIT(20));
            pn->pn_index = index;
            op = FUN_FLAT_CLOSURE(fun) ? JSOP_DEFLOCALFUN_FC : JSOP_DEFLOCALFUN;
            if (!EmitSlotIndexOp(cx, op, slot, index, cg))
                return JS_FALSE;
        }
        break;
      }

      case TOK_ARGSBODY:
        ok = js_EmitTree(cx, cg, pn->last());
        break;

      case TOK_UPVARS:
        JS_ASSERT(cg->lexdeps.count == 0);
        JS_ASSERT(pn->pn_names.count != 0);
        cg->lexdeps = pn->pn_names;
        ok = js_EmitTree(cx, cg, pn->pn_tree);
        break;

      case TOK_IF:
        
        stmtInfo.type = STMT_IF;
        beq = jmp = -1;
        noteIndex = -1;

      if_again:
        
        if (!js_EmitTree(cx, cg, pn->pn_kid1))
            return JS_FALSE;
        top = CG_OFFSET(cg);
        if (stmtInfo.type == STMT_IF) {
            js_PushStatement(cg, &stmtInfo, STMT_IF, top);
        } else {
            








            JS_ASSERT(stmtInfo.type == STMT_ELSE);
            stmtInfo.type = STMT_IF;
            stmtInfo.update = top;
            if (!js_SetSrcNoteOffset(cx, cg, noteIndex, 0, jmp - beq))
                return JS_FALSE;
            if (!js_SetSrcNoteOffset(cx, cg, noteIndex, 1, top - jmp))
                return JS_FALSE;
        }

        
        pn3 = pn->pn_kid3;
        noteIndex = js_NewSrcNote(cx, cg, pn3 ? SRC_IF_ELSE : SRC_IF);
        if (noteIndex < 0)
            return JS_FALSE;
        beq = EmitJump(cx, cg, JSOP_IFEQ, 0);
        if (beq < 0)
            return JS_FALSE;

        
        if (!js_EmitTree(cx, cg, pn->pn_kid2))
            return JS_FALSE;
        if (pn3) {
            
            stmtInfo.type = STMT_ELSE;

            





            jmp = EmitGoto(cx, cg, &stmtInfo, &stmtInfo.breaks, NULL, SRC_NULL);
            if (jmp < 0)
                return JS_FALSE;

            
            CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq);
            if (pn3->pn_type == TOK_IF) {
                pn = pn3;
                goto if_again;
            }

            if (!js_EmitTree(cx, cg, pn3))
                return JS_FALSE;

            






            if (!js_SetSrcNoteOffset(cx, cg, noteIndex, 0, jmp - beq))
                return JS_FALSE;
        } else {
            
            CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq);
        }
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_SWITCH:
        
        ok = EmitSwitch(cx, cg, pn, &stmtInfo);
        break;

      case TOK_WHILE:
        



















        js_PushStatement(cg, &stmtInfo, STMT_WHILE_LOOP, top);
        noteIndex = js_NewSrcNote(cx, cg, SRC_WHILE);
        if (noteIndex < 0)
            return JS_FALSE;
        jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
        if (jmp < 0)
            return JS_FALSE;
        top = js_Emit1(cx, cg, JSOP_TRACE);
        if (top < 0)
            return JS_FALSE;
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
        if (beq < 0)
            return JS_FALSE;
        if (!js_SetSrcNoteOffset(cx, cg, noteIndex, 0, beq - jmp))
            return JS_FALSE;
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_DO:
        
        noteIndex = js_NewSrcNote(cx, cg, SRC_WHILE);
        if (noteIndex < 0 || js_Emit1(cx, cg, JSOP_NOP) < 0)
            return JS_FALSE;

        
        top = js_Emit1(cx, cg, JSOP_TRACE);
        if (top < 0)
            return JS_FALSE;
        js_PushStatement(cg, &stmtInfo, STMT_DO_LOOP, top);
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;

        
        stmt = &stmtInfo;
        do {
            stmt->update = CG_OFFSET(cg);
        } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

        
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;

        




        beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
        if (beq < 0)
            return JS_FALSE;
        if (!js_SetSrcNoteOffset(cx, cg, noteIndex, 0, 1 + (beq - top)))
            return JS_FALSE;
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_FOR:
        beq = 0;                
        jmp = -1;
        pn2 = pn->pn_left;
        js_PushStatement(cg, &stmtInfo, STMT_FOR_LOOP, top);

        if (pn2->pn_type == TOK_IN) {
            
            stmtInfo.type = STMT_FOR_IN_LOOP;

            
























            pn3 = pn2->pn_left;
            type = PN_TYPE(pn3);
            cg->flags |= TCF_IN_FOR_INIT;
            if (TOKEN_TYPE_IS_DECL(type) && !js_EmitTree(cx, cg, pn3))
                return JS_FALSE;
            cg->flags &= ~TCF_IN_FOR_INIT;

            
            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;

            




            JS_ASSERT(pn->pn_op == JSOP_ITER);
            if (js_Emit2(cx, cg, JSOP_ITER, (uint8) pn->pn_iflags) < 0)
                return JS_FALSE;

            
            noteIndex = js_NewSrcNote(cx, cg, SRC_FOR_IN);
            if (noteIndex < 0)
                return JS_FALSE;

            



            jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
            if (jmp < 0)
                return JS_FALSE;

            top = CG_OFFSET(cg);
            SET_STATEMENT_TOP(&stmtInfo, top);
            if (js_Emit1(cx, cg, JSOP_TRACE) < 0)
                return JS_FALSE;

#ifdef DEBUG
            intN loopDepth = cg->stackDepth;
#endif

            









            op = JSOP_SETNAME;
            switch (type) {
#if JS_HAS_BLOCK_SCOPE
              case TOK_LET:
#endif
              case TOK_VAR:
                JS_ASSERT(pn3->pn_arity == PN_LIST && pn3->pn_count == 1);
                pn3 = pn3->pn_head;
#if JS_HAS_DESTRUCTURING
                if (pn3->pn_type == TOK_ASSIGN) {
                    pn3 = pn3->pn_left;
                    JS_ASSERT(pn3->pn_type == TOK_RB || pn3->pn_type == TOK_RC);
                }
                if (pn3->pn_type == TOK_RB || pn3->pn_type == TOK_RC) {
                    op = PN_OP(pn2->pn_left);
                    goto destructuring_for;
                }
#else
                JS_ASSERT(pn3->pn_type == TOK_NAME);
#endif
                

              case TOK_NAME:
                







                if ((
#if JS_HAS_BLOCK_SCOPE
                     type == TOK_LET ||
#endif
                     (type == TOK_VAR && !pn3->maybeExpr())) &&
                    js_NewSrcNote2(cx, cg, SRC_DECL,
                                   (type == TOK_VAR)
                                   ? SRC_DECL_VAR
                                   : SRC_DECL_LET) < 0) {
                    return JS_FALSE;
                }
                if (pn3->pn_cookie != FREE_UPVAR_COOKIE) {
                    op = PN_OP(pn3);
                    switch (op) {
                      case JSOP_GETARG:   
                      case JSOP_SETARG:   op = JSOP_FORARG; break;
                      case JSOP_GETGVAR:  
                      case JSOP_SETGVAR:  op = JSOP_FORNAME; break;
                      case JSOP_GETLOCAL: 
                      case JSOP_SETLOCAL: op = JSOP_FORLOCAL; break;
                      default:            JS_ASSERT(0);
                    }
                } else {
                    pn3->pn_op = JSOP_FORNAME;
                    if (!BindNameToSlot(cx, cg, pn3))
                        return JS_FALSE;
                    op = PN_OP(pn3);
                }
                if (pn3->isConst()) {
                    js_ReportCompileErrorNumber(cx, CG_TS(cg), pn3, JSREPORT_ERROR,
                                                JSMSG_BAD_FOR_LEFTSIDE);
                    return JS_FALSE;
                }
                if (pn3->pn_cookie != FREE_UPVAR_COOKIE) {
                    atomIndex = (jsatomid) pn3->pn_cookie;
                    EMIT_UINT16_IMM_OP(op, atomIndex);
                } else {
                    if (!EmitAtomOp(cx, pn3, op, cg))
                        return JS_FALSE;
                }
                break;

              case TOK_DOT:
                



                useful = JS_FALSE;
                if (!CheckSideEffects(cx, cg, pn3->expr(), &useful))
                    return JS_FALSE;
                if (!useful) {
                    if (!EmitPropOp(cx, pn3, JSOP_FORPROP, cg, JS_FALSE))
                        return JS_FALSE;
                    break;
                }
                

#if JS_HAS_DESTRUCTURING
              destructuring_for:
#endif
              default:
                if (js_Emit1(cx, cg, JSOP_FORELEM) < 0)
                    return JS_FALSE;
                JS_ASSERT(cg->stackDepth >= 3);

#if JS_HAS_DESTRUCTURING
                if (pn3->pn_type == TOK_RB || pn3->pn_type == TOK_RC) {
                    if (!EmitDestructuringOps(cx, cg, op, pn3))
                        return JS_FALSE;
                    if (js_Emit1(cx, cg, JSOP_POP) < 0)
                        return JS_FALSE;
                } else
#endif
                if (pn3->pn_type == TOK_LP) {
                    JS_ASSERT(pn3->pn_op == JSOP_SETCALL);
                    if (!js_EmitTree(cx, cg, pn3))
                        return JS_FALSE;
                    if (js_Emit1(cx, cg, JSOP_ENUMELEM) < 0)
                        return JS_FALSE;
                } else
#if JS_HAS_XML_SUPPORT
                if (pn3->pn_type == TOK_UNARYOP) {
                    JS_ASSERT(pn3->pn_op == JSOP_BINDXMLNAME);
                    if (!js_EmitTree(cx, cg, pn3))
                        return JS_FALSE;
                    if (js_Emit1(cx, cg, JSOP_ENUMELEM) < 0)
                        return JS_FALSE;
                } else
#endif
                if (!EmitElemOp(cx, pn3, JSOP_ENUMELEM, cg))
                    return JS_FALSE;
                break;
            }

            
            JS_ASSERT(cg->stackDepth == loopDepth);

            
            if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, CG_OFFSET(cg) - jmp))
                return JS_FALSE;

            
            if (!js_EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;

            
            stmt = &stmtInfo;
            do {
                stmt->update = CG_OFFSET(cg);
            } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

            


            CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
            if (js_Emit1(cx, cg, JSOP_NEXTITER) < 0)
                return JS_FALSE;
            beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
            if (beq < 0)
                return JS_FALSE;

            
            if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 1, beq - jmp))
                return JS_FALSE;
        } else {
            
            op = JSOP_POP;
            pn3 = pn2->pn_kid1;
            if (!pn3) {
                
                op = JSOP_NOP;
            } else {
                cg->flags |= TCF_IN_FOR_INIT;
#if JS_HAS_DESTRUCTURING
                if (pn3->pn_type == TOK_ASSIGN &&
                    !MaybeEmitGroupAssignment(cx, cg, op, pn3, &op)) {
                    return JS_FALSE;
                }
#endif
                if (op == JSOP_POP) {
                    if (!js_EmitTree(cx, cg, pn3))
                        return JS_FALSE;
                    if (TOKEN_TYPE_IS_DECL(pn3->pn_type)) {
                        





                        JS_ASSERT(pn3->pn_arity == PN_LIST);
                        if (pn3->pn_xflags & PNX_GROUPINIT)
                            op = JSOP_NOP;
                    }
                }
                cg->flags &= ~TCF_IN_FOR_INIT;
            }

            





            noteIndex = js_NewSrcNote(cx, cg, SRC_FOR);
            if (noteIndex < 0 || js_Emit1(cx, cg, op) < 0)
                return JS_FALSE;
            tmp = CG_OFFSET(cg);

            if (pn2->pn_kid2) {
                
                jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
                if (jmp < 0)
                    return JS_FALSE;
            }

            top = CG_OFFSET(cg);
            SET_STATEMENT_TOP(&stmtInfo, top);

            
            if (js_Emit1(cx, cg, JSOP_TRACE) < 0)
                return JS_FALSE;
            if (!js_EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;

            
            JS_ASSERT(noteIndex != -1);
            if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 1,
                                     CG_OFFSET(cg) - tmp)) {
                return JS_FALSE;
            }

            
            stmt = &stmtInfo;
            do {
                stmt->update = CG_OFFSET(cg);
            } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

            
            pn3 = pn2->pn_kid3;
            if (pn3) {
                op = JSOP_POP;
#if JS_HAS_DESTRUCTURING
                if (pn3->pn_type == TOK_ASSIGN &&
                    !MaybeEmitGroupAssignment(cx, cg, op, pn3, &op)) {
                    return JS_FALSE;
                }
#endif
                if (op == JSOP_POP && !js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;

                
                if (js_Emit1(cx, cg, op) < 0)
                    return JS_FALSE;

                
                off = (ptrdiff_t) pn->pn_pos.end.lineno;
                if (CG_CURRENT_LINE(cg) != (uintN) off) {
                    if (js_NewSrcNote2(cx, cg, SRC_SETLINE, off) < 0)
                        return JS_FALSE;
                    CG_CURRENT_LINE(cg) = (uintN) off;
                }
            }

            
            if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0,
                                     CG_OFFSET(cg) - tmp)) {
                return JS_FALSE;
            }

            if (pn2->pn_kid2) {
                
                JS_ASSERT(jmp >= 0);
                CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);

                if (!js_EmitTree(cx, cg, pn2->pn_kid2))
                    return JS_FALSE;
            }

            
            if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 2,
                                     CG_OFFSET(cg) - tmp)) {
                return JS_FALSE;
            }

            if (pn2->pn_kid2) {
                beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
                if (beq < 0)
                    return JS_FALSE;
            } else {
                
                jmp = EmitJump(cx, cg, JSOP_GOTO, top - CG_OFFSET(cg));
                if (jmp < 0)
                    return JS_FALSE;
            }
        }

        
        if (!js_PopStatementCG(cx, cg))
            return JS_FALSE;

        if (pn2->pn_type == TOK_IN) {
            





            JS_ASSERT(js_CodeSpec[JSOP_ENDITER].nuses == 2);
            if (!NewTryNote(cx, cg, JSTRY_ITER, cg->stackDepth, top, CG_OFFSET(cg)) ||
                js_Emit1(cx, cg, JSOP_ENDITER) < 0) {
                return JS_FALSE;
            }
        }
        break;

      case TOK_BREAK:
        stmt = cg->topStmt;
        atom = pn->pn_atom;
        if (atom) {
            ale = cg->atomList.add(cg->compiler, atom);
            if (!ale)
                return JS_FALSE;
            while (stmt->type != STMT_LABEL || stmt->label != atom)
                stmt = stmt->down;
            noteType = SRC_BREAK2LABEL;
        } else {
            ale = NULL;
            while (!STMT_IS_LOOP(stmt) && stmt->type != STMT_SWITCH)
                stmt = stmt->down;
            noteType = (stmt->type == STMT_SWITCH) ? SRC_NULL : SRC_BREAK;
        }

        if (EmitGoto(cx, cg, stmt, &stmt->breaks, ale, noteType) < 0)
            return JS_FALSE;
        break;

      case TOK_CONTINUE:
        stmt = cg->topStmt;
        atom = pn->pn_atom;
        if (atom) {
            
            JSStmtInfo *loop = NULL;
            ale = cg->atomList.add(cg->compiler, atom);
            if (!ale)
                return JS_FALSE;
            while (stmt->type != STMT_LABEL || stmt->label != atom) {
                if (STMT_IS_LOOP(stmt))
                    loop = stmt;
                stmt = stmt->down;
            }
            stmt = loop;
            noteType = SRC_CONT2LABEL;
        } else {
            ale = NULL;
            while (!STMT_IS_LOOP(stmt))
                stmt = stmt->down;
            noteType = SRC_CONTINUE;
        }

        if (EmitGoto(cx, cg, stmt, &stmt->continues, ale, noteType) < 0)
            return JS_FALSE;
        break;

      case TOK_WITH:
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        js_PushStatement(cg, &stmtInfo, STMT_WITH, CG_OFFSET(cg));
        if (js_Emit1(cx, cg, JSOP_ENTERWITH) < 0)
            return JS_FALSE;
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        if (js_Emit1(cx, cg, JSOP_LEAVEWITH) < 0)
            return JS_FALSE;
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_TRY:
      {
        ptrdiff_t tryStart, tryEnd, catchJump, finallyStart;
        intN depth;
        JSParseNode *lastCatch;

        catchJump = -1;

        








        js_PushStatement(cg, &stmtInfo,
                         pn->pn_kid3 ? STMT_FINALLY : STMT_TRY,
                         CG_OFFSET(cg));

        








        depth = cg->stackDepth;

        
        if (js_Emit1(cx, cg, JSOP_TRY) < 0)
            return JS_FALSE;
        tryStart = CG_OFFSET(cg);
        if (!js_EmitTree(cx, cg, pn->pn_kid1))
            return JS_FALSE;
        JS_ASSERT(depth == cg->stackDepth);

        
        if (pn->pn_kid3) {
            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &GOSUBS(stmtInfo));
            if (jmp < 0)
                return JS_FALSE;
        }

        
        if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
            return JS_FALSE;
        jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &catchJump);
        if (jmp < 0)
            return JS_FALSE;

        tryEnd = CG_OFFSET(cg);

        
        pn2 = pn->pn_kid2;
        lastCatch = NULL;
        if (pn2) {
            jsint count = 0;    

            





















            for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                ptrdiff_t guardJump, catchNote;

                JS_ASSERT(cg->stackDepth == depth);
                guardJump = GUARDJUMP(stmtInfo);
                if (guardJump != -1) {
                    
                    CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, guardJump);

                    




                    cg->stackDepth = depth + count + 1;

                    





                    if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0 ||
                        js_Emit1(cx, cg, JSOP_THROWING) < 0) {
                        return JS_FALSE;
                    }
                    if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                        return JS_FALSE;
                    EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, count);
                    JS_ASSERT(cg->stackDepth == depth);
                }

                






                catchNote = js_NewSrcNote2(cx, cg, SRC_CATCH, 0);
                if (catchNote < 0)
                    return JS_FALSE;
                CATCHNOTE(stmtInfo) = catchNote;

                




                JS_ASSERT(pn3->pn_type == TOK_LEXICALSCOPE);
                count = OBJ_BLOCK_COUNT(cx, pn3->pn_objbox->object);
                if (!js_EmitTree(cx, cg, pn3))
                    return JS_FALSE;

                
                if (pn->pn_kid3) {
                    jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH,
                                          &GOSUBS(stmtInfo));
                    if (jmp < 0)
                        return JS_FALSE;
                    JS_ASSERT(cg->stackDepth == depth);
                }

                



                if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                    return JS_FALSE;
                jmp = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &catchJump);
                if (jmp < 0)
                    return JS_FALSE;

                



                lastCatch = pn3->expr();
            }
        }

        





        if (lastCatch && lastCatch->pn_kid2) {
            CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, GUARDJUMP(stmtInfo));

            
            JS_ASSERT(cg->stackDepth == depth);
            cg->stackDepth = depth + 1;

            



            if (js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0 ||
                js_Emit1(cx, cg, JSOP_THROW) < 0) {
                return JS_FALSE;
            }
        }

        JS_ASSERT(cg->stackDepth == depth);

        
        finallyStart = 0;   
        if (pn->pn_kid3) {
            



            if (!BackPatch(cx, cg, GOSUBS(stmtInfo), CG_NEXT(cg), JSOP_GOSUB))
                return JS_FALSE;

            finallyStart = CG_OFFSET(cg);

            
            stmtInfo.type = STMT_SUBROUTINE;
            if (!UpdateLineNumberNotes(cx, cg, pn->pn_kid3->pn_pos.begin.lineno))
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_FINALLY) < 0 ||
                !js_EmitTree(cx, cg, pn->pn_kid3) ||
                js_Emit1(cx, cg, JSOP_RETSUB) < 0) {
                return JS_FALSE;
            }
            JS_ASSERT(cg->stackDepth == depth);
        }
        if (!js_PopStatementCG(cx, cg))
            return JS_FALSE;

        if (js_NewSrcNote(cx, cg, SRC_ENDBRACE) < 0 ||
            js_Emit1(cx, cg, JSOP_NOP) < 0) {
            return JS_FALSE;
        }

        
        if (!BackPatch(cx, cg, catchJump, CG_NEXT(cg), JSOP_GOTO))
            return JS_FALSE;

        



        if (pn->pn_kid2 &&
            !NewTryNote(cx, cg, JSTRY_CATCH, depth, tryStart, tryEnd)) {
            return JS_FALSE;
        }

        




        if (pn->pn_kid3 &&
            !NewTryNote(cx, cg, JSTRY_FINALLY, depth, tryStart, finallyStart)) {
            return JS_FALSE;
        }
        break;
      }

      case TOK_CATCH:
      {
        ptrdiff_t catchStart, guardJump;
        JSObject *blockObj;

        



        stmt = cg->topStmt;
        JS_ASSERT(stmt->type == STMT_BLOCK && (stmt->flags & SIF_SCOPE));
        stmt->type = STMT_CATCH;
        catchStart = stmt->update;
        blockObj = stmt->blockObj;

        
        stmt = stmt->down;
        JS_ASSERT(stmt->type == STMT_TRY || stmt->type == STMT_FINALLY);

        
        if (js_Emit1(cx, cg, JSOP_EXCEPTION) < 0)
            return JS_FALSE;

        



        if (pn->pn_kid2 && js_Emit1(cx, cg, JSOP_DUP) < 0)
            return JS_FALSE;

        pn2 = pn->pn_kid1;
        switch (pn2->pn_type) {
#if JS_HAS_DESTRUCTURING
          case TOK_RB:
          case TOK_RC:
            if (!EmitDestructuringOps(cx, cg, JSOP_NOP, pn2))
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
            break;
#endif

          case TOK_NAME:
            
            JS_ASSERT(pn2->pn_cookie != FREE_UPVAR_COOKIE);
            EMIT_UINT16_IMM_OP(JSOP_SETLOCALPOP, pn2->pn_cookie);
            break;

          default:
            JS_ASSERT(0);
        }

        
        if (pn->pn_kid2) {
            if (!js_EmitTree(cx, cg, pn->pn_kid2))
                return JS_FALSE;
            if (!js_SetSrcNoteOffset(cx, cg, CATCHNOTE(*stmt), 0,
                                     CG_OFFSET(cg) - catchStart)) {
                return JS_FALSE;
            }
            
            guardJump = EmitJump(cx, cg, JSOP_IFEQ, 0);
            if (guardJump < 0)
                return JS_FALSE;
            GUARDJUMP(*stmt) = guardJump;

            
            if (js_Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
        }

        
        if (!js_EmitTree(cx, cg, pn->pn_kid3))
            return JS_FALSE;

        



        off = cg->stackDepth;
        if (js_NewSrcNote2(cx, cg, SRC_CATCH, off) < 0)
            return JS_FALSE;
        break;
      }

      case TOK_VAR:
        if (!EmitVariables(cx, cg, pn, JS_FALSE, &noteIndex))
            return JS_FALSE;
        break;

      case TOK_RETURN:
        
        pn2 = pn->pn_kid;
        if (pn2) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        } else {
            if (js_Emit1(cx, cg, JSOP_PUSH) < 0)
                return JS_FALSE;
        }

        










        top = CG_OFFSET(cg);
        if (js_Emit1(cx, cg, JSOP_RETURN) < 0)
            return JS_FALSE;
        if (!EmitNonLocalJumpFixup(cx, cg, NULL))
            return JS_FALSE;
        if (top + JSOP_RETURN_LENGTH != CG_OFFSET(cg)) {
            CG_BASE(cg)[top] = JSOP_SETRVAL;
            if (js_Emit1(cx, cg, JSOP_RETRVAL) < 0)
                return JS_FALSE;
        }
        break;

#if JS_HAS_GENERATORS
      case TOK_YIELD:
        if (!(cg->flags & TCF_IN_FUNCTION)) {
            js_ReportCompileErrorNumber(cx, CG_TS(cg), pn, JSREPORT_ERROR,
                                        JSMSG_BAD_RETURN_OR_YIELD,
                                        js_yield_str);
            return JS_FALSE;
        }
        if (pn->pn_kid) {
            if (!js_EmitTree(cx, cg, pn->pn_kid))
                return JS_FALSE;
        } else {
            if (js_Emit1(cx, cg, JSOP_PUSH) < 0)
                return JS_FALSE;
        }
        if (pn->pn_hidden && js_NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
            return JS_FALSE;
        if (js_Emit1(cx, cg, JSOP_YIELD) < 0)
            return JS_FALSE;
        break;
#endif

      case TOK_LC:
      {
#if JS_HAS_XML_SUPPORT
        if (pn->pn_arity == PN_UNARY) {
            if (!js_EmitTree(cx, cg, pn->pn_kid))
                return JS_FALSE;
            if (js_Emit1(cx, cg, PN_OP(pn)) < 0)
                return JS_FALSE;
            break;
        }
#endif

        JS_ASSERT(pn->pn_arity == PN_LIST);

        noteIndex = -1;
        tmp = CG_OFFSET(cg);
        if (pn->pn_xflags & PNX_NEEDBRACES) {
            noteIndex = js_NewSrcNote2(cx, cg, SRC_BRACE, 0);
            if (noteIndex < 0 || js_Emit1(cx, cg, JSOP_NOP) < 0)
                return JS_FALSE;
        }

        js_PushStatement(cg, &stmtInfo, STMT_BLOCK, top);

        JSParseNode *pnchild = pn->pn_head;
        if (pn->pn_xflags & PNX_FUNCDEFS) {
            










            JS_ASSERT(cg->flags & TCF_IN_FUNCTION);
            if (pn->pn_xflags & PNX_DESTRUCT) {
                



                JS_ASSERT(pnchild->pn_type == TOK_SEMI);
                JS_ASSERT(pnchild->pn_kid->pn_type == TOK_COMMA);
                if (!js_EmitTree(cx, cg, pnchild))
                    return JS_FALSE;
                pnchild = pnchild->pn_next;
            }

            for (pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
                if (pn2->pn_type == TOK_FUNCTION) {
                    if (pn2->pn_op == JSOP_NOP) {
                        if (!js_EmitTree(cx, cg, pn2))
                            return JS_FALSE;
                    } else {
                        





                        JS_ASSERT(pn2->pn_op == JSOP_DEFFUN);
                    }
                }
            }
        }
        for (pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }

        if (noteIndex >= 0 &&
            !js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0,
                                 CG_OFFSET(cg) - tmp)) {
            return JS_FALSE;
        }

        ok = js_PopStatementCG(cx, cg);
        break;
      }

      case TOK_SEQ:
        JS_ASSERT(pn->pn_arity == PN_LIST);
        js_PushStatement(cg, &stmtInfo, STMT_SEQ, top);
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }
        ok = js_PopStatementCG(cx, cg);
        break;

      case TOK_SEMI:
        pn2 = pn->pn_kid;
        if (pn2) {
            








            useful = wantval = !(cg->flags & (TCF_IN_FUNCTION | TCF_NO_SCRIPT_RVAL));
            if (!useful) {
                if (!CheckSideEffects(cx, cg, pn2, &useful))
                    return JS_FALSE;
            }

            





            if (!useful &&
                (!cg->topStmt ||
                 cg->topStmt->type != STMT_LABEL ||
                 cg->topStmt->update < CG_OFFSET(cg))) {
                CG_CURRENT_LINE(cg) = pn2->pn_pos.begin.lineno;
                if (!js_ReportCompileErrorNumber(cx, CG_TS(cg), pn2,
                                                 JSREPORT_WARNING |
                                                 JSREPORT_STRICT,
                                                 JSMSG_USELESS_EXPR)) {
                    return JS_FALSE;
                }
            } else {
                op = wantval ? JSOP_POPV : JSOP_POP;
#if JS_HAS_DESTRUCTURING
                if (!wantval &&
                    pn2->pn_type == TOK_ASSIGN &&
                    !MaybeEmitGroupAssignment(cx, cg, op, pn2, &op)) {
                    return JS_FALSE;
                }
#endif
                if (op != JSOP_NOP) {
                    if (!js_EmitTree(cx, cg, pn2))
                        return JS_FALSE;
                    if (js_Emit1(cx, cg, op) < 0)
                        return JS_FALSE;
                }
            }
        }
        break;

      case TOK_COLON:
        
        atom = pn->pn_atom;
        ale = cg->atomList.add(cg->compiler, atom);
        if (!ale)
            return JS_FALSE;
        pn2 = pn->expr();
        noteType = (pn2->pn_type == TOK_LC ||
                    (pn2->pn_type == TOK_LEXICALSCOPE &&
                     pn2->expr()->pn_type == TOK_LC))
                   ? SRC_LABELBRACE
                   : SRC_LABEL;
        noteIndex = js_NewSrcNote2(cx, cg, noteType,
                                   (ptrdiff_t) ALE_INDEX(ale));
        if (noteIndex < 0 ||
            js_Emit1(cx, cg, JSOP_NOP) < 0) {
            return JS_FALSE;
        }

        
        js_PushStatement(cg, &stmtInfo, STMT_LABEL, CG_OFFSET(cg));
        stmtInfo.label = atom;
        if (!js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
        if (!js_PopStatementCG(cx, cg))
            return JS_FALSE;

        
        if (noteType == SRC_LABELBRACE) {
            if (js_NewSrcNote(cx, cg, SRC_ENDBRACE) < 0 ||
                js_Emit1(cx, cg, JSOP_NOP) < 0) {
                return JS_FALSE;
            }
        }
        break;

      case TOK_COMMA:
        




        off = noteIndex = -1;
        for (pn2 = pn->pn_head; ; pn2 = pn2->pn_next) {
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            tmp = CG_OFFSET(cg);
            if (noteIndex >= 0) {
                if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp-off))
                    return JS_FALSE;
            }
            if (!pn2->pn_next)
                break;
            off = tmp;
            noteIndex = js_NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
            if (noteIndex < 0 ||
                js_Emit1(cx, cg, JSOP_POP) < 0) {
                return JS_FALSE;
            }
        }
        break;

      case TOK_ASSIGN:
        




        pn2 = pn->pn_left;
        atomIndex = (jsatomid) -1;              
        switch (pn2->pn_type) {
          case TOK_NAME:
            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            if (pn2->pn_cookie != FREE_UPVAR_COOKIE) {
                atomIndex = (jsatomid) pn2->pn_cookie;
            } else {
                ale = cg->atomList.add(cg->compiler, pn2->pn_atom);
                if (!ale)
                    return JS_FALSE;
                atomIndex = ALE_INDEX(ale);
                if (!pn2->isConst())
                    EMIT_INDEX_OP(JSOP_BINDNAME, atomIndex);
            }
            break;
          case TOK_DOT:
            if (!js_EmitTree(cx, cg, pn2->expr()))
                return JS_FALSE;
            ale = cg->atomList.add(cg->compiler, pn2->pn_atom);
            if (!ale)
                return JS_FALSE;
            atomIndex = ALE_INDEX(ale);
            break;
          case TOK_LB:
            JS_ASSERT(pn2->pn_arity == PN_BINARY);
            if (!js_EmitTree(cx, cg, pn2->pn_left))
                return JS_FALSE;
            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;
            break;
#if JS_HAS_DESTRUCTURING
          case TOK_RB:
          case TOK_RC:
            break;
#endif
          case TOK_LP:
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            break;
#if JS_HAS_XML_SUPPORT
          case TOK_UNARYOP:
            JS_ASSERT(pn2->pn_op == JSOP_SETXMLNAME);
            if (!js_EmitTree(cx, cg, pn2->pn_kid))
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_BINDXMLNAME) < 0)
                return JS_FALSE;
            break;
#endif
          default:
            JS_ASSERT(0);
        }

        op = PN_OP(pn);
#if JS_HAS_GETTER_SETTER
        if (op == JSOP_GETTER || op == JSOP_SETTER) {
            if (pn2->pn_type == TOK_NAME && PN_OP(pn2) != JSOP_SETNAME) {
                



                js_ReportCompileErrorNumber(cx,
                                            TS(cg->compiler),
                                            pn2, JSREPORT_ERROR,
                                            JSMSG_BAD_GETTER_OR_SETTER,
                                            (op == JSOP_GETTER)
                                            ? js_getter_str
                                            : js_setter_str);
                return JS_FALSE;
            }

            
        } else
#endif
        
        if (op != JSOP_NOP) {
            switch (pn2->pn_type) {
              case TOK_NAME:
                if (pn2->isConst()) {
                    if (PN_OP(pn2) == JSOP_CALLEE) {
                        if (js_Emit1(cx, cg, JSOP_CALLEE) < 0)
                            return JS_FALSE;
                    } else {
                        EMIT_INDEX_OP(PN_OP(pn2), atomIndex);
                    }
                } else if (PN_OP(pn2) == JSOP_SETNAME) {
                    if (js_Emit1(cx, cg, JSOP_DUP) < 0)
                        return JS_FALSE;
                    EMIT_INDEX_OP(JSOP_GETXPROP, atomIndex);
                } else {
                    EMIT_UINT16_IMM_OP((PN_OP(pn2) == JSOP_SETGVAR)
                                       ? JSOP_GETGVAR
                                       : (PN_OP(pn2) == JSOP_GETUPVAR)
                                       ? JSOP_GETUPVAR
                                       : (PN_OP(pn2) == JSOP_SETARG)
                                       ? JSOP_GETARG
                                       : JSOP_GETLOCAL,
                                       atomIndex);
                }
                break;
              case TOK_DOT:
                if (js_Emit1(cx, cg, JSOP_DUP) < 0)
                    return JS_FALSE;
                if (pn2->pn_atom == cx->runtime->atomState.lengthAtom) {
                    if (js_Emit1(cx, cg, JSOP_LENGTH) < 0)
                        return JS_FALSE;
                } else if (pn2->pn_atom == cx->runtime->atomState.protoAtom) {
                    if (!EmitIndexOp(cx, JSOP_QNAMEPART, atomIndex, cg))
                        return JS_FALSE;
                    if (js_Emit1(cx, cg, JSOP_GETELEM) < 0)
                        return JS_FALSE;
                } else {
                    EMIT_INDEX_OP(JSOP_GETPROP, atomIndex);
                }
                break;
              case TOK_LB:
              case TOK_LP:
#if JS_HAS_XML_SUPPORT
              case TOK_UNARYOP:
#endif
                if (js_Emit1(cx, cg, JSOP_DUP2) < 0)
                    return JS_FALSE;
                if (js_Emit1(cx, cg, JSOP_GETELEM) < 0)
                    return JS_FALSE;
                break;
              default:;
            }
        }

        
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;

        
        if (op != JSOP_NOP) {
            




            if (pn2->pn_type != TOK_NAME || !pn2->isConst()) {
                if (js_NewSrcNote(cx, cg, SRC_ASSIGNOP) < 0)
                    return JS_FALSE;
            }
            if (js_Emit1(cx, cg, op) < 0)
                return JS_FALSE;
        }

        
        if (pn2->pn_type != TOK_NAME &&
#if JS_HAS_DESTRUCTURING
            pn2->pn_type != TOK_RB &&
            pn2->pn_type != TOK_RC &&
#endif
            js_NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0) {
            return JS_FALSE;
        }

        
        switch (pn2->pn_type) {
          case TOK_NAME:
            if (pn2->isConst())
                break;
            
          case TOK_DOT:
            EMIT_INDEX_OP(PN_OP(pn2), atomIndex);
            break;
          case TOK_LB:
          case TOK_LP:
            if (js_Emit1(cx, cg, JSOP_SETELEM) < 0)
                return JS_FALSE;
            break;
#if JS_HAS_DESTRUCTURING
          case TOK_RB:
          case TOK_RC:
            if (!EmitDestructuringOps(cx, cg, JSOP_SETNAME, pn2))
                return JS_FALSE;
            break;
#endif
#if JS_HAS_XML_SUPPORT
          case TOK_UNARYOP:
            if (js_Emit1(cx, cg, JSOP_SETXMLNAME) < 0)
                return JS_FALSE;
            break;
#endif
          default:
            JS_ASSERT(0);
        }
        break;

      case TOK_HOOK:
        
        if (!js_EmitTree(cx, cg, pn->pn_kid1))
            return JS_FALSE;
        noteIndex = js_NewSrcNote(cx, cg, SRC_COND);
        if (noteIndex < 0)
            return JS_FALSE;
        beq = EmitJump(cx, cg, JSOP_IFEQ, 0);
        if (beq < 0 || !js_EmitTree(cx, cg, pn->pn_kid2))
            return JS_FALSE;

        
        jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
        if (jmp < 0)
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq);

        











        JS_ASSERT(cg->stackDepth > 0);
        cg->stackDepth--;
        if (!js_EmitTree(cx, cg, pn->pn_kid3))
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
        if (!js_SetSrcNoteOffset(cx, cg, noteIndex, 0, jmp - beq))
            return JS_FALSE;
        break;

      case TOK_OR:
      case TOK_AND:
        









        if (pn->pn_arity == PN_BINARY) {
            if (!js_EmitTree(cx, cg, pn->pn_left))
                return JS_FALSE;
            top = EmitJump(cx, cg, JSOP_BACKPATCH_POP, 0);
            if (top < 0)
                return JS_FALSE;
            if (!js_EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;
            off = CG_OFFSET(cg);
            pc = CG_CODE(cg, top);
            CHECK_AND_SET_JUMP_OFFSET(cx, cg, pc, off - top);
            *pc = pn->pn_op;
        } else {
            JS_ASSERT(pn->pn_arity == PN_LIST);
            JS_ASSERT(pn->pn_head->pn_next->pn_next);

            
            pn2 = pn->pn_head;
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            top = EmitJump(cx, cg, JSOP_BACKPATCH_POP, 0);
            if (top < 0)
                return JS_FALSE;

            
            jmp = top;
            while ((pn2 = pn2->pn_next)->pn_next) {
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                off = EmitJump(cx, cg, JSOP_BACKPATCH_POP, 0);
                if (off < 0)
                    return JS_FALSE;
                if (!SetBackPatchDelta(cx, cg, CG_CODE(cg, jmp), off - jmp))
                    return JS_FALSE;
                jmp = off;

            }
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;

            pn2 = pn->pn_head;
            off = CG_OFFSET(cg);
            do {
                pc = CG_CODE(cg, top);
                tmp = GetJumpOffset(cg, pc);
                CHECK_AND_SET_JUMP_OFFSET(cx, cg, pc, off - top);
                *pc = pn->pn_op;
                top += tmp;
            } while ((pn2 = pn2->pn_next)->pn_next);
        }
        break;

      case TOK_PLUS:
        
        if (pn->pn_arity == PN_LIST && pn->pn_count < JS_BIT(16) &&
            (cg->flags & TCF_IN_FUNCTION)) {
            
            for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
                if (pn2->pn_type == TOK_STRING)
                    break;
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                if (pn2 != pn->pn_head && js_Emit1(cx, cg, JSOP_ADD) < 0)
                    return JS_FALSE;
            }

            
            for (index = 0; pn2; pn2 = pn2->pn_next, index++) {
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
            }

            if (index != 0) {
                EMIT_UINT16_IMM_OP(JSOP_CONCATN, index);

                
                if (pn->pn_head->pn_type != TOK_STRING &&
                    js_Emit1(cx, cg, JSOP_ADD) < 0) {
                    return JS_FALSE;
                }
            }
            break;
        }
      case TOK_BITOR:
      case TOK_BITXOR:
      case TOK_BITAND:
      case TOK_EQOP:
      case TOK_RELOP:
      case TOK_IN:
      case TOK_INSTANCEOF:
      case TOK_SHOP:
      case TOK_MINUS:
      case TOK_STAR:
      case TOK_DIVOP:
        if (pn->pn_arity == PN_LIST) {
            
            pn2 = pn->pn_head;
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            op = PN_OP(pn);
            while ((pn2 = pn2->pn_next) != NULL) {
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                if (js_Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            }
        } else {
#if JS_HAS_XML_SUPPORT
            uintN oldflags;

      case TOK_DBLCOLON:
            if (pn->pn_arity == PN_NAME) {
                if (!js_EmitTree(cx, cg, pn->expr()))
                    return JS_FALSE;
                if (!EmitAtomOp(cx, pn, PN_OP(pn), cg))
                    return JS_FALSE;
                break;
            }

            




            oldflags = cg->flags;
            cg->flags &= ~TCF_IN_FOR_INIT;
#endif

            
            if (!js_EmitTree(cx, cg, pn->pn_left))
                return JS_FALSE;
            if (!js_EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;
#if JS_HAS_XML_SUPPORT
            cg->flags |= oldflags & TCF_IN_FOR_INIT;
#endif
            if (js_Emit1(cx, cg, PN_OP(pn)) < 0)
                return JS_FALSE;
        }
        break;

      case TOK_THROW:
#if JS_HAS_XML_SUPPORT
      case TOK_AT:
      case TOK_DEFAULT:
        JS_ASSERT(pn->pn_arity == PN_UNARY);
        
#endif
      case TOK_UNARYOP:
      {
        uintN oldflags;

        
        op = PN_OP(pn);
#if JS_HAS_XML_SUPPORT
        if (op == JSOP_XMLNAME) {
            if (!EmitXMLName(cx, pn, op, cg))
                return JS_FALSE;
            break;
        }
#endif
        pn2 = pn->pn_kid;

        
        JS_ASSERT_IF(op == JSOP_TYPEOF, pn2->pn_type == TOK_NAME);

        oldflags = cg->flags;
        cg->flags &= ~TCF_IN_FOR_INIT;
        if (!js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
        cg->flags |= oldflags & TCF_IN_FOR_INIT;
        if (js_Emit1(cx, cg, op) < 0)
            return JS_FALSE;
        break;
      }

      case TOK_INC:
      case TOK_DEC:
        
        pn2 = pn->pn_kid;
        JS_ASSERT(pn2->pn_type != TOK_RP);
        op = PN_OP(pn);
        switch (pn2->pn_type) {
          default:
            JS_ASSERT(pn2->pn_type == TOK_NAME);
            pn2->pn_op = op;
            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            op = PN_OP(pn2);
            if (op == JSOP_CALLEE) {
                if (js_Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            } else if (pn2->pn_cookie != FREE_UPVAR_COOKIE) {
                atomIndex = (jsatomid) pn2->pn_cookie;
                EMIT_UINT16_IMM_OP(op, atomIndex);
            } else {
                JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
                if (!EmitAtomOp(cx, pn2, op, cg))
                    return JS_FALSE;
                break;
            }
            if (pn2->isConst()) {
                if (js_Emit1(cx, cg, JSOP_POS) < 0)
                    return JS_FALSE;
                op = PN_OP(pn);
                if (!(js_CodeSpec[op].format & JOF_POST)) {
                    if (js_Emit1(cx, cg, JSOP_ONE) < 0)
                        return JS_FALSE;
                    op = (js_CodeSpec[op].format & JOF_INC) ? JSOP_ADD : JSOP_SUB;
                    if (js_Emit1(cx, cg, op) < 0)
                        return JS_FALSE;
                }
            }
            break;
          case TOK_DOT:
            if (!EmitPropOp(cx, pn2, op, cg, JS_FALSE))
                return JS_FALSE;
            break;
          case TOK_LB:
            if (!EmitElemOp(cx, pn2, op, cg))
                return JS_FALSE;
            break;
          case TOK_LP:
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            if (js_NewSrcNote2(cx, cg, SRC_PCBASE,
                               CG_OFFSET(cg) - pn2->pn_offset) < 0) {
                return JS_FALSE;
            }
            if (js_Emit1(cx, cg, op) < 0)
                return JS_FALSE;
            break;
#if JS_HAS_XML_SUPPORT
          case TOK_UNARYOP:
            JS_ASSERT(pn2->pn_op == JSOP_SETXMLNAME);
            if (!js_EmitTree(cx, cg, pn2->pn_kid))
                return JS_FALSE;
            if (js_Emit1(cx, cg, JSOP_BINDXMLNAME) < 0)
                return JS_FALSE;
            if (js_Emit1(cx, cg, op) < 0)
                return JS_FALSE;
            break;
#endif
        }
        break;

      case TOK_DELETE:
        



        pn2 = pn->pn_kid;
        switch (pn2->pn_type) {
          case TOK_NAME:
            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            op = PN_OP(pn2);
            if (op == JSOP_FALSE) {
                if (js_Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            } else {
                if (!EmitAtomOp(cx, pn2, op, cg))
                    return JS_FALSE;
            }
            break;
          case TOK_DOT:
            if (!EmitPropOp(cx, pn2, JSOP_DELPROP, cg, JS_FALSE))
                return JS_FALSE;
            break;
#if JS_HAS_XML_SUPPORT
          case TOK_DBLDOT:
            if (!EmitElemOp(cx, pn2, JSOP_DELDESC, cg))
                return JS_FALSE;
            break;
#endif
          case TOK_LB:
            if (!EmitElemOp(cx, pn2, JSOP_DELELEM, cg))
                return JS_FALSE;
            break;
          default:
            



            useful = JS_FALSE;
            if (!CheckSideEffects(cx, cg, pn2, &useful))
                return JS_FALSE;
            if (!useful) {
                off = noteIndex = -1;
            } else {
                if (pn2->pn_op == JSOP_SETCALL)
                    pn2->pn_op = JSOP_CALL;
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                off = CG_OFFSET(cg);
                noteIndex = js_NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
                if (noteIndex < 0 || js_Emit1(cx, cg, JSOP_POP) < 0)
                    return JS_FALSE;
            }
            if (js_Emit1(cx, cg, JSOP_TRUE) < 0)
                return JS_FALSE;
            if (noteIndex >= 0) {
                tmp = CG_OFFSET(cg);
                if (!js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp-off))
                    return JS_FALSE;
            }
        }
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_FILTER:
        if (!js_EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        jmp = js_Emit3(cx, cg, JSOP_FILTER, 0, 0);
        if (jmp < 0)
            return JS_FALSE;
        top = js_Emit1(cx, cg, JSOP_TRACE);
        if (top < 0)
            return JS_FALSE;
        if (!js_EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
        if (EmitJump(cx, cg, JSOP_ENDFILTER, top - CG_OFFSET(cg)) < 0)
            return JS_FALSE;
        break;
#endif

      case TOK_DOT:
        




        ok = EmitPropOp(cx, pn, PN_OP(pn), cg, JS_FALSE);
        break;

      case TOK_LB:
#if JS_HAS_XML_SUPPORT
      case TOK_DBLDOT:
#endif
        





        ok = EmitElemOp(cx, pn, PN_OP(pn), cg);
        break;

      case TOK_NEW:
      case TOK_LP:
      {
        bool callop = (PN_TYPE(pn) == TOK_LP);
        uintN oldflags;

        
















        pn2 = pn->pn_head;
        switch (pn2->pn_type) {
          case TOK_NAME:
            if (!EmitNameOp(cx, cg, pn2, callop))
                return JS_FALSE;
            break;
          case TOK_DOT:
            if (!EmitPropOp(cx, pn2, PN_OP(pn2), cg, callop))
                return JS_FALSE;
            break;
          case TOK_LB:
            JS_ASSERT(pn2->pn_op == JSOP_GETELEM);
            if (!EmitElemOp(cx, pn2, callop ? JSOP_CALLELEM : JSOP_GETELEM, cg))
                return JS_FALSE;
            break;
          case TOK_UNARYOP:
#if JS_HAS_XML_SUPPORT
            if (pn2->pn_op == JSOP_XMLNAME) {
                if (!EmitXMLName(cx, pn2, JSOP_CALLXMLNAME, cg))
                    return JS_FALSE;
                callop = true;          
                break;
            }
#endif
            
          default:
            



            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            callop = false;             
            break;
        }
        if (!callop && js_Emit1(cx, cg, JSOP_NULL) < 0)
            return JS_FALSE;

        
        off = top;

        




        oldflags = cg->flags;
        cg->flags &= ~TCF_IN_FOR_INIT;
        for (pn3 = pn2->pn_next; pn3; pn3 = pn3->pn_next) {
            if (!js_EmitTree(cx, cg, pn3))
                return JS_FALSE;
        }
        cg->flags |= oldflags & TCF_IN_FOR_INIT;
        if (js_NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - off) < 0)
            return JS_FALSE;

        argc = pn->pn_count - 1;
        if (js_Emit3(cx, cg, PN_OP(pn), ARGC_HI(argc), ARGC_LO(argc)) < 0)
            return JS_FALSE;
        if (PN_OP(pn) == JSOP_CALL) {
            
            if (js_Emit1(cx, cg, JSOP_TRACE) < 0)
                return JS_FALSE;
        }
        if (PN_OP(pn) == JSOP_EVAL)
            EMIT_UINT16_IMM_OP(JSOP_LINENO, pn->pn_pos.begin.lineno);
        break;
      }

      case TOK_LEXICALSCOPE:
      {
        JSObjectBox *objbox;
        uintN count;

        objbox = pn->pn_objbox;
        js_PushBlockScope(cg, &stmtInfo, objbox->object, CG_OFFSET(cg));

        







        noteIndex = -1;
        type = PN_TYPE(pn->expr());
        if (type != TOK_CATCH && type != TOK_LET && type != TOK_FOR &&
            (!(stmt = stmtInfo.down)
             ? !(cg->flags & TCF_IN_FUNCTION)
             : stmt->type == STMT_BLOCK)) {
#if defined DEBUG_brendan || defined DEBUG_mrbkap
            
            JS_ASSERT(CG_NOTE_COUNT(cg) == 0 ||
                      CG_LAST_NOTE_OFFSET(cg) != CG_OFFSET(cg) ||
                      !GettableNoteForNextOp(cg));
#endif
            noteIndex = js_NewSrcNote2(cx, cg, SRC_BRACE, 0);
            if (noteIndex < 0)
                return JS_FALSE;
        }

        JS_ASSERT(CG_OFFSET(cg) == top);
        if (!EmitEnterBlock(cx, pn, cg))
            return JS_FALSE;

        if (!js_EmitTree(cx, cg, pn->pn_expr))
            return JS_FALSE;

        op = PN_OP(pn);
        if (op == JSOP_LEAVEBLOCKEXPR) {
            if (js_NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
                return JS_FALSE;
        } else {
            if (noteIndex >= 0 &&
                !js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0,
                                     CG_OFFSET(cg) - top)) {
                return JS_FALSE;
            }
        }

        
        count = OBJ_BLOCK_COUNT(cx, objbox->object);
        EMIT_UINT16_IMM_OP(op, count);

        ok = js_PopStatementCG(cx, cg);
        break;
      }

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        
        if (pn->pn_arity == PN_BINARY) {
            pn2 = pn->pn_right;
            pn = pn->pn_left;
        } else {
            pn2 = NULL;
        }

        
        JS_ASSERT(pn->pn_arity == PN_LIST);
        if (!EmitVariables(cx, cg, pn, pn2 != NULL, &noteIndex))
            return JS_FALSE;

        
        tmp = CG_OFFSET(cg);
        if (pn2 && !js_EmitTree(cx, cg, pn2))
            return JS_FALSE;

        if (noteIndex >= 0 &&
            !js_SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0,
                                 CG_OFFSET(cg) - tmp)) {
            return JS_FALSE;
        }
        break;
#endif 

#if JS_HAS_GENERATORS
      case TOK_ARRAYPUSH: {
        jsint slot;

        




        if (!js_EmitTree(cx, cg, pn->pn_kid))
            return JS_FALSE;
        slot = AdjustBlockSlot(cx, cg, cg->arrayCompDepth);
        if (slot < 0)
            return JS_FALSE;
        EMIT_UINT16_IMM_OP(PN_OP(pn), slot);
        break;
      }
#endif

      case TOK_RB:
#if JS_HAS_GENERATORS
      case TOK_ARRAYCOMP:
#endif
        















#if JS_HAS_SHARP_VARS
        sharpnum = -1;
      do_emit_array:
#endif

        op = (JS_LIKELY(pn->pn_count < JS_BIT(16)) && (cg->flags & TCF_IN_FUNCTION))
             ? JSOP_NEWARRAY
             : JSOP_NEWINIT;

#if JS_HAS_GENERATORS
        if (pn->pn_type == TOK_ARRAYCOMP)
            op = JSOP_NEWINIT;
#endif
#if JS_HAS_SHARP_VARS
        JS_ASSERT_IF(sharpnum >= 0, cg->hasSharps());
        if (cg->hasSharps())
            op = JSOP_NEWINIT;
#endif

        if (op == JSOP_NEWINIT && !EmitNewInit(cx, cg, JSProto_Array, pn, sharpnum))
            return JS_FALSE;

#if JS_HAS_GENERATORS
        if (pn->pn_type == TOK_ARRAYCOMP) {
            uintN saveDepth;

            




            JS_ASSERT(cg->stackDepth > 0);
            saveDepth = cg->arrayCompDepth;
            cg->arrayCompDepth = (uint32) (cg->stackDepth - 1);
            if (!js_EmitTree(cx, cg, pn->pn_head))
                return JS_FALSE;
            cg->arrayCompDepth = saveDepth;

            
            if (!EmitEndInit(cx, cg, 1))
                return JS_FALSE;
            break;
        }
#endif 

        pn2 = pn->pn_head;
        for (atomIndex = 0; pn2; atomIndex++, pn2 = pn2->pn_next) {
            if (op == JSOP_NEWINIT && !EmitNumberOp(cx, atomIndex, cg))
                return JS_FALSE;
            if (pn2->pn_type == TOK_COMMA && pn2->pn_arity == PN_NULLARY) {
                if (js_Emit1(cx, cg, JSOP_HOLE) < 0)
                    return JS_FALSE;
            } else {
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
            }
            if (op == JSOP_NEWINIT && js_Emit1(cx, cg, JSOP_INITELEM) < 0)
                return JS_FALSE;
        }
        JS_ASSERT(atomIndex == pn->pn_count);

        if (pn->pn_xflags & PNX_ENDCOMMA) {
            
            if (js_NewSrcNote(cx, cg, SRC_CONTINUE) < 0)
                return JS_FALSE;
        }

        if (op == JSOP_NEWINIT) {
            



            if (!EmitEndInit(cx, cg, atomIndex))
                return JS_FALSE;
            break;
        }

        JS_ASSERT(atomIndex < JS_BIT(16));
        EMIT_UINT16_IMM_OP(JSOP_NEWARRAY, atomIndex);
        break;

      case TOK_RC:
#if JS_HAS_SHARP_VARS
        sharpnum = -1;
      do_emit_object:
#endif
#if JS_HAS_DESTRUCTURING_SHORTHAND
        if (pn->pn_xflags & PNX_DESTRUCT) {
            js_ReportCompileErrorNumber(cx, CG_TS(cg), pn, JSREPORT_ERROR,
                                        JSMSG_BAD_OBJECT_INIT);
            return JS_FALSE;
        }
#endif
        







        if (!EmitNewInit(cx, cg, JSProto_Object, pn, sharpnum))
            return JS_FALSE;

        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            
            pn3 = pn2->pn_left;
            if (pn3->pn_type == TOK_NUMBER) {
                if (!EmitNumberOp(cx, pn3->pn_dval, cg))
                    return JS_FALSE;
            }

            
            if (!js_EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;

#if JS_HAS_GETTER_SETTER
            op = PN_OP(pn2);
            if (op == JSOP_GETTER || op == JSOP_SETTER) {
                if (js_Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            }
#endif
            
            if (pn3->pn_type == TOK_NUMBER) {
                if (js_NewSrcNote(cx, cg, SRC_INITPROP) < 0)
                    return JS_FALSE;
                if (js_Emit1(cx, cg, JSOP_INITELEM) < 0)
                    return JS_FALSE;
            } else {
                JS_ASSERT(pn3->pn_type == TOK_NAME ||
                          pn3->pn_type == TOK_STRING);
                ale = cg->atomList.add(cg->compiler, pn3->pn_atom);
                if (!ale)
                    return JS_FALSE;

                JSOp initOp = (PN_OP(pn2->pn_right) == JSOP_LAMBDA &&
                               !(pn2->pn_right->pn_funbox->tcflags
                                 & (TCF_FUN_USES_ARGUMENTS | TCF_FUN_USES_OWN_NAME))
#if JS_HAS_GETTER_SETTER
                               && op != JSOP_GETTER && op != JSOP_SETTER
#endif
                               )
                              ? JSOP_INITMETHOD
                              : JSOP_INITPROP;
                EMIT_INDEX_OP(initOp, ALE_INDEX(ale));
            }
        }

        if (!EmitEndInit(cx, cg, pn->pn_count))
            return JS_FALSE;
        break;

#if JS_HAS_SHARP_VARS
      case TOK_DEFSHARP:
        JS_ASSERT(cg->hasSharps());
        sharpnum = pn->pn_num;
        pn = pn->pn_kid;
        if (pn->pn_type == TOK_RB)
            goto do_emit_array;
# if JS_HAS_GENERATORS
        if (pn->pn_type == TOK_ARRAYCOMP)
            goto do_emit_array;
# endif
        if (pn->pn_type == TOK_RC)
            goto do_emit_object;

        if (!js_EmitTree(cx, cg, pn))
            return JS_FALSE;
        EMIT_UINT16PAIR_IMM_OP(JSOP_DEFSHARP, cg->sharpSlotBase, (jsatomid) sharpnum);
        break;

      case TOK_USESHARP:
        JS_ASSERT(cg->hasSharps());
        EMIT_UINT16PAIR_IMM_OP(JSOP_USESHARP, cg->sharpSlotBase, (jsatomid) pn->pn_num);
        break;
#endif 

      case TOK_NAME:
        




        if (pn->pn_op == JSOP_NOP)
            return JS_TRUE;
        if (!EmitNameOp(cx, cg, pn, JS_FALSE))
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_XMLATTR:
      case TOK_XMLSPACE:
      case TOK_XMLTEXT:
      case TOK_XMLCDATA:
      case TOK_XMLCOMMENT:
#endif
      case TOK_STRING:
        ok = EmitAtomOp(cx, pn, PN_OP(pn), cg);
        break;

      case TOK_NUMBER:
        ok = EmitNumberOp(cx, pn->pn_dval, cg);
        break;

      case TOK_REGEXP:
        






        JS_ASSERT(pn->pn_op == JSOP_REGEXP);
        if (cg->flags & TCF_COMPILE_N_GO) {
            ok = EmitObjectOp(cx, pn->pn_objbox, JSOP_OBJECT, cg);
        } else {
            ok = EmitIndexOp(cx, JSOP_REGEXP,
                             cg->regexpList.index(pn->pn_objbox),
                             cg);
        }
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_ANYNAME:
#endif
      case TOK_PRIMARY:
        if (js_Emit1(cx, cg, PN_OP(pn)) < 0)
            return JS_FALSE;
        break;

#if JS_HAS_DEBUGGER_KEYWORD
      case TOK_DEBUGGER:
        if (js_Emit1(cx, cg, JSOP_DEBUGGER) < 0)
            return JS_FALSE;
        break;
#endif 

#if JS_HAS_XML_SUPPORT
      case TOK_XMLELEM:
      case TOK_XMLLIST:
        if (pn->pn_op == JSOP_XMLOBJECT) {
            ok = EmitObjectOp(cx, pn->pn_objbox, PN_OP(pn), cg);
            break;
        }

        JS_ASSERT(PN_TYPE(pn) == TOK_XMLLIST || pn->pn_count != 0);
        switch (pn->pn_head ? PN_TYPE(pn->pn_head) : TOK_XMLLIST) {
          case TOK_XMLETAGO:
            JS_ASSERT(0);
            
          case TOK_XMLPTAGC:
          case TOK_XMLSTAGO:
            break;
          default:
            if (js_Emit1(cx, cg, JSOP_STARTXML) < 0)
                return JS_FALSE;
        }

        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->pn_type == TOK_LC &&
                js_Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0) {
                return JS_FALSE;
            }
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            if (pn2 != pn->pn_head && js_Emit1(cx, cg, JSOP_ADD) < 0)
                return JS_FALSE;
        }

        if (pn->pn_xflags & PNX_XMLROOT) {
            if (pn->pn_count == 0) {
                JS_ASSERT(pn->pn_type == TOK_XMLLIST);
                atom = cx->runtime->atomState.emptyAtom;
                ale = cg->atomList.add(cg->compiler, atom);
                if (!ale)
                    return JS_FALSE;
                EMIT_INDEX_OP(JSOP_STRING, ALE_INDEX(ale));
            }
            if (js_Emit1(cx, cg, PN_OP(pn)) < 0)
                return JS_FALSE;
        }
#ifdef DEBUG
        else
            JS_ASSERT(pn->pn_count != 0);
#endif
        break;

      case TOK_XMLPTAGC:
        if (pn->pn_op == JSOP_XMLOBJECT) {
            ok = EmitObjectOp(cx, pn->pn_objbox, PN_OP(pn), cg);
            break;
        }
        

      case TOK_XMLSTAGO:
      case TOK_XMLETAGO:
      {
        uint32 i;

        if (js_Emit1(cx, cg, JSOP_STARTXML) < 0)
            return JS_FALSE;

        ale = cg->atomList.add(cg->compiler,
                               (pn->pn_type == TOK_XMLETAGO)
                               ? cx->runtime->atomState.etagoAtom
                               : cx->runtime->atomState.stagoAtom);
        if (!ale)
            return JS_FALSE;
        EMIT_INDEX_OP(JSOP_STRING, ALE_INDEX(ale));

        JS_ASSERT(pn->pn_count != 0);
        pn2 = pn->pn_head;
        if (pn2->pn_type == TOK_LC && js_Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0)
            return JS_FALSE;
        if (!js_EmitTree(cx, cg, pn2))
            return JS_FALSE;
        if (js_Emit1(cx, cg, JSOP_ADD) < 0)
            return JS_FALSE;

        for (pn2 = pn2->pn_next, i = 0; pn2; pn2 = pn2->pn_next, i++) {
            if (pn2->pn_type == TOK_LC &&
                js_Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0) {
                return JS_FALSE;
            }
            if (!js_EmitTree(cx, cg, pn2))
                return JS_FALSE;
            if ((i & 1) && pn2->pn_type == TOK_LC) {
                if (js_Emit1(cx, cg, JSOP_TOATTRVAL) < 0)
                    return JS_FALSE;
            }
            if (js_Emit1(cx, cg,
                         (i & 1) ? JSOP_ADDATTRVAL : JSOP_ADDATTRNAME) < 0) {
                return JS_FALSE;
            }
        }

        ale = cg->atomList.add(cg->compiler,
                               (pn->pn_type == TOK_XMLPTAGC)
                               ? cx->runtime->atomState.ptagcAtom
                               : cx->runtime->atomState.tagcAtom);
        if (!ale)
            return JS_FALSE;
        EMIT_INDEX_OP(JSOP_STRING, ALE_INDEX(ale));
        if (js_Emit1(cx, cg, JSOP_ADD) < 0)
            return JS_FALSE;

        if ((pn->pn_xflags & PNX_XMLROOT) && js_Emit1(cx, cg, PN_OP(pn)) < 0)
            return JS_FALSE;
        break;
      }

      case TOK_XMLNAME:
        if (pn->pn_arity == PN_LIST) {
            JS_ASSERT(pn->pn_count != 0);
            for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
                if (pn2->pn_type == TOK_LC &&
                    js_Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0) {
                    return JS_FALSE;
                }
                if (!js_EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                if (pn2 != pn->pn_head && js_Emit1(cx, cg, JSOP_ADD) < 0)
                    return JS_FALSE;
            }
        } else {
            JS_ASSERT(pn->pn_arity == PN_NULLARY);
            ok = (pn->pn_op == JSOP_OBJECT)
                 ? EmitObjectOp(cx, pn->pn_objbox, PN_OP(pn), cg)
                 : EmitAtomOp(cx, pn, PN_OP(pn), cg);
        }
        break;

      case TOK_XMLPI:
        ale = cg->atomList.add(cg->compiler, pn->pn_atom2);
        if (!ale)
            return JS_FALSE;
        if (!EmitIndexOp(cx, JSOP_QNAMEPART, ALE_INDEX(ale), cg))
            return JS_FALSE;
        if (!EmitAtomOp(cx, pn, JSOP_XMLPI, cg))
            return JS_FALSE;
        break;
#endif 

      default:
        JS_ASSERT(0);
    }

    if (ok && --cg->emitLevel == 0) {
        if (cg->spanDeps)
            ok = OptimizeSpanDeps(cx, cg);
        if (!UpdateLineNumberNotes(cx, cg, pn->pn_pos.end.lineno))
            return JS_FALSE;
    }

    return ok;
}





JS_FRIEND_DATA(JSSrcNoteSpec) js_SrcNoteSpec[] = {
    {"null",            0,      0,      0},
    {"if",              0,      0,      0},
    {"if-else",         2,      0,      1},
    {"for",             3,      1,      1},
    {"while",           1,      0,      1},
    {"continue",        0,      0,      0},
    {"decl",            1,      1,      1},
    {"pcdelta",         1,      0,      1},
    {"assignop",        0,      0,      0},
    {"cond",            1,      0,      1},
    {"brace",           1,      0,      1},
    {"hidden",          0,      0,      0},
    {"pcbase",          1,      0,     -1},
    {"label",           1,      0,      0},
    {"labelbrace",      1,      0,      0},
    {"endbrace",        0,      0,      0},
    {"break2label",     1,      0,      0},
    {"cont2label",      1,      0,      0},
    {"switch",          2,      0,      1},
    {"funcdef",         1,      0,      0},
    {"catch",           1,      0,      1},
    {"extended",       -1,      0,      0},
    {"newline",         0,      0,      0},
    {"setline",         1,      0,      0},
    {"xdelta",          0,      0,      0},
};

static intN
AllocSrcNote(JSContext *cx, JSCodeGenerator *cg)
{
    intN index;
    JSArenaPool *pool;
    size_t size;

    index = CG_NOTE_COUNT(cg);
    if (((uintN)index & CG_NOTE_MASK(cg)) == 0) {
        pool = cg->notePool;
        size = SRCNOTE_SIZE(CG_NOTE_MASK(cg) + 1);
        if (!CG_NOTES(cg)) {
            
            JS_ARENA_ALLOCATE_CAST(CG_NOTES(cg), jssrcnote *, pool, size);
        } else {
            
            JS_ARENA_GROW_CAST(CG_NOTES(cg), jssrcnote *, pool, size, size);
            if (CG_NOTES(cg))
                CG_NOTE_MASK(cg) = (CG_NOTE_MASK(cg) << 1) | 1;
        }
        if (!CG_NOTES(cg)) {
            js_ReportOutOfScriptQuota(cx);
            return -1;
        }
    }

    CG_NOTE_COUNT(cg) = index + 1;
    return index;
}

intN
js_NewSrcNote(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type)
{
    intN index, n;
    jssrcnote *sn;
    ptrdiff_t offset, delta, xdelta;

    



    index = AllocSrcNote(cx, cg);
    if (index < 0)
        return -1;
    sn = &CG_NOTES(cg)[index];

    



    offset = CG_OFFSET(cg);
    delta = offset - CG_LAST_NOTE_OFFSET(cg);
    CG_LAST_NOTE_OFFSET(cg) = offset;
    if (delta >= SN_DELTA_LIMIT) {
        do {
            xdelta = JS_MIN(delta, SN_XDELTA_MASK);
            SN_MAKE_XDELTA(sn, xdelta);
            delta -= xdelta;
            index = AllocSrcNote(cx, cg);
            if (index < 0)
                return -1;
            sn = &CG_NOTES(cg)[index];
        } while (delta >= SN_DELTA_LIMIT);
    }

    




    SN_MAKE_NOTE(sn, type, delta);
    for (n = (intN)js_SrcNoteSpec[type].arity; n > 0; n--) {
        if (js_NewSrcNote(cx, cg, SRC_NULL) < 0)
            return -1;
    }
    return index;
}

intN
js_NewSrcNote2(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type,
               ptrdiff_t offset)
{
    intN index;

    index = js_NewSrcNote(cx, cg, type);
    if (index >= 0) {
        if (!js_SetSrcNoteOffset(cx, cg, index, 0, offset))
            return -1;
    }
    return index;
}

intN
js_NewSrcNote3(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type,
               ptrdiff_t offset1, ptrdiff_t offset2)
{
    intN index;

    index = js_NewSrcNote(cx, cg, type);
    if (index >= 0) {
        if (!js_SetSrcNoteOffset(cx, cg, index, 0, offset1))
            return -1;
        if (!js_SetSrcNoteOffset(cx, cg, index, 1, offset2))
            return -1;
    }
    return index;
}

static JSBool
GrowSrcNotes(JSContext *cx, JSCodeGenerator *cg)
{
    JSArenaPool *pool;
    size_t size;

    
    pool = cg->notePool;
    size = SRCNOTE_SIZE(CG_NOTE_MASK(cg) + 1);
    JS_ARENA_GROW_CAST(CG_NOTES(cg), jssrcnote *, pool, size, size);
    if (!CG_NOTES(cg)) {
        js_ReportOutOfScriptQuota(cx);
        return JS_FALSE;
    }
    CG_NOTE_MASK(cg) = (CG_NOTE_MASK(cg) << 1) | 1;
    return JS_TRUE;
}

jssrcnote *
js_AddToSrcNoteDelta(JSContext *cx, JSCodeGenerator *cg, jssrcnote *sn,
                     ptrdiff_t delta)
{
    ptrdiff_t base, limit, newdelta, diff;
    intN index;

    



    JS_ASSERT(cg->current == &cg->main);
    JS_ASSERT((unsigned) delta < (unsigned) SN_XDELTA_LIMIT);

    base = SN_DELTA(sn);
    limit = SN_IS_XDELTA(sn) ? SN_XDELTA_LIMIT : SN_DELTA_LIMIT;
    newdelta = base + delta;
    if (newdelta < limit) {
        SN_SET_DELTA(sn, newdelta);
    } else {
        index = sn - cg->main.notes;
        if ((cg->main.noteCount & cg->main.noteMask) == 0) {
            if (!GrowSrcNotes(cx, cg))
                return NULL;
            sn = cg->main.notes + index;
        }
        diff = cg->main.noteCount - index;
        cg->main.noteCount++;
        memmove(sn + 1, sn, SRCNOTE_SIZE(diff));
        SN_MAKE_XDELTA(sn, delta);
        sn++;
    }
    return sn;
}

JS_FRIEND_API(uintN)
js_SrcNoteLength(jssrcnote *sn)
{
    uintN arity;
    jssrcnote *base;

    arity = (intN)js_SrcNoteSpec[SN_TYPE(sn)].arity;
    for (base = sn++; arity; sn++, arity--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }
    return sn - base;
}

JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, uintN which)
{
    
    JS_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    JS_ASSERT((intN) which < js_SrcNoteSpec[SN_TYPE(sn)].arity);
    for (sn++; which; sn++, which--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }
    if (*sn & SN_3BYTE_OFFSET_FLAG) {
        return (ptrdiff_t)(((uint32)(sn[0] & SN_3BYTE_OFFSET_MASK) << 16)
                           | (sn[1] << 8)
                           | sn[2]);
    }
    return (ptrdiff_t)*sn;
}

JSBool
js_SetSrcNoteOffset(JSContext *cx, JSCodeGenerator *cg, uintN index,
                    uintN which, ptrdiff_t offset)
{
    jssrcnote *sn;
    ptrdiff_t diff;

    if ((jsuword)offset >= (jsuword)((ptrdiff_t)SN_3BYTE_OFFSET_FLAG << 16)) {
        ReportStatementTooLarge(cx, cg);
        return JS_FALSE;
    }

    
    sn = &CG_NOTES(cg)[index];
    JS_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    JS_ASSERT((intN) which < js_SrcNoteSpec[SN_TYPE(sn)].arity);
    for (sn++; which; sn++, which--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }

    
    if (offset > (ptrdiff_t)SN_3BYTE_OFFSET_MASK) {
        
        if (!(*sn & SN_3BYTE_OFFSET_FLAG)) {
            
            index = sn - CG_NOTES(cg);

            




            if (((CG_NOTE_COUNT(cg) + 1) & CG_NOTE_MASK(cg)) <= 1) {
                if (!GrowSrcNotes(cx, cg))
                    return JS_FALSE;
                sn = CG_NOTES(cg) + index;
            }
            CG_NOTE_COUNT(cg) += 2;

            diff = CG_NOTE_COUNT(cg) - (index + 3);
            JS_ASSERT(diff >= 0);
            if (diff > 0)
                memmove(sn + 3, sn + 1, SRCNOTE_SIZE(diff));
        }
        *sn++ = (jssrcnote)(SN_3BYTE_OFFSET_FLAG | (offset >> 16));
        *sn++ = (jssrcnote)(offset >> 8);
    }
    *sn = (jssrcnote)offset;
    return JS_TRUE;
}

#ifdef DEBUG_notme
#define DEBUG_srcnotesize
#endif

#ifdef DEBUG_srcnotesize
#define NBINS 10
static uint32 hist[NBINS];

void DumpSrcNoteSizeHist()
{
    static FILE *fp;
    int i, n;

    if (!fp) {
        fp = fopen("/tmp/srcnotes.hist", "w");
        if (!fp)
            return;
        setvbuf(fp, NULL, _IONBF, 0);
    }
    fprintf(fp, "SrcNote size histogram:\n");
    for (i = 0; i < NBINS; i++) {
        fprintf(fp, "%4u %4u ", JS_BIT(i), hist[i]);
        for (n = (int) JS_HOWMANY(hist[i], 10); n > 0; --n)
            fputc('*', fp);
        fputc('\n', fp);
    }
    fputc('\n', fp);
}
#endif







JSBool
js_FinishTakingSrcNotes(JSContext *cx, JSCodeGenerator *cg, jssrcnote *notes)
{
    uintN prologCount, mainCount, totalCount;
    ptrdiff_t offset, delta;
    jssrcnote *sn;

    JS_ASSERT(cg->current == &cg->main);

    prologCount = cg->prolog.noteCount;
    if (prologCount && cg->prolog.currentLine != cg->firstLine) {
        CG_SWITCH_TO_PROLOG(cg);
        if (js_NewSrcNote2(cx, cg, SRC_SETLINE, (ptrdiff_t)cg->firstLine) < 0)
            return JS_FALSE;
        prologCount = cg->prolog.noteCount;
        CG_SWITCH_TO_MAIN(cg);
    } else {
        






        offset = CG_PROLOG_OFFSET(cg) - cg->prolog.lastNoteOffset;
        JS_ASSERT(offset >= 0);
        if (offset > 0 && cg->main.noteCount != 0) {
            
            sn = cg->main.notes;
            delta = SN_IS_XDELTA(sn)
                    ? SN_XDELTA_MASK - (*sn & SN_XDELTA_MASK)
                    : SN_DELTA_MASK - (*sn & SN_DELTA_MASK);
            if (offset < delta)
                delta = offset;
            for (;;) {
                if (!js_AddToSrcNoteDelta(cx, cg, sn, delta))
                    return JS_FALSE;
                offset -= delta;
                if (offset == 0)
                    break;
                delta = JS_MIN(offset, SN_XDELTA_MASK);
                sn = cg->main.notes;
            }
        }
    }

    mainCount = cg->main.noteCount;
    totalCount = prologCount + mainCount;
    if (prologCount)
        memcpy(notes, cg->prolog.notes, SRCNOTE_SIZE(prologCount));
    memcpy(notes + prologCount, cg->main.notes, SRCNOTE_SIZE(mainCount));
    SN_MAKE_TERMINATOR(&notes[totalCount]);

#ifdef DEBUG_notme
  { int bin = JS_CeilingLog2(totalCount);
    if (bin >= NBINS)
        bin = NBINS - 1;
    ++hist[bin];
  }
#endif
    return JS_TRUE;
}

static JSBool
NewTryNote(JSContext *cx, JSCodeGenerator *cg, JSTryNoteKind kind,
           uintN stackDepth, size_t start, size_t end)
{
    JSTryNode *tryNode;

    JS_ASSERT((uintN)(uint16)stackDepth == stackDepth);
    JS_ASSERT(start <= end);
    JS_ASSERT((size_t)(uint32)start == start);
    JS_ASSERT((size_t)(uint32)end == end);

    JS_ARENA_ALLOCATE_TYPE(tryNode, JSTryNode, &cx->tempPool);
    if (!tryNode) {
        js_ReportOutOfScriptQuota(cx);
        return JS_FALSE;
    }

    tryNode->note.kind = kind;
    tryNode->note.stackDepth = (uint16)stackDepth;
    tryNode->note.start = (uint32)start;
    tryNode->note.length = (uint32)(end - start);
    tryNode->prev = cg->lastTryNode;
    cg->lastTryNode = tryNode;
    cg->ntrynotes++;
    return JS_TRUE;
}

void
js_FinishTakingTryNotes(JSCodeGenerator *cg, JSTryNoteArray *array)
{
    JSTryNode *tryNode;
    JSTryNote *tn;

    JS_ASSERT(array->length > 0 && array->length == cg->ntrynotes);
    tn = array->vector + array->length;
    tryNode = cg->lastTryNode;
    do {
        *--tn = tryNode->note;
    } while ((tryNode = tryNode->prev) != NULL);
    JS_ASSERT(tn == array->vector);
}












































uintN
JSCGObjectList::index(JSObjectBox *objbox)
{
    JS_ASSERT(!objbox->emitLink);
    objbox->emitLink = lastbox;
    lastbox = objbox;
    return length++;
}

void
JSCGObjectList::finish(JSObjectArray *array)
{
    JSObject **cursor;
    JSObjectBox *objbox;

    JS_ASSERT(length <= INDEX_LIMIT);
    JS_ASSERT(length == array->length);

    cursor = array->vector + array->length;
    objbox = lastbox;
    do {
        --cursor;
        JS_ASSERT(!*cursor);
        *cursor = objbox->object;
    } while ((objbox = objbox->emitLink) != NULL);
    JS_ASSERT(cursor == array->vector);
}
