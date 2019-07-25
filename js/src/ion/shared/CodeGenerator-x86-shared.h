








































#ifndef jsion_codegen_x86_shared_h__
#define jsion_codegen_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class OutOfLineBailout;
class MulNegativeZeroCheck;

class CodeGeneratorX86Shared : public CodeGeneratorShared
{
    friend class MoveResolverX86;

    CodeGeneratorX86Shared *thisFromCtor() {
        return this;
    }

    template <typename T>
    bool bailout(const T &t, LSnapshot *snapshot);

  protected:
    
    HeapLabel *returnLabel_;
    HeapLabel *deoptLabel_;

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

  protected:
    bool generatePrologue();
    bool generateEpilogue();
    bool generateOutOfLineCode();

    void emitDoubleToInt32(const FloatRegister &src, const Register &dest, Label *fail);
    void emitTruncateDouble(const FloatRegister &src, const Register &dest, Label *fail);

    Operand createArrayElementOperand(Register elements, const LAllocation *index);

    enum NaNCond {
        NaN_Unexpected,
        NaN_IsTrue,
        NaN_IsFalse
    };

    void emitCompare(MIRType type, const LAllocation *left, const LAllocation *right);

    
    void emitSet(Assembler::Condition cond, const Register &dest, NaNCond ifNaN = NaN_Unexpected);

    
    
    void emitBranch(Assembler::Condition cond, MBasicBlock *ifTrue, MBasicBlock *ifFalse,
                    NaNCond ifNaN = NaN_Unexpected);

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph);

  public:
    
    virtual bool visitAbsD(LAbsD *ins);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitSubI(LSubI *ins);
    virtual bool visitMulI(LMulI *ins);
    virtual bool visitDivI(LDivI *ins);
    virtual bool visitModI(LModI *ins);
    virtual bool visitBitNotI(LBitNotI *ins);
    virtual bool visitBitOpI(LBitOpI *ins);
    virtual bool visitShiftOp(LShiftOp *ins);
    virtual bool visitMoveGroup(LMoveGroup *group);
    virtual bool visitTestIAndBranch(LTestIAndBranch *test);
    virtual bool visitTestDAndBranch(LTestDAndBranch *test);
    virtual bool visitCompare(LCompare *comp);
    virtual bool visitCompareAndBranch(LCompareAndBranch *comp);
    virtual bool visitCompareD(LCompareD *comp);
    virtual bool visitCompareDAndBranch(LCompareDAndBranch *comp);
    virtual bool visitNotI(LNotI *comp);
    virtual bool visitNotD(LNotD *comp);
    virtual bool visitMathD(LMathD *math);
    virtual bool visitRound(LRound *lir);
    virtual bool visitTableSwitch(LTableSwitch *ins);
    virtual bool visitGuardShape(LGuardShape *guard);
    virtual bool visitGuardClass(LGuardClass *guard);

    
    bool visitOutOfLineBailout(OutOfLineBailout *ool);
    bool visitMulNegativeZeroCheck(MulNegativeZeroCheck *ool);
    bool generateInvalidateEpilogue();
};


class OutOfLineBailout : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LSnapshot *snapshot_;

  public:
    OutOfLineBailout(LSnapshot *snapshot)
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

