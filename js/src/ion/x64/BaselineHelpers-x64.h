






#if !defined(jsion_baseline_helpers_x64_h__) && defined(JS_ION)
#define jsion_baseline_helpers_x64_h__

#include "ion/IonMacroAssembler.h"
#include "ion/BaselineRegisters.h"
#include "ion/BaselineIC.h"

namespace js {
namespace ion {


static const size_t ICStackValueOffset = sizeof(void *);

inline void
EmitRestoreTailCallReg(MacroAssembler &masm)
{
    masm.pop(BaselineTailCallReg);
}

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
EmitEnterTypeMonitorIC(MacroAssembler &masm)
{
    
    
    masm.movq(Operand(BaselineStubReg, (int32_t) ICMonitoredStub::offsetOfFirstMonitorStub()),
              BaselineStubReg);

    
    masm.movq(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
              BaselineTailCallReg);

    
    masm.jmp(Operand(BaselineTailCallReg));
}

inline void
EmitReturnFromIC(MacroAssembler &masm)
{
    masm.ret();
}

inline void
EmitTailCallVM(IonCode *target, MacroAssembler &masm, uint32_t argSize)
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
EmitCreateStubFrameDescriptor(MacroAssembler &masm, Register reg)
{
    
    
    masm.movq(BaselineFrameReg, reg);
    masm.addq(Imm32(sizeof(void *) * 2), reg);
    masm.subq(BaselineStackReg, reg);

    masm.makeFrameDescriptor(reg, IonFrame_BaselineStub);
}

inline void
EmitCallVM(IonCode *target, MacroAssembler &masm)
{
    EmitCreateStubFrameDescriptor(masm, ScratchReg);
    masm.push(ScratchReg);
    masm.call(target);
}

inline void
EmitEnterStubFrame(MacroAssembler &masm, Register)
{
    EmitRestoreTailCallReg(masm);

    
    masm.movq(BaselineFrameReg, ScratchReg);
    masm.addq(Imm32(BaselineFrame::FramePointerOffset), ScratchReg);
    masm.subq(BaselineStackReg, ScratchReg);

    masm.storePtr(ScratchReg, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    masm.makeFrameDescriptor(ScratchReg, IonFrame_BaselineJS);
    masm.push(ScratchReg);
    masm.push(BaselineTailCallReg);

    
    masm.push(BaselineStubReg);
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);
}

inline void
EmitLeaveStubFrame(MacroAssembler &masm)
{
    
    masm.mov(BaselineFrameReg, BaselineStackReg);
    masm.pop(BaselineFrameReg);
    masm.pop(BaselineStubReg);

    
    masm.pop(BaselineTailCallReg);

    
    masm.pop(ScratchReg);

    
    
    masm.push(BaselineTailCallReg);
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

