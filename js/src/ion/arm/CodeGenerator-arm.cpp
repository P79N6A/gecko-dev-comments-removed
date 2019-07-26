







#include "jscntxt.h"
#include "jscompartment.h"
#include "jsnum.h"

#include "CodeGenerator-arm.h"
#include "ion/IonCompartment.h"
#include "ion/IonFrames.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "ion/MoveEmitter.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "vm/Shape.h"

#include "jsscriptinlines.h"

#include "vm/Shape-inl.h"

using namespace js;
using namespace js::ion;


CodeGeneratorARM::CodeGeneratorARM(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm)
  : CodeGeneratorShared(gen, graph, masm),
    deoptLabel_(NULL)
{
}

bool
CodeGeneratorARM::generatePrologue()
{
    
    masm.reserveStack(frameSize());
    masm.checkStackAlignment();
    
    
    returnLabel_ = new HeapLabel();

    return true;
}

bool
CodeGeneratorARM::generateEpilogue()
{
    masm.bind(returnLabel_);

    
    masm.freeStack(frameSize());
    JS_ASSERT(masm.framePushed() == 0);

    masm.ma_pop(pc);
    masm.dumpPool();
    return true;
}

void
CodeGeneratorARM::emitBranch(Assembler::Condition cond, MBasicBlock *mirTrue, MBasicBlock *mirFalse)
{
    LBlock *ifTrue = mirTrue->lir();
    LBlock *ifFalse = mirFalse->lir();
    if (isNextBlock(ifFalse)) {
        masm.ma_b(ifTrue->label(), cond);
    } else {
        masm.ma_b(ifFalse->label(), Assembler::InvertCondition(cond));
        if (!isNextBlock(ifTrue)) {
            masm.ma_b(ifTrue->label());
        }
    }
}


bool
OutOfLineBailout::accept(CodeGeneratorARM *codegen)
{
    return codegen->visitOutOfLineBailout(this);
}

bool
CodeGeneratorARM::visitTestIAndBranch(LTestIAndBranch *test)
{
    const LAllocation *opd = test->getOperand(0);
    LBlock *ifTrue = test->ifTrue()->lir();
    LBlock *ifFalse = test->ifFalse()->lir();

    
    masm.ma_cmp(ToRegister(opd), Imm32(0));

    if (isNextBlock(ifFalse)) {
        masm.ma_b(ifTrue->label(), Assembler::NonZero);
    } else if (isNextBlock(ifTrue)) {
        masm.ma_b(ifFalse->label(), Assembler::Zero);
    } else {
        masm.ma_b(ifFalse->label(), Assembler::Zero);
        masm.ma_b(ifTrue->label());
    }
    return true;
}

bool
CodeGeneratorARM::visitCompare(LCompare *comp)
{
    Assembler::Condition cond = JSOpToCondition(comp->mir()->compareType(), comp->jsop());
    const LAllocation *left = comp->getOperand(0);
    const LAllocation *right = comp->getOperand(1);
    const LDefinition *def = comp->getDef(0);

    if (right->isConstant())
        masm.ma_cmp(ToRegister(left), Imm32(ToInt32(right)));
    else
        masm.ma_cmp(ToRegister(left), ToOperand(right));
    masm.ma_mov(Imm32(0), ToRegister(def));
    masm.ma_mov(Imm32(1), ToRegister(def), NoSetCond, cond);
    return true;
}

bool
CodeGeneratorARM::visitCompareAndBranch(LCompareAndBranch *comp)
{
    Assembler::Condition cond = JSOpToCondition(comp->mir()->compareType(), comp->jsop());
    if (comp->right()->isConstant())
        masm.ma_cmp(ToRegister(comp->left()), Imm32(ToInt32(comp->right())));
    else
        masm.ma_cmp(ToRegister(comp->left()), ToOperand(comp->right()));
    emitBranch(cond, comp->ifTrue(), comp->ifFalse());
    return true;

}

bool
CodeGeneratorARM::generateOutOfLineCode()
{
    if (!CodeGeneratorShared::generateOutOfLineCode())
        return false;

    if (deoptLabel_) {
        
        masm.bind(deoptLabel_);

        
        masm.ma_mov(Imm32(frameSize()), lr);

        IonCompartment *ion = GetIonContext()->compartment->ionCompartment();
        IonCode *handler = ion->getGenericBailoutHandler();

        masm.branch(handler);
    }

    return true;
}

