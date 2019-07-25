










































#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <new>
#include <string.h>

#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsautooplen.h"        

#include "ds/LifoAlloc.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/BytecodeGenerator.h"
#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "vm/RegExpObject.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"


#define BYTECODE_CHUNK_LENGTH  1024    /* initial bytecode chunk length */
#define SRCNOTE_CHUNK_LENGTH   1024    /* initial srcnote chunk length */


#define BYTECODE_SIZE(n)        ((n) * sizeof(jsbytecode))
#define SRCNOTE_SIZE(n)         ((n) * sizeof(jssrcnote))

using namespace js;
using namespace js::gc;
using namespace js::frontend;

#ifdef JS_TRACER
extern uint8 js_opcode2extra[];
#endif

static JSBool
NewTryNote(JSContext *cx, CodeGenerator *cg, JSTryNoteKind kind, uintN stackDepth,
           size_t start, size_t end);

static bool
EmitIndexOp(JSContext *cx, JSOp op, uintN index, CodeGenerator *cg, JSOp *psuffix = NULL);

static JSBool
EmitLeaveBlock(JSContext *cx, CodeGenerator *cg, JSOp op, ObjectBox *box);

static JSBool
SetSrcNoteOffset(JSContext *cx, CodeGenerator *cg, uintN index, uintN which, ptrdiff_t offset);

void
TreeContext::trace(JSTracer *trc)
{
    bindings.trace(trc);
}

CodeGenerator::CodeGenerator(Parser *parser, uintN lineno)
  : TreeContext(parser),
    atomIndices(parser->context),
    stackDepth(0), maxStackDepth(0),
    ntrynotes(0), lastTryNode(NULL),
    spanDeps(NULL), jumpTargets(NULL), jtFreeList(NULL),
    numSpanDeps(0), numJumpTargets(0), spanDepTodo(0),
    arrayCompDepth(0),
    emitLevel(0),
    constMap(parser->context),
    constList(parser->context),
    upvarIndices(parser->context),
    upvarMap(parser->context),
    globalUses(parser->context),
    globalMap(parser->context),
    closedArgs(parser->context),
    closedVars(parser->context),
    traceIndex(0),
    typesetCount(0)
{
    flags = TCF_COMPILING;
    memset(&prolog, 0, sizeof prolog);
    memset(&main, 0, sizeof main);
    current = &main;
    firstLine = prolog.currentLine = main.currentLine = lineno;
}

bool
CodeGenerator::init(JSContext *cx, TreeContext::InitBehavior ib)
{
    roLexdeps.init();
    return TreeContext::init(cx, ib) && constMap.init() && atomIndices.ensureMap(cx);
}

CodeGenerator::~CodeGenerator()
{
    JSContext *cx = parser->context;

    cx->free_(prolog.base);
    cx->free_(prolog.notes);
    cx->free_(main.base);
    cx->free_(main.notes);

    
    if (spanDeps)
        cx->free_(spanDeps);
}

static ptrdiff_t
EmitCheck(JSContext *cx, CodeGenerator *cg, ptrdiff_t delta)
{
    jsbytecode *base = CG_BASE(cg);
    jsbytecode *newbase;
    jsbytecode *next = CG_NEXT(cg);
    jsbytecode *limit = CG_LIMIT(cg);
    ptrdiff_t offset = next - base;
    size_t minlength = offset + delta;

    if (next + delta > limit) {
        size_t newlength;
        if (!base) {
            JS_ASSERT(!next && !limit);
            newlength = BYTECODE_CHUNK_LENGTH;
            if (newlength < minlength)     
                newlength = RoundUpPow2(minlength);
            newbase = (jsbytecode *) cx->malloc_(BYTECODE_SIZE(newlength));
        } else {
            JS_ASSERT(base <= next && next <= limit);
            newlength = (limit - base) * 2;
            if (newlength < minlength)     
                newlength = RoundUpPow2(minlength);
            newbase = (jsbytecode *) cx->realloc_(base, BYTECODE_SIZE(newlength));
        }
        if (!newbase) {
            js_ReportOutOfMemory(cx);
            return -1;
        }
        JS_ASSERT(newlength >= size_t(offset + delta));
        CG_BASE(cg) = newbase;
        CG_LIMIT(cg) = newbase + newlength;
        CG_NEXT(cg) = newbase + offset;
    }
    return offset;
}

static void
UpdateDepth(JSContext *cx, CodeGenerator *cg, ptrdiff_t target)
{
    jsbytecode *pc;
    JSOp op;
    const JSCodeSpec *cs;
    uintN extra, nuses;
    intN ndefs;

    pc = CG_CODE(cg, target);
    op = (JSOp) *pc;
    cs = &js_CodeSpec[op];
#ifdef JS_TRACER
    extra = js_opcode2extra[op];
#else
    extra = 0;
#endif
    if ((cs->format & JOF_TMPSLOT_MASK) || extra) {
        uintN depth = (uintN) cg->stackDepth +
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
        TokenStream *ts;

        JS_snprintf(numBuf, sizeof numBuf, "%d", target);
        ts = &cg->parser->tokenStream;
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_WARNING,
                                     js_GetErrorMessage, NULL,
                                     JSMSG_STACK_UNDERFLOW,
                                     ts->getFilename() ? ts->getFilename() : "stdin",
                                     numBuf);
    }
    ndefs = cs->ndefs;
    if (ndefs < 0) {
        JSObject *blockObj;

        
        JS_ASSERT(op == JSOP_ENTERBLOCK);
        JS_ASSERT(nuses == 0);
        blockObj = cg->objectList.lastbox->object;
        JS_ASSERT(blockObj->isStaticBlock());
        JS_ASSERT(blockObj->getSlot(JSSLOT_BLOCK_DEPTH).isUndefined());

        OBJ_SET_BLOCK_DEPTH(cx, blockObj, cg->stackDepth);
        ndefs = OBJ_BLOCK_COUNT(cx, blockObj);
    }
    cg->stackDepth += ndefs;
    if ((uintN)cg->stackDepth > cg->maxStackDepth)
        cg->maxStackDepth = cg->stackDepth;
}

static inline void
UpdateDecomposeLength(CodeGenerator *cg, uintN start)
{
    uintN end = CG_OFFSET(cg);
    JS_ASSERT(uintN(end - start) < 256);
    CG_CODE(cg, start)[-1] = end - start;
}

ptrdiff_t
frontend::Emit1(JSContext *cx, CodeGenerator *cg, JSOp op)
{
    ptrdiff_t offset = EmitCheck(cx, cg, 1);

    if (offset >= 0) {
        *CG_NEXT(cg)++ = (jsbytecode)op;
        UpdateDepth(cx, cg, offset);
    }
    return offset;
}

