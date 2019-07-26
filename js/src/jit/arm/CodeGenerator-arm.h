





#ifndef jit_arm_CodeGenerator_arm_h
#define jit_arm_CodeGenerator_arm_h

#include "jit/arm/Assembler-arm.h"
#include "jit/shared/CodeGenerator-shared.h"

namespace js {
namespace jit {

class OutOfLineBailout;
class OutOfLineTableSwitch;

class CodeGeneratorARM : public CodeGeneratorShared
{
    friend class MoveResolverARM;

    CodeGeneratorARM *thisFromCtor() {return this;}

  protected:
    
    NonAssertingLabel returnLabel_;
    NonAssertingLabel deoptLabel_;
    
    
    
    inline Operand ToOperand(const LAllocation &a) {
        if (a.isGeneralReg())
            return Operand(a.toGeneralReg()->reg());
        if (a.isFloatReg())
            return Operand(a.toFloatReg()->reg());
        return Operand(StackPointer, ToStackOffset(&a));
    }
    inline Operand ToOperand(const LAllocation *a) {
        return ToOperand(*a);
    }
    inline Operand ToOperand(const LDefinition *def) {
        return ToOperand(def->output());
    }

    MoveResolver::MoveOperand toMoveOperand(const LAllocation *a) const;

    bool bailoutIf(Assembler::Condition condition, LSnapshot *snapshot);
    bool bailoutFrom(Label *label, LSnapshot *snapshot);
    bool bailout(LSnapshot *snapshot);

  protected:
    bool generatePrologue();
    bool generateEpilogue();
    bool generateOutOfLineCode();

    void emitRoundDouble(const FloatRegister &src, const Register &dest, Label *fail);

    
    
    void emitBranch(Assembler::Condition cond, MBasicBlock *ifTrue, MBasicBlock *ifFalse);

    bool emitTableSwitchDispatch(MTableSwitch *mir, const Register &index, const Register &base);

  public:
    
    virtual bool visitMinMaxD(LMinMaxD *ins);
    virtual bool visitAbsD(LAbsD *ins);
    virtual bool visitSqrtD(LSqrtD *ins);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitSubI(LSubI *ins);
    virtual bool visitBitNotI(LBitNotI *ins);
    virtual bool visitBitOpI(LBitOpI *ins);

    virtual bool visitMulI(LMulI *ins);

    virtual bool visitDivI(LDivI *ins);
    virtual bool visitSoftDivI(LSoftDivI *ins);
    virtual bool visitDivPowTwoI(LDivPowTwoI *ins);
    virtual bool visitModI(LModI *ins);
    virtual bool visitSoftModI(LSoftModI *ins);
    virtual bool visitModPowTwoI(LModPowTwoI *ins);
    virtual bool visitModMaskI(LModMaskI *ins);
    virtual bool visitPowHalfD(LPowHalfD *ins);
    virtual bool visitShiftI(LShiftI *ins);
    virtual bool visitUrshD(LUrshD *ins);

    virtual bool visitTestIAndBranch(LTestIAndBranch *test);
    virtual bool visitCompare(LCompare *comp);
    virtual bool visitCompareAndBranch(LCompareAndBranch *comp);
    virtual bool visitTestDAndBranch(LTestDAndBranch *test);
    virtual bool visitCompareD(LCompareD *comp);
    virtual bool visitCompareDAndBranch(LCompareDAndBranch *comp);
    virtual bool visitCompareB(LCompareB *lir);
    virtual bool visitCompareBAndBranch(LCompareBAndBranch *lir);
    virtual bool visitCompareV(LCompareV *lir);
    virtual bool visitCompareVAndBranch(LCompareVAndBranch *lir);
    virtual bool visitBitAndAndBranch(LBitAndAndBranch *baab);
    virtual bool visitUInt32ToDouble(LUInt32ToDouble *lir);
    virtual bool visitNotI(LNotI *ins);
    virtual bool visitNotD(LNotD *ins);

