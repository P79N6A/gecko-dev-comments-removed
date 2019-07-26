










#include "mozilla/DebugOnly.h"
#include "mozilla/FloatingPoint.h"

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>

#include "jstypes.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsautooplen.h"        

#include "ds/LifoAlloc.h"
#include "frontend/BytecodeEmitter.h"
#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "ion/AsmJS.h"
#include "vm/Debugger.h"
#include "vm/RegExpObject.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/SharedContext-inl.h"
#include "vm/Shape-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::frontend;

using mozilla::DebugOnly;

static bool
SetSrcNoteOffset(JSContext *cx, BytecodeEmitter *bce, unsigned index, unsigned which, ptrdiff_t offset);

struct frontend::StmtInfoBCE : public StmtInfoBase
{
    StmtInfoBCE     *down;          
    StmtInfoBCE     *downScope;     

    ptrdiff_t       update;         
    ptrdiff_t       breaks;         
    ptrdiff_t       continues;      

    StmtInfoBCE(JSContext *cx) : StmtInfoBase(cx) {}

    








    ptrdiff_t &gosubs() {
        JS_ASSERT(type == STMT_FINALLY);
        return breaks;
    }

    ptrdiff_t &guardJump() {
        JS_ASSERT(type == STMT_TRY || type == STMT_FINALLY);
        return continues;
    }
};

BytecodeEmitter::BytecodeEmitter(BytecodeEmitter *parent,
                                 Parser<FullParseHandler> *parser, SharedContext *sc,
                                 HandleScript script, HandleScript evalCaller, bool hasGlobalScope,
                                 uint32_t lineNum, bool selfHostingMode)
  : sc(sc),
    parent(parent),
    script(sc->context, script),
    prolog(sc->context, lineNum),
    main(sc->context, lineNum),
    current(&main),
    parser(parser),
    evalCaller(evalCaller),
    topStmt(NULL),
    topScopeStmt(NULL),
    blockChain(sc->context),
    atomIndices(sc->context),
    firstLine(lineNum),
    stackDepth(0), maxStackDepth(0),
    tryNoteList(sc->context),
    arrayCompDepth(0),
    emitLevel(0),
    constList(sc->context),
    typesetCount(0),
    hasSingletons(false),
    emittingForInit(false),
    emittingRunOnceLambda(false),
    hasGlobalScope(hasGlobalScope),
    selfHostingMode(selfHostingMode)
{
}

bool
BytecodeEmitter::init()
{
    return atomIndices.ensureMap(sc->context);
}

static ptrdiff_t
EmitCheck(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t delta)
{
    ptrdiff_t offset = bce->code().length();

    
    if (bce->code().capacity() == 0 && !bce->code().reserve(1024))
        return -1;

    jsbytecode dummy = 0;
    if (!bce->code().appendN(dummy, delta)) {
        js_ReportOutOfMemory(cx);
        return -1;
    }
    return offset;
}

static StaticBlockObject &
CurrentBlock(StmtInfoBCE *topStmt)
{
    JS_ASSERT(topStmt->type == STMT_BLOCK || topStmt->type == STMT_SWITCH);
    JS_ASSERT(topStmt->blockObj->isStaticBlock());
    return *topStmt->blockObj;
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
        ndefs = CurrentBlock(bce->topStmt).slotCount();
    } else if (op == JSOP_ENTERLET0) {
        nuses = ndefs = CurrentBlock(bce->topStmt).slotCount();
    } else if (op == JSOP_ENTERLET1) {
        nuses = ndefs = CurrentBlock(bce->topStmt).slotCount() + 1;
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
    if (offset < 0)
        return -1;

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    UpdateDepth(cx, bce, offset);
    return offset;
}

ptrdiff_t
frontend::Emit2(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1)
{
    ptrdiff_t offset = EmitCheck(cx, bce, 2);
    if (offset < 0)
        return -1;

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    code[1] = op1;
    UpdateDepth(cx, bce, offset);
    return offset;
}

ptrdiff_t
frontend::Emit3(JSContext *cx, BytecodeEmitter *bce, JSOp op, jsbytecode op1,
                    jsbytecode op2)
{
    
    JS_ASSERT(!IsArgOp(op));
    JS_ASSERT(!IsLocalOp(op));

    ptrdiff_t offset = EmitCheck(cx, bce, 3);
    if (offset < 0)
        return -1;

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    code[1] = op1;
    code[2] = op2;
    UpdateDepth(cx, bce, offset);
    return offset;
}

ptrdiff_t
frontend::EmitN(JSContext *cx, BytecodeEmitter *bce, JSOp op, size_t extra)
{
    ptrdiff_t length = 1 + (ptrdiff_t)extra;
    ptrdiff_t offset = EmitCheck(cx, bce, length);
    if (offset < 0)
        return -1;

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    

    



    if (js_CodeSpec[op].nuses >= 0)
        UpdateDepth(cx, bce, offset);

    return offset;
}

static ptrdiff_t
EmitJump(JSContext *cx, BytecodeEmitter *bce, JSOp op, ptrdiff_t off)
{
    ptrdiff_t offset = EmitCheck(cx, bce, 5);
    if (offset < 0)
        return -1;

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    SET_JUMP_OFFSET(code, off);
    UpdateDepth(cx, bce, offset);
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
StatementName(StmtInfoBCE *topStmt)
{
    if (!topStmt)
        return js_script_str;
    return statementName[topStmt->type];
}

static void
ReportStatementTooLarge(JSContext *cx, StmtInfoBCE *topStmt)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEED_DIET,
                         StatementName(topStmt));
}





static ptrdiff_t
EmitBackPatchOp(JSContext *cx, BytecodeEmitter *bce, ptrdiff_t *lastp)
{
    ptrdiff_t offset, delta;

    offset = bce->offset();
    delta = offset - *lastp;
    *lastp = offset;
    JS_ASSERT(delta > 0);
    return EmitJump(cx, bce, JSOP_BACKPATCH, delta);
}


static inline bool
UpdateLineNumberNotes(JSContext *cx, BytecodeEmitter *bce, uint32_t offset)
{
    TokenStream *ts = &bce->parser->tokenStream;
    if (!ts->srcCoords.isOnThisLine(offset, bce->currentLine())) {
        unsigned line = ts->srcCoords.lineNum(offset);
        unsigned delta = line - bce->currentLine();

        










        bce->current->currentLine = line;
        bce->current->lastColumn  = 0;
        if (delta >= (unsigned)(2 + ((line > SN_3BYTE_OFFSET_MASK)<<1))) {
            if (NewSrcNote2(cx, bce, SRC_SETLINE, (ptrdiff_t)line) < 0)
                return false;
        } else {
            do {
                if (NewSrcNote(cx, bce, SRC_NEWLINE) < 0)
                    return false;
            } while (--delta != 0);
        }
    }
    return true;
}


static bool
UpdateSourceCoordNotes(JSContext *cx, BytecodeEmitter *bce, uint32_t offset)
{
    if (!UpdateLineNumberNotes(cx, bce, offset))
        return false;

    uint32_t columnIndex = bce->parser->tokenStream.srcCoords.columnIndex(offset);
    ptrdiff_t colspan = ptrdiff_t(columnIndex) - ptrdiff_t(bce->current->lastColumn);
    if (colspan != 0) {
        if (colspan < 0) {
            colspan += SN_COLSPAN_DOMAIN;
        } else if (colspan >= SN_COLSPAN_DOMAIN / 2) {
            
            
            
            
            
            return true;
        }
        if (NewSrcNote2(cx, bce, SRC_COLSPAN, colspan) < 0)
            return false;
        bce->current->lastColumn = columnIndex;
    }
    return true;
}

static ptrdiff_t
EmitLoopHead(JSContext *cx, BytecodeEmitter *bce, ParseNode *nextpn)
{
    if (nextpn) {
        




        JS_ASSERT_IF(nextpn->isKind(PNK_STATEMENTLIST), nextpn->isArity(PN_LIST));
        if (nextpn->isKind(PNK_STATEMENTLIST) && nextpn->pn_head)
            nextpn = nextpn->pn_head;
        if (!UpdateSourceCoordNotes(cx, bce, nextpn->pn_pos.begin))
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
        if (!UpdateSourceCoordNotes(cx, bce, nextpn->pn_pos.begin))
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
            return false;                                                     \
        CheckTypeSet(cx, bce, op);                                            \
    JS_END_MACRO

#define EMIT_UINT16PAIR_IMM_OP(op, i, j)                                      \
    JS_BEGIN_MACRO                                                            \
        ptrdiff_t off_ = EmitN(cx, bce, op, 2 * UINT16_LEN);                  \
        if (off_ < 0)                                                         \
            return false;                                                     \
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

static bool
FlushPops(JSContext *cx, BytecodeEmitter *bce, int *npops)
{
    JS_ASSERT(*npops != 0);
    if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
        return false;
    EMIT_UINT16_IMM_OP(JSOP_POPN, *npops);
    *npops = 0;
    return true;
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




static bool
EmitNonLocalJumpFixup(JSContext *cx, BytecodeEmitter *bce, StmtInfoBCE *toStmt)
{
    





    int depth = bce->stackDepth;
    int npops = 0;

#define FLUSH_POPS() if (npops && !FlushPops(cx, bce, &npops)) return false

    for (StmtInfoBCE *stmt = bce->topStmt; stmt != toStmt; stmt = stmt->down) {
        switch (stmt->type) {
          case STMT_FINALLY:
            FLUSH_POPS();
            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return false;
            if (EmitBackPatchOp(cx, bce, &stmt->gosubs()) < 0)
                return false;
            break;

          case STMT_WITH:
            
            FLUSH_POPS();
            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return false;
            if (Emit1(cx, bce, JSOP_LEAVEWITH) < 0)
                return false;
            break;

          case STMT_FOR_IN_LOOP:
            FLUSH_POPS();
            if (!PopIterator(cx, bce))
                return false;
            break;

          case STMT_SUBROUTINE:
            



            npops += 2;
            break;

          default:;
        }

        if (stmt->isBlockScope) {
            FLUSH_POPS();
            unsigned blockObjCount = stmt->blockObj->slotCount();
            if (stmt->isForLetBlock) {
                




                JS_ASSERT(stmt->down->type == STMT_FOR_IN_LOOP);
                stmt = stmt->down;
                if (stmt == toStmt)
                    break;
                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                    return false;
                if (Emit1(cx, bce, JSOP_LEAVEFORLETIN) < 0)
                    return false;
                if (!PopIterator(cx, bce))
                    return false;
                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                    return false;
                EMIT_UINT16_IMM_OP(JSOP_POPN, blockObjCount);
            } else {
                
                if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                    return false;
                EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, blockObjCount);
            }
        }
    }

    FLUSH_POPS();
    bce->stackDepth = depth;
    return true;

#undef FLUSH_POPS
}

static const jsatomid INVALID_ATOMID = -1;

static ptrdiff_t
EmitGoto(JSContext *cx, BytecodeEmitter *bce, StmtInfoBCE *toStmt, ptrdiff_t *lastp,
         SrcNoteType noteType = SRC_NULL)
{
    if (!EmitNonLocalJumpFixup(cx, bce, toStmt))
        return -1;

    if (noteType != SRC_NULL) {
        if (NewSrcNote(cx, bce, noteType) < 0)
            return -1;
    }

    return EmitBackPatchOp(cx, bce, lastp);
}

static bool
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
    return true;
}

#define SET_STATEMENT_TOP(stmt, top)                                          \
    ((stmt)->update = (top), (stmt)->breaks = (stmt)->continues = (-1))

static void
PushStatementBCE(BytecodeEmitter *bce, StmtInfoBCE *stmt, StmtType type, ptrdiff_t top)
{
    SET_STATEMENT_TOP(stmt, top);
    PushStatement(bce, stmt, type);
}





static JSObject *
EnclosingStaticScope(BytecodeEmitter *bce)
{
    if (bce->blockChain)
        return bce->blockChain;

    if (!bce->sc->isFunctionBox()) {
        JS_ASSERT(!bce->parent);
        return NULL;
    }

    return bce->sc->asFunctionBox()->function();
}


static void
PushBlockScopeBCE(BytecodeEmitter *bce, StmtInfoBCE *stmt, StaticBlockObject &blockObj,
                  ptrdiff_t top)
{
    PushStatementBCE(bce, stmt, STMT_BLOCK, top);
    blockObj.initEnclosingStaticScope(EnclosingStaticScope(bce));
    FinishPushBlockScope(bce, stmt, blockObj);
}



static bool
PopStatementBCE(JSContext *cx, BytecodeEmitter *bce)
{
    StmtInfoBCE *stmt = bce->topStmt;
    if (!stmt->isTrying() &&
        (!BackPatch(cx, bce, stmt->breaks, bce->code().end(), JSOP_GOTO) ||
         !BackPatch(cx, bce, stmt->continues, bce->code(stmt->update), JSOP_GOTO)))
    {
        return false;
    }
    FinishPopStatement(bce);
    return true;
}

static bool
EmitIndex32(JSContext *cx, JSOp op, uint32_t index, BytecodeEmitter *bce)
{
    const size_t len = 1 + UINT32_INDEX_LEN;
    JS_ASSERT(len == size_t(js_CodeSpec[op].length));
    ptrdiff_t offset = EmitCheck(cx, bce, len);
    if (offset < 0)
        return false;

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    SET_UINT32_INDEX(code, index);
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

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    SET_UINT32_INDEX(code, index);
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, op);
    return true;
}