ptrdiff_t
frontend::Emit2(JSContext *cx, CodeGenerator *cg, JSOp op, jsbytecode op1)
{
    ptrdiff_t offset = EmitCheck(cx, cg, 2);

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
frontend::Emit3(JSContext *cx, CodeGenerator *cg, JSOp op, jsbytecode op1,
                    jsbytecode op2)
{
    ptrdiff_t offset = EmitCheck(cx, cg, 3);

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
frontend::Emit5(JSContext *cx, CodeGenerator *cg, JSOp op, uint16 op1, uint16 op2)
{
    ptrdiff_t offset = EmitCheck(cx, cg, 5);

    if (offset >= 0) {
        jsbytecode *next = CG_NEXT(cg);
        next[0] = (jsbytecode)op;
        next[1] = UINT16_HI(op1);
        next[2] = UINT16_LO(op1);
        next[3] = UINT16_HI(op2);
        next[4] = UINT16_LO(op2);
        CG_NEXT(cg) = next + 5;
        UpdateDepth(cx, cg, offset);
    }
    return offset;
}

ptrdiff_t
frontend::EmitN(JSContext *cx, CodeGenerator *cg, JSOp op, size_t extra)
{
    ptrdiff_t length = 1 + (ptrdiff_t)extra;
    ptrdiff_t offset = EmitCheck(cx, cg, length);

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
StatementName(CodeGenerator *cg)
{
    if (!cg->topStmt)
        return js_script_str;
    return statementName[cg->topStmt->type];
}

static void
ReportStatementTooLarge(JSContext *cx, CodeGenerator *cg)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET,
                         StatementName(cg));
}



































































static int
BalanceJumpTargets(JumpTarget **jtp)
{
    JumpTarget *jt = *jtp;
    JS_ASSERT(jt->balance != 0);

    int dir;
    JSBool doubleRotate;
    if (jt->balance < -1) {
        dir = JT_RIGHT;
        doubleRotate = (jt->kids[JT_LEFT]->balance > 0);
    } else if (jt->balance > 1) {
        dir = JT_LEFT;
        doubleRotate = (jt->kids[JT_RIGHT]->balance < 0);
    } else {
        return 0;
    }

    int otherDir = JT_OTHER_DIR(dir);
    JumpTarget *root;
    int heightChanged;
    if (doubleRotate) {
        JumpTarget *jt2 = jt->kids[otherDir];
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

struct AddJumpTargetArgs {
    JSContext           *cx;
    CodeGenerator       *cg;
    ptrdiff_t           offset;
    JumpTarget          *node;
};

static int
AddJumpTarget(AddJumpTargetArgs *args, JumpTarget **jtp)
{
    JumpTarget *jt = *jtp;
    if (!jt) {
        CodeGenerator *cg = args->cg;

        jt = cg->jtFreeList;
        if (jt) {
            cg->jtFreeList = jt->kids[JT_LEFT];
        } else {
            jt = args->cx->tempLifoAlloc().new_<JumpTarget>();
            if (!jt) {
                js_ReportOutOfMemory(args->cx);
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

    int balanceDelta;
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
static int
AVLCheck(JumpTarget *jt)
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
SetSpanDepTarget(JSContext *cx, CodeGenerator *cg, SpanDep *sd, ptrdiff_t off)
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
#define SPANDEPS_SIZE(n)        ((n) * sizeof(js::SpanDep))
#define SPANDEPS_SIZE_MIN       SPANDEPS_SIZE(SPANDEPS_MIN)

static JSBool
AddSpanDep(JSContext *cx, CodeGenerator *cg, jsbytecode *pc, jsbytecode *pc2, ptrdiff_t off)
{
    uintN index = cg->numSpanDeps;
    if (index + 1 == 0) {
        ReportStatementTooLarge(cx, cg);
        return JS_FALSE;
    }

    SpanDep *sdbase;
    if ((index & (index - 1)) == 0 &&
        (!(sdbase = cg->spanDeps) || index >= SPANDEPS_MIN))
    {
        size_t size = sdbase ? SPANDEPS_SIZE(index) : SPANDEPS_SIZE_MIN / 2;
        sdbase = (SpanDep *) cx->realloc_(sdbase, size + size);
        if (!sdbase)
            return JS_FALSE;
        cg->spanDeps = sdbase;
    }

    cg->numSpanDeps = index + 1;
    SpanDep *sd = cg->spanDeps + index;
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
AddSwitchSpanDeps(JSContext *cx, CodeGenerator *cg, jsbytecode *pc)
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
BuildSpanDepTable(JSContext *cx, CodeGenerator *cg)
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

static SpanDep *
GetSpanDep(CodeGenerator *cg, jsbytecode *pc)
{
    uintN index;
    ptrdiff_t offset;
    int lo, hi, mid;
    SpanDep *sd;

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
SetBackPatchDelta(JSContext *cx, CodeGenerator *cg, jsbytecode *pc, ptrdiff_t delta)
{
    SpanDep *sd;

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
UpdateJumpTargets(JumpTarget *jt, ptrdiff_t pivot, ptrdiff_t delta)
{
    if (jt->offset > pivot) {
        jt->offset += delta;
        if (jt->kids[JT_LEFT])
            UpdateJumpTargets(jt->kids[JT_LEFT], pivot, delta);
    }
    if (jt->kids[JT_RIGHT])
        UpdateJumpTargets(jt->kids[JT_RIGHT], pivot, delta);
}

static SpanDep *
FindNearestSpanDep(CodeGenerator *cg, ptrdiff_t offset, int lo, SpanDep *guard)
{
    int num = cg->numSpanDeps;
    JS_ASSERT(num > 0);
    int hi = num - 1;
    SpanDep *sdbase = cg->spanDeps;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        SpanDep *sd = sdbase + mid;
        if (sd->before == offset)
            return sd;
        if (sd->before < offset)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    if (lo == num)
        return guard;
    SpanDep *sd = sdbase + lo;
    JS_ASSERT(sd->before >= offset && (lo == 0 || sd[-1].before < offset));
    return sd;
}

static void
FreeJumpTargets(CodeGenerator *cg, JumpTarget *jt)
{
    if (jt->kids[JT_LEFT])
        FreeJumpTargets(cg, jt->kids[JT_LEFT]);
    if (jt->kids[JT_RIGHT])
        FreeJumpTargets(cg, jt->kids[JT_RIGHT]);
    jt->kids[JT_LEFT] = cg->jtFreeList;
    cg->jtFreeList = jt;
}

static JSBool
OptimizeSpanDeps(JSContext *cx, CodeGenerator *cg)
{
    jsbytecode *pc, *oldpc, *base, *limit, *next;
    SpanDep *sd, *sd2, *sdbase, *sdlimit, *sdtop, guard;
    ptrdiff_t offset, growth, delta, top, pivot, span, length, target;
    JSBool done;
    JSOp op;
    uint32 type;
    jssrcnote *sn, *snlimit;
    JSSrcNoteSpec *spec;
    uintN i, n, noteIndex;
    TryNode *tryNode;
    DebugOnly<int> passes = 0;

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
        TokenStream *ts = &cg->parser->tokenStream;

        printf("%s:%u: %u/%u jumps extended in %d passes (%d=%d+%d)\n",
               ts->filename ? ts->filename : "stdin", cg->firstLine,
               growth / (JUMPX_OFFSET_LEN - JUMP_OFFSET_LEN), cg->numSpanDeps,
               passes, offset + growth, offset, growth);
#endif

        



        limit = CG_LIMIT(cg);
        length = offset + growth;
        next = base + length;
        if (next > limit) {
            base = (jsbytecode *) cx->realloc_(base, BYTECODE_SIZE(length));
            if (!base) {
                js_ReportOutOfMemory(cx);
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
        size_t size = BYTECODE_SIZE(delta - (1 + JUMP_OFFSET_LEN));
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
                    sn = AddToSrcNoteDelta(cx, cg, sn, delta);
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
                    if (!SetSrcNoteOffset(cx, cg, noteIndex, i, span))
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

    




    cx->free_(cg->spanDeps);
    cg->spanDeps = NULL;
    FreeJumpTargets(cg, cg->jumpTargets);
    cg->jumpTargets = NULL;
    cg->numSpanDeps = cg->numJumpTargets = 0;
    cg->spanDepTodo = CG_OFFSET(cg);
    return JS_TRUE;
}

static ptrdiff_t
EmitJump(JSContext *cx, CodeGenerator *cg, JSOp op, ptrdiff_t off)
{
    JSBool extend;
    ptrdiff_t jmp;
    jsbytecode *pc;

    extend = off < JUMP_OFFSET_MIN || JUMP_OFFSET_MAX < off;
    if (extend && !cg->spanDeps && !BuildSpanDepTable(cx, cg))
        return -1;

    jmp = Emit3(cx, cg, op, JUMP_OFFSET_HI(off), JUMP_OFFSET_LO(off));
    if (jmp >= 0 && (extend || cg->spanDeps)) {
        pc = CG_CODE(cg, jmp);
        if (!AddSpanDep(cx, cg, pc, pc, off))
            return -1;
    }
    return jmp;
}

static ptrdiff_t
GetJumpOffset(CodeGenerator *cg, jsbytecode *pc)
{
    if (!cg->spanDeps)
        return GET_JUMP_OFFSET(pc);

    SpanDep *sd = GetSpanDep(cg, pc);
    JumpTarget *jt = sd->target;
    if (!JT_HAS_TAG(jt))
        return JT_TO_BPDELTA(jt);

    ptrdiff_t top = sd->top;
    while (--sd >= cg->spanDeps && sd->top == top)
        continue;
    sd++;
    return JT_CLR_TAG(jt)->offset - sd->offset;
}

JSBool
frontend::SetJumpOffset(JSContext *cx, CodeGenerator *cg, jsbytecode *pc, ptrdiff_t off)
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
TreeContext::inStatement(StmtType type)
{
    for (StmtInfo *stmt = topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == type)
            return true;
    }
    return false;
}

bool
TreeContext::ensureSharpSlots()
{
#if JS_HAS_SHARP_VARS
    JS_STATIC_ASSERT(SHARP_NSLOTS == 2);

    if (sharpSlotBase >= 0) {
        JS_ASSERT(flags & TCF_HAS_SHARPS);
        return true;
    }

    JS_ASSERT(!(flags & TCF_HAS_SHARPS));
    if (inFunction()) {
        JSContext *cx = parser->context;
        JSAtom *sharpArrayAtom = js_Atomize(cx, "#array", 6);
        JSAtom *sharpDepthAtom = js_Atomize(cx, "#depth", 6);
        if (!sharpArrayAtom || !sharpDepthAtom)
            return false;

        sharpSlotBase = bindings.countVars();
        if (!bindings.addVariable(cx, sharpArrayAtom))
            return false;
        if (!bindings.addVariable(cx, sharpDepthAtom))
            return false;
    } else {
        




        sharpSlotBase = 0;
    }
    flags |= TCF_HAS_SHARPS;
#endif
    return true;
}

bool
TreeContext::skipSpansGenerator(unsigned skip)
{
    TreeContext *tc = this;
    for (unsigned i = 0; i < skip; ++i, tc = tc->parent) {
        if (!tc)
            return false;
        if (tc->flags & TCF_FUN_IS_GENERATOR)
            return true;
    }
    return false;
}

bool
frontend::SetStaticLevel(TreeContext *tc, uintN staticLevel)
{
    



    if (UpvarCookie::isLevelReserved(staticLevel)) {
        JS_ReportErrorNumber(tc->parser->context, js_GetErrorMessage, NULL,
                             JSMSG_TOO_DEEP, js_function_str);
        return false;
    }
    tc->staticLevel = staticLevel;
    return true;
}

bool
frontend::GenerateBlockId(TreeContext *tc, uint32& blockid)
{
    if (tc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(tc->parser->context, js_GetErrorMessage, NULL,
                             JSMSG_NEED_DIET, "program");
        return false;
    }
    blockid = tc->blockidGen++;
    return true;
}

void
frontend::PushStatement(TreeContext *tc, StmtInfo *stmt, StmtType type, ptrdiff_t top)
{
    stmt->type = type;
    stmt->flags = 0;
    stmt->blockid = tc->blockid();
    SET_STATEMENT_TOP(stmt, top);
    stmt->label = NULL;
    JS_ASSERT(!stmt->blockBox);
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
frontend::PushBlockScope(TreeContext *tc, StmtInfo *stmt, ObjectBox *blockBox, ptrdiff_t top)
{
    PushStatement(tc, stmt, STMT_BLOCK, top);
    stmt->flags |= SIF_SCOPE;
    blockBox->parent = tc->blockChainBox;
    if (tc->blockChain())
        blockBox->object->setScopeChain(tc->blockChain());
    stmt->downScope = tc->topScopeStmt;
    tc->topScopeStmt = stmt;
    tc->blockChainBox = blockBox;
    stmt->blockBox = blockBox;
}





static ptrdiff_t
EmitBackPatchOp(JSContext *cx, CodeGenerator *cg, JSOp op, ptrdiff_t *lastp)
{
    ptrdiff_t offset, delta;

    offset = CG_OFFSET(cg);
    delta = offset - *lastp;
    *lastp = offset;
    JS_ASSERT(delta > 0);
    return EmitJump(cx, cg, op, delta);
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
                if (NewSrcNote2(cx, cg, SRC_SETLINE, (ptrdiff_t)line_) < 0)   \
                    return JS_FALSE;                                          \
            } else {                                                          \
                do {                                                          \
                    if (NewSrcNote(cx, cg, SRC_NEWLINE) < 0)                  \
                        return JS_FALSE;                                      \
                } while (--delta_ != 0);                                      \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO


static JSBool
UpdateLineNumberNotes(JSContext *cx, CodeGenerator *cg, uintN line)
{
    UPDATE_LINE_NUMBER_NOTES(cx, cg, line);
    return JS_TRUE;
}

static ptrdiff_t
EmitTraceOp(JSContext *cx, CodeGenerator *cg, ParseNode *nextpn)
{
    if (nextpn) {
        




        if (nextpn->isKind(TOK_LC) && nextpn->isArity(PN_LIST) && nextpn->pn_head)
            nextpn = nextpn->pn_head;
        if (!UpdateLineNumberNotes(cx, cg, nextpn->pn_pos.begin.lineno))
            return -1;
    }

    uint32 index = cg->traceIndex;
    if (index < UINT16_MAX)
        cg->traceIndex++;
    return Emit3(cx, cg, JSOP_TRACE, UINT16_HI(index), UINT16_LO(index));
}





static inline void
CheckTypeSet(JSContext *cx, CodeGenerator *cg, JSOp op)
{
    if (js_CodeSpec[op].format & JOF_TYPESET) {
        if (cg->typesetCount < UINT16_MAX)
            cg->typesetCount++;
    }
}







#define EMIT_UINT16_IMM_OP(op, i)                                             \
    JS_BEGIN_MACRO                                                            \
        if (Emit3(cx, cg, op, UINT16_HI(i), UINT16_LO(i)) < 0)                \
            return JS_FALSE;                                                  \
        CheckTypeSet(cx, cg, op);                                             \
    JS_END_MACRO

#define EMIT_UINT16PAIR_IMM_OP(op, i, j)                                      \
    JS_BEGIN_MACRO                                                            \
        ptrdiff_t off_ = EmitN(cx, cg, op, 2 * UINT16_LEN);                   \
        if (off_ < 0)                                                         \
            return JS_FALSE;                                                  \
        jsbytecode *pc_ = CG_CODE(cg, off_);                                  \
        SET_UINT16(pc_, i);                                                   \
        pc_ += UINT16_LEN;                                                    \
        SET_UINT16(pc_, j);                                                   \
    JS_END_MACRO

#define EMIT_UINT16_IN_PLACE(offset, op, i)                                   \
    JS_BEGIN_MACRO                                                            \
        CG_CODE(cg, offset)[0] = op;                                          \
        CG_CODE(cg, offset)[1] = UINT16_HI(i);                                \
        CG_CODE(cg, offset)[2] = UINT16_LO(i);                                \
    JS_END_MACRO

static JSBool
FlushPops(JSContext *cx, CodeGenerator *cg, intN *npops)
{
    JS_ASSERT(*npops != 0);
    if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
        return JS_FALSE;
    EMIT_UINT16_IMM_OP(JSOP_POPN, *npops);
    *npops = 0;
    return JS_TRUE;
}




static JSBool
EmitNonLocalJumpFixup(JSContext *cx, CodeGenerator *cg, StmtInfo *toStmt)
{
    





    intN depth = cg->stackDepth;
    intN npops = 0;

#define FLUSH_POPS() if (npops && !FlushPops(cx, cg, &npops)) return JS_FALSE

    for (StmtInfo *stmt = cg->topStmt; stmt != toStmt; stmt = stmt->down) {
        switch (stmt->type) {
          case STMT_FINALLY:
            FLUSH_POPS();
            if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &GOSUBS(*stmt)) < 0)
                return JS_FALSE;
            break;

          case STMT_WITH:
            
            FLUSH_POPS();
            if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (Emit1(cx, cg, JSOP_LEAVEWITH) < 0)
                return JS_FALSE;
            break;

          case STMT_FOR_IN_LOOP:
            


            FLUSH_POPS();
            if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (Emit1(cx, cg, JSOP_ENDITER) < 0)
                return JS_FALSE;
            break;

          case STMT_SUBROUTINE:
            



            npops += 2;
            break;

          default:;
        }

        if (stmt->flags & SIF_SCOPE) {
            
            FLUSH_POPS();
            if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (!EmitLeaveBlock(cx, cg, JSOP_LEAVEBLOCK, stmt->blockBox))
                return JS_FALSE;
        }
    }

    FLUSH_POPS();
    cg->stackDepth = depth;
    return JS_TRUE;

#undef FLUSH_POPS
}

static JSBool
EmitKnownBlockChain(JSContext *cx, CodeGenerator *cg, ObjectBox *box)
{
    if (box)
        return EmitIndexOp(cx, JSOP_BLOCKCHAIN, box->index, cg);
    return Emit1(cx, cg, JSOP_NULLBLOCKCHAIN) >= 0;
}

static JSBool
EmitBlockChain(JSContext *cx, CodeGenerator *cg)
{
    return EmitKnownBlockChain(cx, cg, cg->blockChainBox);
}

static const jsatomid INVALID_ATOMID = -1;

static ptrdiff_t
EmitGoto(JSContext *cx, CodeGenerator *cg, StmtInfo *toStmt, ptrdiff_t *lastp,
         jsatomid labelIndex = INVALID_ATOMID, SrcNoteType noteType = SRC_NULL)
{
    intN index;

    if (!EmitNonLocalJumpFixup(cx, cg, toStmt))
        return -1;

    if (labelIndex != INVALID_ATOMID)
        index = NewSrcNote2(cx, cg, noteType, ptrdiff_t(labelIndex));
    else if (noteType != SRC_NULL)
        index = NewSrcNote(cx, cg, noteType);
    else
        index = 0;
    if (index < 0)
        return -1;

    ptrdiff_t result = EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, lastp);
    if (result < 0)
        return result;

    if (!EmitBlockChain(cx, cg))
        return -1;

    return result;
}

static JSBool
BackPatch(JSContext *cx, CodeGenerator *cg, ptrdiff_t last, jsbytecode *target, jsbytecode op)
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
frontend::PopStatementTC(TreeContext *tc)
{
    StmtInfo *stmt = tc->topStmt;
    tc->topStmt = stmt->down;
    if (STMT_LINKS_SCOPE(stmt)) {
        tc->topScopeStmt = stmt->downScope;
        if (stmt->flags & SIF_SCOPE) {
            tc->blockChainBox = stmt->blockBox->parent;
        }
    }
}

JSBool
frontend::PopStatementCG(JSContext *cx, CodeGenerator *cg)
{
    StmtInfo *stmt = cg->topStmt;
    if (!STMT_IS_TRYING(stmt) &&
        (!BackPatch(cx, cg, stmt->breaks, CG_NEXT(cg), JSOP_GOTO) ||
         !BackPatch(cx, cg, stmt->continues, CG_CODE(cg, stmt->update),
                    JSOP_GOTO))) {
        return JS_FALSE;
    }
    PopStatementTC(cg);
    return JS_TRUE;
}

JSBool
frontend::DefineCompileTimeConstant(JSContext *cx, CodeGenerator *cg, JSAtom *atom, ParseNode *pn)
{
    
    if (pn->isKind(TOK_NUMBER)) {
        if (!cg->constMap.put(atom, NumberValue(pn->pn_dval)))
            return JS_FALSE;
    }
    return JS_TRUE;
}

StmtInfo *
frontend::LexicalLookup(TreeContext *tc, JSAtom *atom, jsint *slotp, StmtInfo *stmt)
{
    if (!stmt)
        stmt = tc->topScopeStmt;
    for (; stmt; stmt = stmt->downScope) {
        if (stmt->type == STMT_WITH)
            break;

        
        if (!(stmt->flags & SIF_SCOPE))
            continue;

        JSObject *obj = stmt->blockBox->object;
        JS_ASSERT(obj->isStaticBlock());

        const Shape *shape = obj->nativeLookup(tc->parser->context, ATOM_TO_JSID(atom));
        if (shape) {
            JS_ASSERT(shape->hasShortID());

            if (slotp) {
                JS_ASSERT(obj->getSlot(JSSLOT_BLOCK_DEPTH).isInt32());
                *slotp = obj->getSlot(JSSLOT_BLOCK_DEPTH).toInt32() + shape->shortid();
            }
            return stmt;
        }
    }

    if (slotp)
        *slotp = -1;
    return stmt;
}





static JSBool
LookupCompileTimeConstant(JSContext *cx, CodeGenerator *cg, JSAtom *atom, Value *constp)
{
    




    constp->setMagic(JS_NO_CONSTANT);
    do {
        if (cg->inFunction() || cg->compileAndGo()) {
            
            StmtInfo *stmt = LexicalLookup(cg, atom, NULL);
            if (stmt)
                return JS_TRUE;

            if (CodeGenerator::ConstMap::Ptr p = cg->constMap.lookup(atom)) {
                JS_ASSERT(!p->value.isMagic(JS_NO_CONSTANT));
                *constp = p->value;
                return JS_TRUE;
            }

            






            if (cg->inFunction()) {
                if (cg->bindings.hasBinding(cx, atom))
                    break;
            } else {
                JS_ASSERT(cg->compileAndGo());
                JSObject *obj = cg->scopeChain();

                const Shape *shape = obj->nativeLookup(cx, ATOM_TO_JSID(atom));
                if (shape) {
                    





                    if (!shape->writable() && !shape->configurable() &&
                        shape->hasDefaultGetter() && obj->containsSlot(shape->slot())) {
                        *constp = obj->getSlot(shape->slot());
                    }
                }

                if (shape)
                    break;
            }
        }
    } while (cg->parent && (cg = cg->parent->asCodeGenerator()));
    return JS_TRUE;
}

static inline bool
FitsWithoutBigIndex(uintN index)
{
    return index < JS_BIT(16);
}







static JSOp
EmitBigIndexPrefix(JSContext *cx, CodeGenerator *cg, uintN index)
{
    uintN indexBase;

    



    JS_STATIC_ASSERT(INDEX_LIMIT <= JS_BIT(24));
    JS_STATIC_ASSERT(INDEX_LIMIT >=
                     (JSOP_INDEXBASE3 - JSOP_INDEXBASE1 + 2) << 16);

    if (FitsWithoutBigIndex(index))
        return JSOP_NOP;
    indexBase = index >> 16;
    if (indexBase <= JSOP_INDEXBASE3 - JSOP_INDEXBASE1 + 1) {
        if (Emit1(cx, cg, (JSOp)(JSOP_INDEXBASE1 + indexBase - 1)) < 0)
            return JSOP_FALSE;
        return JSOP_RESETBASE0;
    }

    if (index >= INDEX_LIMIT) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_TOO_MANY_LITERALS);
        return JSOP_FALSE;
    }

    if (Emit2(cx, cg, JSOP_INDEXBASE, (JSOp)indexBase) < 0)
        return JSOP_FALSE;
    return JSOP_RESETBASE;
}











static bool
EmitIndexOp(JSContext *cx, JSOp op, uintN index, CodeGenerator *cg, JSOp *psuffix)
{
    JSOp bigSuffix;

    bigSuffix = EmitBigIndexPrefix(cx, cg, index);
    if (bigSuffix == JSOP_FALSE)
        return false;
    EMIT_UINT16_IMM_OP(op, index);

    




    JS_ASSERT(!!(js_CodeSpec[op].format & JOF_DECOMPOSE) == (psuffix != NULL));
    if (psuffix) {
        *psuffix = bigSuffix;
        return true;
    }

    return bigSuffix == JSOP_NOP || Emit1(cx, cg, bigSuffix) >= 0;
}





#define EMIT_INDEX_OP(op, index)                                              \
    JS_BEGIN_MACRO                                                            \
        if (!EmitIndexOp(cx, op, index, cg))                                  \
            return JS_FALSE;                                                  \
    JS_END_MACRO

static bool
EmitAtomOp(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg, JSOp *psuffix = NULL)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

    if (op == JSOP_GETPROP &&
        pn->pn_atom == cx->runtime->atomState.lengthAtom) {
        
        op = JSOP_LENGTH;
    }

    jsatomid index;
    if (!cg->makeAtomIndex(pn->pn_atom, &index))
        return false;

    return EmitIndexOp(cx, op, index, cg, psuffix);
}

static JSBool
EmitObjectOp(JSContext *cx, ObjectBox *objbox, JSOp op, CodeGenerator *cg)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_OBJECT);
    return EmitIndexOp(cx, op, cg->objectList.index(objbox), cg);
}








JS_STATIC_ASSERT(ARGNO_LEN == 2);
JS_STATIC_ASSERT(SLOTNO_LEN == 2);

static JSBool
EmitSlotIndexOp(JSContext *cx, JSOp op, uintN slot, uintN index, CodeGenerator *cg)
{
    JSOp bigSuffix;
    ptrdiff_t off;
    jsbytecode *pc;

    JS_ASSERT(JOF_OPTYPE(op) == JOF_SLOTATOM ||
              JOF_OPTYPE(op) == JOF_SLOTOBJECT);
    bigSuffix = EmitBigIndexPrefix(cx, cg, index);
    if (bigSuffix == JSOP_FALSE)
        return JS_FALSE;

    
    off = EmitN(cx, cg, op, 2 + INDEX_LEN);
    if (off < 0)
        return JS_FALSE;
    pc = CG_CODE(cg, off);
    SET_UINT16(pc, slot);
    pc += 2;
    SET_INDEX(pc, index);
    return bigSuffix == JSOP_NOP || Emit1(cx, cg, bigSuffix) >= 0;
}

bool
CodeGenerator::shouldNoteClosedName(ParseNode *pn)
{
    return !callsEval() && pn->isDefn() && pn->isClosed();
}









static jsint
AdjustBlockSlot(JSContext *cx, CodeGenerator *cg, jsint slot)
{
    JS_ASSERT((jsuint) slot < cg->maxStackDepth);
    if (cg->inFunction()) {
        slot += cg->bindings.countVars();
        if ((uintN) slot >= SLOTNO_LIMIT) {
            ReportCompileErrorNumber(cx, CG_TS(cg), NULL, JSREPORT_ERROR, JSMSG_TOO_MANY_LOCALS);
            slot = -1;
        }
    }
    return slot;
}

static bool
EmitEnterBlock(JSContext *cx, ParseNode *pn, CodeGenerator *cg)
{
    JS_ASSERT(pn->isKind(TOK_LEXICALSCOPE));
    if (!EmitObjectOp(cx, pn->pn_objbox, JSOP_ENTERBLOCK, cg))
        return false;

    JSObject *blockObj = pn->pn_objbox->object;
    jsint depth = AdjustBlockSlot(cx, cg, OBJ_BLOCK_DEPTH(cx, blockObj));
    if (depth < 0)
        return false;

    uintN base = JSSLOT_FREE(&BlockClass);
    for (uintN slot = base, limit = base + OBJ_BLOCK_COUNT(cx, blockObj); slot < limit; slot++) {
        const Value &v = blockObj->getSlot(slot);

        
        if (v.isUndefined()) {
            JS_ASSERT(slot + 1 <= limit);
            continue;
        }

        Definition *dn = (Definition *) v.toPrivate();
        JS_ASSERT(dn->isDefn());
        JS_ASSERT(uintN(dn->frameSlot() + depth) < JS_BIT(16));
        dn->pn_cookie.set(dn->pn_cookie.level(), uint16(dn->frameSlot() + depth));
#ifdef DEBUG
        for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
            JS_ASSERT(pnu->pn_lexdef == dn);
            JS_ASSERT(!(pnu->pn_dflags & PND_BOUND));
            JS_ASSERT(pnu->pn_cookie.isFree());
        }
#endif

        




        bool isClosed = cg->shouldNoteClosedName(dn);
        blockObj->setSlot(slot, BooleanValue(isClosed));
    }

    




    if ((cg->flags & TCF_FUN_EXTENSIBLE_SCOPE) ||
        cg->bindings.extensibleParents()) {
        Shape *shape = blockObj->lastProperty();
        if (!Shape::setExtensibleParents(cx, &shape))
            return false;
        blockObj->setLastPropertyInfallible(shape);
    }

    return true;
}

static JSBool
EmitLeaveBlock(JSContext *cx, CodeGenerator *cg, JSOp op, ObjectBox *box)
{
    JSOp bigSuffix;
    uintN count = OBJ_BLOCK_COUNT(cx, box->object);

    bigSuffix = EmitBigIndexPrefix(cx, cg, box->index);
    if (bigSuffix == JSOP_FALSE)
        return JS_FALSE;
    if (Emit5(cx, cg, op, count, box->index) < 0)
        return JS_FALSE;
    return bigSuffix == JSOP_NOP || Emit1(cx, cg, bigSuffix) >= 0;
}






















static bool
TryConvertToGname(CodeGenerator *cg, ParseNode *pn, JSOp *op)
{
    if (cg->compileAndGo() &&
        cg->compiler()->globalScope->globalObj &&
        !cg->mightAliasLocals() &&
        !pn->isDeoptimized() &&
        !(cg->flags & TCF_STRICT_MODE_CODE))
    {
        switch (*op) {
          case JSOP_NAME:     *op = JSOP_GETGNAME; break;
          case JSOP_SETNAME:  *op = JSOP_SETGNAME; break;
          case JSOP_INCNAME:  *op = JSOP_INCGNAME; break;
          case JSOP_NAMEINC:  *op = JSOP_GNAMEINC; break;
          case JSOP_DECNAME:  *op = JSOP_DECGNAME; break;
          case JSOP_NAMEDEC:  *op = JSOP_GNAMEDEC; break;
          case JSOP_SETCONST:
          case JSOP_DELNAME:
            
            return false;
          default: JS_NOT_REACHED("gname");
        }
        return true;
    }
    return false;
}




static bool
BindKnownGlobal(JSContext *cx, CodeGenerator *cg, ParseNode *dn, ParseNode *pn, JSAtom *atom)
{
    
    JS_ASSERT(pn->pn_cookie.isFree());

    if (cg->mightAliasLocals())
        return true;

    GlobalScope *globalScope = cg->compiler()->globalScope;

    jsatomid index;
    if (dn->pn_cookie.isFree()) {
        
        
        AtomIndexPtr p = globalScope->names.lookup(atom);
        JS_ASSERT(!!p);
        index = p.value();
    } else {
        CodeGenerator *globalcg = globalScope->cg;

        
        
        if (globalcg == cg) {
            pn->pn_cookie = dn->pn_cookie;
            pn->pn_dflags |= PND_BOUND;
            return true;
        }

        
        
        index = globalcg->globalUses[dn->pn_cookie.asInteger()].slot;
    }

    if (!cg->addGlobalUse(atom, index, &pn->pn_cookie))
        return false;

    if (!pn->pn_cookie.isFree())
        pn->pn_dflags |= PND_BOUND;

    return true;
}


static bool
BindGlobal(JSContext *cx, CodeGenerator *cg, ParseNode *pn, JSAtom *atom)
{
    pn->pn_cookie.makeFree();

    Definition *dn;
    if (pn->isUsed()) {
        dn = pn->pn_lexdef;
    } else {
        if (!pn->isDefn())
            return true;
        dn = (Definition *)pn;
    }

    
    if (!dn->isGlobal())
        return true;

    return BindKnownGlobal(cx, cg, dn, pn, atom);
}


















static JSBool
BindNameToSlot(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    Definition *dn;
    JSOp op;
    JSAtom *atom;
    Definition::Kind dn_kind;

    JS_ASSERT(pn->isKind(TOK_NAME));

    
    if (pn->pn_dflags & PND_BOUND)
        return JS_TRUE;

    
    JS_ASSERT(!pn->isOp(JSOP_ARGUMENTS) && !pn->isOp(JSOP_CALLEE));

    



    if (pn->isUsed()) {
        JS_ASSERT(pn->pn_cookie.isFree());
        dn = pn->pn_lexdef;
        JS_ASSERT(dn->isDefn());
        if (pn->isDeoptimized())
            return JS_TRUE;
        pn->pn_dflags |= (dn->pn_dflags & PND_CONST);
    } else {
        if (!pn->isDefn())
            return JS_TRUE;
        dn = (Definition *) pn;
    }

    op = pn->getOp();
    if (op == JSOP_NOP)
        return JS_TRUE;

    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
    atom = pn->pn_atom;
    UpvarCookie cookie = dn->pn_cookie;
    dn_kind = dn->kind();

    








    switch (op) {
      case JSOP_NAME:
      case JSOP_SETCONST:
        break;
      case JSOP_DELNAME:
        if (dn_kind != Definition::UNKNOWN) {
            if (cg->parser->callerFrame && dn->isTopLevel())
                JS_ASSERT(cg->compileAndGo());
            else
                pn->setOp(JSOP_FALSE);
            pn->pn_dflags |= PND_BOUND;
            return JS_TRUE;
        }
        break;
      default:
        if (pn->isConst()) {
            if (cg->needStrictChecks()) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(cx, atom, &name) ||
                    !ReportStrictModeError(cx, CG_TS(cg), cg, pn, JSMSG_READ_ONLY, name.ptr())) {
                    return JS_FALSE;
                }
            }
            pn->setOp(op = JSOP_NAME);
        }
    }

    if (dn->isGlobal()) {
        if (op == JSOP_NAME) {
            




            if (!pn->pn_cookie.isFree()) {
                pn->setOp(JSOP_GETGNAME);
                pn->pn_dflags |= PND_BOUND;
                return JS_TRUE;
            }
        }

        





        cookie.makeFree();
    }

    if (cookie.isFree()) {
        StackFrame *caller = cg->parser->callerFrame;
        if (caller) {
            JS_ASSERT(cg->compileAndGo());

            



            if (cg->flags & TCF_IN_FOR_INIT)
                return JS_TRUE;

            JS_ASSERT(caller->isScriptFrame());

            



            if (caller->isGlobalFrame() && TryConvertToGname(cg, pn, &op)) {
                jsatomid _;
                if (!cg->makeAtomIndex(atom, &_))
                    return JS_FALSE;

                pn->setOp(op);
                pn->pn_dflags |= PND_BOUND;
                return JS_TRUE;
            }

            



            return JS_TRUE;
        }

        
        if (!cg->mightAliasLocals() && !TryConvertToGname(cg, pn, &op))
            return JS_TRUE;

        jsatomid _;
        if (!cg->makeAtomIndex(atom, &_))
            return JS_FALSE;

        pn->setOp(op);
        pn->pn_dflags |= PND_BOUND;

        return JS_TRUE;
    }

    uint16 level = cookie.level();
    JS_ASSERT(cg->staticLevel >= level);

    const uintN skip = cg->staticLevel - level;
    if (skip != 0) {
        JS_ASSERT(cg->inFunction());
        JS_ASSERT_IF(cookie.slot() != UpvarCookie::CALLEE_SLOT, cg->roLexdeps->lookup(atom));
        JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

        



        if (op != JSOP_NAME)
            return JS_TRUE;
        if (skip >= UpvarCookie::UPVAR_LEVEL_LIMIT)
            return JS_TRUE;
        if (cg->flags & TCF_FUN_HEAVYWEIGHT)
            return JS_TRUE;

        if (!cg->fun()->isFlatClosure())
            return JS_TRUE;

        if (!cg->upvarIndices.ensureMap(cx))
            return JS_FALSE;

        AtomIndexAddPtr p = cg->upvarIndices->lookupForAdd(atom);
        jsatomid index;
        if (p) {
            index = p.value();
        } else {
            if (!cg->bindings.addUpvar(cx, atom))
                return JS_FALSE;

            index = cg->upvarIndices->count();
            if (!cg->upvarIndices->add(p, atom, index))
                return JS_FALSE;

            UpvarCookies &upvarMap = cg->upvarMap;
            
            size_t lexdepCount = cg->roLexdeps->count();

            JS_ASSERT_IF(!upvarMap.empty(), lexdepCount == upvarMap.length());
            if (upvarMap.empty()) {
                
                if (lexdepCount <= upvarMap.sMaxInlineStorage) {
                    JS_ALWAYS_TRUE(upvarMap.growByUninitialized(lexdepCount));
                } else {
                    void *buf = upvarMap.allocPolicy().malloc_(lexdepCount * sizeof(UpvarCookie));
                    if (!buf)
                        return JS_FALSE;
                    upvarMap.replaceRawBuffer(static_cast<UpvarCookie *>(buf), lexdepCount);
                }
                for (size_t i = 0; i < lexdepCount; ++i)
                    upvarMap[i] = UpvarCookie();
            }

            uintN slot = cookie.slot();
            if (slot != UpvarCookie::CALLEE_SLOT && dn_kind != Definition::ARG) {
                TreeContext *tc = cg;
                do {
                    tc = tc->parent;
                } while (tc->staticLevel != level);
                if (tc->inFunction())
                    slot += tc->fun()->nargs;
            }

            JS_ASSERT(index < upvarMap.length());
            upvarMap[index].set(skip, slot);
        }

        pn->setOp(JSOP_GETFCSLOT);
        JS_ASSERT((index & JS_BITMASK(16)) == index);
        pn->pn_cookie.set(0, index);
        pn->pn_dflags |= PND_BOUND;
        return JS_TRUE;
    }

    




    switch (dn_kind) {
      case Definition::UNKNOWN:
        return JS_TRUE;

      case Definition::LET:
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETLOCAL; break;
          case JSOP_SETNAME:  op = JSOP_SETLOCAL; break;
          case JSOP_INCNAME:  op = JSOP_INCLOCAL; break;
          case JSOP_NAMEINC:  op = JSOP_LOCALINC; break;
          case JSOP_DECNAME:  op = JSOP_DECLOCAL; break;
          case JSOP_NAMEDEC:  op = JSOP_LOCALDEC; break;
          default: JS_NOT_REACHED("let");
        }
        break;

      case Definition::ARG:
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETARG; break;
          case JSOP_SETNAME:  op = JSOP_SETARG; break;
          case JSOP_INCNAME:  op = JSOP_INCARG; break;
          case JSOP_NAMEINC:  op = JSOP_ARGINC; break;
          case JSOP_DECNAME:  op = JSOP_DECARG; break;
          case JSOP_NAMEDEC:  op = JSOP_ARGDEC; break;
          default: JS_NOT_REACHED("arg");
        }
        JS_ASSERT(!pn->isConst());
        break;

      case Definition::VAR:
        if (dn->isOp(JSOP_CALLEE)) {
            JS_ASSERT(op != JSOP_CALLEE);
            JS_ASSERT((cg->fun()->flags & JSFUN_LAMBDA) && atom == cg->fun()->atom);

            























            JS_ASSERT(op != JSOP_DELNAME);
            if (!(cg->flags & TCF_FUN_HEAVYWEIGHT)) {
                op = JSOP_CALLEE;
                pn->pn_dflags |= PND_CONST;
            }

            pn->setOp(op);
            pn->pn_dflags |= PND_BOUND;
            return JS_TRUE;
        }
        

      default:
        JS_ASSERT_IF(dn_kind != Definition::FUNCTION,
                     dn_kind == Definition::VAR ||
                     dn_kind == Definition::CONST);
        switch (op) {
          case JSOP_NAME:     op = JSOP_GETLOCAL; break;
          case JSOP_SETNAME:  op = JSOP_SETLOCAL; break;
          case JSOP_SETCONST: op = JSOP_SETLOCAL; break;
          case JSOP_INCNAME:  op = JSOP_INCLOCAL; break;
          case JSOP_NAMEINC:  op = JSOP_LOCALINC; break;
          case JSOP_DECNAME:  op = JSOP_DECLOCAL; break;
          case JSOP_NAMEDEC:  op = JSOP_LOCALDEC; break;
          default: JS_NOT_REACHED("local");
        }
        JS_ASSERT_IF(dn_kind == Definition::CONST, pn->pn_dflags & PND_CONST);
        break;
    }

    JS_ASSERT(!pn->isOp(op));
    pn->setOp(op);
    pn->pn_cookie.set(0, cookie.slot());
    pn->pn_dflags |= PND_BOUND;
    return JS_TRUE;
}

