





#ifndef ion_MoveEmitter_x86_shared_h
#define ion_MoveEmitter_x86_shared_h

#include "ion/IonMacroAssembler.h"
#include "ion/MoveResolver.h"

namespace js {
namespace ion {

class CodeGenerator;

class MoveEmitterX86
{
    typedef MoveResolver::Move Move;
    typedef MoveResolver::MoveOperand MoveOperand;

    bool inCycle_;
    MacroAssemblerSpecific &masm;

    
    uint32_t pushedAtStart_;

    
    
    int32_t pushedAtCycle_;

    void assertDone();
    Operand cycleSlot();
    Operand toOperand(const MoveOperand &operand) const;
    Operand toPopOperand(const MoveOperand &operand) const;

    size_t characterizeCycle(const MoveResolver &moves, size_t i,
                             bool *allGeneralRegs, bool *allFloatRegs);
    bool maybeEmitOptimizedCycle(const MoveResolver &moves, size_t i,
                                 bool allGeneralRegs, bool allFloatRegs, size_t swapCount);
    void emitGeneralMove(const MoveOperand &from, const MoveOperand &to);
    void emitDoubleMove(const MoveOperand &from, const MoveOperand &to);
    void breakCycle(const MoveOperand &to, Move::Kind kind);
    void completeCycle(const MoveOperand &to, Move::Kind kind);

  public:
    MoveEmitterX86(MacroAssemblerSpecific &masm);
    ~MoveEmitterX86();
    void emit(const MoveResolver &moves);
    void finish();
};

typedef MoveEmitterX86 MoveEmitter;

} 
} 

#endif 
