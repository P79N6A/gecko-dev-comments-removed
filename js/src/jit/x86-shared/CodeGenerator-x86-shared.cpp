





#include "jit/x86-shared/CodeGenerator-x86-shared.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#include "jsmath.h"

#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/Linker.h"
#include "jit/RangeAnalysis.h"
#include "vm/TraceLogging.h"

#include "jit/shared/CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::Abs;
using mozilla::FloatingPoint;
using mozilla::FloorLog2;
using mozilla::NegativeInfinity;
using mozilla::SpecificNaN;

using JS::GenericNaN;

namespace js {
namespace jit {

CodeGeneratorX86Shared::CodeGeneratorX86Shared(MIRGenerator* gen, LIRGraph* graph, MacroAssembler* masm)
  : CodeGeneratorShared(gen, graph, masm)
{
}

bool
CodeGeneratorX86Shared::generatePrologue()
{
    MOZ_ASSERT(masm.framePushed() == 0);
    MOZ_ASSERT(!gen->compilingAsmJS());

    
    if (isProfilerInstrumentationEnabled())
        masm.profilerEnterFrame(StackPointer, CallTempReg0);

    
    masm.assertStackAlignment(JitStackAlignment, 0);

    
    masm.reserveStack(frameSize());

    emitTracelogIonStart();

    return true;
}

bool
CodeGeneratorX86Shared::generateEpilogue()
{
    MOZ_ASSERT(!gen->compilingAsmJS());

    masm.bind(&returnLabel_);

    emitTracelogIonStop();

    
    masm.freeStack(frameSize());
    MOZ_ASSERT(masm.framePushed() == 0);

    
    
    if (isProfilerInstrumentationEnabled())
        masm.profilerExitFrame();

    masm.ret();
    return true;
}

void
OutOfLineBailout::accept(CodeGeneratorX86Shared* codegen)
{
    codegen->visitOutOfLineBailout(this);
}

void
CodeGeneratorX86Shared::emitBranch(Assembler::Condition cond, MBasicBlock* mirTrue,
                                   MBasicBlock* mirFalse, Assembler::NaNCond ifNaN)
{
    if (ifNaN == Assembler::NaN_IsFalse)
        jumpToBlock(mirFalse, Assembler::Parity);
    else if (ifNaN == Assembler::NaN_IsTrue)
        jumpToBlock(mirTrue, Assembler::Parity);

    if (isNextBlock(mirFalse->lir())) {
        jumpToBlock(mirTrue, cond);
    } else {
        jumpToBlock(mirFalse, Assembler::InvertCondition(cond));
        jumpToBlock(mirTrue);
    }
}

void
CodeGeneratorX86Shared::visitDouble(LDouble* ins)
{
    const LDefinition* out = ins->getDef(0);
    masm.loadConstantDouble(ins->getDouble(), ToFloatRegister(out));
}

void
CodeGeneratorX86Shared::visitFloat32(LFloat32* ins)
{
    const LDefinition* out = ins->getDef(0);
    masm.loadConstantFloat32(ins->getFloat(), ToFloatRegister(out));
}

void
CodeGeneratorX86Shared::visitTestIAndBranch(LTestIAndBranch* test)
{
    const LAllocation* opd = test->input();

    
    masm.test32(ToRegister(opd), ToRegister(opd));
    emitBranch(Assembler::NonZero, test->ifTrue(), test->ifFalse());
}

void
CodeGeneratorX86Shared::visitTestDAndBranch(LTestDAndBranch* test)
{
    const LAllocation* opd = test->input();

    
    
    
    
    
    
    
    
    
    
    masm.zeroDouble(ScratchDoubleReg);
    masm.vucomisd(ScratchDoubleReg, ToFloatRegister(opd));
    emitBranch(Assembler::NotEqual, test->ifTrue(), test->ifFalse());
}

void
CodeGeneratorX86Shared::visitTestFAndBranch(LTestFAndBranch* test)
{
    const LAllocation* opd = test->input();
    
    masm.zeroFloat32(ScratchFloat32Reg);
    masm.vucomiss(ScratchFloat32Reg, ToFloatRegister(opd));
    emitBranch(Assembler::NotEqual, test->ifTrue(), test->ifFalse());
}

void
CodeGeneratorX86Shared::visitBitAndAndBranch(LBitAndAndBranch* baab)
{
    if (baab->right()->isConstant())
        masm.test32(ToRegister(baab->left()), Imm32(ToInt32(baab->right())));
    else
        masm.test32(ToRegister(baab->left()), ToRegister(baab->right()));
    emitBranch(Assembler::NonZero, baab->ifTrue(), baab->ifFalse());
}

void
CodeGeneratorX86Shared::emitCompare(MCompare::CompareType type, const LAllocation* left, const LAllocation* right)
{
#ifdef JS_CODEGEN_X64
    if (type == MCompare::Compare_Object) {
        masm.cmpPtr(ToRegister(left), ToOperand(right));
        return;
    }
#endif

    if (right->isConstant())
        masm.cmp32(ToRegister(left), Imm32(ToInt32(right)));
    else
        masm.cmp32(ToRegister(left), ToOperand(right));
}

void
CodeGeneratorX86Shared::visitCompare(LCompare* comp)
{
    MCompare* mir = comp->mir();
    emitCompare(mir->compareType(), comp->left(), comp->right());
    masm.emitSet(JSOpToCondition(mir->compareType(), comp->jsop()), ToRegister(comp->output()));
}

void
CodeGeneratorX86Shared::visitCompareAndBranch(LCompareAndBranch* comp)
{
    MCompare* mir = comp->cmpMir();
    emitCompare(mir->compareType(), comp->left(), comp->right());
    Assembler::Condition cond = JSOpToCondition(mir->compareType(), comp->jsop());
    emitBranch(cond, comp->ifTrue(), comp->ifFalse());
}

void
CodeGeneratorX86Shared::visitCompareD(LCompareD* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());

    Assembler::NaNCond nanCond = Assembler::NaNCondFromDoubleCondition(cond);
    if (comp->mir()->operandsAreNeverNaN())
        nanCond = Assembler::NaN_HandledByCond;

    masm.compareDouble(cond, lhs, rhs);
    masm.emitSet(Assembler::ConditionFromDoubleCondition(cond), ToRegister(comp->output()), nanCond);
}

void
CodeGeneratorX86Shared::visitCompareF(LCompareF* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());

    Assembler::NaNCond nanCond = Assembler::NaNCondFromDoubleCondition(cond);
    if (comp->mir()->operandsAreNeverNaN())
        nanCond = Assembler::NaN_HandledByCond;

    masm.compareFloat(cond, lhs, rhs);
    masm.emitSet(Assembler::ConditionFromDoubleCondition(cond), ToRegister(comp->output()), nanCond);
}

void
CodeGeneratorX86Shared::visitNotI(LNotI* ins)
{
    masm.cmp32(ToRegister(ins->input()), Imm32(0));
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()));
}

void
CodeGeneratorX86Shared::visitNotD(LNotD* ins)
{
    FloatRegister opd = ToFloatRegister(ins->input());

    
    
    Assembler::NaNCond nanCond = Assembler::NaN_IsTrue;
    if (ins->mir()->operandIsNeverNaN())
        nanCond = Assembler::NaN_HandledByCond;

    masm.zeroDouble(ScratchDoubleReg);
    masm.compareDouble(Assembler::DoubleEqualOrUnordered, opd, ScratchDoubleReg);
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()), nanCond);
}

void
CodeGeneratorX86Shared::visitNotF(LNotF* ins)
{
    FloatRegister opd = ToFloatRegister(ins->input());

    
    
    Assembler::NaNCond nanCond = Assembler::NaN_IsTrue;
    if (ins->mir()->operandIsNeverNaN())
        nanCond = Assembler::NaN_HandledByCond;

    masm.zeroFloat32(ScratchFloat32Reg);
    masm.compareFloat(Assembler::DoubleEqualOrUnordered, opd, ScratchFloat32Reg);
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()), nanCond);
}

void
CodeGeneratorX86Shared::visitCompareDAndBranch(LCompareDAndBranch* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->cmpMir()->jsop());

    Assembler::NaNCond nanCond = Assembler::NaNCondFromDoubleCondition(cond);
    if (comp->cmpMir()->operandsAreNeverNaN())
        nanCond = Assembler::NaN_HandledByCond;

    masm.compareDouble(cond, lhs, rhs);
    emitBranch(Assembler::ConditionFromDoubleCondition(cond), comp->ifTrue(), comp->ifFalse(), nanCond);
}

void
CodeGeneratorX86Shared::visitCompareFAndBranch(LCompareFAndBranch* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->cmpMir()->jsop());

    Assembler::NaNCond nanCond = Assembler::NaNCondFromDoubleCondition(cond);
    if (comp->cmpMir()->operandsAreNeverNaN())
        nanCond = Assembler::NaN_HandledByCond;

    masm.compareFloat(cond, lhs, rhs);
    emitBranch(Assembler::ConditionFromDoubleCondition(cond), comp->ifTrue(), comp->ifFalse(), nanCond);
}

void
CodeGeneratorX86Shared::visitAsmJSPassStackArg(LAsmJSPassStackArg* ins)
{
    const MAsmJSPassStackArg* mir = ins->mir();
    Address dst(StackPointer, mir->spOffset());
    if (ins->arg()->isConstant()) {
        masm.storePtr(ImmWord(ToInt32(ins->arg())), dst);
    } else {
        if (ins->arg()->isGeneralReg()) {
            masm.storePtr(ToRegister(ins->arg()), dst);
        } else {
            switch (mir->input()->type()) {
              case MIRType_Double:
              case MIRType_Float32:
                masm.storeDouble(ToFloatRegister(ins->arg()), dst);
                return;
              
              
              case MIRType_Int32x4:
                masm.storeAlignedInt32x4(ToFloatRegister(ins->arg()), dst);
                return;
              case MIRType_Float32x4:
                masm.storeAlignedFloat32x4(ToFloatRegister(ins->arg()), dst);
                return;
              default: break;
            }
            MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("unexpected mir type in AsmJSPassStackArg");
        }
    }
}

void
CodeGeneratorX86Shared::visitOutOfLineLoadTypedArrayOutOfBounds(OutOfLineLoadTypedArrayOutOfBounds* ool)
{
    switch (ool->viewType()) {
      case Scalar::Float32x4:
      case Scalar::Int32x4:
      case Scalar::MaxTypedArrayViewType:
        MOZ_CRASH("unexpected array type");
      case Scalar::Float32:
        masm.loadConstantFloat32(float(GenericNaN()), ool->dest().fpu());
        break;
      case Scalar::Float64:
        masm.loadConstantDouble(GenericNaN(), ool->dest().fpu());
        break;
      case Scalar::Int8:
      case Scalar::Uint8:
      case Scalar::Int16:
      case Scalar::Uint16:
      case Scalar::Int32:
      case Scalar::Uint32:
      case Scalar::Uint8Clamped:
        Register destReg = ool->dest().gpr();
        masm.mov(ImmWord(0), destReg);
        break;
    }
    masm.jmp(ool->rejoin());
}

void
CodeGeneratorX86Shared::visitOffsetBoundsCheck(OffsetBoundsCheck* oolCheck)
{
    
    
    
    
    MOZ_ASSERT(oolCheck->offset() != 0,
               "An access without a constant offset doesn't need a separate OffsetBoundsCheck");
    masm.cmp32(oolCheck->ptrReg(), Imm32(-uint32_t(oolCheck->offset())));
    masm.j(Assembler::Below, oolCheck->outOfBounds());

#ifdef JS_CODEGEN_X64
    
    
    
    masm.movslq(oolCheck->ptrReg(), oolCheck->ptrReg());
#endif

    masm.jmp(oolCheck->rejoin());
}

