









#include "frontend/BytecodeEmitter.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"

#include <string.h>

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jstypes.h"
#include "jsutil.h"

#include "asmjs/AsmJSLink.h"
#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "vm/Debugger.h"
#include "vm/GeneratorObject.h"
#include "vm/Stack.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "frontend/ParseMaps-inl.h"
#include "frontend/ParseNode-inl.h"
#include "vm/NativeObject-inl.h"
#include "vm/ScopeObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::frontend;

using mozilla::DebugOnly;
using mozilla::NumberIsInt32;
using mozilla::PodCopy;
using mozilla::UniquePtr;

struct frontend::StmtInfoBCE : public StmtInfoBase
{
    StmtInfoBCE     *down;          
    StmtInfoBCE     *downScope;     

    ptrdiff_t       update;         
    ptrdiff_t       breaks;         
    ptrdiff_t       continues;      
    uint32_t        blockScopeIndex; 

    explicit StmtInfoBCE(ExclusiveContext *cx) : StmtInfoBase(cx) {}

    








    ptrdiff_t &gosubs() {
        MOZ_ASSERT(type == STMT_FINALLY);
        return breaks;
    }

    ptrdiff_t &guardJump() {
        MOZ_ASSERT(type == STMT_TRY || type == STMT_FINALLY);
        return continues;
    }
};


namespace {

struct LoopStmtInfo : public StmtInfoBCE
{
    int32_t         stackDepth;     
    uint32_t        loopDepth;      

    
    bool            canIonOsr;

    explicit LoopStmtInfo(ExclusiveContext *cx) : StmtInfoBCE(cx) {}

    static LoopStmtInfo* fromStmtInfo(StmtInfoBCE *stmt) {
        MOZ_ASSERT(stmt->isLoop());
        return static_cast<LoopStmtInfo*>(stmt);
    }
};

} 

BytecodeEmitter::BytecodeEmitter(BytecodeEmitter *parent,
                                 Parser<FullParseHandler> *parser, SharedContext *sc,
                                 HandleScript script, Handle<LazyScript *> lazyScript,
                                 bool insideEval, HandleScript evalCaller,
                                 Handle<StaticEvalObject *> staticEvalScope,
                                 bool insideNonGlobalEval, uint32_t lineNum,
                                 EmitterMode emitterMode)
  : sc(sc),
    cx(sc->context),
    parent(parent),
    script(cx, script),
    lazyScript(cx, lazyScript),
    prolog(cx, lineNum),
    main(cx, lineNum),
    current(&main),
    parser(parser),
    evalCaller(evalCaller),
    evalStaticScope(staticEvalScope),
    topStmt(nullptr),
    topScopeStmt(nullptr),
    staticScope(cx),
    atomIndices(cx),
    firstLine(lineNum),
    localsToFrameSlots_(cx),
    stackDepth(0), maxStackDepth(0),
    arrayCompDepth(0),
    emitLevel(0),
    constList(cx),
    tryNoteList(cx),
    blockScopeList(cx),
    yieldOffsetList(cx),
    typesetCount(0),
    hasSingletons(false),
    hasTryFinally(false),
    emittingForInit(false),
    emittingRunOnceLambda(false),
    insideEval(insideEval),
    insideNonGlobalEval(insideNonGlobalEval),
    emitterMode(emitterMode)
{
    MOZ_ASSERT_IF(evalCaller, insideEval);
    MOZ_ASSERT_IF(emitterMode == LazyFunction, lazyScript);
    
    MOZ_ASSERT_IF(evalStaticScope, !sc->isFunctionBox());
}

bool
BytecodeEmitter::init()
{
    return atomIndices.ensureMap(cx);
}

bool
BytecodeEmitter::updateLocalsToFrameSlots()
{
    
    
    

    if (localsToFrameSlots_.length() == script->bindings.numLocals()) {
        
        
        return true;
    }

    localsToFrameSlots_.clear();

    if (!localsToFrameSlots_.reserve(script->bindings.numLocals()))
        return false;

    uint32_t slot = 0;
    for (BindingIter bi(script); !bi.done(); bi++) {
        if (bi->kind() == Binding::ARGUMENT)
            continue;

        if (bi->aliased())
            localsToFrameSlots_.infallibleAppend(UINT32_MAX);
        else
            localsToFrameSlots_.infallibleAppend(slot++);
    }

    for (size_t i = 0; i < script->bindings.numBlockScoped(); i++)
        localsToFrameSlots_.infallibleAppend(slot++);

    return true;
}

ptrdiff_t
BytecodeEmitter::emitCheck(ptrdiff_t delta)
{
    ptrdiff_t offset = code().length();

    
    
    if (code().capacity() == 0 && !code().reserve(1024))
        return -1;

    jsbytecode dummy = 0;
    if (!code().appendN(dummy, delta)) {
        ReportOutOfMemory(cx);
        return -1;
    }
    return offset;
}

void
BytecodeEmitter::updateDepth(ptrdiff_t target)
{
    jsbytecode *pc = code(target);
    JSOp op = (JSOp) *pc;
    const JSCodeSpec *cs = &js_CodeSpec[op];

    if (cs->format & JOF_TMPSLOT_MASK) {
        



        uint32_t depth = (uint32_t) stackDepth +
                         ((cs->format & JOF_TMPSLOT_MASK) >> JOF_TMPSLOT_SHIFT);
        if (depth > maxStackDepth)
            maxStackDepth = depth;
    }

    int nuses = StackUses(nullptr, pc);
    int ndefs = StackDefs(nullptr, pc);

    stackDepth -= nuses;
    MOZ_ASSERT(stackDepth >= 0);
    stackDepth += ndefs;
    if ((uint32_t)stackDepth > maxStackDepth)
        maxStackDepth = stackDepth;
}

#ifdef DEBUG
static bool
CheckStrictOrSloppy(BytecodeEmitter *bce, JSOp op)
{
    if (IsCheckStrictOp(op) && !bce->sc->strict())
        return false;
    if (IsCheckSloppyOp(op) && bce->sc->strict())
        return false;
    return true;
}
#endif

bool
BytecodeEmitter::emit1(JSOp op)
{
    MOZ_ASSERT(CheckStrictOrSloppy(this, op));
    ptrdiff_t offset = emitCheck(1);
    if (offset < 0)
        return false;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    updateDepth(offset);
    return true;
}

bool
BytecodeEmitter::emit2(JSOp op, jsbytecode op1)
{
    MOZ_ASSERT(CheckStrictOrSloppy(this, op));
    ptrdiff_t offset = emitCheck(2);
    if (offset < 0)
        return false;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    code[1] = op1;
    updateDepth(offset);
    return true;
}

bool
BytecodeEmitter::emit3(JSOp op, jsbytecode op1, jsbytecode op2)
{
    MOZ_ASSERT(CheckStrictOrSloppy(this, op));

    
    MOZ_ASSERT(!IsArgOp(op));
    MOZ_ASSERT(!IsLocalOp(op));

    ptrdiff_t offset = emitCheck(3);
    if (offset < 0)
        return false;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    code[1] = op1;
    code[2] = op2;
    updateDepth(offset);
    return true;
}

ptrdiff_t
BytecodeEmitter::emitN(JSOp op, size_t extra)
{
    MOZ_ASSERT(CheckStrictOrSloppy(this, op));
    ptrdiff_t length = 1 + (ptrdiff_t)extra;
    ptrdiff_t offset = emitCheck(length);
    if (offset < 0)
        return -1;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    

    



    if (js_CodeSpec[op].nuses >= 0)
        updateDepth(offset);

    return offset;
}

ptrdiff_t
BytecodeEmitter::emitJump(JSOp op, ptrdiff_t off)
{
    ptrdiff_t offset = emitCheck(5);
    if (offset < 0)
        return -1;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    SET_JUMP_OFFSET(code, off);
    updateDepth(offset);
    return offset;
}

bool
BytecodeEmitter::emitCall(JSOp op, uint16_t argc, ParseNode *pn)
{
    if (pn && !updateSourceCoordNotes(pn->pn_pos.begin))
        return false;
    return emit3(op, ARGC_HI(argc), ARGC_LO(argc));
}

bool
BytecodeEmitter::emitDupAt(unsigned slot)
{
    MOZ_ASSERT(slot < unsigned(stackDepth));

    
    unsigned slotFromTop = stackDepth - 1 - slot;
    if (slotFromTop >= JS_BIT(24)) {
        reportError(nullptr, JSMSG_TOO_MANY_LOCALS);
        return false;
    }

    ptrdiff_t off = emitN(JSOP_DUPAT, 3);
    if (off < 0)
        return false;

    jsbytecode *pc = code(off);
    SET_UINT24(pc, slotFromTop);
    return true;
}


const char js_with_statement_str[] = "with statement";
const char js_finally_block_str[]  = "finally block";
const char js_script_str[]         = "script";

static const char * const statementName[] = {
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
    "for/of loop",           
    "while loop",            
    "spread",                
};

static_assert(MOZ_ARRAY_LENGTH(statementName) == STMT_LIMIT,
              "statementName array and StmtType enum must be consistent");

static const char *
StatementName(StmtInfoBCE *topStmt)
{
    if (!topStmt)
        return js_script_str;
    return statementName[topStmt->type];
}

static void
ReportStatementTooLarge(TokenStream &ts, StmtInfoBCE *topStmt)
{
    ts.reportError(JSMSG_NEED_DIET, StatementName(topStmt));
}





bool
BytecodeEmitter::emitBackPatchOp(ptrdiff_t *lastp)
{
    ptrdiff_t delta = offset() - *lastp;
    *lastp = offset();
    MOZ_ASSERT(delta > 0);
    return emitJump(JSOP_BACKPATCH, delta) >= 0;
}

static inline unsigned
LengthOfSetLine(unsigned line)
{
    return 1  + (line > SN_4BYTE_OFFSET_MASK ? 4 : 1);
}


bool
BytecodeEmitter::updateLineNumberNotes(uint32_t offset)
{
    TokenStream *ts = &parser->tokenStream;
    if (!ts->srcCoords.isOnThisLine(offset, currentLine())) {
        unsigned line = ts->srcCoords.lineNum(offset);
        unsigned delta = line - currentLine();

        










        current->currentLine = line;
        current->lastColumn  = 0;
        if (delta >= LengthOfSetLine(line)) {
            if (NewSrcNote2(cx, this, SRC_SETLINE, (ptrdiff_t)line) < 0)
                return false;
        } else {
            do {
                if (NewSrcNote(cx, this, SRC_NEWLINE) < 0)
                    return false;
            } while (--delta != 0);
        }
    }
    return true;
}


bool
BytecodeEmitter::updateSourceCoordNotes(uint32_t offset)
{
    if (!updateLineNumberNotes(offset))
        return false;

    uint32_t columnIndex = parser->tokenStream.srcCoords.columnIndex(offset);
    ptrdiff_t colspan = ptrdiff_t(columnIndex) - ptrdiff_t(current->lastColumn);
    if (colspan != 0) {
        
        
        
        
        
        if (!SN_REPRESENTABLE_COLSPAN(colspan))
            return true;
        if (NewSrcNote2(cx, this, SRC_COLSPAN, SN_COLSPAN_TO_OFFSET(colspan)) < 0)
            return false;
        current->lastColumn = columnIndex;
    }
    return true;
}

bool
BytecodeEmitter::emitLoopHead(ParseNode *nextpn)
{
    if (nextpn) {
        




        MOZ_ASSERT_IF(nextpn->isKind(PNK_STATEMENTLIST), nextpn->isArity(PN_LIST));
        if (nextpn->isKind(PNK_STATEMENTLIST) && nextpn->pn_head)
            nextpn = nextpn->pn_head;
        if (!updateSourceCoordNotes(nextpn->pn_pos.begin))
            return false;
    }

    return emit1(JSOP_LOOPHEAD);
}

bool
BytecodeEmitter::emitLoopEntry(ParseNode *nextpn)
{
    if (nextpn) {
        
        MOZ_ASSERT_IF(nextpn->isKind(PNK_STATEMENTLIST), nextpn->isArity(PN_LIST));
        if (nextpn->isKind(PNK_STATEMENTLIST) && nextpn->pn_head)
            nextpn = nextpn->pn_head;
        if (!updateSourceCoordNotes(nextpn->pn_pos.begin))
            return false;
    }

    LoopStmtInfo *loop = LoopStmtInfo::fromStmtInfo(topStmt);
    MOZ_ASSERT(loop->loopDepth > 0);

    uint8_t loopDepthAndFlags = PackLoopEntryDepthHintAndFlags(loop->loopDepth, loop->canIonOsr);
    return emit2(JSOP_LOOPENTRY, loopDepthAndFlags);
}

void
BytecodeEmitter::checkTypeSet(JSOp op)
{
    if (js_CodeSpec[op].format & JOF_TYPESET) {
        if (typesetCount < UINT16_MAX)
            typesetCount++;
    }
}

bool
BytecodeEmitter::emitUint16Operand(JSOp op, uint32_t i)
{
    MOZ_ASSERT(i <= UINT16_MAX);
    if (!emit3(op, UINT16_HI(i), UINT16_LO(i)))
        return false;
    checkTypeSet(op);
    return true;
}

bool
BytecodeEmitter::flushPops(int *npops)
{
    MOZ_ASSERT(*npops != 0);
    if (!emitUint16Operand(JSOP_POPN, *npops))
        return false;

    *npops = 0;
    return true;
}

static bool
PopIterator(ExclusiveContext *cx, BytecodeEmitter *bce)
{
    return bce->emit1(JSOP_ENDITER);
}

namespace {

class NonLocalExitScope {
    ExclusiveContext *cx;
    BytecodeEmitter *bce;
    const uint32_t savedScopeIndex;
    const int savedDepth;
    uint32_t openScopeIndex;

    NonLocalExitScope(const NonLocalExitScope &) = delete;

  public:
    explicit NonLocalExitScope(ExclusiveContext *cx_, BytecodeEmitter *bce_)
      : cx(cx_),
        bce(bce_),
        savedScopeIndex(bce->blockScopeList.length()),
        savedDepth(bce->stackDepth),
        openScopeIndex(UINT32_MAX) {
        if (bce->staticScope) {
            StmtInfoBCE *stmt = bce->topStmt;
            while (1) {
                MOZ_ASSERT(stmt);
                if (stmt->isNestedScope) {
                    openScopeIndex = stmt->blockScopeIndex;
                    break;
                }
                stmt = stmt->down;
            }
        }
    }

    ~NonLocalExitScope() {
        for (uint32_t n = savedScopeIndex; n < bce->blockScopeList.length(); n++)
            bce->blockScopeList.recordEnd(n, bce->offset());
        bce->stackDepth = savedDepth;
    }

    bool popScopeForNonLocalExit(uint32_t blockScopeIndex) {
        uint32_t scopeObjectIndex = bce->blockScopeList.findEnclosingScope(blockScopeIndex);
        uint32_t parent = openScopeIndex;

        if (!bce->blockScopeList.append(scopeObjectIndex, bce->offset(), parent))
            return false;
        openScopeIndex = bce->blockScopeList.length() - 1;
        return true;
    }

    bool prepareForNonLocalJump(StmtInfoBCE *toStmt);
};




bool
NonLocalExitScope::prepareForNonLocalJump(StmtInfoBCE *toStmt)
{
    int npops = 0;

#define FLUSH_POPS() if (npops && !bce->flushPops(&npops)) return false

    for (StmtInfoBCE *stmt = bce->topStmt; stmt != toStmt; stmt = stmt->down) {
        switch (stmt->type) {
          case STMT_FINALLY:
            FLUSH_POPS();
            if (!bce->emitBackPatchOp(&stmt->gosubs()))
                return false;
            break;

          case STMT_WITH:
            if (!bce->emit1(JSOP_LEAVEWITH))
                return false;
            MOZ_ASSERT(stmt->isNestedScope);
            if (!popScopeForNonLocalExit(stmt->blockScopeIndex))
                return false;
            break;

          case STMT_FOR_OF_LOOP:
            npops += 2;
            break;

          case STMT_FOR_IN_LOOP:
            
            npops += 1;
            FLUSH_POPS();
            if (!PopIterator(cx, bce))
                return false;
            break;

          case STMT_SPREAD:
            MOZ_ASSERT_UNREACHABLE("can't break/continue/return from inside a spread");
            break;

          case STMT_SUBROUTINE:
            



            npops += 2;
            break;

          default:;
        }

        if (stmt->isBlockScope) {
            MOZ_ASSERT(stmt->isNestedScope);
            StaticBlockObject &blockObj = stmt->staticBlock();
            if (!bce->emit1(JSOP_DEBUGLEAVEBLOCK))
                return false;
            if (!popScopeForNonLocalExit(stmt->blockScopeIndex))
                return false;
            if (blockObj.needsClone()) {
                if (!bce->emit1(JSOP_POPBLOCKSCOPE))
                    return false;
            }
        }
    }

    FLUSH_POPS();
    return true;

#undef FLUSH_POPS
}

}  

ptrdiff_t
BytecodeEmitter::emitGoto(StmtInfoBCE *toStmt, ptrdiff_t *lastp, SrcNoteType noteType)
{
    NonLocalExitScope nle(cx, this);

    if (!nle.prepareForNonLocalJump(toStmt))
        return -1;

    if (noteType != SRC_NULL) {
        if (NewSrcNote(cx, this, noteType) < 0)
            return -1;
    }

    if (!emitBackPatchOp(lastp))
        return -1;
    return *lastp;
}

void
BytecodeEmitter::backPatch(ptrdiff_t last, jsbytecode *target, jsbytecode op)
{
    jsbytecode *pc = code(last);
    jsbytecode *stop = code(-1);
    while (pc != stop) {
        ptrdiff_t delta = GET_JUMP_OFFSET(pc);
        ptrdiff_t span = target - pc;
        SET_JUMP_OFFSET(pc, span);
        *pc = op;
        pc -= delta;
    }
}

#define SET_STATEMENT_TOP(stmt, top)                                          \
    ((stmt)->update = (top), (stmt)->breaks = (stmt)->continues = (-1))

void
BytecodeEmitter::pushStatementInner(StmtInfoBCE *stmt, StmtType type, ptrdiff_t top)
{
    SET_STATEMENT_TOP(stmt, top);
    PushStatement(this, stmt, type);
}

void
BytecodeEmitter::pushStatement(StmtInfoBCE *stmt, StmtType type, ptrdiff_t top)
{
    pushStatementInner(stmt, type, top);
    MOZ_ASSERT(!stmt->isLoop());
}

static void
PushLoopStatement(BytecodeEmitter *bce, LoopStmtInfo *stmt, StmtType type, ptrdiff_t top)
{
    bce->pushStatementInner(stmt, type, top);
    MOZ_ASSERT(stmt->isLoop());

    LoopStmtInfo *downLoop = nullptr;
    for (StmtInfoBCE *outer = stmt->down; outer; outer = outer->down) {
        if (outer->isLoop()) {
            downLoop = LoopStmtInfo::fromStmtInfo(outer);
            break;
        }
    }

    stmt->stackDepth = bce->stackDepth;
    stmt->loopDepth = downLoop ? downLoop->loopDepth + 1 : 1;

    int loopSlots;
    if (type == STMT_SPREAD)
        loopSlots = 3;
    else if (type == STMT_FOR_IN_LOOP || type == STMT_FOR_OF_LOOP)
        loopSlots = 2;
    else
        loopSlots = 0;

    MOZ_ASSERT(loopSlots <= stmt->stackDepth);

    if (downLoop)
        stmt->canIonOsr = (downLoop->canIonOsr &&
                           stmt->stackDepth == downLoop->stackDepth + loopSlots);
    else
        stmt->canIonOsr = stmt->stackDepth == loopSlots;
}





static JSObject *
EnclosingStaticScope(BytecodeEmitter *bce)
{
    if (bce->staticScope)
        return bce->staticScope;

    if (!bce->sc->isFunctionBox()) {
        MOZ_ASSERT(!bce->parent);

        
        
        return bce->evalStaticScope;
    }

    return bce->sc->asFunctionBox()->function();
}

#ifdef DEBUG
static bool
AllLocalsAliased(StaticBlockObject &obj)
{
    for (unsigned i = 0; i < obj.numVariables(); i++)
        if (!obj.isAliased(i))
            return false;
    return true;
}
#endif

static bool
ComputeAliasedSlots(ExclusiveContext *cx, BytecodeEmitter *bce, Handle<StaticBlockObject *> blockObj)
{
    uint32_t numAliased = bce->script->bindings.numAliasedBodyLevelLocals();

    for (unsigned i = 0; i < blockObj->numVariables(); i++) {
        Definition *dn = blockObj->definitionParseNode(i);

        MOZ_ASSERT(dn->isDefn());

        
        
        
        if (!dn->pn_cookie.set(bce->parser->tokenStream, dn->pn_cookie.level(),
                               numAliased + blockObj->blockIndexToLocalIndex(dn->frameSlot())))
        {
            return false;
        }

#ifdef DEBUG
        for (ParseNode *pnu = dn->dn_uses; pnu; pnu = pnu->pn_link) {
            MOZ_ASSERT(pnu->pn_lexdef == dn);
            MOZ_ASSERT(!(pnu->pn_dflags & PND_BOUND));
            MOZ_ASSERT(pnu->pn_cookie.isFree());
        }
#endif

        blockObj->setAliased(i, bce->isAliasedName(dn));
    }

    MOZ_ASSERT_IF(bce->sc->allLocalsAliased(), AllLocalsAliased(*blockObj));

    return true;
}





static void
ComputeLocalOffset(ExclusiveContext *cx, BytecodeEmitter *bce, Handle<StaticBlockObject *> blockObj)
{
    unsigned nbodyfixed = bce->sc->isFunctionBox()
                          ? bce->script->bindings.numUnaliasedBodyLevelLocals()
                          : 0;
    unsigned localOffset = nbodyfixed;

    if (bce->staticScope) {
        Rooted<NestedScopeObject *> outer(cx, bce->staticScope);
        for (; outer; outer = outer->enclosingNestedScope()) {
            if (outer->is<StaticBlockObject>()) {
                StaticBlockObject &outerBlock = outer->as<StaticBlockObject>();
                localOffset = outerBlock.localOffset() + outerBlock.numVariables();
                break;
            }
        }
    }

    MOZ_ASSERT(localOffset + blockObj->numVariables()
               <= nbodyfixed + bce->script->bindings.numBlockScoped());

    blockObj->setLocalOffset(localOffset);
}















































static bool
EnterNestedScope(ExclusiveContext *cx, BytecodeEmitter *bce, StmtInfoBCE *stmt, ObjectBox *objbox,
                 StmtType stmtType)
{
    Rooted<NestedScopeObject *> scopeObj(cx, &objbox->object->as<NestedScopeObject>());
    uint32_t scopeObjectIndex = bce->objectList.add(objbox);

    switch (stmtType) {
      case STMT_BLOCK: {
        Rooted<StaticBlockObject *> blockObj(cx, &scopeObj->as<StaticBlockObject>());

        ComputeLocalOffset(cx, bce, blockObj);

        if (!ComputeAliasedSlots(cx, bce, blockObj))
            return false;

        if (blockObj->needsClone()) {
            if (!bce->emitInternedObjectOp(scopeObjectIndex, JSOP_PUSHBLOCKSCOPE))
                return false;
        }
        break;
      }
      case STMT_WITH:
        MOZ_ASSERT(scopeObj->is<StaticWithObject>());
        if (!bce->emitInternedObjectOp(scopeObjectIndex, JSOP_ENTERWITH))
            return false;
        break;
      default:
        MOZ_CRASH("Unexpected scope statement");
    }

    uint32_t parent = BlockScopeNote::NoBlockScopeIndex;
    if (StmtInfoBCE *stmt = bce->topScopeStmt) {
        for (; stmt->staticScope != bce->staticScope; stmt = stmt->down) {}
        parent = stmt->blockScopeIndex;
    }

    stmt->blockScopeIndex = bce->blockScopeList.length();
    if (!bce->blockScopeList.append(scopeObjectIndex, bce->offset(), parent))
        return false;

    bce->pushStatement(stmt, stmtType, bce->offset());
    scopeObj->initEnclosingNestedScope(EnclosingStaticScope(bce));
    FinishPushNestedScope(bce, stmt, *scopeObj);
    MOZ_ASSERT(stmt->isNestedScope);
    stmt->isBlockScope = (stmtType == STMT_BLOCK);

    return true;
}



