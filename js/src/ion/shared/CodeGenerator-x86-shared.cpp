







































#include "CodeGenerator-x86-shared.h"
#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "CodeGenerator-shared-inl.h"
#include "ion/IonFrames.h"
#include "ion/MoveEmitter.h"

using namespace js;
using namespace js::ion;

CodeGeneratorX86Shared::CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph)
  : CodeGeneratorShared(gen, graph)
{
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
    if (class_ < JS_ARRAY_LENGTH(FrameSizes))
        return FrameSizes[class_];

    uint32 step = class_ - JS_ARRAY_LENGTH(FrameSizes);
    return LAST_FRAME_SIZE + step * LAST_FRAME_INCREMENT;
}

bool
CodeGeneratorX86Shared::generatePrologue()
{
    
    masm.reserveStack(frameStaticSize_);

    
    
    returnLabel_ = gen->allocate<Label>();
    new (returnLabel_) Label();

    return true;
}

bool
CodeGeneratorX86Shared::generateEpilogue()
{
    masm.bind(returnLabel_);
    masm.freeStack(frameStaticSize_);
    masm.ret();
    return true;
}

bool
CodeGeneratorX86Shared::visitLabel(LLabel *label)
{
    masm.bind(label->label());
    return true;
}

bool
CodeGeneratorX86Shared::visitGoto(LGoto *jump)
{
    LBlock *target = jump->target()->lir();
    LLabel *header = target->begin()->toLabel();

    
    if (current->mir()->id() + 1 == target->mir()->id())
        return true;

    masm.jmp(header->label());
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

typedef MoveResolver::MoveOperand MoveOperand;

MoveOperand
CodeGeneratorX86Shared::toMoveOperand(const LAllocation *a)
{
    if (a->isGeneralReg())
        return MoveOperand(ToRegister(a));
    if (a->isFloatReg())
        return MoveOperand(ToFloatRegister(a));
    int32 disp = a->isStackSlot()
                 ? SlotToStackOffset(a->toStackSlot()->slot())
                 : ArgToStackOffset(a->toArgument()->index());
    return MoveOperand(StackPointer, disp);
}

bool
CodeGeneratorX86Shared::visitMoveGroup(LMoveGroup *group)
{
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
    emitter.emit(resolver, group->freeRegs());
    emitter.finish();

    return true;
}

