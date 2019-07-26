










#include "mozilla/FloatingPoint.h"

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <new>
#include <string.h>

#include "jstypes.h"
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
#include "frontend/BytecodeEmitter.h"
#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "vm/RegExpObject.h"

#include "jsatominlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/TreeContext-inl.h"


#define BYTECODE_CHUNK_LENGTH  1024    /* initial bytecode chunk length */
#define SRCNOTE_CHUNK_LENGTH   1024    /* initial srcnote chunk length */


#define BYTECODE_SIZE(n)        ((n) * sizeof(jsbytecode))
#define SRCNOTE_SIZE(n)         ((n) * sizeof(jssrcnote))

using namespace js;
using namespace js::gc;
using namespace js::frontend;

static JSBool
NewTryNote(JSContext *cx, BytecodeEmitter *bce, JSTryNoteKind kind, unsigned stackDepth,
           size_t start, size_t end);

static JSBool
SetSrcNoteOffset(JSContext *cx, BytecodeEmitter *bce, unsigned index, unsigned which, ptrdiff_t offset);

BytecodeEmitter::BytecodeEmitter(Parser *parser, SharedContext *sc, Handle<JSScript*> script,
                                 unsigned lineno)
  : sc(sc),
    parent(NULL),
    script(sc->context, script),
    parser(parser),
    atomIndices(sc->context),
    stackDepth(0), maxStackDepth(0),
    ntrynotes(0), lastTryNode(NULL),
    arrayCompDepth(0),
    emitLevel(0),
    constMap(sc->context),
    constList(sc->context),
    globalScope(NULL),
    closedArgs(sc->context),
    closedVars(sc->context),
    typesetCount(0),
    hasSingletons(false),
    inForInit(false)
{
    memset(&prolog, 0, sizeof prolog);
    memset(&main, 0, sizeof main);
    current = &main;
    firstLine = prolog.currentLine = main.currentLine = lineno;
}

bool
BytecodeEmitter::init()
{
    return constMap.init() && atomIndices.ensureMap(sc->context);
}

BytecodeEmitter::~BytecodeEmitter()
{
    JSContext *cx = sc->context;

    cx->free_(prolog.base);
    cx->free_(prolog.notes);
    cx->free_(main.base);
    cx->free_(main.notes);
}

static ptrdiff_t
EmitCheck(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t delta)
{
    jsbytecode *base = bce->base();
    jsbytecode *newbase;
    jsbytecode *next = bce->next();
    jsbytecode *limit = bce->limit();
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
        bce->current->base = newbase;
        bce->current->limit = newbase + newlength;
        bce->current->next = newbase + offset;
    }
    return offset;
}

static StaticBlockObject &
CurrentBlock(SharedContext *sc)
{
    JS_ASSERT(sc->topStmt->type == STMT_BLOCK || sc->topStmt->type == STMT_SWITCH);
    JS_ASSERT(sc->topStmt->blockObj->isStaticBlock());
    return *sc->topStmt->blockObj;
}

static void
UpdateDepth(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t target)
{
    jsbytecode *pc = bce->code(target);
    JSOp op = (JSOp) *pc;
    const JSCodeSpec *cs = &js_CodeSpec[op];


    if (cs->format & JOF_TMPSLOT_MASK) {
        



        unsigned depth = (unsigned) bce->stackDepth +
                      ((cs->format & JOF_TMPSLOT_MASK) >> JOF_TMPSLOT_SHIFT);
        if (depth > bce->maxStackDepth)
            bce->maxStackDepth = depth;
    }

    




    int nuses, ndefs;
    if (op == JSOP_ENTERBLOCK) {
        nuses = 0;
        ndefs = CurrentBlock(bce->sc).slotCount();
    } else if (op == JSOP_ENTERLET0) {
        nuses = ndefs = CurrentBlock(bce->sc).slotCount();
    } else if (op == JSOP_ENTERLET1) {
        nuses = ndefs = CurrentBlock(bce->sc).slotCount() + 1;
    } else {
        nuses = StackUses(NULL, pc);
        ndefs = StackDefs(NULL, pc);
    }

    bce->stackDepth -= nuses;
    JS_ASSERT(bce->stackDepth >= 0);
    bce->stackDepth += ndefs;
    if ((unsigned)bce->stackDepth > bce->maxStackDepth)
        bce->maxStackDepth = bce->stackDepth;
}

static inline void
UpdateDecomposeLength(BytecodeEmitter *bce, unsigned start)
{
    unsigned end = bce->offset();
    JS_ASSERT(unsigned(end - start) < 256);
    bce->code(start)[-1] = end - start;
}

ptrdiff_t
frontend::Emit1(JSContext *cx, BytecodeEmitter *bce, JSOp op)
{
    ptrdiff_t offset = EmitCheck(cx, bce, 1);

    if (offset >= 0) {
        *bce->current->next++ = (jsbytecode)op;
        UpdateDepth(cx, bce, offset);
    }
    return offset;
}

ptrdiff_t
frontend::Emit2(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1)
{
    ptrdiff_t offset = EmitCheck(cx, bce, 2);

    if (offset >= 0) {
        jsbytecode *next = bce->next();
        next[0] = (jsbytecode)op;
        next[1] = op1;
        bce->current->next = next + 2;
        UpdateDepth(cx, bce, offset);
    }
    return offset;
}

ptrdiff_t
frontend::Emit3(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1,
                    jsbytecode op2)
{
    
    JS_ASSERT(JOF_OPTYPE(op) != JOF_QARG);
    JS_ASSERT(JOF_OPTYPE(op) != JOF_LOCAL);

    ptrdiff_t offset = EmitCheck(cx, bce, 3);

    if (offset >= 0) {
        jsbytecode *next = bce->next();
        next[0] = (jsbytecode)op;
        next[1] = op1;
        next[2] = op2;
        bce->current->next = next + 3;
        UpdateDepth(cx, bce, offset);
    }
    return offset;
}

ptrdiff_t
frontend::EmitN(JSContext *cx, BytecodeEmitter *bce, JSOp op, size_t extra)
{
    ptrdiff_t length = 1 + (ptrdiff_t)extra;
    ptrdiff_t offset = EmitCheck(cx, bce, length);

    if (offset >= 0) {
        jsbytecode *next = bce->next();
        *next = (jsbytecode)op;
        memset(next + 1, 0, BYTECODE_SIZE(extra));
        bce->current->next = next + length;

        



        if (js_CodeSpec[op].nuses >= 0)
            UpdateDepth(cx, bce, offset);
    }
    return offset;
}

static ptrdiff_t
EmitJump(JSContext *cx, BytecodeEmitter *bce, JSOp op, ptrdiff_t off)
{
    ptrdiff_t offset = EmitCheck(cx, bce, 5);

    if (offset >= 0) {
        jsbytecode *next = bce->next();
        next[0] = (jsbytecode)op;
        SET_JUMP_OFFSET(next, off);
        bce->current->next = next + 5;
        UpdateDepth(cx, bce, offset);
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
StatementName(SharedContext *sc)
{
    if (!sc->topStmt)
        return js_script_str;
    return statementName[sc->topStmt->type];
}

static void
ReportStatementTooLarge(JSContext *cx, SharedContext *sc)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET,
                         StatementName(sc));
}





static ptrdiff_t
EmitBackPatchOp(JSContext *cx, BytecodeEmitter *bce, JSOp op, ptrdiff_t *lastp)
{
    ptrdiff_t offset, delta;

    offset = bce->offset();
    delta = offset - *lastp;
    *lastp = offset;
    JS_ASSERT(delta > 0);
    return EmitJump(cx, bce, op, delta);
}


#define UPDATE_LINE_NUMBER_NOTES(cx, bce, line)                               \
    JS_BEGIN_MACRO                                                            \
        unsigned line_ = (line);                                                 \
        unsigned delta_ = line_ - bce->currentLine();                            \
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
            bce->current->currentLine = line_;                                \
            if (delta_ >= (unsigned)(2 + ((line_ > SN_3BYTE_OFFSET_MASK)<<1))) { \
                if (NewSrcNote2(cx, bce, SRC_SETLINE, (ptrdiff_t)line_) < 0)  \
                    return JS_FALSE;                                          \
            } else {                                                          \
                do {                                                          \
                    if (NewSrcNote(cx, bce, SRC_NEWLINE) < 0)                 \
                        return JS_FALSE;                                      \
                } while (--delta_ != 0);                                      \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO


static JSBool
UpdateLineNumberNotes(JSContext *cx, BytecodeEmitter *bce, unsigned line)
{
    UPDATE_LINE_NUMBER_NOTES(cx, bce, line);
    return JS_TRUE;
}

static ptrdiff_t
EmitLoopHead(JSContext *cx, BytecodeEmitter *bce, ParseNode *nextpn)
{
    if (nextpn) {
        




        JS_ASSERT_IF(nextpn->isKind(PNK_STATEMENTLIST), nextpn->isArity(PN_LIST));
        if (nextpn->isKind(PNK_STATEMENTLIST) && nextpn->pn_head)
            nextpn = nextpn->pn_head;
        if (!UpdateLineNumberNotes(cx, bce, nextpn->pn_pos.begin.lineno))
            return -1;
    }

    return Emit1(cx, bce, JSOP_LOOPHEAD);
}

static bool
EmitLoopEntry(JSContext *cx, BytecodeEmitter *bce, ParseNode *nextpn)
{
    if (nextpn) {
        
        JS_ASSERT_IF(nextpn->isKind(PNK_STATEMENTLIST), nextpn->isArity(PN_LIST));
        if (nextpn->isKind(PNK_STATEMENTLIST) && nextpn->pn_head)
            nextpn = nextpn->pn_head;
        if (!UpdateLineNumberNotes(cx, bce, nextpn->pn_pos.begin.lineno))
            return false;
    }

    return Emit1(cx, bce, JSOP_LOOPENTRY) >= 0;
}





static inline void
CheckTypeSet(JSContext *cx, BytecodeEmitter *bce, JSOp op)
{
    if (js_CodeSpec[op].format & JOF_TYPESET) {
        if (bce->typesetCount < UINT16_MAX)
            bce->typesetCount++;
    }
}







#define EMIT_UINT16_IMM_OP(op, i)                                             \
    JS_BEGIN_MACRO                                                            \
        if (Emit3(cx, bce, op, UINT16_HI(i), UINT16_LO(i)) < 0)               \
            return JS_FALSE;                                                  \
        CheckTypeSet(cx, bce, op);                                            \
    JS_END_MACRO

#define EMIT_UINT16PAIR_IMM_OP(op, i, j)                                      \
    JS_BEGIN_MACRO                                                            \
        ptrdiff_t off_ = EmitN(cx, bce, op, 2 * UINT16_LEN);                  \
        if (off_ < 0)                                                         \
            return JS_FALSE;                                                  \
        jsbytecode *pc_ = bce->code(off_);                                    \
        SET_UINT16(pc_, i);                                                   \
        pc_ += UINT16_LEN;                                                    \
        SET_UINT16(pc_, j);                                                   \
    JS_END_MACRO

#define EMIT_UINT16_IN_PLACE(offset, op, i)                                   \
    JS_BEGIN_MACRO                                                            \
        bce->code(offset)[0] = op;                                            \
        bce->code(offset)[1] = UINT16_HI(i);                                  \
        bce->code(offset)[2] = UINT16_LO(i);                                  \
    JS_END_MACRO

#define EMIT_UINT32_IN_PLACE(offset, op, i)                                   \
    JS_BEGIN_MACRO                                                            \
        bce->code(offset)[0] = op;                                            \
        bce->code(offset)[1] = jsbytecode(i >> 24);                           \
        bce->code(offset)[2] = jsbytecode(i >> 16);                           \
        bce->code(offset)[3] = jsbytecode(i >> 8);                            \
        bce->code(offset)[4] = jsbytecode(i);                                 \
    JS_END_MACRO

static JSBool
FlushPops(JSContext *cx, BytecodeEmitter *bce, int *npops)
{
    JS_ASSERT(*npops != 0);
    if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
        return JS_FALSE;
    EMIT_UINT16_IMM_OP(JSOP_POPN, *npops);
    *npops = 0;
    return JS_TRUE;
}

static bool
PopIterator(JSContext *cx, BytecodeEmitter *bce)
{
    if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
        return false;
    if (Emit1(cx, bce, JSOP_ENDITER) < 0)
        return false;
    return true;
}




static JSBool
EmitNonLocalJumpFixup(JSContext *cx, BytecodeEmitter *bce, StmtInfo *toStmt)
{
    





    int depth = bce->stackDepth;
    int npops = 0;

#define FLUSH_POPS() if (npops && !FlushPops(cx, bce, &npops)) return JS_FALSE

    for (StmtInfo *stmt = bce->sc->topStmt; stmt != toStmt; stmt = stmt->down) {
        switch (stmt->type) {
          case STMT_FINALLY:
            FLUSH_POPS();
            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (EmitBackPatchOp(cx, bce, JSOP_BACKPATCH, &GOSUBS(*stmt)) < 0)
                return JS_FALSE;
            break;

          case STMT_WITH:
            
            FLUSH_POPS();
            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return JS_FALSE;
            if (Emit1(cx, bce, JSOP_LEAVEWITH) < 0)
                return JS_FALSE;
            break;

          case STMT_FOR_IN_LOOP:
            FLUSH_POPS();
            if (!PopIterator(cx, bce))
                return JS_FALSE;
            break;

          case STMT_SUBROUTINE:
            



            npops += 2;
            break;

          default:;
        }

        if (stmt->flags & SIF_SCOPE) {
            FLUSH_POPS();
            unsigned blockObjCount = stmt->blockObj->slotCount();
            if (stmt->flags & SIF_FOR_BLOCK) {
                




                JS_ASSERT(stmt->down->type == STMT_FOR_IN_LOOP);
                stmt = stmt->down;
                if (stmt == toStmt)
                    break;
                if (Emit1(cx, bce, JSOP_LEAVEFORLETIN) < 0)
                    return JS_FALSE;
                if (!PopIterator(cx, bce))
                    return JS_FALSE;
                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                    return JS_FALSE;
                EMIT_UINT16_IMM_OP(JSOP_POPN, blockObjCount);
            } else {
                
                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                    return JS_FALSE;
                EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, blockObjCount);
            }
        }
    }

    FLUSH_POPS();
    bce->stackDepth = depth;
    return JS_TRUE;

#undef FLUSH_POPS
}

static const jsatomid INVALID_ATOMID = -1;

static ptrdiff_t
EmitGoto(JSContext *cx, BytecodeEmitter *bce, StmtInfo *toStmt, ptrdiff_t *lastp,
         jsatomid labelIndex = INVALID_ATOMID, SrcNoteType noteType = SRC_NULL)
{
    int index;

    if (!EmitNonLocalJumpFixup(cx, bce, toStmt))
        return -1;

    if (labelIndex != INVALID_ATOMID)
        index = NewSrcNote2(cx, bce, noteType, ptrdiff_t(labelIndex));
    else if (noteType != SRC_NULL)
        index = NewSrcNote(cx, bce, noteType);
    else
        index = 0;
    if (index < 0)
        return -1;

    return EmitBackPatchOp(cx, bce, JSOP_BACKPATCH, lastp);
}

static JSBool
BackPatch(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t last, jsbytecode *target, jsbytecode op)
{
    jsbytecode *pc, *stop;
    ptrdiff_t delta, span;

    pc = bce->code(last);
    stop = bce->code(-1);
    while (pc != stop) {
        delta = GET_JUMP_OFFSET(pc);
        span = target - pc;
        SET_JUMP_OFFSET(pc, span);
        *pc = op;
        pc -= delta;
    }
    return JS_TRUE;
}

JSBool
frontend::PopStatementBCE(JSContext *cx, BytecodeEmitter *bce)
{
    StmtInfo *stmt = bce->sc->topStmt;
    if (!STMT_IS_TRYING(stmt) &&
        (!BackPatch(cx, bce, stmt->breaks, bce->next(), JSOP_GOTO) ||
         !BackPatch(cx, bce, stmt->continues, bce->code(stmt->update), JSOP_GOTO)))
    {
        return JS_FALSE;
    }
    PopStatementSC(bce->sc);
    return JS_TRUE;
}

JSBool
frontend::DefineCompileTimeConstant(JSContext *cx, BytecodeEmitter *bce, JSAtom *atom, ParseNode *pn)
{
    
    if (pn->isKind(PNK_NUMBER)) {
        if (!bce->constMap.put(atom, NumberValue(pn->pn_dval)))
            return JS_FALSE;
    }
    return JS_TRUE;
}





static JSBool
LookupCompileTimeConstant(JSContext *cx, BytecodeEmitter *bce, JSAtom *atom, Value *constp)
{
    




    constp->setMagic(JS_NO_CONSTANT);
    do {
        if (bce->sc->inFunction() || bce->script->compileAndGo) {
            
            StmtInfo *stmt = LexicalLookup(bce->sc, atom, NULL);
            if (stmt)
                return JS_TRUE;

            if (BytecodeEmitter::ConstMap::Ptr p = bce->constMap.lookup(atom)) {
                JS_ASSERT(!p->value.isMagic(JS_NO_CONSTANT));
                *constp = p->value;
                return JS_TRUE;
            }

            






            if (bce->sc->inFunction()) {
                if (bce->sc->bindings.hasBinding(cx, atom))
                    break;
            } else {
                JS_ASSERT(bce->script->compileAndGo);
                JSObject *obj = bce->sc->scopeChain();

                const Shape *shape = obj->nativeLookup(cx, AtomToId(atom));
                if (shape) {
                    





                    if (!shape->writable() && !shape->configurable() &&
                        shape->hasDefaultGetter() && shape->hasSlot()) {
                        *constp = obj->nativeGetSlot(shape->slot());
                    }
                }

                if (shape)
                    break;
            }
        }
        bce = bce->parent;
    } while (bce);

    return JS_TRUE;
}

static bool
EmitIndex32(JSContext *cx, JSOp op, uint32_t index, BytecodeEmitter *bce)
{
    const size_t len = 1 + UINT32_INDEX_LEN;
    JS_ASSERT(len == size_t(js_CodeSpec[op].length));
    ptrdiff_t offset = EmitCheck(cx, bce, len);
    if (offset < 0)
        return false;

    jsbytecode *next = bce->next();
    next[0] = jsbytecode(op);
    SET_UINT32_INDEX(next, index);
    bce->current->next = next + len;
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, op);
    return true;
}

static bool
EmitIndexOp(JSContext *cx, JSOp op, uint32_t index, BytecodeEmitter *bce)
{
    const size_t len = js_CodeSpec[op].length;
    JS_ASSERT(len >= 1 + UINT32_INDEX_LEN);
    ptrdiff_t offset = EmitCheck(cx, bce, len);
    if (offset < 0)
        return false;

    jsbytecode *next = bce->next();
    next[0] = jsbytecode(op);
    SET_UINT32_INDEX(next, index);
    bce->current->next = next + len;
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, op);
    return true;
}

static bool
EmitAtomOp(JSContext *cx, JSAtom *atom, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

    if (op == JSOP_GETPROP && atom == cx->runtime->atomState.lengthAtom) {
        
        op = JSOP_LENGTH;
    }

    jsatomid index;
    if (!bce->makeAtomIndex(atom, &index))
        return false;

    return EmitIndexOp(cx, op, index, bce);
}

static bool
EmitAtomOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(pn->pn_atom != NULL);
    return EmitAtomOp(cx, pn->pn_atom, op, bce);
}

static bool
EmitAtomIncDec(JSContext *cx, JSAtom *atom, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
    JS_ASSERT(js_CodeSpec[op].format & (JOF_INC | JOF_DEC));

    jsatomid index;
    if (!bce->makeAtomIndex(atom, &index))
        return false;

    const size_t len = 1 + UINT32_INDEX_LEN + 1;
    JS_ASSERT(size_t(js_CodeSpec[op].length) == len);
    ptrdiff_t offset = EmitCheck(cx, bce, len);
    if (offset < 0)
        return false;

    jsbytecode *next = bce->next();
    next[0] = jsbytecode(op);
    SET_UINT32_INDEX(next, index);
    bce->current->next = next + len;
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, op);
    return true;
}

static bool
EmitFunctionOp(JSContext *cx, JSOp op, uint32_t index, BytecodeEmitter *bce)
{
    return EmitIndex32(cx, op, index, bce);
}

static bool
EmitObjectOp(JSContext *cx, ObjectBox *objbox, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_OBJECT);
    return EmitIndex32(cx, op, bce->objectList.add(objbox), bce);
}

static bool
EmitRegExp(JSContext *cx, uint32_t index, BytecodeEmitter *bce)
{
    return EmitIndex32(cx, JSOP_REGEXP, index, bce);
}








static bool
EmitUnaliasedVarOp(JSContext *cx, JSOp op, uint16_t slot, BytecodeEmitter *bce)
{
    JS_ASSERT(JOF_OPTYPE(op) != JOF_SCOPECOORD);
    ptrdiff_t off = EmitN(cx, bce, op, sizeof(uint16_t));
    if (off < 0)
        return false;
    SET_UINT16(bce->code(off), slot);
    return true;
}

static bool
EmitAliasedVarOp(JSContext *cx, JSOp op, ScopeCoordinate sc, BytecodeEmitter *bce)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_SCOPECOORD);

    uint32_t maybeBlockIndex = UINT32_MAX;
    if (bce->sc->blockChain)
        maybeBlockIndex = bce->objectList.indexOf(bce->sc->blockChain);

    bool decomposed = js_CodeSpec[op].format & JOF_DECOMPOSE;
    unsigned n = 2 * sizeof(uint16_t) + sizeof(uint32_t) + (decomposed ? 1 : 0);
    JS_ASSERT(int(n) + 1  == js_CodeSpec[op].length);

    ptrdiff_t off = EmitN(cx, bce, op, n);
    if (off < 0)
        return false;

    jsbytecode *pc = bce->code(off);
    SET_UINT16(pc, sc.hops);
    pc += sizeof(uint16_t);
    SET_UINT16(pc, sc.slot);
    pc += sizeof(uint16_t);
    SET_UINT32_INDEX(pc, maybeBlockIndex);
    return true;
}

static unsigned
ClonedBlockDepth(BytecodeEmitter *bce)
{
    unsigned clonedBlockDepth = 0;
    for (StaticBlockObject *b = bce->sc->blockChain; b; b = b->enclosingBlock()) {
        if (b->needsClone())
            ++clonedBlockDepth;
    }

    return clonedBlockDepth;
}