bool
CodeGeneratorARM::bailoutIf(Assembler::Condition condition, LSnapshot *snapshot)
{
    if (!encode(snapshot))
        return false;

    
    
    
    JS_ASSERT_IF(frameClass_ != FrameSizeClass::None(),
                 frameClass_.frameSize() == masm.framePushed());

    if (assignBailoutId(snapshot)) {
        uint8_t *code = deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE;
        masm.ma_b(code, Relocation::HARDCODED, condition);
        return true;
    }

    
    
    
    OutOfLineBailout *ool = new OutOfLineBailout(snapshot, masm.framePushed());
    if (!addOutOfLineCode(ool))
        return false;

    masm.ma_b(ool->entry(), condition);

    return true;
}
bool
CodeGeneratorARM::bailoutFrom(Label *label, LSnapshot *snapshot)
{
    JS_ASSERT(label->used() && !label->bound());
    if (!encode(snapshot))
        return false;

    
    
    
    JS_ASSERT_IF(frameClass_ != FrameSizeClass::None(),
                 frameClass_.frameSize() == masm.framePushed());
    
    
    
    
    
    
    
#if 0
    if (assignBailoutId(snapshot)) {
        uint8_t *code = deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE;
        masm.retarget(label, code, Relocation::HARDCODED);
        return true;
    }
#endif
    
    
    
    OutOfLineBailout *ool = new OutOfLineBailout(snapshot, masm.framePushed());
    if (!addOutOfLineCode(ool)) {
        return false;
    }

    masm.retarget(label, ool->entry());

    return true;
}

bool
CodeGeneratorARM::bailout(LSnapshot *snapshot)
{
    Label label;
    masm.ma_b(&label);
    return bailoutFrom(&label, snapshot);
}

bool
CodeGeneratorARM::visitOutOfLineBailout(OutOfLineBailout *ool)
{
    if (!deoptLabel_)
        deoptLabel_ = new HeapLabel();
    masm.ma_mov(Imm32(ool->snapshot()->snapshotOffset()), ScratchRegister);
    masm.ma_push(ScratchRegister);
    masm.ma_push(ScratchRegister);
    masm.ma_b(deoptLabel_);
    return true;
}

bool
CodeGeneratorARM::visitMinMaxD(LMinMaxD *ins)
{
    FloatRegister first = ToFloatRegister(ins->first());
    FloatRegister second = ToFloatRegister(ins->second());
    FloatRegister output = ToFloatRegister(ins->output());

    JS_ASSERT(first == output);

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
    masm.compareDouble(first, InvalidFloatReg);
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
    masm.loadStaticDouble(&js_NaN, output);
    masm.ma_b(&done);

    masm.bind(&returnSecond);
    masm.ma_vmov(second, output);

    masm.bind(&done);
    return true;
}

bool
CodeGeneratorARM::visitAbsD(LAbsD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(input == ToFloatRegister(ins->output()));
    masm.ma_vabs(input, input);
    return true;
}

bool
CodeGeneratorARM::visitSqrtD(LSqrtD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    JS_ASSERT(input == ToFloatRegister(ins->output()));
    masm.ma_vsqrt(input, input);
    return true;
}

bool
CodeGeneratorARM::visitAddI(LAddI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);

    if (rhs->isConstant())
        masm.ma_add(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest), SetCond);
    else
        masm.ma_add(ToRegister(lhs), ToOperand(rhs), ToRegister(dest), SetCond);

    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;

    return true;
}