void
BytecodeEmitter::popStatement()
{
    if (!topStmt->isTrying()) {
        backPatch(topStmt->breaks, code().end(), JSOP_GOTO);
        backPatch(topStmt->continues, code(topStmt->update), JSOP_GOTO);
    }

    FinishPopStatement(this);
}

static bool
LeaveNestedScope(ExclusiveContext *cx, BytecodeEmitter *bce, StmtInfoBCE *stmt)
{
    MOZ_ASSERT(stmt == bce->topStmt);
    MOZ_ASSERT(stmt->isNestedScope);
    MOZ_ASSERT(stmt->isBlockScope == !(stmt->type == STMT_WITH));
    uint32_t blockScopeIndex = stmt->blockScopeIndex;

#ifdef DEBUG
    MOZ_ASSERT(bce->blockScopeList.list[blockScopeIndex].length == 0);
    uint32_t blockObjIndex = bce->blockScopeList.list[blockScopeIndex].index;
    ObjectBox *blockObjBox = bce->objectList.find(blockObjIndex);
    NestedScopeObject *staticScope = &blockObjBox->object->as<NestedScopeObject>();
    MOZ_ASSERT(stmt->staticScope == staticScope);
    MOZ_ASSERT(staticScope == bce->staticScope);
    MOZ_ASSERT_IF(!stmt->isBlockScope, staticScope->is<StaticWithObject>());
#endif

    bce->popStatement();

    if (!bce->emit1(stmt->isBlockScope ? JSOP_DEBUGLEAVEBLOCK : JSOP_LEAVEWITH))
        return false;

    bce->blockScopeList.recordEnd(blockScopeIndex, bce->offset());

    if (stmt->isBlockScope && stmt->staticScope->as<StaticBlockObject>().needsClone()) {
        if (!bce->emit1(JSOP_POPBLOCKSCOPE))
            return false;
    }

    return true;
}

bool
BytecodeEmitter::emitIndex32(JSOp op, uint32_t index)
{
    MOZ_ASSERT(CheckStrictOrSloppy(this, op));

    const size_t len = 1 + UINT32_INDEX_LEN;
    MOZ_ASSERT(len == size_t(js_CodeSpec[op].length));

    ptrdiff_t offset = emitCheck(len);
    if (offset < 0)
        return false;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    SET_UINT32_INDEX(code, index);
    updateDepth(offset);
    checkTypeSet(op);
    return true;
}

bool
BytecodeEmitter::emitIndexOp(JSOp op, uint32_t index)
{
    MOZ_ASSERT(CheckStrictOrSloppy(this, op));

    const size_t len = js_CodeSpec[op].length;
    MOZ_ASSERT(len >= 1 + UINT32_INDEX_LEN);

    ptrdiff_t offset = emitCheck(len);
    if (offset < 0)
        return false;

    jsbytecode *code = this->code(offset);
    code[0] = jsbytecode(op);
    SET_UINT32_INDEX(code, index);
    updateDepth(offset);
    checkTypeSet(op);
    return true;
}

bool
BytecodeEmitter::emitAtomOp(JSAtom *atom, JSOp op)
{
    MOZ_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

    
    
    MOZ_ASSERT_IF(op == JSOP_GETNAME || op == JSOP_GETGNAME, !sc->isDotVariable(atom));

    if (op == JSOP_GETPROP && atom == cx->names().length) {
        
        op = JSOP_LENGTH;
    }

    jsatomid index;
    if (!makeAtomIndex(atom, &index))
        return false;

    return emitIndexOp(op, index);
}

bool
BytecodeEmitter::emitAtomOp(ParseNode *pn, JSOp op)
{
    MOZ_ASSERT(pn->pn_atom != nullptr);
    return emitAtomOp(pn->pn_atom, op);
}

bool
BytecodeEmitter::emitInternedObjectOp(uint32_t index, JSOp op)
{
    MOZ_ASSERT(JOF_OPTYPE(op) == JOF_OBJECT);
    MOZ_ASSERT(index < objectList.length);
    return emitIndex32(op, index);
}

bool
BytecodeEmitter::emitObjectOp(ObjectBox *objbox, JSOp op)
{
    return emitInternedObjectOp(objectList.add(objbox), op);
}

bool
BytecodeEmitter::emitObjectPairOp(ObjectBox *objbox1, ObjectBox *objbox2, JSOp op)
{
    uint32_t index = objectList.add(objbox1);
    objectList.add(objbox2);
    return emitInternedObjectOp(index, op);
}

bool
BytecodeEmitter::emitRegExp(uint32_t index)
{
    return emitIndex32(JSOP_REGEXP, index);
}

bool
BytecodeEmitter::emitLocalOp(JSOp op, uint32_t slot)
{
    MOZ_ASSERT(JOF_OPTYPE(op) != JOF_SCOPECOORD);
    MOZ_ASSERT(IsLocalOp(op));

    ptrdiff_t off = emitN(op, LOCALNO_LEN);
    if (off < 0)
        return false;

    SET_LOCALNO(code(off), slot);
    return true;
}

bool
BytecodeEmitter::emitUnaliasedVarOp(JSOp op, uint32_t slot, MaybeCheckLexical checkLexical)
{
    MOZ_ASSERT(JOF_OPTYPE(op) != JOF_SCOPECOORD);

    if (IsLocalOp(op)) {
        
        
        
        MOZ_ASSERT(localsToFrameSlots_[slot] <= slot);
        slot = localsToFrameSlots_[slot];

        if (checkLexical) {
            MOZ_ASSERT(op != JSOP_INITLEXICAL);
            if (!emitLocalOp(JSOP_CHECKLEXICAL, slot))
                return false;
        }

        return emitLocalOp(op, slot);
    }

    MOZ_ASSERT(IsArgOp(op));
    ptrdiff_t off = emitN(op, ARGNO_LEN);
    if (off < 0)
        return false;

    SET_ARGNO(code(off), slot);
    return true;
}

bool
BytecodeEmitter::emitScopeCoordOp(JSOp op, ScopeCoordinate sc)
{
    MOZ_ASSERT(JOF_OPTYPE(op) == JOF_SCOPECOORD);

    unsigned n = SCOPECOORD_HOPS_LEN + SCOPECOORD_SLOT_LEN;
    MOZ_ASSERT(int(n) + 1  == js_CodeSpec[op].length);

    ptrdiff_t off = emitN(op, n);
    if (off < 0)
        return false;

    jsbytecode *pc = code(off);
    SET_SCOPECOORD_HOPS(pc, sc.hops());
    pc += SCOPECOORD_HOPS_LEN;
    SET_SCOPECOORD_SLOT(pc, sc.slot());
    pc += SCOPECOORD_SLOT_LEN;
    checkTypeSet(op);
    return true;
}

bool
BytecodeEmitter::emitAliasedVarOp(JSOp op, ScopeCoordinate sc, MaybeCheckLexical checkLexical)
{
    if (checkLexical) {
        MOZ_ASSERT(op != JSOP_INITALIASEDLEXICAL);
        if (!emitScopeCoordOp(JSOP_CHECKALIASEDLEXICAL, sc))
            return false;
    }

    return emitScopeCoordOp(op, sc);
}



static unsigned
DynamicNestedScopeDepth(BytecodeEmitter *bce)
{
    unsigned depth = 0;
    for (NestedScopeObject *b = bce->staticScope; b; b = b->enclosingNestedScope()) {
        if (!b->is<StaticBlockObject>() || b->as<StaticBlockObject>().needsClone())
            ++depth;
    }

    return depth;
}

static bool
LookupAliasedName(BytecodeEmitter *bce, HandleScript script, PropertyName *name, uint32_t *pslot,
                  ParseNode *pn = nullptr)
{
    LazyScript::FreeVariable *freeVariables = nullptr;
    uint32_t lexicalBegin = 0;
    uint32_t numFreeVariables = 0;
    if (bce->emitterMode == BytecodeEmitter::LazyFunction) {
        freeVariables = bce->lazyScript->freeVariables();
        lexicalBegin = script->bindings.lexicalBegin();
        numFreeVariables = bce->lazyScript->numFreeVariables();
    }

    



    uint32_t bindingIndex = 0;
    uint32_t slot = CallObject::RESERVED_SLOTS;
    for (BindingIter bi(script); !bi.done(); bi++) {
        if (bi->aliased()) {
            if (bi->name() == name) {
                
                
                
                if (freeVariables) {
                    for (uint32_t i = 0; i < numFreeVariables; i++) {
                        if (freeVariables[i].atom() == name) {
                            if (freeVariables[i].isHoistedUse() && bindingIndex >= lexicalBegin) {
                                MOZ_ASSERT(pn);
                                MOZ_ASSERT(pn->isUsed());
                                pn->pn_dflags |= PND_LEXICAL;
                            }

                            break;
                        }
                    }
                }

                *pslot = slot;
                return true;
            }
            slot++;
        }
        bindingIndex++;
    }
    return false;
}

static bool
LookupAliasedNameSlot(BytecodeEmitter *bce, HandleScript script, PropertyName *name,
                      ScopeCoordinate *sc)
{
    uint32_t slot;
    if (!LookupAliasedName(bce, script, name, &slot))
        return false;

    sc->setSlot(slot);
    return true;
}





static bool
AssignHops(BytecodeEmitter *bce, ParseNode *pn, unsigned src, ScopeCoordinate *dst)
{
    if (src > UINT8_MAX) {
        bce->reportError(pn, JSMSG_TOO_DEEP, js_function_str);
        return false;
    }

    dst->setHops(src);
    return true;
}

static inline MaybeCheckLexical
NodeNeedsCheckLexical(ParseNode *pn)
{
    return pn->isHoistedLexicalUse() ? CheckLexical : DontCheckLexical;
}

bool
BytecodeEmitter::emitAliasedVarOp(JSOp op, ParseNode *pn)
{
    








    unsigned skippedScopes = 0;
    BytecodeEmitter *bceOfDef = this;
    if (pn->isUsed()) {
        




        for (unsigned i = pn->pn_cookie.level(); i; i--) {
            skippedScopes += DynamicNestedScopeDepth(bceOfDef);
            FunctionBox *funbox = bceOfDef->sc->asFunctionBox();
            if (funbox->isHeavyweight()) {
                skippedScopes++;
                if (funbox->function()->isNamedLambda())
                    skippedScopes++;
            }
            bceOfDef = bceOfDef->parent;
        }
    } else {
        MOZ_ASSERT(pn->isDefn());
        MOZ_ASSERT(pn->pn_cookie.level() == script->staticLevel());
    }

    






    ScopeCoordinate sc;
    if (IsArgOp(pn->getOp())) {
        if (!AssignHops(this, pn, skippedScopes + DynamicNestedScopeDepth(bceOfDef), &sc))
            return false;
        JS_ALWAYS_TRUE(LookupAliasedNameSlot(bceOfDef, bceOfDef->script, pn->name(), &sc));
    } else {
        MOZ_ASSERT(IsLocalOp(pn->getOp()) || pn->isKind(PNK_FUNCTION));
        uint32_t local = pn->pn_cookie.slot();
        if (local < bceOfDef->script->bindings.numBodyLevelLocals()) {
            if (!AssignHops(this, pn, skippedScopes + DynamicNestedScopeDepth(bceOfDef), &sc))
                return false;
            JS_ALWAYS_TRUE(LookupAliasedNameSlot(bceOfDef, bceOfDef->script, pn->name(), &sc));
        } else {
            MOZ_ASSERT_IF(this->sc->isFunctionBox(), local <= bceOfDef->script->bindings.numLocals());
            MOZ_ASSERT(bceOfDef->staticScope->is<StaticBlockObject>());
            Rooted<StaticBlockObject*> b(cx, &bceOfDef->staticScope->as<StaticBlockObject>());
            local = bceOfDef->localsToFrameSlots_[local];
            while (local < b->localOffset()) {
                if (b->needsClone())
                    skippedScopes++;
                b = &b->enclosingNestedScope()->as<StaticBlockObject>();
            }
            if (!AssignHops(this, pn, skippedScopes, &sc))
                return false;
            sc.setSlot(b->localIndexToSlot(local));
        }
    }

    return emitAliasedVarOp(op, sc, NodeNeedsCheckLexical(pn));
}

bool
BytecodeEmitter::emitVarOp(ParseNode *pn, JSOp op)
{
    MOZ_ASSERT(pn->isKind(PNK_FUNCTION) || pn->isKind(PNK_NAME));
    MOZ_ASSERT(!pn->pn_cookie.isFree());

    if (IsAliasedVarOp(op)) {
        ScopeCoordinate sc;
        sc.setHops(pn->pn_cookie.level());
        sc.setSlot(pn->pn_cookie.slot());
        return emitAliasedVarOp(op, sc, NodeNeedsCheckLexical(pn));
    }

    MOZ_ASSERT_IF(pn->isKind(PNK_NAME), IsArgOp(op) || IsLocalOp(op));

    if (!isAliasedName(pn)) {
        MOZ_ASSERT(pn->isUsed() || pn->isDefn());
        MOZ_ASSERT_IF(pn->isUsed(), pn->pn_cookie.level() == 0);
        MOZ_ASSERT_IF(pn->isDefn(), pn->pn_cookie.level() == script->staticLevel());
        return emitUnaliasedVarOp(op, pn->pn_cookie.slot(), NodeNeedsCheckLexical(pn));
    }

    switch (op) {
      case JSOP_GETARG: case JSOP_GETLOCAL: op = JSOP_GETALIASEDVAR; break;
      case JSOP_SETARG: case JSOP_SETLOCAL: op = JSOP_SETALIASEDVAR; break;
      case JSOP_INITLEXICAL: op = JSOP_INITALIASEDLEXICAL; break;
      default: MOZ_CRASH("unexpected var op");
    }

    return emitAliasedVarOp(op, pn);
}

static JSOp
GetIncDecInfo(ParseNodeKind kind, bool *post)
{
    MOZ_ASSERT(kind == PNK_POSTINCREMENT || kind == PNK_PREINCREMENT ||
               kind == PNK_POSTDECREMENT || kind == PNK_PREDECREMENT);
    *post = kind == PNK_POSTINCREMENT || kind == PNK_POSTDECREMENT;
    return (kind == PNK_POSTINCREMENT || kind == PNK_PREINCREMENT) ? JSOP_ADD : JSOP_SUB;
}

bool
BytecodeEmitter::emitVarIncDec(ParseNode *pn)
{
    JSOp op = pn->pn_kid->getOp();
    MOZ_ASSERT(IsArgOp(op) || IsLocalOp(op) || IsAliasedVarOp(op));
    MOZ_ASSERT(pn->pn_kid->isKind(PNK_NAME));
    MOZ_ASSERT(!pn->pn_kid->pn_cookie.isFree());

    bool post;
    JSOp binop = GetIncDecInfo(pn->getKind(), &post);

    JSOp getOp, setOp;
    if (IsLocalOp(op)) {
        getOp = JSOP_GETLOCAL;
        setOp = JSOP_SETLOCAL;
    } else if (IsArgOp(op)) {
        getOp = JSOP_GETARG;
        setOp = JSOP_SETARG;
    } else {
        getOp = JSOP_GETALIASEDVAR;
        setOp = JSOP_SETALIASEDVAR;
    }

    if (!emitVarOp(pn->pn_kid, getOp))                       
        return false;
    if (!emit1(JSOP_POS))                                    
        return false;
    if (post && !emit1(JSOP_DUP))                            
        return false;
    if (!emit1(JSOP_ONE))                                    
        return false;
    if (!emit1(binop))                                       
        return false;
    if (!emitVarOp(pn->pn_kid, setOp))                       
        return false;
    if (post && !emit1(JSOP_POP))                            
        return false;

    return true;
}

bool
BytecodeEmitter::isAliasedName(ParseNode *pn)
{
    Definition *dn = pn->resolve();
    MOZ_ASSERT(dn->isDefn());
    MOZ_ASSERT(!dn->isPlaceholder());
    MOZ_ASSERT(dn->isBound());

    
    if (dn->pn_cookie.level() != script->staticLevel())
        return true;

    switch (dn->kind()) {
      case Definition::LET:
      case Definition::CONST:
        









        return dn->isClosed() || sc->allLocalsAliased();
      case Definition::ARG:
        









        return script->formalIsAliased(pn->pn_cookie.slot());
      case Definition::VAR:
      case Definition::GLOBALCONST:
        MOZ_ASSERT_IF(sc->allLocalsAliased(), script->cookieIsAliased(pn->pn_cookie));
        return script->cookieIsAliased(pn->pn_cookie);
      case Definition::PLACEHOLDER:
      case Definition::NAMED_LAMBDA:
      case Definition::MISSING:
        MOZ_CRASH("unexpected dn->kind");
    }
    return false;
}

static JSOp
StrictifySetNameOp(JSOp op, BytecodeEmitter *bce)
{
    switch (op) {
      case JSOP_SETNAME:
        if (bce->sc->strict())
            op = JSOP_STRICTSETNAME;
        break;
      case JSOP_SETGNAME:
        if (bce->sc->strict())
            op = JSOP_STRICTSETGNAME;
        break;
        default:;
    }
    return op;
}

static void
StrictifySetNameNode(ParseNode *pn, BytecodeEmitter *bce)
{
    pn->setOp(StrictifySetNameOp(pn->getOp(), bce));
}






static bool
TryConvertFreeName(BytecodeEmitter *bce, ParseNode *pn)
{
    





    if (bce->emitterMode == BytecodeEmitter::SelfHosting) {
        JSOp op;
        switch (pn->getOp()) {
          case JSOP_GETNAME:  op = JSOP_GETINTRINSIC; break;
          case JSOP_SETNAME:  op = JSOP_SETINTRINSIC; break;
          
          default: MOZ_CRASH("intrinsic");
        }
        pn->setOp(op);
        return true;
    }

    




    if (bce->emitterMode == BytecodeEmitter::LazyFunction) {
        
        
        for (StmtInfoBCE *stmt = bce->topStmt; stmt; stmt = stmt->down) {
            if (stmt->type == STMT_CATCH)
                return true;
        }

        size_t hops = 0;
        FunctionBox *funbox = bce->sc->asFunctionBox();
        if (funbox->hasExtensibleScope())
            return false;
        if (funbox->function()->isNamedLambda() && funbox->function()->atom() == pn->pn_atom)
            return false;
        if (funbox->isHeavyweight()) {
            hops++;
            if (funbox->function()->isNamedLambda())
                hops++;
        }
        if (bce->script->directlyInsideEval())
            return false;
        RootedObject outerScope(bce->cx, bce->script->enclosingStaticScope());
        for (StaticScopeIter<CanGC> ssi(bce->cx, outerScope); !ssi.done(); ssi++) {
            if (ssi.type() != StaticScopeIter<CanGC>::Function) {
                if (ssi.type() == StaticScopeIter<CanGC>::Block) {
                    
                    return false;
                }
                if (ssi.hasDynamicScopeObject())
                    hops++;
                continue;
            }
            RootedScript script(bce->cx, ssi.funScript());
            if (script->functionNonDelazifying()->atom() == pn->pn_atom)
                return false;
            if (ssi.hasDynamicScopeObject()) {
                uint32_t slot;
                if (LookupAliasedName(bce, script, pn->pn_atom->asPropertyName(), &slot, pn)) {
                    JSOp op;
                    switch (pn->getOp()) {
                      case JSOP_GETNAME: op = JSOP_GETALIASEDVAR; break;
                      case JSOP_SETNAME: op = JSOP_SETALIASEDVAR; break;
                      default: return false;
                    }

                    pn->setOp(op);
                    JS_ALWAYS_TRUE(pn->pn_cookie.set(bce->parser->tokenStream, hops, slot));
                    return true;
                }
                hops++;
            }

            
            
            
            
            
            if (script->funHasExtensibleScope() || script->directlyInsideEval())
                return false;
        }
    }

    
    
    if (bce->insideNonGlobalEval)
        return false;

    
    
    if (bce->script->hasPollutedGlobalScope())
        return false;

    
    if (pn->isDeoptimized())
        return false;

    if (bce->sc->isFunctionBox()) {
        
        
        
        FunctionBox *funbox = bce->sc->asFunctionBox();
        if (funbox->mightAliasLocals())
            return false;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (bce->insideEval && bce->sc->strict())
        return false;

    JSOp op;
    switch (pn->getOp()) {
      case JSOP_GETNAME:  op = JSOP_GETGNAME; break;
      case JSOP_SETNAME:  op = StrictifySetNameOp(JSOP_SETGNAME, bce); break;
      case JSOP_SETCONST:
        
        return false;
      default: MOZ_CRASH("gname");
    }
    pn->setOp(op);
    return true;
}


















static bool
BindNameToSlotHelper(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    MOZ_ASSERT(pn->isKind(PNK_NAME));

    
    if (pn->isBound() || pn->isDeoptimized())
        return true;

    
    JSOp op = pn->getOp();
    MOZ_ASSERT(op != JSOP_CALLEE);
    MOZ_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);

    



    Definition *dn;
    if (pn->isUsed()) {
        MOZ_ASSERT(pn->pn_cookie.isFree());
        dn = pn->pn_lexdef;
        MOZ_ASSERT(dn->isDefn());
        pn->pn_dflags |= (dn->pn_dflags & PND_CONST);
    } else if (pn->isDefn()) {
        dn = (Definition *) pn;
    } else {
        return true;
    }

    
    switch (op) {
      case JSOP_GETNAME:
      case JSOP_SETCONST:
        break;
      default:
        if (pn->isConst()) {
            JSAutoByteString name;
            if (!AtomToPrintableString(cx, pn->pn_atom, &name))
                return false;
            bce->reportError(pn, JSMSG_BAD_CONST_ASSIGN, name.ptr());
            return false;
        }
    }

    if (dn->pn_cookie.isFree()) {
        if (HandleScript caller = bce->evalCaller) {
            MOZ_ASSERT(bce->script->compileAndGo());

            



            if (bce->emittingForInit)
                return true;

            



            if (!caller->functionOrCallerFunction() && TryConvertFreeName(bce, pn)) {
                pn->pn_dflags |= PND_BOUND;
                return true;
            }

            



            return true;
        }

        
        if (!TryConvertFreeName(bce, pn))
            return true;

        pn->pn_dflags |= PND_BOUND;
        return true;
    }

    













    MOZ_ASSERT(!pn->isDefn());
    MOZ_ASSERT(pn->isUsed());
    MOZ_ASSERT(pn->pn_lexdef);
    MOZ_ASSERT(pn->pn_cookie.isFree());

    




    switch (dn->kind()) {
      case Definition::ARG:
        switch (op) {
          case JSOP_GETNAME:
            op = JSOP_GETARG; break;
          case JSOP_SETNAME:
          case JSOP_STRICTSETNAME:
            op = JSOP_SETARG; break;
          default: MOZ_CRASH("arg");
        }
        MOZ_ASSERT(!pn->isConst());
        break;

      case Definition::VAR:
      case Definition::GLOBALCONST:
      case Definition::CONST:
      case Definition::LET:
        switch (op) {
          case JSOP_GETNAME:
            op = JSOP_GETLOCAL; break;
          case JSOP_SETNAME:
          case JSOP_STRICTSETNAME:
            op = JSOP_SETLOCAL; break;
          case JSOP_SETCONST:
            op = JSOP_SETLOCAL; break;
          default: MOZ_CRASH("local");
        }
        break;

      case Definition::NAMED_LAMBDA: {
        MOZ_ASSERT(dn->isOp(JSOP_CALLEE));
        MOZ_ASSERT(op != JSOP_CALLEE);

        



        if (dn->pn_cookie.level() != bce->script->staticLevel())
            return true;

        DebugOnly<JSFunction *> fun = bce->sc->asFunctionBox()->function();
        MOZ_ASSERT(fun->isLambda());
        MOZ_ASSERT(pn->pn_atom == fun->atom());

        























        if (!bce->sc->asFunctionBox()->isHeavyweight()) {
            op = JSOP_CALLEE;
            pn->pn_dflags |= PND_CONST;
        }

        pn->setOp(op);
        pn->pn_dflags |= PND_BOUND;
        return true;
      }

      case Definition::PLACEHOLDER:
        return true;

      case Definition::MISSING:
        MOZ_CRASH("missing");
    }

    




    unsigned skip = bce->script->staticLevel() - dn->pn_cookie.level();
    MOZ_ASSERT_IF(skip, dn->isClosed());

    








    if (skip) {
        BytecodeEmitter *bceSkipped = bce;
        for (unsigned i = 0; i < skip; i++)
            bceSkipped = bceSkipped->parent;
        if (!bceSkipped->sc->isFunctionBox())
            return true;
    }

    MOZ_ASSERT(!pn->isOp(op));
    pn->setOp(op);
    if (!pn->pn_cookie.set(bce->parser->tokenStream, skip, dn->pn_cookie.slot()))
        return false;

    pn->pn_dflags |= PND_BOUND;
    return true;
}






