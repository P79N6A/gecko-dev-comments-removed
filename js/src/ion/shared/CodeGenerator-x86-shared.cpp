






#include "mozilla/DebugOnly.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsmath.h"
#include "CodeGenerator-x86-shared.h"
#include "CodeGenerator-shared-inl.h"
#include "ion/IonFrames.h"
#include "ion/MoveEmitter.h"
#include "ion/IonCompartment.h"
#include "ion/ParallelFunctions.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {

CodeGeneratorX86Shared::CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph *graph)
  : CodeGeneratorShared(gen, graph),
    deoptLabel_(NULL)
{
}

double
test(double x, double y)
{
    return x + y;
}

bool
CodeGeneratorX86Shared::generatePrologue()
{
    
    masm.reserveStack(frameSize());

    
    
    returnLabel_ = new HeapLabel();

    return true;
}

bool
CodeGeneratorX86Shared::generateEpilogue()
{
    masm.bind(returnLabel_);

    
    masm.freeStack(frameSize());
    JS_ASSERT(masm.framePushed() == 0);

    masm.ret();
    return true;
}

bool
OutOfLineBailout::accept(CodeGeneratorX86Shared *codegen)
{
    return codegen->visitOutOfLineBailout(this);
}

void
CodeGeneratorX86Shared::emitBranch(Assembler::Condition cond, MBasicBlock *mirTrue,
                                   MBasicBlock *mirFalse, Assembler::NaNCond ifNaN)
{
    LBlock *ifTrue = mirTrue->lir();
    LBlock *ifFalse = mirFalse->lir();

    if (ifNaN == Assembler::NaN_IsFalse)
        masm.j(Assembler::Parity, ifFalse->label());
    else if (ifNaN == Assembler::NaN_IsTrue)
        masm.j(Assembler::Parity, ifTrue->label());

    if (isNextBlock(ifFalse)) {
        masm.j(cond, ifTrue->label());
    } else {
        masm.j(Assembler::InvertCondition(cond), ifFalse->label());
        if (!isNextBlock(ifTrue))
            masm.jmp(ifTrue->label());
    }
}

bool
CodeGeneratorX86Shared::visitDouble(LDouble *ins)
{
    const LDefinition *out = ins->getDef(0);
    masm.loadConstantDouble(ins->getDouble(), ToFloatRegister(out));
    return true;
}

bool
CodeGeneratorX86Shared::visitTestIAndBranch(LTestIAndBranch *test)
{
    const LAllocation *opd = test->input();

    
    masm.testl(ToRegister(opd), ToRegister(opd));
    emitBranch(Assembler::NonZero, test->ifTrue(), test->ifFalse());
    return true;
}

bool
CodeGeneratorX86Shared::visitTestDAndBranch(LTestDAndBranch *test)
{
    const LAllocation *opd = test->input();

    
    
    
    
    
    
    
    
    
    
    masm.xorpd(ScratchFloatReg, ScratchFloatReg);
    masm.ucomisd(ToFloatRegister(opd), ScratchFloatReg);
    emitBranch(Assembler::NotEqual, test->ifTrue(), test->ifFalse());
    return true;
}

void
CodeGeneratorX86Shared::emitCompare(MCompare::CompareType type, const LAllocation *left, const LAllocation *right)
{
#ifdef JS_CPU_X64
    if (type == MCompare::Compare_Object) {
        masm.cmpq(ToRegister(left), ToOperand(right));
        return;
    }
#endif

    if (right->isConstant())
        masm.cmpl(ToRegister(left), Imm32(ToInt32(right)));
    else
        masm.cmpl(ToRegister(left), ToOperand(right));
}

bool
CodeGeneratorX86Shared::visitCompare(LCompare *comp)
{
    emitCompare(comp->mir()->compareType(), comp->left(), comp->right());
    masm.emitSet(JSOpToCondition(comp->jsop()), ToRegister(comp->output()));
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareAndBranch(LCompareAndBranch *comp)
{
    emitCompare(comp->mir()->compareType(), comp->left(), comp->right());
    Assembler::Condition cond = JSOpToCondition(comp->jsop());
    emitBranch(cond, comp->ifTrue(), comp->ifFalse());
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareD(LCompareD *comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());
    masm.compareDouble(cond, lhs, rhs);
    masm.emitSet(Assembler::ConditionFromDoubleCondition(cond), ToRegister(comp->output()),
            Assembler::NaNCondFromDoubleCondition(cond));
    return true;
}

bool
CodeGeneratorX86Shared::visitNotI(LNotI *ins)
{
    masm.cmpl(ToRegister(ins->input()), Imm32(0));
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()));
    return true;
}