bool
CodeGeneratorARM::visitSubI(LSubI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);

    if (rhs->isConstant())
        masm.ma_sub(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest), SetCond);
    else
        masm.ma_sub(ToRegister(lhs), ToOperand(rhs), ToRegister(dest), SetCond);

    if (ins->snapshot() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorARM::visitMulI(LMulI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);
    MMul *mul = ins->mir();
    JS_ASSERT_IF(mul->mode() == MMul::Integer, !mul->canBeNegativeZero() && !mul->canOverflow());

    if (rhs->isConstant()) {
        
        Assembler::Condition c = Assembler::Overflow;
        
        int32_t constant = ToInt32(rhs);
        if (mul->canBeNegativeZero() && constant <= 0) {
            Assembler::Condition bailoutCond = (constant == 0) ? Assembler::LessThan : Assembler::Equal;
            masm.ma_cmp(ToRegister(lhs), Imm32(0));
            if (!bailoutIf(bailoutCond, ins->snapshot()))
                return false;
        }
        
        switch (constant) {
          case -1:
            masm.ma_rsb(ToRegister(lhs), Imm32(0), ToRegister(dest), SetCond);
            break;
          case 0:
            masm.ma_mov(Imm32(0), ToRegister(dest));
            return true; 
          case 1:
            
            masm.ma_mov(ToRegister(lhs), ToRegister(dest));
            return true; 
          case 2:
            masm.ma_add(ToRegister(lhs), ToRegister(lhs), ToRegister(dest), SetCond);
            
            break;
          default: {
            bool handled = false;
            if (!mul->canOverflow()) {
                
                Register src = ToRegister(lhs);
                uint32_t shift;
                JS_FLOOR_LOG2(shift, constant);
                uint32_t rest = constant - (1 << shift);
                
                if ((1 << shift) == constant) {
                    masm.ma_lsl(Imm32(shift), src, ToRegister(dest));
                    handled = true;
                } else {
                    
                    
                    uint32_t shift_rest;
                    JS_FLOOR_LOG2(shift_rest, rest);
                    if ((1u << shift_rest) == rest) {
                        masm.as_add(ToRegister(dest), src, lsl(src, shift-shift_rest));
                        if (shift_rest != 0)
                            masm.ma_lsl(Imm32(shift_rest), ToRegister(dest), ToRegister(dest));
                        handled = true;
                    }
                }
            } else if (ToRegister(lhs) != ToRegister(dest)) {
                
                

                uint32_t shift;
                JS_FLOOR_LOG2(shift, constant);
                if ((1 << shift) == constant) {
                    
                    masm.ma_lsl(Imm32(shift), ToRegister(lhs), ToRegister(dest));
                    
                    
                    
                    masm.as_cmp(ToRegister(lhs), asr(ToRegister(dest), shift));
                    c = Assembler::NotEqual;
                    handled = true;
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
        
        if (mul->canOverflow() && !bailoutIf(c, ins->snapshot()))
            return false;
    } else {
        Assembler::Condition c = Assembler::Overflow;

        
        if (mul->canOverflow())
            c = masm.ma_check_mul(ToRegister(lhs), ToRegister(rhs), ToRegister(dest), c);
        else
            masm.ma_mul(ToRegister(lhs), ToRegister(rhs), ToRegister(dest));

        
        if (mul->canOverflow() && !bailoutIf(c, ins->snapshot()))
            return false;

        if (mul->canBeNegativeZero()) {
            Label done;
            masm.ma_cmp(ToRegister(dest), Imm32(0));
            masm.ma_b(&done, Assembler::NotEqual);

            
            masm.ma_cmn(ToRegister(lhs), ToRegister(rhs));
            if (!bailoutIf(Assembler::Signed, ins->snapshot()))
                return false;

            masm.bind(&done);
        }
    }

    return true;
}

extern "C" {
    extern int __aeabi_idivmod(int,int);
}

bool
CodeGeneratorARM::visitDivI(LDivI *ins)
{
    
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    MDiv *mir = ins->mir();

    Label done;

    if (mir->canBeNegativeOverflow()) {
        
        
        masm.ma_cmp(lhs, Imm32(INT32_MIN)); 
        masm.ma_cmp(rhs, Imm32(-1), Assembler::Equal); 
        if (mir->isTruncated()) {
            
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            masm.ma_mov(Imm32(INT32_MIN), r0);
            masm.ma_b(&done);
            masm.bind(&skip);
        } else {
            JS_ASSERT(mir->fallible());
            if (!bailoutIf(Assembler::Equal, ins->snapshot()))
                return false;
        }
    }

    
    
    
    

    
    
    
    
    
    
    
    
    if (mir->canBeDivideByZero() || mir->canBeNegativeZero()) {
        masm.ma_cmp(rhs, Imm32(0));
        masm.ma_cmp(lhs, Imm32(0), Assembler::LessThan);
        if (mir->isTruncated()) {
            
            Label skip;
            masm.ma_b(&skip, Assembler::NotEqual);
            masm.ma_mov(Imm32(0), r0);
            masm.ma_b(&done);
            masm.bind(&skip);
        } else {
            JS_ASSERT(mir->fallible());
            if (!bailoutIf(Assembler::Equal, ins->snapshot()))
                return false;
        }
    }
    masm.setupAlignedABICall(2);
    masm.passABIArg(lhs);
    masm.passABIArg(rhs);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, __aeabi_idivmod));
    
    if (!mir->isTruncated()) {
        JS_ASSERT(mir->fallible());
        masm.ma_cmp(r1, Imm32(0));
        if (!bailoutIf(Assembler::NonZero, ins->snapshot()))
            return false;
    }

    masm.bind(&done);

    return true;
}

bool
CodeGeneratorARM::visitModI(LModI *ins)
{
    
    Register lhs = ToRegister(ins->lhs());
    Register rhs = ToRegister(ins->rhs());
    Register callTemp = ToRegister(ins->getTemp(2));
    MMod *mir = ins->mir();
    Label done;
    
    JS_ASSERT(callTemp.code() > r3.code() && callTemp.code() < r12.code());
    masm.ma_mov(lhs, callTemp);
    
    
    masm.ma_cmp(lhs, Imm32(INT_MIN)); 
    masm.ma_cmp(rhs, Imm32(-1), Assembler::Equal); 
    if (mir->isTruncated()) {
        
        Label skip;
        masm.ma_b(&skip, Assembler::NotEqual);
        masm.ma_mov(Imm32(0), r1);
        masm.ma_b(&done);
        masm.bind(&skip);
    } else {
        JS_ASSERT(mir->fallible());
        if (!bailoutIf(Assembler::Equal, ins->snapshot()))
            return false;
    }
    
    
    
    

    
    
    
    
    
    
    
    
    masm.ma_cmp(rhs, Imm32(0));
    masm.ma_cmp(lhs, Imm32(0), Assembler::LessThan);
    if (mir->isTruncated()) {
        
        Label skip;
        masm.ma_b(&skip, Assembler::NotEqual);
        masm.ma_mov(Imm32(0), r1);
        masm.ma_b(&done);
        masm.bind(&skip);
    } else {
        JS_ASSERT(mir->fallible());
        if (!bailoutIf(Assembler::Equal, ins->snapshot()))
            return false;
    }
    masm.setupAlignedABICall(2);
    masm.passABIArg(lhs);
    masm.passABIArg(rhs);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, __aeabi_idivmod));
    
    
    masm.ma_cmp(r1, Imm32(0));
    if (mir->isTruncated()) {
        
    } else {
        JS_ASSERT(mir->fallible());
        masm.ma_b(&done, Assembler::NotEqual);
        masm.ma_cmp(callTemp, Imm32(0));
        if (!bailoutIf(Assembler::Signed, ins->snapshot()))
            return false;
    }
    masm.bind(&done);
    return true;
}

