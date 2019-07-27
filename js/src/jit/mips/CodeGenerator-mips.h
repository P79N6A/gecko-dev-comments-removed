





#ifndef jit_mips_CodeGenerator_mips_h
#define jit_mips_CodeGenerator_mips_h

#include "jit/mips/Assembler-mips.h"
#include "jit/shared/CodeGenerator-shared.h"

namespace js {
namespace jit {

class OutOfLineBailout;
class OutOfLineTableSwitch;

class CodeGeneratorMIPS : public CodeGeneratorShared
{
    friend class MoveResolverMIPS;

    CodeGeneratorMIPS *thisFromCtor() {
        return this;
    }

  protected:
    
    NonAssertingLabel returnLabel_;
    NonAssertingLabel deoptLabel_;

    inline Address ToAddress(const LAllocation &a) {
        MOZ_ASSERT(a.isMemory());
        int32_t offset = ToStackOffset(&a);

        return Address(StackPointer, offset);
    }

    inline Address ToAddress(const LAllocation *a) {
        return ToAddress(*a);
    }

    inline Operand ToOperand(const LAllocation &a) {
        if (a.isGeneralReg())
            return Operand(a.toGeneralReg()->reg());
        if (a.isFloatReg())
            return Operand(a.toFloatReg()->reg());

        MOZ_ASSERT(a.isMemory());
        int32_t offset = ToStackOffset(&a);

        return Operand(StackPointer, offset);
    }
    inline Operand ToOperand(const LAllocation *a) {
        return ToOperand(*a);
    }
    inline Operand ToOperand(const LDefinition *def) {
        return ToOperand(def->output());
    }

    MoveOperand toMoveOperand(const LAllocation *a) const;

    template <typename T1, typename T2>
    bool bailoutCmp32(Assembler::Condition c, T1 lhs, T2 rhs, LSnapshot *snapshot) {
        bool goodBailout;
        Label skip;
        masm.ma_b(lhs, rhs, &skip, Assembler::InvertCondition(c), ShortJump);
        goodBailout = bailout(snapshot);
        masm.bind(&skip);
        return goodBailout;
    }
    template<typename T>
    bool bailoutCmp32(Assembler::Condition c, Operand lhs, T rhs, LSnapshot *snapshot) {
        if (lhs.getTag() == Operand::REG)
              return bailoutCmp32(c, lhs.toReg(), rhs, snapshot);
        if (lhs.getTag() == Operand::MEM)
              return bailoutCmp32(c, lhs.toAddress(), rhs, snapshot);
        MOZ_CRASH("Invalid operand tag.");
    }
    template<typename T>
    bool bailoutTest32(Assembler::Condition c, Register lhs, T rhs, LSnapshot *snapshot) {
        Label bail;
        masm.branchTest32(c, lhs, rhs, &bail);
        return bailoutFrom(&bail, snapshot);
    }
    template <typename T1, typename T2>
    bool bailoutCmpPtr(Assembler::Condition c, T1 lhs, T2 rhs, LSnapshot *snapshot) {
        return bailoutCmp32(c, lhs, rhs, snapshot);
    }
    bool bailoutTestPtr(Assembler::Condition c, Register lhs, Register rhs, LSnapshot *snapshot) {
        Label bail;
        masm.branchTestPtr(c, lhs, rhs, &bail);
        return bailoutFrom(&bail, snapshot);
    }
    bool bailoutIfFalseBool(Register reg, LSnapshot *snapshot) {
        Label bail;
        masm.branchTest32(Assembler::Zero, reg, Imm32(0xFF), &bail);
        return bailoutFrom(&bail, snapshot);
    }

    bool bailoutFrom(Label *label, LSnapshot *snapshot);
    bool bailout(LSnapshot *snapshot);

  protected:
    bool generatePrologue();
    bool generateEpilogue();
    bool generateOutOfLineCode();

    template <typename T>
    void branchToBlock(Register lhs, T rhs, MBasicBlock *mir, Assembler::Condition cond)
    {
        mir = skipTrivialBlocks(mir);

        Label *label = mir->lir()->label();
        if (Label *oolEntry = labelForBackedgeWithImplicitCheck(mir)) {
            
            
            RepatchLabel rejoin;
            CodeOffsetJump backedge;
            Label skip;

            masm.ma_b(lhs, rhs, &skip, Assembler::InvertCondition(cond), ShortJump);
            backedge = masm.backedgeJump(&rejoin);
            masm.bind(&rejoin);
            masm.bind(&skip);

            if (!patchableBackedges_.append(PatchableBackedgeInfo(backedge, label, oolEntry)))
                MOZ_CRASH();
        } else {
            masm.ma_b(lhs, rhs, label, cond);
        }
    }
    void branchToBlock(Assembler::FloatFormat fmt, FloatRegister lhs, FloatRegister rhs,
                       MBasicBlock *mir, Assembler::DoubleCondition cond);

    
    