bool
BytecodeEmitter::bindNameToSlot(ParseNode *pn)
{
    if (!BindNameToSlotHelper(cx, this, pn))
        return false;

    StrictifySetNameNode(pn, this);

    if (emitterMode == BytecodeEmitter::SelfHosting && !pn->isBound()) {
        reportError(pn, JSMSG_SELFHOSTED_UNBOUND_NAME);
        return false;
    }

    return true;
}













static bool
CheckSideEffects(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pn, bool *answer)
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
      case PN_BINARY_OBJ:
        if (pn->isAssignment()) {
            








            ParseNode *pn2 = pn->pn_left;
            if (!pn2->isKind(PNK_NAME)) {
                *answer = true;
            } else {
                if (!bce->bindNameToSlot(pn2))
                    return false;
                if (!CheckSideEffects(cx, bce, pn->pn_right, answer))
                    return false;
                if (!*answer && (!pn->isOp(JSOP_NOP) || !pn2->isConst()))
                    *answer = true;
            }
            return true;
        }

        MOZ_ASSERT(!pn->isOp(JSOP_OR), "|| produces a list now");
        MOZ_ASSERT(!pn->isOp(JSOP_AND), "&& produces a list now");
        MOZ_ASSERT(!pn->isOp(JSOP_STRICTEQ), "=== produces a list now");
        MOZ_ASSERT(!pn->isOp(JSOP_STRICTNE), "!== produces a list now");

        



        *answer = true;
        return true;

      case PN_UNARY:
        switch (pn->getKind()) {
          case PNK_DELETE:
          {
            ParseNode *pn2 = pn->pn_kid;
            switch (pn2->getKind()) {
              case PNK_NAME:
                if (!bce->bindNameToSlot(pn2))
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
            MOZ_CRASH("We have a returning default case");
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
        MOZ_CRASH("We have a returning default case");

      case PN_NAME:
        




        if (pn->isKind(PNK_NAME) && !pn->isOp(JSOP_NOP)) {
            if (!bce->bindNameToSlot(pn))
                return false;
            if (!pn->isOp(JSOP_CALLEE) && pn->pn_cookie.isFree()) {
                




                *answer = true;
            }
        }

        if (pn->isHoistedLexicalUse()) {
            
            *answer = true;
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
    if (!script->compileAndGo() || sc->isFunctionBox() || isInLoop())
        return false;
    hasSingletons = true;
    return true;
}

bool
BytecodeEmitter::needsImplicitThis()
{
    if (sc->isFunctionBox() && sc->asFunctionBox()->inWith)
        return true;

    for (StmtInfoBCE *stmt = topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == STMT_WITH)
            return true;
    }
    return false;
}

void
BytecodeEmitter::tellDebuggerAboutCompiledScript(ExclusiveContext *cx)
{
    
    
    if (!cx->isJSContext())
        return;

    
    
    if (emitterMode != LazyFunction && !parent) {
        Debugger::onNewScript(cx->asJSContext(), script);
    }
}

inline TokenStream *
BytecodeEmitter::tokenStream()
{
    return &parser->tokenStream;
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
    bool result = tokenStream()->reportStrictModeErrorNumberVA(pos.begin, sc->strict(),
                                                               errorNumber, args);
    va_end(args);
    return result;
}

bool
BytecodeEmitter::emitNewInit(JSProtoKey key)
{
    const size_t len = 1 + UINT32_INDEX_LEN;
    ptrdiff_t offset = emitCheck(len);
    if (offset < 0)
        return false;

    jsbytecode *code = this->code(offset);
    code[0] = JSOP_NEWINIT;
    code[1] = jsbytecode(key);
    code[2] = 0;
    code[3] = 0;
    code[4] = 0;
    updateDepth(offset);
    checkTypeSet(JSOP_NEWINIT);
    return true;
}

static bool
IteratorResultShape(ExclusiveContext *cx, BytecodeEmitter *bce, unsigned *shape)
{
    RootedPlainObject obj(cx);
    
    
    gc::AllocKind kind = gc::GetGCObjectKind(2);
    obj = NewBuiltinClassInstance<PlainObject>(cx, kind);
    if (!obj)
        return false;

    Rooted<jsid> value_id(cx, AtomToId(cx->names().value));
    Rooted<jsid> done_id(cx, AtomToId(cx->names().done));
    if (!NativeDefineProperty(cx, obj, value_id, UndefinedHandleValue, nullptr, nullptr,
                              JSPROP_ENUMERATE))
    {
        return false;
    }
    if (!NativeDefineProperty(cx, obj, done_id, UndefinedHandleValue, nullptr, nullptr,
                              JSPROP_ENUMERATE))
    {
        return false;
    }

    ObjectBox *objbox = bce->parser->newObjectBox(obj);
    if (!objbox)
        return false;

    *shape = bce->objectList.add(objbox);

    return true;
}

bool
BytecodeEmitter::emitPrepareIteratorResult()
{
    unsigned shape;
    if (!IteratorResultShape(cx, this, &shape))
        return false;
    return emitIndex32(JSOP_NEWOBJECT, shape);
}

bool
BytecodeEmitter::emitFinishIteratorResult(bool done)
{
    jsatomid value_id;
    if (!makeAtomIndex(cx->names().value, &value_id))
        return false;
    jsatomid done_id;
    if (!makeAtomIndex(cx->names().done, &done_id))
        return false;

    if (!emitIndex32(JSOP_INITPROP, value_id))
        return false;
    if (!emit1(done ? JSOP_TRUE : JSOP_FALSE))
        return false;
    if (!emitIndex32(JSOP_INITPROP, done_id))
        return false;
    return true;
}

bool
BytecodeEmitter::emitNameOp(ParseNode *pn, bool callContext)
{
    if (!bindNameToSlot(pn))
        return false;

    JSOp op = pn->getOp();

    if (op == JSOP_CALLEE) {
        if (!emit1(op))
            return false;
    } else {
        if (!pn->pn_cookie.isFree()) {
            MOZ_ASSERT(JOF_OPTYPE(op) != JOF_ATOM);
            if (!emitVarOp(pn, op))
                return false;
        } else {
            if (!emitAtomOp(pn, op))
                return false;
        }
    }

    
    if (callContext) {
        if (op == JSOP_GETNAME || op == JSOP_GETGNAME) {
            JSOp thisOp = needsImplicitThis() ? JSOP_IMPLICITTHIS : JSOP_GIMPLICITTHIS;
            if (!emitAtomOp(pn, thisOp))
                return false;
        } else {
            if (!emit1(JSOP_UNDEFINED))
                return false;
        }
    }

    return true;
}

bool
BytecodeEmitter::emitPropLHS(ParseNode *pn, JSOp op)
{
    MOZ_ASSERT(pn->isKind(PNK_DOT));
    ParseNode *pn2 = pn->maybeExpr();

    




    if (pn2->isKind(PNK_DOT)) {
        ParseNode *pndot = pn2;
        ParseNode *pnup = nullptr, *pndown;
        ptrdiff_t top = offset();
        for (;;) {
            
            pndot->pn_offset = top;
            MOZ_ASSERT(!pndot->isUsed());
            pndown = pndot->pn_expr;
            pndot->pn_expr = pnup;
            if (!pndown->isKind(PNK_DOT))
                break;
            pnup = pndot;
            pndot = pndown;
        }

        
        if (!emitTree(pndown))
            return false;

        do {
            
            if (!emitAtomOp(pndot, JSOP_GETPROP))
                return false;

            
            pnup = pndot->pn_expr;
            pndot->pn_expr = pndown;
            pndown = pndot;
        } while ((pndot = pnup) != nullptr);
        return true;
    }

    
    return emitTree(pn2);
}

bool
BytecodeEmitter::emitPropOp(ParseNode *pn, JSOp op)
{
    MOZ_ASSERT(pn->isArity(PN_NAME));

    if (!emitPropLHS(pn, op))
        return false;

    if (op == JSOP_CALLPROP && !emit1(JSOP_DUP))
        return false;

    if (!emitAtomOp(pn, op))
        return false;

    if (op == JSOP_CALLPROP && !emit1(JSOP_SWAP))
        return false;

    return true;
}

bool
BytecodeEmitter::emitPropIncDec(ParseNode *pn)
{
    MOZ_ASSERT(pn->pn_kid->getKind() == PNK_DOT);

    bool post;
    JSOp binop = GetIncDecInfo(pn->getKind(), &post);

    JSOp get = JSOP_GETPROP;
    if (!emitPropLHS(pn->pn_kid, get))              
        return false;
    if (!emit1(JSOP_DUP))                           
        return false;
    if (!emitAtomOp(pn->pn_kid, JSOP_GETPROP))      
        return false;
    if (!emit1(JSOP_POS))                           
        return false;
    if (post && !emit1(JSOP_DUP))                   
        return false;
    if (!emit1(JSOP_ONE))                           
        return false;
    if (!emit1(binop))                              
        return false;

    if (post) {
        if (!emit2(JSOP_PICK, (jsbytecode)2))       
            return false;
        if (!emit1(JSOP_SWAP))                      
            return false;
    }

    JSOp setOp = sc->strict() ? JSOP_STRICTSETPROP : JSOP_SETPROP;
    if (!emitAtomOp(pn->pn_kid, setOp))             
        return false;
    if (post && !emit1(JSOP_POP))                   
        return false;

    return true;
}

bool
BytecodeEmitter::emitNameIncDec(ParseNode *pn)
{
    const JSCodeSpec *cs = &js_CodeSpec[pn->pn_kid->getOp()];

    bool global = (cs->format & JOF_GNAME);
    bool post;
    JSOp binop = GetIncDecInfo(pn->getKind(), &post);

    if (!emitAtomOp(pn->pn_kid, global ? JSOP_BINDGNAME : JSOP_BINDNAME))  
        return false;
    if (!emitAtomOp(pn->pn_kid, global ? JSOP_GETGNAME : JSOP_GETNAME))    
        return false;
    if (!emit1(JSOP_POS))                      
        return false;
    if (post && !emit1(JSOP_DUP))              
        return false;
    if (!emit1(JSOP_ONE))                      
        return false;
    if (!emit1(binop))                         
        return false;

    if (post) {
        if (!emit2(JSOP_PICK, (jsbytecode)2))  
            return false;
        if (!emit1(JSOP_SWAP))                 
            return false;
    }

    JSOp setOp = StrictifySetNameOp(global ? JSOP_SETGNAME : JSOP_SETNAME, this);
    if (!emitAtomOp(pn->pn_kid, setOp))        
        return false;
    if (post && !emit1(JSOP_POP))              
        return false;

    return true;
}

bool
BytecodeEmitter::emitElemOperands(ParseNode *pn, JSOp op)
{
    MOZ_ASSERT(pn->isArity(PN_BINARY));
    if (!emitTree(pn->pn_left))
        return false;
    if (op == JSOP_CALLELEM && !emit1(JSOP_DUP))
        return false;
    if (!emitTree(pn->pn_right))
        return false;
    bool isSetElem = op == JSOP_SETELEM || op == JSOP_STRICTSETELEM;
    if (isSetElem && !emit2(JSOP_PICK, (jsbytecode)2))
        return false;
    return true;
}

bool
BytecodeEmitter::emitElemOpBase(JSOp op)
{
    if (!emit1(op))
        return false;

    checkTypeSet(op);
    return true;
}

bool
BytecodeEmitter::emitElemOp(ParseNode *pn, JSOp op)
{
    return emitElemOperands(pn, op) && emitElemOpBase(op);
}

bool
BytecodeEmitter::emitElemIncDec(ParseNode *pn)
{
    MOZ_ASSERT(pn->pn_kid->getKind() == PNK_ELEM);

    if (!emitElemOperands(pn->pn_kid, JSOP_GETELEM))
        return false;

    bool post;
    JSOp binop = GetIncDecInfo(pn->getKind(), &post);

    



                                                    
    if (!emit1(JSOP_TOID))                          
        return false;
    if (!emit1(JSOP_DUP2))                          
        return false;
    if (!emitElemOpBase(JSOP_GETELEM))              
        return false;
    if (!emit1(JSOP_POS))                           
        return false;
    if (post && !emit1(JSOP_DUP))                   
        return false;
    if (!emit1(JSOP_ONE))                           
        return false;
    if (!emit1(binop))                              
        return false;

    if (post) {
        if (!emit2(JSOP_PICK, (jsbytecode)3))       
            return false;
        if (!emit2(JSOP_PICK, (jsbytecode)3))       
            return false;
        if (!emit2(JSOP_PICK, (jsbytecode)2))       
            return false;
    }

    JSOp setOp = sc->strict() ? JSOP_STRICTSETELEM : JSOP_SETELEM;
    if (!emitElemOpBase(setOp))                     
        return false;
    if (post && !emit1(JSOP_POP))                   
        return false;

    return true;
}

bool
BytecodeEmitter::emitNumberOp(double dval)
{
    int32_t ival;
    uint32_t u;
    ptrdiff_t off;
    jsbytecode *pc;

    if (NumberIsInt32(dval, &ival)) {
        if (ival == 0)
            return emit1(JSOP_ZERO);
        if (ival == 1)
            return emit1(JSOP_ONE);
        if ((int)(int8_t)ival == ival)
            return emit2(JSOP_INT8, (jsbytecode)(int8_t)ival);

        u = (uint32_t)ival;
        if (u < JS_BIT(16)) {
            emitUint16Operand(JSOP_UINT16, u);
        } else if (u < JS_BIT(24)) {
            off = emitN(JSOP_UINT24, 3);
            if (off < 0)
                return false;
            pc = code(off);
            SET_UINT24(pc, u);
        } else {
            off = emitN(JSOP_INT32, 4);
            if (off < 0)
                return false;
            pc = code(off);
            SET_INT32(pc, ival);
        }
        return true;
    }

    if (!constList.append(DoubleValue(dval)))
        return false;

    return emitIndex32(JSOP_DOUBLE, constList.length() - 1);
}

void
BytecodeEmitter::setJumpOffsetAt(ptrdiff_t off)
{
    SET_JUMP_OFFSET(code(off), offset() - off);
}

static bool
PushInitialConstants(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp op, unsigned n)
{
    MOZ_ASSERT(op == JSOP_UNDEFINED || op == JSOP_UNINITIALIZED);
    for (unsigned i = 0; i < n; ++i) {
        if (!bce->emit1(op))
            return false;
    }
    return true;
}

static bool
InitializeBlockScopedLocalsFromStack(ExclusiveContext *cx, BytecodeEmitter *bce,
                                     Handle<StaticBlockObject *> blockObj)
{
    for (unsigned i = blockObj->numVariables(); i > 0; --i) {
        if (blockObj->isAliased(i - 1)) {
            ScopeCoordinate sc;
            sc.setHops(0);
            sc.setSlot(BlockObject::RESERVED_SLOTS + i - 1);
            if (!bce->emitAliasedVarOp(JSOP_INITALIASEDLEXICAL, sc, DontCheckLexical))
                return false;
        } else {
            
            
            
            
            uint32_t numAliased = bce->script->bindings.numAliasedBodyLevelLocals();
            unsigned local = blockObj->blockIndexToLocalIndex(i - 1) + numAliased;
            if (!bce->emitUnaliasedVarOp(JSOP_INITLEXICAL, local, DontCheckLexical))
                return false;
        }
        if (!bce->emit1(JSOP_POP))
            return false;
    }
    return true;
}

static bool
EnterBlockScope(ExclusiveContext *cx, BytecodeEmitter *bce, StmtInfoBCE *stmtInfo,
                ObjectBox *objbox, JSOp initialValueOp, unsigned alreadyPushed = 0)
{
    
    
    
    
    Rooted<StaticBlockObject *> blockObj(cx, &objbox->object->as<StaticBlockObject>());
    if (!PushInitialConstants(cx, bce, initialValueOp, blockObj->numVariables() - alreadyPushed))
        return false;

    if (!EnterNestedScope(cx, bce, stmtInfo, objbox, STMT_BLOCK))
        return false;

    if (!InitializeBlockScopedLocalsFromStack(cx, bce, blockObj))
        return false;

    return true;
}






MOZ_NEVER_INLINE bool
BytecodeEmitter::emitSwitch(ParseNode *pn)
{
    JSOp switchOp;
    bool hasDefault;
    ptrdiff_t top, off, defaultOffset;
    ParseNode *pn2, *pn3, *pn4;
    int32_t low, high;
    int noteIndex;
    size_t switchSize;
    jsbytecode *pc;

    
    switchOp = JSOP_TABLESWITCH;
    hasDefault = false;
    defaultOffset = -1;

    pn2 = pn->pn_right;
    MOZ_ASSERT(pn2->isKind(PNK_LEXICALSCOPE) || pn2->isKind(PNK_STATEMENTLIST));

    
    if (!emitTree(pn->pn_left))
        return false;

    StmtInfoBCE stmtInfo(cx);
    if (pn2->isKind(PNK_LEXICALSCOPE)) {
        if (!EnterBlockScope(cx, this, &stmtInfo, pn2->pn_objbox, JSOP_UNINITIALIZED, 0))
            return false;

        stmtInfo.type = STMT_SWITCH;
        stmtInfo.update = top = offset();
        
        pn2 = pn2->expr();
    } else {
        MOZ_ASSERT(pn2->isKind(PNK_STATEMENTLIST));
        top = offset();
        pushStatement(&stmtInfo, STMT_SWITCH, top);
    }

    
    uint32_t caseCount = pn2->pn_count;
    uint32_t tableLength = 0;
    UniquePtr<ParseNode*[], JS::FreePolicy> table(nullptr);

    if (caseCount > JS_BIT(16)) {
        parser->tokenStream.reportError(JSMSG_TOO_MANY_CASES);
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
        jsbitmap *intmap = nullptr;
        int32_t intmap_bitlen = 0;

        low  = JSVAL_INT_MAX;
        high = JSVAL_INT_MIN;

        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            if (pn3->isKind(PNK_DEFAULT)) {
                hasDefault = true;
                caseCount--;    
                continue;
            }

            MOZ_ASSERT(pn3->isKind(PNK_CASE));
            if (switchOp == JSOP_CONDSWITCH)
                continue;

            MOZ_ASSERT(switchOp == JSOP_TABLESWITCH);

            pn4 = pn3->pn_left;

            if (pn4->getKind() != PNK_NUMBER) {
                switchOp = JSOP_CONDSWITCH;
                continue;
            }

            int32_t i;
            if (!NumberIsInt32(pn4->pn_dval, &i)) {
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
                    size_t(i) < (INTMAP_LENGTH * JS_BITMAP_NBITS)) {
                    intmap = intmap_space;
                    intmap_bitlen = INTMAP_LENGTH * JS_BITMAP_NBITS;
                } else {
                    
                    intmap_bitlen = JS_BIT(16);
                    intmap = cx->pod_malloc<jsbitmap>(JS_BIT(16) / JS_BITMAP_NBITS);
                    if (!intmap) {
                        ReportOutOfMemory(cx);
                        return false;
                    }
                }
                memset(intmap, 0, size_t(intmap_bitlen) / CHAR_BIT);
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
        noteIndex = NewSrcNote3(cx, this, SRC_CONDSWITCH, 0, 0);
    } else {
        MOZ_ASSERT(switchOp == JSOP_TABLESWITCH);

        
        switchSize = (size_t)(JUMP_OFFSET_LEN * (3 + tableLength));
        noteIndex = NewSrcNote2(cx, this, SRC_TABLESWITCH, 0);
    }
    if (noteIndex < 0)
        return false;

    
    if (emitN(switchOp, switchSize) < 0)
        return false;

    off = -1;
    if (switchOp == JSOP_CONDSWITCH) {
        int caseNoteIndex = -1;
        bool beforeCases = true;

        
        for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
            pn4 = pn3->pn_left;
            if (pn4 && !emitTree(pn4))
                return false;
            if (caseNoteIndex >= 0) {
                
                if (!setSrcNoteOffset(unsigned(caseNoteIndex), 0, offset() - off))
                    return false;
            }
            if (!pn4) {
                MOZ_ASSERT(pn3->isKind(PNK_DEFAULT));
                continue;
            }
            caseNoteIndex = NewSrcNote2(cx, this, SRC_NEXTCASE, 0);
            if (caseNoteIndex < 0)
                return false;
            off = emitJump(JSOP_CASE, 0);
            if (off < 0)
                return false;
            pn3->pn_offset = off;
            if (beforeCases) {
                unsigned noteCount, noteCountDelta;

                
                noteCount = notes().length();
                if (!setSrcNoteOffset(unsigned(noteIndex), 1, off - top))
                    return false;
                noteCountDelta = notes().length() - noteCount;
                if (noteCountDelta != 0)
                    caseNoteIndex += noteCountDelta;
                beforeCases = false;
            }
        }

        





        if (!hasDefault &&
            caseNoteIndex >= 0 &&
            !setSrcNoteOffset(unsigned(caseNoteIndex), 0, offset() - off))
        {
            return false;
        }

        
        defaultOffset = emitJump(JSOP_DEFAULT, 0);
        if (defaultOffset < 0)
            return false;
    } else {
        MOZ_ASSERT(switchOp == JSOP_TABLESWITCH);
        pc = code(top + JUMP_OFFSET_LEN);

        
        SET_JUMP_OFFSET(pc, low);
        pc += JUMP_OFFSET_LEN;
        SET_JUMP_OFFSET(pc, high);
        pc += JUMP_OFFSET_LEN;

        



        if (tableLength != 0) {
            table = cx->make_zeroed_pod_array<ParseNode*>(tableLength);
            if (!table)
                return false;
            for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
                if (pn3->isKind(PNK_DEFAULT))
                    continue;

                MOZ_ASSERT(pn3->isKind(PNK_CASE));

                pn4 = pn3->pn_left;
                MOZ_ASSERT(pn4->getKind() == PNK_NUMBER);

                int32_t i = int32_t(pn4->pn_dval);
                MOZ_ASSERT(double(i) == pn4->pn_dval);

                i -= low;
                MOZ_ASSERT(uint32_t(i) < tableLength);
                table[i] = pn3;
            }
        }
    }

    
    for (pn3 = pn2->pn_head; pn3; pn3 = pn3->pn_next) {
        if (switchOp == JSOP_CONDSWITCH && !pn3->isKind(PNK_DEFAULT))
            setJumpOffsetAt(pn3->pn_offset);
        pn4 = pn3->pn_right;
        if (!emitTree(pn4))
            return false;
        pn3->pn_offset = pn4->pn_offset;
        if (pn3->isKind(PNK_DEFAULT))
            off = pn3->pn_offset - top;
    }

    if (!hasDefault) {
        
        off = offset() - top;
    }

    
    MOZ_ASSERT(off != -1);

    
    if (switchOp == JSOP_CONDSWITCH) {
        pc = nullptr;
        MOZ_ASSERT(defaultOffset != -1);
        SET_JUMP_OFFSET(code(defaultOffset), off - (defaultOffset - top));
    } else {
        pc = code(top);
        SET_JUMP_OFFSET(pc, off);
        pc += JUMP_OFFSET_LEN;
    }

    
    off = offset() - top;
    if (!setSrcNoteOffset(unsigned(noteIndex), 0, off))
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

    if (pn->pn_right->isKind(PNK_LEXICALSCOPE)) {
        if (!LeaveNestedScope(cx, this, &stmtInfo))
            return false;
    } else {
        popStatement();
    }

    return true;
}

bool
BytecodeEmitter::isRunOnceLambda()
{
    
    
    

    if (!(parent && parent->emittingRunOnceLambda) &&
        (emitterMode != LazyFunction || !lazyScript->treatAsRunOnce()))
    {
        return false;
    }

    FunctionBox *funbox = sc->asFunctionBox();
    return !funbox->argumentsHasLocalBinding() &&
           !funbox->isGenerator() &&
           !funbox->function()->name();
}

bool
BytecodeEmitter::emitYieldOp(JSOp op)
{
    if (op == JSOP_FINALYIELDRVAL)
        return emit1(JSOP_FINALYIELDRVAL);

    MOZ_ASSERT(op == JSOP_INITIALYIELD || op == JSOP_YIELD);

    ptrdiff_t off = emitN(op, 3);
    if (off < 0)
        return false;

    uint32_t yieldIndex = yieldOffsetList.length();
    if (yieldIndex >= JS_BIT(24)) {
        reportError(nullptr, JSMSG_TOO_MANY_YIELDS);
        return false;
    }

    SET_UINT24(code(off), yieldIndex);

    if (!yieldOffsetList.append(offset()))
        return false;

    return emit1(JSOP_DEBUGAFTERYIELD);
}