bool
CodeGenerator::addGlobalUse(JSAtom *atom, uint32 slot, UpvarCookie *cookie)
{
    if (!globalMap.ensureMap(context()))
        return false;

    AtomIndexAddPtr p = globalMap->lookupForAdd(atom);
    if (p) {
        jsatomid index = p.value();
        cookie->set(0, index);
        return true;
    }

    
    if (globalUses.length() >= UINT16_LIMIT) {
        cookie->makeFree();
        return true;
    }

    
    jsatomid allAtomIndex;
    if (!makeAtomIndex(atom, &allAtomIndex))
        return false;

    jsatomid globalUseIndex = globalUses.length();
    cookie->set(0, globalUseIndex);

    GlobalSlotArray::Entry entry = { allAtomIndex, slot };
    if (!globalUses.append(entry))
        return false;

    return globalMap->add(p, atom, globalUseIndex);
}













static JSBool
CheckSideEffects(JSContext *cx, CodeGenerator *cg, ParseNode *pn, JSBool *answer)
{
    JSBool ok;
    ParseNode *pn2;

    ok = JS_TRUE;
    if (!pn || *answer)
        return ok;

    switch (pn->getArity()) {
      case PN_FUNC:
        






        *answer = JS_FALSE;
        break;

      case PN_LIST:
        if (pn->isOp(JSOP_NOP) || pn->isOp(JSOP_OR) || pn->isOp(JSOP_AND) ||
            pn->isOp(JSOP_STRICTEQ) || pn->isOp(JSOP_STRICTNE)) {
            



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
        if (pn->isKind(TOK_ASSIGN)) {
            








            pn2 = pn->pn_left;
            if (!pn2->isKind(TOK_NAME)) {
                *answer = JS_TRUE;
            } else {
                if (!BindNameToSlot(cx, cg, pn2))
                    return JS_FALSE;
                if (!CheckSideEffects(cx, cg, pn->pn_right, answer))
                    return JS_FALSE;
                if (!*answer && (!pn->isOp(JSOP_NOP) || !pn2->isConst()))
                    *answer = JS_TRUE;
            }
        } else {
            if (pn->isOp(JSOP_OR) || pn->isOp(JSOP_AND) || pn->isOp(JSOP_STRICTEQ) ||
                pn->isOp(JSOP_STRICTNE)) {
                



                ok = CheckSideEffects(cx, cg, pn->pn_left, answer) &&
                     CheckSideEffects(cx, cg, pn->pn_right, answer);
            } else {
                



                *answer = JS_TRUE;
            }
        }
        break;

      case PN_UNARY:
        switch (pn->getKind()) {
          case TOK_DELETE:
            pn2 = pn->pn_kid;
            switch (pn2->getKind()) {
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
                JS_ASSERT_IF(pn2->getKind() == TOK_DBLDOT, !cg->inStrictMode());
                

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
            if (pn->isOp(JSOP_NOT)) {
                
                ok = CheckSideEffects(cx, cg, pn->pn_kid, answer);
                break;
            }
            

          default:
            





            *answer = JS_TRUE;
            break;
        }
        break;

      case PN_NAME:
        




        if (pn->isKind(TOK_NAME) && !pn->isOp(JSOP_NOP)) {
            if (!BindNameToSlot(cx, cg, pn))
                return JS_FALSE;
            if (!pn->isOp(JSOP_ARGUMENTS) && !pn->isOp(JSOP_CALLEE) &&
                pn->pn_cookie.isFree()) {
                




                *answer = JS_TRUE;
            }
        }
        pn2 = pn->maybeExpr();
        if (pn->isKind(TOK_DOT)) {
            if (pn2->isKind(TOK_NAME) && !BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            if (!(pn2->isOp(JSOP_ARGUMENTS) &&
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
        if (pn->isKind(TOK_DEBUGGER))
            *answer = JS_TRUE;
        break;
    }
    return ok;
}

static JSBool
EmitNameOp(JSContext *cx, CodeGenerator *cg, ParseNode *pn, JSBool callContext)
{
    JSOp op;

    if (!BindNameToSlot(cx, cg, pn))
        return JS_FALSE;
    op = pn->getOp();

    if (callContext) {
        switch (op) {
          case JSOP_NAME:
            op = JSOP_CALLNAME;
            break;
          case JSOP_GETGNAME:
            op = JSOP_CALLGNAME;
            break;
          case JSOP_GETARG:
            op = JSOP_CALLARG;
            break;
          case JSOP_GETLOCAL:
            op = JSOP_CALLLOCAL;
            break;
          case JSOP_GETFCSLOT:
            op = JSOP_CALLFCSLOT;
            break;
          default:
            JS_ASSERT(op == JSOP_ARGUMENTS || op == JSOP_CALLEE);
            break;
        }
    }

    if (op == JSOP_ARGUMENTS || op == JSOP_CALLEE) {
        if (Emit1(cx, cg, op) < 0)
            return JS_FALSE;
        if (callContext && Emit1(cx, cg, JSOP_PUSH) < 0)
            return JS_FALSE;
    } else {
        if (!pn->pn_cookie.isFree()) {
            EMIT_UINT16_IMM_OP(op, pn->pn_cookie.asInteger());
        } else {
            if (!EmitAtomOp(cx, pn, op, cg))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
}

#if JS_HAS_XML_SUPPORT
static bool
EmitXMLName(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg)
{
    JS_ASSERT(!cg->inStrictMode());
    JS_ASSERT(pn->isKind(TOK_UNARYOP));
    JS_ASSERT(pn->isOp(JSOP_XMLNAME));
    JS_ASSERT(op == JSOP_XMLNAME || op == JSOP_CALLXMLNAME);

    ParseNode *pn2 = pn->pn_kid;
    uintN oldflags = cg->flags;
    cg->flags &= ~TCF_IN_FOR_INIT;
    if (!EmitTree(cx, cg, pn2))
        return false;
    cg->flags |= oldflags & TCF_IN_FOR_INIT;
    if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - pn2->pn_offset) < 0)
        return false;

    return Emit1(cx, cg, op) >= 0;
}
#endif

static inline bool
EmitElemOpBase(JSContext *cx, CodeGenerator *cg, JSOp op)
{
    if (Emit1(cx, cg, op) < 0)
        return false;
    CheckTypeSet(cx, cg, op);
    return true;
}

static bool
EmitSpecialPropOp(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg)
{
    




    jsatomid index;
    if (!cg->makeAtomIndex(pn->pn_atom, &index))
        return false;
    if (!EmitIndexOp(cx, JSOP_QNAMEPART, index, cg))
        return false;
    return EmitElemOpBase(cx, cg, op);
}

static bool
EmitPropOp(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg,
           JSBool callContext, JSOp *psuffix = NULL)
{
    ParseNode *pn2, *pndot, *pnup, *pndown;
    ptrdiff_t top;

    JS_ASSERT(pn->isArity(PN_NAME));
    pn2 = pn->maybeExpr();

    
    if ((op == JSOP_GETPROP || op == JSOP_CALLPROP) &&
        pn->pn_atom == cx->runtime->atomState.protoAtom) {
        if (pn2 && !EmitTree(cx, cg, pn2))
            return false;
        return EmitSpecialPropOp(cx, pn, callContext ? JSOP_CALLELEM : JSOP_GETELEM, cg);
    }

    if (callContext) {
        JS_ASSERT(pn->isKind(TOK_DOT));
        JS_ASSERT(op == JSOP_GETPROP);
        op = JSOP_CALLPROP;
    } else if (op == JSOP_GETPROP && pn->isKind(TOK_DOT)) {
        if (pn2->isKind(TOK_NAME)) {
            



            if (!BindNameToSlot(cx, cg, pn2))
                return false;
            if (!cx->typeInferenceEnabled() &&
                pn->pn_atom == cx->runtime->atomState.lengthAtom) {
                if (pn2->isOp(JSOP_ARGUMENTS))
                    return Emit1(cx, cg, JSOP_ARGCNT) >= 0;
            }
        }
    }

    




    if (pn2->isKind(TOK_DOT)) {
        pndot = pn2;
        pnup = NULL;
        top = CG_OFFSET(cg);
        for (;;) {
            
            pndot->pn_offset = top;
            JS_ASSERT(!pndot->isUsed());
            pndown = pndot->pn_expr;
            pndot->pn_expr = pnup;
            if (!pndown->isKind(TOK_DOT))
                break;
            pnup = pndot;
            pndot = pndown;
        }

        
        if (!EmitTree(cx, cg, pndown))
            return false;

        do {
            
            if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - pndown->pn_offset) < 0)
                return false;

            
            if (pndot->isArity(PN_NAME) && pndot->pn_atom == cx->runtime->atomState.protoAtom) {
                if (!EmitSpecialPropOp(cx, pndot, JSOP_GETELEM, cg))
                    return false;
            } else if (!EmitAtomOp(cx, pndot, pndot->getOp(), cg)) {
                return false;
            }

            
            pnup = pndot->pn_expr;
            pndot->pn_expr = pndown;
            pndown = pndot;
        } while ((pndot = pnup) != NULL);
    } else {
        if (!EmitTree(cx, cg, pn2))
            return false;
    }

    if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - pn2->pn_offset) < 0)
        return false;

    return EmitAtomOp(cx, pn, op, cg, psuffix);
}

static bool
EmitPropIncDec(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg)
{
    JSOp suffix = JSOP_NOP;
    if (!EmitPropOp(cx, pn, op, cg, false, &suffix))
        return false;
    if (Emit1(cx, cg, JSOP_NOP) < 0)
        return false;

    



    int start = CG_OFFSET(cg);

    if (suffix != JSOP_NOP && Emit1(cx, cg, suffix) < 0)
        return false;

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT(cs->format & JOF_PROP);
    JS_ASSERT(cs->format & (JOF_INC | JOF_DEC));

    bool post = (cs->format & JOF_POST);
    JSOp binop = (cs->format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

                                                   
    if (Emit1(cx, cg, JSOP_DUP) < 0)               
        return false;
    if (!EmitAtomOp(cx, pn, JSOP_GETPROP, cg))     
        return false;
    if (Emit1(cx, cg, JSOP_POS) < 0)               
        return false;
    if (post && Emit1(cx, cg, JSOP_DUP) < 0)       
        return false;
    if (Emit1(cx, cg, JSOP_ONE) < 0)               
        return false;
    if (Emit1(cx, cg, binop) < 0)                  
        return false;

    if (post) {
        if (Emit2(cx, cg, JSOP_PICK, (jsbytecode)2) < 0) 
            return false;
        if (Emit1(cx, cg, JSOP_SWAP) < 0)                
            return false;
    }

    if (!EmitAtomOp(cx, pn, JSOP_SETPROP, cg))     
        return false;
    if (post && Emit1(cx, cg, JSOP_POP) < 0)       
        return false;

    UpdateDecomposeLength(cg, start);

    if (suffix != JSOP_NOP && Emit1(cx, cg, suffix) < 0)
        return false;

    return true;
}

static bool
EmitNameIncDec(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg)
{
    JSOp suffix = JSOP_NOP;
    if (!EmitAtomOp(cx, pn, op, cg, &suffix))
        return false;
    if (Emit1(cx, cg, JSOP_NOP) < 0)
        return false;

    
    cg->stackDepth--;

    int start = CG_OFFSET(cg);

    if (suffix != JSOP_NOP && Emit1(cx, cg, suffix) < 0)
        return false;

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT((cs->format & JOF_NAME) || (cs->format & JOF_GNAME));
    JS_ASSERT(cs->format & (JOF_INC | JOF_DEC));

    bool global = (cs->format & JOF_GNAME);
    bool post = (cs->format & JOF_POST);
    JSOp binop = (cs->format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

    if (!EmitAtomOp(cx, pn, global ? JSOP_BINDGNAME : JSOP_BINDNAME, cg))  
        return false;
    if (!EmitAtomOp(cx, pn, global ? JSOP_GETGNAME : JSOP_NAME, cg))       
        return false;
    if (Emit1(cx, cg, JSOP_POS) < 0)               
        return false;
    if (post && Emit1(cx, cg, JSOP_DUP) < 0)       
        return false;
    if (Emit1(cx, cg, JSOP_ONE) < 0)               
        return false;
    if (Emit1(cx, cg, binop) < 0)                  
        return false;

    if (post) {
        if (Emit2(cx, cg, JSOP_PICK, (jsbytecode)2) < 0)    
            return false;
        if (Emit1(cx, cg, JSOP_SWAP) < 0)                   
            return false;
    }

    if (!EmitAtomOp(cx, pn, global ? JSOP_SETGNAME : JSOP_SETNAME, cg))     
        return false;
    if (post && Emit1(cx, cg, JSOP_POP) < 0)       
        return false;

    UpdateDecomposeLength(cg, start);

    if (suffix != JSOP_NOP && Emit1(cx, cg, suffix) < 0)
        return false;

    return true;
}

static JSBool
EmitElemOp(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg)
{
    ptrdiff_t top;
    ParseNode *left, *right, *next;
    int32_t slot;

    top = CG_OFFSET(cg);
    if (pn->isArity(PN_LIST)) {
        
        JS_ASSERT(pn->isOp(JSOP_GETELEM));
        JS_ASSERT(pn->pn_count >= 3);
        left = pn->pn_head;
        right = pn->last();
        next = left->pn_next;
        JS_ASSERT(next != right);

        




        if (left->isKind(TOK_NAME) && next->isKind(TOK_NUMBER)) {
            if (!BindNameToSlot(cx, cg, left))
                return false;
            if (left->isOp(JSOP_ARGUMENTS) &&
                JSDOUBLE_IS_INT32(next->pn_dval, &slot) &&
                jsuint(slot) < JS_BIT(16) &&
                !cx->typeInferenceEnabled() &&
                (!cg->inStrictMode() ||
                 (!cg->mutatesParameter() && !cg->callsEval()))) {
                



                JS_ASSERT(op != JSOP_CALLELEM || next->pn_next);
                left->pn_offset = next->pn_offset = top;
                EMIT_UINT16_IMM_OP(JSOP_ARGSUB, (jsatomid)slot);
                left = next;
                next = left->pn_next;
            }
        }

        






        JS_ASSERT(next != right || pn->pn_count == 3);
        if (left == pn->pn_head) {
            if (!EmitTree(cx, cg, left))
                return false;
        }
        while (next != right) {
            if (!EmitTree(cx, cg, next))
                return false;
            if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
                return false;
            if (!EmitElemOpBase(cx, cg, JSOP_GETELEM))
                return false;
            next = next->pn_next;
        }
    } else {
        if (pn->isArity(PN_NAME)) {
            






            left = pn->maybeExpr();
            if (!left) {
                left = NullaryNode::create(cg);
                if (!left)
                    return false;
                left->setKind(TOK_STRING);
                left->setOp(JSOP_BINDNAME);
                left->pn_pos = pn->pn_pos;
                left->pn_atom = pn->pn_atom;
            }
            right = NullaryNode::create(cg);
            if (!right)
                return false;
            right->setKind(TOK_STRING);
            right->setOp(IsIdentifier(pn->pn_atom) ? JSOP_QNAMEPART : JSOP_STRING);
            right->pn_pos = pn->pn_pos;
            right->pn_atom = pn->pn_atom;
        } else {
            JS_ASSERT(pn->isArity(PN_BINARY));
            left = pn->pn_left;
            right = pn->pn_right;
        }

        



        if (op == JSOP_GETELEM &&
            left->isKind(TOK_NAME) &&
            right->isKind(TOK_NUMBER)) {
            if (!BindNameToSlot(cx, cg, left))
                return false;
            if (left->isOp(JSOP_ARGUMENTS) &&
                JSDOUBLE_IS_INT32(right->pn_dval, &slot) &&
                jsuint(slot) < JS_BIT(16) &&
                !cx->typeInferenceEnabled() &&
                (!cg->inStrictMode() ||
                 (!cg->mutatesParameter() && !cg->callsEval()))) {
                left->pn_offset = right->pn_offset = top;
                EMIT_UINT16_IMM_OP(JSOP_ARGSUB, (jsatomid)slot);
                return true;
            }
        }

        if (!EmitTree(cx, cg, left))
            return false;
    }

    
    JS_ASSERT(op != JSOP_DESCENDANTS || !right->isKind(TOK_STRING) ||
              right->isOp(JSOP_QNAMEPART));
    if (!EmitTree(cx, cg, right))
        return false;
    if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
        return false;
    return EmitElemOpBase(cx, cg, op);
}

static bool
EmitElemIncDec(JSContext *cx, ParseNode *pn, JSOp op, CodeGenerator *cg)
{
    if (pn) {
        if (!EmitElemOp(cx, pn, op, cg))
            return false;
    } else {
        if (!EmitElemOpBase(cx, cg, op))
            return false;
    }
    if (Emit1(cx, cg, JSOP_NOP) < 0)
        return false;

    
    cg->stackDepth++;

    int start = CG_OFFSET(cg);

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT(cs->format & JOF_ELEM);
    JS_ASSERT(cs->format & (JOF_INC | JOF_DEC));

    bool post = (cs->format & JOF_POST);
    JSOp binop = (cs->format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

    



                                                 
    if (Emit1(cx, cg, JSOP_TOID) < 0)            
        return false;
    if (Emit1(cx, cg, JSOP_DUP2) < 0)            
        return false;
    if (!EmitElemOpBase(cx, cg, JSOP_GETELEM))   
        return false;
    if (Emit1(cx, cg, JSOP_POS) < 0)             
        return false;
    if (post && Emit1(cx, cg, JSOP_DUP) < 0)     
        return false;
    if (Emit1(cx, cg, JSOP_ONE) < 0)             
        return false;
    if (Emit1(cx, cg, binop) < 0)                
        return false;

    if (post) {
        if (Emit2(cx, cg, JSOP_PICK, (jsbytecode)3) < 0)    
            return false;
        if (Emit2(cx, cg, JSOP_PICK, (jsbytecode)3) < 0)    
            return false;
        if (Emit2(cx, cg, JSOP_PICK, (jsbytecode)2) < 0)    
            return false;
    }

    if (!EmitElemOpBase(cx, cg, JSOP_SETELEM))   
        return false;
    if (post && Emit1(cx, cg, JSOP_POP) < 0)     
        return false;

    UpdateDecomposeLength(cg, start);

    return true;
}

static JSBool
EmitNumberOp(JSContext *cx, jsdouble dval, CodeGenerator *cg)
{
    int32_t ival;
    uint32 u;
    ptrdiff_t off;
    jsbytecode *pc;

    if (JSDOUBLE_IS_INT32(dval, &ival)) {
        if (ival == 0)
            return Emit1(cx, cg, JSOP_ZERO) >= 0;
        if (ival == 1)
            return Emit1(cx, cg, JSOP_ONE) >= 0;
        if ((jsint)(int8)ival == ival)
            return Emit2(cx, cg, JSOP_INT8, (jsbytecode)(int8)ival) >= 0;

        u = (uint32)ival;
        if (u < JS_BIT(16)) {
            EMIT_UINT16_IMM_OP(JSOP_UINT16, u);
        } else if (u < JS_BIT(24)) {
            off = EmitN(cx, cg, JSOP_UINT24, 3);
            if (off < 0)
                return JS_FALSE;
            pc = CG_CODE(cg, off);
            SET_UINT24(pc, u);
        } else {
            off = EmitN(cx, cg, JSOP_INT32, 4);
            if (off < 0)
                return JS_FALSE;
            pc = CG_CODE(cg, off);
            SET_INT32(pc, ival);
        }
        return JS_TRUE;
    }

    if (!cg->constList.append(DoubleValue(dval)))
        return JS_FALSE;

    return EmitIndexOp(cx, JSOP_DOUBLE, cg->constList.length() - 1, cg);
}









static Value *
AllocateSwitchConstant(JSContext *cx)
{
    return cx->tempLifoAlloc().new_<Value>();
}













class TempPopScope {
    StmtInfo *savedStmt;
    StmtInfo *savedScopeStmt;
    ObjectBox *savedBlockBox;

  public:
    TempPopScope() : savedStmt(NULL), savedScopeStmt(NULL), savedBlockBox(NULL) {}

    bool popBlock(JSContext *cx, CodeGenerator *cg) {
        savedStmt = cg->topStmt;
        savedScopeStmt = cg->topScopeStmt;
        savedBlockBox = cg->blockChainBox;

        if (cg->topStmt->type == STMT_FOR_LOOP || cg->topStmt->type == STMT_FOR_IN_LOOP)
            PopStatementTC(cg);
        JS_ASSERT(STMT_LINKS_SCOPE(cg->topStmt));
        JS_ASSERT(cg->topStmt->flags & SIF_SCOPE);
        PopStatementTC(cg);

        






        return Emit1(cx, cg, JSOP_NOP) >= 0 && EmitBlockChain(cx, cg);
    }

    bool repushBlock(JSContext *cx, CodeGenerator *cg) {
        JS_ASSERT(savedStmt);
        cg->topStmt = savedStmt;
        cg->topScopeStmt = savedScopeStmt;
        cg->blockChainBox = savedBlockBox;
        return Emit1(cx, cg, JSOP_NOP) >= 0 && EmitBlockChain(cx, cg);
    }
};

static JSBool
EmitSwitch(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    JSOp switchOp;
    JSBool ok, hasDefault, constPropagated;
    ptrdiff_t top, off, defaultOffset;
    ParseNode *pn2, *pn3, *pn4;
    uint32 caseCount, tableLength;
    ParseNode **table;
    int32_t i, low, high;
    intN noteIndex;
    size_t switchSize, tableSize;
    jsbytecode *pc, *savepc;
#if JS_HAS_BLOCK_SCOPE
    ObjectBox *box;
#endif
    StmtInfo stmtInfo;

    
    switchOp = JSOP_TABLESWITCH;
    ok = JS_TRUE;
    hasDefault = constPropagated = JS_FALSE;
    defaultOffset = -1;

    





    pn2 = pn->pn_right;
#if JS_HAS_BLOCK_SCOPE
    TempPopScope tps;
    if (pn2->isKind(TOK_LEXICALSCOPE)) {
        





        box = pn2->pn_objbox;
        PushBlockScope(cg, &stmtInfo, box, -1);
        stmtInfo.type = STMT_SWITCH;

        
        if (!EmitEnterBlock(cx, pn2, cg))
            return JS_FALSE;

        



        if (!tps.popBlock(cx, cg))
            return JS_FALSE;
    }
#ifdef __GNUC__
    else {
        box = NULL;
    }
#endif
#endif

    



    if (!EmitTree(cx, cg, pn->pn_left))
        return JS_FALSE;

    
    top = CG_OFFSET(cg);
#if !JS_HAS_BLOCK_SCOPE
    PushStatement(cg, &stmtInfo, STMT_SWITCH, top);
#else
    if (pn2->isKind(TOK_LC)) {
        PushStatement(cg, &stmtInfo, STMT_SWITCH, top);
    } else {
        
        if (!tps.repushBlock(cx, cg))
            return JS_FALSE;

        



        stmtInfo.update = top = CG_OFFSET(cg);

        
        pn2 = pn2->expr();
    }
#endif

    caseCount = pn2->pn_count;
    tableLength = 0;
    table = NULL;

    if (caseCount == 0 ||
        (caseCount == 1 &&
         (hasDefault = (pn2->pn_head->isKind(TOK_DEFAULT))))) {
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
            if (pn3->isKind(TOK_DEFAULT)) {
                hasDefault = JS_TRUE;
                caseCount--;    
                continue;
            }

            JS_ASSERT(pn3->isKind(TOK_CASE));
            if (switchOp == JSOP_CONDSWITCH)
                continue;

            pn4 = pn3->pn_left;
            while (pn4->isKind(TOK_RP))
                pn4 = pn4->pn_kid;

            Value constVal;
            switch (pn4->getKind()) {
              case TOK_NUMBER:
                constVal.setNumber(pn4->pn_dval);
                break;
              case TOK_STRING:
                constVal.setString(pn4->pn_atom);
                break;
              case TOK_NAME:
                if (!pn4->maybeExpr()) {
                    ok = LookupCompileTimeConstant(cx, cg, pn4->pn_atom, &constVal);
                    if (!ok)
                        goto release;
                    if (!constVal.isMagic(JS_NO_CONSTANT)) {
                        if (constVal.isObject()) {
                            



                            switchOp = JSOP_CONDSWITCH;
                            continue;
                        }
                        constPropagated = JS_TRUE;
                        break;
                    }
                }
                
              case TOK_PRIMARY:
                if (pn4->isOp(JSOP_TRUE)) {
                    constVal.setBoolean(true);
                    break;
                }
                if (pn4->isOp(JSOP_FALSE)) {
                    constVal.setBoolean(false);
                    break;
                }
                if (pn4->isOp(JSOP_NULL)) {
                    constVal.setNull();
                    break;
                }
                
              default:
                switchOp = JSOP_CONDSWITCH;
                continue;
            }
            JS_ASSERT(constVal.isPrimitive());

            pn3->pn_pval = AllocateSwitchConstant(cx);
            if (!pn3->pn_pval) {
                ok = JS_FALSE;
                goto release;
            }

            *pn3->pn_pval = constVal;

            if (switchOp != JSOP_TABLESWITCH)
                continue;
            if (!pn3->pn_pval->isInt32()) {
                switchOp = JSOP_LOOKUPSWITCH;
                continue;
            }
            i = pn3->pn_pval->toInt32();
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
                        cx->malloc_((JS_BIT(16) >> JS_BITS_PER_WORD_LOG2)
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
            cx->free_(intmap);
        if (!ok)
            return JS_FALSE;

        



        if (switchOp == JSOP_TABLESWITCH) {
            tableLength = (uint32)(high - low + 1);
            if (tableLength >= JS_BIT(16) || tableLength > 2 * caseCount)
                switchOp = JSOP_LOOKUPSWITCH;
        } else if (switchOp == JSOP_LOOKUPSWITCH) {
            





            if (caseCount + cg->constList.length() > JS_BIT(16))
                switchOp = JSOP_CONDSWITCH;
        }
    }

    



    noteIndex = NewSrcNote3(cx, cg, SRC_SWITCH, 0, 0);
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

    










    if (EmitN(cx, cg, switchOp, switchSize) < 0)
        return JS_FALSE;

    off = -1;
    if (switchOp == JSOP_CONDSWITCH) {
        intN caseNoteIndex = -1;
        JSBool beforeCases = JS_TRUE;

        
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            pn4 = pn3->pn_left;
            if (pn4 && !EmitTree(cx, cg, pn4))
                return JS_FALSE;
            if (caseNoteIndex >= 0) {
                
                if (!SetSrcNoteOffset(cx, cg, (uintN)caseNoteIndex, 0, CG_OFFSET(cg) - off))
                    return JS_FALSE;
            }
            if (!pn4) {
                JS_ASSERT(pn3->isKind(TOK_DEFAULT));
                continue;
            }
            caseNoteIndex = NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
            if (caseNoteIndex < 0)
                return JS_FALSE;
            off = EmitJump(cx, cg, JSOP_CASE, 0);
            if (off < 0)
                return JS_FALSE;
            pn3->pn_offset = off;
            if (beforeCases) {
                uintN noteCount, noteCountDelta;

                
                noteCount = CG_NOTE_COUNT(cg);
                if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 1, off - top))
                    return JS_FALSE;
                noteCountDelta = CG_NOTE_COUNT(cg) - noteCount;
                if (noteCountDelta != 0)
                    caseNoteIndex += noteCountDelta;
                beforeCases = JS_FALSE;
            }
        }

        





        if (!hasDefault &&
            caseNoteIndex >= 0 &&
            !SetSrcNoteOffset(cx, cg, (uintN)caseNoteIndex, 0, CG_OFFSET(cg) - off))
        {
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
                table = (ParseNode **) cx->malloc_(tableSize);
                if (!table)
                    return JS_FALSE;
                memset(table, 0, tableSize);
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    if (pn3->isKind(TOK_DEFAULT))
                        continue;
                    i = pn3->pn_pval->toInt32();
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
                        pn4->isKind(TOK_NAME)) {
                        
                        JS_ASSERT(!pn4->maybeExpr());
                        jsatomid index;
                        if (!cg->makeAtomIndex(pn4->pn_atom, &index))
                            goto bad;
                        CG_NEXT(cg) = pc;
                        if (NewSrcNote2(cx, cg, SRC_LABEL, ptrdiff_t(index)) < 0)
                            goto bad;
                    }
                    pc += JUMP_OFFSET_LEN;
                }
            } else {
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    pn4 = pn3->pn_left;
                    if (pn4 && pn4->isKind(TOK_NAME)) {
                        
                        JS_ASSERT(!pn4->maybeExpr());
                        jsatomid index;
                        if (!cg->makeAtomIndex(pn4->pn_atom, &index))
                            goto bad;
                        CG_NEXT(cg) = pc;
                        if (NewSrcNote2(cx, cg, SRC_LABEL, ptrdiff_t(index)) < 0)
                            goto bad;
                    }
                    pc += INDEX_LEN + JUMP_OFFSET_LEN;
                }
            }
            CG_NEXT(cg) = savepc;
        }
    }

    
    for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
        if (switchOp == JSOP_CONDSWITCH && !pn3->isKind(TOK_DEFAULT))
            CHECK_AND_SET_JUMP_OFFSET_AT_CUSTOM(cx, cg, pn3->pn_offset, goto bad);
        pn4 = pn3->pn_right;
        ok = EmitTree(cx, cg, pn4);
        if (!ok)
            goto out;
        pn3->pn_offset = pn4->pn_offset;
        if (pn3->isKind(TOK_DEFAULT))
            off = pn3->pn_offset - top;
    }

    if (!hasDefault) {
        
        off = CG_OFFSET(cg) - top;
    }

    
    JS_ASSERT(off != -1);

    
    if (switchOp == JSOP_CONDSWITCH) {
        pc = NULL;
        JS_ASSERT(defaultOffset != -1);
        ok = SetJumpOffset(cx, cg, CG_CODE(cg, defaultOffset), off - (defaultOffset - top));
        if (!ok)
            goto out;
    } else {
        pc = CG_CODE(cg, top);
        ok = SetJumpOffset(cx, cg, pc, off);
        if (!ok)
            goto out;
        pc += JUMP_OFFSET_LEN;
    }

    
    off = CG_OFFSET(cg) - top;
    ok = SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, off);
    if (!ok)
        goto out;

    if (switchOp == JSOP_TABLESWITCH) {
        
        pc += 2 * JUMP_OFFSET_LEN;

        
        for (i = 0; i < (jsint)tableLength; i++) {
            pn3 = table[i];
            off = pn3 ? pn3->pn_offset - top : 0;
            ok = SetJumpOffset(cx, cg, pc, off);
            if (!ok)
                goto out;
            pc += JUMP_OFFSET_LEN;
        }
    } else if (switchOp == JSOP_LOOKUPSWITCH) {
        
        pc += INDEX_LEN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->isKind(TOK_DEFAULT))
                continue;
            if (!cg->constList.append(*pn3->pn_pval))
                goto bad;
            SET_INDEX(pc, cg->constList.length() - 1);
            pc += INDEX_LEN;

            off = pn3->pn_offset - top;
            ok = SetJumpOffset(cx, cg, pc, off);
            if (!ok)
                goto out;
            pc += JUMP_OFFSET_LEN;
        }
    }

