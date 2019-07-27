





#include "jit/arm/MoveEmitter-arm.h"

#include "jit/MacroAssembler-inl.h"

using namespace js;
using namespace js::jit;

MoveEmitterARM::MoveEmitterARM(MacroAssembler& masm)
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
MoveEmitterARM::emit(const MoveResolver& moves)
{
    if (moves.numCycles()) {
        
        masm.reserveStack(moves.numCycles() * sizeof(double));
        pushedAtCycle_ = masm.framePushed();
    }

    for (size_t i = 0; i < moves.numMoves(); i++)
        emit(moves.getMove(i));
}

MoveEmitterARM::~MoveEmitterARM()
{
    assertDone();
}

Address
MoveEmitterARM::cycleSlot(uint32_t slot, uint32_t subslot) const
{
    int32_t offset =  masm.framePushed() - pushedAtCycle_;
    MOZ_ASSERT(offset < 4096 && offset > -4096);
    return Address(StackPointer, offset + slot * sizeof(double) + subslot);
}

Address
MoveEmitterARM::spillSlot() const
{
    int32_t offset =  masm.framePushed() - pushedAtSpill_;
    MOZ_ASSERT(offset < 4096 && offset > -4096);
    return Address(StackPointer, offset);
}

Address
MoveEmitterARM::toAddress(const MoveOperand& operand) const
{
    MOZ_ASSERT(operand.isMemoryOrEffectiveAddress());

    if (operand.base() != StackPointer) {
        MOZ_ASSERT(operand.disp() < 1024 && operand.disp() > -1024);
        return Operand(operand.base(), operand.disp()).toAddress();
    }

    MOZ_ASSERT(operand.disp() >= 0);

    
    return Address(StackPointer, operand.disp() + (masm.framePushed() - pushedAtStart_));
}

Register
MoveEmitterARM::tempReg()
{
    if (spilledReg_ != InvalidReg)
        return spilledReg_;

    
    
    
    
    
    
    spilledReg_ = r14;
    if (pushedAtSpill_ == -1) {
        masm.Push(spilledReg_);
        pushedAtSpill_ = masm.framePushed();
    } else {
        masm.ma_str(spilledReg_, spillSlot());
    }
    return spilledReg_;
}

