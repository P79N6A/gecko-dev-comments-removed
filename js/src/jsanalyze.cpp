






































#include "jsanalyze.h"
#include "jsautooplen.h"
#include "jscompartment.h"
#include "jscntxt.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"

namespace js {
namespace analyze {





Script::Script()
{
    PodZero(this);
}

Script::~Script()
{
    JS_FinishArenaPool(&pool);
}





bool
Bytecode::mergeDefines(JSContext *cx, Script *script, bool initial,
                       unsigned newDepth, uint32 *newArray, unsigned newCount)
{
    if (initial) {
        



        stackDepth = newDepth;
        defineArray = newArray;
        defineCount = newCount;
        return true;
    }

    



    if (analyzed) {
#ifdef DEBUG
        









        JS_ASSERT(stackDepth == newDepth);
        for (unsigned i = 0; i < defineCount; i++) {
            bool found = false;
            for (unsigned j = 0; j < newCount; j++) {
                if (newArray[j] == defineArray[i])
                    found = true;
            }
            JS_ASSERT(found);
        }
#endif
    } else {
        JS_ASSERT(stackDepth == newDepth);
        bool owned = false;
        for (unsigned i = 0; i < defineCount; i++) {
            bool found = false;
            for (unsigned j = 0; j < newCount; j++) {
                if (newArray[j] == defineArray[i])
                    found = true;
            }
            if (!found) {
                




                if (!owned) {
                    uint32 *reallocArray = ArenaArray<uint32>(script->pool, defineCount);
                    if (!reallocArray) {
                        script->setOOM(cx);
                        return false;
                    }
                    memcpy(reallocArray, defineArray, defineCount * sizeof(uint32));
                    defineArray = reallocArray;
                    owned = true;
                }

                
                defineArray[i--] = defineArray[--defineCount];
            }
        }
    }

    return true;
}





inline bool
Script::addJump(JSContext *cx, unsigned offset,
                unsigned *currentOffset, unsigned *forwardJump,
                unsigned stackDepth, uint32 *defineArray, unsigned defineCount)
{
    JS_ASSERT(offset < script->length);

    Bytecode *&code = codeArray[offset];
    bool initial = (code == NULL);
    if (initial) {
        code = ArenaNew<Bytecode>(pool);
        if (!code) {
            setOOM(cx);
            return false;
        }
    }

    if (!code->mergeDefines(cx, this, initial, stackDepth, defineArray, defineCount))
        return false;
    code->jumpTarget = true;

    if (offset < *currentOffset) {
        
        isInlineable = false;

        
        if (!code->analyzed) {
            if (*forwardJump == 0)
                *forwardJump = *currentOffset;
            *currentOffset = offset;
        }
    } else if (offset > *forwardJump) {
        *forwardJump = offset;
    }

    return true;
}

inline void
Script::setLocal(uint32 local, uint32 offset)
{
    JS_ASSERT(local < localCount());
    JS_ASSERT(offset != LOCAL_CONDITIONALLY_DEFINED);

    












    JS_ASSERT(locals[local] == LOCAL_CONDITIONALLY_DEFINED ||
              locals[local] == offset || offset == LOCAL_USE_BEFORE_DEF);

    locals[local] = offset;
}

static inline ptrdiff_t
GetJumpOffset(jsbytecode *pc, jsbytecode *pc2)
{
    uint32 type = JOF_OPTYPE(*pc);
    if (JOF_TYPE_IS_EXTENDED_JUMP(type))
        return GET_JUMPX_OFFSET(pc2);
    return GET_JUMP_OFFSET(pc2);
}


static inline bool
BytecodeNoFallThrough(JSOp op)
{
    switch (op) {
      case JSOP_GOTO:
      case JSOP_GOTOX:
      case JSOP_DEFAULT:
      case JSOP_DEFAULTX:
      case JSOP_RETURN:
      case JSOP_STOP:
      case JSOP_RETRVAL:
      case JSOP_THROW:
      case JSOP_TABLESWITCH:
      case JSOP_TABLESWITCHX:
      case JSOP_LOOKUPSWITCH:
      case JSOP_LOOKUPSWITCHX:
      case JSOP_FILTER:
        return true;
      case JSOP_GOSUB:
      case JSOP_GOSUBX:
        
        return false;
      default:
        return false;
    }
}

void
Script::analyze(JSContext *cx, JSScript *script)
{
    JS_InitArenaPool(&pool, "script_analyze", 256, 8, NULL);

    JS_ASSERT(script && !codeArray && !locals);
    this->script = script;

    unsigned length = script->length;
    unsigned nargs = script->fun ? script->fun->nargs : 0;
    unsigned nfixed = localCount();

    codeArray = ArenaArray<Bytecode*>(pool, length);
    locals = ArenaArray<uint32>(pool, nfixed);
    closedArgs = ArenaArray<JSPackedBool>(pool, nargs);
    closedVars = ArenaArray<JSPackedBool>(pool, nfixed);

    if (!codeArray || !locals || !closedArgs || !closedVars) {
        setOOM(cx);
        return;
    }

    PodZero(codeArray, length);

    for (unsigned i = 0; i < nfixed; i++)
        locals[i] = LOCAL_CONDITIONALLY_DEFINED;

    PodZero(closedArgs, nargs);
    for (uint32 i = 0; i < script->nClosedArgs; i++) {
        unsigned arg = script->getClosedArg(i);
        JS_ASSERT(arg < nargs);
        closedArgs[arg] = true;
    }

    PodZero(closedVars, nfixed);
    for (uint32 i = 0; i < script->nClosedVars; i++) {
        unsigned local = script->getClosedVar(i);
        if (local < nfixed)
            closedVars[local] = true;
    }

    




    if (script->usesEval || cx->compartment->debugMode) {
        for (uint32 i = 0; i < nfixed; i++)
            setLocal(i, LOCAL_USE_BEFORE_DEF);
    }

    for (uint32 i = 0; i < script->nClosedVars; i++) {
        uint32 slot = script->getClosedVar(i);
        if (slot < nfixed)
            setLocal(slot, LOCAL_USE_BEFORE_DEF);
    }

    



    if (cx->compartment->debugMode)
        usesRval = true;

    isInlineable = true;
    if (script->nClosedArgs || script->nClosedVars || script->nfixed >= LOCAL_LIMIT ||
        (script->fun && script->fun->isHeavyweight()) ||
        script->usesEval || script->usesArguments || cx->compartment->debugMode) {
        isInlineable = false;
    }

    




    unsigned forwardJump = 0;

    



    unsigned forwardCatch = 0;

    
    Bytecode *startcode = ArenaNew<Bytecode>(pool);
    if (!startcode) {
        setOOM(cx);
        return;
    }

    startcode->stackDepth = 0;
    codeArray[0] = startcode;

    unsigned offset, nextOffset = 0;
    while (nextOffset < length) {
        offset = nextOffset;

        JS_ASSERT(forwardCatch <= forwardJump);

        
        if (forwardJump && forwardJump == offset)
            forwardJump = 0;
        if (forwardCatch && forwardCatch == offset)
            forwardCatch = 0;

        Bytecode *code = maybeCode(offset);
        jsbytecode *pc = script->code + offset;

        UntrapOpcode untrap(cx, script, pc);

        JSOp op = (JSOp)*pc;
        JS_ASSERT(op < JSOP_LIMIT);

        
        unsigned successorOffset = offset + GetBytecodeLength(pc);

        



        nextOffset = successorOffset;

        if (!code) {
            
            continue;
        }

        if (code->analyzed) {
            
            continue;
        }

        code->analyzed = true;

        if (forwardCatch)
            code->inTryBlock = true;

        if (untrap.trap) {
            code->safePoint = true;
            isInlineable = false;
        }

        unsigned stackDepth = code->stackDepth;
        uint32 *defineArray = code->defineArray;
        unsigned defineCount = code->defineCount;

        if (!forwardJump) {
            








            for (unsigned i = 0; i < defineCount; i++) {
                uint32 local = defineArray[i];
                JS_ASSERT_IF(locals[local] != LOCAL_CONDITIONALLY_DEFINED &&
                             locals[local] != LOCAL_USE_BEFORE_DEF,
                             locals[local] <= offset);
                if (locals[local] == LOCAL_CONDITIONALLY_DEFINED)
                    setLocal(local, offset);
            }
            defineArray = code->defineArray = NULL;
            defineCount = code->defineCount = 0;
        }

        unsigned nuses = GetUseCount(script, offset);
        unsigned ndefs = GetDefCount(script, offset);

        JS_ASSERT(stackDepth >= nuses);
        stackDepth -= nuses;
        stackDepth += ndefs;

        switch (op) {

          case JSOP_SETRVAL:
          case JSOP_POPV:
            usesRval = true;
            isInlineable = false;
            break;

          case JSOP_NAME:
          case JSOP_CALLNAME:
          case JSOP_BINDNAME:
          case JSOP_SETNAME:
          case JSOP_DELNAME:
          case JSOP_INCNAME:
          case JSOP_DECNAME:
          case JSOP_NAMEINC:
          case JSOP_NAMEDEC:
          case JSOP_FORNAME:
            usesScope = true;
            isInlineable = false;
            break;

          case JSOP_THIS:
            usesThis = true;
            break;

          case JSOP_CALL:
          case JSOP_NEW:
            
            hasCalls = true;
            break;

          case JSOP_TABLESWITCH:
          case JSOP_TABLESWITCHX: {
            isInlineable = false;
            jsbytecode *pc2 = pc;
            unsigned jmplen = (op == JSOP_TABLESWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
            unsigned defaultOffset = offset + GetJumpOffset(pc, pc2);
            pc2 += jmplen;
            jsint low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            jsint high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump,
                         stackDepth, defineArray, defineCount)) {
                return;
            }
            getCode(defaultOffset).switchTarget = true;
            getCode(defaultOffset).safePoint = true;

            for (jsint i = low; i <= high; i++) {
                unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
                if (targetOffset != offset) {
                    if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                                 stackDepth, defineArray, defineCount)) {
                        return;
                    }
                }
                getCode(targetOffset).switchTarget = true;
                getCode(targetOffset).safePoint = true;
                pc2 += jmplen;
            }
            break;
          }