uint32_t
CodeGeneratorX86Shared::emitAsmJSBoundsCheckBranch(const MAsmJSHeapAccess* access,
                                                   const MInstruction* mir,
                                                   Register ptr, Label* fail)
{
    

    MOZ_ASSERT(gen->needsAsmJSBoundsCheckBranch(access));

    Label* pass = nullptr;

    
    
    
    
    if (access->offset() != 0) {
        OffsetBoundsCheck* oolCheck = new(alloc()) OffsetBoundsCheck(fail, ptr, access->offset());
        fail = oolCheck->entry();
        pass = oolCheck->rejoin();
        addOutOfLineCode(oolCheck, mir);
    }

    
    
    
    
    
    uint32_t maybeCmpOffset = masm.cmp32WithPatch(ptr, Imm32(-access->endOffset())).offset();
    masm.j(Assembler::Above, fail);

    if (pass)
        masm.bind(pass);

    return maybeCmpOffset;
}

void
CodeGeneratorX86Shared::cleanupAfterAsmJSBoundsCheckBranch(const MAsmJSHeapAccess* access,
                                                           Register ptr)
{
    

    MOZ_ASSERT(gen->needsAsmJSBoundsCheckBranch(access));

#ifdef JS_CODEGEN_X64
    
    if (access->offset() != 0) {
        
        
        masm.movl(ptr, ptr);
    }
#endif
}

bool
CodeGeneratorX86Shared::generateOutOfLineCode()
{
    if (!CodeGeneratorShared::generateOutOfLineCode())
        return false;

    if (deoptLabel_.used()) {
        
        masm.bind(&deoptLabel_);

        
        masm.push(Imm32(frameSize()));

        JitCode* handler = gen->jitRuntime()->getGenericBailoutHandler();
        masm.jmp(ImmPtr(handler->raw()), Relocation::JITCODE);
    }

    return true;
}

class BailoutJump {
    Assembler::Condition cond_;

  public:
    explicit BailoutJump(Assembler::Condition cond) : cond_(cond)
    { }
#ifdef JS_CODEGEN_X86
    void operator()(MacroAssembler& masm, uint8_t* code) const {
        masm.j(cond_, ImmPtr(code), Relocation::HARDCODED);
    }
#endif
    void operator()(MacroAssembler& masm, Label* label) const {
        masm.j(cond_, label);
    }
};

class BailoutLabel {
    Label* label_;

  public:
    explicit BailoutLabel(Label* label) : label_(label)
    { }
#ifdef JS_CODEGEN_X86
    void operator()(MacroAssembler& masm, uint8_t* code) const {
        masm.retarget(label_, ImmPtr(code), Relocation::HARDCODED);
    }
#endif
    void operator()(MacroAssembler& masm, Label* label) const {
        masm.retarget(label_, label);
    }
};

template <typename T> void
CodeGeneratorX86Shared::bailout(const T& binder, LSnapshot* snapshot)
{
    encode(snapshot);

    
    
    
    MOZ_ASSERT_IF(frameClass_ != FrameSizeClass::None() && deoptTable_,
                  frameClass_.frameSize() == masm.framePushed());

#ifdef JS_CODEGEN_X86
    
    
    
    if (assignBailoutId(snapshot)) {
        binder(masm, deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE);
        return;
    }
#endif

    
    
    
    
    
    
    InlineScriptTree* tree = snapshot->mir()->block()->trackedTree();
    OutOfLineBailout* ool = new(alloc()) OutOfLineBailout(snapshot);
    addOutOfLineCode(ool, new(alloc()) BytecodeSite(tree, tree->script()->code()));

    binder(masm, ool->entry());
}

void
CodeGeneratorX86Shared::bailoutIf(Assembler::Condition condition, LSnapshot* snapshot)
{
    bailout(BailoutJump(condition), snapshot);
}

void
CodeGeneratorX86Shared::bailoutIf(Assembler::DoubleCondition condition, LSnapshot* snapshot)
{
    MOZ_ASSERT(Assembler::NaNCondFromDoubleCondition(condition) == Assembler::NaN_HandledByCond);
    bailoutIf(Assembler::ConditionFromDoubleCondition(condition), snapshot);
}

void
CodeGeneratorX86Shared::bailoutFrom(Label* label, LSnapshot* snapshot)
{
    MOZ_ASSERT(label->used() && !label->bound());
    bailout(BailoutLabel(label), snapshot);
}

void
CodeGeneratorX86Shared::bailout(LSnapshot* snapshot)
{
    Label label;
    masm.jump(&label);
    bailoutFrom(&label, snapshot);
}

void
CodeGeneratorX86Shared::visitOutOfLineBailout(OutOfLineBailout* ool)
{
    masm.push(Imm32(ool->snapshot()->snapshotOffset()));
    masm.jmp(&deoptLabel_);
}

void
CodeGeneratorX86Shared::visitMinMaxD(LMinMaxD* ins)
{
    FloatRegister first = ToFloatRegister(ins->first());
    FloatRegister second = ToFloatRegister(ins->second());
#ifdef DEBUG
    FloatRegister output = ToFloatRegister(ins->output());
    MOZ_ASSERT(first == output);
#endif

    Label done, nan, minMaxInst;

    
    
    
    
    
    masm.vucomisd(second, first);
    masm.j(Assembler::NotEqual, &minMaxInst);
    if (!ins->mir()->range() || ins->mir()->range()->canBeNaN())
        masm.j(Assembler::Parity, &nan);

    
    
    
    if (ins->mir()->isMax())
        masm.vandpd(second, first, first);
    else
        masm.vorpd(second, first, first);
    masm.jump(&done);

    
    
    
    if (!ins->mir()->range() || ins->mir()->range()->canBeNaN()) {
        masm.bind(&nan);
        masm.vucomisd(first, first);
        masm.j(Assembler::Parity, &done);
    }

    
    
    masm.bind(&minMaxInst);
    if (ins->mir()->isMax())
        masm.vmaxsd(second, first, first);
    else
        masm.vminsd(second, first, first);

    masm.bind(&done);
}

void
CodeGeneratorX86Shared::visitMinMaxF(LMinMaxF* ins)
{
    FloatRegister first = ToFloatRegister(ins->first());
    FloatRegister second = ToFloatRegister(ins->second());
#ifdef DEBUG
    FloatRegister output = ToFloatRegister(ins->output());
    MOZ_ASSERT(first == output);
#endif

    Label done, nan, minMaxInst;

    
    
    
    
    
    masm.vucomiss(second, first);
    masm.j(Assembler::NotEqual, &minMaxInst);
    if (!ins->mir()->range() || ins->mir()->range()->canBeNaN())
        masm.j(Assembler::Parity, &nan);

    
    
    
    if (ins->mir()->isMax())
        masm.vandps(second, first, first);
    else
        masm.vorps(second, first, first);
    masm.jump(&done);

    
    
    
    if (!ins->mir()->range() || ins->mir()->range()->canBeNaN()) {
        masm.bind(&nan);
        masm.vucomiss(first, first);
        masm.j(Assembler::Parity, &done);
    }

    
    
    masm.bind(&minMaxInst);
    if (ins->mir()->isMax())
        masm.vmaxss(second, first, first);
    else
        masm.vminss(second, first, first);

    masm.bind(&done);
}

void
CodeGeneratorX86Shared::visitAbsD(LAbsD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    MOZ_ASSERT(input == ToFloatRegister(ins->output()));
    
    masm.loadConstantDouble(SpecificNaN<double>(0, FloatingPoint<double>::kSignificandBits),
                            ScratchDoubleReg);
    masm.vandpd(ScratchDoubleReg, input, input);
}

void
CodeGeneratorX86Shared::visitAbsF(LAbsF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    MOZ_ASSERT(input == ToFloatRegister(ins->output()));
    
    masm.loadConstantFloat32(SpecificNaN<float>(0, FloatingPoint<float>::kSignificandBits),
                             ScratchFloat32Reg);
    masm.vandps(ScratchFloat32Reg, input, input);
}

void
CodeGeneratorX86Shared::visitClzI(LClzI* ins)
{
    Register input = ToRegister(ins->input());
    Register output = ToRegister(ins->output());

    
    Label done, nonzero;
    if (!ins->mir()->operandIsNeverZero()) {
        masm.test32(input, input);
        masm.j(Assembler::NonZero, &nonzero);
        masm.move32(Imm32(32), output);
        masm.jump(&done);
    }

    masm.bind(&nonzero);
    masm.bsr(input, output);
    masm.xor32(Imm32(0x1F), output);
    masm.bind(&done);
}

void
CodeGeneratorX86Shared::visitSqrtD(LSqrtD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());
    masm.vsqrtsd(input, output, output);
}

void
CodeGeneratorX86Shared::visitSqrtF(LSqrtF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());
    masm.vsqrtss(input, output, output);
}

void
CodeGeneratorX86Shared::visitPowHalfD(LPowHalfD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    Label done, sqrt;

    if (!ins->mir()->operandIsNeverNegativeInfinity()) {
        
        masm.loadConstantDouble(NegativeInfinity<double>(), ScratchDoubleReg);

        Assembler::DoubleCondition cond = Assembler::DoubleNotEqualOrUnordered;
        if (ins->mir()->operandIsNeverNaN())
            cond = Assembler::DoubleNotEqual;
        masm.branchDouble(cond, input, ScratchDoubleReg, &sqrt);

        
        masm.zeroDouble(input);
        masm.subDouble(ScratchDoubleReg, input);
        masm.jump(&done);

        masm.bind(&sqrt);
    }

    if (!ins->mir()->operandIsNeverNegativeZero()) {
        
        masm.zeroDouble(ScratchDoubleReg);
        masm.addDouble(ScratchDoubleReg, input);
    }

    masm.vsqrtsd(input, output, output);

    masm.bind(&done);
}

class OutOfLineUndoALUOperation : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LInstruction* ins_;

  public:
    explicit OutOfLineUndoALUOperation(LInstruction* ins)
        : ins_(ins)
    { }

    virtual void accept(CodeGeneratorX86Shared* codegen) {
        codegen->visitOutOfLineUndoALUOperation(this);
    }
    LInstruction* ins() const {
        return ins_;
    }
};

void
CodeGeneratorX86Shared::visitAddI(LAddI* ins)
{
    if (ins->rhs()->isConstant())
        masm.addl(Imm32(ToInt32(ins->rhs())), ToOperand(ins->lhs()));
    else
        masm.addl(ToOperand(ins->rhs()), ToRegister(ins->lhs()));

    if (ins->snapshot()) {
        if (ins->recoversInput()) {
            OutOfLineUndoALUOperation* ool = new(alloc()) OutOfLineUndoALUOperation(ins);
            addOutOfLineCode(ool, ins->mir());
            masm.j(Assembler::Overflow, ool->entry());
        } else {
            bailoutIf(Assembler::Overflow, ins->snapshot());
        }
    }
}

void
CodeGeneratorX86Shared::visitSubI(LSubI* ins)
{
    if (ins->rhs()->isConstant())
        masm.subl(Imm32(ToInt32(ins->rhs())), ToOperand(ins->lhs()));
    else
        masm.subl(ToOperand(ins->rhs()), ToRegister(ins->lhs()));

    if (ins->snapshot()) {
        if (ins->recoversInput()) {
            OutOfLineUndoALUOperation* ool = new(alloc()) OutOfLineUndoALUOperation(ins);
            addOutOfLineCode(ool, ins->mir());
            masm.j(Assembler::Overflow, ool->entry());
        } else {
            bailoutIf(Assembler::Overflow, ins->snapshot());
        }
    }
}

