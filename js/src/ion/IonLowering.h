








































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

class LBlock : public TempObject
{
    MBasicBlock *block_;
    InlineList<LInstruction> instructions_;

    LBlock(MBasicBlock *block)
      : block_(block)
    { }

  public:
    static LBlock *New(MBasicBlock *from) {
        return new LBlock(from);
    }

    void add(LInstruction *ins) {
        instructions_.insert(ins);
    }

    MBasicBlock *mir() const {
        return block_;
    }
};

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
        return ++vregGen_;
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

    template <size_t X, size_t Y>
    inline bool define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir,
                        const LDefinition &def);

    template <size_t X, size_t Y>
    inline bool define(LInstructionHelper<1, X, Y> *lir, MInstruction *mir,
                       LDefinition::Policy policy = LDefinition::DEFAULT);

    template <size_t X, size_t Y>
    bool defineBox(LInstructionHelper<BOX_PIECES, X, Y> *lir, MInstruction *mir,
                   LDefinition::Policy policy = LDefinition::DEFAULT);

    bool add(LInstruction *ins) {
        current->add(ins);
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

