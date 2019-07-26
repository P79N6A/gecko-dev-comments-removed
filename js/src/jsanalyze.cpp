





#include "jsanalyzeinlines.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/PodOperations.h"

#include "jsautooplen.h"
#include "jscntxt.h"
#include "jscompartment.h"

#include "jsinferinlines.h"

using namespace js;
using namespace js::analyze;

using mozilla::DebugOnly;
using mozilla::PodCopy;
using mozilla::PodZero;
using mozilla::FloorLog2;





#ifdef DEBUG
void
analyze::PrintBytecode(JSContext *cx, HandleScript script, jsbytecode *pc)
{
    fprintf(stderr, "#%u:", script->id());
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return;
    js_Disassemble1(cx, script, pc, pc - script->code, true, &sprinter);
    fprintf(stderr, "%s", sprinter.string());
}
#endif





inline bool
ScriptAnalysis::addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump, unsigned *forwardLoop,
                        unsigned stackDepth)
{
    JS_ASSERT(offset < script_->length);

    Bytecode *&code = codeArray[offset];
    if (!code) {
        code = cx->typeLifoAlloc().new_<Bytecode>();
        if (!code) {
            setOOM(cx);
            return false;
        }
        code->stackDepth = stackDepth;
    }
    JS_ASSERT(code->stackDepth == stackDepth);

    code->jumpTarget = true;

    if (offset < *currentOffset) {
        hasLoops_ = true;

        if (!code->analyzed) {
            




            if (*forwardJump == 0)
                *forwardJump = *currentOffset;
            if (*forwardLoop == 0)
                *forwardLoop = *currentOffset;
            *currentOffset = offset;
        }
    } else if (offset > *forwardJump) {
        *forwardJump = offset;
    }

    return true;
}

