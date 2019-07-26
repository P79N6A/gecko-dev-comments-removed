





#ifndef jit_x86_BaselineHelpers_x86_h
#define jit_x86_BaselineHelpers_x86_h

#ifdef JS_ION
#include "jit/BaselineFrame.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineRegisters.h"
#include "jit/IonMacroAssembler.h"

namespace js {
namespace jit {


static const size_t ICStackValueOffset = sizeof(void *);

inline void
EmitRestoreTailCallReg(MacroAssembler &masm)
{
    masm.pop(BaselineTailCallReg);
}

inline void
EmitRepushTailCallReg(MacroAssembler &masm)
{
    masm.push(BaselineTailCallReg);
}

inline void
EmitCallIC(CodeOffsetLabel *patchOffset, MacroAssembler &masm)
{
    
    CodeOffsetLabel offset = masm.movWithPatch(ImmWord(-1), BaselineStubReg);
    *patchOffset = offset;

    
    masm.loadPtr(Address(BaselineStubReg, (int32_t) ICEntry::offsetOfFirstStub()),
                 BaselineStubReg);

    
    
    masm.call(Operand(BaselineStubReg, ICStub::offsetOfStubCode()));
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler &masm,
                       size_t monitorStubOffset = ICMonitoredStub::offsetOfFirstMonitorStub())
{
    
    
    masm.loadPtr(Address(BaselineStubReg, (int32_t) monitorStubOffset), BaselineStubReg);

    
    masm.jmp(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()));
}

inline void
EmitReturnFromIC(MacroAssembler &masm)
{
    masm.ret();
}

inline void
EmitChangeICReturnAddress(MacroAssembler &masm, Register reg)
{
    masm.storePtr(reg, Address(StackPointer, 0));
}

inline void
EmitTailCallVM(IonCode *target, MacroAssembler &masm, uint32_t argSize)
{
    

    
    masm.movl(BaselineFrameReg, eax);
    masm.addl(Imm32(BaselineFrame::FramePointerOffset), eax);
    masm.subl(BaselineStackReg, eax);

    
    masm.movl(eax, ebx);
    masm.subl(Imm32(argSize), ebx);
    masm.store32(ebx, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
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


static const uint32_t STUB_FRAME_SIZE = 4 * sizeof(void *);
static const uint32_t STUB_FRAME_SAVED_STUB_OFFSET = sizeof(void *);

inline void
EmitEnterStubFrame(MacroAssembler &masm, Register scratch)
{
    JS_ASSERT(scratch != BaselineTailCallReg);

    EmitRestoreTailCallReg(masm);

    
    masm.movl(BaselineFrameReg, scratch);
    masm.addl(Imm32(BaselineFrame::FramePointerOffset), scratch);
    masm.subl(BaselineStackReg, scratch);

    masm.store32(scratch, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    
    

    
    masm.makeFrameDescriptor(scratch, IonFrame_BaselineJS);
    masm.push(scratch);
    masm.push(BaselineTailCallReg);

    
    masm.push(BaselineStubReg);
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);
}

inline void
EmitLeaveStubFrame(MacroAssembler &masm, bool calledIntoIon = false)
{
    
    
    
    
    if (calledIntoIon) {
        Register scratch = BaselineTailCallReg;
        masm.pop(scratch);
        masm.shrl(Imm32(FRAMESIZE_SHIFT), scratch);
        masm.addl(scratch, BaselineStackReg);
    } else {
        masm.mov(BaselineFrameReg, BaselineStackReg);
    }

    masm.pop(BaselineFrameReg);
    masm.pop(BaselineStubReg);

    
    masm.pop(BaselineTailCallReg);

    
    
    masm.storePtr(BaselineTailCallReg, Address(BaselineStackReg, 0));
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
EmitUnstowICValues(MacroAssembler &masm, int values, bool discard = false)
{
    JS_ASSERT(values >= 0 && values <= 2);
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
EmitCallTypeUpdateIC(MacroAssembler &masm, IonCode *code, uint32_t objectOffset)
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
EmitPreBarrier(MacroAssembler &masm, const AddrType &addr, MIRType type)
{
    masm.patchableCallPreBarrier(addr, type);
}

inline void
EmitStubGuardFailure(MacroAssembler &masm)
{
    
    

    

    
    masm.loadPtr(Address(BaselineStubReg, (int32_t) ICStub::offsetOfNext()), BaselineStubReg);

    
    masm.jmp(Operand(BaselineStubReg, (int32_t) ICStub::offsetOfStubCode()));
}


} 
} 

#endif 

#endif 
