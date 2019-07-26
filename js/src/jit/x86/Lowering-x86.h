





#ifndef jit_x86_Lowering_x86_h
#define jit_x86_Lowering_x86_h

#include "jit/shared/Lowering-x86-shared.h"

namespace js {
namespace jit {

class LIRGeneratorX86 : public LIRGeneratorX86Shared
{
  public:
    LIRGeneratorX86(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorX86Shared(gen, graph, lirGraph)
    { }

  protected:
    
    
    bool useBox(LInstruction *lir, size_t n, MDefinition *mir,
                LUse::Policy policy = LUse::REGISTER, bool useAtStart = false);
    bool useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register reg2);

    
    
    
    
    
    LAllocation useByteOpRegister(MDefinition *mir);
    LAllocation useByteOpRegisterOrNonDoubleConstant(MDefinition *mir);

    inline LDefinition tempToUnbox() {
        return LDefinition::BogusTemp();
    }

    LDefinition tempForDispatchCache(MIRType outputType = MIRType_None);

    void lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex);
    bool defineUntypedPhi(MPhi *phi, size_t lirIndex);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitReturn(MReturn *ret);
    bool visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins);
    bool visitAsmJSLoadHeap(MAsmJSLoadHeap *ins);
    bool visitAsmJSStoreHeap(MAsmJSStoreHeap *ins);
    bool visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins);
    bool visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins);
    bool lowerPhi(MPhi *phi);

    static bool allowTypedElementHoleCheck() {
        return true;
    }

    static bool allowStaticTypedArrayAccesses() {
        return true;
    }

    static bool allowFloat32Optimizations() {
        return true;
    }
};

typedef LIRGeneratorX86 LIRGeneratorSpecific;

} 
} 

#endif 
