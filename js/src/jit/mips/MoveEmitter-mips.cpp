





#include "jit/mips/MoveEmitter-mips.h"

using namespace js;
using namespace js::jit;

MoveEmitterMIPS::MoveEmitterMIPS(MacroAssemblerMIPSCompat &masm)
  : inCycle_(0),
    masm(masm),
    pushedAtCycle_(-1),
    pushedAtSpill_(-1),
    spilledReg_(InvalidReg),
    spilledFloatReg_(InvalidFloatReg)
{
    pushedAtStart_ = masm.framePushed();
}

void
MoveEmitterMIPS::emit(const MoveResolver &moves)
{
    if (moves.numCycles()) {
        
        masm.reserveStack(moves.numCycles() * sizeof(double));
        pushedAtCycle_ = masm.framePushed();
    }

    for (size_t i = 0; i < moves.numMoves(); i++)
        emit(moves.getMove(i));
}

MoveEmitterMIPS::~MoveEmitterMIPS()
{
    assertDone();
}

Address
MoveEmitterMIPS::cycleSlot(uint32_t slot, uint32_t subslot) const
{
    int32_t offset = masm.framePushed() - pushedAtCycle_;
    MOZ_ASSERT(Imm16::IsInSignedRange(offset));
    return Address(StackPointer, offset + slot * sizeof(double) + subslot);
}

int32_t
MoveEmitterMIPS::getAdjustedOffset(const MoveOperand &operand)
{
    MOZ_ASSERT(operand.isMemoryOrEffectiveAddress());
    if (operand.base() != StackPointer)
        return operand.disp();

    
    return operand.disp() + masm.framePushed() - pushedAtStart_;
}

Address
MoveEmitterMIPS::getAdjustedAddress(const MoveOperand &operand)
{
    return Address(operand.base(), getAdjustedOffset(operand));
}


Register
MoveEmitterMIPS::tempReg()
{
    spilledReg_ = SecondScratchReg;
    return SecondScratchReg;
}

void
MoveEmitterMIPS::breakCycle(const MoveOperand &from, const MoveOperand &to,
                            MoveOp::Type type, uint32_t slotId)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::FLOAT32:
        if (to.isMemory()) {
            FloatRegister temp = ScratchFloat32Reg;
            masm.loadFloat32(getAdjustedAddress(to), temp);
            
            
            masm.storeFloat32(temp, cycleSlot(slotId, 0));
            masm.storeFloat32(temp, cycleSlot(slotId, 4));
        } else {
            
            masm.storeDouble(to.floatReg().doubleOverlay(), cycleSlot(slotId, 0));
        }
        break;
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            FloatRegister temp = ScratchDoubleReg;
            masm.loadDouble(getAdjustedAddress(to), temp);
            masm.storeDouble(temp, cycleSlot(slotId, 0));
        } else {
            masm.storeDouble(to.floatReg(), cycleSlot(slotId, 0));
        }
        break;
      case MoveOp::INT32:
        MOZ_ASSERT(sizeof(uintptr_t) == sizeof(int32_t));
      case MoveOp::GENERAL:
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.loadPtr(getAdjustedAddress(to), temp);
            masm.storePtr(temp, cycleSlot(0, 0));
        } else {
            
            MOZ_ASSERT(to.reg() != spilledReg_);
            masm.storePtr(to.reg(), cycleSlot(0, 0));
        }
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterMIPS::completeCycle(const MoveOperand &from, const MoveOperand &to,
                               MoveOp::Type type, uint32_t slotId)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::FLOAT32:
        if (to.isMemory()) {
            FloatRegister temp = ScratchFloat32Reg;
            masm.loadFloat32(cycleSlot(slotId, 0), temp);
            masm.storeFloat32(temp, getAdjustedAddress(to));
        } else {
            uint32_t offset = 0;
            if (from.floatReg().numAlignedAliased() == 1)
                offset = sizeof(float);
            masm.loadFloat32(cycleSlot(slotId, offset), to.floatReg());
        }
        break;
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            FloatRegister temp = ScratchDoubleReg;
            masm.loadDouble(cycleSlot(slotId, 0), temp);
            masm.storeDouble(temp, getAdjustedAddress(to));
        } else {
            masm.loadDouble(cycleSlot(slotId, 0), to.floatReg());
        }
        break;
      case MoveOp::INT32:
        MOZ_ASSERT(sizeof(uintptr_t) == sizeof(int32_t));
      case MoveOp::GENERAL:
        MOZ_ASSERT(slotId == 0);
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.loadPtr(cycleSlot(0, 0), temp);
            masm.storePtr(temp, getAdjustedAddress(to));
        } else {
            
            MOZ_ASSERT(to.reg() != spilledReg_);
            masm.loadPtr(cycleSlot(0, 0), to.reg());
        }
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterMIPS::emitMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isGeneralReg()) {
        
        MOZ_ASSERT(from.reg() != spilledReg_);

        if (to.isGeneralReg())
            masm.movePtr(from.reg(), to.reg());
        else if (to.isMemory())
            masm.storePtr(from.reg(), getAdjustedAddress(to));
        else
            MOZ_CRASH("Invalid emitMove arguments.");
    } else if (from.isMemory()) {
        if (to.isGeneralReg()) {
            masm.loadPtr(getAdjustedAddress(from), to.reg());
        } else if (to.isMemory()) {
            masm.loadPtr(getAdjustedAddress(from), tempReg());
            masm.storePtr(tempReg(), getAdjustedAddress(to));
        } else {
            MOZ_CRASH("Invalid emitMove arguments.");
        }
    } else if (from.isEffectiveAddress()) {
        if (to.isGeneralReg()) {
            masm.computeEffectiveAddress(getAdjustedAddress(from), to.reg());
        } else if (to.isMemory()) {
            masm.computeEffectiveAddress(getAdjustedAddress(from), tempReg());
            masm.storePtr(tempReg(), getAdjustedAddress(to));
        } else {
            MOZ_CRASH("Invalid emitMove arguments.");
        }
    } else {
        MOZ_CRASH("Invalid emitMove arguments.");
    }
}

