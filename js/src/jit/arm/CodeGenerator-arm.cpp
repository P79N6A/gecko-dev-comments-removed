





#include "jit/arm/CodeGenerator-arm.h"

#include "mozilla/MathAlgorithms.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsnum.h"

#include "jit/CodeGenerator.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"
#include "js/Conversions.h"
#include "vm/Shape.h"
#include "vm/TraceLogging.h"

#include "jsscriptinlines.h"

#include "jit/shared/CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::FloorLog2;
using mozilla::NegativeInfinity;
using JS::GenericNaN;
using JS::ToInt32;


CodeGeneratorARM::CodeGeneratorARM(MIRGenerator* gen, LIRGraph* graph, MacroAssembler* masm)
  : CodeGeneratorShared(gen, graph, masm)
{
}

bool
CodeGeneratorARM::generatePrologue()
{
    MOZ_ASSERT(masm.framePushed() == 0);
    MOZ_ASSERT(!gen->compilingAsmJS());
#ifdef JS_USE_LINK_REGISTER
    masm.pushReturnAddress();
#endif

    
    if (isProfilerInstrumentationEnabled())
        masm.profilerEnterFrame(StackPointer, CallTempReg0);

    
    masm.assertStackAlignment(JitStackAlignment, 0);

    
    masm.reserveStack(frameSize());
    masm.checkStackAlignment();

    emitTracelogIonStart();

    return true;
}

bool
CodeGeneratorARM::generateEpilogue()
{
    MOZ_ASSERT(!gen->compilingAsmJS());
    masm.bind(&returnLabel_);

    emitTracelogIonStop();

    masm.freeStack(frameSize());
    MOZ_ASSERT(masm.framePushed() == 0);

    
    
    if (isProfilerInstrumentationEnabled())
        masm.profilerExitFrame();

    masm.pop(pc);
    masm.flushBuffer();
    return true;
}

void
CodeGeneratorARM::emitBranch(Assembler::Condition cond, MBasicBlock* mirTrue, MBasicBlock* mirFalse)
{
    if (isNextBlock(mirFalse->lir())) {
        jumpToBlock(mirTrue, cond);
    } else {
        jumpToBlock(mirFalse, Assembler::InvertCondition(cond));
        jumpToBlock(mirTrue);
    }
}

void
OutOfLineBailout::accept(CodeGeneratorARM* codegen)
{
    codegen->visitOutOfLineBailout(this);
}

void
CodeGeneratorARM::visitTestIAndBranch(LTestIAndBranch* test)
{
    const LAllocation* opd = test->getOperand(0);
    MBasicBlock* ifTrue = test->ifTrue();
    MBasicBlock* ifFalse = test->ifFalse();

    
    masm.ma_cmp(ToRegister(opd), Imm32(0));

    if (isNextBlock(ifFalse->lir())) {
        jumpToBlock(ifTrue, Assembler::NonZero);
    } else if (isNextBlock(ifTrue->lir())) {
        jumpToBlock(ifFalse, Assembler::Zero);
    } else {
        jumpToBlock(ifFalse, Assembler::Zero);
        jumpToBlock(ifTrue);
    }
}

void
CodeGeneratorARM::visitCompare(LCompare* comp)
{
    Assembler::Condition cond = JSOpToCondition(comp->mir()->compareType(), comp->jsop());
    const LAllocation* left = comp->getOperand(0);
    const LAllocation* right = comp->getOperand(1);
    const LDefinition* def = comp->getDef(0);

    if (right->isConstant())
        masm.ma_cmp(ToRegister(left), Imm32(ToInt32(right)));
    else
        masm.ma_cmp(ToRegister(left), ToOperand(right));
    masm.ma_mov(Imm32(0), ToRegister(def));
    masm.ma_mov(Imm32(1), ToRegister(def), NoSetCond, cond);
}

void
CodeGeneratorARM::visitCompareAndBranch(LCompareAndBranch* comp)
{
    Assembler::Condition cond = JSOpToCondition(comp->cmpMir()->compareType(), comp->jsop());
    if (comp->right()->isConstant())
        masm.ma_cmp(ToRegister(comp->left()), Imm32(ToInt32(comp->right())));
    else
        masm.ma_cmp(ToRegister(comp->left()), ToOperand(comp->right()));
    emitBranch(cond, comp->ifTrue(), comp->ifFalse());
}

bool
CodeGeneratorARM::generateOutOfLineCode()
{
    if (!CodeGeneratorShared::generateOutOfLineCode())
        return false;

    if (deoptLabel_.used()) {
        
        masm.bind(&deoptLabel_);

        
        masm.ma_mov(Imm32(frameSize()), lr);

        JitCode* handler = gen->jitRuntime()->getGenericBailoutHandler();
        masm.branch(handler);
    }

    return true;
}

void
CodeGeneratorARM::bailoutIf(Assembler::Condition condition, LSnapshot* snapshot)
{
    encode(snapshot);

    
    
    
    MOZ_ASSERT_IF(frameClass_ != FrameSizeClass::None(),
                  frameClass_.frameSize() == masm.framePushed());

    if (assignBailoutId(snapshot)) {
        uint8_t* code = Assembler::BailoutTableStart(deoptTable_->raw()) + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE;
        masm.ma_b(code, Relocation::HARDCODED, condition);
        return;
    }

    
    
    
    InlineScriptTree* tree = snapshot->mir()->block()->trackedTree();
    OutOfLineBailout* ool = new(alloc()) OutOfLineBailout(snapshot, masm.framePushed());

    
    
    addOutOfLineCode(ool, new(alloc()) BytecodeSite(tree, tree->script()->code()));

    masm.ma_b(ool->entry(), condition);
}

void
CodeGeneratorARM::bailoutFrom(Label* label, LSnapshot* snapshot)
{
    if (masm.bailed())
        return;

    MOZ_ASSERT(label->used());
    MOZ_ASSERT(!label->bound());

    encode(snapshot);

    
    
    
    MOZ_ASSERT_IF(frameClass_ != FrameSizeClass::None(),
                  frameClass_.frameSize() == masm.framePushed());

    
    InlineScriptTree* tree = snapshot->mir()->block()->trackedTree();
    OutOfLineBailout* ool = new(alloc()) OutOfLineBailout(snapshot, masm.framePushed());

    
    
    addOutOfLineCode(ool, new(alloc()) BytecodeSite(tree, tree->script()->code()));

    masm.retarget(label, ool->entry());
}

void
CodeGeneratorARM::bailout(LSnapshot* snapshot)
{
    Label label;
    masm.ma_b(&label);
    bailoutFrom(&label, snapshot);
}

void
CodeGeneratorARM::visitOutOfLineBailout(OutOfLineBailout* ool)
{
    masm.ma_mov(Imm32(ool->snapshot()->snapshotOffset()), ScratchRegister);
    masm.ma_push(ScratchRegister); 
    masm.ma_push(ScratchRegister); 
    masm.ma_b(&deoptLabel_);
}

void
CodeGeneratorARM::visitMinMaxD(LMinMaxD* ins)
{
    FloatRegister first = ToFloatRegister(ins->first());
    FloatRegister second = ToFloatRegister(ins->second());
    FloatRegister output = ToFloatRegister(ins->output());

    MOZ_ASSERT(first == output);

    Assembler::Condition cond = ins->mir()->isMax()
        ? Assembler::VFP_LessThanOrEqual
        : Assembler::VFP_GreaterThanOrEqual;
    Label nan, equal, returnSecond, done;

    masm.compareDouble(first, second);
    
    masm.ma_b(&nan, Assembler::VFP_Unordered);
    
    masm.ma_b(&equal, Assembler::VFP_Equal);
    masm.ma_b(&returnSecond, cond);
    masm.ma_b(&done);

    
    masm.bind(&equal);
    masm.compareDouble(first, NoVFPRegister);
    
    masm.ma_b(&done, Assembler::VFP_NotEqualOrUnordered);
    
    if (ins->mir()->isMax()) {
        
        masm.ma_vadd(second, first, first);
    } else {
        masm.ma_vneg(first, first);
        masm.ma_vsub(first, second, first);
        masm.ma_vneg(first, first);
    }
    masm.ma_b(&done);

    masm.bind(&nan);
    masm.loadConstantDouble(GenericNaN(), output);
    masm.ma_b(&done);

    masm.bind(&returnSecond);
    masm.ma_vmov(second, output);

    masm.bind(&done);
}