    template <typename T>
    void emitBranch(Register lhs, T rhs, Assembler::Condition cond,
                    MBasicBlock *mirTrue, MBasicBlock *mirFalse)
    {
        if (isNextBlock(mirFalse->lir())) {
            branchToBlock(lhs, rhs, mirTrue, cond);
        } else {
            branchToBlock(lhs, rhs, mirFalse, Assembler::InvertCondition(cond));
            jumpToBlock(mirTrue);
        }
    }
    void testNullEmitBranch(Assembler::Condition cond, const ValueOperand &value,
                            MBasicBlock *ifTrue, MBasicBlock *ifFalse)
    {
        emitBranch(value.typeReg(), (Imm32)ImmType(JSVAL_TYPE_NULL), cond, ifTrue, ifFalse);
    }
    void testUndefinedEmitBranch(Assembler::Condition cond, const ValueOperand &value,
                                 MBasicBlock *ifTrue, MBasicBlock *ifFalse)
    {
        emitBranch(value.typeReg(), (Imm32)ImmType(JSVAL_TYPE_UNDEFINED), cond, ifTrue, ifFalse);
    }

    bool emitTableSwitchDispatch(MTableSwitch *mir, Register index, Register base);

  public:
    
    virtual bool visitMinMaxD(LMinMaxD *ins);
    virtual bool visitAbsD(LAbsD *ins);
    virtual bool visitAbsF(LAbsF *ins);
    virtual bool visitSqrtD(LSqrtD *ins);
    virtual bool visitSqrtF(LSqrtF *ins);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitSubI(LSubI *ins);
    virtual bool visitBitNotI(LBitNotI *ins);
    virtual bool visitBitOpI(LBitOpI *ins);

    virtual bool visitMulI(LMulI *ins);

    virtual bool visitDivI(LDivI *ins);
    virtual bool visitDivPowTwoI(LDivPowTwoI *ins);
    virtual bool visitModI(LModI *ins);
    virtual bool visitModPowTwoI(LModPowTwoI *ins);
    virtual bool visitModMaskI(LModMaskI *ins);
    virtual bool visitPowHalfD(LPowHalfD *ins);
    virtual bool visitShiftI(LShiftI *ins);
    virtual bool visitUrshD(LUrshD *ins);

    virtual bool visitClzI(LClzI *ins);

    virtual bool visitTestIAndBranch(LTestIAndBranch *test);
    virtual bool visitCompare(LCompare *comp);
    virtual bool visitCompareAndBranch(LCompareAndBranch *comp);
    virtual bool visitTestDAndBranch(LTestDAndBranch *test);
    virtual bool visitTestFAndBranch(LTestFAndBranch *test);
    virtual bool visitCompareD(LCompareD *comp);
    virtual bool visitCompareF(LCompareF *comp);
    virtual bool visitCompareDAndBranch(LCompareDAndBranch *comp);
    virtual bool visitCompareFAndBranch(LCompareFAndBranch *comp);
    virtual bool visitCompareB(LCompareB *lir);
    virtual bool visitCompareBAndBranch(LCompareBAndBranch *lir);
    virtual bool visitCompareV(LCompareV *lir);
    virtual bool visitCompareVAndBranch(LCompareVAndBranch *lir);
    virtual bool visitBitAndAndBranch(LBitAndAndBranch *lir);
    virtual bool visitAsmJSUInt32ToDouble(LAsmJSUInt32ToDouble *lir);
    virtual bool visitAsmJSUInt32ToFloat32(LAsmJSUInt32ToFloat32 *lir);
    virtual bool visitNotI(LNotI *ins);
    virtual bool visitNotD(LNotD *ins);
    virtual bool visitNotF(LNotF *ins);

