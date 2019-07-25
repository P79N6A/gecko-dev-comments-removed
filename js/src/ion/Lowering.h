








































#ifndef jsion_ion_lowering_h__
#define jsion_ion_lowering_h__




#include "IonAllocPolicy.h"
#include "IonLIR.h"
#include "MOpcodes.h"

namespace js {
namespace ion {

class MBasicBlock;
class MTableSwitch;
class MIRGenerator;
class MIRGraph;
class MDefinition;
class MInstruction;

class LIRGenerator : public MInstructionVisitor
{
  protected:
    MIRGenerator *gen;

  private:
    MIRGraph &graph;
    LIRGraph &lirGraph_;
    LBlock *current;
    MSnapshot *last_snapshot_;

  public:
    LIRGenerator(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : gen(gen),
        graph(graph),
        lirGraph_(lirGraph),
        last_snapshot_(NULL)
    { }

    bool generate();
    MIRGenerator *mir() {
        return gen;
    }

  protected:
    
    
    
    
    
    
    
    
    
    bool emitAtUses(MInstruction *mir);

    
    
    inline bool ensureDefined(MDefinition *mir);

    
    
    LUse use(MDefinition *mir, LUse policy);
    inline LUse use(MDefinition *mir);
    inline LUse useRegister(MDefinition *mir);
    inline LUse useFixed(MDefinition *mir, Register reg);
    inline LUse useFixed(MDefinition *mir, FloatRegister reg);
    inline LAllocation useOrConstant(MDefinition *mir);
    inline LAllocation useKeepaliveOrConstant(MDefinition *mir);
    inline LAllocation useRegisterOrConstant(MDefinition *mir);

    
    
    virtual bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir) = 0;

    
    inline LDefinition temp(LDefinition::Type type);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                        const LDefinition &def);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                       LDefinition::Policy policy = LDefinition::DEFAULT);

    template <size_t Ops, size_t Temps>
    inline bool defineReuseInput(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir);

    template <size_t Ops, size_t Temps>
    inline bool defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir,
                          LDefinition::Policy policy = LDefinition::DEFAULT);

    typedef LInstructionHelper<1, 2, 0> LMathI;
    virtual bool lowerForALU(LMathI *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs) = 0;

    virtual bool lowerForFPU(LMathD *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs) = 0;

    uint32 getVirtualRegister() {
        return lirGraph_.getVirtualRegister();
    }

    template <typename T> bool annotate(T *ins);
    template <typename T> bool add(T *ins);

    bool addPhi(LPhi *phi) {
        return current->addPhi(phi) && annotate(phi);
    }

    
    bool assignSnapshot(LInstruction *ins);
    virtual void fillSnapshot(LSnapshot *snapshot) = 0;

    
    virtual bool preparePhi(MPhi *phi) = 0;

  public:
    virtual bool lowerPhi(MPhi *phi);
    bool doBitOp(JSOp op, MInstruction *ins);

  public:
    bool visitInstruction(MInstruction *ins);
    bool visitBlock(MBasicBlock *block);

#define VISITMIR(op) bool visit##op(M##op *ins);
    MIR_OPCODE_LIST(VISITMIR)
#undef VISITMIR
};

} 
} 

#endif 