void
CodeGeneratorARM::visitMinMaxF(LMinMaxF* ins)
{
    FloatRegister first = ToFloatRegister(ins->first());
    FloatRegister second = ToFloatRegister(ins->second());
    FloatRegister output = ToFloatRegister(ins->output());

    MOZ_ASSERT(first == output);

    Assembler::Condition cond = ins->mir()->isMax()
        ? Assembler::VFP_LessThanOrEqual
        : Assembler::VFP_GreaterThanOrEqual;
    Label nan, equal, returnSecond, done;

    masm.compareFloat(first, second);
    
    masm.ma_b(&nan, Assembler::VFP_Unordered);
    
    masm.ma_b(&equal, Assembler::VFP_Equal);
    masm.ma_b(&returnSecond, cond);
    masm.ma_b(&done);

    
    masm.bind(&equal);
    masm.compareFloat(first, NoVFPRegister);
    
    masm.ma_b(&done, Assembler::VFP_NotEqualOrUnordered);
    
    if (ins->mir()->isMax()) {
        
        masm.ma_vadd_f32(second, first, first);
    } else {
        masm.ma_vneg_f32(first, first);
        masm.ma_vsub_f32(first, second, first);
        masm.ma_vneg_f32(first, first);
    }
    masm.ma_b(&done);

    masm.bind(&nan);
    masm.loadConstantFloat32(GenericNaN(), output);
    masm.ma_b(&done);

    masm.bind(&returnSecond);
    masm.ma_vmov_f32(second, output);

    masm.bind(&done);
}

void
CodeGeneratorARM::visitAbsD(LAbsD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    MOZ_ASSERT(input == ToFloatRegister(ins->output()));
    masm.ma_vabs(input, input);
}

void
CodeGeneratorARM::visitAbsF(LAbsF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    MOZ_ASSERT(input == ToFloatRegister(ins->output()));
    masm.ma_vabs_f32(input, input);
}

void
CodeGeneratorARM::visitSqrtD(LSqrtD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());
    masm.ma_vsqrt(input, output);
}

void
CodeGeneratorARM::visitSqrtF(LSqrtF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());
    masm.ma_vsqrt_f32(input, output);
}

void
CodeGeneratorARM::visitAddI(LAddI* ins)
{
    const LAllocation* lhs = ins->getOperand(0);
    const LAllocation* rhs = ins->getOperand(1);
    const LDefinition* dest = ins->getDef(0);

    if (rhs->isConstant())
        masm.ma_add(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest), SetCond);
    else
        masm.ma_add(ToRegister(lhs), ToOperand(rhs), ToRegister(dest), SetCond);

    if (ins->snapshot())
        bailoutIf(Assembler::Overflow, ins->snapshot());
}

void
CodeGeneratorARM::visitSubI(LSubI* ins)
{
    const LAllocation* lhs = ins->getOperand(0);
    const LAllocation* rhs = ins->getOperand(1);
    const LDefinition* dest = ins->getDef(0);

    if (rhs->isConstant())
        masm.ma_sub(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest), SetCond);
    else
        masm.ma_sub(ToRegister(lhs), ToOperand(rhs), ToRegister(dest), SetCond);

    if (ins->snapshot())
        bailoutIf(Assembler::Overflow, ins->snapshot());
}

void
CodeGeneratorARM::visitMulI(LMulI* ins)
{
    const LAllocation* lhs = ins->getOperand(0);
    const LAllocation* rhs = ins->getOperand(1);
    const LDefinition* dest = ins->getDef(0);
    MMul* mul = ins->mir();
    MOZ_ASSERT_IF(mul->mode() == MMul::Integer, !mul->canBeNegativeZero() && !mul->canOverflow());

    if (rhs->isConstant()) {
        
        Assembler::Condition c = Assembler::Overflow;
        
        int32_t constant = ToInt32(rhs);
        if (mul->canBeNegativeZero() && constant <= 0) {
            Assembler::Condition bailoutCond = (constant == 0) ? Assembler::LessThan : Assembler::Equal;
            masm.ma_cmp(ToRegister(lhs), Imm32(0));
            bailoutIf(bailoutCond, ins->snapshot());
        }
        
        switch (constant) {
          case -1:
            masm.ma_rsb(ToRegister(lhs), Imm32(0), ToRegister(dest), SetCond);
            break;
          case 0:
            masm.ma_mov(Imm32(0), ToRegister(dest));
            return; 
          case 1:
            
            masm.ma_mov(ToRegister(lhs), ToRegister(dest));
            return; 
          case 2:
            masm.ma_add(ToRegister(lhs), ToRegister(lhs), ToRegister(dest), SetCond);
            
            break;
          default: {
            bool handled = false;
            if (constant > 0) {
                
                if (!mul->canOverflow()) {
                    
                    Register src = ToRegister(lhs);
                    uint32_t shift = FloorLog2(constant);
                    uint32_t rest = constant - (1 << shift);
                    
                    
                    if ((1 << shift) == constant) {
                        masm.ma_lsl(Imm32(shift), src, ToRegister(dest));
                        handled = true;
                    } else {
                        
                        
                        
                        uint32_t shift_rest = FloorLog2(rest);
                        if ((1u << shift_rest) == rest) {
                            masm.as_add(ToRegister(dest), src, lsl(src, shift-shift_rest));
                            if (shift_rest != 0)
                                masm.ma_lsl(Imm32(shift_rest), ToRegister(dest), ToRegister(dest));
                            handled = true;
                        }
                    }
                } else if (ToRegister(lhs) != ToRegister(dest)) {
                    
                    

                    uint32_t shift = FloorLog2(constant);
                    if ((1 << shift) == constant) {
                        
                        masm.ma_lsl(Imm32(shift), ToRegister(lhs), ToRegister(dest));
                        
                        
                        
                        masm.as_cmp(ToRegister(lhs), asr(ToRegister(dest), shift));
                        c = Assembler::NotEqual;
                        handled = true;
                    }
                }
            }

            if (!handled) {
                if (mul->canOverflow())
                    c = masm.ma_check_mul(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest), c);
                else
                    masm.ma_mul(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest));
            }
          }
        }
        
        if (mul->canOverflow())
            bailoutIf(c, ins->snapshot());
    } else {
        Assembler::Condition c = Assembler::Overflow;

        
        if (mul->canOverflow())
            c = masm.ma_check_mul(ToRegister(lhs), ToRegister(rhs), ToRegister(dest), c);
        else
            masm.ma_mul(ToRegister(lhs), ToRegister(rhs), ToRegister(dest));

        
        if (mul->canOverflow())
            bailoutIf(c, ins->snapshot());

        if (mul->canBeNegativeZero()) {
            Label done;
            masm.ma_cmp(ToRegister(dest), Imm32(0));
            masm.ma_b(&done, Assembler::NotEqual);

            
            masm.ma_cmn(ToRegister(lhs), ToRegister(rhs));
            bailoutIf(Assembler::Signed, ins->snapshot());

            masm.bind(&done);
        }
    }
}

void
CodeGeneratorARM::divICommon(MDiv* mir, Register lhs, Register rhs, Register output,
                             LSnapshot* snapshot, Label& done)
{
    if (mir->canBeNegativeOverflow()) {
        
        

        
        masm.ma_cmp(lhs, Imm32(INT32_MIN));
        
        masm.ma_cmp(rhs, Imm32(-1), Assembler::Equal);
        if (mir->canTruncateOverflow()) {
            
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            masm.ma_mov(Imm32(INT32_MIN), output);
            masm.ma_b(&done);
            masm.bind(&skip);
        } else {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Equal, snapshot);
        }
    }

    
    if (mir->canBeDivideByZero()) {
        masm.ma_cmp(rhs, Imm32(0));
        if (mir->canTruncateInfinities()) {
            
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            masm.ma_mov(Imm32(0), output);
            masm.ma_b(&done);
            masm.bind(&skip);
        } else {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Equal, snapshot);
        }
    }

    
    if (!mir->canTruncateNegativeZero() && mir->canBeNegativeZero()) {
        Label nonzero;
        masm.ma_cmp(lhs, Imm32(0));
        masm.ma_b(&nonzero, Assembler::NotEqual);
        masm.ma_cmp(rhs, Imm32(0));
        MOZ_ASSERT(mir->fallible());
        bailoutIf(Assembler::LessThan, snapshot);
        masm.bind(&nonzero);
    }
}

void
CodeGeneratorARM::visitDivI(LDivI* ins)
{
    
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register temp = ToRegister(ins->getTemp(0));
    Register output = ToRegister(ins->output());
    MDiv* mir = ins->mir();

    Label done;
    divICommon(mir, lhs, rhs, output, ins->snapshot(), done);

    if (mir->canTruncateRemainder()) {
        masm.ma_sdiv(lhs, rhs, output);
    } else {
        masm.ma_sdiv(lhs, rhs, ScratchRegister);
        masm.ma_mul(ScratchRegister, rhs, temp);
        masm.ma_cmp(lhs, temp);
        bailoutIf(Assembler::NotEqual, ins->snapshot());
        masm.ma_mov(ScratchRegister, output);
    }

    masm.bind(&done);
}

extern "C" {
    extern MOZ_EXPORT int64_t __aeabi_idivmod(int,int);
    extern MOZ_EXPORT int64_t __aeabi_uidivmod(int,int);
}

void
CodeGeneratorARM::visitSoftDivI(LSoftDivI* ins)
{
    
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());
    MDiv* mir = ins->mir();

    Label done;
    divICommon(mir, lhs, rhs, output, ins->snapshot(), done);

    masm.setupAlignedABICall(2);
    masm.passABIArg(lhs);
    masm.passABIArg(rhs);
    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_aeabi_idivmod);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, __aeabi_idivmod));

    
    if (!mir->canTruncateRemainder()) {
        MOZ_ASSERT(mir->fallible());
        masm.ma_cmp(r1, Imm32(0));
        bailoutIf(Assembler::NonZero, ins->snapshot());
    }

    masm.bind(&done);
}