void
MoveEmitterMIPS::emitFloat32Move(const MoveOperand &from, const MoveOperand &to)
{
    
    MOZ_ASSERT_IF(from.isFloatReg(), from.floatReg() != ScratchFloat32Reg);
    MOZ_ASSERT_IF(to.isFloatReg(), to.floatReg() != ScratchFloat32Reg);

    if (from.isFloatReg()) {
        if (to.isFloatReg()) {
            masm.moveFloat32(from.floatReg(), to.floatReg());
        } else if (to.isGeneralReg()) {
            
            MOZ_ASSERT(to.reg() == a1 || to.reg() == a2 || to.reg() == a3);
            masm.moveFromFloat32(from.floatReg(), to.reg());
        } else {
            MOZ_ASSERT(to.isMemory());
            masm.storeFloat32(from.floatReg(), getAdjustedAddress(to));
        }
    } else if (to.isFloatReg()) {
        MOZ_ASSERT(from.isMemory());
        masm.loadFloat32(getAdjustedAddress(from), to.floatReg());
    } else if (to.isGeneralReg()) {
        MOZ_ASSERT(from.isMemory());
        
        MOZ_ASSERT(to.reg() == a1 || to.reg() == a2 || to.reg() == a3);
        masm.loadPtr(getAdjustedAddress(from), to.reg());
    } else {
        MOZ_ASSERT(from.isMemory());
        MOZ_ASSERT(to.isMemory());
        masm.loadFloat32(getAdjustedAddress(from), ScratchFloat32Reg);
        masm.storeFloat32(ScratchFloat32Reg, getAdjustedAddress(to));
    }
}

void
MoveEmitterMIPS::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    
    MOZ_ASSERT_IF(from.isFloatReg(), from.floatReg() != ScratchDoubleReg);
    MOZ_ASSERT_IF(to.isFloatReg(), to.floatReg() != ScratchDoubleReg);

    if (from.isFloatReg()) {
        if (to.isFloatReg()) {
            masm.moveDouble(from.floatReg(), to.floatReg());
        } else if (to.isGeneralReg()) {
            
            
            
            if(to.reg() == a2)
                masm.moveFromDoubleLo(from.floatReg(), a2);
            else if(to.reg() == a3)
                masm.moveFromDoubleHi(from.floatReg(), a3);
            else
                MOZ_CRASH("Invalid emitDoubleMove arguments.");
        } else {
            MOZ_ASSERT(to.isMemory());
            masm.storeDouble(from.floatReg(), getAdjustedAddress(to));
        }
    } else if (to.isFloatReg()) {
        MOZ_ASSERT(from.isMemory());
        masm.loadDouble(getAdjustedAddress(from), to.floatReg());
    } else if (to.isGeneralReg()) {
        MOZ_ASSERT(from.isMemory());
        
        
        
        if(to.reg() == a2)
            masm.loadPtr(getAdjustedAddress(from), a2);
        else if(to.reg() == a3)
            masm.loadPtr(Address(from.base(), getAdjustedOffset(from) + sizeof(uint32_t)), a3);
        else
            MOZ_CRASH("Invalid emitDoubleMove arguments.");
    } else {
        MOZ_ASSERT(from.isMemory());
        MOZ_ASSERT(to.isMemory());
        masm.loadDouble(getAdjustedAddress(from), ScratchDoubleReg);
        masm.storeDouble(ScratchDoubleReg, getAdjustedAddress(to));
    }
}

void
MoveEmitterMIPS::emit(const MoveOp &move)
{
    const MoveOperand &from = move.from();
    const MoveOperand &to = move.to();

    if (move.isCycleEnd() && move.isCycleBegin()) {
        
        
        breakCycle(from, to, move.endCycleType(), move.cycleBeginSlot());
        completeCycle(from, to, move.type(), move.cycleEndSlot());
        return;
    }

    if (move.isCycleEnd()) {
        MOZ_ASSERT(inCycle_);
        completeCycle(from, to, move.type(), move.cycleEndSlot());
        MOZ_ASSERT(inCycle_ > 0);
        inCycle_--;
        return;
    }

    if (move.isCycleBegin()) {
        breakCycle(from, to, move.endCycleType(), move.cycleBeginSlot());
        inCycle_++;
    }

    switch (move.type()) {
      case MoveOp::FLOAT32:
        emitFloat32Move(from, to);
        break;
      case MoveOp::DOUBLE:
        emitDoubleMove(from, to);
        break;
      case MoveOp::INT32:
        MOZ_ASSERT(sizeof(uintptr_t) == sizeof(int32_t));
      case MoveOp::GENERAL:
        emitMove(from, to);
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterMIPS::assertDone()
{
    MOZ_ASSERT(inCycle_ == 0);
}

void
MoveEmitterMIPS::finish()
{
    assertDone();

    masm.freeStack(masm.framePushed() - pushedAtStart_);
}