bool
CodeGeneratorARM::visitModPowTwoI(LModPowTwoI *ins)
{
    Register in = ToRegister(ins->getOperand(0));
    Register out = ToRegister(ins->getDef(0));
    MMod *mir = ins->mir();
    Label fin;
    
    masm.ma_mov(in, out, SetCond);
    masm.ma_b(&fin, Assembler::Zero);
    masm.ma_rsb(Imm32(0), out, NoSetCond, Assembler::Signed);
    masm.ma_and(Imm32((1<<ins->shift())-1), out);
    masm.ma_rsb(Imm32(0), out, SetCond, Assembler::Signed);
    if (!mir->isTruncated()) {
        JS_ASSERT(mir->fallible());
        if (!bailoutIf(Assembler::Zero, ins->snapshot()))
            return false;
    } else {
        
    }
    masm.bind(&fin);
    return true;
}

bool
CodeGeneratorARM::visitModMaskI(LModMaskI *ins)
{
    Register src = ToRegister(ins->getOperand(0));
    Register dest = ToRegister(ins->getDef(0));
    Register tmp = ToRegister(ins->getTemp(0));
    MMod *mir = ins->mir();
    masm.ma_mod_mask(src, dest, tmp, ins->shift());
    if (!mir->isTruncated()) {
        JS_ASSERT(mir->fallible());
        if (!bailoutIf(Assembler::Zero, ins->snapshot()))
            return false;
    } else {
        
    }
    return true;
}
bool
CodeGeneratorARM::visitBitNotI(LBitNotI *ins)
{
    const LAllocation *input = ins->getOperand(0);
    const LDefinition *dest = ins->getDef(0);
    
    
    
    JS_ASSERT(!input->isConstant());

    masm.ma_mvn(ToRegister(input), ToRegister(dest));
    return true;
}

bool
CodeGeneratorARM::visitBitOpI(LBitOpI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);
    
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
        JS_NOT_REACHED("unexpected binary opcode");
    }

    return true;
}

