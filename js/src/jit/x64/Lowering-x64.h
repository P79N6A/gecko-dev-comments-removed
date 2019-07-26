





#ifndef jit_x64_Lowering_x64_h
#define jit_x64_Lowering_x64_h

#include "jit/shared/Lowering-x86-shared.h"

namespace js {
namespace jit {

class LIRGeneratorX64 : public LIRGeneratorX86Shared
{
  public:
    LIRGeneratorX64(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorX86Shared(gen, graph, lirGraph)
    { }

  protected:
    void lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex);
    bool defineUntypedPhi(MPhi *phi, size_t lirIndex);

    
    bool useBox(LInstruction *lir, size_t n, MDefinition *mir,
                LUse::Policy policy = LUse::REGISTER, bool useAtStart = false);
    bool useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register);

    
    
    LAllocation useByteOpRegister(MDefinition *mir);
    LAllocation useByteOpRegisterOrNonDoubleConstant(MDefinition *mir);

    LDefinition tempToUnbox();

    
    
    LDefinition tempForDispatchCache(MIRType outputType = MIRType_None) {
        return LDefinition::BogusTemp();
    }

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitReturn(MReturn *ret);
    bool visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins);
    bool visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins);
    bool visitAsmJSLoadHeap(MAsmJSLoadHeap *ins);
    bool visitAsmJSStoreHeap(MAsmJSStoreHeap *ins);
    bool visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins);
    bool visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins);

    static bool allowFloat32Optimizations() {
        return true;
    }
};

typedef LIRGeneratorX64 LIRGeneratorSpecific;

} 
} 

#endif 
