





#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/PodOperations.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "vm/Opcodes.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"

using mozilla::DebugOnly;
using mozilla::PodCopy;
using mozilla::PodZero;
using mozilla::FloorLog2;

namespace js {
namespace analyze {
class LoopAnalysis;
} 
} 

namespace mozilla {

template <> struct IsPod<js::analyze::LifetimeVariable> : TrueType {};
template <> struct IsPod<js::analyze::LoopAnalysis>     : TrueType {};
template <> struct IsPod<js::analyze::SlotValue>        : TrueType {};
template <> struct IsPod<js::analyze::SSAValue>         : TrueType {};
template <> struct IsPod<js::analyze::SSAUseChain>      : TrueType {};

} 

namespace js {
namespace analyze {






























class Bytecode
{
    friend class ScriptAnalysis;

  public:
    Bytecode() { mozilla::PodZero(this); }

    

    
    bool jumpTarget : 1;

    
    bool fallthrough : 1;

    
    bool jumpFallthrough : 1;

    



    bool unconditional : 1;

    
    bool analyzed : 1;

    
    bool exceptionEntry : 1;

    
    uint32_t stackDepth;

  private:

    
    LoopAnalysis *loop;

    

    
    SSAValue *poppedValues;

    
    SSAUseChain **pushedUses;

    union {
        





        SlotValue *newValues;

        







        Vector<SlotValue> *pendingValues;
    };
};








struct Lifetime
{
    



    uint32_t start;
    uint32_t end;

    



    uint32_t savedEnd;

    



    bool write;

    
    Lifetime *next;

    Lifetime(uint32_t offset, uint32_t savedEnd, Lifetime *next)
        : start(offset), end(offset), savedEnd(savedEnd),
          write(false), next(next)
    {}
};


class LoopAnalysis
{
  public:
    
    LoopAnalysis *parent;

    
    uint32_t head;

    



    uint32_t backedge;
};


struct LifetimeVariable
{
    
    Lifetime *lifetime;

    
    Lifetime *saved;

    
    uint32_t savedEnd : 31;

    
    bool ensured : 1;

    
    Lifetime * live(uint32_t offset) const {
        if (lifetime && lifetime->end >= offset)
            return lifetime;
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= offset) {
            if (segment->end >= offset)
                return segment;
            segment = segment->next;
        }
        return nullptr;
    }

    



    uint32_t firstWrite(uint32_t start, uint32_t end) const {
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= end) {
            if (segment->start >= start && segment->write)
                return segment->start;
            segment = segment->next;
        }
        return UINT32_MAX;
    }
    uint32_t firstWrite(LoopAnalysis *loop) const {
        return firstWrite(loop->head, loop->backedge);
    }

    



    uint32_t onlyWrite(LoopAnalysis *loop) const {
        uint32_t offset = UINT32_MAX;
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= loop->backedge) {
            if (segment->start >= loop->head && segment->write) {
                if (offset != UINT32_MAX)
                    return UINT32_MAX;
                offset = segment->start;
            }
            segment = segment->next;
        }
        return offset;
    }

#ifdef DEBUG
    void print() const;
#endif
};

struct SSAPhiNode;







class SSAValue
{
    friend class ScriptAnalysis;

  public:
    enum Kind {
        EMPTY  = 0, 
        PUSHED = 1, 
        VAR    = 2, 
        PHI    = 3  
    };

    Kind kind() const {
        JS_ASSERT(u.pushed.kind == u.var.kind && u.pushed.kind == u.phi.kind);

        
        return (Kind) (u.pushed.kind & 0x3);
    }

    bool operator==(const SSAValue &o) const {
        return !memcmp(this, &o, sizeof(SSAValue));
    }

    

    uint32_t pushedOffset() const {
        JS_ASSERT(kind() == PUSHED);
        return u.pushed.offset;
    }

    uint32_t pushedIndex() const {
        JS_ASSERT(kind() == PUSHED);
        return u.pushed.index;
    }

    

    bool varInitial() const {
        JS_ASSERT(kind() == VAR);
        return u.var.initial;
    }

    uint32_t varSlot() const {
        JS_ASSERT(kind() == VAR);
        return u.var.slot;
    }

    uint32_t varOffset() const {
        JS_ASSERT(!varInitial());
        return u.var.offset;
    }

    

