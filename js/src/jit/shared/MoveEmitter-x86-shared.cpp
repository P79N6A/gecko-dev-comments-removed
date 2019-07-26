





#include "jit/shared/MoveEmitter-x86-shared.h"

using namespace js;
using namespace js::jit;

MoveEmitterX86::MoveEmitterX86(MacroAssemblerSpecific &masm)
  : inCycle_(false),
    masm(masm),
    pushedAtCycle_(-1)
{
    pushedAtStart_ = masm.framePushed();
}




size_t
MoveEmitterX86::characterizeCycle(const MoveResolver &moves, size_t i,
                                  bool *allGeneralRegs, bool *allFloatRegs)
{
    size_t swapCount = 0;

    for (size_t j = i; ; j++) {
        const Move &move = moves.getMove(j);

        
        
        if (!move.to().isGeneralReg())
            *allGeneralRegs = false;
        if (!move.to().isFloatReg())
            *allFloatRegs = false;
        if (!*allGeneralRegs && !*allFloatRegs)
            return -1;

        
        
        if (j != i && move.inCycle())
            break;

        
        
        
        if (move.from() != moves.getMove(j + 1).to()) {
            *allGeneralRegs = false;
            *allFloatRegs = false;
            return -1;
        }

        swapCount++;
    }

    
    const Move &move = moves.getMove(i + swapCount);
    if (move.from() != moves.getMove(i).to()) {
        *allGeneralRegs = false;
        *allFloatRegs = false;
        return -1;
    }

    return swapCount;
}



bool
MoveEmitterX86::maybeEmitOptimizedCycle(const MoveResolver &moves, size_t i,
                                        bool allGeneralRegs, bool allFloatRegs, size_t swapCount)
{
    if (allGeneralRegs && swapCount <= 2) {
        
        
        
        for (size_t k = 0; k < swapCount; k++)
            masm.xchg(moves.getMove(i + k).to().reg(), moves.getMove(i + k + 1).to().reg());
        return true;
    }

    if (allFloatRegs && swapCount == 1) {
        
        
        FloatRegister a = moves.getMove(i).to().floatReg();
        FloatRegister b = moves.getMove(i + 1).to().floatReg();
        masm.xorpd(a, b);
        masm.xorpd(b, a);
        masm.xorpd(a, b);
        return true;
    }

    return false;
}

void
MoveEmitterX86::emit(const MoveResolver &moves)
{
    for (size_t i = 0; i < moves.numMoves(); i++) {
        const Move &move = moves.getMove(i);
        const MoveOperand &from = move.from();
        const MoveOperand &to = move.to();

        if (move.inCycle()) {
            
            
            if (inCycle_) {
                completeCycle(to, move.kind());
                inCycle_ = false;
                continue;
            }

            
            bool allGeneralRegs = true, allFloatRegs = true;
            size_t swapCount = characterizeCycle(moves, i, &allGeneralRegs, &allFloatRegs);

            
            if (maybeEmitOptimizedCycle(moves, i, allGeneralRegs, allFloatRegs, swapCount)) {
                i += swapCount;
                continue;
            }

            
            breakCycle(to, move.kind());
            inCycle_ = true;
        }

        
        if (move.kind() == Move::DOUBLE)
            emitDoubleMove(from, to);
        else
            emitGeneralMove(from, to);
    }
}

MoveEmitterX86::~MoveEmitterX86()
{
    assertDone();
}

Operand
MoveEmitterX86::cycleSlot()
{
    if (pushedAtCycle_ == -1) {
        
        masm.reserveStack(sizeof(double));
        pushedAtCycle_ = masm.framePushed();
    }

    return Operand(StackPointer, masm.framePushed() - pushedAtCycle_);
}




Operand
MoveEmitterX86::toOperand(const MoveOperand &operand) const
{
    if (operand.isMemory() || operand.isEffectiveAddress() || operand.isFloatAddress()) {
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



Operand
MoveEmitterX86::toPopOperand(const MoveOperand &operand) const
{
    if (operand.isMemory()) {
        if (operand.base() != StackPointer)
            return Operand(operand.base(), operand.disp());

        JS_ASSERT(operand.disp() >= 0);

        
        
        
        return Operand(StackPointer,
                       operand.disp() + (masm.framePushed() - sizeof(void *) - pushedAtStart_));
    }
    if (operand.isGeneralReg())
        return Operand(operand.reg());

    JS_ASSERT(operand.isFloatReg());
    return Operand(operand.floatReg());
}

void
MoveEmitterX86::breakCycle(const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (kind == Move::DOUBLE) {
        if (to.isMemory()) {
            masm.loadDouble(toOperand(to), ScratchFloatReg);
            masm.storeDouble(ScratchFloatReg, cycleSlot());
        } else {
            masm.storeDouble(to.floatReg(), cycleSlot());
        }
    } else {
        if (to.isMemory())
            masm.Push(toOperand(to));
        else
            masm.Push(to.reg());
    }
}

void
MoveEmitterX86::completeCycle(const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (kind == Move::DOUBLE) {
        if (to.isMemory()) {
            masm.loadDouble(cycleSlot(), ScratchFloatReg);
            masm.storeDouble(ScratchFloatReg, toOperand(to));
        } else {
            masm.loadDouble(cycleSlot(), to.floatReg());
        }
    } else {
        if (to.isMemory()) {
            masm.Pop(toPopOperand(to));
        } else {
            masm.Pop(to.reg());
        }
    }
}

void
MoveEmitterX86::emitGeneralMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isGeneralReg()) {
        masm.mov(from.reg(), toOperand(to));
    } else if (to.isGeneralReg()) {
        JS_ASSERT(from.isMemory() || from.isEffectiveAddress());
        if (from.isMemory())
            masm.mov(toOperand(from), to.reg());
        else
            masm.lea(toOperand(from), to.reg());
    } else if (from.isMemory()) {
        
#ifdef JS_CPU_X64
        
        masm.mov(toOperand(from), ScratchReg);
        masm.mov(ScratchReg, toOperand(to));
#else
        
        masm.Push(toOperand(from));
        masm.Pop(toPopOperand(to));
#endif
    } else {
        
        JS_ASSERT(from.isEffectiveAddress());
#ifdef JS_CPU_X64
        
        masm.lea(toOperand(from), ScratchReg);
        masm.mov(ScratchReg, toOperand(to));
#else
        
        
        masm.Push(from.base());
        masm.Pop(toPopOperand(to));
        masm.addPtr(Imm32(from.disp()), toOperand(to));
#endif
    }
}

void
MoveEmitterX86::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        masm.movsd(from.floatReg(), toOperand(to));
    } else if (to.isFloatReg()) {
        masm.loadDouble(toOperand(from), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        masm.loadDouble(toOperand(from), ScratchFloatReg);
        masm.storeDouble(ScratchFloatReg, toOperand(to));
    }
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

    masm.freeStack(masm.framePushed() - pushedAtStart_);
}

