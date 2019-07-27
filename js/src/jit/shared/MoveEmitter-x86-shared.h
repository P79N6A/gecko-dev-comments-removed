





#ifndef jit_MoveEmitter_x86_shared_h
#define jit_MoveEmitter_x86_shared_h

#if defined(JS_CODEGEN_X86)
# include "jit/x86/MacroAssembler-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/MacroAssembler-x64.h"
#else
# error "Wrong architecture. Only x86 and x64 should build this file!"
#endif
#include "jit/MoveResolver.h"

namespace js {
namespace jit {

class CodeGenerator;

class MoveEmitterX86
{
    bool inCycle_;
    MacroAssemblerSpecific &masm;

    
    uint32_t pushedAtStart_;

    
    
    int32_t pushedAtCycle_;

    void assertDone();
    Address cycleSlot();
    Address toAddress(const MoveOperand &operand) const;
    Operand toOperand(const MoveOperand &operand) const;
    Operand toPopOperand(const MoveOperand &operand) const;

    size_t characterizeCycle(const MoveResolver &moves, size_t i,
                             bool *allGeneralRegs, bool *allFloatRegs);
    bool maybeEmitOptimizedCycle(const MoveResolver &moves, size_t i,
                                 bool allGeneralRegs, bool allFloatRegs, size_t swapCount);
    void emitInt32Move(const MoveOperand &from, const MoveOperand &to);
    void emitGeneralMove(const MoveOperand &from, const MoveOperand &to);
    void emitFloat32Move(const MoveOperand &from, const MoveOperand &to);
    void emitDoubleMove(const MoveOperand &from, const MoveOperand &to);
    void emitInt32X4Move(const MoveOperand &from, const MoveOperand &to);
    void breakCycle(const MoveOperand &to, MoveOp::Type type);
    void completeCycle(const MoveOperand &to, MoveOp::Type type);

  public:
    explicit MoveEmitterX86(MacroAssemblerSpecific &masm);
    ~MoveEmitterX86();
    void emit(const MoveResolver &moves);
    void finish();
};

typedef MoveEmitterX86 MoveEmitter;

} 
} 

#endif 