bool
CodeGeneratorARM::visitShiftI(LShiftI *ins)
{
    Register lhs = ToRegister(ins->lhs());
    const LAllocation *rhs = ins->rhs();
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
                if (ins->mir()->toUrsh()->canOverflow()) {
                    masm.ma_cmp(dest, Imm32(0));
                    if (!bailoutIf(Assembler::LessThan, ins->snapshot()))
                        return false;
                }
            }
            break;
          default:
            JS_NOT_REACHED("Unexpected shift op");
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
            if (ins->mir()->toUrsh()->canOverflow()) {
                
                masm.ma_cmp(dest, Imm32(0));
                if (!bailoutIf(Assembler::LessThan, ins->snapshot()))
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
CodeGeneratorARM::visitUrshD(LUrshD *ins)
{
    Register lhs = ToRegister(ins->lhs());
    Register temp = ToRegister(ins->temp());

    const LAllocation *rhs = ins->rhs();
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
    return true;
}

bool
CodeGeneratorARM::visitPowHalfD(LPowHalfD *ins)
{
    FloatRegister input = ToFloatRegister(ins->input());
    FloatRegister output = ToFloatRegister(ins->output());

    Label done;

    
    masm.ma_vimm(js_NegativeInfinity, ScratchFloatReg);
    masm.compareDouble(input, ScratchFloatReg);
    masm.ma_vneg(ScratchFloatReg, output, Assembler::Equal);
    masm.ma_b(&done, Assembler::Equal);

    
    masm.ma_vimm(0.0, ScratchFloatReg);
    masm.ma_vadd(ScratchFloatReg, input, output);
    masm.ma_vsqrt(output, output);

    masm.bind(&done);
    return true;
}

typedef MoveResolver::MoveOperand MoveOperand;

MoveOperand
CodeGeneratorARM::toMoveOperand(const LAllocation *a) const
{
    if (a->isGeneralReg())
        return MoveOperand(ToRegister(a));
    if (a->isFloatReg())
        return MoveOperand(ToFloatRegister(a));
    return MoveOperand(StackPointer, ToStackOffset(a));
}

bool
CodeGeneratorARM::visitMoveGroup(LMoveGroup *group)
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

class js::ion::OutOfLineTableSwitch : public OutOfLineCodeBase<CodeGeneratorARM>
{
    MTableSwitch *mir_;
    Vector<CodeLabel, 8, IonAllocPolicy> codeLabels_;

    bool accept(CodeGeneratorARM *codegen) {
        return codegen->visitOutOfLineTableSwitch(this);
    }

  public:
    OutOfLineTableSwitch(MTableSwitch *mir)
      : mir_(mir)
    {}

    MTableSwitch *mir() const {
        return mir_;
    }

    bool addCodeLabel(CodeLabel label) {
        return codeLabels_.append(label);
    }
    CodeLabel codeLabel(unsigned i) {
        return codeLabels_[i];
    }
};

bool
CodeGeneratorARM::visitOutOfLineTableSwitch(OutOfLineTableSwitch *ool)
{
    MTableSwitch *mir = ool->mir();

    int numCases = mir->numCases();
    for (size_t i = 0; i < numCases; i++) {
        LBlock *caseblock = mir->getCase(numCases - 1 - i)->lir();
        Label *caseheader = caseblock->label();
        uint32_t caseoffset = caseheader->offset();

        
        
        CodeLabel cl = ool->codeLabel(i);
        cl.src()->bind(caseoffset);
        if (!masm.addCodeLabel(cl))
            return false;
    }

    return true;
}

bool
CodeGeneratorARM::emitTableSwitchDispatch(MTableSwitch *mir, const Register &index,
                                          const Register &base)
{
    
    
    
    
    
    

    
    
    

    
    
    
    
    

    
    
    
    
    
    
    
    
    
    Label *defaultcase = mir->getDefault()->lir()->label();

    int32_t cases = mir->numCases();
    
    masm.ma_sub(index, Imm32(mir->low()), index, SetCond);
    masm.ma_rsb(index, Imm32(cases - 1), index, SetCond, Assembler::Unsigned);
    AutoForbidPools afp(&masm);
    masm.ma_ldr(DTRAddr(pc, DtrRegImmShift(index, LSL, 2)), pc, Offset, Assembler::Unsigned);
    masm.ma_b(defaultcase);

    
    
    
    OutOfLineTableSwitch *ool = new OutOfLineTableSwitch(mir);
    for (int32_t i = 0; i < cases; i++) {
        CodeLabel cl;
        masm.writeCodePointer(cl.dest());
        if (!ool->addCodeLabel(cl))
            return false;
    }
    if (!addOutOfLineCode(ool))
        return false;

    return true;
}

bool
CodeGeneratorARM::visitMathD(LMathD *math)
{
    const LAllocation *src1 = math->getOperand(0);
    const LAllocation *src2 = math->getOperand(1);
    const LDefinition *output = math->getDef(0);
    
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
        JS_NOT_REACHED("unexpected opcode");
        return false;
    }
    return true;
}

