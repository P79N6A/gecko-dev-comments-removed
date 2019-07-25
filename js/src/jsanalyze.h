







































#ifndef jsanalyze_h___
#define jsanalyze_h___

#include "jscompartment.h"
#include "jscntxt.h"
#include "jsinfer.h"
#include "jsscript.h"

#include "ds/LifoAlloc.h"
#include "js/TemplateLib.h"

struct JSScript;


namespace js { namespace mjit { struct RegisterAllocation; } }

namespace js {
namespace analyze {






























class Bytecode
{
    friend class ScriptAnalysis;

  public:
    Bytecode() { PodZero(this); }

    

    
    bool jumpTarget : 1;

    
    bool fallthrough : 1;

    
    bool jumpFallthrough : 1;

    
    bool switchTarget : 1;

    



    bool unconditional : 1;

    
    bool analyzed : 1;

    
    bool exceptionEntry : 1;

    
    bool inTryBlock : 1;

    
    bool safePoint : 1;

    



    bool monitoredTypes : 1;

    
    bool monitoredTypesReturn : 1;

    



    bool arrayWriteHole: 1;  
    bool getStringElement:1; 
    bool accessGetter: 1;    

    
    uint32_t stackDepth;

  private:

    union {
        
        types::TypeSet *observedTypes;

        
        LoopAnalysis *loop;
    };

    

    
    mjit::RegisterAllocation *allocation;

    

    
    SSAValue *poppedValues;

    
    SSAUseChain **pushedUses;

    union {
        





        SlotValue *newValues;

        







        Vector<SlotValue> *pendingValues;
    };

    

    
    types::TypeSet *pushedTypes;

    
    types::TypeBarrier *typeBarriers;
};

static inline unsigned
GetDefCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;

    



    switch (JSOp(*pc)) {
      case JSOP_OR:
      case JSOP_AND:
        return 1;
      case JSOP_FILTER:
        return 2;
      case JSOP_PICK:
        





        return (pc[1] + 1);
      default:
        return StackDefs(script, pc);
    }
}

static inline unsigned
GetUseCount(JSScript *script, unsigned offset)
{
    JS_ASSERT(offset < script->length);
    jsbytecode *pc = script->code + offset;

    if (JSOp(*pc) == JSOP_PICK)
        return (pc[1] + 1);
    if (js_CodeSpec[*pc].nuses == -1)
        return StackUses(script, pc);
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
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
        return true;
      default:
        return false;
    }
}


static inline bool
BytecodeNoFallThrough(JSOp op)
{
    switch (op) {
      case JSOP_GOTO:
      case JSOP_DEFAULT:
      case JSOP_RETURN:
      case JSOP_STOP:
      case JSOP_RETRVAL:
      case JSOP_THROW:
      case JSOP_TABLESWITCH:
      case JSOP_LOOKUPSWITCH:
      case JSOP_FILTER:
        return true;
      case JSOP_GOSUB:
        
        return false;
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
        JS_NOT_REACHED("unrecognized op");
        return op;
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
        JS_NOT_REACHED("unrecognized op");
        return op;
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


static inline uint32_t CalleeSlot() {
    return 0;
}
static inline uint32_t ThisSlot() {
    return 1;
}
static inline uint32_t ArgSlot(uint32_t arg) {
    return 2 + arg;
}
static inline uint32_t LocalSlot(JSScript *script, uint32_t local) {
    return 2 + (script->function() ? script->function()->nargs : 0) + local;
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
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
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
        return LocalSlot(script, GET_SLOTNO(pc));

      case JSOP_THIS:
        return ThisSlot();

      default:
        JS_NOT_REACHED("Bad slot opcode");
        return 0;
    }
}


static inline bool
BytecodeUpdatesSlot(JSOp op)
{
    switch (op) {
      case JSOP_SETARG:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
        return true;
      default:
        return false;
    }
}

static inline int32_t
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
    



    uint32_t start;
    uint32_t end;

    



    uint32_t savedEnd;

    




    bool loopTail;

    



    bool write;

    
    Lifetime *next;

    Lifetime(uint32_t offset, uint32_t savedEnd, Lifetime *next)
        : start(offset), end(offset), savedEnd(savedEnd),
          loopTail(false), write(false), next(next)
    {}
};


class LoopAnalysis
{
  public:
    
    LoopAnalysis *parent;

    
    uint32_t head;

    



    uint32_t backedge;

    
    uint32_t entry;

    





    uint32_t lastBlock;

    