void
ScriptAnalysis::analyzeBytecode(JSContext *cx)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);
    JS_ASSERT(!ranBytecode());
    LifoAlloc &alloc = cx->typeLifoAlloc();

    numSlots = TotalSlots(script_);

    unsigned length = script_->length;
    codeArray = alloc.newArray<Bytecode*>(length);
    escapedSlots = alloc.newArray<bool>(numSlots);

    if (!codeArray || !escapedSlots) {
        setOOM(cx);
        return;
    }

    PodZero(codeArray, length);

    











    PodZero(escapedSlots, numSlots);

    bool allVarsAliased = script_->compartment()->debugMode();
    bool allArgsAliased = allVarsAliased || script_->argumentsHasVarBinding();

    RootedScript script(cx, script_);
    for (BindingIter bi(script); bi; bi++) {
        if (bi->kind() == ARGUMENT)
            escapedSlots[ArgSlot(bi.frameIndex())] = allArgsAliased || bi->aliased();
        else
            escapedSlots[LocalSlot(script_, bi.frameIndex())] = allVarsAliased || bi->aliased();
    }

    canTrackVars = true;

    




    unsigned forwardJump = 0;

    
    unsigned forwardLoop = 0;

    



    unsigned forwardCatch = 0;

    
    Bytecode *startcode = alloc.new_<Bytecode>();
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
        jsbytecode *pc = script_->code + offset;

        JSOp op = (JSOp)*pc;
        JS_ASSERT(op < JSOP_LIMIT);

        
        unsigned successorOffset = offset + GetBytecodeLength(pc);

        



        nextOffset = successorOffset;

        if (!code) {
            
            continue;
        }

        



        if (forwardLoop) {
            if (forwardLoop <= offset)
                forwardLoop = 0;
        }

        if (code->analyzed) {
            
            continue;
        }

        code->analyzed = true;

        if (script_->hasBreakpointsAt(pc))
            canTrackVars = false;

        unsigned stackDepth = code->stackDepth;

        if (!forwardJump)
            code->unconditional = true;

        unsigned nuses = GetUseCount(script_, offset);
        unsigned ndefs = GetDefCount(script_, offset);

        JS_ASSERT(stackDepth >= nuses);
        stackDepth -= nuses;
        stackDepth += ndefs;

        switch (op) {

          case JSOP_RETURN:
          case JSOP_STOP:
            numReturnSites_++;
            break;

          case JSOP_NAME:
          case JSOP_CALLNAME:
          case JSOP_BINDNAME:
          case JSOP_SETNAME:
          case JSOP_DELNAME:
          case JSOP_GETALIASEDVAR:
          case JSOP_CALLALIASEDVAR:
          case JSOP_SETALIASEDVAR:
          case JSOP_LAMBDA:
            usesScopeChain_ = true;
            break;

          case JSOP_DEFFUN:
          case JSOP_DEFVAR:
          case JSOP_DEFCONST:
          case JSOP_SETCONST:
            usesScopeChain_ = true; 
            canTrackVars = false;
            break;

          case JSOP_EVAL:
          case JSOP_SPREADEVAL:
          case JSOP_ENTERWITH:
            canTrackVars = false;
            break;

          case JSOP_TABLESWITCH: {
            unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            int32_t low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            int32_t high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump, &forwardLoop, stackDepth))
                return;

            for (int32_t i = low; i <= high; i++) {
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (targetOffset != offset) {
                    if (!addJump(cx, targetOffset, &nextOffset, &forwardJump, &forwardLoop, stackDepth))
                        return;
                }
                pc2 += JUMP_OFFSET_LEN;
            }
            break;
          }

          case JSOP_TRY: {
            





            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script_->mainOffset + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    
                    if (catchOffset > forwardCatch)
                        forwardCatch = catchOffset;

                    if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP) {
                        if (!addJump(cx, catchOffset, &nextOffset, &forwardJump, &forwardLoop, stackDepth))
                            return;
                        getCode(catchOffset).exceptionEntry = true;
                    }
                }
            }
            break;
          }

          case JSOP_GETLOCAL: {
            




            jsbytecode *next = pc + JSOP_GETLOCAL_LENGTH;
            if (JSOp(*next) != JSOP_POP || jumpTarget(next)) {
                uint32_t local = GET_SLOTNO(pc);
                if (local >= script_->nfixed) {
                    localsAliasStack_ = true;
                    break;
                }
            }
            break;
          }

          case JSOP_CALLLOCAL:
          case JSOP_SETLOCAL: {
            uint32_t local = GET_SLOTNO(pc);
            if (local >= script_->nfixed) {
                localsAliasStack_ = true;
                break;
            }
            break;
          }

          case JSOP_GETPROP:
          case JSOP_CALLPROP:
          case JSOP_LENGTH:
          case JSOP_GETELEM:
          case JSOP_CALLELEM:
            numPropertyReads_++;
            break;

          case JSOP_FINALLY:
            hasTryFinally_ = true;
            break;

          default:
            break;
        }

        bool jump = IsJumpOpcode(op);

        
        if (jump) {
            
            unsigned newStackDepth = stackDepth;
            if (op == JSOP_CASE)
                newStackDepth--;

            unsigned targetOffset = offset + GET_JUMP_OFFSET(pc);
            if (!addJump(cx, targetOffset, &nextOffset, &forwardJump, &forwardLoop, newStackDepth))
                return;
        }

        
        if (BytecodeFallsThrough(op)) {
            JS_ASSERT(successorOffset < script_->length);

            Bytecode *&nextcode = codeArray[successorOffset];

            if (!nextcode) {
                nextcode = alloc.new_<Bytecode>();
                if (!nextcode) {
                    setOOM(cx);
                    return;
                }
                nextcode->stackDepth = stackDepth;
            }
            JS_ASSERT(nextcode->stackDepth == stackDepth);

            if (jump)
                nextcode->jumpFallthrough = true;

            
            if (jump)
                nextcode->jumpTarget = true;
            else
                nextcode->fallthrough = true;
        }
    }

    JS_ASSERT(!failed());
    JS_ASSERT(forwardJump == 0 && forwardLoop == 0 && forwardCatch == 0);

    ranBytecode_ = true;

    




    if (!script_->analyzedArgsUsage())
        analyzeSSA(cx);
}