static bool
EmitAliasedVarOp(JSContext *cx, JSOp op, ParseNode *pn, BytecodeEmitter *bce)
{
    







    ScopeCoordinate sc;
    if (JOF_OPTYPE(pn->getOp()) == JOF_QARG) {
        JS_ASSERT(bce->sc->funIsHeavyweight());
        sc.hops = ClonedBlockDepth(bce);
        sc.slot = bce->sc->bindings.argToSlot(pn->pn_cookie.slot());
    } else {
        JS_ASSERT(JOF_OPTYPE(pn->getOp()) == JOF_LOCAL || pn->isKind(PNK_FUNCTION));
        unsigned local = pn->pn_cookie.slot();
        if (local < bce->sc->bindings.numVars()) {
            sc.hops = ClonedBlockDepth(bce);
            sc.slot = bce->sc->bindings.localToSlot(local);
        } else {
            unsigned depth = local - bce->sc->bindings.numVars();
            unsigned hops = 0;
            StaticBlockObject *b = bce->sc->blockChain;
            while (!b->containsVarAtDepth(depth)) {
                if (b->needsClone())
                    hops++;
                b = b->enclosingBlock();
            }
            sc.hops = hops;
            sc.slot = depth - b->stackDepth();
        }
    }

    return EmitAliasedVarOp(cx, op, sc, bce);
}

static bool
EmitVarOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(pn->isKind(PNK_FUNCTION) || pn->isKind(PNK_NAME));
    JS_ASSERT_IF(pn->isKind(PNK_NAME), JOF_OPTYPE(op) == JOF_QARG || JOF_OPTYPE(op) == JOF_LOCAL);
    JS_ASSERT(!pn->pn_cookie.isFree());

    if (!bce->isAliasedName(pn))
        return EmitUnaliasedVarOp(cx, op, pn->pn_cookie.slot(), bce);

    switch (op) {
      case JSOP_GETARG: case JSOP_GETLOCAL: op = JSOP_GETALIASEDVAR; break;
      case JSOP_SETARG: case JSOP_SETLOCAL: op = JSOP_SETALIASEDVAR; break;
      case JSOP_CALLARG: case JSOP_CALLLOCAL: op = JSOP_CALLALIASEDVAR; break;
      default: JS_NOT_REACHED("unexpected var op");
    }

    return EmitAliasedVarOp(cx, op, pn, bce);
}

static bool
EmitVarIncDec(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(pn->isKind(PNK_NAME));
    JS_ASSERT(JOF_OPTYPE(op) == JOF_QARG || JOF_OPTYPE(op) == JOF_LOCAL);
    JS_ASSERT(js_CodeSpec[op].format & (JOF_INC | JOF_DEC));
    JS_ASSERT(!pn->pn_cookie.isFree());

    if (!bce->isAliasedName(pn))
        return EmitUnaliasedVarOp(cx, op, pn->pn_cookie.slot(), bce);

    switch (op) {
      case JSOP_INCARG: case JSOP_INCLOCAL: op = JSOP_INCALIASEDVAR; break;
      case JSOP_ARGINC: case JSOP_LOCALINC: op = JSOP_ALIASEDVARINC; break;
      case JSOP_DECARG: case JSOP_DECLOCAL: op = JSOP_DECALIASEDVAR; break;
      case JSOP_ARGDEC: case JSOP_LOCALDEC: op = JSOP_ALIASEDVARDEC; break;
      default: JS_NOT_REACHED("unexpected var op");
    }

    if (!EmitAliasedVarOp(cx, op, pn, bce))
        return false;

    
    bce->stackDepth--;

    int start = bce->offset();

    const JSCodeSpec &cs = js_CodeSpec[op];
    bool post = (cs.format & JOF_POST);
    JSOp binop = (cs.format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

    if (!EmitAliasedVarOp(cx, JSOP_GETALIASEDVAR, pn, bce))  
        return false;
    if (Emit1(cx, bce, JSOP_POS) < 0)                        
        return false;
    if (post && Emit1(cx, bce, JSOP_DUP) < 0)                
        return false;
    if (Emit1(cx, bce, JSOP_ONE) < 0)                        
        return false;
    if (Emit1(cx, bce, binop) < 0)                           
        return false;
    if (!EmitAliasedVarOp(cx, JSOP_SETALIASEDVAR, pn, bce))  
        return false;
    if (post && Emit1(cx, bce, JSOP_POP) < 0)                
        return false;

    UpdateDecomposeLength(bce, start);
    return true;
}

bool
BytecodeEmitter::isAliasedName(ParseNode *pn)
{
    return sc->bindingsAccessedDynamically() || shouldNoteClosedName(pn->resolve());
}

bool
BytecodeEmitter::shouldNoteClosedName(ParseNode *pn)
{
    return !sc->bindingsAccessedDynamically() && pn->isDefn() && pn->isClosed();
}

bool
BytecodeEmitter::noteClosedVar(ParseNode *pn)
{
#ifdef DEBUG
    JS_ASSERT(shouldNoteClosedName(pn));
    Definition *dn = (Definition *)pn;
    JS_ASSERT(dn->kind() == Definition::VAR || dn->kind() == Definition::CONST ||
              dn->kind() == Definition::FUNCTION);
    JS_ASSERT(pn->pn_cookie.slot() < sc->bindings.numVars());
    for (size_t i = 0; i < closedVars.length(); ++i)
        JS_ASSERT(closedVars[i] != pn->pn_cookie.slot());
#endif
    sc->setFunIsHeavyweight();
    return closedVars.append(pn->pn_cookie.slot());
}

bool
BytecodeEmitter::noteClosedArg(ParseNode *pn)
{
#ifdef DEBUG
    JS_ASSERT(shouldNoteClosedName(pn));
    Definition *dn = (Definition *)pn;
    JS_ASSERT(dn->kind() == Definition::ARG);
    JS_ASSERT(pn->pn_cookie.slot() < sc->bindings.numArgs());
    for (size_t i = 0; i < closedArgs.length(); ++i)
        JS_ASSERT(closedArgs[i] != pn->pn_cookie.slot());
#endif
    sc->setFunIsHeavyweight();
    return closedArgs.append(pn->pn_cookie.slot());
}









static int
AdjustBlockSlot(JSContext *cx, BytecodeEmitter *bce, int slot)
{
    JS_ASSERT((unsigned) slot < bce->maxStackDepth);
    if (bce->sc->inFunction()) {
        slot += bce->sc->bindings.numVars();
        if ((unsigned) slot >= SLOTNO_LIMIT) {
            ReportCompileErrorNumber(cx, bce->tokenStream(), NULL, JSREPORT_ERROR,
                                     JSMSG_TOO_MANY_LOCALS);
            slot = -1;
        }
    }
    return slot;
}

static bool
EmitEnterBlock(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, JSOp op)
{
    JS_ASSERT(pn->isKind(PNK_LEXICALSCOPE));
    if (!EmitObjectOp(cx, pn->pn_objbox, op, bce))
        return false;

    Rooted<StaticBlockObject*> blockObj(cx, &pn->pn_objbox->object->asStaticBlock());

    int depth = bce->stackDepth -
                (blockObj->slotCount() + ((op == JSOP_ENTERLET1) ? 1 : 0));
    JS_ASSERT(depth >= 0);

    blockObj->setStackDepth(depth);

    int depthPlusFixed = AdjustBlockSlot(cx, bce, depth);
    if (depthPlusFixed < 0)
        return false;

    for (unsigned i = 0; i < blockObj->slotCount(); i++) {
        Definition *dn = blockObj->maybeDefinitionParseNode(i);

        
        if (!dn) {
            blockObj->setAliased(i, bce->sc->bindingsAccessedDynamically());
            continue;
        }

        JS_ASSERT(dn->isDefn());
        JS_ASSERT(unsigned(dn->frameSlot() + depthPlusFixed) < JS_BIT(16));
        if (!dn->pn_cookie.set(cx, dn->pn_cookie.level(),
                               uint16_t(dn->frameSlot() + depthPlusFixed)))
            return false;

#ifdef DEBUG
        for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
            JS_ASSERT(pnu->pn_lexdef == dn);
            JS_ASSERT(!(pnu->pn_dflags & PND_BOUND));
            JS_ASSERT(pnu->pn_cookie.isFree());
        }
#endif

        bool aliased = bce->sc->bindingsAccessedDynamically() || bce->shouldNoteClosedName(dn);
        blockObj->setAliased(i, aliased);
    }

    




    if (bce->sc->funHasExtensibleScope() ||
        bce->sc->bindings.extensibleParents()) {
        Shape *newShape = Shape::setExtensibleParents(cx, blockObj->lastProperty());
        if (!newShape)
            return false;
        blockObj->setLastPropertyInfallible(newShape);
    }

    return true;
}






















static bool
TryConvertToGname(BytecodeEmitter *bce, ParseNode *pn, JSOp *op)
{
    if (bce->script->compileAndGo &&
        bce->globalScope->globalObj &&
        !bce->sc->funMightAliasLocals() &&
        !pn->isDeoptimized() &&
        !bce->sc->inStrictMode()) {
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
BindNameToSlot(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    Definition *dn;
    JSOp op;
    Definition::Kind dn_kind;

    JS_ASSERT(pn->isKind(PNK_NAME));

    
    if (pn->pn_dflags & PND_BOUND)
        return true;

    
    JS_ASSERT(!pn->isOp(JSOP_CALLEE));

    



    if (pn->isUsed()) {
        JS_ASSERT(pn->pn_cookie.isFree());
        dn = pn->pn_lexdef;
        JS_ASSERT(dn->isDefn());
        if (pn->isDeoptimized())
            return true;
        pn->pn_dflags |= (dn->pn_dflags & PND_CONST);
    } else {
        if (!pn->isDefn())
            return true;
        dn = (Definition *) pn;
    }

    op = pn->getOp();
    if (op == JSOP_NOP)
        return true;

    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
    RootedAtom atom(cx, pn->pn_atom);
    UpvarCookie cookie = dn->pn_cookie;
    dn_kind = dn->kind();

    








    switch (op) {
      case JSOP_NAME:
      case JSOP_SETCONST:
        break;
      case JSOP_DELNAME:
        if (dn_kind != Definition::UNKNOWN) {
            if (bce->parser->callerFrame && dn->isTopLevel())
                JS_ASSERT(bce->script->compileAndGo);
            else
                pn->setOp(JSOP_FALSE);
            pn->pn_dflags |= PND_BOUND;
            return true;
        }
        break;
      default:
        if (pn->isConst()) {
            if (bce->sc->needStrictChecks()) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(cx, atom, &name) ||
                    !ReportStrictModeError(cx, bce->tokenStream(), pn, JSMSG_READ_ONLY, name.ptr()))
                {
                    return false;
                }
            }
            pn->setOp(op = JSOP_NAME);
        }
    }

    if (cookie.isFree()) {
        StackFrame *caller = bce->parser->callerFrame;
        if (caller) {
            JS_ASSERT(bce->script->compileAndGo);

            



            if (bce->inForInit)
                return true;

            JS_ASSERT(caller->isScriptFrame());

            



            if (caller->isGlobalFrame() && TryConvertToGname(bce, pn, &op)) {
                pn->setOp(op);
                pn->pn_dflags |= PND_BOUND;
                return true;
            }

            



            return true;
        }

        
        if (!TryConvertToGname(bce, pn, &op))
            return true;

        pn->setOp(op);
        pn->pn_dflags |= PND_BOUND;

        return true;
    }

    uint16_t level = cookie.level();
    JS_ASSERT(bce->script->staticLevel >= level);

    const unsigned skip = bce->script->staticLevel - level;
    if (skip != 0)
        return true;

    




    switch (dn_kind) {
      case Definition::UNKNOWN:
        return true;

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
            JS_ASSERT((bce->sc->fun()->flags & JSFUN_LAMBDA) && atom == bce->sc->fun()->atom);

            























            JS_ASSERT(op != JSOP_DELNAME);
            if (!bce->sc->funIsHeavyweight()) {
                op = JSOP_CALLEE;
                pn->pn_dflags |= PND_CONST;
            }

            pn->setOp(op);
            pn->pn_dflags |= PND_BOUND;
            return true;
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
    if (!pn->pn_cookie.set(bce->sc->context, 0, cookie.slot()))
        return false;
    pn->pn_dflags |= PND_BOUND;
    return true;
}













static JSBool
CheckSideEffects(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, JSBool *answer)
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
                ok &= CheckSideEffects(cx, bce, pn2, answer);
        } else {
            













            *answer = JS_TRUE;
        }
        break;

      case PN_TERNARY:
        ok = CheckSideEffects(cx, bce, pn->pn_kid1, answer) &&
             CheckSideEffects(cx, bce, pn->pn_kid2, answer) &&
             CheckSideEffects(cx, bce, pn->pn_kid3, answer);
        break;

      case PN_BINARY:
        if (pn->isAssignment()) {
            








            pn2 = pn->pn_left;
            if (!pn2->isKind(PNK_NAME)) {
                *answer = JS_TRUE;
            } else {
                if (!BindNameToSlot(cx, bce, pn2))
                    return JS_FALSE;
                if (!CheckSideEffects(cx, bce, pn->pn_right, answer))
                    return JS_FALSE;
                if (!*answer && (!pn->isOp(JSOP_NOP) || !pn2->isConst()))
                    *answer = JS_TRUE;
            }
        } else {
            if (pn->isOp(JSOP_OR) || pn->isOp(JSOP_AND) || pn->isOp(JSOP_STRICTEQ) ||
                pn->isOp(JSOP_STRICTNE)) {
                



                ok = CheckSideEffects(cx, bce, pn->pn_left, answer) &&
                     CheckSideEffects(cx, bce, pn->pn_right, answer);
            } else {
                



                *answer = JS_TRUE;
            }
        }
        break;

      case PN_UNARY:
        switch (pn->getKind()) {
          case PNK_DELETE:
            pn2 = pn->pn_kid;
            switch (pn2->getKind()) {
              case PNK_NAME:
                if (!BindNameToSlot(cx, bce, pn2))
                    return JS_FALSE;
                if (pn2->isConst()) {
                    *answer = JS_FALSE;
                    break;
                }
                
              case PNK_DOT:
#if JS_HAS_XML_SUPPORT
              case PNK_DBLDOT:
                JS_ASSERT_IF(pn2->getKind() == PNK_DBLDOT, !bce->sc->inStrictMode());
                

#endif
              case PNK_LP:
              case PNK_LB:
                
                *answer = JS_TRUE;
                break;
              default:
                ok = CheckSideEffects(cx, bce, pn2, answer);
                break;
            }
            break;

          case PNK_TYPEOF:
          case PNK_VOID:
          case PNK_NOT:
          case PNK_BITNOT:
            if (pn->isOp(JSOP_NOT)) {
                
                ok = CheckSideEffects(cx, bce, pn->pn_kid, answer);
                break;
            }
            

          default:
            





            *answer = JS_TRUE;
            break;
        }
        break;

      case PN_NAME:
        




        if (pn->isKind(PNK_NAME) && !pn->isOp(JSOP_NOP)) {
            if (!BindNameToSlot(cx, bce, pn))
                return JS_FALSE;
            if (!pn->isOp(JSOP_CALLEE) && pn->pn_cookie.isFree()) {
                




                *answer = JS_TRUE;
            }
        }
        if (pn->isKind(PNK_DOT)) {
            
            *answer = JS_TRUE;
        }
        ok = CheckSideEffects(cx, bce, pn->maybeExpr(), answer);
        break;

      case PN_NAMESET:
        ok = CheckSideEffects(cx, bce, pn->pn_tree, answer);
        break;

      case PN_NULLARY:
        if (pn->isKind(PNK_DEBUGGER))
            *answer = JS_TRUE;
        break;
    }
    return ok;
}

bool
BytecodeEmitter::needsImplicitThis()
{
    if (!script->compileAndGo)
        return true;

    if (sc->inFunction()) {
        for (const FunctionBox *funbox = this->sc->funbox(); funbox; funbox = funbox->parent) {
            if (funbox->inWith)
                return true;
        }
    } else {
        JSObject *scope = sc->scopeChain();
        while (scope) {
            if (scope->isWith())
                return true;
            scope = scope->enclosingScope();
        }
    }

    for (StmtInfo *stmt = sc->topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == STMT_WITH)
            return true;
    }
    return false;
}

static JSBool
EmitNameOp(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, JSBool callContext)
{
    JSOp op;

    if (!BindNameToSlot(cx, bce, pn))
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
          default:
            JS_ASSERT(op == JSOP_CALLEE);
            break;
        }
    }

    if (op == JSOP_CALLEE) {
        if (Emit1(cx, bce, op) < 0)
            return false;
    } else {
        if (!pn->pn_cookie.isFree()) {
            JS_ASSERT(JOF_OPTYPE(op) != JOF_ATOM);
            if (!EmitVarOp(cx, pn, op, bce))
                return false;
        } else {
            if (!EmitAtomOp(cx, pn, op, bce))
                return false;
        }
    }

    
    if (callContext) {
        if (op == JSOP_CALLNAME && bce->needsImplicitThis()) {
            if (!EmitAtomOp(cx, pn, JSOP_IMPLICITTHIS, bce))
                return false;
        } else {
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return false;
        }
        if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
            return false;
    }

    return JS_TRUE;
}

#if JS_HAS_XML_SUPPORT
static bool
EmitXMLName(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(!bce->sc->inStrictMode());
    JS_ASSERT(pn->isKind(PNK_XMLUNARY));
    JS_ASSERT(pn->isOp(JSOP_XMLNAME));
    JS_ASSERT(op == JSOP_XMLNAME || op == JSOP_CALLXMLNAME);

    ParseNode *pn2 = pn->pn_kid;
    bool oldInForInit = bce->inForInit;
    bce->inForInit = false;
    if (!EmitTree(cx, bce, pn2))
        return false;
    bce->inForInit = oldInForInit;
    if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - pn2->pn_offset) < 0)
        return false;

    if (Emit1(cx, bce, op) < 0)
        return false;

    return true;
}
#endif

static inline bool
EmitElemOpBase(JSContext *cx, BytecodeEmitter *bce, JSOp op)
{
    if (Emit1(cx, bce, op) < 0)
        return false;
    CheckTypeSet(cx, bce, op);

    if (op == JSOP_CALLELEM) {
        if (Emit1(cx, bce, JSOP_SWAP) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
            return false;
    }
    return true;
}

static bool
EmitSpecialPropOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    




    if (op == JSOP_CALLELEM && Emit1(cx, bce, JSOP_DUP) < 0)
        return false;

    jsatomid index;
    if (!bce->makeAtomIndex(pn->pn_atom, &index))
        return false;
    if (!EmitIndex32(cx, JSOP_QNAMEPART, index, bce))
        return false;

    return EmitElemOpBase(cx, bce, op);
}

static bool
EmitPropOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce,
           JSBool callContext)
{
    ParseNode *pn2, *pndot, *pnup, *pndown;
    ptrdiff_t top;

    JS_ASSERT(pn->isArity(PN_NAME));
    pn2 = pn->maybeExpr();

    
    if ((op == JSOP_GETPROP || op == JSOP_CALLPROP) &&
        pn->pn_atom == cx->runtime->atomState.protoAtom) {
        if (pn2 && !EmitTree(cx, bce, pn2))
            return false;
        return EmitSpecialPropOp(cx, pn, callContext ? JSOP_CALLELEM : JSOP_GETELEM, bce);
    }

    if (callContext) {
        JS_ASSERT(pn->isKind(PNK_DOT));
        JS_ASSERT(op == JSOP_GETPROP);
        op = JSOP_CALLPROP;
    } else if (op == JSOP_GETPROP && pn->isKind(PNK_DOT)) {
        if (pn2->isKind(PNK_NAME)) {
            if (!BindNameToSlot(cx, bce, pn2))
                return false;
        }
    }

    




    if (pn2->isKind(PNK_DOT)) {
        pndot = pn2;
        pnup = NULL;
        top = bce->offset();
        for (;;) {
            
            pndot->pn_offset = top;
            JS_ASSERT(!pndot->isUsed());
            pndown = pndot->pn_expr;
            pndot->pn_expr = pnup;
            if (!pndown->isKind(PNK_DOT))
                break;
            pnup = pndot;
            pndot = pndown;
        }

        
        if (!EmitTree(cx, bce, pndown))
            return false;

        do {
            
            if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - pndown->pn_offset) < 0)
                return false;

            
            if (pndot->isArity(PN_NAME) && pndot->pn_atom == cx->runtime->atomState.protoAtom) {
                if (!EmitSpecialPropOp(cx, pndot, JSOP_GETELEM, bce))
                    return false;
            } else if (!EmitAtomOp(cx, pndot, pndot->getOp(), bce)) {
                return false;
            }

            
            pnup = pndot->pn_expr;
            pndot->pn_expr = pndown;
            pndown = pndot;
        } while ((pndot = pnup) != NULL);
    } else {
        if (!EmitTree(cx, bce, pn2))
            return false;
    }

    if (op == JSOP_CALLPROP && Emit1(cx, bce, JSOP_DUP) < 0)
        return false;

    if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - pn2->pn_offset) < 0)
        return false;

    if (!EmitAtomOp(cx, pn, op, bce))
        return false;

    if (op == JSOP_CALLPROP && Emit1(cx, bce, JSOP_SWAP) < 0)
        return false;

    if (op == JSOP_CALLPROP && Emit1(cx, bce, JSOP_NOTEARG) < 0)
        return false;

    return true;
}

