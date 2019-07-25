







































#include "jscntxt.h"
#include "jscompartment.h"
#include "CodeGenerator-x86-shared.h"
#include "CodeGenerator-shared-inl.h"
#include "ion/IonFrames.h"
#include "ion/MoveEmitter.h"
#include "ion/IonCompartment.h"

using namespace js;
using namespace js::ion;

class DeferredJumpTable : public DeferredData
{
    LTableSwitch *lswitch;

  public:
    DeferredJumpTable(LTableSwitch *lswitch)
      : lswitch(lswitch)
    { }
    
    void copy(IonCode *code, uint8 *buffer) const {
        void **jumpData = (void **)buffer;

        
        for (uint j = 0; j < lswitch->mir()->numCases(); j++) { 
            LBlock *caseblock = lswitch->mir()->getCase(j)->lir();
            Label *caseheader = caseblock->label();

            uint32 offset = caseheader->offset();
            *jumpData = (void *)(code->raw() + offset);
            jumpData++;
        }
    }
};

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
        masm.j(Assembler::NonZero, ifTrue->label());
    } else if (isNextBlock(ifTrue)) {
        masm.j(Assembler::Zero, ifFalse->label());
    } else {
        masm.j(Assembler::Zero, ifFalse->label());
        masm.jmp(ifTrue->label());
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareI(LCompareI *comp)
{
    const LAllocation *left = comp->getOperand(0);
    const LAllocation *right = comp->getOperand(1);
    const LDefinition *def = comp->getDef(0);

    
    
    if (GeneralRegisterSet(Registers::SingleByteRegs).has(ToRegister(def))) {
        masm.xorl(ToRegister(def), ToRegister(def));
        masm.cmpl(ToRegister(left), ToOperand(right));
        masm.setCC(comp->condition(), ToRegister(def));
        return true;
    }

    Label ifTrue;
    masm.movl(Imm32(1), ToRegister(def));
    masm.cmpl(ToRegister(left), ToOperand(right));
    masm.j(comp->condition(), &ifTrue);
    masm.movl(Imm32(0), ToRegister(def));
    masm.bind(&ifTrue);
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareIAndBranch(LCompareIAndBranch *comp)
{
    const LAllocation *left = comp->getOperand(0);
    const LAllocation *right = comp->getOperand(1);
    LBlock *ifTrue = comp->ifTrue()->lir();
    LBlock *ifFalse = comp->ifFalse()->lir();
    Assembler::Condition cond = comp->condition();

    
    masm.cmpl(ToRegister(left), ToOperand(right));

    
    if (isNextBlock(ifFalse)) {
        masm.j(cond, ifTrue->label());
    } else if (isNextBlock(ifTrue)) {
        masm.j(Assembler::inverseCondition(cond), ifFalse->label());
    } else {
        masm.j(cond, ifTrue->label());
        masm.jmp(ifFalse->label());
    }
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

        IonCompartment *ion = gen->cx->compartment->ionCompartment();
        IonCode *handler = ion->getGenericBailoutHandler(gen->cx);
        if (!handler)
            return false;

        masm.jmp(handler->raw(), Relocation::CODE);
    }

    return true;
}

class BailoutJump {
    Assembler::Condition cond_;

  public:
    BailoutJump(Assembler::Condition cond) : cond_(cond)
    { }
#ifdef JS_CPU_X86
    void operator()(MacroAssembler &masm, uint8 *code) const {
        masm.j(cond_, code, Relocation::EXTERNAL);
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
    void operator()(MacroAssembler &masm, uint8 *code) const {
        masm.retarget(label_, code, Relocation::EXTERNAL);
    }
#endif
    void operator()(MacroAssembler &masm, Label *label) const {
        masm.retarget(label_, label);
    }
};

template <typename T> bool
CodeGeneratorX86Shared::bailout(const T &binder, LSnapshot *snapshot)
{
    if (!encode(snapshot))
        return false;

    
    
    
    JS_ASSERT_IF(frameClass_ != FrameSizeClass::None(),
                 frameClass_.frameSize() == masm.framePushed());

#ifdef JS_CPU_X86
    
    
    
    if (assignBailoutId(snapshot)) {
        binder(masm, deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE);
        return true;
    }
#endif

    
    
    
    OutOfLineBailout *ool = new OutOfLineBailout(snapshot, masm.framePushed());
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
    return bailout(BailoutLabel(label), snapshot);
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
    MMul *mul = ins->mir();

    if (rhs->isConstant()) {
        
        int32 constant = ToInt32(rhs);
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
                
                int32 shift = JS_FloorLog2(constant);
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
            masm.testl(ToRegister(lhs), ToRegister(lhs));
            if (!bailoutIf(Assembler::Equal, ins->snapshot()))
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
CodeGeneratorX86Shared::visitShiftOp(LShiftOp *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);

    switch (ins->bitop()) {
        case JSOP_LSH:
            if (rhs->isConstant())
                masm.shll(Imm32(ToInt32(rhs) & 0x1F), ToRegister(lhs));
            else
                masm.shll_cl(ToRegister(lhs));
            break;
        case JSOP_RSH:
            if (rhs->isConstant())
                masm.sarl(Imm32(ToInt32(rhs) & 0x1F), ToRegister(lhs));
            else
                masm.sarl_cl(ToRegister(lhs));
            break;
        case JSOP_URSH: {
            MUrsh *ursh = ins->mir()->toUrsh(); 
            if (rhs->isConstant())
                masm.shrl(Imm32(ToInt32(rhs) & 0x1F), ToRegister(lhs));
            else
                masm.shrl_cl(ToRegister(lhs));
 
            
            
            
            
            
            
            
            if (ursh->canOverflow()) {
                masm.cmpl(ToOperand(lhs), Imm32(0));
                if (!bailoutIf(Assembler::LessThan, ins->snapshot()))
                    return false;
            }
            break;
        }
        default:
            JS_NOT_REACHED("unexpected shift opcode");
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
    Label *defaultcase = mir->getDefault()->lir()->label();
    const LAllocation *temp;

    if (ins->index()->isDouble()) {
        temp = ins->tempInt();

        
        
        emitDoubleToInt32(ToFloatRegister(ins->index()), ToRegister(temp), defaultcase);
    } else {
        temp = ins->index();
    }

    
    if (mir->low() != 0)
        masm.subl(Imm32(mir->low()), ToRegister(temp));

    
    int32 cases = mir->numCases();
    masm.cmpl(ToRegister(temp), Imm32(cases));
    masm.j(AssemblerX86Shared::AboveOrEqual, defaultcase);

    
    DeferredJumpTable *d = new DeferredJumpTable(ins);
    if (!masm.addDeferredData(d, (1 << ScalePointer) * cases))
        return false;
   
    
    const LAllocation *base = ins->tempPointer();
    masm.mov(d->label(), ToRegister(base));
    Operand pointer = Operand(ToRegister(base), ToRegister(temp), ScalePointer);

    
    masm.jmp(pointer);

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
        break;
      default:
        JS_NOT_REACHED("unexpected opcode");
        return false;
    }
    return true;
}




void
CodeGeneratorX86Shared::emitDoubleToInt32(const FloatRegister &src, const Register &dest, Label *fail)
{
    
    
    
    masm.cvttsd2s(src, dest);
    masm.cvtsi2sd(dest, ScratchFloatReg);
    masm.ucomisd(src, ScratchFloatReg);
    masm.j(Assembler::Parity, fail);
    masm.j(Assembler::NotEqual, fail);

    
    Label notZero;
    masm.testl(dest, dest);
    masm.j(Assembler::NonZero, &notZero);

    if (Assembler::HasSSE41()) {
        masm.ptest(src, src);
        masm.j(Assembler::NonZero, fail);
    } else {
        
        
        masm.movmskpd(src, dest);
        masm.andl(Imm32(1), dest);
        masm.j(Assembler::NonZero, fail);
    }
    
    masm.bind(&notZero);
}