void
ScriptAnalysis::analyzeLifetimes(JSContext *cx)
{
    JS_ASSERT(cx->compartment()->activeAnalysis && !ranLifetimes() && !failed());

    if (!ranBytecode()) {
        analyzeBytecode(cx);
        if (failed())
            return;
    }

    LifoAlloc &alloc = cx->typeLifoAlloc();

    lifetimes = alloc.newArray<LifetimeVariable>(numSlots);
    if (!lifetimes) {
        setOOM(cx);
        return;
    }
    PodZero(lifetimes, numSlots);

    



    LifetimeVariable **saved = cx->pod_calloc<LifetimeVariable*>(numSlots);
    if (!saved) {
        setOOM(cx);
        return;
    }
    unsigned savedCount = 0;

    LoopAnalysis *loop = NULL;

    uint32_t offset = script_->length - 1;
    while (offset < script_->length) {
        Bytecode *code = maybeCode(offset);
        if (!code) {
            offset--;
            continue;
        }

        jsbytecode *pc = script_->code + offset;

        JSOp op = (JSOp) *pc;

        if (op == JSOP_LOOPHEAD && code->loop) {
            





            JS_ASSERT(loop == code->loop);
            unsigned backedge = code->loop->backedge;
            for (unsigned i = 0; i < numSlots; i++) {
                if (lifetimes[i].lifetime)
                    extendVariable(cx, lifetimes[i], offset, backedge);
            }

            loop = loop->parent;
            JS_ASSERT_IF(loop, loop->head < offset);
        }

        if (code->exceptionEntry) {
            DebugOnly<bool> found = false;
            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script_->mainOffset + tn->start;
                if (startOffset + tn->length == offset) {
                    



                    for (unsigned i = 0; i < numSlots; i++) {
                        if (lifetimes[i].lifetime)
                            ensureVariable(lifetimes[i], startOffset - 1);
                    }

                    found = true;
                    break;
                }
            }
            JS_ASSERT(found);
        }

        switch (op) {
          case JSOP_GETARG:
          case JSOP_CALLARG:
          case JSOP_GETLOCAL:
          case JSOP_CALLLOCAL:
          case JSOP_THIS: {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (!slotEscapes(slot))
                addVariable(cx, lifetimes[slot], offset, saved, savedCount);
            break;
          }

          case JSOP_SETARG:
          case JSOP_SETLOCAL: {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (!slotEscapes(slot))
                killVariable(cx, lifetimes[slot], offset, saved, savedCount);
            break;
          }

          case JSOP_TABLESWITCH:
            
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                var.lifetime = alloc.new_<Lifetime>(offset, uint32_t(var.savedEnd), var.saved);
                if (!var.lifetime) {
                    js_free(saved);
                    setOOM(cx);
                    return;
                }
                var.saved = NULL;
                saved[i--] = saved[--savedCount];
            }
            savedCount = 0;
            break;

          case JSOP_TRY:
            for (unsigned i = 0; i < numSlots; i++) {
                LifetimeVariable &var = lifetimes[i];
                if (var.ensured) {
                    JS_ASSERT(var.lifetime);
                    if (var.lifetime->start == offset)
                        var.ensured = false;
                }
            }
            break;

          case JSOP_LOOPENTRY:
            getCode(offset).loop = loop;
            break;

          default:;
        }

        if (IsJumpOpcode(op)) {
            




            uint32_t targetOffset = FollowBranch(cx, script_, offset);

            if (targetOffset < offset) {
                

#ifdef DEBUG
                JSOp nop = JSOp(script_->code[targetOffset]);
                JS_ASSERT(nop == JSOP_LOOPHEAD);
#endif

                LoopAnalysis *nloop = alloc.new_<LoopAnalysis>();
                if (!nloop) {
                    js_free(saved);
                    setOOM(cx);
                    return;
                }
                PodZero(nloop);

                nloop->parent = loop;
                loop = nloop;

                getCode(targetOffset).loop = loop;
                loop->head = targetOffset;
                loop->backedge = offset;

                



                uint32_t entry = targetOffset;
                if (entry) {
                    do {
                        entry--;
                    } while (!maybeCode(entry));

                    jsbytecode *entrypc = script_->code + entry;

                    if (JSOp(*entrypc) == JSOP_GOTO)
                        entry += GET_JUMP_OFFSET(entrypc);
                    else
                        entry = targetOffset;
                } else {
                    
                    entry = targetOffset;
                }
                JS_ASSERT(script_->code[entry] == JSOP_LOOPHEAD ||
                          script_->code[entry] == JSOP_LOOPENTRY);
            } else {
                for (unsigned i = 0; i < savedCount; i++) {
                    LifetimeVariable &var = *saved[i];
                    JS_ASSERT(!var.lifetime && var.saved);
                    if (var.live(targetOffset)) {
                        



                        var.lifetime =
                            alloc.new_<Lifetime>(offset, uint32_t(var.savedEnd), var.saved);
                        if (!var.lifetime) {
                            js_free(saved);
                            setOOM(cx);
                            return;
                        }
                        var.saved = NULL;
                        saved[i--] = saved[--savedCount];
                    } else if (loop && !var.savedEnd) {
                        






                        var.savedEnd = offset;
                    }
                }
            }
        }

        offset--;
    }

    js_free(saved);

    ranLifetimes_ = true;
}

#ifdef DEBUG
void
LifetimeVariable::print() const
{
    Lifetime *segment = lifetime ? lifetime : saved;
    while (segment) {
        printf(" (%u,%u)", segment->start, segment->end);
        segment = segment->next;
    }
    printf("\n");
}
#endif 

inline void
ScriptAnalysis::addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                            LifetimeVariable **&saved, unsigned &savedCount)
{
    if (var.lifetime) {
        if (var.ensured)
            return;

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
        var.lifetime =
            cx->typeLifoAlloc().new_<Lifetime>(offset, uint32_t(var.savedEnd), var.saved);
        if (!var.lifetime) {
            setOOM(cx);
            return;
        }
        var.saved = NULL;
    }
}