bool
frontend::EmitFunctionScript(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *body)
{
    if (!bce->updateLocalsToFrameSlots())
        return false;

    






    FunctionBox *funbox = bce->sc->asFunctionBox();
    if (funbox->argumentsHasLocalBinding()) {
        MOZ_ASSERT(bce->offset() == 0);  
        bce->switchToProlog();
        if (!bce->emit1(JSOP_ARGUMENTS))
            return false;
        InternalBindingsHandle bindings(bce->script, &bce->script->bindings);
        BindingIter bi = Bindings::argumentsBinding(cx, bindings);
        if (bce->script->bindingIsAliased(bi)) {
            ScopeCoordinate sc;
            sc.setHops(0);
            sc.setSlot(0);  
            JS_ALWAYS_TRUE(LookupAliasedNameSlot(bce, bce->script, cx->names().arguments, &sc));
            if (!bce->emitAliasedVarOp(JSOP_SETALIASEDVAR, sc, DontCheckLexical))
                return false;
        } else {
            if (!bce->emitUnaliasedVarOp(JSOP_SETLOCAL, bi.localIndex(), DontCheckLexical))
                return false;
        }
        if (!bce->emit1(JSOP_POP))
            return false;
        bce->switchToMain();
    }

    




    bool runOnce = bce->isRunOnceLambda();
    if (runOnce) {
        bce->switchToProlog();
        if (!bce->emit1(JSOP_RUNONCE))
            return false;
        bce->switchToMain();
    }

    if (!bce->emitTree(body))
        return false;

    if (bce->sc->isFunctionBox()) {
        if (bce->sc->asFunctionBox()->isGenerator()) {
            
            if (bce->sc->asFunctionBox()->isStarGenerator() && !bce->emitPrepareIteratorResult())
                return false;

            if (!bce->emit1(JSOP_UNDEFINED))
                return false;

            if (bce->sc->asFunctionBox()->isStarGenerator() &&
                !bce->emitFinishIteratorResult(true))
            {
                return false;
            }

            if (!bce->emit1(JSOP_SETRVAL))
                return false;

            ScopeCoordinate sc;
            
            
            sc.setHops(0);
            MOZ_ALWAYS_TRUE(LookupAliasedNameSlot(bce, bce->script, cx->names().dotGenerator, &sc));
            if (!bce->emitAliasedVarOp(JSOP_GETALIASEDVAR, sc, DontCheckLexical))
                return false;

            
            if (!bce->emitYieldOp(JSOP_FINALYIELDRVAL))
                return false;
        } else {
            
            
            
            
            if (bce->hasTryFinally) {
                if (!bce->emit1(JSOP_UNDEFINED))
                    return false;
                if (!bce->emit1(JSOP_RETURN))
                    return false;
            }
        }
    }

    
    
    if (!bce->emit1(JSOP_RETRVAL))
        return false;

    
    
    
    
    if (bce->sc->allLocalsAliased())
        bce->script->bindings.setAllLocalsAliased();

    if (!JSScript::fullyInitFromEmitter(cx, bce->script, bce))
        return false;

    



    if (runOnce) {
        bce->script->setTreatAsRunOnce();
        MOZ_ASSERT(!bce->script->hasRunOnce());
    }

    
    RootedFunction fun(cx, bce->script->functionNonDelazifying());
    MOZ_ASSERT(fun->isInterpreted());

    if (fun->isInterpretedLazy())
        fun->setUnlazifiedScript(bce->script);
    else
        fun->setScript(bce->script);

    bce->tellDebuggerAboutCompiledScript(cx);

    return true;
}

bool
BytecodeEmitter::maybeEmitVarDecl(JSOp prologOp, ParseNode *pn, jsatomid *result)
{
    jsatomid atomIndex;

    if (!pn->pn_cookie.isFree()) {
        atomIndex = pn->pn_cookie.slot();
    } else {
        if (!makeAtomIndex(pn->pn_atom, &atomIndex))
            return false;
    }

    if (JOF_OPTYPE(pn->getOp()) == JOF_ATOM &&
        (!sc->isFunctionBox() || sc->asFunctionBox()->isHeavyweight()))
    {
        switchToProlog();
        if (!updateSourceCoordNotes(pn->pn_pos.begin))
            return false;
        if (!emitIndexOp(prologOp, atomIndex))
            return false;
        switchToMain();
    }

    if (result)
        *result = atomIndex;
    return true;
}

typedef bool
(*DestructuringDeclEmitter)(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn);

template <DestructuringDeclEmitter EmitName>
static bool
EmitDestructuringDeclsWithEmitter(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp prologOp,
                                  ParseNode *pattern)
{
    if (pattern->isKind(PNK_ARRAY)) {
        for (ParseNode *element = pattern->pn_head; element; element = element->pn_next) {
            if (element->isKind(PNK_ELISION))
                continue;
            ParseNode *target = element;
            if (element->isKind(PNK_SPREAD)) {
                MOZ_ASSERT(element->pn_kid->isKind(PNK_NAME));
                target = element->pn_kid;
            }
            if (target->isKind(PNK_ASSIGN))
                target = target->pn_left;
            if (target->isKind(PNK_NAME)) {
                if (!EmitName(cx, bce, prologOp, target))
                    return false;
            } else {
                if (!EmitDestructuringDeclsWithEmitter<EmitName>(cx, bce, prologOp, target))
                    return false;
            }
        }
        return true;
    }

    MOZ_ASSERT(pattern->isKind(PNK_OBJECT));
    for (ParseNode *member = pattern->pn_head; member; member = member->pn_next) {
        MOZ_ASSERT(member->isKind(PNK_MUTATEPROTO) ||
                   member->isKind(PNK_COLON) ||
                   member->isKind(PNK_SHORTHAND));

        ParseNode *target = member->isKind(PNK_MUTATEPROTO) ? member->pn_kid : member->pn_right;

        if (target->isKind(PNK_ASSIGN))
            target = target->pn_left;
        if (target->isKind(PNK_NAME)) {
            if (!EmitName(cx, bce, prologOp, target))
                return false;
        } else {
            if (!EmitDestructuringDeclsWithEmitter<EmitName>(cx, bce, prologOp, target))
                return false;
        }
    }
    return true;
}

bool
EmitDestructuringDecl(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp prologOp, ParseNode *pn)
{
    MOZ_ASSERT(pn->isKind(PNK_NAME));
    if (!bce->bindNameToSlot(pn))
        return false;

    MOZ_ASSERT(!pn->isOp(JSOP_CALLEE));
    return bce->maybeEmitVarDecl(prologOp, pn, nullptr);
}

static inline bool
EmitDestructuringDecls(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp prologOp,
                       ParseNode *pattern)
{
    return EmitDestructuringDeclsWithEmitter<EmitDestructuringDecl>(cx, bce, prologOp, pattern);
}

bool
EmitInitializeDestructuringDecl(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp prologOp,
                                ParseNode *pn)
{
    MOZ_ASSERT(pn->isKind(PNK_NAME));
    MOZ_ASSERT(pn->isBound());
    return bce->emitVarOp(pn, pn->getOp());
}



static inline bool
EmitInitializeDestructuringDecls(ExclusiveContext *cx, BytecodeEmitter *bce, JSOp prologOp,
                                 ParseNode *pattern)
{
    return EmitDestructuringDeclsWithEmitter<EmitInitializeDestructuringDecl>(cx, bce,
                                                                              prologOp, pattern);
}

static bool
EmitDestructuringOpsHelper(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pattern,
                           VarEmitOption emitOption);

bool
BytecodeEmitter::emitDestructuringLHS(ParseNode *target, VarEmitOption emitOption)
{
    MOZ_ASSERT(emitOption != DefineVars);

    
    
    
    
    if (target->isKind(PNK_SPREAD))
        target = target->pn_kid;
    else if (target->isKind(PNK_ASSIGN))
        target = target->pn_left;
    if (target->isKind(PNK_ARRAY) || target->isKind(PNK_OBJECT)) {
        if (!EmitDestructuringOpsHelper(cx, this, target, emitOption))
            return false;
        if (emitOption == InitializeVars) {
            
            
            if (!emit1(JSOP_POP))
                return false;
        }
    } else if (emitOption == PushInitialValues) {
        
        
        MOZ_ASSERT(target->getOp() == JSOP_SETLOCAL || target->getOp() == JSOP_INITLEXICAL);
        MOZ_ASSERT(target->pn_dflags & PND_BOUND);
    } else {
        switch (target->getKind()) {
          case PNK_NAME:
            if (!bindNameToSlot(target))
                return false;

            switch (target->getOp()) {
              case JSOP_SETNAME:
              case JSOP_STRICTSETNAME:
              case JSOP_SETGNAME:
              case JSOP_STRICTSETGNAME:
              case JSOP_SETCONST: {
                
                
                
                
                
                
                
                
                
                
                jsatomid atomIndex;
                if (!makeAtomIndex(target->pn_atom, &atomIndex))
                    return false;

                if (!target->isOp(JSOP_SETCONST)) {
                    bool global = target->isOp(JSOP_SETGNAME) || target->isOp(JSOP_STRICTSETGNAME);
                    JSOp bindOp = global ? JSOP_BINDGNAME : JSOP_BINDNAME;
                    if (!emitIndex32(bindOp, atomIndex))
                        return false;
                    if (!emit1(JSOP_SWAP))
                        return false;
                }

                if (!emitIndexOp(target->getOp(), atomIndex))
                    return false;
                break;
              }

              case JSOP_SETLOCAL:
              case JSOP_SETARG:
              case JSOP_INITLEXICAL:
                if (!emitVarOp(target, target->getOp()))
                    return false;
                break;

              default:
                MOZ_CRASH("emitDestructuringLHS: bad name op");
            }
            break;

          case PNK_DOT:
          {
            
            
            
            
            
            
            
            
            if (!emitTree(target->pn_expr))
                return false;
            if (!emit1(JSOP_SWAP))
                return false;
            JSOp setOp = sc->strict() ? JSOP_STRICTSETPROP : JSOP_SETPROP;
            if (!emitAtomOp(target, setOp))
                return false;
            break;
          }

          case PNK_ELEM:
          {
            
            
            
            JSOp setOp = sc->strict() ? JSOP_STRICTSETELEM : JSOP_SETELEM;
            if (!emitElemOp(target, setOp))
                return false;
            break;
          }

          case PNK_CALL:
            MOZ_ASSERT(target->pn_xflags & PNX_SETCALL);
            if (!emitTree(target))
                return false;

            
            
            
            
            
            if (!emit1(JSOP_POP))
                return false;
            break;

          default:
            MOZ_CRASH("emitDestructuringLHS: bad lhs kind");
        }

        
        if (!emit1(JSOP_POP))
            return false;
    }

    return true;
}

bool
BytecodeEmitter::emitIteratorNext(ParseNode *pn)
{
    MOZ_ASSERT(emitterMode != BytecodeEmitter::SelfHosting,
               ".next() iteration is prohibited in self-hosted code because it "
               "can run user-modifiable iteration code");

    if (!emit1(JSOP_DUP))                                 
        return false;
    if (!emitAtomOp(cx->names().next, JSOP_CALLPROP))     
        return false;
    if (!emit1(JSOP_SWAP))                                
        return false;
    if (!emitCall(JSOP_CALL, 0, pn))                      
        return false;
    checkTypeSet(JSOP_CALL);
    return true;
}

bool
BytecodeEmitter::emitDefault(ParseNode *defaultExpr)
{
    if (!emit1(JSOP_DUP))                                 
        return false;
    if (!emit1(JSOP_UNDEFINED))                           
        return false;
    if (!emit1(JSOP_STRICTEQ))                            
        return false;
    
    if (NewSrcNote(cx, this, SRC_IF) < 0)
        return false;
    ptrdiff_t jump = emitJump(JSOP_IFEQ, 0);              
    if (jump < 0)
        return false;
    if (!emit1(JSOP_POP))                                 
        return false;
    if (!emitTree(defaultExpr))                           
        return false;
    setJumpOffsetAt(jump);
    return true;
}

static bool
EmitDestructuringOpsArrayHelper(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pattern,
                                VarEmitOption emitOption)
{
    MOZ_ASSERT(pattern->isKind(PNK_ARRAY));
    MOZ_ASSERT(pattern->isArity(PN_LIST));
    MOZ_ASSERT(bce->stackDepth != 0);

    



    if (emitOption == InitializeVars) {
        if (!bce->emit1(JSOP_DUP))                                     
            return false;
    }
    if (!bce->emitIterator())                                          
        return false;
    bool needToPopIterator = true;

    for (ParseNode *member = pattern->pn_head; member; member = member->pn_next) {
        




        ParseNode *pndefault = nullptr;
        ParseNode *elem = member;
        if (elem->isKind(PNK_ASSIGN)) {
            pndefault = elem->pn_right;
            elem = elem->pn_left;
        }

        if (elem->isKind(PNK_SPREAD)) {
            
            ptrdiff_t off = bce->emitN(JSOP_NEWARRAY, 3);              
            if (off < 0)
                return false;
            bce->checkTypeSet(JSOP_NEWARRAY);
            jsbytecode *pc = bce->code(off);
            SET_UINT24(pc, 0);

            if (!bce->emitNumberOp(0))                                 
                return false;
            if (!bce->emitSpread())                                    
                return false;
            if (!bce->emit1(JSOP_POP))                                 
                return false;
            needToPopIterator = false;
        } else {
            if (!bce->emit1(JSOP_DUP))                                 
                return false;
            if (!bce->emitIteratorNext(pattern))                       
                return false;
            if (!bce->emit1(JSOP_DUP))                                 
                return false;
            if (!bce->emitAtomOp(cx->names().done, JSOP_GETPROP))      
                return false;

            
            
            
            ptrdiff_t noteIndex = NewSrcNote(cx, bce, SRC_COND);
            if (noteIndex < 0)
                return false;
            ptrdiff_t beq = bce->emitJump(JSOP_IFEQ, 0);
            if (beq < 0)
                return false;

            if (!bce->emit1(JSOP_POP))                                 
                return false;
            if (!bce->emit1(JSOP_UNDEFINED))                           
                return false;

            
            ptrdiff_t jmp = bce->emitJump(JSOP_GOTO, 0);
            if (jmp < 0)
                return false;
            bce->setJumpOffsetAt(beq);

            if (!bce->emitAtomOp(cx->names().value, JSOP_GETPROP))     
                return false;

            bce->setJumpOffsetAt(jmp);
            if (!bce->setSrcNoteOffset(noteIndex, 0, jmp - beq))
                return false;
        }

        if (pndefault && !bce->emitDefault(pndefault))
            return false;

        
        ParseNode *subpattern = elem;
        if (subpattern->isKind(PNK_ELISION)) {
            
            if (!bce->emit1(JSOP_POP))                             
                return false;
            continue;
        }

        int32_t depthBefore = bce->stackDepth;
        if (!bce->emitDestructuringLHS(subpattern, emitOption))
            return false;

        if (emitOption == PushInitialValues && needToPopIterator) {
            









            MOZ_ASSERT((bce->stackDepth - bce->stackDepth) >= -1);
            uint32_t pickDistance = (uint32_t)((bce->stackDepth + 1) - depthBefore);
            if (pickDistance > 0) {
                if (pickDistance > UINT8_MAX) {
                    bce->reportError(subpattern, JSMSG_TOO_MANY_LOCALS);
                    return false;
                }
                if (!bce->emit2(JSOP_PICK, (jsbytecode)pickDistance))
                    return false;
            }
        }
    }

    if (needToPopIterator && !bce->emit1(JSOP_POP))
        return false;

    return true;
}

static bool
EmitDestructuringOpsObjectHelper(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pattern,
                                 VarEmitOption emitOption)
{
    MOZ_ASSERT(pattern->isKind(PNK_OBJECT));
    MOZ_ASSERT(pattern->isArity(PN_LIST));

    MOZ_ASSERT(bce->stackDepth != 0);                                  

    for (ParseNode *member = pattern->pn_head; member; member = member->pn_next) {
        
        if (!bce->emit1(JSOP_DUP))                                     
            return false;

        
        
        
        bool needsGetElem = true;

        ParseNode *subpattern;
        if (member->isKind(PNK_MUTATEPROTO)) {
            if (!bce->emitAtomOp(cx->names().proto, JSOP_GETPROP))     
                return false;
            needsGetElem = false;
            subpattern = member->pn_kid;
        } else {
            MOZ_ASSERT(member->isKind(PNK_COLON) || member->isKind(PNK_SHORTHAND));

            ParseNode *key = member->pn_left;
            if (key->isKind(PNK_NUMBER)) {
                if (!bce->emitNumberOp(key->pn_dval))                  
                    return false;
            } else if (key->isKind(PNK_OBJECT_PROPERTY_NAME) || key->isKind(PNK_STRING)) {
                PropertyName *name = key->pn_atom->asPropertyName();

                
                
                
                jsid id = NameToId(name);
                if (id != IdToTypeId(id)) {
                    if (!bce->emitTree(key))                           
                        return false;
                } else {
                    if (!bce->emitAtomOp(name, JSOP_GETPROP))          
                        return false;
                    needsGetElem = false;
                }
            } else {
                MOZ_ASSERT(key->isKind(PNK_COMPUTED_NAME));
                if (!bce->emitTree(key->pn_kid))                       
                    return false;
            }

            subpattern = member->pn_right;
        }

        
        if (needsGetElem && !bce->emitElemOpBase(JSOP_GETELEM))        
            return false;

        if (subpattern->isKind(PNK_ASSIGN)) {
            if (!bce->emitDefault(subpattern->pn_right))
                return false;
            subpattern = subpattern->pn_left;
        }

        
        int32_t depthBefore = bce->stackDepth;
        if (!bce->emitDestructuringLHS(subpattern, emitOption))
            return false;

        
        
        
        
        if (emitOption == InitializeVars)                              
            continue;

        MOZ_ASSERT(emitOption == PushInitialValues);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        MOZ_ASSERT((bce->stackDepth - bce->stackDepth) >= -1);
        uint32_t pickDistance = (uint32_t)((bce->stackDepth + 1) - depthBefore);
        if (pickDistance > 0) {
            if (pickDistance > UINT8_MAX) {
                bce->reportError(subpattern, JSMSG_TOO_MANY_LOCALS);
                return false;
            }
            if (!bce->emit2(JSOP_PICK, (jsbytecode)pickDistance))
                return false;
        }
    }

    if (emitOption == PushInitialValues) {
        
        
        
        if (!bce->emit1(JSOP_POP))                                 
            return false;
    }

    return true;
}














static bool
EmitDestructuringOpsHelper(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pattern,
                           VarEmitOption emitOption)
{
    MOZ_ASSERT(emitOption != DefineVars);

    if (pattern->isKind(PNK_ARRAY))
        return EmitDestructuringOpsArrayHelper(cx, bce, pattern, emitOption);
    return EmitDestructuringOpsObjectHelper(cx, bce, pattern, emitOption);
}

static bool
EmitDestructuringOps(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pattern,
                     bool isLet = false)
{
    



    VarEmitOption emitOption = isLet ? PushInitialValues : InitializeVars;
    return EmitDestructuringOpsHelper(cx, bce, pattern, emitOption);
}

bool
BytecodeEmitter::emitTemplateString(ParseNode *pn)
{
    MOZ_ASSERT(pn->isArity(PN_LIST));

    for (ParseNode *pn2 = pn->pn_head; pn2 != NULL; pn2 = pn2->pn_next) {
        if (pn2->getKind() != PNK_STRING && pn2->getKind() != PNK_TEMPLATE_STRING) {
            
            if (!updateSourceCoordNotes(pn2->pn_pos.begin))
                return false;
        }
        if (!emitTree(pn2))
            return false;

        if (pn2->getKind() != PNK_STRING && pn2->getKind() != PNK_TEMPLATE_STRING) {
            
            if (!emit1(JSOP_TOSTRING))
                return false;
        }

        if (pn2 != pn->pn_head) {
            
            if (!emit1(JSOP_ADD))
                return false;
        }

    }
    return true;
}

bool
BytecodeEmitter::emitVariables(ParseNode *pn, VarEmitOption emitOption, bool isLetExpr)
{
    MOZ_ASSERT(pn->isArity(PN_LIST));
    MOZ_ASSERT(isLetExpr == (emitOption == PushInitialValues));

    ParseNode *next;
    for (ParseNode *pn2 = pn->pn_head; ; pn2 = next) {
        if (!updateSourceCoordNotes(pn2->pn_pos.begin))
            return false;
        next = pn2->pn_next;

        ParseNode *pn3;
        if (!pn2->isKind(PNK_NAME)) {
            if (pn2->isKind(PNK_ARRAY) || pn2->isKind(PNK_OBJECT)) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                MOZ_ASSERT(pn->pn_count == 1);
                if (emitOption == DefineVars) {
                    if (!EmitDestructuringDecls(cx, this, pn->getOp(), pn2))
                        return false;
                } else {
                    
                    
                    MOZ_ASSERT(emitOption != DefineVars);
                    MOZ_ASSERT_IF(emitOption == InitializeVars, pn->pn_xflags & PNX_POPVAR);
                    if (!emit1(JSOP_UNDEFINED))
                        return false;
                    if (!EmitInitializeDestructuringDecls(cx, this, pn->getOp(), pn2))
                        return false;
                }
                break;
            }

            





            MOZ_ASSERT(pn2->isKind(PNK_ASSIGN));
            MOZ_ASSERT(pn2->isOp(JSOP_NOP));
            MOZ_ASSERT(emitOption != DefineVars);

            




            if (pn2->pn_left->isKind(PNK_NAME)) {
                pn3 = pn2->pn_right;
                pn2 = pn2->pn_left;
                goto do_name;
            }

            pn3 = pn2->pn_left;
            if (!EmitDestructuringDecls(cx, this, pn->getOp(), pn3))
                return false;

            if (!emitTree(pn2->pn_right))
                return false;

            if (!EmitDestructuringOps(cx, this, pn3, isLetExpr))
                return false;

            
            if (emitOption != InitializeVars) {
                if (next)
                    continue;
                break;
            }
            goto emit_note_pop;
        }

        





        pn3 = pn2->maybeExpr();

     do_name:
        if (!bindNameToSlot(pn2))
            return false;


        JSOp op;
        op = pn2->getOp();
        MOZ_ASSERT(op != JSOP_CALLEE);
        MOZ_ASSERT(!pn2->pn_cookie.isFree() || !pn->isOp(JSOP_NOP));

        jsatomid atomIndex;
        if (!maybeEmitVarDecl(pn->getOp(), pn2, &atomIndex))
            return false;

        if (pn3) {
            MOZ_ASSERT(emitOption != DefineVars);
            if (op == JSOP_SETNAME ||
                op == JSOP_STRICTSETNAME ||
                op == JSOP_SETGNAME ||
                op == JSOP_STRICTSETGNAME ||
                op == JSOP_SETINTRINSIC)
            {
                MOZ_ASSERT(emitOption != PushInitialValues);
                JSOp bindOp;
                if (op == JSOP_SETNAME || op == JSOP_STRICTSETNAME)
                    bindOp = JSOP_BINDNAME;
                else if (op == JSOP_SETGNAME || op == JSOP_STRICTSETGNAME)
                    bindOp = JSOP_BINDGNAME;
                else
                    bindOp = JSOP_BINDINTRINSIC;
                if (!emitIndex32(bindOp, atomIndex))
                    return false;
            }

            bool oldEmittingForInit = emittingForInit;
            emittingForInit = false;
            if (!emitTree(pn3))
                return false;
            emittingForInit = oldEmittingForInit;
        } else if (op == JSOP_INITLEXICAL || isLetExpr) {
            
            
            MOZ_ASSERT(emitOption != DefineVars);
            MOZ_ASSERT_IF(emitOption == InitializeVars, pn->pn_xflags & PNX_POPVAR);
            if (!emit1(JSOP_UNDEFINED))
                return false;
        }

        
        
        if (emitOption != InitializeVars) {
            if (next)
                continue;
            break;
        }

        MOZ_ASSERT_IF(pn2->isDefn(), pn3 == pn2->pn_expr);
        if (!pn2->pn_cookie.isFree()) {
            if (!emitVarOp(pn2, op))
                return false;
        } else {
            if (!emitIndexOp(op, atomIndex))
                return false;
        }

    emit_note_pop:
        if (!next)
            break;
        if (!emit1(JSOP_POP))
            return false;
    }

    if (pn->pn_xflags & PNX_POPVAR) {
        if (!emit1(JSOP_POP))
            return false;
    }

    return true;
}

