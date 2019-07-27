





#ifndef jit_arm_MoveEmitter_arm_h
#define jit_arm_MoveEmitter_arm_h

#include "jit/MacroAssembler.h"
#include "jit/MoveResolver.h"

namespace js {
namespace jit {

class MoveEmitterARM
{
    uint32_t inCycle_;
    MacroAssembler& masm;

    
    uint32_t pushedAtStart_;

    
    
    
    int32_t pushedAtCycle_;
    int32_t pushedAtSpill_;

    
    
    
    Register spilledReg_;
    FloatRegister spilledFloatReg_;

    void assertDone();
    Register tempReg();
    FloatRegister tempFloatReg();
    Address cycleSlot(uint32_t slot, uint32_t subslot) const;
    Address spillSlot() const;
    Address toAddress(const MoveOperand& operand) const;

    void emitMove(const MoveOperand& from, const MoveOperand& to);
    void emitFloat32Move(const MoveOperand& from, const MoveOperand& to);
    void emitDoubleMove(const MoveOperand& from, const MoveOperand& to);
    void breakCycle(const MoveOperand& from, const MoveOperand& to,
                    MoveOp::Type type, uint32_t slot);
    void completeCycle(const MoveOperand& from, const MoveOperand& to,
                       MoveOp::Type type, uint32_t slot);
    void emit(const MoveOp& move);

  public:
    MoveEmitterARM(MacroAssembler& masm);
    ~MoveEmitterARM();
    void emit(const MoveResolver& moves);
    void finish();

    void setScratchRegister(Register reg) {}
};

typedef MoveEmitterARM MoveEmitter;

} 
} 

#endif 
