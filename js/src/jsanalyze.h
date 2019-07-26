







#ifndef jsanalyze_h
#define jsanalyze_h

#include "jscompartment.h"

namespace js {
namespace analyze {

class LoopAnalysis;
class SlotValue;
class SSAValue;
class SSAUseChain;






























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
      case JSOP_CALLARG:
      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL:
        return true;
      default:
        return false;
    }
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
      case JSOP_EQ:
      case JSOP_NE:
      case JSOP_STRICTEQ:
      case JSOP_STRICTNE:
        return op;
      default:
        MOZ_ASSUME_UNREACHABLE("unrecognized op");
    }
}

static inline JSOp
NegateCompareOp(JSOp op)
{
    switch (op) {
      case JSOP_GT:
        return JSOP_LE;
      case JSOP_GE:
        return JSOP_LT;
      case JSOP_LT:
        return JSOP_GE;
      case JSOP_LE:
        return JSOP_GT;
      case JSOP_EQ:
        return JSOP_NE;
      case JSOP_NE:
        return JSOP_EQ;
      case JSOP_STRICTNE:
        return JSOP_STRICTEQ;
      case JSOP_STRICTEQ:
        return JSOP_STRICTNE;
      default:
        MOZ_ASSUME_UNREACHABLE("unrecognized op");
    }
}

static inline unsigned
FollowBranch(JSContext *cx, JSScript *script, unsigned offset)
{
    




    jsbytecode *pc = script->code + offset;
    unsigned targetOffset = offset + GET_JUMP_OFFSET(pc);
    if (targetOffset < offset) {
        jsbytecode *target = script->code + targetOffset;
        JSOp nop = JSOp(*target);
        if (nop == JSOP_GOTO)
            return targetOffset + GET_JUMP_OFFSET(target);
    }
    return targetOffset;
}


static inline uint32_t ThisSlot() {
    return 0;
}
static inline uint32_t ArgSlot(uint32_t arg) {
    return 1 + arg;
}
static inline uint32_t LocalSlot(JSScript *script, uint32_t local) {
    return 1 + (script->function() ? script->function()->nargs : 0) + local;
}
static inline uint32_t TotalSlots(JSScript *script) {
    return LocalSlot(script, 0) + script->nfixed;
}

static inline uint32_t StackSlot(JSScript *script, uint32_t index) {
    return TotalSlots(script) + index;
}

static inline uint32_t GetBytecodeSlot(JSScript *script, jsbytecode *pc)
{
    switch (JSOp(*pc)) {

      case JSOP_GETARG:
      case JSOP_CALLARG:
      case JSOP_SETARG:
        return ArgSlot(GET_SLOTNO(pc));

      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL:
      case JSOP_SETLOCAL:
        return LocalSlot(script, GET_SLOTNO(pc));

      case JSOP_THIS:
        return ThisSlot();

      default:
        MOZ_ASSUME_UNREACHABLE("Bad slot opcode");
    }
}








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
        return NULL;
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

struct NeedsArgsObjState;


class ScriptAnalysis
{
    friend class Bytecode;

    JSScript *script_;

    Bytecode **codeArray;

    uint32_t numSlots;
    uint32_t numPropertyReads_;

    bool outOfMemory;
    bool hadFailure;

    bool *escapedSlots;

    
    bool ranBytecode_;
    bool ranSSA_;
    bool ranLifetimes_;

#ifdef DEBUG
    
    bool originalDebugMode_: 1;
#endif

    

    bool usesScopeChain_:1;
    bool localsAliasStack_:1;
    bool canTrackVars:1;
    bool hasLoops_:1;
    bool hasTryFinally_:1;

    uint32_t numReturnSites_;

    

    LifetimeVariable *lifetimes;

  public:

    ScriptAnalysis(JSScript *script) {
        mozilla::PodZero(this);
        this->script_ = script;
#ifdef DEBUG
        this->originalDebugMode_ = script_->compartment()->debugMode();
#endif
    }

    bool ranBytecode() { return ranBytecode_; }
    bool ranSSA() { return ranSSA_; }
    bool ranLifetimes() { return ranLifetimes_; }

    void analyzeBytecode(JSContext *cx);
    void analyzeSSA(JSContext *cx);
    void analyzeLifetimes(JSContext *cx);

    bool OOM() const { return outOfMemory; }
    bool failed() const { return hadFailure; }

    
    bool hasTryFinally() const { return hasTryFinally_; }

    
    uint32_t numPropertyReads() const { return numPropertyReads_; }

    
    bool usesScopeChain() const { return usesScopeChain_; }

    uint32_t numReturnSites() const { return numReturnSites_; }

    bool hasLoops() const { return hasLoops_; }

    



    bool localsAliasStack() { return localsAliasStack_; }

    

    Bytecode& getCode(uint32_t offset) {
        JS_ASSERT(offset < script_->length);
        JS_ASSERT(codeArray[offset]);
        return *codeArray[offset];
    }
    Bytecode& getCode(const jsbytecode *pc) { return getCode(pc - script_->code); }

