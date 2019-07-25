








































#ifndef jsion_codegen_h__
#define jsion_codegen_h__

#include "ion/IonLIR.h"
#include "ion/MoveGroupResolver.h"
#include "ion/IonLinker.h"

namespace js {
namespace ion {

class CodeGeneratorShared : public LInstructionVisitor
{
  protected:
    Assembler masm;
    MIRGenerator *gen;
    LIRGraph &graph;
    LBlock *current;
    MoveGroupResolver moveGroupResolver;

    static inline int32 ToInt32(const LAllocation *a) {
        return a->toConstant()->toInt32();
    }

  protected:
    
    
    
    int32 frameDepth_;

    
    
    
    int32 framePushed_;

    inline int32 ArgToStackOffset(int32 slot) {
        JS_ASSERT(slot >= 0);
        return framePushed_ + frameDepth_ + ION_FRAME_PREFIX_SIZE + slot;
    }

    inline int32 SlotToStackOffset(int32 slot) {
        JS_ASSERT(slot >= 0 && slot <= int32(graph.localSlotCount()));
        int32 offset = framePushed_ + frameDepth_ - slot * STACK_SLOT_SIZE;
        JS_ASSERT(offset >= 0);
        return offset;
    }

  private:
    virtual bool generatePrologue() = 0;
    bool generateBody();

  public:
    CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph);

    bool generate();

  public:
    
    virtual bool visitParameter(LParameter *param);
};

} 
} 

#endif 