out:
    if (table)
        cx->free_(table);
    if (ok) {
        ok = PopStatementCG(cx, cg);

#if JS_HAS_BLOCK_SCOPE
        if (ok && pn->pn_right->isKind(TOK_LEXICALSCOPE))
            ok = EmitLeaveBlock(cx, cg, JSOP_LEAVEBLOCK, box);
#endif
    }
    return ok;

bad:
    ok = JS_FALSE;
    goto out;
}

JSBool
frontend::EmitFunctionScript(JSContext *cx, CodeGenerator *cg, ParseNode *body)
{
    






    if (cg->flags & TCF_FUN_IS_GENERATOR) {
        
        CG_SWITCH_TO_PROLOG(cg);
        JS_ASSERT(CG_NEXT(cg) == CG_BASE(cg));
        if (Emit1(cx, cg, JSOP_GENERATOR) < 0)
            return false;
        CG_SWITCH_TO_MAIN(cg);
    }

    






    if (cg->needsEagerArguments()) {
        CG_SWITCH_TO_PROLOG(cg);
        if (Emit1(cx, cg, JSOP_ARGUMENTS) < 0 || Emit1(cx, cg, JSOP_POP) < 0)
            return false;
        CG_SWITCH_TO_MAIN(cg);
    }

    return EmitTree(cx, cg, body) &&
           Emit1(cx, cg, JSOP_STOP) >= 0 &&
           JSScript::NewScriptFromCG(cx, cg);
}

static bool
MaybeEmitVarDecl(JSContext *cx, CodeGenerator *cg, JSOp prologOp,
                 ParseNode *pn, jsatomid *result)
{
    jsatomid atomIndex;

    if (!pn->pn_cookie.isFree()) {
        atomIndex = pn->pn_cookie.slot();
    } else {
        if (!cg->makeAtomIndex(pn->pn_atom, &atomIndex))
            return false;
    }

    if (JOF_OPTYPE(pn->getOp()) == JOF_ATOM &&
        (!cg->inFunction() || (cg->flags & TCF_FUN_HEAVYWEIGHT)) &&
        !(pn->pn_dflags & PND_GVAR))
    {
        CG_SWITCH_TO_PROLOG(cg);
        if (!UpdateLineNumberNotes(cx, cg, pn->pn_pos.begin.lineno))
            return false;
        EMIT_INDEX_OP(prologOp, atomIndex);
        CG_SWITCH_TO_MAIN(cg);
    }

    if (cg->inFunction() &&
        JOF_OPTYPE(pn->getOp()) == JOF_LOCAL &&
        pn->pn_cookie.slot() < cg->bindings.countVars() &&
        cg->shouldNoteClosedName(pn))
    {
        if (!cg->closedVars.append(pn->pn_cookie.slot()))
            return false;
    }

    if (result)
        *result = atomIndex;
    return true;
}

#if JS_HAS_DESTRUCTURING

typedef JSBool
(*DestructuringDeclEmitter)(JSContext *cx, CodeGenerator *cg, JSOp prologOp, ParseNode *pn);

static JSBool
EmitDestructuringDecl(JSContext *cx, CodeGenerator *cg, JSOp prologOp, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(TOK_NAME));
    if (!BindNameToSlot(cx, cg, pn))
        return JS_FALSE;

    JS_ASSERT(!pn->isOp(JSOP_ARGUMENTS) && !pn->isOp(JSOP_CALLEE));
    return MaybeEmitVarDecl(cx, cg, prologOp, pn, NULL);
}