static bool
EmitPropIncDec(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    if (!EmitPropOp(cx, pn, op, bce, false))
        return false;

    



    int start = bce->offset();

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT(cs->format & JOF_PROP);
    JS_ASSERT(cs->format & (JOF_INC | JOF_DEC));

    bool post = (cs->format & JOF_POST);
    JSOp binop = (cs->format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

                                                    
    if (Emit1(cx, bce, JSOP_DUP) < 0)               
        return false;
    if (!EmitAtomOp(cx, pn, JSOP_GETPROP, bce))     
        return false;
    if (Emit1(cx, bce, JSOP_POS) < 0)               
        return false;
    if (post && Emit1(cx, bce, JSOP_DUP) < 0)       
        return false;
    if (Emit1(cx, bce, JSOP_ONE) < 0)               
        return false;
    if (Emit1(cx, bce, binop) < 0)                  
        return false;

    if (post) {
        if (Emit2(cx, bce, JSOP_PICK, (jsbytecode)2) < 0)   
            return false;
        if (Emit1(cx, bce, JSOP_SWAP) < 0)                  
            return false;
    }

    if (!EmitAtomOp(cx, pn, JSOP_SETPROP, bce))     
        return false;
    if (post && Emit1(cx, bce, JSOP_POP) < 0)       
        return false;

    UpdateDecomposeLength(bce, start);

    return true;
}

static bool
EmitNameIncDec(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    
    if (!EmitAtomIncDec(cx, pn->pn_atom, op, bce))
        return false;

    
    bce->stackDepth--;

    int start = bce->offset();

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT((cs->format & JOF_NAME) || (cs->format & JOF_GNAME));
    JS_ASSERT(cs->format & (JOF_INC | JOF_DEC));

    bool global = (cs->format & JOF_GNAME);
    bool post = (cs->format & JOF_POST);
    JSOp binop = (cs->format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

    if (!EmitAtomOp(cx, pn, global ? JSOP_BINDGNAME : JSOP_BINDNAME, bce))  
        return false;
    if (!EmitAtomOp(cx, pn, global ? JSOP_GETGNAME : JSOP_NAME, bce))       
        return false;
    if (Emit1(cx, bce, JSOP_POS) < 0)               
        return false;
    if (post && Emit1(cx, bce, JSOP_DUP) < 0)       
        return false;
    if (Emit1(cx, bce, JSOP_ONE) < 0)               
        return false;
    if (Emit1(cx, bce, binop) < 0)                  
        return false;

    if (post) {
        if (Emit2(cx, bce, JSOP_PICK, (jsbytecode)2) < 0)   
            return false;
        if (Emit1(cx, bce, JSOP_SWAP) < 0)                  
            return false;
    }

    if (!EmitAtomOp(cx, pn, global ? JSOP_SETGNAME : JSOP_SETNAME, bce))    
        return false;
    if (post && Emit1(cx, bce, JSOP_POP) < 0)       
        return false;

    UpdateDecomposeLength(bce, start);

    return true;
}

static JSBool
EmitElemOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    ParseNode *left, *right;

    ptrdiff_t top = bce->offset();

    if (pn->isArity(PN_NAME)) {
        






        left = pn->maybeExpr();
        if (!left) {
            left = NullaryNode::create(PNK_STRING, bce->parser);
            if (!left)
                return false;
            left->setOp(JSOP_BINDNAME);
            left->pn_pos = pn->pn_pos;
            left->pn_atom = pn->pn_atom;
        }
        right = NullaryNode::create(PNK_STRING, bce->parser);
        if (!right)
            return false;
        right->setOp(IsIdentifier(pn->pn_atom) ? JSOP_QNAMEPART : JSOP_STRING);
        right->pn_pos = pn->pn_pos;
        right->pn_atom = pn->pn_atom;
    } else {
        JS_ASSERT(pn->isArity(PN_BINARY));
        left = pn->pn_left;
        right = pn->pn_right;
    }

    if (op == JSOP_GETELEM && left->isKind(PNK_NAME) && right->isKind(PNK_NUMBER)) {
        if (!BindNameToSlot(cx, bce, left))
            return false;
    }

    if (!EmitTree(cx, bce, left))
        return false;

    if (op == JSOP_CALLELEM && Emit1(cx, bce, JSOP_DUP) < 0)
        return false;

    
    JS_ASSERT(op != JSOP_DESCENDANTS || !right->isKind(PNK_STRING) ||
              right->isOp(JSOP_QNAMEPART));
    if (!EmitTree(cx, bce, right))
        return false;
    if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - top) < 0)
        return false;
    return EmitElemOpBase(cx, bce, op);
}

static bool
EmitElemIncDec(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    if (pn) {
        if (!EmitElemOp(cx, pn, op, bce))
            return false;
    } else {
        if (!EmitElemOpBase(cx, bce, op))
            return false;
    }
    if (Emit1(cx, bce, JSOP_NOP) < 0)
        return false;

    
    bce->stackDepth++;

    int start = bce->offset();

    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT(cs->format & JOF_ELEM);
    JS_ASSERT(cs->format & (JOF_INC | JOF_DEC));

    bool post = (cs->format & JOF_POST);
    JSOp binop = (cs->format & JOF_INC) ? JSOP_ADD : JSOP_SUB;

    



                                                    
    if (Emit1(cx, bce, JSOP_TOID) < 0)              
        return false;
    if (Emit1(cx, bce, JSOP_DUP2) < 0)              
        return false;
    if (!EmitElemOpBase(cx, bce, JSOP_GETELEM))     
        return false;
    if (Emit1(cx, bce, JSOP_POS) < 0)               
        return false;
    if (post && Emit1(cx, bce, JSOP_DUP) < 0)       
        return false;
    if (Emit1(cx, bce, JSOP_ONE) < 0)               
        return false;
    if (Emit1(cx, bce, binop) < 0)                  
        return false;

    if (post) {
        if (Emit2(cx, bce, JSOP_PICK, (jsbytecode)3) < 0)   
            return false;
        if (Emit2(cx, bce, JSOP_PICK, (jsbytecode)3) < 0)   
            return false;
        if (Emit2(cx, bce, JSOP_PICK, (jsbytecode)2) < 0)   
            return false;
    }

    if (!EmitElemOpBase(cx, bce, JSOP_SETELEM))     
        return false;
    if (post && Emit1(cx, bce, JSOP_POP) < 0)       
        return false;

    UpdateDecomposeLength(bce, start);

    return true;
}

static JSBool
EmitNumberOp(JSContext *cx, double dval, BytecodeEmitter *bce)
{
    int32_t ival;
    uint32_t u;
    ptrdiff_t off;
    jsbytecode *pc;

    if (MOZ_DOUBLE_IS_INT32(dval, &ival)) {
        if (ival == 0)
            return Emit1(cx, bce, JSOP_ZERO) >= 0;
        if (ival == 1)
            return Emit1(cx, bce, JSOP_ONE) >= 0;
        if ((int)(int8_t)ival == ival)
            return Emit2(cx, bce, JSOP_INT8, (jsbytecode)(int8_t)ival) >= 0;

        u = (uint32_t)ival;
        if (u < JS_BIT(16)) {
            EMIT_UINT16_IMM_OP(JSOP_UINT16, u);
        } else if (u < JS_BIT(24)) {
            off = EmitN(cx, bce, JSOP_UINT24, 3);
            if (off < 0)
                return JS_FALSE;
            pc = bce->code(off);
            SET_UINT24(pc, u);
        } else {
            off = EmitN(cx, bce, JSOP_INT32, 4);
            if (off < 0)
                return JS_FALSE;
            pc = bce->code(off);
            SET_INT32(pc, ival);
        }
        return JS_TRUE;
    }

    if (!bce->constList.append(DoubleValue(dval)))
        return JS_FALSE;

    return EmitIndex32(cx, JSOP_DOUBLE, bce->constList.length() - 1, bce);
}









static Value *
AllocateSwitchConstant(JSContext *cx)
{
    return cx->tempLifoAlloc().new_<Value>();
}

static inline void
SetJumpOffsetAt(BytecodeEmitter *bce, ptrdiff_t off)
{
    SET_JUMP_OFFSET(bce->code(off), bce->offset() - off);
}






MOZ_NEVER_INLINE static JSBool
EmitSwitch(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JSOp switchOp;
    JSBool ok, hasDefault, constPropagated;
    ptrdiff_t top, off, defaultOffset;
    ParseNode *pn2, *pn3, *pn4;
    uint32_t caseCount, tableLength;
    ParseNode **table;
    int32_t i, low, high;
    int noteIndex;
    size_t switchSize, tableSize;
    jsbytecode *pc, *savepc;
    StmtInfo stmtInfo(cx);

    
    switchOp = JSOP_TABLESWITCH;
    ok = JS_TRUE;
    hasDefault = constPropagated = JS_FALSE;
    defaultOffset = -1;

    pn2 = pn->pn_right;
#if JS_HAS_BLOCK_SCOPE
    



    uint32_t blockObjCount = 0;
    if (pn2->isKind(PNK_LEXICALSCOPE)) {
        blockObjCount = pn2->pn_objbox->object->asStaticBlock().slotCount();
        for (uint32_t i = 0; i < blockObjCount; ++i) {
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return JS_FALSE;
        }
    }
#endif

    
    if (!EmitTree(cx, bce, pn->pn_left))
        return JS_FALSE;

#if JS_HAS_BLOCK_SCOPE
    if (pn2->isKind(PNK_LEXICALSCOPE)) {
        PushBlockScope(bce->sc, &stmtInfo, pn2->pn_objbox->object->asStaticBlock(), -1);
        stmtInfo.type = STMT_SWITCH;
        if (!EmitEnterBlock(cx, bce, pn2, JSOP_ENTERLET1))
            return JS_FALSE;
    }
#endif

    
    top = bce->offset();
#if !JS_HAS_BLOCK_SCOPE
    PushStatement(bce->sc, &stmtInfo, STMT_SWITCH, top);
#else
    if (pn2->isKind(PNK_STATEMENTLIST)) {
        PushStatement(bce->sc, &stmtInfo, STMT_SWITCH, top);
    } else {
        



        stmtInfo.update = top = bce->offset();

        
        pn2 = pn2->expr();
    }
#endif

    caseCount = pn2->pn_count;
    tableLength = 0;
    table = NULL;

    if (caseCount == 0 ||
        (caseCount == 1 &&
         (hasDefault = (pn2->pn_head->isKind(PNK_DEFAULT))))) {
        caseCount = 0;
        low = 0;
        high = -1;
    } else {
#define INTMAP_LENGTH   256
        jsbitmap intmap_space[INTMAP_LENGTH];
        jsbitmap *intmap = NULL;
        int32_t intmap_bitlen = 0;

        low  = JSVAL_INT_MAX;
        high = JSVAL_INT_MIN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->isKind(PNK_DEFAULT)) {
                hasDefault = JS_TRUE;
                caseCount--;    
                continue;
            }

            JS_ASSERT(pn3->isKind(PNK_CASE));
            if (switchOp == JSOP_CONDSWITCH)
                continue;

            pn4 = pn3->pn_left;
            while (pn4->isKind(PNK_RP))
                pn4 = pn4->pn_kid;

            Value constVal;
            switch (pn4->getKind()) {
              case PNK_NUMBER:
                constVal.setNumber(pn4->pn_dval);
                break;
              case PNK_STRING:
                constVal.setString(pn4->pn_atom);
                break;
              case PNK_TRUE:
                constVal.setBoolean(true);
                break;
              case PNK_FALSE:
                constVal.setBoolean(false);
                break;
              case PNK_NULL:
                constVal.setNull();
                break;
              case PNK_NAME:
                if (!pn4->maybeExpr()) {
                    ok = LookupCompileTimeConstant(cx, bce, pn4->pn_atom, &constVal);
                    if (!ok)
                        goto release;
                    if (!constVal.isMagic(JS_NO_CONSTANT)) {
                        if (constVal.isObject()) {
                            



                            switchOp = JSOP_CONDSWITCH;
                            continue;
                        }
                        if (constVal.isString()) {
                            JSAtom *atom = js_AtomizeString(cx, constVal.toString());
                            if (!atom) {
                                ok = JS_FALSE;
                                goto release;
                            }
                            constVal.setString(atom);
                        }
                        constPropagated = JS_TRUE;
                        break;
                    }
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
            if ((unsigned)(i + (int)JS_BIT(15)) >= (unsigned)JS_BIT(16)) {
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
            tableLength = (uint32_t)(high - low + 1);
            if (tableLength >= JS_BIT(16) || tableLength > 2 * caseCount)
                switchOp = JSOP_LOOKUPSWITCH;
        } else if (switchOp == JSOP_LOOKUPSWITCH) {
            





            if (caseCount + bce->constList.length() > JS_BIT(16))
                switchOp = JSOP_CONDSWITCH;
        }
    }

    



    noteIndex = NewSrcNote3(cx, bce, SRC_SWITCH, 0, 0);
    if (noteIndex < 0)
        return JS_FALSE;

    if (switchOp == JSOP_CONDSWITCH) {
        


        switchSize = 0;
    } else if (switchOp == JSOP_TABLESWITCH) {
        


        switchSize = (size_t)(JUMP_OFFSET_LEN * (3 + tableLength));
    } else {
        




        switchSize = (size_t)(JUMP_OFFSET_LEN + UINT16_LEN +
                              (UINT32_INDEX_LEN + JUMP_OFFSET_LEN) * caseCount);
    }

    
    if (EmitN(cx, bce, switchOp, switchSize) < 0)
        return JS_FALSE;

    off = -1;
    if (switchOp == JSOP_CONDSWITCH) {
        int caseNoteIndex = -1;
        JSBool beforeCases = JS_TRUE;

        
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            pn4 = pn3->pn_left;
            if (pn4 && !EmitTree(cx, bce, pn4))
                return JS_FALSE;
            if (caseNoteIndex >= 0) {
                
                if (!SetSrcNoteOffset(cx, bce, (unsigned)caseNoteIndex, 0, bce->offset() - off))
                    return JS_FALSE;
            }
            if (!pn4) {
                JS_ASSERT(pn3->isKind(PNK_DEFAULT));
                continue;
            }
            caseNoteIndex = NewSrcNote2(cx, bce, SRC_PCDELTA, 0);
            if (caseNoteIndex < 0)
                return JS_FALSE;
            off = EmitJump(cx, bce, JSOP_CASE, 0);
            if (off < 0)
                return JS_FALSE;
            pn3->pn_offset = off;
            if (beforeCases) {
                unsigned noteCount, noteCountDelta;

                
                noteCount = bce->noteCount();
                if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 1, off - top))
                    return JS_FALSE;
                noteCountDelta = bce->noteCount() - noteCount;
                if (noteCountDelta != 0)
                    caseNoteIndex += noteCountDelta;
                beforeCases = JS_FALSE;
            }
        }

        





        if (!hasDefault &&
            caseNoteIndex >= 0 &&
            !SetSrcNoteOffset(cx, bce, (unsigned)caseNoteIndex, 0, bce->offset() - off))
        {
            return JS_FALSE;
        }

        
        defaultOffset = EmitJump(cx, bce, JSOP_DEFAULT, 0);
        if (defaultOffset < 0)
            return JS_FALSE;
    } else {
        pc = bce->code(top + JUMP_OFFSET_LEN);

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
                    if (pn3->isKind(PNK_DEFAULT))
                        continue;
                    i = pn3->pn_pval->toInt32();
                    i -= low;
                    JS_ASSERT((uint32_t)i < tableLength);
                    table[i] = pn3;
                }
            }
        } else {
            JS_ASSERT(switchOp == JSOP_LOOKUPSWITCH);

            
            SET_UINT16(pc, caseCount);
            pc += UINT16_LEN;
        }

        




        MUST_FLOW_THROUGH("out");

        if (constPropagated) {
            




            savepc = bce->next();
            bce->current->next = pc + 1;
            if (switchOp == JSOP_TABLESWITCH) {
                for (i = 0; i < (int)tableLength; i++) {
                    pn3 = table[i];
                    if (pn3 &&
                        (pn4 = pn3->pn_left) != NULL &&
                        pn4->isKind(PNK_NAME))
                    {
                        
                        JS_ASSERT(!pn4->maybeExpr());
                        jsatomid index;
                        if (!bce->makeAtomIndex(pn4->pn_atom, &index))
                            goto bad;
                        bce->current->next = pc;
                        if (NewSrcNote2(cx, bce, SRC_LABEL, ptrdiff_t(index)) < 0)
                            goto bad;
                    }
                    pc += JUMP_OFFSET_LEN;
                }
            } else {
                for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                    pn4 = pn3->pn_left;
                    if (pn4 && pn4->isKind(PNK_NAME)) {
                        
                        JS_ASSERT(!pn4->maybeExpr());
                        jsatomid index;
                        if (!bce->makeAtomIndex(pn4->pn_atom, &index))
                            goto bad;
                        bce->current->next = pc;
                        if (NewSrcNote2(cx, bce, SRC_LABEL, ptrdiff_t(index)) < 0)
                            goto bad;
                    }
                    pc += UINT32_INDEX_LEN + JUMP_OFFSET_LEN;
                }
            }
            bce->current->next = savepc;
        }
    }

    
    for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
        if (switchOp == JSOP_CONDSWITCH && !pn3->isKind(PNK_DEFAULT))
            SetJumpOffsetAt(bce, pn3->pn_offset);
        pn4 = pn3->pn_right;
        ok = EmitTree(cx, bce, pn4);
        if (!ok)
            goto out;
        pn3->pn_offset = pn4->pn_offset;
        if (pn3->isKind(PNK_DEFAULT))
            off = pn3->pn_offset - top;
    }

    if (!hasDefault) {
        
        off = bce->offset() - top;
    }

    
    JS_ASSERT(off != -1);

    
    if (switchOp == JSOP_CONDSWITCH) {
        pc = NULL;
        JS_ASSERT(defaultOffset != -1);
        SET_JUMP_OFFSET(bce->code(defaultOffset), off - (defaultOffset - top));
    } else {
        pc = bce->code(top);
        SET_JUMP_OFFSET(pc, off);
        pc += JUMP_OFFSET_LEN;
    }

    
    off = bce->offset() - top;
    ok = SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, off);
    if (!ok)
        goto out;

    if (switchOp == JSOP_TABLESWITCH) {
        
        pc += 2 * JUMP_OFFSET_LEN;

        
        for (i = 0; i < (int)tableLength; i++) {
            pn3 = table[i];
            off = pn3 ? pn3->pn_offset - top : 0;
            SET_JUMP_OFFSET(pc, off);
            pc += JUMP_OFFSET_LEN;
        }
    } else if (switchOp == JSOP_LOOKUPSWITCH) {
        
        pc += UINT16_LEN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->isKind(PNK_DEFAULT))
                continue;
            if (!bce->constList.append(*pn3->pn_pval))
                goto bad;
            SET_UINT32_INDEX(pc, bce->constList.length() - 1);
            pc += UINT32_INDEX_LEN;

            off = pn3->pn_offset - top;
            SET_JUMP_OFFSET(pc, off);
            pc += JUMP_OFFSET_LEN;
        }
    }

out:
    if (table)
        cx->free_(table);
    if (ok) {
        ok = PopStatementBCE(cx, bce);

#if JS_HAS_BLOCK_SCOPE
        if (ok && pn->pn_right->isKind(PNK_LEXICALSCOPE))
            EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, blockObjCount);
#endif
    }
    return ok;

bad:
    ok = JS_FALSE;
    goto out;
}

JSBool
frontend::EmitFunctionScript(JSContext *cx, BytecodeEmitter *bce, ParseNode *body)
{
    






    if (bce->sc->funArgumentsHasLocalBinding()) {
        JS_ASSERT(bce->next() == bce->base());  
        bce->switchToProlog();
        if (Emit1(cx, bce, JSOP_ARGUMENTS) < 0)
            return false;
        if (bce->sc->bindingsAccessedDynamically()) {
            ScopeCoordinate sc;
            sc.hops = 0;
            sc.slot = bce->sc->bindings.localToSlot(bce->sc->argumentsLocal());
            if (!EmitAliasedVarOp(cx, JSOP_SETALIASEDVAR, sc, bce))
                return false;
        } else {
            if (!EmitUnaliasedVarOp(cx, JSOP_SETLOCAL, bce->sc->argumentsLocal(), bce))
                return false;
        }
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        bce->switchToMain();
    }

    if (bce->sc->funIsGenerator()) {
        bce->switchToProlog();
        if (Emit1(cx, bce, JSOP_GENERATOR) < 0)
            return false;
        bce->switchToMain();
    }

    if (!EmitTree(cx, bce, body))
        return false;
        
    if (Emit1(cx, bce, JSOP_STOP) < 0)
        return false;

    if (!bce->script->fullyInitFromEmitter(cx, bce))
        return false;

    return true;
}

static bool
MaybeEmitVarDecl(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn,
                 jsatomid *result)
{
    jsatomid atomIndex;

    if (!pn->pn_cookie.isFree()) {
        atomIndex = pn->pn_cookie.slot();
    } else {
        if (!bce->makeAtomIndex(pn->pn_atom, &atomIndex))
            return false;
    }

    if (JOF_OPTYPE(pn->getOp()) == JOF_ATOM &&
        (!bce->sc->inFunction() || bce->sc->funIsHeavyweight()))
    {
        bce->switchToProlog();
        if (!UpdateLineNumberNotes(cx, bce, pn->pn_pos.begin.lineno))
            return false;
        if (!EmitIndexOp(cx, prologOp, atomIndex, bce))
            return false;
        bce->switchToMain();
    }

    if (bce->sc->inFunction() &&
        JOF_OPTYPE(pn->getOp()) == JOF_LOCAL &&
        !pn->isLet() &&
        bce->shouldNoteClosedName(pn))
    {
        if (!bce->noteClosedVar(pn))
            return false;
    }

    if (result)
        *result = atomIndex;
    return true;
}









enum VarEmitOption
{
    DefineVars        = 0,
    PushInitialValues = 1,
    InitializeVars    = 2
};

#if JS_HAS_DESTRUCTURING

typedef JSBool
(*DestructuringDeclEmitter)(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn);

static JSBool
EmitDestructuringDecl(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_NAME));
    if (!BindNameToSlot(cx, bce, pn))
        return JS_FALSE;

    JS_ASSERT(!pn->isOp(JSOP_CALLEE));
    return MaybeEmitVarDecl(cx, bce, prologOp, pn, NULL);
}