bool
CodeGeneratorARM::visitFloor(LFloor *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    Label bail;
    masm.floor(input, output, &bail);
    if (!bailoutFrom(&bail, lir->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorARM::visitRound(LRound *lir)
{
    FloatRegister input = ToFloatRegister(lir->input());
    Register output = ToRegister(lir->output());
    FloatRegister tmp = ToFloatRegister(lir->temp());
    Label bail;
    
    
    masm.round(input, output, &bail, tmp);
    if (!bailoutFrom(&bail, lir->snapshot()))
        return false;
    return true;
}

void
CodeGeneratorARM::emitRoundDouble(const FloatRegister &src, const Register &dest, Label *fail)
{
    masm.ma_vcvt_F64_I32(src, ScratchFloatReg);
    masm.ma_vxfer(ScratchFloatReg, dest);
    masm.ma_cmp(dest, Imm32(0x7fffffff));
    masm.ma_cmp(dest, Imm32(0x80000000), Assembler::NotEqual);
    masm.ma_b(fail, Assembler::Equal);
}

bool
CodeGeneratorARM::visitTruncateDToInt32(LTruncateDToInt32 *ins)
{
    return emitTruncateDouble(ToFloatRegister(ins->input()), ToRegister(ins->output()));
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
    JS_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);
    JS_ASSERT(class_ < JS_ARRAY_LENGTH(FrameSizes));

    return FrameSizes[class_];
}

ValueOperand
CodeGeneratorARM::ToValue(LInstruction *ins, size_t pos)
{
    Register typeReg = ToRegister(ins->getOperand(pos + TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getOperand(pos + PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

ValueOperand
CodeGeneratorARM::ToOutValue(LInstruction *ins)
{
    Register typeReg = ToRegister(ins->getDef(TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getDef(PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

ValueOperand
CodeGeneratorARM::ToTempValue(LInstruction *ins, size_t pos)
{
    Register typeReg = ToRegister(ins->getTemp(pos + TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getTemp(pos + PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

bool
CodeGeneratorARM::visitValue(LValue *value)
{
    const ValueOperand out = ToOutValue(value);

    masm.moveValue(value->value(), out);
    return true;
}

bool
CodeGeneratorARM::visitOsrValue(LOsrValue *value)
{
    const LAllocation *frame   = value->getOperand(0);
    const ValueOperand out     = ToOutValue(value);

    const ptrdiff_t frameOffset = value->mir()->frameOffset();

    masm.loadValue(Address(ToRegister(frame), frameOffset), out);
    return true;
}

bool
CodeGeneratorARM::visitBox(LBox *box)
{
    const LDefinition *type = box->getDef(TYPE_INDEX);

    JS_ASSERT(!box->getOperand(0)->isConstant());

    
    
    
    masm.ma_mov(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
    return true;
}

bool
CodeGeneratorARM::visitBoxDouble(LBoxDouble *box)
{
    const LDefinition *payload = box->getDef(PAYLOAD_INDEX);
    const LDefinition *type = box->getDef(TYPE_INDEX);
    const LAllocation *in = box->getOperand(0);

    
    
    masm.ma_vxfer(VFPRegister(ToFloatRegister(in)), ToRegister(payload), ToRegister(type));
    return true;
}

bool
CodeGeneratorARM::visitUnbox(LUnbox *unbox)
{
    
    
    MUnbox *mir = unbox->mir();
    Register type = ToRegister(unbox->type());

    if (mir->fallible()) {
        masm.ma_cmp(type, Imm32(MIRTypeToTag(mir->type())));
        if (!bailoutIf(Assembler::NotEqual, unbox->snapshot()))
            return false;
    }
    return true;
}

bool
CodeGeneratorARM::visitDouble(LDouble *ins)
{

    const LDefinition *out = ins->getDef(0);
    const LConstantIndex *cindex = ins->getOperand(0)->toConstantIndex();
    const Value &v = graph.getConstant(cindex->index());

    masm.ma_vimm(v.toDouble(), ToFloatRegister(out));
    return true;
#if 0
    DeferredDouble *d = new DeferredDouble(cindex->index());
    if (!deferredDoubles_.append(d))
        return false;

    masm.movsd(d->label(), ToFloatRegister(out));
    return true;
#endif
}

Register
CodeGeneratorARM::splitTagForTest(const ValueOperand &value)
{
    return value.typeReg();
}

bool
CodeGeneratorARM::visitTestDAndBranch(LTestDAndBranch *test)
{
    const LAllocation *opd = test->input();
    masm.ma_vcmpz(ToFloatRegister(opd));
    masm.as_vmrs(pc);

    LBlock *ifTrue = test->ifTrue()->lir();
    LBlock *ifFalse = test->ifFalse()->lir();
    
    
    masm.ma_b(ifFalse->label(), Assembler::Zero);
    
    
    masm.ma_b(ifFalse->label(), Assembler::Overflow);
    if (!isNextBlock(ifTrue))
        masm.ma_b(ifTrue->label());
    return true;
}

bool
CodeGeneratorARM::visitCompareD(LCompareD *comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());
    masm.compareDouble(lhs, rhs);
    masm.emitSet(Assembler::ConditionFromDoubleCondition(cond), ToRegister(comp->output()));
    return true;
}

bool
CodeGeneratorARM::visitCompareDAndBranch(LCompareDAndBranch *comp)
{
    FloatRegister lhs = ToFloatRegister(comp->left());
    FloatRegister rhs = ToFloatRegister(comp->right());

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(comp->mir()->jsop());
    masm.compareDouble(lhs, rhs);
    emitBranch(Assembler::ConditionFromDoubleCondition(cond), comp->ifTrue(), comp->ifFalse());
    return true;
}

bool
CodeGeneratorARM::visitCompareB(LCompareB *lir)
{
    MCompare *mir = lir->mir();

    const ValueOperand lhs = ToValue(lir, LCompareB::Lhs);
    const LAllocation *rhs = lir->rhs();
    const Register output = ToRegister(lir->output());

    JS_ASSERT(mir->jsop() == JSOP_STRICTEQ || mir->jsop() == JSOP_STRICTNE);

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
    return true;
}

bool
CodeGeneratorARM::visitCompareBAndBranch(LCompareBAndBranch *lir)
{
    MCompare *mir = lir->mir();
    const ValueOperand lhs = ToValue(lir, LCompareBAndBranch::Lhs);
    const LAllocation *rhs = lir->rhs();

    JS_ASSERT(mir->jsop() == JSOP_STRICTEQ || mir->jsop() == JSOP_STRICTNE);

    if (mir->jsop() == JSOP_STRICTEQ)
        masm.branchTestBoolean(Assembler::NotEqual, lhs, lir->ifFalse()->lir()->label());
    else
        masm.branchTestBoolean(Assembler::NotEqual, lhs, lir->ifTrue()->lir()->label());

    if (rhs->isConstant())
        masm.cmp32(lhs.payloadReg(), Imm32(rhs->toConstant()->toBoolean()));
    else
        masm.cmp32(lhs.payloadReg(), ToRegister(rhs));
    emitBranch(JSOpToCondition(mir->compareType(), mir->jsop()), lir->ifTrue(), lir->ifFalse());
    return true;
}

bool
CodeGeneratorARM::visitCompareV(LCompareV *lir)
{
    MCompare *mir = lir->mir();
    Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
    const ValueOperand lhs = ToValue(lir, LCompareV::LhsInput);
    const ValueOperand rhs = ToValue(lir, LCompareV::RhsInput);
    const Register output = ToRegister(lir->output());

    JS_ASSERT(mir->jsop() == JSOP_EQ || mir->jsop() == JSOP_STRICTEQ ||
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
    return true;
}

bool
CodeGeneratorARM::visitCompareVAndBranch(LCompareVAndBranch *lir)
{
    MCompare *mir = lir->mir();
    Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
    const ValueOperand lhs = ToValue(lir, LCompareVAndBranch::LhsInput);
    const ValueOperand rhs = ToValue(lir, LCompareVAndBranch::RhsInput);

    JS_ASSERT(mir->jsop() == JSOP_EQ || mir->jsop() == JSOP_STRICTEQ ||
              mir->jsop() == JSOP_NE || mir->jsop() == JSOP_STRICTNE);

    Label *notEqual;
    if (cond == Assembler::Equal)
        notEqual = lir->ifFalse()->lir()->label();
    else
        notEqual = lir->ifTrue()->lir()->label();

    masm.cmp32(lhs.typeReg(), rhs.typeReg());
    masm.j(Assembler::NotEqual, notEqual);
    masm.cmp32(lhs.payloadReg(), rhs.payloadReg());
    emitBranch(cond, lir->ifTrue(), lir->ifFalse());

    return true;
}

bool
CodeGeneratorARM::visitUInt32ToDouble(LUInt32ToDouble *lir)
{
    masm.convertUInt32ToDouble(ToRegister(lir->input()), ToFloatRegister(lir->output()));
    return true;
}

bool
CodeGeneratorARM::visitNotI(LNotI *ins)
{
    
    masm.ma_cmp(ToRegister(ins->input()), Imm32(0));
    masm.emitSet(Assembler::Equal, ToRegister(ins->output()));
    return true;
}

bool
CodeGeneratorARM::visitNotD(LNotD *ins)
{
    
    
    
    
    FloatRegister opd = ToFloatRegister(ins->input());
    Register dest = ToRegister(ins->output());

    
    masm.ma_vcmpz(opd);
    bool nocond = true;
    if (nocond) {
        
        masm.as_vmrs(dest);
        masm.ma_lsr(Imm32(28), dest, dest);
        masm.ma_alu(dest, lsr(dest, 2), dest, op_orr); 
        masm.ma_and(Imm32(1), dest);
    } else {
        masm.as_vmrs(pc);
        masm.ma_mov(Imm32(0), dest);
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::Equal);
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::Overflow);
#if 0
        masm.as_vmrs(ToRegister(dest));
        
        
        masm.ma_and(Imm32(0x50000000), dest, dest, Assembler::SetCond);
        
        masm.ma_mov(Imm32(1), dest, NoSetCond, Assembler::NotEqual);
#endif
    }
    return true;
}

bool
CodeGeneratorARM::visitLoadSlotV(LLoadSlotV *load)
{
    const ValueOperand out = ToOutValue(load);
    Register base = ToRegister(load->input());
    int32_t offset = load->mir()->slot() * sizeof(js::Value);

    masm.loadValue(Address(base, offset), out);
    return true;
}

bool
CodeGeneratorARM::visitLoadSlotT(LLoadSlotT *load)
{
    Register base = ToRegister(load->input());
    int32_t offset = load->mir()->slot() * sizeof(js::Value);

    if (load->mir()->type() == MIRType_Double)
        masm.loadInt32OrDouble(Operand(base, offset), ToFloatRegister(load->output()));
    else
        masm.ma_ldr(Operand(base, offset + NUNBOX32_PAYLOAD_OFFSET), ToRegister(load->output()));
    return true;
}

bool
CodeGeneratorARM::visitStoreSlotT(LStoreSlotT *store)
{

    Register base = ToRegister(store->slots());
    int32_t offset = store->mir()->slot() * sizeof(js::Value);

    const LAllocation *value = store->value();
    MIRType valueType = store->mir()->value()->type();

    if (store->mir()->needsBarrier())
        emitPreBarrier(Address(base, offset), store->mir()->slotType());

    if (valueType == MIRType_Double) {
        masm.ma_vstr(ToFloatRegister(value), Operand(base, offset));
        return true;
    }

    
    if (valueType != store->mir()->slotType())
        masm.storeTypeTag(ImmType(ValueTypeFromMIRType(valueType)), Operand(base, offset));

    
    if (value->isConstant())
        masm.storePayload(*value->toConstant(), Operand(base, offset));
    else
        masm.storePayload(ToRegister(value), Operand(base, offset));

    return true;
}

bool
CodeGeneratorARM::visitLoadElementT(LLoadElementT *load)
{
    Register base = ToRegister(load->elements());
    if (load->mir()->type() == MIRType_Double) {
        FloatRegister fpreg = ToFloatRegister(load->output());
        if (load->index()->isConstant()) {
            Address source(base, ToInt32(load->index()) * sizeof(Value));
            if (load->mir()->loadDoubles())
                masm.loadDouble(source, fpreg);
            else
                masm.loadInt32OrDouble(source, fpreg);
        } else {
            Register index = ToRegister(load->index());
            if (load->mir()->loadDoubles())
                masm.loadDouble(BaseIndex(base, index, TimesEight), fpreg);
            else
                masm.loadInt32OrDouble(base, index, fpreg);
        }
    } else {
        if (load->index()->isConstant()) {
            Address source(base, ToInt32(load->index()) * sizeof(Value));
            masm.load32(source, ToRegister(load->output()));
        } else {
            masm.ma_ldr(DTRAddr(base, DtrRegImmShift(ToRegister(load->index()), LSL, 3)),
                        ToRegister(load->output()));
        }
    }
    JS_ASSERT(!load->mir()->needsHoleCheck());
    return true;
}

void
CodeGeneratorARM::storeElementTyped(const LAllocation *value, MIRType valueType, MIRType elementType,
                                    const Register &elements, const LAllocation *index)
{
    if (index->isConstant()) {
        Address dest = Address(elements, ToInt32(index) * sizeof(Value));
        if (valueType == MIRType_Double) {
            masm.ma_vstr(ToFloatRegister(value), Operand(dest));
            return;
        }

        
        if (valueType != elementType)
            masm.storeTypeTag(ImmType(ValueTypeFromMIRType(valueType)), dest);

        
        if (value->isConstant())
            masm.storePayload(*value->toConstant(), dest);
        else
            masm.storePayload(ToRegister(value), dest);
    } else {
        Register indexReg = ToRegister(index);
        if (valueType == MIRType_Double) {
            masm.ma_vstr(ToFloatRegister(value), elements, indexReg);
            return;
        }

        
        if (valueType != elementType)
            masm.storeTypeTag(ImmType(ValueTypeFromMIRType(valueType)), elements, indexReg);

        
        if (value->isConstant())
            masm.storePayload(*value->toConstant(), elements, indexReg);
        else
            masm.storePayload(ToRegister(value), elements, indexReg);
    }
}

bool
CodeGeneratorARM::visitGuardShape(LGuardShape *guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());
    masm.ma_ldr(DTRAddr(obj, DtrOffImm(JSObject::offsetOfShape())), tmp);
    masm.ma_cmp(tmp, ImmGCPtr(guard->mir()->shape()));

    if (!bailoutIf(Assembler::NotEqual, guard->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorARM::visitGuardClass(LGuardClass *guard)
{
    Register obj = ToRegister(guard->input());
    Register tmp = ToRegister(guard->tempInt());

    masm.loadObjClass(obj, tmp);
    masm.ma_cmp(tmp, Imm32((uint32_t)guard->mir()->getClass()));
    if (!bailoutIf(Assembler::NotEqual, guard->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorARM::visitImplicitThis(LImplicitThis *lir)
{
    Register callee = ToRegister(lir->callee());
    const ValueOperand out = ToOutValue(lir);

    
    
    masm.ma_ldr(DTRAddr(callee, DtrOffImm(JSFunction::offsetOfEnvironment())), out.typeReg());
    masm.ma_cmp(out.typeReg(), ImmGCPtr(&gen->info().script()->global()));

    
    if (!bailoutIf(Assembler::NotEqual, lir->snapshot()))
        return false;

    masm.moveValue(UndefinedValue(), out);
    return true;
}

typedef bool (*InterruptCheckFn)(JSContext *);
static const VMFunction InterruptCheckInfo = FunctionInfo<InterruptCheckFn>(InterruptCheck);

bool
CodeGeneratorARM::visitInterruptCheck(LInterruptCheck *lir)
{
    OutOfLineCode *ool = oolCallVM(InterruptCheckInfo, lir, (ArgList()), StoreNothing());
    if (!ool)
        return false;

    void *interrupt = (void*)&gen->compartment->rt->interrupt;
    masm.load32(AbsoluteAddress(interrupt), lr);
    masm.ma_cmp(lr, Imm32(0));
    masm.ma_b(ool->entry(), Assembler::NonZero);
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGeneratorARM::generateInvalidateEpilogue()
{
    
    
    
    for (size_t i = 0; i < sizeof(void *); i+= Assembler::nopSize())
        masm.nop();

    masm.bind(&invalidate_);

    
    masm.Push(lr);

    
    invalidateEpilogueData_ = masm.pushWithPatch(ImmWord(uintptr_t(-1)));
    IonCode *thunk = GetIonContext()->compartment->ionCompartment()->getInvalidationThunk();

    masm.branch(thunk);

    
    
    masm.breakpoint();
    return true;
}