bool
CodeGeneratorX86Shared::visitNotD(LNotD *ins)
{
    FloatRegister opd = ToFloatRegister(ins->input());

    masm.xorpd(ScratchFloatReg, ScratchFloatReg);
    masm.compareDouble(Assembler::DoubleEqualOrUnordered, opd, ScratchFloatReg);
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()), Assembler::NaN_IsTrue);
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareDAndBranch(LCompareDAndBranch *comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());
    masm.compareDouble(cond, lhs, rhs);
    emitBranch(Assembler::ConditionFromDoubleCondition(cond), comp->ifTrue(), comp->ifFalse(),
               Assembler::NaNCondFromDoubleCondition(cond));
    return true;
}

bool
CodeGeneratorX86Shared::generateOutOfLineCode()
{
    if (!CodeGeneratorShared::generateOutOfLineCode())
        return false;

    if (deoptLabel_) {
        
        masm.bind(deoptLabel_);

        
        masm.push(Imm32(frameSize()));

        IonCompartment *ion = GetIonContext()->compartment->ionCompartment();
        IonCode *handler = ion->getGenericBailoutHandler();

        masm.jmp(handler->raw(), Relocation::IONCODE);
    }

    return true;
}

class BailoutJump {
    Assembler::Condition cond_;

  public:
    BailoutJump(Assembler::Condition cond) : cond_(cond)
    { }
#ifdef JS_CPU_X86
    void operator()(MacroAssembler &masm, uint8_t *code) const {
        masm.j(cond_, code, Relocation::HARDCODED);
    }
#endif
    void operator()(MacroAssembler &masm, Label *label) const {
        masm.j(cond_, label);
    }
};

class BailoutLabel {
    Label *label_;

  public:
    BailoutLabel(Label *label) : label_(label)
    { }
#ifdef JS_CPU_X86
    void operator()(MacroAssembler &masm, uint8_t *code) const {
        masm.retarget(label_, code, Relocation::HARDCODED);
    }
#endif
    void operator()(MacroAssembler &masm, Label *label) const {
        masm.retarget(label_, label);
    }
};

template <typename T> bool
CodeGeneratorX86Shared::bailout(const T &binder, LSnapshot *snapshot)
{
    CompileInfo &info = snapshot->mir()->block()->info();
    switch (info.executionMode()) {
      case ParallelExecution: {
        
        Label *ool;
        if (!ensureOutOfLineParallelAbort(&ool))
            return false;
        binder(masm, ool);
        return true;
      }

      case SequentialExecution: break;
    }

    if (!encode(snapshot))
        return false;

    
    
    
    JS_ASSERT_IF(frameClass_ != FrameSizeClass::None() && deoptTable_,
                 frameClass_.frameSize() == masm.framePushed());

#ifdef JS_CPU_X86
    
    
    
    if (assignBailoutId(snapshot)) {
        binder(masm, deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE);
        return true;
    }
#endif

    
    
    
    OutOfLineBailout *ool = new OutOfLineBailout(snapshot);
    if (!addOutOfLineCode(ool))
        return false;

    binder(masm, ool->entry());
    return true;
}

bool
CodeGeneratorX86Shared::bailoutIf(Assembler::Condition condition, LSnapshot *snapshot)
{
    return bailout(BailoutJump(condition), snapshot);
}

bool
CodeGeneratorX86Shared::bailoutFrom(Label *label, LSnapshot *snapshot)
{
    JS_ASSERT(label->used() && !label->bound());
    return bailout(BailoutLabel(label), snapshot);
}

bool
CodeGeneratorX86Shared::bailout(LSnapshot *snapshot)
{
    Label label;
    masm.jump(&label);
    return bailoutFrom(&label, snapshot);
}

bool
CodeGeneratorX86Shared::visitOutOfLineBailout(OutOfLineBailout *ool)
{
    if (!deoptLabel_)
        deoptLabel_ = new HeapLabel();

    masm.push(Imm32(ool->snapshot()->snapshotOffset()));
    masm.jmp(deoptLabel_);
    return true;
}


bool
CodeGeneratorX86Shared::visitMinMaxD(LMinMaxD *ins)
{
    FloatRegister first = ToFloatRegister(ins->first());
    FloatRegister second = ToFloatRegister(ins->second());
    FloatRegister output = ToFloatRegister(ins->output());

    JS_ASSERT(first == output);

    Assembler::Condition cond = ins->mir()->isMax()
                               ? Assembler::Above
                               : Assembler::Below;
    Label nan, equal, returnSecond, done;

    masm.ucomisd(second, first);
    masm.j(Assembler::Parity, &nan); 
    masm.j(Assembler::Equal, &equal); 
    masm.j(cond, &returnSecond);
    masm.jmp(&done);

    
    masm.bind(&equal);
    masm.xorpd(ScratchFloatReg, ScratchFloatReg);
    masm.ucomisd(first, ScratchFloatReg);
    masm.j(Assembler::NotEqual, &done); 
    
    if (ins->mir()->isMax())
        masm.addsd(second, first); 
    else
        masm.orpd(second, first); 
    masm.jmp(&done);

    masm.bind(&nan);
    masm.loadStaticDouble(&js_NaN, output);
    masm.jmp(&done);

    masm.bind(&returnSecond);
    masm.movsd(second, output);

    masm.bind(&done);
    return true;
}

