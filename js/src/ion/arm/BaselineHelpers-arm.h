






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
EmitTailCall(IonCode *target, MacroAssembler &masm, uint32_t argSize)
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