    bool hasSafePoints;

    
    bool hasCallsLoops;
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
    types::TypeSet *phiTypes() const;

    
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
        PodZero(this);
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
    types::TypeSet types;
    uint32_t slot;
    uint32_t length;
    SSAValue *options;
    SSAUseChain *uses;
    SSAPhiNode() { PodZero(this); }
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

inline types::TypeSet *
SSAValue::phiTypes() const
{
    JS_ASSERT(kind() == PHI);
    return &u.phi.node->types;
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

    SSAUseChain() { PodZero(this); }
};

class SlotValue
{
  public:
    uint32_t slot;
    SSAValue value;
    SlotValue(uint32_t slot, const SSAValue &value) : slot(slot), value(value) {}
};


class ScriptAnalysis
{
    friend class Bytecode;

    JSScript *script;

    Bytecode **codeArray;

    uint32_t numSlots;

    bool outOfMemory;
    bool hadFailure;

    JSPackedBool *escapedSlots;

    
    bool ranBytecode_;
    bool ranSSA_;
    bool ranLifetimes_;
    bool ranInference_;

#ifdef DEBUG
    
    bool originalDebugMode_: 1;
#endif

    

    bool usesReturnValue_:1;
    bool usesScopeChain_:1;
    bool usesThisValue_:1;
    bool hasFunctionCalls_:1;
    bool modifiesArguments_:1;
    bool extendsScope_:1;
    bool addsScopeObjects_:1;
    bool localsAliasStack_:1;
    bool isInlineable:1;
    bool isCompileable:1;
    bool canTrackVars:1;

    uint32_t numReturnSites_;

    

    LifetimeVariable *lifetimes;

  public:

    ScriptAnalysis(JSScript *script) { 
        PodZero(this);
        this->script = script;
#ifdef DEBUG
        this->originalDebugMode_ = script->compartment()->debugMode();
#endif        
    }

    bool ranBytecode() { return ranBytecode_; }
    bool ranSSA() { return ranSSA_; }
    bool ranLifetimes() { return ranLifetimes_; }
    bool ranInference() { return ranInference_; }

    void analyzeBytecode(JSContext *cx);
    void analyzeSSA(JSContext *cx);
    void analyzeLifetimes(JSContext *cx);
    void analyzeTypes(JSContext *cx);

    
    void analyzeTypesNew(JSContext *cx);

    bool OOM() const { return outOfMemory; }
    bool failed() const { return hadFailure; }
    bool inlineable() const { return isInlineable; }
    bool inlineable(uint32_t argc) const { return isInlineable && argc == script->function()->nargs; }
    bool compileable() { return isCompileable; }

    
    bool usesReturnValue() const { return usesReturnValue_; }

    
    bool usesScopeChain() const { return usesScopeChain_; }

    bool usesThisValue() const { return usesThisValue_; }
    bool hasFunctionCalls() const { return hasFunctionCalls_; }
    uint32_t numReturnSites() const { return numReturnSites_; }

    



    bool modifiesArguments() { return modifiesArguments_; }

    



    bool extendsScope() { return extendsScope_; }

    
    bool addsScopeObjects() { return addsScopeObjects_; }

    



    bool localsAliasStack() { return localsAliasStack_; }

    

    Bytecode& getCode(uint32_t offset) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT(codeArray[offset]);
        return *codeArray[offset];
    }
    Bytecode& getCode(const jsbytecode *pc) { return getCode(pc - script->code); }

    Bytecode* maybeCode(uint32_t offset) {
        JS_ASSERT(offset < script->length);
        return codeArray[offset];
    }
    Bytecode* maybeCode(const jsbytecode *pc) { return maybeCode(pc - script->code); }

    bool jumpTarget(uint32_t offset) {
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

    types::TypeSet *bytecodeTypes(const jsbytecode *pc) {
        JS_ASSERT(js_CodeSpec[*pc].format & JOF_TYPESET);
        return getCode(pc).observedTypes;
    }

    const SSAValue &poppedValue(uint32_t offset, uint32_t which) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT(which < GetUseCount(script, offset) +
                  (ExtendedUse(script->code + offset) ? 1 : 0));
        return getCode(offset).poppedValues[which];
    }
    const SSAValue &poppedValue(const jsbytecode *pc, uint32_t which) {
        return poppedValue(pc - script->code, which);
    }

    const SlotValue *newValues(uint32_t offset) {
        JS_ASSERT(offset < script->length);
        return getCode(offset).newValues;
    }
    const SlotValue *newValues(const jsbytecode *pc) { return newValues(pc - script->code); }

    types::TypeSet *pushedTypes(uint32_t offset, uint32_t which = 0) {
        JS_ASSERT(offset < script->length);
        JS_ASSERT(which < GetDefCount(script, offset) +
                  (ExtendedDef(script->code + offset) ? 1 : 0));
        types::TypeSet *array = getCode(offset).pushedTypes;
        JS_ASSERT(array);
        return array + which;
    }
    types::TypeSet *pushedTypes(const jsbytecode *pc, uint32_t which) {
        return pushedTypes(pc - script->code, which);
    }