static JSBool
EmitDestructuringDecls(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn)
{
    ParseNode *pn2, *pn3;
    DestructuringDeclEmitter emitter;

    if (pn->isKind(PNK_RB)) {
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(PNK_COMMA))
                continue;
            emitter = (pn2->isKind(PNK_NAME))
                      ? EmitDestructuringDecl
                      : EmitDestructuringDecls;
            if (!emitter(cx, bce, prologOp, pn2))
                return JS_FALSE;
        }
    } else {
        JS_ASSERT(pn->isKind(PNK_RC));
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            pn3 = pn2->pn_right;
            emitter = pn3->isKind(PNK_NAME) ? EmitDestructuringDecl : EmitDestructuringDecls;
            if (!emitter(cx, bce, prologOp, pn3))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
EmitDestructuringOpsHelper(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn,
                           VarEmitOption emitOption);













static JSBool
EmitDestructuringLHS(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, VarEmitOption emitOption)
{
    JS_ASSERT(emitOption != DefineVars);

    





    if (pn->isKind(PNK_RB) || pn->isKind(PNK_RC)) {
        if (!EmitDestructuringOpsHelper(cx, bce, pn, emitOption))
            return JS_FALSE;
        if (emitOption == InitializeVars) {
            



            if (Emit1(cx, bce, JSOP_POP) < 0)
                return JS_FALSE;
        }
    } else {
        if (emitOption == PushInitialValues) {
            



            JS_ASSERT(pn->getOp() == JSOP_SETLOCAL);
            JS_ASSERT(pn->pn_dflags & PND_BOUND);
            return JS_TRUE;
        }

        

        if (pn->isKind(PNK_NAME)) {
            if (!BindNameToSlot(cx, bce, pn))
                return JS_FALSE;
            if (pn->isConst() && !pn->isInitialized())
                return Emit1(cx, bce, JSOP_POP) >= 0;
        }

        switch (pn->getOp()) {
          case JSOP_SETNAME:
          case JSOP_SETGNAME:
            




            if (!EmitElemOp(cx, pn, JSOP_ENUMELEM, bce))
                return JS_FALSE;
            break;

          case JSOP_SETCONST:
            if (!EmitElemOp(cx, pn, JSOP_ENUMCONSTELEM, bce))
                return JS_FALSE;
            break;

          case JSOP_SETLOCAL:
          case JSOP_SETARG:
            if (!EmitVarOp(cx, pn, pn->getOp(), bce))
                return JS_FALSE;
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return JS_FALSE;
            break;

          default:
          {
            ptrdiff_t top;

            top = bce->offset();
            if (!EmitTree(cx, bce, pn))
                return JS_FALSE;
            if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - top) < 0)
                return JS_FALSE;
            if (!EmitElemOpBase(cx, bce, JSOP_ENUMELEM))
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
EmitDestructuringOpsHelper(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn,
                           VarEmitOption emitOption)
{
    JS_ASSERT(emitOption != DefineVars);

    unsigned index;
    ParseNode *pn2, *pn3;
    JSBool doElemOp;

#ifdef DEBUG
    int stackDepth = bce->stackDepth;
    JS_ASSERT(stackDepth != 0);
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isKind(PNK_RB) || pn->isKind(PNK_RC));
#endif

    if (pn->pn_count == 0) {
        
        if (Emit1(cx, bce, JSOP_DUP) < 0 || Emit1(cx, bce, JSOP_POP) < 0)
            return JS_FALSE;
    }

    index = 0;
    for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
        



        if (pn2 != pn->pn_head && NewSrcNote(cx, bce, SRC_CONTINUE) < 0)
            return JS_FALSE;
        if (Emit1(cx, bce, JSOP_DUP) < 0)
            return JS_FALSE;

        





        doElemOp = JS_TRUE;
        if (pn->isKind(PNK_RB)) {
            if (!EmitNumberOp(cx, index, bce))
                return JS_FALSE;
            pn3 = pn2;
        } else {
            JS_ASSERT(pn->isKind(PNK_RC));
            JS_ASSERT(pn2->isKind(PNK_COLON));
            pn3 = pn2->pn_left;
            if (pn3->isKind(PNK_NUMBER)) {
                




                if (NewSrcNote(cx, bce, SRC_INITPROP) < 0)
                    return JS_FALSE;
                if (!EmitNumberOp(cx, pn3->pn_dval, bce))
                    return JS_FALSE;
            } else {
                JS_ASSERT(pn3->isKind(PNK_STRING) || pn3->isKind(PNK_NAME));
                if (!EmitAtomOp(cx, pn3, JSOP_GETPROP, bce))
                    return JS_FALSE;
                doElemOp = JS_FALSE;
            }
            pn3 = pn2->pn_right;
        }

        if (doElemOp) {
            




            if (!EmitElemOpBase(cx, bce, JSOP_GETELEM))
                return JS_FALSE;
            JS_ASSERT(bce->stackDepth >= stackDepth + 1);
        }

        
        if (pn3->isKind(PNK_COMMA) && pn3->isArity(PN_NULLARY)) {
            JS_ASSERT(pn->isKind(PNK_RB));
            JS_ASSERT(pn2 == pn3);
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return JS_FALSE;
        } else {
            int depthBefore = bce->stackDepth;
            if (!EmitDestructuringLHS(cx, bce, pn3, emitOption))
                return JS_FALSE;

            if (emitOption == PushInitialValues) {
                









                JS_ASSERT((bce->stackDepth - bce->stackDepth) >= -1);
                unsigned pickDistance = (unsigned)((bce->stackDepth + 1) - depthBefore);
                if (pickDistance > 0) {
                    if (pickDistance > UINT8_MAX) {
                        ReportCompileErrorNumber(cx, bce->tokenStream(), pn3, JSREPORT_ERROR,
                                                 JSMSG_TOO_MANY_LOCALS);
                        return JS_FALSE;
                    }
                    if (Emit2(cx, bce, JSOP_PICK, (jsbytecode)pickDistance) < 0)
                        return false;
                }
            }
        }

        ++index;
    }

    if (emitOption == PushInitialValues) {
        



        if (Emit1(cx, bce, JSOP_POP) < 0)
            return JS_FALSE;
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








class LetNotes
{
    struct Pair {
        ptrdiff_t dup;
        unsigned index;
        Pair(ptrdiff_t dup, unsigned index) : dup(dup), index(index) {}
    };
    Vector<Pair> notes;
    bool groupAssign;
    DebugOnly<bool> updateCalled;

  public:
    LetNotes(JSContext *cx) : notes(cx), groupAssign(false), updateCalled(false) {}

    ~LetNotes() {
        JS_ASSERT_IF(!notes.allocPolicy().context()->isExceptionPending(), updateCalled);
    }

    void setGroupAssign() {
        JS_ASSERT(notes.empty());
        groupAssign = true;
    }

    bool isGroupAssign() const {
        return groupAssign;
    }

    bool append(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t dup, unsigned index) {
        JS_ASSERT(!groupAssign);
        JS_ASSERT(SN_TYPE(bce->notes() + index) == SRC_DESTRUCTLET);
        if (!notes.append(Pair(dup, index)))
            return false;

        



        if (!SetSrcNoteOffset(cx, bce, index, 0, SN_MAX_OFFSET))
            return false;
        JS_ASSERT(bce->notes()[index + 1] & SN_3BYTE_OFFSET_FLAG);
        return true;
    }

    
    bool update(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t offset) {
        JS_ASSERT(!updateCalled);
        for (size_t i = 0; i < notes.length(); ++i) {
            JS_ASSERT(offset > notes[i].dup);
            JS_ASSERT(*bce->code(notes[i].dup) == JSOP_DUP);
            JS_ASSERT(bce->notes()[notes[i].index + 1] & SN_3BYTE_OFFSET_FLAG);
            if (!SetSrcNoteOffset(cx, bce, notes[i].index, 0, offset - notes[i].dup))
                return false;
        }
        updateCalled = true;
        return true;
    }
};

static JSBool
EmitDestructuringOps(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t declType, ParseNode *pn,
                     LetNotes *letNotes = NULL)
{
    





    if (letNotes) {
        ptrdiff_t index = NewSrcNote2(cx, bce, SRC_DESTRUCTLET, 0);
        if (index < 0 || !letNotes->append(cx, bce, bce->offset(), (unsigned)index))
            return JS_FALSE;
    } else {
        if (NewSrcNote2(cx, bce, SRC_DESTRUCT, declType) < 0)
            return JS_FALSE;
    }

    



    VarEmitOption emitOption = letNotes ? PushInitialValues : InitializeVars;
    return EmitDestructuringOpsHelper(cx, bce, pn, emitOption);
}

static JSBool
EmitGroupAssignment(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp,
                    ParseNode *lhs, ParseNode *rhs)
{
    unsigned depth, limit, i, nslots;
    ParseNode *pn;

    depth = limit = (unsigned) bce->stackDepth;
    for (pn = rhs->pn_head; pn; pn = pn->pn_next) {
        if (limit == JS_BIT(16)) {
            ReportCompileErrorNumber(cx, bce->tokenStream(), rhs, JSREPORT_ERROR,
                                     JSMSG_ARRAY_INIT_TOO_BIG);
            return JS_FALSE;
        }

        
        JS_ASSERT(!(pn->isKind(PNK_COMMA) && pn->isArity(PN_NULLARY)));
        if (!EmitTree(cx, bce, pn))
            return JS_FALSE;
        ++limit;
    }

    if (NewSrcNote2(cx, bce, SRC_GROUPASSIGN, OpToDeclType(prologOp)) < 0)
        return JS_FALSE;

    i = depth;
    for (pn = lhs->pn_head; pn; pn = pn->pn_next, ++i) {
        
        JS_ASSERT(i < limit);
        int slot = AdjustBlockSlot(cx, bce, i);
        if (slot < 0)
            return JS_FALSE;

        if (!EmitUnaliasedVarOp(cx, JSOP_GETLOCAL, slot, bce))
            return JS_FALSE;

        if (pn->isKind(PNK_COMMA) && pn->isArity(PN_NULLARY)) {
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return JS_FALSE;
        } else {
            if (!EmitDestructuringLHS(cx, bce, pn, InitializeVars))
                return JS_FALSE;
        }
    }

    nslots = limit - depth;
    EMIT_UINT16_IMM_OP(JSOP_POPN, nslots);
    bce->stackDepth = (unsigned) depth;
    return JS_TRUE;
}

enum GroupOption { GroupIsDecl, GroupIsNotDecl };






static bool
MaybeEmitGroupAssignment(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn,
                         GroupOption groupOption, JSOp *pop)
{
    JS_ASSERT(pn->isKind(PNK_ASSIGN));
    JS_ASSERT(pn->isOp(JSOP_NOP));
    JS_ASSERT(*pop == JSOP_POP || *pop == JSOP_POPV);

    ParseNode *lhs = pn->pn_left;
    ParseNode *rhs = pn->pn_right;
    if (lhs->isKind(PNK_RB) && rhs->isKind(PNK_RB) &&
        !(rhs->pn_xflags & PNX_HOLEY) &&
        lhs->pn_count <= rhs->pn_count)
    {
        if (groupOption == GroupIsDecl && !EmitDestructuringDecls(cx, bce, prologOp, lhs))
            return false;
        if (!EmitGroupAssignment(cx, bce, prologOp, lhs, rhs))
            return false;
        *pop = JSOP_NOP;
    }
    return JS_TRUE;
}











static bool
MaybeEmitLetGroupDecl(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn,
                      LetNotes *letNotes, JSOp *pop)
{
    JS_ASSERT(pn->isKind(PNK_ASSIGN));
    JS_ASSERT(pn->isOp(JSOP_NOP));
    JS_ASSERT(*pop == JSOP_POP || *pop == JSOP_POPV);

    ParseNode *lhs = pn->pn_left;
    ParseNode *rhs = pn->pn_right;
    if (lhs->isKind(PNK_RB) && rhs->isKind(PNK_RB) &&
        !(rhs->pn_xflags & PNX_HOLEY) &&
        !(lhs->pn_xflags & PNX_HOLEY) &&
        lhs->pn_count == rhs->pn_count)
    {
        for (ParseNode *l = lhs->pn_head; l; l = l->pn_next) {
            if (l->getOp() != JSOP_SETLOCAL)
                return true;
        }

        for (ParseNode *r = rhs->pn_head; r; r = r->pn_next) {
            if (!EmitTree(cx, bce, r))
                return false;
        }

        letNotes->setGroupAssign();
        *pop = JSOP_NOP;
    }
    return true;
}

#endif 

static JSBool
EmitVariables(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, VarEmitOption emitOption,
              LetNotes *letNotes = NULL)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(!!letNotes == (emitOption == PushInitialValues));

    ptrdiff_t off = -1, noteIndex = -1;
    ParseNode *next;
    for (ParseNode *pn2 = pn->pn_head; ; pn2 = next) {
        bool first = pn2 == pn->pn_head;
        next = pn2->pn_next;

        ParseNode *pn3;
        if (!pn2->isKind(PNK_NAME)) {
#if JS_HAS_DESTRUCTURING
            if (pn2->isKind(PNK_RB) || pn2->isKind(PNK_RC)) {
                







                JS_ASSERT(emitOption == DefineVars);
                JS_ASSERT(pn->pn_count == 1);
                if (!EmitDestructuringDecls(cx, bce, pn->getOp(), pn2))
                    return JS_FALSE;
                break;
            }
#endif

            





            JS_ASSERT(pn2->isKind(PNK_ASSIGN));
            JS_ASSERT(pn2->isOp(JSOP_NOP));
            JS_ASSERT(emitOption != DefineVars);

            




#if !JS_HAS_DESTRUCTURING
            JS_ASSERT(pn2->pn_left->isKind(PNK_NAME));
#else
            if (pn2->pn_left->isKind(PNK_NAME))
#endif
            {
                pn3 = pn2->pn_right;
                pn2 = pn2->pn_left;
                goto do_name;
            }

#if JS_HAS_DESTRUCTURING
            ptrdiff_t stackDepthBefore = bce->stackDepth;
            JSOp op = JSOP_POP;
            if (pn->pn_count == 1) {
                





                JS_ASSERT(noteIndex < 0 && !pn2->pn_next);
                if (letNotes) {
                    if (!MaybeEmitLetGroupDecl(cx, bce, pn2, letNotes, &op))
                        return JS_FALSE;
                } else {
                    if (!MaybeEmitGroupAssignment(cx, bce, pn->getOp(), pn2, GroupIsDecl, &op))
                        return JS_FALSE;
                }
            }
            if (op == JSOP_NOP) {
                pn->pn_xflags = (pn->pn_xflags & ~PNX_POPVAR) | PNX_GROUPINIT;
            } else {
                pn3 = pn2->pn_left;
                if (!EmitDestructuringDecls(cx, bce, pn->getOp(), pn3))
                    return JS_FALSE;

                if (!EmitTree(cx, bce, pn2->pn_right))
                    return JS_FALSE;

                
                ptrdiff_t declType = pn2 == pn->pn_head
                                     ? OpToDeclType(pn->getOp())
                                     : SRC_DECL_NONE;

                if (!EmitDestructuringOps(cx, bce, declType, pn3, letNotes))
                    return JS_FALSE;
            }
            ptrdiff_t stackDepthAfter = bce->stackDepth;

            
            JS_ASSERT(stackDepthBefore <= stackDepthAfter);
            if (letNotes && stackDepthBefore == stackDepthAfter) {
                if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                    return JS_FALSE;
            }

            
            if (emitOption != InitializeVars) {
                if (next)
                    continue;
                break;
            }
            goto emit_note_pop;
#endif
        }

        





        pn3 = pn2->maybeExpr();

     do_name:
        if (!BindNameToSlot(cx, bce, pn2))
            return JS_FALSE;


        JSOp op;
        op = pn2->getOp();
        JS_ASSERT(op != JSOP_CALLEE);
        JS_ASSERT(!pn2->pn_cookie.isFree() || !pn->isOp(JSOP_NOP));

        jsatomid atomIndex;
        if (!MaybeEmitVarDecl(cx, bce, pn->getOp(), pn2, &atomIndex))
            return JS_FALSE;

        if (pn3) {
            JS_ASSERT(emitOption != DefineVars);
            JS_ASSERT_IF(emitOption == PushInitialValues, op == JSOP_SETLOCAL);
            if (op == JSOP_SETNAME || op == JSOP_SETGNAME) {
                JSOp bindOp = (op == JSOP_SETNAME) ? JSOP_BINDNAME : JSOP_BINDGNAME;
                if (!EmitIndex32(cx, bindOp, atomIndex, bce))
                    return false;
            }
            if (pn->isOp(JSOP_DEFCONST) &&
                !DefineCompileTimeConstant(cx, bce, pn2->pn_atom, pn3))
            {
                return JS_FALSE;
            }

            bool oldInForInit = bce->inForInit;
            bce->inForInit = false;
            if (!EmitTree(cx, bce, pn3))
                return JS_FALSE;
            bce->inForInit = oldInForInit;
        } else if (letNotes) {
            
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return JS_FALSE;
        }

        
        if (emitOption != InitializeVars) {
            if (next)
                continue;
            break;
        }

        JS_ASSERT_IF(pn2->isDefn(), pn3 == pn2->pn_expr);
        if (first && NewSrcNote2(cx, bce, SRC_DECL,
                                 (pn->isOp(JSOP_DEFCONST))
                                 ? SRC_DECL_CONST
                                 : (pn->isOp(JSOP_DEFVAR))
                                 ? SRC_DECL_VAR
                                 : SRC_DECL_LET) < 0) {
            return JS_FALSE;
        }
        if (!pn2->pn_cookie.isFree()) {
            if (!EmitVarOp(cx, pn2, op, bce))
                return false;
        } else {
            if (!EmitIndexOp(cx, op, atomIndex, bce))
                return false;
        }

#if JS_HAS_DESTRUCTURING
    emit_note_pop:
#endif
        ptrdiff_t tmp = bce->offset();
        if (noteIndex >= 0) {
            if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, tmp-off))
                return JS_FALSE;
        }
        if (!next)
            break;
        off = tmp;
        noteIndex = NewSrcNote2(cx, bce, SRC_PCDELTA, 0);
        if (noteIndex < 0 || Emit1(cx, bce, JSOP_POP) < 0)
            return JS_FALSE;
    }

    if (pn->pn_xflags & PNX_POPVAR) {
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return JS_FALSE;
    }

    return JS_TRUE;
}

static bool
EmitAssignment(JSContext *cx, BytecodeEmitter *bce, ParseNode *lhs, JSOp op, ParseNode *rhs)
{
    ptrdiff_t top = bce->offset();

    




    jsatomid atomIndex = (jsatomid) -1;              
    jsbytecode offset = 1;

    switch (lhs->getKind()) {
      case PNK_NAME:
        if (!BindNameToSlot(cx, bce, lhs))
            return false;
        if (!lhs->pn_cookie.isFree()) {
            JS_ASSERT(lhs->pn_cookie.level() == 0);
            atomIndex = lhs->pn_cookie.slot();
        } else {
            if (!bce->makeAtomIndex(lhs->pn_atom, &atomIndex))
                return false;
            if (!lhs->isConst()) {
                JSOp op = lhs->isOp(JSOP_SETGNAME) ? JSOP_BINDGNAME : JSOP_BINDNAME;
                if (!EmitIndex32(cx, op, atomIndex, bce))
                    return false;
                offset++;
            }
        }
        break;
      case PNK_DOT:
        if (!EmitTree(cx, bce, lhs->expr()))
            return false;
        offset++;
        if (!bce->makeAtomIndex(lhs->pn_atom, &atomIndex))
            return false;
        break;
      case PNK_LB:
        JS_ASSERT(lhs->isArity(PN_BINARY));
        if (!EmitTree(cx, bce, lhs->pn_left))
            return false;
        if (!EmitTree(cx, bce, lhs->pn_right))
            return false;
        offset += 2;
        break;
#if JS_HAS_DESTRUCTURING
      case PNK_RB:
      case PNK_RC:
        break;
#endif
      case PNK_LP:
        if (!EmitTree(cx, bce, lhs))
            return false;
        offset++;
        break;
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(!bce->sc->inStrictMode());
        JS_ASSERT(lhs->isOp(JSOP_SETXMLNAME));
        if (!EmitTree(cx, bce, lhs->pn_kid))
            return false;
        if (Emit1(cx, bce, JSOP_BINDXMLNAME) < 0)
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
          case PNK_NAME:
            if (lhs->isConst()) {
                if (lhs->isOp(JSOP_CALLEE)) {
                    if (Emit1(cx, bce, JSOP_CALLEE) < 0)
                        return false;
                } else if (lhs->isOp(JSOP_NAME) || lhs->isOp(JSOP_GETGNAME)) {
                    if (!EmitIndex32(cx, lhs->getOp(), atomIndex, bce))
                        return false;
                } else {
                    JS_ASSERT(JOF_OPTYPE(lhs->getOp()) != JOF_ATOM);
                    if (!EmitVarOp(cx, lhs, lhs->getOp(), bce))
                        return false;
                }
            } else if (lhs->isOp(JSOP_SETNAME)) {
                if (Emit1(cx, bce, JSOP_DUP) < 0)
                    return false;
                if (!EmitIndex32(cx, JSOP_GETXPROP, atomIndex, bce))
                    return false;
            } else if (lhs->isOp(JSOP_SETGNAME)) {
                JS_ASSERT(lhs->pn_cookie.isFree());
                if (!EmitAtomOp(cx, lhs, JSOP_GETGNAME, bce))
                    return false;
            } else {
                JSOp op = lhs->isOp(JSOP_SETARG) ? JSOP_GETARG : JSOP_GETLOCAL;
                if (!EmitVarOp(cx, lhs, op, bce))
                    return false;
            }
            break;
          case PNK_DOT:
            if (Emit1(cx, bce, JSOP_DUP) < 0)
                return false;
            if (lhs->pn_atom == cx->runtime->atomState.protoAtom) {
                if (!EmitIndex32(cx, JSOP_QNAMEPART, atomIndex, bce))
                    return false;
                if (!EmitElemOpBase(cx, bce, JSOP_GETELEM))
                    return false;
            } else {
                bool isLength = (lhs->pn_atom == cx->runtime->atomState.lengthAtom);
                if (!EmitIndex32(cx, isLength ? JSOP_LENGTH : JSOP_GETPROP, atomIndex, bce))
                    return false;
            }
            break;
          case PNK_LB:
          case PNK_LP:
#if JS_HAS_XML_SUPPORT
          case PNK_XMLUNARY:
#endif
            if (Emit1(cx, bce, JSOP_DUP2) < 0)
                return false;
            if (!EmitElemOpBase(cx, bce, JSOP_GETELEM))
                return false;
            break;
          default:;
        }
    }

    
    if (rhs) {
        if (!EmitTree(cx, bce, rhs))
            return false;
    } else {
        
        if (Emit2(cx, bce, JSOP_ITERNEXT, offset) < 0)
            return false;
    }

    
    if (op != JSOP_NOP) {
        




        if (!lhs->isKind(PNK_NAME) || !lhs->isConst()) {
            if (NewSrcNote(cx, bce, SRC_ASSIGNOP) < 0)
                return false;
        }
        if (Emit1(cx, bce, op) < 0)
            return false;
    }

    
    if (!lhs->isKind(PNK_NAME) &&
#if JS_HAS_DESTRUCTURING
        !lhs->isKind(PNK_RB) &&
        !lhs->isKind(PNK_RC) &&
#endif
        NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - top) < 0)
    {
        return false;
    }

    
    switch (lhs->getKind()) {
      case PNK_NAME:
        if (lhs->isConst()) {
            if (!rhs) {
                ReportCompileErrorNumber(cx, bce->tokenStream(), lhs, JSREPORT_ERROR,
                                         JSMSG_BAD_FOR_LEFTSIDE);
                return false;
            }
            break;
        }
        if (lhs->isOp(JSOP_SETARG) || lhs->isOp(JSOP_SETLOCAL)) {
            if (!EmitVarOp(cx, lhs, lhs->getOp(), bce))
                return false;
        } else {
            if (!EmitIndexOp(cx, lhs->getOp(), atomIndex, bce))
                return false;
        }
        break;
      case PNK_DOT:
        if (!EmitIndexOp(cx, lhs->getOp(), atomIndex, bce))
            return false;
        break;
      case PNK_LB:
      case PNK_LP:
        if (Emit1(cx, bce, JSOP_SETELEM) < 0)
            return false;
        break;
#if JS_HAS_DESTRUCTURING
      case PNK_RB:
      case PNK_RC:
        if (!EmitDestructuringOps(cx, bce, SRC_DECL_NONE, lhs))
            return false;
        break;
#endif
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(!bce->sc->inStrictMode());
        if (Emit1(cx, bce, JSOP_SETXMLNAME) < 0)
            return false;
        break;
#endif
      default:
        JS_ASSERT(0);
    }
    return true;
}

