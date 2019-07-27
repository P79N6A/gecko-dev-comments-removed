





#ifndef jit_arm64_MoveEmitter_arm64_h
#define jit_arm64_MoveEmitter_arm64_h

#include "jit/arm64/Assembler-arm64.h"
#include "jit/MacroAssembler.h"
#include "jit/MoveResolver.h"

namespace js {
namespace jit {

class CodeGenerator;

class MoveEmitterARM64
{
    bool inCycle_;
    MacroAssembler& masm;

    
    uint32_t pushedAtStart_;

    
    
    
    int32_t pushedAtCycle_;
    int32_t pushedAtSpill_;

    void assertDone() {
        MOZ_ASSERT(!inCycle_);
    }

    MemOperand cycleSlot();
    MemOperand toMemOperand(const MoveOperand& operand) const {
        MOZ_ASSERT(operand.isMemory());
        ARMRegister base(operand.base(), 64);
        if (operand.base() == masm.getStackPointer())
            return MemOperand(base, operand.disp() + (masm.framePushed() -  pushedAtStart_));
        return MemOperand(base, operand.disp());
    }
    ARMRegister toARMReg32(const MoveOperand& operand) const {
        MOZ_ASSERT(operand.isGeneralReg());
        return ARMRegister(operand.reg(), 32);
    }
    ARMRegister toARMReg64(const MoveOperand& operand) const {
        if (operand.isGeneralReg())
            return ARMRegister(operand.reg(), 64);
        else
            return ARMRegister(operand.base(), 64);
    }
    ARMFPRegister toFPReg(const MoveOperand& operand, MoveOp::Type t) const {
        MOZ_ASSERT(operand.isFloatReg());
        return ARMFPRegister(operand.floatReg().encoding(), t == MoveOp::FLOAT32 ? 32 : 64);
    }

    void emitFloat32Move(const MoveOperand& from, const MoveOperand& to);
    void emitDoubleMove(const MoveOperand& from, const MoveOperand& to);
    void emitInt32Move(const MoveOperand& from, const MoveOperand& to);
    void emitGeneralMove(const MoveOperand& from, const MoveOperand& to);

    void emitMove(const MoveOp& move);
    void breakCycle(const MoveOperand& from, const MoveOperand& to, MoveOp::Type type);
    void completeCycle(const MoveOperand& from, const MoveOperand& to, MoveOp::Type type);

  public:
    MoveEmitterARM64(MacroAssembler& masm)
      : inCycle_(false),
        masm(masm),
        pushedAtStart_(masm.framePushed()),
        pushedAtCycle_(-1),
        pushedAtSpill_(-1)
    { }

    ~MoveEmitterARM64() {
        assertDone();
    }

    void emit(const MoveResolver& moves);
    void finish();
    void setScratchRegister(Register reg) {}
};

typedef MoveEmitterARM64 MoveEmitter;

} 
} 

#endif 
