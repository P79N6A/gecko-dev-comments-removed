





#include "jit/BytecodeAnalysis.h"

#include "jsopcode.h"

#include "jsopcodeinlines.h"

using namespace js;
using namespace js::jit;

BytecodeAnalysis::BytecodeAnalysis(JSScript *script)
  : script_(script),
    infos_(),
    usesScopeChain_(false),
    hasTryFinally_(false)
{
}


struct CatchFinallyRange
{
    uint32_t start; 
    uint32_t end;   

    CatchFinallyRange(uint32_t start, uint32_t end)
      : start(start), end(end)
    {
        JS_ASSERT(end > start);
    }

    bool contains(uint32_t offset) const {
        return start <= offset && offset < end;
    }
};

bool
BytecodeAnalysis::init(JSContext *cx)
{
    if (!infos_.growByUninitialized(script_->length))
        return false;

    jsbytecode *end = script_->code + script_->length;

    
    mozilla::PodZero(infos_.begin(), infos_.length());
    infos_[0].init(0);

    Vector<CatchFinallyRange, 0, IonAllocPolicy> catchFinallyRanges;

    for (jsbytecode *pc = script_->code; pc < end; pc += GetBytecodeLength(pc)) {
        JSOp op = JSOp(*pc);
        unsigned offset = pc - script_->code;

        IonSpew(IonSpew_BaselineOp, "Analyzing op @ %d (end=%d): %s",
                int(pc - script_->code), int(end - script_->code), js_CodeName[op]);

        
        if (!infos_[offset].initialized)
            continue;


        unsigned stackDepth = infos_[offset].stackDepth;
#ifdef DEBUG
        for (jsbytecode *chkpc = pc + 1; chkpc < (pc + GetBytecodeLength(pc)); chkpc++)
            JS_ASSERT(!infos_[chkpc - script_->code].initialized);
#endif

        unsigned nuses = GetUseCount(script_, offset);
        unsigned ndefs = GetDefCount(script_, offset);

        JS_ASSERT(stackDepth >= nuses);
        stackDepth -= nuses;
        stackDepth += ndefs;

        
        JS_ASSERT(stackDepth <= BytecodeInfo::MAX_STACK_DEPTH);

        switch (op) {
          case JSOP_TABLESWITCH: {
            unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
            jsbytecode *pc2 = pc + JUMP_OFFSET_LEN;
            int32_t low = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;
            int32_t high = GET_JUMP_OFFSET(pc2);
            pc2 += JUMP_OFFSET_LEN;

            infos_[defaultOffset].init(stackDepth);
            infos_[defaultOffset].jumpTarget = true;

            for (int32_t i = low; i <= high; i++) {
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (targetOffset != offset) {
                    infos_[targetOffset].init(stackDepth);
                    infos_[targetOffset].jumpTarget = true;
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

                    if (tn->kind != JSTRY_ITER) {
                        infos_[catchOffset].init(stackDepth);
                        infos_[catchOffset].jumpTarget = true;
                    }
                }
            }

            
            
            jssrcnote *sn = js_GetSrcNote(cx, script_, pc);
            JS_ASSERT(SN_TYPE(sn) == SRC_TRY);

            jsbytecode *endOfTry = pc + js_GetSrcNoteOffset(sn, 0);
            JS_ASSERT(JSOp(*endOfTry) == JSOP_GOTO);

            jsbytecode *afterTry = endOfTry + GET_JUMP_OFFSET(endOfTry);
            JS_ASSERT(afterTry > endOfTry);

            
            while (!catchFinallyRanges.empty() && catchFinallyRanges.back().end <= offset)
                catchFinallyRanges.popBack();

            CatchFinallyRange range(endOfTry - script_->code, afterTry - script_->code);
            if (!catchFinallyRanges.append(range))
                return false;
            break;
          }

          case JSOP_LOOPENTRY:
            for (size_t i = 0; i < catchFinallyRanges.length(); i++) {
                if (catchFinallyRanges[i].contains(offset))
                    infos_[offset].loopEntryInCatchOrFinally = true;
            }
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
          case JSOP_DEFFUN:
          case JSOP_DEFVAR:
          case JSOP_DEFCONST:
          case JSOP_SETCONST:
            usesScopeChain_ = true;
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

            
            bool jumpBack = (targetOffset < offset) && !infos_[targetOffset].initialized;

            infos_[targetOffset].init(newStackDepth);
            infos_[targetOffset].jumpTarget = true;

            if (jumpBack)
                pc = script_->code + targetOffset;
        }

        
        if (BytecodeFallsThrough(op)) {
            jsbytecode *nextpc = pc + GetBytecodeLength(pc);
            JS_ASSERT(nextpc < end);
            unsigned nextOffset = nextpc - script_->code;

            infos_[nextOffset].init(stackDepth);

            if (jump)
                infos_[nextOffset].jumpFallthrough = true;

            
            if (jump)
                infos_[nextOffset].jumpTarget = true;
            else
                infos_[nextOffset].fallthrough = true;
        }
    }

    return true;
}