#ifdef DEBUG
static JSBool
GettableNoteForNextOp(BytecodeEmitter *bce)
{
    ptrdiff_t offset, target;
    jssrcnote *sn, *end;

    offset = 0;
    target = bce->offset();
    for (sn = bce->notes(), end = sn + bce->noteCount(); sn < end;
         sn = SN_NEXT(sn)) {
        if (offset == target && SN_IS_GETTABLE(sn))
            return JS_TRUE;
        offset += SN_DELTA(sn);
    }
    return JS_FALSE;
}
#endif


static JSBool
EmitFunctionDefNop(JSContext *cx, BytecodeEmitter *bce, unsigned index)
{
    return NewSrcNote2(cx, bce, SRC_FUNCDEF, (ptrdiff_t)index) >= 0 &&
           Emit1(cx, bce, JSOP_NOP) >= 0;
}

static bool
EmitNewInit(JSContext *cx, BytecodeEmitter *bce, JSProtoKey key, ParseNode *pn)
{
    const size_t len = 1 + UINT32_INDEX_LEN;
    ptrdiff_t offset = EmitCheck(cx, bce, len);
    if (offset < 0)
        return false;

    jsbytecode *next = bce->next();
    next[0] = JSOP_NEWINIT;
    next[1] = jsbytecode(key);
    next[2] = 0;
    next[3] = 0;
    next[4] = 0;
    bce->current->next = next + len;
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, JSOP_NEWINIT);
    return true;
}

bool
ParseNode::getConstantValue(JSContext *cx, bool strictChecks, Value *vp)
{
    switch (getKind()) {
      case PNK_NUMBER:
        vp->setNumber(pn_dval);
        return true;
      case PNK_STRING:
        vp->setString(pn_atom);
        return true;
      case PNK_TRUE:
        vp->setBoolean(true);
        return true;
      case PNK_FALSE:
        vp->setBoolean(false);
        return true;
      case PNK_NULL:
        vp->setNull();
        return true;
      case PNK_SPREAD:
        return false;
      case PNK_RB: {
        JS_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));

        RootedObject obj(cx, NewDenseAllocatedArray(cx, pn_count));
        if (!obj)
            return false;

        unsigned idx = 0;
        RootedId id(cx);
        for (ParseNode *pn = pn_head; pn; idx++, pn = pn->pn_next) {
            Value value;
            if (!pn->getConstantValue(cx, strictChecks, &value))
                return false;
            id = INT_TO_JSID(idx);
            if (!obj->defineGeneric(cx, id, value, NULL, NULL, JSPROP_ENUMERATE))
                return false;
        }
        JS_ASSERT(idx == pn_count);

        types::FixArrayType(cx, obj);
        vp->setObject(*obj);
        return true;
      }
      case PNK_RC: {
        JS_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));

        gc::AllocKind kind = GuessObjectGCKind(pn_count);
        RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass, kind));
        if (!obj)
            return false;

        for (ParseNode *pn = pn_head; pn; pn = pn->pn_next) {
            Value value;
            if (!pn->pn_right->getConstantValue(cx, strictChecks, &value))
                return false;

            ParseNode *pnid = pn->pn_left;
            if (pnid->isKind(PNK_NUMBER)) {
                Value idvalue = NumberValue(pnid->pn_dval);
                RootedId id(cx);
                if (idvalue.isInt32() && INT_FITS_IN_JSID(idvalue.toInt32()))
                    id = INT_TO_JSID(idvalue.toInt32());
                else if (!InternNonIntElementId(cx, obj, idvalue, id.address()))
                    return false;
                if (!obj->defineGeneric(cx, id, value, NULL, NULL, JSPROP_ENUMERATE))
                    return false;
            } else {
                JS_ASSERT(pnid->isKind(PNK_NAME) || pnid->isKind(PNK_STRING));
                JS_ASSERT(pnid->pn_atom != cx->runtime->atomState.protoAtom);
                RootedId id(cx, AtomToId(pnid->pn_atom));
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
EmitSingletonInitialiser(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    Value value;
    if (!pn->getConstantValue(cx, bce->sc->needStrictChecks(), &value))
        return false;

    JS_ASSERT(value.isObject());
    ObjectBox *objbox = bce->parser->newObjectBox(&value.toObject());
    if (!objbox)
        return false;

    return EmitObjectOp(cx, objbox, JSOP_OBJECT, bce);
}


JS_STATIC_ASSERT(JSOP_NOP_LENGTH == 1);
JS_STATIC_ASSERT(JSOP_POP_LENGTH == 1);

class EmitLevelManager
{
    BytecodeEmitter *bce;
  public:
    EmitLevelManager(BytecodeEmitter *bce) : bce(bce) { bce->emitLevel++; }
    ~EmitLevelManager() { bce->emitLevel--; }
};

static bool
EmitCatch(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    ptrdiff_t catchStart, guardJump;

    



    StmtInfo *stmt = bce->sc->topStmt;
    JS_ASSERT(stmt->type == STMT_BLOCK && (stmt->flags & SIF_SCOPE));
    stmt->type = STMT_CATCH;
    catchStart = stmt->update;

    
    stmt = stmt->down;
    JS_ASSERT(stmt->type == STMT_TRY || stmt->type == STMT_FINALLY);

    
    if (Emit1(cx, bce, JSOP_EXCEPTION) < 0)
        return false;

    



    if (pn->pn_kid2 && Emit1(cx, bce, JSOP_DUP) < 0)
        return false;

    ParseNode *pn2 = pn->pn_kid1;
    switch (pn2->getKind()) {
#if JS_HAS_DESTRUCTURING
      case PNK_RB:
      case PNK_RC:
        if (!EmitDestructuringOps(cx, bce, SRC_DECL_NONE, pn2))
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        break;
#endif

      case PNK_NAME:
        
        JS_ASSERT(!pn2->pn_cookie.isFree());
        if (!EmitVarOp(cx, pn2, JSOP_SETLOCAL, bce))
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        break;

      default:
        JS_ASSERT(0);
    }

    
    if (pn->pn_kid2) {
        if (!EmitTree(cx, bce, pn->pn_kid2))
            return false;
        if (!SetSrcNoteOffset(cx, bce, CATCHNOTE(*stmt), 0, bce->offset() - catchStart))
            return false;
        
        guardJump = EmitJump(cx, bce, JSOP_IFEQ, 0);
        if (guardJump < 0)
            return false;
        GUARDJUMP(*stmt) = guardJump;

        
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    
    if (!EmitTree(cx, bce, pn->pn_kid3))
        return false;

    



    ptrdiff_t off = bce->stackDepth;
    if (NewSrcNote2(cx, bce, SRC_CATCH, off) < 0)
        return false;
    return true;
}





MOZ_NEVER_INLINE static bool
EmitTry(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    StmtInfo stmtInfo(cx);
    ptrdiff_t catchJump = -1;

    








    PushStatement(bce->sc, &stmtInfo, pn->pn_kid3 ? STMT_FINALLY : STMT_TRY, bce->offset());

    








    int depth = bce->stackDepth;

    
    if (Emit1(cx, bce, JSOP_TRY) < 0)
        return false;
    ptrdiff_t tryStart = bce->offset();
    if (!EmitTree(cx, bce, pn->pn_kid1))
        return false;
    JS_ASSERT(depth == bce->stackDepth);

    
    if (pn->pn_kid3) {
        if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
            return false;
        if (EmitBackPatchOp(cx, bce, JSOP_BACKPATCH, &GOSUBS(stmtInfo)) < 0)
            return false;
    }

    
    if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
        return false;
    if (EmitBackPatchOp(cx, bce, JSOP_BACKPATCH, &catchJump) < 0)
        return false;

    ptrdiff_t tryEnd = bce->offset();

    
    ParseNode *lastCatch = NULL;
    if (ParseNode *pn2 = pn->pn_kid2) {
        unsigned count = 0;    

        





















        for (ParseNode *pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            ptrdiff_t guardJump, catchNote;

            JS_ASSERT(bce->stackDepth == depth);
            guardJump = GUARDJUMP(stmtInfo);
            if (guardJump != -1) {
                
                SetJumpOffsetAt(bce, guardJump);

                




                bce->stackDepth = depth + count + 1;

                





                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0 ||
                    Emit1(cx, bce, JSOP_THROWING) < 0) {
                    return false;
                }
                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                    return false;
                EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, count);
                JS_ASSERT(bce->stackDepth == depth);
            }

            






            catchNote = NewSrcNote2(cx, bce, SRC_CATCH, 0);
            if (catchNote < 0)
                return false;
            CATCHNOTE(stmtInfo) = catchNote;

            




            JS_ASSERT(pn3->isKind(PNK_LEXICALSCOPE));
            count = pn3->pn_objbox->object->asStaticBlock().slotCount();
            if (!EmitTree(cx, bce, pn3))
                return false;

            
            if (pn->pn_kid3) {
                if (EmitBackPatchOp(cx, bce, JSOP_BACKPATCH, &GOSUBS(stmtInfo)) < 0)
                    return false;
                JS_ASSERT(bce->stackDepth == depth);
            }

            



            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return false;
            if (EmitBackPatchOp(cx, bce, JSOP_BACKPATCH, &catchJump) < 0)
                return false;

            



            lastCatch = pn3->expr();
        }
    }

    





    if (lastCatch && lastCatch->pn_kid2) {
        SetJumpOffsetAt(bce, GUARDJUMP(stmtInfo));

        
        JS_ASSERT(bce->stackDepth == depth);
        bce->stackDepth = depth + 1;

        



        if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0 || Emit1(cx, bce, JSOP_THROW) < 0)
            return false;
    }

    JS_ASSERT(bce->stackDepth == depth);

    
    ptrdiff_t finallyStart = 0;   
    if (pn->pn_kid3) {
        



        if (!BackPatch(cx, bce, GOSUBS(stmtInfo), bce->next(), JSOP_GOSUB))
            return false;

        finallyStart = bce->offset();

        
        stmtInfo.type = STMT_SUBROUTINE;
        if (!UpdateLineNumberNotes(cx, bce, pn->pn_kid3->pn_pos.begin.lineno))
            return false;
        if (Emit1(cx, bce, JSOP_FINALLY) < 0 ||
            !EmitTree(cx, bce, pn->pn_kid3) ||
            Emit1(cx, bce, JSOP_RETSUB) < 0)
        {
            return false;
        }
        JS_ASSERT(bce->stackDepth == depth);
    }
    if (!PopStatementBCE(cx, bce))
        return false;

    if (NewSrcNote(cx, bce, SRC_ENDBRACE) < 0 || Emit1(cx, bce, JSOP_NOP) < 0)
        return false;

    
    if (!BackPatch(cx, bce, catchJump, bce->next(), JSOP_GOTO))
        return false;

    



    if (pn->pn_kid2 && !NewTryNote(cx, bce, JSTRY_CATCH, depth, tryStart, tryEnd))
        return false;

    




    if (pn->pn_kid3 && !NewTryNote(cx, bce, JSTRY_FINALLY, depth, tryStart, finallyStart))
        return false;

    return true;
}

static bool
EmitIf(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    StmtInfo stmtInfo(cx);

    
    stmtInfo.type = STMT_IF;
    ptrdiff_t beq = -1;
    ptrdiff_t jmp = -1;
    ptrdiff_t noteIndex = -1;

  if_again:
    
    if (!EmitTree(cx, bce, pn->pn_kid1))
        return JS_FALSE;
    ptrdiff_t top = bce->offset();
    if (stmtInfo.type == STMT_IF) {
        PushStatement(bce->sc, &stmtInfo, STMT_IF, top);
    } else {
        








        JS_ASSERT(stmtInfo.type == STMT_ELSE);
        stmtInfo.type = STMT_IF;
        stmtInfo.update = top;
        if (!SetSrcNoteOffset(cx, bce, noteIndex, 0, jmp - beq))
            return JS_FALSE;
        if (!SetSrcNoteOffset(cx, bce, noteIndex, 1, top - beq))
            return JS_FALSE;
    }

    
    ParseNode *pn3 = pn->pn_kid3;
    noteIndex = NewSrcNote(cx, bce, pn3 ? SRC_IF_ELSE : SRC_IF);
    if (noteIndex < 0)
        return JS_FALSE;
    beq = EmitJump(cx, bce, JSOP_IFEQ, 0);
    if (beq < 0)
        return JS_FALSE;

    
    if (!EmitTree(cx, bce, pn->pn_kid2))
        return JS_FALSE;
    if (pn3) {
        
        stmtInfo.type = STMT_ELSE;

        





        jmp = EmitGoto(cx, bce, &stmtInfo, &stmtInfo.breaks);
        if (jmp < 0)
            return JS_FALSE;

        
        SetJumpOffsetAt(bce, beq);
        if (pn3->isKind(PNK_IF)) {
            pn = pn3;
            goto if_again;
        }

        if (!EmitTree(cx, bce, pn3))
            return JS_FALSE;

        






        if (!SetSrcNoteOffset(cx, bce, noteIndex, 0, jmp - beq))
            return JS_FALSE;
    } else {
        
        SetJumpOffsetAt(bce, beq);
    }
    return PopStatementBCE(cx, bce);
}

#if JS_HAS_BLOCK_SCOPE










































MOZ_NEVER_INLINE static bool
EmitLet(JSContext *cx, BytecodeEmitter *bce, ParseNode *pnLet)
{
    JS_ASSERT(pnLet->isArity(PN_BINARY));
    ParseNode *varList = pnLet->pn_left;
    JS_ASSERT(varList->isArity(PN_LIST));
    ParseNode *letBody = pnLet->pn_right;
    JS_ASSERT(letBody->isLet() && letBody->isKind(PNK_LEXICALSCOPE));
    Rooted<StaticBlockObject*> blockObj(cx, &letBody->pn_objbox->object->asStaticBlock());

    ptrdiff_t letHeadOffset = bce->offset();
    int letHeadDepth = bce->stackDepth;

    LetNotes letNotes(cx);
    if (!EmitVariables(cx, bce, varList, PushInitialValues, &letNotes))
        return false;

    
    uint32_t alreadyPushed = unsigned(bce->stackDepth - letHeadDepth);
    uint32_t blockObjCount = blockObj->slotCount();
    for (uint32_t i = alreadyPushed; i < blockObjCount; ++i) {
        
        if (NewSrcNote(cx, bce, SRC_CONTINUE) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
            return false;
    }

    StmtInfo stmtInfo(cx);
    PushBlockScope(bce->sc, &stmtInfo, *blockObj, bce->offset());

    if (!letNotes.update(cx, bce, bce->offset()))
        return false;

    ptrdiff_t declNote = NewSrcNote(cx, bce, SRC_DECL);
    if (declNote < 0)
        return false;

    ptrdiff_t bodyBegin = bce->offset();
    if (!EmitEnterBlock(cx, bce, letBody, JSOP_ENTERLET0))
        return false;

    if (!EmitTree(cx, bce, letBody->pn_expr))
        return false;

    JSOp leaveOp = letBody->getOp();
    if (leaveOp == JSOP_LEAVEBLOCKEXPR) {
        if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - letHeadOffset) < 0)
            return false;
    }

    JS_ASSERT(leaveOp == JSOP_LEAVEBLOCK || leaveOp == JSOP_LEAVEBLOCKEXPR);
    EMIT_UINT16_IMM_OP(leaveOp, blockObj->slotCount());

    ptrdiff_t bodyEnd = bce->offset();
    JS_ASSERT(bodyEnd > bodyBegin);

    if (!PopStatementBCE(cx, bce))
        return false;

    ptrdiff_t o = PackLetData((bodyEnd - bodyBegin) -
                              (JSOP_ENTERLET0_LENGTH + JSOP_LEAVEBLOCK_LENGTH),
                              letNotes.isGroupAssign());
    return SetSrcNoteOffset(cx, bce, declNote, 0, o);
}
#endif

#if JS_HAS_XML_SUPPORT
static bool
EmitXMLTag(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(!bce->sc->inStrictMode());

    if (Emit1(cx, bce, JSOP_STARTXML) < 0)
        return false;

    {
        jsatomid index;
        JSAtom *tagAtom = (pn->isKind(PNK_XMLETAGO))
                          ? cx->runtime->atomState.etagoAtom
                          : cx->runtime->atomState.stagoAtom;
        if (!bce->makeAtomIndex(tagAtom, &index))
            return false;
        if (!EmitIndex32(cx, JSOP_STRING, index, bce))
            return false;
    }

    JS_ASSERT(pn->pn_count != 0);
    ParseNode *pn2 = pn->pn_head;
    if (pn2->isKind(PNK_XMLCURLYEXPR) && Emit1(cx, bce, JSOP_STARTXMLEXPR) < 0)
        return false;
    if (!EmitTree(cx, bce, pn2))
        return false;
    if (Emit1(cx, bce, JSOP_ADD) < 0)
        return false;

    uint32_t i;
    for (pn2 = pn2->pn_next, i = 0; pn2; pn2 = pn2->pn_next, i++) {
        if (pn2->isKind(PNK_XMLCURLYEXPR) && Emit1(cx, bce, JSOP_STARTXMLEXPR) < 0)
            return false;
        if (!EmitTree(cx, bce, pn2))
            return false;
        if ((i & 1) && pn2->isKind(PNK_XMLCURLYEXPR)) {
            if (Emit1(cx, bce, JSOP_TOATTRVAL) < 0)
                return false;
        }
        if (Emit1(cx, bce, (i & 1) ? JSOP_ADDATTRVAL : JSOP_ADDATTRNAME) < 0)
            return false;
    }

    {
        jsatomid index;
        JSAtom *tmp = (pn->isKind(PNK_XMLPTAGC)) ? cx->runtime->atomState.ptagcAtom
                                                 : cx->runtime->atomState.tagcAtom;
        if (!bce->makeAtomIndex(tmp, &index))
            return false;
        if (!EmitIndex32(cx, JSOP_STRING, index, bce))
            return false;
    }
    if (Emit1(cx, bce, JSOP_ADD) < 0)
        return false;

    if ((pn->pn_xflags & PNX_XMLROOT) && Emit1(cx, bce, pn->getOp()) < 0)
        return false;

    return true;
}

static bool
EmitXMLProcessingInstruction(JSContext *cx, BytecodeEmitter *bce, XMLProcessingInstruction &pi)
{
    JS_ASSERT(!bce->sc->inStrictMode());

    jsatomid index;
    if (!bce->makeAtomIndex(pi.data(), &index))
        return false;
    if (!EmitIndex32(cx, JSOP_QNAMEPART, index, bce))
        return false;
    if (!EmitAtomOp(cx, pi.target(), JSOP_XMLPI, bce))
        return false;
    return true;
}
#endif





MOZ_NEVER_INLINE static bool
EmitLexicalScope(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_LEXICALSCOPE));
    JS_ASSERT(pn->getOp() == JSOP_LEAVEBLOCK);

    StmtInfo stmtInfo(cx);
    ObjectBox *objbox = pn->pn_objbox;
    StaticBlockObject &blockObj = objbox->object->asStaticBlock();
    size_t slots = blockObj.slotCount();
    PushBlockScope(bce->sc, &stmtInfo, blockObj, bce->offset());

    






    ptrdiff_t noteIndex = -1;
    if (pn->expr()->getKind() != PNK_FOR &&
        pn->expr()->getKind() != PNK_CATCH &&
        (stmtInfo.down
         ? stmtInfo.down->type == STMT_BLOCK &&
           (!stmtInfo.down->down || stmtInfo.down->down->type != STMT_FOR_IN_LOOP)
         : !bce->sc->inFunction()))
    {
        
        JS_ASSERT(bce->noteCount() == 0 ||
                  bce->lastNoteOffset() != bce->offset() ||
                  !GettableNoteForNextOp(bce));
        noteIndex = NewSrcNote2(cx, bce, SRC_BRACE, 0);
        if (noteIndex < 0)
            return false;
    }

    ptrdiff_t bodyBegin = bce->offset();
    if (!EmitEnterBlock(cx, bce, pn, JSOP_ENTERBLOCK))
        return false;

    if (!EmitTree(cx, bce, pn->pn_expr))
        return false;

    if (noteIndex >= 0) {
        if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, bce->offset() - bodyBegin))
            return false;
    }

    EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, slots);

    return PopStatementBCE(cx, bce);
}

static bool
EmitWith(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    StmtInfo stmtInfo(cx);
    if (!EmitTree(cx, bce, pn->pn_left))
        return false;
    PushStatement(bce->sc, &stmtInfo, STMT_WITH, bce->offset());
    if (Emit1(cx, bce, JSOP_ENTERWITH) < 0)
        return false;

    if (!EmitTree(cx, bce, pn->pn_right))
        return false;
    if (Emit1(cx, bce, JSOP_LEAVEWITH) < 0)
        return false;
    return PopStatementBCE(cx, bce);
}

