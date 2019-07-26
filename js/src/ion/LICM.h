





#ifndef ion_LICM_h
#define ion_LICM_h

#include "ion/IonAllocPolicy.h"
#include "ion/IonAnalysis.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"


namespace js {
namespace ion {

class MIRGraph;
class MBasicBlock;

typedef Vector<MBasicBlock*, 1, IonAllocPolicy> BlockQueue;
typedef Vector<MInstruction*, 1, IonAllocPolicy> InstructionQueue;

class LICM
{
    MIRGenerator *mir;
    MIRGraph &graph;

  public:
    LICM(MIRGenerator *mir, MIRGraph &graph);
    bool analyze();
};

class Loop
{
    MIRGenerator *mir;

  public:
    
    enum LoopReturn {
        LoopReturn_Success,
        LoopReturn_Error, 
        LoopReturn_Skip   
    };

  public:
    
    Loop(MIRGenerator *mir, MBasicBlock *footer);

    
    LoopReturn init();

    
    bool optimize();

  private:
    
    MBasicBlock *header_;

    
    
    MBasicBlock* preLoop_;

    bool hoistInstructions(InstructionQueue &toHoist);

    
    bool isInLoop(MDefinition *ins);
    bool isBeforeLoop(MDefinition *ins);
    bool isLoopInvariant(MInstruction *ins);
    bool isLoopInvariant(MDefinition *ins);

    
    
    bool checkHotness(MBasicBlock *block);

    
    InstructionQueue worklist_;
    bool insertInWorklist(MInstruction *ins);
    MInstruction* popFromWorklist();

    inline bool isHoistable(const MDefinition *ins) const {
        return ins->isMovable() && !ins->isEffectful() && !ins->neverHoist();
    }
};

} 
} 

#endif
