





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
        const MoveOp &move = moves.getMove(j);

        
        
        if (!move.to().isGeneralReg())
            *allGeneralRegs = false;
        if (!move.to().isFloatReg())
            *allFloatRegs = false;
        if (!*allGeneralRegs && !*allFloatRegs)
            return -1;

        
        if (j != i && move.isCycleEnd())
            break;

        
        
        
        if (move.from() != moves.getMove(j + 1).to()) {
            *allGeneralRegs = false;
            *allFloatRegs = false;
            return -1;
        }

        swapCount++;
    }

    
    const MoveOp &move = moves.getMove(i + swapCount);
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
        const MoveOp &move = moves.getMove(i);
        const MoveOperand &from = move.from();
        const MoveOperand &to = move.to();

        if (move.isCycleEnd()) {
            JS_ASSERT(inCycle_);
            completeCycle(to, move.type());
            inCycle_ = false;
            continue;
        }

        if (move.isCycleBegin()) {
            JS_ASSERT(!inCycle_);

            
            bool allGeneralRegs = true, allFloatRegs = true;
            size_t swapCount = characterizeCycle(moves, i, &allGeneralRegs, &allFloatRegs);

            
            if (maybeEmitOptimizedCycle(moves, i, allGeneralRegs, allFloatRegs, swapCount)) {
                i += swapCount;
                continue;
            }

            
            breakCycle(to, move.endCycleType());
            inCycle_ = true;
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
          case MoveOp::INT32X4:
            emitInt32X4Move(from, to);
            break;
          case MoveOp::FLOAT32X4:
            emitFloat32X4Move(from, to);
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("Unexpected move type");
        }
    }
}

MoveEmitterX86::~MoveEmitterX86()
{
    assertDone();
}

Address
MoveEmitterX86::cycleSlot()
{
    if (pushedAtCycle_ == -1) {
        
        masm.reserveStack(Simd128DataSize);
        pushedAtCycle_ = masm.framePushed();
    }

    return Address(StackPointer, masm.framePushed() - pushedAtCycle_);
}

Address
MoveEmitterX86::toAddress(const MoveOperand &operand) const
{
    if (operand.base() != StackPointer)
        return Address(operand.base(), operand.disp());

    JS_ASSERT(operand.disp() >= 0);

    
    return Address(StackPointer, operand.disp() + (masm.framePushed() - pushedAtStart_));
}




Operand
MoveEmitterX86::toOperand(const MoveOperand &operand) const
{
    if (operand.isMemoryOrEffectiveAddress())
        return Operand(toAddress(operand));
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
MoveEmitterX86::breakCycle(const MoveOperand &to, MoveOp::Type type)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::INT32X4:
        if (to.isMemory()) {
            masm.loadAlignedInt32x4(toAddress(to), ScratchSimdReg);
            masm.storeAlignedInt32x4(ScratchSimdReg, cycleSlot());
        } else {
            masm.storeAlignedInt32x4(to.floatReg(), cycleSlot());
        }
        break;
      case MoveOp::FLOAT32X4:
        if (to.isMemory()) {
            masm.loadAlignedFloat32x4(toAddress(to), ScratchSimdReg);
            masm.storeAlignedFloat32x4(ScratchSimdReg, cycleSlot());
        } else {
            masm.storeAlignedFloat32x4(to.floatReg(), cycleSlot());
        }
        break;
      case MoveOp::FLOAT32:
        if (to.isMemory()) {
            masm.loadFloat32(toAddress(to), ScratchFloat32Reg);
            masm.storeFloat32(ScratchFloat32Reg, cycleSlot());
        } else {
            masm.storeFloat32(to.floatReg(), cycleSlot());
        }
        break;
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            masm.loadDouble(toAddress(to), ScratchDoubleReg);
            masm.storeDouble(ScratchDoubleReg, cycleSlot());
        } else {
            masm.storeDouble(to.floatReg(), cycleSlot());
        }
        break;
      case MoveOp::INT32:
#ifdef JS_CODEGEN_X64
        
        if (to.isMemory()) {
            masm.load32(toAddress(to), ScratchReg);
            masm.store32(ScratchReg, cycleSlot());
        } else {
            masm.store32(to.reg(), cycleSlot());
        }
        break;
#endif
      case MoveOp::GENERAL:
        masm.Push(toOperand(to));
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Unexpected move type");
    }
}