void
CodeGeneratorX86Shared::visitOutOfLineUndoALUOperation(OutOfLineUndoALUOperation* ool)
{
    LInstruction* ins = ool->ins();
    Register reg = ToRegister(ins->getDef(0));

    mozilla::DebugOnly<LAllocation*> lhs = ins->getOperand(0);
    LAllocation* rhs = ins->getOperand(1);

    MOZ_ASSERT(reg == ToRegister(lhs));
    MOZ_ASSERT_IF(rhs->isGeneralReg(), reg != ToRegister(rhs));

    
    
    
    
    

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

    bailout(ool->ins()->snapshot());
}

class MulNegativeZeroCheck : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    LMulI* ins_;

  public:
    explicit MulNegativeZeroCheck(LMulI* ins)
      : ins_(ins)
    { }

    virtual void accept(CodeGeneratorX86Shared* codegen) {
        codegen->visitMulNegativeZeroCheck(this);
    }
    LMulI* ins() const {
        return ins_;
    }
};

void
CodeGeneratorX86Shared::visitMulI(LMulI* ins)
{
    const LAllocation* lhs = ins->lhs();
    const LAllocation* rhs = ins->rhs();
    MMul* mul = ins->mir();
    MOZ_ASSERT_IF(mul->mode() == MMul::Integer, !mul->canBeNegativeZero() && !mul->canOverflow());

    if (rhs->isConstant()) {
        
        int32_t constant = ToInt32(rhs);
        if (mul->canBeNegativeZero() && constant <= 0) {
            Assembler::Condition bailoutCond = (constant == 0) ? Assembler::Signed : Assembler::Equal;
            masm.test32(ToRegister(lhs), ToRegister(lhs));
            bailoutIf(bailoutCond, ins->snapshot());
        }

        switch (constant) {
          case -1:
            masm.negl(ToOperand(lhs));
            break;
          case 0:
            masm.xorl(ToOperand(lhs), ToRegister(lhs));
            return; 
          case 1:
            
            return; 
          case 2:
            masm.addl(ToOperand(lhs), ToRegister(lhs));
            break;
          default:
            if (!mul->canOverflow() && constant > 0) {
                
                int32_t shift = FloorLog2(constant);
                if ((1 << shift) == constant) {
                    masm.shll(Imm32(shift), ToRegister(lhs));
                    return;
                }
            }
            masm.imull(Imm32(ToInt32(rhs)), ToRegister(lhs));
        }

        
        if (mul->canOverflow())
            bailoutIf(Assembler::Overflow, ins->snapshot());
    } else {
        masm.imull(ToOperand(rhs), ToRegister(lhs));

        
        if (mul->canOverflow())
            bailoutIf(Assembler::Overflow, ins->snapshot());

        if (mul->canBeNegativeZero()) {
            
            MulNegativeZeroCheck* ool = new(alloc()) MulNegativeZeroCheck(ins);
            addOutOfLineCode(ool, mul);

            masm.test32(ToRegister(lhs), ToRegister(lhs));
            masm.j(Assembler::Zero, ool->entry());
            masm.bind(ool->rejoin());
        }
    }
}

class ReturnZero : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    Register reg_;

  public:
    explicit ReturnZero(Register reg)
      : reg_(reg)
    { }

    virtual void accept(CodeGeneratorX86Shared* codegen) {
        codegen->visitReturnZero(this);
    }
    Register reg() const {
        return reg_;
    }
};

void
CodeGeneratorX86Shared::visitReturnZero(ReturnZero* ool)
{
    masm.mov(ImmWord(0), ool->reg());
    masm.jmp(ool->rejoin());
}

void
CodeGeneratorX86Shared::visitUDivOrMod(LUDivOrMod* ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());

    MOZ_ASSERT_IF(lhs != rhs, rhs != eax);
    MOZ_ASSERT(rhs != edx);
    MOZ_ASSERT_IF(output == eax, ToRegister(ins->remainder()) == edx);

    ReturnZero* ool = nullptr;

    
    if (lhs != eax)
        masm.mov(lhs, eax);

    
    if (ins->canBeDivideByZero()) {
        masm.test32(rhs, rhs);
        if (ins->mir()->isTruncated()) {
            if (!ool)
                ool = new(alloc()) ReturnZero(output);
            masm.j(Assembler::Zero, ool->entry());
        } else {
            bailoutIf(Assembler::Zero, ins->snapshot());
        }
    }

    
    masm.mov(ImmWord(0), edx);
    masm.udiv(rhs);

    
    if (ins->mir()->isDiv() && !ins->mir()->toDiv()->canTruncateRemainder()) {
        Register remainder = ToRegister(ins->remainder());
        masm.test32(remainder, remainder);
        bailoutIf(Assembler::NonZero, ins->snapshot());
    }

    
    
    if (!ins->mir()->isTruncated()) {
        masm.test32(output, output);
        bailoutIf(Assembler::Signed, ins->snapshot());
    }

    if (ool) {
        addOutOfLineCode(ool, ins->mir());
        masm.bind(ool->rejoin());
    }
}

void
CodeGeneratorX86Shared::visitMulNegativeZeroCheck(MulNegativeZeroCheck* ool)
{
    LMulI* ins = ool->ins();
    Register result = ToRegister(ins->output());
    Operand lhsCopy = ToOperand(ins->lhsCopy());
    Operand rhs = ToOperand(ins->rhs());
    MOZ_ASSERT_IF(lhsCopy.kind() == Operand::REG, lhsCopy.reg() != result.code());

    
    masm.movl(lhsCopy, result);
    masm.orl(rhs, result);
    bailoutIf(Assembler::Signed, ins->snapshot());

    masm.mov(ImmWord(0), result);
    masm.jmp(ool->rejoin());
}

void
CodeGeneratorX86Shared::visitDivPowTwoI(LDivPowTwoI* ins)
{
    Register lhs = ToRegister(ins->numerator());
    mozilla::DebugOnly<Register> output = ToRegister(ins->output());

    int32_t shift = ins->shift();
    bool negativeDivisor = ins->negativeDivisor();
    MDiv* mir = ins->mir();

    
    
    MOZ_ASSERT(lhs == output);

    if (!mir->isTruncated() && negativeDivisor) {
        
        masm.test32(lhs, lhs);
        bailoutIf(Assembler::Zero, ins->snapshot());
    }

    if (shift != 0) {
        if (!mir->isTruncated()) {
            
            masm.test32(lhs, Imm32(UINT32_MAX >> (32 - shift)));
            bailoutIf(Assembler::NonZero, ins->snapshot());
        }

        
        
        
        if (mir->canBeNegativeDividend()) {
            Register lhsCopy = ToRegister(ins->numeratorCopy());
            MOZ_ASSERT(lhsCopy != lhs);
            if (shift > 1)
                masm.sarl(Imm32(31), lhs);
            masm.shrl(Imm32(32 - shift), lhs);
            masm.addl(lhsCopy, lhs);
        }

        masm.sarl(Imm32(shift), lhs);
        if (negativeDivisor)
            masm.negl(lhs);
    } else if (shift == 0 && negativeDivisor) {
        
        masm.negl(lhs);
        if (!mir->isTruncated())
            bailoutIf(Assembler::Overflow, ins->snapshot());
    }
}

void
CodeGeneratorX86Shared::visitDivOrModConstantI(LDivOrModConstantI* ins) {
    Register lhs = ToRegister(ins->numerator());
    Register output = ToRegister(ins->output());
    int32_t d = ins->denominator();

    
    MOZ_ASSERT(output == eax || output == edx);
    MOZ_ASSERT(lhs != eax && lhs != edx);
    bool isDiv = (output == edx);

    
    
    MOZ_ASSERT((Abs(d) & (Abs(d) - 1)) != 0);

    
    
    ReciprocalMulConstants rmc = computeDivisionConstants(Abs(d));

    
    
    
    
    
    masm.movl(Imm32(rmc.multiplier), eax);
    masm.imull(lhs);
    if (rmc.multiplier < 0)
        masm.addl(lhs, edx);
    masm.sarl(Imm32(rmc.shiftAmount), edx);

    
    
    if (ins->canBeNegativeDividend()) {
        masm.movl(lhs, eax);
        masm.sarl(Imm32(31), eax);
        masm.subl(eax, edx);
    }

    
    if (d < 0)
        masm.negl(edx);

    if (!isDiv) {
        masm.imull(Imm32(-d), edx, eax);
        masm.addl(lhs, eax);
    }

    if (!ins->mir()->isTruncated()) {
        if (isDiv) {
            
            
            masm.imull(Imm32(d), edx, eax);
            masm.cmp32(lhs, eax);
            bailoutIf(Assembler::NotEqual, ins->snapshot());

            
            
            if (d < 0) {
                masm.test32(lhs, lhs);
                bailoutIf(Assembler::Zero, ins->snapshot());
            }
        } else if (ins->canBeNegativeDividend()) {
            
            
            Label done;

            masm.cmp32(lhs, Imm32(0));
            masm.j(Assembler::GreaterThanOrEqual, &done);

            masm.test32(eax, eax);
            bailoutIf(Assembler::Zero, ins->snapshot());

            masm.bind(&done);
        }
    }
}

void
CodeGeneratorX86Shared::visitDivI(LDivI* ins)
{
    Register remainder = ToRegister(ins->remainder());
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());

    MDiv* mir = ins->mir();

    MOZ_ASSERT_IF(lhs != rhs, rhs != eax);
    MOZ_ASSERT(rhs != edx);
    MOZ_ASSERT(remainder == edx);
    MOZ_ASSERT(output == eax);

    Label done;
    ReturnZero* ool = nullptr;

    
    
    if (lhs != eax)
        masm.mov(lhs, eax);

    
    if (mir->canBeDivideByZero()) {
        masm.test32(rhs, rhs);
        if (mir->canTruncateInfinities()) {
            
            if (!ool)
                ool = new(alloc()) ReturnZero(output);
            masm.j(Assembler::Zero, ool->entry());
        } else {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Zero, ins->snapshot());
        }
    }

    
    if (mir->canBeNegativeOverflow()) {
        Label notmin;
        masm.cmp32(lhs, Imm32(INT32_MIN));
        masm.j(Assembler::NotEqual, &notmin);
        masm.cmp32(rhs, Imm32(-1));
        if (mir->canTruncateOverflow()) {
            
            
            masm.j(Assembler::Equal, &done);
        } else {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Equal, ins->snapshot());
        }
        masm.bind(&notmin);
    }

    
    if (!mir->canTruncateNegativeZero() && mir->canBeNegativeZero()) {
        Label nonzero;
        masm.test32(lhs, lhs);
        masm.j(Assembler::NonZero, &nonzero);
        masm.cmp32(rhs, Imm32(0));
        bailoutIf(Assembler::LessThan, ins->snapshot());
        masm.bind(&nonzero);
    }

    
    if (lhs != eax)
        masm.mov(lhs, eax);
    masm.cdq();
    masm.idiv(rhs);

    if (!mir->canTruncateRemainder()) {
        
        masm.test32(remainder, remainder);
        bailoutIf(Assembler::NonZero, ins->snapshot());
    }

    masm.bind(&done);

    if (ool) {
        addOutOfLineCode(ool, mir);
        masm.bind(ool->rejoin());
    }
}

void
CodeGeneratorX86Shared::visitModPowTwoI(LModPowTwoI* ins)
{
    Register lhs = ToRegister(ins->getOperand(0));
    int32_t shift = ins->shift();

    Label negative;

    if (ins->mir()->canBeNegativeDividend()) {
        
        
        masm.branchTest32(Assembler::Signed, lhs, lhs, &negative);
    }

    masm.andl(Imm32((uint32_t(1) << shift) - 1), lhs);

    if (ins->mir()->canBeNegativeDividend()) {
        Label done;
        masm.jump(&done);

        
        masm.bind(&negative);

        
        
        
        
        
        
        masm.negl(lhs);
        masm.andl(Imm32((uint32_t(1) << shift) - 1), lhs);
        masm.negl(lhs);

        
        
        if (!ins->mir()->isTruncated())
            bailoutIf(Assembler::Zero, ins->snapshot());
        masm.bind(&done);
    }
}

