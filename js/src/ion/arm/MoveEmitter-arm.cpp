








































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
    int offset =  masm.framePushed() - pushedAtCycle_;
    JS_ASSERT(offset < 4096 && offset > -4096);
    return Operand(DTRAddr(StackPointer, DtrOffImm(offset)));
}


Operand
MoveEmitterARM::spillSlot() const
{
    int offset =  masm.framePushed() - pushedAtSpill_;
    JS_ASSERT(offset < 4096 && offset > -4096);
    return Operand(DTRAddr(StackPointer, DtrOffImm(offset)));
}

Operand
MoveEmitterARM::doubleSpillSlot() const
{
    int offset =  masm.framePushed() - pushedAtCycle_;
    JS_ASSERT(offset < 4096 && offset > -4096);
    
    
    
    
    
    return Operand(DTRAddr(StackPointer, DtrOffImm(offset)));
}

Operand
MoveEmitterARM::toOperand(const MoveOperand &operand) const
{
    if (operand.isMemory()) {
        if (operand.base() != StackPointer) {
            JS_ASSERT(operand.disp() < 4096 && operand.disp() > -4096);
            return Operand(DTRAddr(operand.base(),DtrOffImm(operand.disp())));
        }

        JS_ASSERT(operand.disp() >= 0);

        
        return Operand(DTRAddr(StackPointer,
                               DtrOffImm(operand.disp() + (masm.framePushed() - pushedAtStart_))));
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

    
    
    spilledReg_ = Register::FromCode(12);
    if (pushedAtSpill_ == -1) {
        masm.ma_push(spilledReg_);
        pushedAtSpill_ = masm.framePushed();
    } else {
        masm.ma_str(spilledReg_, spillSlot().toDTRAddr());
    }
    return spilledReg_;
}

FloatRegister
MoveEmitterARM::tempFloatReg()
{
    if (spilledFloatReg_ != InvalidFloatReg) {
        return spilledFloatReg_;
    }
    
    
    spilledFloatReg_ = FloatRegister::FromCode(7);
    if (pushedAtDoubleSpill_ == -1) {
        masm.reserveStack(sizeof(double));
        pushedAtDoubleSpill_ = masm.framePushed();
    }
    
    JS_NOT_REACHED("add vfp-offsets to the union type");
    return spilledFloatReg_;
}

void
MoveEmitterARM::breakCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (to.isDouble()) {
        if (to.isMemory()) {
            FloatRegister temp = tempFloatReg();
            masm.ma_vldr(toOperand(to).toVFPAddr(), temp);
            masm.ma_vstr(temp, cycleSlot().toVFPAddr());
        } else {
            masm.ma_vstr(to.floatReg(), cycleSlot().toVFPAddr());
        }
    } else {
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.ma_ldr(toOperand(to).toDTRAddr(), temp);
            masm.ma_str(temp, cycleSlot().toDTRAddr());
        } else {
            masm.ma_str(to.reg(), cycleSlot().toDTRAddr());
        }
    }
}

void
MoveEmitterARM::completeCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind)
{
    
    
    
    
    
    
    if (kind == Move::DOUBLE) {
        if (to.isMemory()) {
            FloatRegister temp = tempFloatReg();
            masm.ma_vldr(cycleSlot().toVFPAddr(), temp);
            masm.ma_vstr(temp, toOperand(to).toVFPAddr());
        } else {
            masm.ma_vldr(cycleSlot().toVFPAddr(), to.floatReg());
        }
    } else {
        if (to.isMemory()) {
            Register temp = tempReg();
            masm.ma_ldr(cycleSlot().toDTRAddr(), temp);
            masm.ma_str(temp, toOperand(to).toDTRAddr());
        } else {
            masm.ma_ldr(cycleSlot().toDTRAddr(), to.reg());
        }
    }
}

void
MoveEmitterARM::emitMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isGeneralReg()) {
        if (from.reg() == spilledReg_) {
            
            
            masm.ma_ldr(spillSlot().toDTRAddr(), spilledReg_);
            spilledReg_ = InvalidReg;
        }
        switch (toOperand(to).getTag()) {
        case Operand::OP2:
            
            masm.ma_mov(from.reg(), to.reg());
            break;
        case Operand::DTR:
            masm.ma_str(from.reg(), toOperand(to).toDTRAddr());
            break;
        default:
            JS_NOT_REACHED("strange move!");
        }
    } else if (to.isGeneralReg()) {
        if (to.reg() == spilledReg_) {
            
            
            spilledReg_ = InvalidReg;
        }
        masm.ma_ldr(toOperand(from).toDTRAddr(), to.reg());
    } else {
        
        Register reg = tempReg();
        masm.ma_ldr(toOperand(from).toDTRAddr(), reg);
        masm.ma_str(reg, toOperand(to).toDTRAddr());
    }
}

void
MoveEmitterARM::emitDoubleMove(const MoveOperand &from, const MoveOperand &to)
{
    if (from.isFloatReg()) {
        if (from.floatReg() == spilledFloatReg_) {
            
            
            masm.ma_vldr(doubleSpillSlot().toVFPAddr(), spilledFloatReg_);
            spilledFloatReg_ = InvalidFloatReg;
        }
        masm.ma_vstr(from.floatReg(), toOperand(to).toVFPAddr());
    } else if (to.isFloatReg()) {
        if (to.floatReg() == spilledFloatReg_) {
            
            
            spilledFloatReg_ = InvalidFloatReg;
        }
        masm.ma_vldr(toOperand(from).toVFPAddr(), to.floatReg());
    } else {
        
        FloatRegister reg = tempFloatReg();
        masm.ma_vldr(toOperand(from).toVFPAddr(), reg);
        masm.ma_vstr(reg, toOperand(to).toVFPAddr());
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

    if (pushedAtDoubleSpill_ != -1 && spilledFloatReg_ != InvalidFloatReg) {
        masm.ma_vldr(doubleSpillSlot().toVFPAddr(), spilledFloatReg_);
    }
    if (pushedAtSpill_ != -1 && spilledReg_ != InvalidReg) {
        masm.ma_ldr(spillSlot().toDTRAddr(), spilledReg_);
    }
    masm.freeStack(masm.framePushed() - pushedAtStart_);
}