inline void
ScriptAnalysis::killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                             LifetimeVariable **&saved, unsigned &savedCount)
{
    if (!var.lifetime) {
        
        Lifetime *lifetime =
            cx->typeLifoAlloc().new_<Lifetime>(offset, uint32_t(var.savedEnd), var.saved);
        if (!lifetime) {
            setOOM(cx);
            return;
        }
        if (!var.saved)
            saved[savedCount++] = &var;
        var.saved = lifetime;
        var.saved->write = true;
        var.savedEnd = 0;
        return;
    }

    JS_ASSERT_IF(!var.ensured, offset < var.lifetime->start);
    unsigned start = var.lifetime->start;

    




    var.lifetime->start = offset;
    var.lifetime->write = true;

    if (var.ensured) {
        





        var.lifetime = cx->typeLifoAlloc().new_<Lifetime>(start, 0, var.lifetime);
        if (!var.lifetime) {
            setOOM(cx);
            return;
        }
        var.lifetime->end = offset;
    } else {
        var.saved = var.lifetime;
        var.savedEnd = 0;
        var.lifetime = NULL;

        saved[savedCount++] = &var;
    }
}

inline void
ScriptAnalysis::extendVariable(JSContext *cx, LifetimeVariable &var,
                               unsigned start, unsigned end)
{
    JS_ASSERT(var.lifetime);
    if (var.ensured) {
        




        JS_ASSERT(var.lifetime->start < start);
        return;
    }

    var.lifetime->start = start;

    







































    Lifetime *segment = var.lifetime;
    while (segment && segment->start < end) {
        uint32_t savedEnd = segment->savedEnd;
        if (!segment->next || segment->next->start >= end) {
            




            if (segment->end >= end) {
                
                break;
            }
            savedEnd = end;
        }
        JS_ASSERT(savedEnd <= end);
        if (savedEnd > segment->end) {
            Lifetime *tail = cx->typeLifoAlloc().new_<Lifetime>(savedEnd, 0, segment->next);
            if (!tail) {
                setOOM(cx);
                return;
            }
            tail->start = segment->end;

            




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
}

inline void
ScriptAnalysis::ensureVariable(LifetimeVariable &var, unsigned until)
{
    JS_ASSERT(var.lifetime);

    



    if (var.ensured) {
        JS_ASSERT(var.lifetime->start <= until);
        return;
    }

    JS_ASSERT(until < var.lifetime->start);
    var.lifetime->start = until;
    var.ensured = true;
}





void
ScriptAnalysis::analyzeSSA(JSContext *cx)
{
    JS_ASSERT(cx->compartment()->activeAnalysis && !ranSSA() && !failed());

    if (!ranLifetimes()) {
        analyzeLifetimes(cx);
        if (failed())
            return;
    }

    LifoAlloc &alloc = cx->typeLifoAlloc();
    unsigned maxDepth = script_->nslots - script_->nfixed;

    



    SSAValueInfo *values = cx->pod_calloc<SSAValueInfo>(numSlots + maxDepth);
    if (!values) {
        setOOM(cx);
        return;
    }
    struct FreeSSAValues {
        SSAValueInfo *values;
        FreeSSAValues(SSAValueInfo *values) : values(values) {}
        ~FreeSSAValues() { js_free(values); }
    } free(values);

    SSAValueInfo *stack = values + numSlots;
    uint32_t stackDepth = 0;

    for (uint32_t slot = ArgSlot(0); slot < numSlots; slot++) {
        if (trackSlot(slot))
            values[slot].v.initInitial(slot);
    }

    





    Vector<uint32_t> branchTargets(cx);

    




    Vector<uint32_t> exceptionTargets(cx);

    uint32_t offset = 0;
    while (offset < script_->length) {
        jsbytecode *pc = script_->code + offset;
        JSOp op = (JSOp)*pc;

        uint32_t successorOffset = offset + GetBytecodeLength(pc);

        Bytecode *code = maybeCode(pc);
        if (!code) {
            offset = successorOffset;
            continue;
        }

        if (code->exceptionEntry) {
            
            for (size_t i = 0; i < exceptionTargets.length(); i++) {
                if (exceptionTargets[i] == offset) {
                    exceptionTargets[i] = exceptionTargets.back();
                    exceptionTargets.popBack();
                    break;
                }
            }
        }

        if (code->stackDepth > stackDepth)
            PodZero(stack + stackDepth, code->stackDepth - stackDepth);
        stackDepth = code->stackDepth;

        if (op == JSOP_LOOPHEAD && code->loop) {
            














            Vector<SlotValue> *&pending = code->pendingValues;
            if (!pending) {
                pending = cx->new_< Vector<SlotValue> >(cx);
                if (!pending) {
                    setOOM(cx);
                    return;
                }
            }

            




            for (unsigned i = 0; i < pending->length(); i++) {
                SlotValue &v = (*pending)[i];
                if (v.slot < numSlots && liveness(v.slot).firstWrite(code->loop) != UINT32_MAX) {
                    if (v.value.kind() != SSAValue::PHI || v.value.phiOffset() != offset) {
                        JS_ASSERT(v.value.phiOffset() < offset);
                        SSAValue ov = v.value;
                        if (!makePhi(cx, v.slot, offset, &ov))
                            return;
                        insertPhi(cx, ov, v.value);
                        v.value = ov;
                    }
                }
                if (code->fallthrough || code->jumpFallthrough)
                    mergeValue(cx, offset, values[v.slot].v, &v);
                mergeBranchTarget(cx, values[v.slot], v.slot, branchTargets, offset - 1);
                values[v.slot].v = v.value;
            }

            






            for (uint32_t slot = ArgSlot(0); slot < numSlots + stackDepth; slot++) {
                if (slot >= numSlots || !trackSlot(slot))
                    continue;
                if (liveness(slot).firstWrite(code->loop) == UINT32_MAX)
                    continue;
                if (values[slot].v.kind() == SSAValue::PHI && values[slot].v.phiOffset() == offset) {
                    
                    continue;
                }
                SSAValue ov;
                if (!makePhi(cx, slot, offset, &ov))
                    return;
                if (code->fallthrough || code->jumpFallthrough)
                    insertPhi(cx, ov, values[slot].v);
                mergeBranchTarget(cx, values[slot], slot, branchTargets, offset - 1);
                values[slot].v = ov;
                if (!pending->append(SlotValue(slot, ov))) {
                    setOOM(cx);
                    return;
                }
            }
        } else if (code->pendingValues) {
            









            bool exception = getCode(offset).exceptionEntry;
            Vector<SlotValue> *pending = code->pendingValues;
            for (unsigned i = 0; i < pending->length(); i++) {
                SlotValue &v = (*pending)[i];
                if (code->fallthrough || code->jumpFallthrough ||
                    (exception && values[v.slot].v.kind() != SSAValue::EMPTY)) {
                    mergeValue(cx, offset, values[v.slot].v, &v);
                }
                mergeBranchTarget(cx, values[v.slot], v.slot, branchTargets, offset);
                values[v.slot].v = v.value;
            }
            freezeNewValues(cx, offset);
        }

        unsigned nuses = GetUseCount(script_, offset);
        unsigned ndefs = GetDefCount(script_, offset);
        JS_ASSERT(stackDepth >= nuses);

        unsigned xuses = ExtendedUse(pc) ? nuses + 1 : nuses;

        if (xuses) {
            code->poppedValues = alloc.newArray<SSAValue>(xuses);
            if (!code->poppedValues) {
                setOOM(cx);
                return;
            }
            for (unsigned i = 0; i < nuses; i++) {
                SSAValue &v = stack[stackDepth - 1 - i].v;
                code->poppedValues[i] = v;
                v.clear();
            }
            if (xuses > nuses) {
                



                uint32_t slot = GetBytecodeSlot(script_, pc);
                if (trackSlot(slot))
                    code->poppedValues[nuses] = values[slot].v;
                else
                    code->poppedValues[nuses].clear();
            }

            if (xuses) {
                SSAUseChain *useChains = alloc.newArray<SSAUseChain>(xuses);
                if (!useChains) {
                    setOOM(cx);
                    return;
                }
                PodZero(useChains, xuses);
                for (unsigned i = 0; i < xuses; i++) {
                    const SSAValue &v = code->poppedValues[i];
                    if (trackUseChain(v)) {
                        SSAUseChain *&uses = useChain(v);
                        useChains[i].popped = true;
                        useChains[i].offset = offset;
                        useChains[i].u.which = i;
                        useChains[i].next = uses;
                        uses = &useChains[i];
                    }
                }
            }
        }

        stackDepth -= nuses;

        for (unsigned i = 0; i < ndefs; i++)
            stack[stackDepth + i].v.initPushed(offset, i);

        unsigned xdefs = ExtendedDef(pc) ? ndefs + 1 : ndefs;
        if (xdefs) {
            code->pushedUses = alloc.newArray<SSAUseChain *>(xdefs);
            if (!code->pushedUses) {
                setOOM(cx);
                return;
            }
            PodZero(code->pushedUses, xdefs);
        }

        stackDepth += ndefs;

        if (op == JSOP_SETARG || op == JSOP_SETLOCAL) {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (trackSlot(slot)) {
                mergeBranchTarget(cx, values[slot], slot, branchTargets, offset);
                mergeExceptionTarget(cx, values[slot].v, slot, exceptionTargets);
                values[slot].v.initWritten(slot, offset);
            }
        }

        switch (op) {
          case JSOP_GETARG:
          case JSOP_GETLOCAL: {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (trackSlot(slot)) {
                



                stack[stackDepth - 1].v = code->poppedValues[0] = values[slot].v;
            }
            break;
          }

          

          case JSOP_MOREITER:
            stack[stackDepth - 2].v = code->poppedValues[0];
            break;

          case JSOP_INITPROP:
          case JSOP_INITPROP_GETTER:
          case JSOP_INITPROP_SETTER:
            stack[stackDepth - 1].v = code->poppedValues[1];
            break;

          case JSOP_SPREAD:
          case JSOP_INITELEM_INC:
            stack[stackDepth - 2].v = code->poppedValues[2];
            break;

          case JSOP_INITELEM_ARRAY:
            stack[stackDepth - 1].v = code->poppedValues[1];
            break;

          case JSOP_INITELEM:
          case JSOP_INITELEM_GETTER:
          case JSOP_INITELEM_SETTER:
            stack[stackDepth - 1].v = code->poppedValues[2];
            break;

          case JSOP_DUP:
            stack[stackDepth - 1].v = stack[stackDepth - 2].v = code->poppedValues[0];
            break;

          case JSOP_DUP2:
            stack[stackDepth - 1].v = stack[stackDepth - 3].v = code->poppedValues[0];
            stack[stackDepth - 2].v = stack[stackDepth - 4].v = code->poppedValues[1];
            break;

          case JSOP_SWAP:
            
          case JSOP_PICK: {
            unsigned pickedDepth = (op == JSOP_SWAP ? 1 : pc[1]);
            stack[stackDepth - 1].v = code->poppedValues[pickedDepth];
            for (unsigned i = 0; i < pickedDepth; i++)
                stack[stackDepth - 2 - i].v = code->poppedValues[i];
            break;
          }

          




          case JSOP_TABLESWITCH: {
            unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            int32_t low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            int32_t high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            for (int32_t i = low; i <= high; i++) {
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (targetOffset != offset)
                    checkBranchTarget(cx, targetOffset, branchTargets, values, stackDepth);
                pc2 += JUMP_OFFSET_LEN;
            }

            checkBranchTarget(cx, defaultOffset, branchTargets, values, stackDepth);
            break;
          }

          case JSOP_TRY: {
            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script_->mainOffset + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP) {
                        checkBranchTarget(cx, catchOffset, branchTargets, values, stackDepth);
                        checkExceptionTarget(cx, catchOffset, exceptionTargets);
                    }
                }
            }
            break;
          }

          case JSOP_THROW:
          case JSOP_RETURN:
          case JSOP_STOP:
          case JSOP_RETRVAL:
            mergeAllExceptionTargets(cx, values, exceptionTargets);
            break;

          default:;
        }

        if (IsJumpOpcode(op)) {
            unsigned targetOffset = FollowBranch(cx, script_, offset);
            checkBranchTarget(cx, targetOffset, branchTargets, values, stackDepth);

            



            if (targetOffset < offset)
                freezeNewValues(cx, targetOffset);
        }

        offset = successorOffset;
    }

    ranSSA_ = true;

    



    if (!script_->analyzedArgsUsage())
        script_->setNeedsArgsObj(needsArgsObj(cx));
}


