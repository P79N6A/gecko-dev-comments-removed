







































#ifndef jsanalyze_h___
#define jsanalyze_h___

#include "jsarena.h"
#include "jscompartment.h"
#include "jscntxt.h"
#include "jsinfer.h"
#include "jsscript.h"

struct JSScript;


namespace js { namespace mjit { struct RegisterAllocation; } }

namespace js {
namespace analyze {





























class SSAValue;
struct SSAUseChain;
struct LoopAnalysis;
struct SlotValue;


class Bytecode
{
    friend class ScriptAnalysis;

  public:
    Bytecode() { PodZero(this); }

    

    
    bool jumpTarget : 1;

    
    bool loopHead : 1;

    
    bool fallthrough : 1;

    
    bool jumpFallthrough : 1;

    
    bool switchTarget : 1;

    



    bool unconditional : 1;

    
    bool analyzed : 1;

    
    bool exceptionEntry : 1;

    
    bool inTryBlock : 1;

    
    bool safePoint : 1;

    



    bool monitoredTypes : 1;

    
    uint32 stackDepth;

  private:
    



    uint32 defineCount;
    uint32 *defineArray;

    

    
    LoopAnalysis *loop;

    
    mjit::RegisterAllocation *allocation;

    

    
    SSAValue *poppedValues;

    
    SSAUseChain **pushedUses;

    union {
        





        SlotValue *newValues;

        







        Vector<SlotValue> *pendingValues;
    };

    

    
    types::TypeSet *pushedTypes;

    
    types::TypeBarrier *typeBarriers;

    

    bool mergeDefines(JSContext *cx, ScriptAnalysis *script, bool initial,
                      uint32 newDepth, uint32 *newArray, uint32 newCount);

    
    bool isDefined(uint32 slot)
    {
        JS_ASSERT(analyzed);
        for (unsigned i = 0; i < defineCount; i++) {
            if (defineArray[i] == slot)
                return true;
        }
        return false;
    }
};

static inline unsigned
GetBytecodeLength(jsbytecode *pc)
{
    JSOp op = (JSOp)*pc;
    JS_ASSERT(op < JSOP_LIMIT);
    JS_ASSERT(op != JSOP_TRAP);

    if (js_CodeSpec[op].length != -1)
        return js_CodeSpec[op].length;
    return js_GetVariableBytecodeLength(pc);
}

static inline unsigned
GetDefCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;
    if (js_CodeSpec[*pc].ndefs == -1)
        return js_GetEnterBlockStackDefs(NULL, script, pc);

    



    switch (JSOp(*pc)) {
      case JSOP_OR:
      case JSOP_ORX:
      case JSOP_AND:
      case JSOP_ANDX:
        return 1;
      case JSOP_FILTER:
        return 2;
      default:
        return js_CodeSpec[*pc].ndefs;
    }
}

static inline unsigned
GetUseCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;
    if (js_CodeSpec[*pc].nuses == -1)
        return js_GetVariableStackUses(JSOp(*pc), pc);
    return js_CodeSpec[*pc].nuses;
}





static inline bool
ExtendedDef(jsbytecode *pc)
{
    switch ((JSOp)*pc) {
      case JSOP_SETARG:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_FORARG:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
      case JSOP_FORLOCAL:
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
      case JSOP_CALLARG:
      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL:
        return true;
      default:
        return false;
    }
}

static inline ptrdiff_t
GetJumpOffset(jsbytecode *pc, jsbytecode *pc2)
{
    uint32 type = JOF_OPTYPE(*pc);
    if (JOF_TYPE_IS_EXTENDED_JUMP(type))
        return GET_JUMPX_OFFSET(pc2);
    return GET_JUMP_OFFSET(pc2);
}

static inline unsigned
FollowBranch(JSScript *script, unsigned offset)
{
    




    jsbytecode *pc = script->code + offset;
    unsigned targetOffset = offset + GetJumpOffset(pc, pc);
    if (targetOffset < offset) {
        JSOp nop = JSOp(script->code[targetOffset]);
        if (nop == JSOP_GOTO || nop == JSOP_GOTOX) {
            jsbytecode *target = script->code + targetOffset;
            return targetOffset + GetJumpOffset(target, target);
        }
    }
    return targetOffset;
}

