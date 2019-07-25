








































#include "CodeGenerator-arm.h"
#include "ion/shared/CodeGenerator-shared-inl.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "jsnum.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/IonFrames.h"
#include "ion/MoveEmitter.h"
#include "ion/IonCompartment.h"

using namespace js;
using namespace js::ion;


CodeGeneratorARM::CodeGeneratorARM(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorShared(gen, graph),
    deoptLabel_(NULL)
{
}

bool
CodeGeneratorARM::generatePrologue()
{
    
    masm.reserveStack(frameSize());

    
    
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
CodeGeneratorARM::visitGoto(LGoto *jump)
{
    LBlock *target = jump->target()->lir();

    
    if (isNextBlock(target))
        return true;

    masm.ma_b(target->label());
    return true;
}

bool
CodeGeneratorARM::visitTestIAndBranch(LTestIAndBranch *test)
{
    const LAllocation *opd = test->getOperand(0);
    LBlock *ifTrue = test->ifTrue()->lir();
    LBlock *ifFalse = test->ifFalse()->lir();

    
    masm.ma_cmp(Imm32(0), ToRegister(opd));

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
CodeGeneratorARM::visitCompareI(LCompareI *comp)
{
    const LAllocation *left = comp->getOperand(0);
    const LAllocation *right = comp->getOperand(1);
    const LDefinition *def = comp->getDef(0);

    masm.ma_cmp(ToRegister(left), ToOperand(right));
    masm.ma_mov(Imm32(1), ToRegister(def));
    masm.ma_mov(Imm32(0), ToRegister(def),
                NoSetCond, Assembler::NotEqual);
    return true;
}

static inline Assembler::Condition
JSOpToCondition(JSOp op)
{
    switch (op) {
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

bool
CodeGeneratorARM::visitCompareIAndBranch(LCompareIAndBranch *comp)
{
#if 0
    const LAllocation *left = comp->getOperand(0);
    const LAllocation *right = comp->getOperand(1);
    LBlock *ifTrue = comp->ifTrue()->lir();
    LBlock *ifFalse = comp->ifFalse()->lir();
    Assembler::Condition cond = comp->condition();

    
    masm.ma_cmp(ToRegister(left), ToOperand(right));

    
    if (isNextBlock(ifFalse)) {
        masm.ma_b(ifTrue->label(), cond);
    } else if (isNextBlock(ifTrue)) {
        masm.ma_b(ifFalse->label(), Assembler::inverseCondition(cond));
    } else {
        masm.ma_b(ifTrue->label(), cond);
        masm.ma_b(ifFalse->label(), Assembler::Always);
    }
    return true;
#endif
    Assembler::Condition cond = JSOpToCondition(comp->jsop());
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

        
        masm.ma_mov(Imm32(frameSize()), ScratchRegister);
        masm.ma_push(ScratchRegister);

        IonCompartment *ion = gen->cx->compartment->ionCompartment();
        IonCode *handler = ion->getGenericBailoutHandler(gen->cx);
        if (!handler)
            return false;

        masm.ma_b(handler->raw(), Relocation::CODE);
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
        uint8 *code = deoptTable_->raw() + snapshot->bailoutId() * BAILOUT_TABLE_ENTRY_SIZE;
        masm.ma_b(code, condition, Relocation::EXTERNAL);
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
    JS_NOT_REACHED("Feature NYI");
    JS_ASSERT(label->used() && !label->bound());
    
    return false;
}

bool
CodeGeneratorARM::visitOutOfLineBailout(OutOfLineBailout *ool)
{
    masm.bind(ool->entry());

    if (!deoptLabel_)
        deoptLabel_ = new HeapLabel();
    masm.ma_mov(Imm32(ool->snapshot()->snapshotOffset()), ScratchRegister);
    masm.ma_push(ScratchRegister);
    masm.ma_b(deoptLabel_);
    return true;
}

bool
CodeGeneratorARM::visitAddI(LAddI *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);

    if (rhs->isConstant()) {
        masm.ma_add(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest));
    } else {
        masm.ma_add(ToRegister(lhs), ToOperand(rhs), ToRegister(dest));
    }

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

    if (rhs->isConstant()) {
        masm.ma_sub(ToRegister(lhs), Imm32(ToInt32(rhs)), ToRegister(dest));
    } else {
        masm.ma_sub(ToRegister(lhs), ToOperand(rhs), ToRegister(dest));
    }

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

    if (rhs->isConstant()) {
        
        int32 constant = ToInt32(rhs);
        if (mul->canBeNegativeZero() && constant <= 0) {
            Assembler::Condition bailoutCond = (constant == 0) ? Assembler::LessThan : Assembler::Equal;
            masm.ma_cmp(Imm32(0), ToRegister(lhs));
            if (bailoutIf(bailoutCond, ins->snapshot()))
                    return false;
        }

        switch (constant) {
          case -1:
              masm.ma_rsb(ToRegister(lhs), Imm32(0), ToRegister(dest));
            break;
          case 0:
              masm.ma_mov(Imm32(0), ToRegister(dest));
            return true; 
          case 1:
            
            return true; 
          case 2:
              masm.ma_lsl(Imm32(1), ToRegister(lhs), ToRegister(dest));
            break;
          default:
            if (!mul->canOverflow() && constant > 0) {
                
                int32 shift;
                JS_FLOOR_LOG2(shift, constant);
                if ((1 << shift) == constant) {
                    masm.ma_lsl(Imm32(shift), ToRegister(lhs), ToRegister(dest));
                    return true;
                }
            } else if (!mul->canOverflow()) {
                int32 shift;
                JS_FLOOR_LOG2(shift, -constant);
                if ((1<<shift) == -constant) {
                    
                    
                    
                }
            }
            
            JS_NOT_REACHED("need to implement emitinst for mul/mull");
        }

        
        if (mul->canOverflow() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
            return false;
    } else {
        
        JS_NOT_REACHED("need to implement emitinst for mul/mull");

        
        if (mul->canOverflow() && !bailoutIf(Assembler::Overflow, ins->snapshot()))
            return false;

        
        if (mul->canBeNegativeZero()) {
            masm.ma_cmp(Imm32(0), ToRegister(lhs));
            if (!bailoutIf(Assembler::Zero, ins->snapshot()))
                return false;
        }
    }

    return true;
}

bool
CodeGeneratorARM::visitDivI(LDivI *ins)
{
    JS_NOT_REACHED("codegen for DIVI NYI");
#if 0
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
    masm.ma_b(&notmin, Assembler::NotEqual);
    masm.cmpl(rhs, Imm32(-1));
    if (!bailoutIf(Assembler::Equal, ins->snapshot()))
        return false;
    masm.bind(&notmin);

    
    Label nonzero;
    masm.testl(lhs, lhs);
    masm.ma_b(&nonzero, Assembler::NonZero);
    masm.cmpl(rhs, Imm32(0));
    if (!bailoutIf(Assembler::LessThan, ins->snapshot()))
        return false;
    masm.bind(&nonzero);

    
    masm.cdq();
    masm.idiv(rhs);

    
    masm.testl(remainder, remainder);
    if (!bailoutIf(Assembler::NonZero, ins->snapshot()))
        return false;

#endif
    return true;
}

bool
CodeGeneratorARM::visitBitNot(LBitNot *ins)
{
    const LAllocation *input = ins->getOperand(0);
    const LDefinition *dest = ins->getDef(0);
    
    
    
    JS_ASSERT(!input->isConstant());

    masm.ma_mvn(ToRegister(input), ToRegister(dest));
    return true;
}

bool
CodeGeneratorARM::visitBitOp(LBitOp *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);
    
    switch (ins->bitop()) {
      case JSOP_BITOR:
        if (rhs->isConstant()) {
            masm.ma_orr(Imm32(ToInt32(rhs)), ToRegister(lhs), ToRegister(dest));
        } else {
            masm.ma_orr(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
        }
        break;
      case JSOP_BITXOR:
        if (rhs->isConstant()) {
            masm.ma_eor(Imm32(ToInt32(rhs)), ToRegister(lhs), ToRegister(dest));
        } else {
            masm.ma_eor(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
        }
        break;
      case JSOP_BITAND:
        if (rhs->isConstant()) {
            masm.ma_and(Imm32(ToInt32(rhs)), ToRegister(lhs), ToRegister(dest));
        } else {
            masm.ma_and(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
        }
        break;
      default:
        JS_NOT_REACHED("unexpected binary opcode");
    }

    return true;
}

bool
CodeGeneratorARM::visitShiftOp(LShiftOp *ins)
{
    const LAllocation *lhs = ins->getOperand(0);
    const LAllocation *rhs = ins->getOperand(1);
    const LDefinition *dest = ins->getDef(0);
    
    
    
    switch (ins->bitop()) {
        case JSOP_LSH:
          if (rhs->isConstant()) {
                masm.ma_lsl(Imm32(ToInt32(rhs) & 0x1F), ToRegister(lhs), ToRegister(dest));
          } else {
                masm.ma_lsl(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
          }
            break;
        case JSOP_RSH:
          if (rhs->isConstant()) {
                masm.ma_asr(Imm32(ToInt32(rhs) & 0x1F), ToRegister(lhs), ToRegister(dest));
          } else {
                masm.ma_asr(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
          }
            break;
        case JSOP_URSH: {
            MUrsh *ursh = ins->mir()->toUrsh();
            if (rhs->isConstant()) {
                masm.ma_lsr(Imm32(ToInt32(rhs) & 0x1F), ToRegister(lhs), ToRegister(dest));
            } else {
                masm.ma_lsr(ToRegister(rhs), ToRegister(lhs), ToRegister(dest));
            }

            
            
            
            
            
            
            
            if (ursh->canOverflow()) {
                masm.ma_cmp(Imm32(0), ToRegister(lhs));
                if (!bailoutIf(Assembler::LessThan, ins->snapshot())) {
                    return false;
                }
            }
            break;
        }
        default:
            JS_NOT_REACHED("unexpected shift opcode");
    }

    return true;
}

bool
CodeGeneratorARM::visitInteger(LInteger *ins)
{
    const LDefinition *def = ins->getDef(0);
    masm.ma_mov(Imm32(ins->getValue()), ToRegister(def));
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

        if (!resolver.addMove(toMoveOperand(from), toMoveOperand(to), kind)) {
            return false;
        }
    }

    if (!resolver.resolve())
        return false;

    MoveEmitter emitter(masm);
    emitter.emit(resolver);
    emitter.finish();

    return true;
}

bool
CodeGeneratorARM::visitTableSwitch(LTableSwitch *ins)
{
#if 0
    MTableSwitch *mir = ins->mir();
    const LAllocation *input = ins->getOperand(0);

    
    LDefinition *index = ins->getTemp(0);
    masm.mov(ToOperand(input), ToRegister(index));

    
    if (mir->low() != 0)
        masm.subl(Imm32(mir->low()), ToOperand(index));

    
    LBlock *defaultcase = mir->getDefault()->lir();
    int32 cases = mir->numCases();
    masm.cmpl(Imm32(cases), ToRegister(index));
    masm.ma_b(defaultcase->label(), Assembler::AboveOrEqual);

    
    
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
#endif
    JS_NOT_REACHED("what the deuce are tables");
    return false;
}

bool
CodeGeneratorARM::visitMathD(LMathD *math)
{
    const LAllocation *src1 = math->getOperand(1);
    const LAllocation *src2 = math->getOperand(0);
    const LDefinition *output = math->getDef(0);
    
    switch (math->jsop()) {
      case JSOP_ADD:
          masm.ma_vadd(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
        break;
      case JSOP_MUL:
          masm.ma_vmul(ToFloatRegister(src1), ToFloatRegister(src2), ToFloatRegister(output));
      default:
        JS_NOT_REACHED("unexpected opcode");
        return false;
    }
    return true;
}




bool
CodeGeneratorARM::emitDoubleToInt32(const FloatRegister &src, const Register &dest, Label *fail)
{
    
    
    
    
    masm.ma_vcvt_F64_I32(src, ScratchFloatReg);
    
    masm.ma_vmov(ScratchFloatReg, dest);
    masm.ma_vcvt_I32_F64(ScratchFloatReg, ScratchFloatReg);
    masm.ma_vcmp_F64(ScratchFloatReg, src);
    
    masm.ma_b(fail, Assembler::NotEqual_Unordered);
    
    return true;
}
    
    
    
    
    
    

void
CodeGeneratorARM::emitTruncateDouble(const FloatRegister &src, const Register &dest, Label *fail)
{
    JS_NOT_REACHED("truncate Double NYI");
}




static const uint32 LAST_FRAME_SIZE = 512;
static const uint32 LAST_FRAME_INCREMENT = 512;
static const uint32 FrameSizes[] = { 128, 256, LAST_FRAME_SIZE };

FrameSizeClass
FrameSizeClass::FromDepth(uint32 frameDepth)
{
    for (uint32 i = 0; i < JS_ARRAY_LENGTH(FrameSizes); i++) {
        if (frameDepth < FrameSizes[i])
            return FrameSizeClass(i);
    }

    uint32 newFrameSize = frameDepth - LAST_FRAME_SIZE;
    uint32 sizeClass = (newFrameSize / LAST_FRAME_INCREMENT) + 1;

    return FrameSizeClass(JS_ARRAY_LENGTH(FrameSizes) + sizeClass);
}
uint32
FrameSizeClass::frameSize() const
{
    JS_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);

    if (class_ < JS_ARRAY_LENGTH(FrameSizes))
        return FrameSizes[class_];

    uint32 step = class_ - JS_ARRAY_LENGTH(FrameSizes);
    return LAST_FRAME_SIZE + step * LAST_FRAME_INCREMENT;
}

ValueOperand
CodeGeneratorARM::ToValue(LInstruction *ins, size_t pos)
{
    Register typeReg = ToRegister(ins->getOperand(pos + TYPE_INDEX));
    Register payloadReg = ToRegister(ins->getOperand(pos + PAYLOAD_INDEX));
    return ValueOperand(typeReg, payloadReg);
}

bool
CodeGeneratorARM::visitValue(LValue *value)
{
    jsval_layout jv = JSVAL_TO_IMPL(value->value());

    LDefinition *type = value->getDef(TYPE_INDEX);
    LDefinition *payload = value->getDef(PAYLOAD_INDEX);

    masm.ma_mov(Imm32(jv.s.tag), ToRegister(type));
    if (value->value().isMarkable()) {
        masm.ma_mov(ImmGCPtr((gc::Cell *)jv.s.payload.ptr), ToRegister(payload));
    } else {
        masm.ma_mov(Imm32(jv.s.payload.u32), ToRegister(payload));
    }
    return true;
}

static inline JSValueTag
MIRTypeToTag(MIRType type)
{
    switch (type) {
      case MIRType_Boolean:
        return JSVAL_TAG_BOOLEAN;
      case MIRType_Int32:
        return JSVAL_TAG_INT32;
      case MIRType_String:
        return JSVAL_TAG_STRING;
      case MIRType_Object:
        return JSVAL_TAG_OBJECT;
      default:
        JS_NOT_REACHED("no payload...");
    }
    return JSVAL_TAG_NULL;
}

bool
CodeGeneratorARM::visitBox(LBox *box)
{
    const LAllocation *a = box->getOperand(0);
    const LDefinition *type = box->getDef(TYPE_INDEX);

    JS_ASSERT(!a->isConstant());

    
    
    
    
    masm.ma_mov(Imm32(MIRTypeToTag(box->type())), ToRegister(type));
    return true;
}

bool
CodeGeneratorARM::visitBoxDouble(LBoxDouble *box)
{
    const LDefinition *payload = box->getDef(PAYLOAD_INDEX);
    const LDefinition *type = box->getDef(TYPE_INDEX);
    const LAllocation *in = box->getOperand(0);

    masm.as_vxfer(ToRegister(payload), ToRegister(type),
                  VFPRegister(ToFloatRegister(in)), Assembler::FloatToCore);
    return true;
}

bool
CodeGeneratorARM::visitUnbox(LUnbox *unbox)
{
    LAllocation *type = unbox->getOperand(TYPE_INDEX);
    masm.ma_cmp(Imm32(MIRTypeToTag(unbox->type())), ToRegister(type));
    if (!bailoutIf(Assembler::NotEqual, unbox->snapshot()))
        return false;
    return true;
}

bool
CodeGeneratorARM::visitReturn(LReturn *ret)
{

#ifdef DEBUG
    LAllocation *type = ret->getOperand(TYPE_INDEX);
    LAllocation *payload = ret->getOperand(PAYLOAD_INDEX);

    JS_ASSERT(ToRegister(type) == JSReturnReg_Type);
    JS_ASSERT(ToRegister(payload) == JSReturnReg_Data);
#endif
    
    if (current->mir() != *gen->graph().poBegin())
        masm.ma_b(returnLabel_);
    return true;
}

void
CodeGeneratorARM::linkAbsoluteLabels()
{
    IonCode *method = gen->script->ion->method();

    for (size_t i = 0; i < deferredDoubles_.length(); i++) {
        DeferredDouble *d = deferredDoubles_[i];
        const Value &v = gen->script->ion->getConstant(d->index());
        MacroAssembler::Bind(method, d->label(), &v);
    }
}

bool
CodeGeneratorARM::visitDouble(LDouble *ins)
{

    const LDefinition *out = ins->getDef(0);
    const LConstantIndex *cindex = ins->getOperand(0)->toConstantIndex();
    const Value &v = graph.getConstant(cindex->index());

    jsdpun dpun;
    dpun.d = v.toDouble();
    if ((dpun.u64 & 0xffffffff) == 0) {
        VFPImm dblEnc(dpun.u64 >> 32);
        if (dblEnc.isValid()) {
            masm.as_vimm(ToFloatRegister(out), dblEnc);
            return true;
        }

    }
    JS_NOT_REACHED("immediate NYI");
    return false;
#if 0
    DeferredDouble *d = new DeferredDouble(cindex->index());
    if (!deferredDoubles_.append(d))
        return false;

    masm.movsd(d->label(), ToFloatRegister(out));
    return true;
#endif
}

bool
CodeGeneratorARM::visitUnboxDouble(LUnboxDouble *ins)
{
    const ValueOperand box = ToValue(ins, LUnboxDouble::Input);
    const LDefinition *result = ins->output();

    Assembler::Condition cond = masm.testDouble(Assembler::NotEqual, box);
    if (!bailoutIf(cond, ins->snapshot()))
        return false;
    masm.unboxDouble(box, ToFloatRegister(result));
    return true;
}
Register
CodeGeneratorARM::splitTagForTest(const ValueOperand &value)
{
    return value.typeReg();
}
Assembler::Condition
CodeGeneratorARM::testStringTruthy(bool truthy, const ValueOperand &value)
{
    Register string = value.payloadReg();
    

    size_t mask = (0xFFFFFFFF << JSString::LENGTH_SHIFT);
    masm.ma_dtr(IsLoad, string, Imm32(JSString::offsetOfLengthAndFlags()), ScratchRegister);
    masm.ma_tst(Imm32(mask), ScratchRegister);
    return truthy ? Assembler::NonZero : Assembler::Zero;
}


bool
CodeGeneratorARM::visitCompareD(LCompareD *comp)
{
    JS_NOT_REACHED("Codegen for CompareD NYI");
    return false;
}

bool
CodeGeneratorARM::visitCompareDAndBranch(LCompareDAndBranch *comp)
{
    JS_NOT_REACHED("Codegen for CompareDAndBranch NYI");
    return false;
}


bool
CodeGeneratorARM::visitStackArg(LStackArg *arg)
{
    ValueOperand val = ToValue(arg, 0);
    uint32 argslot = arg->argslot();
    int32 stack_offset = StackOffsetOfPassedArg(argslot);

    masm.ma_str(val.typeReg(), DTRAddr(StackPointer, DtrOffImm(stack_offset)));
    masm.ma_str(val.payloadReg(), DTRAddr(StackPointer, DtrOffImm(stack_offset+4)));
    return true;
}

bool
CodeGeneratorARM::visitCallGeneric(LCallGeneric *call)
{

    
    const LAllocation *obj = call->getFunction();
    Register objreg  = ToRegister(obj);

    
    const LAllocation *tok = call->getToken();
    Register tokreg  = ToRegister(tok);

    
    const LAllocation *nargs = call->getNargsReg();
    Register nargsreg = ToRegister(nargs);

    uint32 callargslot  = call->argslot();
    uint32 unused_stack = StackOffsetOfPassedArg(callargslot);


#if 0
    
    masm.ma_ldr(DTRAddr(objreg, DtrOffImm(JSObject::offsetOfClassPointer())), tokreg);
    masm.ma_ldr(DTRAddr(tokreg, DtrOffImm(0)), tokreg);

    masm.cmpPtr(tokreg, ImmWord(&js::FunctionClass));
    if (!bailoutIf(Assembler::NotEqual, call->snapshot()))
        return false;

    
    masm.movePtr(Operand(objreg, offsetof(JSObject, privateData)), objreg);

    
    
    masm.movl(Operand(objreg, offsetof(JSFunction, flags)), tokreg);
    masm.andl(Imm32(JSFUN_KINDMASK), tokreg);
    masm.cmpl(tokreg, Imm32(JSFUN_INTERPRETED));
    if (!bailoutIf(Assembler::Below, call->snapshot()))
        return false;

    
    masm.mov(objreg, tokreg);

    
    masm.movePtr(Operand(objreg, offsetof(JSFunction, u.i.script)), objreg);
    masm.movePtr(Operand(objreg, offsetof(JSScript, ion)), objreg);

    
    masm.testPtr(objreg, objreg);
    if (!bailoutIf(Assembler::Zero, call->snapshot()))
        return false;

    
    JS_STATIC_ASSERT(IonFramePrefix::JSFrame == 0x0);
    uint32 stack_size = masm.framePushed() - unused_stack;
    uint32 size_descriptor = stack_size << IonFramePrefix::FrameTypeBits;

    
    if (unused_stack)
        masm.addPtr(Imm32(unused_stack), StackPointer);

    
    masm.push(tokreg);
    masm.push(Imm32(size_descriptor));

    
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
        masm.jump(&rejoin);

        
        masm.bind(&thunk);
        masm.mov(Imm32(call->nargs()), ArgumentsRectifierReg);
        masm.movePtr(ImmWord(argumentsRectifier->raw()), ecx); 
        masm.call(ecx);

        masm.bind(&rejoin);
    }

    
    int prefix_garbage = 2 * sizeof(void *);
    int restore_diff = prefix_garbage - unused_stack;
    
    if (restore_diff > 0)
        masm.addPtr(Imm32(restore_diff), StackPointer);
    else if (restore_diff < 0)
        masm.subPtr(Imm32(-restore_diff), StackPointer);

    return true;
#endif
    return false;
}
