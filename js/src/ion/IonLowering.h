








































#ifndef jsion_ion_lowering_h__
#define jsion_ion_lowering_h__




#include "IonAllocPolicy.h"
#include "IonLIR.h"
#include "MOpcodes.h"

namespace js {
namespace ion {

class MBasicBlock;
class MIRGenerator;
class MIRGraph;
class MInstruction;

static const uint32 VREG_INCREMENT = 1;

class LIRGenerator : public MInstructionVisitor
{
    MIRGenerator *gen;
    MIRGraph &graph;
    LBlock *current;
    uint32 vregGen_;

  public:
    LIRGenerator(MIRGenerator *gen, MIRGraph &graph)
      : gen(gen),
        graph(graph),
        vregGen_(0)
    { }

    bool generate();
    uint32 nextVirtualRegister() {
        vregGen_ += VREG_INCREMENT;
        return vregGen_;
    }

  protected:
    
    
    
    
    
    
    
    
    
    bool emitAtUses(MInstruction *mir);

    
    
    inline void startUsing(MInstruction *mir);
    inline void stopUsing(MInstruction *mir);

    
    
    LUse use(MInstruction *mir, LUse policy);
    inline LUse use(MInstruction *mir);
    inline LUse useRegister(MInstruction *mir);
    inline LUse useFixed(MInstruction *mir, Register reg);
    inline LUse useFixed(MInstruction *mir, FloatRegister reg);
    inline LAllocation useOrConstant(MInstruction *mir);
    inline LAllocation useRegisterOrConstant(MInstruction *mir);

    
    inline LDefinition temp(LDefinition::Type type);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MInstruction *mir,
                        const LDefinition &def);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MInstruction *mir,
                       LDefinition::Policy policy = LDefinition::DEFAULT);

    template <size_t Ops, size_t Temps>
    bool defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MInstruction *mir,
                   LDefinition::Policy policy = LDefinition::DEFAULT);

    template <typename T>
    bool add(T *ins) {
        current->add(ins);
        if (ins->numDefs()) {
            ins->setId(ins->getDef(0)->virtualRegister());
        } else {
            ins->setId(nextVirtualRegister());
            if (ins->id() >= MAX_VIRTUAL_REGISTERS)
                return false;
        }
        return true;
    }

  public:
    bool visitBlock(MBasicBlock *block);

#define VISITMIR(op) bool visit##op(M##op *ins);
    MIR_OPCODE_LIST(VISITMIR)
#undef VISITMIR
};

} 
} 

#endif 

