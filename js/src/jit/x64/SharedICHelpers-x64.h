





#ifndef jit_x64_SharedICHelpers_x64_h
#define jit_x64_SharedICHelpers_x64_h

#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/MacroAssembler.h"
#include "jit/SharedICRegisters.h"

namespace js {
namespace jit {


static const size_t ICStackValueOffset = sizeof(void*);

inline void
EmitRestoreTailCallReg(MacroAssembler& masm)
{
    masm.pop(BaselineTailCallReg);
}

inline void
EmitRepushTailCallReg(MacroAssembler& masm)
{
    masm.push(BaselineTailCallReg);
}

inline void
EmitCallIC(CodeOffsetLabel* patchOffset, MacroAssembler& masm)
{
    
    CodeOffsetLabel offset = masm.movWithPatch(ImmWord(-1), BaselineStubReg);
    *patchOffset = offset;

    
    masm.loadPtr(Address(BaselineStubReg, (int32_t) ICEntry::offsetOfFirstStub()),
                 BaselineStubReg);

    
    masm.call(Operand(BaselineStubReg, ICStub::offsetOfStubCode()));
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler& masm,
                       size_t monitorStubOffset = ICMonitoredStub::offsetOfFirstMonitorStub())
{
    
    
    masm.loadPtr(Address(BaselineStubReg, (int32_t) monitorStubOffset), BaselineStubReg);

    
    masm.jmp(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()));
}

inline void
EmitReturnFromIC(MacroAssembler& masm)
{
    masm.ret();
}

inline void
EmitChangeICReturnAddress(MacroAssembler& masm, Register reg)
{
    masm.storePtr(reg, Address(StackPointer, 0));
}

inline void
EmitTailCallVM(JitCode* target, MacroAssembler& masm, uint32_t argSize)
{
    
    masm.movq(BaselineFrameReg, ScratchReg);
    masm.addq(Imm32(BaselineFrame::FramePointerOffset), ScratchReg);
    masm.subq(BaselineStackReg, ScratchReg);

    
    masm.movq(ScratchReg, rdx);
    masm.subq(Imm32(argSize), rdx);
    masm.store32(rdx, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    masm.makeFrameDescriptor(ScratchReg, JitFrame_BaselineJS);
    masm.push(ScratchReg);
    masm.push(BaselineTailCallReg);
    masm.jmp(target);
}

inline void
EmitCreateStubFrameDescriptor(MacroAssembler& masm, Register reg)
{
    
    
    masm.movq(BaselineFrameReg, reg);
    masm.addq(Imm32(sizeof(void*) * 2), reg);
    masm.subq(BaselineStackReg, reg);

    masm.makeFrameDescriptor(reg, JitFrame_BaselineStub);
}

inline void
EmitCallVM(JitCode* target, MacroAssembler& masm)
{
    EmitCreateStubFrameDescriptor(masm, ScratchReg);
    masm.push(ScratchReg);
    masm.call(target);
}


static const uint32_t STUB_FRAME_SIZE = 4 * sizeof(void*);
static const uint32_t STUB_FRAME_SAVED_STUB_OFFSET = sizeof(void*);

inline void
EmitEnterStubFrame(MacroAssembler& masm, Register)
{
    EmitRestoreTailCallReg(masm);

    
    masm.movq(BaselineFrameReg, ScratchReg);
    masm.addq(Imm32(BaselineFrame::FramePointerOffset), ScratchReg);
    masm.subq(BaselineStackReg, ScratchReg);

    masm.store32(ScratchReg, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    

    
    masm.makeFrameDescriptor(ScratchReg, JitFrame_BaselineJS);
    masm.push(ScratchReg);
    masm.push(BaselineTailCallReg);

    
    masm.push(BaselineStubReg);
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);
}

inline void
EmitLeaveStubFrame(MacroAssembler& masm, bool calledIntoIon = false)
{
    
    
    
    
    if (calledIntoIon) {
        masm.pop(ScratchReg);
        masm.shrq(Imm32(FRAMESIZE_SHIFT), ScratchReg);
        masm.addq(ScratchReg, BaselineStackReg);
    } else {
        masm.mov(BaselineFrameReg, BaselineStackReg);
    }

    masm.pop(BaselineFrameReg);
    masm.pop(BaselineStubReg);

    
    masm.pop(BaselineTailCallReg);

    
    
    masm.storePtr(BaselineTailCallReg, Address(BaselineStackReg, 0));
}

inline void
EmitStowICValues(MacroAssembler& masm, int values)
{
    MOZ_ASSERT(values >= 0 && values <= 2);
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
EmitUnstowICValues(MacroAssembler& masm, int values, bool discard = false)
{
    MOZ_ASSERT(values >= 0 && values <= 2);
    switch(values) {
      case 1:
        
        masm.pop(BaselineTailCallReg);
        if (discard)
            masm.addPtr(Imm32(sizeof(Value)), BaselineStackReg);
        else
            masm.popValue(R0);
        masm.push(BaselineTailCallReg);
        break;
      case 2:
        
        masm.pop(BaselineTailCallReg);
        if (discard) {
            masm.addPtr(Imm32(sizeof(Value) * 2), BaselineStackReg);
        } else {
            masm.popValue(R1);
            masm.popValue(R0);
        }
        masm.push(BaselineTailCallReg);
        break;
    }
}

inline void
EmitCallTypeUpdateIC(MacroAssembler& masm, JitCode* code, uint32_t objectOffset)
{
    
    
    

    
    masm.push(BaselineStubReg);

    
    
    masm.loadPtr(Address(BaselineStubReg, (int32_t) ICUpdatedStub::offsetOfFirstUpdateStub()),
                 BaselineStubReg);

    
    masm.call(Operand(BaselineStubReg, ICStub::offsetOfStubCode()));

    
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
    masm.patchableCallPreBarrier(addr, type);
}

inline void
EmitStubGuardFailure(MacroAssembler& masm)
{
    
    

    

    
    masm.loadPtr(Address(BaselineStubReg, ICStub::offsetOfNext()), BaselineStubReg);

    
    masm.jmp(Operand(BaselineStubReg, ICStub::offsetOfStubCode()));
}

} 
} 

#endif 