class ModOverflowCheck : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    Label done_;
    LModI* ins_;
    Register rhs_;

  public:
    explicit ModOverflowCheck(LModI* ins, Register rhs)
      : ins_(ins), rhs_(rhs)
    { }

    virtual void accept(CodeGeneratorX86Shared* codegen) {
        codegen->visitModOverflowCheck(this);
    }
    Label* done() {
        return &done_;
    }
    LModI* ins() const {
        return ins_;
    }
    Register rhs() const {
        return rhs_;
    }
};

void
CodeGeneratorX86Shared::visitModOverflowCheck(ModOverflowCheck* ool)
{
    masm.cmp32(ool->rhs(), Imm32(-1));
    if (ool->ins()->mir()->isTruncated()) {
        masm.j(Assembler::NotEqual, ool->rejoin());
        masm.mov(ImmWord(0), edx);
        masm.jmp(ool->done());
    } else {
        bailoutIf(Assembler::Equal, ool->ins()->snapshot());
        masm.jmp(ool->rejoin());
    }
}

void
CodeGeneratorX86Shared::visitModI(LModI* ins)
{
    Register remainder = ToRegister(ins->remainder());
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());

    
    MOZ_ASSERT_IF(lhs != rhs, rhs != eax);
    MOZ_ASSERT(rhs != edx);
    MOZ_ASSERT(remainder == edx);
    MOZ_ASSERT(ToRegister(ins->getTemp(0)) == eax);

    Label done;
    ReturnZero* ool = nullptr;
    ModOverflowCheck* overflow = nullptr;

    
    if (lhs != eax)
        masm.mov(lhs, eax);

    
    if (ins->mir()->canBeDivideByZero()) {
        masm.test32(rhs, rhs);
        if (ins->mir()->isTruncated()) {
            if (!ool)
                ool = new(alloc()) ReturnZero(edx);
            masm.j(Assembler::Zero, ool->entry());
        } else {
            bailoutIf(Assembler::Zero, ins->snapshot());
        }
    }

    Label negative;

    
    if (ins->mir()->canBeNegativeDividend())
        masm.branchTest32(Assembler::Signed, lhs, lhs, &negative);

    
    {
        
        if (ins->mir()->canBePowerOfTwoDivisor()) {
            MOZ_ASSERT(rhs != remainder);

            
            
            
            
            
            
            Label notPowerOfTwo;
            masm.mov(rhs, remainder);
            masm.subl(Imm32(1), remainder);
            masm.branchTest32(Assembler::NonZero, remainder, rhs, &notPowerOfTwo);
            {
                masm.andl(lhs, remainder);
                masm.jmp(&done);
            }
            masm.bind(&notPowerOfTwo);
        }

        
        masm.mov(ImmWord(0), edx);
        masm.idiv(rhs);
    }

    
    if (ins->mir()->canBeNegativeDividend()) {
        masm.jump(&done);

        masm.bind(&negative);

        
        Label notmin;
        masm.cmp32(lhs, Imm32(INT32_MIN));
        overflow = new(alloc()) ModOverflowCheck(ins, rhs);
        masm.j(Assembler::Equal, overflow->entry());
        masm.bind(overflow->rejoin());
        masm.cdq();
        masm.idiv(rhs);

        if (!ins->mir()->isTruncated()) {
            
            masm.test32(remainder, remainder);
            bailoutIf(Assembler::Zero, ins->snapshot());
        }
    }

    masm.bind(&done);

    if (overflow) {
        addOutOfLineCode(overflow, ins->mir());
        masm.bind(overflow->done());
    }

    if (ool) {
        addOutOfLineCode(ool, ins->mir());
        masm.bind(ool->rejoin());
    }
}

void
CodeGeneratorX86Shared::visitBitNotI(LBitNotI* ins)
{
    const LAllocation* input = ins->getOperand(0);
    MOZ_ASSERT(!input->isConstant());

    masm.notl(ToOperand(input));
}

void
CodeGeneratorX86Shared::visitBitOpI(LBitOpI* ins)
{
    const LAllocation* lhs = ins->getOperand(0);
    const LAllocation* rhs = ins->getOperand(1);

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
            MOZ_CRASH("unexpected binary opcode");
    }
}

void
CodeGeneratorX86Shared::visitShiftI(LShiftI* ins)
{
    Register lhs = ToRegister(ins->lhs());
    const LAllocation* rhs = ins->rhs();

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
            } else if (ins->mir()->toUrsh()->fallible()) {
                
                masm.test32(lhs, lhs);
                bailoutIf(Assembler::Signed, ins->snapshot());
            }
            break;
          default:
            MOZ_CRASH("Unexpected shift op");
        }
    } else {
        MOZ_ASSERT(ToRegister(rhs) == ecx);
        switch (ins->bitop()) {
          case JSOP_LSH:
            masm.shll_cl(lhs);
            break;
          case JSOP_RSH:
            masm.sarl_cl(lhs);
            break;
          case JSOP_URSH:
            masm.shrl_cl(lhs);
            if (ins->mir()->toUrsh()->fallible()) {
                
                masm.test32(lhs, lhs);
                bailoutIf(Assembler::Signed, ins->snapshot());
            }
            break;
          default:
            MOZ_CRASH("Unexpected shift op");
        }
    }
}

void
CodeGeneratorX86Shared::visitUrshD(LUrshD* ins)
{
    Register lhs = ToRegister(ins->lhs());
    MOZ_ASSERT(ToRegister(ins->temp()) == lhs);

    const LAllocation* rhs = ins->rhs();
    FloatRegister out = ToFloatRegister(ins->output());

    if (rhs->isConstant()) {
        int32_t shift = ToInt32(rhs) & 0x1F;
        if (shift)
            masm.shrl(Imm32(shift), lhs);
    } else {
        MOZ_ASSERT(ToRegister(rhs) == ecx);
        masm.shrl_cl(lhs);
    }

    masm.convertUInt32ToDouble(lhs, out);
}

MoveOperand
CodeGeneratorX86Shared::toMoveOperand(const LAllocation* a) const
{
    if (a->isGeneralReg())
        return MoveOperand(ToRegister(a));
    if (a->isFloatReg())
        return MoveOperand(ToFloatRegister(a));
    return MoveOperand(StackPointer, ToStackOffset(a));
}

class OutOfLineTableSwitch : public OutOfLineCodeBase<CodeGeneratorX86Shared>
{
    MTableSwitch* mir_;
    CodeLabel jumpLabel_;

    void accept(CodeGeneratorX86Shared* codegen) {
        codegen->visitOutOfLineTableSwitch(this);
    }

  public:
    explicit OutOfLineTableSwitch(MTableSwitch* mir)
      : mir_(mir)
    {}

    MTableSwitch* mir() const {
        return mir_;
    }

    CodeLabel* jumpLabel() {
        return &jumpLabel_;
    }
};

void
CodeGeneratorX86Shared::visitOutOfLineTableSwitch(OutOfLineTableSwitch* ool)
{
    MTableSwitch* mir = ool->mir();

    masm.haltingAlign(sizeof(void*));
    masm.bind(ool->jumpLabel()->src());
    masm.addCodeLabel(*ool->jumpLabel());

    for (size_t i = 0; i < mir->numCases(); i++) {
        LBlock* caseblock = skipTrivialBlocks(mir->getCase(i))->lir();
        Label* caseheader = caseblock->label();
        uint32_t caseoffset = caseheader->offset();

        
        
        CodeLabel cl;
        masm.writeCodePointer(cl.dest());
        cl.src()->bind(caseoffset);
        masm.addCodeLabel(cl);
    }
}

void
CodeGeneratorX86Shared::emitTableSwitchDispatch(MTableSwitch* mir, Register index, Register base)
{
    Label* defaultcase = skipTrivialBlocks(mir->getDefault())->lir()->label();

    
    if (mir->low() != 0)
        masm.subl(Imm32(mir->low()), index);

    
    int32_t cases = mir->numCases();
    masm.cmp32(index, Imm32(cases));
    masm.j(AssemblerX86Shared::AboveOrEqual, defaultcase);

    
    
    
    OutOfLineTableSwitch* ool = new(alloc()) OutOfLineTableSwitch(mir);
    addOutOfLineCode(ool, mir);

    
    masm.mov(ool->jumpLabel()->dest(), base);
    Operand pointer = Operand(base, index, ScalePointer);

    
    masm.jmp(pointer);
}

void
CodeGeneratorX86Shared::visitMathD(LMathD* math)
{
    FloatRegister lhs = ToFloatRegister(math->lhs());
    Operand rhs = ToOperand(math->rhs());
    FloatRegister output = ToFloatRegister(math->output());

    switch (math->jsop()) {
      case JSOP_ADD:
        masm.vaddsd(rhs, lhs, output);
        break;
      case JSOP_SUB:
        masm.vsubsd(rhs, lhs, output);
        break;
      case JSOP_MUL:
        masm.vmulsd(rhs, lhs, output);
        break;
      case JSOP_DIV:
        masm.vdivsd(rhs, lhs, output);
        break;
      default:
        MOZ_CRASH("unexpected opcode");
    }
}

void
CodeGeneratorX86Shared::visitMathF(LMathF* math)
{
    FloatRegister lhs = ToFloatRegister(math->lhs());
    Operand rhs = ToOperand(math->rhs());
    FloatRegister output = ToFloatRegister(math->output());

    switch (math->jsop()) {
      case JSOP_ADD:
        masm.vaddss(rhs, lhs, output);
        break;
      case JSOP_SUB:
        masm.vsubss(rhs, lhs, output);
        break;
      case JSOP_MUL:
        masm.vmulss(rhs, lhs, output);
        break;
      case JSOP_DIV:
        masm.vdivss(rhs, lhs, output);
        break;
      default:
        MOZ_CRASH("unexpected opcode");
    }
}

void
CodeGeneratorX86Shared::visitFloor(LFloor* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister scratch = ScratchDoubleReg;
    Register output = ToRegister(lir->output());

    Label bailout;

    if (AssemblerX86Shared::HasSSE41()) {
        
        masm.branchNegativeZero(input, output, &bailout);
        bailoutFrom(&bailout, lir->snapshot());

        
        masm.vroundsd(X86Encoding::RoundDown, input, scratch, scratch);

        bailoutCvttsd2si(scratch, output, lir->snapshot());
    } else {
        Label negative, end;

        
        masm.zeroDouble(scratch);
        masm.branchDouble(Assembler::DoubleLessThan, input, scratch, &negative);

        
        masm.branchNegativeZero(input, output, &bailout);
        bailoutFrom(&bailout, lir->snapshot());

        
        bailoutCvttsd2si(input, output, lir->snapshot());

        masm.jump(&end);

        
        
        
        masm.bind(&negative);
        {
            
            
            bailoutCvttsd2si(input, output, lir->snapshot());

            
            masm.convertInt32ToDouble(output, scratch);
            masm.branchDouble(Assembler::DoubleEqualOrUnordered, input, scratch, &end);

            
            
            masm.subl(Imm32(1), output);
            
        }

        masm.bind(&end);
    }
}

