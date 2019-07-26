






#if !defined(jsion_baseline_helpers_x64_h__) && defined(JS_ION)
#define jsion_baseline_helpers_x64_h__

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

    
    masm.movq(Operand(BaselineStubReg, (int32_t) ICEntry::offsetOfFirstStub()),
              BaselineStubReg);

    
    
    masm.movq(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
              BaselineTailCallReg);

    
    masm.call(BaselineTailCallReg);
}

inline void
EmitTailCall(IonCode *target, MacroAssembler &masm, uint32_t argSize)
{
    
    masm.movq(BaselineFrameReg, ScratchReg);
    masm.addq(Imm32(BaselineFrame::FramePointerOffset), ScratchReg);
    masm.subq(BaselineStackReg, ScratchReg);

    
    masm.movq(ScratchReg, rdx);
    masm.subq(Imm32(argSize), rdx);
    masm.storePtr(rdx, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    masm.makeFrameDescriptor(ScratchReg, IonFrame_BaselineJS);
    masm.push(ScratchReg);
    masm.push(BaselineTailCallReg);
    masm.jmp(target);
}

inline void
EmitStubGuardFailure(MacroAssembler &masm)
{
    
    

    

    
    masm.movq(Operand(BaselineStubReg, ICStub::offsetOfNext()), BaselineStubReg);

    
    
    masm.movq(Operand(BaselineStubReg, ICStub::offsetOfStubCode()), BaselineTailCallReg);

    
    masm.jmp(Operand(BaselineTailCallReg));
}


} 
} 

#endif

