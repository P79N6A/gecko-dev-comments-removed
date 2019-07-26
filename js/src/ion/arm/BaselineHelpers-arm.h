






#if !defined(jsion_baseline_helpers_arm_h__) && defined(JS_ION)
#define jsion_baseline_helpers_arm_h__

#include "ion/IonMacroAssembler.h"
#include "ion/BaselineRegisters.h"
#include "ion/BaselineIC.h"

namespace js {
namespace ion {


static const size_t ICStackValueOffset = 0;

inline void
EmitRestoreTailCallReg(MacroAssembler &masm)
{
    
}

inline void
EmitCallIC(CodeOffsetLabel *patchOffset, MacroAssembler &masm)
{
    
    CodeOffsetLabel offset = masm.movWithPatch(ImmWord(-1), BaselineStubReg);
    *patchOffset = offset;

    
    masm.loadPtr(Address(BaselineStubReg, ICEntry::offsetOfFirstStub()), BaselineStubReg);

    
    
    JS_ASSERT(R2 == ValueOperand(r1, r0));
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.ma_blx(r0);
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler &masm)
{
    
    
    masm.loadPtr(Address(BaselineStubReg, ICMonitoredStub::offsetOfFirstMonitorStub()),
                 BaselineStubReg);

    
    
    JS_ASSERT(R2 == ValueOperand(r1, r0));
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.branch(r0);
}

inline void
EmitReturnFromIC(MacroAssembler &masm)
{
    masm.ma_mov(lr, pc);
}

inline void
EmitTailCallVM(IonCode *target, MacroAssembler &masm, uint32_t argSize)
{
    
    
    JS_ASSERT(R2 == ValueOperand(r1, r0));

    
    masm.movePtr(BaselineFrameReg, r0);
    masm.ma_add(Imm32(BaselineFrame::FramePointerOffset), r0);
    masm.ma_sub(BaselineStackReg, r0);

    
    masm.ma_sub(r0, Imm32(argSize), r1);
    masm.storePtr(r1, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    
    
    
    JS_ASSERT(BaselineTailCallReg == lr);
    masm.makeFrameDescriptor(r0, IonFrame_BaselineJS);
    masm.push(r0);
    masm.push(lr);
    masm.branch(target);
}

inline void
EmitCreateStubFrameDescriptor(MacroAssembler &masm, Register reg)
{
    
    
    masm.mov(BaselineFrameReg, reg);
    masm.ma_add(Imm32(sizeof(void *) * 2), reg);
    masm.ma_sub(BaselineStackReg, reg);

    masm.makeFrameDescriptor(reg, IonFrame_BaselineStub);
}

inline void
EmitCallVM(IonCode *target, MacroAssembler &masm)
{
    EmitCreateStubFrameDescriptor(masm, r0);
    masm.push(r0);
    masm.call(target);
}

inline void
EmitEnterStubFrame(MacroAssembler &masm, Register scratch)
{
    JS_ASSERT(scratch != BaselineTailCallReg);

    
    masm.mov(BaselineFrameReg, scratch);
    masm.ma_add(Imm32(BaselineFrame::FramePointerOffset), scratch);
    masm.ma_sub(BaselineStackReg, scratch);

    masm.storePtr(scratch, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    masm.makeFrameDescriptor(scratch, IonFrame_BaselineJS);
    masm.push(scratch);
    masm.push(BaselineTailCallReg);

    
    masm.push(BaselineStubReg);
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);

    
    masm.checkStackAlignment();
}

inline void
EmitLeaveStubFrame(MacroAssembler &masm, bool calledIntoIon = false)
{
    
    
    
    
    if (calledIntoIon) {
        masm.pop(ScratchRegister);
        masm.ma_lsr(Imm32(FRAMESIZE_SHIFT), ScratchRegister, ScratchRegister);
        masm.ma_add(ScratchRegister, BaselineStackReg);
    } else {
        masm.mov(BaselineFrameReg, BaselineStackReg);
    }

    masm.pop(BaselineFrameReg);
    masm.pop(BaselineStubReg);

    
    masm.pop(BaselineTailCallReg);

    
    masm.pop(ScratchRegister);
}

inline void
EmitStowICValues(MacroAssembler &masm, int values)
{
    JS_ASSERT(values >= 0 && values <= 2);
    switch(values) {
      case 1:
        
        masm.pushValue(R0);
        break;
      case 2:
        
        masm.pushValue(R0);
        masm.pushValue(R1);
        break;
    }
}

inline void
EmitUnstowICValues(MacroAssembler &masm, int values)
{
    JS_ASSERT(values >= 0 && values <= 2);
    switch(values) {
      case 1:
        
        masm.popValue(R0);
        break;
      case 2:
        
        masm.popValue(R1);
        masm.popValue(R0);
        break;
    }
}

inline void
EmitCallTypeUpdateIC(MacroAssembler &masm, IonCode *code)
{
    JS_ASSERT(R2 == ValueOperand(r1, r0));
    

    
    
    masm.push(BaselineStubReg);
    masm.push(BaselineTailCallReg);

    
    
    masm.loadPtr(Address(BaselineStubReg, ICUpdatedStub::offsetOfFirstUpdateStub()),
                 BaselineStubReg);

    

    
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.ma_blx(r0);

    
    masm.pop(BaselineTailCallReg);
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
    JS_ASSERT(R2 == ValueOperand(r1, r0));

    
    

    

    
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfNext()), BaselineStubReg);

    
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    JS_ASSERT(BaselineTailCallReg == lr);
    masm.branch(r0);
}


} 
} 

#endif