          case JSOP_LOOKUPSWITCH:
          case JSOP_LOOKUPSWITCHX: {
            isInlineable = false;
            jsbytecode *pc2 = pc;
            unsigned jmplen = (op == JSOP_LOOKUPSWITCH) ? JUMP_OFFSET_LEN : JUMPX_OFFSET_LEN;
            unsigned defaultOffset = offset + GetJumpOffset(pc, pc2);
            pc2 += jmplen;
            unsigned npairs = GET_UINT16(pc2);
            pc2 += UINT16_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump,
                         stackDepth, defineArray, defineCount)) {
                return;
            }
            getCode(defaultOffset).switchTarget = true;
            getCode(defaultOffset).safePoint = true;

            while (npairs) {
                pc2 += INDEX_LEN;
                unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
                if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                             stackDepth, defineArray, defineCount)) {
                    return;
                }
                getCode(targetOffset).switchTarget = true;
                getCode(targetOffset).safePoint = true;
                pc2 += jmplen;
                npairs--;
            }
            break;
          }

          case JSOP_TRY: {
            





            isInlineable = false;
            JSTryNote *tn = script->trynotes()->vector;
            JSTryNote *tnlimit = tn + script->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script->main - script->code + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    
                    if (catchOffset > forwardCatch)
                        forwardCatch = catchOffset;

                    if (tn->kind != JSTRY_ITER) {
                        if (!addJump(cx, catchOffset, &nextOffset, &forwardJump,
                                     stackDepth, defineArray, defineCount)) {
                            return;
                        }
                        getCode(catchOffset).exceptionEntry = true;
                        getCode(catchOffset).safePoint = true;
                    }
                }
            }
            break;
          }

          case JSOP_GETLOCAL:
            




            if (pc[JSOP_GETLOCAL_LENGTH] != JSOP_POP) {
                uint32 local = GET_SLOTNO(pc);
                if (local < nfixed && !localDefined(local, offset)) {
                    setLocal(local, LOCAL_USE_BEFORE_DEF);
                    isInlineable = false;
                }
            }
            break;

          case JSOP_CALLLOCAL:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC: {
            uint32 local = GET_SLOTNO(pc);
            if (local < nfixed && !localDefined(local, offset)) {
                setLocal(local, LOCAL_USE_BEFORE_DEF);
                isInlineable = false;
            }
            break;
          }

          case JSOP_SETLOCAL:
          case JSOP_FORLOCAL: {
            uint32 local = GET_SLOTNO(pc);

            






            if (local < nfixed && locals[local] == LOCAL_CONDITIONALLY_DEFINED) {
                if (forwardJump) {
                    
                    uint32 *newArray = ArenaArray<uint32>(pool, defineCount + 1);
                    if (!newArray) {
                        setOOM(cx);
                        return;
                    }
                    if (defineCount)
                        memcpy(newArray, defineArray, defineCount * sizeof(uint32));
                    defineArray = newArray;
                    defineArray[defineCount++] = local;
                } else {
                    
                    setLocal(local, offset);
                }
            }
            break;
          }

          
          case JSOP_ARGUMENTS:
          case JSOP_EVAL:
          case JSOP_FORARG:
          case JSOP_SETARG:
          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC:
          case JSOP_THROW:
          case JSOP_EXCEPTION:
          case JSOP_DEFFUN:
          case JSOP_DEFVAR:
          case JSOP_DEFCONST:
          case JSOP_SETCONST:
          case JSOP_DEFLOCALFUN:
          case JSOP_DEFLOCALFUN_FC:
          case JSOP_LAMBDA:
          case JSOP_LAMBDA_FC:
          case JSOP_GETFCSLOT:
          case JSOP_CALLFCSLOT:
          case JSOP_ARGSUB:
          case JSOP_ARGCNT:
          case JSOP_DEBUGGER:
          case JSOP_ENTERBLOCK:
          case JSOP_LEAVEBLOCK:
          case JSOP_FUNCALL:
          case JSOP_FUNAPPLY:
            isInlineable = false;
            break;

          default:
            break;
        }

        uint32 type = JOF_TYPE(js_CodeSpec[op].format);

        
        if (type == JOF_JUMP || type == JOF_JUMPX) {
            
            unsigned newStackDepth = stackDepth;

            switch (op) {
              case JSOP_OR:
              case JSOP_AND:
              case JSOP_ORX:
              case JSOP_ANDX:
                




                stackDepth--;
                break;

              case JSOP_CASE:
              case JSOP_CASEX:
                
                newStackDepth--;
                break;

              default:;
            }

            unsigned targetOffset = offset + GetJumpOffset(pc, pc);
            if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                         newStackDepth, defineArray, defineCount)) {
                return;
            }
        }

        
        if (!BytecodeNoFallThrough(op)) {
            JS_ASSERT(successorOffset < script->length);

            Bytecode *&nextcode = codeArray[successorOffset];
            bool initial = (nextcode == NULL);

            if (initial) {
                nextcode = ArenaNew<Bytecode>(pool);
                if (!nextcode) {
                    setOOM(cx);
                    return;
                }
            }

            if (type == JOF_JUMP || type == JOF_JUMPX)
                nextcode->jumpFallthrough = true;

            if (!nextcode->mergeDefines(cx, this, initial, stackDepth,
                                        defineArray, defineCount)) {
                return;
            }

            
            if (type == JOF_JUMP || type == JOF_JUMPX)
                nextcode->jumpTarget = true;
            else
                nextcode->fallthrough = true;
        }
    }

    JS_ASSERT(!failed());
    JS_ASSERT(forwardJump == 0 && forwardCatch == 0);
}





