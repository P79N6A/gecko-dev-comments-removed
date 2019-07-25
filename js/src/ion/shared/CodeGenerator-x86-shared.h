








































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

  protected:
    bool generatePrologue();
    bool generateEpilogue();
    bool generateOutOfLineCode();

    bool emitDoubleToInt32(const FloatRegister &src, const Register &dest, LSnapshot *snapshot);

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph);

  public:
    
    virtual bool visitGoto(LGoto *jump);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitBitNot(LBitNot *ins);
    virtual bool visitBitOp(LBitOp *ins);
    virtual bool visitMoveGroup(LMoveGroup *group);
    virtual bool visitInteger(LInteger *ins);
    virtual bool visitTestIAndBranch(LTestIAndBranch *test);
    virtual bool visitCompareI(LCompareI *comp);
    virtual bool visitCompareIAndBranch(LCompareIAndBranch *comp);
    virtual bool visitMathD(LMathD *math);
    virtual bool visitTableSwitch(LTableSwitch *ins);

    
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

