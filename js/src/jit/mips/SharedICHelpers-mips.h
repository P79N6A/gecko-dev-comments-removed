





#ifndef jit_mips_SharedICHelpers_mips_h
#define jit_mips_SharedICHelpers_mips_h

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
    
    CodeOffsetLabel offset = masm.movWithPatch(ImmWord(-1), ICStubReg);
    *patchOffset = offset;

    
    masm.loadPtr(Address(ICStubReg, ICEntry::offsetOfFirstStub()), ICStubReg);

    
    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), R2.scratchReg());

    
    masm.call(R2.scratchReg());
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler& masm,
                       size_t monitorStubOffset = ICMonitoredStub::offsetOfFirstMonitorStub())
{
    
    
    masm.loadPtr(Address(ICStubReg, (uint32_t) monitorStubOffset), ICStubReg);

    
    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), R2.scratchReg());

    
    masm.branch(R2.scratchReg());
}

inline void
EmitReturnFromIC(MacroAssembler& masm)
{
    masm.branch(ra);
}

inline void
EmitChangeICReturnAddress(MacroAssembler& masm, Register reg)
{
    masm.movePtr(reg, ra);
}

inline void
EmitTailCallVM(JitCode* target, MacroAssembler& masm, uint32_t argSize)
{
    
    
    MOZ_ASSERT(R2 == ValueOperand(t7, t6));

    
    masm.movePtr(BaselineFrameReg, t6);
    masm.addPtr(Imm32(BaselineFrame::FramePointerOffset), t6);
    masm.subPtr(BaselineStackReg, t6);

    
    masm.ma_subu(t7, t6, Imm32(argSize));
    masm.storePtr(t7, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    
    
    
    MOZ_ASSERT(ICTailCallReg == ra);
    masm.makeFrameDescriptor(t6, JitFrame_BaselineJS);
    masm.subPtr(Imm32(sizeof(CommonFrameLayout)), StackPointer);
    masm.storePtr(t6, Address(StackPointer, CommonFrameLayout::offsetOfDescriptor()));
    masm.storePtr(ra, Address(StackPointer, CommonFrameLayout::offsetOfReturnAddress()));

    masm.branch(target);
}

inline void
EmitCreateStubFrameDescriptor(MacroAssembler& masm, Register reg)
{
    
    
    masm.movePtr(BaselineFrameReg, reg);
    masm.addPtr(Imm32(sizeof(intptr_t) * 2), reg);
    masm.subPtr(BaselineStackReg, reg);

    masm.makeFrameDescriptor(reg, JitFrame_BaselineStub);
}

inline void
EmitCallVM(JitCode* target, MacroAssembler& masm)
{
    EmitCreateStubFrameDescriptor(masm, t6);
    masm.push(t6);
    masm.call(target);
}

struct BaselineStubFrame {
    uintptr_t savedFrame;
    uintptr_t savedStub;
    uintptr_t returnAddress;
    uintptr_t descriptor;
};

static const uint32_t STUB_FRAME_SIZE = sizeof(BaselineStubFrame);
static const uint32_t STUB_FRAME_SAVED_STUB_OFFSET = offsetof(BaselineStubFrame, savedStub);

inline void
EmitEnterStubFrame(MacroAssembler& masm, Register scratch)
{
    MOZ_ASSERT(scratch != ICTailCallReg);

    
    masm.movePtr(BaselineFrameReg, scratch);
    masm.addPtr(Imm32(BaselineFrame::FramePointerOffset), scratch);
    masm.subPtr(BaselineStackReg, scratch);

    masm.storePtr(scratch, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    

    
    masm.makeFrameDescriptor(scratch, JitFrame_BaselineJS);
    masm.subPtr(Imm32(STUB_FRAME_SIZE), StackPointer);
    masm.storePtr(scratch, Address(StackPointer, offsetof(BaselineStubFrame, descriptor)));
    masm.storePtr(ICTailCallReg, Address(StackPointer,
                                               offsetof(BaselineStubFrame, returnAddress)));

    
    masm.storePtr(ICStubReg, Address(StackPointer,
                                           offsetof(BaselineStubFrame, savedStub)));
    masm.storePtr(BaselineFrameReg, Address(StackPointer,
                                            offsetof(BaselineStubFrame, savedFrame)));
    masm.movePtr(BaselineStackReg, BaselineFrameReg);

    
    masm.checkStackAlignment();
}

inline void
EmitLeaveStubFrame(MacroAssembler& masm, bool calledIntoIon = false)
{
    
    
    
    
    if (calledIntoIon) {
        masm.pop(ScratchRegister);
        masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), ScratchRegister);
        masm.addPtr(ScratchRegister, BaselineStackReg);
    } else {
        masm.movePtr(BaselineFrameReg, BaselineStackReg);
    }

    masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, savedFrame)),
                 BaselineFrameReg);
    masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, savedStub)),
                 ICStubReg);

    
    masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, returnAddress)),
                 ICTailCallReg);

    
    masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, descriptor)), ScratchRegister);
    masm.addPtr(Imm32(STUB_FRAME_SIZE), StackPointer);
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
    
    
    

    
    
    masm.subPtr(Imm32(2 * sizeof(intptr_t)), StackPointer);
    masm.storePtr(ICStubReg, Address(StackPointer, sizeof(intptr_t)));
    masm.storePtr(ICTailCallReg, Address(StackPointer, 0));

    
    
    masm.loadPtr(Address(ICStubReg, ICUpdatedStub::offsetOfFirstUpdateStub()),
                 ICStubReg);

    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), R2.scratchReg());

    
    masm.call(R2.scratchReg());

    
    masm.loadPtr(Address(StackPointer, 0), ICTailCallReg);
    masm.loadPtr(Address(StackPointer, sizeof(intptr_t)), ICStubReg);
    masm.addPtr(Imm32(2 * sizeof(intptr_t)), StackPointer);

    
    
    Label success;
    masm.ma_b(R1.scratchReg(), Imm32(1), &success, Assembler::Equal, ShortJump);

    
    EmitEnterStubFrame(masm, R1.scratchReg());

    masm.loadValue(Address(BaselineStackReg, STUB_FRAME_SIZE + objectOffset), R1);

    masm.pushValue(R0);
    masm.pushValue(R1);
    masm.push(ICStubReg);

    
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
    
    masm.push(ra);
    masm.patchableCallPreBarrier(addr, type);
    masm.pop(ra);
}

inline void
EmitStubGuardFailure(MacroAssembler& masm)
{
    
    

    

    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfNext()), ICStubReg);

    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), R2.scratchReg());

    
    MOZ_ASSERT(ICTailCallReg == ra);
    masm.branch(R2.scratchReg());
}


} 
} 

#endif 
