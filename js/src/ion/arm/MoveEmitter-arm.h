








































#ifndef jsion_move_resolver_arm_shared_h__
#define jsion_move_resolver_arm_shared_h__

#include "ion/MoveResolver.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

class CodeGenerator;

class MoveEmitterARM
{
    typedef MoveResolver::Move Move;
    typedef MoveResolver::MoveOperand MoveOperand;

    bool inCycle_;
    MacroAssemblerARM &masm;

    
    uint32 pushedAtStart_;

    
    
    
    int32 pushedAtCycle_;
    int32 pushedAtSpill_;
    int32 pushedAtDoubleSpill_;

    
    
    
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
    MoveEmitterARM(MacroAssemblerARM &masm);
    ~MoveEmitterARM();
    void emit(const MoveResolver &moves);
    void finish();
};

typedef MoveEmitterARM MoveEmitter;

} 
} 

#endif 

