





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
    void defineUntypedPhi(MPhi *phi, size_t lirIndex);

    
    void useBox(LInstruction *lir, size_t n, MDefinition *mir,
                LUse::Policy policy = LUse::REGISTER, bool useAtStart = false);
    void useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register);

    
    
    LAllocation useByteOpRegister(MDefinition *mir);
    LAllocation useByteOpRegisterOrNonDoubleConstant(MDefinition *mir);
    LDefinition tempByteOpRegister();

    LDefinition tempToUnbox();

    bool needTempForPostBarrier() { return false; }

  public:
    void visitBox(MBox *box);
    void visitUnbox(MUnbox *unbox);
    void visitReturn(MReturn *ret);
    void visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins);
    void visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins);
    void visitAsmJSLoadHeap(MAsmJSLoadHeap *ins);
    void visitAsmJSStoreHeap(MAsmJSStoreHeap *ins);
    void visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins);
    void visitAsmJSCompareExchangeHeap(MAsmJSCompareExchangeHeap *ins);
    void visitAsmJSAtomicBinopHeap(MAsmJSAtomicBinopHeap *ins);
    void visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins);
    void visitSubstr(MSubstr *ins);
};

typedef LIRGeneratorX64 LIRGeneratorSpecific;

} 
} 

#endif 