void
CodeGeneratorARM::visitDivPowTwoI(LDivPowTwoI* ins)
{
    Register lhs = ToRegister(ins->numerator());
    Register output = ToRegister(ins->output());
    int32_t shift = ins->shift();

    if (shift != 0) {
        MDiv* mir = ins->mir();
        if (!mir->isTruncated()) {
            
            masm.as_mov(ScratchRegister, lsl(lhs, 32 - shift), SetCond);
            bailoutIf(Assembler::NonZero, ins->snapshot());
        }

        if (!mir->canBeNegativeDividend()) {
            
            masm.as_mov(output, asr(lhs, shift));
            return;
        }

        
        
        
        if (shift > 1) {
            masm.as_mov(ScratchRegister, asr(lhs, 31));
            masm.as_add(ScratchRegister, lhs, lsr(ScratchRegister, 32 - shift));
        } else
            masm.as_add(ScratchRegister, lhs, lsr(lhs, 32 - shift));

        
        masm.as_mov(output, asr(ScratchRegister, shift));
    } else {
        masm.ma_mov(lhs, output);
    }
}

void
CodeGeneratorARM::modICommon(MMod* mir, Register lhs, Register rhs, Register output,
                             LSnapshot* snapshot, Label& done)
{
    
    
    
    

    
    
    
    
    
    
    
    
    
    if (mir->canBeDivideByZero() || mir->canBeNegativeDividend()) {
        masm.ma_cmp(rhs, Imm32(0));
        masm.ma_cmp(lhs, Imm32(0), Assembler::LessThan);
        if (mir->isTruncated()) {
            
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            masm.ma_mov(Imm32(0), output);
            masm.ma_b(&done);
            masm.bind(&skip);
        } else {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Equal, snapshot);
        }
    }
}

void
CodeGeneratorARM::visitModI(LModI* ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());
    Register callTemp = ToRegister(ins->callTemp());
    MMod* mir = ins->mir();

    
    masm.ma_mov(lhs, callTemp);

    Label done;
    modICommon(mir, lhs, rhs, output, ins->snapshot(), done);

    masm.ma_smod(lhs, rhs, output);

    
    if (mir->canBeNegativeDividend()) {
        if (mir->isTruncated()) {
            
        } else {
            MOZ_ASSERT(mir->fallible());
            
            masm.ma_cmp(output, Imm32(0));
            masm.ma_b(&done, Assembler::NotEqual);
            masm.ma_cmp(callTemp, Imm32(0));
            bailoutIf(Assembler::Signed, ins->snapshot());
        }
    }

    masm.bind(&done);
}

void
CodeGeneratorARM::visitSoftModI(LSoftModI* ins)
{
    
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());
    Register callTemp = ToRegister(ins->callTemp());
    MMod* mir = ins->mir();
    Label done;

    
    MOZ_ASSERT(callTemp.code() > r3.code() && callTemp.code() < r12.code());
    masm.ma_mov(lhs, callTemp);

    
    
    if (mir->canBeNegativeDividend()) {
        
        masm.ma_cmp(lhs, Imm32(INT_MIN));
        
        masm.ma_cmp(rhs, Imm32(-1), Assembler::Equal);
        if (mir->isTruncated()) {
            
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            masm.ma_mov(Imm32(0), output);
            masm.ma_b(&done);
            masm.bind(&skip);
        } else {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Equal, ins->snapshot());
        }
    }

    modICommon(mir, lhs, rhs, output, ins->snapshot(), done);

    masm.setupAlignedABICall(2);
    masm.passABIArg(lhs);
    masm.passABIArg(rhs);
    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_aeabi_idivmod);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, __aeabi_idivmod));

    
    if (mir->canBeNegativeDividend()) {
        if (mir->isTruncated()) {
            
        } else {
            MOZ_ASSERT(mir->fallible());
            
            masm.ma_cmp(r1, Imm32(0));
            masm.ma_b(&done, Assembler::NotEqual);
            masm.ma_cmp(callTemp, Imm32(0));
            bailoutIf(Assembler::Signed, ins->snapshot());
        }
    }
    masm.bind(&done);
}

void
CodeGeneratorARM::visitModPowTwoI(LModPowTwoI* ins)
{
    Register in = ToRegister(ins->getOperand(0));
    Register out = ToRegister(ins->getDef(0));
    MMod* mir = ins->mir();
    Label fin;
    
    
    masm.ma_mov(in, out, SetCond);
    masm.ma_b(&fin, Assembler::Zero);
    masm.ma_rsb(Imm32(0), out, NoSetCond, Assembler::Signed);
    masm.ma_and(Imm32((1 << ins->shift()) - 1), out);
    masm.ma_rsb(Imm32(0), out, SetCond, Assembler::Signed);
    if (mir->canBeNegativeDividend()) {
        if (!mir->isTruncated()) {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Zero, ins->snapshot());
        } else {
            
        }
    }
    masm.bind(&fin);
}

void
CodeGeneratorARM::visitModMaskI(LModMaskI* ins)
{
    Register src = ToRegister(ins->getOperand(0));
    Register dest = ToRegister(ins->getDef(0));
    Register tmp1 = ToRegister(ins->getTemp(0));
    Register tmp2 = ToRegister(ins->getTemp(1));
    MMod* mir = ins->mir();
    masm.ma_mod_mask(src, dest, tmp1, tmp2, ins->shift());
    if (mir->canBeNegativeDividend()) {
        if (!mir->isTruncated()) {
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Zero, ins->snapshot());
        } else {
            
        }
    }
}

void
CodeGeneratorARM::visitBitNotI(LBitNotI* ins)
{
    const LAllocation* input = ins->getOperand(0);
    const LDefinition* dest = ins->getDef(0);
    
    
    MOZ_ASSERT(!input->isConstant());

    masm.ma_mvn(ToRegister(input), ToRegister(dest));
}

