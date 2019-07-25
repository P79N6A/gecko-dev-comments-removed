






































#include "jsanalyze.h"
#include "jsautooplen.h"
#include "jscompartment.h"
#include "jscntxt.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"

namespace js {
namespace analyze {





#ifdef DEBUG
void
PrintBytecode(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    printf("#%u:", script->id());
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return;
    js_Disassemble1(cx, script, pc, pc - script->code, true, &sprinter);
    fprintf(stdout, "%s", sprinter.string());
}
#endif





inline bool
ScriptAnalysis::addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump,
                        unsigned stackDepth)
{
    JS_ASSERT(offset < script->length);

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

void
ScriptAnalysis::checkAliasedName(JSContext *cx, jsbytecode *pc)
{
    







    JSAtom *atom;
    if (JSOp(*pc) == JSOP_DEFFUN) {
        JSFunction *fun = script->getFunction(GET_UINT32_INDEX(pc));
        atom = fun->atom;
    } else {
        JS_ASSERT(JOF_TYPE(js_CodeSpec[*pc].format) == JOF_ATOM);
        atom = script->getAtom(GET_UINT32_INDEX(pc));
    }

    unsigned index;
    BindingKind kind = script->bindings.lookup(cx, atom, &index);

    if (kind == ARGUMENT)
        escapedSlots[ArgSlot(index)] = true;
    else if (kind == VARIABLE)
        escapedSlots[LocalSlot(script, index)] = true;
}

void
ScriptAnalysis::analyzeBytecode(JSContext *cx)
{
    JS_ASSERT(cx->compartment->activeAnalysis);
    JS_ASSERT(!ranBytecode());
    LifoAlloc &tla = cx->typeLifoAlloc();

    unsigned length = script->length;
    unsigned nargs = script->function() ? script->function()->nargs : 0;

    numSlots = TotalSlots(script);

    codeArray = tla.newArray<Bytecode*>(length);
    escapedSlots = tla.newArray<bool>(numSlots);

    if (!codeArray || !escapedSlots) {
        setOOM(cx);
        return;
    }

    PodZero(codeArray, length);

    








    PodZero(escapedSlots, numSlots);

    if (script->usesEval || script->mayNeedArgsObj() || script->compartment()->debugMode()) {
        for (unsigned i = 0; i < nargs; i++)
            escapedSlots[ArgSlot(i)] = true;
    } else {
        for (unsigned i = 0; i < script->nClosedArgs; i++) {
            unsigned arg = script->getClosedArg(i);
            JS_ASSERT(arg < nargs);
            escapedSlots[ArgSlot(arg)] = true;
        }
    }

    if (script->usesEval || script->compartment()->debugMode()) {
        for (unsigned i = 0; i < script->nfixed; i++)
            escapedSlots[LocalSlot(script, i)] = true;
    } else {
        for (uint32_t i = 0; i < script->nClosedVars; i++) {
            unsigned local = script->getClosedVar(i);
            JS_ASSERT(local < script->nfixed);
            escapedSlots[LocalSlot(script, local)] = true;
        }
    }

    



    if (cx->compartment->debugMode())
        usesReturnValue_ = true;

    bool heavyweight = script->function() && script->function()->isHeavyweight();

    isCompileable = true;

    isInlineable = true;
    if (script->nClosedArgs || script->nClosedVars || heavyweight ||
        script->usesEval || script->mayNeedArgsObj() || cx->compartment->debugMode()) {
        isInlineable = false;
    }

    modifiesArguments_ = false;
    if (script->nClosedArgs || heavyweight)
        modifiesArguments_ = true;

    canTrackVars = true;

    




    unsigned forwardJump = 0;

    



    unsigned forwardCatch = 0;

    
    Bytecode *startcode = tla.new_<Bytecode>();
    if (!startcode) {
        setOOM(cx);
        return;
    }

    startcode->stackDepth = 0;
    codeArray[0] = startcode;

    
    unsigned nTypeSets = 0;
    types::TypeSet *typeArray = script->types->typeArray();

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

        if (script->hasBreakpointsAt(pc)) {
            code->safePoint = true;
            isInlineable = canTrackVars = false;
        }

        unsigned stackDepth = code->stackDepth;

        if (!forwardJump)
            code->unconditional = true;

        



        if (!(js_CodeSpec[op].format & JOF_DECOMPOSE)) {
            unsigned nuses = GetUseCount(script, offset);
            unsigned ndefs = GetDefCount(script, offset);

            JS_ASSERT(stackDepth >= nuses);
            stackDepth -= nuses;
            stackDepth += ndefs;
        }

        






        if ((js_CodeSpec[op].format & JOF_TYPESET) && cx->typeInferenceEnabled()) {
            if (nTypeSets < script->nTypeSets) {
                code->observedTypes = &typeArray[nTypeSets++];
            } else {
                JS_ASSERT(nTypeSets == UINT16_MAX);
                code->observedTypes = &typeArray[nTypeSets - 1];
            }
        }

        switch (op) {

          case JSOP_RETURN:
          case JSOP_STOP:
            numReturnSites_++;
            break;

          case JSOP_SETRVAL:
          case JSOP_POPV:
            usesReturnValue_ = true;
            isInlineable = false;
            break;

          case JSOP_QNAMEPART:
          case JSOP_QNAMECONST:
            isCompileable = false;
          case JSOP_NAME:
          case JSOP_CALLNAME:
          case JSOP_BINDNAME:
          case JSOP_SETNAME:
          case JSOP_DELNAME:
            checkAliasedName(cx, pc);
            usesScopeChain_ = true;
            isInlineable = false;
            break;

          case JSOP_DEFLOCALFUN:
          case JSOP_LAMBDA:
            usesScopeChain_ = true;
            isInlineable = false;
            break;

          case JSOP_DEFFUN:
          case JSOP_DEFVAR:
          case JSOP_DEFCONST:
          case JSOP_SETCONST:
            checkAliasedName(cx, pc);
            extendsScope_ = true;
            usesScopeChain_ = true; 
            isInlineable = canTrackVars = false;
            break;

          case JSOP_EVAL:
            extendsScope_ = true;
            isInlineable = canTrackVars = false;
            break;

          case JSOP_ENTERWITH:
            addsScopeObjects_ = true;
            isCompileable = isInlineable = canTrackVars = false;
            break;

          case JSOP_ENTERLET0:
          case JSOP_ENTERLET1:
          case JSOP_ENTERBLOCK:
          case JSOP_LEAVEBLOCK:
            addsScopeObjects_ = true;
            isInlineable = false;
            break;

          case JSOP_THIS:
            usesThisValue_ = true;
            break;

          case JSOP_CALL:
          case JSOP_NEW:
            
            hasFunctionCalls_ = true;
            break;

          case JSOP_TABLESWITCH: {
            isInlineable = false;
            unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            int32_t low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            int32_t high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump, stackDepth))
                return;
            getCode(defaultOffset).switchTarget = true;
            getCode(defaultOffset).safePoint = true;

            for (int32_t i = low; i <= high; i++) {
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (targetOffset != offset) {
                    if (!addJump(cx, targetOffset, &nextOffset, &forwardJump, stackDepth))
                        return;
                }
                getCode(targetOffset).switchTarget = true;
                getCode(targetOffset).safePoint = true;
                pc2 += JUMP_OFFSET_LEN;
            }
            break;
          }

          case JSOP_LOOKUPSWITCH: {
            isInlineable = false;
            unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            unsigned npairs = GET_UINT16(pc2);
            pc2 += UINT16_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump, stackDepth))
                return;
            getCode(defaultOffset).switchTarget = true;
            getCode(defaultOffset).safePoint = true;

