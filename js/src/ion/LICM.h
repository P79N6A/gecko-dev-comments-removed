








































#ifndef jsion_licm_h__
#define jsion_licm_h__

#include "ion/IonAllocPolicy.h"
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
    MIRGraph &graph;

  public:
    LICM(MIRGraph &graph);
    bool analyze();
};



struct LinearSum
{
    MDefinition *term;
    int32 constant;

    LinearSum(MDefinition *term, int32 constant)
        : term(term), constant(constant)
    {}
};

LinearSum
ExtractLinearSum(MDefinition *ins);



bool
ExtractLinearInequality(MTest *test, BranchDirection direction,
                        LinearSum *plhs, MDefinition **prhs, bool *plessEqual);

class Loop
{
    MIRGraph &graph;

  public:
    
    enum LoopReturn {
        LoopReturn_Success,
        LoopReturn_Error, 
        LoopReturn_Skip   
    };

  public:
    
    Loop(MBasicBlock *header, MBasicBlock *footer, MIRGraph &graph);

    
    LoopReturn init();

    
    bool optimize();

  private:
    
    
    MBasicBlock *footer_;
    MBasicBlock *header_;

    
    
    MBasicBlock* preLoop_;

    
    
    
    LoopReturn iterateLoopBlocks(MBasicBlock *current);

    bool hoistInstructions(InstructionQueue &toHoist, InstructionQueue &boundsChecks);

    
    bool isInLoop(MDefinition *ins);
    bool isLoopInvariant(MInstruction *ins);
    bool isLoopInvariant(MDefinition *ins);

    
    
    bool checkHotness(MBasicBlock *block);

    
    InstructionQueue worklist_;
    bool insertInWorklist(MInstruction *ins);
    MInstruction* popFromWorklist();

    inline bool isHoistable(const MDefinition *ins) const {
        return ins->isMovable() && !ins->isEffectful();
    }

    
    
    

    void tryHoistBoundsCheck(MBoundsCheck *ins, MTest *test, BranchDirection direction,
                             MInstruction **pupper, MInstruction **plower);

    bool nonDecreasing(MDefinition *initial, MDefinition *start);
};

} 
} 

#endif 