void
CodeGeneratorARM::visitBitOpI(LBitOpI* ins)
{
    const LAllocation* lhs = ins->getOperand(0);
    const LAllocation* rhs = ins->getOperand(1);
    const LDefinition* dest = ins->getDef(0);
    
    switch (ins->bitop()) {
      case JSOP_BITOR:
        if (rhs->isConstant())
            masm.ma_orr(Imm32(ToInt32(rhs)), ToRegister(lhs), ToRegister(dest));
        else
            masm.ma_orr(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
        break;
      case JSOP_BITXOR:
        if (rhs->isConstant())
            masm.ma_eor(Imm32(ToInt32(rhs)), ToRegister(lhs), ToRegister(dest));
        else
            masm.ma_eor(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
        break;
      case JSOP_BITAND:
        if (rhs->isConstant())
            masm.ma_and(Imm32(ToInt32(rhs)), ToRegister(lhs), ToRegister(dest));
        else
            masm.ma_and(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
        break;
      default:
        MOZ_CRASH("unexpected binary opcode");
    }
}

void
CodeGeneratorARM::visitShiftI(LShiftI* ins)
{
    Register lhs = ToRegister(ins->lhs());
    const LAllocation* rhs = ins->rhs();
    Register dest = ToRegister(ins->output());

    if (rhs->isConstant()) {
        int32_t shift = ToInt32(rhs) & 0x1F;
        switch (ins->bitop()) {
          case JSOP_LSH:
            if (shift)
                masm.ma_lsl(Imm32(shift), lhs, dest);
            else
                masm.ma_mov(lhs, dest);
            break;
          case JSOP_RSH:
            if (shift)
                masm.ma_asr(Imm32(shift), lhs, dest);
            else
                masm.ma_mov(lhs, dest);
            break;
          case JSOP_URSH:
            if (shift) {
                masm.ma_lsr(Imm32(shift), lhs, dest);
            } else {
                
                masm.ma_mov(lhs, dest);
                if (ins->mir()->toUrsh()->fallible()) {
                    masm.ma_cmp(dest, Imm32(0));
                    bailoutIf(Assembler::LessThan, ins->snapshot());
                }
            }
            break;
          default:
            MOZ_CRASH("Unexpected shift op");
        }
    } else {
        
        
        
        masm.ma_and(Imm32(0x1F), ToRegister(rhs), dest);

        switch (ins->bitop()) {
          case JSOP_LSH:
            masm.ma_lsl(dest, lhs, dest);
            break;
          case JSOP_RSH:
            masm.ma_asr(dest, lhs, dest);
            break;
          case JSOP_URSH:
            masm.ma_lsr(dest, lhs, dest);
            if (ins->mir()->toUrsh()->fallible()) {
                
                masm.ma_cmp(dest, Imm32(0));
                bailoutIf(Assembler::LessThan, ins->snapshot());
            }
            break;
          default:
            MOZ_CRASH("Unexpected shift op");
        }
    }
}

void
CodeGeneratorARM::visitUrshD(LUrshD* ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register temp = ToRegister(ins->temp());

    const LAllocation* rhs = ins->rhs();
    FloatRegister out = ToFloatRegister(ins->output());

    if (rhs->isConstant()) {
        int32_t shift = ToInt32(rhs) & 0x1F;
        if (shift)
            masm.ma_lsr(Imm32(shift), lhs, temp);
        else
            masm.ma_mov(lhs, temp);
    } else {
        masm.ma_and(Imm32(0x1F), ToRegister(rhs), temp);
        masm.ma_lsr(temp, lhs, temp);
    }

    masm.convertUInt32ToDouble(temp, out);
}

void
CodeGeneratorARM::visitClzI(LClzI* ins)
{
    Register input = ToRegister(ins->input());
    Register output = ToRegister(ins->output());

    masm.ma_clz(input, output);
}

void
CodeGeneratorARM::visitPowHalfD(LPowHalfD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    Label done;

    
    masm.ma_vimm(NegativeInfinity<double>(), ScratchDoubleReg);
    masm.compareDouble(input, ScratchDoubleReg);
    masm.ma_vneg(ScratchDoubleReg, output, Assembler::Equal);
    masm.ma_b(&done, Assembler::Equal);

    
    
    masm.ma_vimm(0.0, ScratchDoubleReg);
    masm.ma_vadd(ScratchDoubleReg, input, output);
    masm.ma_vsqrt(output, output);

    masm.bind(&done);
}

MoveOperand
CodeGeneratorARM::toMoveOperand(const LAllocation* a) const
{
    if (a->isGeneralReg())
        return MoveOperand(ToRegister(a));
    if (a->isFloatReg())
        return MoveOperand(ToFloatRegister(a));
    int32_t offset = ToStackOffset(a);
    MOZ_ASSERT((offset & 3) == 0);
    return MoveOperand(StackPointer, offset);
}

class js::jit::OutOfLineTableSwitch : public OutOfLineCodeBase<CodeGeneratorARM>
{
    MTableSwitch* mir_;
    Vector<CodeLabel, 8, JitAllocPolicy> codeLabels_;

    void accept(CodeGeneratorARM* codegen) {
        codegen->visitOutOfLineTableSwitch(this);
    }

  public:
    OutOfLineTableSwitch(TempAllocator& alloc, MTableSwitch* mir)
      : mir_(mir),
        codeLabels_(alloc)
    {}

    MTableSwitch* mir() const {
        return mir_;
    }

    bool addCodeLabel(CodeLabel label) {
        return codeLabels_.append(label);
    }
    CodeLabel codeLabel(unsigned i) {
        return codeLabels_[i];
    }
};

void
CodeGeneratorARM::visitOutOfLineTableSwitch(OutOfLineTableSwitch* ool)
{
    MTableSwitch* mir = ool->mir();

    size_t numCases = mir->numCases();
    for (size_t i = 0; i < numCases; i++) {
        LBlock* caseblock = skipTrivialBlocks(mir->getCase(numCases - 1 - i))->lir();
        Label* caseheader = caseblock->label();
        uint32_t caseoffset = caseheader->offset();

        
        
        CodeLabel cl = ool->codeLabel(i);
        cl.src()->bind(caseoffset);
        masm.addCodeLabel(cl);
    }
}

void
CodeGeneratorARM::emitTableSwitchDispatch(MTableSwitch* mir, Register index, Register base)
{
    
    
    
    
    
    

    
    
    
    

    
    
    
    
    

    
    
    
    
    
    
    
    
    
    Label* defaultcase = skipTrivialBlocks(mir->getDefault())->lir()->label();

    int32_t cases = mir->numCases();
    
    masm.ma_sub(index, Imm32(mir->low()), index, SetCond);
    masm.ma_rsb(index, Imm32(cases - 1), index, SetCond, Assembler::NotSigned);
    
    
    
    AutoForbidPools afp(&masm, 1 + 1 + cases);
    masm.ma_ldr(DTRAddr(pc, DtrRegImmShift(index, LSL, 2)), pc, Offset, Assembler::NotSigned);
    masm.ma_b(defaultcase);

    
    
    
    OutOfLineTableSwitch* ool = new(alloc()) OutOfLineTableSwitch(alloc(), mir);
    for (int32_t i = 0; i < cases; i++) {
        CodeLabel cl;
        masm.writeCodePointer(cl.dest());
        ool->addCodeLabel(cl);
    }
    addOutOfLineCode(ool, mir);
}

void
CodeGeneratorARM::visitMathD(LMathD* math)
{
    const LAllocation* src1 = math->getOperand(0);
    const LAllocation* src2 = math->getOperand(1);
    const LDefinition* output = math->getDef(0);

    switch (math->jsop()) {
      case JSOP_ADD:
        masm.ma_vadd(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_SUB:
        masm.ma_vsub(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_MUL:
        masm.ma_vmul(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_DIV:
        masm.ma_vdiv(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      default:
        MOZ_CRASH("unexpected opcode");
    }
}

void
CodeGeneratorARM::visitMathF(LMathF* math)
{
    const LAllocation* src1 = math->getOperand(0);
    const LAllocation* src2 = math->getOperand(1);
    const LDefinition* output = math->getDef(0);

    switch (math->jsop()) {
      case JSOP_ADD:
        masm.ma_vadd_f32(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_SUB:
        masm.ma_vsub_f32(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_MUL:
        masm.ma_vmul_f32(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_DIV:
        masm.ma_vdiv_f32(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      default:
        MOZ_CRASH("unexpected opcode");
    }
}

void
CodeGeneratorARM::visitFloor(LFloor* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    Label bail;
    masm.floor(input, output, &bail);
    bailoutFrom(&bail, lir->snapshot());
}

void
CodeGeneratorARM::visitFloorF(LFloorF* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    Label bail;
    masm.floorf(input, output, &bail);
    bailoutFrom(&bail, lir->snapshot());
}

void
CodeGeneratorARM::visitCeil(LCeil* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    Label bail;
    masm.ceil(input, output, &bail);
    bailoutFrom(&bail, lir->snapshot());
}

void
CodeGeneratorARM::visitCeilF(LCeilF* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    Label bail;
    masm.ceilf(input, output, &bail);
    bailoutFrom(&bail, lir->snapshot());
}

void
CodeGeneratorARM::visitRound(LRound* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    FloatRegister tmp = ToFloatRegister(lir->temp());
    Label bail;
    
    
    masm.round(input, output, &bail, tmp);
    bailoutFrom(&bail, lir->snapshot());
}

void
CodeGeneratorARM::visitRoundF(LRoundF* lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    FloatRegister tmp = ToFloatRegister(lir->temp());
    Label bail;
    
    
    masm.roundf(input, output, &bail, tmp);
    bailoutFrom(&bail, lir->snapshot());
}

void
CodeGeneratorARM::emitRoundDouble(FloatRegister src, Register dest, Label* fail)
{
    masm.ma_vcvt_F64_I32(src, ScratchDoubleReg);
    masm.ma_vxfer(ScratchDoubleReg, dest);
    masm.ma_cmp(dest, Imm32(0x7fffffff));
    masm.ma_cmp(dest, Imm32(0x80000000), Assembler::NotEqual);
    masm.ma_b(fail, Assembler::Equal);
}

void
CodeGeneratorARM::visitTruncateDToInt32(LTruncateDToInt32* ins)
{
    emitTruncateDouble(ToFloatRegister(ins->input()), ToRegister(ins->output()), ins->mir());
}

void
CodeGeneratorARM::visitTruncateFToInt32(LTruncateFToInt32* ins)
{
    emitTruncateFloat32(ToFloatRegister(ins->input()), ToRegister(ins->output()), ins->mir());
}

static const uint32_t FrameSizes[] = { 128, 256, 512, 1024 };

FrameSizeClass
FrameSizeClass::FromDepth(uint32_t frameDepth)
{
    for (uint32_t i = 0; i < JS_ARRAY_LENGTH(FrameSizes); i++) {
        if (frameDepth < FrameSizes[i])
            return FrameSizeClass(i);
    }

    return FrameSizeClass::None();
}

FrameSizeClass
FrameSizeClass::ClassLimit()
{
    return FrameSizeClass(JS_ARRAY_LENGTH(FrameSizes));
}

uint32_t
FrameSizeClass::frameSize() const
{
    MOZ_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);
    MOZ_ASSERT(class_ < JS_ARRAY_LENGTH(FrameSizes));

    return FrameSizes[class_];
}

ValueOperand
CodeGeneratorARM::ToValue(LInstruction* ins, size_t pos)
{
    Register typeReg = ToRegister(ins->getOperand(pos + TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getOperand(pos + PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

ValueOperand
CodeGeneratorARM::ToOutValue(LInstruction* ins)
{
    Register typeReg = ToRegister(ins->getDef(TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getDef(PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

ValueOperand
CodeGeneratorARM::ToTempValue(LInstruction* ins, size_t pos)
{
    Register typeReg = ToRegister(ins->getTemp(pos + TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getTemp(pos + PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

void
CodeGeneratorARM::visitValue(LValue* value)
{
    const ValueOperand out = ToOutValue(value);

    masm.moveValue(value->value(), out);
}

void
CodeGeneratorARM::visitBox(LBox* box)
{
    const LDefinition* type = box->getDef(TYPE_INDEX);

    MOZ_ASSERT(!box->getOperand(0)->isConstant());

    
    
    
    masm.ma_mov(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
}

void
CodeGeneratorARM::visitBoxFloatingPoint(LBoxFloatingPoint* box)
{
    const LDefinition* payload = box->getDef(PAYLOAD_INDEX);
    const LDefinition* type = box->getDef(TYPE_INDEX);
    const LAllocation* in = box->getOperand(0);

    FloatRegister reg = ToFloatRegister(in);
    if (box->type() == MIRType_Float32) {
        masm.convertFloat32ToDouble(reg, ScratchFloat32Reg);
        reg = ScratchFloat32Reg;
    }

    
    
    masm.ma_vxfer(VFPRegister(reg), ToRegister(payload), ToRegister(type));
}

void
CodeGeneratorARM::visitUnbox(LUnbox* unbox)
{
    
    
    MUnbox* mir = unbox->mir();
    Register type = ToRegister(unbox->type());

    if (mir->fallible()) {
        masm.ma_cmp(type, Imm32(MIRTypeToTag(mir->type())));
        bailoutIf(Assembler::NotEqual, unbox->snapshot());
    }
}

void
CodeGeneratorARM::visitDouble(LDouble* ins)
{
    const LDefinition* out = ins->getDef(0);

    masm.ma_vimm(ins->getDouble(), ToFloatRegister(out));
}

void
CodeGeneratorARM::visitFloat32(LFloat32* ins)
{
    const LDefinition* out = ins->getDef(0);
    masm.loadConstantFloat32(ins->getFloat(), ToFloatRegister(out));
}

Register
CodeGeneratorARM::splitTagForTest(const ValueOperand& value)
{
    return value.typeReg();
}

void
CodeGeneratorARM::visitTestDAndBranch(LTestDAndBranch* test)
{
    const LAllocation* opd = test->input();
    masm.ma_vcmpz(ToFloatRegister(opd));
    masm.as_vmrs(pc);

    MBasicBlock* ifTrue = test->ifTrue();
    MBasicBlock* ifFalse = test->ifFalse();
    
    jumpToBlock(ifFalse, Assembler::Zero);
    
    
    jumpToBlock(ifFalse, Assembler::Overflow);
    jumpToBlock(ifTrue);
}

void
CodeGeneratorARM::visitTestFAndBranch(LTestFAndBranch* test)
{
    const LAllocation* opd = test->input();
    masm.ma_vcmpz_f32(ToFloatRegister(opd));
    masm.as_vmrs(pc);

    MBasicBlock* ifTrue = test->ifTrue();
    MBasicBlock* ifFalse = test->ifFalse();
    
    jumpToBlock(ifFalse, Assembler::Zero);
    
    
    jumpToBlock(ifFalse, Assembler::Overflow);
    jumpToBlock(ifTrue);
}

void
CodeGeneratorARM::visitCompareD(LCompareD* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());
    masm.compareDouble(lhs, rhs);
    masm.emitSet(Assembler::ConditionFromDoubleCondition(cond), ToRegister(comp->output()));
}

void
CodeGeneratorARM::visitCompareF(LCompareF* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());
    masm.compareFloat(lhs, rhs);
    masm.emitSet(Assembler::ConditionFromDoubleCondition(cond), ToRegister(comp->output()));
}

void
CodeGeneratorARM::visitCompareDAndBranch(LCompareDAndBranch* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->cmpMir()->jsop());
    masm.compareDouble(lhs, rhs);
    emitBranch(Assembler::ConditionFromDoubleCondition(cond), comp->ifTrue(), comp->ifFalse());
}

void
CodeGeneratorARM::visitCompareFAndBranch(LCompareFAndBranch* comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->cmpMir()->jsop());
    masm.compareFloat(lhs, rhs);
    emitBranch(Assembler::ConditionFromDoubleCondition(cond), comp->ifTrue(), comp->ifFalse());
}

void
CodeGeneratorARM::visitCompareB(LCompareB* lir)
{
    MCompare* mir = lir->mir();

    const ValueOperand lhs = ToValue(lir, LCompareB::Lhs);
    const LAllocation* rhs = lir->rhs();
    const Register output = ToRegister(lir->output());

    MOZ_ASSERT(mir->jsop() == JSOP_STRICTEQ || mir->jsop() == JSOP_STRICTNE);

    Label notBoolean, done;
    masm.branchTestBoolean(Assembler::NotEqual, lhs, &notBoolean);
    {
        if (rhs->isConstant())
            masm.cmp32(lhs.payloadReg(), Imm32(rhs->toConstant()->toBoolean()));
        else
            masm.cmp32(lhs.payloadReg(), ToRegister(rhs));
        masm.emitSet(JSOpToCondition(mir->compareType(), mir->jsop()), output);
        masm.jump(&done);
    }

    masm.bind(&notBoolean);
    {
        masm.move32(Imm32(mir->jsop() == JSOP_STRICTNE), output);
    }

    masm.bind(&done);
}

void
CodeGeneratorARM::visitCompareBAndBranch(LCompareBAndBranch* lir)
{
    MCompare* mir = lir->cmpMir();
    const ValueOperand lhs = ToValue(lir, LCompareBAndBranch::Lhs);
    const LAllocation* rhs = lir->rhs();

    MOZ_ASSERT(mir->jsop() == JSOP_STRICTEQ || mir->jsop() == JSOP_STRICTNE);

    Assembler::Condition cond = masm.testBoolean(Assembler::NotEqual, lhs);
    jumpToBlock((mir->jsop() == JSOP_STRICTEQ) ? lir->ifFalse() : lir->ifTrue(), cond);

    if (rhs->isConstant())
        masm.cmp32(lhs.payloadReg(), Imm32(rhs->toConstant()->toBoolean()));
    else
        masm.cmp32(lhs.payloadReg(), ToRegister(rhs));
    emitBranch(JSOpToCondition(mir->compareType(), mir->jsop()), lir->ifTrue(), lir->ifFalse());
}

void
CodeGeneratorARM::visitCompareV(LCompareV* lir)
{
    MCompare* mir = lir->mir();
    Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
    const ValueOperand lhs = ToValue(lir, LCompareV::LhsInput);
    const ValueOperand rhs = ToValue(lir, LCompareV::RhsInput);
    const Register output = ToRegister(lir->output());

    MOZ_ASSERT(mir->jsop() == JSOP_EQ || mir->jsop() == JSOP_STRICTEQ ||
               mir->jsop() == JSOP_NE || mir->jsop() == JSOP_STRICTNE);

    Label notEqual, done;
    masm.cmp32(lhs.typeReg(), rhs.typeReg());
    masm.j(Assembler::NotEqual, &notEqual);
    {
        masm.cmp32(lhs.payloadReg(), rhs.payloadReg());
        masm.emitSet(cond, output);
        masm.jump(&done);
    }
    masm.bind(&notEqual);
    {
        masm.move32(Imm32(cond == Assembler::NotEqual), output);
    }

    masm.bind(&done);
}

void
CodeGeneratorARM::visitCompareVAndBranch(LCompareVAndBranch* lir)
{
    MCompare* mir = lir->cmpMir();
    Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
    const ValueOperand lhs = ToValue(lir, LCompareVAndBranch::LhsInput);
    const ValueOperand rhs = ToValue(lir, LCompareVAndBranch::RhsInput);

    MOZ_ASSERT(mir->jsop() == JSOP_EQ || mir->jsop() == JSOP_STRICTEQ ||
               mir->jsop() == JSOP_NE || mir->jsop() == JSOP_STRICTNE);

    MBasicBlock* notEqual = (cond == Assembler::Equal) ? lir->ifFalse() : lir->ifTrue();

    masm.cmp32(lhs.typeReg(), rhs.typeReg());
    jumpToBlock(notEqual, Assembler::NotEqual);
    masm.cmp32(lhs.payloadReg(), rhs.payloadReg());
    emitBranch(cond, lir->ifTrue(), lir->ifFalse());
}

void
CodeGeneratorARM::visitBitAndAndBranch(LBitAndAndBranch* baab)
{
    if (baab->right()->isConstant())
        masm.ma_tst(ToRegister(baab->left()), Imm32(ToInt32(baab->right())));
    else
        masm.ma_tst(ToRegister(baab->left()), ToRegister(baab->right()));
    emitBranch(Assembler::NonZero, baab->ifTrue(), baab->ifFalse());
}

void
CodeGeneratorARM::visitAsmJSUInt32ToDouble(LAsmJSUInt32ToDouble* lir)
{
    masm.convertUInt32ToDouble(ToRegister(lir->input()), ToFloatRegister(lir->output()));
}

void
CodeGeneratorARM::visitAsmJSUInt32ToFloat32(LAsmJSUInt32ToFloat32* lir)
{
    masm.convertUInt32ToFloat32(ToRegister(lir->input()), ToFloatRegister(lir->output()));
}

void
CodeGeneratorARM::visitNotI(LNotI* ins)
{
    
    masm.ma_cmp(ToRegister(ins->input()), Imm32(0));
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()));
}

void
CodeGeneratorARM::visitNotD(LNotD* ins)
{
    
    
    
    FloatRegister opd = ToFloatRegister(ins->input());
    Register dest = ToRegister(ins->output());

    
    masm.ma_vcmpz(opd);
    
    bool nocond = true;
    if (nocond) {
        
        masm.as_vmrs(dest);
        masm.ma_lsr(Imm32(28), dest, dest);
        
        masm.ma_alu(dest, lsr(dest, 2), dest, OpOrr);
        masm.ma_and(Imm32(1), dest);
    } else {
        masm.as_vmrs(pc);
        masm.ma_mov(Imm32(0), dest);
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::Equal);
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::Overflow);
    }
}

void
CodeGeneratorARM::visitNotF(LNotF* ins)
{
    
    
    
    FloatRegister opd = ToFloatRegister(ins->input());
    Register dest = ToRegister(ins->output());

    
    masm.ma_vcmpz_f32(opd);
    
    bool nocond = true;
    if (nocond) {
        
        masm.as_vmrs(dest);
        masm.ma_lsr(Imm32(28), dest, dest);
        
        masm.ma_alu(dest, lsr(dest, 2), dest, OpOrr);
        masm.ma_and(Imm32(1), dest);
    } else {
        masm.as_vmrs(pc);
        masm.ma_mov(Imm32(0), dest);
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::Equal);
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::Overflow);
    }
}

void
CodeGeneratorARM::visitGuardShape(LGuardShape* guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());

    masm.ma_ldr(DTRAddr(obj, DtrOffImm(JSObject::offsetOfShape())), tmp);
    masm.ma_cmp(tmp, ImmGCPtr(guard->mir()->shape()));

    bailoutIf(Assembler::NotEqual, guard->snapshot());
}

void
CodeGeneratorARM::visitGuardObjectGroup(LGuardObjectGroup* guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());
    MOZ_ASSERT(obj != tmp);

    masm.ma_ldr(DTRAddr(obj, DtrOffImm(JSObject::offsetOfGroup())), tmp);
    masm.ma_cmp(tmp, ImmGCPtr(guard->mir()->group()));

    Assembler::Condition cond =
        guard->mir()->bailOnEquality() ? Assembler::Equal : Assembler::NotEqual;
    bailoutIf(cond, guard->snapshot());
}

void
CodeGeneratorARM::visitGuardClass(LGuardClass* guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());

    masm.loadObjClass(obj, tmp);
    masm.ma_cmp(tmp, Imm32((uint32_t)guard->mir()->getClass()));
    bailoutIf(Assembler::NotEqual, guard->snapshot());
}

void
CodeGeneratorARM::generateInvalidateEpilogue()
{
    
    
    for (size_t i = 0; i < sizeof(void*); i += Assembler::NopSize())
        masm.nop();

    masm.bind(&invalidate_);

    
    masm.Push(lr);

    
    invalidateEpilogueData_ = masm.pushWithPatch(ImmWord(uintptr_t(-1)));
    JitCode* thunk = gen->jitRuntime()->getInvalidationThunk();

    masm.branch(thunk);

    
    
    masm.assumeUnreachable("Should have returned directly to its caller instead of here.");
}

void
DispatchIonCache::initializeAddCacheState(LInstruction* ins, AddCacheState* addState)
{
    
    addState->dispatchScratch = ScratchRegister;
}

void
CodeGeneratorARM::visitLoadTypedArrayElementStatic(LLoadTypedArrayElementStatic* ins)
{
    MOZ_CRASH("NYI");
}

void
CodeGeneratorARM::visitStoreTypedArrayElementStatic(LStoreTypedArrayElementStatic* ins)
{
    MOZ_CRASH("NYI");
}

void
CodeGeneratorARM::visitAsmJSCall(LAsmJSCall* ins)
{
    MAsmJSCall* mir = ins->mir();

    if (UseHardFpABI() || mir->callee().which() != MAsmJSCall::Callee::Builtin) {
        emitAsmJSCall(ins);
        return;
    }

    
    
    
    
    

    for (unsigned i = 0, e = ins->numOperands(); i < e; i++) {
        LAllocation* a = ins->getOperand(i);
        if (a->isFloatReg()) {
            FloatRegister fr = ToFloatRegister(a);
            if (fr.isDouble()) {
                uint32_t srcId = fr.singleOverlay().id();
                masm.ma_vxfer(fr, Register::FromCode(srcId), Register::FromCode(srcId + 1));
            } else {
                uint32_t srcId = fr.id();
                masm.ma_vxfer(fr, Register::FromCode(srcId));
            }
        }
    }

    emitAsmJSCall(ins);

    switch (mir->type()) {
      case MIRType_Double:
        masm.ma_vxfer(r0, r1, d0);
        break;
      case MIRType_Float32:
        masm.as_vxfer(r0, InvalidReg, VFPRegister(d0).singleOverlay(), Assembler::CoreToFloat);
        break;
      default:
        break;
    }
}

void
CodeGeneratorARM::visitAsmJSLoadHeap(LAsmJSLoadHeap* ins)
{
    const MAsmJSLoadHeap* mir = ins->mir();
    bool isSigned;
    int size;
    bool isFloat = false;
    switch (mir->accessType()) {
      case Scalar::Int8:    isSigned = true;  size =  8; break;
      case Scalar::Uint8:   isSigned = false; size =  8; break;
      case Scalar::Int16:   isSigned = true;  size = 16; break;
      case Scalar::Uint16:  isSigned = false; size = 16; break;
      case Scalar::Int32:
      case Scalar::Uint32:  isSigned = true;  size = 32; break;
      case Scalar::Float64: isFloat = true;   size = 64; break;
      case Scalar::Float32: isFloat = true;   size = 32; break;
      default: MOZ_CRASH("unexpected array type");
    }

    memoryBarrier(mir->barrierBefore());

    const LAllocation* ptr = ins->ptr();

    if (ptr->isConstant()) {
        MOZ_ASSERT(!mir->needsBoundsCheck());
        int32_t ptrImm = ptr->toConstant()->toInt32();
        MOZ_ASSERT(ptrImm >= 0);
        if (isFloat) {
            VFPRegister vd(ToFloatRegister(ins->output()));
            if (size == 32)
                masm.ma_vldr(Operand(HeapReg, ptrImm), vd.singleOverlay(), Assembler::Always);
            else
                masm.ma_vldr(Operand(HeapReg, ptrImm), vd, Assembler::Always);
        }  else {
            masm.ma_dataTransferN(IsLoad, size, isSigned, HeapReg, Imm32(ptrImm),
                                  ToRegister(ins->output()), Offset, Assembler::Always);
        }
        memoryBarrier(mir->barrierAfter());
        return;
    }

    Register ptrReg = ToRegister(ptr);

    if (!mir->needsBoundsCheck()) {
        if (isFloat) {
            VFPRegister vd(ToFloatRegister(ins->output()));
            if (size == 32)
                masm.ma_vldr(vd.singleOverlay(), HeapReg, ptrReg, 0, Assembler::Always);
            else
                masm.ma_vldr(vd, HeapReg, ptrReg, 0, Assembler::Always);
        } else {
            masm.ma_dataTransferN(IsLoad, size, isSigned, HeapReg, ptrReg,
                                  ToRegister(ins->output()), Offset, Assembler::Always);
        }
        memoryBarrier(mir->barrierAfter());
        return;
    }

    BufferOffset bo = masm.ma_BoundsCheck(ptrReg);
    if (isFloat) {
        FloatRegister dst = ToFloatRegister(ins->output());
        VFPRegister vd(dst);
        if (size == 32) {
            masm.ma_vldr(Operand(GlobalReg, AsmJSNaN32GlobalDataOffset - AsmJSGlobalRegBias),
                         vd.singleOverlay(), Assembler::AboveOrEqual);
            masm.ma_vldr(vd.singleOverlay(), HeapReg, ptrReg, 0, Assembler::Below);
        } else {
            masm.ma_vldr(Operand(GlobalReg, AsmJSNaN64GlobalDataOffset - AsmJSGlobalRegBias),
                         vd, Assembler::AboveOrEqual);
            masm.ma_vldr(vd, HeapReg, ptrReg, 0, Assembler::Below);
        }
    } else {
        Register d = ToRegister(ins->output());
        masm.ma_mov(Imm32(0), d, NoSetCond, Assembler::AboveOrEqual);
        masm.ma_dataTransferN(IsLoad, size, isSigned, HeapReg, ptrReg, d, Offset, Assembler::Below);
    }
    memoryBarrier(mir->barrierAfter());
    masm.append(AsmJSHeapAccess(bo.getOffset()));
}

void
CodeGeneratorARM::visitAsmJSStoreHeap(LAsmJSStoreHeap* ins)
{
    const MAsmJSStoreHeap* mir = ins->mir();
    bool isSigned;
    int size;
    bool isFloat = false;
    switch (mir->accessType()) {
      case Scalar::Int8:
      case Scalar::Uint8:   isSigned = false; size = 8; break;
      case Scalar::Int16:
      case Scalar::Uint16:  isSigned = false; size = 16; break;
      case Scalar::Int32:
      case Scalar::Uint32:  isSigned = true;  size = 32; break;
      case Scalar::Float64: isFloat  = true;  size = 64; break;
      case Scalar::Float32: isFloat = true;   size = 32; break;
      default: MOZ_CRASH("unexpected array type");
    }
    const LAllocation* ptr = ins->ptr();
    memoryBarrier(mir->barrierBefore());
    if (ptr->isConstant()) {
        MOZ_ASSERT(!mir->needsBoundsCheck());
        int32_t ptrImm = ptr->toConstant()->toInt32();
        MOZ_ASSERT(ptrImm >= 0);
        if (isFloat) {
            VFPRegister vd(ToFloatRegister(ins->value()));
            if (size == 32)
                masm.ma_vstr(vd.singleOverlay(), Operand(HeapReg, ptrImm), Assembler::Always);
            else
                masm.ma_vstr(vd, Operand(HeapReg, ptrImm), Assembler::Always);
        } else {
            masm.ma_dataTransferN(IsStore, size, isSigned, HeapReg, Imm32(ptrImm),
                                  ToRegister(ins->value()), Offset, Assembler::Always);
        }
        memoryBarrier(mir->barrierAfter());
        return;
    }

    Register ptrReg = ToRegister(ptr);

    if (!mir->needsBoundsCheck()) {
        Register ptrReg = ToRegister(ptr);
        if (isFloat) {
            VFPRegister vd(ToFloatRegister(ins->value()));
            if (size == 32)
                masm.ma_vstr(vd.singleOverlay(), HeapReg, ptrReg, 0, 0, Assembler::Always);
            else
                masm.ma_vstr(vd, HeapReg, ptrReg, 0, 0, Assembler::Always);
        } else {
            masm.ma_dataTransferN(IsStore, size, isSigned, HeapReg, ptrReg,
                                  ToRegister(ins->value()), Offset, Assembler::Always);
        }
        memoryBarrier(mir->barrierAfter());
        return;
    }

    BufferOffset bo = masm.ma_BoundsCheck(ptrReg);
    if (isFloat) {
        VFPRegister vd(ToFloatRegister(ins->value()));
        if (size == 32)
            masm.ma_vstr(vd.singleOverlay(), HeapReg, ptrReg, 0, 0, Assembler::Below);
        else
            masm.ma_vstr(vd, HeapReg, ptrReg, 0, 0, Assembler::Below);
    } else {
        masm.ma_dataTransferN(IsStore, size, isSigned, HeapReg, ptrReg,
                              ToRegister(ins->value()), Offset, Assembler::Below);
    }
    memoryBarrier(mir->barrierAfter());
    masm.append(AsmJSHeapAccess(bo.getOffset()));
}

void
CodeGeneratorARM::visitAsmJSCompareExchangeHeap(LAsmJSCompareExchangeHeap* ins)
{
    MAsmJSCompareExchangeHeap* mir = ins->mir();
    Scalar::Type vt = mir->accessType();
    const LAllocation* ptr = ins->ptr();
    Register ptrReg = ToRegister(ptr);
    BaseIndex srcAddr(HeapReg, ptrReg, TimesOne);
    MOZ_ASSERT(ins->addrTemp()->isBogusTemp());

    Register oldval = ToRegister(ins->oldValue());
    Register newval = ToRegister(ins->newValue());

    Label rejoin;
    uint32_t maybeCmpOffset = 0;
    if (mir->needsBoundsCheck()) {
        Label goahead;
        BufferOffset bo = masm.ma_BoundsCheck(ptrReg);
        Register out = ToRegister(ins->output());
        maybeCmpOffset = bo.getOffset();
        masm.ma_b(&goahead, Assembler::Below);
        memoryBarrier(MembarFull);
        masm.as_eor(out, out, O2Reg(out));
        masm.ma_b(&rejoin, Assembler::Always);
        masm.bind(&goahead);
    }
    masm.compareExchangeToTypedIntArray(vt == Scalar::Uint32 ? Scalar::Int32 : vt,
                                        srcAddr, oldval, newval, InvalidReg,
                                        ToAnyRegister(ins->output()));
    if (rejoin.used()) {
        masm.bind(&rejoin);
        masm.append(AsmJSHeapAccess(maybeCmpOffset));
    }
}

void
CodeGeneratorARM::visitAsmJSCompareExchangeCallout(LAsmJSCompareExchangeCallout* ins)
{
    const MAsmJSCompareExchangeHeap* mir = ins->mir();
    Scalar::Type viewType = mir->accessType();
    Register ptr = ToRegister(ins->ptr());
    Register oldval = ToRegister(ins->oldval());
    Register newval = ToRegister(ins->newval());

    MOZ_ASSERT(ToRegister(ins->output()) == ReturnReg);

    masm.setupAlignedABICall(4);
    masm.ma_mov(Imm32(viewType), ScratchRegister);
    masm.passABIArg(ScratchRegister);
    masm.passABIArg(ptr);
    masm.passABIArg(oldval);
    masm.passABIArg(newval);

    masm.callWithABI(AsmJSImm_AtomicCmpXchg);
}

void
CodeGeneratorARM::visitAsmJSAtomicBinopHeap(LAsmJSAtomicBinopHeap* ins)
{
    MOZ_ASSERT(ins->mir()->hasUses());
    MOZ_ASSERT(ins->addrTemp()->isBogusTemp());

    MAsmJSAtomicBinopHeap* mir = ins->mir();
    Scalar::Type vt = mir->accessType();
    Register ptrReg = ToRegister(ins->ptr());
    Register temp = ins->temp()->isBogusTemp() ? InvalidReg : ToRegister(ins->temp());
    const LAllocation* value = ins->value();
    AtomicOp op = mir->operation();

    BaseIndex srcAddr(HeapReg, ptrReg, TimesOne);

    Label rejoin;
    uint32_t maybeCmpOffset = 0;
    if (mir->needsBoundsCheck()) {
        Label goahead;
        BufferOffset bo = masm.ma_BoundsCheck(ptrReg);
        Register out = ToRegister(ins->output());
        maybeCmpOffset = bo.getOffset();
        masm.ma_b(&goahead, Assembler::Below);
        memoryBarrier(MembarFull);
        masm.as_eor(out, out, O2Reg(out));
        masm.ma_b(&rejoin, Assembler::Always);
        masm.bind(&goahead);
    }
    if (value->isConstant())
        masm.atomicBinopToTypedIntArray(op, vt == Scalar::Uint32 ? Scalar::Int32 : vt,
                                        Imm32(ToInt32(value)), srcAddr, temp, InvalidReg,
                                        ToAnyRegister(ins->output()));
    else
        masm.atomicBinopToTypedIntArray(op, vt == Scalar::Uint32 ? Scalar::Int32 : vt,
                                        ToRegister(value), srcAddr, temp, InvalidReg,
                                        ToAnyRegister(ins->output()));
    if (rejoin.used()) {
        masm.bind(&rejoin);
        masm.append(AsmJSHeapAccess(maybeCmpOffset));
    }
}

void
CodeGeneratorARM::visitAsmJSAtomicBinopHeapForEffect(LAsmJSAtomicBinopHeapForEffect* ins)
{
    MOZ_ASSERT(!ins->mir()->hasUses());
    MOZ_ASSERT(ins->temp()->isBogusTemp());
    MOZ_ASSERT(ins->addrTemp()->isBogusTemp());

    MAsmJSAtomicBinopHeap* mir = ins->mir();
    Scalar::Type vt = mir->accessType();
    Register ptrReg = ToRegister(ins->ptr());
    const LAllocation* value = ins->value();
    AtomicOp op = mir->operation();

    BaseIndex srcAddr(HeapReg, ptrReg, TimesOne);

    Label rejoin;
    uint32_t maybeCmpOffset = 0;
    if (mir->needsBoundsCheck()) {
        Label goahead;
        BufferOffset bo = masm.ma_BoundsCheck(ptrReg);
        maybeCmpOffset = bo.getOffset();
        masm.ma_b(&goahead, Assembler::Below);
        memoryBarrier(MembarFull);
        masm.ma_b(&rejoin, Assembler::Always);
        masm.bind(&goahead);
    }

    if (value->isConstant())
        masm.atomicBinopToTypedIntArray(op, vt, Imm32(ToInt32(value)), srcAddr);
    else
        masm.atomicBinopToTypedIntArray(op, vt, ToRegister(value), srcAddr);

    if (rejoin.used()) {
        masm.bind(&rejoin);
        masm.append(AsmJSHeapAccess(maybeCmpOffset));
    }
}

void
CodeGeneratorARM::visitAsmJSAtomicBinopCallout(LAsmJSAtomicBinopCallout* ins)
{
    const MAsmJSAtomicBinopHeap* mir = ins->mir();
    Scalar::Type viewType = mir->accessType();
    Register ptr = ToRegister(ins->ptr());
    Register value = ToRegister(ins->value());

    masm.setupAlignedABICall(3);
    masm.ma_mov(Imm32(viewType), ScratchRegister);
    masm.passABIArg(ScratchRegister);
    masm.passABIArg(ptr);
    masm.passABIArg(value);

    switch (mir->operation()) {
      case AtomicFetchAddOp:
        masm.callWithABI(AsmJSImm_AtomicFetchAdd);
        break;
      case AtomicFetchSubOp:
        masm.callWithABI(AsmJSImm_AtomicFetchSub);
        break;
      case AtomicFetchAndOp:
        masm.callWithABI(AsmJSImm_AtomicFetchAnd);
        break;
      case AtomicFetchOrOp:
        masm.callWithABI(AsmJSImm_AtomicFetchOr);
        break;
      case AtomicFetchXorOp:
        masm.callWithABI(AsmJSImm_AtomicFetchXor);
        break;
      default:
        MOZ_CRASH("Unknown op");
    }
}

void
CodeGeneratorARM::visitAsmJSPassStackArg(LAsmJSPassStackArg* ins)
{
    const MAsmJSPassStackArg* mir = ins->mir();
    Operand dst(StackPointer, mir->spOffset());
    if (ins->arg()->isConstant()) {
        
        masm.ma_storeImm(Imm32(ToInt32(ins->arg())), dst);
    } else {
        if (ins->arg()->isGeneralReg())
            masm.ma_str(ToRegister(ins->arg()), dst);
        else
            masm.ma_vstr(ToFloatRegister(ins->arg()), dst);
    }
}

void
CodeGeneratorARM::visitUDiv(LUDiv* ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());

    Label done;
    generateUDivModZeroCheck(rhs, output, &done, ins->snapshot(), ins->mir());

    masm.ma_udiv(lhs, rhs, output);

    if (!ins->mir()->isTruncated()) {
        masm.ma_cmp(output, Imm32(0));
        bailoutIf(Assembler::LessThan, ins->snapshot());
    }

    masm.bind(&done);
}

void
CodeGeneratorARM::visitUMod(LUMod* ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());

    Label done;
    generateUDivModZeroCheck(rhs, output, &done, ins->snapshot(), ins->mir());

    masm.ma_umod(lhs, rhs, output);

    if (!ins->mir()->isTruncated()) {
        masm.ma_cmp(output, Imm32(0));
        bailoutIf(Assembler::LessThan, ins->snapshot());
    }

    masm.bind(&done);
}

template<class T>
void
CodeGeneratorARM::generateUDivModZeroCheck(Register rhs, Register output, Label* done,
                                           LSnapshot* snapshot, T* mir)
{
    if (!mir)
        return;
    if (mir->canBeDivideByZero()) {
        masm.ma_cmp(rhs, Imm32(0));
        if (mir->isTruncated()) {
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            
            masm.ma_mov(Imm32(0), output);
            masm.ma_b(done);
            masm.bind(&skip);
        } else {
            
            MOZ_ASSERT(mir->fallible());
            bailoutIf(Assembler::Equal, snapshot);
        }
    }
}

void
CodeGeneratorARM::visitSoftUDivOrMod(LSoftUDivOrMod* ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register output = ToRegister(ins->output());

    MOZ_ASSERT(lhs == r0);
    MOZ_ASSERT(rhs == r1);
    MOZ_ASSERT(ins->mirRaw()->isDiv() || ins->mirRaw()->isMod());
    MOZ_ASSERT_IF(ins->mirRaw()->isDiv(), output == r0);
    MOZ_ASSERT_IF(ins->mirRaw()->isMod(), output == r1);

    Label done;
    MDiv* div = ins->mir()->isDiv() ? ins->mir()->toDiv() : nullptr;
    MMod* mod = !div ? ins->mir()->toMod() : nullptr;

    generateUDivModZeroCheck(rhs, output, &done, ins->snapshot(), div);
    generateUDivModZeroCheck(rhs, output, &done, ins->snapshot(), mod);

    masm.setupAlignedABICall(2);
    masm.passABIArg(lhs);
    masm.passABIArg(rhs);
    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_aeabi_uidivmod);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, __aeabi_uidivmod));

    
    if (div && !div->canTruncateRemainder()) {
        MOZ_ASSERT(div->fallible());
        masm.ma_cmp(r1, Imm32(0));
        bailoutIf(Assembler::NonZero, ins->snapshot());
    }

    
    if ((div && !div->isTruncated()) || (mod && !mod->isTruncated())) {
        DebugOnly<bool> isFallible = (div && div->fallible()) || (mod && mod->fallible());
        MOZ_ASSERT(isFallible);
        masm.ma_cmp(output, Imm32(0));
        bailoutIf(Assembler::LessThan, ins->snapshot());
    }

    masm.bind(&done);
}

void
CodeGeneratorARM::visitEffectiveAddress(LEffectiveAddress* ins)
{
    const MEffectiveAddress* mir = ins->mir();
    Register base = ToRegister(ins->base());
    Register index = ToRegister(ins->index());
    Register output = ToRegister(ins->output());
    masm.as_add(output, base, lsl(index, mir->scale()));
    masm.ma_add(Imm32(mir->displacement()), output);
}

void
CodeGeneratorARM::visitAsmJSLoadGlobalVar(LAsmJSLoadGlobalVar* ins)
{
    const MAsmJSLoadGlobalVar* mir = ins->mir();
    unsigned addr = mir->globalDataOffset() - AsmJSGlobalRegBias;
    if (mir->type() == MIRType_Int32) {
        masm.ma_dtr(IsLoad, GlobalReg, Imm32(addr), ToRegister(ins->output()));
    } else if (mir->type() == MIRType_Float32) {
        VFPRegister vd(ToFloatRegister(ins->output()));
        masm.ma_vldr(Operand(GlobalReg, addr), vd.singleOverlay());
    } else {
        masm.ma_vldr(Operand(GlobalReg, addr), ToFloatRegister(ins->output()));
    }
}

void
CodeGeneratorARM::visitAsmJSStoreGlobalVar(LAsmJSStoreGlobalVar* ins)
{
    const MAsmJSStoreGlobalVar* mir = ins->mir();

    MIRType type = mir->value()->type();
    MOZ_ASSERT(IsNumberType(type));

    unsigned addr = mir->globalDataOffset() - AsmJSGlobalRegBias;
    if (type == MIRType_Int32) {
        masm.ma_dtr(IsStore, GlobalReg, Imm32(addr), ToRegister(ins->value()));
    } else if (type == MIRType_Float32) {
        VFPRegister vd(ToFloatRegister(ins->value()));
        masm.ma_vstr(vd.singleOverlay(), Operand(GlobalReg, addr));
    } else {
        masm.ma_vstr(ToFloatRegister(ins->value()), Operand(GlobalReg, addr));
    }
}

void
CodeGeneratorARM::visitAsmJSLoadFuncPtr(LAsmJSLoadFuncPtr* ins)
{
    const MAsmJSLoadFuncPtr* mir = ins->mir();

    Register index = ToRegister(ins->index());
    Register tmp = ToRegister(ins->temp());
    Register out = ToRegister(ins->output());
    unsigned addr = mir->globalDataOffset();
    masm.ma_mov(Imm32(addr - AsmJSGlobalRegBias), tmp);
    masm.as_add(tmp, tmp, lsl(index, 2));
    masm.ma_ldr(DTRAddr(GlobalReg, DtrRegImmShift(tmp, LSL, 0)), out);
}

void
CodeGeneratorARM::visitAsmJSLoadFFIFunc(LAsmJSLoadFFIFunc* ins)
{
    const MAsmJSLoadFFIFunc* mir = ins->mir();

    masm.ma_ldr(Operand(GlobalReg, mir->globalDataOffset() - AsmJSGlobalRegBias),
                ToRegister(ins->output()));
}

void
CodeGeneratorARM::visitNegI(LNegI* ins)
{
    Register input = ToRegister(ins->input());
    masm.ma_neg(input, ToRegister(ins->output()));
}

void
CodeGeneratorARM::visitNegD(LNegD* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    masm.ma_vneg(input, ToFloatRegister(ins->output()));
}

void
CodeGeneratorARM::visitNegF(LNegF* ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    masm.ma_vneg_f32(input, ToFloatRegister(ins->output()));
}

void
CodeGeneratorARM::memoryBarrier(MemoryBarrierBits barrier)
{
    
    if (barrier == (MembarStoreStore|MembarSynchronizing))
        masm.ma_dsb(masm.BarrierST);
    else if (barrier & MembarSynchronizing)
        masm.ma_dsb();
    else if (barrier == MembarStoreStore)
        masm.ma_dmb(masm.BarrierST);
    else if (barrier)
        masm.ma_dmb();
}

void
CodeGeneratorARM::visitMemoryBarrier(LMemoryBarrier* ins)
{
    memoryBarrier(ins->type());
}