static bool
EmitAtomOp(JSContext *cx, JSAtom *atom, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

    if (op == JSOP_GETPROP && atom == cx->names().length) {
        
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

    jsbytecode *code = bce->code(offset);
    code[0] = jsbytecode(op);
    SET_UINT32_INDEX(code, index);
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, op);
    return true;
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
    if (bce->blockChain)
        maybeBlockIndex = bce->objectList.indexOf(bce->blockChain);

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
    CheckTypeSet(cx, bce, op);
    return true;
}

static unsigned
ClonedBlockDepth(BytecodeEmitter *bce)
{
    unsigned clonedBlockDepth = 0;
    for (StaticBlockObject *b = bce->blockChain; b; b = b->enclosingBlock()) {
        if (b->needsClone())
            ++clonedBlockDepth;
    }

    return clonedBlockDepth;
}

static uint16_t
AliasedNameToSlot(HandleScript script, PropertyName *name)
{
    



    unsigned slot = CallObject::RESERVED_SLOTS;
    for (BindingIter bi(script); ; bi++) {
        if (bi->aliased()) {
            if (bi->name() == name)
                return slot;
            slot++;
        }
    }

    return 0;
}

static bool
EmitAliasedVarOp(JSContext *cx, JSOp op, ParseNode *pn, BytecodeEmitter *bce)
{
    unsigned skippedScopes = 0;
    BytecodeEmitter *bceOfDef = bce;
    if (pn->isUsed()) {
        




        for (unsigned i = pn->pn_cookie.level(); i; i--) {
            skippedScopes += ClonedBlockDepth(bceOfDef);
            JSFunction *funOfDef = bceOfDef->sc->asFunctionBox()->function();
            if (funOfDef->isHeavyweight()) {
                skippedScopes++;
                if (funOfDef->isNamedLambda())
                    skippedScopes++;
            }
            bceOfDef = bceOfDef->parent;
        }
    } else {
        JS_ASSERT(pn->isDefn());
        JS_ASSERT(pn->pn_cookie.level() == bce->script->staticLevel);
    }

    ScopeCoordinate sc;
    if (IsArgOp(pn->getOp())) {
        sc.hops = skippedScopes + ClonedBlockDepth(bceOfDef);
        sc.slot = AliasedNameToSlot(bceOfDef->script, pn->name());
    } else {
        JS_ASSERT(IsLocalOp(pn->getOp()) || pn->isKind(PNK_FUNCTION));
        unsigned local = pn->pn_cookie.slot();
        if (local < bceOfDef->script->bindings.numVars()) {
            sc.hops = skippedScopes + ClonedBlockDepth(bceOfDef);
            sc.slot = AliasedNameToSlot(bceOfDef->script, pn->name());
        } else {
            unsigned depth = local - bceOfDef->script->bindings.numVars();
            StaticBlockObject *b = bceOfDef->blockChain;
            while (!b->containsVarAtDepth(depth)) {
                if (b->needsClone())
                    skippedScopes++;
                b = b->enclosingBlock();
            }
            sc.hops = skippedScopes;
            sc.slot = b->localIndexToSlot(bceOfDef->script->bindings, local);
        }
    }

    return EmitAliasedVarOp(cx, op, sc, bce);
}

static bool
EmitVarOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(pn->isKind(PNK_FUNCTION) || pn->isKind(PNK_NAME));
    JS_ASSERT_IF(pn->isKind(PNK_NAME), IsArgOp(op) || IsLocalOp(op));
    JS_ASSERT(!pn->pn_cookie.isFree());

    if (!bce->isAliasedName(pn)) {
        JS_ASSERT(pn->isUsed() || pn->isDefn());
        JS_ASSERT_IF(pn->isUsed(), pn->pn_cookie.level() == 0);
        JS_ASSERT_IF(pn->isDefn(), pn->pn_cookie.level() == bce->script->staticLevel);
        return EmitUnaliasedVarOp(cx, op, pn->pn_cookie.slot(), bce);
    }

    switch (op) {
      case JSOP_GETARG: case JSOP_GETLOCAL: op = JSOP_GETALIASEDVAR; break;
      case JSOP_SETARG: case JSOP_SETLOCAL: op = JSOP_SETALIASEDVAR; break;
      case JSOP_CALLARG: case JSOP_CALLLOCAL: op = JSOP_CALLALIASEDVAR; break;
      case JSOP_INCARG: case JSOP_INCLOCAL: op = JSOP_INCALIASEDVAR; break;
      case JSOP_ARGINC: case JSOP_LOCALINC: op = JSOP_ALIASEDVARINC; break;
      case JSOP_DECARG: case JSOP_DECLOCAL: op = JSOP_DECALIASEDVAR; break;
      case JSOP_ARGDEC: case JSOP_LOCALDEC: op = JSOP_ALIASEDVARDEC; break;
      default: JS_NOT_REACHED("unexpected var op");
    }

    return EmitAliasedVarOp(cx, op, pn, bce);
}

static bool
EmitVarIncDec(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    JS_ASSERT(pn->isKind(PNK_NAME));
    JS_ASSERT(IsArgOp(op) || IsLocalOp(op));
    JS_ASSERT(js_CodeSpec[op].format & (JOF_INC | JOF_DEC));
    JS_ASSERT(!pn->pn_cookie.isFree());

    if (!EmitVarOp(cx, pn, op, bce))
        return false;

    
    bce->stackDepth--;

    int start = bce->offset();

    const JSCodeSpec &cs = js_CodeSpec[op];
    bool post = (cs.format & JOF_POST);
    JSOp binop = (cs.format & JOF_INC) ? JSOP_ADD : JSOP_SUB;
    JSOp getOp = IsLocalOp(op) ? JSOP_GETLOCAL : JSOP_GETARG;
    JSOp setOp = IsLocalOp(op) ? JSOP_SETLOCAL : JSOP_SETARG;

    if (!EmitVarOp(cx, pn, getOp, bce))                      
        return false;
    if (Emit1(cx, bce, JSOP_POS) < 0)                        
        return false;
    if (post && Emit1(cx, bce, JSOP_DUP) < 0)                
        return false;
    if (Emit1(cx, bce, JSOP_ONE) < 0)                        
        return false;
    if (Emit1(cx, bce, binop) < 0)                           
        return false;
    if (!EmitVarOp(cx, pn, setOp, bce))                      
        return false;
    if (post && Emit1(cx, bce, JSOP_POP) < 0)                
        return false;

    UpdateDecomposeLength(bce, start);
    return true;
}

bool
BytecodeEmitter::isAliasedName(ParseNode *pn)
{
    Definition *dn = pn->resolve();
    JS_ASSERT(dn->isDefn());
    JS_ASSERT(!dn->isPlaceholder());
    JS_ASSERT(dn->isBound());

    
    if (dn->pn_cookie.level() != script->staticLevel)
        return true;

    switch (dn->kind()) {
      case Definition::LET:
        




        return dn->isClosed() || sc->bindingsAccessedDynamically();
      case Definition::ARG:
        









        return script->formalIsAliased(pn->pn_cookie.slot());
      case Definition::VAR:
      case Definition::CONST:
        return script->varIsAliased(pn->pn_cookie.slot());
      case Definition::PLACEHOLDER:
      case Definition::NAMED_LAMBDA:
        JS_NOT_REACHED("unexpected dn->kind");
    }
    return false;
}