static inline unsigned
PhiNodeCapacity(unsigned length)
{
    if (length <= 4)
        return 4;

    return 1 << (FloorLog2(length - 1) + 1);
}

bool
ScriptAnalysis::makePhi(JSContext *cx, uint32_t slot, uint32_t offset, SSAValue *pv)
{
    SSAPhiNode *node = cx->typeLifoAlloc().new_<SSAPhiNode>();
    SSAValue *options = cx->typeLifoAlloc().newArray<SSAValue>(PhiNodeCapacity(0));
    if (!node || !options) {
        setOOM(cx);
        return false;
    }
    node->slot = slot;
    node->options = options;
    pv->initPhi(offset, node);
    return true;
}

void
ScriptAnalysis::insertPhi(JSContext *cx, SSAValue &phi, const SSAValue &v)
{
    JS_ASSERT(phi.kind() == SSAValue::PHI);
    SSAPhiNode *node = phi.phiNode();

    




    if (node->length <= 8) {
        for (unsigned i = 0; i < node->length; i++) {
            if (v == node->options[i])
                return;
        }
    }

    if (trackUseChain(v)) {
        SSAUseChain *&uses = useChain(v);

        SSAUseChain *use = cx->typeLifoAlloc().new_<SSAUseChain>();
        if (!use) {
            setOOM(cx);
            return;
        }

        use->popped = false;
        use->offset = phi.phiOffset();
        use->u.phi = node;
        use->next = uses;
        uses = use;
    }

    if (node->length < PhiNodeCapacity(node->length)) {
        node->options[node->length++] = v;
        return;
    }

    SSAValue *newOptions =
        cx->typeLifoAlloc().newArray<SSAValue>(PhiNodeCapacity(node->length + 1));
    if (!newOptions) {
        setOOM(cx);
        return;
    }

    PodCopy(newOptions, node->options, node->length);
    node->options = newOptions;
    node->options[node->length++] = v;
}