    virtual bool visitMathD(LMathD *math);
    virtual bool visitMathF(LMathF *math);
    virtual bool visitFloor(LFloor *lir);
    virtual bool visitFloorF(LFloorF *lir);
    virtual bool visitCeil(LCeil *lir);
    virtual bool visitCeilF(LCeilF *lir);
    virtual bool visitRound(LRound *lir);
    virtual bool visitRoundF(LRoundF *lir);
    virtual bool visitTruncateDToInt32(LTruncateDToInt32 *ins);
    virtual bool visitTruncateFToInt32(LTruncateFToInt32 *ins);

    
    bool visitOutOfLineBailout(OutOfLineBailout *ool);
    bool visitOutOfLineTableSwitch(OutOfLineTableSwitch *ool);

  protected:
    ValueOperand ToValue(LInstruction *ins, size_t pos);
    ValueOperand ToOutValue(LInstruction *ins);
    ValueOperand ToTempValue(LInstruction *ins, size_t pos);

    
    Register splitTagForTest(const ValueOperand &value);

  public:
    CodeGeneratorMIPS(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    bool visitBox(LBox *box);
    bool visitBoxFloatingPoint(LBoxFloatingPoint *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitValue(LValue *value);
    bool visitDouble(LDouble *ins);
    bool visitFloat32(LFloat32 *ins);

    bool visitGuardShape(LGuardShape *guard);
    bool visitGuardObjectType(LGuardObjectType *guard);
    bool visitGuardClass(LGuardClass *guard);

    bool visitNegI(LNegI *lir);
    bool visitNegD(LNegD *lir);
    bool visitNegF(LNegF *lir);
    bool visitLoadTypedArrayElementStatic(LLoadTypedArrayElementStatic *ins);
    bool visitStoreTypedArrayElementStatic(LStoreTypedArrayElementStatic *ins);
    bool visitAsmJSCall(LAsmJSCall *ins);
    bool visitAsmJSLoadHeap(LAsmJSLoadHeap *ins);
    bool visitAsmJSStoreHeap(LAsmJSStoreHeap *ins);
    bool visitAsmJSLoadGlobalVar(LAsmJSLoadGlobalVar *ins);
    bool visitAsmJSStoreGlobalVar(LAsmJSStoreGlobalVar *ins);
    bool visitAsmJSLoadFuncPtr(LAsmJSLoadFuncPtr *ins);
    bool visitAsmJSLoadFFIFunc(LAsmJSLoadFFIFunc *ins);

    bool visitAsmJSPassStackArg(LAsmJSPassStackArg *ins);

    bool visitForkJoinGetSlice(LForkJoinGetSlice *ins);

    bool generateInvalidateEpilogue();

  protected:
    bool visitEffectiveAddress(LEffectiveAddress *ins);
    bool visitUDiv(LUDiv *ins);
    bool visitUMod(LUMod *ins);

  public:
    
    bool visitSimdSplatX4(LSimdSplatX4 *lir) { MOZ_CRASH("NYI"); }
    bool visitInt32x4(LInt32x4 *ins) { MOZ_CRASH("NYI"); }
    bool visitFloat32x4(LFloat32x4 *ins) { MOZ_CRASH("NYI"); }
    bool visitSimdExtractElementI(LSimdExtractElementI *ins) { MOZ_CRASH("NYI"); }
    bool visitSimdExtractElementF(LSimdExtractElementF *ins) { MOZ_CRASH("NYI"); }
    bool visitSimdSignMaskX4(LSimdSignMaskX4 *ins) { MOZ_CRASH("NYI"); }
    bool visitSimdBinaryCompIx4(LSimdBinaryCompIx4 *lir) { MOZ_CRASH("NYI"); }
    bool visitSimdBinaryCompFx4(LSimdBinaryCompFx4 *lir) { MOZ_CRASH("NYI"); }
    bool visitSimdBinaryArithIx4(LSimdBinaryArithIx4 *lir) { MOZ_CRASH("NYI"); }
    bool visitSimdBinaryArithFx4(LSimdBinaryArithFx4 *lir) { MOZ_CRASH("NYI"); }
    bool visitSimdBinaryBitwiseX4(LSimdBinaryBitwiseX4 *lir) { MOZ_CRASH("NYI"); }
};

typedef CodeGeneratorMIPS CodeGeneratorSpecific;


class OutOfLineBailout : public OutOfLineCodeBase<CodeGeneratorMIPS>
{
    LSnapshot *snapshot_;
    uint32_t frameSize_;

  public:
    OutOfLineBailout(LSnapshot *snapshot, uint32_t frameSize)
      : snapshot_(snapshot),
        frameSize_(frameSize)
    { }

    bool accept(CodeGeneratorMIPS *codegen);

    LSnapshot *snapshot() const {
        return snapshot_;
    }
};

} 
} 

#endif 