static int
AdjustBlockSlot(JSContext *cx, BytecodeEmitter *bce, int slot)
{
    JS_ASSERT((unsigned) slot < bce->maxStackDepth);
    if (bce->sc->isFunctionBox()) {
        slot += bce->script->bindings.numVars();
        if ((unsigned) slot >= SLOTNO_LIMIT) {
            bce->reportError(NULL, JSMSG_TOO_MANY_LOCALS);
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

        blockObj->setAliased(i, bce->isAliasedName(dn));
    }

    return true;
}



























static bool
TryConvertToGname(BytecodeEmitter *bce, ParseNode *pn, JSOp *op)
{
    if (bce->selfHostingMode) {
        switch (*op) {
          case JSOP_NAME:     *op = JSOP_GETINTRINSIC; break;
          case JSOP_SETNAME:  *op = JSOP_SETINTRINSIC; break;
          
          default: JS_NOT_REACHED("intrinsic");
        }
        return true;
    }
    if (bce->script->compileAndGo &&
        bce->hasGlobalScope &&
        !(bce->sc->isFunctionBox() && bce->sc->asFunctionBox()->mightAliasLocals()) &&
        !pn->isDeoptimized() &&
        !bce->sc->strict)
    {
        
        
        switch (*op) {
          case JSOP_NAME:     *op = JSOP_GETGNAME; break;
          case JSOP_SETNAME:  *op = JSOP_SETGNAME; break;
          case JSOP_INCNAME:  *op = JSOP_INCGNAME; break;
          case JSOP_NAMEINC:  *op = JSOP_GNAMEINC; break;
          case JSOP_DECNAME:  *op = JSOP_DECGNAME; break;
          case JSOP_NAMEDEC:  *op = JSOP_GNAMEDEC; break;
          case JSOP_SETCONST:
            
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
    JS_ASSERT(pn->isKind(PNK_NAME));

    JS_ASSERT_IF(pn->isKind(PNK_FUNCTION), pn->isBound());

    
    if (pn->isBound() || pn->isDeoptimized())
        return true;

    
    JSOp op = pn->getOp();
    JS_ASSERT(op != JSOP_CALLEE);
    JS_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

    



    Definition *dn;
    if (pn->isUsed()) {
        JS_ASSERT(pn->pn_cookie.isFree());
        dn = pn->pn_lexdef;
        JS_ASSERT(dn->isDefn());
        pn->pn_dflags |= (dn->pn_dflags & PND_CONST);
    } else if (pn->isDefn()) {
        dn = (Definition *) pn;
    } else {
        return true;
    }

    








    switch (op) {
      case JSOP_NAME:
      case JSOP_SETCONST:
        break;
      default:
        if (pn->isConst()) {
            if (bce->sc->needStrictChecks()) {
                JSAutoByteString name;
                if (!js_AtomToPrintableString(cx, pn->pn_atom, &name) ||
                    !bce->reportStrictModeError(pn, JSMSG_READ_ONLY, name.ptr()))
                {
                    return false;
                }
            }
            pn->setOp(op = JSOP_NAME);
        }
    }

    if (dn->pn_cookie.isFree()) {
        if (HandleScript caller = bce->evalCaller) {
            JS_ASSERT(bce->script->compileAndGo);

            



            if (bce->emittingForInit)
                return true;

            



            if (!caller->functionOrCallerFunction() && TryConvertToGname(bce, pn, &op)) {
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

    













    JS_ASSERT(!pn->isDefn());
    JS_ASSERT(pn->isUsed());
    JS_ASSERT(pn->pn_lexdef);
    JS_ASSERT(pn->pn_cookie.isFree());

    




    switch (dn->kind()) {
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
      case Definition::CONST:
      case Definition::LET:
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
        break;

      case Definition::NAMED_LAMBDA: {
        JS_ASSERT(dn->isOp(JSOP_CALLEE));
        JS_ASSERT(op != JSOP_CALLEE);

        



        if (dn->pn_cookie.level() != bce->script->staticLevel)
            return true;

        RootedFunction fun(cx, bce->sc->asFunctionBox()->function());
        JS_ASSERT(fun->isLambda());
        JS_ASSERT(pn->pn_atom == fun->atom());

        























        if (!fun->isHeavyweight()) {
            op = JSOP_CALLEE;
            pn->pn_dflags |= PND_CONST;
        }

        pn->setOp(op);
        pn->pn_dflags |= PND_BOUND;
        return true;
      }

      case Definition::PLACEHOLDER:
        return true;
    }

    




    unsigned skip = bce->script->staticLevel - dn->pn_cookie.level();
    JS_ASSERT_IF(skip, dn->isClosed());

    








    if (skip) {
        BytecodeEmitter *bceSkipped = bce;
        for (unsigned i = 0; i < skip; i++)
            bceSkipped = bceSkipped->parent;
        if (!bceSkipped->sc->isFunctionBox())
            return true;
    }

    JS_ASSERT(!pn->isOp(op));
    pn->setOp(op);
    if (!pn->pn_cookie.set(bce->sc->context, skip, dn->pn_cookie.slot()))
        return false;

    pn->pn_dflags |= PND_BOUND;
    return true;
}













static bool
CheckSideEffects(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, bool *answer)
{
    if (!pn || *answer)
        return true;

    switch (pn->getArity()) {
      case PN_CODE:
        






        MOZ_ASSERT(*answer == false);
        return true;

      case PN_LIST:
        if (pn->isOp(JSOP_NOP) || pn->isOp(JSOP_OR) || pn->isOp(JSOP_AND) ||
            pn->isOp(JSOP_STRICTEQ) || pn->isOp(JSOP_STRICTNE)) {
            



            bool ok = true;
            for (ParseNode *pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next)
                ok &= CheckSideEffects(cx, bce, pn2, answer);
            return ok;
        }

        if (pn->isKind(PNK_GENEXP)) {
            
            MOZ_ASSERT(*answer == false);
            return true;
        }

        













        *answer = true;
        return true;

      case PN_TERNARY:
        return CheckSideEffects(cx, bce, pn->pn_kid1, answer) &&
               CheckSideEffects(cx, bce, pn->pn_kid2, answer) &&
               CheckSideEffects(cx, bce, pn->pn_kid3, answer);

      case PN_BINARY:
        if (pn->isAssignment()) {
            








            ParseNode *pn2 = pn->pn_left;
            if (!pn2->isKind(PNK_NAME)) {
                *answer = true;
            } else {
                if (!BindNameToSlot(cx, bce, pn2))
                    return false;
                if (!CheckSideEffects(cx, bce, pn->pn_right, answer))
                    return false;
                if (!*answer && (!pn->isOp(JSOP_NOP) || !pn2->isConst()))
                    *answer = true;
            }
            return true;
        }

        if (pn->isOp(JSOP_OR) || pn->isOp(JSOP_AND) || pn->isOp(JSOP_STRICTEQ) ||
            pn->isOp(JSOP_STRICTNE)) {
            



            return CheckSideEffects(cx, bce, pn->pn_left, answer) &&
                   CheckSideEffects(cx, bce, pn->pn_right, answer);
        }

        



        *answer = true;
        return true;

      case PN_UNARY:
        switch (pn->getKind()) {
          case PNK_DELETE:
          {
            ParseNode *pn2 = pn->pn_kid;
            switch (pn2->getKind()) {
              case PNK_NAME:
                if (!BindNameToSlot(cx, bce, pn2))
                    return false;
                if (pn2->isConst()) {
                    MOZ_ASSERT(*answer == false);
                    return true;
                }
                
              case PNK_DOT:
              case PNK_CALL:
              case PNK_ELEM:
                
                *answer = true;
                return true;
              default:
                return CheckSideEffects(cx, bce, pn2, answer);
            }
            MOZ_NOT_REACHED("We have a returning default case");
            return false;
          }

          case PNK_TYPEOF:
          case PNK_VOID:
          case PNK_NOT:
          case PNK_BITNOT:
            if (pn->isOp(JSOP_NOT)) {
                
                return CheckSideEffects(cx, bce, pn->pn_kid, answer);
            }
            

          default:
            





            *answer = true;
            return true;
        }
        MOZ_NOT_REACHED("We have a returning default case");
        return false;

      case PN_NAME:
        




        if (pn->isKind(PNK_NAME) && !pn->isOp(JSOP_NOP)) {
            if (!BindNameToSlot(cx, bce, pn))
                return false;
            if (!pn->isOp(JSOP_CALLEE) && pn->pn_cookie.isFree()) {
                




                *answer = true;
            }
        }
        if (pn->isKind(PNK_DOT)) {
            
            *answer = true;
        }
        return CheckSideEffects(cx, bce, pn->maybeExpr(), answer);

      case PN_NULLARY:
        if (pn->isKind(PNK_DEBUGGER))
            *answer = true;
        return true;
    }
    return true;
}

bool
BytecodeEmitter::isInLoop()
{
    for (StmtInfoBCE *stmt = topStmt; stmt; stmt = stmt->down) {
        if (stmt->isLoop())
            return true;
    }
    return false;
}

bool
BytecodeEmitter::checkSingletonContext()
{
    if (!script->compileAndGo || sc->isFunctionBox() || isInLoop())
        return false;
    hasSingletons = true;
    return true;
}

bool
BytecodeEmitter::needsImplicitThis()
{
    if (!script->compileAndGo)
        return true;

    if (sc->isModuleBox()) {
        
        return false;
    } if (sc->isFunctionBox()) {
        if (sc->asFunctionBox()->inWith)
            return true;
    } else {
        JSObject *scope = sc->asGlobalSharedContext()->scopeChain();
        while (scope) {
            if (scope->isWith())
                return true;
            scope = scope->enclosingScope();
        }
    }

    for (StmtInfoBCE *stmt = topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == STMT_WITH)
            return true;
    }
    return false;
}

void
BytecodeEmitter::tellDebuggerAboutCompiledScript(JSContext *cx)
{
    RootedFunction function(cx, script->function());
    CallNewScriptHook(cx, script, function);
    if (!parent) {
        GlobalObject *compileAndGoGlobal = NULL;
        if (script->compileAndGo)
            compileAndGoGlobal = &script->global();
        Debugger::onNewScript(cx, script, compileAndGoGlobal);
    }
}

bool
BytecodeEmitter::reportError(ParseNode *pn, unsigned errorNumber, ...)
{
    TokenPos pos = pn ? pn->pn_pos : tokenStream()->currentToken().pos;

    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream()->reportCompileErrorNumberVA(pos.begin, JSREPORT_ERROR,
                                                            errorNumber, args);
    va_end(args);
    return result;
}

bool
BytecodeEmitter::reportStrictWarning(ParseNode *pn, unsigned errorNumber, ...)
{
    TokenPos pos = pn ? pn->pn_pos : tokenStream()->currentToken().pos;

    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream()->reportStrictWarningErrorNumberVA(pos.begin, errorNumber, args);
    va_end(args);
    return result;
}

bool
BytecodeEmitter::reportStrictModeError(ParseNode *pn, unsigned errorNumber, ...)
{
    TokenPos pos = pn ? pn->pn_pos : tokenStream()->currentToken().pos;

    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream()->reportStrictModeErrorNumberVA(pos.begin, sc->strict,
                                                               errorNumber, args);
    va_end(args);
    return result;
}

static bool
EmitNameOp(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, bool callContext)
{
    JSOp op;

    if (!BindNameToSlot(cx, bce, pn))
        return false;
    op = pn->getOp();

    if (callContext) {
        switch (op) {
          case JSOP_NAME:
            op = JSOP_CALLNAME;
            break;
          case JSOP_GETINTRINSIC:
            op = JSOP_CALLINTRINSIC;
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

    return true;
}

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
EmitPropOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce, bool callContext)
{
    ParseNode *pn2, *pndot, *pnup, *pndown;
    ptrdiff_t top;

    JS_ASSERT(pn->isArity(PN_NAME));
    pn2 = pn->maybeExpr();

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
            
            if (!EmitAtomOp(cx, pndot, pndot->getOp(), bce))
                return false;

            
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

bool
frontend::EmitElemOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce)
{
    ParseNode *left, *right;

    if (pn->isArity(PN_NAME)) {
        






        left = pn->maybeExpr();
        if (!left) {
            left = NullaryNode::create(PNK_STRING, &bce->parser->handler);
            if (!left)
                return false;
            left->setOp(JSOP_BINDNAME);
            left->pn_pos = pn->pn_pos;
            left->pn_atom = pn->pn_atom;
        }
        right = NullaryNode::create(PNK_STRING, &bce->parser->handler);
        if (!right)
            return false;
        right->setOp(JSOP_STRING);
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

    if (!EmitTree(cx, bce, right))
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

static bool
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
                return false;
            pc = bce->code(off);
            SET_UINT24(pc, u);
        } else {
            off = EmitN(cx, bce, JSOP_INT32, 4);
            if (off < 0)
                return false;
            pc = bce->code(off);
            SET_INT32(pc, ival);
        }
        return true;
    }

    if (!bce->constList.append(DoubleValue(dval)))
        return false;

    return EmitIndex32(cx, JSOP_DOUBLE, bce->constList.length() - 1, bce);
}

static inline void
SetJumpOffsetAt(BytecodeEmitter *bce, ptrdiff_t off)
{
    SET_JUMP_OFFSET(bce->code(off), bce->offset() - off);
}






MOZ_NEVER_INLINE static bool
EmitSwitch(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JSOp switchOp;
    bool hasDefault;
    ptrdiff_t top, off, defaultOffset;
    ParseNode *pn2, *pn3, *pn4;
    int32_t low, high;
    int noteIndex;
    size_t switchSize;
    jsbytecode *pc;
    StmtInfoBCE stmtInfo(cx);

    
    switchOp = JSOP_TABLESWITCH;
    hasDefault = false;
    defaultOffset = -1;

    pn2 = pn->pn_right;
#if JS_HAS_BLOCK_SCOPE
    



    uint32_t blockObjCount = 0;
    if (pn2->isKind(PNK_LEXICALSCOPE)) {
        blockObjCount = pn2->pn_objbox->object->asStaticBlock().slotCount();
        for (uint32_t i = 0; i < blockObjCount; ++i) {
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return false;
        }
    }
#endif

    
    if (!EmitTree(cx, bce, pn->pn_left))
        return false;

#if JS_HAS_BLOCK_SCOPE
    if (pn2->isKind(PNK_LEXICALSCOPE)) {
        PushBlockScopeBCE(bce, &stmtInfo, pn2->pn_objbox->object->asStaticBlock(), -1);
        stmtInfo.type = STMT_SWITCH;
        if (!EmitEnterBlock(cx, bce, pn2, JSOP_ENTERLET1))
            return false;
    }
#endif

    
    top = bce->offset();
#if !JS_HAS_BLOCK_SCOPE
    PushStatementBCE(bce, &stmtInfo, STMT_SWITCH, top);
#else
    if (pn2->isKind(PNK_STATEMENTLIST)) {
        PushStatementBCE(bce, &stmtInfo, STMT_SWITCH, top);
    } else {
        



        stmtInfo.update = top = bce->offset();

        
        pn2 = pn2->expr();
    }
#endif

    uint32_t caseCount = pn2->pn_count;
    uint32_t tableLength = 0;
    ScopedJSFreePtr<ParseNode*> table(NULL);

    if (caseCount > JS_BIT(16)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_TOO_MANY_CASES);
        return false;
    }

    if (caseCount == 0 ||
        (caseCount == 1 &&
         (hasDefault = (pn2->pn_head->isKind(PNK_DEFAULT))))) {
        caseCount = 0;
        low = 0;
        high = -1;
    } else {
        bool ok = true;
#define INTMAP_LENGTH   256
        jsbitmap intmap_space[INTMAP_LENGTH];
        jsbitmap *intmap = NULL;
        int32_t intmap_bitlen = 0;

        low  = JSVAL_INT_MAX;
        high = JSVAL_INT_MIN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->isKind(PNK_DEFAULT)) {
                hasDefault = true;
                caseCount--;    
                continue;
            }

            JS_ASSERT(pn3->isKind(PNK_CASE));
            if (switchOp == JSOP_CONDSWITCH)
                continue;

            JS_ASSERT(switchOp == JSOP_TABLESWITCH);

            pn4 = pn3->pn_left;

            if (pn4->getKind() != PNK_NUMBER) {
                switchOp = JSOP_CONDSWITCH;
                continue;
            }

            int32_t i;
            if (!MOZ_DOUBLE_IS_INT32(pn4->pn_dval, &i)) {
                switchOp = JSOP_CONDSWITCH;
                continue;
            }

            if ((unsigned)(i + (int)JS_BIT(15)) >= (unsigned)JS_BIT(16)) {
                switchOp = JSOP_CONDSWITCH;
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
                    intmap = cx->pod_malloc<jsbitmap>(JS_BIT(16) >> JS_BITS_PER_WORD_LOG2);
                    if (!intmap) {
                        JS_ReportOutOfMemory(cx);
                        return false;
                    }
                }
                memset(intmap, 0, intmap_bitlen >> JS_BITS_PER_BYTE_LOG2);
            }
            if (JS_TEST_BIT(intmap, i)) {
                switchOp = JSOP_CONDSWITCH;
                continue;
            }
            JS_SET_BIT(intmap, i);
        }

        if (intmap && intmap != intmap_space)
            js_free(intmap);
        if (!ok)
            return false;

        



        if (switchOp == JSOP_TABLESWITCH) {
            tableLength = (uint32_t)(high - low + 1);
            if (tableLength >= JS_BIT(16) || tableLength > 2 * caseCount)
                switchOp = JSOP_CONDSWITCH;
        }
    }

    



    if (switchOp == JSOP_CONDSWITCH) {
        
        switchSize = 0;
        noteIndex = NewSrcNote3(cx, bce, SRC_CONDSWITCH, 0, 0);
    } else {
        JS_ASSERT(switchOp == JSOP_TABLESWITCH);

        
        switchSize = (size_t)(JUMP_OFFSET_LEN * (3 + tableLength));
        noteIndex = NewSrcNote2(cx, bce, SRC_TABLESWITCH, 0);
    }
    if (noteIndex < 0)
        return false;

    
    if (EmitN(cx, bce, switchOp, switchSize) < 0)
        return false;

    off = -1;
    if (switchOp == JSOP_CONDSWITCH) {
        int caseNoteIndex = -1;
        bool beforeCases = true;

        
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            pn4 = pn3->pn_left;
            if (pn4 && !EmitTree(cx, bce, pn4))
                return false;
            if (caseNoteIndex >= 0) {
                
                if (!SetSrcNoteOffset(cx, bce, (unsigned)caseNoteIndex, 0, bce->offset() - off))
                    return false;
            }
            if (!pn4) {
                JS_ASSERT(pn3->isKind(PNK_DEFAULT));
                continue;
            }
            caseNoteIndex = NewSrcNote2(cx, bce, SRC_NEXTCASE, 0);
            if (caseNoteIndex < 0)
                return false;
            off = EmitJump(cx, bce, JSOP_CASE, 0);
            if (off < 0)
                return false;
            pn3->pn_offset = off;
            if (beforeCases) {
                unsigned noteCount, noteCountDelta;

                
                noteCount = bce->notes().length();
                if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 1, off - top))
                    return false;
                noteCountDelta = bce->notes().length() - noteCount;
                if (noteCountDelta != 0)
                    caseNoteIndex += noteCountDelta;
                beforeCases = false;
            }
        }

        





        if (!hasDefault &&
            caseNoteIndex >= 0 &&
            !SetSrcNoteOffset(cx, bce, (unsigned)caseNoteIndex, 0, bce->offset() - off))
        {
            return false;
        }

        
        defaultOffset = EmitJump(cx, bce, JSOP_DEFAULT, 0);
        if (defaultOffset < 0)
            return false;
    } else {
        JS_ASSERT(switchOp == JSOP_TABLESWITCH);
        pc = bce->code(top + JUMP_OFFSET_LEN);

        
        SET_JUMP_OFFSET(pc, low);
        pc += JUMP_OFFSET_LEN;
        SET_JUMP_OFFSET(pc, high);
        pc += JUMP_OFFSET_LEN;

        



        if (tableLength != 0) {
            table = cx->pod_calloc<ParseNode*>(tableLength);
            if (!table)
                return false;
            for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                if (pn3->isKind(PNK_DEFAULT))
                    continue;

                JS_ASSERT(pn3->isKind(PNK_CASE));

                pn4 = pn3->pn_left;
                JS_ASSERT(pn4->getKind() == PNK_NUMBER);

                int32_t i = int32_t(pn4->pn_dval);
                JS_ASSERT(double(i) == pn4->pn_dval);

                i -= low;
                JS_ASSERT(uint32_t(i) < tableLength);
                table[i] = pn3;
            }
        }
    }

    
    for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
        if (switchOp == JSOP_CONDSWITCH && !pn3->isKind(PNK_DEFAULT))
            SetJumpOffsetAt(bce, pn3->pn_offset);
        pn4 = pn3->pn_right;
        if (!EmitTree(cx, bce, pn4))
            return false;
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
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, off))
        return false;

    if (switchOp == JSOP_TABLESWITCH) {
        
        pc += 2 * JUMP_OFFSET_LEN;

        
        for (uint32_t i = 0; i < tableLength; i++) {
            pn3 = table[i];
            off = pn3 ? pn3->pn_offset - top : 0;
            SET_JUMP_OFFSET(pc, off);
            pc += JUMP_OFFSET_LEN;
        }
    }

    if (!PopStatementBCE(cx, bce))
        return false;