static JSBool
EmitDestructuringDecls(JSContext *cx, CodeGenerator *cg, JSOp prologOp, ParseNode *pn)
{
    ParseNode *pn2, *pn3;
    DestructuringDeclEmitter emitter;

    if (pn->isKind(TOK_RB)) {
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(TOK_COMMA))
                continue;
            emitter = (pn2->isKind(TOK_NAME))
                      ? EmitDestructuringDecl
                      : EmitDestructuringDecls;
            if (!emitter(cx, cg, prologOp, pn2))
                return JS_FALSE;
        }
    } else {
        JS_ASSERT(pn->isKind(TOK_RC));
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            pn3 = pn2->pn_right;
            emitter = pn3->isKind(TOK_NAME) ? EmitDestructuringDecl : EmitDestructuringDecls;
            if (!emitter(cx, cg, prologOp, pn3))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
EmitDestructuringOpsHelper(JSContext *cx, CodeGenerator *cg, ParseNode *pn);

static JSBool
EmitDestructuringLHS(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    





    if (pn->isKind(TOK_RB) || pn->isKind(TOK_RC)) {
        if (!EmitDestructuringOpsHelper(cx, cg, pn))
            return JS_FALSE;
        if (Emit1(cx, cg, JSOP_POP) < 0)
            return JS_FALSE;
    } else {
        if (pn->isKind(TOK_NAME)) {
            if (!BindNameToSlot(cx, cg, pn))
                return JS_FALSE;
            if (pn->isConst() && !pn->isInitialized())
                return Emit1(cx, cg, JSOP_POP) >= 0;
        }

        switch (pn->getOp()) {
          case JSOP_SETNAME:
          case JSOP_SETGNAME:
            




            if (!EmitElemOp(cx, pn, JSOP_ENUMELEM, cg))
                return JS_FALSE;
            break;

          case JSOP_SETCONST:
            if (!EmitElemOp(cx, pn, JSOP_ENUMCONSTELEM, cg))
                return JS_FALSE;
            break;

          case JSOP_SETLOCAL:
          {
            jsuint slot = pn->pn_cookie.asInteger();
            EMIT_UINT16_IMM_OP(JSOP_SETLOCALPOP, slot);
            break;
          }

          case JSOP_SETARG:
          {
            jsuint slot = pn->pn_cookie.asInteger();
            EMIT_UINT16_IMM_OP(pn->getOp(), slot);
            if (Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
            break;
          }

          default:
          {
            ptrdiff_t top;

            top = CG_OFFSET(cg);
            if (!EmitTree(cx, cg, pn))
                return JS_FALSE;
            if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
                return JS_FALSE;
            if (!EmitElemOpBase(cx, cg, JSOP_ENUMELEM))
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
EmitDestructuringOpsHelper(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    jsuint index;
    ParseNode *pn2, *pn3;
    JSBool doElemOp;

#ifdef DEBUG
    intN stackDepth = cg->stackDepth;
    JS_ASSERT(stackDepth != 0);
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isKind(TOK_RB) || pn->isKind(TOK_RC));
#endif

    if (pn->pn_count == 0) {
        
        return Emit1(cx, cg, JSOP_DUP) >= 0 &&
               Emit1(cx, cg, JSOP_POP) >= 0;
    }

    index = 0;
    for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
        



        if (pn2 != pn->pn_head && NewSrcNote(cx, cg, SRC_CONTINUE) < 0)
            return JS_FALSE;
        if (Emit1(cx, cg, JSOP_DUP) < 0)
            return JS_FALSE;

        





        doElemOp = JS_TRUE;
        if (pn->isKind(TOK_RB)) {
            if (!EmitNumberOp(cx, index, cg))
                return JS_FALSE;
            pn3 = pn2;
        } else {
            JS_ASSERT(pn->isKind(TOK_RC));
            JS_ASSERT(pn2->isKind(TOK_COLON));
            pn3 = pn2->pn_left;
            if (pn3->isKind(TOK_NUMBER)) {
                




                if (NewSrcNote(cx, cg, SRC_INITPROP) < 0)
                    return JS_FALSE;
                if (!EmitNumberOp(cx, pn3->pn_dval, cg))
                    return JS_FALSE;
            } else {
                JS_ASSERT(pn3->isKind(TOK_STRING) || pn3->isKind(TOK_NAME));
                if (!EmitAtomOp(cx, pn3, JSOP_GETPROP, cg))
                    return JS_FALSE;
                doElemOp = JS_FALSE;
            }
            pn3 = pn2->pn_right;
        }

        if (doElemOp) {
            




            if (!EmitElemOpBase(cx, cg, JSOP_GETELEM))
                return JS_FALSE;
            JS_ASSERT(cg->stackDepth == stackDepth + 1);
        }

        
        if (pn3->isKind(TOK_COMMA) && pn3->isArity(PN_NULLARY)) {
            JS_ASSERT(pn->isKind(TOK_RB));
            JS_ASSERT(pn2 == pn3);
            if (Emit1(cx, cg, JSOP_POP) < 0)
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
EmitDestructuringOps(JSContext *cx, CodeGenerator *cg, JSOp prologOp, ParseNode *pn)
{
    





    if (NewSrcNote2(cx, cg, SRC_DESTRUCT, OpToDeclType(prologOp)) < 0)
        return JS_FALSE;

    



    return EmitDestructuringOpsHelper(cx, cg, pn);
}

static JSBool
EmitGroupAssignment(JSContext *cx, CodeGenerator *cg, JSOp prologOp,
                    ParseNode *lhs, ParseNode *rhs)
{
    jsuint depth, limit, i, nslots;
    ParseNode *pn;

    depth = limit = (uintN) cg->stackDepth;
    for (pn = rhs->pn_head; pn; pn = pn->pn_next) {
        if (limit == JS_BIT(16)) {
            ReportCompileErrorNumber(cx, CG_TS(cg), rhs, JSREPORT_ERROR, JSMSG_ARRAY_INIT_TOO_BIG);
            return JS_FALSE;
        }

        
        JS_ASSERT(!(pn->isKind(TOK_COMMA) && pn->isArity(PN_NULLARY)));
        if (!EmitTree(cx, cg, pn))
            return JS_FALSE;
        ++limit;
    }

    if (NewSrcNote2(cx, cg, SRC_GROUPASSIGN, OpToDeclType(prologOp)) < 0)
        return JS_FALSE;

    i = depth;
    for (pn = lhs->pn_head; pn; pn = pn->pn_next, ++i) {
        
        JS_ASSERT(i < limit);
        jsint slot = AdjustBlockSlot(cx, cg, i);
        if (slot < 0)
            return JS_FALSE;
        EMIT_UINT16_IMM_OP(JSOP_GETLOCAL, slot);

        if (pn->isKind(TOK_COMMA) && pn->isArity(PN_NULLARY)) {
            if (Emit1(cx, cg, JSOP_POP) < 0)
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
MaybeEmitGroupAssignment(JSContext *cx, CodeGenerator *cg, JSOp prologOp, ParseNode *pn, JSOp *pop)
{
    ParseNode *lhs, *rhs;

    JS_ASSERT(pn->isKind(TOK_ASSIGN));
    JS_ASSERT(*pop == JSOP_POP || *pop == JSOP_POPV);
    lhs = pn->pn_left;
    rhs = pn->pn_right;
    if (lhs->isKind(TOK_RB) && rhs->isKind(TOK_RB) &&
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
EmitVariables(JSContext *cx, CodeGenerator *cg, ParseNode *pn, JSBool inLetHead,
              ptrdiff_t *headNoteIndex)
{
    bool forInVar, first;
    ptrdiff_t off, noteIndex, tmp;
    ParseNode *pn2, *pn3, *next;
    JSOp op;
    jsatomid atomIndex;
    uintN oldflags;

    
    *headNoteIndex = -1;

    











    DebugOnly<bool> let = (pn->isOp(JSOP_NOP));
    forInVar = (pn->pn_xflags & PNX_FORINVAR) != 0;

    off = noteIndex = -1;
    for (pn2 = pn->pn_head; ; pn2 = next) {
        first = pn2 == pn->pn_head;
        next = pn2->pn_next;

        if (!pn2->isKind(TOK_NAME)) {
#if JS_HAS_DESTRUCTURING
            if (pn2->isKind(TOK_RB) || pn2->isKind(TOK_RC)) {
                







                JS_ASSERT(forInVar);
                JS_ASSERT(pn->pn_count == 1);
                if (!EmitDestructuringDecls(cx, cg, pn->getOp(), pn2))
                    return JS_FALSE;
                break;
            }
#endif

            





            JS_ASSERT(pn2->isKind(TOK_ASSIGN));
            JS_ASSERT(!forInVar);

            




#if !JS_HAS_DESTRUCTURING
            JS_ASSERT(pn2->pn_left->isKind(TOK_NAME));
#else
            if (pn2->pn_left->isKind(TOK_NAME))
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
                                              inLetHead ? JSOP_POP : pn->getOp(),
                                              pn2, &op)) {
                    return JS_FALSE;
                }
                if (op == JSOP_NOP) {
                    pn->pn_xflags = (pn->pn_xflags & ~PNX_POPVAR) | PNX_GROUPINIT;
                    break;
                }
            }

            pn3 = pn2->pn_left;
            if (!EmitDestructuringDecls(cx, cg, pn->getOp(), pn3))
                return JS_FALSE;

            if (!EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;

            




            if (!EmitDestructuringOps(cx, cg,
                                      inLetHead ? JSOP_POP : pn->getOp(),
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

        op = pn2->getOp();
        if (op == JSOP_ARGUMENTS) {
            
            JS_ASSERT(!pn3 && !let);
            pn3 = NULL;
#ifdef __GNUC__
            atomIndex = 0;            
#endif
        } else {
            JS_ASSERT(op != JSOP_CALLEE);
            JS_ASSERT(!pn2->pn_cookie.isFree() || !let);
            if (!MaybeEmitVarDecl(cx, cg, pn->getOp(), pn2, &atomIndex))
                return JS_FALSE;

            if (pn3) {
                JS_ASSERT(!forInVar);
                if (op == JSOP_SETNAME) {
                    JS_ASSERT(!let);
                    EMIT_INDEX_OP(JSOP_BINDNAME, atomIndex);
                } else if (op == JSOP_SETGNAME) {
                    JS_ASSERT(!let);
                    EMIT_INDEX_OP(JSOP_BINDGNAME, atomIndex);
                }
                if (pn->isOp(JSOP_DEFCONST) &&
                    !DefineCompileTimeConstant(cx, cg, pn2->pn_atom, pn3))
                {
                    return JS_FALSE;
                }

                oldflags = cg->flags;
                cg->flags &= ~TCF_IN_FOR_INIT;
                if (!EmitTree(cx, cg, pn3))
                    return JS_FALSE;
                cg->flags |= oldflags & TCF_IN_FOR_INIT;
            }
        }

        







        JS_ASSERT_IF(pn2->isDefn(), pn3 == pn2->pn_expr);
        if (forInVar) {
            JS_ASSERT(pn->pn_count == 1);
            JS_ASSERT(!pn3);
            break;
        }

        if (first &&
            !inLetHead &&
            NewSrcNote2(cx, cg, SRC_DECL,
                        (pn->isOp(JSOP_DEFCONST))
                        ? SRC_DECL_CONST
                        : (pn->isOp(JSOP_DEFVAR))
                        ? SRC_DECL_VAR
                        : SRC_DECL_LET) < 0)
        {
            return JS_FALSE;
        }
        if (op == JSOP_ARGUMENTS) {
            if (Emit1(cx, cg, op) < 0)
                return JS_FALSE;
        } else if (!pn2->pn_cookie.isFree()) {
            EMIT_UINT16_IMM_OP(op, atomIndex);
        } else {
            EMIT_INDEX_OP(op, atomIndex);
        }

#if JS_HAS_DESTRUCTURING
    emit_note_pop:
#endif
        tmp = CG_OFFSET(cg);
        if (noteIndex >= 0) {
            if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp-off))
                return JS_FALSE;
        }
        if (!next)
            break;
        off = tmp;
        noteIndex = NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
        if (noteIndex < 0 || Emit1(cx, cg, JSOP_POP) < 0)
            return JS_FALSE;
    }

    
    if (inLetHead) {
        *headNoteIndex = NewSrcNote(cx, cg, SRC_DECL);
        if (*headNoteIndex < 0)
            return JS_FALSE;
        if (!(pn->pn_xflags & PNX_POPVAR))
            return Emit1(cx, cg, JSOP_NOP) >= 0;
    }

    return !(pn->pn_xflags & PNX_POPVAR) || Emit1(cx, cg, JSOP_POP) >= 0;
}

static bool
EmitAssignment(JSContext *cx, CodeGenerator *cg, ParseNode *lhs, JSOp op, ParseNode *rhs)
{
    ptrdiff_t top = CG_OFFSET(cg);

    




    jsatomid atomIndex = (jsatomid) -1;              
    jsbytecode offset = 1;

    switch (lhs->getKind()) {
      case TOK_NAME:
        if (!BindNameToSlot(cx, cg, lhs))
            return false;
        if (!lhs->pn_cookie.isFree()) {
            atomIndex = lhs->pn_cookie.asInteger();
        } else {
            if (!cg->makeAtomIndex(lhs->pn_atom, &atomIndex))
                return false;
            if (!lhs->isConst()) {
                JSOp op = lhs->isOp(JSOP_SETGNAME) ? JSOP_BINDGNAME : JSOP_BINDNAME;
                EMIT_INDEX_OP(op, atomIndex);
                offset++;
            }
        }
        break;
      case TOK_DOT:
        if (!EmitTree(cx, cg, lhs->expr()))
            return false;
        offset++;
        if (!cg->makeAtomIndex(lhs->pn_atom, &atomIndex))
            return false;
        break;
      case TOK_LB:
        JS_ASSERT(lhs->isArity(PN_BINARY));
        if (!EmitTree(cx, cg, lhs->pn_left))
            return false;
        if (!EmitTree(cx, cg, lhs->pn_right))
            return false;
        offset += 2;
        break;
#if JS_HAS_DESTRUCTURING
      case TOK_RB:
      case TOK_RC:
        break;
#endif
      case TOK_LP:
        if (!EmitTree(cx, cg, lhs))
            return false;
        offset++;
        break;
#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        JS_ASSERT(!cg->inStrictMode());
        JS_ASSERT(lhs->isOp(JSOP_SETXMLNAME));

        if (!EmitTree(cx, cg, lhs->pn_kid))
            return false;
        if (Emit1(cx, cg, JSOP_BINDXMLNAME) < 0)
            return false;
        offset++;
        break;
#endif
      default:
        JS_ASSERT(0);
    }

    if (op != JSOP_NOP) {
        JS_ASSERT(rhs);
        switch (lhs->getKind()) {
          case TOK_NAME:
            if (lhs->isConst()) {
                if (lhs->isOp(JSOP_CALLEE)) {
                    if (Emit1(cx, cg, JSOP_CALLEE) < 0)
                        return false;
                } else {
                    EMIT_INDEX_OP(lhs->getOp(), atomIndex);
                }
            } else if (lhs->isOp(JSOP_SETNAME)) {
                if (Emit1(cx, cg, JSOP_DUP) < 0)
                    return false;
                EMIT_INDEX_OP(JSOP_GETXPROP, atomIndex);
            } else if (lhs->isOp(JSOP_SETGNAME)) {
                if (!BindGlobal(cx, cg, lhs, lhs->pn_atom))
                    return false;
                EmitAtomOp(cx, lhs, JSOP_GETGNAME, cg);
            } else {
                EMIT_UINT16_IMM_OP(lhs->isOp(JSOP_SETARG) ? JSOP_GETARG : JSOP_GETLOCAL, atomIndex);
            }
            break;
          case TOK_DOT:
            if (Emit1(cx, cg, JSOP_DUP) < 0)
                return false;
            if (lhs->pn_atom == cx->runtime->atomState.protoAtom) {
                if (!EmitIndexOp(cx, JSOP_QNAMEPART, atomIndex, cg))
                    return false;
                if (!EmitElemOpBase(cx, cg, JSOP_GETELEM))
                    return false;
            } else {
                bool isLength = (lhs->pn_atom == cx->runtime->atomState.lengthAtom);
                EMIT_INDEX_OP(isLength ? JSOP_LENGTH : JSOP_GETPROP, atomIndex);
            }
            break;
          case TOK_LB:
          case TOK_LP:
#if JS_HAS_XML_SUPPORT
          case TOK_UNARYOP:
#endif
            if (Emit1(cx, cg, JSOP_DUP2) < 0)
                return false;
            if (!EmitElemOpBase(cx, cg, JSOP_GETELEM))
                return false;
            break;
          default:;
        }
    }

    
    if (rhs) {
        if (!EmitTree(cx, cg, rhs))
            return false;
    } else {
        
        if (Emit2(cx, cg, JSOP_ITERNEXT, offset) < 0)
            return false;
    }

    
    if (op != JSOP_NOP) {
        




        if (!lhs->isKind(TOK_NAME) || !lhs->isConst()) {
            if (NewSrcNote(cx, cg, SRC_ASSIGNOP) < 0)
                return false;
        }
        if (Emit1(cx, cg, op) < 0)
            return false;
    }

    
    if (!lhs->isKind(TOK_NAME) &&
#if JS_HAS_DESTRUCTURING
        !lhs->isKind(TOK_RB) &&
        !lhs->isKind(TOK_RC) &&
#endif
        NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
    {
        return false;
    }

    
    switch (lhs->getKind()) {
      case TOK_NAME:
        if (lhs->isConst()) {
            if (!rhs) {
                ReportCompileErrorNumber(cx, CG_TS(cg), lhs, JSREPORT_ERROR,
                                         JSMSG_BAD_FOR_LEFTSIDE);
                return false;
            }
            break;
        }
        
      case TOK_DOT:
        EMIT_INDEX_OP(lhs->getOp(), atomIndex);
        break;
      case TOK_LB:
      case TOK_LP:
        if (Emit1(cx, cg, JSOP_SETELEM) < 0)
            return false;
        break;
#if JS_HAS_DESTRUCTURING
      case TOK_RB:
      case TOK_RC:
        if (!EmitDestructuringOps(cx, cg, JSOP_SETNAME, lhs))
            return false;
        break;
#endif
#if JS_HAS_XML_SUPPORT
      case TOK_UNARYOP:
        JS_ASSERT(!cg->inStrictMode());
        if (Emit1(cx, cg, JSOP_SETXMLNAME) < 0)
            return false;
        break;
#endif
      default:
        JS_ASSERT(0);
    }
    return true;
}

#if defined DEBUG_brendan || defined DEBUG_mrbkap
static JSBool
GettableNoteForNextOp(CodeGenerator *cg)
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
EmitFunctionDefNop(JSContext *cx, CodeGenerator *cg, uintN index)
{
    return NewSrcNote2(cx, cg, SRC_FUNCDEF, (ptrdiff_t)index) >= 0 &&
           Emit1(cx, cg, JSOP_NOP) >= 0;
}

static bool
EmitNewInit(JSContext *cx, CodeGenerator *cg, JSProtoKey key, ParseNode *pn, int sharpnum)
{
    if (Emit3(cx, cg, JSOP_NEWINIT, (jsbytecode) key, 0) < 0)
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
EmitEndInit(JSContext *cx, CodeGenerator *cg, uint32 count)
{
#if JS_HAS_SHARP_VARS
    
    if (cg->hasSharps() && count != 0)
        EMIT_UINT16_IMM_OP(JSOP_SHARPINIT, cg->sharpSlotBase);
#endif
    return Emit1(cx, cg, JSOP_ENDINIT) >= 0;
}

bool
ParseNode::getConstantValue(JSContext *cx, bool strictChecks, Value *vp)
{
    switch (getKind()) {
      case TOK_NUMBER:
        vp->setNumber(pn_dval);
        return true;
      case TOK_STRING:
        vp->setString(pn_atom);
        return true;
      case TOK_PRIMARY:
        switch (getOp()) {
          case JSOP_NULL:
            vp->setNull();
            return true;
          case JSOP_FALSE:
            vp->setBoolean(false);
            return true;
          case JSOP_TRUE:
            vp->setBoolean(true);
            return true;
          default:
            JS_NOT_REACHED("Unexpected node");
            return false;
        }
      case TOK_RB: {
        JS_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));

        JSObject *obj = NewDenseAllocatedArray(cx, pn_count);
        if (!obj)
            return false;

        unsigned idx = 0;
        for (ParseNode *pn = pn_head; pn; idx++, pn = pn->pn_next) {
            Value value;
            if (!pn->getConstantValue(cx, strictChecks, &value))
                return false;
            if (!obj->defineGeneric(cx, INT_TO_JSID(idx), value, NULL, NULL, JSPROP_ENUMERATE))
                return false;
        }
        JS_ASSERT(idx == pn_count);

        types::FixArrayType(cx, obj);
        vp->setObject(*obj);
        return true;
      }
      case TOK_RC: {
        JS_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));

        gc::AllocKind kind = GuessObjectGCKind(pn_count);
        JSObject *obj = NewBuiltinClassInstance(cx, &ObjectClass, kind);
        if (!obj)
            return false;

        for (ParseNode *pn = pn_head; pn; pn = pn->pn_next) {
            Value value;
            if (!pn->pn_right->getConstantValue(cx, strictChecks, &value))
                return false;

            ParseNode *pnid = pn->pn_left;
            if (pnid->isKind(TOK_NUMBER)) {
                Value idvalue = NumberValue(pnid->pn_dval);
                jsid id;
                if (idvalue.isInt32() && INT_FITS_IN_JSID(idvalue.toInt32()))
                    id = INT_TO_JSID(idvalue.toInt32());
                else if (!js_InternNonIntElementId(cx, obj, idvalue, &id))
                    return false;
                if (!obj->defineGeneric(cx, id, value, NULL, NULL, JSPROP_ENUMERATE))
                    return false;
            } else {
                JS_ASSERT(pnid->isKind(TOK_NAME) ||
                          pnid->isKind(TOK_STRING));
                JS_ASSERT(pnid->pn_atom != cx->runtime->atomState.protoAtom);
                jsid id = ATOM_TO_JSID(pnid->pn_atom);
                if (!DefineNativeProperty(cx, obj, id, value, NULL, NULL,
                                          JSPROP_ENUMERATE, 0, 0)) {
                    return false;
                }
            }
        }

        types::FixObjectType(cx, obj);
        vp->setObject(*obj);
        return true;
      }
      default:
        JS_NOT_REACHED("Unexpected node");
    }
    return false;
}

static bool
EmitSingletonInitialiser(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    Value value;
    if (!pn->getConstantValue(cx, cg->needStrictChecks(), &value))
        return false;

    JS_ASSERT(value.isObject());
    ObjectBox *objbox = cg->parser->newObjectBox(&value.toObject());
    if (!objbox)
        return false;

    return EmitObjectOp(cx, objbox, JSOP_OBJECT, cg);
}


JS_STATIC_ASSERT(JSOP_NOP_LENGTH == 1);
JS_STATIC_ASSERT(JSOP_POP_LENGTH == 1);

class EmitLevelManager
{
    CodeGenerator *cg;
  public:
    EmitLevelManager(CodeGenerator *cg) : cg(cg) { cg->emitLevel++; }
    ~EmitLevelManager() { cg->emitLevel--; }
};

static bool
EmitCatch(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    ptrdiff_t catchStart, guardJump;

    



    StmtInfo *stmt = cg->topStmt;
    JS_ASSERT(stmt->type == STMT_BLOCK && (stmt->flags & SIF_SCOPE));
    stmt->type = STMT_CATCH;
    catchStart = stmt->update;

    
    stmt = stmt->down;
    JS_ASSERT(stmt->type == STMT_TRY || stmt->type == STMT_FINALLY);

    
    if (Emit1(cx, cg, JSOP_EXCEPTION) < 0)
        return false;

    



    if (pn->pn_kid2 && Emit1(cx, cg, JSOP_DUP) < 0)
        return false;

    ParseNode *pn2 = pn->pn_kid1;
    switch (pn2->getKind()) {
#if JS_HAS_DESTRUCTURING
      case TOK_RB:
      case TOK_RC:
        if (!EmitDestructuringOps(cx, cg, JSOP_NOP, pn2))
            return false;
        if (Emit1(cx, cg, JSOP_POP) < 0)
            return false;
        break;
#endif

      case TOK_NAME:
        
        JS_ASSERT(!pn2->pn_cookie.isFree());
        EMIT_UINT16_IMM_OP(JSOP_SETLOCALPOP, pn2->pn_cookie.asInteger());
        break;

      default:
        JS_ASSERT(0);
    }

    
    if (pn->pn_kid2) {
        if (!EmitTree(cx, cg, pn->pn_kid2))
            return false;
        if (!SetSrcNoteOffset(cx, cg, CATCHNOTE(*stmt), 0, CG_OFFSET(cg) - catchStart))
            return false;
        
        guardJump = EmitJump(cx, cg, JSOP_IFEQ, 0);
        if (guardJump < 0)
            return false;
        GUARDJUMP(*stmt) = guardJump;

        
        if (Emit1(cx, cg, JSOP_POP) < 0)
            return false;
    }

    
    if (!EmitTree(cx, cg, pn->pn_kid3))
        return false;

    



    ptrdiff_t off = cg->stackDepth;
    if (NewSrcNote2(cx, cg, SRC_CATCH, off) < 0)
        return false;
    return true;
}

static bool
EmitTry(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    StmtInfo stmtInfo;
    ptrdiff_t catchJump = -1;

    








    PushStatement(cg, &stmtInfo, pn->pn_kid3 ? STMT_FINALLY : STMT_TRY, CG_OFFSET(cg));

    








    intN depth = cg->stackDepth;

    
    if (Emit1(cx, cg, JSOP_TRY) < 0)
        return false;
    ptrdiff_t tryStart = CG_OFFSET(cg);
    if (!EmitTree(cx, cg, pn->pn_kid1))
        return false;
    JS_ASSERT(depth == cg->stackDepth);

    
    if (pn->pn_kid3) {
        if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
            return false;
        if (EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &GOSUBS(stmtInfo)) < 0)
            return false;
    }

    
    if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
        return false;
    if (EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &catchJump) < 0)
        return false;

    ptrdiff_t tryEnd = CG_OFFSET(cg);

    ObjectBox *prevBox = NULL;
    
    ParseNode *lastCatch = NULL;
    if (ParseNode *pn2 = pn->pn_kid2) {
        uintN count = 0;    

        






















        for (ParseNode *pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            ptrdiff_t guardJump, catchNote;

            JS_ASSERT(cg->stackDepth == depth);
            guardJump = GUARDJUMP(stmtInfo);
            if (guardJump != -1) {
                if (EmitKnownBlockChain(cx, cg, prevBox) < 0)
                    return false;

                
                CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, guardJump);

                




                cg->stackDepth = depth + count + 1;

                





                if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0 ||
                    Emit1(cx, cg, JSOP_THROWING) < 0) {
                    return false;
                }
                if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                    return false;
                if (!EmitLeaveBlock(cx, cg, JSOP_LEAVEBLOCK, prevBox))
                    return false;
                JS_ASSERT(cg->stackDepth == depth);
            }

            






            catchNote = NewSrcNote2(cx, cg, SRC_CATCH, 0);
            if (catchNote < 0)
                return false;
            CATCHNOTE(stmtInfo) = catchNote;

            




            JS_ASSERT(pn3->isKind(TOK_LEXICALSCOPE));
            count = OBJ_BLOCK_COUNT(cx, pn3->pn_objbox->object);
            prevBox = pn3->pn_objbox;
            if (!EmitTree(cx, cg, pn3))
                return false;

            
            if (pn->pn_kid3) {
                if (EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &GOSUBS(stmtInfo)) < 0)
                    return false;
                JS_ASSERT(cg->stackDepth == depth);
            }

            



            if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
                return false;
            if (EmitBackPatchOp(cx, cg, JSOP_BACKPATCH, &catchJump) < 0)
                return false;

            



            lastCatch = pn3->expr();
        }
    }

    





    if (lastCatch && lastCatch->pn_kid2) {
        if (EmitKnownBlockChain(cx, cg, prevBox) < 0)
            return false;

        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, GUARDJUMP(stmtInfo));

        
        JS_ASSERT(cg->stackDepth == depth);
        cg->stackDepth = depth + 1;

        



        if (NewSrcNote(cx, cg, SRC_HIDDEN) < 0 || Emit1(cx, cg, JSOP_THROW) < 0)
            return false;

        if (EmitBlockChain(cx, cg) < 0)
            return false;
    }

    JS_ASSERT(cg->stackDepth == depth);

    
    ptrdiff_t finallyStart = 0;   
    if (pn->pn_kid3) {
        



        if (!BackPatch(cx, cg, GOSUBS(stmtInfo), CG_NEXT(cg), JSOP_GOSUB))
            return false;

        finallyStart = CG_OFFSET(cg);

        
        stmtInfo.type = STMT_SUBROUTINE;
        if (!UpdateLineNumberNotes(cx, cg, pn->pn_kid3->pn_pos.begin.lineno))
            return false;
        if (Emit1(cx, cg, JSOP_FINALLY) < 0 ||
            !EmitTree(cx, cg, pn->pn_kid3) ||
            Emit1(cx, cg, JSOP_RETSUB) < 0)
        {
            return false;
        }
        JS_ASSERT(cg->stackDepth == depth);
    }
    if (!PopStatementCG(cx, cg))
        return false;

    if (NewSrcNote(cx, cg, SRC_ENDBRACE) < 0 || Emit1(cx, cg, JSOP_NOP) < 0)
        return false;

    
    if (!BackPatch(cx, cg, catchJump, CG_NEXT(cg), JSOP_GOTO))
        return false;

    



    if (pn->pn_kid2 && !NewTryNote(cx, cg, JSTRY_CATCH, depth, tryStart, tryEnd))
        return false;

    




    if (pn->pn_kid3 && !NewTryNote(cx, cg, JSTRY_FINALLY, depth, tryStart, finallyStart))
        return false;

    return true;
}

static bool
EmitIf(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    StmtInfo stmtInfo;

    
    stmtInfo.type = STMT_IF;
    ptrdiff_t beq = -1;
    ptrdiff_t jmp = -1;
    ptrdiff_t noteIndex = -1;

  if_again:
    
    if (!EmitTree(cx, cg, pn->pn_kid1))
        return JS_FALSE;
    ptrdiff_t top = CG_OFFSET(cg);
    if (stmtInfo.type == STMT_IF) {
        PushStatement(cg, &stmtInfo, STMT_IF, top);
    } else {
        








        JS_ASSERT(stmtInfo.type == STMT_ELSE);
        stmtInfo.type = STMT_IF;
        stmtInfo.update = top;
        if (!SetSrcNoteOffset(cx, cg, noteIndex, 0, jmp - beq))
            return JS_FALSE;
        if (!SetSrcNoteOffset(cx, cg, noteIndex, 1, top - beq))
            return JS_FALSE;
    }

    
    ParseNode *pn3 = pn->pn_kid3;
    noteIndex = NewSrcNote(cx, cg, pn3 ? SRC_IF_ELSE : SRC_IF);
    if (noteIndex < 0)
        return JS_FALSE;
    beq = EmitJump(cx, cg, JSOP_IFEQ, 0);
    if (beq < 0)
        return JS_FALSE;

    
    if (!EmitTree(cx, cg, pn->pn_kid2))
        return JS_FALSE;
    if (pn3) {
        
        stmtInfo.type = STMT_ELSE;

        





        jmp = EmitGoto(cx, cg, &stmtInfo, &stmtInfo.breaks);
        if (jmp < 0)
            return JS_FALSE;

        
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq);
        if (pn3->isKind(TOK_IF)) {
            pn = pn3;
            goto if_again;
        }

        if (!EmitTree(cx, cg, pn3))
            return JS_FALSE;

        






        if (!SetSrcNoteOffset(cx, cg, noteIndex, 0, jmp - beq))
            return JS_FALSE;
    } else {
        
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq);
    }
    return PopStatementCG(cx, cg);
}

#if JS_HAS_BLOCK_SCOPE
static bool
EmitLet(JSContext *cx, CodeGenerator *cg, ParseNode *&pn)
{
    










    ParseNode *pn2;
    if (pn->isArity(PN_BINARY)) {
        pn2 = pn->pn_right;
        pn = pn->pn_left;
    } else {
        pn2 = NULL;
    }

    








    JS_ASSERT(pn->isArity(PN_LIST));
    TempPopScope tps;
    bool popScope = pn2 || (cg->flags & TCF_IN_FOR_INIT);
    if (popScope && !tps.popBlock(cx, cg))
        return false;
    ptrdiff_t noteIndex;
    if (!EmitVariables(cx, cg, pn, pn2 != NULL, &noteIndex))
        return false;
    ptrdiff_t tmp = CG_OFFSET(cg);
    if (popScope && !tps.repushBlock(cx, cg))
        return false;

    
    if (pn2 && !EmitTree(cx, cg, pn2))
        return false;

    if (noteIndex >= 0 && !SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, CG_OFFSET(cg) - tmp))
        return false;

    return true;
}
#endif