bool
BytecodeEmitter::emitAssignment(ParseNode *lhs, JSOp op, ParseNode *rhs)
{
    




    jsatomid atomIndex = (jsatomid) -1;
    jsbytecode offset = 1;

    switch (lhs->getKind()) {
      case PNK_NAME:
        if (!bindNameToSlot(lhs))
            return false;
        if (lhs->pn_cookie.isFree()) {
            if (!makeAtomIndex(lhs->pn_atom, &atomIndex))
                return false;
            if (!lhs->isConst()) {
                JSOp bindOp;
                if (lhs->isOp(JSOP_SETNAME) || lhs->isOp(JSOP_STRICTSETNAME))
                    bindOp = JSOP_BINDNAME;
                else if (lhs->isOp(JSOP_SETGNAME) || lhs->isOp(JSOP_STRICTSETGNAME))
                    bindOp = JSOP_BINDGNAME;
                else
                    bindOp = JSOP_BINDINTRINSIC;
                if (!emitIndex32(bindOp, atomIndex))
                    return false;
                offset++;
            }
        }
        break;
      case PNK_DOT:
        if (!emitTree(lhs->expr()))
            return false;
        offset++;
        if (!makeAtomIndex(lhs->pn_atom, &atomIndex))
            return false;
        break;
      case PNK_ELEM:
        MOZ_ASSERT(lhs->isArity(PN_BINARY));
        if (!emitTree(lhs->pn_left))
            return false;
        if (!emitTree(lhs->pn_right))
            return false;
        offset += 2;
        break;
      case PNK_ARRAY:
      case PNK_OBJECT:
        break;
      case PNK_CALL:
        MOZ_ASSERT(lhs->pn_xflags & PNX_SETCALL);
        if (!emitTree(lhs))
            return false;
        if (!emit1(JSOP_POP))
            return false;
        break;
      default:
        MOZ_ASSERT(0);
    }

    if (op != JSOP_NOP) {
        MOZ_ASSERT(rhs);
        switch (lhs->getKind()) {
          case PNK_NAME:
            if (lhs->isConst()) {
                if (lhs->isOp(JSOP_CALLEE)) {
                    if (!emit1(JSOP_CALLEE))
                        return false;
                } else if (lhs->isOp(JSOP_GETNAME) || lhs->isOp(JSOP_GETGNAME)) {
                    if (!emitIndex32(lhs->getOp(), atomIndex))
                        return false;
                } else {
                    MOZ_ASSERT(JOF_OPTYPE(lhs->getOp()) != JOF_ATOM);
                    if (!emitVarOp(lhs, lhs->getOp()))
                        return false;
                }
            } else if (lhs->isOp(JSOP_SETNAME) || lhs->isOp(JSOP_STRICTSETNAME)) {
                if (!emit1(JSOP_DUP))
                    return false;
                if (!emitIndex32(JSOP_GETXPROP, atomIndex))
                    return false;
            } else if (lhs->isOp(JSOP_SETGNAME) || lhs->isOp(JSOP_STRICTSETGNAME)) {
                MOZ_ASSERT(lhs->pn_cookie.isFree());
                if (!emitAtomOp(lhs, JSOP_GETGNAME))
                    return false;
            } else if (lhs->isOp(JSOP_SETINTRINSIC)) {
                MOZ_ASSERT(lhs->pn_cookie.isFree());
                if (!emitAtomOp(lhs, JSOP_GETINTRINSIC))
                    return false;
            } else {
                JSOp op;
                switch (lhs->getOp()) {
                  case JSOP_SETARG: op = JSOP_GETARG; break;
                  case JSOP_SETLOCAL: op = JSOP_GETLOCAL; break;
                  case JSOP_SETALIASEDVAR: op = JSOP_GETALIASEDVAR; break;
                  default: MOZ_CRASH("Bad op");
                }
                if (!emitVarOp(lhs, op))
                    return false;
            }
            break;
          case PNK_DOT: {
            if (!emit1(JSOP_DUP))
                return false;
            bool isLength = (lhs->pn_atom == cx->names().length);
            if (!emitIndex32(isLength ? JSOP_LENGTH : JSOP_GETPROP, atomIndex))
                return false;
            break;
          }
          case PNK_ELEM:
            if (!emit1(JSOP_DUP2))
                return false;
            if (!emitElemOpBase(JSOP_GETELEM))
                return false;
            break;
          case PNK_CALL:
            




            MOZ_ASSERT(lhs->pn_xflags & PNX_SETCALL);
            if (!emit1(JSOP_NULL))
                return false;
            break;
          default:;
        }
    }

    
    if (rhs) {
        if (!emitTree(rhs))
            return false;
    } else {
        






        if (offset != 1 && !emit2(JSOP_PICK, offset - 1))
            return false;
    }

    
    if (op != JSOP_NOP) {
        




        if (!lhs->isKind(PNK_NAME) || !lhs->isConst()) {
            if (NewSrcNote(cx, this, SRC_ASSIGNOP) < 0)
                return false;
        }
        if (!emit1(op))
            return false;
    }

    
    switch (lhs->getKind()) {
      case PNK_NAME:
        if (lhs->isOp(JSOP_SETARG) || lhs->isOp(JSOP_SETLOCAL) || lhs->isOp(JSOP_SETALIASEDVAR)) {
            if (!emitVarOp(lhs, lhs->getOp()))
                return false;
        } else {
            if (!emitIndexOp(lhs->getOp(), atomIndex))
                return false;
        }
        break;
      case PNK_DOT:
      {
        JSOp setOp = sc->strict() ? JSOP_STRICTSETPROP : JSOP_SETPROP;
        if (!emitIndexOp(setOp, atomIndex))
            return false;
        break;
      }
      case PNK_CALL:
        
        MOZ_ASSERT(lhs->pn_xflags & PNX_SETCALL);
        break;
      case PNK_ELEM:
      {
        JSOp setOp = sc->strict() ? JSOP_STRICTSETELEM : JSOP_SETELEM;
        if (!emit1(setOp))
            return false;
        break;
      }
      case PNK_ARRAY:
      case PNK_OBJECT:
        if (!EmitDestructuringOps(cx, this, lhs))
            return false;
        break;
      default:
        MOZ_ASSERT(0);
    }
    return true;
}

bool
ParseNode::getConstantValue(ExclusiveContext *cx, AllowConstantObjects allowObjects, MutableHandleValue vp,
                            NewObjectKind newKind)
{
    MOZ_ASSERT(newKind == TenuredObject || newKind == SingletonObject);

    switch (getKind()) {
      case PNK_NUMBER:
        vp.setNumber(pn_dval);
        return true;
      case PNK_TEMPLATE_STRING:
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
      case PNK_CALLSITEOBJ:
      case PNK_ARRAY: {
        RootedValue value(cx);
        unsigned count;
        ParseNode *pn;

        if (allowObjects == DontAllowObjects) {
            vp.setMagic(JS_GENERIC_MAGIC);
            return true;
        }
        if (allowObjects == DontAllowNestedObjects)
            allowObjects = DontAllowObjects;

        if (getKind() == PNK_CALLSITEOBJ) {
            count = pn_count - 1;
            pn = pn_head->pn_next;
        } else {
            MOZ_ASSERT(isOp(JSOP_NEWINIT) && !(pn_xflags & PNX_NONCONST));
            count = pn_count;
            pn = pn_head;
        }

        RootedArrayObject obj(cx, NewDenseFullyAllocatedArray(cx, count, NullPtr(), newKind));
        if (!obj)
            return false;

        unsigned idx = 0;
        RootedId id(cx);
        for (; pn; idx++, pn = pn->pn_next) {
            if (!pn->getConstantValue(cx, allowObjects, &value))
                return false;
            if (value.isMagic(JS_GENERIC_MAGIC)) {
                vp.setMagic(JS_GENERIC_MAGIC);
                return true;
            }
            id = INT_TO_JSID(idx);
            if (!DefineProperty(cx, obj, id, value, nullptr, nullptr, JSPROP_ENUMERATE))
                return false;
        }
        MOZ_ASSERT(idx == count);

        ObjectGroup::fixArrayGroup(cx, obj);
        vp.setObject(*obj);
        return true;
      }
      case PNK_OBJECT: {
        MOZ_ASSERT(isOp(JSOP_NEWINIT));
        MOZ_ASSERT(!(pn_xflags & PNX_NONCONST));

        if (allowObjects == DontAllowObjects) {
            vp.setMagic(JS_GENERIC_MAGIC);
            return true;
        }
        if (allowObjects == DontAllowNestedObjects)
            allowObjects = DontAllowObjects;

        AutoIdValueVector properties(cx);

        RootedValue value(cx), idvalue(cx);
        for (ParseNode *pn = pn_head; pn; pn = pn->pn_next) {
            if (!pn->pn_right->getConstantValue(cx, allowObjects, &value))
                return false;
            if (value.isMagic(JS_GENERIC_MAGIC)) {
                vp.setMagic(JS_GENERIC_MAGIC);
                return true;
            }

            ParseNode *pnid = pn->pn_left;
            if (pnid->isKind(PNK_NUMBER)) {
                idvalue = NumberValue(pnid->pn_dval);
            } else {
                MOZ_ASSERT(pnid->isKind(PNK_OBJECT_PROPERTY_NAME) || pnid->isKind(PNK_STRING));
                MOZ_ASSERT(pnid->pn_atom != cx->names().proto);
                idvalue = StringValue(pnid->pn_atom);
            }

            RootedId id(cx);
            if (!ValueToId<CanGC>(cx, idvalue, &id))
                return false;

            if (!properties.append(IdValuePair(id, value)))
                return false;
        }

        JSObject *obj = ObjectGroup::newPlainObject(cx, properties.begin(), properties.length(),
                                                    newKind);
        if (!obj)
            return false;

        vp.setObject(*obj);
        return true;
      }
      default:
        MOZ_CRASH("Unexpected node");
    }
    return false;
}

bool
BytecodeEmitter::emitSingletonInitialiser(ParseNode *pn)
{
    NewObjectKind newKind = (pn->getKind() == PNK_OBJECT) ? SingletonObject : TenuredObject;

    RootedValue value(cx);
    if (!pn->getConstantValue(cx, ParseNode::AllowObjects, &value, newKind))
        return false;

    MOZ_ASSERT_IF(newKind == SingletonObject, value.toObject().isSingleton());

    ObjectBox *objbox = parser->newObjectBox(&value.toObject());
    if (!objbox)
        return false;

    return emitObjectOp(objbox, JSOP_OBJECT);
}

bool
BytecodeEmitter::emitCallSiteObject(ParseNode *pn)
{
    RootedValue value(cx);
    if (!pn->getConstantValue(cx, ParseNode::AllowObjects, &value))
        return false;

    MOZ_ASSERT(value.isObject());

    ObjectBox *objbox1 = parser->newObjectBox(&value.toObject().as<NativeObject>());
    if (!objbox1)
        return false;

    if (!pn->as<CallSiteNode>().getRawArrayValue(cx, &value))
        return false;

    MOZ_ASSERT(value.isObject());

    ObjectBox *objbox2 = parser->newObjectBox(&value.toObject().as<NativeObject>());
    if (!objbox2)
        return false;

    return emitObjectPairOp(objbox1, objbox2, JSOP_CALLSITEOBJ);
}


JS_STATIC_ASSERT(JSOP_NOP_LENGTH == 1);
JS_STATIC_ASSERT(JSOP_POP_LENGTH == 1);

namespace {

class EmitLevelManager
{
    BytecodeEmitter *bce;
  public:
    explicit EmitLevelManager(BytecodeEmitter *bce) : bce(bce) { bce->emitLevel++; }
    ~EmitLevelManager() { bce->emitLevel--; }
};

} 

bool
BytecodeEmitter::emitCatch(ParseNode *pn)
{
    



    StmtInfoBCE *stmt = topStmt;
    MOZ_ASSERT(stmt->type == STMT_BLOCK && stmt->isBlockScope);
    stmt->type = STMT_CATCH;

    
    stmt = stmt->down;
    MOZ_ASSERT(stmt->type == STMT_TRY || stmt->type == STMT_FINALLY);

    
    if (!emit1(JSOP_EXCEPTION))
        return false;

    



    if (pn->pn_kid2 && !emit1(JSOP_DUP))
        return false;

    ParseNode *pn2 = pn->pn_kid1;
    switch (pn2->getKind()) {
      case PNK_ARRAY:
      case PNK_OBJECT:
        if (!EmitDestructuringOps(cx, this, pn2))
            return false;
        if (!emit1(JSOP_POP))
            return false;
        break;

      case PNK_NAME:
        
        MOZ_ASSERT(!pn2->pn_cookie.isFree());
        if (!emitVarOp(pn2, JSOP_INITLEXICAL))
            return false;
        if (!emit1(JSOP_POP))
            return false;
        break;

      default:
        MOZ_ASSERT(0);
    }

    
    
    if (pn->pn_kid2) {
        if (!emitTree(pn->pn_kid2))
            return false;

        
        
        
        ptrdiff_t guardCheck = emitJump(JSOP_IFNE, 0);
        if (guardCheck < 0)
            return false;

        {
            NonLocalExitScope nle(cx, this);

            
            
            if (!emit1(JSOP_THROWING))
                return false;

            
            if (!nle.prepareForNonLocalJump(stmt))
                return false;

            
            ptrdiff_t guardJump = emitJump(JSOP_GOTO, 0);
            if (guardJump < 0)
                return false;
            stmt->guardJump() = guardJump;
        }

        
        setJumpOffsetAt(guardCheck);

        
        if (!emit1(JSOP_POP))
            return false;
    }

    
    return emitTree(pn->pn_kid3);
}



MOZ_NEVER_INLINE bool
BytecodeEmitter::emitTry(ParseNode *pn)
{
    StmtInfoBCE stmtInfo(cx);

    
    
    
    
    
    
    
    
    pushStatement(&stmtInfo, pn->pn_kid3 ? STMT_FINALLY : STMT_TRY, offset());

    
    
    
    
    
    
    
    
    int depth = stackDepth;

    
    ptrdiff_t noteIndex = NewSrcNote(cx, this, SRC_TRY);
    if (noteIndex < 0 || !emit1(JSOP_TRY))
        return false;
    ptrdiff_t tryStart = offset();
    if (!emitTree(pn->pn_kid1))
        return false;
    MOZ_ASSERT(depth == stackDepth);

    
    if (pn->pn_kid3) {
        if (!emitBackPatchOp(&stmtInfo.gosubs()))
            return false;
    }

    
    if (!setSrcNoteOffset(noteIndex, 0, offset() - tryStart + JSOP_TRY_LENGTH))
        return false;

    
    ptrdiff_t catchJump = -1;
    if (!emitBackPatchOp(&catchJump))
        return false;

    ptrdiff_t tryEnd = offset();

    
    ParseNode *catchList = pn->pn_kid2;
    if (catchList) {
        MOZ_ASSERT(catchList->isKind(PNK_CATCHLIST));

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        for (ParseNode *pn3 = catchList->pn_head; pn3; pn3 = pn3->pn_next) {
            MOZ_ASSERT(this->stackDepth == depth);

            
            MOZ_ASSERT(pn3->isKind(PNK_LEXICALSCOPE));
            if (!emitTree(pn3))
                return false;

            
            if (pn->pn_kid3) {
                if (!emitBackPatchOp(&stmtInfo.gosubs()))
                    return false;
                MOZ_ASSERT(this->stackDepth == depth);
            }

            
            
            if (!emitBackPatchOp(&catchJump))
                return false;

            
            
            if (stmtInfo.guardJump() != -1) {
                setJumpOffsetAt(stmtInfo.guardJump());
                stmtInfo.guardJump() = -1;

                
                
                if (!pn3->pn_next) {
                    if (!emit1(JSOP_EXCEPTION))
                        return false;
                    if (!emit1(JSOP_THROW))
                        return false;
                }
            }
        }
    }

    MOZ_ASSERT(this->stackDepth == depth);

    
    ptrdiff_t finallyStart = 0;
    if (pn->pn_kid3) {
        
        
        backPatch(stmtInfo.gosubs(), code().end(), JSOP_GOSUB);

        finallyStart = offset();

        
        stmtInfo.type = STMT_SUBROUTINE;
        if (!updateSourceCoordNotes(pn->pn_kid3->pn_pos.begin))
            return false;
        if (!emit1(JSOP_FINALLY) ||
            !emitTree(pn->pn_kid3) ||
            !emit1(JSOP_RETSUB))
        {
            return false;
        }
        hasTryFinally = true;
        MOZ_ASSERT(this->stackDepth == depth);
    }
    popStatement();

    
    if (!emit1(JSOP_NOP))
        return false;

    
    backPatch(catchJump, code().end(), JSOP_GOTO);

    
    
    if (catchList && !tryNoteList.append(JSTRY_CATCH, depth, tryStart, tryEnd))
        return false;

    
    
    
    if (pn->pn_kid3 && !tryNoteList.append(JSTRY_FINALLY, depth, tryStart, finallyStart))
        return false;

    return true;
}

bool
BytecodeEmitter::emitIf(ParseNode *pn)
{
    StmtInfoBCE stmtInfo(cx);

    
    stmtInfo.type = STMT_IF;
    ptrdiff_t beq = -1;
    ptrdiff_t jmp = -1;
    ptrdiff_t noteIndex = -1;

  if_again:
    
    if (!emitTree(pn->pn_kid1))
        return false;
    ptrdiff_t top = offset();
    if (stmtInfo.type == STMT_IF) {
        pushStatement(&stmtInfo, STMT_IF, top);
    } else {
        




        MOZ_ASSERT(stmtInfo.type == STMT_ELSE);
        stmtInfo.type = STMT_IF;
        stmtInfo.update = top;
        if (!setSrcNoteOffset(noteIndex, 0, jmp - beq))
            return false;
    }

    
    ParseNode *pn3 = pn->pn_kid3;
    noteIndex = NewSrcNote(cx, this, pn3 ? SRC_IF_ELSE : SRC_IF);
    if (noteIndex < 0)
        return false;
    beq = emitJump(JSOP_IFEQ, 0);
    if (beq < 0)
        return false;

    
    if (!emitTree(pn->pn_kid2))
        return false;
    if (pn3) {
        
        stmtInfo.type = STMT_ELSE;

        





        jmp = emitGoto(&stmtInfo, &stmtInfo.breaks);
        if (jmp < 0)
            return false;

        
        setJumpOffsetAt(beq);
        if (pn3->isKind(PNK_IF)) {
            pn = pn3;
            goto if_again;
        }

        if (!emitTree(pn3))
            return false;

        






        if (!setSrcNoteOffset(noteIndex, 0, jmp - beq))
            return false;
    } else {
        
        setJumpOffsetAt(beq);
    }

    popStatement();
    return true;
}



































MOZ_NEVER_INLINE static bool
EmitLet(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pnLet)
{
    MOZ_ASSERT(pnLet->isArity(PN_BINARY));
    ParseNode *varList = pnLet->pn_left;
    MOZ_ASSERT(varList->isArity(PN_LIST));
    ParseNode *letBody = pnLet->pn_right;
    MOZ_ASSERT(letBody->isLexical() && letBody->isKind(PNK_LEXICALSCOPE));

    int letHeadDepth = bce->stackDepth;

    if (!bce->emitVariables(varList, PushInitialValues, true))
        return false;

    
    uint32_t valuesPushed = bce->stackDepth - letHeadDepth;
    StmtInfoBCE stmtInfo(cx);
    if (!EnterBlockScope(cx, bce, &stmtInfo, letBody->pn_objbox, JSOP_UNINITIALIZED, valuesPushed))
        return false;

    if (!bce->emitTree(letBody->pn_expr))
        return false;

    if (!LeaveNestedScope(cx, bce, &stmtInfo))
        return false;

    return true;
}





MOZ_NEVER_INLINE static bool
EmitLexicalScope(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pn)
{
    MOZ_ASSERT(pn->isKind(PNK_LEXICALSCOPE));

    StmtInfoBCE stmtInfo(cx);
    if (!EnterBlockScope(cx, bce, &stmtInfo, pn->pn_objbox, JSOP_UNINITIALIZED, 0))
        return false;

    if (!bce->emitTree(pn->pn_expr))
        return false;

    if (!LeaveNestedScope(cx, bce, &stmtInfo))
        return false;

    return true;
}

bool
BytecodeEmitter::emitWith(ParseNode *pn)
{
    StmtInfoBCE stmtInfo(cx);
    if (!emitTree(pn->pn_left))
        return false;
    if (!EnterNestedScope(cx, this, &stmtInfo, pn->pn_binary_obj, STMT_WITH))
        return false;
    if (!emitTree(pn->pn_right))
        return false;
    if (!LeaveNestedScope(cx, this, &stmtInfo))
        return false;
    return true;
}

bool
BytecodeEmitter::emitIterator()
{
    
    if (!emit1(JSOP_DUP))                                 
        return false;
    if (!emit2(JSOP_SYMBOL, jsbytecode(JS::SymbolCode::iterator))) 
        return false;
    if (!emitElemOpBase(JSOP_CALLELEM))                   
        return false;
    if (!emit1(JSOP_SWAP))                                
        return false;
    if (!emitCall(JSOP_CALL, 0))                          
        return false;
    checkTypeSet(JSOP_CALL);
    return true;
}

bool
BytecodeEmitter::emitForInOrOfVariables(ParseNode *pn, bool *letDecl)
{
    *letDecl = pn->isKind(PNK_LEXICALSCOPE);
    MOZ_ASSERT_IF(*letDecl, pn->isLexical());

    
    
    
    
    
    
    
    
    
    
    if (!*letDecl) {
        emittingForInit = true;
        if (pn->isKind(PNK_VAR)) {
            if (!emitVariables(pn, DefineVars))
                return false;
        } else {
            MOZ_ASSERT(pn->isKind(PNK_LET));
            if (!emitVariables(pn, InitializeVars))
                return false;
        }
        emittingForInit = false;
    }

    return true;
}


bool
BytecodeEmitter::emitForOf(StmtType type, ParseNode *pn, ptrdiff_t top)
{
    MOZ_ASSERT(type == STMT_FOR_OF_LOOP || type == STMT_SPREAD);
    MOZ_ASSERT_IF(type == STMT_FOR_OF_LOOP, pn && pn->pn_left->isKind(PNK_FOROF));
    MOZ_ASSERT_IF(type == STMT_SPREAD, !pn);

    ParseNode *forHead = pn ? pn->pn_left : nullptr;
    ParseNode *forHeadExpr = forHead ? forHead->pn_kid3 : nullptr;
    ParseNode *forBody = pn ? pn->pn_right : nullptr;

    ParseNode *pn1 = forHead ? forHead->pn_kid1 : nullptr;
    bool letDecl = false;
    if (pn1 && !emitForInOrOfVariables(pn1, &letDecl))
        return false;

    if (type == STMT_FOR_OF_LOOP) {
        
        

        
        if (!emitTree(forHeadExpr))
            return false;
        if (!emitIterator())
            return false;

        
        if (!emit1(JSOP_UNDEFINED))                
            return false;
    }

    
    
    
    StmtInfoBCE letStmt(cx);
    if (letDecl) {
        if (!EnterBlockScope(cx, this, &letStmt, pn1->pn_objbox, JSOP_UNDEFINED, 0))
            return false;
    }

    LoopStmtInfo stmtInfo(cx);
    PushLoopStatement(this, &stmtInfo, type, top);

    
    
    
    int noteIndex = NewSrcNote(cx, this, SRC_FOR_OF);
    if (noteIndex < 0)
        return false;
    ptrdiff_t jmp = emitJump(JSOP_GOTO, 0);
    if (jmp < 0)
        return false;

    top = offset();
    SET_STATEMENT_TOP(&stmtInfo, top);
    if (!emitLoopHead(nullptr))
        return false;

    if (type == STMT_SPREAD)
        this->stackDepth++;

#ifdef DEBUG
    int loopDepth = this->stackDepth;
#endif

    
    if (type == STMT_FOR_OF_LOOP) {
        if (!emit1(JSOP_DUP))                             
            return false;
    }
    if (!emitAtomOp(cx->names().value, JSOP_GETPROP))     
        return false;
    if (type == STMT_FOR_OF_LOOP) {
        if (!emitAssignment(forHead->pn_kid2, JSOP_NOP, nullptr)) 
            return false;
        if (!emit1(JSOP_POP))                             
            return false;

        
        MOZ_ASSERT(this->stackDepth == loopDepth);

        
        if (!emitTree(forBody))
            return false;

        
        StmtInfoBCE *stmt = &stmtInfo;
        do {
            stmt->update = offset();
        } while ((stmt = stmt->down) != nullptr && stmt->type == STMT_LABEL);
    } else {
        if (!emit1(JSOP_INITELEM_INC))                    
            return false;

        MOZ_ASSERT(this->stackDepth == loopDepth - 1);

        
    }

    
    setJumpOffsetAt(jmp);
    if (!emitLoopEntry(forHeadExpr))
        return false;

    if (type == STMT_FOR_OF_LOOP) {
        if (!emit1(JSOP_POP))                             
            return false;
        if (!emit1(JSOP_DUP))                             
            return false;
    } else {
        if (!emitDupAt(this->stackDepth - 1 - 2))         
            return false;
    }
    if (!emitIteratorNext(forHead))                       
        return false;
    if (!emit1(JSOP_DUP))                                 
        return false;
    if (!emitAtomOp(cx->names().done, JSOP_GETPROP))      
        return false;

    ptrdiff_t beq = emitJump(JSOP_IFEQ, top - offset());  
    if (beq < 0)
        return false;

    MOZ_ASSERT(this->stackDepth == loopDepth);

    
    if (!setSrcNoteOffset(unsigned(noteIndex), 0, beq - jmp))
        return false;

    
    
    popStatement();

    if (!tryNoteList.append(JSTRY_FOR_OF, stackDepth, top, offset()))
        return false;

    if (letDecl) {
        if (!LeaveNestedScope(cx, this, &letStmt))
            return false;
    }

    if (type == STMT_SPREAD) {
        if (!emit2(JSOP_PICK, (jsbytecode)3))      
            return false;
    }

    
    return emitUint16Operand(JSOP_POPN, 2);
}