    bool hasPushedTypes(const jsbytecode *pc) { return getCode(pc).pushedTypes != NULL; }

    types::TypeBarrier *typeBarriers(JSContext *cx, uint32_t offset) {
        if (getCode(offset).typeBarriers)
            pruneTypeBarriers(cx, offset);
        return getCode(offset).typeBarriers;
    }
    types::TypeBarrier *typeBarriers(JSContext *cx, const jsbytecode *pc) {
        return typeBarriers(cx, pc - script->code);
    }
    void addTypeBarrier(JSContext *cx, const jsbytecode *pc,
                        types::TypeSet *target, types::Type type);
    void addSingletonTypeBarrier(JSContext *cx, const jsbytecode *pc,
                                 types::TypeSet *target, JSObject *singleton, jsid singletonId);

    
    void pruneTypeBarriers(JSContext *cx, uint32_t offset);

    




    void breakTypeBarriers(JSContext *cx, uint32_t offset, bool all);

    
    void breakTypeBarriersSSA(JSContext *cx, const SSAValue &v);

    inline void addPushedType(JSContext *cx, uint32_t offset, uint32_t which, types::Type type);

    types::TypeSet *getValueTypes(const SSAValue &v) {
        switch (v.kind()) {
          case SSAValue::PUSHED:
            return pushedTypes(v.pushedOffset(), v.pushedIndex());
          case SSAValue::VAR:
            JS_ASSERT(!slotEscapes(v.varSlot()));
            if (v.varInitial()) {
                return types::TypeScript::SlotTypes(script, v.varSlot());
            } else {
                





                return pushedTypes(v.varOffset(), 0);
            }
          case SSAValue::PHI:
            return &v.phiNode()->types;
          default:
            
            JS_NOT_REACHED("Bad SSA value");
            return NULL;
        }
    }

    types::TypeSet *poppedTypes(uint32_t offset, uint32_t which) {
        return getValueTypes(poppedValue(offset, which));
    }
    types::TypeSet *poppedTypes(const jsbytecode *pc, uint32_t which) {
        return getValueTypes(poppedValue(pc, which));
    }

    
    bool integerOperation(JSContext *cx, jsbytecode *pc);

    bool trackUseChain(const SSAValue &v) {
        JS_ASSERT_IF(v.kind() == SSAValue::VAR, trackSlot(v.varSlot()));
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

    mjit::RegisterAllocation *&getAllocation(uint32_t offset) {
        JS_ASSERT(offset < script->length);
        return getCode(offset).allocation;
    }
    mjit::RegisterAllocation *&getAllocation(const jsbytecode *pc) {
        return getAllocation(pc - script->code);
    }

    LoopAnalysis *getLoop(uint32_t offset) {
        JS_ASSERT(offset < script->length);
        return getCode(offset).loop;
    }
    LoopAnalysis *getLoop(const jsbytecode *pc) { return getLoop(pc - script->code); }

    
    jsbytecode *getCallPC(jsbytecode *pc)
    {
        SSAUseChain *uses = useChain(SSAValue::PushedValue(pc - script->code, 0));
        JS_ASSERT(uses && uses->popped);
        JS_ASSERT_IF(uses->next,
                     !uses->next->next &&
                     uses->next->popped &&
                     script->code[uses->next->offset] == JSOP_SWAP);
        return script->code + uses->offset;
    }

    

    






    bool slotEscapes(uint32_t slot) {
        JS_ASSERT(script->compartment()->activeAnalysis);
        if (slot >= numSlots)
            return true;
        return escapedSlots[slot];
    }

    





    bool trackSlot(uint32_t slot) { return !slotEscapes(slot) && canTrackVars; }

    const LifetimeVariable & liveness(uint32_t slot) {
        JS_ASSERT(script->compartment()->activeAnalysis);
        JS_ASSERT(!slotEscapes(slot));
        return lifetimes[slot];
    }

    



    struct NameAccess {
        JSScript *script;
        types::TypeScriptNesting *nesting;
        uint32_t slot;

        
        bool arg;
        uint32_t index;

        const Value **basePointer() const {
            return arg ? &nesting->argArray : &nesting->varArray;
        }
    };
    NameAccess resolveNameAccess(JSContext *cx, jsid id, bool addDependency = false);

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
                        unsigned stackDepth);
    void checkAliasedName(JSContext *cx, jsbytecode *pc);

    
    inline void addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                            LifetimeVariable **&saved, unsigned &savedCount);
    inline void killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                             LifetimeVariable **&saved, unsigned &savedCount);
    inline void extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end);
    inline void ensureVariable(LifetimeVariable &var, unsigned until);

    
    bool makePhi(JSContext *cx, uint32_t slot, uint32_t offset, SSAValue *pv);
    void insertPhi(JSContext *cx, SSAValue &phi, const SSAValue &v);
    void mergeValue(JSContext *cx, uint32_t offset, const SSAValue &v, SlotValue *pv);
    void checkPendingValue(JSContext *cx, const SSAValue &v, uint32_t slot,
                           Vector<SlotValue> *pending);
    void checkBranchTarget(JSContext *cx, uint32_t targetOffset, Vector<uint32_t> &branchTargets,
                           SSAValue *values, uint32_t stackDepth);
    void checkExceptionTarget(JSContext *cx, uint32_t catchOffset,
                              Vector<uint32_t> &exceptionTargets);
    void mergeBranchTarget(JSContext *cx, const SSAValue &value, uint32_t slot,
                           const Vector<uint32_t> &branchTargets);
    void mergeExceptionTarget(JSContext *cx, const SSAValue &value, uint32_t slot,
                              const Vector<uint32_t> &exceptionTargets);
    void mergeAllExceptionTargets(JSContext *cx, SSAValue *values,
                                  const Vector<uint32_t> &exceptionTargets);
    bool removeBranchTarget(Vector<uint32_t> &branchTargets,
                            Vector<uint32_t> &exceptionTargets,
                            uint32_t offset);
    void freezeNewValues(JSContext *cx, uint32_t offset);

    struct TypeInferenceState {
        Vector<SSAPhiNode *> phiNodes;
        bool hasGetSet;
        bool hasHole;
        types::TypeSet *forTypes;
        TypeInferenceState(JSContext *cx)
            : phiNodes(cx), hasGetSet(false), hasHole(false), forTypes(NULL)
        {}
    };

    
    bool analyzeTypesBytecode(JSContext *cx, unsigned offset, TypeInferenceState &state);
    bool followEscapingArguments(JSContext *cx, const SSAValue &v, Vector<SSAValue> *seen);
    bool followEscapingArguments(JSContext *cx, SSAUseChain *use, Vector<SSAValue> *seen);

  public:
