





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
    
    
    void useBox(LInstruction *lir, size_t n, MDefinition *mir,
                LUse::Policy policy = LUse::REGISTER, bool useAtStart = false);
    void useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register reg2);

    
    
    
    
    
    LAllocation useByteOpRegister(MDefinition *mir);
    LAllocation useByteOpRegisterOrNonDoubleConstant(MDefinition *mir);
    LDefinition tempByteOpRegister();

    inline LDefinition tempToUnbox() {
        return LDefinition::BogusTemp();
    }

    bool needTempForPostBarrier() { return true; }

    void lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex);
    void defineUntypedPhi(MPhi *phi, size_t lirIndex);

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
    void lowerPhi(MPhi *phi);

    static bool allowTypedElementHoleCheck() {
        return true;
    }

    static bool allowStaticTypedArrayAccesses() {
        return true;
    }
};

typedef LIRGeneratorX86 LIRGeneratorSpecific;

} 
} 

#endif 