    Bytecode* maybeCode(uint32_t offset) {
        JS_ASSERT(offset < script_->length);
        return codeArray[offset];
    }
    Bytecode* maybeCode(const jsbytecode *pc) { return maybeCode(pc - script_->code); }

    bool jumpTarget(uint32_t offset) {
        JS_ASSERT(offset < script_->length);
        return codeArray[offset] && codeArray[offset]->jumpTarget;
    }
    bool jumpTarget(const jsbytecode *pc) { return jumpTarget(pc - script_->code); }

    bool popGuaranteed(jsbytecode *pc) {
        jsbytecode *next = pc + GetBytecodeLength(pc);
        return JSOp(*next) == JSOP_POP && !jumpTarget(next);
    }

    inline const SSAValue &poppedValue(uint32_t offset, uint32_t which);

    inline const SSAValue &poppedValue(const jsbytecode *pc, uint32_t which);

    const SlotValue *newValues(uint32_t offset) {
        JS_ASSERT(offset < script_->length);
        return getCode(offset).newValues;
    }
    const SlotValue *newValues(const jsbytecode *pc) { return newValues(pc - script_->code); }

    bool trackUseChain(const SSAValue &v) {
        JS_ASSERT_IF(v.kind() == SSAValue::VAR, trackSlot(v.varSlot()));
        return v.kind() != SSAValue::EMPTY &&
               (v.kind() != SSAValue::VAR || !v.varInitial());
    }

    



    inline SSAUseChain *& useChain(const SSAValue &v);


    
    inline jsbytecode *getCallPC(jsbytecode *pc);

    

    






    bool slotEscapes(uint32_t slot) {
        JS_ASSERT(script_->compartment()->activeAnalysis);
        if (slot >= numSlots)
            return true;
        return escapedSlots[slot];
    }

    





    bool trackSlot(uint32_t slot) { return !slotEscapes(slot) && canTrackVars && slot < 1000; }

    const LifetimeVariable & liveness(uint32_t slot) {
        JS_ASSERT(script_->compartment()->activeAnalysis);
        JS_ASSERT(!slotEscapes(slot));
        return lifetimes[slot];
    }

    void printSSA(JSContext *cx);
    void printTypes(JSContext *cx);

  private:
    void setOOM(JSContext *cx) {
        if (!outOfMemory)
            js_ReportOutOfMemory(cx);
        outOfMemory = true;
        hadFailure = true;
    }

    
    inline bool addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump, unsigned *forwardLoop,
                        unsigned stackDepth);

    
    inline void addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                            LifetimeVariable **&saved, unsigned &savedCount);
    inline void killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                             LifetimeVariable **&saved, unsigned &savedCount);
    inline void extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end);
    inline void ensureVariable(LifetimeVariable &var, unsigned until);

    
    struct SSAValueInfo
    {
        SSAValue v;

        




        int32_t branchSize;
    };

    
    bool makePhi(JSContext *cx, uint32_t slot, uint32_t offset, SSAValue *pv);
    void insertPhi(JSContext *cx, SSAValue &phi, const SSAValue &v);
    void mergeValue(JSContext *cx, uint32_t offset, const SSAValue &v, SlotValue *pv);
    void checkPendingValue(JSContext *cx, const SSAValue &v, uint32_t slot,
                           Vector<SlotValue> *pending);
    void checkBranchTarget(JSContext *cx, uint32_t targetOffset, Vector<uint32_t> &branchTargets,
                           SSAValueInfo *values, uint32_t stackDepth);
    void checkExceptionTarget(JSContext *cx, uint32_t catchOffset,
                              Vector<uint32_t> &exceptionTargets);
    void mergeBranchTarget(JSContext *cx, SSAValueInfo &value, uint32_t slot,
                           const Vector<uint32_t> &branchTargets, uint32_t currentOffset);
    void mergeExceptionTarget(JSContext *cx, const SSAValue &value, uint32_t slot,
                              const Vector<uint32_t> &exceptionTargets);
    void mergeAllExceptionTargets(JSContext *cx, SSAValueInfo *values,
                                  const Vector<uint32_t> &exceptionTargets);
    void freezeNewValues(JSContext *cx, uint32_t offset);

    typedef Vector<SSAValue, 16> SeenVector;
    bool needsArgsObj(JSContext *cx, SeenVector &seen, const SSAValue &v);
    bool needsArgsObj(JSContext *cx, SeenVector &seen, SSAUseChain *use);
    bool needsArgsObj(JSContext *cx);

  public:
#ifdef DEBUG
    void assertMatchingDebugMode();
#else
    void assertMatchingDebugMode() { }
#endif
};

#ifdef DEBUG
void PrintBytecode(JSContext *cx, HandleScript script, jsbytecode *pc);
#endif

} 
} 

namespace mozilla {

template <> struct IsPod<js::analyze::LifetimeVariable> : TrueType {};
template <> struct IsPod<js::analyze::LoopAnalysis>     : TrueType {};
template <> struct IsPod<js::analyze::SlotValue>        : TrueType {};
template <> struct IsPod<js::analyze::SSAValue>         : TrueType {};
template <> struct IsPod<js::analyze::SSAUseChain>      : TrueType {};

} 

#endif 