void
MoveEmitterX86::completeCycle(const MoveOperand &to, MoveOp::Type type)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::INT32X4:
        JS_ASSERT(pushedAtCycle_ != -1);
        JS_ASSERT(pushedAtCycle_ - pushedAtStart_ >= Simd128DataSize);
        if (to.isMemory()) {
            masm.loadAlignedInt32x4(cycleSlot(), ScratchSimdReg);
            masm.storeAlignedInt32x4(ScratchSimdReg, toAddress(to));
        } else {
            masm.loadAlignedInt32x4(cycleSlot(), to.floatReg());
        }
        break;
      case MoveOp::FLOAT32X4:
        JS_ASSERT(pushedAtCycle_ != -1);
        JS_ASSERT(pushedAtCycle_ - pushedAtStart_ >= Simd128DataSize);
        if (to.isMemory()) {
            masm.loadAlignedFloat32x4(cycleSlot(), ScratchSimdReg);
            masm.storeAlignedFloat32x4(ScratchSimdReg, toAddress(to));
        } else {
            masm.loadAlignedFloat32x4(cycleSlot(), to.floatReg());
        }
        break;
      case MoveOp::FLOAT32:
        JS_ASSERT(pushedAtCycle_ != -1);
        JS_ASSERT(pushedAtCycle_ - pushedAtStart_ >= sizeof(float));
        if (to.isMemory()) {
            masm.loadFloat32(cycleSlot(), ScratchFloat32Reg);
            masm.storeFloat32(ScratchFloat32Reg, toAddress(to));
        } else {
            masm.loadFloat32(cycleSlot(), to.floatReg());
        }
        break;
      case MoveOp::DOUBLE:
        JS_ASSERT(pushedAtCycle_ != -1);
        JS_ASSERT(pushedAtCycle_ - pushedAtStart_ >= sizeof(double));
        if (to.isMemory()) {
            masm.loadDouble(cycleSlot(), ScratchDoubleReg);
            masm.storeDouble(ScratchDoubleReg, toAddress(to));
        } else {
            masm.loadDouble(cycleSlot(), to.floatReg());
        }
        break;
      case MoveOp::INT32:
#ifdef JS_CODEGEN_X64
        JS_ASSERT(pushedAtCycle_ != -1);
        JS_ASSERT(pushedAtCycle_ - pushedAtStart_ >= sizeof(int32_t));
        
        if (to.isMemory()) {
            masm.load32(cycleSlot(), ScratchReg);
            masm.store32(ScratchReg, toAddress(to));
        } else {
            masm.load32(cycleSlot(), to.reg());
        }
        break;
#endif
      case MoveOp::GENERAL:
        JS_ASSERT(masm.framePushed() - pushedAtStart_ >= sizeof(intptr_t));
        masm.Pop(toPopOperand(to));
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Unexpected move type");
    }
}

void
MoveEmitterX86::emitInt32Move(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isGeneralReg()) {
        masm.move32(from.reg(), toOperand(to));
    } else if (to.isGeneralReg()) {
        JS_ASSERT(from.isMemory());
        masm.load32(toAddress(from), to.reg());
    } else {
        
        JS_ASSERT(from.isMemory());
#ifdef JS_CODEGEN_X64
        
        masm.load32(toAddress(from), ScratchReg);
        masm.move32(ScratchReg, toOperand(to));
#else
        
        masm.Push(toOperand(from));
        masm.Pop(toPopOperand(to));
#endif
    }
}

void
MoveEmitterX86::emitGeneralMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isGeneralReg()) {
        masm.mov(from.reg(), toOperand(to));
    } else if (to.isGeneralReg()) {
        JS_ASSERT(from.isMemoryOrEffectiveAddress());
        if (from.isMemory())
            masm.loadPtr(toAddress(from), to.reg());
        else
            masm.lea(toOperand(from), to.reg());
    } else if (from.isMemory()) {
        
#ifdef JS_CODEGEN_X64
        
        masm.loadPtr(toAddress(from), ScratchReg);
        masm.mov(ScratchReg, toOperand(to));
#else
        
        masm.Push(toOperand(from));
        masm.Pop(toPopOperand(to));
#endif
    } else {
        
        JS_ASSERT(from.isEffectiveAddress());
#ifdef JS_CODEGEN_X64
        
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
MoveEmitterX86::emitFloat32Move(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.moveFloat32(from.floatReg(), to.floatReg());
        else
            masm.storeFloat32(from.floatReg(), toAddress(to));
    } else if (to.isFloatReg()) {
        masm.loadFloat32(toAddress(from), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        masm.loadFloat32(toAddress(from), ScratchFloat32Reg);
        masm.storeFloat32(ScratchFloat32Reg, toAddress(to));
    }
}

void
MoveEmitterX86::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.moveDouble(from.floatReg(), to.floatReg());
        else
            masm.storeDouble(from.floatReg(), toAddress(to));
    } else if (to.isFloatReg()) {
        masm.loadDouble(toAddress(from), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        masm.loadDouble(toAddress(from), ScratchDoubleReg);
        masm.storeDouble(ScratchDoubleReg, toAddress(to));
    }
}

void
MoveEmitterX86::emitInt32X4Move(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.moveAlignedInt32x4(from.floatReg(), to.floatReg());
        else
            masm.storeAlignedInt32x4(from.floatReg(), toAddress(to));
    } else if (to.isFloatReg()) {
        masm.loadAlignedInt32x4(toAddress(from), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        masm.loadAlignedInt32x4(toAddress(from), ScratchSimdReg);
        masm.storeAlignedInt32x4(ScratchSimdReg, toAddress(to));
    }
}

void
MoveEmitterX86::emitFloat32X4Move(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.moveAlignedFloat32x4(from.floatReg(), to.floatReg());
        else
            masm.storeAlignedFloat32x4(from.floatReg(), toAddress(to));
    } else if (to.isFloatReg()) {
        masm.loadAlignedFloat32x4(toAddress(from), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        masm.loadAlignedFloat32x4(toAddress(from), ScratchSimdReg);
        masm.storeAlignedFloat32x4(ScratchSimdReg, toAddress(to));
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

