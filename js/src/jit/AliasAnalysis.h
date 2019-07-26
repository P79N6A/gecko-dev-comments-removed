





#ifndef jit_AliasAnalysis_h
#define jit_AliasAnalysis_h

#include "jit/MIR.h"
#include "jit/MIRGraph.h"

namespace js {
namespace jit {

class MIRGraph;

typedef Vector<MDefinition *, 4, IonAllocPolicy> InstructionVector;

class LoopAliasInfo : public TempObject
{
  private:
    LoopAliasInfo *outer_;
    MBasicBlock *loopHeader_;
    InstructionVector invariantLoads_;

  public:
    LoopAliasInfo(LoopAliasInfo *outer, MBasicBlock *loopHeader)
      : outer_(outer), loopHeader_(loopHeader)
    { }

    MBasicBlock *loopHeader() const {
        return loopHeader_;
    }
    LoopAliasInfo *outer() const {
        return outer_;
    }
    bool addInvariantLoad(MDefinition *ins) {
        return invariantLoads_.append(ins);
    }
    const InstructionVector& invariantLoads() const {
        return invariantLoads_;
    }
    MDefinition *firstInstruction() const {
        return *loopHeader_->begin();
    }
};

class AliasAnalysis
{
    MIRGenerator *mir;
    MIRGraph &graph_;
    LoopAliasInfo *loop_;

  public:
    AliasAnalysis(MIRGenerator *mir, MIRGraph &graph);
    bool analyze();
};

} 
} 

#endif 