bool
CodeGeneratorX86Shared::visitAbsD(LAbsD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(input == ToFloatRegister(ins->output()));
    masm.xorpd(ScratchFloatReg, ScratchFloatReg);
    masm.subsd(input, ScratchFloatReg); 
    masm.andpd(ScratchFloatReg, input); 
    return true;
}

bool
CodeGeneratorX86Shared::visitSqrtD(LSqrtD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(input == ToFloatRegister(ins->output()));
    masm.sqrtsd(input, input);
    return true;
}

bool
CodeGeneratorX86Shared::visitPowHalfD(LPowHalfD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    Register scratch = ToRegister(ins->temp());
    JS_ASSERT(input == ToFloatRegister(ins->output()));

    const uint32_t NegInfinityFloatBits = 0xFF800000;
    Label done, sqrt;

    
    masm.move32(Imm32(NegInfinityFloatBits), scratch);
    masm.loadFloatAsDouble(scratch, ScratchFloatReg);
    masm.branchDouble(Assembler::DoubleNotEqualOrUnordered, input, ScratchFloatReg, &sqrt);

    
    masm.xorpd(input, input);
    masm.subsd(ScratchFloatReg, input);
    masm.jump(&done);

    
    masm.bind(&sqrt);
    masm.xorpd(ScratchFloatReg, ScratchFloatReg);
    masm.addsd(ScratchFloatReg, input);
    masm.sqrtsd(input, input);

    masm.bind(&done);
    return true;
}

class OutOfLineUndoALUOperation : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LInstruction *ins_;

  public:
    OutOfLineUndoALUOperation(LInstruction *ins)
        : ins_(ins)
    { }

    virtual bool accept(CodeGeneratorX86Shared *codegen) {
        return codegen->visitOutOfLineUndoALUOperation(this);
    }
    LInstruction *ins() const {
        return ins_;
    }
};