void
MoveEmitterARM::breakCycle(const MoveOperand& from, const MoveOperand& to,
                           MoveOp::Type type, uint32_t slotId)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::FLOAT32:
        if (to.isMemory()) {
            VFPRegister temp = ScratchFloat32Reg;
            masm.ma_vldr(toAddress(to), temp);
            
            
            masm.ma_vstr(temp, cycleSlot(slotId, 0));
            masm.ma_vstr(temp, cycleSlot(slotId, 4));
        } else {
            FloatRegister src = to.floatReg();
            
            
            masm.ma_vstr(src.doubleOverlay(), cycleSlot(slotId, 0));
        }
        break;
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            FloatRegister temp = ScratchDoubleReg;
            masm.ma_vldr(toAddress(to), temp);
            masm.ma_vstr(temp, cycleSlot(slotId, 0));
        } else {
            masm.ma_vstr(to.floatReg().doubleOverlay(), cycleSlot(slotId, 0));
        }
        break;
      case MoveOp::INT32:
      case MoveOp::GENERAL:
        
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.ma_ldr(toAddress(to), temp);
            masm.ma_str(temp, cycleSlot(0,0));
        } else {
            if (to.reg() == spilledReg_) {
                
                masm.ma_ldr(spillSlot(), spilledReg_);
                spilledReg_ = InvalidReg;
            }
            masm.ma_str(to.reg(), cycleSlot(0,0));
        }
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM::completeCycle(const MoveOperand& from, const MoveOperand& to, MoveOp::Type type, uint32_t slotId)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::FLOAT32:
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            FloatRegister temp = ScratchDoubleReg;
            masm.ma_vldr(cycleSlot(slotId, 0), temp);
            masm.ma_vstr(temp, toAddress(to));
        } else {
            uint32_t offset = 0;
            if ((!from.isMemory()) && from.floatReg().numAlignedAliased() == 1)
                offset = sizeof(float);
            masm.ma_vldr(cycleSlot(slotId, offset), to.floatReg());
        }
        break;
      case MoveOp::INT32:
      case MoveOp::GENERAL:
        MOZ_ASSERT(slotId == 0);
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.ma_ldr(cycleSlot(slotId, 0), temp);
            masm.ma_str(temp, toAddress(to));
        } else {
            if (to.reg() == spilledReg_) {
                
                spilledReg_ = InvalidReg;
            }
            masm.ma_ldr(cycleSlot(slotId, 0), to.reg());
        }
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM::emitMove(const MoveOperand& from, const MoveOperand& to)
{
    if (to.isGeneralReg() && to.reg() == spilledReg_) {
        
        
        spilledReg_ = InvalidReg;
    }

    if (from.isGeneralReg()) {
        if (from.reg() == spilledReg_) {
            
            
            masm.ma_ldr(spillSlot(), spilledReg_);
            spilledReg_ = InvalidReg;
        }
        if (to.isMemoryOrEffectiveAddress())
            masm.ma_str(from.reg(), toAddress(to));
        else
            masm.ma_mov(from.reg(), to.reg());
    } else if (to.isGeneralReg()) {
        MOZ_ASSERT(from.isMemoryOrEffectiveAddress());
        if (from.isMemory())
            masm.ma_ldr(toAddress(from), to.reg());
        else
            masm.ma_add(from.base(), Imm32(from.disp()), to.reg());
    } else {
        
        Register reg = tempReg();

        MOZ_ASSERT(from.isMemoryOrEffectiveAddress());
        if (from.isMemory())
            masm.ma_ldr(toAddress(from), reg);
        else
            masm.ma_add(from.base(), Imm32(from.disp()), reg);
        MOZ_ASSERT(to.base() != reg);
        masm.ma_str(reg, toAddress(to));
    }
}

void
MoveEmitterARM::emitFloat32Move(const MoveOperand& from, const MoveOperand& to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.ma_vmov_f32(from.floatReg(), to.floatReg());
        else
            masm.ma_vstr(VFPRegister(from.floatReg()).singleOverlay(), toAddress(to));
    } else if (to.isFloatReg()) {
        masm.ma_vldr(toAddress(from), VFPRegister(to.floatReg()).singleOverlay());
    } else {
        
        MOZ_ASSERT(from.isMemory());
        FloatRegister reg = ScratchFloat32Reg;
        masm.ma_vldr(toAddress(from), VFPRegister(reg).singleOverlay());
        masm.ma_vstr(VFPRegister(reg).singleOverlay(), toAddress(to));
    }
}

void
MoveEmitterARM::emitDoubleMove(const MoveOperand& from, const MoveOperand& to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.ma_vmov(from.floatReg(), to.floatReg());
        else
            masm.ma_vstr(from.floatReg(), toAddress(to));
    } else if (to.isFloatReg()) {
        masm.ma_vldr(toAddress(from), to.floatReg());
    } else {
        
        MOZ_ASSERT(from.isMemory());
        FloatRegister reg = ScratchDoubleReg;
        masm.ma_vldr(toAddress(from), reg);
        masm.ma_vstr(reg, toAddress(to));
    }
}

void
MoveEmitterARM::emit(const MoveOp& move)
{
    const MoveOperand& from = move.from();
    const MoveOperand& to = move.to();

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
      case MoveOp::GENERAL:
        emitMove(from, to);
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM::assertDone()
{
    MOZ_ASSERT(inCycle_ == 0);
}

void
MoveEmitterARM::finish()
{
    assertDone();

    if (pushedAtSpill_ != -1 && spilledReg_ != InvalidReg)
        masm.ma_ldr(spillSlot(), spilledReg_);
    masm.freeStack(masm.framePushed() - pushedAtStart_);
}
