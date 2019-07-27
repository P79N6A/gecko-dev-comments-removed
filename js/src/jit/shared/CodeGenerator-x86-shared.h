





#ifndef jit_shared_CodeGenerator_x86_shared_h
#define jit_shared_CodeGenerator_x86_shared_h

#include "jit/shared/CodeGenerator-shared.h"

namespace js {
namespace jit {

class OutOfLineBailout;
class OutOfLineUndoALUOperation;
class OutOfLineLoadTypedArrayOutOfBounds;
class MulNegativeZeroCheck;
class ModOverflowCheck;
class ReturnZero;
class OutOfLineTableSwitch;

class CodeGeneratorX86Shared : public CodeGeneratorShared
{
    friend class MoveResolverX86;

    CodeGeneratorX86Shared *thisFromCtor() {
        return this;
    }

    template <typename T>
    bool bailout(const T &t, LSnapshot *snapshot);

  protected:

    
    
    class OutOfLineLoadTypedArrayOutOfBounds : public OutOfLineCodeBase<CodeGeneratorX86Shared>
    {
        AnyRegister dest_;
        bool isFloat32Load_;
      public:
        OutOfLineLoadTypedArrayOutOfBounds(AnyRegister dest, bool isFloat32Load)
          : dest_(dest), isFloat32Load_(isFloat32Load)
        {}

        AnyRegister dest() const { return dest_; }
        bool isFloat32Load() const { return isFloat32Load_; }
        bool accept(CodeGeneratorX86Shared *codegen) {
            return codegen->visitOutOfLineLoadTypedArrayOutOfBounds(this);
        }
    };

    
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

    MoveOperand toMoveOperand(const LAllocation *a) const;

    bool bailoutIf(Assembler::Condition condition, LSnapshot *snapshot);
    bool bailoutIf(Assembler::DoubleCondition condition, LSnapshot *snapshot);
    bool bailoutFrom(Label *label, LSnapshot *snapshot);
    bool bailout(LSnapshot *snapshot);

    template <typename T1, typename T2>
    bool bailoutCmpPtr(Assembler::Condition c, T1 lhs, T2 rhs, LSnapshot *snapshot) {
        masm.cmpPtr(lhs, rhs);
        return bailoutIf(c, snapshot);
    }
    bool bailoutTestPtr(Assembler::Condition c, Register lhs, Register rhs, LSnapshot *snapshot) {
        masm.testPtr(lhs, rhs);
        return bailoutIf(c, snapshot);
    }
    template <typename T1, typename T2>
    bool bailoutCmp32(Assembler::Condition c, T1 lhs, T2 rhs, LSnapshot *snapshot) {
        masm.cmp32(lhs, rhs);
        return bailoutIf(c, snapshot);
    }
    template <typename T1, typename T2>
    bool bailoutTest32(Assembler::Condition c, T1 lhs, T2 rhs, LSnapshot *snapshot) {
        masm.test32(lhs, rhs);
        return bailoutIf(c, snapshot);
    }
    bool bailoutIfFalseBool(Register reg, LSnapshot *snapshot) {
        masm.test32(reg, Imm32(0xFF));
        return bailoutIf(Assembler::Zero, snapshot);
    }
    bool bailoutCvttsd2si(FloatRegister src, Register dest, LSnapshot *snapshot) {
        
        
        
        
        masm.cvttsd2si(src, dest);
        masm.cmp32(dest, Imm32(1));
        return bailoutIf(Assembler::Overflow, snapshot);
    }
    bool bailoutCvttss2si(FloatRegister src, Register dest, LSnapshot *snapshot) {
        
        masm.cvttss2si(src, dest);
        masm.cmp32(dest, Imm32(1));
        return bailoutIf(Assembler::Overflow, snapshot);
    }

  protected:
    bool generatePrologue();
    bool generateEpilogue();
    bool generateOutOfLineCode();

    void emitCompare(MCompare::CompareType type, const LAllocation *left, const LAllocation *right);

    
    
    void emitBranch(Assembler::Condition cond, MBasicBlock *ifTrue, MBasicBlock *ifFalse,
                    Assembler::NaNCond ifNaN = Assembler::NaN_HandledByCond);
    void emitBranch(Assembler::DoubleCondition cond, MBasicBlock *ifTrue, MBasicBlock *ifFalse);

    void testNullEmitBranch(Assembler::Condition cond, const ValueOperand &value,
                            MBasicBlock *ifTrue, MBasicBlock *ifFalse)
    {
        cond = masm.testNull(cond, value);
        emitBranch(cond, ifTrue, ifFalse);
    }
    void testUndefinedEmitBranch(Assembler::Condition cond, const ValueOperand &value,
                                 MBasicBlock *ifTrue, MBasicBlock *ifFalse)
    {
        cond = masm.testUndefined(cond, value);
        emitBranch(cond, ifTrue, ifFalse);
    }