            while (npairs) {
                pc2 += UINT32_INDEX_LEN;
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (!addJump(cx, targetOffset, &nextOffset, &forwardJump, stackDepth))
                    return;
                getCode(targetOffset).switchTarget = true;
                getCode(targetOffset).safePoint = true;
                pc2 += JUMP_OFFSET_LEN;
                npairs--;
            }
            break;
          }

          case JSOP_TRY: {
            





            isInlineable = false;
            JSTryNote *tn = script->trynotes()->vector;
            JSTryNote *tnlimit = tn + script->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script->mainOffset + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    
                    if (catchOffset > forwardCatch)
                        forwardCatch = catchOffset;

                    if (tn->kind != JSTRY_ITER) {
                        if (!addJump(cx, catchOffset, &nextOffset, &forwardJump, stackDepth))
                            return;
                        getCode(catchOffset).exceptionEntry = true;
                        getCode(catchOffset).safePoint = true;
                    }
                }
            }
            break;
          }

          case JSOP_GETLOCAL: {
            




            jsbytecode *next = pc + JSOP_GETLOCAL_LENGTH;
            if (JSOp(*next) != JSOP_POP || jumpTarget(next)) {
                uint32_t local = GET_SLOTNO(pc);
                if (local >= script->nfixed) {
                    localsAliasStack_ = true;
                    break;
                }
            }
            break;
          }

          case JSOP_CALLLOCAL:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC:
          case JSOP_SETLOCAL:
          case JSOP_SETLOCALPOP: {
            uint32_t local = GET_SLOTNO(pc);
            if (local >= script->nfixed) {
                localsAliasStack_ = true;
                break;
            }
            break;
          }

          case JSOP_SETARG:
          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC:
            modifiesArguments_ = true;
            isInlineable = false;
            break;

          
          case JSOP_ARGUMENTS:
          case JSOP_THROW:
          case JSOP_EXCEPTION:
          case JSOP_DEBUGGER:
          case JSOP_FUNCALL:
          case JSOP_FUNAPPLY:
            isInlineable = false;
            break;

          
          case JSOP_NOP:
          case JSOP_UNDEFINED:
          case JSOP_GOTO:
          case JSOP_DEFAULT:
          case JSOP_IFEQ:
          case JSOP_IFNE:
          case JSOP_ITERNEXT:
          case JSOP_DUP:
          case JSOP_DUP2:
          case JSOP_SWAP:
          case JSOP_PICK:
          case JSOP_BITOR:
          case JSOP_BITXOR:
          case JSOP_BITAND:
          case JSOP_LT:
          case JSOP_LE:
          case JSOP_GT:
          case JSOP_GE:
          case JSOP_EQ:
          case JSOP_NE:
          case JSOP_LSH:
          case JSOP_RSH:
          case JSOP_URSH:
          case JSOP_ADD:
          case JSOP_SUB:
          case JSOP_MUL:
          case JSOP_DIV:
          case JSOP_MOD:
          case JSOP_NOT:
          case JSOP_BITNOT:
          case JSOP_NEG:
          case JSOP_POS:
          case JSOP_DELPROP:
          case JSOP_DELELEM:
          case JSOP_TYPEOF:
          case JSOP_TYPEOFEXPR:
          case JSOP_VOID:
          case JSOP_GETPROP:
          case JSOP_CALLPROP:
          case JSOP_LENGTH:
          case JSOP_GETELEM:
          case JSOP_CALLELEM:
          case JSOP_TOID:
          case JSOP_SETELEM:
          case JSOP_IMPLICITTHIS:
          case JSOP_DOUBLE:
          case JSOP_STRING:
          case JSOP_ZERO:
          case JSOP_ONE:
          case JSOP_NULL:
          case JSOP_FALSE:
          case JSOP_TRUE:
          case JSOP_OR:
          case JSOP_AND:
          case JSOP_CASE:
          case JSOP_STRICTEQ:
          case JSOP_STRICTNE:
          case JSOP_ITER:
          case JSOP_MOREITER:
          case JSOP_ENDITER:
          case JSOP_POP:
          case JSOP_GETARG:
          case JSOP_CALLARG:
          case JSOP_BINDGNAME:
          case JSOP_UINT16:
          case JSOP_NEWINIT:
          case JSOP_NEWARRAY:
          case JSOP_NEWOBJECT:
          case JSOP_ENDINIT:
          case JSOP_INITMETHOD:
          case JSOP_INITPROP:
          case JSOP_INITELEM:
          case JSOP_SETPROP:
          case JSOP_SETMETHOD:
          case JSOP_IN:
          case JSOP_INSTANCEOF:
          case JSOP_LINENO:
          case JSOP_ENUMELEM:
          case JSOP_CONDSWITCH:
          case JSOP_LABEL:
          case JSOP_RETRVAL:
          case JSOP_GETGNAME:
          case JSOP_CALLGNAME:
          case JSOP_SETGNAME:
          case JSOP_REGEXP:
          case JSOP_OBJECT:
          case JSOP_UINT24:
          case JSOP_GETXPROP:
          case JSOP_INT8:
          case JSOP_INT32:
          case JSOP_HOLE:
          case JSOP_LOOPHEAD:
          case JSOP_LOOPENTRY:
          case JSOP_NOTEARG:
            break;

          default:
            if (!(js_CodeSpec[op].format & JOF_DECOMPOSE))
                isCompileable = isInlineable = false;
            break;
        }

        uint32_t type = JOF_TYPE(js_CodeSpec[op].format);

        
        if (type == JOF_JUMP) {
            
            unsigned newStackDepth = stackDepth;

            switch (op) {
              case JSOP_CASE:
                
                newStackDepth--;
                break;

              default:;
            }

            unsigned targetOffset = offset + GET_JUMP_OFFSET(pc);
            if (!addJump(cx, targetOffset, &nextOffset, &forwardJump, newStackDepth))
                return;
        }

        
        if (!BytecodeNoFallThrough(op)) {
            JS_ASSERT(successorOffset < script->length);

            Bytecode *&nextcode = codeArray[successorOffset];

            if (!nextcode) {
                nextcode = tla.new_<Bytecode>();
                if (!nextcode) {
                    setOOM(cx);
                    return;
                }
                nextcode->stackDepth = stackDepth;
            }
            JS_ASSERT(nextcode->stackDepth == stackDepth);

            if (type == JOF_JUMP)
                nextcode->jumpFallthrough = true;

            
            if (type == JOF_JUMP)
                nextcode->jumpTarget = true;
            else
                nextcode->fallthrough = true;
        }
    }

    JS_ASSERT(!failed());
    JS_ASSERT(forwardJump == 0 && forwardCatch == 0);

    ranBytecode_ = true;

    




    if (!script->analyzedArgsUsage()) {
        if (!script->mayNeedArgsObj())
            script->setNeedsArgsObj(false);
        else
            analyzeSSA(cx);
        JS_ASSERT_IF(!failed(), script->analyzedArgsUsage());
    }
}