static bool
EmitForIn(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_FOR_IN_LOOP, top);

    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    ParseNode *pn1 = forHead->pn_kid1;
    bool letDecl = pn1 && pn1->isKind(PNK_LEXICALSCOPE);
    JS_ASSERT_IF(letDecl, pn1->isLet());

    Rooted<StaticBlockObject*> blockObj(cx, letDecl ? &pn1->pn_objbox->object->asStaticBlock() : NULL);
    uint32_t blockObjCount = blockObj ? blockObj->slotCount() : 0;

    if (letDecl) {
        
















        for (uint32_t i = 0; i < blockObjCount; ++i) {
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return false;
        }
    }

    





    if (pn1) {
        ParseNode *decl = letDecl ? pn1->pn_expr : pn1;
        JS_ASSERT(decl->isKind(PNK_VAR) || decl->isKind(PNK_LET));
        bce->inForInit = true;
        if (!EmitVariables(cx, bce, decl, DefineVars))
            return false;
        bce->inForInit = false;
    }

    
    if (!EmitTree(cx, bce, forHead->pn_kid3))
        return JS_FALSE;

    




    JS_ASSERT(pn->isOp(JSOP_ITER));
    if (Emit2(cx, bce, JSOP_ITER, (uint8_t) pn->pn_iflags) < 0)
        return false;

    
    StmtInfo letStmt(cx);
    if (letDecl) {
        PushBlockScope(bce->sc, &letStmt, *blockObj, bce->offset());
        letStmt.flags |= SIF_FOR_BLOCK;
        if (!EmitEnterBlock(cx, bce, pn1, JSOP_ENTERLET1))
            return false;
    }

    
    int noteIndex = NewSrcNote(cx, bce, SRC_FOR_IN);
    if (noteIndex < 0)
        return false;

    



    ptrdiff_t jmp = EmitJump(cx, bce, JSOP_GOTO, 0);
    if (jmp < 0)
        return false;

    top = bce->offset();
    SET_STATEMENT_TOP(&stmtInfo, top);
    if (EmitLoopHead(cx, bce, NULL) < 0)
        return false;

#ifdef DEBUG
    int loopDepth = bce->stackDepth;
#endif

    





    if (!EmitAssignment(cx, bce, forHead->pn_kid2, JSOP_NOP, NULL))
        return false;

    ptrdiff_t tmp2 = bce->offset();
    if (forHead->pn_kid1 && NewSrcNote2(cx, bce, SRC_DECL,
                                        (forHead->pn_kid1->isOp(JSOP_DEFVAR))
                                        ? SRC_DECL_VAR
                                        : SRC_DECL_LET) < 0) {
        return false;
    }
    if (Emit1(cx, bce, JSOP_POP) < 0)
        return false;

    
    JS_ASSERT(bce->stackDepth == loopDepth);

    
    if (!EmitTree(cx, bce, forBody))
        return false;

    
    StmtInfo *stmt = &stmtInfo;
    do {
        stmt->update = bce->offset();
    } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

    


    SetJumpOffsetAt(bce, jmp);
    if (!EmitLoopEntry(cx, bce, NULL))
        return false;
    if (Emit1(cx, bce, JSOP_MOREITER) < 0)
        return false;
    ptrdiff_t beq = EmitJump(cx, bce, JSOP_IFNE, top - bce->offset());
    if (beq < 0)
        return false;

    
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, tmp2 - jmp))
        return false;
    
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 1, beq - jmp))
        return false;

    
    if (!PopStatementBCE(cx, bce))
        return false;

    if (letDecl) {
        if (!PopStatementBCE(cx, bce))
            return false;
        if (Emit1(cx, bce, JSOP_LEAVEFORLETIN) < 0)
            return false;
    }

    if (!NewTryNote(cx, bce, JSTRY_ITER, bce->stackDepth, top, bce->offset()))
        return false;
    if (Emit1(cx, bce, JSOP_ENDITER) < 0)
        return false;

    if (letDecl) {
        
        if (NewSrcNote(cx, bce, SRC_CONTINUE) < 0)
            return false;
        EMIT_UINT16_IMM_OP(JSOP_POPN, blockObjCount);
    }

    return true;
}

static bool
EmitNormalFor(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_FOR_LOOP, top);

    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    
    JSOp op = JSOP_POP;
    ParseNode *pn3 = forHead->pn_kid1;
    if (!pn3) {
        
        op = JSOP_NOP;
    } else {
        bce->inForInit = true;
#if JS_HAS_DESTRUCTURING
        if (pn3->isKind(PNK_ASSIGN)) {
            JS_ASSERT(pn3->isOp(JSOP_NOP));
            if (!MaybeEmitGroupAssignment(cx, bce, op, pn3, GroupIsNotDecl, &op))
                return false;
        }
#endif
        if (op == JSOP_POP) {
            if (!EmitTree(cx, bce, pn3))
                return false;
            if (pn3->isKind(PNK_VAR) || pn3->isKind(PNK_CONST) || pn3->isKind(PNK_LET)) {
                





                JS_ASSERT(pn3->isArity(PN_LIST) || pn3->isArity(PN_BINARY));
                if (pn3->pn_xflags & PNX_GROUPINIT)
                    op = JSOP_NOP;
            }
        }
        bce->inForInit = false;
    }

    





    int noteIndex = NewSrcNote(cx, bce, SRC_FOR);
    if (noteIndex < 0 || Emit1(cx, bce, op) < 0)
        return false;
    ptrdiff_t tmp = bce->offset();

    ptrdiff_t jmp = -1;
    if (forHead->pn_kid2) {
        
        jmp = EmitJump(cx, bce, JSOP_GOTO, 0);
        if (jmp < 0)
            return false;
    } else {
        if (op != JSOP_NOP && Emit1(cx, bce, JSOP_NOP) < 0)
            return false;
    }

    top = bce->offset();
    SET_STATEMENT_TOP(&stmtInfo, top);

    
    if (EmitLoopHead(cx, bce, forBody) < 0)
        return false;
    if (jmp == -1 && !EmitLoopEntry(cx, bce, forBody))
        return false;
    if (!EmitTree(cx, bce, forBody))
        return false;

    
    JS_ASSERT(noteIndex != -1);
    ptrdiff_t tmp2 = bce->offset();

    
    StmtInfo *stmt = &stmtInfo;
    do {
        stmt->update = bce->offset();
    } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

    
    pn3 = forHead->pn_kid3;
    if (pn3) {
        op = JSOP_POP;
#if JS_HAS_DESTRUCTURING
        if (pn3->isKind(PNK_ASSIGN)) {
            JS_ASSERT(pn3->isOp(JSOP_NOP));
            if (!MaybeEmitGroupAssignment(cx, bce, op, pn3, GroupIsNotDecl, &op))
                return false;
        }
#endif
        if (op == JSOP_POP && !EmitTree(cx, bce, pn3))
            return false;

        
        if (Emit1(cx, bce, op) < 0)
            return false;

        
        ptrdiff_t lineno = pn->pn_pos.end.lineno;
        if (bce->currentLine() != (unsigned) lineno) {
            if (NewSrcNote2(cx, bce, SRC_SETLINE, lineno) < 0)
                return false;
            bce->current->currentLine = (unsigned) lineno;
        }
    }

    ptrdiff_t tmp3 = bce->offset();

    if (forHead->pn_kid2) {
        
        JS_ASSERT(jmp >= 0);
        SetJumpOffsetAt(bce, jmp);
        if (!EmitLoopEntry(cx, bce, forHead->pn_kid2))
            return false;

        if (!EmitTree(cx, bce, forHead->pn_kid2))
            return false;
    }

    
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, tmp3 - tmp))
        return false;
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 1, tmp2 - tmp))
        return false;
    
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 2, bce->offset() - tmp))
        return false;

    
    op = forHead->pn_kid2 ? JSOP_IFNE : JSOP_GOTO;
    if (EmitJump(cx, bce, op, top - bce->offset()) < 0)
        return false;

    
    return PopStatementBCE(cx, bce);
}

static inline bool
EmitFor(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    JS_ASSERT(pn->pn_left->isKind(PNK_FORIN) || pn->pn_left->isKind(PNK_FORHEAD));
    return pn->pn_left->isKind(PNK_FORIN)
           ? EmitForIn(cx, bce, pn, top)
           : EmitNormalFor(cx, bce, pn, top);
}

static JS_NEVER_INLINE bool
EmitFunc(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
#if JS_HAS_XML_SUPPORT
    if (pn->isArity(PN_NULLARY))
        return Emit1(cx, bce, JSOP_GETFUNNS) >= 0;
#endif

    RootedFunction fun(cx, pn->pn_funbox->function());
    JS_ASSERT(fun->isInterpreted());
    if (fun->script()) {
        




        JS_ASSERT(pn->isOp(JSOP_NOP));
        JS_ASSERT(bce->sc->inFunction());
        return EmitFunctionDefNop(cx, bce, pn->pn_index);
    }

    JS_ASSERT_IF(pn->pn_funbox->funIsHeavyweight(),
                 fun->kind() == JSFUN_INTERPRETED);

    {
        FunctionBox *funbox = pn->pn_funbox;
        SharedContext sc(cx,  NULL, fun, funbox);
        sc.cxFlags = funbox->cxFlags;
        if (bce->sc->funMightAliasLocals())
            sc.setFunMightAliasLocals();  
        sc.bindings.transfer(cx, &funbox->bindings);

        
        GlobalObject *globalObject = fun->getParent() ? &fun->getParent()->global() : NULL;
        Rooted<JSScript*> script(cx);
        Rooted<JSScript*> parent(cx, bce->script);
        script = JSScript::Create(cx, parent->savedCallerFun, parent->principals,
                                  parent->originPrincipals, parent->compileAndGo,
                                   false, globalObject,
                                  parent->getVersion(), parent->staticLevel + 1);
        if (!script)
            return false;

        BytecodeEmitter bce2(bce->parser, &sc, script, pn->pn_pos.begin.lineno);
        if (!bce2.init())
            return false;
        bce2.parent = bce;
        bce2.globalScope = bce->globalScope;

        
        if (!EmitFunctionScript(cx, &bce2, pn->pn_body))
            return false;
    }

    
    unsigned index = bce->objectList.add(pn->pn_funbox);

    
    if (pn->getOp() != JSOP_NOP) {
        if (pn->pn_funbox->inGenexpLambda && NewSrcNote(cx, bce, SRC_GENEXP) < 0)
            return false;

        return EmitFunctionOp(cx, pn->getOp(), index, bce);
    }

    








    if (!bce->sc->inFunction()) {
        JS_ASSERT(!bce->sc->topStmt);
        JS_ASSERT(pn->pn_cookie.isFree());
        if (pn->pn_cookie.isFree()) {
            bce->switchToProlog();
            if (!EmitFunctionOp(cx, JSOP_DEFFUN, index, bce))
                return false;
            if (!UpdateLineNumberNotes(cx, bce, pn->pn_pos.begin.lineno))
                return false;
            bce->switchToMain();
        }

        
        if (!EmitFunctionDefNop(cx, bce, index))
            return false;
    } else {
        unsigned slot;
        DebugOnly<BindingKind> kind = bce->sc->bindings.lookup(cx, fun->atom, &slot);
        JS_ASSERT(kind == VARIABLE || kind == CONSTANT);
        JS_ASSERT(index < JS_BIT(20));
        pn->pn_index = index;
        if (bce->shouldNoteClosedName(pn) && !bce->noteClosedVar(pn))
            return false;

        if (NewSrcNote(cx, bce, SRC_CONTINUE) < 0)
            return false;
        if (!EmitIndexOp(cx, JSOP_LAMBDA, index, bce))
            return false;
        if (!EmitVarOp(cx, pn, JSOP_SETLOCAL, bce))
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    return true;
}

static bool
EmitDo(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    
    ptrdiff_t noteIndex = NewSrcNote(cx, bce, SRC_WHILE);
    if (noteIndex < 0 || Emit1(cx, bce, JSOP_NOP) < 0)
        return false;

    ptrdiff_t noteIndex2 = NewSrcNote(cx, bce, SRC_WHILE);
    if (noteIndex2 < 0)
        return false;

    
    ptrdiff_t top = EmitLoopHead(cx, bce, pn->pn_left);
    if (top < 0)
        return false;
    if (!EmitLoopEntry(cx, bce, NULL))
        return false;

    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_DO_LOOP, top);
    if (!EmitTree(cx, bce, pn->pn_left))
        return false;

    
    ptrdiff_t off = bce->offset();
    StmtInfo *stmt = &stmtInfo;
    do {
        stmt->update = off;
    } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

    
    if (!EmitTree(cx, bce, pn->pn_right))
        return false;

    




    ptrdiff_t beq = EmitJump(cx, bce, JSOP_IFNE, top - bce->offset());
    if (beq < 0)
        return false;

    



    if (!SetSrcNoteOffset(cx, bce, noteIndex2, 0, beq - top))
        return false;
    if (!SetSrcNoteOffset(cx, bce, noteIndex, 0, 1 + (off - top)))
        return false;

    return PopStatementBCE(cx, bce);
}

static bool
EmitWhile(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    












    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_WHILE_LOOP, top);

    ptrdiff_t noteIndex = NewSrcNote(cx, bce, SRC_WHILE);
    if (noteIndex < 0)
        return false;

    ptrdiff_t jmp = EmitJump(cx, bce, JSOP_GOTO, 0);
    if (jmp < 0)
        return false;

    top = EmitLoopHead(cx, bce, pn->pn_right);
    if (top < 0)
        return false;

    if (!EmitTree(cx, bce, pn->pn_right))
        return false;

    SetJumpOffsetAt(bce, jmp);
    if (!EmitLoopEntry(cx, bce, pn->pn_left))
        return false;
    if (!EmitTree(cx, bce, pn->pn_left))
        return false;

    ptrdiff_t beq = EmitJump(cx, bce, JSOP_IFNE, top - bce->offset());
    if (beq < 0)
        return false;

    if (!SetSrcNoteOffset(cx, bce, noteIndex, 0, beq - jmp))
        return false;

    return PopStatementBCE(cx, bce);
}

static bool
EmitBreak(JSContext *cx, BytecodeEmitter *bce, PropertyName *label)
{
    StmtInfo *stmt = bce->sc->topStmt;
    SrcNoteType noteType;
    jsatomid labelIndex;
    if (label) {
        if (!bce->makeAtomIndex(label, &labelIndex))
            return false;

        while (stmt->type != STMT_LABEL || stmt->label != label)
            stmt = stmt->down;
        noteType = SRC_BREAK2LABEL;
    } else {
        labelIndex = INVALID_ATOMID;
        while (!STMT_IS_LOOP(stmt) && stmt->type != STMT_SWITCH)
            stmt = stmt->down;
        noteType = (stmt->type == STMT_SWITCH) ? SRC_SWITCHBREAK : SRC_BREAK;
    }

    return EmitGoto(cx, bce, stmt, &stmt->breaks, labelIndex, noteType) >= 0;
}

static bool
EmitContinue(JSContext *cx, BytecodeEmitter *bce, PropertyName *label)
{
    StmtInfo *stmt = bce->sc->topStmt;
    SrcNoteType noteType;
    jsatomid labelIndex;
    if (label) {
        if (!bce->makeAtomIndex(label, &labelIndex))
            return false;

        
        StmtInfo *loop = NULL;
        while (stmt->type != STMT_LABEL || stmt->label != label) {
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

    return EmitGoto(cx, bce, stmt, &stmt->continues, labelIndex, noteType) >= 0;
}

static bool
EmitReturn(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    
    if (ParseNode *pn2 = pn->pn_kid) {
        if (!EmitTree(cx, bce, pn2))
            return false;
    } else {
        
        if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
            return false;
    }

    










    ptrdiff_t top = bce->offset();

    if (Emit1(cx, bce, JSOP_RETURN) < 0)
        return false;
    if (!EmitNonLocalJumpFixup(cx, bce, NULL))
        return false;
    if (top + JSOP_RETURN_LENGTH != bce->offset()) {
        bce->base()[top] = JSOP_SETRVAL;
        if (Emit1(cx, bce, JSOP_RETRVAL) < 0)
            return false;
    }

    return true;
}

static bool
EmitStatementList(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    JS_ASSERT(pn->isArity(PN_LIST));

    ptrdiff_t noteIndex = -1;
    ptrdiff_t tmp = bce->offset();
    if (pn->pn_xflags & PNX_NEEDBRACES) {
        noteIndex = NewSrcNote2(cx, bce, SRC_BRACE, 0);
        if (noteIndex < 0 || Emit1(cx, bce, JSOP_NOP) < 0)
            return false;
    }

    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_BLOCK, top);

    ParseNode *pnchild = pn->pn_head;

    if (pn->pn_xflags & PNX_DESTRUCT)
        pnchild = pnchild->pn_next;

    for (ParseNode *pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
        if (!EmitTree(cx, bce, pn2))
            return false;
    }

    if (noteIndex >= 0 && !SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, bce->offset() - tmp))
        return false;

    return PopStatementBCE(cx, bce);
}

static bool
EmitStatement(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_SEMI));

    ParseNode *pn2 = pn->pn_kid;
    if (!pn2)
        return true;

    








    bool wantval = false;
    JSBool useful = JS_FALSE;
    if (bce->sc->inFunction()) {
        JS_ASSERT(!bce->script->noScriptRval);
    } else {
        useful = wantval = !bce->script->noScriptRval;
    }

    
    if (!useful) {
        if (!CheckSideEffects(cx, bce, pn2, &useful))
            return false;

        





        if (bce->sc->topStmt &&
            bce->sc->topStmt->type == STMT_LABEL &&
            bce->sc->topStmt->update >= bce->offset())
        {
            useful = true;
        }
    }

    if (useful) {
        JSOp op = wantval ? JSOP_POPV : JSOP_POP;
        JS_ASSERT_IF(pn2->isKind(PNK_ASSIGN), pn2->isOp(JSOP_NOP));
#if JS_HAS_DESTRUCTURING
        if (!wantval &&
            pn2->isKind(PNK_ASSIGN) &&
            !MaybeEmitGroupAssignment(cx, bce, op, pn2, GroupIsNotDecl, &op))
        {
            return false;
        }
#endif
        if (op != JSOP_NOP) {
            if (!EmitTree(cx, bce, pn2))
                return false;
            if (Emit1(cx, bce, op) < 0)
                return false;
        }
    } else if (!pn->isDirectivePrologueMember()) {
        
        bce->current->currentLine = pn2->pn_pos.begin.lineno;
        if (!ReportCompileErrorNumber(cx, bce->tokenStream(), pn2,
                                      JSREPORT_WARNING | JSREPORT_STRICT, JSMSG_USELESS_EXPR))
        {
            return false;
        }
    }

    return true;
}

static bool
EmitDelete(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    



    ParseNode *pn2 = pn->pn_kid;
    switch (pn2->getKind()) {
      case PNK_NAME:
      {
        if (!BindNameToSlot(cx, bce, pn2))
            return false;
        JSOp op = pn2->getOp();
        if (op == JSOP_FALSE) {
            if (Emit1(cx, bce, op) < 0)
                return false;
        } else {
            if (!EmitAtomOp(cx, pn2, op, bce))
                return false;
        }
        break;
      }
      case PNK_DOT:
        if (!EmitPropOp(cx, pn2, JSOP_DELPROP, bce, false))
            return false;
        break;
#if JS_HAS_XML_SUPPORT
      case PNK_DBLDOT:
        JS_ASSERT(!bce->sc->inStrictMode());
        if (!EmitElemOp(cx, pn2, JSOP_DELDESC, bce))
            return false;
        break;
#endif
      case PNK_LB:
        if (!EmitElemOp(cx, pn2, JSOP_DELELEM, bce))
            return false;
        break;
      default:
      {
        



        JSBool useful = false;
        if (!CheckSideEffects(cx, bce, pn2, &useful))
            return false;

        ptrdiff_t off, noteIndex;
        if (useful) {
            JS_ASSERT_IF(pn2->isKind(PNK_LP), !(pn2->pn_xflags & PNX_SETCALL));
            if (!EmitTree(cx, bce, pn2))
                return false;
            off = bce->offset();
            noteIndex = NewSrcNote2(cx, bce, SRC_PCDELTA, 0);
            if (noteIndex < 0 || Emit1(cx, bce, JSOP_POP) < 0)
                return false;
        } else {
            off = noteIndex = -1;
        }

        if (Emit1(cx, bce, JSOP_TRUE) < 0)
            return false;
        if (noteIndex >= 0) {
            ptrdiff_t tmp = bce->offset();
            if (!SetSrcNoteOffset(cx, bce, unsigned(noteIndex), 0, tmp - off))
                return false;
        }
      }
    }

    return true;
}

static bool
EmitCallOrNew(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    bool callop = pn->isKind(PNK_LP);

    














    ParseNode *pn2 = pn->pn_head;
    switch (pn2->getKind()) {
      case PNK_NAME:
        if (!EmitNameOp(cx, bce, pn2, callop))
            return false;
        break;
      case PNK_DOT:
        if (!EmitPropOp(cx, pn2, pn2->getOp(), bce, callop))
            return false;
        break;
      case PNK_LB:
        JS_ASSERT(pn2->isOp(JSOP_GETELEM));
        if (!EmitElemOp(cx, pn2, callop ? JSOP_CALLELEM : JSOP_GETELEM, bce))
            return false;
        break;
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(pn2->isOp(JSOP_XMLNAME));
        if (!EmitXMLName(cx, pn2, JSOP_CALLXMLNAME, bce))
            return false;
        callop = true;          
        break;
#endif
      default:
        if (!EmitTree(cx, bce, pn2))
            return false;
        callop = false;             
        break;
    }
    if (!callop) {
        if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
            return false;
    }

    
    ptrdiff_t off = top;

    




    bool oldInForInit = bce->inForInit;
    bce->inForInit = false;
    for (ParseNode *pn3 = pn2->pn_next; pn3; pn3 = pn3->pn_next) {
        if (!EmitTree(cx, bce, pn3))
            return false;
        if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
            return false;
    }
    bce->inForInit = oldInForInit;
    if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - off) < 0)
        return false;

    uint32_t argc = pn->pn_count - 1;
    if (Emit3(cx, bce, pn->getOp(), ARGC_HI(argc), ARGC_LO(argc)) < 0)
        return false;
    CheckTypeSet(cx, bce, pn->getOp());
    if (pn->isOp(JSOP_EVAL))
        EMIT_UINT16_IMM_OP(JSOP_LINENO, pn->pn_pos.begin.lineno);
    if (pn->pn_xflags & PNX_SETCALL) {
        if (Emit1(cx, bce, JSOP_SETCALL) < 0)
            return false;
    }
    return true;
}

