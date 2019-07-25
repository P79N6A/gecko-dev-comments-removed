







































#include "jscntxt.h"
#include "jscompartment.h"
#include "CodeGenerator-x86-shared.h"
#include "CodeGenerator-shared-inl.h"
#include "ion/IonFrames.h"
#include "ion/MoveEmitter.h"
#include "ion/IonCompartment.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX86Shared::CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorShared(gen, graph),
    deoptLabel_(NULL)
{
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

bool
CodeGeneratorX86Shared::visitGoto(LGoto *jump)
{
    LBlock *target = jump->target()->lir();

    
    if (isNextBlock(target))
        return true;

    masm.jmp(target->label());
    return true;
}

bool
CodeGeneratorX86Shared::visitTestIAndBranch(LTestIAndBranch *test)
{
    const LAllocation *opd = test->getOperand(0);
    LBlock *ifTrue = test->ifTrue()->lir();
    LBlock *ifFalse = test->ifFalse()->lir();

    
    masm.testl(ToRegister(opd), ToRegister(opd));

    if (isNextBlock(ifFalse)) {
        masm.j(AssemblerX86Shared::NonZero, ifTrue->label());
    } else if (isNextBlock(ifTrue)) {
        masm.j(AssemblerX86Shared::Zero, ifFalse->label());
    } else {
        masm.j(AssemblerX86Shared::Zero, ifFalse->label());
        masm.jmp(ifTrue->label());
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareI(LCompareI *comp)
{
    JS_NOT_REACHED("Codegen for LCompareI NYI");
    return false;
}

bool
CodeGeneratorX86Shared::visitCompareIAndBranch(LCompareIAndBranch *comp)
{
    JS_NOT_REACHED("Codegen for LCompareIAndBranch NYI");
    return false;
}

bool
CodeGeneratorX86Shared::generateOutOfLineCode()
{
    if (!CodeGeneratorShared::generateOutOfLineCode())
        return false;

    if (deoptLabel_) {
        
        masm.bind(deoptLabel_);
        
        
        masm.push(Imm32(frameSize()));

        IonCompartment *ion = gen->cx->compartment->ionCompartment();
        IonCode *handler = ion->getGenericBailoutHandler(gen->cx);
        if (!handler)
            return false;

        masm.jmp(handler->raw(), Relocation::CODE);
    }

    return true;
}

bool
CodeGeneratorX86Shared::bailoutIf(Assembler::Condition condition, LSnapshot *snapshot)
{
    if (!encode(snapshot))
        return false;

    
    
    
    JS_ASSERT_IF(frameClass_ != FrameSizeClass::None(),
                 frameClass_.frameSize() == masm.framePushed());

    
    
    
#ifdef JS_CPU_X86
    if (assignBailoutId(snapshot)) {
        uint8 *code = deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE;
        masm.j(condition, code, Relocation::EXTERNAL);
        return true;
    }
#endif

    
    
    
    OutOfLineBailout *ool = new OutOfLineBailout(snapshot, masm.framePushed());
    if (!addOutOfLineCode(ool))
        return false;

    masm.j(condition, ool->entry());

    return true;
}

bool
CodeGeneratorX86Shared::visitOutOfLineBailout(OutOfLineBailout *ool)
{
    masm.bind(ool->entry());

    if (!deoptLabel_)
        deoptLabel_ = new HeapLabel();

    masm.push(Imm32(ool->snapshot()->snapshotOffset()));
    masm.jmp(deoptLabel_);
    return true;
}

bool
CodeGeneratorX86Shared::visitAddI(LAddI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);

    if (rhs->isConstant())
        masm.addl(Imm32(ToInt32(rhs)), ToOperand(lhs));
    else
        masm.addl(ToOperand(rhs), ToRegister(lhs));

    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;

    return true;
}

bool
CodeGeneratorX86Shared::visitMulI(LMulI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);

    if (rhs->isConstant()) {
        
        int32 constant = ToInt32(rhs);
        if (ins->snapshot() && constant <= 0) {
            Assembler::Condition bailoutCond = (constant == 0) ? Assembler::LessThan : Assembler::Equal;
            masm.cmpl(Imm32(0), ToRegister(lhs));
            if (bailoutIf(bailoutCond, ins->snapshot()))
                    return false;
        }

        masm.imull(Imm32(ToInt32(rhs)), ToRegister(lhs));

        
        if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
            return false;
    } else {
        masm.imull(ToOperand(rhs), ToRegister(lhs));

        
        if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
            return false;

        
        if (ins->snapshot()) {
            masm.cmpl(Imm32(0), ToRegister(lhs));
            if (!bailoutIf(Assembler::Zero, ins->snapshot()))
                return false;
        }
    }

    return true;
}

bool
CodeGeneratorX86Shared::visitBitNot(LBitNot *ins)
{
    const LAllocation *input = ins->getOperand(0);
    JS_ASSERT(!input->isConstant());

    masm.notl(ToOperand(input));
    return true;
}

bool
CodeGeneratorX86Shared::visitBitOp(LBitOp *ins)
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
CodeGeneratorX86Shared::visitInteger(LInteger *ins)
{
    const LDefinition *def = ins->getDef(0);
    masm.movl(Imm32(ins->getValue()), ToRegister(def));
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

bool
CodeGeneratorX86Shared::visitTableSwitch(LTableSwitch *ins)
{
    MTableSwitch *mir = ins->mir();
    const LAllocation *input = ins->getOperand(0);

    
    LDefinition *index = ins->getTemp(0);
    masm.mov(ToOperand(input), ToRegister(index));

    
    if (mir->low() != 0)
        masm.subl(Imm32(mir->low()), ToOperand(index));

    
    LBlock *defaultcase = mir->getDefault()->lir();
    int32 cases = mir->numCases();
    masm.cmpl(Imm32(cases), ToRegister(index));
    masm.j(AssemblerX86Shared::AboveOrEqual, defaultcase->label());
 
    
    
    CodeLabel *label = new CodeLabel();
    if (!masm.addCodeLabel(label))
        return false;
  
    
    LDefinition *base = ins->getTemp(1);
    masm.mov(label->dest(), ToRegister(base));
    Operand pointer = Operand(ToRegister(base), ToRegister(index), TimesEight);
    masm.lea(pointer, ToRegister(base));

    
    masm.jmp(ToOperand(base));

    
    
    
    masm.align(1 << TimesFour);
    masm.bind(label->src());

    for (uint j=0; j<ins->mir()->numCases(); j++) { 
        LBlock *caseblock = ins->mir()->getCase(j)->lir();

        masm.jmp(caseblock->label());
        masm.align(1 << TimesFour);
    }

    return true;
}

bool
CodeGeneratorX86Shared::visitMathD(LMathD *math)
{
    const LAllocation *input = math->getOperand(1);
    const LDefinition *output = math->getDef(0);

    switch (math->jsop()) {
      case JSOP_ADD:
        masm.addsd(ToFloatRegister(input), ToFloatRegister(output));
        break;
      case JSOP_MUL:
        masm.mulsd(ToFloatRegister(input), ToFloatRegister(output));
      default:
        JS_NOT_REACHED("unexpected opcode");
        return false;
    }
    return true;
}




bool
CodeGeneratorX86Shared::emitDoubleToInt32(const FloatRegister &src, const Register &dest, LSnapshot *snapshot)
{
    
    
    
    masm.cvttsd2s(src, dest);
    masm.cvtsi2sd(dest, ScratchFloatReg);
    masm.ucomisd(src, ScratchFloatReg);
    if (!bailoutIf(Assembler::Parity, snapshot))
        return false;
    if (!bailoutIf(Assembler::NotEqual, snapshot))
        return false;

    
    Label notZero;
    masm.testl(dest, dest);
    masm.j(Assembler::NonZero, &notZero);

    if (Assembler::HasSSE41()) {
        masm.ptest(src, src);
        if (!bailoutIf(Assembler::NonZero, snapshot))
            return false;
    } else {
        
        
        masm.movmskpd(src, dest);
        masm.andl(Imm32(1), dest);
        if (!bailoutIf(Assembler::NonZero, snapshot))
            return false;
    }
    
    masm.bind(&notZero);

    return true;
}

