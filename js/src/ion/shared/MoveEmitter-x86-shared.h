








































#ifndef jsion_move_resolver_x86_shared_h__
#define jsion_move_resolver_x86_shared_h__

#include "ion/MoveResolver.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

class CodeGenerator;

class MoveEmitterX86
{
    typedef MoveResolver::Move Move;
    typedef MoveResolver::MoveOperand MoveOperand;

    bool inCycle_;
    MacroAssembler &masm;

    RegisterSet freeRegs_;

    
    uint32 pushedAtStart_;

    
    
    
    int32 pushedAtCycle_;
    int32 pushedAtSpill_;
    int32 pushedAtDoubleSpill_;

    
    
    
    Register spilledReg_;
    FloatRegister spilledFloatReg_;

    
    Register cycleReg_;
    FloatRegister cycleFloatReg_;

    void assertDone();
    void assertValidMove(const MoveOperand &from, const MoveOperand &to);
    Register tempReg();
    FloatRegister tempFloatReg();
    Operand cycleSlot() const;
    Operand spillSlot() const;
    Operand doubleSpillSlot() const;
    Operand toOperand(const MoveOperand &operand) const;

    void emitMove(const MoveOperand &from, const MoveOperand &to);
    void emitDoubleMove(const MoveOperand &from, const MoveOperand &to);
    void breakCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind);
    void completeCycle(const MoveOperand &from, const MoveOperand &to, Move::Kind kind);
    void emit(const Move &move);

  public:
    MoveEmitterX86(MacroAssembler &masm);
    ~MoveEmitterX86();
    void emit(const MoveResolver &moves, const RegisterSet &freeRegs);
    void finish();
};

typedef MoveEmitterX86 MoveEmitter;

} 
} 

#endif 