static bool
EmitLogical(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    









    if (pn->isArity(PN_BINARY)) {
        if (!EmitTree(cx, bce, pn->pn_left))
            return false;
        ptrdiff_t top = EmitJump(cx, bce, JSOP_BACKPATCH, 0);
        if (top < 0)
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        if (!EmitTree(cx, bce, pn->pn_right))
            return false;
        ptrdiff_t off = bce->offset();
        jsbytecode *pc = bce->code(top);
        SET_JUMP_OFFSET(pc, off - top);
        *pc = pn->getOp();
        return true;
    }

    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->pn_head->pn_next->pn_next);

    
    ParseNode *pn2 = pn->pn_head;
    if (!EmitTree(cx, bce, pn2))
        return false;
    ptrdiff_t top = EmitJump(cx, bce, JSOP_BACKPATCH, 0);
    if (top < 0)
        return false;
    if (Emit1(cx, bce, JSOP_POP) < 0)
        return false;

    
    ptrdiff_t jmp = top;
    while ((pn2 = pn2->pn_next)->pn_next) {
        if (!EmitTree(cx, bce, pn2))
            return false;
        ptrdiff_t off = EmitJump(cx, bce, JSOP_BACKPATCH, 0);
        if (off < 0)
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        SET_JUMP_OFFSET(bce->code(jmp), off - jmp);
        jmp = off;
    }
    if (!EmitTree(cx, bce, pn2))
        return false;

    pn2 = pn->pn_head;
    ptrdiff_t off = bce->offset();
    do {
        jsbytecode *pc = bce->code(top);
        ptrdiff_t tmp = GET_JUMP_OFFSET(pc);
        SET_JUMP_OFFSET(pc, off - top);
        *pc = pn->getOp();
        top += tmp;
    } while ((pn2 = pn2->pn_next)->pn_next);

    return true;
}

static bool
EmitIncOrDec(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    
    ParseNode *pn2 = pn->pn_kid;
    JS_ASSERT(!pn2->isKind(PNK_RP));
    JSOp op = pn->getOp();
    switch (pn2->getKind()) {
      case PNK_DOT:
        if (!EmitPropIncDec(cx, pn2, op, bce))
            return false;
        break;
      case PNK_LB:
        if (!EmitElemIncDec(cx, pn2, op, bce))
            return false;
        break;
      case PNK_LP:
        if (!EmitTree(cx, bce, pn2))
            return false;
        if (NewSrcNote2(cx, bce, SRC_PCBASE, bce->offset() - pn2->pn_offset) < 0)
            return false;
        if (Emit1(cx, bce, op) < 0)
            return false;
        




        JS_ASSERT(js_CodeSpec[op].format & JOF_DECOMPOSE);
        JS_ASSERT(js_CodeSpec[op].format & JOF_ELEM);
        if (Emit1(cx, bce, (JSOp)1) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        break;
#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        JS_ASSERT(!bce->sc->inStrictMode());
        JS_ASSERT(pn2->isOp(JSOP_SETXMLNAME));
        if (!EmitTree(cx, bce, pn2->pn_kid))
            return false;
        if (Emit1(cx, bce, JSOP_BINDXMLNAME) < 0)
            return false;
        if (!EmitElemIncDec(cx, NULL, op, bce))
            return false;
        break;
#endif
      default:
        JS_ASSERT(pn2->isKind(PNK_NAME));
        pn2->setOp(op);
        if (!BindNameToSlot(cx, bce, pn2))
            return false;
        op = pn2->getOp();
        if (op == JSOP_CALLEE) {
            if (Emit1(cx, bce, op) < 0)
                return false;
        } else if (!pn2->pn_cookie.isFree()) {
            if (js_CodeSpec[op].format & (JOF_INC | JOF_DEC)) {
                if (!EmitVarIncDec(cx, pn2, op, bce))
                    return false;
            } else {
                if (!EmitVarOp(cx, pn2, op, bce))
                    return false;
            }
        } else {
            JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
            if (js_CodeSpec[op].format & (JOF_INC | JOF_DEC)) {
                if (!EmitNameIncDec(cx, pn2, op, bce))
                    return false;
            } else {
                if (!EmitAtomOp(cx, pn2, op, bce))
                    return false;
            }
            break;
        }
        if (pn2->isConst()) {
            if (Emit1(cx, bce, JSOP_POS) < 0)
                return false;
            op = pn->getOp();
            if (!(js_CodeSpec[op].format & JOF_POST)) {
                if (Emit1(cx, bce, JSOP_ONE) < 0)
                    return false;
                op = (js_CodeSpec[op].format & JOF_INC) ? JSOP_ADD : JSOP_SUB;
                if (Emit1(cx, bce, op) < 0)
                    return false;
            }
        }
    }
    return true;
}





MOZ_NEVER_INLINE static bool
EmitLabel(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    




    JSAtom *atom = pn->pn_atom;

    jsatomid index;
    if (!bce->makeAtomIndex(atom, &index))
        return false;

    ParseNode *pn2 = pn->expr();
    SrcNoteType noteType = (pn2->isKind(PNK_STATEMENTLIST) ||
                            (pn2->isKind(PNK_LEXICALSCOPE) &&
                             pn2->expr()->isKind(PNK_STATEMENTLIST)))
                           ? SRC_LABELBRACE
                           : SRC_LABEL;
    ptrdiff_t noteIndex = NewSrcNote2(cx, bce, noteType, ptrdiff_t(index));
    if (noteIndex < 0)
        return false;

    ptrdiff_t top = EmitJump(cx, bce, JSOP_LABEL, 0);
    if (top < 0)
        return false;

    
    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_LABEL, bce->offset());
    stmtInfo.label = atom;
    if (!EmitTree(cx, bce, pn2))
        return false;
    if (!PopStatementBCE(cx, bce))
        return false;

    
    SetJumpOffsetAt(bce, top);

    
    if (noteType == SRC_LABELBRACE) {
        if (NewSrcNote(cx, bce, SRC_ENDBRACE) < 0 || Emit1(cx, bce, JSOP_NOP) < 0)
            return false;
    }

    return true;
}

static bool
EmitSyntheticStatements(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    StmtInfo stmtInfo(cx);
    PushStatement(bce->sc, &stmtInfo, STMT_SEQ, top);
    ParseNode *pn2 = pn->pn_head;
    if (pn->pn_xflags & PNX_DESTRUCT)
        pn2 = pn2->pn_next;
    for (; pn2; pn2 = pn2->pn_next) {
        if (!EmitTree(cx, bce, pn2))
            return false;
    }
    return PopStatementBCE(cx, bce);
}

static bool
EmitConditionalExpression(JSContext *cx, BytecodeEmitter *bce, ConditionalExpression &conditional)
{
    
    if (!EmitTree(cx, bce, &conditional.condition()))
        return false;
    ptrdiff_t noteIndex = NewSrcNote(cx, bce, SRC_COND);
    if (noteIndex < 0)
        return false;
    ptrdiff_t beq = EmitJump(cx, bce, JSOP_IFEQ, 0);
    if (beq < 0 || !EmitTree(cx, bce, &conditional.thenExpression()))
        return false;

    
    ptrdiff_t jmp = EmitJump(cx, bce, JSOP_GOTO, 0);
    if (jmp < 0)
        return false;
    SetJumpOffsetAt(bce, beq);

    









    JS_ASSERT(bce->stackDepth > 0);
    bce->stackDepth--;
    if (!EmitTree(cx, bce, &conditional.elseExpression()))
        return false;
    SetJumpOffsetAt(bce, jmp);
    return SetSrcNoteOffset(cx, bce, noteIndex, 0, jmp - beq);
}

static bool
EmitObject(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
#if JS_HAS_DESTRUCTURING_SHORTHAND
    if (pn->pn_xflags & PNX_DESTRUCT) {
        ReportCompileErrorNumber(cx, bce->tokenStream(), pn, JSREPORT_ERROR,
                                 JSMSG_BAD_OBJECT_INIT);
        return false;
    }
#endif

    if (!(pn->pn_xflags & PNX_NONCONST) && pn->pn_head && bce->checkSingletonContext())
        return EmitSingletonInitialiser(cx, bce, pn);

    







    ptrdiff_t offset = bce->next() - bce->base();
    if (!EmitNewInit(cx, bce, JSProto_Object, pn))
        return false;

    



    RootedObject obj(cx);
    if (bce->script->compileAndGo) {
        gc::AllocKind kind = GuessObjectGCKind(pn->pn_count);
        obj = NewBuiltinClassInstance(cx, &ObjectClass, kind);
        if (!obj)
            return false;
    }

    for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
        
        ParseNode *pn3 = pn2->pn_left;
        if (pn3->isKind(PNK_NUMBER)) {
            if (!EmitNumberOp(cx, pn3->pn_dval, bce))
                return false;
        }

        
        if (!EmitTree(cx, bce, pn2->pn_right))
            return false;

        JSOp op = pn2->getOp();
        if (op == JSOP_GETTER || op == JSOP_SETTER) {
            obj = NULL;
            if (Emit1(cx, bce, op) < 0)
                return false;
        }

        
        if (pn3->isKind(PNK_NUMBER)) {
            obj = NULL;
            if (NewSrcNote(cx, bce, SRC_INITPROP) < 0)
                return false;
            if (Emit1(cx, bce, JSOP_INITELEM) < 0)
                return false;
        } else {
            JS_ASSERT(pn3->isKind(PNK_NAME) || pn3->isKind(PNK_STRING));
            jsatomid index;
            if (!bce->makeAtomIndex(pn3->pn_atom, &index))
                return false;

            



            if (pn3->pn_atom == cx->runtime->atomState.protoAtom)
                obj = NULL;
            op = JSOP_INITPROP;

            if (obj) {
                JS_ASSERT(!obj->inDictionaryMode());
                if (!DefineNativeProperty(cx, obj, RootedId(cx, AtomToId(pn3->pn_atom)),
                                          UndefinedValue(), NULL, NULL,
                                          JSPROP_ENUMERATE, 0, 0))
                {
                    return false;
                }
                if (obj->inDictionaryMode())
                    obj = NULL;
            }

            if (!EmitIndex32(cx, op, index, bce))
                return false;
        }
    }

    if (Emit1(cx, bce, JSOP_ENDINIT) < 0)
        return false;

    if (obj) {
        



        ObjectBox *objbox = bce->parser->newObjectBox(obj);
        if (!objbox)
            return false;
        unsigned index = bce->objectList.add(objbox);
        MOZ_STATIC_ASSERT(JSOP_NEWINIT_LENGTH == JSOP_NEWOBJECT_LENGTH,
                          "newinit and newobject must have equal length to edit in-place");
        EMIT_UINT32_IN_PLACE(offset, JSOP_NEWOBJECT, uint32_t(index));
    }

    return true;
}

static bool
EmitArray(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    








#if JS_HAS_GENERATORS
    if (pn->isKind(PNK_ARRAYCOMP)) {
        if (!EmitNewInit(cx, bce, JSProto_Array, pn))
            return false;

        




        JS_ASSERT(bce->stackDepth > 0);
        unsigned saveDepth = bce->arrayCompDepth;
        bce->arrayCompDepth = (uint32_t) (bce->stackDepth - 1);
        if (!EmitTree(cx, bce, pn->pn_head))
            return false;
        bce->arrayCompDepth = saveDepth;

        
        return Emit1(cx, bce, JSOP_ENDINIT) >= 0;
    }
#endif 

    if (!(pn->pn_xflags & PNX_NONCONST) && pn->pn_head && bce->checkSingletonContext())
        return EmitSingletonInitialiser(cx, bce, pn);

    int32_t nspread = 0;
    for (ParseNode *elt = pn->pn_head; elt; elt = elt->pn_next) {
        if (elt->isKind(PNK_SPREAD))
            nspread++;
    }

    ptrdiff_t off = EmitN(cx, bce, JSOP_NEWARRAY, 3);
    if (off < 0)
        return false;
    CheckTypeSet(cx, bce, JSOP_NEWARRAY);
    jsbytecode *pc = bce->code(off);

    
    
    SET_UINT24(pc, pn->pn_count - nspread);

    ParseNode *pn2 = pn->pn_head;
    jsatomid atomIndex;
    if (nspread && !EmitNumberOp(cx, 0, bce))
        return false;
    for (atomIndex = 0; pn2; atomIndex++, pn2 = pn2->pn_next) {
        if (!nspread && !EmitNumberOp(cx, atomIndex, bce))
            return false;
        if (pn2->isKind(PNK_COMMA) && pn2->isArity(PN_NULLARY)) {
            if (Emit1(cx, bce, JSOP_HOLE) < 0)
                return false;
        } else {
            ParseNode *expr = pn2->isKind(PNK_SPREAD) ? pn2->pn_kid : pn2;
            if (!EmitTree(cx, bce, expr))
                return false;
        }
        if (pn2->isKind(PNK_SPREAD)) {
            if (Emit1(cx, bce, JSOP_SPREAD) < 0)
                return false;
        } else if (Emit1(cx, bce, nspread ? JSOP_INITELEM_INC : JSOP_INITELEM) < 0) {
            return false;
        }
    }
    JS_ASSERT(atomIndex == pn->pn_count);
    if (nspread) {
        if (NewSrcNote(cx, bce, SRC_CONTINUE) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    if (pn->pn_xflags & PNX_ENDCOMMA) {
        
        if (NewSrcNote(cx, bce, SRC_CONTINUE) < 0)
            return false;
    }

    
    return Emit1(cx, bce, JSOP_ENDINIT) >= 0;
}

static bool
EmitUnary(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    
    JSOp op = pn->getOp();
    ParseNode *pn2 = pn->pn_kid;

    JS_ASSERT(op != JSOP_XMLNAME);
    if (op == JSOP_TYPEOF && !pn2->isKind(PNK_NAME))
        op = JSOP_TYPEOFEXPR;

    bool oldInForInit = bce->inForInit;
    bce->inForInit = false;
    if (!EmitTree(cx, bce, pn2))
        return false;

    bce->inForInit = oldInForInit;
    return Emit1(cx, bce, op) >= 0;
}

static bool
EmitDefaults(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_ARGSBODY));
    uint16_t ndefaults = bce->sc->funbox()->ndefaults;
    JSFunction *fun = bce->sc->fun();
    unsigned nformal = fun->nargs - fun->hasRest();
    EMIT_UINT16_IMM_OP(JSOP_ACTUALSFILLED, nformal - ndefaults);
    ptrdiff_t top = bce->offset();
    size_t tableSize = (size_t)(JUMP_OFFSET_LEN * (3 + ndefaults));
    if (EmitN(cx, bce, JSOP_TABLESWITCH, tableSize) < 0)
        return false;
    jsbytecode *pc = bce->code(top + JUMP_OFFSET_LEN);
    JS_ASSERT(nformal >= ndefaults);
    uint16_t defstart = nformal - ndefaults;
    SET_JUMP_OFFSET(pc, defstart);
    pc += JUMP_OFFSET_LEN;
    SET_JUMP_OFFSET(pc, nformal - 1);
    pc += JUMP_OFFSET_LEN;

    
    unsigned i;
    ParseNode *arg, *pnlast = pn->last();
    for (arg = pn->pn_head, i = 0; arg != pnlast; arg = arg->pn_next, i++) {
        if (!(arg->pn_dflags & PND_DEFAULT))
            continue;
        SET_JUMP_OFFSET(pc, bce->offset() - top);
        pc += JUMP_OFFSET_LEN;
        ParseNode *expr;
        if (arg->isKind(PNK_NAME)) {
            expr = arg->expr();
        } else {
            
            
            JS_ASSERT(!arg->isDefn());
            JS_ASSERT(arg->isKind(PNK_ASSIGN));
            expr = arg->pn_right;
        }
        if (!EmitTree(cx, bce, expr))
            return false;
        if (arg->isKind(PNK_NAME)) {
            if (!BindNameToSlot(cx, bce, arg))
                return false;
            if (!EmitVarOp(cx, arg, JSOP_SETARG, bce))
                return false;
        } else {
            
            
            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return false;
            ptrdiff_t hop = bce->offset();
            if (EmitJump(cx, bce, JSOP_GOTO, 0) < 0)
                return false;

            unsigned slot;
            bce->sc->bindings.lookup(cx, arg->pn_left->atom(), &slot);

            
            
            if (!EmitUnaliasedVarOp(cx, JSOP_SETLOCAL, slot, bce))
                return false;
            SET_JUMP_OFFSET(bce->code(hop), bce->offset() - hop);
        }
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }
    JS_ASSERT(pc == bce->code(top + tableSize));
    SET_JUMP_OFFSET(bce->code(top), bce->offset() - top);
    return true;
}

