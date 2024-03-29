





#include "jit/arm64/MoveEmitter-arm64.h"

using namespace js;
using namespace js::jit;

void
MoveEmitterARM64::emit(const MoveResolver& moves)
{
    if (moves.numCycles()) {
        masm.reserveStack(sizeof(void*));
        pushedAtCycle_ = masm.framePushed();
    }

    for (size_t i = 0; i < moves.numMoves(); i++)
        emitMove(moves.getMove(i));
}

void
MoveEmitterARM64::finish()
{
    assertDone();
    masm.freeStack(masm.framePushed() - pushedAtStart_);
    MOZ_ASSERT(masm.framePushed() == pushedAtStart_);
}

void
MoveEmitterARM64::emitMove(const MoveOp& move)
{
    const MoveOperand& from = move.from();
    const MoveOperand& to = move.to();

    if (move.isCycleBegin()) {
        MOZ_ASSERT(!inCycle_ && !move.isCycleEnd());
        breakCycle(from, to, move.endCycleType());
        inCycle_ = true;
    } else if (move.isCycleEnd()) {
        MOZ_ASSERT(inCycle_);
        completeCycle(from, to, move.type());
        inCycle_ = false;
        return;
    }

    switch (move.type()) {
      case MoveOp::FLOAT32:
        emitFloat32Move(from, to);
        break;
      case MoveOp::DOUBLE:
        emitDoubleMove(from, to);
        break;
      case MoveOp::INT32:
        emitInt32Move(from, to);
        break;
      case MoveOp::GENERAL:
        emitGeneralMove(from, to);
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM64::emitFloat32Move(const MoveOperand& from, const MoveOperand& to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.Fmov(toFPReg(to, MoveOp::FLOAT32), toFPReg(from, MoveOp::FLOAT32));
        else
            masm.Str(toFPReg(from, MoveOp::FLOAT32), toMemOperand(to));
        return;
    }

    if (to.isFloatReg()) {
        masm.Ldr(toFPReg(to, MoveOp::FLOAT32), toMemOperand(from));
        return;
    }

    vixl::UseScratchRegisterScope temps(&masm.asVIXL());
    const ARMFPRegister scratch32 = temps.AcquireS();
    masm.Ldr(scratch32, toMemOperand(from));
    masm.Str(scratch32, toMemOperand(to));
}

void
MoveEmitterARM64::emitDoubleMove(const MoveOperand& from, const MoveOperand& to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.Fmov(toFPReg(to, MoveOp::DOUBLE), toFPReg(from, MoveOp::DOUBLE));
        else
            masm.Str(toFPReg(from, MoveOp::DOUBLE), toMemOperand(to));
        return;
    }

    if (to.isFloatReg()) {
        masm.Ldr(toFPReg(to, MoveOp::DOUBLE), toMemOperand(from));
        return;
    }

    vixl::UseScratchRegisterScope temps(&masm.asVIXL());
    const ARMFPRegister scratch = temps.AcquireD();
    masm.Ldr(scratch, toMemOperand(from));
    masm.Str(scratch, toMemOperand(to));
}

void
MoveEmitterARM64::emitInt32Move(const MoveOperand& from, const MoveOperand& to)
{
    if (from.isGeneralReg()) {
        if (to.isGeneralReg())
            masm.Mov(toARMReg32(to), toARMReg32(from));
        else
            masm.Str(toARMReg32(from), toMemOperand(to));
        return;
    }

    if (to.isGeneralReg()) {
        masm.Ldr(toARMReg32(to), toMemOperand(from));
        return;
    }

    vixl::UseScratchRegisterScope temps(&masm.asVIXL());
    const ARMRegister scratch32 = temps.AcquireW();
    masm.Ldr(scratch32, toMemOperand(from));
    masm.Str(scratch32, toMemOperand(to));
}

