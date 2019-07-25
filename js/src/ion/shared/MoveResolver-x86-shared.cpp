







































#if defined(JS_CPU_X86)
# include "ion/x86/CodeGenerator-x86.h"
#else
# include "ion/x64/CodeGenerator-x64.h"
#endif
#include "CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::ion;

MoveResolverX86::MoveResolverX86(CodeGenerator *codegen)
  : inCycle_(false),
    codegen(codegen),
    pushedAtCycle_(-1),
    pushedAtSpill_(-1),
    pushedAtDoubleSpill_(-1),
    spilledReg_(InvalidReg),
    spilledFloatReg_(InvalidFloatReg)
{
}

MoveResolverX86::~MoveResolverX86()
{
}

void
MoveResolverX86::setup(LMoveGroup *group)
{
    pushedAtCycle_ = -1;
    freeRegs_ = group->freeRegs();

    if (codegen->moveGroupResolver.hasCycles()) {
        cycleReg_ = freeRegs_.empty(false) ? InvalidReg : freeRegs_.takeGeneral();
        cycleFloatReg_ = freeRegs_.empty(true) ? InvalidFloatReg : freeRegs_.takeFloat();

        
        if (cycleReg_ == InvalidReg && cycleFloatReg_ == InvalidFloatReg) {
            codegen->masm.reserveStack(sizeof(double));
            pushedAtCycle_ = codegen->framePushed_;
        }
    }

    spilledReg_ = InvalidReg;
    spilledFloatReg_ = InvalidFloatReg;
    pushedAtSpill_ = -1;
    pushedAtDoubleSpill_ = -1;
}

Operand
MoveResolverX86::cycleSlot() const
{
    return Operand(StackPointer, codegen->framePushed_ - pushedAtCycle_);
}

Operand
MoveResolverX86::spillSlot() const
{
    return Operand(StackPointer, codegen->framePushed_ - pushedAtSpill_);
}

Operand
MoveResolverX86::doubleSpillSlot() const
{
    return Operand(StackPointer, codegen->framePushed_ - pushedAtDoubleSpill_);
}

Register
MoveResolverX86::tempReg()
{
    if (spilledReg_ != InvalidReg)
        return spilledReg_;

    if (!freeRegs_.empty(false)) {
        spilledReg_ = freeRegs_.takeGeneral();
        return spilledReg_;
    }

    
    
    
    spilledReg_ = Register::FromCode(2);
    if (pushedAtSpill_ != -1) {
        codegen->masm.push(Operand(spilledReg_));
        codegen->framePushed_ += STACK_SLOT_SIZE;
        pushedAtSpill_ = codegen->framePushed_;
    } else {
        codegen->masm.mov(spilledReg_, spillSlot());
    }
    return spilledReg_;
}

FloatRegister
MoveResolverX86::tempFloatReg()
{
    if (spilledFloatReg_ != InvalidFloatReg)
        return spilledFloatReg_;

    if (!freeRegs_.empty(true)) {
        spilledFloatReg_ = freeRegs_.takeFloat();
        return spilledFloatReg_;
    }

    
    
    
    spilledFloatReg_ = FloatRegister::FromCode(7);
    if (pushedAtDoubleSpill_ != -1) {
        codegen->masm.reserveStack(sizeof(double));
        codegen->framePushed_ += sizeof(double);
        pushedAtDoubleSpill_ = codegen->framePushed_;
    }
    codegen->masm.movsd(spilledFloatReg_, doubleSpillSlot());
    return spilledFloatReg_;
}

void
MoveResolverX86::breakCycle(const LAllocation *from, const LAllocation *to)
{
    
    
    
    
    
    
    if (to->isDouble()) {
        if (cycleFloatReg_ != InvalidFloatReg) {
            codegen->masm.movsd(codegen->ToOperand(to), cycleFloatReg_);
        } else if (to->isMemory()) {
            FloatRegister temp = tempFloatReg();
            codegen->masm.movsd(codegen->ToOperand(to), temp);
            codegen->masm.movsd(temp, cycleSlot());
        } else {
            codegen->masm.movsd(ToFloatRegister(to), cycleSlot());
        }
    } else {
        if (cycleReg_ != InvalidReg) {
            codegen->masm.mov(codegen->ToOperand(to), cycleReg_);
        } else if (to->isMemory()) {
            Register temp = tempReg();
            codegen->masm.mov(codegen->ToOperand(to), temp);
            codegen->masm.mov(temp, cycleSlot());
        } else {
            codegen->masm.mov(ToRegister(to), cycleSlot());
        }
    }
}