void
ScriptAnalysis::analyzeLifetimes(JSContext *cx)
{
    JS_ASSERT(cx->compartment->activeAnalysis && !ranLifetimes() && !failed());

    if (!ranBytecode()) {
        analyzeBytecode(cx);
        if (failed())
            return;
    }

    LifoAlloc &tla = cx->typeLifoAlloc();

    lifetimes = tla.newArray<LifetimeVariable>(numSlots);
    if (!lifetimes) {
        setOOM(cx);
        return;
    }
    PodZero(lifetimes, numSlots);

    



    LifetimeVariable **saved = (LifetimeVariable **)
        cx->calloc_(numSlots * sizeof(LifetimeVariable*));
    if (!saved) {
        setOOM(cx);
        return;
    }
    unsigned savedCount = 0;

    LoopAnalysis *loop = NULL;

    uint32_t offset = script->length - 1;
    while (offset < script->length) {
        Bytecode *code = maybeCode(offset);
        if (!code) {
            offset--;
            continue;
        }

        if (loop && code->safePoint)
            loop->hasSafePoints = true;

        jsbytecode *pc = script->code + offset;

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

        
        if (loop && code->jumpTarget && offset != loop->entry && offset > loop->lastBlock)
            loop->lastBlock = offset;

        if (code->exceptionEntry) {
            DebugOnly<bool> found = false;
            JSTryNote *tn = script->trynotes()->vector;
            JSTryNote *tnlimit = tn + script->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script->mainOffset + tn->start;
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
            uint32_t slot = GetBytecodeSlot(script, pc);
            if (!slotEscapes(slot))
                addVariable(cx, lifetimes[slot], offset, saved, savedCount);
            break;
          }

          case JSOP_SETARG:
          case JSOP_SETLOCAL:
          case JSOP_SETLOCALPOP:
          case JSOP_DEFLOCALFUN: {
            uint32_t slot = GetBytecodeSlot(script, pc);
            if (!slotEscapes(slot))
                killVariable(cx, lifetimes[slot], offset, saved, savedCount);
            break;
          }

          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC: {
            uint32_t slot = GetBytecodeSlot(script, pc);
            if (!slotEscapes(slot)) {
                killVariable(cx, lifetimes[slot], offset, saved, savedCount);
                addVariable(cx, lifetimes[slot], offset, saved, savedCount);
            }
            break;
          }

          case JSOP_LOOKUPSWITCH:
          case JSOP_TABLESWITCH:
            
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                var.lifetime = tla.new_<Lifetime>(offset, var.savedEnd, var.saved);
                if (!var.lifetime) {
                    cx->free_(saved);
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

        uint32_t type = JOF_TYPE(js_CodeSpec[op].format);
        if (type == JOF_JUMP) {
            




            uint32_t targetOffset = FollowBranch(cx, script, offset);

            



            if (loop && loop->entry == targetOffset && loop->entry > loop->lastBlock)
                loop->lastBlock = loop->entry;

            if (targetOffset < offset) {
                

#ifdef DEBUG
                JSOp nop = JSOp(script->code[targetOffset]);
                JS_ASSERT(nop == JSOP_LOOPHEAD);
#endif

                




                if (loop && loop->entry > loop->lastBlock)
                    loop->lastBlock = loop->entry;

                LoopAnalysis *nloop = tla.new_<LoopAnalysis>();
                if (!nloop) {
                    cx->free_(saved);
                    setOOM(cx);
                    return;
                }
                PodZero(nloop);

                if (loop)
                    loop->hasCallsLoops = true;

                nloop->parent = loop;
                loop = nloop;

                getCode(targetOffset).loop = loop;
                loop->head = targetOffset;
                loop->backedge = offset;
                loop->lastBlock = loop->head;

                



                uint32_t entry = targetOffset;
                if (entry) {
                    do {
                        entry--;
                    } while (!maybeCode(entry));

                    jsbytecode *entrypc = script->code + entry;

                    if (JSOp(*entrypc) == JSOP_GOTO || JSOp(*entrypc) == JSOP_FILTER)
                        loop->entry = entry + GET_JUMP_OFFSET(entrypc);
                    else
                        loop->entry = targetOffset;
                } else {
                    
                    loop->entry = targetOffset;
                }
                JS_ASSERT(script->code[loop->entry] == JSOP_LOOPHEAD ||
                          script->code[loop->entry] == JSOP_LOOPENTRY);
            } else {
                for (unsigned i = 0; i < savedCount; i++) {
                    LifetimeVariable &var = *saved[i];
                    JS_ASSERT(!var.lifetime && var.saved);
                    if (var.live(targetOffset)) {
                        



                        var.lifetime = tla.new_<Lifetime>(offset, var.savedEnd, var.saved);
                        if (!var.lifetime) {
                            cx->free_(saved);
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

    cx->free_(saved);

    ranLifetimes_ = true;
}

#ifdef DEBUG
void
LifetimeVariable::print() const
{
    Lifetime *segment = lifetime ? lifetime : saved;
    while (segment) {
        printf(" (%u,%u%s)", segment->start, segment->end, segment->loopTail ? ",tail" : "");
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
        var.lifetime = cx->typeLifoAlloc().new_<Lifetime>(offset, var.savedEnd, var.saved);
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
        
        if (!var.saved)
            saved[savedCount++] = &var;
        var.saved = cx->typeLifoAlloc().new_<Lifetime>(offset, var.savedEnd, var.saved);
        if (!var.saved) {
            setOOM(cx);
            return;
        }
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
ScriptAnalysis::clearAllocations()
{
    




    for (unsigned i = 0; i < script->length; i++) {
        Bytecode *code = maybeCode(i);
        if (code)
            code->allocation = NULL;
    }
}





void
ScriptAnalysis::analyzeSSA(JSContext *cx)
{
    JS_ASSERT(cx->compartment->activeAnalysis && !ranSSA() && !failed());

    if (!ranLifetimes()) {
        analyzeLifetimes(cx);
        if (failed())
            return;
    }

    LifoAlloc &tla = cx->typeLifoAlloc();
    unsigned maxDepth = script->nslots - script->nfixed;

    



    SSAValueInfo *values = (SSAValueInfo *)
        cx->calloc_((numSlots + maxDepth) * sizeof(SSAValueInfo));
    if (!values) {
        setOOM(cx);
        return;
    }
    struct FreeSSAValues {
        JSContext *cx;
        SSAValueInfo *values;
        FreeSSAValues(JSContext *cx, SSAValueInfo *values) : cx(cx), values(values) {}
        ~FreeSSAValues() { cx->free_(values); }
    } free(cx, values);

    SSAValueInfo *stack = values + numSlots;
    uint32_t stackDepth = 0;

    for (uint32_t slot = ArgSlot(0); slot < numSlots; slot++) {
        if (trackSlot(slot))
            values[slot].v.initInitial(slot);
    }

    





    Vector<uint32_t> branchTargets(cx);

    




    Vector<uint32_t> exceptionTargets(cx);

    uint32_t offset = 0;
    while (offset < script->length) {
        jsbytecode *pc = script->code + offset;
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

        if (js_CodeSpec[op].format & JOF_DECOMPOSE) {
            offset = successorOffset;
            continue;
        }

        unsigned nuses = GetUseCount(script, offset);
        unsigned ndefs = GetDefCount(script, offset);
        JS_ASSERT(stackDepth >= nuses);

        unsigned xuses = ExtendedUse(pc) ? nuses + 1 : nuses;

        if (xuses) {
            code->poppedValues = tla.newArray<SSAValue>(xuses);
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
                



                uint32_t slot = GetBytecodeSlot(script, pc);
                if (trackSlot(slot))
                    code->poppedValues[nuses] = values[slot].v;
                else
                    code->poppedValues[nuses].clear();
            }

            if (xuses) {
                SSAUseChain *useChains = tla.newArray<SSAUseChain>(xuses);
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
            code->pushedUses = tla.newArray<SSAUseChain *>(xdefs);
            if (!code->pushedUses) {
                setOOM(cx);
                return;
            }
            PodZero(code->pushedUses, xdefs);
        }

        stackDepth += ndefs;

        if (BytecodeUpdatesSlot(op)) {
            uint32_t slot = GetBytecodeSlot(script, pc);
            if (trackSlot(slot)) {
                mergeBranchTarget(cx, values[slot], slot, branchTargets, offset);
                mergeExceptionTarget(cx, values[slot].v, slot, exceptionTargets);
                values[slot].v.initWritten(slot, offset);
            }
        }

        switch (op) {
          case JSOP_GETARG:
          case JSOP_GETLOCAL: {
            uint32_t slot = GetBytecodeSlot(script, pc);
            if (trackSlot(slot)) {
                



                stack[stackDepth - 1].v = code->poppedValues[0] = values[slot].v;
            }
            break;
          }

          

          case JSOP_MOREITER:
            stack[stackDepth - 2].v = code->poppedValues[0];
            break;

          case JSOP_INITPROP:
          case JSOP_INITMETHOD:
            stack[stackDepth - 1].v = code->poppedValues[1];
            break;

          case JSOP_INITELEM:
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

          case JSOP_LOOKUPSWITCH: {
            unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            unsigned npairs = GET_UINT16(pc2);
            pc2 += UINT16_LEN;

            while (npairs) {
                pc2 += UINT32_INDEX_LEN;
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                checkBranchTarget(cx, targetOffset, branchTargets, values, stackDepth);
                pc2 += JUMP_OFFSET_LEN;
                npairs--;
            }

            checkBranchTarget(cx, defaultOffset, branchTargets, values, stackDepth);
            break;
          }

          case JSOP_TRY: { 
            JSTryNote *tn = script->trynotes()->vector;
            JSTryNote *tnlimit = tn + script->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script->mainOffset + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    if (tn->kind != JSTRY_ITER) {
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

        uint32_t type = JOF_TYPE(js_CodeSpec[op].format);
        if (type == JOF_JUMP) {
            unsigned targetOffset = FollowBranch(cx, script, offset);
            checkBranchTarget(cx, targetOffset, branchTargets, values, stackDepth);

            



            if (targetOffset < offset)
                freezeNewValues(cx, targetOffset);
        }

        offset = successorOffset;
    }

    ranSSA_ = true;

    






    if (script->analyzedArgsUsage())
        return;

    
    JS_ASSERT(script->function());
    JS_ASSERT(script->mayNeedArgsObj());
    JS_ASSERT(!script->usesEval);

    



    if (localsAliasStack()) {
        script->setNeedsArgsObj(true);
        return;
    }

    










    bool canOptimizeApply = !script->function()->isHeavyweight();
    bool haveOptimizedApply = false;

    jsbytecode *pc;
    for (offset = 0; offset < script->length; offset += GetBytecodeLength(pc)) {
        pc = script->code + offset;

        
        JS_ASSERT_IF(script->strictModeCode, *pc != JSOP_SETARG);

        
        if (JSOp(*pc) != JSOP_ARGUMENTS)
            continue;

        
        if (!maybeCode(offset))
            continue;

        if (SpeculateApplyOptimization(pc) && canOptimizeApply) {
            haveOptimizedApply = true;
            continue;
        }

        Vector<SSAValue> seen(cx);
        if (haveOptimizedApply ||
            !followEscapingArguments(cx, SSAValue::PushedValue(offset, 0), &seen))
        {
            script->setNeedsArgsObj(true);
            return;
        }

        canOptimizeApply = false;
    }

    script->setNeedsArgsObj(false);
}


static inline unsigned
PhiNodeCapacity(unsigned length)
{
    if (length <= 4)
        return 4;

    unsigned log2;
    JS_FLOOR_LOG2(log2, length - 1);
    return 1 << (log2 + 1);
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
        uint32_t slot = StackSlot(script, i);
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
        cx->delete_(pending);
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

    cx->delete_(pending);
}

bool
ScriptAnalysis::followEscapingArguments(JSContext *cx, const SSAValue &v, Vector<SSAValue> *seen)
{
    



    if (!trackUseChain(v))
        return true;

    for (unsigned i = 0; i < seen->length(); i++) {
        if (v == (*seen)[i])
            return true;
    }
    if (!seen->append(v)) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return false;
    }

    SSAUseChain *use = useChain(v);
    while (use) {
        if (!followEscapingArguments(cx, use, seen))
            return false;
        use = use->next;
    }

    return true;
}

bool
ScriptAnalysis::followEscapingArguments(JSContext *cx, SSAUseChain *use, Vector<SSAValue> *seen)
{
    if (!use->popped)
        return followEscapingArguments(cx, SSAValue::PhiValue(use->offset, use->u.phi), seen);

    jsbytecode *pc = script->code + use->offset;
    uint32_t which = use->u.which;

    JSOp op = JSOp(*pc);

    if (op == JSOP_POP || op == JSOP_POPN)
        return true;

    
    if (op == JSOP_GETELEM && which == 1)
        return true;

    
    if (op == JSOP_LENGTH)
        return true;

    

    if (op == JSOP_SETLOCAL) {
        uint32_t slot = GetBytecodeSlot(script, pc);
        if (!trackSlot(slot) || script->strictModeCode)
            return false;
        if (!followEscapingArguments(cx, SSAValue::PushedValue(use->offset, 0), seen))
            return false;
        return followEscapingArguments(cx, SSAValue::WrittenVar(slot, use->offset), seen);
    }

    if (op == JSOP_GETLOCAL)
        return followEscapingArguments(cx, SSAValue::PushedValue(use->offset, 0), seen);

    return false;
}

CrossSSAValue
CrossScriptSSA::foldValue(const CrossSSAValue &cv)
{
    const Frame &frame = getFrame(cv.frame);
    const SSAValue &v = cv.v;

    JSScript *parentScript = NULL;
    ScriptAnalysis *parentAnalysis = NULL;
    if (frame.parent != INVALID_FRAME) {
        parentScript = getFrame(frame.parent).script;
        parentAnalysis = parentScript->analysis();
    }

    if (v.kind() == SSAValue::VAR && v.varInitial() && parentScript) {
        uint32_t slot = v.varSlot();
        if (slot >= ArgSlot(0) && slot < LocalSlot(frame.script, 0)) {
            uint32_t argc = GET_ARGC(frame.parentpc);
            SSAValue argv = parentAnalysis->poppedValue(frame.parentpc, argc - 1 - (slot - ArgSlot(0)));
            return foldValue(CrossSSAValue(frame.parent, argv));
        }
    }

    if (v.kind() == SSAValue::PUSHED) {
        jsbytecode *pc = frame.script->code + v.pushedOffset();

        switch (JSOp(*pc)) {
          case JSOP_THIS:
            if (parentScript) {
                uint32_t argc = GET_ARGC(frame.parentpc);
                SSAValue thisv = parentAnalysis->poppedValue(frame.parentpc, argc);
                return foldValue(CrossSSAValue(frame.parent, thisv));
            }
            break;

          case JSOP_CALL: {
            



            JSScript *callee = NULL;
            uint32_t calleeFrame = INVALID_FRAME;
            for (unsigned i = 0; i < numFrames(); i++) {
                if (iterFrame(i).parent == cv.frame && iterFrame(i).parentpc == pc) {
                    if (callee)
                        return cv;  
                    callee = iterFrame(i).script;
                    calleeFrame = iterFrame(i).index;
                }
            }
            if (callee && callee->analysis()->numReturnSites() == 1) {
                ScriptAnalysis *analysis = callee->analysis();
                uint32_t offset = 0;
                while (offset < callee->length) {
                    jsbytecode *pc = callee->code + offset;
                    if (analysis->maybeCode(pc) && JSOp(*pc) == JSOP_RETURN)
                        return foldValue(CrossSSAValue(calleeFrame, analysis->poppedValue(pc, 0)));
                    offset += GetBytecodeLength(pc);
                }
            }
            break;
          }

          case JSOP_TOID: {
            




            ScriptAnalysis *analysis = frame.script->analysis();
            SSAValue toidv = analysis->poppedValue(pc, 0);
            if (analysis->getValueTypes(toidv)->getKnownTypeTag(cx) == JSVAL_TYPE_INT32)
                return foldValue(CrossSSAValue(cv.frame, toidv));
            break;
          }

          default:;
        }
    }

    return cv;
}

#ifdef DEBUG

void
ScriptAnalysis::printSSA(JSContext *cx)
{
    AutoEnterAnalysis enter(cx);

    printf("\n");

    for (unsigned offset = 0; offset < script->length; offset++) {
        Bytecode *code = maybeCode(offset);
        if (!code)
            continue;

        jsbytecode *pc = script->code + offset;

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

        unsigned nuses = GetUseCount(script, offset);
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
        JS_NOT_REACHED("Bad kind");
    }
}

void
ScriptAnalysis::assertMatchingDebugMode()
{
    JS_ASSERT(!!script->compartment()->debugMode() == !!originalDebugMode_);
}

#endif  

} 
} 