    uint32_t phiSlot() const;
    uint32_t phiLength() const;
    const SSAValue &phiValue(uint32_t i) const;

    
    uint32_t phiOffset() const {
        JS_ASSERT(kind() == PHI);
        return u.phi.offset;
    }

    SSAPhiNode *phiNode() const {
        JS_ASSERT(kind() == PHI);
        return u.phi.node;
    }

    

#ifdef DEBUG
    void print() const;
#endif

    void clear() {
        mozilla::PodZero(this);
        JS_ASSERT(kind() == EMPTY);
    }

    void initPushed(uint32_t offset, uint32_t index) {
        clear();
        u.pushed.kind = PUSHED;
        u.pushed.offset = offset;
        u.pushed.index = index;
    }

    static SSAValue PushedValue(uint32_t offset, uint32_t index) {
        SSAValue v;
        v.initPushed(offset, index);
        return v;
    }

    void initInitial(uint32_t slot) {
        clear();
        u.var.kind = VAR;
        u.var.initial = true;
        u.var.slot = slot;
    }

    void initWritten(uint32_t slot, uint32_t offset) {
        clear();
        u.var.kind = VAR;
        u.var.initial = false;
        u.var.slot = slot;
        u.var.offset = offset;
    }

    static SSAValue WrittenVar(uint32_t slot, uint32_t offset) {
        SSAValue v;
        v.initWritten(slot, offset);
        return v;
    }

    void initPhi(uint32_t offset, SSAPhiNode *node) {
        clear();
        u.phi.kind = PHI;
        u.phi.offset = offset;
        u.phi.node = node;
    }

    static SSAValue PhiValue(uint32_t offset, SSAPhiNode *node) {
        SSAValue v;
        v.initPhi(offset, node);
        return v;
    }

  private:
    union {
        struct {
            Kind kind : 2;
            uint32_t offset : 30;
            uint32_t index;
        } pushed;
        struct {
            Kind kind : 2;
            bool initial : 1;
            uint32_t slot : 29;
            uint32_t offset;
        } var;
        struct {
            Kind kind : 2;
            uint32_t offset : 30;
            SSAPhiNode *node;
        } phi;
    } u;
};







struct SSAPhiNode
{
    uint32_t slot;
    uint32_t length;
    SSAValue *options;
    SSAUseChain *uses;
    SSAPhiNode() { mozilla::PodZero(this); }
};

inline uint32_t
SSAValue::phiSlot() const
{
    return u.phi.node->slot;
}

inline uint32_t
SSAValue::phiLength() const
{
    JS_ASSERT(kind() == PHI);
    return u.phi.node->length;
}

inline const SSAValue &
SSAValue::phiValue(uint32_t i) const
{
    JS_ASSERT(kind() == PHI && i < phiLength());
    return u.phi.node->options[i];
}

class SSAUseChain
{
  public:
    bool popped : 1;
    uint32_t offset : 31;
    
    union {
        uint32_t which;
        SSAPhiNode *phi;
    } u;
    SSAUseChain *next;

    SSAUseChain() { mozilla::PodZero(this); }
};

class SlotValue
{
  public:
    uint32_t slot;
    SSAValue value;
    SlotValue(uint32_t slot, const SSAValue &value) : slot(slot), value(value) {}
};





#ifdef DEBUG
void
PrintBytecode(JSContext *cx, HandleScript script, jsbytecode *pc)
{
    fprintf(stderr, "#%u:", script->id());
    Sprinter sprinter(cx);
    if (!sprinter.init())
        return;
    js_Disassemble1(cx, script, pc, script->pcToOffset(pc), true, &sprinter);
    fprintf(stderr, "%s", sprinter.string());
}
#endif





static inline bool
ExtendedDef(jsbytecode *pc)
{
    switch ((JSOp)*pc) {
      case JSOP_SETARG:
      case JSOP_SETLOCAL:
        return true;
      default:
        return false;
    }
}





static inline bool
ExtendedUse(jsbytecode *pc)
{
    if (ExtendedDef(pc))
        return true;
    switch ((JSOp)*pc) {
      case JSOP_GETARG:
      case JSOP_GETLOCAL:
        return true;
      default:
        return false;
    }
}