void
CodeGeneratorX86Shared::visitFloorF(LFloorF* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister scratch = ScratchFloat32Reg;
    Register output = ToRegister(lir->output());

    Label bailout;

    if (AssemblerX86Shared::HasSSE41()) {
        
        masm.branchNegativeZeroFloat32(input, output, &bailout);
        bailoutFrom(&bailout, lir->snapshot());

        
        masm.vroundss(X86Encoding::RoundDown, input, scratch, scratch);

        bailoutCvttss2si(scratch, output, lir->snapshot());
    } else {
        Label negative, end;

        
        masm.zeroFloat32(scratch);
        masm.branchFloat(Assembler::DoubleLessThan, input, scratch, &negative);

        
        masm.branchNegativeZeroFloat32(input, output, &bailout);
        bailoutFrom(&bailout, lir->snapshot());

        
        bailoutCvttss2si(input, output, lir->snapshot());

        masm.jump(&end);

        
        
        
        masm.bind(&negative);
        {
            
            
            bailoutCvttss2si(input, output, lir->snapshot());

            
            masm.convertInt32ToFloat32(output, scratch);
            masm.branchFloat(Assembler::DoubleEqualOrUnordered, input, scratch, &end);

            
            
            masm.subl(Imm32(1), output);
            
        }

        masm.bind(&end);
    }
}

void
CodeGeneratorX86Shared::visitCeil(LCeil* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister scratch = ScratchDoubleReg;
    Register output = ToRegister(lir->output());

    Label bailout, lessThanMinusOne;

    
    masm.loadConstantDouble(-1, scratch);
    masm.branchDouble(Assembler::DoubleLessThanOrEqualOrUnordered, input,
                      scratch, &lessThanMinusOne);

    
    masm.vmovmskpd(input, output);
    masm.branchTest32(Assembler::NonZero, output, Imm32(1), &bailout);
    bailoutFrom(&bailout, lir->snapshot());

    if (AssemblerX86Shared::HasSSE41()) {
        
        masm.bind(&lessThanMinusOne);
        
        masm.vroundsd(X86Encoding::RoundUp, input, scratch, scratch);
        bailoutCvttsd2si(scratch, output, lir->snapshot());
        return;
    }

    
    Label end;

    
    
    
    
    bailoutCvttsd2si(input, output, lir->snapshot());
    masm.convertInt32ToDouble(output, scratch);
    masm.branchDouble(Assembler::DoubleEqualOrUnordered, input, scratch, &end);

    
    masm.addl(Imm32(1), output);
    
    bailoutIf(Assembler::Overflow, lir->snapshot());
    masm.jump(&end);

    
    masm.bind(&lessThanMinusOne);
    bailoutCvttsd2si(input, output, lir->snapshot());

    masm.bind(&end);
}

void
CodeGeneratorX86Shared::visitCeilF(LCeilF* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister scratch = ScratchFloat32Reg;
    Register output = ToRegister(lir->output());

    Label bailout, lessThanMinusOne;

    
    masm.loadConstantFloat32(-1.f, scratch);
    masm.branchFloat(Assembler::DoubleLessThanOrEqualOrUnordered, input,
                     scratch, &lessThanMinusOne);

    
    masm.vmovmskps(input, output);
    masm.branchTest32(Assembler::NonZero, output, Imm32(1), &bailout);
    bailoutFrom(&bailout, lir->snapshot());

    if (AssemblerX86Shared::HasSSE41()) {
        
        masm.bind(&lessThanMinusOne);
        
        masm.vroundss(X86Encoding::RoundUp, input, scratch, scratch);
        bailoutCvttss2si(scratch, output, lir->snapshot());
        return;
    }

    
    Label end;

    
    
    
    
    bailoutCvttss2si(input, output, lir->snapshot());
    masm.convertInt32ToFloat32(output, scratch);
    masm.branchFloat(Assembler::DoubleEqualOrUnordered, input, scratch, &end);

    
    masm.addl(Imm32(1), output);
    
    bailoutIf(Assembler::Overflow, lir->snapshot());
    masm.jump(&end);

    
    masm.bind(&lessThanMinusOne);
    bailoutCvttss2si(input, output, lir->snapshot());

    masm.bind(&end);
}

void
CodeGeneratorX86Shared::visitRound(LRound* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister temp = ToFloatRegister(lir->temp());
    FloatRegister scratch = ScratchDoubleReg;
    Register output = ToRegister(lir->output());

    Label negativeOrZero, negative, end, bailout;

    
    masm.zeroDouble(scratch);
    masm.loadConstantDouble(GetBiggestNumberLessThan(0.5), temp);
    masm.branchDouble(Assembler::DoubleLessThanOrEqual, input, scratch, &negativeOrZero);

    
    
    
    
    
    masm.addDouble(input, temp);
    bailoutCvttsd2si(temp, output, lir->snapshot());

    masm.jump(&end);

    
    masm.bind(&negativeOrZero);
    
    masm.j(Assembler::NotEqual, &negative);

    
    masm.branchNegativeZero(input, output, &bailout,  false);
    bailoutFrom(&bailout, lir->snapshot());

    
    masm.xor32(output, output);
    masm.jump(&end);

    
    masm.bind(&negative);

    
    
    Label loadJoin;
    masm.loadConstantDouble(-0.5, scratch);
    masm.branchDouble(Assembler::DoubleLessThan, input, scratch, &loadJoin);
    masm.loadConstantDouble(0.5, temp);
    masm.bind(&loadJoin);

    if (AssemblerX86Shared::HasSSE41()) {
        
        
        masm.addDouble(input, temp);
        masm.vroundsd(X86Encoding::RoundDown, temp, scratch, scratch);

        
        bailoutCvttsd2si(scratch, output, lir->snapshot());

        
        
        masm.test32(output, output);
        bailoutIf(Assembler::Zero, lir->snapshot());
    } else {
        masm.addDouble(input, temp);

        
        {
            
            masm.compareDouble(Assembler::DoubleGreaterThanOrEqual, temp, scratch);
            bailoutIf(Assembler::DoubleGreaterThanOrEqual, lir->snapshot());

            
            
            bailoutCvttsd2si(temp, output, lir->snapshot());

            
            masm.convertInt32ToDouble(output, scratch);
            masm.branchDouble(Assembler::DoubleEqualOrUnordered, temp, scratch, &end);

            
            
            masm.subl(Imm32(1), output);
            
        }
    }

    masm.bind(&end);
}

void
CodeGeneratorX86Shared::visitRoundF(LRoundF* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    FloatRegister temp = ToFloatRegister(lir->temp());
    FloatRegister scratch = ScratchFloat32Reg;
    Register output = ToRegister(lir->output());

    Label negativeOrZero, negative, end, bailout;

    
    masm.zeroFloat32(scratch);
    masm.loadConstantFloat32(GetBiggestNumberLessThan(0.5f), temp);
    masm.branchFloat(Assembler::DoubleLessThanOrEqual, input, scratch, &negativeOrZero);

    
    
    
    
    
    masm.addFloat32(input, temp);

    bailoutCvttss2si(temp, output, lir->snapshot());

    masm.jump(&end);

    
    masm.bind(&negativeOrZero);
    
    masm.j(Assembler::NotEqual, &negative);

    
    masm.branchNegativeZeroFloat32(input, output, &bailout);
    bailoutFrom(&bailout, lir->snapshot());

    
    masm.xor32(output, output);
    masm.jump(&end);

    
    masm.bind(&negative);

    
    
    Label loadJoin;
    masm.loadConstantFloat32(-0.5f, scratch);
    masm.branchFloat(Assembler::DoubleLessThan, input, scratch, &loadJoin);
    masm.loadConstantFloat32(0.5f, temp);
    masm.bind(&loadJoin);

    if (AssemblerX86Shared::HasSSE41()) {
        
        
        masm.addFloat32(input, temp);
        masm.vroundss(X86Encoding::RoundDown, temp, scratch, scratch);

        
        bailoutCvttss2si(scratch, output, lir->snapshot());

        
        
        masm.test32(output, output);
        bailoutIf(Assembler::Zero, lir->snapshot());
    } else {
        masm.addFloat32(input, temp);
        
        {
            
            masm.compareFloat(Assembler::DoubleGreaterThanOrEqual, temp, scratch);
            bailoutIf(Assembler::DoubleGreaterThanOrEqual, lir->snapshot());

            
            
            bailoutCvttss2si(temp, output, lir->snapshot());

            
            masm.convertInt32ToFloat32(output, scratch);
            masm.branchFloat(Assembler::DoubleEqualOrUnordered, temp, scratch, &end);

            
            
            masm.subl(Imm32(1), output);
            
        }
    }

    masm.bind(&end);
}

void
CodeGeneratorX86Shared::visitGuardShape(LGuardShape* guard)
{
    Register obj = ToRegister(guard->input());
    masm.cmpPtr(Operand(obj, JSObject::offsetOfShape()), ImmGCPtr(guard->mir()->shape()));

    bailoutIf(Assembler::NotEqual, guard->snapshot());
}

void
CodeGeneratorX86Shared::visitGuardObjectGroup(LGuardObjectGroup* guard)
{
    Register obj = ToRegister(guard->input());

    masm.cmpPtr(Operand(obj, JSObject::offsetOfGroup()), ImmGCPtr(guard->mir()->group()));

    Assembler::Condition cond =
        guard->mir()->bailOnEquality() ? Assembler::Equal : Assembler::NotEqual;
    bailoutIf(cond, guard->snapshot());
}

void
CodeGeneratorX86Shared::visitGuardClass(LGuardClass* guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());

    masm.loadPtr(Address(obj, JSObject::offsetOfGroup()), tmp);
    masm.cmpPtr(Operand(tmp, ObjectGroup::offsetOfClasp()), ImmPtr(guard->mir()->getClass()));
    bailoutIf(Assembler::NotEqual, guard->snapshot());
}

void
CodeGeneratorX86Shared::visitEffectiveAddress(LEffectiveAddress* ins)
{
    const MEffectiveAddress* mir = ins->mir();
    Register base = ToRegister(ins->base());
    Register index = ToRegister(ins->index());
    Register output = ToRegister(ins->output());
    masm.leal(Operand(base, index, mir->scale(), mir->displacement()), output);
}

void
CodeGeneratorX86Shared::generateInvalidateEpilogue()
{
    
    
    
    for (size_t i = 0; i < sizeof(void*); i += Assembler::NopSize())
        masm.nop();

    masm.bind(&invalidate_);

    
    invalidateEpilogueData_ = masm.pushWithPatch(ImmWord(uintptr_t(-1)));
    JitCode* thunk = gen->jitRuntime()->getInvalidationThunk();

    masm.call(thunk);

    
    
    masm.assumeUnreachable("Should have returned directly to its caller instead of here.");
}

void
CodeGeneratorX86Shared::visitNegI(LNegI* ins)
{
    Register input = ToRegister(ins->input());
    MOZ_ASSERT(input == ToRegister(ins->output()));

    masm.neg32(input);
}

void
CodeGeneratorX86Shared::visitNegD(LNegD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    MOZ_ASSERT(input == ToFloatRegister(ins->output()));

    masm.negateDouble(input);
}

void
CodeGeneratorX86Shared::visitNegF(LNegF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    MOZ_ASSERT(input == ToFloatRegister(ins->output()));

    masm.negateFloat(input);
}

void
CodeGeneratorX86Shared::visitInt32x4(LInt32x4* ins)
{
    const LDefinition* out = ins->getDef(0);
    masm.loadConstantInt32x4(ins->getValue(), ToFloatRegister(out));
}

void
CodeGeneratorX86Shared::visitFloat32x4(LFloat32x4* ins)
{
    const LDefinition* out = ins->getDef(0);
    masm.loadConstantFloat32x4(ins->getValue(), ToFloatRegister(out));
}

void
CodeGeneratorX86Shared::visitInt32x4ToFloat32x4(LInt32x4ToFloat32x4* ins)
{
    FloatRegister in = ToFloatRegister(ins->input());
    FloatRegister out = ToFloatRegister(ins->output());
    masm.convertInt32x4ToFloat32x4(in, out);
}