bool
CodeGeneratorX86Shared::visitAddI(LAddI *ins)
{
    if (ins->rhs()->isConstant())
        masm.addl(Imm32(ToInt32(ins->rhs())), ToOperand(ins->lhs()));
    else
        masm.addl(ToOperand(ins->rhs()), ToRegister(ins->lhs()));

    if (ins->snapshot()) {
        if (ins->recoversInput()) {
            OutOfLineUndoALUOperation *ool = new OutOfLineUndoALUOperation(ins);
            if (!addOutOfLineCode(ool))
                return false;
            masm.j(Assembler::Overflow, ool->entry());
        } else {
            if (!bailoutIf(Assembler::Overflow, ins->snapshot()))
                return false;
        }
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitSubI(LSubI *ins)
{
    if (ins->rhs()->isConstant())
        masm.subl(Imm32(ToInt32(ins->rhs())), ToOperand(ins->lhs()));
    else
        masm.subl(ToOperand(ins->rhs()), ToRegister(ins->lhs()));

    if (ins->snapshot()) {
        if (ins->recoversInput()) {
            OutOfLineUndoALUOperation *ool = new OutOfLineUndoALUOperation(ins);
            if (!addOutOfLineCode(ool))
                return false;
            masm.j(Assembler::Overflow, ool->entry());
        } else {
            if (!bailoutIf(Assembler::Overflow, ins->snapshot()))
                return false;
        }
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitOutOfLineUndoALUOperation(OutOfLineUndoALUOperation *ool)
{
    LInstruction *ins = ool->ins();
    Register reg = ToRegister(ins->getDef(0));

    mozilla::DebugOnly<LAllocation *> lhs = ins->getOperand(0);
    LAllocation *rhs = ins->getOperand(1);

    JS_ASSERT(reg == ToRegister(lhs));
    JS_ASSERT_IF(rhs->isGeneralReg(), reg != ToRegister(rhs));

    
    
    
    
    

    if (rhs->isConstant()) {
        Imm32 constant(ToInt32(rhs));
        if (ins->isAddI())
            masm.subl(constant, reg);
        else
            masm.addl(constant, reg);
    } else {
        if (ins->isAddI())
            masm.subl(ToOperand(rhs), reg);
        else
            masm.addl(ToOperand(rhs), reg);
    }

    return bailout(ool->ins()->snapshot());
}

class MulNegativeZeroCheck : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LMulI *ins_;

  public:
    MulNegativeZeroCheck(LMulI *ins)
      : ins_(ins)
    { }

    virtual bool accept(CodeGeneratorX86Shared *codegen) {
        return codegen->visitMulNegativeZeroCheck(this);
    }
    LMulI *ins() const {
        return ins_;
    }
};

bool
CodeGeneratorX86Shared::visitMulI(LMulI *ins)
{
    const LAllocation *lhs = ins->lhs();
    const LAllocation *rhs = ins->rhs();
    MMul *mul = ins->mir();
    JS_ASSERT_IF(mul->mode() == MMul::Integer, !mul->canBeNegativeZero() && !mul->canOverflow());

    if (rhs->isConstant()) {
        
        int32_t constant = ToInt32(rhs);
        if (mul->canBeNegativeZero() && constant <= 0) {
            Assembler::Condition bailoutCond = (constant == 0) ? Assembler::Signed : Assembler::Equal;
            masm.testl(ToRegister(lhs), ToRegister(lhs));
            if (!bailoutIf(bailoutCond, ins->snapshot()))
                    return false;
        }

        switch (constant) {
          case -1:
            masm.negl(ToOperand(lhs));
            break;
          case 0:
            masm.xorl(ToOperand(lhs), ToRegister(lhs));
            return true; 
          case 1:
            
            return true; 
          case 2:
            masm.addl(ToOperand(lhs), ToRegister(lhs));
            break;
          default:
            if (!mul->canOverflow() && constant > 0) {
                
                int32_t shift;
                JS_FLOOR_LOG2(shift, constant);
                if ((1 << shift) == constant) {
                    masm.shll(Imm32(shift), ToRegister(lhs));
                    return true;
                }
            }
            masm.imull(Imm32(ToInt32(rhs)), ToRegister(lhs));
        }

        
        if (mul->canOverflow() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
            return false;
    } else {
        masm.imull(ToOperand(rhs), ToRegister(lhs));

        
        if (mul->canOverflow() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
            return false;

        if (mul->canBeNegativeZero()) {
            
            MulNegativeZeroCheck *ool = new MulNegativeZeroCheck(ins);
            if (!addOutOfLineCode(ool))
                return false;

            masm.testl(ToRegister(lhs), ToRegister(lhs));
            masm.j(Assembler::Zero, ool->entry());
            masm.bind(ool->rejoin());
        }
    }

    return true;
}

bool
CodeGeneratorX86Shared::visitMulNegativeZeroCheck(MulNegativeZeroCheck *ool)
{
    LMulI *ins = ool->ins();
    Register result = ToRegister(ins->output());
    Operand lhsCopy = ToOperand(ins->lhsCopy());
    Operand rhs = ToOperand(ins->rhs());
    JS_ASSERT_IF(lhsCopy.kind() == Operand::REG, lhsCopy.reg() != result.code());

    
    masm.movl(lhsCopy, result);
    masm.orl(rhs, result);
    if (!bailoutIf(Assembler::Signed, ins->snapshot()))
        return false;

    masm.xorl(result, result);
    masm.jmp(ool->rejoin());
    return true;
}

bool
CodeGeneratorX86Shared::visitDivI(LDivI *ins)
{
    Register remainder = ToRegister(ins->remainder());
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());

    MDiv *mir = ins->mir();

    JS_ASSERT(remainder == edx);
    JS_ASSERT(lhs == eax);
    JS_ASSERT(output == eax);

    Label done;

    
    if (mir->canBeDivideByZero()) {
        masm.testl(rhs, rhs);
        if (mir->isTruncated()) {
            
            Label notzero;
            masm.j(Assembler::NonZero, &notzero);
            masm.xorl(output, output);
            masm.jmp(&done);
            masm.bind(&notzero);
        } else {
            JS_ASSERT(mir->fallible());
            if (!bailoutIf(Assembler::Zero, ins->snapshot()))
                return false;
        }
    }

    
    if (mir->canBeNegativeOverflow()) {
        Label notmin;
        masm.cmpl(lhs, Imm32(INT32_MIN));
        masm.j(Assembler::NotEqual, &notmin);
        masm.cmpl(rhs, Imm32(-1));
        if (mir->isTruncated()) {
            
            
            masm.j(Assembler::Equal, &done);
        } else {
            JS_ASSERT(mir->fallible());
            if (!bailoutIf(Assembler::Equal, ins->snapshot()))
                return false;
        }
        masm.bind(&notmin);
    }

    
    if (!mir->isTruncated() && mir->canBeNegativeZero()) {
        Label nonzero;
        masm.testl(lhs, lhs);
        masm.j(Assembler::NonZero, &nonzero);
        masm.cmpl(rhs, Imm32(0));
        if (!bailoutIf(Assembler::LessThan, ins->snapshot()))
            return false;
        masm.bind(&nonzero);
    }

    
    masm.cdq();
    masm.idiv(rhs);

    if (!mir->isTruncated()) {
        
        masm.testl(remainder, remainder);
        if (!bailoutIf(Assembler::NonZero, ins->snapshot()))
            return false;
    }

    masm.bind(&done);

    return true;
}

bool
CodeGeneratorX86Shared::visitModPowTwoI(LModPowTwoI *ins)
{
    Register lhs = ToRegister(ins->getOperand(0));
    int32_t shift = ins->shift();

    Label negative, done;
    
    
    masm.branchTest32(Assembler::Signed, lhs, lhs, &negative);
    {
        masm.andl(Imm32((1 << shift) - 1), lhs);
        masm.jump(&done);
    }
    
    {
        masm.bind(&negative);
        
        
        masm.negl(lhs);
        masm.andl(Imm32((1 << shift) - 1), lhs);
        masm.negl(lhs);
        if (!ins->mir()->isTruncated() && !bailoutIf(Assembler::Zero, ins->snapshot()))
            return false;
    }
    masm.bind(&done);
    return true;

}

bool
CodeGeneratorX86Shared::visitModI(LModI *ins)
{
    Register remainder = ToRegister(ins->remainder());
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register temp = ToRegister(ins->getTemp(0));

    
    JS_ASSERT(remainder == edx);
    JS_ASSERT(temp == eax);

    if (lhs != temp) {
        masm.mov(lhs, temp);
        lhs = temp;
    }

    Label done;

    
    masm.testl(rhs, rhs);
    if (ins->mir()->isTruncated()) {
        Label notzero;
        masm.j(Assembler::NonZero, &notzero);
        masm.xorl(edx, edx);
        masm.jmp(&done);
        masm.bind(&notzero);
    } else {
        if (!bailoutIf(Assembler::Zero, ins->snapshot()))
            return false;
    }

    Label negative;

    
    masm.branchTest32(Assembler::Signed, lhs, lhs, &negative);
    
    {
        
        masm.xorl(edx, edx);
        masm.idiv(rhs);
        masm.jump(&done);
    }

    
    {
        masm.bind(&negative);

        
        Label notmin;
        masm.cmpl(lhs, Imm32(INT32_MIN));
        masm.j(Assembler::NotEqual, &notmin);
        masm.cmpl(rhs, Imm32(-1));
        if (ins->mir()->isTruncated()) {
            masm.j(Assembler::NotEqual, &notmin);
            masm.xorl(edx, edx);
            masm.jmp(&done);
        } else {
            if (!bailoutIf(Assembler::Equal, ins->snapshot()))
                return false;
        }
        masm.bind(&notmin);

        masm.cdq();
        masm.idiv(rhs);

        if (!ins->mir()->isTruncated()) {
            
            masm.testl(remainder, remainder);
            if (!bailoutIf(Assembler::Zero, ins->snapshot()))
                return false;
        }
    }

    masm.bind(&done);
    return true;
}

bool
CodeGeneratorX86Shared::visitBitNotI(LBitNotI *ins)
{
    const LAllocation *input = ins->getOperand(0);
    JS_ASSERT(!input->isConstant());

    masm.notl(ToOperand(input));
    return true;
}

bool
CodeGeneratorX86Shared::visitBitOpI(LBitOpI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);

    switch (ins->bitop()) {
        case JSOP_BITOR:
            if (rhs->isConstant())
                masm.orl(Imm32(ToInt32(rhs)), ToOperand(lhs));
            else
                masm.orl(ToOperand(rhs), ToRegister(lhs));
            break;
        case JSOP_BITXOR:
            if (rhs->isConstant())
                masm.xorl(Imm32(ToInt32(rhs)), ToOperand(lhs));
            else
                masm.xorl(ToOperand(rhs), ToRegister(lhs));
            break;
        case JSOP_BITAND:
            if (rhs->isConstant())
                masm.andl(Imm32(ToInt32(rhs)), ToOperand(lhs));
            else
                masm.andl(ToOperand(rhs), ToRegister(lhs));
            break;
        default:
            JS_NOT_REACHED("unexpected binary opcode");
    }

    return true;
}

bool
CodeGeneratorX86Shared::visitShiftI(LShiftI *ins)
{
    Register lhs = ToRegister(ins->lhs());
    const LAllocation *rhs = ins->rhs();

    if (rhs->isConstant()) {
        int32_t shift = ToInt32(rhs) & 0x1F;
        switch (ins->bitop()) {
          case JSOP_LSH:
            if (shift)
                masm.shll(Imm32(shift), lhs);
            break;
          case JSOP_RSH:
            if (shift)
                masm.sarl(Imm32(shift), lhs);
            break;
          case JSOP_URSH:
            if (shift) {
                masm.shrl(Imm32(shift), lhs);
            } else if (ins->mir()->toUrsh()->canOverflow()) {
                
                masm.testl(lhs, lhs);
                if (!bailoutIf(Assembler::Signed, ins->snapshot()))
                    return false;
            }
            break;
          default:
            JS_NOT_REACHED("Unexpected shift op");
        }
    } else {
        JS_ASSERT(ToRegister(rhs) == ecx);
        switch (ins->bitop()) {
          case JSOP_LSH:
            masm.shll_cl(lhs);
            break;
          case JSOP_RSH:
            masm.sarl_cl(lhs);
            break;
          case JSOP_URSH:
            masm.shrl_cl(lhs);
            if (ins->mir()->toUrsh()->canOverflow()) {
                
                masm.testl(lhs, lhs);
                if (!bailoutIf(Assembler::Signed, ins->snapshot()))
                    return false;
            }
            break;
          default:
            JS_NOT_REACHED("Unexpected shift op");
        }
    }

    return true;
}

bool
CodeGeneratorX86Shared::visitUrshD(LUrshD *ins)
{
    Register lhs = ToRegister(ins->lhs());
    JS_ASSERT(ToRegister(ins->temp()) == lhs);

    const LAllocation *rhs = ins->rhs();
    FloatRegister out = ToFloatRegister(ins->output());

    if (rhs->isConstant()) {
        int32_t shift = ToInt32(rhs) & 0x1F;
        if (shift)
            masm.shrl(Imm32(shift), lhs);
    } else {
        JS_ASSERT(ToRegister(rhs) == ecx);
        masm.shrl_cl(lhs);
    }

    masm.convertUInt32ToDouble(lhs, out);
    return true;
}

typedef MoveResolver::MoveOperand MoveOperand;

MoveOperand
CodeGeneratorX86Shared::toMoveOperand(const LAllocation *a) const
{
    if (a->isGeneralReg())
        return MoveOperand(ToRegister(a));
    if (a->isFloatReg())
        return MoveOperand(ToFloatRegister(a));
    return MoveOperand(StackPointer, ToStackOffset(a));
}

bool
CodeGeneratorX86Shared::visitMoveGroup(LMoveGroup *group)
{
    if (!group->numMoves())
        return true;

    MoveResolver &resolver = masm.moveResolver();

    for (size_t i = 0; i < group->numMoves(); i++) {
        const LMove &move = group->getMove(i);

        const LAllocation *from = move.from();
        const LAllocation *to = move.to();

        
        JS_ASSERT(*from != *to);
        JS_ASSERT(!from->isConstant());
        JS_ASSERT(from->isDouble() == to->isDouble());

        MoveResolver::Move::Kind kind = from->isDouble()
                                        ? MoveResolver::Move::DOUBLE
                                        : MoveResolver::Move::GENERAL;

        if (!resolver.addMove(toMoveOperand(from), toMoveOperand(to), kind))
            return false;
    }

    if (!resolver.resolve())
        return false;

    MoveEmitter emitter(masm);
    emitter.emit(resolver);
    emitter.finish();

    return true;
}

class OutOfLineTableSwitch : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    MTableSwitch *mir_;
    CodeLabel jumpLabel_;

    bool accept(CodeGeneratorX86Shared *codegen) {
        return codegen->visitOutOfLineTableSwitch(this);
    }

  public:
    OutOfLineTableSwitch(MTableSwitch *mir)
      : mir_(mir)
    {}

    MTableSwitch *mir() const {
        return mir_;
    }

    CodeLabel *jumpLabel() {
        return &jumpLabel_;
    }
};

bool
CodeGeneratorX86Shared::visitOutOfLineTableSwitch(OutOfLineTableSwitch *ool)
{
    MTableSwitch *mir = ool->mir();

    masm.align(sizeof(void*));
    masm.bind(ool->jumpLabel()->src());
    if (!masm.addCodeLabel(*ool->jumpLabel()))
        return false;

    for (size_t i = 0; i < mir->numCases(); i++) {
        LBlock *caseblock = mir->getCase(i)->lir();
        Label *caseheader = caseblock->label();
        uint32_t caseoffset = caseheader->offset();

        
        
        CodeLabel cl;
        masm.writeCodePointer(cl.dest());
        cl.src()->bind(caseoffset);
        if (!masm.addCodeLabel(cl))
            return false;
    }

    return true;
}

bool
CodeGeneratorX86Shared::emitTableSwitchDispatch(MTableSwitch *mir, const Register &index,
                                                const Register &base)
{
    Label *defaultcase = mir->getDefault()->lir()->label();

    
    if (mir->low() != 0)
        masm.subl(Imm32(mir->low()), index);

    
    int32_t cases = mir->numCases();
    masm.cmpl(index, Imm32(cases));
    masm.j(AssemblerX86Shared::AboveOrEqual, defaultcase);

    
    
    
    OutOfLineTableSwitch *ool = new OutOfLineTableSwitch(mir);
    if (!addOutOfLineCode(ool))
        return false;

    
    masm.mov(ool->jumpLabel()->dest(), base);
    Operand pointer = Operand(base, index, ScalePointer);

    
    masm.jmp(pointer);

    return true;
}

bool
CodeGeneratorX86Shared::visitMathD(LMathD *math)
{
    FloatRegister lhs = ToFloatRegister(math->lhs());
    Operand rhs = ToOperand(math->rhs());

    JS_ASSERT(ToFloatRegister(math->output()) == lhs);

    switch (math->jsop()) {
      case JSOP_ADD:
        masm.addsd(rhs, lhs);
        break;
      case JSOP_SUB:
        masm.subsd(rhs, lhs);
        break;
      case JSOP_MUL:
        masm.mulsd(rhs, lhs);
        break;
      case JSOP_DIV:
        masm.divsd(rhs, lhs);
        break;
      default:
        JS_NOT_REACHED("unexpected opcode");
        return false;
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitFloor(LFloor *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister scratch = ScratchFloatReg;
    Register output = ToRegister(lir->output());

    if (AssemblerX86Shared::HasSSE41()) {
        
        Assembler::Condition bailCond = masm.testNegativeZero(input, output);
        if (!bailoutIf(bailCond, lir->snapshot()))
            return false;

        
        masm.roundsd(input, scratch, JSC::X86Assembler::RoundDown);

        masm.cvttsd2si(scratch, output);
        masm.cmp32(output, Imm32(INT_MIN));
        if (!bailoutIf(Assembler::Equal, lir->snapshot()))
            return false;
    } else {
        Label negative, end;

        
        masm.xorpd(scratch, scratch);
        masm.branchDouble(Assembler::DoubleLessThan, input, scratch, &negative);

        
        Assembler::Condition bailCond = masm.testNegativeZero(input, output);
        if (!bailoutIf(bailCond, lir->snapshot()))
            return false;

        
        masm.cvttsd2si(input, output);
        masm.cmp32(output, Imm32(INT_MIN));
        if (!bailoutIf(Assembler::Equal, lir->snapshot()))
            return false;

        masm.jump(&end);

        
        
        
        masm.bind(&negative);
        {
            
            
            masm.cvttsd2si(input, output);
            masm.cmp32(output, Imm32(INT_MIN));
            if (!bailoutIf(Assembler::Equal, lir->snapshot()))
                return false;

            
            masm.cvtsi2sd(output, scratch);
            masm.branchDouble(Assembler::DoubleEqualOrUnordered, input, scratch, &end);

            
            
            masm.subl(Imm32(1), output);
            
        }

        masm.bind(&end);
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitRound(LRound *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister temp = ToFloatRegister(lir->temp());
    FloatRegister scratch = ScratchFloatReg;
    Register output = ToRegister(lir->output());

    Label negative, end;

    
    static const double PointFive = 0.5;
    masm.loadStaticDouble(&PointFive, temp);

    
    masm.xorpd(scratch, scratch);
    masm.branchDouble(Assembler::DoubleLessThan, input, scratch, &negative);

    
    Assembler::Condition bailCond = masm.testNegativeZero(input, output);
    if (!bailoutIf(bailCond, lir->snapshot()))
        return false;

    
    
    
    masm.addsd(input, temp);

    masm.cvttsd2si(temp, output);
    masm.cmp32(output, Imm32(INT_MIN));
    if (!bailoutIf(Assembler::Equal, lir->snapshot()))
        return false;

    masm.jump(&end);


    
    masm.bind(&negative);

    if (AssemblerX86Shared::HasSSE41()) {
        
        
        masm.addsd(input, temp);
        masm.roundsd(temp, scratch, JSC::X86Assembler::RoundDown);

        
        masm.cvttsd2si(scratch, output);
        masm.cmp32(output, Imm32(INT_MIN));
        if (!bailoutIf(Assembler::Equal, lir->snapshot()))
            return false;

        
        
        masm.testl(output, output);
        if (!bailoutIf(Assembler::Zero, lir->snapshot()))
            return false;

    } else {
        masm.addsd(input, temp);

        
        Label testZero;
        {
            
            
            masm.cvttsd2si(temp, output);
            masm.cmp32(output, Imm32(INT_MIN));
            if (!bailoutIf(Assembler::Equal, lir->snapshot()))
                return false;

            
            masm.cvtsi2sd(output, scratch);
            masm.branchDouble(Assembler::DoubleEqualOrUnordered, temp, scratch, &testZero);

            
            
            masm.subl(Imm32(1), output);
            

            
        }

        masm.bind(&testZero);
        if (!bailoutIf(Assembler::Zero, lir->snapshot()))
            return false;
    }

    masm.bind(&end);
    return true;
}

bool
CodeGeneratorX86Shared::visitGuardShape(LGuardShape *guard)
{
    Register obj = ToRegister(guard->input());
    masm.cmpPtr(Operand(obj, JSObject::offsetOfShape()), ImmGCPtr(guard->mir()->shape()));
    if (!bailoutIf(Assembler::NotEqual, guard->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorX86Shared::visitGuardClass(LGuardClass *guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());

    masm.loadPtr(Address(obj, JSObject::offsetOfType()), tmp);
    masm.cmpPtr(Operand(tmp, offsetof(types::TypeObject, clasp)), ImmWord(guard->mir()->getClass()));
    if (!bailoutIf(Assembler::NotEqual, guard->snapshot()))
        return false;
    return true;
}

class OutOfLineTruncate : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LTruncateDToInt32 *ins_;

  public:
    OutOfLineTruncate(LTruncateDToInt32 *ins)
      : ins_(ins)
    { }

    bool accept(CodeGeneratorX86Shared *codegen) {
        return codegen->visitOutOfLineTruncate(this);
    }
    LTruncateDToInt32 *ins() const {
        return ins_;
    }
};

bool
CodeGeneratorX86Shared::visitTruncateDToInt32(LTruncateDToInt32 *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    Register output = ToRegister(ins->output());

    OutOfLineTruncate *ool = new OutOfLineTruncate(ins);
    if (!addOutOfLineCode(ool))
        return false;

    masm.branchTruncateDouble(input, output, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGeneratorX86Shared::visitOutOfLineTruncate(OutOfLineTruncate *ool)
{
    LTruncateDToInt32 *ins = ool->ins();
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister temp = ToFloatRegister(ins->tempFloat());
    Register output = ToRegister(ins->output());

    
    
    
    
    Label fail;
    masm.xorpd(ScratchFloatReg, ScratchFloatReg);
    masm.ucomisd(input, ScratchFloatReg);
    masm.j(Assembler::Parity, &fail);

    {
        Label positive;
        masm.j(Assembler::Above, &positive);

        static const double shiftNeg = 4294967296.0;
        masm.loadStaticDouble(&shiftNeg, temp);
        Label skip;
        masm.jmp(&skip);

        masm.bind(&positive);
        static const double shiftPos = -4294967296.0;
        masm.loadStaticDouble(&shiftPos, temp);
        masm.bind(&skip);
    }

    masm.addsd(input, temp);
    masm.cvttsd2si(temp, output);
    masm.cvtsi2sd(output, ScratchFloatReg);

    masm.ucomisd(temp, ScratchFloatReg);
    masm.j(Assembler::Parity, &fail);
    masm.j(Assembler::Equal, ool->rejoin());

    masm.bind(&fail);
    {
        saveVolatile(output);

        masm.setupUnalignedABICall(1, output);
        masm.passABIArg(input);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::ToInt32));
        masm.storeCallResult(output);

        restoreVolatile(output);
    }

    masm.jump(ool->rejoin());
    return true;
}

Operand
CodeGeneratorX86Shared::createArrayElementOperand(Register elements, const LAllocation *index)
{
    if (index->isConstant())
        return Operand(elements, ToInt32(index) * sizeof(js::Value));

    return Operand(elements, ToRegister(index), TimesEight);
}
bool
CodeGeneratorX86Shared::generateInvalidateEpilogue()
{
    
    
    
    for (size_t i = 0; i < sizeof(void *); i+= Assembler::nopSize())
        masm.nop();

    masm.bind(&invalidate_);

    
    invalidateEpilogueData_ = masm.pushWithPatch(ImmWord(uintptr_t(-1)));
    IonCode *thunk = GetIonContext()->compartment->ionCompartment()->getInvalidationThunk();

    masm.call(thunk);

    
    
    masm.breakpoint();
    return true;
}

} 
} 