bool
BytecodeEmitter::emitForIn(ParseNode *pn, ptrdiff_t top)
{
    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    ParseNode *pn1 = forHead->pn_kid1;
    bool letDecl = false;
    if (pn1 && !emitForInOrOfVariables(pn1, &letDecl))
        return false;

    
    if (!emitTree(forHead->pn_kid3))
        return false;

    




    MOZ_ASSERT(pn->isOp(JSOP_ITER));
    if (!emit2(JSOP_ITER, (uint8_t) pn->pn_iflags))
        return false;

    
    
    if (!emit1(JSOP_UNDEFINED))
        return false;

    
    
    
    StmtInfoBCE letStmt(cx);
    if (letDecl) {
        if (!EnterBlockScope(cx, this, &letStmt, pn1->pn_objbox, JSOP_UNDEFINED, 0))
            return false;
    }

    LoopStmtInfo stmtInfo(cx);
    PushLoopStatement(this, &stmtInfo, STMT_FOR_IN_LOOP, top);

    
    int noteIndex = NewSrcNote(cx, this, SRC_FOR_IN);
    if (noteIndex < 0)
        return false;

    



    ptrdiff_t jmp = emitJump(JSOP_GOTO, 0);
    if (jmp < 0)
        return false;

    top = offset();
    SET_STATEMENT_TOP(&stmtInfo, top);
    if (!emitLoopHead(nullptr))
        return false;

#ifdef DEBUG
    int loopDepth = this->stackDepth;
#endif

    
    
    if (!emitAssignment(forHead->pn_kid2, JSOP_NOP, nullptr))
        return false;

    
    MOZ_ASSERT(this->stackDepth == loopDepth);

    
    if (!emitTree(forBody))
        return false;

    
    StmtInfoBCE *stmt = &stmtInfo;
    do {
        stmt->update = offset();
    } while ((stmt = stmt->down) != nullptr && stmt->type == STMT_LABEL);

    


    setJumpOffsetAt(jmp);
    if (!emitLoopEntry(nullptr))
        return false;
    if (!emit1(JSOP_POP))
        return false;
    if (!emit1(JSOP_MOREITER))
        return false;
    if (!emit1(JSOP_ISNOITER))
        return false;
    ptrdiff_t beq = emitJump(JSOP_IFEQ, top - offset());
    if (beq < 0)
        return false;

    
    if (!setSrcNoteOffset(unsigned(noteIndex), 0, beq - jmp))
        return false;

    
    popStatement();

    
    if (!emit1(JSOP_POP))
        return false;

    if (!tryNoteList.append(JSTRY_FOR_IN, this->stackDepth, top, offset()))
        return false;
    if (!emit1(JSOP_ENDITER))
        return false;

    if (letDecl) {
        if (!LeaveNestedScope(cx, this, &letStmt))
            return false;
    }

    return true;
}

bool
BytecodeEmitter::emitNormalFor(ParseNode *pn, ptrdiff_t top)
{
    LoopStmtInfo stmtInfo(cx);
    PushLoopStatement(this, &stmtInfo, STMT_FOR_LOOP, top);

    ParseNode *forHead = pn->pn_left;
    ParseNode *forBody = pn->pn_right;

    
    JSOp op = JSOP_POP;
    ParseNode *pn3 = forHead->pn_kid1;
    if (!pn3) {
        
        
        op = JSOP_NOP;
    } else {
        emittingForInit = true;
        if (!updateSourceCoordNotes(pn3->pn_pos.begin))
            return false;
        if (!emitTree(pn3))
            return false;
        emittingForInit = false;
    }

    





    int noteIndex = NewSrcNote(cx, this, SRC_FOR);
    if (noteIndex < 0 || !emit1(op))
        return false;
    ptrdiff_t tmp = offset();

    ptrdiff_t jmp = -1;
    if (forHead->pn_kid2) {
        
        jmp = emitJump(JSOP_GOTO, 0);
        if (jmp < 0)
            return false;
    } else {
        if (op != JSOP_NOP && !emit1(JSOP_NOP))
            return false;
    }

    top = offset();
    SET_STATEMENT_TOP(&stmtInfo, top);

    
    if (!emitLoopHead(forBody))
        return false;
    if (jmp == -1 && !emitLoopEntry(forBody))
        return false;
    if (!emitTree(forBody))
        return false;

    
    MOZ_ASSERT(noteIndex != -1);
    ptrdiff_t tmp2 = offset();

    
    StmtInfoBCE *stmt = &stmtInfo;
    do {
        stmt->update = offset();
    } while ((stmt = stmt->down) != nullptr && stmt->type == STMT_LABEL);

    
    pn3 = forHead->pn_kid3;
    if (pn3) {
        if (!updateSourceCoordNotes(pn3->pn_pos.begin))
            return false;
        op = JSOP_POP;
        if (!emitTree(pn3))
            return false;

        
        if (!emit1(op))
            return false;

        
        uint32_t lineNum = parser->tokenStream.srcCoords.lineNum(pn->pn_pos.end);
        if (currentLine() != lineNum) {
            if (NewSrcNote2(cx, this, SRC_SETLINE, ptrdiff_t(lineNum)) < 0)
                return false;
            current->currentLine = lineNum;
            current->lastColumn = 0;
        }
    }

    ptrdiff_t tmp3 = offset();

    if (forHead->pn_kid2) {
        
        MOZ_ASSERT(jmp >= 0);
        setJumpOffsetAt(jmp);
        if (!emitLoopEntry(forHead->pn_kid2))
            return false;

        if (!emitTree(forHead->pn_kid2))
            return false;
    }

    
    if (!setSrcNoteOffset(unsigned(noteIndex), 0, tmp3 - tmp))
        return false;
    if (!setSrcNoteOffset(unsigned(noteIndex), 1, tmp2 - tmp))
        return false;
    
    if (!setSrcNoteOffset(unsigned(noteIndex), 2, offset() - tmp))
        return false;

    
    op = forHead->pn_kid2 ? JSOP_IFNE : JSOP_GOTO;
    if (emitJump(op, top - offset()) < 0)
        return false;

    if (!tryNoteList.append(JSTRY_LOOP, stackDepth, top, offset()))
        return false;

    
    popStatement();
    return true;
}

bool
BytecodeEmitter::emitFor(ParseNode *pn, ptrdiff_t top)
{
    if (pn->pn_left->isKind(PNK_FORIN))
        return emitForIn(pn, top);

    if (pn->pn_left->isKind(PNK_FOROF))
        return emitForOf(STMT_FOR_OF_LOOP, pn, top);

    MOZ_ASSERT(pn->pn_left->isKind(PNK_FORHEAD));
    return emitNormalFor(pn, top);
}

static MOZ_NEVER_INLINE bool
EmitFunc(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pn, bool needsProto = false)
{
    FunctionBox *funbox = pn->pn_funbox;
    RootedFunction fun(cx, funbox->function());
    MOZ_ASSERT_IF(fun->isInterpretedLazy(), fun->lazyScript());

    




    if (pn->pn_dflags & PND_EMITTEDFUNCTION) {
        MOZ_ASSERT_IF(fun->hasScript(), fun->nonLazyScript());
        MOZ_ASSERT(pn->functionIsHoisted());
        MOZ_ASSERT(bce->sc->isFunctionBox());
        return true;
    }

    pn->pn_dflags |= PND_EMITTEDFUNCTION;

    





    if (fun->isInterpreted()) {
        bool singleton =
            bce->script->compileAndGo() &&
            fun->isInterpreted() &&
            (bce->checkSingletonContext() ||
             (!bce->isInLoop() && bce->isRunOnceLambda()));
        if (!JSFunction::setTypeForScriptedFunction(cx, fun, singleton))
            return false;

        if (fun->isInterpretedLazy()) {
            if (!fun->lazyScript()->sourceObject()) {
                JSObject *scope = EnclosingStaticScope(bce);
                JSObject *source = bce->script->sourceObject();
                fun->lazyScript()->setParent(scope, &source->as<ScriptSourceObject>());
            }
            if (bce->emittingRunOnceLambda)
                fun->lazyScript()->setTreatAsRunOnce();
        } else {
            SharedContext *outersc = bce->sc;

            if (outersc->isFunctionBox() && outersc->asFunctionBox()->mightAliasLocals())
                funbox->setMightAliasLocals();      
            MOZ_ASSERT_IF(outersc->strict(), funbox->strictScript);

            
            Rooted<JSScript*> parent(cx, bce->script);
            CompileOptions options(cx, bce->parser->options());
            options.setMutedErrors(parent->mutedErrors())
                   .setCompileAndGo(parent->compileAndGo())
                   .setHasPollutedScope(parent->hasPollutedGlobalScope())
                   .setSelfHostingMode(parent->selfHosted())
                   .setNoScriptRval(false)
                   .setForEval(false)
                   .setVersion(parent->getVersion());

            Rooted<JSObject*> enclosingScope(cx, EnclosingStaticScope(bce));
            Rooted<JSObject*> sourceObject(cx, bce->script->sourceObject());
            Rooted<JSScript*> script(cx, JSScript::Create(cx, enclosingScope, false, options,
                                                          parent->staticLevel() + 1,
                                                          sourceObject,
                                                          funbox->bufStart, funbox->bufEnd));
            if (!script)
                return false;

            script->bindings = funbox->bindings;

            uint32_t lineNum = bce->parser->tokenStream.srcCoords.lineNum(pn->pn_pos.begin);
            BytecodeEmitter bce2(bce, bce->parser, funbox, script,  js::NullPtr(),
                                 bce->insideEval, bce->evalCaller,
                                  js::NullPtr(),
                                 bce->insideNonGlobalEval, lineNum, bce->emitterMode);
            if (!bce2.init())
                return false;

            
            if (!EmitFunctionScript(cx, &bce2, pn->pn_body))
                return false;

            if (funbox->usesArguments && funbox->usesApply && funbox->usesThis)
                script->setUsesArgumentsApplyAndThis();
        }
    } else {
        MOZ_ASSERT(IsAsmJSModuleNative(fun->native()));
    }

    
    unsigned index = bce->objectList.add(pn->pn_funbox);

    
    if (!pn->functionIsHoisted()) {
        
        MOZ_ASSERT(fun->isArrow() == (pn->getOp() == JSOP_LAMBDA_ARROW));
        if (fun->isArrow() && !bce->emit1(JSOP_THIS))
            return false;
        if (needsProto) {
            MOZ_ASSERT(pn->getOp() == JSOP_LAMBDA);
            pn->setOp(JSOP_FUNWITHPROTO);
        }
        return bce->emitIndex32(pn->getOp(), index);
    }

    MOZ_ASSERT(!needsProto);

    








    if (!bce->sc->isFunctionBox()) {
        MOZ_ASSERT(pn->pn_cookie.isFree());
        MOZ_ASSERT(pn->getOp() == JSOP_NOP);
        MOZ_ASSERT(!bce->topStmt);
        bce->switchToProlog();
        if (!bce->emitIndex32(JSOP_DEFFUN, index))
            return false;
        if (!bce->updateSourceCoordNotes(pn->pn_pos.begin))
            return false;
        bce->switchToMain();
    } else {
#ifdef DEBUG
        BindingIter bi(bce->script);
        while (bi->name() != fun->atom())
            bi++;
        MOZ_ASSERT(bi->kind() == Binding::VARIABLE || bi->kind() == Binding::CONSTANT ||
                   bi->kind() == Binding::ARGUMENT);
        MOZ_ASSERT(bi.argOrLocalIndex() < JS_BIT(20));
#endif
        pn->pn_index = index;
        if (!bce->emitIndexOp(JSOP_LAMBDA, index))
            return false;
        MOZ_ASSERT(pn->getOp() == JSOP_GETLOCAL || pn->getOp() == JSOP_GETARG);
        JSOp setOp = pn->getOp() == JSOP_GETLOCAL ? JSOP_SETLOCAL : JSOP_SETARG;
        if (!bce->emitVarOp(pn, setOp))
            return false;
        if (!bce->emit1(JSOP_POP))
            return false;
    }

    return true;
}

bool
BytecodeEmitter::emitDo(ParseNode *pn)
{
    
    ptrdiff_t noteIndex = NewSrcNote(cx, this, SRC_WHILE);
    if (noteIndex < 0 || !emit1(JSOP_NOP))
        return false;

    ptrdiff_t noteIndex2 = NewSrcNote(cx, this, SRC_WHILE);
    if (noteIndex2 < 0)
        return false;

    
    ptrdiff_t top = offset();
    if (!emitLoopHead(pn->pn_left))
        return false;

    LoopStmtInfo stmtInfo(cx);
    PushLoopStatement(this, &stmtInfo, STMT_DO_LOOP, top);

    if (!emitLoopEntry(nullptr))
        return false;

    if (!emitTree(pn->pn_left))
        return false;

    
    ptrdiff_t off = offset();
    StmtInfoBCE *stmt = &stmtInfo;
    do {
        stmt->update = off;
    } while ((stmt = stmt->down) != nullptr && stmt->type == STMT_LABEL);

    
    if (!emitTree(pn->pn_right))
        return false;

    ptrdiff_t beq = emitJump(JSOP_IFNE, top - offset());
    if (beq < 0)
        return false;

    if (!tryNoteList.append(JSTRY_LOOP, stackDepth, top, offset()))
        return false;

    






    if (!setSrcNoteOffset(noteIndex2, 0, beq - top))
        return false;
    if (!setSrcNoteOffset(noteIndex, 0, 1 + (off - top)))
        return false;

    popStatement();
    return true;
}

bool
BytecodeEmitter::emitWhile(ParseNode *pn, ptrdiff_t top)
{
    












    LoopStmtInfo stmtInfo(cx);
    PushLoopStatement(this, &stmtInfo, STMT_WHILE_LOOP, top);

    ptrdiff_t noteIndex = NewSrcNote(cx, this, SRC_WHILE);
    if (noteIndex < 0)
        return false;

    ptrdiff_t jmp = emitJump(JSOP_GOTO, 0);
    if (jmp < 0)
        return false;

    top = offset();
    if (!emitLoopHead(pn->pn_right))
        return false;

    if (!emitTree(pn->pn_right))
        return false;

    setJumpOffsetAt(jmp);
    if (!emitLoopEntry(pn->pn_left))
        return false;
    if (!emitTree(pn->pn_left))
        return false;

    ptrdiff_t beq = emitJump(JSOP_IFNE, top - offset());
    if (beq < 0)
        return false;

    if (!tryNoteList.append(JSTRY_LOOP, stackDepth, top, offset()))
        return false;

    if (!setSrcNoteOffset(noteIndex, 0, beq - jmp))
        return false;

    popStatement();
    return true;
}

bool
BytecodeEmitter::emitBreak(PropertyName *label)
{
    StmtInfoBCE *stmt = topStmt;
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

    return emitGoto(stmt, &stmt->breaks, noteType) >= 0;
}

