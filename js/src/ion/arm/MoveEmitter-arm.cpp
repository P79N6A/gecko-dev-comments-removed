








































#include "MoveEmitter-arm.h"

using namespace js;
using namespace js::ion;

MoveEmitterARM::MoveEmitterARM(MacroAssembler &masm)
  : inCycle_(false),
    masm(masm),
    pushedAtCycle_(-1),
    pushedAtSpill_(-1),
    pushedAtDoubleSpill_(-1),
    spilledReg_(InvalidReg),
    spilledFloatReg_(InvalidFloatReg)
{
    pushedAtStart_ = masm.framePushed();
}

void
MoveEmitterARM::emit(const MoveResolver &moves)
{
    if (moves.hasCycles()) {
        
        masm.reserveStack(sizeof(double));
        pushedAtCycle_ = masm.framePushed();
    }

    for (size_t i = 0; i < moves.numMoves(); i++)
        emit(moves.getMove(i));
}

MoveEmitterARM::~MoveEmitterARM()
{
    assertDone();
}

Operand
MoveEmitterARM::cycleSlot() const
{
    return Operand(StackPointer, masm.framePushed() - pushedAtCycle_);
}

Operand
MoveEmitterARM::spillSlot() const
{
    return Operand(StackPointer, masm.framePushed() - pushedAtSpill_);
}

Operand
MoveEmitterARM::doubleSpillSlot() const
{
    return Operand(StackPointer, masm.framePushed() - pushedAtDoubleSpill_);
}

Operand
MoveEmitterARM::toOperand(const MoveOperand &operand) const
{
    if (operand.isMemory()) {
        if (operand.base() != StackPointer)
            return Operand(operand.base(), operand.disp());

        JS_ASSERT(operand.disp() >= 0);

        
        return Operand(StackPointer, operand.disp() + (masm.framePushed() - pushedAtStart_));
    }
    if (operand.isGeneralReg())
        return Operand(operand.reg());

    JS_ASSERT(operand.isFloatReg());
    return Operand(operand.floatReg());
}

Register
MoveEmitterARM::tempReg()
{
    if (spilledReg_ != InvalidReg)
        return spilledReg_;

    
    
    spilledReg_ = Register::FromCode(2);
    if (pushedAtSpill_ == -1) {
        masm.Push(spilledReg_);
        pushedAtSpill_ = masm.framePushed();
    } else {
        masm.mov(spilledReg_, spillSlot());
    }
    return spilledReg_;
}

FloatRegister
MoveEmitterARM::tempFloatReg()
{
    if (spilledFloatReg_ != InvalidFloatReg)
        return spilledFloatReg_;

    
    
    spilledFloatReg_ = FloatRegister::FromCode(7);
    if (pushedAtDoubleSpill_ == -1) {
        masm.reserveStack(sizeof(double));
        pushedAtDoubleSpill_ = masm.framePushed();
    }
    masm.movsd(spilledFloatReg_, doubleSpillSlot());
    return spilledFloatReg_;
}

void
MoveEmitterARM::breakCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (to.isDouble()) {
        if (to.isMemory()) {
            FloatRegister temp = tempFloatReg();
            masm.movsd(toOperand(to), temp);
            masm.movsd(temp, cycleSlot());
        } else {
            masm.movsd(to.floatReg(), cycleSlot());
        }
    } else {
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.mov(toOperand(to), temp);
            masm.mov(temp, cycleSlot());
        } else {
            masm.mov(to.reg(), cycleSlot());
        }
    }
}

void
MoveEmitterARM::completeCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (kind == Move::DOUBLE) {
        if (to.isMemory()) {
            FloatRegister temp = tempFloatReg();
            masm.movsd(cycleSlot(), temp);
            masm.movsd(temp, toOperand(to));
        } else {
            masm.movsd(cycleSlot(), to.floatReg());
        }
    } else {
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.mov(cycleSlot(), temp);
            masm.mov(temp, toOperand(to));
        } else {
            masm.mov(cycleSlot(), to.reg());
        }
    }
}

void
MoveEmitterARM::emitMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isGeneralReg()) {
        if (from.reg() == spilledReg_) {
            
            
            masm.mov(spillSlot(), spilledReg_);
            spilledReg_ = InvalidReg;
        }
        masm.mov(from.reg(), toOperand(to));
    } else if (to.isGeneralReg()) {
        if (to.reg() == spilledReg_) {
            
            
            spilledReg_ = InvalidReg;
        }
        masm.mov(toOperand(from), to.reg());
    } else {
        
        Register reg = tempReg();
        masm.mov(toOperand(from), reg);
        masm.mov(reg, toOperand(to));
    }
}

void
MoveEmitterARM::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (from.floatReg() == spilledFloatReg_) {
            
            
            masm.movsd(doubleSpillSlot(), spilledFloatReg_);
            spilledFloatReg_ = InvalidFloatReg;
        }
        masm.movsd(from.floatReg(), toOperand(to));
    } else if (to.isFloatReg()) {
        if (to.floatReg() == spilledFloatReg_) {
            
            
            spilledFloatReg_ = InvalidFloatReg;
        }
        masm.movsd(toOperand(from), to.floatReg());
    } else {
        
        FloatRegister reg = tempFloatReg();
        masm.movsd(toOperand(from), reg);
        masm.movsd(reg, toOperand(to));
    }
}

void
MoveEmitterARM::emit(const Move &move)
{
    const MoveOperand &from = move.from();
    const MoveOperand &to = move.to();

    if (move.inCycle()) {
        if (inCycle_) {
            completeCycle(from, to, move.kind());
            inCycle_ = false;
            return;
        }

        breakCycle(from, to, move.kind());
        inCycle_ = true;
    }

    if (move.kind() == Move::DOUBLE)
        emitDoubleMove(from, to);
    else
        emitMove(from, to);
}

void
MoveEmitterARM::assertDone()
{
    JS_ASSERT(!inCycle_);
}

void
MoveEmitterARM::finish()
{
    assertDone();

    if (pushedAtDoubleSpill_ != -1 && spilledFloatReg_ != InvalidFloatReg)
        masm.movsd(doubleSpillSlot(), spilledFloatReg_);
    if (pushedAtSpill_ != -1 && spilledReg_ != InvalidReg)
        masm.mov(spillSlot(), spilledReg_);

    masm.freeStack(masm.framePushed() - pushedAtStart_);
}