#ifdef DEBUG
    void assertMatchingDebugMode();
#else
    void assertMatchingDebugMode() { }
#endif
};


class AutoEnterAnalysis
{
    JSCompartment *compartment;
    bool oldActiveAnalysis;
    bool left;

    void construct(JSCompartment *compartment)
    {
        this->compartment = compartment;
        oldActiveAnalysis = compartment->activeAnalysis;
        compartment->activeAnalysis = true;
        left = false;
    }

  public:
    AutoEnterAnalysis(JSContext *cx) { construct(cx->compartment); }
    AutoEnterAnalysis(JSCompartment *compartment) { construct(compartment); }

    void leave()
    {
        if (!left) {
            left = true;
            compartment->activeAnalysis = oldActiveAnalysis;
        }
    }

    ~AutoEnterAnalysis()
    {
        leave();
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

    static const uint32_t OUTER_FRAME = UINT32_MAX;
    static const unsigned INVALID_FRAME = uint32_t(-2);

    struct Frame {
        uint32_t index;
        JSScript *script;
        uint32_t depth;  
        uint32_t parent;
        jsbytecode *parentpc;

        Frame(uint32_t index, JSScript *script, uint32_t depth, uint32_t parent, jsbytecode *parentpc)
            : index(index), script(script), depth(depth), parent(parent), parentpc(parentpc)
        {}
    };

    const Frame &getFrame(uint32_t index) {
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

    
    size_t frameLength(uint32_t index) {
        if (index == OUTER_FRAME)
            return 0;
        size_t res = outerFrame.script->length;
        for (unsigned i = 0; i < index; i++)
            res += inlineFrames[i].script->length;
        return res;
    }

    types::TypeSet *getValueTypes(const CrossSSAValue &cv) {
        return getFrame(cv.frame).script->analysis()->getValueTypes(cv.v);
    }

    bool addInlineFrame(JSScript *script, uint32_t depth, uint32_t parent, jsbytecode *parentpc)
    {
        uint32_t index = inlineFrames.length();
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

namespace js {
namespace tl {

template <> struct IsPodType<js::analyze::LifetimeVariable> { static const bool result = true; };
template <> struct IsPodType<js::analyze::LoopAnalysis>     { static const bool result = true; };
template <> struct IsPodType<js::analyze::SlotValue>        { static const bool result = true; };
template <> struct IsPodType<js::analyze::SSAValue>         { static const bool result = true; };
template <> struct IsPodType<js::analyze::SSAUseChain>      { static const bool result = true; };

} 
} 

#endif 