#if JS_HAS_XML_SUPPORT
static bool
EmitXMLTag(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    JS_ASSERT(!cg->inStrictMode());

    if (Emit1(cx, cg, JSOP_STARTXML) < 0)
        return false;

    {
        jsatomid index;
        JSAtom *tagAtom = (pn->isKind(TOK_XMLETAGO))
                          ? cx->runtime->atomState.etagoAtom
                          : cx->runtime->atomState.stagoAtom;
        if (!cg->makeAtomIndex(tagAtom, &index))
            return false;
        EMIT_INDEX_OP(JSOP_STRING, index);
    }

    JS_ASSERT(pn->pn_count != 0);
    ParseNode *pn2 = pn->pn_head;
    if (pn2->isKind(TOK_LC) && Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0)
        return false;
    if (!EmitTree(cx, cg, pn2))
        return false;
    if (Emit1(cx, cg, JSOP_ADD) < 0)
        return false;

    uint32 i;
    for (pn2 = pn2->pn_next, i = 0; pn2; pn2 = pn2->pn_next, i++) {
        if (pn2->isKind(TOK_LC) && Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0)
            return false;
        if (!EmitTree(cx, cg, pn2))
            return false;
        if ((i & 1) && pn2->isKind(TOK_LC)) {
            if (Emit1(cx, cg, JSOP_TOATTRVAL) < 0)
                return false;
        }
        if (Emit1(cx, cg, (i & 1) ? JSOP_ADDATTRVAL : JSOP_ADDATTRNAME) < 0)
            return false;
    }

    {
        jsatomid index;
        JSAtom *tmp = (pn->isKind(TOK_XMLPTAGC)) ? cx->runtime->atomState.ptagcAtom
                                                 : cx->runtime->atomState.tagcAtom;
        if (!cg->makeAtomIndex(tmp, &index))
            return false;
        EMIT_INDEX_OP(JSOP_STRING, index);
    }
    if (Emit1(cx, cg, JSOP_ADD) < 0)
        return false;

    if ((pn->pn_xflags & PNX_XMLROOT) && Emit1(cx, cg, pn->getOp()) < 0)
        return false;

    return true;
}

static bool
EmitXMLProcessingInstruction(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    JS_ASSERT(!cg->inStrictMode());

    jsatomid index;
    if (!cg->makeAtomIndex(pn->pn_pidata, &index))
        return false;
    if (!EmitIndexOp(cx, JSOP_QNAMEPART, index, cg))
        return false;
    if (!EmitAtomOp(cx, pn, JSOP_XMLPI, cg))
        return false;
    return true;
}
#endif

static bool
EmitLexicalScope(JSContext *cx, CodeGenerator *cg, ParseNode *pn, JSBool &ok)
{
    StmtInfo stmtInfo;
    StmtInfo *stmt;
    ObjectBox *objbox = pn->pn_objbox;
    PushBlockScope(cg, &stmtInfo, objbox, CG_OFFSET(cg));

    







    ptrdiff_t noteIndex = -1;
    TokenKind type = pn->expr()->getKind();
    if (type != TOK_CATCH && type != TOK_LET && type != TOK_FOR &&
        (!(stmt = stmtInfo.down)
         ? !cg->inFunction()
         : stmt->type == STMT_BLOCK)) {
#if defined DEBUG_brendan || defined DEBUG_mrbkap
        
        JS_ASSERT(CG_NOTE_COUNT(cg) == 0 ||
                  CG_LAST_NOTE_OFFSET(cg) != CG_OFFSET(cg) ||
                  !GettableNoteForNextOp(cg));
#endif
        noteIndex = NewSrcNote2(cx, cg, SRC_BRACE, 0);
        if (noteIndex < 0)
            return false;
    }

    ptrdiff_t top = CG_OFFSET(cg);
    if (!EmitEnterBlock(cx, pn, cg))
        return false;

    if (!EmitTree(cx, cg, pn->pn_expr))
        return false;

    JSOp op = pn->getOp();
    if (op == JSOP_LEAVEBLOCKEXPR) {
        if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - top) < 0)
            return false;
    } else {
        if (noteIndex >= 0 && !SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, CG_OFFSET(cg) - top))
            return false;
    }

    
    if (!EmitLeaveBlock(cx, cg, op, objbox))
        return false;

    ok = PopStatementCG(cx, cg);
    return true;
}

static bool
EmitWith(JSContext *cx, CodeGenerator *cg, ParseNode *pn, JSBool &ok)
{
    StmtInfo stmtInfo;
    if (!EmitTree(cx, cg, pn->pn_left))
        return false;
    PushStatement(cg, &stmtInfo, STMT_WITH, CG_OFFSET(cg));
    if (Emit1(cx, cg, JSOP_ENTERWITH) < 0)
        return false;

    
    if (EmitBlockChain(cx, cg) < 0)
        return false;
    if (!EmitTree(cx, cg, pn->pn_right))
        return false;
    if (Emit1(cx, cg, JSOP_LEAVEWITH) < 0)
        return false;
    ok = PopStatementCG(cx, cg);
    return true;
}

static bool
SetMethodFunction(JSContext *cx, FunctionBox *funbox, JSAtom *atom)
{
    
    JSFunction *fun = js_NewFunction(cx, NULL, NULL,
                                     funbox->function()->nargs,
                                     funbox->function()->flags,
                                     funbox->function()->getParent(),
                                     funbox->function()->atom,
                                     JSFunction::ExtendedFinalizeKind);
    if (!fun)
        return false;

    JSScript *script = funbox->function()->script();
    if (script) {
        fun->setScript(funbox->function()->script());
        if (!fun->script()->typeSetFunction(cx, fun))
            return false;
    }

    JS_ASSERT(funbox->function()->joinable());
    fun->setJoinable();

    fun->setMethodAtom(atom);

    funbox->object = fun;
    return true;
}

static bool
EmitForIn(JSContext *cx, CodeGenerator *cg, ParseNode *pn, ptrdiff_t top)
{
    StmtInfo stmtInfo;
    PushStatement(cg, &stmtInfo, STMT_FOR_IN_LOOP, top);

    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    






    bool forLet = false;
    if (ParseNode *decl = forHead->pn_kid1) {
        JS_ASSERT(TokenKindIsDecl(decl->getKind()));
        forLet = decl->isKind(TOK_LET);
        cg->flags |= TCF_IN_FOR_INIT;
        if (!EmitTree(cx, cg, decl))
            return false;
        cg->flags &= ~TCF_IN_FOR_INIT;
    }

    
    {
        TempPopScope tps;
        if (forLet && !tps.popBlock(cx, cg))
            return false;
        if (!EmitTree(cx, cg, forHead->pn_kid3))
            return false;
        if (forLet && !tps.repushBlock(cx, cg))
            return false;
    }

    




    JS_ASSERT(pn->isOp(JSOP_ITER));
    if (Emit2(cx, cg, JSOP_ITER, (uint8) pn->pn_iflags) < 0)
        return false;

    
    intN noteIndex = NewSrcNote(cx, cg, SRC_FOR_IN);
    if (noteIndex < 0)
        return false;

    



    ptrdiff_t jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
    if (jmp < 0)
        return false;

    intN noteIndex2 = NewSrcNote(cx, cg, SRC_TRACE);
    if (noteIndex2 < 0)
        return false;

    top = CG_OFFSET(cg);
    SET_STATEMENT_TOP(&stmtInfo, top);
    if (EmitTraceOp(cx, cg, NULL) < 0)
        return false;

#ifdef DEBUG
    intN loopDepth = cg->stackDepth;
#endif

    





    if (!EmitAssignment(cx, cg, forHead->pn_kid2, JSOP_NOP, NULL))
        return false;
    ptrdiff_t tmp2 = CG_OFFSET(cg);
    if (forHead->pn_kid1 && NewSrcNote2(cx, cg, SRC_DECL,
                                        (forHead->pn_kid1->isOp(JSOP_DEFVAR))
                                        ? SRC_DECL_VAR
                                        : SRC_DECL_LET) < 0) {
        return false;
    }
    if (Emit1(cx, cg, JSOP_POP) < 0)
        return false;

    
    JS_ASSERT(cg->stackDepth == loopDepth);

    
    if (!EmitTree(cx, cg, forBody))
        return false;

    
    StmtInfo *stmt = &stmtInfo;
    do {
        stmt->update = CG_OFFSET(cg);
    } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

    


    CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
    if (Emit1(cx, cg, JSOP_MOREITER) < 0)
        return false;
    ptrdiff_t beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
    if (beq < 0)
        return false;

    



    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex2, 0, beq - top))
        return false;
    
    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp2 - jmp))
        return false;
    
    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 1, beq - jmp))
        return false;

    
    if (!PopStatementCG(cx, cg))
        return false;

    if (!NewTryNote(cx, cg, JSTRY_ITER, cg->stackDepth, top, CG_OFFSET(cg)))
        return false;

    return Emit1(cx, cg, JSOP_ENDITER) >= 0;
}

static bool
EmitNormalFor(JSContext *cx, CodeGenerator *cg, ParseNode *pn, ptrdiff_t top)
{
    StmtInfo stmtInfo;
    PushStatement(cg, &stmtInfo, STMT_FOR_LOOP, top);

    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    
    JSOp op = JSOP_POP;
    ParseNode *pn3 = forHead->pn_kid1;
    if (!pn3) {
        
        op = JSOP_NOP;
    } else {
        cg->flags |= TCF_IN_FOR_INIT;
#if JS_HAS_DESTRUCTURING
        if (pn3->isKind(TOK_ASSIGN) &&
            !MaybeEmitGroupAssignment(cx, cg, op, pn3, &op)) {
            return false;
        }
#endif
        if (op == JSOP_POP) {
            if (!EmitTree(cx, cg, pn3))
                return false;
            if (TokenKindIsDecl(pn3->getKind())) {
                





                JS_ASSERT(pn3->isArity(PN_LIST));
                if (pn3->pn_xflags & PNX_GROUPINIT)
                    op = JSOP_NOP;
            }
        }
        cg->flags &= ~TCF_IN_FOR_INIT;
    }

    





    intN noteIndex = NewSrcNote(cx, cg, SRC_FOR);
    if (noteIndex < 0 || Emit1(cx, cg, op) < 0)
        return false;
    ptrdiff_t tmp = CG_OFFSET(cg);

    ptrdiff_t jmp = -1;
    if (forHead->pn_kid2) {
        
        jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
        if (jmp < 0)
            return false;
    }

    top = CG_OFFSET(cg);
    SET_STATEMENT_TOP(&stmtInfo, top);

    intN noteIndex2 = NewSrcNote(cx, cg, SRC_TRACE);
    if (noteIndex2 < 0)
        return false;

    
    if (EmitTraceOp(cx, cg, forBody) < 0)
        return false;
    if (!EmitTree(cx, cg, forBody))
        return false;

    
    JS_ASSERT(noteIndex != -1);
    ptrdiff_t tmp2 = CG_OFFSET(cg);

    
    StmtInfo *stmt = &stmtInfo;
    do {
        stmt->update = CG_OFFSET(cg);
    } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

    
    pn3 = forHead->pn_kid3;
    if (pn3) {
        op = JSOP_POP;
#if JS_HAS_DESTRUCTURING
        if (pn3->isKind(TOK_ASSIGN) &&
            !MaybeEmitGroupAssignment(cx, cg, op, pn3, &op)) {
            return false;
        }
#endif
        if (op == JSOP_POP && !EmitTree(cx, cg, pn3))
            return false;

        
        if (Emit1(cx, cg, op) < 0)
            return false;

        
        ptrdiff_t lineno = pn->pn_pos.end.lineno;
        if (CG_CURRENT_LINE(cg) != (uintN) lineno) {
            if (NewSrcNote2(cx, cg, SRC_SETLINE, lineno) < 0)
                return false;
            CG_CURRENT_LINE(cg) = (uintN) lineno;
        }
    }

    ptrdiff_t tmp3 = CG_OFFSET(cg);

    if (forHead->pn_kid2) {
        
        JS_ASSERT(jmp >= 0);
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);

        if (!EmitTree(cx, cg, forHead->pn_kid2))
            return false;
    }

    



    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex2, 0, CG_OFFSET(cg) - top))
        return false;
    
    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp3 - tmp))
        return false;
    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 1, tmp2 - tmp))
        return false;
    
    if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 2, CG_OFFSET(cg) - tmp))
        return false;

    
    op = forHead->pn_kid2 ? JSOP_IFNE : JSOP_GOTO;
    if (EmitJump(cx, cg, op, top - CG_OFFSET(cg)) < 0)
        return false;

    
    return PopStatementCG(cx, cg);
}

static inline bool
EmitFor(JSContext *cx, CodeGenerator *cg, ParseNode *pn, ptrdiff_t top)
{
    return pn->pn_left->isKind(TOK_IN)
           ? EmitForIn(cx, cg, pn, top)
           : EmitNormalFor(cx, cg, pn, top);
}