bool
StackAnalysis::analyze(JSArenaPool &pool, JSScript *script,
                       uint32 start, uint32 length, Script *analysis)
{
    this->script = script;
    this->start = start;
    this->length = length;

    poppedArray = ArenaArray<PoppedValue*>(pool, length);
    if (!poppedArray)
        return false;
    PodZero(poppedArray, length);

    PoppedValue *stack = ArenaArray<PoppedValue>(pool, script->nslots - script->nfixed);
    if (!stack)
        return false;

    unsigned depth = analysis->getCode(start).stackDepth;
    for (unsigned i = 0; i < depth; i++)
        stack[i].reset();

    unsigned offset = start;
    while (offset < start + length) {
        jsbytecode *pc = script->code + offset;
        uint32 successorOffset = offset + GetBytecodeLength(pc);

        Bytecode *code = analysis->maybeCode(pc);
        if (!code) {
            offset = successorOffset;
            continue;
        }

        for (unsigned i = depth; i < code->stackDepth; i++)
            stack[i].reset();
        depth = code->stackDepth;

        if (code->jumpTarget) {
            for (unsigned i = 0; i < depth; i++)
                stack[i].reset();
        }

        unsigned nuses = GetUseCount(script, offset);
        unsigned ndefs = GetDefCount(script, offset);

        if (nuses) {
            PoppedValue *popped = ArenaArray<PoppedValue>(pool, nuses);
            if (!popped)
                return false;
            for (unsigned i = 0; i < nuses; i++)
                popped[i] = stack[depth - 1 - i];
            poppedArray[offset - start] = popped;
        }

        for (unsigned i = 0; i < ndefs; i++) {
            PoppedValue &value = stack[depth - nuses + i];
            value.offset = offset;
            value.which = i;
        }

        depth -= nuses;
        depth += ndefs;

        offset = successorOffset;
    }

    return true;
}





