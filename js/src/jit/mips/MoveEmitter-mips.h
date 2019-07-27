





#ifndef jit_mips_MoveEmitter_mips_h
#define jit_mips_MoveEmitter_mips_h

#include "jit/IonMacroAssembler.h"
#include "jit/MoveResolver.h"

namespace js {
namespace jit {

class CodeGenerator;

class MoveEmitterMIPS
{
    uint32_t inCycle_;
    MacroAssemblerMIPSCompat &masm;

    
    uint32_t pushedAtStart_;

    
    
    
    int32_t pushedAtCycle_;
    int32_t pushedAtSpill_;

    
    
    
    Register spilledReg_;
    FloatRegister spilledFloatReg_;

    void assertDone();
    Register tempReg();
    FloatRegister tempFloatReg();
    Address cycleSlot(uint32_t slot, uint32_t subslot) const;
    int32_t getAdjustedOffset(const MoveOperand &operand);
    Address getAdjustedAddress(const MoveOperand &operand);

    void emitMove(const MoveOperand &from, const MoveOperand &to);
    void emitFloat32Move(const MoveOperand &from, const MoveOperand &to);
    void emitDoubleMove(const MoveOperand &from, const MoveOperand &to);
    void breakCycle(const MoveOperand &from, const MoveOperand &to,
                    MoveOp::Type type, uint32_t slot);
    void completeCycle(const MoveOperand &from, const MoveOperand &to,
                       MoveOp::Type type, uint32_t slot);
    void emit(const MoveOp &move);

  public:
    MoveEmitterMIPS(MacroAssemblerMIPSCompat &masm);
    ~MoveEmitterMIPS();
    void emit(const MoveResolver &moves);
    void finish();
};

typedef MoveEmitterMIPS MoveEmitter;

} 
} 

#endif 