JSBool
frontend::EmitTree(JSContext *cx, CodeGenerator *cg, ParseNode *pn)
{
    JSBool useful, wantval;
    StmtInfo stmtInfo;
    StmtInfo *stmt;
    ptrdiff_t top, off, tmp, beq, jmp;
    ParseNode *pn2, *pn3;
    JSAtom *atom;
    jsatomid atomIndex;
    uintN index;
    ptrdiff_t noteIndex, noteIndex2;
    SrcNoteType noteType;
    jsbytecode *pc;
    JSOp op;
    uint32 argc;
    EmitLevelManager elm(cg);
#if JS_HAS_SHARP_VARS
    jsint sharpnum;
#endif

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    JSBool ok = true;
    pn->pn_offset = top = CG_OFFSET(cg);

    
    UPDATE_LINE_NUMBER_NOTES(cx, cg, pn->pn_pos.begin.lineno);

    switch (pn->getKind()) {
      case TOK_FUNCTION:
      {
        JSFunction *fun;
        uintN slot;

#if JS_HAS_XML_SUPPORT
        if (pn->isArity(PN_NULLARY)) {
            if (Emit1(cx, cg, JSOP_GETFUNNS) < 0)
                return JS_FALSE;
            break;
        }
#endif

        fun = pn->pn_funbox->function();
        JS_ASSERT(fun->isInterpreted());
        if (fun->script()) {
            




            JS_ASSERT(pn->isOp(JSOP_NOP));
            JS_ASSERT(cg->inFunction());
            if (!EmitFunctionDefNop(cx, cg, pn->pn_index))
                return JS_FALSE;
            break;
        }

        JS_ASSERT_IF(pn->pn_funbox->tcflags & TCF_FUN_HEAVYWEIGHT,
                     fun->kind() == JSFUN_INTERPRETED);

        
        CodeGenerator *cg2 = cx->new_<CodeGenerator>(cg->parser, pn->pn_pos.begin.lineno);
        if (!cg2) {
            js_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
        if (!cg2->init(cx))
            return JS_FALSE;

        cg2->flags = pn->pn_funbox->tcflags | TCF_COMPILING | TCF_IN_FUNCTION |
                     (cg->flags & TCF_FUN_MIGHT_ALIAS_LOCALS);
        cg2->bindings.transfer(cx, &pn->pn_funbox->bindings);
#if JS_HAS_SHARP_VARS
        if (cg2->flags & TCF_HAS_SHARPS) {
            cg2->sharpSlotBase = cg2->bindings.sharpSlotBase(cx);
            if (cg2->sharpSlotBase < 0)
                return JS_FALSE;
        }
#endif
        cg2->setFunction(fun);
        cg2->funbox = pn->pn_funbox;
        cg2->parent = cg;

        





        JS_ASSERT(cg->staticLevel < JS_BITMASK(16) - 1);
        cg2->staticLevel = cg->staticLevel + 1;

        
        if (!EmitFunctionScript(cx, cg2, pn->pn_body))
            pn = NULL;

        cx->delete_(cg2);
        cg2 = NULL;
        if (!pn)
            return JS_FALSE;

        
        index = cg->objectList.index(pn->pn_funbox);

        
        op = pn->getOp();
        if (op != JSOP_NOP) {
            if ((pn->pn_funbox->tcflags & TCF_GENEXP_LAMBDA) &&
                NewSrcNote(cx, cg, SRC_GENEXP) < 0)
            {
                return JS_FALSE;
            }
            EMIT_INDEX_OP(op, index);

            
            if (EmitBlockChain(cx, cg) < 0)
                return JS_FALSE;
            break;
        }

        








        if (!cg->inFunction()) {
            JS_ASSERT(!cg->topStmt);
            if (!BindGlobal(cx, cg, pn, fun->atom))
                return false;
            if (pn->pn_cookie.isFree()) {
                CG_SWITCH_TO_PROLOG(cg);
                op = fun->isFlatClosure() ? JSOP_DEFFUN_FC : JSOP_DEFFUN;
                EMIT_INDEX_OP(op, index);

                
                if (EmitBlockChain(cx, cg) < 0)
                    return JS_FALSE;
                CG_SWITCH_TO_MAIN(cg);
            }

            
            if (!EmitFunctionDefNop(cx, cg, index))
                return JS_FALSE;
        } else {
            DebugOnly<BindingKind> kind = cg->bindings.lookup(cx, fun->atom, &slot);
            JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
            JS_ASSERT(index < JS_BIT(20));
            pn->pn_index = index;
            op = fun->isFlatClosure() ? JSOP_DEFLOCALFUN_FC : JSOP_DEFLOCALFUN;
            if (pn->isClosed() &&
                !cg->callsEval() &&
                !cg->closedVars.append(pn->pn_cookie.slot())) {
                return JS_FALSE;
            }
            if (!EmitSlotIndexOp(cx, op, slot, index, cg))
                return JS_FALSE;

            
            if (EmitBlockChain(cx, cg) < 0)
                return JS_FALSE;
        }
        break;
      }

      case TOK_ARGSBODY:
      {
        ParseNode *pnlast = pn->last();
        for (ParseNode *pn2 = pn->pn_head; pn2 != pnlast; pn2 = pn2->pn_next) {
            if (!pn2->isDefn())
                continue;
            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            if (JOF_OPTYPE(pn2->getOp()) == JOF_QARG && cg->shouldNoteClosedName(pn2)) {
                if (!cg->closedArgs.append(pn2->pn_cookie.slot()))
                    return JS_FALSE;
            }
        }
        ok = EmitTree(cx, cg, pnlast);
        break;
      }

      case TOK_UPVARS:
        JS_ASSERT(pn->pn_names->count() != 0);
        cg->roLexdeps = pn->pn_names;
        ok = EmitTree(cx, cg, pn->pn_tree);
        cg->roLexdeps.clearMap();
        pn->pn_names.releaseMap(cx);
        break;

      case TOK_IF:
        ok = EmitIf(cx, cg, pn);
        break;

      case TOK_SWITCH:
        ok = EmitSwitch(cx, cg, pn);
        break;

      case TOK_WHILE:
        



















        PushStatement(cg, &stmtInfo, STMT_WHILE_LOOP, top);
        noteIndex = NewSrcNote(cx, cg, SRC_WHILE);
        if (noteIndex < 0)
            return JS_FALSE;
        jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
        if (jmp < 0)
            return JS_FALSE;
        noteIndex2 = NewSrcNote(cx, cg, SRC_TRACE);
        if (noteIndex2 < 0)
            return JS_FALSE;
        top = EmitTraceOp(cx, cg, pn->pn_right);
        if (top < 0)
            return JS_FALSE;
        if (!EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
        if (!EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
        if (beq < 0)
            return JS_FALSE;
        



        if (!SetSrcNoteOffset(cx, cg, noteIndex2, 0, beq - top))
            return JS_FALSE;
        if (!SetSrcNoteOffset(cx, cg, noteIndex, 0, beq - jmp))
            return JS_FALSE;
        ok = PopStatementCG(cx, cg);
        break;

      case TOK_DO:
        
        noteIndex = NewSrcNote(cx, cg, SRC_WHILE);
        if (noteIndex < 0 || Emit1(cx, cg, JSOP_NOP) < 0)
            return JS_FALSE;

        noteIndex2 = NewSrcNote(cx, cg, SRC_TRACE);
        if (noteIndex2 < 0)
            return JS_FALSE;

        
        top = EmitTraceOp(cx, cg, pn->pn_left);
        if (top < 0)
            return JS_FALSE;
        PushStatement(cg, &stmtInfo, STMT_DO_LOOP, top);
        if (!EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;

        
        off = CG_OFFSET(cg);
        stmt = &stmtInfo;
        do {
            stmt->update = off;
        } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

        
        if (!EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;

        




        beq = EmitJump(cx, cg, JSOP_IFNE, top - CG_OFFSET(cg));
        if (beq < 0)
            return JS_FALSE;
        



        if (!SetSrcNoteOffset(cx, cg, noteIndex2, 0, beq - top))
            return JS_FALSE;
        if (!SetSrcNoteOffset(cx, cg, noteIndex, 0, 1 + (off - top)))
            return JS_FALSE;
        ok = PopStatementCG(cx, cg);
        break;

      case TOK_FOR:
        ok = EmitFor(cx, cg, pn, top);
        break;

      case TOK_BREAK: {
        stmt = cg->topStmt;
        atom = pn->pn_atom;

        jsatomid labelIndex;
        if (atom) {
            if (!cg->makeAtomIndex(atom, &labelIndex))
                return JS_FALSE;

            while (stmt->type != STMT_LABEL || stmt->label != atom)
                stmt = stmt->down;
            noteType = SRC_BREAK2LABEL;
        } else {
            labelIndex = INVALID_ATOMID;
            while (!STMT_IS_LOOP(stmt) && stmt->type != STMT_SWITCH)
                stmt = stmt->down;
            noteType = (stmt->type == STMT_SWITCH) ? SRC_SWITCHBREAK : SRC_BREAK;
        }

        if (EmitGoto(cx, cg, stmt, &stmt->breaks, labelIndex, noteType) < 0)
            return JS_FALSE;
        break;
      }

      case TOK_CONTINUE: {
        stmt = cg->topStmt;
        atom = pn->pn_atom;

        jsatomid labelIndex;
        if (atom) {
            
            StmtInfo *loop = NULL;
            if (!cg->makeAtomIndex(atom, &labelIndex))
                return JS_FALSE;
            while (stmt->type != STMT_LABEL || stmt->label != atom) {
                if (STMT_IS_LOOP(stmt))
                    loop = stmt;
                stmt = stmt->down;
            }
            stmt = loop;
            noteType = SRC_CONT2LABEL;
        } else {
            labelIndex = INVALID_ATOMID;
            while (!STMT_IS_LOOP(stmt))
                stmt = stmt->down;
            noteType = SRC_CONTINUE;
        }

        if (EmitGoto(cx, cg, stmt, &stmt->continues, labelIndex, noteType) < 0)
            return JS_FALSE;
        break;
      }

      case TOK_WITH:
        if (!EmitWith(cx, cg, pn, ok))
            return false;
        break;

      case TOK_TRY:
        if (!EmitTry(cx, cg, pn))
            return false;
        break;

      case TOK_CATCH:
        if (!EmitCatch(cx, cg, pn))
            return false;
        break;

      case TOK_VAR:
        if (!EmitVariables(cx, cg, pn, JS_FALSE, &noteIndex))
            return JS_FALSE;
        break;

      case TOK_RETURN:
        
        pn2 = pn->pn_kid;
        if (pn2) {
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
        } else {
            if (Emit1(cx, cg, JSOP_PUSH) < 0)
                return JS_FALSE;
        }

        










        top = CG_OFFSET(cg);
        if (Emit1(cx, cg, JSOP_RETURN) < 0)
            return JS_FALSE;
        if (!EmitNonLocalJumpFixup(cx, cg, NULL))
            return JS_FALSE;
        if (top + JSOP_RETURN_LENGTH != CG_OFFSET(cg)) {
            CG_BASE(cg)[top] = JSOP_SETRVAL;
            if (Emit1(cx, cg, JSOP_RETRVAL) < 0)
                return JS_FALSE;
            if (EmitBlockChain(cx, cg) < 0)
                return JS_FALSE;
        }
        break;

#if JS_HAS_GENERATORS
      case TOK_YIELD:
        JS_ASSERT(cg->inFunction());
        if (pn->pn_kid) {
            if (!EmitTree(cx, cg, pn->pn_kid))
                return JS_FALSE;
        } else {
            if (Emit1(cx, cg, JSOP_PUSH) < 0)
                return JS_FALSE;
        }
        if (pn->pn_hidden && NewSrcNote(cx, cg, SRC_HIDDEN) < 0)
            return JS_FALSE;
        if (Emit1(cx, cg, JSOP_YIELD) < 0)
            return JS_FALSE;
        break;
#endif

      case TOK_LC:
      {
#if JS_HAS_XML_SUPPORT
        if (pn->isArity(PN_UNARY)) {
            if (!EmitTree(cx, cg, pn->pn_kid))
                return JS_FALSE;
            if (Emit1(cx, cg, pn->getOp()) < 0)
                return JS_FALSE;
            break;
        }
#endif

        JS_ASSERT(pn->isArity(PN_LIST));

        noteIndex = -1;
        tmp = CG_OFFSET(cg);
        if (pn->pn_xflags & PNX_NEEDBRACES) {
            noteIndex = NewSrcNote2(cx, cg, SRC_BRACE, 0);
            if (noteIndex < 0 || Emit1(cx, cg, JSOP_NOP) < 0)
                return JS_FALSE;
        }

        PushStatement(cg, &stmtInfo, STMT_BLOCK, top);

        ParseNode *pnchild = pn->pn_head;
        if (pn->pn_xflags & PNX_FUNCDEFS) {
            










            JS_ASSERT(cg->inFunction());
            if (pn->pn_xflags & PNX_DESTRUCT) {
                



                JS_ASSERT(pnchild->isKind(TOK_SEMI));
                JS_ASSERT(pnchild->pn_kid->isKind(TOK_VAR));
                if (!EmitTree(cx, cg, pnchild))
                    return JS_FALSE;
                pnchild = pnchild->pn_next;
            }

            for (pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
                if (pn2->isKind(TOK_FUNCTION)) {
                    if (pn2->isOp(JSOP_NOP)) {
                        if (!EmitTree(cx, cg, pn2))
                            return JS_FALSE;
                    } else {
                        





                        JS_ASSERT(pn2->isOp(JSOP_DEFFUN));
                    }
                }
            }
        }
        for (pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }

        if (noteIndex >= 0 && !SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, CG_OFFSET(cg) - tmp))
            return JS_FALSE;

        ok = PopStatementCG(cx, cg);
        break;
      }

      case TOK_SEQ:
        JS_ASSERT(pn->isArity(PN_LIST));
        PushStatement(cg, &stmtInfo, STMT_SEQ, top);
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
        }
        ok = PopStatementCG(cx, cg);
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
                cg->topStmt &&
                cg->topStmt->type == STMT_LABEL &&
                cg->topStmt->update >= CG_OFFSET(cg)) {
                useful = true;
            }

            if (!useful) {
                
                if (!pn->isDirectivePrologueMember()) {
                    CG_CURRENT_LINE(cg) = pn2->pn_pos.begin.lineno;
                    if (!ReportCompileErrorNumber(cx, CG_TS(cg), pn2,
                                                  JSREPORT_WARNING | JSREPORT_STRICT,
                                                  JSMSG_USELESS_EXPR)) {
                        return JS_FALSE;
                    }
                }
            } else {
                op = wantval ? JSOP_POPV : JSOP_POP;
#if JS_HAS_DESTRUCTURING
                if (!wantval &&
                    pn2->isKind(TOK_ASSIGN) &&
                    !MaybeEmitGroupAssignment(cx, cg, op, pn2, &op)) {
                    return JS_FALSE;
                }
#endif
                if (op != JSOP_NOP) {
                    






                    if (!wantval &&
                        pn2->isKind(TOK_ASSIGN) &&
                        pn2->isOp(JSOP_NOP) &&
                        pn2->pn_left->isOp(JSOP_SETPROP) &&
                        pn2->pn_right->isOp(JSOP_LAMBDA) &&
                        pn2->pn_right->pn_funbox->joinable()) {
                        if (!SetMethodFunction(cx, pn2->pn_right->pn_funbox, pn2->pn_left->pn_atom))
                            return JS_FALSE;
                        pn2->pn_left->setOp(JSOP_SETMETHOD);
                    }
                    if (!EmitTree(cx, cg, pn2))
                        return JS_FALSE;
                    if (Emit1(cx, cg, op) < 0)
                        return JS_FALSE;
                }
            }
        }
        break;

      case TOK_COLON:
        
        atom = pn->pn_atom;

        jsatomid index;
        if (!cg->makeAtomIndex(atom, &index))
            return JS_FALSE;

        pn2 = pn->expr();
        noteType = (pn2->isKind(TOK_LC) ||
                    (pn2->isKind(TOK_LEXICALSCOPE) &&
                     pn2->expr()->isKind(TOK_LC)))
                   ? SRC_LABELBRACE
                   : SRC_LABEL;
        noteIndex = NewSrcNote2(cx, cg, noteType, ptrdiff_t(index));
        if (noteIndex < 0 || Emit1(cx, cg, JSOP_NOP) < 0)
            return JS_FALSE;

        
        PushStatement(cg, &stmtInfo, STMT_LABEL, CG_OFFSET(cg));
        stmtInfo.label = atom;
        if (!EmitTree(cx, cg, pn2))
            return JS_FALSE;
        if (!PopStatementCG(cx, cg))
            return JS_FALSE;

        
        if (noteType == SRC_LABELBRACE) {
            if (NewSrcNote(cx, cg, SRC_ENDBRACE) < 0 ||
                Emit1(cx, cg, JSOP_NOP) < 0) {
                return JS_FALSE;
            }
        }
        break;

      case TOK_COMMA:
        




        off = noteIndex = -1;
        for (pn2 = pn->pn_head; ; pn2 = pn2->pn_next) {
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
            tmp = CG_OFFSET(cg);
            if (noteIndex >= 0) {
                if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp-off))
                    return JS_FALSE;
            }
            if (!pn2->pn_next)
                break;
            off = tmp;
            noteIndex = NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
            if (noteIndex < 0 ||
                Emit1(cx, cg, JSOP_POP) < 0) {
                return JS_FALSE;
            }
        }
        break;

      case TOK_ASSIGN:
        if (!EmitAssignment(cx, cg, pn->pn_left, pn->getOp(), pn->pn_right))
            return false;
        break;

      case TOK_HOOK:
        
        if (!EmitTree(cx, cg, pn->pn_kid1))
            return JS_FALSE;
        noteIndex = NewSrcNote(cx, cg, SRC_COND);
        if (noteIndex < 0)
            return JS_FALSE;
        beq = EmitJump(cx, cg, JSOP_IFEQ, 0);
        if (beq < 0 || !EmitTree(cx, cg, pn->pn_kid2))
            return JS_FALSE;

        
        jmp = EmitJump(cx, cg, JSOP_GOTO, 0);
        if (jmp < 0)
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, beq);

        











        JS_ASSERT(cg->stackDepth > 0);
        cg->stackDepth--;
        if (!EmitTree(cx, cg, pn->pn_kid3))
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
        if (!SetSrcNoteOffset(cx, cg, noteIndex, 0, jmp - beq))
            return JS_FALSE;
        break;

      case TOK_OR:
      case TOK_AND:
        









        if (pn->isArity(PN_BINARY)) {
            if (!EmitTree(cx, cg, pn->pn_left))
                return JS_FALSE;
            top = EmitJump(cx, cg, JSOP_BACKPATCH_POP, 0);
            if (top < 0)
                return JS_FALSE;
            if (!EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;
            off = CG_OFFSET(cg);
            pc = CG_CODE(cg, top);
            CHECK_AND_SET_JUMP_OFFSET(cx, cg, pc, off - top);
            *pc = pn->getOp();
        } else {
            JS_ASSERT(pn->isArity(PN_LIST));
            JS_ASSERT(pn->pn_head->pn_next->pn_next);

            
            pn2 = pn->pn_head;
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
            top = EmitJump(cx, cg, JSOP_BACKPATCH_POP, 0);
            if (top < 0)
                return JS_FALSE;

            
            jmp = top;
            while ((pn2 = pn2->pn_next)->pn_next) {
                if (!EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                off = EmitJump(cx, cg, JSOP_BACKPATCH_POP, 0);
                if (off < 0)
                    return JS_FALSE;
                if (!SetBackPatchDelta(cx, cg, CG_CODE(cg, jmp), off - jmp))
                    return JS_FALSE;
                jmp = off;

            }
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;

            pn2 = pn->pn_head;
            off = CG_OFFSET(cg);
            do {
                pc = CG_CODE(cg, top);
                tmp = GetJumpOffset(cg, pc);
                CHECK_AND_SET_JUMP_OFFSET(cx, cg, pc, off - top);
                *pc = pn->getOp();
                top += tmp;
            } while ((pn2 = pn2->pn_next)->pn_next);
        }
        break;

      case TOK_PLUS:
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
        if (pn->isArity(PN_LIST)) {
            
            pn2 = pn->pn_head;
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
            op = pn->getOp();
            while ((pn2 = pn2->pn_next) != NULL) {
                if (!EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                if (Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            }
        } else {
#if JS_HAS_XML_SUPPORT
            uintN oldflags;

      case TOK_DBLCOLON:
            if (pn->isArity(PN_NAME)) {
                if (!EmitTree(cx, cg, pn->expr()))
                    return JS_FALSE;
                if (!EmitAtomOp(cx, pn, pn->getOp(), cg))
                    return JS_FALSE;
                break;
            }

            




            oldflags = cg->flags;
            cg->flags &= ~TCF_IN_FOR_INIT;
#endif

            
            if (!EmitTree(cx, cg, pn->pn_left))
                return JS_FALSE;
            if (!EmitTree(cx, cg, pn->pn_right))
                return JS_FALSE;
#if JS_HAS_XML_SUPPORT
            cg->flags |= oldflags & TCF_IN_FOR_INIT;
#endif
            if (Emit1(cx, cg, pn->getOp()) < 0)
                return JS_FALSE;
        }
        break;

      case TOK_THROW:
#if JS_HAS_XML_SUPPORT
      case TOK_AT:
      case TOK_DEFAULT:
        JS_ASSERT(pn->isArity(PN_UNARY));
        
#endif
      case TOK_UNARYOP:
      {
        uintN oldflags;

        
        op = pn->getOp();
#if JS_HAS_XML_SUPPORT
        if (op == JSOP_XMLNAME) {
            if (!EmitXMLName(cx, pn, op, cg))
                return JS_FALSE;
            break;
        }
#endif
        pn2 = pn->pn_kid;

        if (op == JSOP_TYPEOF && !pn2->isKind(TOK_NAME))
            op = JSOP_TYPEOFEXPR;

        oldflags = cg->flags;
        cg->flags &= ~TCF_IN_FOR_INIT;
        if (!EmitTree(cx, cg, pn2))
            return JS_FALSE;
        cg->flags |= oldflags & TCF_IN_FOR_INIT;
        if (Emit1(cx, cg, op) < 0)
            return JS_FALSE;
        break;
      }

      case TOK_INC:
      case TOK_DEC:
        
        pn2 = pn->pn_kid;
        JS_ASSERT(!pn2->isKind(TOK_RP));
        op = pn->getOp();
        switch (pn2->getKind()) {
          default:
            JS_ASSERT(pn2->isKind(TOK_NAME));
            pn2->setOp(op);
            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            op = pn2->getOp();
            if (op == JSOP_CALLEE) {
                if (Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            } else if (!pn2->pn_cookie.isFree()) {
                atomIndex = pn2->pn_cookie.asInteger();
                EMIT_UINT16_IMM_OP(op, atomIndex);
            } else {
                JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
                if (js_CodeSpec[op].format & (JOF_INC | JOF_DEC)) {
                    if (!EmitNameIncDec(cx, pn2, op, cg))
                        return JS_FALSE;
                } else {
                    if (!EmitAtomOp(cx, pn2, op, cg))
                        return JS_FALSE;
                }
                break;
            }
            if (pn2->isConst()) {
                if (Emit1(cx, cg, JSOP_POS) < 0)
                    return JS_FALSE;
                op = pn->getOp();
                if (!(js_CodeSpec[op].format & JOF_POST)) {
                    if (Emit1(cx, cg, JSOP_ONE) < 0)
                        return JS_FALSE;
                    op = (js_CodeSpec[op].format & JOF_INC) ? JSOP_ADD : JSOP_SUB;
                    if (Emit1(cx, cg, op) < 0)
                        return JS_FALSE;
                }
            }
            break;
          case TOK_DOT:
            if (!EmitPropIncDec(cx, pn2, op, cg))
                return JS_FALSE;
            break;
          case TOK_LB:
            if (!EmitElemIncDec(cx, pn2, op, cg))
                return JS_FALSE;
            break;
          case TOK_LP:
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
            if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - pn2->pn_offset) < 0)
                return JS_FALSE;
            if (Emit1(cx, cg, op) < 0)
                return JS_FALSE;
            




            JS_ASSERT(js_CodeSpec[op].format & JOF_DECOMPOSE);
            JS_ASSERT(js_CodeSpec[op].format & JOF_ELEM);
            if (Emit1(cx, cg, (JSOp)1) < 0)
                return JS_FALSE;
            if (Emit1(cx, cg, JSOP_POP) < 0)
                return JS_FALSE;
            break;
#if JS_HAS_XML_SUPPORT
          case TOK_UNARYOP:
            JS_ASSERT(!cg->inStrictMode());
            JS_ASSERT(pn2->isOp(JSOP_SETXMLNAME));
            if (!EmitTree(cx, cg, pn2->pn_kid))
                return JS_FALSE;
            if (Emit1(cx, cg, JSOP_BINDXMLNAME) < 0)
                return JS_FALSE;
            if (!EmitElemIncDec(cx, NULL, op, cg))
                return JS_FALSE;
            break;
#endif
        }
        break;

      case TOK_DELETE:
        



        pn2 = pn->pn_kid;
        switch (pn2->getKind()) {
          case TOK_NAME:
            if (!BindNameToSlot(cx, cg, pn2))
                return JS_FALSE;
            op = pn2->getOp();
            if (op == JSOP_FALSE) {
                if (Emit1(cx, cg, op) < 0)
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
            JS_ASSERT(!cg->inStrictMode());
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
                JS_ASSERT_IF(pn2->isKind(TOK_LP), !(pn2->pn_xflags & PNX_SETCALL));
                if (!EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                off = CG_OFFSET(cg);
                noteIndex = NewSrcNote2(cx, cg, SRC_PCDELTA, 0);
                if (noteIndex < 0 || Emit1(cx, cg, JSOP_POP) < 0)
                    return JS_FALSE;
            }
            if (Emit1(cx, cg, JSOP_TRUE) < 0)
                return JS_FALSE;
            if (noteIndex >= 0) {
                tmp = CG_OFFSET(cg);
                if (!SetSrcNoteOffset(cx, cg, (uintN)noteIndex, 0, tmp-off))
                    return JS_FALSE;
            }
        }
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_FILTER:
        JS_ASSERT(!cg->inStrictMode());

        if (!EmitTree(cx, cg, pn->pn_left))
            return JS_FALSE;
        jmp = EmitJump(cx, cg, JSOP_FILTER, 0);
        if (jmp < 0)
            return JS_FALSE;
        top = EmitTraceOp(cx, cg, pn->pn_right);
        if (top < 0)
            return JS_FALSE;
        if (!EmitTree(cx, cg, pn->pn_right))
            return JS_FALSE;
        CHECK_AND_SET_JUMP_OFFSET_AT(cx, cg, jmp);
        if (EmitJump(cx, cg, JSOP_ENDFILTER, top - CG_OFFSET(cg)) < 0)
            return JS_FALSE;

        
        if (EmitBlockChain(cx, cg) < 0)
            return JS_FALSE;
        break;
#endif

      case TOK_DOT:
        




        ok = EmitPropOp(cx, pn, pn->getOp(), cg, JS_FALSE);
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_DBLDOT:
        JS_ASSERT(!cg->inStrictMode());
        
#endif
      case TOK_LB:
        





        ok = EmitElemOp(cx, pn, pn->getOp(), cg);
        break;

      case TOK_NEW:
      case TOK_LP:
      {
        bool callop = pn->isKind(TOK_LP);

        














        pn2 = pn->pn_head;
        switch (pn2->getKind()) {
          case TOK_NAME:
            if (!EmitNameOp(cx, cg, pn2, callop))
                return JS_FALSE;
            break;
          case TOK_DOT:
            if (!EmitPropOp(cx, pn2, pn2->getOp(), cg, callop))
                return JS_FALSE;
            break;
          case TOK_LB:
            JS_ASSERT(pn2->isOp(JSOP_GETELEM));
            if (!EmitElemOp(cx, pn2, callop ? JSOP_CALLELEM : JSOP_GETELEM, cg))
                return JS_FALSE;
            break;
          case TOK_UNARYOP:
#if JS_HAS_XML_SUPPORT
            if (pn2->isOp(JSOP_XMLNAME)) {
                if (!EmitXMLName(cx, pn2, JSOP_CALLXMLNAME, cg))
                    return JS_FALSE;
                callop = true;          
                break;
            }
#endif
            
          default:
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
            callop = false;             
            break;
        }
        if (!callop && Emit1(cx, cg, JSOP_PUSH) < 0)
            return JS_FALSE;

        
        off = top;

        




        uintN oldflags = cg->flags;
        cg->flags &= ~TCF_IN_FOR_INIT;
        for (pn3 = pn2->pn_next; pn3; pn3 = pn3->pn_next) {
            if (!EmitTree(cx, cg, pn3))
                return JS_FALSE;
        }
        cg->flags |= oldflags & TCF_IN_FOR_INIT;
        if (NewSrcNote2(cx, cg, SRC_PCBASE, CG_OFFSET(cg) - off) < 0)
            return JS_FALSE;

        argc = pn->pn_count - 1;
        if (Emit3(cx, cg, pn->getOp(), ARGC_HI(argc), ARGC_LO(argc)) < 0)
            return JS_FALSE;
        CheckTypeSet(cx, cg, pn->getOp());
        if (pn->isOp(JSOP_EVAL)) {
            EMIT_UINT16_IMM_OP(JSOP_LINENO, pn->pn_pos.begin.lineno);
            if (EmitBlockChain(cx, cg) < 0)
                return JS_FALSE;
        }
        if (pn->pn_xflags & PNX_SETCALL) {
            if (Emit1(cx, cg, JSOP_SETCALL) < 0)
                return JS_FALSE;
        }
        break;
      }

      case TOK_LEXICALSCOPE:
        if (!EmitLexicalScope(cx, cg, pn, ok))
            return false;
        break;

#if JS_HAS_BLOCK_SCOPE
      case TOK_LET:
        if (!EmitLet(cx, cg, pn))
            return false;
        break;
#endif 

#if JS_HAS_GENERATORS
      case TOK_ARRAYPUSH: {
        jsint slot;

        




        if (!EmitTree(cx, cg, pn->pn_kid))
            return JS_FALSE;
        slot = AdjustBlockSlot(cx, cg, cg->arrayCompDepth);
        if (slot < 0)
            return JS_FALSE;
        EMIT_UINT16_IMM_OP(pn->getOp(), slot);
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

#if JS_HAS_GENERATORS
        if (pn->isKind(TOK_ARRAYCOMP)) {
            uintN saveDepth;

            if (!EmitNewInit(cx, cg, JSProto_Array, pn, sharpnum))
                return JS_FALSE;

            




            JS_ASSERT(cg->stackDepth > 0);
            saveDepth = cg->arrayCompDepth;
            cg->arrayCompDepth = (uint32) (cg->stackDepth - 1);
            if (!EmitTree(cx, cg, pn->pn_head))
                return JS_FALSE;
            cg->arrayCompDepth = saveDepth;

            
            if (!EmitEndInit(cx, cg, 1))
                return JS_FALSE;
            break;
        }
#endif 

        if (!cg->hasSharps() && !(pn->pn_xflags & PNX_NONCONST) && pn->pn_head &&
            cg->checkSingletonContext()) {
            if (!EmitSingletonInitialiser(cx, cg, pn))
                return JS_FALSE;
            break;
        }

        
        if (cg->hasSharps()) {
            if (!EmitNewInit(cx, cg, JSProto_Array, pn, sharpnum))
                return JS_FALSE;
        } else {
            ptrdiff_t off = EmitN(cx, cg, JSOP_NEWARRAY, 3);
            if (off < 0)
                return JS_FALSE;
            pc = CG_CODE(cg, off);
            SET_UINT24(pc, pn->pn_count);
        }

        pn2 = pn->pn_head;
        for (atomIndex = 0; pn2; atomIndex++, pn2 = pn2->pn_next) {
            if (!EmitNumberOp(cx, atomIndex, cg))
                return JS_FALSE;
            if (pn2->isKind(TOK_COMMA) && pn2->isArity(PN_NULLARY)) {
                if (Emit1(cx, cg, JSOP_HOLE) < 0)
                    return JS_FALSE;
            } else {
                if (!EmitTree(cx, cg, pn2))
                    return JS_FALSE;
            }
            if (Emit1(cx, cg, JSOP_INITELEM) < 0)
                return JS_FALSE;
        }
        JS_ASSERT(atomIndex == pn->pn_count);

        if (pn->pn_xflags & PNX_ENDCOMMA) {
            
            if (NewSrcNote(cx, cg, SRC_CONTINUE) < 0)
                return JS_FALSE;
        }

        



        if (!EmitEndInit(cx, cg, atomIndex))
            return JS_FALSE;
        break;

      case TOK_RC: {
#if JS_HAS_SHARP_VARS
        sharpnum = -1;
      do_emit_object:
#endif
#if JS_HAS_DESTRUCTURING_SHORTHAND
        if (pn->pn_xflags & PNX_DESTRUCT) {
            ReportCompileErrorNumber(cx, CG_TS(cg), pn, JSREPORT_ERROR, JSMSG_BAD_OBJECT_INIT);
            return JS_FALSE;
        }
#endif

        if (!cg->hasSharps() && !(pn->pn_xflags & PNX_NONCONST) && pn->pn_head &&
            cg->checkSingletonContext()) {
            if (!EmitSingletonInitialiser(cx, cg, pn))
                return JS_FALSE;
            break;
        }

        







        ptrdiff_t offset = CG_NEXT(cg) - CG_BASE(cg);
        if (!EmitNewInit(cx, cg, JSProto_Object, pn, sharpnum))
            return JS_FALSE;

        



        JSObject *obj = NULL;
        if (!cg->hasSharps() && cg->compileAndGo()) {
            gc::AllocKind kind = GuessObjectGCKind(pn->pn_count);
            obj = NewBuiltinClassInstance(cx, &ObjectClass, kind);
            if (!obj)
                return JS_FALSE;
        }

        uintN methodInits = 0, slowMethodInits = 0;
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            
            pn3 = pn2->pn_left;
            if (pn3->isKind(TOK_NUMBER)) {
                if (!EmitNumberOp(cx, pn3->pn_dval, cg))
                    return JS_FALSE;
            }

            
            if (!EmitTree(cx, cg, pn2->pn_right))
                return JS_FALSE;

            op = pn2->getOp();
            if (op == JSOP_GETTER || op == JSOP_SETTER) {
                obj = NULL;
                if (Emit1(cx, cg, op) < 0)
                    return JS_FALSE;
            }

            
            if (pn3->isKind(TOK_NUMBER)) {
                obj = NULL;
                if (NewSrcNote(cx, cg, SRC_INITPROP) < 0)
                    return JS_FALSE;
                if (Emit1(cx, cg, JSOP_INITELEM) < 0)
                    return JS_FALSE;
            } else {
                JS_ASSERT(pn3->isKind(TOK_NAME) ||
                          pn3->isKind(TOK_STRING));
                jsatomid index;
                if (!cg->makeAtomIndex(pn3->pn_atom, &index))
                    return JS_FALSE;

                
                ParseNode *init = pn2->pn_right;
                bool lambda = init->isOp(JSOP_LAMBDA);
                if (lambda)
                    ++methodInits;
                if (op == JSOP_INITPROP && lambda && init->pn_funbox->joinable()) {
                    obj = NULL;
                    op = JSOP_INITMETHOD;
                    pn2->setOp(op);
                    if (!SetMethodFunction(cx, init->pn_funbox, pn3->pn_atom))
                        return JS_FALSE;
                } else {
                    



                    if (pn3->pn_atom == cx->runtime->atomState.protoAtom)
                        obj = NULL;
                    op = JSOP_INITPROP;
                    if (lambda)
                        ++slowMethodInits;
                }

                if (obj) {
                    JS_ASSERT(!obj->inDictionaryMode());
                    if (!DefineNativeProperty(cx, obj, ATOM_TO_JSID(pn3->pn_atom),
                                              UndefinedValue(), NULL, NULL,
                                              JSPROP_ENUMERATE, 0, 0)) {
                        return false;
                    }
                    if (obj->inDictionaryMode())
                        obj = NULL;
                }

                EMIT_INDEX_OP(op, index);
            }
        }

        if (!EmitEndInit(cx, cg, pn->pn_count))
            return JS_FALSE;

        if (obj) {
            



            ObjectBox *objbox = cg->parser->newObjectBox(obj);
            if (!objbox)
                return JS_FALSE;
            unsigned index = cg->objectList.index(objbox);
            if (FitsWithoutBigIndex(index))
                EMIT_UINT16_IN_PLACE(offset, JSOP_NEWOBJECT, uint16(index));
        }

        break;
      }

#if JS_HAS_SHARP_VARS
      case TOK_DEFSHARP:
        JS_ASSERT(cg->hasSharps());
        sharpnum = pn->pn_num;
        pn = pn->pn_kid;
        if (pn->isKind(TOK_RB))
            goto do_emit_array;
# if JS_HAS_GENERATORS
        if (pn->isKind(TOK_ARRAYCOMP))
            goto do_emit_array;
# endif
        if (pn->isKind(TOK_RC))
            goto do_emit_object;

        if (!EmitTree(cx, cg, pn))
            return JS_FALSE;
        EMIT_UINT16PAIR_IMM_OP(JSOP_DEFSHARP, cg->sharpSlotBase, (jsatomid) sharpnum);
        break;

      case TOK_USESHARP:
        JS_ASSERT(cg->hasSharps());
        EMIT_UINT16PAIR_IMM_OP(JSOP_USESHARP, cg->sharpSlotBase, (jsatomid) pn->pn_num);
        break;
#endif 

      case TOK_NAME:
        




        if (pn->isOp(JSOP_NOP))
            break;
        if (!EmitNameOp(cx, cg, pn, JS_FALSE))
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_XMLATTR:
      case TOK_XMLSPACE:
      case TOK_XMLTEXT:
      case TOK_XMLCDATA:
      case TOK_XMLCOMMENT:
        JS_ASSERT(!cg->inStrictMode());
        
#endif
      case TOK_STRING:
        ok = EmitAtomOp(cx, pn, pn->getOp(), cg);
        break;

      case TOK_NUMBER:
        ok = EmitNumberOp(cx, pn->pn_dval, cg);
        break;

      case TOK_REGEXP:
        JS_ASSERT(pn->isOp(JSOP_REGEXP));
        ok = EmitIndexOp(cx, JSOP_REGEXP, cg->regexpList.index(pn->pn_objbox), cg);
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_ANYNAME:
#endif
      case TOK_PRIMARY:
        if (Emit1(cx, cg, pn->getOp()) < 0)
            return JS_FALSE;
        break;

      case TOK_DEBUGGER:
        if (Emit1(cx, cg, JSOP_DEBUGGER) < 0)
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case TOK_XMLELEM:
      case TOK_XMLLIST:
        JS_ASSERT(!cg->inStrictMode());
        JS_ASSERT(pn->isKind(TOK_XMLLIST) || pn->pn_count != 0);

        switch (pn->pn_head ? pn->pn_head->getKind() : TOK_XMLLIST) {
          case TOK_XMLETAGO:
            JS_ASSERT(0);
            
          case TOK_XMLPTAGC:
          case TOK_XMLSTAGO:
            break;
          default:
            if (Emit1(cx, cg, JSOP_STARTXML) < 0)
                return JS_FALSE;
        }

        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(TOK_LC) &&
                Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0) {
                return JS_FALSE;
            }
            if (!EmitTree(cx, cg, pn2))
                return JS_FALSE;
            if (pn2 != pn->pn_head && Emit1(cx, cg, JSOP_ADD) < 0)
                return JS_FALSE;
        }

        if (pn->pn_xflags & PNX_XMLROOT) {
            if (pn->pn_count == 0) {
                JS_ASSERT(pn->isKind(TOK_XMLLIST));
                atom = cx->runtime->atomState.emptyAtom;
                jsatomid index;
                if (!cg->makeAtomIndex(atom, &index))
                    return JS_FALSE;
                EMIT_INDEX_OP(JSOP_STRING, index);
            }
            if (Emit1(cx, cg, pn->getOp()) < 0)
                return JS_FALSE;
        }
#ifdef DEBUG
        else
            JS_ASSERT(pn->pn_count != 0);
#endif
        break;

      case TOK_XMLPTAGC:
      case TOK_XMLSTAGO:
      case TOK_XMLETAGO:
        if (!EmitXMLTag(cx, cg, pn))
            return false;
        break;

      case TOK_XMLNAME:
        JS_ASSERT(!cg->inStrictMode());

        if (pn->isArity(PN_LIST)) {
            JS_ASSERT(pn->pn_count != 0);
            for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
                if (pn2->isKind(TOK_LC) &&
                    Emit1(cx, cg, JSOP_STARTXMLEXPR) < 0) {
                    return JS_FALSE;
                }
                if (!EmitTree(cx, cg, pn2))
                    return JS_FALSE;
                if (pn2 != pn->pn_head && Emit1(cx, cg, JSOP_ADD) < 0)
                    return JS_FALSE;
            }
        } else {
            JS_ASSERT(pn->isArity(PN_NULLARY));
            ok = pn->isOp(JSOP_OBJECT)
                 ? EmitObjectOp(cx, pn->pn_objbox, pn->getOp(), cg)
                 : EmitAtomOp(cx, pn, pn->getOp(), cg);
        }
        break;

      case TOK_XMLPI:
        if (!EmitXMLProcessingInstruction(cx, cg, pn))
            return false;
        break;
#endif 

      default:
        JS_ASSERT(0);
    }

    
    if (ok && cg->emitLevel == 1) {
        if (cg->spanDeps)
            ok = OptimizeSpanDeps(cx, cg);
        if (!UpdateLineNumberNotes(cx, cg, pn->pn_pos.end.lineno))
            return JS_FALSE;
    }

    return ok;
}