LifetimeScript::LifetimeScript()
{
    PodZero(this);
}

LifetimeScript::~LifetimeScript()
{
    JS_FinishArenaPool(&pool);
}

bool
LifetimeScript::analyze(JSContext *cx, analyze::Script *analysis, JSScript *script)
{
    JS_ASSERT(analysis->hasAnalyzed() && !analysis->failed());

    JS_InitArenaPool(&pool, "script_liverange", 256, 8, NULL);

    this->analysis = analysis;
    this->script = script;

    codeArray = ArenaArray<LifetimeBytecode>(pool, script->length);
    if (!codeArray)
        return false;
    PodZero(codeArray, script->length);

    unsigned nfixed = analysis->localCount();
    unsigned nargs = script->fun ? script->fun->nargs : 0;

    nLifetimes = 2 + nargs + nfixed;
    lifetimes = ArenaArray<LifetimeVariable>(pool, nLifetimes);
    if (!lifetimes)
        return false;
    PodZero(lifetimes, nLifetimes);

    LifetimeVariable *thisVar = lifetimes + 1;
    LifetimeVariable *args = lifetimes + 2;
    LifetimeVariable *locals = lifetimes + 2 + nargs;

    saved = ArenaArray<LifetimeVariable*>(pool, nLifetimes);
    if (!saved)
        return false;
    savedCount = 0;

    LifetimeLoop *loop = NULL;

    uint32 offset = script->length - 1;
    while (offset < script->length) {
        Bytecode *code = analysis->maybeCode(offset);
        if (!code) {
            offset--;
            continue;
        }

        if (loop && code->safePoint)
            loop->hasSafePoints = true;

        UntrapOpcode untrap(cx, script, script->code + offset);

        if (codeArray[offset].loop) {
            





            JS_ASSERT(loop == codeArray[offset].loop);
            unsigned backedge = codeArray[offset].loop->backedge;
            for (unsigned i = 0; i < nfixed; i++) {
                if (locals[i].lifetime && !extendVariable(cx, locals[i], offset, backedge))
                    return false;
            }
            for (unsigned i = 0; i < nargs; i++) {
                if (args[i].lifetime && !extendVariable(cx, args[i], offset, backedge))
                    return false;
            }

            loop = loop->parent;
            JS_ASSERT_IF(loop, loop->head < offset);
        }

        
        if (loop && code->jumpTarget && offset != loop->entry && offset > loop->lastBlock)
            loop->lastBlock = offset;

        jsbytecode *pc = script->code + offset;
        JSOp op = (JSOp) *pc;

        switch (op) {
          case JSOP_GETARG:
          case JSOP_CALLARG: {
            unsigned arg = GET_ARGNO(pc);
            if (!analysis->argEscapes(arg)) {
                if (!addVariable(cx, args[arg], offset))
                    return false;
            }
            break;
          }

          case JSOP_SETARG: {
            unsigned arg = GET_ARGNO(pc);
            if (!analysis->argEscapes(arg)) {
                if (!killVariable(cx, args[arg], offset))
                    return false;
            }
            break;
          }

          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC: {
            unsigned arg = GET_ARGNO(pc);
            if (!analysis->argEscapes(arg)) {
                if (!killVariable(cx, args[arg], offset))
                    return false;
                if (!addVariable(cx, args[arg], offset))
                    return false;
            }
            break;
          }

          case JSOP_GETLOCAL:
          case JSOP_CALLLOCAL: {
            unsigned local = GET_SLOTNO(pc);
            if (!analysis->localEscapes(local)) {
                JS_ASSERT(local < nfixed);
                if (!addVariable(cx, locals[local], offset))
                    return false;
            }
            break;
          }

          case JSOP_SETLOCAL:
          case JSOP_SETLOCALPOP:
          case JSOP_DEFLOCALFUN: {
            unsigned local = GET_SLOTNO(pc);
            if (!analysis->localEscapes(local)) {
                JS_ASSERT(local < nfixed);
                if (!killVariable(cx, locals[local], offset))
                    return false;
            }
            break;
          }

          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC: {
            unsigned local = GET_SLOTNO(pc);
            if (!analysis->localEscapes(local)) {
                if (!killVariable(cx, locals[local], offset))
                    return false;
                if (!addVariable(cx, locals[local], offset))
                    return false;
            }
            break;
          }

          case JSOP_THIS:
            if (!addVariable(cx, *thisVar, offset))
                return false;
            break;

          case JSOP_IFEQ:
          case JSOP_IFEQX:
          case JSOP_IFNE:
          case JSOP_IFNEX:
          case JSOP_OR:
          case JSOP_ORX:
          case JSOP_AND:
          case JSOP_ANDX:
          case JSOP_GOTO:
          case JSOP_GOTOX: {
            




            uint32 targetOffset = offset + GetJumpOffset(pc, pc);
            if (targetOffset < offset) {
                JSOp nop = JSOp(script->code[targetOffset]);
                if (nop == JSOP_GOTO || nop == JSOP_GOTOX) {
                    
                    jsbytecode *target = script->code + targetOffset;
                    targetOffset = targetOffset + GetJumpOffset(target, target);

                    




                    JS_ASSERT(loop);
                    if (loop->entry == targetOffset && loop->entry > loop->lastBlock)
                        loop->lastBlock = loop->entry;
                } else {
                    
                    JS_ASSERT(nop == JSOP_TRACE || nop == JSOP_NOTRACE);

                    




                    if (loop && loop->entry > loop->lastBlock)
                        loop->lastBlock = loop->entry;

                    LifetimeLoop *nloop = ArenaNew<LifetimeLoop>(pool);
                    if (!nloop)
                        return false;
                    PodZero(nloop);

                    if (loop)
                        loop->hasCallsLoops = true;

                    nloop->parent = loop;
                    loop = nloop;

                    codeArray[targetOffset].loop = loop;
                    loop->head = targetOffset;
                    loop->backedge = offset;
                    loop->lastBlock = loop->head;

                    



                    uint32 entry = targetOffset;
                    if (entry) {
                        do {
                            entry--;
                        } while (!analysis->maybeCode(entry));

                        jsbytecode *entrypc = script->code + entry;
                        if (JSOp(*entrypc) == JSOP_GOTO || JSOp(*entrypc) == JSOP_GOTOX)
                            loop->entry = entry + GetJumpOffset(entrypc, entrypc);
                        else
                            loop->entry = targetOffset;
                    } else {
                        
                        loop->entry = targetOffset;
                    }

                    break;
                }
            }
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                JS_ASSERT(!var.lifetime && var.saved);
                if (var.live(targetOffset)) {
                    



                    var.lifetime = ArenaNew<Lifetime>(pool, offset, var.savedEnd, var.saved);
                    if (!var.lifetime)
                        return false;
                    var.saved = NULL;
                    saved[i--] = saved[--savedCount];
                } else if (loop && !var.savedEnd) {
                    






                    var.savedEnd = offset;
                }
            }
            break;
          }

          case JSOP_LOOKUPSWITCH:
          case JSOP_LOOKUPSWITCHX:
          case JSOP_TABLESWITCH:
          case JSOP_TABLESWITCHX:
            
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                var.lifetime = ArenaNew<Lifetime>(pool, offset, var.savedEnd, var.saved);
                if (!var.lifetime)
                    return false;
                var.saved = NULL;
                saved[i--] = saved[--savedCount];
            }
            savedCount = 0;
            break;

          case JSOP_NEW:
          case JSOP_CALL:
          case JSOP_EVAL:
          case JSOP_FUNAPPLY:
          case JSOP_FUNCALL:
            if (loop)
                loop->hasCallsLoops = true;
            break;

          default:;
        }

        offset--;
    }

    return true;
}

