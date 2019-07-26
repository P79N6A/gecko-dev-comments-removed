






#if !defined(jsion_baseline_helpers_x86_h__) && defined(JS_ION)
#define jsion_baseline_helpers_x86_h__

#include "ion/IonMacroAssembler.h"
#include "ion/BaselineRegisters.h"
#include "ion/BaselineIC.h"

namespace js {
namespace ion {

inline void
EmitCallIC(CodeOffsetLabel *patchOffset, MacroAssembler &masm)
{
    
    CodeOffsetLabel offset = masm.movWithPatch(ImmWord(-1), BaselineStubReg);
    *patchOffset = offset;

    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICEntry::offsetOfFirstStub()),
              BaselineStubReg);

    
    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
              BaselineTailCallReg);

    
    masm.call(BaselineTailCallReg);
}

inline void
EmitTailCall(IonCode *target, MacroAssembler &masm)
{
    
    masm.movl(BaselineFrameReg, eax);
    masm.subl(BaselineStackReg, eax);
    masm.addl(Imm32(4), eax);
    masm.makeFrameDescriptor(eax, IonFrame_BaselineJS);
    masm.push(eax);
    masm.push(BaselineTailCallReg);
    masm.jmp(target);
}

inline void
EmitStubGuardFailure(MacroAssembler &masm)
{
    
    

    

    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfNext()), BaselineStubReg);

    
    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
              BaselineTailCallReg);

    
    masm.jmp(Operand(BaselineTailCallReg));
}


} 
} 

#endif

