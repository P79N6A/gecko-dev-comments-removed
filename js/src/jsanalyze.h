







#ifndef jsanalyze_h
#define jsanalyze_h

#include "jscompartment.h"

namespace js {
namespace analyze {

class Bytecode;
struct LifetimeVariable;
class SlotValue;
class SSAValue;
struct SSAValueInfo;
class SSAUseChain;



static inline uint32_t ThisSlot() {
    return 0;
}
static inline uint32_t ArgSlot(uint32_t arg) {
    return 1 + arg;
}
static inline uint32_t LocalSlot(JSScript *script, uint32_t local) {
    return 1 + local +
           (script->functionNonDelazifying() ? script->functionNonDelazifying()->nargs() : 0);
}
static inline uint32_t TotalSlots(JSScript *script) {
    return LocalSlot(script, 0) + script->nfixed();
}





class ScriptAnalysis
{
    friend class Bytecode;

    JSScript *script_;

    Bytecode **codeArray;

    uint32_t numSlots;

    bool *escapedSlots;

#ifdef DEBUG
    
    bool originalDebugMode_: 1;
#endif

    

    bool canTrackVars:1;
    bool argumentsContentsObserved_:1;

    

    LifetimeVariable *lifetimes;

  public:
    ScriptAnalysis(JSScript *script) {
        mozilla::PodZero(this);
        this->script_ = script;
#ifdef DEBUG
        this->originalDebugMode_ = script_->compartment()->debugMode();
#endif
    }

    MOZ_WARN_UNUSED_RESULT
    bool analyzeBytecode(JSContext *cx);

    bool isReachable(const jsbytecode *pc) { return maybeCode(pc); }

  private:
    MOZ_WARN_UNUSED_RESULT
    bool analyzeSSA(JSContext *cx);
    MOZ_WARN_UNUSED_RESULT
    bool analyzeLifetimes(JSContext *cx);

    
    Bytecode& getCode(uint32_t offset) {
        JS_ASSERT(offset < script_->length());
        JS_ASSERT(codeArray[offset]);
        return *codeArray[offset];
    }
    Bytecode& getCode(const jsbytecode *pc) { return getCode(script_->pcToOffset(pc)); }

    Bytecode* maybeCode(uint32_t offset) {
        JS_ASSERT(offset < script_->length());
        return codeArray[offset];
    }
    Bytecode* maybeCode(const jsbytecode *pc) { return maybeCode(script_->pcToOffset(pc)); }

    inline bool jumpTarget(uint32_t offset);
    inline bool jumpTarget(const jsbytecode *pc);

    inline const SSAValue &poppedValue(uint32_t offset, uint32_t which);
    inline const SSAValue &poppedValue(const jsbytecode *pc, uint32_t which);

    inline const SlotValue *newValues(uint32_t offset);
    inline const SlotValue *newValues(const jsbytecode *pc);

    inline bool trackUseChain(const SSAValue &v);

    


    inline SSAUseChain *& useChain(const SSAValue &v);


    
    inline jsbytecode *getCallPC(jsbytecode *pc);

    

    






    inline bool slotEscapes(uint32_t slot);

    





    inline bool trackSlot(uint32_t slot);

    inline const LifetimeVariable & liveness(uint32_t slot);

    void printSSA(JSContext *cx);
    void printTypes(JSContext *cx);

    
    MOZ_WARN_UNUSED_RESULT
    inline bool addJump(JSContext *cx, unsigned offset,
                        unsigned *currentOffset, unsigned *forwardJump, unsigned *forwardLoop,
                        unsigned stackDepth);

    
    MOZ_WARN_UNUSED_RESULT
    inline bool addVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                            LifetimeVariable **&saved, unsigned &savedCount);
    MOZ_WARN_UNUSED_RESULT
    inline bool killVariable(JSContext *cx, LifetimeVariable &var, unsigned offset,
                             LifetimeVariable **&saved, unsigned &savedCount);
    MOZ_WARN_UNUSED_RESULT
    inline bool extendVariable(JSContext *cx, LifetimeVariable &var, unsigned start, unsigned end);

    inline void ensureVariable(LifetimeVariable &var, unsigned until);

    
    MOZ_WARN_UNUSED_RESULT
    bool makePhi(JSContext *cx, uint32_t slot, uint32_t offset, SSAValue *pv);
    MOZ_WARN_UNUSED_RESULT
    bool insertPhi(JSContext *cx, SSAValue &phi, const SSAValue &v);
    MOZ_WARN_UNUSED_RESULT
    bool mergeValue(JSContext *cx, uint32_t offset, const SSAValue &v, SlotValue *pv);
    MOZ_WARN_UNUSED_RESULT
    bool checkPendingValue(JSContext *cx, const SSAValue &v, uint32_t slot,
                           Vector<SlotValue> *pending);
    MOZ_WARN_UNUSED_RESULT
    bool checkBranchTarget(JSContext *cx, uint32_t targetOffset, Vector<uint32_t> &branchTargets,
                           SSAValueInfo *values, uint32_t stackDepth);
    MOZ_WARN_UNUSED_RESULT
    bool checkExceptionTarget(JSContext *cx, uint32_t catchOffset,
                              Vector<uint32_t> &exceptionTargets);
    MOZ_WARN_UNUSED_RESULT
    bool mergeBranchTarget(JSContext *cx, SSAValueInfo &value, uint32_t slot,
                           const Vector<uint32_t> &branchTargets, uint32_t currentOffset);
    MOZ_WARN_UNUSED_RESULT
    bool mergeExceptionTarget(JSContext *cx, const SSAValue &value, uint32_t slot,
                              const Vector<uint32_t> &exceptionTargets);
    MOZ_WARN_UNUSED_RESULT
    bool mergeAllExceptionTargets(JSContext *cx, SSAValueInfo *values,
                                  const Vector<uint32_t> &exceptionTargets);
    MOZ_WARN_UNUSED_RESULT
    bool freezeNewValues(JSContext *cx, uint32_t offset);

    typedef Vector<SSAValue, 16> SeenVector;
    bool needsArgsObj(JSContext *cx, SeenVector &seen, const SSAValue &v);
    bool needsArgsObj(JSContext *cx, SeenVector &seen, SSAUseChain *use);
    bool needsArgsObj(JSContext *cx);

  public:
#ifdef DEBUG
    void assertMatchingDebugMode();
    void assertMatchingStackDepthAtOffset(uint32_t offset, uint32_t stackDepth);
#else
    void assertMatchingDebugMode() { }
    void assertMatchingStackDepthAtOffset(uint32_t offset, uint32_t stackDepth) { }
#endif
};

#ifdef DEBUG
void PrintBytecode(JSContext *cx, HandleScript script, jsbytecode *pc);
#endif

} 
} 

#endif 
