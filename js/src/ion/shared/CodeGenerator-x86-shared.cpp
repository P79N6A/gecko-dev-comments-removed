







































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

        
        for (size_t j = 0; j < lswitch->mir()->numCases(); j++) { 
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

void
CodeGeneratorX86Shared::emitBranch(Assembler::Condition cond, MBasicBlock *mirTrue,
                                   MBasicBlock *mirFalse, NaNCond ifNaN)
{
    LBlock *ifTrue = mirTrue->lir();
    LBlock *ifFalse = mirFalse->lir();

    if (ifNaN == NaN_IsFalse)
        masm.j(Assembler::Parity, ifFalse->label());
    else if (ifNaN == NaN_IsTrue)
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

static inline Assembler::Condition
JSOpToCondition(JSOp op)
{
    switch (op) {
      case JSOP_EQ:
      case JSOP_STRICTEQ:
        return Assembler::Equal;
      case JSOP_NE:
      case JSOP_STRICTNE:
        return Assembler::NotEqual;
      case JSOP_LT:
        return Assembler::LessThan;
      case JSOP_LE:
        return Assembler::LessThanOrEqual;
      case JSOP_GT:
        return Assembler::GreaterThan;
      case JSOP_GE:
        return Assembler::GreaterThanOrEqual;
      default:
        JS_NOT_REACHED("Unrecognized comparison operation");
        return Assembler::Equal;
    }
}

void
CodeGeneratorX86Shared::emitSet(Assembler::Condition cond, const Register &dest, NaNCond ifNaN)
{
    if (GeneralRegisterSet(Registers::SingleByteRegs).has(dest)) {
        
        
        masm.setCC(cond, dest);
        masm.movzxbl(dest, dest);

        if (ifNaN != NaN_Unexpected) {
            Label noNaN;
            masm.j(Assembler::NoParity, &noNaN);
            if (ifNaN == NaN_IsTrue)
                masm.movl(Imm32(1), dest);
            else
                masm.xorl(dest, dest);
            masm.bind(&noNaN);
        }
    } else {
        Label end;
        Label ifFalse;

        if (ifNaN == NaN_IsFalse)
            masm.j(Assembler::Parity, &ifFalse);
        masm.movl(Imm32(1), dest);
        masm.j(cond, &end);
        if (ifNaN == NaN_IsTrue)
            masm.j(Assembler::Parity, &end);
        masm.bind(&ifFalse);
        masm.xorl(dest, dest);

        masm.bind(&end);
    }
}

bool
CodeGeneratorX86Shared::visitCompareI(LCompareI *comp)
{
    if (comp->right()->isConstant())
        masm.cmpl(ToRegister(comp->left()), Imm32(ToInt32(comp->right())));
    else
        masm.cmpl(ToRegister(comp->left()), ToOperand(comp->right()));
    emitSet(JSOpToCondition(comp->jsop()), ToRegister(comp->output()));
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareIAndBranch(LCompareIAndBranch *comp)
{
    if (comp->right()->isConstant())
        masm.cmpl(ToRegister(comp->left()), Imm32(ToInt32(comp->right())));
    else
        masm.cmpl(ToRegister(comp->left()), ToOperand(comp->right()));
    Assembler::Condition cond = JSOpToCondition(comp->jsop());
    emitBranch(cond, comp->ifTrue(), comp->ifFalse());
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareD(LCompareD *comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::Condition cond = masm.compareDoubles(comp->jsop(), lhs, rhs);
    NaNCond ifNaN = (cond == Assembler::NotEqual) ? NaN_IsTrue : NaN_IsFalse;
    emitSet(cond, ToRegister(comp->output()), ifNaN);
    return true;
}

bool
CodeGeneratorX86Shared::visitCompareDAndBranch(LCompareDAndBranch *comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::Condition cond = masm.compareDoubles(comp->jsop(), lhs, rhs);
    NaNCond ifNaN = (cond == Assembler::NotEqual) ? NaN_IsTrue : NaN_IsFalse;
    emitBranch(cond, comp->ifTrue(), comp->ifFalse(), ifNaN);
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
CodeGeneratorX86Shared::visitOutOfLineBailout(OutOfLineBailout *ool)
{
    if (!deoptLabel_)
        deoptLabel_ = new HeapLabel();

    masm.push(Imm32(ool->snapshot()->snapshotOffset()));
    masm.jmp(deoptLabel_);
    return true;
}

bool
CodeGeneratorX86Shared::visitAddI(LAddI *ins)
{
    if (ins->rhs()->isConstant())
        masm.addl(Imm32(ToInt32(ins->rhs())), ToOperand(ins->lhs()));
    else
        masm.addl(ToOperand(ins->rhs()), ToRegister(ins->lhs()));

    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorX86Shared::visitSubI(LSubI *ins)
{
    if (ins->rhs()->isConstant())
        masm.subl(Imm32(ToInt32(ins->rhs())), ToOperand(ins->lhs()));
    else
        masm.subl(ToOperand(ins->rhs()), ToRegister(ins->lhs()));

    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorX86Shared::visitMulI(LMulI *ins)
{
    const LAllocation *lhs = ins->lhs();
    const LAllocation *rhs = ins->rhs();
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
                
                int32 shift;
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
            masm.testl(ToRegister(lhs), ToRegister(lhs));
            if (!bailoutIf(Assembler::Equal, ins->snapshot()))
                return false;
        }
    }

    return true;
}

bool
CodeGeneratorX86Shared::visitDivI(LDivI *ins)
{
    Register remainder = ToRegister(ins->remainder());
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());

    JS_ASSERT(remainder == edx);
    JS_ASSERT(lhs == eax);

    
    masm.testl(rhs, rhs);
    if (!bailoutIf(Assembler::Zero, ins->snapshot()))
        return false;

    
    Label notmin;
    masm.cmpl(lhs, Imm32(INT_MIN));
    masm.j(Assembler::NotEqual, &notmin);
    masm.cmpl(rhs, Imm32(-1));
    if (!bailoutIf(Assembler::Equal, ins->snapshot()))
        return false;
    masm.bind(&notmin);

    
    Label nonzero;
    masm.testl(lhs, lhs);
    masm.j(Assembler::NonZero, &nonzero);
    masm.cmpl(rhs, Imm32(0));
    if (!bailoutIf(Assembler::LessThan, ins->snapshot()))
        return false;
    masm.bind(&nonzero);

    
    masm.cdq();
    masm.idiv(rhs);

    
    masm.testl(remainder, remainder);
    if (!bailoutIf(Assembler::NonZero, ins->snapshot()))
        return false;

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
CodeGeneratorX86Shared::visitCallGeneric(LCallGeneric *call)
{
    
    const LAllocation *callee = call->getFunction();
    Register calleereg  = ToRegister(callee);

    
    const LAllocation *obj = call->getTempObject();
    Register objreg  = ToRegister(obj);
    masm.mov(calleereg, objreg);
    
    const LAllocation *tok = call->getToken();
    Register tokreg  = ToRegister(tok);

    
    const LAllocation *nargs = call->getNargsReg();
    Register nargsreg = ToRegister(nargs);

    uint32 callargslot  = call->argslot();
    uint32 unusedStack = StackOffsetOfPassedArg(callargslot);

    
    masm.loadObjClass(objreg, tokreg);
    masm.cmpPtr(tokreg, ImmWord(&js::FunctionClass));
    if (!bailoutIf(Assembler::NotEqual, call->snapshot()))
        return false;

    
    masm.mov(objreg, tokreg);

    Label end, invoke;

    
    
    masm.movl(Operand(objreg, offsetof(JSFunction, flags)), nargsreg);
    masm.andl(Imm32(JSFUN_KINDMASK), nargsreg);
    masm.cmpl(nargsreg, Imm32(JSFUN_INTERPRETED));
    
    masm.branch32(Assembler::Below, nargsreg, Imm32(JSFUN_INTERPRETED), &invoke);

    
    masm.movePtr(Operand(objreg, offsetof(JSFunction, u.i.script_)), objreg);
    masm.movePtr(Operand(objreg, offsetof(JSScript, ion)), objreg);

    
    Label compiled;
    
    masm.branchPtr(Assembler::BelowOrEqual, objreg, ImmWord(ION_DISABLED_SCRIPT), &invoke);
    
    masm.jmp(&compiled);
    {
        masm.bind(&invoke);

        typedef bool (*pf)(JSContext *, JSFunction *, uint32, Value *, Value *);
        static const VMFunction InvokeFunctionInfo = FunctionInfo<pf>(InvokeFunction);

        
        masm.freeStack(unusedStack);

        pushArg(StackPointer);          
        pushArg(Imm32(call->nargs()));  
        pushArg(tokreg);                

        if (!callVM(InvokeFunctionInfo, call))
            return false;

        
        masm.reserveStack(unusedStack);

        
        masm.jump(&end);
    }
    masm.bind(&compiled);

    
    uint32 stackSize = masm.framePushed() - unusedStack;
    uint32 sizeDescriptor = (stackSize << FRAMETYPE_BITS) | IonFrame_JS;

    
    if (unusedStack)
        masm.freeStack(unusedStack);

    
    masm.Push(tokreg);
    masm.Push(Imm32(sizeDescriptor));

    
    {
        Label thunk, rejoin;

        
        IonCompartment *ion = gen->ionCompartment();
        IonCode *argumentsRectifier = ion->getArgumentsRectifier(gen->cx);
        if (!argumentsRectifier)
            return false;

        
        masm.load16(Operand(tokreg, offsetof(JSFunction, nargs)), nargsreg);
        masm.cmpl(nargsreg, Imm32(call->nargs()));
        masm.j(Assembler::Above, &thunk);

        
        masm.movePtr(Operand(objreg, offsetof(IonScript, method_)), objreg);
        masm.movePtr(Operand(objreg, IonCode::OffsetOfCode()), objreg);
        masm.call(objreg);
        if (!createSafepoint(call))
            return false;
        masm.jump(&rejoin);

        
        masm.bind(&thunk);
        masm.mov(Imm32(call->nargs()), ArgumentsRectifierReg);
        masm.movePtr(ImmWord(argumentsRectifier->raw()), ecx); 
        masm.call(ecx);
        if (!createSafepoint(call))
            return false;
        masm.bind(&rejoin);
    }

    
    int prefixGarbage = 2 * sizeof(void *);
    int restoreDiff = prefixGarbage - unusedStack;
    
    if (restoreDiff > 0)
        masm.freeStack(restoreDiff);
    else if (restoreDiff < 0)
        masm.reserveStack(-restoreDiff);

    masm.bind(&end);

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
      case JSOP_SUB:
        masm.subsd(ToFloatRegister(input), ToFloatRegister(output));
        break;
      case JSOP_MUL:
        masm.mulsd(ToFloatRegister(input), ToFloatRegister(output));
        break;
      case JSOP_DIV:
        masm.divsd(ToFloatRegister(input), ToFloatRegister(output));
        break;
      default:
        JS_NOT_REACHED("unexpected opcode");
        return false;
    }
    return true;
}

bool
CodeGeneratorX86Shared::visitBoundsCheck(LBoundsCheck *lir)
{
    if (lir->index()->isConstant())
        masm.cmpl(ToRegister(lir->length()), Imm32(ToInt32(lir->index())));
    else
        masm.cmpl(ToRegister(lir->length()), ToRegister(lir->index()));
    return bailoutIf(Assembler::BelowOrEqual, lir->snapshot());
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

    masm.loadBaseShape(obj, tmp);
    masm.cmpPtr(Operand(tmp, BaseShape::offsetOfClass()), ImmWord(guard->mir()->getClass()));
    if (!bailoutIf(Assembler::NotEqual, guard->snapshot()))
        return false;
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

JS_STATIC_ASSERT(INT_MIN == int(0x80000000));

void
CodeGeneratorX86Shared::emitTruncateDouble(const FloatRegister &src, const Register &dest, Label *fail)
{
    masm.cvttsd2si(src, dest);
    masm.cmpl(dest, Imm32(INT_MIN));
    masm.j(Assembler::Equal, fail);
}

Operand
CodeGeneratorX86Shared::createArrayElementOperand(Register elements, const LAllocation *index)
{
    if (index->isConstant())
        return Operand(elements, ToInt32(index) * sizeof(js::Value));

    return Operand(elements, ToRegister(index), TimesEight);
}
