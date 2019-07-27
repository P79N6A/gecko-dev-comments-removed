





#ifndef jit_MoveEmitter_x86_shared_h
#define jit_MoveEmitter_x86_shared_h

#include "jit/MacroAssembler.h"
#include "jit/MoveResolver.h"

namespace js {
namespace jit {

class MoveEmitterX86
{
    bool inCycle_;
    MacroAssembler& masm;

    
    uint32_t pushedAtStart_;

    
    
    int32_t pushedAtCycle_;

#ifdef JS_CODEGEN_X86
    
    mozilla::Maybe<Register> scratchRegister_;
#endif

    void assertDone();
    Address cycleSlot();
    Address toAddress(const MoveOperand& operand) const;
    Operand toOperand(const MoveOperand& operand) const;
    Operand toPopOperand(const MoveOperand& operand) const;

    size_t characterizeCycle(const MoveResolver& moves, size_t i,
                             bool* allGeneralRegs, bool* allFloatRegs);
    bool maybeEmitOptimizedCycle(const MoveResolver& moves, size_t i,
                                 bool allGeneralRegs, bool allFloatRegs, size_t swapCount);
    void emitInt32Move(const MoveOperand& from, const MoveOperand& to);
    void emitGeneralMove(const MoveOperand& from, const MoveOperand& to);
    void emitFloat32Move(const MoveOperand& from, const MoveOperand& to);
    void emitDoubleMove(const MoveOperand& from, const MoveOperand& to);
    void emitFloat32X4Move(const MoveOperand& from, const MoveOperand& to);
    void emitInt32X4Move(const MoveOperand& from, const MoveOperand& to);
    void breakCycle(const MoveOperand& to, MoveOp::Type type);
    void completeCycle(const MoveOperand& to, MoveOp::Type type);

  public:
    explicit MoveEmitterX86(MacroAssembler& masm);
    ~MoveEmitterX86();
    void emit(const MoveResolver& moves);
    void finish();

    void setScratchRegister(Register reg) {
#ifdef JS_CODEGEN_X86
        scratchRegister_.emplace(reg);
#endif
    }

    bool hasScratchRegister() {
#ifdef JS_CODEGEN_X86
        return scratchRegister_.isSome();
#else
        return true;
#endif
    }

    Register scratchRegister() {
        MOZ_ASSERT(hasScratchRegister());
#ifdef JS_CODEGEN_X86
        return scratchRegister_.value();
#else
        return ScratchReg;
#endif
    }
};

typedef MoveEmitterX86 MoveEmitter;

} 
} 

#endif 