inline void
ScriptAnalysis::mergeValue(JSContext *cx, uint32_t offset, const SSAValue &v, SlotValue *pv)
{
    
    JS_ASSERT(v.kind() != SSAValue::EMPTY && pv->value.kind() != SSAValue::EMPTY);

    if (v == pv->value)
        return;

    if (pv->value.kind() != SSAValue::PHI || pv->value.phiOffset() < offset) {
        SSAValue ov = pv->value;
        if (makePhi(cx, pv->slot, offset, &pv->value)) {
            insertPhi(cx, pv->value, v);
            insertPhi(cx, pv->value, ov);
        }
        return;
    }

    JS_ASSERT(pv->value.phiOffset() == offset);
    insertPhi(cx, pv->value, v);
}

void
ScriptAnalysis::checkPendingValue(JSContext *cx, const SSAValue &v, uint32_t slot,
                                  Vector<SlotValue> *pending)
{
    JS_ASSERT(v.kind() != SSAValue::EMPTY);

    for (unsigned i = 0; i < pending->length(); i++) {
        if ((*pending)[i].slot == slot)
            return;
    }

    if (!pending->append(SlotValue(slot, v)))
        setOOM(cx);
}

void
ScriptAnalysis::checkBranchTarget(JSContext *cx, uint32_t targetOffset,
                                  Vector<uint32_t> &branchTargets,
                                  SSAValueInfo *values, uint32_t stackDepth)
{
    unsigned targetDepth = getCode(targetOffset).stackDepth;
    JS_ASSERT(targetDepth <= stackDepth);

    




    Vector<SlotValue> *&pending = getCode(targetOffset).pendingValues;
    if (pending) {
        for (unsigned i = 0; i < pending->length(); i++) {
            SlotValue &v = (*pending)[i];
            mergeValue(cx, targetOffset, values[v.slot].v, &v);
        }
    } else {
        pending = cx->new_< Vector<SlotValue> >(cx);
        if (!pending || !branchTargets.append(targetOffset)) {
            setOOM(cx);
            return;
        }
    }

    





    for (unsigned i = 0; i < targetDepth; i++) {
        uint32_t slot = StackSlot(script_, i);
        checkPendingValue(cx, values[slot].v, slot, pending);
    }
}