void
CodeGeneratorX86Shared::visitFloat32x4ToInt32x4(LFloat32x4ToInt32x4* ins)
{
    FloatRegister in = ToFloatRegister(ins->input());
    FloatRegister out = ToFloatRegister(ins->output());
    masm.convertFloat32x4ToInt32x4(in, out);
}

void
CodeGeneratorX86Shared::visitSimdValueInt32x4(LSimdValueInt32x4* ins)
{
    MOZ_ASSERT(ins->mir()->type() == MIRType_Int32x4);

    FloatRegister output = ToFloatRegister(ins->output());
    if (AssemblerX86Shared::HasSSE41()) {
        masm.vmovd(ToRegister(ins->getOperand(0)), output);
        for (size_t i = 1; i < 4; ++i) {
            Register r = ToRegister(ins->getOperand(i));
            masm.vpinsrd(i, r, output, output);
        }
        return;
    }

    masm.reserveStack(Simd128DataSize);
    for (size_t i = 0; i < 4; ++i) {
        Register r = ToRegister(ins->getOperand(i));
        masm.store32(r, Address(StackPointer, i * sizeof(int32_t)));
    }
    masm.loadAlignedInt32x4(Address(StackPointer, 0), output);
    masm.freeStack(Simd128DataSize);
}

void
CodeGeneratorX86Shared::visitSimdValueFloat32x4(LSimdValueFloat32x4* ins)
{
    MOZ_ASSERT(ins->mir()->type() == MIRType_Float32x4);

    FloatRegister r0 = ToFloatRegister(ins->getOperand(0));
    FloatRegister r1 = ToFloatRegister(ins->getOperand(1));
    FloatRegister r2 = ToFloatRegister(ins->getOperand(2));
    FloatRegister r3 = ToFloatRegister(ins->getOperand(3));
    FloatRegister tmp = ToFloatRegister(ins->getTemp(0));
    FloatRegister output = ToFloatRegister(ins->output());

    FloatRegister r0Copy = masm.reusedInputFloat32x4(r0, output);
    FloatRegister r1Copy = masm.reusedInputFloat32x4(r1, tmp);

    masm.vunpcklps(r3, r1Copy, tmp);
    masm.vunpcklps(r2, r0Copy, output);
    masm.vunpcklps(tmp, output, output);
}

void
CodeGeneratorX86Shared::visitSimdSplatX4(LSimdSplatX4* ins)
{
    FloatRegister output = ToFloatRegister(ins->output());

    MSimdSplatX4* mir = ins->mir();
    MOZ_ASSERT(IsSimdType(mir->type()));
    JS_STATIC_ASSERT(sizeof(float) == sizeof(int32_t));

    switch (mir->type()) {
      case MIRType_Int32x4: {
        Register r = ToRegister(ins->getOperand(0));
        masm.vmovd(r, output);
        masm.vpshufd(0, output, output);
        break;
      }
      case MIRType_Float32x4: {
        FloatRegister r = ToFloatRegister(ins->getOperand(0));
        FloatRegister rCopy = masm.reusedInputFloat32x4(r, output);
        masm.vshufps(0, rCopy, rCopy, output);
        break;
      }
      default:
        MOZ_CRASH("Unknown SIMD kind");
    }
}

void
CodeGeneratorX86Shared::visitSimdReinterpretCast(LSimdReinterpretCast* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    if (input.aliases(output))
        return;

    switch (ins->mir()->type()) {
      case MIRType_Int32x4:
        masm.vmovdqa(input, output);
        break;
      case MIRType_Float32x4:
        masm.vmovaps(input, output);
        break;
      default:
        MOZ_CRASH("Unknown SIMD kind");
    }
}

void
CodeGeneratorX86Shared::visitSimdExtractElementI(LSimdExtractElementI* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    Register output = ToRegister(ins->output());

    SimdLane lane = ins->lane();
    if (lane == LaneX) {
        
        masm.moveLowInt32(input, output);
    } else if (AssemblerX86Shared::HasSSE41()) {
        masm.vpextrd(lane, input, output);
    } else {
        uint32_t mask = MacroAssembler::ComputeShuffleMask(lane);
        masm.shuffleInt32(mask, input, ScratchSimdReg);
        masm.moveLowInt32(ScratchSimdReg, output);
    }
}

void
CodeGeneratorX86Shared::visitSimdExtractElementF(LSimdExtractElementF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    SimdLane lane = ins->lane();
    if (lane == LaneX) {
        
        if (input != output)
            masm.moveFloat32(input, output);
    } else if (lane == LaneZ) {
        masm.moveHighPairToLowPairFloat32(input, output);
    } else {
        uint32_t mask = MacroAssembler::ComputeShuffleMask(lane);
        masm.shuffleFloat32(mask, input, output);
    }
    
    
    
    
    if (!gen->compilingAsmJS())
        masm.canonicalizeFloat(output);
}

void
CodeGeneratorX86Shared::visitSimdInsertElementI(LSimdInsertElementI* ins)
{
    FloatRegister vector = ToFloatRegister(ins->vector());
    Register value = ToRegister(ins->value());
    FloatRegister output = ToFloatRegister(ins->output());
    MOZ_ASSERT(vector == output); 

    unsigned component = unsigned(ins->lane());

    
    
    
    if (AssemblerX86Shared::HasSSE41()) {
        
        masm.vpinsrd(component, value, vector, output);
        return;
    }

    masm.reserveStack(Simd128DataSize);
    masm.storeAlignedInt32x4(vector, Address(StackPointer, 0));
    masm.store32(value, Address(StackPointer, component * sizeof(int32_t)));
    masm.loadAlignedInt32x4(Address(StackPointer, 0), output);
    masm.freeStack(Simd128DataSize);
}

void
CodeGeneratorX86Shared::visitSimdInsertElementF(LSimdInsertElementF* ins)
{
    FloatRegister vector = ToFloatRegister(ins->vector());
    FloatRegister value = ToFloatRegister(ins->value());
    FloatRegister output = ToFloatRegister(ins->output());
    MOZ_ASSERT(vector == output); 

    if (ins->lane() == SimdLane::LaneX) {
        
        
        if (value != output)
            masm.vmovss(value, vector, output);
        return;
    }

    if (AssemblerX86Shared::HasSSE41()) {
        
        masm.vinsertps(masm.vinsertpsMask(SimdLane::LaneX, ins->lane()), value, output, output);
        return;
    }

    unsigned component = unsigned(ins->lane());
    masm.reserveStack(Simd128DataSize);
    masm.storeAlignedFloat32x4(vector, Address(StackPointer, 0));
    masm.storeFloat32(value, Address(StackPointer, component * sizeof(int32_t)));
    masm.loadAlignedFloat32x4(Address(StackPointer, 0), output);
    masm.freeStack(Simd128DataSize);
}

void
CodeGeneratorX86Shared::visitSimdSignMaskX4(LSimdSignMaskX4* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    Register output = ToRegister(ins->output());

    
    masm.vmovmskps(input, output);
}

template <class T, class Reg> void
CodeGeneratorX86Shared::visitSimdGeneralShuffle(LSimdGeneralShuffleBase* ins, Reg tempRegister)
{
    MSimdGeneralShuffle* mir = ins->mir();
    unsigned numVectors = mir->numVectors();

    Register laneTemp = ToRegister(ins->temp());

    
    
    

    
    unsigned stackSpace = Simd128DataSize * (numVectors + 1);
    masm.reserveStack(stackSpace);

    for (unsigned i = 0; i < numVectors; i++) {
        masm.storeAlignedVector<T>(ToFloatRegister(ins->vector(i)),
                                   Address(StackPointer, Simd128DataSize * (1 + i)));
    }

    Label bail;

    for (size_t i = 0; i < mir->numLanes(); i++) {
        Operand lane = ToOperand(ins->lane(i));

        masm.cmp32(lane, Imm32(numVectors * mir->numLanes() - 1));
        masm.j(Assembler::Above, &bail);

        if (lane.kind() == Operand::REG) {
            masm.loadScalar<T>(Operand(StackPointer, ToRegister(ins->lane(i)), TimesFour, Simd128DataSize),
                               tempRegister);
        } else {
            masm.load32(lane, laneTemp);
            masm.loadScalar<T>(Operand(StackPointer, laneTemp, TimesFour, Simd128DataSize), tempRegister);
        }

        masm.storeScalar<T>(tempRegister, Address(StackPointer, i * sizeof(T)));
    }

    FloatRegister output = ToFloatRegister(ins->output());
    masm.loadAlignedVector<T>(Address(StackPointer, 0), output);

    Label join;
    masm.jump(&join);

    {
        masm.bind(&bail);
        masm.freeStack(stackSpace);
        bailout(ins->snapshot());
    }

    masm.bind(&join);
    masm.setFramePushed(masm.framePushed() + stackSpace);
    masm.freeStack(stackSpace);
}

void
CodeGeneratorX86Shared::visitSimdGeneralShuffleI(LSimdGeneralShuffleI* ins)
{
    visitSimdGeneralShuffle<int32_t, Register>(ins, ToRegister(ins->temp()));
}
void
CodeGeneratorX86Shared::visitSimdGeneralShuffleF(LSimdGeneralShuffleF* ins)
{
    visitSimdGeneralShuffle<float, FloatRegister>(ins, ScratchFloat32Reg);
}

void
CodeGeneratorX86Shared::visitSimdSwizzleI(LSimdSwizzleI* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    uint32_t x = ins->laneX();
    uint32_t y = ins->laneY();
    uint32_t z = ins->laneZ();
    uint32_t w = ins->laneW();

    uint32_t mask = MacroAssembler::ComputeShuffleMask(x, y, z, w);
    masm.shuffleInt32(mask, input, output);
}

void
CodeGeneratorX86Shared::visitSimdSwizzleF(LSimdSwizzleF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    uint32_t x = ins->laneX();
    uint32_t y = ins->laneY();
    uint32_t z = ins->laneZ();
    uint32_t w = ins->laneW();

    if (AssemblerX86Shared::HasSSE3()) {
        if (ins->lanesMatch(0, 0, 2, 2)) {
            masm.vmovsldup(input, output);
            return;
        }
        if (ins->lanesMatch(1, 1, 3, 3)) {
            masm.vmovshdup(input, output);
            return;
        }
    }

    
    
    if (ins->lanesMatch(2, 3, 2, 3)) {
        FloatRegister inputCopy = masm.reusedInputFloat32x4(input, output);
        masm.vmovhlps(input, inputCopy, output);
        return;
    }

    if (ins->lanesMatch(0, 1, 0, 1)) {
        if (AssemblerX86Shared::HasSSE3() && !AssemblerX86Shared::HasAVX()) {
            masm.vmovddup(input, output);
            return;
        }
        FloatRegister inputCopy = masm.reusedInputFloat32x4(input, output);
        masm.vmovlhps(input, inputCopy, output);
        return;
    }

    if (ins->lanesMatch(0, 0, 1, 1)) {
        FloatRegister inputCopy = masm.reusedInputFloat32x4(input, output);
        masm.vunpcklps(input, inputCopy, output);
        return;
    }

    if (ins->lanesMatch(2, 2, 3, 3)) {
        FloatRegister inputCopy = masm.reusedInputFloat32x4(input, output);
        masm.vunpckhps(input, inputCopy, output);
        return;
    }

    uint32_t mask = MacroAssembler::ComputeShuffleMask(x, y, z, w);
    masm.shuffleFloat32(mask, input, output);
}

