






#if !defined(jsion_baseline_helpers_x86_h__) && defined(JS_ION)
#define jsion_baseline_helpers_x86_h__

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

    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICEntry::offsetOfFirstStub()),
              BaselineStubReg);

    
    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
              BaselineTailCallReg);

    
    masm.call(BaselineTailCallReg);
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler &masm)
{
    
    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICMonitoredStub::offsetOfFirstMonitorStub()),
              BaselineStubReg);

    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
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
    

    
    masm.movl(BaselineFrameReg, eax);
    masm.addl(Imm32(BaselineFrame::FramePointerOffset), eax);
    masm.subl(BaselineStackReg, eax);

    
    masm.movl(eax, ebx);
    masm.subl(Imm32(argSize), ebx);
    masm.store32(ebx, Operand(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    masm.makeFrameDescriptor(eax, IonFrame_BaselineJS);
    masm.push(eax);
    masm.push(BaselineTailCallReg);
    masm.jmp(target);
}

inline void
EmitCreateStubFrameDescriptor(MacroAssembler &masm, Register reg)
{
    
    
    masm.movl(BaselineFrameReg, reg);
    masm.addl(Imm32(sizeof(void *) * 2), reg);
    masm.subl(BaselineStackReg, reg);

    masm.makeFrameDescriptor(reg, IonFrame_BaselineStub);
}

inline void
EmitCallVM(IonCode *target, MacroAssembler &masm)
{
    EmitCreateStubFrameDescriptor(masm, eax);
    masm.push(eax);
    masm.call(target);
}

inline void
EmitEnterStubFrame(MacroAssembler &masm, Register scratch)
{
    JS_ASSERT(scratch != BaselineTailCallReg);

    EmitRestoreTailCallReg(masm);

    
    masm.movl(BaselineFrameReg, scratch);
    masm.addl(Imm32(BaselineFrame::FramePointerOffset), scratch);
    masm.subl(BaselineStackReg, scratch);

    masm.store32(scratch, Operand(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    masm.makeFrameDescriptor(scratch, IonFrame_BaselineJS);
    masm.push(scratch);
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

    
    masm.pop(eax);

    
    
    masm.push(BaselineTailCallReg);
}

inline void
EmitStowICValues(MacroAssembler &masm, int values)
{
    JS_ASSERT(values >= 0 && values <= 2);
    switch(values) {
      case 1:
        
        masm.pop(BaselineTailCallReg);
        masm.pushValue(R0);
        masm.push(BaselineTailCallReg);
        break;
      case 2:
        
        masm.pop(BaselineTailCallReg);
        masm.pushValue(R0);
        masm.pushValue(R1);
        masm.push(BaselineTailCallReg);
        break;
    }
}

inline void
EmitUnstowICValues(MacroAssembler &masm, int values)
{
    JS_ASSERT(values >= 0 && values <= 2);
    switch(values) {
      case 1:
        
        masm.pop(BaselineTailCallReg);
        masm.popValue(R0);
        masm.push(BaselineTailCallReg);
        break;
      case 2:
        
        masm.pop(BaselineTailCallReg);
        masm.popValue(R1);
        masm.popValue(R0);
        masm.push(BaselineTailCallReg);
        break;
    }
}

inline void
EmitCallTypeUpdateIC(MacroAssembler &masm, IonCode *code)
{
    

    
    masm.push(BaselineStubReg);

    
    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICUpdatedStub::offsetOfFirstUpdateStub()),
              BaselineStubReg);

    
    masm.movl(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()),
              BaselineTailCallReg);

    
    masm.call(BaselineTailCallReg);

    
    masm.pop(BaselineStubReg);

    
    
    Label success;
    masm.cmp32(R1.scratchReg(), Imm32(1));
    masm.j(Assembler::Equal, &success);

    
    EmitEnterStubFrame(masm, R1.scratchReg());

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    EmitCallVM(code, masm);
    EmitLeaveStubFrame(masm);

    
    masm.bind(&success);
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

