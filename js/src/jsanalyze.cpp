






































#include "jsanalyze.h"
#include "jsautooplen.h"
#include "jscompartment.h"
#include "jscntxt.h"

#include "jsinferinlines.h"

namespace js {
namespace analyze {





void
Script::destroy()
{
    JS_FinishArenaPool(&pool);
}





bool
Bytecode::mergeDefines(JSContext *cx, Script *script, bool initial,
                       unsigned newDepth, types::TypeStack *newStack,
                       uint32 *newArray, unsigned newCount)
{
    if (initial) {
        



        stackDepth = newDepth;
        defineArray = newArray;
        defineCount = newCount;
#ifdef JS_TYPE_INFERENCE
        inStack = newStack;
#endif
        return true;
    }

#ifdef JS_TYPE_INFERENCE
    types::TypeStack::merge(cx, newStack, inStack);
#endif

    



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
                unsigned stackDepth, types::TypeStack *stack,
                uint32 *defineArray, unsigned defineCount)
{
    JS_ASSERT(offset < script->length);

    Bytecode *&code = codeArray[offset];
    bool initial = (code == NULL);
    if (initial) {
        code = ArenaNew<Bytecode>(pool, this, offset);
        if (!code) {
            setOOM(cx);
            return false;
        }
    }

    if (!code->mergeDefines(cx, this, initial, stackDepth, stack, defineArray, defineCount))
        return false;
    code->jumpTarget = true;

    if (offset < *currentOffset) {
        
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


struct UntrapOpcode
{
    jsbytecode *pc;
    bool trap;

    UntrapOpcode(JSContext *cx, JSScript *script, jsbytecode *pc)
        : pc(pc), trap(JSOp(*pc) == JSOP_TRAP)
    {
        if (trap)
            *pc = JS_GetTrapOpcode(cx, script, pc);
    }

    ~UntrapOpcode()
    {
        if (trap)
            *pc = JSOP_TRAP;
    }
};

#ifdef JS_TYPE_INFERENCE






struct InitializerInfo
{
    
    types::TypeObject *object;

    
    bool isArray;

    
    types::TypeObject *initObject;

    
    bool initArray;

    
    InitializerInfo *outer;

    InitializerInfo() { PodZero(this); }
};

#endif 

void
Script::init(JSScript *script)
{
    this->script = script;
    JS_InitArenaPool(&pool, "script_analyze", 256, 8, NULL);
}

void
Script::analyze(JSContext *cx)
{
    JS_ASSERT(script && !codeArray && !locals);

    unsigned length = script->length;
    unsigned nfixed = localCount();

    codeArray = ArenaArray<Bytecode*>(pool, length);
    locals = ArenaArray<uint32>(pool, nfixed);

    if (!codeArray || !locals) {
        setOOM(cx);
        return;
    }

    PodZero(codeArray, length);

    for (unsigned i = 0; i < nfixed; i++)
        locals[i] = LOCAL_CONDITIONALLY_DEFINED;

    




    if (script->usesEval || cx->compartment->debugMode) {
        for (uint32 i = 0; i < nfixed; i++)
            setLocal(i, LOCAL_USE_BEFORE_DEF);
    }

    for (uint32 i = 0; i < script->nClosedVars; i++) {
        uint32 slot = script->getClosedVar(i);
        if (slot < nfixed)
            setLocal(slot, LOCAL_USE_BEFORE_DEF);
    }

    




    unsigned forwardJump = 0;

    



    unsigned forwardCatch = 0;

#ifdef JS_TYPE_INFERENCE
    
    InitializerInfo *initializerStack = NULL;
#endif

    
    Bytecode *startcode = ArenaNew<Bytecode>(pool, this, 0);
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
            defineArray = NULL;
            defineCount = 0;
        }

        unsigned nuses = GetUseCount(script, offset);
        unsigned ndefs = GetDefCount(script, offset);

        JS_ASSERT(stackDepth >= nuses);
        stackDepth -= nuses;
        stackDepth += ndefs;

        types::TypeStack *stack = NULL;

#ifdef JS_TYPE_INFERENCE

        stack = code->inStack;

        for (unsigned i = 0; i < nuses; i++)
            stack = stack->group()->innerStack;

        code->pushedArray = ArenaArray<types::TypeStack>(pool, ndefs);
        PodZero(code->pushedArray, ndefs);

        for (unsigned i = 0; i < ndefs; i++) {
            code->pushedArray[i].types.setPool(&pool);
            code->pushedArray[i].setInnerStack(stack);
            stack = &code->pushedArray[i];

#ifdef JS_TYPES_DEBUG_SPEW
            fprintf(cx->typeOut(), "pushed #%u:%05u %u T%u\n",
                    id, offset, i, stack->types.id);
#endif
        }

        
        if (op == JSOP_NEWINIT) {
            int i = GET_UINT16(pc);
            JS_ASSERT(i == JSProto_Array || i == JSProto_Object);
            bool newArray = (i == JSProto_Array);

            types::TypeObject *object;
            if (initializerStack && initializerStack->initObject &&
                initializerStack->initArray == newArray) {
                object = code->initObject = initializerStack->initObject;
            } else {
                object = code->getInitObject(cx, newArray);

                if (initializerStack && initializerStack->isArray) {
                    initializerStack->initObject = object;
                    initializerStack->initArray = newArray;
                }
            }

            InitializerInfo *info = (InitializerInfo *) cx->calloc(sizeof(InitializerInfo));
            info->outer = initializerStack;
            info->object = object;
            info->isArray = newArray;
            initializerStack = info;
        } else if (op == JSOP_INITELEM || op == JSOP_INITPROP || op == JSOP_INITMETHOD) {
            JS_ASSERT(initializerStack);
            code->initObject = initializerStack->object;
        } else if (op == JSOP_NEWARRAY) {
            if (initializerStack && initializerStack->initObject &&
                initializerStack->initArray) {
                code->initObject = initializerStack->initObject;
            } else {
                code->getInitObject(cx, true);
            }
        } else if (op == JSOP_ENDINIT) {
            JS_ASSERT(initializerStack);
            InitializerInfo *info = initializerStack;
            initializerStack = initializerStack->outer;
            cx->free(info);
        }

#endif 

        switch (op) {

          case JSOP_SETRVAL:
          case JSOP_POPV:
            usesRval = true;
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
            break;

          
          case JSOP_GOSUB:
          case JSOP_GOSUBX:
          case JSOP_IFPRIMTOP:
          case JSOP_FILTER:
          case JSOP_ENDFILTER:
          case JSOP_TABLESWITCHX:
          case JSOP_LOOKUPSWITCHX:
            hadFailure = true;
            return;

          case JSOP_TABLESWITCH: {
            jsbytecode *pc2 = pc;
            unsigned defaultOffset = offset + GetJumpOffset(pc, pc2);
            pc2 += JUMP_OFFSET_LEN;
            jsint low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            jsint high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump,
                         stackDepth, stack, defineArray, defineCount)) {
                return;
            }

            for (jsint i = low; i <= high; i++) {
                unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
                if (targetOffset != offset) {
                    if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                                 stackDepth, stack, defineArray, defineCount)) {
                        return;
                    }
                }
                pc2 += JUMP_OFFSET_LEN;
            }
            break;
          }