#ifdef DEBUG
void
LifetimeScript::dumpVariable(LifetimeVariable &var)
{
    Lifetime *segment = var.lifetime ? var.lifetime : var.saved;
    while (segment) {
        printf(" (%u,%u%s)", segment->start, segment->end, segment->loopTail ? ",tail" : "");
        segment = segment->next;
    }
    printf("\n");
}
#endif 

inline bool
LifetimeScript::addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset)
{
    if (var.lifetime) {
        JS_ASSERT(offset < var.lifetime->start);
        var.lifetime->start = offset;
    } else {
        if (var.saved) {
            
            for (unsigned i = 0; i < savedCount; i++) {
                if (saved[i] == &var) {
                    JS_ASSERT(savedCount);
                    saved[i--] = saved[--savedCount];
                    break;
                }
            }
        }
        var.lifetime = ArenaNew<Lifetime>(pool, offset, var.savedEnd, var.saved);
        if (!var.lifetime)
            return false;
        var.saved = NULL;
    }
    return true;
}

inline bool
LifetimeScript::killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset)
{
    if (!var.lifetime) {
        
        if (!var.saved)
            saved[savedCount++] = &var;
        var.saved = ArenaNew<Lifetime>(pool, offset, var.savedEnd, var.saved);
        if (!var.saved)
            return false;
        var.saved->write = true;
        var.savedEnd = 0;
        return true;
    }
    JS_ASSERT(offset < var.lifetime->start);

    




    var.lifetime->start = offset;
    var.lifetime->write = true;

    var.saved = var.lifetime;
    var.savedEnd = 0;
    var.lifetime = NULL;

    saved[savedCount++] = &var;

    return true;
}

inline bool
LifetimeScript::extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end)
{
    JS_ASSERT(var.lifetime);
    var.lifetime->start = start;

    





















    Lifetime *segment = var.lifetime;
    while (segment && segment->start < end) {
        uint32 savedEnd = segment->savedEnd;
        if (!segment->next || segment->next->start >= end) {
            




            if (segment->end >= end) {
                
                break;
            }
            savedEnd = end;
        }
        JS_ASSERT(savedEnd <= end);
        if (savedEnd > segment->end) {
            Lifetime *tail = ArenaNew<Lifetime>(pool, savedEnd, 0, segment->next);
            if (!tail)
                return false;
            tail->start = segment->end;
            tail->loopTail = true;

            




            if (segment->savedEnd > end) {
                JS_ASSERT(savedEnd == end);
                tail->savedEnd = segment->savedEnd;
            }
            segment->savedEnd = 0;

            segment->next = tail;
            segment = tail->next;
        } else {
            JS_ASSERT(segment->savedEnd == 0);
            segment = segment->next;
        }
    }

    return true;
}

} 
} 