void
MoveEmitterARM64::emitGeneralMove(const MoveOperand& from, const MoveOperand& to)
{
    if (from.isGeneralReg()) {
        MOZ_ASSERT(to.isGeneralReg() || to.isMemory());
        if (to.isGeneralReg())
            masm.Mov(toARMReg64(to), toARMReg64(from));
        else
            masm.Str(toARMReg64(from), toMemOperand(to));
        return;
    }

    
    if (to.isGeneralReg()) {
        MOZ_ASSERT(from.isMemoryOrEffectiveAddress());
        if (from.isMemory())
            masm.Ldr(toARMReg64(to), toMemOperand(from));
        else
            masm.Add(toARMReg64(to), toARMReg64(from), Operand(from.disp()));
        return;
    }

    vixl::UseScratchRegisterScope temps(&masm.asVIXL());
    const ARMRegister scratch64 = temps.AcquireX();

    
    if (from.isMemory()) {
        MOZ_ASSERT(to.isMemory());
        masm.Ldr(scratch64, toMemOperand(from));
        masm.Str(scratch64, toMemOperand(to));
        return;
    }

    
    MOZ_ASSERT(from.isEffectiveAddress());
    MOZ_ASSERT(to.isMemory());
    masm.Add(scratch64, toARMReg64(from), Operand(from.disp()));
    masm.Str(scratch64, toMemOperand(to));
}

MemOperand
MoveEmitterARM64::cycleSlot()
{
    
    MOZ_ASSERT(!masm.GetStackPointer64().Is(sp));

    
    MOZ_ASSERT(pushedAtCycle_ != -1);

    return MemOperand(masm.GetStackPointer64(), masm.framePushed() - pushedAtCycle_);
}

void
MoveEmitterARM64::breakCycle(const MoveOperand& from, const MoveOperand& to, MoveOp::Type type)
{
    switch (type) {
      case MoveOp::FLOAT32:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMFPRegister scratch32 = temps.AcquireS();
            masm.Ldr(scratch32, toMemOperand(to));
            masm.Str(scratch32, cycleSlot());
        } else {
            masm.Str(toFPReg(to, type), cycleSlot());
        }
        break;

      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMFPRegister scratch64 = temps.AcquireD();
            masm.Ldr(scratch64, toMemOperand(to));
            masm.Str(scratch64, cycleSlot());
        } else {
            masm.Str(toFPReg(to, type), cycleSlot());
        }
        break;

      case MoveOp::INT32:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMRegister scratch32 = temps.AcquireW();
            masm.Ldr(scratch32, toMemOperand(to));
            masm.Str(scratch32, cycleSlot());
        } else {
            masm.Str(toARMReg32(to), cycleSlot());
        }
        break;

      case MoveOp::GENERAL:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMRegister scratch64 = temps.AcquireX();
            masm.Ldr(scratch64, toMemOperand(to));
            masm.Str(scratch64, cycleSlot());
        } else {
            masm.Str(toARMReg64(to), cycleSlot());
        }
        break;

      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM64::completeCycle(const MoveOperand& from, const MoveOperand& to, MoveOp::Type type)
{
    switch (type) {
      case MoveOp::FLOAT32:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMFPRegister scratch32 = temps.AcquireS();
            masm.Ldr(scratch32, cycleSlot());
            masm.Str(scratch32, toMemOperand(to));
        } else {
            masm.Ldr(toFPReg(to, type), cycleSlot());
        }
        break;

      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMFPRegister scratch = temps.AcquireD();
            masm.Ldr(scratch, cycleSlot());
            masm.Str(scratch, toMemOperand(to));
        } else {
            masm.Ldr(toFPReg(to, type), cycleSlot());
        }
        break;

      case MoveOp::INT32:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMRegister scratch32 = temps.AcquireW();
            masm.Ldr(scratch32, cycleSlot());
            masm.Str(scratch32, toMemOperand(to));
        } else {
            masm.Ldr(toARMReg64(to), cycleSlot());
        }
        break;

      case MoveOp::GENERAL:
        if (to.isMemory()) {
            vixl::UseScratchRegisterScope temps(&masm.asVIXL());
            const ARMRegister scratch64 = temps.AcquireX();
            masm.Ldr(scratch64, cycleSlot());
            masm.Str(scratch64, toMemOperand(to));
        } else {
            masm.Ldr(toARMReg64(to), cycleSlot());
        }
        break;

      default:
        MOZ_CRASH("Unexpected move type");
    }
}