#if JS_HAS_BLOCK_SCOPE
    if (pn->pn_right->isKind(PNK_LEXICALSCOPE))
        EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, blockObjCount);
#endif

    return true;
}

bool
frontend::EmitFunctionScript(JSContext *cx, BytecodeEmitter *bce, ParseNode *body)
{
    






    FunctionBox *funbox = bce->sc->asFunctionBox();
    if (funbox->argumentsHasLocalBinding()) {
        JS_ASSERT(bce->offset() == 0);  
        bce->switchToProlog();
        if (Emit1(cx, bce, JSOP_ARGUMENTS) < 0)
            return false;
        InternalBindingsHandle bindings(bce->script, &bce->script->bindings);
        unsigned varIndex = Bindings::argumentsVarIndex(cx, bindings);
        if (bce->script->varIsAliased(varIndex)) {
            ScopeCoordinate sc;
            sc.hops = 0;
            sc.slot = AliasedNameToSlot(bce->script, cx->names().arguments);
            if (!EmitAliasedVarOp(cx, JSOP_SETALIASEDVAR, sc, bce))
                return false;
        } else {
            if (!EmitUnaliasedVarOp(cx, JSOP_SETLOCAL, varIndex, bce))
                return false;
        }
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        bce->switchToMain();
    }

    if (funbox->isGenerator()) {
        bce->switchToProlog();
        if (Emit1(cx, bce, JSOP_GENERATOR) < 0)
            return false;
        bce->switchToMain();
    }

    if (!EmitTree(cx, bce, body))
        return false;

    



    if (Emit1(cx, bce, JSOP_STOP) < 0)
        return false;

    if (!JSScript::fullyInitFromEmitter(cx, bce->script, bce))
        return false;

    



    if (bce->parent && bce->parent->emittingRunOnceLambda)
        bce->script->treatAsRunOnce = true;

    





    bool singleton =
        cx->typeInferenceEnabled() &&
        bce->script->compileAndGo &&
        bce->parent &&
        (bce->parent->checkSingletonContext() ||
         (!bce->parent->isInLoop() &&
          bce->parent->parent &&
          bce->parent->parent->emittingRunOnceLambda));

    
    RootedFunction fun(cx, bce->script->function());
    JS_ASSERT(fun->isInterpreted());
    JS_ASSERT(!fun->hasScript());
    fun->setScript(bce->script);
    if (!JSFunction::setTypeForScriptedFunction(cx, fun, singleton))
        return false;

    bce->tellDebuggerAboutCompiledScript(cx);

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
        (!bce->sc->isFunctionBox() || bce->sc->asFunctionBox()->function()->isHeavyweight()))
    {
        bce->switchToProlog();
        if (!UpdateSourceCoordNotes(cx, bce, pn->pn_pos.begin))
            return false;
        if (!EmitIndexOp(cx, prologOp, atomIndex, bce))
            return false;
        bce->switchToMain();
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

typedef bool
(*DestructuringDeclEmitter)(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn);

static bool
EmitDestructuringDecl(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_NAME));
    if (!BindNameToSlot(cx, bce, pn))
        return false;

    JS_ASSERT(!pn->isOp(JSOP_CALLEE));
    return MaybeEmitVarDecl(cx, bce, prologOp, pn, NULL);
}

static bool
EmitDestructuringDecls(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn)
{
    ParseNode *pn2, *pn3;
    DestructuringDeclEmitter emitter;

    if (pn->isKind(PNK_ARRAY)) {
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            if (pn2->isKind(PNK_COMMA))
                continue;
            emitter = (pn2->isKind(PNK_NAME))
                      ? EmitDestructuringDecl
                      : EmitDestructuringDecls;
            if (!emitter(cx, bce, prologOp, pn2))
                return false;
        }
    } else {
        JS_ASSERT(pn->isKind(PNK_OBJECT));
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            pn3 = pn2->pn_right;
            emitter = pn3->isKind(PNK_NAME) ? EmitDestructuringDecl : EmitDestructuringDecls;
            if (!emitter(cx, bce, prologOp, pn3))
                return false;
        }
    }
    return true;
}

static bool
EmitDestructuringOpsHelper(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn,
                           VarEmitOption emitOption);













static bool
EmitDestructuringLHS(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, VarEmitOption emitOption)
{
    JS_ASSERT(emitOption != DefineVars);

    





    if (pn->isKind(PNK_ARRAY) || pn->isKind(PNK_OBJECT)) {
        if (!EmitDestructuringOpsHelper(cx, bce, pn, emitOption))
            return false;
        if (emitOption == InitializeVars) {
            



            if (Emit1(cx, bce, JSOP_POP) < 0)
                return false;
        }
    } else {
        if (emitOption == PushInitialValues) {
            



            JS_ASSERT(pn->getOp() == JSOP_GETLOCAL);
            JS_ASSERT(pn->pn_dflags & PND_BOUND);
            return true;
        }

        

        if (pn->isKind(PNK_NAME)) {
            if (!BindNameToSlot(cx, bce, pn))
                return false;

            
            if (pn->isConst() && !pn->isDefn())
                return Emit1(cx, bce, JSOP_POP) >= 0;
        }

        switch (pn->getOp()) {
          case JSOP_SETNAME:
          case JSOP_SETGNAME:
            




            if (!EmitElemOp(cx, pn, JSOP_ENUMELEM, bce))
                return false;
            break;

          case JSOP_SETCONST:
            if (!EmitElemOp(cx, pn, JSOP_ENUMCONSTELEM, bce))
                return false;
            break;

          case JSOP_SETLOCAL:
          case JSOP_SETARG:
            if (!EmitVarOp(cx, pn, pn->getOp(), bce))
                return false;
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return false;
            break;

          default:
          {
            if (!EmitTree(cx, bce, pn))
                return false;
            if (!EmitElemOpBase(cx, bce, JSOP_ENUMELEM))
                return false;
            break;
          }

          case JSOP_ENUMELEM:
            JS_ASSERT(0);
        }
    }

    return true;
}














