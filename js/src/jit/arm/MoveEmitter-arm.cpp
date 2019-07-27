





#include "jit/arm/MoveEmitter-arm.h"

using namespace js;
using namespace js::jit;

MoveEmitterARM::MoveEmitterARM(MacroAssemblerARMCompat &masm)
  : inCycle_(false),
    masm(masm),
    pushedAtCycle_(-1),
    pushedAtSpill_(-1),
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
    int offset =  masm.framePushed() - pushedAtCycle_;
    JS_ASSERT(offset < 4096 && offset > -4096);
    return Operand(StackPointer, offset);
}


Operand
MoveEmitterARM::spillSlot() const
{
    int offset =  masm.framePushed() - pushedAtSpill_;
    JS_ASSERT(offset < 4096 && offset > -4096);
    return Operand(StackPointer, offset);
}

Operand
MoveEmitterARM::toOperand(const MoveOperand &operand, bool isFloat) const
{
    if (operand.isMemoryOrEffectiveAddress()) {
        if (operand.base() != StackPointer) {
            JS_ASSERT(operand.disp() < 1024 && operand.disp() > -1024);
            return Operand(operand.base(), operand.disp());
        }

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
MoveEmitterARM::breakCycle(const MoveOperand &from, const MoveOperand &to, MoveOp::Type type)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::FLOAT32:
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            FloatRegister temp = ScratchDoubleReg;
            masm.ma_vldr(toOperand(to, true), temp);
            masm.ma_vstr(temp, cycleSlot());
        } else {
            masm.ma_vstr(to.floatReg(), cycleSlot());
        }
        break;
      case MoveOp::INT32:
      case MoveOp::GENERAL:
        
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.ma_ldr(toOperand(to, false), temp);
            masm.ma_str(temp, cycleSlot());
        } else {
            if (to.reg() == spilledReg_) {
                
                masm.ma_ldr(spillSlot(), spilledReg_);
                spilledReg_ = InvalidReg;
            }
            masm.ma_str(to.reg(), cycleSlot());
        }
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM::completeCycle(const MoveOperand &from, const MoveOperand &to, MoveOp::Type type)
{
    
    
    
    
    
    
    switch (type) {
      case MoveOp::FLOAT32:
      case MoveOp::DOUBLE:
        if (to.isMemory()) {
            FloatRegister temp = ScratchDoubleReg;
            masm.ma_vldr(cycleSlot(), temp);
            masm.ma_vstr(temp, toOperand(to, true));
        } else {
            masm.ma_vldr(cycleSlot(), to.floatReg());
        }
        break;
      case MoveOp::INT32:
      case MoveOp::GENERAL:
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.ma_ldr(cycleSlot(), temp);
            masm.ma_str(temp, toOperand(to, false));
        } else {
            if (to.reg() == spilledReg_) {
                
                spilledReg_ = InvalidReg;
            }
            masm.ma_ldr(cycleSlot(), to.reg());
        }
        break;
      default:
        MOZ_CRASH("Unexpected move type");
    }
}

void
MoveEmitterARM::emitMove(const MoveOperand &from, const MoveOperand &to)
{
    if (to.isGeneralReg() && to.reg() == spilledReg_) {
        
        
        spilledReg_ = InvalidReg;
    }

    if (from.isGeneralReg()) {
        if (from.reg() == spilledReg_) {
            
            
            masm.ma_ldr(spillSlot(), spilledReg_);
            spilledReg_ = InvalidReg;
        }
        switch (toOperand(to, false).getTag()) {
          case Operand::OP2:
            
            masm.ma_mov(from.reg(), to.reg());
            break;
          case Operand::MEM:
            masm.ma_str(from.reg(), toOperand(to, false));
            break;
          default:
            MOZ_CRASH("strange move!");
        }
    } else if (to.isGeneralReg()) {
        JS_ASSERT(from.isMemoryOrEffectiveAddress());
        if (from.isMemory())
            masm.ma_ldr(toOperand(from, false), to.reg());
        else
            masm.ma_add(from.base(), Imm32(from.disp()), to.reg());
    } else {
        
        Register reg = tempReg();

        JS_ASSERT(from.isMemoryOrEffectiveAddress());
        if (from.isMemory())
            masm.ma_ldr(toOperand(from, false), reg);
        else
            masm.ma_add(from.base(), Imm32(from.disp()), reg);
        JS_ASSERT(to.base() != reg);
        masm.ma_str(reg, toOperand(to, false));
    }
}

void
MoveEmitterARM::emitFloat32Move(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.ma_vmov_f32(from.floatReg(), to.floatReg());
        else
            masm.ma_vstr(VFPRegister(from.floatReg()).singleOverlay(),
                         toOperand(to, true));
    } else if (to.isFloatReg()) {
        masm.ma_vldr(toOperand(from, true),
                     VFPRegister(to.floatReg()).singleOverlay());
    } else {
        
        JS_ASSERT(from.isMemory());
        FloatRegister reg = ScratchFloat32Reg;
        masm.ma_vldr(toOperand(from, true),
                     VFPRegister(reg).singleOverlay());
        masm.ma_vstr(VFPRegister(reg).singleOverlay(),
                     toOperand(to, true));
    }
}

void
MoveEmitterARM::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (to.isFloatReg())
            masm.ma_vmov(from.floatReg(), to.floatReg());
        else
            masm.ma_vstr(from.floatReg(), toOperand(to, true));
    } else if (to.isFloatReg()) {
        masm.ma_vldr(toOperand(from, true), to.floatReg());
    } else {
        
        JS_ASSERT(from.isMemory());
        FloatRegister reg = ScratchDoubleReg;
        masm.ma_vldr(toOperand(from, true), reg);
        masm.ma_vstr(reg, toOperand(to, true));
    }
}

void
MoveEmitterARM::emit(const MoveOp &move)
{
    const MoveOperand &from = move.from();
    const MoveOperand &to = move.to();

    if (move.isCycleEnd()) {
        JS_ASSERT(inCycle_);
        completeCycle(from, to, move.type());
        inCycle_ = false;
        return;
    }

    if (move.isCycleBegin()) {
        JS_ASSERT(!inCycle_);
        breakCycle(from, to, move.endCycleType());
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
    JS_ASSERT(!inCycle_);
}

void
MoveEmitterARM::finish()
{
    assertDone();

    if (pushedAtSpill_ != -1 && spilledReg_ != InvalidReg)
        masm.ma_ldr(spillSlot(), spilledReg_);
    masm.freeStack(masm.framePushed() - pushedAtStart_);
}
