








































#ifndef jsion_codegen_x86_shared_h__
#define jsion_codegen_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class OutOfLineBailout;

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

    enum NaNCond {
        NaN_Unexpected,
        NaN_IsTrue,
        NaN_IsFalse
    };

    
    void emitSet(Assembler::Condition cond, const Register &dest, NaNCond ifNaN = NaN_Unexpected);

    
    
    void emitBranch(Assembler::Condition cond, MBasicBlock *ifTrue, MBasicBlock *ifFalse,
                    NaNCond ifNaN = NaN_Unexpected);

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool callVM(const VMFunction *f, LInstruction *ins);

  public:
    
    virtual bool visitGoto(LGoto *jump);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitSubI(LSubI *ins);
    virtual bool visitMulI(LMulI *ins);
    virtual bool visitDivI(LDivI *ins);
    virtual bool visitBitNot(LBitNot *ins);
    virtual bool visitBitOp(LBitOp *ins);
    virtual bool visitShiftOp(LShiftOp *ins);
    virtual bool visitMoveGroup(LMoveGroup *group);
    virtual bool visitInteger(LInteger *ins);
    virtual bool visitTestIAndBranch(LTestIAndBranch *test);
    virtual bool visitTestDAndBranch(LTestDAndBranch *test);
    virtual bool visitCompareI(LCompareI *comp);
    virtual bool visitCompareIAndBranch(LCompareIAndBranch *comp);
    virtual bool visitCompareD(LCompareD *comp);
    virtual bool visitCompareDAndBranch(LCompareDAndBranch *comp);
    virtual bool visitMathD(LMathD *math);
    virtual bool visitTableSwitch(LTableSwitch *ins);
    virtual bool visitCallGeneric(LCallGeneric *call);

    
    bool visitOutOfLineBailout(OutOfLineBailout *ool);
};


class OutOfLineBailout : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LSnapshot *snapshot_;
    uint32 frameSize_;

  public:
    OutOfLineBailout(LSnapshot *snapshot, uint32 frameSize)
      : snapshot_(snapshot),
        frameSize_(frameSize)
    { }

    bool accept(CodeGeneratorX86Shared *codegen);

    LSnapshot *snapshot() const {
        return snapshot_;
    }
};

} 
} 

#endif 