void
ScriptAnalysis::checkExceptionTarget(JSContext *cx, uint32_t catchOffset,
                                     Vector<uint32_t> &exceptionTargets)
{
    JS_ASSERT(getCode(catchOffset).exceptionEntry);

    



    for (unsigned i = 0; i < exceptionTargets.length(); i++) {
        if (exceptionTargets[i] == catchOffset)
            return;
    }
    if (!exceptionTargets.append(catchOffset))
        setOOM(cx);
}

void
ScriptAnalysis::mergeBranchTarget(JSContext *cx, SSAValueInfo &value, uint32_t slot,
                                  const Vector<uint32_t> &branchTargets, uint32_t currentOffset)
{
    if (slot >= numSlots) {
        




        return;
    }

    JS_ASSERT(trackSlot(slot));

    





    for (int i = branchTargets.length() - 1; i >= value.branchSize; i--) {
        if (branchTargets[i] <= currentOffset)
            continue;

        const Bytecode &code = getCode(branchTargets[i]);

        Vector<SlotValue> *pending = code.pendingValues;
        checkPendingValue(cx, value.v, slot, pending);
    }

    value.branchSize = branchTargets.length();
}

void
ScriptAnalysis::mergeExceptionTarget(JSContext *cx, const SSAValue &value, uint32_t slot,
                                     const Vector<uint32_t> &exceptionTargets)
{
    JS_ASSERT(trackSlot(slot));

    







    for (unsigned i = 0; i < exceptionTargets.length(); i++) {
        unsigned offset = exceptionTargets[i];
        Vector<SlotValue> *pending = getCode(offset).pendingValues;

        bool duplicate = false;
        for (unsigned i = 0; i < pending->length(); i++) {
            if ((*pending)[i].slot == slot) {
                duplicate = true;
                SlotValue &v = (*pending)[i];
                mergeValue(cx, offset, value, &v);
                break;
            }
        }

        if (!duplicate && !pending->append(SlotValue(slot, value)))
            setOOM(cx);
    }
}

void
ScriptAnalysis::mergeAllExceptionTargets(JSContext *cx, SSAValueInfo *values,
                                         const Vector<uint32_t> &exceptionTargets)
{
    for (unsigned i = 0; i < exceptionTargets.length(); i++) {
        Vector<SlotValue> *pending = getCode(exceptionTargets[i]).pendingValues;
        for (unsigned i = 0; i < pending->length(); i++) {
            const SlotValue &v = (*pending)[i];
            if (trackSlot(v.slot))
                mergeExceptionTarget(cx, values[v.slot].v, v.slot, exceptionTargets);
        }
    }
}

void
ScriptAnalysis::freezeNewValues(JSContext *cx, uint32_t offset)
{
    Bytecode &code = getCode(offset);

    Vector<SlotValue> *pending = code.pendingValues;
    code.pendingValues = NULL;

    unsigned count = pending->length();
    if (count == 0) {
        js_delete(pending);
        return;
    }

    code.newValues = cx->typeLifoAlloc().newArray<SlotValue>(count + 1);
    if (!code.newValues) {
        setOOM(cx);
        return;
    }

    for (unsigned i = 0; i < count; i++)
        code.newValues[i] = (*pending)[i];
    code.newValues[count].slot = 0;
    code.newValues[count].value.clear();

    js_delete(pending);
}