bool
BytecodeEmitter::emitContinue(PropertyName *label)
{
    StmtInfoBCE *stmt = topStmt;
    if (label) {
        
        StmtInfoBCE *loop = nullptr;
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

    return emitGoto(stmt, &stmt->continues, SRC_CONTINUE) >= 0;
}

static bool
InTryBlockWithFinally(BytecodeEmitter *bce)
{
    for (StmtInfoBCE *stmt = bce->topStmt; stmt; stmt = stmt->down) {
        if (stmt->type == STMT_FINALLY)
            return true;
    }
    return false;
}

bool
BytecodeEmitter::emitReturn(ParseNode *pn)
{
    if (!updateSourceCoordNotes(pn->pn_pos.begin))
        return false;

    if (sc->isFunctionBox() && sc->asFunctionBox()->isStarGenerator()) {
        if (!emitPrepareIteratorResult())
            return false;
    }

    
    if (ParseNode *pn2 = pn->pn_left) {
        if (!emitTree(pn2))
            return false;
    } else {
        
        if (!emit1(JSOP_UNDEFINED))
            return false;
    }

    if (sc->isFunctionBox() && sc->asFunctionBox()->isStarGenerator()) {
        if (!emitFinishIteratorResult(true))
            return false;
    }

    










    ptrdiff_t top = offset();

    bool isGenerator = sc->isFunctionBox() && sc->asFunctionBox()->isGenerator();
    bool useGenRVal = false;
    if (isGenerator) {
        if (sc->asFunctionBox()->isStarGenerator() && InTryBlockWithFinally(this)) {
            
            
            useGenRVal = true;
            MOZ_ASSERT(pn->pn_right);
            if (!emitTree(pn->pn_right))
                return false;
            if (!emit1(JSOP_POP))
                return false;
        } else {
            if (!emit1(JSOP_SETRVAL))
                return false;
        }
    } else {
        if (!emit1(JSOP_RETURN))
            return false;
    }

    NonLocalExitScope nle(cx, this);

    if (!nle.prepareForNonLocalJump(nullptr))
        return false;

    if (isGenerator) {
        ScopeCoordinate sc;
        
        
        sc.setHops(0);
        if (useGenRVal) {
            MOZ_ALWAYS_TRUE(LookupAliasedNameSlot(this, script, cx->names().dotGenRVal, &sc));
            if (!emitAliasedVarOp(JSOP_GETALIASEDVAR, sc, DontCheckLexical))
                return false;
            if (!emit1(JSOP_SETRVAL))
                return false;
        }

        MOZ_ALWAYS_TRUE(LookupAliasedNameSlot(this, script, cx->names().dotGenerator, &sc));
        if (!emitAliasedVarOp(JSOP_GETALIASEDVAR, sc, DontCheckLexical))
            return false;
        if (!emitYieldOp(JSOP_FINALYIELDRVAL))
            return false;
    } else if (top + static_cast<ptrdiff_t>(JSOP_RETURN_LENGTH) != offset()) {
        code()[top] = JSOP_SETRVAL;
        if (!emit1(JSOP_RETRVAL))
            return false;
    }

    return true;
}

bool
BytecodeEmitter::emitYield(ParseNode *pn)
{
    MOZ_ASSERT(sc->isFunctionBox());

    if (pn->getOp() == JSOP_YIELD) {
        if (sc->asFunctionBox()->isStarGenerator()) {
            if (!emitPrepareIteratorResult())
                return false;
        }
        if (pn->pn_left) {
            if (!emitTree(pn->pn_left))
                return false;
        } else {
            if (!emit1(JSOP_UNDEFINED))
                return false;
        }
        if (sc->asFunctionBox()->isStarGenerator()) {
            if (!emitFinishIteratorResult(false))
                return false;
        }
    } else {
        MOZ_ASSERT(pn->getOp() == JSOP_INITIALYIELD);
    }

    if (!emitTree(pn->pn_right))
        return false;

    if (!emitYieldOp(pn->getOp()))
        return false;

    if (pn->getOp() == JSOP_INITIALYIELD && !emit1(JSOP_POP))
        return false;

    return true;
}

bool
BytecodeEmitter::emitYieldStar(ParseNode *iter, ParseNode *gen)
{
    MOZ_ASSERT(sc->isFunctionBox());
    MOZ_ASSERT(sc->asFunctionBox()->isStarGenerator());

    if (!emitTree(iter))                                         
        return false;
    if (!emitIterator())                                         
        return false;

    
    if (!emit1(JSOP_UNDEFINED))                                  
        return false;

    int depth = stackDepth;
    MOZ_ASSERT(depth >= 2);

    ptrdiff_t initialSend = -1;
    if (!emitBackPatchOp(&initialSend))                          
        return false;

    
    StmtInfoBCE stmtInfo(cx);
    pushStatement(&stmtInfo, STMT_TRY, offset());
    ptrdiff_t noteIndex = NewSrcNote(cx, this, SRC_TRY);
    ptrdiff_t tryStart = offset();                               
    if (noteIndex < 0 || !emit1(JSOP_TRY))
        return false;
    MOZ_ASSERT(this->stackDepth == depth);

    
    if (!emitTree(gen))                                          
        return false;

    
    if (!emitYieldOp(JSOP_YIELD))                                
        return false;

    
    if (!setSrcNoteOffset(noteIndex, 0, offset() - tryStart))
        return false;
    ptrdiff_t subsequentSend = -1;
    if (!emitBackPatchOp(&subsequentSend))                       
        return false;
    ptrdiff_t tryEnd = offset();                                 

    
    stackDepth = uint32_t(depth);                                
    if (!emit1(JSOP_POP))                                        
        return false;
    
    if (!emit1(JSOP_EXCEPTION))                                  
        return false;
    if (!emit1(JSOP_SWAP))                                       
        return false;
    if (!emit1(JSOP_DUP))                                        
        return false;
    if (!emitAtomOp(cx->names().throw_, JSOP_STRING))            
        return false;
    if (!emit1(JSOP_SWAP))                                       
        return false;
    if (!emit1(JSOP_IN))                                         
        return false;
    
    ptrdiff_t checkThrow = emitJump(JSOP_IFNE, 0);               
    if (checkThrow < 0)
        return false;
    if (!emit1(JSOP_POP))                                        
        return false;
    if (!emit1(JSOP_THROW))                                      
        return false;

    setJumpOffsetAt(checkThrow);                                 
    
    stackDepth = uint32_t(depth);
    if (!emit1(JSOP_DUP))                                        
        return false;
    if (!emit1(JSOP_DUP))                                        
        return false;
    if (!emitAtomOp(cx->names().throw_, JSOP_CALLPROP))          
        return false;
    if (!emit1(JSOP_SWAP))                                       
        return false;
    if (!emit2(JSOP_PICK, (jsbytecode)3))                        
        return false;
    if (!emitCall(JSOP_CALL, 1, iter))                           
        return false;
    checkTypeSet(JSOP_CALL);
    MOZ_ASSERT(this->stackDepth == depth);
    ptrdiff_t checkResult = -1;
    if (!emitBackPatchOp(&checkResult))                          
        return false;

    
    popStatement();

    
    if (!emit1(JSOP_NOP))
        return false;
    if (!tryNoteList.append(JSTRY_CATCH, depth, tryStart + JSOP_TRY_LENGTH, tryEnd))
        return false;

    
    backPatch(initialSend, code().end(), JSOP_GOTO);  
    backPatch(subsequentSend, code().end(), JSOP_GOTO); 

    
    
    if (!emit1(JSOP_SWAP))                                       
        return false;
    if (!emit1(JSOP_DUP))                                        
        return false;
    if (!emit1(JSOP_DUP))                                        
        return false;
    if (!emitAtomOp(cx->names().next, JSOP_CALLPROP))            
        return false;
    if (!emit1(JSOP_SWAP))                                       
        return false;
    if (!emit2(JSOP_PICK, (jsbytecode)3))                        
        return false;
    if (!emitCall(JSOP_CALL, 1, iter))                           
        return false;
    checkTypeSet(JSOP_CALL);
    MOZ_ASSERT(this->stackDepth == depth);

    backPatch(checkResult, code().end(), JSOP_GOTO);             

    
    if (!emit1(JSOP_DUP))                                        
        return false;
    if (!emitAtomOp(cx->names().done, JSOP_GETPROP))             
        return false;
    
    if (emitJump(JSOP_IFEQ, tryStart - offset()) < 0)            
        return false;

    
    if (!emit1(JSOP_SWAP))                                       
        return false;
    if (!emit1(JSOP_POP))                                        
        return false;
    if (!emitAtomOp(cx->names().value, JSOP_GETPROP))            
        return false;

    MOZ_ASSERT(this->stackDepth == depth - 1);

    return true;
}

bool
BytecodeEmitter::emitStatementList(ParseNode *pn, ptrdiff_t top)
{
    MOZ_ASSERT(pn->isArity(PN_LIST));

    StmtInfoBCE stmtInfo(cx);
    pushStatement(&stmtInfo, STMT_BLOCK, top);

    ParseNode *pnchild = pn->pn_head;

    if (pn->pn_xflags & PNX_DESTRUCT)
        pnchild = pnchild->pn_next;

    for (ParseNode *pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
        if (!emitTree(pn2))
            return false;
    }

    popStatement();
    return true;
}

bool
BytecodeEmitter::emitStatement(ParseNode *pn)
{
    MOZ_ASSERT(pn->isKind(PNK_SEMI));

    ParseNode *pn2 = pn->pn_kid;
    if (!pn2)
        return true;

    if (!updateSourceCoordNotes(pn->pn_pos.begin))
        return false;

    








    bool wantval = false;
    bool useful = false;
    if (sc->isFunctionBox())
        MOZ_ASSERT(!script->noScriptRval());
    else
        useful = wantval = !script->noScriptRval();

    
    if (!useful) {
        if (!CheckSideEffects(cx, this, pn2, &useful))
            return false;

        





        if (topStmt &&
            topStmt->type == STMT_LABEL &&
            topStmt->update >= offset())
        {
            useful = true;
        }
    }

    if (useful) {
        JSOp op = wantval ? JSOP_SETRVAL : JSOP_POP;
        MOZ_ASSERT_IF(pn2->isKind(PNK_ASSIGN), pn2->isOp(JSOP_NOP));
        if (!emitTree(pn2))
            return false;
        if (!emit1(op))
            return false;
    } else if (pn->isDirectivePrologueMember()) {
        
        
    } else {
        if (JSAtom *atom = pn->isStringExprStatement()) {
            
            
            
            
            
            const char *directive = nullptr;
            if (atom == cx->names().useStrict) {
                if (!sc->strictScript)
                    directive = js_useStrict_str;
            } else if (atom == cx->names().useAsm) {
                if (sc->isFunctionBox()) {
                    JSFunction *fun = sc->asFunctionBox()->function();
                    if (fun->isNative() && IsAsmJSModuleNative(fun->native()))
                        directive = js_useAsm_str;
                }
            }

            if (directive) {
                if (!reportStrictWarning(pn2, JSMSG_CONTRARY_NONDIRECTIVE, directive))
                    return false;
            }
        } else {
            current->currentLine = parser->tokenStream.srcCoords.lineNum(pn2->pn_pos.begin);
            current->lastColumn = 0;
            if (!reportStrictWarning(pn2, JSMSG_USELESS_EXPR))
                return false;
        }
    }

    return true;
}

bool
BytecodeEmitter::emitDelete(ParseNode *pn)
{
    



    ParseNode *pn2 = pn->pn_kid;
    switch (pn2->getKind()) {
      case PNK_NAME:
        if (!bindNameToSlot(pn2))
            return false;
        if (!emitAtomOp(pn2, pn2->getOp()))
            return false;
        break;
      case PNK_DOT:
      {
        JSOp delOp = sc->strict() ? JSOP_STRICTDELPROP : JSOP_DELPROP;
        if (!emitPropOp(pn2, delOp))
            return false;
        break;
      }
      case PNK_ELEM:
      {
        JSOp delOp = sc->strict() ? JSOP_STRICTDELELEM : JSOP_DELELEM;
        if (!emitElemOp(pn2, delOp))
            return false;
        break;
      }
      default:
      {
        



        bool useful = false;
        if (!CheckSideEffects(cx, this, pn2, &useful))
            return false;

        if (useful) {
            MOZ_ASSERT_IF(pn2->isKind(PNK_CALL), !(pn2->pn_xflags & PNX_SETCALL));
            if (!emitTree(pn2))
                return false;
            if (!emit1(JSOP_POP))
                return false;
        }

        if (!emit1(JSOP_TRUE))
            return false;
      }
    }

    return true;
}

bool
BytecodeEmitter::emitSelfHostedCallFunction(ParseNode *pn)
{
    
    
    
    
    
    
    
    
    
    if (pn->pn_count < 3) {
        reportError(pn, JSMSG_MORE_ARGS_NEEDED, "callFunction", "1", "s");
        return false;
    }

    ParseNode *pn2 = pn->pn_head;
    ParseNode *funNode = pn2->pn_next;
    if (!emitTree(funNode))
        return false;

    ParseNode *thisArg = funNode->pn_next;
    if (!emitTree(thisArg))
        return false;

    bool oldEmittingForInit = emittingForInit;
    emittingForInit = false;

    for (ParseNode *argpn = thisArg->pn_next; argpn; argpn = argpn->pn_next) {
        if (!emitTree(argpn))
            return false;
    }

    emittingForInit = oldEmittingForInit;

    uint32_t argc = pn->pn_count - 3;
    if (!emitCall(pn->getOp(), argc))
        return false;

    checkTypeSet(pn->getOp());
    return true;
}

bool
BytecodeEmitter::emitSelfHostedResumeGenerator(ParseNode *pn)
{
    
    if (pn->pn_count != 4) {
        reportError(pn, JSMSG_MORE_ARGS_NEEDED, "resumeGenerator", "1", "s");
        return false;
    }

    ParseNode *funNode = pn->pn_head;  

    ParseNode *genNode = funNode->pn_next;
    if (!emitTree(genNode))
        return false;

    ParseNode *valNode = genNode->pn_next;
    if (!emitTree(valNode))
        return false;

    ParseNode *kindNode = valNode->pn_next;
    MOZ_ASSERT(kindNode->isKind(PNK_STRING));
    uint16_t operand = GeneratorObject::getResumeKind(cx, kindNode->pn_atom);
    MOZ_ASSERT(!kindNode->pn_next);

    if (!emitCall(JSOP_RESUME, operand))
        return false;

    return true;
}

bool
BytecodeEmitter::emitSelfHostedForceInterpreter(ParseNode *pn)
{
    if (!emit1(JSOP_FORCEINTERPRETER))
        return false;
    if (!emit1(JSOP_UNDEFINED))
        return false;
    return true;
}

bool
BytecodeEmitter::emitCallOrNew(ParseNode *pn)
{
    bool callop = pn->isKind(PNK_CALL) || pn->isKind(PNK_TAGGED_TEMPLATE);
    














    uint32_t argc = pn->pn_count - 1;

    if (argc >= ARGC_LIMIT) {
        parser->tokenStream.reportError(callop
                                        ? JSMSG_TOO_MANY_FUN_ARGS
                                        : JSMSG_TOO_MANY_CON_ARGS);
        return false;
    }

    ParseNode *pn2 = pn->pn_head;
    bool spread = JOF_OPTYPE(pn->getOp()) == JOF_BYTE;
    switch (pn2->getKind()) {
      case PNK_NAME:
        if (emitterMode == BytecodeEmitter::SelfHosting && !spread) {
            
            MOZ_ASSERT(!(pn->pn_xflags & PNX_SETCALL));

            
            
            if (pn2->name() == cx->names().callFunction)
                return emitSelfHostedCallFunction(pn);
            if (pn2->name() == cx->names().resumeGenerator)
                return emitSelfHostedResumeGenerator(pn);
            if (pn2->name() == cx->names().forceInterpreter)
                return emitSelfHostedForceInterpreter(pn);
            
        }
        if (!emitNameOp(pn2, callop))
            return false;
        break;
      case PNK_DOT:
        if (!emitPropOp(pn2, callop ? JSOP_CALLPROP : JSOP_GETPROP))
            return false;
        break;
      case PNK_ELEM:
        if (!emitElemOp(pn2, callop ? JSOP_CALLELEM : JSOP_GETELEM))
            return false;
        if (callop) {
            if (!emit1(JSOP_SWAP))
                return false;
        }
        break;
      case PNK_FUNCTION:
        









        MOZ_ASSERT(!emittingRunOnceLambda);
        if (checkSingletonContext() || (!isInLoop() && isRunOnceLambda())) {
            emittingRunOnceLambda = true;
            if (!emitTree(pn2))
                return false;
            emittingRunOnceLambda = false;
        } else {
            if (!emitTree(pn2))
                return false;
        }
        callop = false;
        break;
      default:
        if (!emitTree(pn2))
            return false;
        callop = false;             
        break;
    }
    if (!callop) {
        JSOp thisop = pn->isKind(PNK_GENEXP) ? JSOP_THIS : JSOP_UNDEFINED;
        if (!emit1(thisop))
            return false;
    }

    




    bool oldEmittingForInit = emittingForInit;
    emittingForInit = false;
    if (!spread) {
        for (ParseNode *pn3 = pn2->pn_next; pn3; pn3 = pn3->pn_next) {
            if (!emitTree(pn3))
                return false;
        }
    } else {
        if (!emitArray(pn2->pn_next, argc))
            return false;
    }
    emittingForInit = oldEmittingForInit;

    if (!spread) {
        if (!emitCall(pn->getOp(), argc, pn))
            return false;
    } else {
        if (!emit1(pn->getOp()))
            return false;
    }
    checkTypeSet(pn->getOp());
    if (pn->isOp(JSOP_EVAL) ||
        pn->isOp(JSOP_STRICTEVAL) ||
        pn->isOp(JSOP_SPREADEVAL) ||
        pn->isOp(JSOP_STRICTSPREADEVAL))
    {
        uint32_t lineNum = parser->tokenStream.srcCoords.lineNum(pn->pn_pos.begin);
        if (!emitUint16Operand(JSOP_LINENO, lineNum))
            return false;
    }
    if (pn->pn_xflags & PNX_SETCALL) {
        if (!emit1(JSOP_SETCALL))
            return false;
    }
    return true;
}

bool
BytecodeEmitter::emitLogical(ParseNode *pn)
{
    MOZ_ASSERT(pn->isArity(PN_LIST));

    









    
    ParseNode *pn2 = pn->pn_head;
    if (!emitTree(pn2))
        return false;
    ptrdiff_t top = emitJump(JSOP_BACKPATCH, 0);
    if (top < 0)
        return false;
    if (!emit1(JSOP_POP))
        return false;

    
    ptrdiff_t jmp = top;
    while ((pn2 = pn2->pn_next)->pn_next) {
        if (!emitTree(pn2))
            return false;
        ptrdiff_t off = emitJump(JSOP_BACKPATCH, 0);
        if (off < 0)
            return false;
        if (!emit1(JSOP_POP))
            return false;
        SET_JUMP_OFFSET(code(jmp), off - jmp);
        jmp = off;
    }
    if (!emitTree(pn2))
        return false;

    pn2 = pn->pn_head;
    ptrdiff_t off = offset();
    do {
        jsbytecode *pc = code(top);
        ptrdiff_t tmp = GET_JUMP_OFFSET(pc);
        SET_JUMP_OFFSET(pc, off - top);
        *pc = pn->getOp();
        top += tmp;
    } while ((pn2 = pn2->pn_next)->pn_next);

    return true;
}



MOZ_NEVER_INLINE bool
BytecodeEmitter::emitIncOrDec(ParseNode *pn)
{
    
    ParseNode *pn2 = pn->pn_kid;
    switch (pn2->getKind()) {
      case PNK_DOT:
        if (!emitPropIncDec(pn))
            return false;
        break;
      case PNK_ELEM:
        if (!emitElemIncDec(pn))
            return false;
        break;
      case PNK_CALL:
        MOZ_ASSERT(pn2->pn_xflags & PNX_SETCALL);
        if (!emitTree(pn2))
            return false;
        break;
      default:
        MOZ_ASSERT(pn2->isKind(PNK_NAME));
        pn2->setOp(JSOP_SETNAME);
        if (!bindNameToSlot(pn2))
            return false;
        JSOp op = pn2->getOp();
        bool maySet;
        switch (op) {
          case JSOP_SETLOCAL:
          case JSOP_SETARG:
          case JSOP_SETALIASEDVAR:
          case JSOP_SETNAME:
          case JSOP_STRICTSETNAME:
          case JSOP_SETGNAME:
          case JSOP_STRICTSETGNAME:
            maySet = true;
            break;
          default:
            maySet = false;
        }
        if (op == JSOP_CALLEE) {
            if (!emit1(op))
                return false;
        } else if (!pn2->pn_cookie.isFree()) {
            if (maySet) {
                if (!emitVarIncDec(pn))
                    return false;
            } else {
                if (!emitVarOp(pn2, op))
                    return false;
            }
        } else {
            MOZ_ASSERT(JOF_OPTYPE(op) == JOF_ATOM);
            if (maySet) {
                if (!emitNameIncDec(pn))
                    return false;
            } else {
                if (!emitAtomOp(pn2, op))
                    return false;
            }
            break;
        }
        if (pn2->isConst()) {
            if (!emit1(JSOP_POS))
                return false;
            bool post;
            JSOp binop = GetIncDecInfo(pn->getKind(), &post);
            if (!post) {
                if (!emit1(JSOP_ONE))
                    return false;
                if (!emit1(binop))
                    return false;
            }
        }
    }
    return true;
}





MOZ_NEVER_INLINE static bool
EmitLabeledStatement(ExclusiveContext *cx, BytecodeEmitter *bce, const LabeledStatement *pn)
{
    



    jsatomid index;
    if (!bce->makeAtomIndex(pn->label(), &index))
        return false;

    ptrdiff_t top = bce->emitJump(JSOP_LABEL, 0);
    if (top < 0)
        return false;

    
    StmtInfoBCE stmtInfo(cx);
    bce->pushStatement(&stmtInfo, STMT_LABEL, bce->offset());
    stmtInfo.label = pn->label();
    if (!bce->emitTree(pn->statement()))
        return false;
    bce->popStatement();

    
    bce->setJumpOffsetAt(top);
    return true;
}

static bool
EmitSyntheticStatements(ExclusiveContext *cx, BytecodeEmitter *bce, ParseNode *pn, ptrdiff_t top)
{
    MOZ_ASSERT(pn->isArity(PN_LIST));
    StmtInfoBCE stmtInfo(cx);
    bce->pushStatement(&stmtInfo, STMT_SEQ, top);
    ParseNode *pn2 = pn->pn_head;
    if (pn->pn_xflags & PNX_DESTRUCT)
        pn2 = pn2->pn_next;
    for (; pn2; pn2 = pn2->pn_next) {
        if (!bce->emitTree(pn2))
            return false;
    }
    bce->popStatement();
    return true;
}

bool
BytecodeEmitter::emitConditionalExpression(ConditionalExpression &conditional)
{
    
    if (!emitTree(&conditional.condition()))
        return false;

    ptrdiff_t noteIndex = NewSrcNote(cx, this, SRC_COND);
    if (noteIndex < 0)
        return false;

    ptrdiff_t beq = emitJump(JSOP_IFEQ, 0);
    if (beq < 0 || !emitTree(&conditional.thenExpression()))
        return false;

    
    ptrdiff_t jmp = emitJump(JSOP_GOTO, 0);
    if (jmp < 0)
        return false;
    setJumpOffsetAt(beq);

    









    MOZ_ASSERT(stackDepth > 0);
    stackDepth--;
    if (!emitTree(&conditional.elseExpression()))
        return false;
    setJumpOffsetAt(jmp);
    return setSrcNoteOffset(noteIndex, 0, jmp - beq);
}

bool
BytecodeEmitter::emitPropertyList(ParseNode *pn, MutableHandlePlainObject objp, PropListType type)
{
    for (ParseNode *propdef = pn->pn_head; propdef; propdef = propdef->pn_next) {
        if (!updateSourceCoordNotes(propdef->pn_pos.begin))
            return false;

        
        
        if (propdef->isKind(PNK_MUTATEPROTO)) {
            MOZ_ASSERT(type == ObjectLiteral);
            if (!emitTree(propdef->pn_kid))
                return false;
            objp.set(nullptr);
            if (!emit1(JSOP_MUTATEPROTO))
                return false;
            continue;
        }

        bool extraPop = false;
        if (type == ClassBody && propdef->as<ClassMethod>().isStatic()) {
            extraPop = true;
            if (!emit1(JSOP_DUP2))
                return false;
            if (!emit1(JSOP_POP))
                return false;
        }

        
        ParseNode *key = propdef->pn_left;
        bool isIndex = false;
        if (key->isKind(PNK_NUMBER)) {
            if (!emitNumberOp(key->pn_dval))
                return false;
            isIndex = true;
        } else if (key->isKind(PNK_OBJECT_PROPERTY_NAME) || key->isKind(PNK_STRING)) {
            
            if (type == ClassBody && key->pn_atom == cx->names().constructor)
                continue;

            
            
            
            jsid id = NameToId(key->pn_atom->asPropertyName());
            if (id != IdToTypeId(id)) {
                if (!emitTree(key))
                    return false;
                isIndex = true;
            }
        } else {
            MOZ_ASSERT(key->isKind(PNK_COMPUTED_NAME));
            if (!emitTree(key->pn_kid))
                return false;
            isIndex = true;
        }

        
        if (!emitTree(propdef->pn_right))
            return false;

        JSOp op = propdef->getOp();
        MOZ_ASSERT(op == JSOP_INITPROP ||
                   op == JSOP_INITPROP_GETTER ||
                   op == JSOP_INITPROP_SETTER);

        if (op == JSOP_INITPROP_GETTER || op == JSOP_INITPROP_SETTER)
            objp.set(nullptr);

        if (isIndex) {
            objp.set(nullptr);
            switch (op) {
              case JSOP_INITPROP:        op = JSOP_INITELEM;        break;
              case JSOP_INITPROP_GETTER: op = JSOP_INITELEM_GETTER; break;
              case JSOP_INITPROP_SETTER: op = JSOP_INITELEM_SETTER; break;
              default: MOZ_CRASH("Invalid op");
            }
            if (!emit1(op))
                return false;
        } else {
            MOZ_ASSERT(key->isKind(PNK_OBJECT_PROPERTY_NAME) || key->isKind(PNK_STRING));

            jsatomid index;
            if (!makeAtomIndex(key->pn_atom, &index))
                return false;

            if (objp) {
                MOZ_ASSERT(!objp->inDictionaryMode());
                Rooted<jsid> id(cx, AtomToId(key->pn_atom));
                RootedValue undefinedValue(cx, UndefinedValue());
                if (!NativeDefineProperty(cx, objp, id, undefinedValue, nullptr, nullptr,
                                          JSPROP_ENUMERATE))
                {
                    return false;
                }
                if (objp->inDictionaryMode())
                    objp.set(nullptr);
            }

            if (!emitIndex32(op, index))
                return false;
        }

        if (extraPop) {
            if (!emit1(JSOP_POP))
                return false;
        }
    }
    return true;
}



MOZ_NEVER_INLINE bool
BytecodeEmitter::emitObject(ParseNode *pn)
{
    if (!(pn->pn_xflags & PNX_NONCONST) && pn->pn_head && checkSingletonContext())
        return emitSingletonInitialiser(pn);

    




    ptrdiff_t offset = this->offset();
    if (!emitNewInit(JSProto_Object))
        return false;

    



    RootedPlainObject obj(cx);
    
    
    gc::AllocKind kind = gc::GetGCObjectKind(pn->pn_count);
    obj = NewBuiltinClassInstance<PlainObject>(cx, kind, TenuredObject);
    if (!obj)
        return false;

    if (!emitPropertyList(pn, &obj, ObjectLiteral))
        return false;

    if (obj) {
        



        ObjectBox *objbox = parser->newObjectBox(obj);
        if (!objbox)
            return false;

        static_assert(JSOP_NEWINIT_LENGTH == JSOP_NEWOBJECT_LENGTH,
                      "newinit and newobject must have equal length to edit in-place");

        uint32_t index = objectList.add(objbox);
        jsbytecode *code = this->code(offset);
        code[0] = JSOP_NEWOBJECT;
        code[1] = jsbytecode(index >> 24);
        code[2] = jsbytecode(index >> 16);
        code[3] = jsbytecode(index >> 8);
        code[4] = jsbytecode(index);
    }

    return true;
}

bool
BytecodeEmitter::emitArrayComp(ParseNode *pn)
{
    if (!emitNewInit(JSProto_Array))
        return false;

    




    MOZ_ASSERT(stackDepth > 0);
    uint32_t saveDepth = arrayCompDepth;
    arrayCompDepth = (uint32_t) (stackDepth - 1);
    if (!emitTree(pn->pn_head))
        return false;
    arrayCompDepth = saveDepth;

    return true;
}

bool
BytecodeEmitter::emitSpread()
{
    return emitForOf(STMT_SPREAD, nullptr, -1);
}

bool
BytecodeEmitter::emitArray(ParseNode *pn, uint32_t count)
{
    








    int32_t nspread = 0;
    for (ParseNode *elt = pn; elt; elt = elt->pn_next) {
        if (elt->isKind(PNK_SPREAD))
            nspread++;
    }

    ptrdiff_t off = emitN(JSOP_NEWARRAY, 3);                        
    if (off < 0)
        return false;
    checkTypeSet(JSOP_NEWARRAY);
    jsbytecode *pc = code(off);

    
    
    SET_UINT24(pc, count - nspread);

    ParseNode *pn2 = pn;
    jsatomid atomIndex;
    bool afterSpread = false;
    for (atomIndex = 0; pn2; atomIndex++, pn2 = pn2->pn_next) {
        if (!afterSpread && pn2->isKind(PNK_SPREAD)) {
            afterSpread = true;
            if (!emitNumberOp(atomIndex))                           
                return false;
        }
        if (!updateSourceCoordNotes(pn2->pn_pos.begin))
            return false;
        if (pn2->isKind(PNK_ELISION)) {
            if (!emit1(JSOP_HOLE))
                return false;
        } else {
            ParseNode *expr = pn2->isKind(PNK_SPREAD) ? pn2->pn_kid : pn2;
            if (!emitTree(expr))                                         
                return false;
        }
        if (pn2->isKind(PNK_SPREAD)) {
            if (!emitIterator())                                         
                return false;
            if (!emit2(JSOP_PICK, (jsbytecode)2))                        
                return false;
            if (!emit2(JSOP_PICK, (jsbytecode)2))                        
                return false;
            if (!emitSpread())                                           
                return false;
        } else if (afterSpread) {
            if (!emit1(JSOP_INITELEM_INC))
                return false;
        } else {
            off = emitN(JSOP_INITELEM_ARRAY, 3);
            if (off < 0)
                return false;
            SET_UINT24(code(off), atomIndex);
        }
    }
    MOZ_ASSERT(atomIndex == count);
    if (afterSpread) {
        if (!emit1(JSOP_POP))                                            
            return false;
    }
    return true;
}

bool
BytecodeEmitter::emitUnary(ParseNode *pn)
{
    if (!updateSourceCoordNotes(pn->pn_pos.begin))
        return false;

    
    JSOp op = pn->getOp();
    ParseNode *pn2 = pn->pn_kid;

    if (op == JSOP_TYPEOF && !pn2->isKind(PNK_NAME))
        op = JSOP_TYPEOFEXPR;

    bool oldEmittingForInit = emittingForInit;
    emittingForInit = false;
    if (!emitTree(pn2))
        return false;

    emittingForInit = oldEmittingForInit;
    return emit1(op);
}

bool
BytecodeEmitter::emitDefaults(ParseNode *pn)
{
    MOZ_ASSERT(pn->isKind(PNK_ARGSBODY));

    ParseNode *arg, *pnlast = pn->last();
    for (arg = pn->pn_head; arg != pnlast; arg = arg->pn_next) {
        if (!(arg->pn_dflags & PND_DEFAULT))
            continue;
        if (!bindNameToSlot(arg))
            return false;
        if (!emitVarOp(arg, JSOP_GETARG))
            return false;
        if (!emit1(JSOP_UNDEFINED))
            return false;
        if (!emit1(JSOP_STRICTEQ))
            return false;
        
        if (NewSrcNote(cx, this, SRC_IF) < 0)
            return false;
        ptrdiff_t jump = emitJump(JSOP_IFEQ, 0);
        if (jump < 0)
            return false;
        if (!emitTree(arg->expr()))
            return false;
        if (!emitVarOp(arg, JSOP_SETARG))
            return false;
        if (!emit1(JSOP_POP))
            return false;
        SET_JUMP_OFFSET(code(jump), offset() - jump);
    }

    return true;
}


bool
BytecodeEmitter::emitLexicalInitialization(ParseNode *pn, JSOp globalDefOp)
{
    






    MOZ_ASSERT(pn->isKind(PNK_NAME));

    if (!bindNameToSlot(pn))
        return false;

    jsatomid atomIndex;
    if (!maybeEmitVarDecl(globalDefOp, pn, &atomIndex))
        return false;

    if (pn->getOp() != JSOP_INITLEXICAL) {
        bool global = IsGlobalOp(pn->getOp());
        if (!emitIndex32(global ? JSOP_BINDGNAME : JSOP_BINDNAME, atomIndex))
            return false;
        if (!emit1(JSOP_SWAP))
            return false;
    }

    if (!pn->pn_cookie.isFree()) {
        if (!emitVarOp(pn, pn->getOp()))
            return false;
    } else {
        if (!emitIndexOp(pn->getOp(), atomIndex))
            return false;
    }

    return true;
}



bool
BytecodeEmitter::emitClass(ParseNode *pn)
{
    ClassNode &classNode = pn->as<ClassNode>();

    ClassNames *names = classNode.names();

    ParseNode *heritageExpression = classNode.heritage();

    ParseNode *classMethods = classNode.methodList();
    ParseNode *constructor = nullptr;
    for (ParseNode *mn = classMethods->pn_head; mn; mn = mn->pn_next) {
        ClassMethod &method = mn->as<ClassMethod>();
        ParseNode &methodName = method.name();
        if (methodName.isKind(PNK_OBJECT_PROPERTY_NAME) &&
            methodName.pn_atom == cx->names().constructor)
        {
            constructor = &method.method();
            break;
        }
    }
    MOZ_ASSERT(constructor, "For now, no default constructors");

    bool savedStrictness = sc->setLocalStrictMode(true);

    StmtInfoBCE stmtInfo(cx);
    if (names) {
        if (!EnterBlockScope(cx, this, &stmtInfo, classNode.scopeObject(), JSOP_UNINITIALIZED))
            return false;
    }

    if (heritageExpression) {
        if (!emitTree(heritageExpression))
            return false;
        if (!emit1(JSOP_CLASSHERITAGE))
            return false;
    }

    if (!EmitFunc(cx, this, constructor, !!heritageExpression))
        return false;

    if (heritageExpression) {
        
        
        if (!emit1(JSOP_SWAP))
            return false;
        if (!emit1(JSOP_OBJWITHPROTO))
            return false;
    } else {
        if (!emitNewInit(JSProto_Object))
            return false;
    }

    if (!emit1(JSOP_DUP2))
        return false;
    if (!emitAtomOp(cx->names().prototype, JSOP_INITLOCKEDPROP))
        return false;
    if (!emitAtomOp(cx->names().constructor, JSOP_INITHIDDENPROP))
        return false;

    RootedPlainObject obj(cx);
    if (!emitPropertyList(classMethods, &obj, ClassBody))
        return false;

    if (!emit1(JSOP_POP))
        return false;

    if (names) {
        
        ParseNode *innerName = names->innerBinding();
        if (!emitLexicalInitialization(innerName, JSOP_DEFCONST))
            return false;

        if (!LeaveNestedScope(cx, this, &stmtInfo))
            return false;

        ParseNode *outerName = names->outerBinding();
        if (outerName) {
            if (!emitLexicalInitialization(outerName, JSOP_DEFVAR))
                return false;
            
            
            if (!emit1(JSOP_POP))
                return false;
        }
    }

    MOZ_ALWAYS_TRUE(sc->setLocalStrictMode(savedStrictness));

    return true;
}

bool
BytecodeEmitter::emitTree(ParseNode *pn)
{
    JS_CHECK_RECURSION(cx, return false);

    EmitLevelManager elm(this);

    bool ok = true;
    ptrdiff_t top = offset();
    pn->pn_offset = top;

    
    if (!updateLineNumberNotes(pn->pn_pos.begin))
        return false;

    switch (pn->getKind()) {
      case PNK_FUNCTION:
        ok = EmitFunc(cx, this, pn);
        break;

      case PNK_ARGSBODY:
      {
        RootedFunction fun(cx, sc->asFunctionBox()->function());
        ParseNode *pnlast = pn->last();

        
        
        
        
        ParseNode *pnchild = pnlast->pn_head;
        if (pnlast->pn_xflags & PNX_DESTRUCT) {
            
            
            MOZ_ASSERT(pnchild->isKind(PNK_SEMI));
            MOZ_ASSERT(pnchild->pn_kid->isKind(PNK_VAR) || pnchild->pn_kid->isKind(PNK_GLOBALCONST));
            if (!emitTree(pnchild))
                return false;
            pnchild = pnchild->pn_next;
        }
        bool hasDefaults = sc->asFunctionBox()->hasDefaults();
        if (hasDefaults) {
            ParseNode *rest = nullptr;
            bool restIsDefn = false;
            if (fun->hasRest()) {
                MOZ_ASSERT(!sc->asFunctionBox()->argumentsHasLocalBinding());

                
                
                
                
                
                
                rest = pn->pn_head;
                while (rest->pn_next != pnlast)
                    rest = rest->pn_next;
                restIsDefn = rest->isDefn();
                if (!emit1(JSOP_REST))
                    return false;
                checkTypeSet(JSOP_REST);

                
                
                if (restIsDefn) {
                    if (!emit1(JSOP_UNDEFINED))
                        return false;
                    if (!bindNameToSlot(rest))
                        return false;
                    if (!emitVarOp(rest, JSOP_SETARG))
                        return false;
                    if (!emit1(JSOP_POP))
                        return false;
                }
            }
            if (!emitDefaults(pn))
                return false;
            if (fun->hasRest()) {
                if (restIsDefn && !emitVarOp(rest, JSOP_SETARG))
                    return false;
                if (!emit1(JSOP_POP))
                    return false;
            }
        }
        for (ParseNode *pn2 = pn->pn_head; pn2 != pnlast; pn2 = pn2->pn_next) {
            
            
            if (!pn2->isDefn())
                continue;
            if (!bindNameToSlot(pn2))
                return false;
            if (pn2->pn_next == pnlast && fun->hasRest() && !hasDefaults) {
                
                MOZ_ASSERT(!sc->asFunctionBox()->argumentsHasLocalBinding());
                switchToProlog();
                if (!emit1(JSOP_REST))
                    return false;
                checkTypeSet(JSOP_REST);
                if (!emitVarOp(pn2, JSOP_SETARG))
                    return false;
                if (!emit1(JSOP_POP))
                    return false;
                switchToMain();
            }
        }
        if (pnlast->pn_xflags & PNX_FUNCDEFS) {
            
            
            
            
            
            
            
            
            
            for (ParseNode *pn2 = pnchild; pn2; pn2 = pn2->pn_next) {
                if (pn2->isKind(PNK_FUNCTION) && pn2->functionIsHoisted()) {
                    if (!emitTree(pn2))
                        return false;
                }
            }
        }
        ok = emitTree(pnlast);
        break;
      }

      case PNK_IF:
        ok = emitIf(pn);
        break;

      case PNK_SWITCH:
        ok = emitSwitch(pn);
        break;

      case PNK_WHILE:
        ok = emitWhile(pn, top);
        break;

      case PNK_DOWHILE:
        ok = emitDo(pn);
        break;

      case PNK_FOR:
        ok = emitFor(pn, top);
        break;

      case PNK_BREAK:
        ok = emitBreak(pn->as<BreakStatement>().label());
        break;

      case PNK_CONTINUE:
        ok = emitContinue(pn->as<ContinueStatement>().label());
        break;

      case PNK_WITH:
        ok = emitWith(pn);
        break;

      case PNK_TRY:
        if (!emitTry(pn))
            return false;
        break;

      case PNK_CATCH:
        if (!emitCatch(pn))
            return false;
        break;

      case PNK_VAR:
      case PNK_GLOBALCONST:
        if (!emitVariables(pn, InitializeVars))
            return false;
        break;

      case PNK_RETURN:
        ok = emitReturn(pn);
        break;

      case PNK_YIELD_STAR:
        ok = emitYieldStar(pn->pn_left, pn->pn_right);
        break;

      case PNK_GENERATOR:
        if (!emit1(JSOP_GENERATOR))
            return false;
        break;

      case PNK_YIELD:
        ok = emitYield(pn);
        break;

      case PNK_STATEMENTLIST:
        ok = emitStatementList(pn, top);
        break;

      case PNK_SEQ:
        ok = EmitSyntheticStatements(cx, this, pn, top);
        break;

      case PNK_SEMI:
        ok = emitStatement(pn);
        break;

      case PNK_LABEL:
        ok = EmitLabeledStatement(cx, this, &pn->as<LabeledStatement>());
        break;

      case PNK_COMMA:
      {
        for (ParseNode *pn2 = pn->pn_head; ; pn2 = pn2->pn_next) {
            if (!updateSourceCoordNotes(pn2->pn_pos.begin))
                return false;
            if (!emitTree(pn2))
                return false;
            if (!pn2->pn_next)
                break;
            if (!emit1(JSOP_POP))
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
        if (!emitAssignment(pn->pn_left, pn->getOp(), pn->pn_right))
            return false;
        break;

      case PNK_CONDITIONAL:
        ok = emitConditionalExpression(pn->as<ConditionalExpression>());
        break;

      case PNK_OR:
      case PNK_AND:
        ok = emitLogical(pn);
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
      case PNK_MOD: {
        MOZ_ASSERT(pn->isArity(PN_LIST));
        
        ParseNode *subexpr = pn->pn_head;
        if (!emitTree(subexpr))
            return false;
        JSOp op = pn->getOp();
        while ((subexpr = subexpr->pn_next) != nullptr) {
            if (!emitTree(subexpr))
                return false;
            if (!emit1(op))
                return false;
        }
        break;
      }

      case PNK_THROW:
      case PNK_TYPEOF:
      case PNK_VOID:
      case PNK_NOT:
      case PNK_BITNOT:
      case PNK_POS:
      case PNK_NEG:
        ok = emitUnary(pn);
        break;

      case PNK_PREINCREMENT:
      case PNK_PREDECREMENT:
      case PNK_POSTINCREMENT:
      case PNK_POSTDECREMENT:
        ok = emitIncOrDec(pn);
        break;

      case PNK_DELETE:
        ok = emitDelete(pn);
        break;

      case PNK_DOT:
        ok = emitPropOp(pn, JSOP_GETPROP);
        break;

      case PNK_ELEM:
        ok = emitElemOp(pn, JSOP_GETELEM);
        break;

      case PNK_NEW:
      case PNK_TAGGED_TEMPLATE:
      case PNK_CALL:
      case PNK_GENEXP:
        ok = emitCallOrNew(pn);
        break;

      case PNK_LEXICALSCOPE:
        ok = EmitLexicalScope(cx, this, pn);
        break;

      case PNK_LETBLOCK:
      case PNK_LETEXPR:
        ok = EmitLet(cx, this, pn);
        break;

      case PNK_CONST:
      case PNK_LET:
        ok = emitVariables(pn, InitializeVars);
        break;

      case PNK_IMPORT:
      case PNK_EXPORT:
      case PNK_EXPORT_FROM:
       
       reportError(nullptr, JSMSG_MODULES_NOT_IMPLEMENTED);
       return false;

      case PNK_ARRAYPUSH: {
        





        if (!emitTree(pn->pn_kid))
            return false;
        if (!emitDupAt(arrayCompDepth))
            return false;
        if (!emit1(JSOP_ARRAYPUSH))
            return false;
        break;
      }

      case PNK_CALLSITEOBJ:
        ok = emitCallSiteObject(pn);
        break;

      case PNK_ARRAY:
        if (!(pn->pn_xflags & PNX_NONCONST) && pn->pn_head) {
            if (checkSingletonContext()) {
                
                ok = emitSingletonInitialiser(pn);
                break;
            }

            
            
            
            if (emitterMode != BytecodeEmitter::SelfHosting && pn->pn_count != 0) {
                RootedValue value(cx);
                if (!pn->getConstantValue(cx, ParseNode::DontAllowNestedObjects, &value))
                    return false;
                if (!value.isMagic(JS_GENERIC_MAGIC)) {
                    
                    
                    
                    
                    
                    
                    
                    NativeObject *obj = &value.toObject().as<NativeObject>();
                    if (!ObjectElements::MakeElementsCopyOnWrite(cx, obj))
                        return false;

                    ObjectBox *objbox = parser->newObjectBox(obj);
                    if (!objbox)
                        return false;

                    ok = emitObjectOp(objbox, JSOP_NEWARRAY_COPYONWRITE);
                    break;
                }
            }
        }

        ok = emitArray(pn->pn_head, pn->pn_count);
        break;

       case PNK_ARRAYCOMP:
        ok = emitArrayComp(pn);
        break;

      case PNK_OBJECT:
        ok = emitObject(pn);
        break;

      case PNK_NAME:
        if (!emitNameOp(pn, false))
            return false;
        break;

      case PNK_TEMPLATE_STRING_LIST:
        ok = emitTemplateString(pn);
        break;

      case PNK_TEMPLATE_STRING:
      case PNK_STRING:
        ok = emitAtomOp(pn, JSOP_STRING);
        break;

      case PNK_NUMBER:
        ok = emitNumberOp(pn->pn_dval);
        break;

      case PNK_REGEXP:
        ok = emitRegExp(regexpList.add(pn->as<RegExpLiteral>().objbox()));
        break;

      case PNK_TRUE:
      case PNK_FALSE:
      case PNK_THIS:
      case PNK_NULL:
        if (!emit1(pn->getOp()))
            return false;
        break;

      case PNK_DEBUGGER:
        if (!updateSourceCoordNotes(pn->pn_pos.begin))
            return false;
        if (!emit1(JSOP_DEBUGGER))
            return false;
        break;

      case PNK_NOP:
        MOZ_ASSERT(pn->getArity() == PN_NULLARY);
        break;

      case PNK_CLASS:
        ok = emitClass(pn);
        break;

      default:
        MOZ_ASSERT(0);
    }

    
    if (ok && emitLevel == 1) {
        if (!updateSourceCoordNotes(pn->pn_pos.end))
            return false;
    }

    return ok;
}

static int
AllocSrcNote(ExclusiveContext *cx, SrcNotesVector &notes)
{
    
    
    if (notes.capacity() == 0 && !notes.reserve(256))
        return -1;

    jssrcnote dummy = 0;
    if (!notes.append(dummy)) {
        ReportOutOfMemory(cx);
        return -1;
    }
    return notes.length() - 1;
}

int
frontend::NewSrcNote(ExclusiveContext *cx, BytecodeEmitter *bce, SrcNoteType type)
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
frontend::NewSrcNote2(ExclusiveContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset)
{
    int index;

    index = NewSrcNote(cx, bce, type);
    if (index >= 0) {
        if (!bce->setSrcNoteOffset(index, 0, offset))
            return -1;
    }
    return index;
}

int
frontend::NewSrcNote3(ExclusiveContext *cx, BytecodeEmitter *bce, SrcNoteType type, ptrdiff_t offset1,
            ptrdiff_t offset2)
{
    int index;

    index = NewSrcNote(cx, bce, type);
    if (index >= 0) {
        if (!bce->setSrcNoteOffset(index, 0, offset1))
            return -1;
        if (!bce->setSrcNoteOffset(index, 1, offset2))
            return -1;
    }
    return index;
}

bool
frontend::AddToSrcNoteDelta(ExclusiveContext *cx, BytecodeEmitter *bce, jssrcnote *sn, ptrdiff_t delta)
{
    



    MOZ_ASSERT(bce->current == &bce->main);
    MOZ_ASSERT((unsigned) delta < (unsigned) SN_XDELTA_LIMIT);

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

bool
BytecodeEmitter::setSrcNoteOffset(unsigned index, unsigned which, ptrdiff_t offset)
{
    if (!SN_REPRESENTABLE_OFFSET(offset)) {
        ReportStatementTooLarge(parser->tokenStream, topStmt);
        return false;
    }

    SrcNotesVector &notes = this->notes();

    
    jssrcnote *sn = notes.begin() + index;
    MOZ_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    MOZ_ASSERT((int) which < js_SrcNoteSpec[SN_TYPE(sn)].arity);
    for (sn++; which; sn++, which--) {
        if (*sn & SN_4BYTE_OFFSET_FLAG)
            sn += 3;
    }

    




    if (offset > (ptrdiff_t)SN_4BYTE_OFFSET_MASK || (*sn & SN_4BYTE_OFFSET_FLAG)) {
        
        if (!(*sn & SN_4BYTE_OFFSET_FLAG)) {
            
            jssrcnote dummy = 0;
            if (!(sn = notes.insert(sn, dummy)) ||
                !(sn = notes.insert(sn, dummy)) ||
                !(sn = notes.insert(sn, dummy)))
            {
                ReportOutOfMemory(cx);
                return false;
            }
        }
        *sn++ = (jssrcnote)(SN_4BYTE_OFFSET_FLAG | (offset >> 24));
        *sn++ = (jssrcnote)(offset >> 16);
        *sn++ = (jssrcnote)(offset >> 8);
    }
    *sn = (jssrcnote)offset;
    return true;
}





bool
frontend::FinishTakingSrcNotes(ExclusiveContext *cx, BytecodeEmitter *bce, uint32_t *out)
{
    MOZ_ASSERT(bce->current == &bce->main);

    unsigned prologCount = bce->prolog.notes.length();
    if (prologCount && bce->prolog.currentLine != bce->firstLine) {
        bce->switchToProlog();
        if (NewSrcNote2(cx, bce, SRC_SETLINE, (ptrdiff_t)bce->firstLine) < 0)
            return false;
        bce->switchToMain();
    } else {
        






        ptrdiff_t offset = bce->prologOffset() - bce->prolog.lastNoteOffset;
        MOZ_ASSERT(offset >= 0);
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

    
    
    
    *out = bce->prolog.notes.length() + bce->main.notes.length() + 1;
    return true;
}

void
frontend::CopySrcNotes(BytecodeEmitter *bce, jssrcnote *destination, uint32_t nsrcnotes)
{
    unsigned prologCount = bce->prolog.notes.length();
    unsigned mainCount = bce->main.notes.length();
    unsigned totalCount = prologCount + mainCount;
    MOZ_ASSERT(totalCount == nsrcnotes - 1);
    if (prologCount)
        PodCopy(destination, bce->prolog.notes.begin(), prologCount);
    PodCopy(destination + prologCount, bce->main.notes.begin(), mainCount);
    SN_MAKE_TERMINATOR(&destination[totalCount]);
}

void
CGConstList::finish(ConstArray *array)
{
    MOZ_ASSERT(length() == array->length);

    for (unsigned i = 0; i < length(); i++)
        array->vector[i] = list[i];
}











































unsigned
CGObjectList::add(ObjectBox *objbox)
{
    MOZ_ASSERT(!objbox->emitLink);
    objbox->emitLink = lastbox;
    lastbox = objbox;
    return length++;
}

unsigned
CGObjectList::indexOf(JSObject *obj)
{
    MOZ_ASSERT(length > 0);
    unsigned index = length - 1;
    for (ObjectBox *box = lastbox; box->object != obj; box = box->emitLink)
        index--;
    return index;
}

void
CGObjectList::finish(ObjectArray *array)
{
    MOZ_ASSERT(length <= INDEX_LIMIT);
    MOZ_ASSERT(length == array->length);

    js::HeapPtrObject *cursor = array->vector + array->length;
    ObjectBox *objbox = lastbox;
    do {
        --cursor;
        MOZ_ASSERT(!*cursor);
        *cursor = objbox->object;
    } while ((objbox = objbox->emitLink) != nullptr);
    MOZ_ASSERT(cursor == array->vector);
}

ObjectBox*
CGObjectList::find(uint32_t index)
{
    MOZ_ASSERT(index < length);
    ObjectBox *box = lastbox;
    for (unsigned n = length - 1; n > index; n--)
        box = box->emitLink;
    return box;
}

bool
CGTryNoteList::append(JSTryNoteKind kind, uint32_t stackDepth, size_t start, size_t end)
{
    MOZ_ASSERT(start <= end);
    MOZ_ASSERT(size_t(uint32_t(start)) == start);
    MOZ_ASSERT(size_t(uint32_t(end)) == end);

    JSTryNote note;
    note.kind = kind;
    note.stackDepth = stackDepth;
    note.start = uint32_t(start);
    note.length = uint32_t(end - start);

    return list.append(note);
}

void
CGTryNoteList::finish(TryNoteArray *array)
{
    MOZ_ASSERT(length() == array->length);

    for (unsigned i = 0; i < length(); i++)
        array->vector[i] = list[i];
}

bool
CGBlockScopeList::append(uint32_t scopeObject, uint32_t offset, uint32_t parent)
{
    BlockScopeNote note;
    mozilla::PodZero(&note);

    note.index = scopeObject;
    note.start = offset;
    note.parent = parent;

    return list.append(note);
}

uint32_t
CGBlockScopeList::findEnclosingScope(uint32_t index)
{
    MOZ_ASSERT(index < length());
    MOZ_ASSERT(list[index].index != BlockScopeNote::NoBlockScopeIndex);

    DebugOnly<uint32_t> pos = list[index].start;
    while (index--) {
        MOZ_ASSERT(list[index].start <= pos);
        if (list[index].length == 0) {
            
            
            
            return list[index].index;
        } else {
            
            
            MOZ_ASSERT(list[index].start + list[index].length <= pos);
        }
    }

    return BlockScopeNote::NoBlockScopeIndex;
}

void
CGBlockScopeList::recordEnd(uint32_t index, uint32_t offset)
{
    MOZ_ASSERT(index < length());
    MOZ_ASSERT(offset >= list[index].start);
    MOZ_ASSERT(list[index].length == 0);

    list[index].length = offset - list[index].start;
}

void
CGBlockScopeList::finish(BlockScopeArray *array)
{
    MOZ_ASSERT(length() == array->length);

    for (unsigned i = 0; i < length(); i++)
        array->vector[i] = list[i];
}

void
CGYieldOffsetList::finish(YieldOffsetArray &array, uint32_t prologLength)
{
    MOZ_ASSERT(length() == array.length());

    for (unsigned i = 0; i < length(); i++)
        array[i] = prologLength + list[i];
}





const JSSrcNoteSpec js_SrcNoteSpec[] = {
#define DEFINE_SRC_NOTE_SPEC(sym, name, arity) { name, arity },
    FOR_EACH_SRC_NOTE_TYPE(DEFINE_SRC_NOTE_SPEC)
#undef DEFINE_SRC_NOTE_SPEC
};

static int
SrcNoteArity(jssrcnote *sn)
{
    MOZ_ASSERT(SN_TYPE(sn) < SRC_LAST);
    return js_SrcNoteSpec[SN_TYPE(sn)].arity;
}

JS_FRIEND_API(unsigned)
js::SrcNoteLength(jssrcnote *sn)
{
    unsigned arity;
    jssrcnote *base;

    arity = SrcNoteArity(sn);
    for (base = sn++; arity; sn++, arity--) {
        if (*sn & SN_4BYTE_OFFSET_FLAG)
            sn += 3;
    }
    return sn - base;
}

JS_FRIEND_API(ptrdiff_t)
js::GetSrcNoteOffset(jssrcnote *sn, unsigned which)
{
    
    MOZ_ASSERT(SN_TYPE(sn) != SRC_XDELTA);
    MOZ_ASSERT((int) which < SrcNoteArity(sn));
    for (sn++; which; sn++, which--) {
        if (*sn & SN_4BYTE_OFFSET_FLAG)
            sn += 3;
    }
    if (*sn & SN_4BYTE_OFFSET_FLAG) {
        return (ptrdiff_t)(((uint32_t)(sn[0] & SN_4BYTE_OFFSET_MASK) << 24)
                           | (sn[1] << 16)
                           | (sn[2] << 8)
                           | sn[3]);
    }
    return (ptrdiff_t)*sn;
}
