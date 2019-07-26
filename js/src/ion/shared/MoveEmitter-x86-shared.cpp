






#include "MoveEmitter-x86-shared.h"

using namespace js;
using namespace js::ion;

MoveEmitterX86::MoveEmitterX86(MacroAssemblerSpecific &masm)
  : inCycle_(false),
    masm(masm),
    pushedAtCycle_(-1),
    pushedAtSpill_(-1),
    spilledReg_(InvalidReg)
{
    pushedAtStart_ = masm.framePushed();
}

void
MoveEmitterX86::emit(const MoveResolver &moves)
{
    if (moves.hasCycles()) {
        
        masm.reserveStack(sizeof(double));
        pushedAtCycle_ = masm.framePushed();
    }

    for (size_t i = 0; i < moves.numMoves(); i++)
        emit(moves.getMove(i));
}

MoveEmitterX86::~MoveEmitterX86()
{
    assertDone();
}

Operand
MoveEmitterX86::cycleSlot() const
{
    return Operand(StackPointer, masm.framePushed() - pushedAtCycle_);
}

Operand
MoveEmitterX86::spillSlot() const
{
    return Operand(StackPointer, masm.framePushed() - pushedAtSpill_);
}

Operand
MoveEmitterX86::toOperand(const MoveOperand &operand) const
{
    if (operand.isMemory() || operand.isEffectiveAddress()) {
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
MoveEmitterX86::tempReg()
{
    if (spilledReg_ != InvalidReg)
        return spilledReg_;

    
    
    spilledReg_ = edx;

#ifdef JS_CPU_X64
    JS_ASSERT(edx == rdx);
#endif

    if (pushedAtSpill_ == -1) {
        masm.Push(spilledReg_);
        pushedAtSpill_ = masm.framePushed();
    } else {
        masm.mov(spilledReg_, spillSlot());
    }
    return spilledReg_;
}

void
MoveEmitterX86::breakCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (kind == Move::DOUBLE) {
        if (to.isMemory()) {
            masm.movsd(toOperand(to), ScratchFloatReg);
            masm.movsd(ScratchFloatReg, cycleSlot());
        } else {
            masm.movsd(to.floatReg(), cycleSlot());
        }
    } else {
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.mov(toOperand(to), temp);
            masm.mov(temp, cycleSlot());
        } else {
            if (to.reg() == spilledReg_) {
                
                masm.mov(spillSlot(), spilledReg_);
                spilledReg_ = InvalidReg;
            }
            masm.mov(to.reg(), cycleSlot());
        }
    }
}

void
MoveEmitterX86::completeCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (kind == Move::DOUBLE) {
        if (to.isMemory()) {
            masm.movsd(cycleSlot(), ScratchFloatReg);
            masm.movsd(ScratchFloatReg, toOperand(to));
        } else {
            masm.movsd(cycleSlot(), to.floatReg());
        }
    } else {
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.mov(cycleSlot(), temp);
            masm.mov(temp, toOperand(to));
        } else {
            if (to.reg() == spilledReg_) {
                
                spilledReg_ = InvalidReg;
            }
            masm.mov(cycleSlot(), to.reg());
        }
    }
}

void
MoveEmitterX86::emitMove(const MoveOperand &from, const MoveOperand &to)
{
    if (to.isGeneralReg() && to.reg() == spilledReg_) {
        
        
        spilledReg_ = InvalidReg;
    }

    if (from.isGeneralReg()) {
        if (from.reg() == spilledReg_) {
            
            
            masm.mov(spillSlot(), spilledReg_);
            spilledReg_ = InvalidReg;
        }
        masm.mov(from.reg(), toOperand(to));
    } else if (to.isGeneralReg()) {
        JS_ASSERT(from.isMemory() || from.isEffectiveAddress());
        if (from.isMemory())
            masm.mov(toOperand(from), to.reg());
        else
            masm.lea(toOperand(from), to.reg());
    } else {
        
        Register reg = tempReg();
        
        if (reg == from.base())
            masm.mov(spillSlot(), from.base());

        JS_ASSERT(from.isMemory() || from.isEffectiveAddress());
        if (from.isMemory())
            masm.mov(toOperand(from), reg);
        else
            masm.lea(toOperand(from), reg);
        JS_ASSERT(to.base() != reg);
        masm.mov(reg, toOperand(to));
    }
}

void
MoveEmitterX86::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        masm.movsd(from.floatReg(), toOperand(to));
    } else if (to.isFloatReg()) {
        masm.movsd(toOperand(from), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        masm.movsd(toOperand(from), ScratchFloatReg);
        masm.movsd(ScratchFloatReg, toOperand(to));
    }
}

void
MoveEmitterX86::emit(const Move &move)
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
MoveEmitterX86::assertDone()
{
    JS_ASSERT(!inCycle_);
}

void
MoveEmitterX86::finish()
{
    assertDone();

    if (pushedAtSpill_ != -1 && spilledReg_ != InvalidReg)
        masm.mov(spillSlot(), spilledReg_);

    masm.freeStack(masm.framePushed() - pushedAtStart_);
}

