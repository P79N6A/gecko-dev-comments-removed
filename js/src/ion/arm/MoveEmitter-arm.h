





#ifndef ion_arm_MoveEmitter_arm_h
#define ion_arm_MoveEmitter_arm_h

#include "ion/IonMacroAssembler.h"
#include "ion/MoveResolver.h"

namespace js {
namespace ion {

class CodeGenerator;

class MoveEmitterARM
{
    typedef MoveResolver::Move Move;
    typedef MoveResolver::MoveOperand MoveOperand;

    bool inCycle_;
    MacroAssemblerARMCompat &masm;

    
    uint32_t pushedAtStart_;

    
    
    
    int32_t pushedAtCycle_;
    int32_t pushedAtSpill_;
    int32_t pushedAtDoubleSpill_;

    
    
    
    Register spilledReg_;
    FloatRegister spilledFloatReg_;

    void assertDone();
    Register tempReg();
    FloatRegister tempFloatReg();
    Operand cycleSlot() const;
    Operand spillSlot() const;
    Operand doubleSpillSlot() const;
    Operand toOperand(const MoveOperand &operand, bool isFloat) const;

    void emitMove(const MoveOperand &from, const MoveOperand &to);
    void emitDoubleMove(const MoveOperand &from, const MoveOperand &to);
    void breakCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind);
    void completeCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind);
    void emit(const Move &move);

  public:
    MoveEmitterARM(MacroAssemblerARMCompat &masm);
    ~MoveEmitterARM();
    void emit(const MoveResolver &moves);
    void finish();
};

typedef MoveEmitterARM MoveEmitter;

} 
} 

#endif 