static bool
EmitDestructuringOpsHelper(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn,
                           VarEmitOption emitOption)
{
    JS_ASSERT(emitOption != DefineVars);

    unsigned index;
    ParseNode *pn2, *pn3;
    bool doElemOp;

#ifdef DEBUG
    int stackDepth = bce->stackDepth;
    JS_ASSERT(stackDepth != 0);
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->isKind(PNK_ARRAY) || pn->isKind(PNK_OBJECT));
#endif

    if (pn->pn_count == 0) {
        
        if (Emit1(cx, bce, JSOP_DUP) < 0 || Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    index = 0;
    for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
        
        if (Emit1(cx, bce, JSOP_DUP) < 0)
            return false;

        





        doElemOp = true;
        if (pn->isKind(PNK_ARRAY)) {
            if (!EmitNumberOp(cx, index, bce))
                return false;
            pn3 = pn2;
        } else {
            JS_ASSERT(pn->isKind(PNK_OBJECT));
            JS_ASSERT(pn2->isKind(PNK_COLON));
            pn3 = pn2->pn_left;
            if (pn3->isKind(PNK_NUMBER)) {
                if (!EmitNumberOp(cx, pn3->pn_dval, bce))
                    return false;
            } else {
                JS_ASSERT(pn3->isKind(PNK_STRING) || pn3->isKind(PNK_NAME));
                if (!EmitAtomOp(cx, pn3, JSOP_GETPROP, bce))
                    return false;
                doElemOp = false;
            }
            pn3 = pn2->pn_right;
        }

        if (doElemOp) {
            




            if (!EmitElemOpBase(cx, bce, JSOP_GETELEM))
                return false;
            JS_ASSERT(bce->stackDepth >= stackDepth + 1);
        }

        
        if (pn3->isKind(PNK_COMMA) && pn3->isArity(PN_NULLARY)) {
            JS_ASSERT(pn->isKind(PNK_ARRAY));
            JS_ASSERT(pn2 == pn3);
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return false;
        } else {
            int depthBefore = bce->stackDepth;
            if (!EmitDestructuringLHS(cx, bce, pn3, emitOption))
                return false;

            if (emitOption == PushInitialValues) {
                









                JS_ASSERT((bce->stackDepth - bce->stackDepth) >= -1);
                unsigned pickDistance = (unsigned)((bce->stackDepth + 1) - depthBefore);
                if (pickDistance > 0) {
                    if (pickDistance > UINT8_MAX) {
                        bce->reportError(pn3, JSMSG_TOO_MANY_LOCALS);
                        return false;
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
            return false;
    }

    return true;
}

static bool
EmitDestructuringOps(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, bool isLet = false)
{
    



    VarEmitOption emitOption = isLet ? PushInitialValues : InitializeVars;
    return EmitDestructuringOpsHelper(cx, bce, pn, emitOption);
}

static bool
EmitGroupAssignment(JSContext *cx, BytecodeEmitter *bce, JSOp prologOp,
                    ParseNode *lhs, ParseNode *rhs)
{
    unsigned depth, limit, i, nslots;
    ParseNode *pn;

    depth = limit = (unsigned) bce->stackDepth;
    for (pn = rhs->pn_head; pn; pn = pn->pn_next) {
        if (limit == JS_BIT(16)) {
            bce->reportError(rhs, JSMSG_ARRAY_INIT_TOO_BIG);
            return false;
        }

        
        JS_ASSERT(!(pn->isKind(PNK_COMMA) && pn->isArity(PN_NULLARY)));
        if (!EmitTree(cx, bce, pn))
            return false;
        ++limit;
    }

    i = depth;
    for (pn = lhs->pn_head; pn; pn = pn->pn_next, ++i) {
        
        JS_ASSERT(i < limit);
        int slot = AdjustBlockSlot(cx, bce, i);
        if (slot < 0)
            return false;

        if (!EmitUnaliasedVarOp(cx, JSOP_GETLOCAL, slot, bce))
            return false;

        if (pn->isKind(PNK_COMMA) && pn->isArity(PN_NULLARY)) {
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return false;
        } else {
            if (!EmitDestructuringLHS(cx, bce, pn, InitializeVars))
                return false;
        }
    }

    nslots = limit - depth;
    EMIT_UINT16_IMM_OP(JSOP_POPN, nslots);
    bce->stackDepth = (unsigned) depth;
    return true;
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
    if (lhs->isKind(PNK_ARRAY) && rhs->isKind(PNK_ARRAY) &&
        !(rhs->pn_xflags & PNX_SPECIALARRAYINIT) &&
        lhs->pn_count <= rhs->pn_count)
    {
        if (groupOption == GroupIsDecl && !EmitDestructuringDecls(cx, bce, prologOp, lhs))
            return false;
        if (!EmitGroupAssignment(cx, bce, prologOp, lhs, rhs))
            return false;
        *pop = JSOP_NOP;
    }
    return true;
}











static bool
MaybeEmitLetGroupDecl(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, JSOp *pop)
{
    JS_ASSERT(pn->isKind(PNK_ASSIGN));
    JS_ASSERT(pn->isOp(JSOP_NOP));
    JS_ASSERT(*pop == JSOP_POP || *pop == JSOP_POPV);

    ParseNode *lhs = pn->pn_left;
    ParseNode *rhs = pn->pn_right;
    if (lhs->isKind(PNK_ARRAY) && rhs->isKind(PNK_ARRAY) &&
        !(rhs->pn_xflags & PNX_SPECIALARRAYINIT) &&
        !(lhs->pn_xflags & PNX_SPECIALARRAYINIT) &&
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

        *pop = JSOP_NOP;
    }
    return true;
}

#endif 

static bool
EmitVariables(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, VarEmitOption emitOption,
              bool isLet = false)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(isLet == (emitOption == PushInitialValues));

    ParseNode *next;
    for (ParseNode *pn2 = pn->pn_head; ; pn2 = next) {
        next = pn2->pn_next;

        ParseNode *pn3;
        if (!pn2->isKind(PNK_NAME)) {
#if JS_HAS_DESTRUCTURING
            if (pn2->isKind(PNK_ARRAY) || pn2->isKind(PNK_OBJECT)) {
                







                JS_ASSERT(emitOption == DefineVars);
                JS_ASSERT(pn->pn_count == 1);
                if (!EmitDestructuringDecls(cx, bce, pn->getOp(), pn2))
                    return false;
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
                





                JS_ASSERT(!pn2->pn_next);
                if (isLet) {
                    if (!MaybeEmitLetGroupDecl(cx, bce, pn2, &op))
                        return false;
                } else {
                    if (!MaybeEmitGroupAssignment(cx, bce, pn->getOp(), pn2, GroupIsDecl, &op))
                        return false;
                }
            }
            if (op == JSOP_NOP) {
                pn->pn_xflags = (pn->pn_xflags & ~PNX_POPVAR) | PNX_GROUPINIT;
            } else {
                pn3 = pn2->pn_left;
                if (!EmitDestructuringDecls(cx, bce, pn->getOp(), pn3))
                    return false;

                if (!EmitTree(cx, bce, pn2->pn_right))
                    return false;

                if (!EmitDestructuringOps(cx, bce, pn3, isLet))
                    return false;
            }
            ptrdiff_t stackDepthAfter = bce->stackDepth;

            
            JS_ASSERT(stackDepthBefore <= stackDepthAfter);
            if (isLet && stackDepthBefore == stackDepthAfter) {
                if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                    return false;
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
            return false;


        JSOp op;
        op = pn2->getOp();
        JS_ASSERT(op != JSOP_CALLEE);
        JS_ASSERT(!pn2->pn_cookie.isFree() || !pn->isOp(JSOP_NOP));

        jsatomid atomIndex;
        if (!MaybeEmitVarDecl(cx, bce, pn->getOp(), pn2, &atomIndex))
            return false;

        if (pn3) {
            JS_ASSERT(emitOption != DefineVars);
            if (op == JSOP_SETNAME || op == JSOP_SETGNAME || op == JSOP_SETINTRINSIC) {
                JS_ASSERT(emitOption != PushInitialValues);
                JSOp bindOp;
                if (op == JSOP_SETNAME)
                    bindOp = JSOP_BINDNAME;
                else if (op == JSOP_SETGNAME)
                    bindOp = JSOP_BINDGNAME;
                else
                    bindOp = JSOP_BINDINTRINSIC;
                if (!EmitIndex32(cx, bindOp, atomIndex, bce))
                    return false;
            }

            bool oldEmittingForInit = bce->emittingForInit;
            bce->emittingForInit = false;
            if (!EmitTree(cx, bce, pn3))
                return false;
            bce->emittingForInit = oldEmittingForInit;
        } else if (isLet) {
            
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return false;
        }

        
        if (emitOption != InitializeVars) {
            if (next)
                continue;
            break;
        }

        JS_ASSERT_IF(pn2->isDefn(), pn3 == pn2->pn_expr);
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
        if (!next)
            break;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    if (pn->pn_xflags & PNX_POPVAR) {
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    return true;
}

static bool
EmitAssignment(JSContext *cx, BytecodeEmitter *bce, ParseNode *lhs, JSOp op, ParseNode *rhs)
{
    




    jsatomid atomIndex = (jsatomid) -1;
    jsbytecode offset = 1;

    switch (lhs->getKind()) {
      case PNK_NAME:
        if (!BindNameToSlot(cx, bce, lhs))
            return false;
        if (lhs->pn_cookie.isFree()) {
            if (!bce->makeAtomIndex(lhs->pn_atom, &atomIndex))
                return false;
            if (!lhs->isConst()) {
                JSOp bindOp;
                if (lhs->isOp(JSOP_SETNAME))
                    bindOp = JSOP_BINDNAME;
                else if (lhs->isOp(JSOP_SETGNAME))
                    bindOp = JSOP_BINDGNAME;
                else
                    bindOp = JSOP_BINDINTRINSIC;
                if (!EmitIndex32(cx, bindOp, atomIndex, bce))
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
      case PNK_ELEM:
        JS_ASSERT(lhs->isArity(PN_BINARY));
        if (!EmitTree(cx, bce, lhs->pn_left))
            return false;
        if (!EmitTree(cx, bce, lhs->pn_right))
            return false;
        offset += 2;
        break;
#if JS_HAS_DESTRUCTURING
      case PNK_ARRAY:
      case PNK_OBJECT:
        break;
#endif
      case PNK_CALL:
        if (!EmitTree(cx, bce, lhs))
            return false;
        JS_ASSERT(lhs->pn_xflags & PNX_SETCALL);
        offset += 2;
        break;
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
            } else if (lhs->isOp(JSOP_SETINTRINSIC)) {
                JS_ASSERT(lhs->pn_cookie.isFree());
                if (!EmitAtomOp(cx, lhs, JSOP_GETINTRINSIC, bce))
                    return false;
            } else {
                JSOp op = lhs->isOp(JSOP_SETARG) ? JSOP_GETARG : JSOP_GETLOCAL;
                if (!EmitVarOp(cx, lhs, op, bce))
                    return false;
            }
            break;
          case PNK_DOT: {
            if (Emit1(cx, bce, JSOP_DUP) < 0)
                return false;
            bool isLength = (lhs->pn_atom == cx->names().length);
            if (!EmitIndex32(cx, isLength ? JSOP_LENGTH : JSOP_GETPROP, atomIndex, bce))
                return false;
            break;
          }
          case PNK_ELEM:
          case PNK_CALL:
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
        





        if (offset != 1 && Emit2(cx, bce, JSOP_PICK, offset - 1) < 0)
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

    
    switch (lhs->getKind()) {
      case PNK_NAME:
        if (lhs->isConst()) {
            if (!rhs) {
                bce->reportError(lhs, JSMSG_BAD_FOR_LEFTSIDE);
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
      case PNK_ELEM:
      case PNK_CALL:
        if (Emit1(cx, bce, JSOP_SETELEM) < 0)
            return false;
        break;
#if JS_HAS_DESTRUCTURING
      case PNK_ARRAY:
      case PNK_OBJECT:
        if (!EmitDestructuringOps(cx, bce, lhs))
            return false;
        break;
#endif
      default:
        JS_ASSERT(0);
    }
    return true;
}

static bool
EmitNewInit(JSContext *cx, BytecodeEmitter *bce, JSProtoKey key, ParseNode *pn)
{
    const size_t len = 1 + UINT32_INDEX_LEN;
    ptrdiff_t offset = EmitCheck(cx, bce, len);
    if (offset < 0)
        return false;

    jsbytecode *code = bce->code(offset);
    code[0] = JSOP_NEWINIT;
    code[1] = jsbytecode(key);
    code[2] = 0;
    code[3] = 0;
    code[4] = 0;
    UpdateDepth(cx, bce, offset);
    CheckTypeSet(cx, bce, JSOP_NEWINIT);
    return true;
}

bool
ParseNode::getConstantValue(JSContext *cx, bool strictChecks, MutableHandleValue vp)
{
    switch (getKind()) {
      case PNK_NUMBER:
        vp.setNumber(pn_dval);
        return true;
      case PNK_STRING:
        vp.setString(pn_atom);
        return true;
      case PNK_TRUE:
        vp.setBoolean(true);
        return true;
      case PNK_FALSE:
        vp.setBoolean(false);
        return true;
      case PNK_NULL:
        vp.setNull();
        return true;
      case PNK_SPREAD:
        return false;
      case PNK_ARRAY: {
        JS_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));

        RootedObject obj(cx, NewDenseAllocatedArray(cx, pn_count, NULL, MaybeSingletonObject));
        if (!obj)
            return false;

        unsigned idx = 0;
        RootedId id(cx);
        RootedValue value(cx);
        for (ParseNode *pn = pn_head; pn; idx++, pn = pn->pn_next) {
            if (!pn->getConstantValue(cx, strictChecks, &value))
                return false;
            id = INT_TO_JSID(idx);
            if (!JSObject::defineGeneric(cx, obj, id, value, NULL, NULL, JSPROP_ENUMERATE))
                return false;
        }
        JS_ASSERT(idx == pn_count);

        types::FixArrayType(cx, obj);
        vp.setObject(*obj);
        return true;
      }
      case PNK_OBJECT: {
        JS_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));

        gc::AllocKind kind = GuessObjectGCKind(pn_count);
        RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass, kind, MaybeSingletonObject));
        if (!obj)
            return false;

        for (ParseNode *pn = pn_head; pn; pn = pn->pn_next) {
            RootedValue value(cx);
            if (!pn->pn_right->getConstantValue(cx, strictChecks, &value))
                return false;

            ParseNode *pnid = pn->pn_left;
            if (pnid->isKind(PNK_NUMBER)) {
                Value idvalue = NumberValue(pnid->pn_dval);
                RootedId id(cx);
                if (idvalue.isInt32() && INT_FITS_IN_JSID(idvalue.toInt32()))
                    id = INT_TO_JSID(idvalue.toInt32());
                else if (!InternNonIntElementId<CanGC>(cx, obj, idvalue, &id))
                    return false;
                if (!JSObject::defineGeneric(cx, obj, id, value, NULL, NULL, JSPROP_ENUMERATE))
                    return false;
            } else {
                JS_ASSERT(pnid->isKind(PNK_NAME) || pnid->isKind(PNK_STRING));
                JS_ASSERT(pnid->pn_atom != cx->names().proto);
                RootedId id(cx, AtomToId(pnid->pn_atom));
                if (!DefineNativeProperty(cx, obj, id, value, NULL, NULL,
                                          JSPROP_ENUMERATE, 0, 0)) {
                    return false;
                }
            }
        }

        types::FixObjectType(cx, obj);
        vp.setObject(*obj);
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
    RootedValue value(cx);
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
    ptrdiff_t guardJump;

    



    StmtInfoBCE *stmt = bce->topStmt;
    JS_ASSERT(stmt->type == STMT_BLOCK && stmt->isBlockScope);
    stmt->type = STMT_CATCH;

    
    stmt = stmt->down;
    JS_ASSERT(stmt->type == STMT_TRY || stmt->type == STMT_FINALLY);

    
    if (Emit1(cx, bce, JSOP_EXCEPTION) < 0)
        return false;

    



    if (pn->pn_kid2 && Emit1(cx, bce, JSOP_DUP) < 0)
        return false;

    ParseNode *pn2 = pn->pn_kid1;
    switch (pn2->getKind()) {
#if JS_HAS_DESTRUCTURING
      case PNK_ARRAY:
      case PNK_OBJECT:
        if (!EmitDestructuringOps(cx, bce, pn2))
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

        
        guardJump = EmitJump(cx, bce, JSOP_IFEQ, 0);
        if (guardJump < 0)
            return false;
        stmt->guardJump() = guardJump;

        
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    
    if (!EmitTree(cx, bce, pn->pn_kid3))
        return false;

    



    return NewSrcNote(cx, bce, SRC_CATCH) >= 0;
}





MOZ_NEVER_INLINE static bool
EmitTry(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    StmtInfoBCE stmtInfo(cx);
    ptrdiff_t catchJump = -1;

    








    PushStatementBCE(bce, &stmtInfo, pn->pn_kid3 ? STMT_FINALLY : STMT_TRY, bce->offset());

    








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
        if (EmitBackPatchOp(cx, bce, &stmtInfo.gosubs()) < 0)
            return false;
    }

    
    if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
        return false;
    if (EmitBackPatchOp(cx, bce, &catchJump) < 0)
        return false;

    ptrdiff_t tryEnd = bce->offset();

    
    ParseNode *lastCatch = NULL;
    if (ParseNode *pn2 = pn->pn_kid2) {
        unsigned count = 0;    

        





















        for (ParseNode *pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            ptrdiff_t guardJump;

            JS_ASSERT(bce->stackDepth == depth);
            guardJump = stmtInfo.guardJump();
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

            




            JS_ASSERT(pn3->isKind(PNK_LEXICALSCOPE));
            count = pn3->pn_objbox->object->asStaticBlock().slotCount();
            if (!EmitTree(cx, bce, pn3))
                return false;

            
            if (pn->pn_kid3) {
                if (EmitBackPatchOp(cx, bce, &stmtInfo.gosubs()) < 0)
                    return false;
                JS_ASSERT(bce->stackDepth == depth);
            }

            



            if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
                return false;
            if (EmitBackPatchOp(cx, bce, &catchJump) < 0)
                return false;

            



            lastCatch = pn3->expr();
        }
    }

    





    if (lastCatch && lastCatch->pn_kid2) {
        SetJumpOffsetAt(bce, stmtInfo.guardJump());

        
        JS_ASSERT(bce->stackDepth == depth);
        bce->stackDepth = depth + 1;

        



        if (NewSrcNote(cx, bce, SRC_HIDDEN) < 0 || Emit1(cx, bce, JSOP_THROW) < 0)
            return false;
    }

    JS_ASSERT(bce->stackDepth == depth);

    
    ptrdiff_t finallyStart = 0;   
    if (pn->pn_kid3) {
        



        if (!BackPatch(cx, bce, stmtInfo.gosubs(), bce->code().end(), JSOP_GOSUB))
            return false;

        finallyStart = bce->offset();

        
        stmtInfo.type = STMT_SUBROUTINE;
        if (!UpdateSourceCoordNotes(cx, bce, pn->pn_kid3->pn_pos.begin))
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

    
    if (Emit1(cx, bce, JSOP_NOP) < 0)
        return false;

    
    if (!BackPatch(cx, bce, catchJump, bce->code().end(), JSOP_GOTO))
        return false;

    



    if (pn->pn_kid2 && !bce->tryNoteList.append(JSTRY_CATCH, depth, tryStart, tryEnd))
        return false;

    




    if (pn->pn_kid3 && !bce->tryNoteList.append(JSTRY_FINALLY, depth, tryStart, finallyStart))
        return false;

    return true;
}

static bool
EmitIf(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    StmtInfoBCE stmtInfo(cx);

    
    stmtInfo.type = STMT_IF;
    ptrdiff_t beq = -1;
    ptrdiff_t jmp = -1;
    ptrdiff_t noteIndex = -1;

  if_again:
    
    if (!EmitTree(cx, bce, pn->pn_kid1))
        return false;
    ptrdiff_t top = bce->offset();
    if (stmtInfo.type == STMT_IF) {
        PushStatementBCE(bce, &stmtInfo, STMT_IF, top);
    } else {
        




        JS_ASSERT(stmtInfo.type == STMT_ELSE);
        stmtInfo.type = STMT_IF;
        stmtInfo.update = top;
        if (!SetSrcNoteOffset(cx, bce, noteIndex, 0, jmp - beq))
            return false;
    }

    
    ParseNode *pn3 = pn->pn_kid3;
    noteIndex = NewSrcNote(cx, bce, pn3 ? SRC_IF_ELSE : SRC_IF);
    if (noteIndex < 0)
        return false;
    beq = EmitJump(cx, bce, JSOP_IFEQ, 0);
    if (beq < 0)
        return false;

    
    if (!EmitTree(cx, bce, pn->pn_kid2))
        return false;
    if (pn3) {
        
        stmtInfo.type = STMT_ELSE;

        





        jmp = EmitGoto(cx, bce, &stmtInfo, &stmtInfo.breaks);
        if (jmp < 0)
            return false;

        
        SetJumpOffsetAt(bce, beq);
        if (pn3->isKind(PNK_IF)) {
            pn = pn3;
            goto if_again;
        }

        if (!EmitTree(cx, bce, pn3))
            return false;

        






        if (!SetSrcNoteOffset(cx, bce, noteIndex, 0, jmp - beq))
            return false;
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

    int letHeadDepth = bce->stackDepth;

    if (!EmitVariables(cx, bce, varList, PushInitialValues, true))
        return false;

    
    uint32_t alreadyPushed = unsigned(bce->stackDepth - letHeadDepth);
    uint32_t blockObjCount = blockObj->slotCount();
    for (uint32_t i = alreadyPushed; i < blockObjCount; ++i) {
        if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
            return false;
    }

    StmtInfoBCE stmtInfo(cx);
    PushBlockScopeBCE(bce, &stmtInfo, *blockObj, bce->offset());

    DebugOnly<ptrdiff_t> bodyBegin = bce->offset();
    if (!EmitEnterBlock(cx, bce, letBody, JSOP_ENTERLET0))
        return false;

    if (!EmitTree(cx, bce, letBody->pn_expr))
        return false;

    JSOp leaveOp = letBody->getOp();
    JS_ASSERT(leaveOp == JSOP_LEAVEBLOCK || leaveOp == JSOP_LEAVEBLOCKEXPR);
    EMIT_UINT16_IMM_OP(leaveOp, blockObj->slotCount());

    DebugOnly<ptrdiff_t> bodyEnd = bce->offset();
    JS_ASSERT(bodyEnd > bodyBegin);

    return PopStatementBCE(cx, bce);
}
#endif





MOZ_NEVER_INLINE static bool
EmitLexicalScope(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_LEXICALSCOPE));
    JS_ASSERT(pn->getOp() == JSOP_LEAVEBLOCK);

    StmtInfoBCE stmtInfo(cx);
    ObjectBox *objbox = pn->pn_objbox;
    StaticBlockObject &blockObj = objbox->object->asStaticBlock();
    size_t slots = blockObj.slotCount();
    PushBlockScopeBCE(bce, &stmtInfo, blockObj, bce->offset());

    if (!EmitEnterBlock(cx, bce, pn, JSOP_ENTERBLOCK))
        return false;

    if (!EmitTree(cx, bce, pn->pn_expr))
        return false;

    EMIT_UINT16_IMM_OP(JSOP_LEAVEBLOCK, slots);

    return PopStatementBCE(cx, bce);
}

static bool
EmitWith(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    StmtInfoBCE stmtInfo(cx);
    if (!EmitTree(cx, bce, pn->pn_left))
        return false;
    PushStatementBCE(bce, &stmtInfo, STMT_WITH, bce->offset());
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
    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_FOR_IN_LOOP, top);

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
        bce->emittingForInit = true;
        if (!EmitVariables(cx, bce, decl, DefineVars))
            return false;
        bce->emittingForInit = false;
    }

    
    if (!EmitTree(cx, bce, forHead->pn_kid3))
        return false;

    




    JS_ASSERT(pn->isOp(JSOP_ITER));
    if (Emit2(cx, bce, JSOP_ITER, (uint8_t) pn->pn_iflags) < 0)
        return false;

    
    StmtInfoBCE letStmt(cx);
    if (letDecl) {
        PushBlockScopeBCE(bce, &letStmt, *blockObj, bce->offset());
        letStmt.isForLetBlock = true;
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

    





    if (Emit1(cx, bce, JSOP_ITERNEXT) < 0)
        return false;
    if (!EmitAssignment(cx, bce, forHead->pn_kid2, JSOP_NOP, NULL))
        return false;

    if (Emit1(cx, bce, JSOP_POP) < 0)
        return false;

    
    JS_ASSERT(bce->stackDepth == loopDepth);

    
    if (!EmitTree(cx, bce, forBody))
        return false;

    
    StmtInfoBCE *stmt = &stmtInfo;
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

    
    if (!SetSrcNoteOffset(cx, bce, (unsigned)noteIndex, 0, beq - jmp))
        return false;

    
    if (!PopStatementBCE(cx, bce))
        return false;

    if (letDecl) {
        if (!PopStatementBCE(cx, bce))
            return false;
        if (Emit1(cx, bce, JSOP_LEAVEFORLETIN) < 0)
            return false;
    }

    if (!bce->tryNoteList.append(JSTRY_ITER, bce->stackDepth, top, bce->offset()))
        return false;
    if (Emit1(cx, bce, JSOP_ENDITER) < 0)
        return false;

    if (letDecl)
        EMIT_UINT16_IMM_OP(JSOP_POPN, blockObjCount);

    return true;
}

static bool
EmitNormalFor(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_FOR_LOOP, top);

    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    
    JSOp op = JSOP_POP;
    ParseNode *pn3 = forHead->pn_kid1;
    if (!pn3) {
        
        op = JSOP_NOP;
    } else {
        bce->emittingForInit = true;
#if JS_HAS_DESTRUCTURING
        if (pn3->isKind(PNK_ASSIGN)) {
            JS_ASSERT(pn3->isOp(JSOP_NOP));
            if (!MaybeEmitGroupAssignment(cx, bce, op, pn3, GroupIsNotDecl, &op))
                return false;
        }
#endif
        if (op == JSOP_POP) {
            if (!UpdateSourceCoordNotes(cx, bce, pn3->pn_pos.begin))
                return false;
            if (!EmitTree(cx, bce, pn3))
                return false;
            if (pn3->isKind(PNK_VAR) || pn3->isKind(PNK_CONST) || pn3->isKind(PNK_LET)) {
                





                JS_ASSERT(pn3->isArity(PN_LIST) || pn3->isArity(PN_BINARY));
                if (pn3->pn_xflags & PNX_GROUPINIT)
                    op = JSOP_NOP;
            }
        }
        bce->emittingForInit = false;
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

    
    StmtInfoBCE *stmt = &stmtInfo;
    do {
        stmt->update = bce->offset();
    } while ((stmt = stmt->down) != NULL && stmt->type == STMT_LABEL);

    
    pn3 = forHead->pn_kid3;
    if (pn3) {
        if (!UpdateSourceCoordNotes(cx, bce, pn3->pn_pos.begin))
            return false;
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

        
        uint32_t lineNum = bce->parser->tokenStream.srcCoords.lineNum(pn->pn_pos.end);
        if (bce->currentLine() != lineNum) {
            if (NewSrcNote2(cx, bce, SRC_SETLINE, ptrdiff_t(lineNum)) < 0)
                return false;
            bce->current->currentLine = lineNum;
            bce->current->lastColumn = 0;
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
    RootedFunction fun(cx, pn->pn_funbox->function());
    if (fun->isNative()) {
        JS_ASSERT(IsAsmJSModuleNative(fun->native()));
        return true;
    }
    if (fun->hasScript()) {
        JS_ASSERT(pn->functionIsHoisted());
        JS_ASSERT(bce->sc->isFunctionBox());
        return true;
    }

    {
        SharedContext *outersc = bce->sc;
        FunctionBox *funbox = pn->pn_funbox;

        if (outersc->isFunctionBox() && outersc->asFunctionBox()->mightAliasLocals())
            funbox->setMightAliasLocals();      
        JS_ASSERT_IF(outersc->strict, funbox->strict);

        
        Rooted<JSScript*> parent(cx, bce->script);
        CompileOptions options(cx);
        options.setPrincipals(parent->principals())
               .setOriginPrincipals(parent->originPrincipals)
               .setCompileAndGo(parent->compileAndGo)
               .setSelfHostingMode(parent->selfHosted)
               .setNoScriptRval(false)
               .setVersion(parent->getVersion())
               .setUserBit(parent->userBit);

        bool generateBytecode = true;
#ifdef JS_ION
        if (funbox->useAsm) {
            RootedFunction moduleFun(cx);

            
            
            
            
            
            
            
            
            if (!CompileAsmJS(cx, *bce->tokenStream(), pn, options,
                              bce->script->scriptSource(), funbox->asmStart, funbox->bufEnd - 1,
                              &moduleFun))
                return false;

            if (moduleFun) {
                funbox->object = moduleFun;
                generateBytecode = false;
            }
        }
#endif

        if (generateBytecode) {
            Rooted<JSObject*> enclosingScope(cx, EnclosingStaticScope(bce));
            Rooted<JSScript*> script(cx, JSScript::Create(cx, enclosingScope, false, options,
                                                          parent->staticLevel + 1,
                                                          bce->script->scriptSource(),
                                                          funbox->bufStart, funbox->bufEnd));
            if (!script)
                return false;

            script->bindings = funbox->bindings;

            uint32_t lineNum = bce->parser->tokenStream.srcCoords.lineNum(pn->pn_pos.begin);
            BytecodeEmitter bce2(bce, bce->parser, funbox, script, bce->evalCaller,
                                 bce->hasGlobalScope, lineNum, bce->selfHostingMode);
            if (!bce2.init())
                return false;

            
            if (!EmitFunctionScript(cx, &bce2, pn->pn_body))
                return false;
        }
    }

    
    unsigned index = bce->objectList.add(pn->pn_funbox);

    
    if (!pn->functionIsHoisted())
        return EmitIndex32(cx, pn->getOp(), index, bce);

    








    if (!bce->sc->isFunctionBox()) {
        JS_ASSERT(pn->pn_cookie.isFree());
        JS_ASSERT(pn->getOp() == JSOP_NOP);
        JS_ASSERT(!bce->topStmt);
        bce->switchToProlog();
        if (!EmitIndex32(cx, JSOP_DEFFUN, index, bce))
            return false;
        if (!UpdateSourceCoordNotes(cx, bce, pn->pn_pos.begin))
            return false;
        bce->switchToMain();
    } else {
#ifdef DEBUG
        BindingIter bi(bce->script);
        while (bi->name() != fun->atom())
            bi++;
        JS_ASSERT(bi->kind() == VARIABLE || bi->kind() == CONSTANT || bi->kind() == ARGUMENT);
        JS_ASSERT(bi.frameIndex() < JS_BIT(20));
#endif
        pn->pn_index = index;
        if (!EmitIndexOp(cx, JSOP_LAMBDA, index, bce))
            return false;
        JS_ASSERT(pn->getOp() == JSOP_GETLOCAL || pn->getOp() == JSOP_GETARG);
        JSOp setOp = pn->getOp() == JSOP_GETLOCAL ? JSOP_SETLOCAL : JSOP_SETARG;
        if (!EmitVarOp(cx, pn, setOp, bce))
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

    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_DO_LOOP, top);
    if (!EmitTree(cx, bce, pn->pn_left))
        return false;

    
    ptrdiff_t off = bce->offset();
    StmtInfoBCE *stmt = &stmtInfo;
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
    












    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_WHILE_LOOP, top);

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
    StmtInfoBCE *stmt = bce->topStmt;
    SrcNoteType noteType;
    if (label) {
        while (stmt->type != STMT_LABEL || stmt->label != label)
            stmt = stmt->down;
        noteType = SRC_BREAK2LABEL;
    } else {
        while (!stmt->isLoop() && stmt->type != STMT_SWITCH)
            stmt = stmt->down;
        noteType = (stmt->type == STMT_SWITCH) ? SRC_SWITCHBREAK : SRC_BREAK;
    }

    return EmitGoto(cx, bce, stmt, &stmt->breaks, noteType) >= 0;
}

static bool
EmitContinue(JSContext *cx, BytecodeEmitter *bce, PropertyName *label)
{
    StmtInfoBCE *stmt = bce->topStmt;
    if (label) {
        
        StmtInfoBCE *loop = NULL;
        while (stmt->type != STMT_LABEL || stmt->label != label) {
            if (stmt->isLoop())
                loop = stmt;
            stmt = stmt->down;
        }
        stmt = loop;
    } else {
        while (!stmt->isLoop())
            stmt = stmt->down;
    }

    return EmitGoto(cx, bce, stmt, &stmt->continues, SRC_CONTINUE) >= 0;
}

static bool
EmitReturn(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    if (!UpdateSourceCoordNotes(cx, bce, pn->pn_pos.begin))
        return false;

    
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
        bce->code()[top] = JSOP_SETRVAL;
        if (Emit1(cx, bce, JSOP_RETRVAL) < 0)
            return false;
    }

    return true;
}

static bool
EmitStatementList(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    JS_ASSERT(pn->isArity(PN_LIST));

    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_BLOCK, top);

    ParseNode *pnchild = pn->pn_head;

    if (pn->pn_xflags & PNX_DESTRUCT)
        pnchild = pnchild->pn_next;

    for (ParseNode *pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
        if (!EmitTree(cx, bce, pn2))
            return false;
    }

    return PopStatementBCE(cx, bce);
}

static bool
EmitStatement(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_SEMI));

    ParseNode *pn2 = pn->pn_kid;
    if (!pn2)
        return true;

    if (!UpdateSourceCoordNotes(cx, bce, pn->pn_pos.begin))
        return false;

    








    bool wantval = false;
    bool useful = false;
    if (bce->sc->isFunctionBox()) {
        JS_ASSERT(!bce->script->noScriptRval);
    } else {
        useful = wantval = !bce->script->noScriptRval;
    }

    
    if (!useful) {
        if (!CheckSideEffects(cx, bce, pn2, &useful))
            return false;

        





        if (bce->topStmt &&
            bce->topStmt->type == STMT_LABEL &&
            bce->topStmt->update >= bce->offset())
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
        
        bce->current->currentLine = bce->parser->tokenStream.srcCoords.lineNum(pn2->pn_pos.begin);
        bce->current->lastColumn = 0;
        if (!bce->reportStrictWarning(pn2, JSMSG_USELESS_EXPR))
            return false;
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
      case PNK_ELEM:
        if (!EmitElemOp(cx, pn2, JSOP_DELELEM, bce))
            return false;
        break;
      default:
      {
        



        bool useful = false;
        if (!CheckSideEffects(cx, bce, pn2, &useful))
            return false;

        if (useful) {
            JS_ASSERT_IF(pn2->isKind(PNK_CALL), !(pn2->pn_xflags & PNX_SETCALL));
            if (!EmitTree(cx, bce, pn2))
                return false;
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return false;
        }

        if (Emit1(cx, bce, JSOP_TRUE) < 0)
            return false;
      }
    }

    return true;
}

static bool
EmitCallOrNew(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    bool callop = pn->isKind(PNK_CALL);

    














    uint32_t argc = pn->pn_count - 1;

    if (argc >= ARGC_LIMIT) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             callop ? JSMSG_TOO_MANY_FUN_ARGS : JSMSG_TOO_MANY_CON_ARGS);
        return false;
    }

    bool emitArgs = true;
    ParseNode *pn2 = pn->pn_head;
    switch (pn2->getKind()) {
      case PNK_NAME:
        if (bce->selfHostingMode && pn2->name() == cx->names().callFunction)
        {
            










            if (pn->pn_count < 3) {
                bce->reportError(pn, JSMSG_MORE_ARGS_NEEDED, "callFunction", "1", "s");
                return false;
            }
            ParseNode *funNode = pn2->pn_next;
            if (!EmitTree(cx, bce, funNode))
                return false;
            ParseNode *thisArg = funNode->pn_next;
            if (!EmitTree(cx, bce, thisArg))
                return false;
            if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
                return false;
            bool oldEmittingForInit = bce->emittingForInit;
            bce->emittingForInit = false;
            for (ParseNode *argpn = thisArg->pn_next; argpn; argpn = argpn->pn_next) {
                if (!EmitTree(cx, bce, argpn))
                    return false;
                if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
                    return false;
            }
            bce->emittingForInit = oldEmittingForInit;
            argc -= 2;
            emitArgs = false;
            break;
        }
        if (!EmitNameOp(cx, bce, pn2, callop))
            return false;
        break;
      case PNK_DOT:
        if (!EmitPropOp(cx, pn2, pn2->getOp(), bce, callop))
            return false;
        break;
      case PNK_ELEM:
        JS_ASSERT(pn2->isOp(JSOP_GETELEM));
        if (!EmitElemOp(cx, pn2, callop ? JSOP_CALLELEM : JSOP_GETELEM, bce))
            return false;
        break;
      case PNK_FUNCTION:
        









        JS_ASSERT(!bce->emittingRunOnceLambda);
        if (bce->checkSingletonContext()) {
            bce->emittingRunOnceLambda = true;
            if (!EmitTree(cx, bce, pn2))
                return false;
            bce->emittingRunOnceLambda = false;
        } else {
            if (!EmitTree(cx, bce, pn2))
                return false;
        }
        callop = false;
        break;
      default:
        if (!EmitTree(cx, bce, pn2))
            return false;
        callop = false;             
        break;
    }
    if (!callop) {
        JSOp thisop = pn->isKind(PNK_GENEXP) ? JSOP_THIS : JSOP_UNDEFINED;
        if (Emit1(cx, bce, thisop) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
            return false;
    }

    if (emitArgs) {
        




        bool oldEmittingForInit = bce->emittingForInit;
        bce->emittingForInit = false;
        for (ParseNode *pn3 = pn2->pn_next; pn3; pn3 = pn3->pn_next) {
            if (!EmitTree(cx, bce, pn3))
                return false;
            if (Emit1(cx, bce, JSOP_NOTEARG) < 0)
                return false;
        }
        bce->emittingForInit = oldEmittingForInit;
    }

    if (Emit3(cx, bce, pn->getOp(), ARGC_HI(argc), ARGC_LO(argc)) < 0)
        return false;
    CheckTypeSet(cx, bce, pn->getOp());
    if (pn->isOp(JSOP_EVAL)) {
        uint32_t lineNum = bce->parser->tokenStream.srcCoords.lineNum(pn->pn_pos.begin);
        EMIT_UINT16_IMM_OP(JSOP_LINENO, lineNum);
    }
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





MOZ_NEVER_INLINE static bool
EmitIncOrDec(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    
    ParseNode *pn2 = pn->pn_kid;
    JSOp op = pn->getOp();
    switch (pn2->getKind()) {
      case PNK_DOT:
        if (!EmitPropIncDec(cx, pn2, op, bce))
            return false;
        break;
      case PNK_ELEM:
        if (!EmitElemIncDec(cx, pn2, op, bce))
            return false;
        break;
      case PNK_CALL:
        if (!EmitTree(cx, bce, pn2))
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

    ptrdiff_t top = EmitJump(cx, bce, JSOP_LABEL, 0);
    if (top < 0)
        return false;

    
    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_LABEL, bce->offset());
    stmtInfo.label = atom;
    if (!EmitTree(cx, bce, pn->expr()))
        return false;
    if (!PopStatementBCE(cx, bce))
        return false;

    
    SetJumpOffsetAt(bce, top);
    return true;
}

static bool
EmitSyntheticStatements(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    StmtInfoBCE stmtInfo(cx);
    PushStatementBCE(bce, &stmtInfo, STMT_SEQ, top);
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





MOZ_NEVER_INLINE static bool
EmitObject(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
#if JS_HAS_DESTRUCTURING_SHORTHAND
    if (pn->pn_xflags & PNX_DESTRUCT) {
        bce->reportError(pn, JSMSG_BAD_OBJECT_INIT);
        return false;
    }
#endif

    if (!(pn->pn_xflags & PNX_NONCONST) && pn->pn_head && bce->checkSingletonContext())
        return EmitSingletonInitialiser(cx, bce, pn);

    







    ptrdiff_t offset = bce->offset();
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
            if (Emit1(cx, bce, JSOP_INITELEM) < 0)
                return false;
        } else {
            JS_ASSERT(pn3->isKind(PNK_NAME) || pn3->isKind(PNK_STRING));
            jsatomid index;
            if (!bce->makeAtomIndex(pn3->pn_atom, &index))
                return false;

            



            if (pn3->pn_atom == cx->names().proto)
                obj = NULL;
            op = JSOP_INITPROP;

            if (obj) {
                JS_ASSERT(!obj->inDictionaryMode());
                Rooted<jsid> id(cx, AtomToId(pn3->pn_atom));
                RootedValue undefinedValue(cx, UndefinedValue());
                if (!DefineNativeProperty(cx, obj, id, undefinedValue, NULL, NULL,
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
        } else if (nspread) {
            if (Emit1(cx, bce, JSOP_INITELEM_INC) < 0)
                return false;
        } else {
            off = EmitN(cx, bce, JSOP_INITELEM_ARRAY, 3);
            if (off < 0)
                return false;
            SET_UINT24(bce->code(off), atomIndex);
        }
    }
    JS_ASSERT(atomIndex == pn->pn_count);
    if (nspread) {
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
    }

    
    return Emit1(cx, bce, JSOP_ENDINIT) >= 0;
}

static bool
EmitUnary(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    if (!UpdateSourceCoordNotes(cx, bce, pn->pn_pos.begin))
        return false;
    
    JSOp op = pn->getOp();
    ParseNode *pn2 = pn->pn_kid;

    if (op == JSOP_TYPEOF && !pn2->isKind(PNK_NAME))
        op = JSOP_TYPEOFEXPR;

    bool oldEmittingForInit = bce->emittingForInit;
    bce->emittingForInit = false;
    if (!EmitTree(cx, bce, pn2))
        return false;

    bce->emittingForInit = oldEmittingForInit;
    return Emit1(cx, bce, op) >= 0;
}

static bool
EmitDefaults(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_ASSERT(pn->isKind(PNK_ARGSBODY));

    ParseNode *arg, *pnlast = pn->last();
    for (arg = pn->pn_head; arg != pnlast; arg = arg->pn_next) {
        if (!(arg->pn_dflags & PND_DEFAULT) || !arg->isKind(PNK_NAME))
            continue;
        if (!BindNameToSlot(cx, bce, arg))
            return false;
        if (!EmitVarOp(cx, arg, JSOP_GETARG, bce))
            return false;
        if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_STRICTEQ) < 0)
            return false;
        ptrdiff_t jump = EmitJump(cx, bce, JSOP_IFEQ, 0);
        if (jump < 0)
            return false;
        if (!EmitTree(cx, bce, arg->expr()))
            return false;
        if (!EmitVarOp(cx, arg, JSOP_SETARG, bce))
            return false;
        if (Emit1(cx, bce, JSOP_POP) < 0)
            return false;
        SET_JUMP_OFFSET(bce->code(jump), bce->offset() - jump);
    }

    return true;
}

bool
frontend::EmitTree(JSContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    JS_CHECK_RECURSION(cx, return false);

    EmitLevelManager elm(bce);

    bool ok = true;
    ptrdiff_t top = bce->offset();
    pn->pn_offset = top;

    
    if (!UpdateLineNumberNotes(cx, bce, pn->pn_pos.begin))
        return false;

    switch (pn->getKind()) {
      case PNK_FUNCTION:
        ok = EmitFunc(cx, bce, pn);
        break;

      case PNK_ARGSBODY:
      {
        RootedFunction fun(cx, bce->sc->asFunctionBox()->function());
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
                if (pn2->isKind(PNK_FUNCTION) && pn2->functionIsHoisted()) {
                    if (!EmitTree(cx, bce, pn2))
                        return false;
                }
            }
        }
        if (fun->hasDefaults()) {
            ParseNode *rest = NULL;
            bool restIsDefn = false;
            if (fun->hasRest()) {
                JS_ASSERT(!bce->sc->asFunctionBox()->argumentsHasLocalBinding());
                
                
                
                
                
                
                rest = pn->pn_head;
                while (rest->pn_next != pnlast)
                    rest = rest->pn_next;
                restIsDefn = rest->isDefn();
                if (Emit1(cx, bce, JSOP_REST) < 0)
                    return false;
                CheckTypeSet(cx, bce, JSOP_REST);
                
                
                if (restIsDefn) {
                    if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                        return false;
                    if (!BindNameToSlot(cx, bce, rest))
                        return false;
                    if (!EmitVarOp(cx, rest, JSOP_SETARG, bce))
                        return false;
                    if (Emit1(cx, bce, JSOP_POP) < 0)
                        return false;
                }
            }
            if (!EmitDefaults(cx, bce, pn))
                return false;
            if (fun->hasRest()) {
                if (restIsDefn && !EmitVarOp(cx, rest, JSOP_SETARG, bce))
                    return false;
                if (Emit1(cx, bce, JSOP_POP) < 0)
                    return false;
            }
        }
        for (ParseNode *pn2 = pn->pn_head; pn2 != pnlast; pn2 = pn2->pn_next) {
            
            
            if (!pn2->isDefn())
                continue;
            if (!BindNameToSlot(cx, bce, pn2))
                return false;
            if (pn2->pn_next == pnlast && fun->hasRest() && !fun->hasDefaults()) {
                
                JS_ASSERT(!bce->sc->asFunctionBox()->argumentsHasLocalBinding());
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
        ok = EmitBreak(cx, bce, pn->as<BreakStatement>().label());
        break;

      case PNK_CONTINUE:
        ok = EmitContinue(cx, bce, pn->as<ContinueStatement>().label());
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
            return false;
        break;

      case PNK_RETURN:
        ok = EmitReturn(cx, bce, pn);
        break;

#if JS_HAS_GENERATORS
      case PNK_YIELD:
        JS_ASSERT(bce->sc->isFunctionBox());
        if (pn->pn_kid) {
            if (!EmitTree(cx, bce, pn->pn_kid))
                return false;
        } else {
            if (Emit1(cx, bce, JSOP_UNDEFINED) < 0)
                return false;
        }
        if (pn->pn_hidden && NewSrcNote(cx, bce, SRC_HIDDEN) < 0)
            return false;
        if (Emit1(cx, bce, JSOP_YIELD) < 0)
            return false;
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
        for (ParseNode *pn2 = pn->pn_head; ; pn2 = pn2->pn_next) {
            if (!EmitTree(cx, bce, pn2))
                return false;
            if (!pn2->pn_next)
                break;
            if (Emit1(cx, bce, JSOP_POP) < 0)
                return false;
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
        ok = EmitConditionalExpression(cx, bce, pn->as<ConditionalExpression>());
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
                return false;
            JSOp op = pn->getOp();
            while ((pn2 = pn2->pn_next) != NULL) {
                if (!EmitTree(cx, bce, pn2))
                    return false;
                if (Emit1(cx, bce, op) < 0)
                    return false;
            }
        } else {
            
            if (!EmitTree(cx, bce, pn->pn_left))
                return false;
            if (!EmitTree(cx, bce, pn->pn_right))
                return false;
            if (Emit1(cx, bce, pn->getOp()) < 0)
                return false;
        }
        break;

      case PNK_THROW:
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

      case PNK_DOT:
        




        ok = EmitPropOp(cx, pn, pn->getOp(), bce, false);
        break;

      case PNK_ELEM:
        





        ok = EmitElemOp(cx, pn, pn->getOp(), bce);
        break;

      case PNK_NEW:
      case PNK_CALL:
      case PNK_GENEXP:
        ok = EmitCallOrNew(cx, bce, pn);
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
            return false;
        slot = AdjustBlockSlot(cx, bce, bce->arrayCompDepth);
        if (slot < 0)
            return false;
        if (!EmitUnaliasedVarOp(cx, pn->getOp(), slot, bce))
            return false;
        break;
      }
#endif

      case PNK_ARRAY:
#if JS_HAS_GENERATORS
      case PNK_ARRAYCOMP:
#endif
        ok = EmitArray(cx, bce, pn);
        break;

      case PNK_OBJECT:
        ok = EmitObject(cx, bce, pn);
        break;

      case PNK_NAME:
        if (!EmitNameOp(cx, bce, pn, false))
            return false;
        break;

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

      case PNK_TRUE:
      case PNK_FALSE:
      case PNK_THIS:
      case PNK_NULL:
        if (Emit1(cx, bce, pn->getOp()) < 0)
            return false;
        break;

      case PNK_DEBUGGER:
        if (Emit1(cx, bce, JSOP_DEBUGGER) < 0)
            return false;
        break;

      case PNK_NOP:
        JS_ASSERT(pn->getArity() == PN_NULLARY);
        break;

      default:
        JS_ASSERT(0);
    }

    
    if (ok && bce->emitLevel == 1) {
        if (!UpdateSourceCoordNotes(cx, bce, pn->pn_pos.end))
            return false;
    }

    return ok;
}

static int
AllocSrcNote(JSContext *cx, SrcNotesVector &notes)
{
    
    if (notes.capacity() == 0 && !notes.reserve(1024))
        return -1;

    jssrcnote dummy = 0;
    if (!notes.append(dummy)) {
        js_ReportOutOfMemory(cx);
        return -1;
    }
    return notes.length() - 1;
}

int
frontend::NewSrcNote(JSContext *cx, BytecodeEmitter *bce, SrcNoteType type)
{
    SrcNotesVector &notes = bce->notes();
    int index;

    index = AllocSrcNote(cx, notes);
    if (index < 0)
        return -1;

    



    ptrdiff_t offset = bce->offset();
    ptrdiff_t delta = offset - bce->lastNoteOffset();
    bce->current->lastNoteOffset = offset;
    if (delta >= SN_DELTA_LIMIT) {
        do {
            ptrdiff_t xdelta = Min(delta, SN_XDELTA_MASK);
            SN_MAKE_XDELTA(&notes[index], xdelta);
            delta -= xdelta;
            index = AllocSrcNote(cx, notes);
            if (index < 0)
                return -1;
        } while (delta >= SN_DELTA_LIMIT);
    }

    




    SN_MAKE_NOTE(&notes[index], type, delta);
    for (int n = (int)js_SrcNoteSpec[type].arity; n > 0; n--) {
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

bool
frontend::AddToSrcNoteDelta(JSContext *cx, BytecodeEmitter *bce, jssrcnote *sn, ptrdiff_t delta)
{
    



    JS_ASSERT(bce->current == &bce->main);
    JS_ASSERT((unsigned) delta < (unsigned) SN_XDELTA_LIMIT);

    ptrdiff_t base = SN_DELTA(sn);
    ptrdiff_t limit = SN_IS_XDELTA(sn) ? SN_XDELTA_LIMIT : SN_DELTA_LIMIT;
    ptrdiff_t newdelta = base + delta;
    if (newdelta < limit) {
        SN_SET_DELTA(sn, newdelta);
    } else {
        jssrcnote xdelta;
        SN_MAKE_XDELTA(&xdelta, delta);
        if (!(sn = bce->main.notes.insert(sn, xdelta)))
            return false;
    }
    return true;
}

static bool
SetSrcNoteOffset(JSContext *cx, BytecodeEmitter *bce, unsigned index, unsigned which,
                 ptrdiff_t offset)
{
    if (size_t(offset) > SN_MAX_OFFSET) {
        ReportStatementTooLarge(cx, bce->topStmt);
        return false;
    }

    SrcNotesVector &notes = bce->notes();

    
    jssrcnote *sn = notes.begin() + index;
    JS_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    JS_ASSERT((int) which < js_SrcNoteSpec[SN_TYPE(sn)].arity);
    for (sn++; which; sn++, which--) {
        if (*sn & SN_3BYTE_OFFSET_FLAG)
            sn += 2;
    }

    




    if (offset > (ptrdiff_t)SN_3BYTE_OFFSET_MASK || (*sn & SN_3BYTE_OFFSET_FLAG)) {
        
        if (!(*sn & SN_3BYTE_OFFSET_FLAG)) {
            
            jssrcnote dummy = 0;
            if (!(sn = notes.insert(sn, dummy)) ||
                !(sn = notes.insert(sn, dummy)))
            {
                js_ReportOutOfMemory(cx);
                return false;
            }
        }
        *sn++ = (jssrcnote)(SN_3BYTE_OFFSET_FLAG | (offset >> 16));
        *sn++ = (jssrcnote)(offset >> 8);
    }
    *sn = (jssrcnote)offset;
    return true;
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








bool
frontend::FinishTakingSrcNotes(JSContext *cx, BytecodeEmitter *bce, jssrcnote *notes)
{
    JS_ASSERT(bce->current == &bce->main);

    unsigned prologCount = bce->prolog.notes.length();
    if (prologCount && bce->prolog.currentLine != bce->firstLine) {
        bce->switchToProlog();
        if (NewSrcNote2(cx, bce, SRC_SETLINE, (ptrdiff_t)bce->firstLine) < 0)
            return false;
        prologCount = bce->prolog.notes.length();
        bce->switchToMain();
    } else {
        






        ptrdiff_t offset = bce->prologOffset() - bce->prolog.lastNoteOffset;
        JS_ASSERT(offset >= 0);
        if (offset > 0 && bce->main.notes.length() != 0) {
            
            jssrcnote *sn = bce->main.notes.begin();
            ptrdiff_t delta = SN_IS_XDELTA(sn)
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
                delta = Min(offset, SN_XDELTA_MASK);
                sn = bce->main.notes.begin();
            }
        }
    }

    unsigned mainCount = bce->main.notes.length();
    unsigned totalCount = prologCount + mainCount;
    if (prologCount)
        PodCopy(notes, bce->prolog.notes.begin(), prologCount);
    PodCopy(notes + prologCount, bce->main.notes.begin(), mainCount);
    SN_MAKE_TERMINATOR(&notes[totalCount]);

    return true;
}

bool
CGTryNoteList::append(JSTryNoteKind kind, unsigned stackDepth, size_t start, size_t end)
{
    JS_ASSERT(unsigned(uint16_t(stackDepth)) == stackDepth);
    JS_ASSERT(start <= end);
    JS_ASSERT(size_t(uint32_t(start)) == start);
    JS_ASSERT(size_t(uint32_t(end)) == end);

    JSTryNote note;
    note.kind = kind;
    note.stackDepth = uint16_t(stackDepth);
    note.start = uint32_t(start);
    note.length = uint32_t(end - start);

    return list.append(note);
}

void
CGTryNoteList::finish(TryNoteArray *array)
{
    JS_ASSERT(length() == array->length);

    for (unsigned i = 0; i < length(); i++)
        array->vector[i] = list[i];
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
CGConstList::finish(ConstArray *array)
{
    JS_ASSERT(length() == array->length);

    for (unsigned i = 0; i < length(); i++)
        array->vector[i] = list[i];
}





JS_FRIEND_DATA(JSSrcNoteSpec) js_SrcNoteSpec[] = {
 {"null",           0},

 {"if",             0},
 {"if-else",        1},
 {"cond",           1},

 {"for",            3},

 {"while",          1},
 {"for-in",         1},
 {"continue",       0},
 {"break",          0},
 {"break2label",    0},
 {"switchbreak",    0},

 {"tableswitch",    1},
 {"condswitch",     2},

 {"nextcase",       1},

 {"assignop",       0},

 {"hidden",         0},

 {"catch",          0},

 {"colspan",        1},
 {"newline",        0},
 {"setline",        1},

 {"unused20",       0},
 {"unused21",       0},
 {"unused22",       0},
 {"unused23",       0},

 {"xdelta",         0},
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