void
CodeGeneratorX86Shared::visitSimdShuffle(LSimdShuffle* ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    Operand rhs = ToOperand(ins->rhs());
    FloatRegister out = ToFloatRegister(ins->output());

    uint32_t x = ins->laneX();
    uint32_t y = ins->laneY();
    uint32_t z = ins->laneZ();
    uint32_t w = ins->laneW();

    
    unsigned numLanesFromLHS = (x < 4) + (y < 4) + (z < 4) + (w < 4);
    MOZ_ASSERT(numLanesFromLHS >= 2);

    
    
    
    
    
    
    
    

    uint32_t mask;

    
    
    MOZ_ASSERT(numLanesFromLHS < 4);

    
    if (AssemblerX86Shared::HasSSE41()) {
        if (x % 4 == 0 && y % 4 == 1 && z % 4 == 2 && w % 4 == 3) {
            masm.vblendps(masm.blendpsMask(x >= 4, y >= 4, z >= 4, w >= 4), rhs, lhs, out);
            return;
        }
    }

    
    if (numLanesFromLHS == 3) {
        unsigned firstMask = -1, secondMask = -1;

        
        if (ins->lanesMatch(4, 1, 2, 3) && rhs.kind() == Operand::FPREG) {
            masm.vmovss(FloatRegister::FromCode(rhs.fpu()), lhs, out);
            return;
        }

        
        unsigned numLanesUnchanged = (x == 0) + (y == 1) + (z == 2) + (w == 3);
        if (AssemblerX86Shared::HasSSE41() && numLanesUnchanged == 3) {
            SimdLane srcLane;
            SimdLane dstLane;
            if (x >= 4) {
                srcLane = SimdLane(x - 4);
                dstLane = LaneX;
            } else if (y >= 4) {
                srcLane = SimdLane(y - 4);
                dstLane = LaneY;
            } else if (z >= 4) {
                srcLane = SimdLane(z - 4);
                dstLane = LaneZ;
            } else {
                MOZ_ASSERT(w >= 4);
                srcLane = SimdLane(w - 4);
                dstLane = LaneW;
            }
            masm.vinsertps(masm.vinsertpsMask(srcLane, dstLane), rhs, lhs, out);
            return;
        }

        FloatRegister rhsCopy = ToFloatRegister(ins->temp());

        if (x < 4 && y < 4) {
            if (w >= 4) {
                w %= 4;
                
                firstMask = MacroAssembler::ComputeShuffleMask(w, w, z, z);
                
                secondMask = MacroAssembler::ComputeShuffleMask(x, y, LaneZ, LaneX);
            } else {
                MOZ_ASSERT(z >= 4);
                z %= 4;
                
                firstMask = MacroAssembler::ComputeShuffleMask(z, z, w, w);
                
                secondMask = MacroAssembler::ComputeShuffleMask(x, y, LaneX, LaneZ);
            }

            masm.vshufps(firstMask, lhs, rhsCopy, rhsCopy);
            masm.vshufps(secondMask, rhsCopy, lhs, out);
            return;
        }

        MOZ_ASSERT(z < 4 && w < 4);

        if (y >= 4) {
            y %= 4;
            
            firstMask = MacroAssembler::ComputeShuffleMask(y, y, x, x);
            
            secondMask = MacroAssembler::ComputeShuffleMask(LaneZ, LaneX, z, w);
        } else {
            MOZ_ASSERT(x >= 4);
            x %= 4;
            
            firstMask = MacroAssembler::ComputeShuffleMask(x, x, y, y);
            
            secondMask = MacroAssembler::ComputeShuffleMask(LaneX, LaneZ, z, w);
        }

        masm.vshufps(firstMask, lhs, rhsCopy, rhsCopy);
        if (AssemblerX86Shared::HasAVX()) {
            masm.vshufps(secondMask, lhs, rhsCopy, out);
        } else {
            masm.vshufps(secondMask, lhs, rhsCopy, rhsCopy);
            masm.moveFloat32x4(rhsCopy, out);
        }
        return;
    }

    
    MOZ_ASSERT(numLanesFromLHS == 2);

    
    
    if (ins->lanesMatch(2, 3, 6, 7)) {
        if (AssemblerX86Shared::HasAVX()) {
            FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, ScratchSimdReg);
            masm.vmovhlps(lhs, rhsCopy, out);
        } else {
            masm.loadAlignedFloat32x4(rhs, ScratchSimdReg);
            masm.vmovhlps(lhs, ScratchSimdReg, ScratchSimdReg);
            masm.moveFloat32x4(ScratchSimdReg, out);
        }
        return;
    }

    if (ins->lanesMatch(0, 1, 4, 5)) {
        FloatRegister rhsCopy;
        if (rhs.kind() == Operand::FPREG) {
            
            
            rhsCopy = FloatRegister::FromCode(rhs.fpu());
        } else {
            masm.loadAlignedFloat32x4(rhs, ScratchSimdReg);
            rhsCopy = ScratchSimdReg;
        }
        masm.vmovlhps(rhsCopy, lhs, out);
        return;
    }

    if (ins->lanesMatch(0, 4, 1, 5)) {
        masm.vunpcklps(rhs, lhs, out);
        return;
    }

    
    if (ins->lanesMatch(4, 0, 5, 1)) {
        if (AssemblerX86Shared::HasAVX()) {
            FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, ScratchSimdReg);
            masm.vunpcklps(lhs, rhsCopy, out);
        } else {
            masm.loadAlignedFloat32x4(rhs, ScratchSimdReg);
            masm.vunpcklps(lhs, ScratchSimdReg, ScratchSimdReg);
            masm.moveFloat32x4(ScratchSimdReg, out);
        }
        return;
    }

    if (ins->lanesMatch(2, 6, 3, 7)) {
        masm.vunpckhps(rhs, lhs, out);
        return;
    }

    
    if (ins->lanesMatch(6, 2, 7, 3)) {
        if (AssemblerX86Shared::HasAVX()) {
            FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, ScratchSimdReg);
            masm.vunpckhps(lhs, rhsCopy, out);
        } else {
            masm.loadAlignedFloat32x4(rhs, ScratchSimdReg);
            masm.vunpckhps(lhs, ScratchSimdReg, ScratchSimdReg);
            masm.moveFloat32x4(ScratchSimdReg, out);
        }
        return;
    }

    
    if (x < 4 && y < 4) {
        mask = MacroAssembler::ComputeShuffleMask(x, y, z % 4, w % 4);
        masm.vshufps(mask, rhs, lhs, out);
        return;
    }

    
    MOZ_ASSERT(!(z >= 4 && w >= 4));

    
    uint32_t firstMask[4], secondMask[4];
    unsigned i = 0, j = 2, k = 0;

#define COMPUTE_MASK(lane)       \
    if (lane >= 4) {             \
        firstMask[j] = lane % 4; \
        secondMask[k++] = j++;   \
    } else {                     \
        firstMask[i] = lane;     \
        secondMask[k++] = i++;   \
    }

    COMPUTE_MASK(x)
    COMPUTE_MASK(y)
    COMPUTE_MASK(z)
    COMPUTE_MASK(w)
#undef COMPUTE_MASK

    MOZ_ASSERT(i == 2 && j == 4 && k == 4);

    mask = MacroAssembler::ComputeShuffleMask(firstMask[0], firstMask[1],
                                              firstMask[2], firstMask[3]);
    masm.vshufps(mask, rhs, lhs, lhs);

    mask = MacroAssembler::ComputeShuffleMask(secondMask[0], secondMask[1],
                                              secondMask[2], secondMask[3]);
    masm.vshufps(mask, lhs, lhs, lhs);
}

void
CodeGeneratorX86Shared::visitSimdBinaryCompIx4(LSimdBinaryCompIx4* ins)
{
    static const SimdConstant allOnes = SimdConstant::SplatX4(-1);

    FloatRegister lhs = ToFloatRegister(ins->lhs());
    Operand rhs = ToOperand(ins->rhs());
    MOZ_ASSERT(ToFloatRegister(ins->output()) == lhs);

    MSimdBinaryComp::Operation op = ins->operation();
    switch (op) {
      case MSimdBinaryComp::greaterThan:
        masm.packedGreaterThanInt32x4(rhs, lhs);
        return;
      case MSimdBinaryComp::equal:
        masm.packedEqualInt32x4(rhs, lhs);
        return;
      case MSimdBinaryComp::lessThan:
        
        if (rhs.kind() == Operand::FPREG)
            masm.moveInt32x4(ToFloatRegister(ins->rhs()), ScratchSimdReg);
        else
            masm.loadAlignedInt32x4(rhs, ScratchSimdReg);

        
        
        masm.packedGreaterThanInt32x4(ToOperand(ins->lhs()), ScratchSimdReg);
        masm.moveInt32x4(ScratchSimdReg, lhs);
        return;
      case MSimdBinaryComp::notEqual:
        
        
        
        masm.loadConstantInt32x4(allOnes, ScratchSimdReg);
        masm.packedEqualInt32x4(rhs, lhs);
        masm.bitwiseXorX4(Operand(ScratchSimdReg), lhs);
        return;
      case MSimdBinaryComp::greaterThanOrEqual:
        
        if (rhs.kind() == Operand::FPREG)
            masm.moveInt32x4(ToFloatRegister(ins->rhs()), ScratchSimdReg);
        else
            masm.loadAlignedInt32x4(rhs, ScratchSimdReg);
        masm.packedGreaterThanInt32x4(ToOperand(ins->lhs()), ScratchSimdReg);
        masm.loadConstantInt32x4(allOnes, lhs);
        masm.bitwiseXorX4(Operand(ScratchSimdReg), lhs);
        return;
      case MSimdBinaryComp::lessThanOrEqual:
        
        masm.loadConstantInt32x4(allOnes, ScratchSimdReg);
        masm.packedGreaterThanInt32x4(rhs, lhs);
        masm.bitwiseXorX4(Operand(ScratchSimdReg), lhs);
        return;
    }
    MOZ_CRASH("unexpected SIMD op");
}

void
CodeGeneratorX86Shared::visitSimdBinaryCompFx4(LSimdBinaryCompFx4* ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    Operand rhs = ToOperand(ins->rhs());
    FloatRegister output = ToFloatRegister(ins->output());

    MSimdBinaryComp::Operation op = ins->operation();
    switch (op) {
      case MSimdBinaryComp::equal:
        masm.vcmpeqps(rhs, lhs, output);
        return;
      case MSimdBinaryComp::lessThan:
        masm.vcmpltps(rhs, lhs, output);
        return;
      case MSimdBinaryComp::lessThanOrEqual:
        masm.vcmpleps(rhs, lhs, output);
        return;
      case MSimdBinaryComp::notEqual:
        masm.vcmpneqps(rhs, lhs, output);
        return;
      case MSimdBinaryComp::greaterThanOrEqual:
      case MSimdBinaryComp::greaterThan:
        
        
        MOZ_CRASH("lowering should have reversed this");
    }
    MOZ_CRASH("unexpected SIMD op");
}

void
CodeGeneratorX86Shared::visitSimdBinaryArithIx4(LSimdBinaryArithIx4* ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    Operand rhs = ToOperand(ins->rhs());
    FloatRegister output = ToFloatRegister(ins->output());

    MSimdBinaryArith::Operation op = ins->operation();
    switch (op) {
      case MSimdBinaryArith::Op_add:
        masm.vpaddd(rhs, lhs, output);
        return;
      case MSimdBinaryArith::Op_sub:
        masm.vpsubd(rhs, lhs, output);
        return;
      case MSimdBinaryArith::Op_mul: {
        if (AssemblerX86Shared::HasSSE41()) {
            masm.vpmulld(rhs, lhs, output);
            return;
        }

        masm.loadAlignedInt32x4(rhs, ScratchSimdReg);
        masm.vpmuludq(lhs, ScratchSimdReg, ScratchSimdReg);
        

        FloatRegister temp = ToFloatRegister(ins->temp());
        masm.vpshufd(MacroAssembler::ComputeShuffleMask(LaneY, LaneY, LaneW, LaneW), lhs, lhs);
        masm.vpshufd(MacroAssembler::ComputeShuffleMask(LaneY, LaneY, LaneW, LaneW), rhs, temp);
        masm.vpmuludq(temp, lhs, lhs);
        

        masm.vshufps(MacroAssembler::ComputeShuffleMask(LaneX, LaneZ, LaneX, LaneZ), ScratchSimdReg, lhs, lhs);
        
        masm.vshufps(MacroAssembler::ComputeShuffleMask(LaneZ, LaneX, LaneW, LaneY), lhs, lhs, lhs);
        return;
      }
      case MSimdBinaryArith::Op_div:
        
        break;
      case MSimdBinaryArith::Op_max:
        
        
        break;
      case MSimdBinaryArith::Op_min:
        
        
        break;
      case MSimdBinaryArith::Op_minNum:
      case MSimdBinaryArith::Op_maxNum:
        break;
    }
    MOZ_CRASH("unexpected SIMD op");
}