static inline unsigned
FollowBranch(JSContext *cx, JSScript *script, unsigned offset)
{
    




    jsbytecode *pc = script->offsetToPC(offset);
    unsigned targetOffset = offset + GET_JUMP_OFFSET(pc);
    if (targetOffset < offset) {
        jsbytecode *target = script->offsetToPC(targetOffset);
        JSOp nop = JSOp(*target);
        if (nop == JSOP_GOTO)
            return targetOffset + GET_JUMP_OFFSET(target);
    }
    return targetOffset;
}

static inline uint32_t StackSlot(JSScript *script, uint32_t index) {
    return TotalSlots(script) + index;
}

static inline uint32_t GetBytecodeSlot(JSScript *script, jsbytecode *pc)
{
    switch (JSOp(*pc)) {

      case JSOP_GETARG:
      case JSOP_SETARG:
        return ArgSlot(GET_ARGNO(pc));

      case JSOP_GETLOCAL:
      case JSOP_SETLOCAL:
        return LocalSlot(script, GET_LOCALNO(pc));

      case JSOP_THIS:
        return ThisSlot();

      default:
        MOZ_ASSUME_UNREACHABLE("Bad slot opcode");
    }
}





inline bool
ScriptAnalysis::addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump, unsigned *forwardLoop,
                        unsigned stackDepth)
{
    JS_ASSERT(offset < script_->length());

    Bytecode *&code = codeArray[offset];
    if (!code) {
        code = cx->typeLifoAlloc().new_<Bytecode>();
        if (!code) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        code->stackDepth = stackDepth;
    }
    JS_ASSERT(code->stackDepth == stackDepth);

    code->jumpTarget = true;

    if (offset < *currentOffset) {
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

inline bool
ScriptAnalysis::jumpTarget(uint32_t offset)
{
    JS_ASSERT(offset < script_->length());
    return codeArray[offset] && codeArray[offset]->jumpTarget;
}

inline bool
ScriptAnalysis::jumpTarget(const jsbytecode *pc)
{
    return jumpTarget(script_->pcToOffset(pc));
}

inline const SSAValue &
ScriptAnalysis::poppedValue(uint32_t offset, uint32_t which)
{
    JS_ASSERT(which < GetUseCount(script_, offset) +
              (ExtendedUse(script_->offsetToPC(offset)) ? 1 : 0));
    return getCode(offset).poppedValues[which];
}

inline const SSAValue &
ScriptAnalysis::poppedValue(const jsbytecode *pc, uint32_t which)
{
    return poppedValue(script_->pcToOffset(pc), which);
}

inline const SlotValue *
ScriptAnalysis::newValues(uint32_t offset)
{
    JS_ASSERT(offset < script_->length());
    return getCode(offset).newValues;
}

inline const SlotValue *
ScriptAnalysis::newValues(const jsbytecode *pc)
{
    return newValues(script_->pcToOffset(pc));
}

inline bool
ScriptAnalysis::trackUseChain(const SSAValue &v)
{
    JS_ASSERT_IF(v.kind() == SSAValue::VAR, trackSlot(v.varSlot()));
    return v.kind() != SSAValue::EMPTY &&
        (v.kind() != SSAValue::VAR || !v.varInitial());
}




inline SSAUseChain *&
ScriptAnalysis::useChain(const SSAValue &v)
{
    JS_ASSERT(trackUseChain(v));
    if (v.kind() == SSAValue::PUSHED)
        return getCode(v.pushedOffset()).pushedUses[v.pushedIndex()];
    if (v.kind() == SSAValue::VAR)
        return getCode(v.varOffset()).pushedUses[GetDefCount(script_, v.varOffset())];
    return v.phiNode()->uses;
}


inline jsbytecode *
ScriptAnalysis::getCallPC(jsbytecode *pc)
{
    SSAUseChain *uses = useChain(SSAValue::PushedValue(script_->pcToOffset(pc), 0));
    JS_ASSERT(uses && uses->popped);
    JS_ASSERT(js_CodeSpec[script_->code()[uses->offset]].format & JOF_INVOKE);
    return script_->offsetToPC(uses->offset);
}










inline bool
ScriptAnalysis::slotEscapes(uint32_t slot)
{
    JS_ASSERT(script_->compartment()->activeAnalysis);
    if (slot >= numSlots)
        return true;
    return escapedSlots[slot];
}







inline bool
ScriptAnalysis::trackSlot(uint32_t slot)
{
    return !slotEscapes(slot) && canTrackVars && slot < 1000;
}

inline const LifetimeVariable &
ScriptAnalysis::liveness(uint32_t slot)
{
    JS_ASSERT(script_->compartment()->activeAnalysis);
    JS_ASSERT(!slotEscapes(slot));
    return lifetimes[slot];
}

bool
ScriptAnalysis::analyzeBytecode(JSContext *cx)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);
    JS_ASSERT(!codeArray);

    LifoAlloc &alloc = cx->typeLifoAlloc();

    numSlots = TotalSlots(script_);

    unsigned length = script_->length();
    codeArray = alloc.newArray<Bytecode*>(length);
    escapedSlots = alloc.newArray<bool>(numSlots);

    if (!codeArray || !escapedSlots) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    PodZero(codeArray, length);

    











    PodZero(escapedSlots, numSlots);

    bool allVarsAliased = script_->compartment()->debugMode();
    bool allArgsAliased = allVarsAliased || script_->argumentsHasVarBinding();

    RootedScript script(cx, script_);
    for (BindingIter bi(script); bi; bi++) {
        if (bi->kind() == Binding::ARGUMENT)
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
        js_ReportOutOfMemory(cx);
        return false;
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
        jsbytecode *pc = script_->offsetToPC(offset);

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

          case JSOP_DEFFUN:
          case JSOP_DEFVAR:
          case JSOP_DEFCONST:
          case JSOP_SETCONST:
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
                return false;

            for (int32_t i = low; i <= high; i++) {
                unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                if (targetOffset != offset) {
                    if (!addJump(cx, targetOffset, &nextOffset, &forwardJump, &forwardLoop, stackDepth))
                        return false;
                }
                pc2 += JUMP_OFFSET_LEN;
            }
            break;
          }

          case JSOP_TRY: {
            





            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script_->mainOffset() + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    
                    if (catchOffset > forwardCatch)
                        forwardCatch = catchOffset;

                    if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP) {
                        if (!addJump(cx, catchOffset, &nextOffset, &forwardJump, &forwardLoop, stackDepth))
                            return false;
                        getCode(catchOffset).exceptionEntry = true;
                    }
                }
            }
            break;
          }

          case JSOP_GETLOCAL:
          case JSOP_SETLOCAL:
            JS_ASSERT(GET_LOCALNO(pc) < script_->nfixed());
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
                return false;
        }

        
        if (BytecodeFallsThrough(op)) {
            JS_ASSERT(successorOffset < script_->length());

            Bytecode *&nextcode = codeArray[successorOffset];

            if (!nextcode) {
                nextcode = alloc.new_<Bytecode>();
                if (!nextcode) {
                    js_ReportOutOfMemory(cx);
                    return false;
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

    JS_ASSERT(forwardJump == 0 && forwardLoop == 0 && forwardCatch == 0);

    




    if (!script_->analyzedArgsUsage()) {
        if (!analyzeLifetimes(cx))
            return false;
        if (!analyzeSSA(cx))
            return false;

        



        script_->setNeedsArgsObj(needsArgsObj(cx));
    }

    return true;
}





bool
ScriptAnalysis::analyzeLifetimes(JSContext *cx)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);
    JS_ASSERT(codeArray);
    JS_ASSERT(!lifetimes);

    LifoAlloc &alloc = cx->typeLifoAlloc();

    lifetimes = alloc.newArray<LifetimeVariable>(numSlots);
    if (!lifetimes) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    PodZero(lifetimes, numSlots);

    



    LifetimeVariable **saved = cx->pod_calloc<LifetimeVariable*>(numSlots);
    if (!saved) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    unsigned savedCount = 0;

    LoopAnalysis *loop = nullptr;

    uint32_t offset = script_->length() - 1;
    while (offset < script_->length()) {
        Bytecode *code = maybeCode(offset);
        if (!code) {
            offset--;
            continue;
        }

        jsbytecode *pc = script_->offsetToPC(offset);

        JSOp op = (JSOp) *pc;

        if (op == JSOP_LOOPHEAD && code->loop) {
            





            JS_ASSERT(loop == code->loop);
            unsigned backedge = code->loop->backedge;
            for (unsigned i = 0; i < numSlots; i++) {
                if (lifetimes[i].lifetime) {
                    if (!extendVariable(cx, lifetimes[i], offset, backedge))
                        return false;
                }
            }

            loop = loop->parent;
            JS_ASSERT_IF(loop, loop->head < offset);
        }

        if (code->exceptionEntry) {
            DebugOnly<bool> found = false;
            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script_->mainOffset() + tn->start;
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
          case JSOP_GETLOCAL:
          case JSOP_THIS: {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (!slotEscapes(slot)) {
                if (!addVariable(cx, lifetimes[slot], offset, saved, savedCount))
                    return false;
            }
            break;
          }

          case JSOP_SETARG:
          case JSOP_SETLOCAL: {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (!slotEscapes(slot)) {
                if (!killVariable(cx, lifetimes[slot], offset, saved, savedCount))
                    return false;
            }
            break;
          }

          case JSOP_TABLESWITCH:
            
            for (unsigned i = 0; i < savedCount; i++) {
                LifetimeVariable &var = *saved[i];
                uint32_t savedEnd = var.savedEnd;
                var.lifetime = alloc.new_<Lifetime>(offset, savedEnd, var.saved);
                if (!var.lifetime) {
                    js_free(saved);
                    js_ReportOutOfMemory(cx);
                    return false;
                }
                var.saved = nullptr;
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
                JSOp nop = JSOp(script_->code()[targetOffset]);
                JS_ASSERT(nop == JSOP_LOOPHEAD);
#endif

                LoopAnalysis *nloop = alloc.new_<LoopAnalysis>();
                if (!nloop) {
                    js_free(saved);
                    js_ReportOutOfMemory(cx);
                    return false;
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

                    jsbytecode *entrypc = script_->offsetToPC(entry);

                    if (JSOp(*entrypc) == JSOP_GOTO)
                        entry += GET_JUMP_OFFSET(entrypc);
                    else
                        entry = targetOffset;
                } else {
                    
                    entry = targetOffset;
                }
                JS_ASSERT(script_->code()[entry] == JSOP_LOOPHEAD ||
                          script_->code()[entry] == JSOP_LOOPENTRY);
            } else {
                for (unsigned i = 0; i < savedCount; i++) {
                    LifetimeVariable &var = *saved[i];
                    JS_ASSERT(!var.lifetime && var.saved);
                    if (var.live(targetOffset)) {
                        



                        uint32_t savedEnd = var.savedEnd;
                        var.lifetime = alloc.new_<Lifetime>(offset, savedEnd, var.saved);
                        if (!var.lifetime) {
                            js_free(saved);
                            js_ReportOutOfMemory(cx);
                            return false;
                        }
                        var.saved = nullptr;
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

    return true;
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

inline bool
ScriptAnalysis::addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                            LifetimeVariable **&saved, unsigned &savedCount)
{
    if (var.lifetime) {
        if (var.ensured)
            return true;

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
        uint32_t savedEnd = var.savedEnd;
        var.lifetime = cx->typeLifoAlloc().new_<Lifetime>(offset, savedEnd, var.saved);
        if (!var.lifetime) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        var.saved = nullptr;
    }

    return true;
}

inline bool
ScriptAnalysis::killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                             LifetimeVariable **&saved, unsigned &savedCount)
{
    if (!var.lifetime) {
        
        uint32_t savedEnd = var.savedEnd;
        Lifetime *lifetime = cx->typeLifoAlloc().new_<Lifetime>(offset, savedEnd, var.saved);
        if (!lifetime) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        if (!var.saved)
            saved[savedCount++] = &var;
        var.saved = lifetime;
        var.saved->write = true;
        var.savedEnd = 0;
        return true;
    }

    JS_ASSERT_IF(!var.ensured, offset < var.lifetime->start);
    unsigned start = var.lifetime->start;

    




    var.lifetime->start = offset;
    var.lifetime->write = true;

    if (var.ensured) {
        





        var.lifetime = cx->typeLifoAlloc().new_<Lifetime>(start, 0, var.lifetime);
        if (!var.lifetime) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        var.lifetime->end = offset;
    } else {
        var.saved = var.lifetime;
        var.savedEnd = 0;
        var.lifetime = nullptr;

        saved[savedCount++] = &var;
    }

    return true;
}

inline bool
ScriptAnalysis::extendVariable(JSContext *cx, LifetimeVariable &var,
                               unsigned start, unsigned end)
{
    JS_ASSERT(var.lifetime);
    if (var.ensured) {
        




        JS_ASSERT(var.lifetime->start < start);
        return true;
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
                js_ReportOutOfMemory(cx);
                return false;
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
    return true;
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






struct SSAValueInfo
{
    SSAValue v;

    




    int32_t branchSize;
};

bool
ScriptAnalysis::analyzeSSA(JSContext *cx)
{
    JS_ASSERT(cx->compartment()->activeAnalysis);
    JS_ASSERT(codeArray);
    JS_ASSERT(lifetimes);

    LifoAlloc &alloc = cx->typeLifoAlloc();
    unsigned maxDepth = script_->nslots() - script_->nfixed();

    



    SSAValueInfo *values = cx->pod_calloc<SSAValueInfo>(numSlots + maxDepth);
    if (!values) {
        js_ReportOutOfMemory(cx);
        return false;
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
    while (offset < script_->length()) {
        jsbytecode *pc = script_->offsetToPC(offset);
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
                    js_ReportOutOfMemory(cx);
                    return false;
                }
            }

            




            for (unsigned i = 0; i < pending->length(); i++) {
                SlotValue &v = (*pending)[i];
                if (v.slot < numSlots && liveness(v.slot).firstWrite(code->loop) != UINT32_MAX) {
                    if (v.value.kind() != SSAValue::PHI || v.value.phiOffset() != offset) {
                        JS_ASSERT(v.value.phiOffset() < offset);
                        SSAValue ov = v.value;
                        if (!makePhi(cx, v.slot, offset, &ov))
                            return false;
                        if (!insertPhi(cx, ov, v.value))
                            return false;
                        v.value = ov;
                    }
                }
                if (code->fallthrough || code->jumpFallthrough) {
                    if (!mergeValue(cx, offset, values[v.slot].v, &v))
                        return false;
                }
                if (!mergeBranchTarget(cx, values[v.slot], v.slot, branchTargets, offset - 1))
                    return false;
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
                    return false;
                if (code->fallthrough || code->jumpFallthrough) {
                    if (!insertPhi(cx, ov, values[slot].v))
                        return false;
                }
                if (!mergeBranchTarget(cx, values[slot], slot, branchTargets, offset - 1))
                    return false;
                values[slot].v = ov;
                if (!pending->append(SlotValue(slot, ov))) {
                    js_ReportOutOfMemory(cx);
                    return false;
                }
            }
        } else if (code->pendingValues) {
            









            bool exception = getCode(offset).exceptionEntry;
            Vector<SlotValue> *pending = code->pendingValues;
            for (unsigned i = 0; i < pending->length(); i++) {
                SlotValue &v = (*pending)[i];
                if (code->fallthrough || code->jumpFallthrough ||
                    (exception && values[v.slot].v.kind() != SSAValue::EMPTY)) {
                    if (!mergeValue(cx, offset, values[v.slot].v, &v))
                        return false;
                }
                if (!mergeBranchTarget(cx, values[v.slot], v.slot, branchTargets, offset))
                    return false;
                values[v.slot].v = v.value;
            }
            if (!freezeNewValues(cx, offset))
                return false;
        }

        unsigned nuses = GetUseCount(script_, offset);
        unsigned ndefs = GetDefCount(script_, offset);
        JS_ASSERT(stackDepth >= nuses);

        unsigned xuses = ExtendedUse(pc) ? nuses + 1 : nuses;

        if (xuses) {
            code->poppedValues = alloc.newArray<SSAValue>(xuses);
            if (!code->poppedValues) {
                js_ReportOutOfMemory(cx);
                return false;
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
                    js_ReportOutOfMemory(cx);
                    return false;
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
                js_ReportOutOfMemory(cx);
                return false;
            }
            PodZero(code->pushedUses, xdefs);
        }

        stackDepth += ndefs;

        if (op == JSOP_SETARG || op == JSOP_SETLOCAL) {
            uint32_t slot = GetBytecodeSlot(script_, pc);
            if (trackSlot(slot)) {
                if (!mergeBranchTarget(cx, values[slot], slot, branchTargets, offset))
                    return false;
                if (!mergeExceptionTarget(cx, values[slot].v, slot, exceptionTargets))
                    return false;
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

          case JSOP_MUTATEPROTO:
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

          case JSOP_DUPAT: {
            unsigned pickedDepth = GET_UINT24 (pc);
            JS_ASSERT(pickedDepth < stackDepth - 1);
            stack[stackDepth - 1].v = stack[stackDepth - 2 - pickedDepth].v;
            break;
          }

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
                if (targetOffset != offset) {
                    if (!checkBranchTarget(cx, targetOffset, branchTargets, values, stackDepth))
                        return false;
                }
                pc2 += JUMP_OFFSET_LEN;
            }

            if (!checkBranchTarget(cx, defaultOffset, branchTargets, values, stackDepth))
                return false;
            break;
          }

          case JSOP_TRY: {
            JSTryNote *tn = script_->trynotes()->vector;
            JSTryNote *tnlimit = tn + script_->trynotes()->length;
            for (; tn < tnlimit; tn++) {
                unsigned startOffset = script_->mainOffset() + tn->start;
                if (startOffset == offset + 1) {
                    unsigned catchOffset = startOffset + tn->length;

                    if (tn->kind != JSTRY_ITER && tn->kind != JSTRY_LOOP) {
                        if (!checkBranchTarget(cx, catchOffset, branchTargets, values, stackDepth))
                            return false;
                        if (!checkExceptionTarget(cx, catchOffset, exceptionTargets))
                            return false;
                    }
                }
            }
            break;
          }

          case JSOP_THROW:
          case JSOP_RETURN:
          case JSOP_RETRVAL:
            if (!mergeAllExceptionTargets(cx, values, exceptionTargets))
                return false;
            break;

          default:;
        }

        if (IsJumpOpcode(op)) {
            unsigned targetOffset = FollowBranch(cx, script_, offset);
            if (!checkBranchTarget(cx, targetOffset, branchTargets, values, stackDepth))
                return false;

            



            if (targetOffset < offset) {
                if (!freezeNewValues(cx, targetOffset))
                    return false;
            }
        }

        offset = successorOffset;
    }

    return true;
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
        js_ReportOutOfMemory(cx);
        return false;
    }
    node->slot = slot;
    node->options = options;
    pv->initPhi(offset, node);
    return true;
}

bool
ScriptAnalysis::insertPhi(JSContext *cx, SSAValue &phi, const SSAValue &v)
{
    JS_ASSERT(phi.kind() == SSAValue::PHI);
    SSAPhiNode *node = phi.phiNode();

    




    if (node->length <= 8) {
        for (unsigned i = 0; i < node->length; i++) {
            if (v == node->options[i])
                return true;
        }
    }

    if (trackUseChain(v)) {
        SSAUseChain *&uses = useChain(v);

        SSAUseChain *use = cx->typeLifoAlloc().new_<SSAUseChain>();
        if (!use) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        use->popped = false;
        use->offset = phi.phiOffset();
        use->u.phi = node;
        use->next = uses;
        uses = use;
    }

    if (node->length < PhiNodeCapacity(node->length)) {
        node->options[node->length++] = v;
        return true;
    }

    SSAValue *newOptions =
        cx->typeLifoAlloc().newArray<SSAValue>(PhiNodeCapacity(node->length + 1));
    if (!newOptions) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    PodCopy(newOptions, node->options, node->length);
    node->options = newOptions;
    node->options[node->length++] = v;

    return true;
}

inline bool
ScriptAnalysis::mergeValue(JSContext *cx, uint32_t offset, const SSAValue &v, SlotValue *pv)
{
    
    JS_ASSERT(v.kind() != SSAValue::EMPTY && pv->value.kind() != SSAValue::EMPTY);

    if (v == pv->value)
        return true;

    if (pv->value.kind() != SSAValue::PHI || pv->value.phiOffset() < offset) {
        SSAValue ov = pv->value;
        return makePhi(cx, pv->slot, offset, &pv->value)
            && insertPhi(cx, pv->value, v)
            && insertPhi(cx, pv->value, ov);
    }

    JS_ASSERT(pv->value.phiOffset() == offset);
    return insertPhi(cx, pv->value, v);
}

bool
ScriptAnalysis::checkPendingValue(JSContext *cx, const SSAValue &v, uint32_t slot,
                                  Vector<SlotValue> *pending)
{
    JS_ASSERT(v.kind() != SSAValue::EMPTY);

    for (unsigned i = 0; i < pending->length(); i++) {
        if ((*pending)[i].slot == slot)
            return true;
    }

    if (!pending->append(SlotValue(slot, v))) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

bool
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
            if (!mergeValue(cx, targetOffset, values[v.slot].v, &v))
                return false;
        }
    } else {
        pending = cx->new_< Vector<SlotValue> >(cx);
        if (!pending || !branchTargets.append(targetOffset)) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    





    for (unsigned i = 0; i < targetDepth; i++) {
        uint32_t slot = StackSlot(script_, i);
        if (!checkPendingValue(cx, values[slot].v, slot, pending))
            return false;
    }

    return true;
}

bool
ScriptAnalysis::checkExceptionTarget(JSContext *cx, uint32_t catchOffset,
                                     Vector<uint32_t> &exceptionTargets)
{
    JS_ASSERT(getCode(catchOffset).exceptionEntry);

    



    for (unsigned i = 0; i < exceptionTargets.length(); i++) {
        if (exceptionTargets[i] == catchOffset)
            return true;
    }
    if (!exceptionTargets.append(catchOffset)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

bool
ScriptAnalysis::mergeBranchTarget(JSContext *cx, SSAValueInfo &value, uint32_t slot,
                                  const Vector<uint32_t> &branchTargets, uint32_t currentOffset)
{
    if (slot >= numSlots) {
        




        return true;
    }

    JS_ASSERT(trackSlot(slot));

    





    for (int i = branchTargets.length() - 1; i >= value.branchSize; i--) {
        if (branchTargets[i] <= currentOffset)
            continue;

        const Bytecode &code = getCode(branchTargets[i]);

        Vector<SlotValue> *pending = code.pendingValues;
        if (!checkPendingValue(cx, value.v, slot, pending))
            return false;
    }

    value.branchSize = branchTargets.length();
    return true;
}

bool
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
                if (!mergeValue(cx, offset, value, &v))
                    return false;
                break;
            }
        }

        if (!duplicate && !pending->append(SlotValue(slot, value))) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    return true;
}