static inline JSOp
ReverseCompareOp(JSOp op)
{
    switch (op) {
      case JSOP_GT:
        return JSOP_LT;
      case JSOP_GE:
        return JSOP_LE;
      case JSOP_LT:
        return JSOP_GT;
      case JSOP_LE:
        return JSOP_GE;
      default:
        JS_NOT_REACHED("unrecognized op");
        return op;
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

    void retrap()
    {
        if (trap) {
            *pc = JSOP_TRAP;
            trap = false;
        }
    }

    ~UntrapOpcode()
    {
        if (trap)
            *pc = JSOP_TRAP;
    }
};


static inline uint32 CalleeSlot() {
    return 0;
}
static inline uint32 ThisSlot() {
    return 1;
}
static inline uint32 ArgSlot(uint32 arg) {
    return 2 + arg;
}
static inline uint32 LocalSlot(JSScript *script, uint32 local) {
    return 2 + (script->fun ? script->fun->nargs : 0) + local;
}
static inline uint32 TotalSlots(JSScript *script) {
    return LocalSlot(script, 0) + script->nfixed;
}

static inline uint32 StackSlot(JSScript *script, uint32 index) {
    return TotalSlots(script) + index;
}

static inline uint32 GetBytecodeSlot(JSScript *script, jsbytecode *pc)
{
    switch (JSOp(*pc)) {

      case JSOP_GETARG:
      case JSOP_CALLARG:
      case JSOP_SETARG:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_FORARG:
        return ArgSlot(GET_SLOTNO(pc));

      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
      case JSOP_FORLOCAL:
        return LocalSlot(script, GET_SLOTNO(pc));

      case JSOP_THIS:
        return ThisSlot();

      default:
        JS_NOT_REACHED("Bad slot opcode");
        return 0;
    }
}

static inline int32
GetBytecodeInteger(jsbytecode *pc)
{
    switch (JSOp(*pc)) {
      case JSOP_ZERO:   return 0;
      case JSOP_ONE:    return 1;
      case JSOP_UINT16: return GET_UINT16(pc);
      case JSOP_UINT24: return GET_UINT24(pc);
      case JSOP_INT8:   return GET_INT8(pc);
      case JSOP_INT32:  return GET_INT32(pc);
      default:
        JS_NOT_REACHED("Bad op");
        return 0;
    }
}








struct Lifetime
{
    



    uint32 start;
    uint32 end;

    



    uint32 savedEnd;

    




    bool loopTail;

    



    bool write;

    
    Lifetime *next;

    Lifetime(uint32 offset, uint32 savedEnd, Lifetime *next)
        : start(offset), end(offset), savedEnd(savedEnd),
          loopTail(false), write(false), next(next)
    {}
};


struct LoopAnalysis
{
    
    LoopAnalysis *parent;

    
    uint32 head;

    



    uint32 backedge;

    
    uint32 entry;

    





    uint32 lastBlock;

    



    bool hasSafePoints;

    
    bool hasCallsLoops;
};


struct LifetimeVariable
{
    
    Lifetime *lifetime;

    
    Lifetime *saved;

    
    uint32 savedEnd;

    
    Lifetime * live(uint32 offset) const {
        if (lifetime && lifetime->end >= offset)
            return lifetime;
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= offset) {
            if (segment->end >= offset)
                return segment;
            segment = segment->next;
        }
        return NULL;
    }

    



    uint32 firstWrite(uint32 start, uint32 end) const {
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= end) {
            if (segment->start >= start && segment->write)
                return segment->start;
            segment = segment->next;
        }
        return uint32(-1);
    }
    uint32 firstWrite(LoopAnalysis *loop) const {
        return firstWrite(loop->head, loop->backedge);
    }

    
    bool nonDecreasing(JSScript *script, LoopAnalysis *loop) const {
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= loop->backedge) {
            if (segment->start >= loop->head && segment->write) {
                switch (JSOp(script->code[segment->start])) {
                  case JSOP_INCLOCAL:
                  case JSOP_LOCALINC:
                  case JSOP_INCARG:
                  case JSOP_ARGINC:
                    break;
                  default:
                    return false;
                }
            }
            segment = segment->next;
        }
        return true;
    }

    



    uint32 onlyWrite(LoopAnalysis *loop) const {
        uint32 offset = uint32(-1);
        Lifetime *segment = lifetime ? lifetime : saved;
        while (segment && segment->start <= loop->backedge) {
            if (segment->start >= loop->head && segment->write) {
                if (offset != uint32(-1))
                    return uint32(-1);
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

    bool equals(const SSAValue &o) const {
        return !memcmp(this, &o, sizeof(SSAValue));
    }

    

    uint32 pushedOffset() const {
        JS_ASSERT(kind() == PUSHED);
        return u.pushed.offset;
    }

    uint32 pushedIndex() const {
        JS_ASSERT(kind() == PUSHED);
        return u.pushed.index;
    }

    

    bool varInitial() const {
        JS_ASSERT(kind() == VAR);
        return u.var.initial;
    }

    uint32 varSlot() const {
        JS_ASSERT(kind() == VAR);
        return u.var.slot;
    }

    uint32 varOffset() const {
        JS_ASSERT(!varInitial());
        return u.var.offset;
    }

    

    uint32 phiSlot() const;
    uint32 phiLength() const;
    const SSAValue &phiValue(uint32 i) const;
    types::TypeSet *phiTypes() const;

    
    uint32 phiOffset() const {
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
        PodZero(this);
        JS_ASSERT(kind() == EMPTY);
    }

    void initPushed(uint32 offset, uint32 index) {
        clear();
        u.pushed.kind = PUSHED;
        u.pushed.offset = offset;
        u.pushed.index = index;
    }

    static SSAValue PushedValue(uint32 offset, uint32 index) {
        SSAValue v;
        v.initPushed(offset, index);
        return v;
    }

    void initInitial(uint32 slot) {
        clear();
        u.var.kind = VAR;
        u.var.initial = true;
        u.var.slot = slot;
    }

    void initWritten(uint32 slot, uint32 offset) {
        clear();
        u.var.kind = VAR;
        u.var.initial = false;
        u.var.slot = slot;
        u.var.offset = offset;
    }

    void initPhi(uint32 offset, SSAPhiNode *node) {
        clear();
        u.phi.kind = PHI;
        u.phi.offset = offset;
        u.phi.node = node;
    }

  private:
    union {
        struct {
            Kind kind : 2;
            uint32 offset : 30;
            uint32 index;
        } pushed;
        struct {
            Kind kind : 2;
            bool initial : 1;
            uint32 slot : 29;
            uint32 offset;
        } var;
        struct {
            Kind kind : 2;
            uint32 offset : 30;
            SSAPhiNode *node;
        } phi;
    } u;
};







struct SSAPhiNode
{
    types::TypeSet types;
    uint32 slot;
    uint32 length;
    SSAValue *options;
    SSAUseChain *uses;
    SSAPhiNode() { PodZero(this); }
};

inline uint32
SSAValue::phiSlot() const
{
    return u.phi.node->slot;
}

inline uint32
SSAValue::phiLength() const
{
    JS_ASSERT(kind() == PHI);
    return u.phi.node->length;
}

inline const SSAValue &
SSAValue::phiValue(uint32 i) const
{
    JS_ASSERT(kind() == PHI && i < phiLength());
    return u.phi.node->options[i];
}

inline types::TypeSet *
SSAValue::phiTypes() const
{
    JS_ASSERT(kind() == PHI);
    return &u.phi.node->types;
}

struct SSAUseChain
{
    bool popped : 1;
    uint32 offset : 31;
    union {
        uint32 which;
        SSAPhiNode *phi;
    } u;
    SSAUseChain *next;

    SSAUseChain() { PodZero(this); }
};

struct SlotValue
{
    uint32 slot;
    SSAValue value;
    SlotValue(uint32 slot, const SSAValue &value) : slot(slot), value(value) {}
};


class ScriptAnalysis
{
    friend class Bytecode;

    JSScript *script;

    Bytecode **codeArray;

    uint32 numSlots;

    bool outOfMemory;
    bool hadFailure;

    JSPackedBool *escapedSlots;

    
    bool ranBytecode_;
    bool ranSSA_;
    bool ranLifetimes_;
    bool ranInference_;

    

    bool usesRval;
    bool usesScope;
    bool usesThis;
    bool hasCalls;
    bool canTrackVars;
    bool isInlineable;
    uint32 numReturnSites_;

    
    uint32 *definedLocals;

    static const uint32 LOCAL_USE_BEFORE_DEF = uint32(-1);
    static const uint32 LOCAL_CONDITIONALLY_DEFINED = uint32(-2);

    

    LifetimeVariable *lifetimes;

  public:

    ScriptAnalysis(JSScript *script) { PodZero(this); this->script = script; }

    bool ranBytecode() { return ranBytecode_; }
    bool ranSSA() { return ranSSA_; }
    bool ranLifetimes() { return ranLifetimes_; }
    bool ranInference() { return ranInference_; }

    void analyzeBytecode(JSContext *cx);
    void analyzeSSA(JSContext *cx);
    void analyzeLifetimes(JSContext *cx);
    void analyzeTypes(JSContext *cx);

    
    void analyzeTypesNew(JSContext *cx);

    bool OOM() { return outOfMemory; }
    bool failed() { return hadFailure; }
    bool inlineable(uint32 argc) { return isInlineable && argc == script->fun->nargs; }

    
    bool usesReturnValue() const { return usesRval; }

    
    bool usesScopeChain() const { return usesScope; }

    bool usesThisValue() const { return usesThis; }
    bool hasFunctionCalls() const { return hasCalls; }
    uint32 numReturnSites() const { return numReturnSites_; }

    

    Bytecode& getCode(uint32 offset) {
        JS_ASSERT(script->compartment->activeAnalysis);
        JS_ASSERT(offset < script->length);
        JS_ASSERT(codeArray[offset]);
        return *codeArray[offset];
    }
    Bytecode& getCode(const jsbytecode *pc) { return getCode(pc - script->code); }

    Bytecode* maybeCode(uint32 offset) {
        JS_ASSERT(script->compartment->activeAnalysis);
        JS_ASSERT(offset < script->length);
        return codeArray[offset];
    }
    Bytecode* maybeCode(const jsbytecode *pc) { return maybeCode(pc - script->code); }

    bool jumpTarget(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return codeArray[offset] && codeArray[offset]->jumpTarget;
    }
    bool jumpTarget(const jsbytecode *pc) { return jumpTarget(pc - script->code); }

    bool popGuaranteed(jsbytecode *pc) {
        jsbytecode *next = pc + GetBytecodeLength(pc);
        return JSOp(*next) == JSOP_POP && !jumpTarget(next);
    }

    bool incrementInitialValueObserved(jsbytecode *pc) {
        const JSCodeSpec *cs = &js_CodeSpec[*pc];
        return (cs->format & JOF_POST) && !popGuaranteed(pc);
    }

    const SSAValue &poppedValue(uint32 offset, uint32 which) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT_IF(script->code[offset] != JSOP_TRAP,
                     which < GetUseCount(script, offset) +
                     (ExtendedUse(script->code + offset) ? 1 : 0));
        return getCode(offset).poppedValues[which];
    }
    const SSAValue &poppedValue(const jsbytecode *pc, uint32 which) {
        return poppedValue(pc - script->code, which);
    }

    const SlotValue *newValues(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return getCode(offset).newValues;
    }
    const SlotValue *newValues(const jsbytecode *pc) { return newValues(pc - script->code); }

    types::TypeSet *pushedTypes(uint32 offset, uint32 which = 0) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT_IF(script->code[offset] != JSOP_TRAP,
                     which < GetDefCount(script, offset) +
                     (ExtendedDef(script->code + offset) ? 1 : 0));
        types::TypeSet *array = (types::TypeSet *) (~0x1 & (size_t) getCode(offset).pushedTypes);
        JS_ASSERT(array);
        return array + which;
    }
    types::TypeSet *pushedTypes(const jsbytecode *pc, uint32 which) {
        return pushedTypes(pc - script->code, which);
    }

    types::TypeBarrier *typeBarriers(uint32 offset) {
        if (getCode(offset).typeBarriers)
            pruneTypeBarriers(offset);
        return getCode(offset).typeBarriers;
    }
    types::TypeBarrier *typeBarriers(const jsbytecode *pc) {
        return typeBarriers(pc - script->code);
    }

    inline void addPushedType(JSContext *cx, uint32 offset, uint32 which, types::jstype type);

    types::TypeSet *getValueTypes(const SSAValue &v) {
        switch (v.kind()) {
          case SSAValue::PUSHED:
            return pushedTypes(v.pushedOffset(), v.pushedIndex());
          case SSAValue::VAR:
            JS_ASSERT(!slotEscapes(v.varSlot()));
            if (v.varInitial()) {
                return script->slotTypes(v.varSlot());
            } else {
                





                switch (script->code[v.varOffset()]) {
                  case JSOP_FORARG:
                  case JSOP_FORLOCAL:
                    return pushedTypes(v.varOffset(), 1);
                  default:
                    return pushedTypes(v.varOffset(), 0);
                }
            }
          case SSAValue::PHI:
            return &v.phiNode()->types;
          default:
            
            JS_NOT_REACHED("Bad SSA value");
            return NULL;
        }
    }

    types::TypeSet *poppedTypes(uint32 offset, uint32 which) {
        return getValueTypes(poppedValue(offset, which));
    }
    types::TypeSet *poppedTypes(const jsbytecode *pc, uint32 which) {
        return getValueTypes(poppedValue(pc, which));
    }

    bool trackUseChain(const SSAValue &v) {
        return v.kind() != SSAValue::EMPTY &&
            (v.kind() != SSAValue::VAR || !v.varInitial());
    }

    SSAUseChain *& useChain(const SSAValue &v) {
        JS_ASSERT(trackUseChain(v));
        if (v.kind() == SSAValue::PUSHED)
            return getCode(v.pushedOffset()).pushedUses[v.pushedIndex()];
        if (v.kind() == SSAValue::VAR)
            return getCode(v.varOffset()).pushedUses[GetDefCount(script, v.varOffset())];
        return v.phiNode()->uses;
    }

    mjit::RegisterAllocation *&getAllocation(uint32 offset) {
        JS_ASSERT(offset < script->length);
        return getCode(offset).allocation;
    }
    mjit::RegisterAllocation *&getAllocation(const jsbytecode *pc) {
        return getAllocation(pc - script->code);
    }

    LoopAnalysis *getLoop(uint32 offset) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT(getCode(offset).loop);
        return getCode(offset).loop;
    }
    LoopAnalysis *getLoop(const jsbytecode *pc) { return getLoop(pc - script->code); }

    

    bool localHasUseBeforeDef(uint32 local) {
        JS_ASSERT(!failed());
        return slotEscapes(LocalSlot(script, local)) ||
            definedLocals[local] == LOCAL_USE_BEFORE_DEF;
    }

    
    bool localDefined(uint32 local, uint32 offset) {
        return localHasUseBeforeDef(local) || (definedLocals[local] <= offset) ||
            getCode(offset).isDefined(local);
    }
    bool localDefined(uint32 local, jsbytecode *pc) {
        return localDefined(local, pc - script->code);
    }

    bool slotEscapes(uint32 slot) {
        JS_ASSERT(script->compartment->activeAnalysis);
        if (slot >= numSlots)
            return true;
        return escapedSlots[slot];
    }

    





    bool trackSlot(uint32 slot) { return !slotEscapes(slot) && canTrackVars; }

    const LifetimeVariable & liveness(uint32 slot) {
        JS_ASSERT(script->compartment->activeAnalysis);
        JS_ASSERT(!slotEscapes(slot));
        return lifetimes[slot];
    }

    void printSSA(JSContext *cx);
    void printTypes(JSContext *cx);

    void clearAllocations();

  private:
    void setOOM(JSContext *cx) {
        if (!outOfMemory)
            js_ReportOutOfMemory(cx);
        outOfMemory = true;
        hadFailure = true;
    }

    
    inline bool addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump,
                        unsigned stackDepth, uint32 *defineArray, unsigned defineCount);
    inline void setLocal(uint32 local, uint32 offset);

    
    inline void addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                            LifetimeVariable **&saved, unsigned &savedCount);
    inline void killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                             LifetimeVariable **&saved, unsigned &savedCount);
    inline void extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end);

    
    bool makePhi(JSContext *cx, uint32 slot, uint32 offset, SSAValue *pv);
    void insertPhi(JSContext *cx, SSAValue &phi, const SSAValue &v);
    void mergeValue(JSContext *cx, uint32 offset, const SSAValue &v, SlotValue *pv);
    void checkPendingValue(JSContext *cx, const SSAValue &v, uint32 slot,
                           Vector<SlotValue> *pending);
    void checkBranchTarget(JSContext *cx, uint32 targetOffset, Vector<uint32> &branchTargets,
                           SSAValue *values, uint32 stackDepth);
    void mergeBranchTarget(JSContext *cx, const SSAValue &value, uint32 slot,
                           const Vector<uint32> &branchTargets);
    void removeBranchTarget(Vector<uint32> &branchTargets, uint32 offset);
    void freezeNewValues(JSContext *cx, uint32 offset);

    struct TypeInferenceState {
        Vector<SSAPhiNode *> phiNodes;
        bool hasGetSet;
        bool hasHole;
        TypeInferenceState(JSContext *cx)
            : phiNodes(cx), hasGetSet(false), hasHole(false)
        {}
    };

    
    bool analyzeTypesBytecode(JSContext *cx, unsigned offset, TypeInferenceState &state);
    inline void setForTypes(JSContext *cx, jsbytecode *pc, types::TypeSet *types);
    void pruneTypeBarriers(uint32 offset);
};


