





#ifndef jit_arm_SharedICHelpers_arm_h
#define jit_arm_SharedICHelpers_arm_h

#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/MacroAssembler.h"
#include "jit/SharedICRegisters.h"

namespace js {
namespace jit {


static const size_t ICStackValueOffset = 0;

inline void
EmitRestoreTailCallReg(MacroAssembler& masm)
{
    
}

inline void
EmitRepushTailCallReg(MacroAssembler& masm)
{
    
}

inline void
EmitCallIC(CodeOffsetLabel* patchOffset, MacroAssembler& masm)
{
    
    CodeOffsetLabel offset = masm.movWithPatch(ImmWord(-1), BaselineStubReg);
    *patchOffset = offset;

    
    masm.loadPtr(Address(BaselineStubReg, ICEntry::offsetOfFirstStub()), BaselineStubReg);

    
    
    MOZ_ASSERT(R2 == ValueOperand(r1, r0));
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.ma_blx(r0);
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler& masm,
                       size_t monitorStubOffset = ICMonitoredStub::offsetOfFirstMonitorStub())
{
    
    
    masm.loadPtr(Address(BaselineStubReg, (uint32_t) monitorStubOffset), BaselineStubReg);

    
    
    MOZ_ASSERT(R2 == ValueOperand(r1, r0));
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.branch(r0);
}

inline void
EmitReturnFromIC(MacroAssembler& masm)
{
    masm.ma_mov(lr, pc);
}

inline void
EmitChangeICReturnAddress(MacroAssembler& masm, Register reg)
{
    masm.ma_mov(reg, lr);
}

inline void
EmitTailCallVM(JitCode* target, MacroAssembler& masm, uint32_t argSize)
{
    
    
    MOZ_ASSERT(R2 == ValueOperand(r1, r0));

    
    masm.movePtr(BaselineFrameReg, r0);
    masm.ma_add(Imm32(BaselineFrame::FramePointerOffset), r0);
    masm.ma_sub(BaselineStackReg, r0);

    
    masm.ma_sub(r0, Imm32(argSize), r1);
    masm.store32(r1, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    
    
    
    MOZ_ASSERT(BaselineTailCallReg == lr);
    masm.makeFrameDescriptor(r0, JitFrame_BaselineJS);
    masm.push(r0);
    masm.push(lr);
    masm.branch(target);
}

inline void
EmitCreateStubFrameDescriptor(MacroAssembler& masm, Register reg)
{
    
    
    masm.mov(BaselineFrameReg, reg);
    masm.ma_add(Imm32(sizeof(void*) * 2), reg);
    masm.ma_sub(BaselineStackReg, reg);

    masm.makeFrameDescriptor(reg, JitFrame_BaselineStub);
}

inline void
EmitCallVM(JitCode* target, MacroAssembler& masm)
{
    EmitCreateStubFrameDescriptor(masm, r0);
    masm.push(r0);
    masm.call(target);
}


static const uint32_t STUB_FRAME_SIZE = 4 * sizeof(void*);
static const uint32_t STUB_FRAME_SAVED_STUB_OFFSET = sizeof(void*);

inline void
EmitEnterStubFrame(MacroAssembler& masm, Register scratch)
{
    MOZ_ASSERT(scratch != BaselineTailCallReg);

    
    masm.mov(BaselineFrameReg, scratch);
    masm.ma_add(Imm32(BaselineFrame::FramePointerOffset), scratch);
    masm.ma_sub(BaselineStackReg, scratch);

    masm.store32(scratch, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    

    
    masm.makeFrameDescriptor(scratch, JitFrame_BaselineJS);
    masm.push(scratch);
    masm.push(BaselineTailCallReg);

    
    masm.push(BaselineStubReg);
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);

    
    masm.checkStackAlignment();
}

inline void
EmitLeaveStubFrame(MacroAssembler& masm, bool calledIntoIon = false)
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
EmitStowICValues(MacroAssembler& masm, int values)
{
    MOZ_ASSERT(values >= 0 && values <= 2);
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
EmitUnstowICValues(MacroAssembler& masm, int values, bool discard = false)
{
    MOZ_ASSERT(values >= 0 && values <= 2);
    switch(values) {
      case 1:
        
        if (discard)
            masm.addPtr(Imm32(sizeof(Value)), BaselineStackReg);
        else
            masm.popValue(R0);
        break;
      case 2:
        
        if (discard) {
            masm.addPtr(Imm32(sizeof(Value) * 2), BaselineStackReg);
        } else {
            masm.popValue(R1);
            masm.popValue(R0);
        }
        break;
    }
}

inline void
EmitCallTypeUpdateIC(MacroAssembler& masm, JitCode* code, uint32_t objectOffset)
{
    MOZ_ASSERT(R2 == ValueOperand(r1, r0));

    
    
    

    
    
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

    masm.loadValue(Address(BaselineStackReg, STUB_FRAME_SIZE + objectOffset), R1);

    masm.pushValue(R0);
    masm.pushValue(R1);
    masm.push(BaselineStubReg);

    
    masm.loadPtr(Address(BaselineFrameReg, 0), R0.scratchReg());
    masm.pushBaselineFramePtr(R0.scratchReg(), R0.scratchReg());

    EmitCallVM(code, masm);
    EmitLeaveStubFrame(masm);

    
    masm.bind(&success);
}

template <typename AddrType>
inline void
EmitPreBarrier(MacroAssembler& masm, const AddrType& addr, MIRType type)
{
    
    masm.push(lr);
    masm.patchableCallPreBarrier(addr, type);
    masm.pop(lr);
}

inline void
EmitStubGuardFailure(MacroAssembler& masm)
{
    MOZ_ASSERT(R2 == ValueOperand(r1, r0));

    
    

    

    
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfNext()), BaselineStubReg);

    
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfStubCode()), r0);

    
    MOZ_ASSERT(BaselineTailCallReg == lr);
    masm.branch(r0);
}


} 
} 

#endif 