bool
ScriptAnalysis::needsArgsObj(JSContext *cx, SeenVector &seen, const SSAValue &v)
{
    



    if (!trackUseChain(v))
        return false;

    for (unsigned i = 0; i < seen.length(); i++) {
        if (v == seen[i])
            return false;
    }
    if (!seen.append(v)) {
        cx->compartment()->types.setPendingNukeTypes(cx);
        return true;
    }

    SSAUseChain *use = useChain(v);
    while (use) {
        if (needsArgsObj(cx, seen, use))
            return true;
        use = use->next;
    }

    return false;
}

bool
ScriptAnalysis::needsArgsObj(JSContext *cx, SeenVector &seen, SSAUseChain *use)
{
    if (!use->popped)
        return needsArgsObj(cx, seen, SSAValue::PhiValue(use->offset, use->u.phi));

    jsbytecode *pc = script_->code + use->offset;
    JSOp op = JSOp(*pc);

    if (op == JSOP_POP || op == JSOP_POPN)
        return false;

    
    if (op == JSOP_FUNAPPLY && GET_ARGC(pc) == 2 && use->u.which == 0) {
        argumentsContentsObserved_ = true;
        return false;
    }

    
    if (op == JSOP_GETELEM && use->u.which == 1) {
        argumentsContentsObserved_ = true;
        return false;
    }

    
    if (op == JSOP_LENGTH)
        return false;

    

    if (op == JSOP_SETLOCAL) {
        uint32_t slot = GetBytecodeSlot(script_, pc);
        if (!trackSlot(slot))
            return true;
        return needsArgsObj(cx, seen, SSAValue::PushedValue(use->offset, 0)) ||
               needsArgsObj(cx, seen, SSAValue::WrittenVar(slot, use->offset));
    }

    if (op == JSOP_GETLOCAL)
        return needsArgsObj(cx, seen, SSAValue::PushedValue(use->offset, 0));

    return true;
}

bool
ScriptAnalysis::needsArgsObj(JSContext *cx)
{
    JS_ASSERT(script_->argumentsHasVarBinding());

    





    if (cx->compartment()->debugMode() || script_->isGenerator())
        return true;

    









    if (script_->bindingsAccessedDynamically)
        return false;

    



    if (localsAliasStack())
        return true;

    unsigned pcOff = script_->argumentsBytecode() - script_->code;

    SeenVector seen(cx);
    if (needsArgsObj(cx, seen, SSAValue::PushedValue(pcOff, 0)))
        return true;

    





    if (script_->funHasAnyAliasedFormal && argumentsContentsObserved_)
        return true;

    return false;
}

#ifdef DEBUG

void
ScriptAnalysis::printSSA(JSContext *cx)
{
    types::AutoEnterAnalysis enter(cx);

    printf("\n");

    RootedScript script(cx, script_);
    for (unsigned offset = 0; offset < script_->length; offset++) {
        Bytecode *code = maybeCode(offset);
        if (!code)
            continue;

        jsbytecode *pc = script_->code + offset;

        PrintBytecode(cx, script, pc);

        SlotValue *newv = code->newValues;
        if (newv) {
            while (newv->slot) {
                if (newv->value.kind() != SSAValue::PHI || newv->value.phiOffset() != offset) {
                    newv++;
                    continue;
                }
                printf("  phi ");
                newv->value.print();
                printf(" [");
                for (unsigned i = 0; i < newv->value.phiLength(); i++) {
                    if (i)
                        printf(",");
                    newv->value.phiValue(i).print();
                }
                printf("]\n");
                newv++;
            }
        }

        unsigned nuses = GetUseCount(script_, offset);
        unsigned xuses = ExtendedUse(pc) ? nuses + 1 : nuses;

        for (unsigned i = 0; i < xuses; i++) {
            printf("  popped%d: ", i);
            code->poppedValues[i].print();
            printf("\n");
        }
    }

    printf("\n");
}

void
SSAValue::print() const
{
    switch (kind()) {

      case EMPTY:
        printf("empty");
        break;

      case PUSHED:
        printf("pushed:%05u#%u", pushedOffset(), pushedIndex());
        break;

      case VAR:
        if (varInitial())
            printf("initial:%u", varSlot());
        else
            printf("write:%05u", varOffset());
        break;

      case PHI:
        printf("phi:%05u#%u", phiOffset(), phiSlot());
        break;

      default:
        MOZ_ASSUME_UNREACHABLE("Bad kind");
    }
}

void
ScriptAnalysis::assertMatchingDebugMode()
{
    JS_ASSERT(!!script_->compartment()->debugMode() == !!originalDebugMode_);
}

#endif  