void
CodeGeneratorX86Shared::visitSimdBinaryArithFx4(LSimdBinaryArithFx4* ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    Operand rhs = ToOperand(ins->rhs());
    FloatRegister output = ToFloatRegister(ins->output());

    MSimdBinaryArith::Operation op = ins->operation();
    switch (op) {
      case MSimdBinaryArith::Op_add:
        masm.vaddps(rhs, lhs, output);
        return;
      case MSimdBinaryArith::Op_sub:
        masm.vsubps(rhs, lhs, output);
        return;
      case MSimdBinaryArith::Op_mul:
        masm.vmulps(rhs, lhs, output);
        return;
      case MSimdBinaryArith::Op_div:
        masm.vdivps(rhs, lhs, output);
        return;
      case MSimdBinaryArith::Op_max: {
        FloatRegister lhsCopy = masm.reusedInputFloat32x4(lhs, ScratchSimdReg);
        masm.vcmpunordps(rhs, lhsCopy, ScratchSimdReg);

        FloatRegister tmp = ToFloatRegister(ins->temp());
        FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, tmp);
        masm.vmaxps(Operand(lhs), rhsCopy, tmp);
        masm.vmaxps(rhs, lhs, output);

        masm.vandps(tmp, output, output);
        masm.vorps(ScratchSimdReg, output, output); 
        return;
      }
      case MSimdBinaryArith::Op_min: {
        FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, ScratchSimdReg);
        masm.vminps(Operand(lhs), rhsCopy, ScratchSimdReg);
        masm.vminps(rhs, lhs, output);
        masm.vorps(ScratchSimdReg, output, output); 
        return;
      }
      case MSimdBinaryArith::Op_minNum: {
        FloatRegister tmp = ToFloatRegister(ins->temp());
        masm.loadConstantInt32x4(SimdConstant::SplatX4(int32_t(0x80000000)), tmp);

        FloatRegister mask = ScratchSimdReg;
        FloatRegister tmpCopy = masm.reusedInputFloat32x4(tmp, ScratchSimdReg);
        masm.vpcmpeqd(Operand(lhs), tmpCopy, mask);
        masm.vandps(tmp, mask, mask);

        FloatRegister lhsCopy = masm.reusedInputFloat32x4(lhs, tmp);
        masm.vminps(rhs, lhsCopy, tmp);
        masm.vorps(mask, tmp, tmp);

        FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, mask);
        masm.vcmpneqps(rhs, rhsCopy, mask);

        if (AssemblerX86Shared::HasAVX()) {
            masm.vblendvps(mask, lhs, tmp, output);
        } else {
            
            
            
            if (lhs != output)
                masm.moveFloat32x4(lhs, output);
            masm.vandps(Operand(mask), output, output);
            masm.vandnps(Operand(tmp), mask, mask);
            masm.vorps(Operand(mask), output, output);
        }
        return;
      }
      case MSimdBinaryArith::Op_maxNum: {
        FloatRegister mask = ScratchSimdReg;
        masm.loadConstantInt32x4(SimdConstant::SplatX4(0), mask);
        masm.vpcmpeqd(Operand(lhs), mask, mask);

        FloatRegister tmp = ToFloatRegister(ins->temp());
        masm.loadConstantInt32x4(SimdConstant::SplatX4(int32_t(0x80000000)), tmp);
        masm.vandps(tmp, mask, mask);

        FloatRegister lhsCopy = masm.reusedInputFloat32x4(lhs, tmp);
        masm.vmaxps(rhs, lhsCopy, tmp);
        masm.vandnps(Operand(tmp), mask, mask);

        
        mask = tmp;
        tmp = ScratchSimdReg;

        FloatRegister rhsCopy = masm.reusedInputAlignedFloat32x4(rhs, mask);
        masm.vcmpneqps(rhs, rhsCopy, mask);

        if (AssemblerX86Shared::HasAVX()) {
            masm.vblendvps(mask, lhs, tmp, output);
        } else {
            
            
            
            if (lhs != output)
                masm.moveFloat32x4(lhs, output);
            masm.vandps(Operand(mask), output, output);
            masm.vandnps(Operand(tmp), mask, mask);
            masm.vorps(Operand(mask), output, output);
        }
        return;
      }
    }
    MOZ_CRASH("unexpected SIMD op");
}

void
CodeGeneratorX86Shared::visitSimdUnaryArithIx4(LSimdUnaryArithIx4* ins)
{
    Operand in = ToOperand(ins->input());
    FloatRegister out = ToFloatRegister(ins->output());

    static const SimdConstant allOnes = SimdConstant::CreateX4(-1, -1, -1, -1);

    switch (ins->operation()) {
      case MSimdUnaryArith::neg:
        masm.zeroInt32x4(out);
        masm.packedSubInt32(in, out);
        return;
      case MSimdUnaryArith::not_:
        masm.loadConstantInt32x4(allOnes, out);
        masm.bitwiseXorX4(in, out);
        return;
      case MSimdUnaryArith::abs:
      case MSimdUnaryArith::reciprocalApproximation:
      case MSimdUnaryArith::reciprocalSqrtApproximation:
      case MSimdUnaryArith::sqrt:
        break;
    }
    MOZ_CRASH("unexpected SIMD op");
}

void
CodeGeneratorX86Shared::visitSimdUnaryArithFx4(LSimdUnaryArithFx4* ins)
{
    Operand in = ToOperand(ins->input());
    FloatRegister out = ToFloatRegister(ins->output());

    
    float signMask = SpecificNaN<float>(0, FloatingPoint<float>::kSignificandBits);
    static const SimdConstant signMasks = SimdConstant::SplatX4(signMask);

    
    float ones = SpecificNaN<float>(1, FloatingPoint<float>::kSignificandBits);
    static const SimdConstant allOnes = SimdConstant::SplatX4(ones);

    
    static const SimdConstant minusZero = SimdConstant::SplatX4(-0.f);

    switch (ins->operation()) {
      case MSimdUnaryArith::abs:
        masm.loadConstantFloat32x4(signMasks, out);
        masm.bitwiseAndX4(in, out);
        return;
      case MSimdUnaryArith::neg:
        masm.loadConstantFloat32x4(minusZero, out);
        masm.bitwiseXorX4(in, out);
        return;
      case MSimdUnaryArith::not_:
        masm.loadConstantFloat32x4(allOnes, out);
        masm.bitwiseXorX4(in, out);
        return;
      case MSimdUnaryArith::reciprocalApproximation:
        masm.packedRcpApproximationFloat32x4(in, out);
        return;
      case MSimdUnaryArith::reciprocalSqrtApproximation:
        masm.packedRcpSqrtApproximationFloat32x4(in, out);
        return;
      case MSimdUnaryArith::sqrt:
        masm.packedSqrtFloat32x4(in, out);
        return;
    }
    MOZ_CRASH("unexpected SIMD op");
}

void
CodeGeneratorX86Shared::visitSimdBinaryBitwiseX4(LSimdBinaryBitwiseX4* ins)
{
    FloatRegister lhs = ToFloatRegister(ins->lhs());
    Operand rhs = ToOperand(ins->rhs());
    FloatRegister output = ToFloatRegister(ins->output());

    MSimdBinaryBitwise::Operation op = ins->operation();
    switch (op) {
      case MSimdBinaryBitwise::and_:
        if (ins->type() == MIRType_Float32x4)
            masm.vandps(rhs, lhs, output);
        else
            masm.vpand(rhs, lhs, output);
        return;
      case MSimdBinaryBitwise::or_:
        if (ins->type() == MIRType_Float32x4)
            masm.vorps(rhs, lhs, output);
        else
            masm.vpor(rhs, lhs, output);
        return;
      case MSimdBinaryBitwise::xor_:
        if (ins->type() == MIRType_Float32x4)
            masm.vxorps(rhs, lhs, output);
        else
            masm.vpxor(rhs, lhs, output);
        return;
    }
    MOZ_CRASH("unexpected SIMD bitwise op");
}

void
CodeGeneratorX86Shared::visitSimdShift(LSimdShift* ins)
{
    FloatRegister out = ToFloatRegister(ins->output());
    MOZ_ASSERT(ToFloatRegister(ins->vector()) == out); 

    
    
    
    const LAllocation* val = ins->value();
    if (val->isConstant()) {
        uint32_t c = uint32_t(ToInt32(val));
        if (c > 31) {
            switch (ins->operation()) {
              case MSimdShift::lsh:
              case MSimdShift::ursh:
                masm.zeroInt32x4(out);
                return;
              default:
                c = 31;
                break;
            }
        }
        Imm32 count(c);
        switch (ins->operation()) {
          case MSimdShift::lsh:
            masm.packedLeftShiftByScalar(count, out);
            return;
          case MSimdShift::rsh:
            masm.packedRightShiftByScalar(count, out);
            return;
          case MSimdShift::ursh:
            masm.packedUnsignedRightShiftByScalar(count, out);
            return;
        }
        MOZ_CRASH("unexpected SIMD bitwise op");
    }

    MOZ_ASSERT(val->isRegister());
    FloatRegister tmp = ScratchFloat32Reg;
    masm.vmovd(ToRegister(val), tmp);

    switch (ins->operation()) {
      case MSimdShift::lsh:
        masm.packedLeftShiftByScalar(tmp, out);
        return;
      case MSimdShift::rsh:
        masm.packedRightShiftByScalar(tmp, out);
        return;
      case MSimdShift::ursh:
        masm.packedUnsignedRightShiftByScalar(tmp, out);
        return;
    }
    MOZ_CRASH("unexpected SIMD bitwise op");
}

void
CodeGeneratorX86Shared::visitSimdSelect(LSimdSelect* ins)
{
    FloatRegister mask = ToFloatRegister(ins->mask());
    FloatRegister onTrue = ToFloatRegister(ins->lhs());
    FloatRegister onFalse = ToFloatRegister(ins->rhs());
    FloatRegister output = ToFloatRegister(ins->output());
    FloatRegister temp = ToFloatRegister(ins->temp());

    if (onTrue != output)
        masm.vmovaps(onTrue, output);
    if (mask != temp)
        masm.vmovaps(mask, temp);

    MSimdSelect* mir = ins->mir();
    if (mir->isElementWise()) {
        if (AssemblerX86Shared::HasAVX()) {
            masm.vblendvps(mask, onTrue, onFalse, output);
            return;
        }

        
        

        
        if (!mir->mask()->isSimdBinaryComp())
            masm.packedRightShiftByScalar(Imm32(31), temp);
    }

    masm.bitwiseAndX4(Operand(temp), output);
    masm.bitwiseAndNotX4(Operand(onFalse), temp);
    masm.bitwiseOrX4(Operand(temp), output);
}

void
CodeGeneratorX86Shared::visitMemoryBarrier(LMemoryBarrier* ins)
{
    if (ins->type() & MembarStoreLoad)
        masm.storeLoadFence();
}

} 
} 