    bool emitTableSwitchDispatch(MTableSwitch *mir, Register index, Register base);

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    
    virtual bool visitDouble(LDouble *ins);
    virtual bool visitFloat32(LFloat32 *ins);
    virtual bool visitMinMaxD(LMinMaxD *ins);
    virtual bool visitAbsD(LAbsD *ins);
    virtual bool visitAbsF(LAbsF *ins);
    virtual bool visitClzI(LClzI *ins);
    virtual bool visitSqrtD(LSqrtD *ins);
    virtual bool visitSqrtF(LSqrtF *ins);
    virtual bool visitPowHalfD(LPowHalfD *ins);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitSubI(LSubI *ins);
    virtual bool visitMulI(LMulI *ins);
    virtual bool visitDivI(LDivI *ins);
    virtual bool visitDivPowTwoI(LDivPowTwoI *ins);
    virtual bool visitDivOrModConstantI(LDivOrModConstantI *ins);
    virtual bool visitModI(LModI *ins);
    virtual bool visitModPowTwoI(LModPowTwoI *ins);
    virtual bool visitBitNotI(LBitNotI *ins);
    virtual bool visitBitOpI(LBitOpI *ins);
    virtual bool visitShiftI(LShiftI *ins);
    virtual bool visitUrshD(LUrshD *ins);
    virtual bool visitTestIAndBranch(LTestIAndBranch *test);
    virtual bool visitTestDAndBranch(LTestDAndBranch *test);
    virtual bool visitTestFAndBranch(LTestFAndBranch *test);
    virtual bool visitCompare(LCompare *comp);
    virtual bool visitCompareAndBranch(LCompareAndBranch *comp);
    virtual bool visitCompareD(LCompareD *comp);
    virtual bool visitCompareDAndBranch(LCompareDAndBranch *comp);
    virtual bool visitCompareF(LCompareF *comp);
    virtual bool visitCompareFAndBranch(LCompareFAndBranch *comp);
    virtual bool visitBitAndAndBranch(LBitAndAndBranch *baab);
    virtual bool visitNotI(LNotI *comp);
    virtual bool visitNotD(LNotD *comp);
    virtual bool visitNotF(LNotF *comp);
    virtual bool visitMathD(LMathD *math);
    virtual bool visitMathF(LMathF *math);
    virtual bool visitFloor(LFloor *lir);
    virtual bool visitFloorF(LFloorF *lir);
    virtual bool visitCeil(LCeil *lir);
    virtual bool visitCeilF(LCeilF *lir);
    virtual bool visitRound(LRound *lir);
    virtual bool visitRoundF(LRoundF *lir);
    virtual bool visitGuardShape(LGuardShape *guard);
    virtual bool visitGuardObjectType(LGuardObjectType *guard);
    virtual bool visitGuardClass(LGuardClass *guard);
    virtual bool visitEffectiveAddress(LEffectiveAddress *ins);
    virtual bool visitUDivOrMod(LUDivOrMod *ins);
    virtual bool visitAsmJSPassStackArg(LAsmJSPassStackArg *ins);

    bool visitOutOfLineLoadTypedArrayOutOfBounds(OutOfLineLoadTypedArrayOutOfBounds *ool);

    bool visitForkJoinGetSlice(LForkJoinGetSlice *ins);

    bool visitNegI(LNegI *lir);
    bool visitNegD(LNegD *lir);
    bool visitNegF(LNegF *lir);

    
    bool visitSimdValueX4(LSimdValueX4 *lir);
    bool visitInt32x4(LInt32x4 *ins);
    bool visitFloat32x4(LFloat32x4 *ins);
    bool visitSimdExtractElementI(LSimdExtractElementI *lir);
    bool visitSimdExtractElementF(LSimdExtractElementF *lir);
    bool visitSimdBinaryArithIx4(LSimdBinaryArithIx4 *lir);
    bool visitSimdBinaryArithFx4(LSimdBinaryArithFx4 *lir);

    
    bool visitOutOfLineBailout(OutOfLineBailout *ool);
    bool visitOutOfLineUndoALUOperation(OutOfLineUndoALUOperation *ool);
    bool visitMulNegativeZeroCheck(MulNegativeZeroCheck *ool);
    bool visitModOverflowCheck(ModOverflowCheck *ool);
    bool visitReturnZero(ReturnZero *ool);
    bool visitOutOfLineTableSwitch(OutOfLineTableSwitch *ool);
    bool generateInvalidateEpilogue();
};


class OutOfLineBailout : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LSnapshot *snapshot_;

  public:
    explicit OutOfLineBailout(LSnapshot *snapshot)
      : snapshot_(snapshot)
    { }

    bool accept(CodeGeneratorX86Shared *codegen);

    LSnapshot *snapshot() const {
        return snapshot_;
    }
};

} 
} 

#endif 