bool
ScriptAnalysis::mergeAllExceptionTargets(JSContext *cx, SSAValueInfo *values,
                                         const Vector<uint32_t> &exceptionTargets)
{
    for (unsigned i = 0; i < exceptionTargets.length(); i++) {
        Vector<SlotValue> *pending = getCode(exceptionTargets[i]).pendingValues;
        for (unsigned i = 0; i < pending->length(); i++) {
            const SlotValue &v = (*pending)[i];
            if (trackSlot(v.slot)) {
                if (!mergeExceptionTarget(cx, values[v.slot].v, v.slot, exceptionTargets))
                    return false;
            }
        }
    }
    return true;
}

bool
ScriptAnalysis::freezeNewValues(JSContext *cx, uint32_t offset)
{
    Bytecode &code = getCode(offset);

    Vector<SlotValue> *pending = code.pendingValues;
    code.pendingValues = nullptr;

    unsigned count = pending->length();
    if (count == 0) {
        js_delete(pending);
        return true;
    }

    code.newValues = cx->typeLifoAlloc().newArray<SlotValue>(count + 1);
    if (!code.newValues) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    for (unsigned i = 0; i < count; i++)
        code.newValues[i] = (*pending)[i];
    code.newValues[count].slot = 0;
    code.newValues[count].value.clear();

    js_delete(pending);
    return true;
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
    if (!seen.append(v))
        return true;

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

    jsbytecode *pc = script_->offsetToPC(use->offset);
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

    









    if (script_->bindingsAccessedDynamically())
        return false;

    unsigned pcOff = script_->pcToOffset(script_->argumentsBytecode());

    SeenVector seen(cx);
    if (needsArgsObj(cx, seen, SSAValue::PushedValue(pcOff, 0)))
        return true;

    





    if (script_->funHasAnyAliasedFormal() && argumentsContentsObserved_)
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
    for (unsigned offset = 0; offset < script_->length(); offset++) {
        Bytecode *code = maybeCode(offset);
        if (!code)
            continue;

        jsbytecode *pc = script_->offsetToPC(offset);

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

void
ScriptAnalysis::assertMatchingStackDepthAtOffset(uint32_t offset, uint32_t stackDepth) {
    JS_ASSERT_IF(maybeCode(offset), getCode(offset).stackDepth == stackDepth);
}

#endif  

} 
} 
