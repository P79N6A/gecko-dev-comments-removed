





#ifndef jit_arm64_SharedICHelpers_arm64_h
#define jit_arm64_SharedICHelpers_arm64_h

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

    
    
    MOZ_ASSERT(R2 == ValueOperand(r0));
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.Blr(x0);
}

inline void
EmitEnterTypeMonitorIC(MacroAssembler& masm,
                       size_t monitorStubOffset = ICMonitoredStub::offsetOfFirstMonitorStub())
{
    
    
    masm.loadPtr(Address(ICStubReg, (uint32_t) monitorStubOffset), ICStubReg);

    
    
    MOZ_ASSERT(R2 == ValueOperand(r0));
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.Br(x0);
}

inline void
EmitReturnFromIC(MacroAssembler& masm)
{
    masm.abiret(); 
}

inline void
EmitChangeICReturnAddress(MacroAssembler& masm, Register reg)
{
    masm.movePtr(reg, lr);
}

inline void
EmitTailCallVM(JitCode* target, MacroAssembler& masm, uint32_t argSize)
{
    
    MOZ_ASSERT(R2 == ValueOperand(r0));

    
    masm.Sub(x0, BaselineFrameReg64, masm.GetStackPointer64());
    masm.Add(w0, w0, Operand(BaselineFrame::FramePointerOffset));

    
    {
        vixl::UseScratchRegisterScope temps(&masm.asVIXL());
        const ARMRegister scratch32 = temps.AcquireW();

        masm.Sub(scratch32, w0, Operand(argSize));
        masm.store32(scratch32.asUnsized(),
                     Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));
    }

    
    MOZ_ASSERT(ICTailCallReg == lr);
    masm.makeFrameDescriptor(r0, JitFrame_BaselineJS);
    masm.push(r0);

    
    
    
    

    masm.branch(target);
}

inline void
EmitCreateStubFrameDescriptor(MacroAssembler& masm, Register reg)
{
    ARMRegister reg64(reg, 64);

    
    masm.Sub(reg64, masm.GetStackPointer64(), Operand(sizeof(void*) * 2));
    masm.Sub(reg64, BaselineFrameReg64, reg64);

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
    MOZ_ASSERT(scratch != ICTailCallReg);

    
    masm.Add(ARMRegister(scratch, 64), BaselineFrameReg64, Operand(BaselineFrame::FramePointerOffset));
    masm.Sub(ARMRegister(scratch, 64), ARMRegister(scratch, 64), masm.GetStackPointer64());

    masm.store32(scratch, Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    

    
    
    masm.makeFrameDescriptor(scratch, JitFrame_BaselineJS);
    masm.push(scratch, ICTailCallReg, ICStubReg, BaselineFrameReg);

    
    masm.Mov(BaselineFrameReg64, masm.GetStackPointer64());

    
    masm.checkStackAlignment();
}

inline void
EmitLeaveStubFrame(MacroAssembler& masm, bool calledIntoIon = false)
{
    vixl::UseScratchRegisterScope temps(&masm.asVIXL());
    const ARMRegister scratch64 = temps.AcquireX();

    
    
    
    
    if (calledIntoIon) {
        masm.pop(scratch64.asUnsized());
        masm.Lsr(scratch64, scratch64, FRAMESIZE_SHIFT);
        masm.Add(masm.GetStackPointer64(), masm.GetStackPointer64(), scratch64);
    } else {
        masm.Mov(masm.GetStackPointer64(), BaselineFrameReg64);
    }

    
    masm.pop(BaselineFrameReg, ICStubReg, ICTailCallReg, scratch64.asUnsized());

    
    masm.checkStackAlignment();
}

inline void
EmitStowICValues(MacroAssembler& masm, int values)
{
    switch (values) {
      case 1:
        
        masm.pushValue(R0);
        break;
      case 2:
        
        masm.push(R0.valueReg(), R1.valueReg());
        break;
      default:
        MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("Expected 1 or 2 values");
    }
}

inline void
EmitUnstowICValues(MacroAssembler& masm, int values, bool discard = false)
{
    MOZ_ASSERT(values >= 0 && values <= 2);
    switch (values) {
      case 1:
        
        if (discard)
            masm.Drop(Operand(sizeof(Value)));
        else
            masm.popValue(R0);
        break;
      case 2:
        
        if (discard)
            masm.Drop(Operand(sizeof(Value) * 2));
        else
            masm.pop(R1.valueReg(), R0.valueReg());
        break;
      default:
        MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("Expected 1 or 2 values");
    }
}

inline void
EmitCallTypeUpdateIC(MacroAssembler& masm, JitCode* code, uint32_t objectOffset)
{
    
    
    
    MOZ_ASSERT(R2 == ValueOperand(r0));

    
    
    masm.push(ICStubReg, ICTailCallReg);

    
    
    masm.loadPtr(Address(ICStubReg, (int32_t)ICUpdatedStub::offsetOfFirstUpdateStub()),
                 ICStubReg);

    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), ICTailCallReg);

    
    masm.Blr(ARMRegister(ICTailCallReg, 64));

    
    masm.pop(ICTailCallReg, ICStubReg);

    
    
    Label success;
    masm.cmp32(R1.scratchReg(), Imm32(1));
    masm.j(Assembler::Equal, &success);

    
    EmitEnterStubFrame(masm, R1.scratchReg());

    masm.loadValue(Address(masm.getStackPointer(), STUB_FRAME_SIZE + objectOffset), R1);
    masm.push(R0.valueReg(), R1.valueReg(), ICStubReg);

    
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
    
    

    

    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfNext()), ICStubReg);

    
    masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), r0);

    
    masm.Br(x0);
}

} 
} 

#endif 
