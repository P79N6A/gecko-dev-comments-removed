








































#ifndef jsion_licm_h__
#define jsion_licm_h__



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

class Loop
{
    MIRGraph &graph;

  public:
    
    Loop(MBasicBlock *header, MBasicBlock *footer, MIRGraph &graph);

    
    bool init();

    
    bool optimize();

  private:
    
    
    MBasicBlock *footer_;
    MBasicBlock *header_;

    
    
    MBasicBlock* preLoop_;

    
    
    
    bool iterateLoopBlocks(MBasicBlock *current);

    bool hoistInstructions(InstructionQueue &toHoist);

    
    bool isInLoop(MDefinition *ins);
    bool isLoopInvariant(MInstruction *ins);

    
    
    bool shouldHoist(MInstruction *ins);

    
    
    bool checkHotness(MBasicBlock *block);

    
    InstructionQueue worklist_;
    bool insertInWorklist(MInstruction *ins);
    MInstruction* popFromWorklist();
};

} 
} 

#endif 