          case JSOP_LOOKUPSWITCH: {
            jsbytecode *pc2 = pc;
            unsigned defaultOffset = offset + GetJumpOffset(pc, pc2);
            pc2 += JUMP_OFFSET_LEN;
            unsigned npairs = GET_UINT16(pc2);
            pc2 += UINT16_LEN;

            if (!addJump(cx, defaultOffset, &nextOffset, &forwardJump,
                         stackDepth, stack, defineArray, defineCount)) {
                return;
            }

            while (npairs) {
                pc2 += INDEX_LEN;
                unsigned targetOffset = offset + GetJumpOffset(pc, pc2);
                if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                             stackDepth, stack, defineArray, defineCount)) {
                    return;
                }
                pc2 += JUMP_OFFSET_LEN;
                npairs--;
            }
            break;
          }

          case JSOP_TRY: {
            





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
                                     stackDepth, stack, defineArray, defineCount)) {
                            return;
                        }
                        getCode(catchOffset).exceptionEntry = true;
                    }
                }
            }
            break;
          }

          case JSOP_GETLOCAL:
            




            if (pc[JSOP_GETLOCAL_LENGTH] != JSOP_POP) {
                uint32 local = GET_SLOTNO(pc);
                if (local < nfixed && !localDefined(local, offset))
                    setLocal(local, LOCAL_USE_BEFORE_DEF);
            }
            break;

          case JSOP_CALLLOCAL:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC: {
            uint32 local = GET_SLOTNO(pc);
            if (local < nfixed && !localDefined(local, offset))
                setLocal(local, LOCAL_USE_BEFORE_DEF);
            break;
          }

          case JSOP_SETLOCAL:
          case JSOP_FORLOCAL: {
            uint32 local = GET_SLOTNO(pc);
            JS_ASSERT_IF(local < nfixed &&
                         locals[local] != LOCAL_CONDITIONALLY_DEFINED &&
                         locals[local] != LOCAL_USE_BEFORE_DEF,
                         locals[local] <= offset);
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

          default:
            break;
        }

        uint32 type = JOF_TYPE(js_CodeSpec[op].format);

        
        if (type == JOF_JUMP || type == JOF_JUMPX) {
            
            unsigned newStackDepth = stackDepth;
            types::TypeStack *newStack = stack;

            switch (op) {
              case JSOP_OR:
              case JSOP_AND:
              case JSOP_ORX:
              case JSOP_ANDX:
                




                stackDepth--;
#ifdef JS_TYPE_INFERENCE
                stack = stack->group()->innerStack;
#endif
                break;

              case JSOP_CASE:
              case JSOP_CASEX:
                
                newStackDepth--;
#ifdef JS_TYPE_INFERENCE
                newStack = newStack->group()->innerStack;
#endif
                break;

              default:;
            }

            unsigned targetOffset = offset + GetJumpOffset(pc, pc);
            if (!addJump(cx, targetOffset, &nextOffset, &forwardJump,
                         newStackDepth, newStack, defineArray, defineCount)) {
                return;
            }
        }

        
        if (!BytecodeNoFallThrough(op)) {
            JS_ASSERT(successorOffset < script->length);

            Bytecode *&nextcode = codeArray[successorOffset];
            bool initial = (nextcode == NULL);

            if (initial) {
                nextcode = ArenaNew<Bytecode>(pool, this, successorOffset);
                if (!nextcode) {
                    setOOM(cx);
                    return;
                }
            }

            if (!nextcode->mergeDefines(cx, this, initial, stackDepth, stack,
                                        defineArray, defineCount)) {
                return;
            }
        }
    }

    JS_ASSERT(!failed());
    JS_ASSERT(forwardJump == 0 && forwardCatch == 0);

#ifdef JS_TYPE_INFERENCE
    
    offset = 0;
    while (offset < script->length) {
        analyze::Bytecode *code = maybeCode(offset);

        jsbytecode *pc = script->code + offset;
        offset += GetBytecodeLength(pc);

        if (code && code->analyzed)
            analyzeTypes(cx, code);
    }
#endif
}

} 
} 