struct AutoEnterAnalysis
{
    JSContext *cx;
    bool oldActiveAnalysis;

    AutoEnterAnalysis(JSContext *cx)
        : cx(cx), oldActiveAnalysis(cx->compartment->activeAnalysis)
    {
        cx->compartment->activeAnalysis = true;
    }

    ~AutoEnterAnalysis()
    {
        cx->compartment->activeAnalysis = oldActiveAnalysis;
    }
};


struct CrossSSAValue
{
    unsigned frame;
    SSAValue v;
    CrossSSAValue(unsigned frame, const SSAValue &v) : frame(frame), v(v) {}
};






class CrossScriptSSA
{
  public:

    static const uint32 OUTER_FRAME = uint32(-1);
    static const unsigned INVALID_FRAME = uint32(-2);

    struct Frame {
        uint32 index;
        JSScript *script;
        uint32 depth;  
        uint32 parent;
        jsbytecode *parentpc;

        Frame(uint32 index, JSScript *script, uint32 depth, uint32 parent, jsbytecode *parentpc)
            : index(index), script(script), depth(depth), parent(parent), parentpc(parentpc)
        {}
    };

    const Frame &getFrame(uint32 index) {
        if (index == OUTER_FRAME)
            return outerFrame;
        return inlineFrames[index];
    }

    unsigned numFrames() { return 1 + inlineFrames.length(); }
    const Frame &iterFrame(unsigned i) {
        if (i == 0)
            return outerFrame;
        return inlineFrames[i - 1];
    }

    JSScript *outerScript() { return outerFrame.script; }

    types::TypeSet *getValueTypes(const CrossSSAValue &cv) {
        return getFrame(cv.frame).script->analysis(cx)->getValueTypes(cv.v);
    }

    bool addInlineFrame(JSScript *script, uint32 depth, uint32 parent, jsbytecode *parentpc)
    {
        uint32 index = inlineFrames.length();
        return inlineFrames.append(Frame(index, script, depth, parent, parentpc));
    }

    CrossScriptSSA(JSContext *cx, JSScript *outer)
        : cx(cx), outerFrame(OUTER_FRAME, outer, 0, INVALID_FRAME, NULL), inlineFrames(cx)
    {}

    CrossSSAValue foldValue(const CrossSSAValue &cv);

  private:
    JSContext *cx;

    Frame outerFrame;
    Vector<Frame> inlineFrames;
};

#ifdef DEBUG
void PrintBytecode(JSContext *cx, JSScript *script, jsbytecode *pc);
#endif

} 
} 

#endif 