JSBool
frontend::EmitTree(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_CHECK_RECURSION(cx, return JS_FALSE);

    EmitLevelManager elm(bce);

    JSBool ok = true;
    ptrdiff_t top = bce->offset();
    pn->pn_offset = top;

    
    UPDATE_LINE_NUMBER_NOTES(cx, bce, pn->pn_pos.begin.lineno);

    switch (pn->getKind()) {
      case PNK_FUNCTION:
        ok = EmitFunc(cx, bce, pn);
        break;

      case PNK_ARGSBODY:
      {
        JSFunction *fun = bce->sc->fun();
        ParseNode *pnlast = pn->last();

        
        
        
        
        ParseNode *pnchild = pnlast->pn_head;
        if (pnlast->pn_xflags & PNX_DESTRUCT) {
            
            
            JS_ASSERT(pnchild->isKind(PNK_SEMI));
            JS_ASSERT(pnchild->pn_kid->isKind(PNK_VAR) || pnchild->pn_kid->isKind(PNK_CONST));
            if (!EmitTree(cx, bce, pnchild))
                return false;
            pnchild = pnchild->pn_next;
        }
        if (pnlast->pn_xflags & PNX_FUNCDEFS) {
            
            
            
            
            
            
             
            
            
            for (ParseNode *pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
                if (pn2->isKind(PNK_FUNCTION)) {
                    if (pn2->isOp(JSOP_NOP)) {
                        if (!EmitTree(cx, bce, pn2))
                            return false;
                    } else {
                        
                        
                        
                        
                        JS_ASSERT(pn2->isOp(JSOP_DEFFUN));
                    }
                }
            }
        }
        if (fun->hasDefaults()) {
            ParseNode *rest = NULL;
            if (fun->hasRest()) {
                JS_ASSERT(!bce->sc->funArgumentsHasLocalBinding());
                
                
                
                
                
                
                rest = pn->pn_head;
                while (rest->pn_next != pnlast)
                    rest = rest->pn_next;
                if (Emit1(cx, bce, JSOP_REST) < 0)
                    return false;
                CheckTypeSet(cx, bce, JSOP_REST);
                if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                    return false;
                if (!EmitVarOp(cx, rest, JSOP_SETARG, bce))
                    return false;
                if (Emit1(cx, bce, JSOP_POP) < 0)
                    return false;
            }
            if (!EmitDefaults(cx, bce, pn))
                return false;
            if (fun->hasRest()) {
                if (!EmitVarOp(cx, rest, JSOP_SETARG, bce))
                    return false;
                if (Emit1(cx, bce, JSOP_POP) < 0)
                    return false;
            }
        }
        for (ParseNode *pn2 = pn->pn_head; pn2 != pnlast; pn2 = pn2->pn_next) {
            if (!pn2->isDefn())
                continue;
            if (!BindNameToSlot(cx, bce, pn2))
                return JS_FALSE;
            if (JOF_OPTYPE(pn2->getOp()) == JOF_QARG && bce->shouldNoteClosedName(pn2)) {
                if (!bce->noteClosedArg(pn2))
                    return JS_FALSE;
            }
            if (pn2->pn_next == pnlast && fun->hasRest() && !fun->hasDefaults()) {
                
                JS_ASSERT(!bce->sc->funArgumentsHasLocalBinding());
                bce->switchToProlog();
                if (Emit1(cx, bce, JSOP_REST) < 0)
                    return false;
                CheckTypeSet(cx, bce, JSOP_REST);
                if (!EmitVarOp(cx, pn2, JSOP_SETARG, bce))
                    return false;
                if (Emit1(cx, bce, JSOP_POP) < 0)
                    return false;
                bce->switchToMain();
            }
        }
        ok = EmitTree(cx, bce, pnlast);
        break;
      }

      case PNK_UPVARS:
        JS_ASSERT(pn->pn_names->count() != 0);
        ok = EmitTree(cx, bce, pn->pn_tree);
        pn->pn_names.releaseMap(cx);
        break;

      case PNK_IF:
        ok = EmitIf(cx, bce, pn);
        break;

      case PNK_SWITCH:
        ok = EmitSwitch(cx, bce, pn);
        break;

      case PNK_WHILE:
        ok = EmitWhile(cx, bce, pn, top);
        break;

      case PNK_DOWHILE:
        ok = EmitDo(cx, bce, pn);
        break;

      case PNK_FOR:
        ok = EmitFor(cx, bce, pn, top);
        break;

      case PNK_BREAK:
        ok = EmitBreak(cx, bce, pn->asBreakStatement().label());
        break;

      case PNK_CONTINUE:
        ok = EmitContinue(cx, bce, pn->asContinueStatement().label());
        break;

      case PNK_WITH:
        ok = EmitWith(cx, bce, pn);
        break;

      case PNK_TRY:
        if (!EmitTry(cx, bce, pn))
            return false;
        break;

      case PNK_CATCH:
        if (!EmitCatch(cx, bce, pn))
            return false;
        break;

      case PNK_VAR:
      case PNK_CONST:
        if (!EmitVariables(cx, bce, pn, InitializeVars))
            return JS_FALSE;
        break;

      case PNK_RETURN:
        ok = EmitReturn(cx, bce, pn);
        break;

#if JS_HAS_GENERATORS
      case PNK_YIELD:
        JS_ASSERT(bce->sc->inFunction());
        if (pn->pn_kid) {
            if (!EmitTree(cx, bce, pn->pn_kid))
                return JS_FALSE;
        } else {
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return JS_FALSE;
        }
        if (pn->pn_hidden && NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
            return JS_FALSE;
        if (Emit1(cx, bce, JSOP_YIELD) < 0)
            return JS_FALSE;
        break;
#endif

#if JS_HAS_XML_SUPPORT
      case PNK_XMLCURLYEXPR:
        JS_ASSERT(pn->isArity(PN_UNARY));
        if (!EmitTree(cx, bce, pn->pn_kid))
            return JS_FALSE;
        if (Emit1(cx, bce, pn->getOp()) < 0)
            return JS_FALSE;
        break;
#endif

      case PNK_STATEMENTLIST:
        ok = EmitStatementList(cx, bce, pn, top);
        break;

      case PNK_SEQ:
        ok = EmitSyntheticStatements(cx, bce, pn, top);
        break;

      case PNK_SEMI:
        ok = EmitStatement(cx, bce, pn);
        break;

      case PNK_COLON:
        ok = EmitLabel(cx, bce, pn);
        break;

      case PNK_COMMA:
      {
        




        ptrdiff_t off = -1, noteIndex = -1;
        for (ParseNode *pn2 = pn->pn_head; ; pn2 = pn2->pn_next) {
            if (!EmitTree(cx, bce, pn2))
                return JS_FALSE;
            ptrdiff_t tmp = bce->offset();
            if (noteIndex >= 0) {
                if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, tmp-off))
                    return JS_FALSE;
            }
            if (!pn2->pn_next)
                break;
            off = tmp;
            noteIndex = NewSrcNote2(cx, bce, SRC_PCDELTA, 0);
            if (noteIndex < 0 ||
                Emit1(cx, bce, JSOP_POP) < 0) {
                return JS_FALSE;
            }
        }
        break;
      }

      case PNK_ASSIGN:
      case PNK_ADDASSIGN:
      case PNK_SUBASSIGN:
      case PNK_BITORASSIGN:
      case PNK_BITXORASSIGN:
      case PNK_BITANDASSIGN:
      case PNK_LSHASSIGN:
      case PNK_RSHASSIGN:
      case PNK_URSHASSIGN:
      case PNK_MULASSIGN:
      case PNK_DIVASSIGN:
      case PNK_MODASSIGN:
        if (!EmitAssignment(cx, bce, pn->pn_left, pn->getOp(), pn->pn_right))
            return false;
        break;

      case PNK_CONDITIONAL:
        ok = EmitConditionalExpression(cx, bce, pn->asConditionalExpression());
        break;

      case PNK_OR:
      case PNK_AND:
        ok = EmitLogical(cx, bce, pn);
        break;

      case PNK_ADD:
      case PNK_SUB:
      case PNK_BITOR:
      case PNK_BITXOR:
      case PNK_BITAND:
      case PNK_STRICTEQ:
      case PNK_EQ:
      case PNK_STRICTNE:
      case PNK_NE:
      case PNK_LT:
      case PNK_LE:
      case PNK_GT:
      case PNK_GE:
      case PNK_IN:
      case PNK_INSTANCEOF:
      case PNK_LSH:
      case PNK_RSH:
      case PNK_URSH:
      case PNK_STAR:
      case PNK_DIV:
      case PNK_MOD:
        if (pn->isArity(PN_LIST)) {
            
            ParseNode *pn2 = pn->pn_head;
            if (!EmitTree(cx, bce, pn2))
                return JS_FALSE;
            JSOp op = pn->getOp();
            while ((pn2 = pn2->pn_next) != NULL) {
                if (!EmitTree(cx, bce, pn2))
                    return JS_FALSE;
                if (Emit1(cx, bce, op) < 0)
                    return JS_FALSE;
            }
        } else {
#if JS_HAS_XML_SUPPORT

      case PNK_DBLCOLON:
            JS_ASSERT(pn->getOp() != JSOP_XMLNAME);
            if (pn->isArity(PN_NAME)) {
                if (!EmitTree(cx, bce, pn->expr()))
                    return JS_FALSE;
                if (!EmitAtomOp(cx, pn, pn->getOp(), bce))
                    return JS_FALSE;
                break;
            }

            




            bool oldInForInit = bce->inForInit;
            bce->inForInit = false;
#endif

            
            if (!EmitTree(cx, bce, pn->pn_left))
                return JS_FALSE;
            if (!EmitTree(cx, bce, pn->pn_right))
                return JS_FALSE;
#if JS_HAS_XML_SUPPORT
            bce->inForInit = oldInForInit;
#endif
            if (Emit1(cx, bce, pn->getOp()) < 0)
                return JS_FALSE;
        }
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_XMLUNARY:
        if (pn->getOp() == JSOP_XMLNAME) {
            if (!EmitXMLName(cx, pn, JSOP_XMLNAME, bce))
                return false;
        } else {
            JSOp op = pn->getOp();
            JS_ASSERT(op == JSOP_BINDXMLNAME || op == JSOP_SETXMLNAME);
            bool oldInForInit = bce->inForInit;
            bce->inForInit = false;
            if (!EmitTree(cx, bce, pn->pn_kid))
                return false;
            bce->inForInit = oldInForInit;
            if (Emit1(cx, bce, op) < 0)
                return false;
        }
        break;
#endif

      case PNK_THROW:
#if JS_HAS_XML_SUPPORT
      case PNK_AT:
      case PNK_DEFXMLNS:
        JS_ASSERT(pn->isArity(PN_UNARY));
        
#endif
      case PNK_TYPEOF:
      case PNK_VOID:
      case PNK_NOT:
      case PNK_BITNOT:
      case PNK_POS:
      case PNK_NEG:
        ok = EmitUnary(cx, bce, pn);
        break;

      case PNK_PREINCREMENT:
      case PNK_PREDECREMENT:
      case PNK_POSTINCREMENT:
      case PNK_POSTDECREMENT:
        ok = EmitIncOrDec(cx, bce, pn);
        break;

      case PNK_DELETE:
        ok = EmitDelete(cx, bce, pn);
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_FILTER:
      {
        JS_ASSERT(!bce->sc->inStrictMode());

        if (!EmitTree(cx, bce, pn->pn_left))
            return JS_FALSE;
        ptrdiff_t jmp = EmitJump(cx, bce, JSOP_FILTER, 0);
        if (jmp < 0)
            return JS_FALSE;
        top = EmitLoopHead(cx, bce, pn->pn_right);
        if (top < 0)
            return JS_FALSE;
        if (!EmitTree(cx, bce, pn->pn_right))
            return JS_FALSE;
        SetJumpOffsetAt(bce, jmp);
        if (!EmitLoopEntry(cx, bce, NULL))
            return false;
        if (EmitJump(cx, bce, JSOP_ENDFILTER, top - bce->offset()) < 0)
            return JS_FALSE;
        break;
      }
#endif

      case PNK_DOT:
        




        ok = EmitPropOp(cx, pn, pn->getOp(), bce, JS_FALSE);
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_DBLDOT:
        JS_ASSERT(!bce->sc->inStrictMode());
        
#endif
      case PNK_LB:
        





        ok = EmitElemOp(cx, pn, pn->getOp(), bce);
        break;

      case PNK_NEW:
      case PNK_LP:
        ok = EmitCallOrNew(cx, bce, pn, top);
        break;

      case PNK_LEXICALSCOPE:
        ok = EmitLexicalScope(cx, bce, pn);
        break;

#if JS_HAS_BLOCK_SCOPE
      case PNK_LET:
        ok = pn->isArity(PN_BINARY)
             ? EmitLet(cx, bce, pn)
             : EmitVariables(cx, bce, pn, InitializeVars);
        break;
#endif 
#if JS_HAS_GENERATORS
      case PNK_ARRAYPUSH: {
        int slot;

        





        if (!EmitTree(cx, bce, pn->pn_kid))
            return JS_FALSE;
        slot = AdjustBlockSlot(cx, bce, bce->arrayCompDepth);
        if (slot < 0)
            return JS_FALSE;
        if (!EmitUnaliasedVarOp(cx, pn->getOp(), slot, bce))
            return false;
        break;
      }
#endif

      case PNK_RB:
#if JS_HAS_GENERATORS
      case PNK_ARRAYCOMP:
#endif
        ok = EmitArray(cx, bce, pn);
        break;

      case PNK_RC:
        ok = EmitObject(cx, bce, pn);
        break;

      case PNK_NAME:
        




        if (pn->isOp(JSOP_NOP))
            break;
        if (!EmitNameOp(cx, bce, pn, JS_FALSE))
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_XMLATTR:
      case PNK_XMLSPACE:
      case PNK_XMLTEXT:
      case PNK_XMLCDATA:
      case PNK_XMLCOMMENT:
        JS_ASSERT(!bce->sc->inStrictMode());
        
#endif
      case PNK_STRING:
        ok = EmitAtomOp(cx, pn, pn->getOp(), bce);
        break;

      case PNK_NUMBER:
        ok = EmitNumberOp(cx, pn->pn_dval, bce);
        break;

      case PNK_REGEXP:
        JS_ASSERT(pn->isOp(JSOP_REGEXP));
        ok = EmitRegExp(cx, bce->regexpList.add(pn->pn_objbox), bce);
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_ANYNAME:
#endif
      case PNK_TRUE:
      case PNK_FALSE:
      case PNK_THIS:
      case PNK_NULL:
        if (Emit1(cx, bce, pn->getOp()) < 0)
            return JS_FALSE;
        break;

      case PNK_DEBUGGER:
        if (Emit1(cx, bce, JSOP_DEBUGGER) < 0)
            return JS_FALSE;
        break;

#if JS_HAS_XML_SUPPORT
      case PNK_XMLELEM:
      case PNK_XMLLIST:
        JS_ASSERT(!bce->sc->inStrictMode());
        JS_ASSERT(pn->isKind(PNK_XMLLIST) || pn->pn_count != 0);

        switch (pn->pn_head ? pn->pn_head->getKind() : PNK_XMLLIST) {
          case PNK_XMLETAGO:
            JS_ASSERT(0);
            
          case PNK_XMLPTAGC:
          case PNK_XMLSTAGO:
            break;
          default:
            if (Emit1(cx, bce, JSOP_STARTXML) < 0)
                return JS_FALSE;
        }

        for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(PNK_XMLCURLYEXPR) && Emit1(cx, bce, JSOP_STARTXMLEXPR) < 0)
                return JS_FALSE;
            if (!EmitTree(cx, bce, pn2))
                return JS_FALSE;
            if (pn2 != pn->pn_head && Emit1(cx, bce, JSOP_ADD) < 0)
                return JS_FALSE;
        }

        if (pn->pn_xflags & PNX_XMLROOT) {
            if (pn->pn_count == 0) {
                JS_ASSERT(pn->isKind(PNK_XMLLIST));
                JSAtom *atom = cx->runtime->atomState.emptyAtom;
                jsatomid index;
                if (!bce->makeAtomIndex(atom, &index))
                    return JS_FALSE;
                if (!EmitIndex32(cx, JSOP_STRING, index, bce))
                    return false;
            }
            if (Emit1(cx, bce, pn->getOp()) < 0)
                return JS_FALSE;
        }
#ifdef DEBUG
        else
            JS_ASSERT(pn->pn_count != 0);
#endif
        break;

      case PNK_XMLPTAGC:
      case PNK_XMLSTAGO:
      case PNK_XMLETAGO:
        if (!EmitXMLTag(cx, bce, pn))
            return false;
        break;

      case PNK_XMLNAME:
        JS_ASSERT(!bce->sc->inStrictMode());

        if (pn->isArity(PN_LIST)) {
            JS_ASSERT(pn->pn_count != 0);
            for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
                if (pn2->isKind(PNK_XMLCURLYEXPR) && Emit1(cx, bce, JSOP_STARTXMLEXPR) < 0)
                    return JS_FALSE;
                if (!EmitTree(cx, bce, pn2))
                    return JS_FALSE;
                if (pn2 != pn->pn_head && Emit1(cx, bce, JSOP_ADD) < 0)
                    return JS_FALSE;
            }
        } else {
            JS_ASSERT(pn->isArity(PN_NULLARY));
            ok = pn->isOp(JSOP_OBJECT)
                 ? EmitObjectOp(cx, pn->pn_objbox, pn->getOp(), bce)
                 : EmitAtomOp(cx, pn, pn->getOp(), bce);
        }
        break;

      case PNK_XMLPI:
        if (!EmitXMLProcessingInstruction(cx, bce, pn->asXMLProcessingInstruction()))
            return false;
        break;
#endif 

      default:
        JS_ASSERT(0);
    }

    
    if (ok && bce->emitLevel == 1) {
        if (!UpdateLineNumberNotes(cx, bce, pn->pn_pos.end.lineno))
            return JS_FALSE;
    }

    return ok;
}

static int
AllocSrcNote(JSContext *cx, BytecodeEmitter *bce)
{
    jssrcnote *notes = bce->notes();
    jssrcnote *newnotes;
    unsigned index = bce->noteCount();
    unsigned max = bce->noteLimit();

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
        bce->current->notes = newnotes;
        bce->current->noteLimit = newlength;
    }

    bce->current->noteCount = index + 1;
    return (int)index;
}

int
frontend::NewSrcNote(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type)
{
    int index, n;
    jssrcnote *sn;
    ptrdiff_t offset, delta, xdelta;

    



    index = AllocSrcNote(cx, bce);
    if (index < 0)
        return -1;
    sn = &bce->notes()[index];

    



    offset = bce->offset();
    delta = offset - bce->lastNoteOffset();
    bce->current->lastNoteOffset = offset;
    if (delta >= SN_DELTA_LIMIT) {
        do {
            xdelta = JS_MIN(delta, SN_XDELTA_MASK);
            SN_MAKE_XDELTA(sn, xdelta);
            delta -= xdelta;
            index = AllocSrcNote(cx, bce);
            if (index < 0)
                return -1;
            sn = &bce->notes()[index];
        } while (delta >= SN_DELTA_LIMIT);
    }

    




    SN_MAKE_NOTE(sn, type, delta);
    for (n = (int)js_SrcNoteSpec[type].arity; n > 0; n--) {
        if (NewSrcNote(cx, bce, SRC_NULL) < 0)
            return -1;
    }
    return index;
}

int
frontend::NewSrcNote2(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset)
{
    int index;

    index = NewSrcNote(cx, bce, type);
    if (index >= 0) {
        if (!SetSrcNoteOffset(cx, bce, index, 0, offset))
            return -1;
    }
    return index;
}

int
frontend::NewSrcNote3(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset1,
            ptrdiff_t offset2)
{
    int index;

    index = NewSrcNote(cx, bce, type);
    if (index >= 0) {
        if (!SetSrcNoteOffset(cx, bce, index, 0, offset1))
            return -1;
        if (!SetSrcNoteOffset(cx, bce, index, 1, offset2))
            return -1;
    }
    return index;
}

static JSBool
GrowSrcNotes(JSContext *cx, BytecodeEmitter *bce)
{
    size_t newlength = bce->noteLimit() * 2;
    jssrcnote *newnotes = (jssrcnote *) cx->realloc_(bce->notes(), newlength);
    if (!newnotes) {
        js_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    bce->current->notes = newnotes;
    bce->current->noteLimit = newlength;
    return JS_TRUE;
}

jssrcnote *
frontend::AddToSrcNoteDelta(JSContext *cx, BytecodeEmitter *bce, jssrcnote *sn, ptrdiff_t delta)
{
    ptrdiff_t base, limit, newdelta, diff;
    int index;

    



    JS_ASSERT(bce->current == &bce->main);
    JS_ASSERT((unsigned) delta < (unsigned) SN_XDELTA_LIMIT);

    base = SN_DELTA(sn);
    limit = SN_IS_XDELTA(sn) ? SN_XDELTA_LIMIT : SN_DELTA_LIMIT;
    newdelta = base + delta;
    if (newdelta < limit) {
        SN_SET_DELTA(sn, newdelta);
    } else {
        index = sn - bce->main.notes;
        if (bce->main.noteCount == bce->main.noteLimit) {
            if (!GrowSrcNotes(cx, bce))
                return NULL;
            sn = bce->main.notes + index;
        }
        diff = bce->main.noteCount - index;
        bce->main.noteCount++;
        memmove(sn + 1, sn, SRCNOTE_SIZE(diff));
        SN_MAKE_XDELTA(sn, delta);
        sn++;
    }
    return sn;
}

static JSBool
SetSrcNoteOffset(JSContext *cx, BytecodeEmitter *bce, unsigned index, unsigned which, ptrdiff_t offset)
{
    jssrcnote *sn;
    ptrdiff_t diff;

    if (size_t(offset) > SN_MAX_OFFSET) {
        ReportStatementTooLarge(cx, bce->sc);
        return JS_FALSE;
    }

    
    sn = &bce->notes()[index];
    JS_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    JS_ASSERT((int) which < js_SrcNoteSpec[SN_TYPE(sn)].arity);
    for (sn++; which; sn++, which--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }

    




    if (offset > (ptrdiff_t)SN_3BYTE_OFFSET_MASK || (*sn & SN_3BYTE_OFFSET_FLAG)) {
        
        if (!(*sn & SN_3BYTE_OFFSET_FLAG)) {
            
            index = sn - bce->notes();

            




            if (bce->noteCount() + 1 >= bce->noteLimit()) {
                if (!GrowSrcNotes(cx, bce))
                    return JS_FALSE;
                sn = bce->notes() + index;
            }
            bce->current->noteCount += 2;

            diff = bce->noteCount() - (index + 3);
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
static uint32_t hist[NBINS];

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
frontend::FinishTakingSrcNotes(JSContext *cx, BytecodeEmitter *bce, jssrcnote *notes)
{
    unsigned prologCount, mainCount, totalCount;
    ptrdiff_t offset, delta;
    jssrcnote *sn;

    JS_ASSERT(bce->current == &bce->main);

    prologCount = bce->prolog.noteCount;
    if (prologCount && bce->prolog.currentLine != bce->firstLine) {
        bce->switchToProlog();
        if (NewSrcNote2(cx, bce, SRC_SETLINE, (ptrdiff_t)bce->firstLine) < 0)
            return false;
        prologCount = bce->prolog.noteCount;
        bce->switchToMain();
    } else {
        






        offset = bce->prologOffset() - bce->prolog.lastNoteOffset;
        JS_ASSERT(offset >= 0);
        if (offset > 0 && bce->main.noteCount != 0) {
            
            sn = bce->main.notes;
            delta = SN_IS_XDELTA(sn)
                    ? SN_XDELTA_MASK - (*sn & SN_XDELTA_MASK)
                    : SN_DELTA_MASK - (*sn & SN_DELTA_MASK);
            if (offset < delta)
                delta = offset;
            for (;;) {
                if (!AddToSrcNoteDelta(cx, bce, sn, delta))
                    return false;
                offset -= delta;
                if (offset == 0)
                    break;
                delta = JS_MIN(offset, SN_XDELTA_MASK);
                sn = bce->main.notes;
            }
        }
    }

    mainCount = bce->main.noteCount;
    totalCount = prologCount + mainCount;
    if (prologCount)
        PodCopy(notes, bce->prolog.notes, prologCount);
    PodCopy(notes + prologCount, bce->main.notes, mainCount);
    SN_MAKE_TERMINATOR(&notes[totalCount]);

    return true;
}

static JSBool
NewTryNote(JSContext *cx, BytecodeEmitter *bce, JSTryNoteKind kind, unsigned stackDepth, size_t start,
           size_t end)
{
    JS_ASSERT((unsigned)(uint16_t)stackDepth == stackDepth);
    JS_ASSERT(start <= end);
    JS_ASSERT((size_t)(uint32_t)start == start);
    JS_ASSERT((size_t)(uint32_t)end == end);

    TryNode *tryNode = cx->tempLifoAlloc().new_<TryNode>();
    if (!tryNode) {
        js_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    tryNode->note.kind = kind;
    tryNode->note.stackDepth = (uint16_t)stackDepth;
    tryNode->note.start = (uint32_t)start;
    tryNode->note.length = (uint32_t)(end - start);
    tryNode->prev = bce->lastTryNode;
    bce->lastTryNode = tryNode;
    bce->ntrynotes++;
    return JS_TRUE;
}

void
frontend::FinishTakingTryNotes(BytecodeEmitter *bce, TryNoteArray *array)
{
    TryNode *tryNode;
    JSTryNote *tn;

    JS_ASSERT(array->length > 0 && array->length == bce->ntrynotes);
    tn = array->vector + array->length;
    tryNode = bce->lastTryNode;
    do {
        *--tn = tryNode->note;
    } while ((tryNode = tryNode->prev) != NULL);
    JS_ASSERT(tn == array->vector);
}











































unsigned
CGObjectList::add(ObjectBox *objbox)
{
    JS_ASSERT(!objbox->emitLink);
    objbox->emitLink = lastbox;
    lastbox = objbox;
    return length++;
}

unsigned
CGObjectList::indexOf(JSObject *obj)
{
    JS_ASSERT(length > 0);
    unsigned index = length - 1;
    for (ObjectBox *box = lastbox; box->object != obj; box = box->emitLink)
        index--;
    return index;
}

void
CGObjectList::finish(ObjectArray *array)
{
    JS_ASSERT(length <= INDEX_LIMIT);
    JS_ASSERT(length == array->length);

    js::HeapPtrObject *cursor = array->vector + array->length;
    ObjectBox *objbox = lastbox;
    do {
        --cursor;
        JS_ASSERT(!*cursor);
        *cursor = objbox->object;
    } while ((objbox = objbox->emitLink) != NULL);
    JS_ASSERT(cursor == array->vector);
}

void
GCConstList::finish(ConstArray *array)
{
    JS_ASSERT(array->length == list.length());
    Value *src = list.begin(), *srcend = list.end();
    HeapValue *dst = array->vector;
    for (; src != srcend; ++src, ++dst)
        *dst = *src;
}





JS_FRIEND_DATA(JSSrcNoteSpec) js_SrcNoteSpec[] = {
    {"null",            0},
    {"if",              0},
    {"if-else",         2},
    {"for",             3},
    {"while",           1},
    {"continue",        0},
    {"decl",            1},
    {"pcdelta",         1},
    {"assignop",        0},
    {"cond",            1},
    {"brace",           1},
    {"hidden",          0},
    {"pcbase",          1},
    {"label",           1},
    {"labelbrace",      1},
    {"endbrace",        0},
    {"break2label",     1},
    {"cont2label",      1},
    {"switch",          2},
    {"funcdef",         1},
    {"catch",           1},
    {"unused",         -1},
    {"newline",         0},
    {"setline",         1},
    {"xdelta",          0},
};

JS_FRIEND_API(unsigned)
js_SrcNoteLength(jssrcnote *sn)
{
    unsigned arity;
    jssrcnote *base;

    arity = (int)js_SrcNoteSpec[SN_TYPE(sn)].arity;
    for (base = sn++; arity; sn++, arity--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }
    return sn - base;
}

JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, unsigned which)
{
    
    JS_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    JS_ASSERT((int) which < js_SrcNoteSpec[SN_TYPE(sn)].arity);
    for (sn++; which; sn++, which--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }
    if (*sn & SN_3BYTE_OFFSET_FLAG) {
        return (ptrdiff_t)(((uint32_t)(sn[0] & SN_3BYTE_OFFSET_MASK) << 16)
                           | (sn[1] << 8)
                           | sn[2]);
    }
    return (ptrdiff_t)*sn;
}
