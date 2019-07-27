





#ifndef jit_mips_Lowering_mips_h
#define jit_mips_Lowering_mips_h

#include "jit/shared/Lowering-shared.h"

namespace js {
namespace jit {

class LIRGeneratorMIPS : public LIRGeneratorShared
{
  protected:
    LIRGeneratorMIPS(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorShared(gen, graph, lirGraph)
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

    bool needTempForPostBarrier() { return false; }

    
    
    LDefinition tempForDispatchCache(MIRType outputType = MIRType_None) {
        return LDefinition::BogusTemp();
    }

    void lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex);
    void defineUntypedPhi(MPhi *phi, size_t lirIndex);
    void lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                       MDefinition *rhs);
    void lowerUrshD(MUrsh *mir);

    void lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir,
                     MDefinition *input);
    void lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                     MDefinition *lhs, MDefinition *rhs);

    void lowerForFPU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir,
                     MDefinition *src);
    template<size_t Temps>
    void lowerForFPU(LInstructionHelper<1, 2, Temps> *ins, MDefinition *mir,
                     MDefinition *lhs, MDefinition *rhs);

    void lowerForCompIx4(LSimdBinaryCompIx4 *ins, MSimdBinaryComp *mir,
                         MDefinition *lhs, MDefinition *rhs)
    {
        return lowerForFPU(ins, mir, lhs, rhs);
    }
    void lowerForCompFx4(LSimdBinaryCompFx4 *ins, MSimdBinaryComp *mir,
                         MDefinition *lhs, MDefinition *rhs)
    {
        return lowerForFPU(ins, mir, lhs, rhs);
    }

    void lowerForBitAndAndBranch(LBitAndAndBranch *baab, MInstruction *mir,
                                 MDefinition *lhs, MDefinition *rhs);
    void lowerConstantDouble(double d, MInstruction *ins);
    void lowerConstantFloat32(float d, MInstruction *ins);
    void lowerTruncateDToInt32(MTruncateToInt32 *ins);
    void lowerTruncateFToInt32(MTruncateToInt32 *ins);
    void lowerDivI(MDiv *div);
    void lowerModI(MMod *mod);
    void lowerMulI(MMul *mul, MDefinition *lhs, MDefinition *rhs);
    void lowerUDiv(MDiv *div);
    void lowerUMod(MMod *mod);
    void visitPowHalf(MPowHalf *ins);
    void visitAsmJSNeg(MAsmJSNeg *ins);

    LTableSwitch *newLTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                                  MTableSwitch *ins);
    LTableSwitchV *newLTableSwitchV(MTableSwitch *ins);

  public:
    void visitConstant(MConstant *ins);
    void visitBox(MBox *box);
    void visitUnbox(MUnbox *unbox);
    void visitReturn(MReturn *ret);
    void lowerPhi(MPhi *phi);
    void visitGuardShape(MGuardShape *ins);
    void visitGuardObjectType(MGuardObjectType *ins);
    void visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins);
    void visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins);
    void visitAsmJSLoadHeap(MAsmJSLoadHeap *ins);
    void visitAsmJSStoreHeap(MAsmJSStoreHeap *ins);
    void visitAsmJSCompareExchangeHeap(MAsmJSCompareExchangeHeap *ins);
    void visitAsmJSAtomicBinopHeap(MAsmJSAtomicBinopHeap *ins);
    void visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins);
    void visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins);
    void visitForkJoinGetSlice(MForkJoinGetSlice *ins);
    void visitSimdBinaryArith(MSimdBinaryArith *ins);
    void visitSimdTernaryBitwise(MSimdTernaryBitwise *ins);
    void visitSimdSplatX4(MSimdSplatX4 *ins);
    void visitSimdValueX4(MSimdValueX4 *ins);
    void visitCompareExchangeTypedArrayElement(MCompareExchangeTypedArrayElement *ins);
    void visitAtomicTypedArrayElementBinop(MAtomicTypedArrayElementBinop *ins);
    void visitSubstr(MSubstr *ins);
};

typedef LIRGeneratorMIPS LIRGeneratorSpecific;

} 
} 

#endif 