void
MoveResolverX86::completeCycle(const LAllocation *from, const LAllocation *to)
{
    
    
    
    
    
    
    if (from->isDouble()) {
        if (cycleFloatReg_ != InvalidFloatReg) {
            codegen->masm.movsd(cycleFloatReg_, codegen->ToOperand(to));
        } else if (to->isMemory()) {
            FloatRegister temp = tempFloatReg();
            codegen->masm.movsd(cycleSlot(), temp);
            codegen->masm.movsd(temp, codegen->ToOperand(to));
        } else {
            codegen->masm.movsd(cycleSlot(), ToFloatRegister(to));
        }
    } else {
        if (cycleReg_ != InvalidReg) {
            codegen->masm.mov(cycleReg_, codegen->ToOperand(to));
        } else if (to->isMemory()) {
            Register temp = tempReg();
            codegen->masm.mov(cycleSlot(), temp);
            codegen->masm.mov(temp, codegen->ToOperand(to));
        } else {
            codegen->masm.mov(cycleSlot(), ToRegister(to));
        }
    }
}

void
MoveResolverX86::emitMove(const LAllocation *from, const LAllocation *to)
{
    if (from->isGeneralReg()) {
        if (ToRegister(from) == spilledReg_) {
            
            
            codegen->masm.mov(spillSlot(), spilledReg_);
            spilledReg_ = InvalidReg;
        }
        codegen->masm.mov(ToRegister(from), codegen->ToOperand(to));
    } else if (to->isGeneralReg()) {
        if (ToRegister(to) == spilledReg_) {
            
            
            spilledReg_ = InvalidReg;
        }
        codegen->masm.mov(codegen->ToOperand(from), ToRegister(to));
    } else {
        
        Register reg = tempReg();
        codegen->masm.mov(codegen->ToOperand(from), reg);
        codegen->masm.mov(reg, codegen->ToOperand(to));
    }
}

void
MoveResolverX86::emitDoubleMove(const LAllocation *from, const LAllocation *to)
{
    if (from->isFloatReg()) {
        if (ToFloatRegister(from) == spilledFloatReg_) {
            
            
            codegen->masm.movsd(doubleSpillSlot(), spilledFloatReg_);
            spilledFloatReg_ = InvalidFloatReg;
        }
        codegen->masm.movsd(ToFloatRegister(from), codegen->ToOperand(to));
    } else if (to->isFloatReg()) {
        if (ToFloatRegister(to) == spilledFloatReg_) {
            
            
            spilledFloatReg_ = InvalidFloatReg;
        }
        codegen->masm.movsd(codegen->ToOperand(from), ToFloatRegister(to));
    } else {
        
        FloatRegister reg = tempFloatReg();
        codegen->masm.movsd(codegen->ToOperand(from), reg);
        codegen->masm.movsd(reg, codegen->ToOperand(to));
    }
}

void
MoveResolverX86::assertValidMove(const LAllocation *from, const LAllocation *to)
{
    JS_ASSERT(from->isDouble() == to->isDouble());
    JS_ASSERT_IF(from->isGeneralReg(), !freeRegs_.has(ToRegister(from)));
    JS_ASSERT_IF(to->isGeneralReg(), !freeRegs_.has(ToRegister(to)));
    JS_ASSERT_IF(from->isFloatReg(), !freeRegs_.has(ToFloatRegister(from)));
    JS_ASSERT_IF(to->isFloatReg(), !freeRegs_.has(ToFloatRegister(to)));
}

void
MoveResolverX86::emit(const MoveGroupResolver::Move &move)
{
    const LAllocation *from = move.from();
    const LAllocation *to = move.to();
    
    if (move.inCycle()) {
        if (inCycle_) {
            completeCycle(from, to);
            inCycle_ = false;
            return;
        }

        inCycle_ = true;
        completeCycle(from, to);
    }
    
    if (!from->isDouble())
        emitMove(from, to);
    else
        emitDoubleMove(from, to);
}

void
MoveResolverX86::assertDone()
{
    JS_ASSERT(!inCycle_);
}

void
MoveResolverX86::finish()
{
    assertDone();

    int32 decrement = 0;

    if (spilledFloatReg_ != InvalidFloatReg && pushedAtDoubleSpill_ != -1) {
        codegen->masm.movsd(doubleSpillSlot(), spilledFloatReg_);
        decrement += sizeof(double);
    }
    if (spilledReg_ != InvalidReg && pushedAtSpill_ != -1) {
        codegen->masm.mov(spillSlot(), spilledReg_);
        decrement += STACK_SLOT_SIZE;
    }
    if (pushedAtCycle_ != -1)
        decrement += sizeof(double);

    codegen->masm.freeStack(decrement);
}