static intN
AllocSrcNote(JSContext *cx, CodeGenerator *cg)
{
    jssrcnote *notes = CG_NOTES(cg);
    jssrcnote *newnotes;
    uintN index = CG_NOTE_COUNT(cg);
    uintN max = CG_NOTE_LIMIT(cg);

    if (index == max) {
        size_t newlength;
        if (!notes) {
            JS_ASSERT(!index && !max);
            newlength = SRCNOTE_CHUNK_LENGTH;
            newnotes = (jssrcnote *) cx->malloc_(SRCNOTE_SIZE(newlength));
        } else {
            JS_ASSERT(index <= max);
            newlength = max * 2;
            newnotes = (jssrcnote *) cx->realloc_(notes, SRCNOTE_SIZE(newlength));
        }
        if (!newnotes) {
            js_ReportOutOfMemory(cx);
            return -1;
        }
        CG_NOTES(cg) = newnotes;
        CG_NOTE_LIMIT(cg) = newlength;
    }

    CG_NOTE_COUNT(cg) = index + 1;
    return (intN)index;
}

intN
frontend::NewSrcNote(JSContext *cx, CodeGenerator *cg, SrcNoteType type)
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
        if (NewSrcNote(cx, cg, SRC_NULL) < 0)
            return -1;
    }
    return index;
}

intN
frontend::NewSrcNote2(JSContext *cx, CodeGenerator *cg, SrcNoteType type, ptrdiff_t offset)
{
    intN index;

    index = NewSrcNote(cx, cg, type);
    if (index >= 0) {
        if (!SetSrcNoteOffset(cx, cg, index, 0, offset))
            return -1;
    }
    return index;
}

intN
frontend::NewSrcNote3(JSContext *cx, CodeGenerator *cg, SrcNoteType type, ptrdiff_t offset1,
            ptrdiff_t offset2)
{
    intN index;

    index = NewSrcNote(cx, cg, type);
    if (index >= 0) {
        if (!SetSrcNoteOffset(cx, cg, index, 0, offset1))
            return -1;
        if (!SetSrcNoteOffset(cx, cg, index, 1, offset2))
            return -1;
    }
    return index;
}

static JSBool
GrowSrcNotes(JSContext *cx, CodeGenerator *cg)
{
    size_t newlength = CG_NOTE_LIMIT(cg) * 2;
    jssrcnote *newnotes = (jssrcnote *) cx->realloc_(CG_NOTES(cg), newlength);
    if (!newnotes) {
        js_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    CG_NOTES(cg) = newnotes;
    CG_NOTE_LIMIT(cg) = newlength;
    return JS_TRUE;
}

jssrcnote *
frontend::AddToSrcNoteDelta(JSContext *cx, CodeGenerator *cg, jssrcnote *sn, ptrdiff_t delta)
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
        if (cg->main.noteCount == cg->main.noteLimit) {
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

static JSBool
SetSrcNoteOffset(JSContext *cx, CodeGenerator *cg, uintN index, uintN which, ptrdiff_t offset)
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

            




            if (CG_NOTE_COUNT(cg) + 1 >= CG_NOTE_LIMIT(cg)) {
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

static void
DumpSrcNoteSizeHist()
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
frontend::FinishTakingSrcNotes(JSContext *cx, CodeGenerator *cg, jssrcnote *notes)
{
    uintN prologCount, mainCount, totalCount;
    ptrdiff_t offset, delta;
    jssrcnote *sn;

    JS_ASSERT(cg->current == &cg->main);

    prologCount = cg->prolog.noteCount;
    if (prologCount && cg->prolog.currentLine != cg->firstLine) {
        CG_SWITCH_TO_PROLOG(cg);
        if (NewSrcNote2(cx, cg, SRC_SETLINE, (ptrdiff_t)cg->firstLine) < 0)
            return false;
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
                if (!AddToSrcNoteDelta(cx, cg, sn, delta))
                    return false;
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

    return true;
}

static JSBool
NewTryNote(JSContext *cx, CodeGenerator *cg, JSTryNoteKind kind, uintN stackDepth, size_t start,
           size_t end)
{
    JS_ASSERT((uintN)(uint16)stackDepth == stackDepth);
    JS_ASSERT(start <= end);
    JS_ASSERT((size_t)(uint32)start == start);
    JS_ASSERT((size_t)(uint32)end == end);

    TryNode *tryNode = cx->tempLifoAlloc().new_<TryNode>();
    if (!tryNode) {
        js_ReportOutOfMemory(cx);
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
frontend::FinishTakingTryNotes(CodeGenerator *cg, JSTryNoteArray *array)
{
    TryNode *tryNode;
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
CGObjectList::index(ObjectBox *objbox)
{
    JS_ASSERT(!objbox->emitLink);
    objbox->emitLink = lastbox;
    lastbox = objbox;
    objbox->index = length++;
    return objbox->index;
}

void
CGObjectList::finish(JSObjectArray *array)
{
    JS_ASSERT(length <= INDEX_LIMIT);
    JS_ASSERT(length == array->length);

    JSObject **cursor = array->vector + array->length;
    ObjectBox *objbox = lastbox;
    do {
        --cursor;
        JS_ASSERT(!*cursor);
        *cursor = objbox->object;
    } while ((objbox = objbox->emitLink) != NULL);
    JS_ASSERT(cursor == array->vector);
}

void
GCConstList::finish(JSConstArray *array)
{
    JS_ASSERT(array->length == list.length());
    Value *src = list.begin(), *srcend = list.end();
    Value *dst = array->vector;
    for (; src != srcend; ++src, ++dst)
        *dst = *src;
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