    virtual bool visitMathD(LMathD *math);
    virtual bool visitMathF(LMathF *math);
    virtual bool visitFloor(LFloor *lir);
    virtual bool visitRound(LRound *lir);
    virtual bool visitTruncateDToInt32(LTruncateDToInt32 *ins);
    virtual bool visitTruncateFToInt32(LTruncateFToInt32 *ins);

    
    bool visitOutOfLineBailout(OutOfLineBailout *ool);
    bool visitOutOfLineTableSwitch(OutOfLineTableSwitch *ool);

  protected:
    ValueOperand ToValue(LInstruction *ins, size_t pos);
    ValueOperand ToOutValue(LInstruction *ins);
    ValueOperand ToTempValue(LInstruction *ins, size_t pos);

    
    Register splitTagForTest(const ValueOperand &value);

    void storeElementTyped(const LAllocation *value, MIRType valueType, MIRType elementType,
                           const Register &elements, const LAllocation *index);

    bool divICommon(MDiv *mir, Register lhs, Register rhs, Register output, LSnapshot *snapshot,
                    Label &done);
    bool modICommon(MMod *mir, Register lhs, Register rhs, Register output, LSnapshot *snapshot,
                    Label &done);

  public:
    CodeGeneratorARM(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    bool visitBox(LBox *box);
    bool visitBoxFloatingPoint(LBoxFloatingPoint *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitValue(LValue *value);
    bool visitOsrValue(LOsrValue *value);
    bool visitDouble(LDouble *ins);
    bool visitFloat32(LFloat32 *ins);

    bool visitLoadSlotV(LLoadSlotV *load);
    bool visitLoadSlotT(LLoadSlotT *load);
    bool visitStoreSlotT(LStoreSlotT *load);

    bool visitLoadElementT(LLoadElementT *load);

    bool visitGuardShape(LGuardShape *guard);
    bool visitGuardObjectType(LGuardObjectType *guard);
    bool visitGuardClass(LGuardClass *guard);
    bool visitImplicitThis(LImplicitThis *lir);

    bool visitInterruptCheck(LInterruptCheck *lir);

    bool visitNegI(LNegI *lir);
    bool visitNegD(LNegD *lir);
    bool visitNegF(LNegF *lir);
    bool visitLoadTypedArrayElementStatic(LLoadTypedArrayElementStatic *ins);
    bool visitStoreTypedArrayElementStatic(LStoreTypedArrayElementStatic *ins);
    bool visitAsmJSLoadHeap(LAsmJSLoadHeap *ins);
    bool visitAsmJSStoreHeap(LAsmJSStoreHeap *ins);
    bool visitAsmJSLoadGlobalVar(LAsmJSLoadGlobalVar *ins);
    bool visitAsmJSStoreGlobalVar(LAsmJSStoreGlobalVar *ins);
    bool visitAsmJSLoadFuncPtr(LAsmJSLoadFuncPtr *ins);
    bool visitAsmJSLoadFFIFunc(LAsmJSLoadFFIFunc *ins);

    bool visitAsmJSPassStackArg(LAsmJSPassStackArg *ins);

    bool generateInvalidateEpilogue();
  protected:
    void postAsmJSCall(LAsmJSCall *lir) {
#if  !defined(JS_CPU_ARM_HARDFP)
        if (lir->mir()->type() == MIRType_Double) {
            masm.ma_vxfer(r0, r1, d0);
        }
#endif
}

    bool visitEffectiveAddress(LEffectiveAddress *ins);
    bool visitUDiv(LUDiv *ins);
    bool visitUMod(LUMod *ins);
    bool visitSoftUDivOrMod(LSoftUDivOrMod *ins);
};

typedef CodeGeneratorARM CodeGeneratorSpecific;


class OutOfLineBailout : public OutOfLineCodeBase<CodeGeneratorARM>
{
    LSnapshot *snapshot_;
    uint32_t frameSize_;

  public:
    OutOfLineBailout(LSnapshot *snapshot, uint32_t frameSize)
      : snapshot_(snapshot),
        frameSize_(frameSize)
    { }

    bool accept(CodeGeneratorARM *codegen);

    LSnapshot *snapshot() const {
        return snapshot_;
    }
};

} 
} 

#endif 
